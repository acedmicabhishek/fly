package com.fly.desktop.core.transfer

import java.io.File

sealed class TransferItem {
    abstract val id: String
    abstract val deviceName: String
    abstract val isIncoming: Boolean

    data class Text(
        override val id: String,
        override val deviceName: String,
        override val isIncoming: Boolean,
        val content: String
    ) : TransferItem()

    data class FileTransfer(
        override val id: String,
        override val deviceName: String,
        override val isIncoming: Boolean,
        val name: String,
        val mime: String,
        val size: Long,
        val localFile: File? = null,
        val progress: Float = 0f,
        val totalChunks: Int = 0,
        val receivedChunks: Int = 0
    ) : TransferItem()

    data class LogcatStream(
        override val id: String,
        override val deviceName: String,
        override val isIncoming: Boolean,
        val lines: List<String> = emptyList()
    ) : TransferItem()
}
