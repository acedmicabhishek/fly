import SwiftUI

struct TransferRow: View {
    let item: TransferItem

    var body: some View {
        VStack(alignment: .leading, spacing: 6) {
            Text(directionLabel).font(.caption).foregroundStyle(.blue)
            switch item {
            case .text(_, _, _, let content):
                Text(content).font(.body)
            case .file(_, _, _, let name, _, let size, _, let progress, _, _):
                Text(name).font(.body).bold()
                Text(ByteCountFormatter.string(fromByteCount: size, countStyle: .file)).font(.caption).foregroundStyle(.secondary)
                if progress < 1.0 {
                    ProgressView(value: progress).tint(.blue)
                }
            case .logcatStream(_, _, _, let lines):
                Text("Logcat — \(lines.count) lines").font(.body).bold()
                ForEach(lines.suffix(3), id: \.self) { line in
                    Text(line).font(.caption2).foregroundStyle(.secondary).lineLimit(1)
                }
            }
        }
        .padding(.vertical, 4)
    }

    private var directionLabel: String {
        (item.isIncoming ? "← " : "→ ") + item.deviceName
    }
}
