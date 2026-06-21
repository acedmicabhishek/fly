package com.fly.desktop.ui.panels

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.fly.desktop.ui.components.DeviceCard
import com.fly.desktop.viewmodel.AppViewModel

@Composable
fun DevicesSidebar(vm: AppViewModel, onOpenSettings: () -> Unit, modifier: Modifier = Modifier) {
    val devices by vm.discoveredDevices.collectAsState()
    val activeConn by vm.activeConnection.collectAsState()
    val status by vm.status.collectAsState()
    var manualInput by remember { mutableStateOf("") }
    var showManual by remember { mutableStateOf(false) }

    Column(
        modifier = modifier.fillMaxHeight().padding(10.dp),
        verticalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        Text("Fly", style = MaterialTheme.typography.titleMedium, color = MaterialTheme.colorScheme.primary)
        Text(
            status,
            style = MaterialTheme.typography.labelSmall,
            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
        )
        HorizontalDivider()

        if (activeConn != null) {
            OutlinedButton(onClick = { vm.disconnect() }, modifier = Modifier.fillMaxWidth()) {
                Text("Disconnect")
            }
        }

        OutlinedButton(
            onClick = { showManual = !showManual },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (showManual) "Hide manual" else "Manual / localhost")
        }

        if (showManual) {
            OutlinedTextField(
                value = manualInput,
                onValueChange = { manualInput = it },
                label = { Text("host:port") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth()
            )
            Button(
                onClick = {
                    val parts = manualInput.trim().split(":")
                    val host = parts.getOrElse(0) { "localhost" }.trim()
                    val port = parts.getOrElse(1) { "5800" }.trim().toIntOrNull() ?: 5800
                    vm.connectManual(host, port)
                },
                modifier = Modifier.fillMaxWidth()
            ) { Text("Connect") }
        }

        HorizontalDivider()

        Text(
            "Nearby",
            style = MaterialTheme.typography.labelMedium,
            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
        )

        LazyColumn(modifier = Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(4.dp)) {
            if (devices.isEmpty()) {
                item {
                    Row(
                        verticalAlignment = Alignment.CenterVertically,
                        modifier = Modifier.padding(vertical = 4.dp)
                    ) {
                        CircularProgressIndicator(
                            modifier = Modifier.size(12.dp),
                            strokeWidth = 1.5.dp,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                        )
                        Spacer(modifier = Modifier.width(8.dp))
                        Text(
                            "Scanning...",
                            style = MaterialTheme.typography.bodySmall,
                            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.4f)
                        )
                    }
                }
            }
            items(devices, key = { it.id }) { device ->
                DeviceCard(
                    device = device,
                    isActive = activeConn?.device?.id == device.id,
                    onClick = { vm.connectTo(device) }
                )
            }
        }

        HorizontalDivider()
        Spacer(modifier = Modifier.height(2.dp))
        OutlinedButton(onClick = onOpenSettings, modifier = Modifier.fillMaxWidth()) {
            Text("Settings")
        }
    }
}
