package com.fly.desktop.ui.theme

import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.material3.lightColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color

private val DarkColors = darkColorScheme(
    primary = Color(0xFF6DD5FA),
    secondary = Color(0xFF89F7FE),
    background = Color(0xFF0D0D0D),
    surface = Color(0xFF1A1A1A),
    surfaceVariant = Color(0xFF252525),
    onBackground = Color(0xFFEEEEEE),
    onSurface = Color(0xFFEEEEEE),
    onPrimary = Color(0xFF000000)
)

private val LightColors = lightColorScheme(
    primary = Color(0xFF0077A8),
    secondary = Color(0xFF005F88),
    background = Color(0xFFF2F2F2),
    surface = Color(0xFFFFFFFF),
    surfaceVariant = Color(0xFFEAEAEA),
    onBackground = Color(0xFF111111),
    onSurface = Color(0xFF111111),
    onPrimary = Color(0xFFFFFFFF)
)

@Composable
fun FlyTheme(themeMode: String = "system", content: @Composable () -> Unit) {
    val dark = when (themeMode) {
        "dark" -> true
        "light" -> false
        else -> isSystemInDarkTheme()
    }
    MaterialTheme(colorScheme = if (dark) DarkColors else LightColors, content = content)
}
