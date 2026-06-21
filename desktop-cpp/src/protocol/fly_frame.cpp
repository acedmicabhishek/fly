#include "fly_frame.h"
#include <stdexcept>
#include <arpa/inet.h>
#include <unistd.h>

static void read_all(int fd, uint8_t* buf, size_t n) {
    size_t done = 0;
    while (done < n) {
        ssize_t r = ::read(fd, buf + done, n - done);
        if (r <= 0) throw std::runtime_error("connection closed");
        done += static_cast<size_t>(r);
    }
}

static void write_all(int fd, const uint8_t* buf, size_t n) {
    size_t done = 0;
    while (done < n) {
        ssize_t w = ::write(fd, buf + done, n - done);
        if (w <= 0) throw std::runtime_error("write failed");
        done += static_cast<size_t>(w);
    }
}

FlyFrame read_frame(int fd) {
    uint32_t hlen_be;
    read_all(fd, reinterpret_cast<uint8_t*>(&hlen_be), 4);
    uint32_t hlen = ntohl(hlen_be);
    if (hlen < 1 || hlen > 65536)
        throw std::runtime_error("invalid header length");

    std::string header(hlen, '\0');
    read_all(fd, reinterpret_cast<uint8_t*>(header.data()), hlen);

    uint32_t blen_be;
    read_all(fd, reinterpret_cast<uint8_t*>(&blen_be), 4);
    uint32_t blen = ntohl(blen_be);

    std::vector<uint8_t> body(blen);
    if (blen > 0)
        read_all(fd, body.data(), blen);

    return {std::move(header), std::move(body)};
}

void write_frame(int fd, const FlyFrame& frame) {
    uint32_t hlen = htonl(static_cast<uint32_t>(frame.header.size()));
    write_all(fd, reinterpret_cast<uint8_t*>(&hlen), 4);
    write_all(fd, reinterpret_cast<const uint8_t*>(frame.header.data()), frame.header.size());

    uint32_t blen = htonl(static_cast<uint32_t>(frame.body.size()));
    write_all(fd, reinterpret_cast<uint8_t*>(&blen), 4);
    if (!frame.body.empty())
        write_all(fd, frame.body.data(), frame.body.size());
}
