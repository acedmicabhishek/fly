#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class TransferDir { In, Out };

struct TransferItem {
    std::string id;
    std::string device_name;
    TransferDir dir;

    enum class Kind { Text, File, Logcat } kind;

    std::string content;

    std::string  file_name;
    std::string  mime;
    int64_t      file_size{0};
    std::string  local_path;
    float        progress{0.f};
    int          total_chunks{0};
    int          recv_chunks{0};

    std::vector<std::string> log_lines;
};
