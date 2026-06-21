#include "peer_connection.h"
#include "../protocol/fly_frame.h"
#include <unistd.h>

PeerConnection::PeerConnection(int fd) : fd_(fd) {}

PeerConnection::~PeerConnection() {
    close();
}

void PeerConnection::start(MsgCallback on_msg, DiscCallback on_disconnect) {
    read_thread_ = std::jthread([this, on_msg, on_disconnect](std::stop_token st) {
        while (!st.stop_requested() && !closed_) {
            try {
                FlyFrame frame = read_frame(fd_);
                auto msg = fly::parse_frame(frame);
                if (msg && on_msg)
                    on_msg(*msg, frame.body);
            } catch (...) {
                break;
            }
        }
        closed_ = true;
        if (on_disconnect) on_disconnect();
    });
}

bool PeerConnection::send(const fly::Message& msg, const std::vector<uint8_t>& body) {
    if (closed_) return false;
    try {
        std::lock_guard<std::mutex> lk(write_mtx_);
        write_frame(fd_, fly::to_frame(msg, body));
        return true;
    } catch (...) {
        return false;
    }
}

void PeerConnection::close() {
    if (!closed_.exchange(true))
        ::close(fd_);
}
