package com.fly.desktop.ui

import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.width
import androidx.compose.material3.HorizontalDivider
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.awt.ComposeWindow
import androidx.compose.ui.unit.dp
import com.fly.desktop.ui.panels.DevicesSidebar
import com.fly.desktop.ui.panels.SettingsDialog
import com.fly.desktop.ui.panels.TransferPanel
import com.fly.desktop.viewmodel.AppViewModel

@Composable
fun FlyApp(vm: AppViewModel, window: ComposeWindow) {
    var showSettings by remember { mutableStateOf(false) }

    Row(modifier = Modifier.fillMaxSize()) {
        DevicesSidebar(
            vm = vm,
            onOpenSettings = { showSettings = true },
            modifier = Modifier.width(180.dp)
        )
        HorizontalDivider(modifier = Modifier.width(1.dp).fillMaxSize())
        TransferPanel(
            vm = vm,
            window = window,
            modifier = Modifier.weight(1f)
        )
    }

    if (showSettings) {
        SettingsDialog(vm = vm, onDismiss = { showSettings = false })
    }
}
