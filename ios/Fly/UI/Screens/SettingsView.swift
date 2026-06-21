import SwiftUI

struct SettingsView: View {
    @EnvironmentObject var vm: AppViewModel
    @State private var deviceName = ""
    @State private var autoConnect = false
    @State private var thresholdMb: Double = 64
    @State private var chunkSizeMb: Double = 4

    var body: some View {
        NavigationView {
            Form {
                Section("Device") {
                    TextField("Device name", text: $deviceName)
                        .autocorrectionDisabled()
                    Button("Save name") { vm.deviceName = deviceName }
                }
                Section("Connection") {
                    Toggle("Auto-connect to last device", isOn: $autoConnect)
                        .onChange(of: autoConnect) { vm.autoConnect = $0 }
                    LabeledContent("Port", value: "\(vm.port)")
                }
                Section("Transfer") {
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Chunk threshold: \(Int(thresholdMb)) MB")
                            .font(.subheadline)
                        Text("Files above this size use chunked streaming").font(.caption).foregroundStyle(.secondary)
                        Slider(value: $thresholdMb, in: 16...512, step: 16) { _ in vm.chunkThresholdMb = Int(thresholdMb) }
                    }
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Chunk size: \(Int(chunkSizeMb)) MB per chunk")
                            .font(.subheadline)
                        Slider(value: $chunkSizeMb, in: 1...32, step: 1) { _ in vm.chunkSizeMb = Int(chunkSizeMb) }
                    }
                }
                Section("Appearance") {
                    Picker("Theme", selection: Binding(
                        get: { vm.themeMode },
                        set: { vm.saveThemeMode($0) }
                    )) {
                        Text("System").tag("system")
                        Text("Dark").tag("dark")
                        Text("Light").tag("light")
                    }
                    .pickerStyle(.segmented)
                }
            }
            .navigationTitle("Settings")
            .onAppear {
                deviceName = vm.deviceName
                autoConnect = vm.autoConnect
                thresholdMb = Double(vm.chunkThresholdMb)
                chunkSizeMb = Double(vm.chunkSizeMb)
            }
        }
    }
}
