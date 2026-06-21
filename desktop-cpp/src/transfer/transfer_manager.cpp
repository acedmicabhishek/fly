#include "transfer_manager.h"
#include <glib.h>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <stdexcept>

namespace fs = std::filesystem;

static std::string uuid() {
    char* s = g_uuid_string_random();
    std::string r{s};
    g_free(s);
    return r;
}

void TransferManager::add_connection(std::shared_ptr<PeerConnection> conn,
                                     const std::string& device_name,
                                     PeerConnection::DiscCallback on_disconnect,
                                     HelloCb on_hello) {
    conn->start(
        [this, device_name, conn, on_hello](fly::Message msg, std::vector<uint8_t> body) {
            handle_msg(std::move(msg), std::move(body), device_name, conn, on_hello);
        },
        std::move(on_disconnect));
}

void TransferManager::handle_msg(fly::Message msg, std::vector<uint8_t> body,
                                  std::string dname, std::shared_ptr<PeerConnection> conn,
                                  HelloCb on_hello) {
    std::visit([&](auto&& m) {
        using T = std::decay_t<decltype(m)>;

        if constexpr (std::is_same_v<T, fly::MsgHello>) {
            conn->peer_name = m.name;
            if (on_hello) on_hello(m.name, m.platform);
        } else if constexpr (std::is_same_v<T, fly::MsgText>) {
            TransferItem it;
            it.id = m.id; it.device_name = dname;
            it.dir = TransferDir::In; it.kind = TransferItem::Kind::Text;
            it.content = m.content;
            prepend(std::move(it));

        } else if constexpr (std::is_same_v<T, fly::MsgFileOffer>) {
            std::string path = save_file(m.name, body.data(), body.size());
            TransferItem it;
            it.id = m.id; it.device_name = dname;
            it.dir = TransferDir::In; it.kind = TransferItem::Kind::File;
            it.file_name = m.name; it.mime = m.mime; it.file_size = m.size;
            it.local_path = path; it.progress = 1.f;
            prepend(std::move(it));
            conn->send(fly::MsgAck{m.id});

        } else if constexpr (std::is_same_v<T, fly::MsgFileStart>) {
            { std::lock_guard lk(mtx_); pending_infos_[m.id] = m; pending_chunks_[m.id] = {}; }
            TransferItem it;
            it.id = m.id; it.device_name = dname;
            it.dir = TransferDir::In; it.kind = TransferItem::Kind::File;
            it.file_name = m.name; it.mime = m.mime; it.file_size = m.size;
            it.total_chunks = m.total_chunks; it.progress = 0.f;
            prepend(std::move(it));

        } else if constexpr (std::is_same_v<T, fly::MsgChunk>) {
            int recv = 0; int total = 1;
            {
                std::lock_guard lk(mtx_);
                pending_chunks_[m.id].push_back(body);
                recv  = static_cast<int>(pending_chunks_[m.id].size());
                auto it = pending_infos_.find(m.id);
                if (it != pending_infos_.end()) total = it->second.total_chunks;
            }
            update(m.id, [&](TransferItem& it) {
                it.recv_chunks = recv;
                it.progress    = static_cast<float>(recv) / total;
            });

        } else if constexpr (std::is_same_v<T, fly::MsgFileDone>) {
            std::vector<uint8_t> full;
            fly::MsgFileStart info;
            {
                std::lock_guard lk(mtx_);
                auto ci = pending_chunks_.find(m.id);
                auto ii = pending_infos_.find(m.id);
                if (ci == pending_chunks_.end() || ii == pending_infos_.end()) return;
                info = ii->second;
                for (auto& chunk : ci->second)
                    full.insert(full.end(), chunk.begin(), chunk.end());
                pending_chunks_.erase(ci);
                pending_infos_.erase(ii);
            }
            std::string path = save_file(info.name, full.data(), full.size());
            update(m.id, [&](TransferItem& it) {
                it.local_path = path; it.progress = 1.f;
            });
            conn->send(fly::MsgAck{m.id});

        } else if constexpr (std::is_same_v<T, fly::MsgLogcat>) {
            bool found = false;
            {
                std::lock_guard lk(mtx_);
                for (auto& it : items_) {
                    if (it.id == m.id && it.kind == TransferItem::Kind::Logcat) {
                        it.log_lines.push_back(m.line);
                        found = true; break;
                    }
                }
            }
            if (!found) {
                TransferItem it;
                it.id = m.id; it.device_name = dname;
                it.dir = TransferDir::In; it.kind = TransferItem::Kind::Logcat;
                it.log_lines = {m.line};
                prepend(std::move(it));
            } else {
                notify();
            }
        }
    }, msg);
}

void TransferManager::send_text(std::shared_ptr<PeerConnection> conn, const std::string& text) {
    std::string id = uuid();
    conn->send(fly::MsgText{id, text});
    TransferItem it;
    it.id = id; it.device_name = own_name_;
    it.dir = TransferDir::Out; it.kind = TransferItem::Kind::Text;
    it.content = text;
    prepend(std::move(it));
}

void TransferManager::send_file(std::shared_ptr<PeerConnection> conn, const std::string& path) {
    std::string id = uuid();
    fs::path p{path};
    std::string name = p.filename().string();

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return;
    int64_t size = f.tellg(); f.seekg(0);

    std::string mime = "application/octet-stream";

    if (size <= chunk_threshold_) {
        std::vector<uint8_t> data(size);
        f.read(reinterpret_cast<char*>(data.data()), size);
        conn->send(fly::MsgFileOffer{id, name, mime, size}, data);
        TransferItem it;
        it.id = id; it.device_name = own_name_;
        it.dir = TransferDir::Out; it.kind = TransferItem::Kind::File;
        it.file_name = name; it.mime = mime; it.file_size = size;
        it.local_path = path; it.progress = 1.f;
        prepend(std::move(it));
    } else {
        int total = static_cast<int>(std::ceil(static_cast<double>(size) / chunk_size_));
        conn->send(fly::MsgFileStart{id, name, mime, size, total, chunk_size_});
        {
            TransferItem it;
            it.id = id; it.device_name = own_name_;
            it.dir = TransferDir::Out; it.kind = TransferItem::Kind::File;
            it.file_name = name; it.mime = mime; it.file_size = size;
            it.total_chunks = total; it.progress = 0.f;
            prepend(std::move(it));
        }
        std::vector<uint8_t> buf(chunk_size_);
        int idx = 0;
        while (f) {
            f.read(reinterpret_cast<char*>(buf.data()), chunk_size_);
            auto got = static_cast<size_t>(f.gcount());
            if (got == 0) break;
            std::vector<uint8_t> chunk(buf.begin(), buf.begin() + got);
            conn->send(fly::MsgChunk{id, idx}, chunk);
            int sent = ++idx;
            update(id, [&](TransferItem& it) {
                it.recv_chunks = sent;
                it.progress    = static_cast<float>(sent) / total;
            });
        }
        conn->send(fly::MsgFileDone{id});
        update(id, [](TransferItem& it) { it.progress = 1.f; });
    }
}

std::vector<TransferItem> TransferManager::snapshot() const {
    std::lock_guard lk(mtx_);
    return items_;
}

void TransferManager::prepend(TransferItem item) {
    { std::lock_guard lk(mtx_); items_.insert(items_.begin(), std::move(item)); }
    notify();
}

void TransferManager::update(const std::string& id, std::function<void(TransferItem&)> f) {
    { std::lock_guard lk(mtx_); for (auto& it : items_) if (it.id == id) { f(it); break; } }
    notify();
}

void TransferManager::notify() {
    auto snap = snapshot();
    if (on_changed_) on_changed_(std::move(snap));
}

std::string TransferManager::save_file(const std::string& name,
                                        const uint8_t* data, size_t len) {
    std::string dir = download_dir_.empty()
        ? (std::string(g_get_home_dir()) + "/Downloads/fly")
        : download_dir_;
    fs::create_directories(dir);
    std::string path = dir + "/" + name;
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data), len);
    return path;
}
