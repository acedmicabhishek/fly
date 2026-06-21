#pragma once
#include "../protocol/fly_message.h"
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <memory>

class PeerConnection {
public:
    using MsgCallback  = std::function<void(fly::Message, std::vector<uint8_t>)>;
    using DiscCallback = std::function<void()>;

    explicit PeerConnection(int fd);
    ~PeerConnection();

    void start(MsgCallback on_msg, DiscCallback on_disconnect);
    bool send(const fly::Message& msg, const std::vector<uint8_t>& body = {});
    void close();

    bool is_connected() const { return !closed_; }

    std::string peer_name;

private:
    int                    fd_;
    std::atomic<bool>      closed_{false};
    std::mutex             write_mtx_;
    std::jthread           read_thread_;
};
