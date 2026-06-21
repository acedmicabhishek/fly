package com.fly.desktop.core.discovery

data class NetworkDevice(
    val id: String,
    val name: String,
    val platform: String,
    val host: String,
    val port: Int
)
