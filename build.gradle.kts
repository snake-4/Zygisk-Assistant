import java.io.ByteArrayOutputStream

plugins {
    alias(libs.plugins.agp.lib) apply false
}

val commitHash: String by extra({
    val stdout = ByteArrayOutputStream()
    rootProject.exec {
        commandLine("git", "rev-parse", "--short", "HEAD")
        standardOutput = stdout
    }
    stdout.toString().trim()
})

val moduleId by extra("zygisk-assistant")
val moduleName by extra("Zygisk Assistant")
val verName by extra("v2.0.3")
val verCode by extra(203)
val abiList by extra(listOf("armeabi-v7a","arm64-v8a","x86","x86_64"))
