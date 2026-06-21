#pragma once
#include "peer_connection.h"
#include <functional>
#include <thread>
#include <atomic>
#include <memory>

class FlyServer {
public:
    using ConnCallback = std::function<void(std::shared_ptr<PeerConnection>)>;

    explicit FlyServer(int port);
    ~FlyServer();

    void start(ConnCallback on_connection);
    void stop();

private:
    int               port_;
    int               server_fd_{-1};
    std::jthread      accept_thread_;
};
