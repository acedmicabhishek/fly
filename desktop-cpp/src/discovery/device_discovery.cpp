#include "device_discovery.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <map>
#include <chrono>
#include <string>
#include <cstring>

using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

static constexpr int  DISC_PORT    = 5801;
static constexpr int  ANNOUNCE_S   = 2;
static constexpr long TIMEOUT_S    = 8;

struct DeviceDiscovery::Impl {
    int         port;
    std::string device_name;
    ChangedCb   on_changed;
    std::jthread sender_thread;
    std::jthread recv_thread;
    std::mutex   mtx;
    std::map<std::string, NetworkDevice>      devices;
    std::map<std::string, Clock::time_point>  last_seen;

    void notify() {
        std::map<std::string, NetworkDevice> snap;
        { std::lock_guard<std::mutex> lk(mtx); snap = devices; }
        if (on_changed) on_changed(snap);
    }
};

DeviceDiscovery::DeviceDiscovery(int port) : port_(port), impl_(new Impl{}) {
    impl_->port = port;
}

DeviceDiscovery::~DeviceDiscovery() { stop(); delete impl_; }

void DeviceDiscovery::start(const std::string& name, ChangedCb on_changed) {
    impl_->device_name = name;
    impl_->on_changed  = on_changed;
    impl_->sender_thread = std::jthread([this](std::stop_token st) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return;
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));

        sockaddr_in dest{};
        dest.sin_family      = AF_INET;
        dest.sin_port        = htons(DISC_PORT);
        dest.sin_addr.s_addr = inet_addr("255.255.255.255");

        json pkt = {{"v",1},{"name",impl_->device_name},{"port",impl_->port},{"platform","linux"}};
        std::string msg = pkt.dump();

        while (!st.stop_requested()) {
            sendto(sock, msg.c_str(), msg.size(), 0,
                   reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
            for (int i = 0; i < ANNOUNCE_S * 10 && !st.stop_requested(); ++i)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        ::close(sock);
    });

    impl_->recv_thread = std::jthread([this](std::stop_token st) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return;
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
        timeval tv{1, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in bind_addr{};
        bind_addr.sin_family      = AF_INET;
        bind_addr.sin_port        = htons(DISC_PORT);
        bind_addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr));

        char buf[2048];

        while (!st.stop_requested()) {
            auto now = Clock::now();
            bool changed = false;

            sockaddr_in peer{};
            socklen_t peer_len = sizeof(peer);
            ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                                 reinterpret_cast<sockaddr*>(&peer), &peer_len);

            if (n > 0) {
                buf[n] = '\0';
                try {
                    auto j = json::parse(buf, buf + n);
                    if (j.value("v", 0) == 1) {
                        std::string pname    = j.value("name", "");
                        int         pport    = j.value("port", 0);
                        std::string platform = j.value("platform", "unknown");
                        if (!pname.empty() && pname != impl_->device_name && pport > 0) {
                            char host[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &peer.sin_addr, host, sizeof(host));
                            NetworkDevice dev{pname, pname, platform, host, pport};
                            {
                                std::lock_guard lk(impl_->mtx);
                                bool is_new = !impl_->devices.count(pname);
                                impl_->devices[pname]   = dev;
                                impl_->last_seen[pname] = now;
                                changed = is_new;
                            }
                        }
                    }
                } catch (...) {}
            }

            {
                std::lock_guard lk(impl_->mtx);
                for (auto it = impl_->last_seen.begin(); it != impl_->last_seen.end(); ) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                       now - it->second).count();
                    if (elapsed >= TIMEOUT_S) {
                        impl_->devices.erase(it->first);
                        it = impl_->last_seen.erase(it);
                        changed = true;
                    } else {
                        ++it;
                    }
                }
            }

            if (changed) impl_->notify();
        }
        ::close(sock);
    });
}

void DeviceDiscovery::stop() {
    impl_->sender_thread.request_stop();
    impl_->recv_thread.request_stop();
}
