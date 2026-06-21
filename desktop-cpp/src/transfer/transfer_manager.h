#pragma once
#include "transfer_item.h"
#include "../network/peer_connection.h"
#include <functional>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <string>

class TransferManager {
public:
    using ChangedCb = std::function<void(std::vector<TransferItem>)>;

    TransferManager() = default;

    void set_on_changed(ChangedCb cb)      { on_changed_ = std::move(cb); }
    void set_download_dir(std::string d)   { download_dir_ = std::move(d); }
    void set_chunk_threshold(int64_t b)    { chunk_threshold_ = b; }
    void set_chunk_size(int b)             { chunk_size_ = b; }
    void set_device_name(std::string n)    { own_name_ = std::move(n); }

    using HelloCb = std::function<void(const std::string& name, const std::string& platform)>;

    void add_connection(std::shared_ptr<PeerConnection> conn,
                        const std::string& device_name,
                        PeerConnection::DiscCallback on_disconnect = nullptr,
                        HelloCb on_hello = nullptr);

    void send_text(std::shared_ptr<PeerConnection> conn, const std::string& text);
    void send_file(std::shared_ptr<PeerConnection> conn, const std::string& path);

    std::vector<TransferItem> snapshot() const;

private:
    void handle_msg(fly::Message msg, std::vector<uint8_t> body,
                    std::string device_name, std::shared_ptr<PeerConnection> conn,
                    HelloCb on_hello);
    void prepend(TransferItem item);
    void update(const std::string& id, std::function<void(TransferItem&)> f);
    void notify();
    std::string save_file(const std::string& name, const uint8_t* data, size_t len);

    std::string own_name_       = "Desktop";
    std::string download_dir_;
    int64_t     chunk_threshold_{64LL * 1024 * 1024};
    int         chunk_size_     {4 * 1024 * 1024};

    mutable std::mutex            mtx_;
    std::vector<TransferItem>     items_;
    std::map<std::string, std::vector<std::vector<uint8_t>>> pending_chunks_;
    std::map<std::string, fly::MsgFileStart>                 pending_infos_;

    ChangedCb on_changed_;
};
