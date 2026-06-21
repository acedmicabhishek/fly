package com.fly.app.viewmodel

import android.app.Application
import android.content.Context
import android.net.Uri
import android.os.Build
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.fly.app.core.debug.LogcatCollector
import com.fly.app.core.discovery.DeviceDiscovery
import com.fly.app.core.discovery.NetworkDevice
import com.fly.app.core.network.FlyClient
import com.fly.app.core.network.FlyServer
import com.fly.app.core.network.PeerConnection
import com.fly.app.core.protocol.FlyMessage
import com.fly.app.core.transfer.TransferManager
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

class AppViewModel(app: Application) : AndroidViewModel(app) {

    private val prefs = app.getSharedPreferences("fly_prefs", Context.MODE_PRIVATE)

    val port get() = prefs.getInt("port", 5800)
    val deviceName get() = prefs.getString("device_name", Build.MODEL) ?: Build.MODEL
    val autoConnect get() = prefs.getBoolean("auto_connect", false)
    val chunkThresholdMb get() = prefs.getInt("chunk_threshold_mb", 64)
    val chunkSizeMb get() = prefs.getInt("chunk_size_mb", 4)
    val themeMode get() = prefs.getString("theme_mode", "system") ?: "system"

    private val discovery = DeviceDiscovery(app, port)
    private val server = FlyServer(port, viewModelScope)
    private val client = FlyClient(viewModelScope)
    val logcatCollector = LogcatCollector(viewModelScope)

    val transferManager = TransferManager(
        context = app,
        scope = viewModelScope,
        chunkThresholdBytes = { chunkThresholdMb.toLong() * 1024 * 1024 },
        chunkSizeBytes = { chunkSizeMb * 1024 * 1024 },
        deviceName = { deviceName }
    )

    val discoveredDevices: StateFlow<List<NetworkDevice>> = discovery.devices
        .map { it.values.toList().filterNot { dev -> dev.name == deviceName } }
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val _activeConnection = MutableStateFlow<PeerConnection?>(null)
    val activeConnection: StateFlow<PeerConnection?> = _activeConnection.asStateFlow()

    private val _status = MutableStateFlow("Idle")
    val status: StateFlow<String> = _status.asStateFlow()

    private val _themeModeFlow = MutableStateFlow(themeMode)
    val themeModeFlow: StateFlow<String> = _themeModeFlow.asStateFlow()

    init {
        server.start()
        discovery.start(deviceName)
        viewModelScope.launch {
            server.connections.collect { conn ->
                onConnected(conn, "incoming device")
            }
        }
    }

    fun connectTo(device: NetworkDevice) {
        viewModelScope.launch {
            _status.value = "Connecting to ${device.name}..."
            runCatching {
                val conn = client.connect(device)
                conn.send(FlyMessage.Hello(deviceName))
                onConnected(conn, device.name)
            }.onFailure { _status.value = "Failed: ${it.message}" }
        }
    }

    fun connectManual(host: String, port: Int) {
        viewModelScope.launch {
            _status.value = "Connecting to $host..."
            runCatching {
                val conn = client.connect(host, port)
                conn.send(FlyMessage.Hello(deviceName))
                onConnected(conn, "$host:$port")
            }.onFailure { _status.value = "Failed: ${it.message}" }
        }
    }

    private fun onConnected(conn: PeerConnection, name: String) {
        _activeConnection.value = conn
        transferManager.addConnection(conn, name)
        _status.value = "Connected to $name"
        if (autoConnect) prefs.edit().putString("last_host_name", name).apply()
    }

    fun sendText(text: String) {
        val conn = _activeConnection.value ?: return
        viewModelScope.launch { transferManager.sendText(conn, text) }
    }

    fun sendUri(uri: Uri) {
        val conn = _activeConnection.value ?: return
        viewModelScope.launch { transferManager.sendUri(conn, uri) }
    }

    fun sendLogcat(filter: String = "") {
        val conn = _activeConnection.value ?: return
        logcatCollector.startStreaming(conn, filter)
    }

    fun disconnect() {
        _activeConnection.value?.close()
        _activeConnection.value = null
        logcatCollector.stop()
        _status.value = "Disconnected"
    }

    fun saveDeviceName(name: String) {
        prefs.edit().putString("device_name", name).apply()
    }

    fun saveAutoConnect(enabled: Boolean) {
        prefs.edit().putBoolean("auto_connect", enabled).apply()
    }

    fun saveChunkThreshold(mb: Int) {
        prefs.edit().putInt("chunk_threshold_mb", mb).apply()
    }

    fun saveChunkSize(mb: Int) {
        prefs.edit().putInt("chunk_size_mb", mb).apply()
    }

    fun saveThemeMode(mode: String) {
        prefs.edit().putString("theme_mode", mode).apply()
        _themeModeFlow.value = mode
    }

    fun resetSettings() {
        prefs.edit().clear().apply()
        _themeModeFlow.value = "system"
    }

    override fun onCleared() {
        super.onCleared()
        server.stop()
        discovery.stop()
        logcatCollector.stop()
    }
}
