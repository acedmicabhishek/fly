import Foundation
import Network

class FlyClient {
    func connect(to device: NetworkDevice, completion: @escaping (PeerConnection) -> Void) {
        guard let port = NWEndpoint.Port(rawValue: UInt16(device.port)) else { return }
        let host = NWEndpoint.Host(device.host)
        let conn = NWConnection(host: host, port: port, using: .tcp)
        let peer = PeerConnection(connection: conn, device: device)
        conn.stateUpdateHandler = { state in
            if case .ready = state { completion(peer) }
        }
        peer.start()
    }

    func connect(to endpoint: NWEndpoint, completion: @escaping (PeerConnection) -> Void) {
        let conn = NWConnection(to: endpoint, using: .tcp)
        let peer = PeerConnection(connection: conn)
        conn.stateUpdateHandler = { state in
            if case .ready = state { completion(peer) }
        }
        peer.start()
    }

    func connect(host: String, port: Int, completion: @escaping (PeerConnection) -> Void) {
        guard let nwPort = NWEndpoint.Port(rawValue: UInt16(port)) else { return }
        let conn = NWConnection(host: NWEndpoint.Host(host), port: nwPort, using: .tcp)
        let peer = PeerConnection(connection: conn)
        conn.stateUpdateHandler = { state in
            if case .ready = state { completion(peer) }
        }
        peer.start()
    }
}
