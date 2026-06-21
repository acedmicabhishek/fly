#pragma once
#include "peer_connection.h"
#include <memory>
#include <string>

class FlyClient {
public:
    static std::shared_ptr<PeerConnection> connect(const std::string& host, int port);
};
