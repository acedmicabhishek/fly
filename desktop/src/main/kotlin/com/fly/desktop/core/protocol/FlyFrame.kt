package com.fly.desktop.core.protocol

import java.io.DataInputStream
import java.io.DataOutputStream

data class Frame(val header: String, val body: ByteArray = ByteArray(0)) {
    override fun equals(other: Any?) = other is Frame && header == other.header && body.contentEquals(other.body)
    override fun hashCode() = 31 * header.hashCode() + body.contentHashCode()
}

fun writeFrame(out: DataOutputStream, frame: Frame) {
    val headerBytes = frame.header.toByteArray(Charsets.UTF_8)
    out.writeInt(headerBytes.size)
    out.write(headerBytes)
    out.writeInt(frame.body.size)
    if (frame.body.isNotEmpty()) out.write(frame.body)
    out.flush()
}

fun readFrame(input: DataInputStream): Frame {
    val headerLen = input.readInt()
    require(headerLen in 1..65536) { "Invalid header length: $headerLen" }
    val headerBytes = ByteArray(headerLen).also { input.readFully(it) }
    val bodyLen = input.readInt()
    require(bodyLen >= 0) { "Invalid body length: $bodyLen" }
    val body = if (bodyLen > 0) ByteArray(bodyLen).also { input.readFully(it) } else ByteArray(0)
    return Frame(headerBytes.toString(Charsets.UTF_8), body)
}
