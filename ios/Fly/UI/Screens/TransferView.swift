import SwiftUI
import UniformTypeIdentifiers

struct TransferView: View {
    @EnvironmentObject var vm: AppViewModel
    @State private var text = ""
    @State private var logFilter = ""
    @State private var showFilePicker = false

    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                if vm.activeConnection == nil {
                    Spacer()
                    Text("No active connection.\nGo to Devices and connect first.")
                        .multilineTextAlignment(.center)
                        .foregroundStyle(.secondary)
                        .padding()
                    Spacer()
                } else {
                    VStack(spacing: 8) {
                        HStack {
                            TextField("Type a message", text: $text)
                                .textFieldStyle(.roundedBorder)
                                .submitLabel(.send)
                                .onSubmit { sendText() }
                            Button("Send") { sendText() }
                                .disabled(text.trimmingCharacters(in: .whitespaces).isEmpty)
                        }
                        HStack {
                            TextField("Logcat filter (optional)", text: $logFilter)
                                .textFieldStyle(.roundedBorder)
                            Button("Logcat") { vm.sendLogcat(filter: logFilter) }
                        }
                        HStack {
                            Button("Send file") { showFilePicker = true }
                                .buttonStyle(.bordered)
                            Spacer()
                        }
                    }
                    .padding()
                    Divider()
                    List(vm.transferManager.transfers) { item in
                        TransferRow(item: item)
                    }
                    .listStyle(.plain)
                }
            }
            .navigationTitle("Transfer")
            .fileImporter(isPresented: $showFilePicker, allowedContentTypes: [.data], allowsMultipleSelection: true) { result in
                if let urls = try? result.get() {
                    urls.forEach { url in
                        _ = url.startAccessingSecurityScopedResource()
                        vm.sendFile(at: url)
                    }
                }
            }
        }
    }

    private func sendText() {
        let trimmed = text.trimmingCharacters(in: .whitespaces)
        guard !trimmed.isEmpty else { return }
        vm.sendText(trimmed)
        text = ""
    }
}
