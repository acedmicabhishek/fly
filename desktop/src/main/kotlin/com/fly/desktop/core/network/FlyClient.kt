package com.fly.desktop.core.network

import com.fly.desktop.core.discovery.NetworkDevice
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.net.Socket

class FlyClient(private val scope: CoroutineScope) {

    suspend fun connect(device: NetworkDevice): PeerConnection = withContext(Dispatchers.IO) {
        val socket = Socket(device.host, device.port)
        PeerConnection(socket, device, scope).also { it.start() }
    }

    suspend fun connect(host: String, port: Int): PeerConnection = withContext(Dispatchers.IO) {
        val socket = Socket(host, port)
        PeerConnection(socket, null, scope).also { it.start() }
    }
}
