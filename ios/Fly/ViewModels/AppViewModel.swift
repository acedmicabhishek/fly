import Foundation
import Combine
import UIKit

@MainActor
class AppViewModel: ObservableObject {
    @Published var discoveredDevices: [NetworkDevice] = []
    @Published var activeConnection: PeerConnection?
    @Published var status = "Idle"
    @Published var themeMode: String

    let transferManager = TransferManager()
    private let discovery = DeviceDiscovery()
    private let server: FlyServer
    private let client = FlyClient()
    private let logCapture = LogCapture()

    private var cancellables = Set<AnyCancellable>()

    var deviceName: String {
        get { UserDefaults.standard.string(forKey: "device_name") ?? UIDevice.current.name }
        set { UserDefaults.standard.set(newValue, forKey: "device_name") }
    }

    var port: Int {
        let v = UserDefaults.standard.integer(forKey: "port")
        return v == 0 ? 5800 : v
    }

    var autoConnect: Bool {
        get { UserDefaults.standard.bool(forKey: "auto_connect") }
        set { UserDefaults.standard.set(newValue, forKey: "auto_connect") }
    }

    var chunkThresholdMb: Int {
        get { let v = UserDefaults.standard.integer(forKey: "chunk_threshold_mb"); return v == 0 ? 64 : v }
        set { UserDefaults.standard.set(newValue, forKey: "chunk_threshold_mb"); transferManager.chunkThresholdBytes = Int64(newValue) * 1024 * 1024 }
    }

    var chunkSizeMb: Int {
        get { let v = UserDefaults.standard.integer(forKey: "chunk_size_mb"); return v == 0 ? 4 : v }
        set { UserDefaults.standard.set(newValue, forKey: "chunk_size_mb"); transferManager.chunkSizeBytes = newValue * 1024 * 1024 }
    }

    init() {
        themeMode = UserDefaults.standard.string(forKey: "theme_mode") ?? "system"
        server = FlyServer(port: UInt16(UserDefaults.standard.integer(forKey: "port").nonZero ?? 5800))
        transferManager.chunkThresholdBytes = Int64(chunkThresholdMb) * 1024 * 1024
        transferManager.chunkSizeBytes = chunkSizeMb * 1024 * 1024

        discovery.$devices
            .map { $0.values.sorted { $0.name < $1.name } }
            .receive(on: DispatchQueue.main)
            .assign(to: &$discoveredDevices)

        server.newConnection
            .receive(on: DispatchQueue.main)
            .sink { [weak self] conn in
                self?.onConnected(conn, name: "incoming")
            }
            .store(in: &cancellables)

        server.start(deviceName: deviceName)
        discovery.start()
    }

    func connectTo(_ device: NetworkDevice) {
        status = "Connecting to \(device.name)..."
        if let endpoint = discovery.endpoints[device.id] {
            client.connect(to: endpoint) { [weak self] conn in
                Task { @MainActor [weak self] in
                    conn.send(.hello(deviceName: self?.deviceName ?? "iOS", platform: "ios"))
                    self?.onConnected(conn, name: device.name)
                }
            }
        } else {
            client.connect(to: device) { [weak self] conn in
                Task { @MainActor [weak self] in
                    conn.send(.hello(deviceName: self?.deviceName ?? "iOS", platform: "ios"))
                    self?.onConnected(conn, name: device.name)
                }
            }
        }
    }

    func connectManual(host: String, port: Int) {
        status = "Connecting to \(host)..."
        client.connect(host: host, port: port) { [weak self] conn in
            Task { @MainActor [weak self] in
                conn.send(.hello(deviceName: self?.deviceName ?? "iOS", platform: "ios"))
                self?.onConnected(conn, name: "\(host):\(port)")
            }
        }
    }

    private func onConnected(_ conn: PeerConnection, name: String) {
        activeConnection = conn
        transferManager.addConnection(conn, deviceName: name)
        status = "Connected to \(name)"
    }

    func sendText(_ text: String) {
        guard let conn = activeConnection else { return }
        transferManager.sendText(text, via: conn)
    }

    func sendFile(at url: URL) {
        guard let conn = activeConnection else { return }
        transferManager.sendFile(at: url, via: conn)
    }

    func sendLogcat(filter: String = "") {
        guard let conn = activeConnection else { return }
        logCapture.startStreaming(via: conn, filter: filter)
    }

    func disconnect() {
        activeConnection?.close()
        activeConnection = nil
        logCapture.stop()
        status = "Disconnected"
    }

    func saveThemeMode(_ mode: String) {
        UserDefaults.standard.set(mode, forKey: "theme_mode")
        themeMode = mode
    }
}

private extension Int {
    var nonZero: Int? { self == 0 ? nil : self }
}
