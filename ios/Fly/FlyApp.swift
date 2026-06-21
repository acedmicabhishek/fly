import SwiftUI

@main
struct FlyApp: App {
    @StateObject private var vm = AppViewModel()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(vm)
                .preferredColorScheme(colorScheme(for: vm.themeMode))
        }
    }

    private func colorScheme(for mode: String) -> ColorScheme? {
        switch mode {
        case "dark": return .dark
        case "light": return .light
        default: return nil
        }
    }
}
