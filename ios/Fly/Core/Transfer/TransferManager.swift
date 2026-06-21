import Foundation
import Combine

class TransferManager: ObservableObject {
    @Published var transfers: [TransferItem] = []

    private var pendingChunks: [String: [Data]] = [:]
    private var pendingFileInfos: [String: (name: String, mime: String, size: Int64, totalChunks: Int)] = [:]
    private var cancellables = Set<AnyCancellable>()

    var chunkThresholdBytes: Int64 = 64 * 1024 * 1024
    var chunkSizeBytes: Int = 4 * 1024 * 1024

    func addConnection(_ conn: PeerConnection, deviceName: String) {
        conn.incomingMessages
            .receive(on: DispatchQueue.main)
            .sink { [weak self] (msg, body) in
                self?.handle(msg, body: body, deviceName: deviceName, conn: conn)
            }
            .store(in: &cancellables)
    }

    private func handle(_ msg: FlyMessage, body: Data, deviceName: String, conn: PeerConnection) {
        switch msg {
        case .hello:
            break

        case .text(let id, let content):
            prepend(.text(id: id, deviceName: deviceName, isIncoming: true, content: content))

        case .fileOffer(let id, let name, let mime, let size):
            let url = saveFile(name: name, data: body)
            prepend(.file(id: id, deviceName: deviceName, isIncoming: true, name: name, mime: mime, size: size, localURL: url, progress: 1.0, totalChunks: 1, receivedChunks: 1))
            conn.send(.ack(id: id))

        case .fileStart(let id, let name, let mime, let size, let totalChunks, _):
            pendingFileInfos[id] = (name, mime, size, totalChunks)
            pendingChunks[id] = []
            prepend(.file(id: id, deviceName: deviceName, isIncoming: true, name: name, mime: mime, size: size, localURL: nil, progress: 0.0, totalChunks: totalChunks, receivedChunks: 0))

        case .chunk(let id, _):
            pendingChunks[id]?.append(body)
            guard let info = pendingFileInfos[id] else { break }
            let received = pendingChunks[id]?.count ?? 0
            updateFile(id: id, progress: Double(received) / Double(info.totalChunks), received: received)

        case .fileDone(let id):
            guard let chunks = pendingChunks.removeValue(forKey: id),
                  let info = pendingFileInfos.removeValue(forKey: id) else { break }
            let fullData = chunks.reduce(Data()) { $0 + $1 }
            let url = saveFile(name: info.name, data: fullData)
            updateFile(id: id, progress: 1.0, received: info.totalChunks, localURL: url)
            conn.send(.ack(id: id))

        case .logcat(let id, let line):
            if let idx = transfers.firstIndex(where: { $0.id == id }),
               case .logcatStream(_, let dn, let inc, let lines) = transfers[idx] {
                transfers[idx] = .logcatStream(id: id, deviceName: dn, isIncoming: inc, lines: lines + [line])
            } else {
                prepend(.logcatStream(id: id, deviceName: deviceName, isIncoming: true, lines: [line]))
            }

        default:
            break
        }
    }

    func sendText(_ text: String, via conn: PeerConnection) {
        let id = UUID().uuidString
        conn.send(.text(id: id, content: text))
        prepend(.text(id: id, deviceName: "Me", isIncoming: false, content: text))
    }

    func sendFile(at url: URL, via conn: PeerConnection) {
        let id = UUID().uuidString
        let name = url.lastPathComponent
        guard let data = try? Data(contentsOf: url) else { return }
        let size = Int64(data.count)
        let mime = "application/octet-stream"

        if size <= chunkThresholdBytes {
            conn.send(.fileOffer(id: id, name: name, mime: mime, size: size), body: data)
            prepend(.file(id: id, deviceName: "Me", isIncoming: false, name: name, mime: mime, size: size, localURL: url, progress: 1.0, totalChunks: 1, receivedChunks: 1))
        } else {
            let chunkSize = chunkSizeBytes
            let totalChunks = Int(ceil(Double(size) / Double(chunkSize)))
            conn.send(.fileStart(id: id, name: name, mime: mime, size: size, totalChunks: totalChunks, chunkSize: chunkSize))
            prepend(.file(id: id, deviceName: "Me", isIncoming: false, name: name, mime: mime, size: size, localURL: url, progress: 0.0, totalChunks: totalChunks, receivedChunks: 0))
            DispatchQueue.global(qos: .utility).async { [weak self] in
                var offset = 0
                var index = 0
                while offset < data.count {
                    let end = min(offset + chunkSize, data.count)
                    let chunk = data[offset..<end]
                    conn.send(.chunk(id: id, index: index), body: Data(chunk))
                    let received = index + 1
                    DispatchQueue.main.async {
                        self?.updateFile(id: id, progress: Double(received) / Double(totalChunks), received: received)
                    }
                    offset = end
                    index += 1
                }
                conn.send(.fileDone(id: id))
                DispatchQueue.main.async { self?.updateFile(id: id, progress: 1.0, received: totalChunks) }
            }
        }
    }

    private func saveFile(name: String, data: Data) -> URL {
        let dir = FileManager.default.temporaryDirectory.appendingPathComponent("fly_received", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        let url = dir.appendingPathComponent(name)
        try? data.write(to: url)
        return url
    }

    private func prepend(_ item: TransferItem) {
        transfers.insert(item, at: 0)
    }

    private func updateFile(id: String, progress: Double, received: Int, localURL: URL? = nil) {
        if let idx = transfers.firstIndex(where: { $0.id == id }),
           case .file(_, let dn, let inc, let name, let mime, let size, let existingURL, _, let total, _) = transfers[idx] {
            transfers[idx] = .file(id: id, deviceName: dn, isIncoming: inc, name: name, mime: mime, size: size, localURL: localURL ?? existingURL, progress: progress, totalChunks: total, receivedChunks: received)
        }
    }
}
