package com.fly.app.core.debug

import com.fly.app.core.network.PeerConnection
import com.fly.app.core.protocol.FlyMessage
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.BufferedReader
import java.io.InputStreamReader
import java.util.UUID

class LogcatCollector(private val scope: CoroutineScope) {

    private var job: Job? = null
    private var process: Process? = null

    fun startStreaming(conn: PeerConnection, filter: String = "") {
        stop()
        val id = UUID.randomUUID().toString()
        job = scope.launch(Dispatchers.IO) {
            val args = buildList {
                add("logcat")
                add("-v")
                add("threadtime")
                if (filter.isNotBlank()) add(filter)
            }
            process = Runtime.getRuntime().exec(args.toTypedArray())
            val reader = BufferedReader(InputStreamReader(process!!.inputStream))
            runCatching {
                while (isActive) {
                    val line = reader.readLine() ?: break
                    conn.send(FlyMessage.Logcat(id, line))
                }
            }
            reader.close()
        }
    }

    fun stop() {
        job?.cancel()
        process?.destroy()
        job = null
        process = null
    }
}
