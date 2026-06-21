import SwiftUI

struct ContentView: View {
    var body: some View {
        TabView {
            DevicesView()
                .tabItem { Label("Devices", systemImage: "wifi") }
            TransferView()
                .tabItem { Label("Transfer", systemImage: "paperplane.fill") }
            SettingsView()
                .tabItem { Label("Settings", systemImage: "gearshape") }
        }
    }
}
