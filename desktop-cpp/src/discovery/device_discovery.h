#pragma once
#include <string>
#include <functional>
#include <map>
#include <mutex>

struct NetworkDevice {
    std::string id;
    std::string name;
    std::string platform;
    std::string host;
    int         port;
};

class DeviceDiscovery {
public:
    using ChangedCb = std::function<void(std::map<std::string, NetworkDevice>)>;

    explicit DeviceDiscovery(int port);
    ~DeviceDiscovery();

    void start(const std::string& device_name, ChangedCb on_changed);
    void stop();

    struct Impl;
private:
    int   port_;
    Impl* impl_{nullptr};
};
