import Foundation

class LogCapture {
    private var task: Task<Void, Never>?

    func startStreaming(via conn: PeerConnection, filter: String = "") {
        stop()
        let id = UUID().uuidString
        task = Task.detached(priority: .utility) {
            let process = Process()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/log")
            var args = ["stream", "--level", "debug"]
            if !filter.isEmpty { args += ["--predicate", filter] }
            process.arguments = args
            let pipe = Pipe()
            process.standardOutput = pipe
            guard (try? process.run()) != nil else { return }
            for try await line in pipe.fileHandleForReading.bytes.lines {
                guard !Task.isCancelled else { break }
                conn.send(.logcat(id: id, line: line))
            }
            process.terminate()
        }
    }

    func stop() {
        task?.cancel()
        task = nil
    }
}
