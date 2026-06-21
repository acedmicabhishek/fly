package com.fly.app.core.transfer

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import com.fly.app.core.network.PeerConnection
import com.fly.app.core.protocol.FlyMessage
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.util.UUID
import kotlin.math.ceil

class TransferManager(
    private val context: Context,
    private val scope: CoroutineScope,
    private val chunkThresholdBytes: () -> Long,
    private val chunkSizeBytes: () -> Int,
    private val deviceName: () -> String = { "My Phone" }
) {
    private val _transfers = MutableStateFlow<List<TransferItem>>(emptyList())
    val transfers: StateFlow<List<TransferItem>> = _transfers.asStateFlow()

    private val pendingChunks = mutableMapOf<String, MutableList<ByteArray>>()
    private val pendingFileInfos = mutableMapOf<String, FlyMessage.FileStart>()

    fun addConnection(conn: PeerConnection, deviceName: String) {
        var actualDeviceName = deviceName
        scope.launch {
            conn.incoming.collect { (msg, body) ->
                if (msg is FlyMessage.Hello && actualDeviceName == "incoming device") {
                    actualDeviceName = msg.deviceName
                }
                handleIncoming(msg, body, actualDeviceName, conn)
            }
        }
    }

    private suspend fun handleIncoming(msg: FlyMessage, body: ByteArray, deviceName: String, conn: PeerConnection) {
        when (msg) {
            is FlyMessage.Hello -> {}

            is FlyMessage.Text -> prepend(
                TransferItem.Text(msg.id, deviceName, true, msg.content)
            )

            is FlyMessage.FileOffer -> {
                val file = saveFile(msg.name, body)
                prepend(TransferItem.File(msg.id, deviceName, true, msg.name, msg.mime, msg.size, file.absolutePath, 1f))
                conn.send(FlyMessage.Ack(msg.id))
            }

            is FlyMessage.FileStart -> {
                pendingFileInfos[msg.id] = msg
                pendingChunks[msg.id] = mutableListOf()
                prepend(TransferItem.File(msg.id, deviceName, true, msg.name, msg.mime, msg.size, null, 0f, msg.totalChunks, 0))
            }

            is FlyMessage.Chunk -> {
                pendingChunks[msg.id]?.add(body)
                val info = pendingFileInfos[msg.id] ?: return
                val received = pendingChunks[msg.id]?.size ?: 0
                updateFileProgress(msg.id, received.toFloat() / info.totalChunks, received)
            }

            is FlyMessage.FileDone -> {
                val chunks = pendingChunks.remove(msg.id) ?: return
                val info = pendingFileInfos.remove(msg.id) ?: return
                val fullData = chunks.fold(ByteArray(0)) { acc, chunk -> acc + chunk }
                val file = saveFile(info.name, fullData)
                updateTransfer(msg.id) { item ->
                    if (item is TransferItem.File) item.copy(localPath = file.absolutePath, progress = 1f)
                    else item
                }
                conn.send(FlyMessage.Ack(msg.id))
            }

            is FlyMessage.Logcat -> {
                val existing = _transfers.value.find { it.id == msg.id } as? TransferItem.LogcatStream
                if (existing != null) {
                    updateTransfer(msg.id) { item ->
                        if (item is TransferItem.LogcatStream) item.copy(lines = item.lines + msg.line)
                        else item
                    }
                } else {
                    prepend(TransferItem.LogcatStream(msg.id, deviceName, true, listOf(msg.line)))
                }
            }

            else -> {}
        }
    }

    suspend fun sendText(conn: PeerConnection, text: String) {
        val id = UUID.randomUUID().toString()
        conn.send(FlyMessage.Text(id, text))
        prepend(TransferItem.Text(id, deviceName(), false, text))
    }

    suspend fun sendUri(conn: PeerConnection, uri: Uri) {
        val id = UUID.randomUUID().toString()
        val name = resolveFileName(uri)
        val mime = context.contentResolver.getType(uri) ?: "application/octet-stream"
        val size = resolveFileSize(uri)
        val threshold = chunkThresholdBytes()

        if (size <= threshold) {
            val data = context.contentResolver.openInputStream(uri)?.readBytes() ?: return
            conn.send(FlyMessage.FileOffer(id, name, mime, size), data)
            prepend(TransferItem.File(id, deviceName(), false, name, mime, size, null, 1f))
        } else {
            val chunkSize = chunkSizeBytes()
            val totalChunks = ceil(size.toDouble() / chunkSize).toInt()
            conn.send(FlyMessage.FileStart(id, name, mime, size, totalChunks, chunkSize))
            prepend(TransferItem.File(id, deviceName(), false, name, mime, size, null, 0f, totalChunks, 0))
            withContext(Dispatchers.IO) {
                context.contentResolver.openInputStream(uri)?.use { stream ->
                    val buffer = ByteArray(chunkSize)
                    var index = 0
                    var bytesRead: Int
                    while (stream.read(buffer).also { bytesRead = it } != -1) {
                        conn.send(FlyMessage.Chunk(id, index), buffer.copyOf(bytesRead))
                        val received = index + 1
                        updateFileProgress(id, received.toFloat() / totalChunks, received)
                        index++
                    }
                }
            }
            conn.send(FlyMessage.FileDone(id))
            updateFileProgress(id, 1f, totalChunks)
        }
    }

    private fun saveFile(name: String, data: ByteArray): File {
        val dir = File(context.filesDir, "received").also { it.mkdirs() }
        return File(dir, name).also { it.writeBytes(data) }
    }

    private fun prepend(item: TransferItem) {
        _transfers.update { listOf(item) + it }
    }

    private fun updateFileProgress(id: String, progress: Float, received: Int = 0) {
        updateTransfer(id) { item ->
            if (item is TransferItem.File) item.copy(progress = progress, receivedChunks = received)
            else item
        }
    }

    private fun updateTransfer(id: String, transform: (TransferItem) -> TransferItem) {
        _transfers.update { list -> list.map { if (it.id == id) transform(it) else it } }
    }

    private fun resolveFileName(uri: Uri): String {
        if (uri.scheme == "content") {
            context.contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
                if (cursor.moveToFirst()) {
                    val idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    if (idx >= 0) return cursor.getString(idx)
                }
            }
        }
        return uri.lastPathSegment ?: "file_${System.currentTimeMillis()}"
    }

    private fun resolveFileSize(uri: Uri): Long {
        if (uri.scheme == "content") {
            context.contentResolver.query(uri, arrayOf(OpenableColumns.SIZE), null, null, null)?.use { cursor ->
                if (cursor.moveToFirst()) {
                    val idx = cursor.getColumnIndex(OpenableColumns.SIZE)
                    if (idx >= 0 && !cursor.isNull(idx)) return cursor.getLong(idx)
                }
            }
        }
        return 0L
    }
}
