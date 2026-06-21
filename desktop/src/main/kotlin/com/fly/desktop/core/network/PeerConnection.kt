package com.fly.desktop.core.network

import com.fly.desktop.core.discovery.NetworkDevice
import com.fly.desktop.core.protocol.FlyMessage
import com.fly.desktop.core.protocol.readFrame
import com.fly.desktop.core.protocol.toFrame
import com.fly.desktop.core.protocol.toMessage
import com.fly.desktop.core.protocol.writeFrame
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.DataInputStream
import java.io.DataOutputStream
import java.net.Socket

class PeerConnection(
    private val socket: Socket,
    val device: NetworkDevice?,
    private val scope: CoroutineScope
) {
    private val output = DataOutputStream(socket.getOutputStream().buffered(65536))
    private val input = DataInputStream(socket.getInputStream().buffered(65536))

    private val _incoming = MutableSharedFlow<Pair<FlyMessage, ByteArray>>(extraBufferCapacity = 128)
    val incoming: SharedFlow<Pair<FlyMessage, ByteArray>> = _incoming

    private val _disconnected = MutableSharedFlow<Unit>(extraBufferCapacity = 1)
    val disconnected: SharedFlow<Unit> = _disconnected

    val isConnected: Boolean get() = socket.isConnected && !socket.isClosed

    fun start() {
        scope.launch(Dispatchers.IO) {
            runCatching {
                while (isConnected) {
                    val frame = readFrame(input)
                    val msg = frame.toMessage() ?: continue
                    _incoming.emit(msg to frame.body)
                }
            }
            _disconnected.emit(Unit)
        }
    }

    suspend fun send(message: FlyMessage, body: ByteArray = ByteArray(0)) = withContext(Dispatchers.IO) {
        runCatching { writeFrame(output, message.toFrame(body)) }
    }

    fun close() = runCatching { socket.close() }
}
