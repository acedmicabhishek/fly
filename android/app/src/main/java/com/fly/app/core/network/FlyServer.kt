package com.fly.app.core.network

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.launch
import java.net.ServerSocket

class FlyServer(private val port: Int, private val scope: CoroutineScope) {

    private var serverSocket: ServerSocket? = null

    private val _connections = MutableSharedFlow<PeerConnection>(extraBufferCapacity = 16)
    val connections: SharedFlow<PeerConnection> = _connections

    fun start() {
        scope.launch(Dispatchers.IO) {
            runCatching {
                serverSocket = ServerSocket(port)
                while (true) {
                    val socket = serverSocket?.accept() ?: break
                    val conn = PeerConnection(socket, null, scope)
                    conn.start()
                    _connections.emit(conn)
                }
            }
        }
    }

    fun stop() {
        runCatching { serverSocket?.close() }
        serverSocket = null
    }
}
