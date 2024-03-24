import java.io.ByteArrayOutputStream

plugins {
    alias(libs.plugins.agp.lib) apply false
}

val moduleId by extra("qingyue")
val moduleName by extra("Zygisk Qingyue")
val verName by extra("v1.0.0")
val verCode by extra(100)
val abiList by extra(listOf("armeabi-v7a","arm64-v8a","x86","x86_64"))
