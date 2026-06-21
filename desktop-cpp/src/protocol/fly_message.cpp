#include "fly_message.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace fly {

std::optional<Message> parse_frame(const FlyFrame& frame) {
    try {
        auto j = json::parse(frame.header);
        auto t = j.at("type").get<std::string>();

        if (t == "hello")
            return MsgHello{j.at("name"), j.value("platform", "unknown")};
        if (t == "text")
            return MsgText{j.at("id"), j.at("content")};
        if (t == "file")
            return MsgFileOffer{j.at("id"), j.at("name"),
                                j.value("mime", "application/octet-stream"),
                                j.at("size").get<int64_t>()};
        if (t == "file_start")
            return MsgFileStart{j.at("id"), j.at("name"),
                                j.value("mime", "application/octet-stream"),
                                j.at("size").get<int64_t>(),
                                j.at("chunks").get<int>(),
                                j.at("chunk_size").get<int>()};
        if (t == "chunk")
            return MsgChunk{j.at("id"), j.at("index").get<int>()};
        if (t == "file_done")
            return MsgFileDone{j.at("id")};
        if (t == "ack")
            return MsgAck{j.at("id")};
        if (t == "logcat")
            return MsgLogcat{j.at("id"), j.at("line")};
        if (t == "cancel")
            return MsgCancel{j.at("id")};
    } catch (...) {}
    return std::nullopt;
}

FlyFrame to_frame(const Message& msg, const std::vector<uint8_t>& body) {
    std::string header;
    std::visit([&](auto&& m) {
        using T = std::decay_t<decltype(m)>;
        json j;
        if constexpr (std::is_same_v<T, MsgHello>)
            j = {{"type","hello"},{"name",m.name},{"platform",m.platform}};
        else if constexpr (std::is_same_v<T, MsgText>)
            j = {{"type","text"},{"id",m.id},{"content",m.content}};
        else if constexpr (std::is_same_v<T, MsgFileOffer>)
            j = {{"type","file"},{"id",m.id},{"name",m.name},{"mime",m.mime},{"size",m.size}};
        else if constexpr (std::is_same_v<T, MsgFileStart>)
            j = {{"type","file_start"},{"id",m.id},{"name",m.name},{"mime",m.mime},
                 {"size",m.size},{"chunks",m.total_chunks},{"chunk_size",m.chunk_size}};
        else if constexpr (std::is_same_v<T, MsgChunk>)
            j = {{"type","chunk"},{"id",m.id},{"index",m.index}};
        else if constexpr (std::is_same_v<T, MsgFileDone>)
            j = {{"type","file_done"},{"id",m.id}};
        else if constexpr (std::is_same_v<T, MsgAck>)
            j = {{"type","ack"},{"id",m.id}};
        else if constexpr (std::is_same_v<T, MsgLogcat>)
            j = {{"type","logcat"},{"id",m.id},{"line",m.line}};
        else if constexpr (std::is_same_v<T, MsgCancel>)
            j = {{"type","cancel"},{"id",m.id}};
        header = j.dump();
    }, msg);
    return {header, body};
}

}
