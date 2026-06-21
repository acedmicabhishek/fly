package com.fly.app.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material3.Button
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
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
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import com.fly.app.ui.components.DeviceCard
import com.fly.app.viewmodel.AppViewModel

@Composable
fun DevicesScreen(vm: AppViewModel, modifier: Modifier = Modifier) {
    val devices by vm.discoveredDevices.collectAsState()
    val status by vm.status.collectAsState()
    val activeConn by vm.activeConnection.collectAsState()
    var showManual by remember { mutableStateOf(false) }
    var manualInput by remember { mutableStateOf("192.168.1.x:5800") }

    Column(modifier = modifier.fillMaxSize().padding(16.dp)) {
        Row(
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween,
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(
                status,
                style = MaterialTheme.typography.labelMedium,
                color = MaterialTheme.colorScheme.primary
            )
            if (activeConn != null) {
                OutlinedButton(onClick = { vm.disconnect() }) { Text("Disconnect") }
            }
        }
        Spacer(modifier = Modifier.height(8.dp))
        OutlinedButton(
            onClick = { showManual = !showManual },
            modifier = Modifier.fillMaxWidth()
        ) {
            Text(if (showManual) "Hide manual connect" else "Manual connect")
        }
        if (showManual) {
            Spacer(modifier = Modifier.height(8.dp))
            OutlinedTextField(
                value = manualInput,
                onValueChange = { manualInput = it },
                label = { Text("host:port") },
                singleLine = true,
                modifier = Modifier.fillMaxWidth(),
                keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
                keyboardActions = KeyboardActions(onDone = { connectManual(manualInput, vm) })
            )
            Spacer(modifier = Modifier.height(4.dp))
            Button(
                onClick = { connectManual(manualInput, vm) },
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Connect")
            }
        }
        Spacer(modifier = Modifier.height(16.dp))
        if (devices.isEmpty()) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                CircularProgressIndicator(
                    modifier = Modifier.size(16.dp),
                    strokeWidth = 2.dp,
                    color = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(10.dp))
                Text(
                    "Scanning for devices on this network...",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.5f)
                )
            }
        } else {
            LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                items(devices, key = { it.id }) { device ->
                    DeviceCard(
                        device = device,
                        isActive = activeConn?.device?.id == device.id,
                        onClick = { vm.connectTo(device) }
                    )
                }
            }
        }
    }
}

private fun connectManual(input: String, vm: AppViewModel) {
    val parts = input.trim().split(":")
    val host = parts.getOrElse(0) { "localhost" }.trim()
    val port = parts.getOrElse(1) { "5800" }.trim().toIntOrNull() ?: 5800
    vm.connectManual(host, port)
}
