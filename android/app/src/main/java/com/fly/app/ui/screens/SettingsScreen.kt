package com.fly.app.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Slider
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
import com.fly.app.viewmodel.AppViewModel

@Composable
fun SettingsScreen(vm: AppViewModel, modifier: Modifier = Modifier) {
    var deviceName by remember { mutableStateOf(vm.deviceName) }
    var autoConnect by remember { mutableStateOf(vm.autoConnect) }
    var thresholdMb by remember { mutableFloatStateOf(vm.chunkThresholdMb.toFloat()) }
    var chunkSizeMb by remember { mutableFloatStateOf(vm.chunkSizeMb.toFloat()) }
    val themeMode by vm.themeModeFlow.collectAsState()

    Column(
        modifier = modifier
            .fillMaxSize()
            .verticalScroll(rememberScrollState())
            .padding(horizontal = 16.dp, vertical = 8.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text("Device", style = MaterialTheme.typography.titleSmall, color = MaterialTheme.colorScheme.primary)
        OutlinedTextField(
            value = deviceName,
            onValueChange = { deviceName = it },
            label = { Text("Device name") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true
        )
        Button(onClick = { vm.saveDeviceName(deviceName) }, modifier = Modifier.fillMaxWidth()) {
            Text("Save name")
        }

        HorizontalDivider()
        Text("Connection", style = MaterialTheme.typography.titleSmall, color = MaterialTheme.colorScheme.primary)
        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            Text("Auto-connect to last device", modifier = Modifier.weight(1f))
            Switch(checked = autoConnect, onCheckedChange = { autoConnect = it; vm.saveAutoConnect(it) })
        }
        Text(
            "Port: ${vm.port}",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurface.copy(alpha = 0.6f)
        )

        HorizontalDivider()
        Text("Transfer", style = MaterialTheme.typography.titleSmall, color = MaterialTheme.colorScheme.primary)
        Text(
            "Chunk threshold: ${thresholdMb.toInt()} MB — files above this use chunked streaming",
            style = MaterialTheme.typography.bodySmall
        )
        Slider(
            value = thresholdMb,
            onValueChange = { thresholdMb = it },
            valueRange = 16f..512f,
            steps = 30,
            onValueChangeFinished = { vm.saveChunkThreshold(thresholdMb.toInt()) }
        )
        Text(
            "Chunk size: ${chunkSizeMb.toInt()} MB per chunk",
            style = MaterialTheme.typography.bodySmall
        )
        Slider(
            value = chunkSizeMb,
            onValueChange = { chunkSizeMb = it },
            valueRange = 1f..32f,
            steps = 30,
            onValueChangeFinished = { vm.saveChunkSize(chunkSizeMb.toInt()) }
        )

        HorizontalDivider()
        Text("Appearance", style = MaterialTheme.typography.titleSmall, color = MaterialTheme.colorScheme.primary)
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            listOf("system" to "System", "dark" to "Dark", "light" to "Light").forEach { (key, label) ->
                if (themeMode == key) {
                    Button(onClick = {}) { Text(label) }
                } else {
                    OutlinedButton(onClick = { vm.saveThemeMode(key) }) { Text(label) }
                }
            }
        }

        HorizontalDivider()
        Button(
            onClick = { vm.resetSettings() },
            modifier = Modifier.fillMaxWidth(),
            colors = ButtonDefaults.buttonColors(
                containerColor = MaterialTheme.colorScheme.errorContainer,
                contentColor = MaterialTheme.colorScheme.onErrorContainer
            )
        ) {
            Text("Reset to Defaults")
        }
    }
}
