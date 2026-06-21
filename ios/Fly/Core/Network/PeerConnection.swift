import Foundation
import Network
import Combine

class PeerConnection: ObservableObject, Identifiable {
    let id = UUID()
    private let connection: NWConnection
    let device: NetworkDevice?

    let incomingMessages = PassthroughSubject<(FlyMessage, Data), Never>()
    private var receiveBuffer = Data()

    init(connection: NWConnection, device: NetworkDevice? = nil) {
        self.connection = connection
        self.device = device
    }

    func start() {
        connection.start(queue: .global(qos: .utility))
        receiveLoop()
    }

    func send(_ message: FlyMessage, body: Data = Data()) {
        let frame = message.toFrame(body: body)
        let data = frame.encoded()
        connection.send(content: data, completion: .idempotent)
    }

    func close() {
        connection.cancel()
    }

    private func receiveLoop() {
        connection.receive(minimumIncompleteLength: 1, maximumLength: 1 << 20) { [weak self] data, _, isComplete, error in
            guard let self else { return }
            if let data { self.receiveBuffer.append(data) }
            self.processBuffer()
            if error == nil && !isComplete { self.receiveLoop() }
        }
    }

    private func processBuffer() {
        while receiveBuffer.count >= 8 {
            let headerLen = Int(readUInt32(from: receiveBuffer, at: 0))
            guard headerLen > 0, receiveBuffer.count >= 4 + headerLen + 4 else { break }
            let bodyLen = Int(readUInt32(from: receiveBuffer, at: 4 + headerLen))
            let totalSize = 4 + headerLen + 4 + bodyLen
            guard receiveBuffer.count >= totalSize else { break }

            let headerData = receiveBuffer[4..<(4 + headerLen)]
            let headerStr = String(data: headerData, encoding: .utf8) ?? ""
            let body = Data(receiveBuffer[(4 + headerLen + 4)..<totalSize])
            let frame = FlyFrame(header: headerStr, body: body)

            if let msg = FlyMessage.from(frame: frame) {
                incomingMessages.send((msg, frame.body))
            }
            receiveBuffer = Data(receiveBuffer[totalSize...])
        }
    }

    private func readUInt32(from data: Data, at offset: Int) -> UInt32 {
        data[offset..<(offset + 4)].withUnsafeBytes { $0.load(as: UInt32.self).bigEndian }
    }
}
