#include "fly_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

FlyServer::FlyServer(int port) : port_(port) {}
FlyServer::~FlyServer() { stop(); }

void FlyServer::start(ConnCallback on_connection) {
    server_fd_ = ::socket(AF_INET6, SOCK_STREAM, 0);
    bool use_ipv6 = (server_fd_ >= 0);

    if (!use_ipv6)
        server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bool bound = false;
    if (use_ipv6) {
        int v6only = 0;
        ::setsockopt(server_fd_, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only));
        sockaddr_in6 addr{};
        addr.sin6_family = AF_INET6;
        addr.sin6_port   = htons(port_);
        addr.sin6_addr   = in6addr_any;
        bound = (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    }
    if (!bound) {
        if (use_ipv6) { ::close(server_fd_); server_fd_ = ::socket(AF_INET, SOCK_STREAM, 0); }
        ::setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_port        = htons(port_);
        addr.sin_addr.s_addr = INADDR_ANY;
        ::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    }

    ::listen(server_fd_, 16);

    accept_thread_ = std::jthread([this, on_connection](std::stop_token st) {
        while (!st.stop_requested()) {
            int cfd = ::accept(server_fd_, nullptr, nullptr);
            if (cfd < 0) break;
            if (on_connection) on_connection(std::make_shared<PeerConnection>(cfd));
        }
    });
}

void FlyServer::stop() {
    accept_thread_.request_stop();
    if (server_fd_ >= 0) { ::close(server_fd_); server_fd_ = -1; }
}
