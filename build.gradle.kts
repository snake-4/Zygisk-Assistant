import java.io.ByteArrayOutputStream

plugins {
    id("com.android.library") version "8.3.2" apply false
}

val commitHash: String by extra {
    val stdout = ByteArrayOutputStream()
    rootProject.exec {
        commandLine("git", "rev-parse", "--short", "HEAD")
        standardOutput = stdout
    }
    stdout.toString().trim()
}

val moduleId by extra("zygisk-assistant")
val moduleName by extra("Zygisk Assistant")
val verName by extra("v2.1.2")
val verCode by extra(212)
