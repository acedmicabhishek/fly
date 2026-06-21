package com.fly.app.core.network

import com.fly.app.core.discovery.NetworkDevice
import com.fly.app.core.protocol.Frame
import com.fly.app.core.protocol.FlyMessage
import com.fly.app.core.protocol.readFrame
import com.fly.app.core.protocol.toFrame
import com.fly.app.core.protocol.toMessage
import com.fly.app.core.protocol.writeFrame
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
        }
    }

    suspend fun send(message: FlyMessage, body: ByteArray = ByteArray(0)) = withContext(Dispatchers.IO) {
        runCatching { writeFrame(output, message.toFrame(body)) }
    }

    fun close() = runCatching { socket.close() }
}
