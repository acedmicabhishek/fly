#pragma once
#include "fly_frame.h"
#include <string>
#include <variant>
#include <optional>
#include <cstdint>

namespace fly {

struct MsgHello    { std::string name; std::string platform = "desktop"; };
struct MsgText     { std::string id;   std::string content; };
struct MsgFileOffer{ std::string id;   std::string name; std::string mime; int64_t size; };
struct MsgFileStart{ std::string id;   std::string name; std::string mime; int64_t size;
                     int total_chunks; int chunk_size; };
struct MsgChunk    { std::string id;   int index; };
struct MsgFileDone { std::string id; };
struct MsgAck      { std::string id; };
struct MsgLogcat   { std::string id;   std::string line; };
struct MsgCancel   { std::string id; };

using Message = std::variant<
    MsgHello, MsgText, MsgFileOffer, MsgFileStart,
    MsgChunk, MsgFileDone, MsgAck, MsgLogcat, MsgCancel>;

std::optional<Message> parse_frame(const FlyFrame& frame);
FlyFrame to_frame(const Message& msg, const std::vector<uint8_t>& body = {});

}
