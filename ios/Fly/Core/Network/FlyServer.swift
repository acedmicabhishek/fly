import Foundation
import Network
import Combine

class FlyServer {
    private var listener: NWListener?
    let newConnection = PassthroughSubject<PeerConnection, Never>()
    private let port: UInt16

    init(port: UInt16) {
        self.port = port
    }

    func start(deviceName: String) {
        guard let nwPort = NWEndpoint.Port(rawValue: port) else { return }
        listener = try? NWListener(using: .tcp, on: nwPort)
        listener?.service = NWListener.Service(name: deviceName, type: "_fly._tcp")
        listener?.newConnectionHandler = { [weak self] conn in
            let peer = PeerConnection(connection: conn)
            peer.start()
            self?.newConnection.send(peer)
        }
        listener?.start(queue: .global(qos: .utility))
    }

    func stop() {
        listener?.cancel()
        listener = nil
    }
}
