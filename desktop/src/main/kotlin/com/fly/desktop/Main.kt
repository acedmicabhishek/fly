package com.fly.desktop

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.awt.ComposeWindow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.Window
import androidx.compose.ui.window.application
import androidx.compose.ui.window.rememberWindowState
import com.fly.desktop.ui.FlyApp
import com.fly.desktop.ui.theme.FlyTheme
import com.fly.desktop.viewmodel.AppViewModel
import java.awt.datatransfer.DataFlavor
import java.awt.dnd.DnDConstants
import java.awt.dnd.DropTarget
import java.awt.dnd.DropTargetDropEvent
import java.io.File

fun main() = application {
    val vm = remember { AppViewModel() }
    val windowState = rememberWindowState()

    DisposableEffect(Unit) {
        onDispose { vm.close() }
    }

    Window(
        onCloseRequest = ::exitApplication,
        title = "Fly",
        state = windowState
    ) {
        val themeMode by vm.themeMode.collectAsState()

        window.dropTarget = buildDropTarget(vm, window)

        FlyTheme(themeMode = themeMode) {
            FlyApp(vm = vm, window = window)
        }
    }
}

private fun buildDropTarget(vm: AppViewModel, window: ComposeWindow): DropTarget {
    return object : DropTarget() {
        @Synchronized
        override fun drop(event: DropTargetDropEvent) {
            event.acceptDrop(DnDConstants.ACTION_COPY)
            val transferable = event.transferable
            var handled = false

            if (transferable.isDataFlavorSupported(DataFlavor.javaFileListFlavor)) {
                @Suppress("UNCHECKED_CAST")
                val files = transferable.getTransferData(DataFlavor.javaFileListFlavor) as List<File>
                files.forEach { vm.sendFile(it) }
                handled = true
            }

            if (!handled && transferable.isDataFlavorSupported(DataFlavor.stringFlavor)) {
                val text = transferable.getTransferData(DataFlavor.stringFlavor) as? String
                if (!text.isNullOrBlank()) vm.sendText(text)
            }

            event.dropComplete(true)
        }
    }
}
