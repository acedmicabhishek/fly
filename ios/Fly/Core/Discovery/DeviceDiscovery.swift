import Foundation
import Network
import Combine

class DeviceDiscovery: ObservableObject {
    @Published var devices: [String: NetworkDevice] = [:]
    var endpoints: [String: NWEndpoint] = [:]

    private var browser: NWBrowser?

    func start() {
        let params = NWParameters()
        params.includePeerToPeer = true
        browser = NWBrowser(for: .bonjour(type: "_fly._tcp", domain: nil), using: params)
        browser?.browseResultsChangedHandler = { [weak self] _, changes in
            for change in changes {
                switch change {
                case .added(let result):
                    self?.resolve(result)
                case .removed(let result):
                    if case .service(let name, _, _, _) = result.endpoint {
                        DispatchQueue.main.async {
                            self?.devices.removeValue(forKey: name)
                            self?.endpoints.removeValue(forKey: name)
                        }
                    }
                default:
                    break
                }
            }
        }
        browser?.start(queue: .global(qos: .utility))
    }

    func stop() {
        browser?.cancel()
        browser = nil
    }

    private func resolve(_ result: NWBrowser.Result) {
        guard case .service(let name, _, _, _) = result.endpoint else { return }
        let conn = NWConnection(to: result.endpoint, using: .tcp)
        conn.stateUpdateHandler = { [weak self] state in
            switch state {
            case .ready:
                if let remote = conn.currentPath?.remoteEndpoint {
                    var host = ""
                    var port = 0
                    if case .hostPort(let h, let p) = remote {
                        host = "\(h)"
                        port = Int(p.rawValue)
                    }
                    let device = NetworkDevice(id: name, name: name, platform: "unknown", host: host, port: port)
                    DispatchQueue.main.async {
                        self?.devices[name] = device
                        self?.endpoints[name] = result.endpoint
                    }
                }
                conn.cancel()
            case .failed:
                conn.cancel()
            default:
                break
            }
        }
        conn.start(queue: .global(qos: .utility))
    }
}
