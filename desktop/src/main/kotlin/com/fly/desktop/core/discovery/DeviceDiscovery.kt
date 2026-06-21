package com.fly.desktop.core.discovery

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import javax.jmdns.JmDNS
import javax.jmdns.ServiceEvent
import javax.jmdns.ServiceInfo
import javax.jmdns.ServiceListener

class DeviceDiscovery(private val port: Int) {

    private var jmdns: JmDNS? = null
    private val _devices = MutableStateFlow<Map<String, NetworkDevice>>(emptyMap())
    val devices: StateFlow<Map<String, NetworkDevice>> = _devices.asStateFlow()

    fun start(deviceName: String) {
        Thread {
            runCatching {
                jmdns = JmDNS.create()
                val info = ServiceInfo.create(SERVICE_TYPE, deviceName, port, "fly")
                jmdns?.registerService(info)
                jmdns?.addServiceListener(SERVICE_TYPE, object : ServiceListener {
                    override fun serviceAdded(event: ServiceEvent) {
                        jmdns?.requestServiceInfo(event.type, event.name, 1000)
                    }

                    override fun serviceRemoved(event: ServiceEvent) {
                        _devices.value = _devices.value - event.name
                    }

                    override fun serviceResolved(event: ServiceEvent) {
                        val info = event.info
                        val host = info.inetAddresses.firstOrNull { !it.isLoopbackAddress }?.hostAddress
                            ?: info.inetAddresses.firstOrNull()?.hostAddress
                            ?: return
                        val device = NetworkDevice(
                            id = event.name,
                            name = event.name,
                            platform = "unknown",
                            host = host,
                            port = info.port
                        )
                        _devices.value = _devices.value + (device.id to device)
                    }
                })
            }
        }.also { it.isDaemon = true }.start()
    }

    fun stop() {
        runCatching { jmdns?.close() }
        jmdns = null
    }

    companion object {
        const val SERVICE_TYPE = "_fly._tcp.local."
    }
}
