package com.fly.app.core.discovery

import android.content.Context
import android.net.wifi.WifiManager
import kotlinx.coroutines.*
import kotlin.coroutines.coroutineContext
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.json.JSONObject
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.util.concurrent.ConcurrentHashMap

class DeviceDiscovery(private val context: Context, private val port: Int) {

    private val wifiManager = context.applicationContext.getSystemService(WifiManager::class.java)

    private val _devices = MutableStateFlow<Map<String, NetworkDevice>>(emptyMap())
    val devices: StateFlow<Map<String, NetworkDevice>> = _devices.asStateFlow()

    private var scope: CoroutineScope? = null
    private var multicastLock: WifiManager.MulticastLock? = null
    private var deviceName: String = ""

    private val lastSeen = ConcurrentHashMap<String, Long>()

    fun start(deviceName: String) {
        this.deviceName = deviceName
        multicastLock = wifiManager.createMulticastLock("fly").also {
            it.setReferenceCounted(true)
            it.acquire()
        }
        scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
        scope?.launch { sender() }
        scope?.launch { receiver() }
        scope?.launch { pruner() }
    }

    fun stop() {
        scope?.cancel()
        scope = null
        multicastLock?.let { if (it.isHeld) it.release() }
        multicastLock = null
    }

    private suspend fun sender() {
        val socket = DatagramSocket()
        socket.broadcast = true
        val broadcast = InetAddress.getByName("255.255.255.255")
        try {
            while (coroutineContext.isActive) {
                val payload = JSONObject().apply {
                    put("v", 1)
                    put("name", deviceName)
                    put("port", port)
                    put("platform", "android")
                }.toString().toByteArray()
                val dp = DatagramPacket(payload, payload.size, broadcast, DISC_PORT)
                runCatching { socket.send(dp) }
                delay(2000)
            }
        } finally {
            socket.close()
        }
    }

    private suspend fun receiver() {
        val socket = DatagramSocket(DISC_PORT)
        socket.broadcast = true
        socket.soTimeout = 1000
        val buf = ByteArray(2048)
        try {
            while (coroutineContext.isActive) {
                val dp = DatagramPacket(buf, buf.size)
                runCatching { socket.receive(dp) }.onSuccess {
                    runCatching {
                        val j = JSONObject(String(dp.data, 0, dp.length))
                        if (j.optInt("v") != 1) return@onSuccess
                        val name = j.optString("name")
                            .takeIf { it.isNotEmpty() && it != deviceName } ?: return@onSuccess
                        val pport = j.optInt("port").takeIf { it > 0 } ?: return@onSuccess
                        val platform = j.optString("platform", "unknown")
                        val host = dp.address.hostAddress
                            ?.replace(Regex("%.*"), "") ?: return@onSuccess
                        val device = NetworkDevice(name, name, platform, host, pport)
                        lastSeen[name] = System.currentTimeMillis()
                        val current = _devices.value
                        if (!current.containsKey(name) || current[name] != device)
                            _devices.value = current + (name to device)
                    }
                }
            }
        } finally {
            socket.close()
        }
    }

    private suspend fun pruner() {
        while (coroutineContext.isActive) {
            delay(2000)
            val now = System.currentTimeMillis()
            val stale = lastSeen.entries
                .filter { now - it.value >= TIMEOUT_MS }
                .map { it.key }
            if (stale.isNotEmpty()) {
                stale.forEach { lastSeen.remove(it) }
                _devices.value = _devices.value - stale.toSet()
            }
        }
    }

    companion object {
        private const val DISC_PORT  = 5801
        private const val TIMEOUT_MS = 8000L
    }
}
