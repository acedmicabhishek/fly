package com.fly.desktop.viewmodel

import com.fly.desktop.core.discovery.DeviceDiscovery
import com.fly.desktop.core.discovery.NetworkDevice
import com.fly.desktop.core.network.FlyClient
import com.fly.desktop.core.network.FlyServer
import com.fly.desktop.core.network.PeerConnection
import com.fly.desktop.core.protocol.FlyMessage
import com.fly.desktop.core.transfer.TransferManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import java.io.File
import java.net.InetAddress
import java.util.prefs.Preferences

class AppViewModel : AutoCloseable {

    private val prefs = Preferences.userNodeForPackage(AppViewModel::class.java)
    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())

    val port get() = prefs.getInt("port", 5800)
    val deviceName get() = prefs.get("device_name", InetAddress.getLocalHost().hostName)
    val autoConnect get() = prefs.getBoolean("auto_connect", false)
    val chunkThresholdMb get() = prefs.getInt("chunk_threshold_mb", 64)
    val chunkSizeMb get() = prefs.getInt("chunk_size_mb", 4)
    val downloadDir get() = prefs.get("download_dir", "${System.getProperty("user.home")}/Downloads/fly")

    private val _themeMode = MutableStateFlow(prefs.get("theme_mode", "system"))
    val themeMode: StateFlow<String> = _themeMode.asStateFlow()

    private val discovery = DeviceDiscovery(port)
    private val server = FlyServer(port, scope)
    private val client = FlyClient(scope)

    val transferManager = TransferManager(
        scope = scope,
        chunkThresholdBytes = { chunkThresholdMb.toLong() * 1024 * 1024 },
        chunkSizeBytes = { chunkSizeMb * 1024 * 1024 },
        downloadDir = { downloadDir },
        deviceName = { deviceName }
    )

    val discoveredDevices: StateFlow<List<NetworkDevice>> = discovery.devices
        .map { it.values.toList().filterNot { dev -> dev.name == deviceName } }
        .stateIn(scope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val _activeConnection = MutableStateFlow<PeerConnection?>(null)
    val activeConnection: StateFlow<PeerConnection?> = _activeConnection.asStateFlow()

    private val _status = MutableStateFlow("Ready — waiting for connections on port $port")
    val status: StateFlow<String> = _status.asStateFlow()

    init {
        server.start()
        discovery.start(deviceName)
        scope.launch {
            server.connections.collect { conn ->
                onConnected(conn, "incoming device")
            }
        }
    }

    fun connectTo(device: NetworkDevice) {
        scope.launch {
            _status.value = "Connecting to ${device.name}..."
            runCatching {
                val conn = client.connect(device)
                conn.send(FlyMessage.Hello(deviceName))
                onConnected(conn, device.name)
            }.onFailure { _status.value = "Failed: ${it.message}" }
        }
    }

    fun connectManual(host: String, port: Int) {
        scope.launch {
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
        scope.launch {
            conn.disconnected.collect {
                if (_activeConnection.value === conn) {
                    _activeConnection.value = null
                    _status.value = "Disconnected — listening on port $port"
                }
            }
        }
    }

    fun sendText(text: String) {
        val conn = _activeConnection.value ?: return
        scope.launch { transferManager.sendText(conn, text) }
    }

    fun sendFile(file: File) {
        val conn = _activeConnection.value ?: return
        scope.launch { transferManager.sendFile(conn, file) }
    }

    fun disconnect() {
        _activeConnection.value?.close()
        _activeConnection.value = null
        _status.value = "Disconnected — listening on port $port"
    }

    fun saveDeviceName(name: String) { prefs.put("device_name", name) }
    fun saveAutoConnect(v: Boolean) { prefs.putBoolean("auto_connect", v) }
    fun saveChunkThreshold(mb: Int) { prefs.putInt("chunk_threshold_mb", mb) }
    fun saveChunkSize(mb: Int) { prefs.putInt("chunk_size_mb", mb) }
    fun saveThemeMode(mode: String) { prefs.put("theme_mode", mode); _themeMode.value = mode }
    fun savePort(p: Int) { prefs.putInt("port", p) }
    fun saveDownloadDir(dir: String) { prefs.put("download_dir", dir) }

    override fun close() {
        server.stop()
        discovery.stop()
        scope.cancel()
    }
}
