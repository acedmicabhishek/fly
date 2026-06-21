import SwiftUI

struct DevicesView: View {
    @EnvironmentObject var vm: AppViewModel
    @State private var manualInput = "192.168.1.x:5800"
    @State private var showManual = false

    var body: some View {
        NavigationView {
            List {
                Section {
                    HStack {
                        Text(vm.status).foregroundStyle(.secondary).font(.subheadline)
                        Spacer()
                        if vm.activeConnection != nil {
                            Button("Disconnect", role: .destructive) { vm.disconnect() }
                        }
                    }
                    Button(showManual ? "Hide manual connect" : "Manual connect (localhost/custom IP)") {
                        showManual.toggle()
                    }
                    if showManual {
                        TextField("host:port", text: $manualInput)
                            .autocorrectionDisabled()
                            .textInputAutocapitalization(.never)
                        Button("Connect") { connectManual() }
                    }
                }
                Section("Nearby devices") {
                    if vm.discoveredDevices.isEmpty {
                        Text("Scanning...").foregroundStyle(.secondary)
                    } else {
                        ForEach(vm.discoveredDevices) { device in
                            DeviceCard(device: device, onTap: { vm.connectTo(device) })
                        }
                    }
                }
            }
            .navigationTitle("Fly")
        }
    }

    private func connectManual() {
        let parts = manualInput.trimmingCharacters(in: .whitespaces).split(separator: ":")
        let host = String(parts.first ?? "localhost")
        let port = Int(parts.last ?? "5800") ?? 5800
        vm.connectManual(host: host, port: port)
    }
}
