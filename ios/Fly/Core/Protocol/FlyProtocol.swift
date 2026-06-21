import Foundation

struct FlyFrame {
    let header: String
    let body: Data

    init(header: String, body: Data = Data()) {
        self.header = header
        self.body = body
    }

    func encoded() -> Data {
        var data = Data()
        let headerData = header.data(using: .utf8) ?? Data()
        var headerLen = UInt32(headerData.count).bigEndian
        data.append(Data(bytes: &headerLen, count: 4))
        data.append(headerData)
        var bodyLen = UInt32(body.count).bigEndian
        data.append(Data(bytes: &bodyLen, count: 4))
        data.append(body)
        return data
    }
}

enum FlyMessage {
    case hello(deviceName: String, platform: String)
    case text(id: String, content: String)
    case fileOffer(id: String, name: String, mime: String, size: Int64)
    case fileStart(id: String, name: String, mime: String, size: Int64, totalChunks: Int, chunkSize: Int)
    case chunk(id: String, index: Int)
    case fileDone(id: String)
    case ack(id: String)
    case logcat(id: String, line: String)
    case cancel(id: String)
}

extension FlyMessage {
    func toFrame(body: Data = Data()) -> FlyFrame {
        switch self {
        case .hello(let name, let platform):
            return FlyFrame(header: #"{"type":"hello","name":"\#(escaped(name))","platform":"\#(platform)"}"#)
        case .text(let id, let content):
            return FlyFrame(header: #"{"type":"text","id":"\#(id)","content":"\#(escaped(content))"}"#)
        case .fileOffer(let id, let name, let mime, let size):
            return FlyFrame(header: #"{"type":"file","id":"\#(id)","name":"\#(escaped(name))","mime":"\#(mime)","size":\#(size)}"#, body: body)
        case .fileStart(let id, let name, let mime, let size, let chunks, let chunkSize):
            return FlyFrame(header: #"{"type":"file_start","id":"\#(id)","name":"\#(escaped(name))","mime":"\#(mime)","size":\#(size),"chunks":\#(chunks),"chunk_size":\#(chunkSize)}"#)
        case .chunk(let id, let index):
            return FlyFrame(header: #"{"type":"chunk","id":"\#(id)","index":\#(index)}"#, body: body)
        case .fileDone(let id):
            return FlyFrame(header: #"{"type":"file_done","id":"\#(id)"}"#)
        case .ack(let id):
            return FlyFrame(header: #"{"type":"ack","id":"\#(id)"}"#)
        case .logcat(let id, let line):
            return FlyFrame(header: #"{"type":"logcat","id":"\#(id)","line":"\#(escaped(line))"}"#)
        case .cancel(let id):
            return FlyFrame(header: #"{"type":"cancel","id":"\#(id)"}"#)
        }
    }

    static func from(frame: FlyFrame) -> FlyMessage? {
        guard let headerData = frame.header.data(using: .utf8),
              let json = try? JSONSerialization.jsonObject(with: headerData) as? [String: Any],
              let type = json["type"] as? String else { return nil }
        switch type {
        case "hello":
            guard let name = json["name"] as? String else { return nil }
            return .hello(deviceName: name, platform: json["platform"] as? String ?? "unknown")
        case "text":
            guard let id = json["id"] as? String, let content = json["content"] as? String else { return nil }
            return .text(id: id, content: content)
        case "file":
            guard let id = json["id"] as? String, let name = json["name"] as? String,
                  let size = json["size"] as? Int64 else { return nil }
            return .fileOffer(id: id, name: name, mime: json["mime"] as? String ?? "application/octet-stream", size: size)
        case "file_start":
            guard let id = json["id"] as? String, let name = json["name"] as? String,
                  let size = json["size"] as? Int64, let chunks = json["chunks"] as? Int,
                  let chunkSize = json["chunk_size"] as? Int else { return nil }
            return .fileStart(id: id, name: name, mime: json["mime"] as? String ?? "application/octet-stream", size: size, totalChunks: chunks, chunkSize: chunkSize)
        case "chunk":
            guard let id = json["id"] as? String, let index = json["index"] as? Int else { return nil }
            return .chunk(id: id, index: index)
        case "file_done":
            guard let id = json["id"] as? String else { return nil }
            return .fileDone(id: id)
        case "ack":
            guard let id = json["id"] as? String else { return nil }
            return .ack(id: id)
        case "logcat":
            guard let id = json["id"] as? String, let line = json["line"] as? String else { return nil }
            return .logcat(id: id, line: line)
        case "cancel":
            guard let id = json["id"] as? String else { return nil }
            return .cancel(id: id)
        default:
            return nil
        }
    }
}

private func escaped(_ s: String) -> String {
    s.replacingOccurrences(of: "\\", with: "\\\\")
     .replacingOccurrences(of: "\"", with: "\\\"")
     .replacingOccurrences(of: "\n", with: "\\n")
     .replacingOccurrences(of: "\r", with: "\\r")
}
