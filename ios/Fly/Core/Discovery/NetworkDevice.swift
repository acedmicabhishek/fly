import Foundation

struct NetworkDevice: Identifiable, Hashable {
    let id: String
    let name: String
    let platform: String
    let host: String
    let port: Int
}
