#include "app.h"
#include "network/fly_client.h"
#include <glib.h>
#include <glib/gstdio.h>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

static constexpr const char* CFG_GROUP = "fly";
static std::string cfg_path() {
    return std::string(g_get_user_config_dir()) + "/fly/fly.conf";
}

static AppSettings load_settings() {
    AppSettings s;
    GKeyFile* kf = g_key_file_new();
    std::string path = cfg_path();
    if (g_key_file_load_from_file(kf, path.c_str(), G_KEY_FILE_NONE, nullptr)) {
        auto str = [&](const char* k, const char* def) -> std::string {
            char* v = g_key_file_get_string(kf, CFG_GROUP, k, nullptr);
            std::string r = v ? v : def; g_free(v); return r;
        };
        auto intv = [&](const char* k, int def) -> int {
            GError* e = nullptr;
            int v = g_key_file_get_integer(kf, CFG_GROUP, k, &e);
            if (e) { g_error_free(e); return def; } return v;
        };
        auto boolv = [&](const char* k, bool def) -> bool {
            GError* e = nullptr;
            bool v = g_key_file_get_boolean(kf, CFG_GROUP, k, &e);
            if (e) { g_error_free(e); return def; } return v;
        };
        s.device_name        = str("device_name", s.device_name.c_str());
        s.port               = intv("port",              s.port);
        s.download_dir       = str("download_dir",       "");
        s.chunk_threshold_mb = intv("chunk_threshold_mb",s.chunk_threshold_mb);
        s.chunk_size_mb      = intv("chunk_size_mb",     s.chunk_size_mb);
        s.theme_mode         = str("theme_mode",         s.theme_mode.c_str());
        s.auto_connect       = boolv("auto_connect",     s.auto_connect);
    }
    g_key_file_free(kf);
    return s;
}

static void save_settings_to_disk(const AppSettings& s) {
    std::string path = cfg_path();
    fs::create_directories(fs::path(path).parent_path());
    GKeyFile* kf = g_key_file_new();
    g_key_file_set_string(kf,  CFG_GROUP, "device_name",        s.device_name.c_str());
    g_key_file_set_integer(kf, CFG_GROUP, "port",               s.port);
    g_key_file_set_string(kf,  CFG_GROUP, "download_dir",       s.download_dir.c_str());
    g_key_file_set_integer(kf, CFG_GROUP, "chunk_threshold_mb", s.chunk_threshold_mb);
    g_key_file_set_integer(kf, CFG_GROUP, "chunk_size_mb",      s.chunk_size_mb);
    g_key_file_set_string(kf,  CFG_GROUP, "theme_mode",         s.theme_mode.c_str());
    g_key_file_set_boolean(kf, CFG_GROUP, "auto_connect",       s.auto_connect);
    gsize len; char* data = g_key_file_to_data(kf, &len, nullptr);
    g_file_set_contents(path.c_str(), data, len, nullptr);
    g_free(data);
    g_key_file_free(kf);
}

static void dispatch(std::function<void()> fn) {
    auto* f = new std::function<void()>(std::move(fn));
    g_idle_add([](gpointer data) -> gboolean {
        auto* fn = static_cast<std::function<void()>*>(data);
        (*fn)(); delete fn; return G_SOURCE_REMOVE;
    }, f);
}

App::App()
    : settings_(load_settings()),
      server_(settings_.port),
      discovery_(settings_.port) {}

App::~App() {
    discovery_.stop();
    server_.stop();
}

void App::start() {
    tm_.set_device_name(settings_.device_name);
    tm_.set_download_dir(settings_.download_dir);
    tm_.set_chunk_threshold(settings_.chunk_threshold_mb * 1024LL * 1024);
    tm_.set_chunk_size(settings_.chunk_size_mb * 1024 * 1024);

    tm_.set_on_changed([this](std::vector<TransferItem> items) {
        dispatch([this, items = std::move(items)]() {
            if (on_transfers_changed) on_transfers_changed(items);
        });
    });

    server_.start([this](std::shared_ptr<PeerConnection> conn) {
        dispatch([this, conn]() { on_connected(conn, "incoming device"); });
    });

    discovery_.start(settings_.device_name,
        [this](std::map<std::string, NetworkDevice> devs) {
            devs.erase(settings_.device_name);
            dispatch([this, devs = std::move(devs)]() {
                if (on_devices_changed) on_devices_changed(devs);
            });
        });

    dispatch([this]() {
        if (on_status_changed)
            on_status_changed("Listening on port " + std::to_string(settings_.port));
    });
}

void App::connect_to(const NetworkDevice& dev) {
    dispatch([this]() {
        if (on_status_changed) on_status_changed("Connecting...");
    });
    std::thread([this, dev]() {
        auto conn = FlyClient::connect(dev.host, dev.port);
        dispatch([this, conn, name = dev.name]() {
            if (!conn) { if (on_status_changed) on_status_changed("Connection failed"); return; }
            conn->send(fly::MsgHello{settings_.device_name});
            on_connected(conn, name);
        });
    }).detach();
}

void App::connect_manual(const std::string& host, int port) {
    dispatch([this]() {
        if (on_status_changed) on_status_changed("Connecting...");
    });
    std::thread([this, host, port]() {
        auto conn = FlyClient::connect(host, port);
        dispatch([this, conn, label = host + ":" + std::to_string(port)]() {
            if (!conn) { if (on_status_changed) on_status_changed("Connection failed"); return; }
            conn->send(fly::MsgHello{settings_.device_name});
            on_connected(conn, label);
        });
    }).detach();
}

void App::on_connected(std::shared_ptr<PeerConnection> conn, const std::string& name) {
    active_conn_ = conn;
    conn->peer_name = name;

    auto weak = std::weak_ptr<PeerConnection>(conn);

    auto hello_cb = [this, conn](const std::string& hello_name, const std::string&) {
        conn->peer_name = hello_name;
        dispatch([this, conn, hello_name]() {
            if (active_conn_ == conn) {
                if (on_connection_changed) on_connection_changed(conn);
                if (on_status_changed)     on_status_changed("Connected to " + hello_name);
            }
        });
    };

    tm_.add_connection(conn, name,
        [this, weak]() {
            dispatch([this, weak]() {
                if (active_conn_ == weak.lock()) {
                    active_conn_ = nullptr;
                    if (on_connection_changed) on_connection_changed(nullptr);
                    if (on_status_changed)
                        on_status_changed("Disconnected — listening on port " +
                                          std::to_string(settings_.port));
                }
            });
        },
        hello_cb);

    if (on_connection_changed) on_connection_changed(conn);
    if (on_status_changed)     on_status_changed("Connected to " + name);
}

void App::disconnect() {
    if (active_conn_) { active_conn_->close(); active_conn_ = nullptr; }
    if (on_connection_changed) on_connection_changed(nullptr);
    if (on_status_changed)
        on_status_changed("Disconnected — listening on port " + std::to_string(settings_.port));
}

void App::send_text(const std::string& text) {
    if (!active_conn_) return;
    std::thread([this, conn = active_conn_, text]() {
        tm_.send_text(conn, text);
    }).detach();
}

void App::send_file(const std::string& path) {
    if (!active_conn_) return;
    std::thread([this, conn = active_conn_, path]() {
        tm_.send_file(conn, path);
    }).detach();
}

void App::save_settings(AppSettings s) {
    settings_ = std::move(s);
    tm_.set_device_name(settings_.device_name);
    tm_.set_download_dir(settings_.download_dir);
    tm_.set_chunk_threshold(settings_.chunk_threshold_mb * 1024LL * 1024);
    tm_.set_chunk_size(settings_.chunk_size_mb * 1024 * 1024);
    save_settings_to_disk(settings_);
}
