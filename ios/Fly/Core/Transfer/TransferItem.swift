import Foundation

enum TransferItem: Identifiable {
    case text(id: String, deviceName: String, isIncoming: Bool, content: String)
    case file(id: String, deviceName: String, isIncoming: Bool, name: String, mime: String, size: Int64, localURL: URL?, progress: Double, totalChunks: Int, receivedChunks: Int)
    case logcatStream(id: String, deviceName: String, isIncoming: Bool, lines: [String])

    var id: String {
        switch self {
        case .text(let id, _, _, _): return id
        case .file(let id, _, _, _, _, _, _, _, _, _): return id
        case .logcatStream(let id, _, _, _): return id
        }
    }

    var isIncoming: Bool {
        switch self {
        case .text(_, _, let v, _): return v
        case .file(_, _, let v, _, _, _, _, _, _, _): return v
        case .logcatStream(_, _, let v, _): return v
        }
    }

    var deviceName: String {
        switch self {
        case .text(_, let v, _, _): return v
        case .file(_, let v, _, _, _, _, _, _, _, _): return v
        case .logcatStream(_, let v, _, _): return v
        }
    }
}
