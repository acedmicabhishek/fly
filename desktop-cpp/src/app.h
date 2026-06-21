#pragma once
#include "discovery/device_discovery.h"
#include "network/fly_server.h"
#include "network/fly_client.h"
#include "network/peer_connection.h"
#include "transfer/transfer_manager.h"
#include <functional>
#include <memory>
#include <string>
#include <map>

struct AppSettings {
    std::string device_name   = "Desktop";
    int         port          = 5800;
    std::string download_dir;
    int         chunk_threshold_mb = 64;
    int         chunk_size_mb      = 4;
    std::string theme_mode         = "system";
    bool        auto_connect       = false;
};

class App {
public:
    App();
    ~App();

    void start();

    std::function<void(std::map<std::string, NetworkDevice>)> on_devices_changed;
    std::function<void(std::shared_ptr<PeerConnection>)>      on_connection_changed;
    std::function<void(std::string)>                          on_status_changed;
    std::function<void(std::vector<TransferItem>)>            on_transfers_changed;

    void connect_to(const NetworkDevice& dev);
    void connect_manual(const std::string& host, int port);
    void disconnect();
    void send_text(const std::string& text);
    void send_file(const std::string& path);

    std::shared_ptr<PeerConnection> active_conn() const { return active_conn_; }
    const AppSettings& settings() const { return settings_; }
    void save_settings(AppSettings s);

    TransferManager& transfer_manager() { return tm_; }

private:
    void on_connected(std::shared_ptr<PeerConnection> conn, const std::string& name);
    void ui(std::function<void()> fn);

    AppSettings     settings_;
    FlyServer       server_;
    DeviceDiscovery discovery_;
    TransferManager tm_;

    std::shared_ptr<PeerConnection> active_conn_;
};
