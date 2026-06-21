#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct FlyFrame {
    std::string header;
    std::vector<uint8_t> body;
};

FlyFrame  read_frame(int fd);
void      write_frame(int fd, const FlyFrame& frame);
