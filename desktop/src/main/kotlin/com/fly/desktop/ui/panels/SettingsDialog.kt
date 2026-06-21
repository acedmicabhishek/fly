package com.fly.desktop.ui.panels

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Divider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Dialog
import com.fly.desktop.viewmodel.AppViewModel

@Composable
fun SettingsDialog(vm: AppViewModel, onDismiss: () -> Unit) {
    var deviceName by remember { mutableStateOf(vm.deviceName) }
    var autoConnect by remember { mutableStateOf(vm.autoConnect) }
    var thresholdMb by remember { mutableFloatStateOf(vm.chunkThresholdMb.toFloat()) }
    var chunkSizeMb by remember { mutableFloatStateOf(vm.chunkSizeMb.toFloat()) }
    var portText by remember { mutableStateOf(vm.port.toString()) }
    var downloadDir by remember { mutableStateOf(vm.downloadDir) }
    val themeMode by vm.themeMode.collectAsState()

    val portValid = portText.toIntOrNull()?.let { it in 1024..65535 } == true

    Dialog(onDismissRequest = onDismiss) {
        Surface(
            shape = MaterialTheme.shapes.extraLarge,
            tonalElevation = 6.dp,
            modifier = Modifier.width(540.dp)
        ) {
            Column(
                modifier = Modifier
                    .padding(28.dp)
                    .verticalScroll(rememberScrollState()),
                verticalArrangement = Arrangement.spacedBy(18.dp)
            ) {
                Text("Settings", style = MaterialTheme.typography.headlineSmall)

                Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    Text("Device", style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.primary)
                    OutlinedTextField(
                        value = deviceName,
                        onValueChange = { deviceName = it },
                        label = { Text("Device name") },
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = true
                    )
                }

                Divider()

                Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    Text("Network", style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.primary)

                    OutlinedTextField(
                        value = portText,
                        onValueChange = { portText = it },
                        label = { Text("Port") },
                        isError = !portValid,
                        supportingText = if (!portValid) {
                            { Text("Valid range: 1024–65535") }
                        } else {
                            { Text("Requires restart") }
                        },
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = true
                    )

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        verticalAlignment = Alignment.CenterVertically,
                        horizontalArrangement = Arrangement.SpaceBetween
                    ) {
                        Text("Auto-connect to last device")
                        Switch(checked = autoConnect, onCheckedChange = { autoConnect = it })
                    }
                }

                Divider()

                Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    Text("Transfer", style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.primary)

                    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        Text(
                            "Chunk threshold: ${thresholdMb.toInt()} MB",
                            style = MaterialTheme.typography.bodySmall
                        )
                        Text(
                            "Files above this size use chunked streaming",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                        )
                        Slider(
                            value = thresholdMb,
                            onValueChange = { thresholdMb = it },
                            valueRange = 16f..512f,
                            steps = 30
                        )
                    }

                    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        Text(
                            "Chunk size: ${chunkSizeMb.toInt()} MB per chunk",
                            style = MaterialTheme.typography.bodySmall
                        )
                        Slider(
                            value = chunkSizeMb,
                            onValueChange = { chunkSizeMb = it },
                            valueRange = 1f..32f,
                            steps = 30
                        )
                    }

                    OutlinedTextField(
                        value = downloadDir,
                        onValueChange = { downloadDir = it },
                        label = { Text("Download directory") },
                        modifier = Modifier.fillMaxWidth(),
                        singleLine = false,
                        maxLines = 2
                    )
                }

                Divider()

                Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    Text("Appearance", style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.primary)
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        listOf("system" to "System", "dark" to "Dark", "light" to "Light").forEach { (key, label) ->
                            if (themeMode == key) {
                                Button(onClick = {}) { Text(label) }
                            } else {
                                OutlinedButton(onClick = { vm.saveThemeMode(key) }) { Text(label) }
                            }
                        }
                    }
                }

                Spacer(modifier = Modifier.height(8.dp))
                Row(
                    horizontalArrangement = Arrangement.End,
                    modifier = Modifier.fillMaxWidth()
                ) {
                    OutlinedButton(onClick = onDismiss) { Text("Cancel") }
                    Spacer(modifier = Modifier.width(12.dp))
                    Button(
                        enabled = portValid,
                        onClick = {
                            vm.saveDeviceName(deviceName)
                            vm.saveAutoConnect(autoConnect)
                            vm.saveChunkThreshold(thresholdMb.toInt())
                            vm.saveChunkSize(chunkSizeMb.toInt())
                            vm.savePort(portText.toInt())
                            vm.saveDownloadDir(downloadDir)
                            onDismiss()
                        }
                    ) { Text("Save") }
                }
            }
        }
    }
}
