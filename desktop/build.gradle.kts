import org.jetbrains.compose.desktop.application.dsl.TargetFormat

plugins {
    kotlin("jvm") version "2.0.21"
    id("org.jetbrains.compose") version "1.7.0"
    id("org.jetbrains.kotlin.plugin.compose") version "2.0.21"
}

group = "com.fly.desktop"
version = "1.0.0"

kotlin {
    jvmToolchain(17)
}

dependencies {
    implementation(compose.desktop.currentOs)
    implementation(compose.material3)
    implementation(compose.materialIconsExtended)
    implementation(compose.components.resources)
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-swing:1.8.1")
    implementation("org.jmdns:jmdns:3.5.9")
    implementation("org.json:json:20240303")
    runtimeOnly("org.slf4j:slf4j-nop:2.0.16")
}

compose.desktop {
    application {
        mainClass = "com.fly.desktop.MainKt"

        nativeDistributions {
            targetFormats(
                TargetFormat.Dmg,
                TargetFormat.Msi,
                TargetFormat.Deb,
                TargetFormat.Rpm,
                TargetFormat.AppImage
            )
            packageName = "fly"
            packageVersion = "1.0.0"
            description = "Local network file and text transfer"
            copyright = "2024"
            vendor = "fly"

            linux {
                packageName = "fly"
                debMaintainer = "acedmicabhishek"
                menuGroup = "Network;FileTransfer"
                appCategory = "Network"
                iconFile.set(project.file("src/main/resources/icon.png"))
            }

            macOS {
                bundleID = "com.fly.desktop"
                dockName = "Fly"
                iconFile.set(project.file("src/main/resources/icon.icns"))
            }

            windows {
                shortcut = true
                menuGroup = "Fly"
                upgradeUuid = "3f2c1a8e-4b7d-4e9f-a012-567890abcdef"
                iconFile.set(project.file("src/main/resources/icon.ico"))
            }
        }
    }
}
