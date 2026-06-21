package com.fly.desktop.core.protocol

import org.json.JSONObject

sealed class FlyMessage {
    data class Hello(val deviceName: String, val platform: String = "desktop") : FlyMessage()
    data class Text(val id: String, val content: String) : FlyMessage()
    data class FileOffer(val id: String, val name: String, val mime: String, val size: Long) : FlyMessage()
    data class FileStart(val id: String, val name: String, val mime: String, val size: Long, val totalChunks: Int, val chunkSize: Int) : FlyMessage()
    data class Chunk(val id: String, val index: Int) : FlyMessage()
    data class FileDone(val id: String) : FlyMessage()
    data class Ack(val id: String) : FlyMessage()
    data class Logcat(val id: String, val line: String) : FlyMessage()
    data class Cancel(val id: String) : FlyMessage()
}

fun FlyMessage.toFrame(body: ByteArray = ByteArray(0)): Frame = when (this) {
    is FlyMessage.Hello ->
        Frame("""{"type":"hello","name":${JSONObject.quote(deviceName)},"platform":"$platform"}""")
    is FlyMessage.Text ->
        Frame("""{"type":"text","id":"$id","content":${JSONObject.quote(content)}}""")
    is FlyMessage.FileOffer ->
        Frame("""{"type":"file","id":"$id","name":${JSONObject.quote(name)},"mime":"$mime","size":$size}""", body)
    is FlyMessage.FileStart ->
        Frame("""{"type":"file_start","id":"$id","name":${JSONObject.quote(name)},"mime":"$mime","size":$size,"chunks":$totalChunks,"chunk_size":$chunkSize}""")
    is FlyMessage.Chunk ->
        Frame("""{"type":"chunk","id":"$id","index":$index}""", body)
    is FlyMessage.FileDone ->
        Frame("""{"type":"file_done","id":"$id"}""")
    is FlyMessage.Ack ->
        Frame("""{"type":"ack","id":"$id"}""")
    is FlyMessage.Logcat ->
        Frame("""{"type":"logcat","id":"$id","line":${JSONObject.quote(line)}}""")
    is FlyMessage.Cancel ->
        Frame("""{"type":"cancel","id":"$id"}""")
}

fun Frame.toMessage(): FlyMessage? = runCatching {
    val json = JSONObject(header)
    when (json.getString("type")) {
        "hello" -> FlyMessage.Hello(json.getString("name"), json.optString("platform", "unknown"))
        "text" -> FlyMessage.Text(json.getString("id"), json.getString("content"))
        "file" -> FlyMessage.FileOffer(json.getString("id"), json.getString("name"), json.optString("mime", "application/octet-stream"), json.getLong("size"))
        "file_start" -> FlyMessage.FileStart(json.getString("id"), json.getString("name"), json.optString("mime", "application/octet-stream"), json.getLong("size"), json.getInt("chunks"), json.getInt("chunk_size"))
        "chunk" -> FlyMessage.Chunk(json.getString("id"), json.getInt("index"))
        "file_done" -> FlyMessage.FileDone(json.getString("id"))
        "ack" -> FlyMessage.Ack(json.getString("id"))
        "logcat" -> FlyMessage.Logcat(json.getString("id"), json.getString("line"))
        "cancel" -> FlyMessage.Cancel(json.getString("id"))
        else -> null
    }
}.getOrNull()
