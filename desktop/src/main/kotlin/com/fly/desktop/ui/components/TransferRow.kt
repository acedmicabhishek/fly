package com.fly.desktop.ui.components

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.FolderOpen
import androidx.compose.material3.Card
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.fly.desktop.core.transfer.TransferItem
import java.awt.Desktop
import java.awt.Toolkit
import java.awt.datatransfer.StringSelection
import java.text.DecimalFormat

@Composable
fun TransferRow(item: TransferItem) {
    val dirLabel = if (item.isIncoming) "← ${item.deviceName}" else "→ ${item.deviceName}"
    Card(modifier = Modifier.fillMaxWidth()) {
        Column(modifier = Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            Text(dirLabel, style = MaterialTheme.typography.labelSmall, color = MaterialTheme.colorScheme.primary)
            when (item) {
                is TransferItem.Text -> Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.Top
                ) {
                    SelectionContainer(modifier = Modifier.weight(1f)) {
                        Text(item.content, style = MaterialTheme.typography.bodyMedium)
                    }
                    Spacer(modifier = Modifier.width(4.dp))
                    IconButton(onClick = {
                        val clipboard = Toolkit.getDefaultToolkit().systemClipboard
                        clipboard.setContents(StringSelection(item.content), null)
                    }) {
                        Icon(Icons.Default.ContentCopy, contentDescription = "Copy", tint = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f))
                    }
                }

                is TransferItem.FileTransfer -> {
                    Row(horizontalArrangement = Arrangement.SpaceBetween, modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                        Text(item.name, style = MaterialTheme.typography.bodyMedium, modifier = Modifier.weight(1f))
                        Text(formatSize(item.size), style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f))
                        if (item.progress >= 1f && item.localFile != null && Desktop.isDesktopSupported()) {
                            Spacer(modifier = Modifier.width(4.dp))
                            IconButton(onClick = {
                                runCatching { Desktop.getDesktop().open(item.localFile) }
                            }) {
                                Icon(Icons.Default.FolderOpen, contentDescription = "Open", tint = MaterialTheme.colorScheme.primary)
                            }
                        }
                    }
                    if (item.progress < 1f) {
                        LinearProgressIndicator(progress = { item.progress }, modifier = Modifier.fillMaxWidth())
                        Text(
                            "${item.receivedChunks}/${item.totalChunks} chunks",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                        )
                    } else {
                        item.localFile?.let {
                            Text(it.absolutePath, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f))
                        }
                    }
                }

                is TransferItem.LogcatStream -> {
                    Text("Logcat — ${item.lines.size} lines", style = MaterialTheme.typography.bodyMedium)
                    item.lines.takeLast(5).forEach {
                        SelectionContainer { Text(it, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)) }
                    }
                }
            }
        }
    }
}

private fun formatSize(bytes: Long): String {
    if (bytes < 1024) return "$bytes B"
    val kb = bytes / 1024.0
    if (kb < 1024) return "${DecimalFormat("0.#").format(kb)} KB"
    val mb = kb / 1024.0
    if (mb < 1024) return "${DecimalFormat("0.#").format(mb)} MB"
    return "${DecimalFormat("0.##").format(mb / 1024.0)} GB"
}
