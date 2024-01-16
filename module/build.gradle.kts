import android.databinding.tool.ext.capitalizeUS
import org.apache.tools.ant.filters.FixCrLfFilter
import org.apache.tools.ant.filters.ReplaceTokens
import java.security.MessageDigest

plugins {
    alias(libs.plugins.agp.lib)
}

val moduleId: String by rootProject.extra
val moduleName: String by rootProject.extra
val verCode: Int by rootProject.extra
val verName: String by rootProject.extra
val abiList: List<String> by rootProject.extra

android {
    namespace = "io.github.yervant7"
    compileSdk = 34
    defaultConfig {
        ndk {
            abiFilters.addAll(abiList)
        }
        externalNativeBuild {
            ndkBuild {
                arguments("MODULE_NAME=$moduleId")
            }
        }
    }
    externalNativeBuild {
        ndkBuild {
            path("jni/Android.mk")
        }
    }
}

androidComponents.onVariants { variant ->
    val variantLowered = variant.name.lowercase()
    val variantCapped = variant.name.capitalizeUS()
    val buildTypeLowered = variant.buildType?.lowercase()
    val supportedAbis = abiList.map {
        when (it) {
            "arm64-v8a" -> "arm64"
            else -> error("unsupported abi $it")
        }
    }.joinToString(" ")

    val moduleDir = "$buildDir/outputs/module/$variantLowered"
    val zipFileName = "$moduleName-$verName-$verCode-$buildTypeLowered.zip".replace(' ', '-')

    val prepareModuleFilesTask = task<Sync>("prepareModuleFiles$variantCapped") {
        group = "module"
        dependsOn("assemble$variantCapped")
        into(moduleDir)
        from("${rootProject.projectDir}/README.md")
        from("$projectDir/template") {
            exclude("module.prop", "customize.sh", "post-fs-data.sh", "service.sh")
            filter<FixCrLfFilter>("eol" to FixCrLfFilter.CrLf.newInstance("lf"))
        }
        from("$projectDir/template") {
            include("module.prop")
            expand(
                "moduleId" to moduleId,
                "moduleName" to moduleName,
                "versionName" to "$verName ($verCode-$variantLowered)",
                "versionCode" to verCode
            )
        }
        from("$projectDir/template") {
            include("customize.sh", "post-fs-data.sh", "service.sh")
            val tokens = mapOf(
                "DEBUG" to if (buildTypeLowered == "debug") "true" else "false",
                "SONAME" to moduleId,
                "SUPPORTED_ABIS" to supportedAbis
            )
            filter<ReplaceTokens>("tokens" to tokens)
            filter<FixCrLfFilter>("eol" to FixCrLfFilter.CrLf.newInstance("lf"))
        }
        from("$buildDir/intermediates/stripped_native_libs/$variantLowered/out/lib") {
            into("lib")
        }

        doLast {
            fileTree(moduleDir).visit {
                if (isDirectory) return@visit
                val md = MessageDigest.getInstance("SHA-256")
                file.forEachBlock(4096) { bytes, size ->
                    md.update(bytes, 0, size)
                }
                file(file.path + ".sha256").writeText(org.apache.commons.codec.binary.Hex.encodeHexString(md.digest()))
            }
        }
    }

    val zipTask = task<Zip>("zip$variantCapped") {
        group = "module"
        dependsOn(prepareModuleFilesTask)
        archiveFileName.set(zipFileName)
        destinationDirectory.set(file("$buildDir/outputs/release"))
        from(moduleDir)
    }

    val pushTask = task<Exec>("push$variantCapped") {
        group = "module"
        dependsOn(zipTask)
        commandLine("adb", "push", zipTask.outputs.files.singleFile.path, "/data/local/tmp")
    }

    val installKsuTask = task("installKsu$variantCapped") {
        group = "module"
        dependsOn(pushTask)
        doLast {
            exec {
                commandLine(
                    "adb", "shell", "echo",
                    "/data/adb/ksud module install /data/local/tmp/$zipFileName",
                    "> /data/local/tmp/install.sh"
                )
            }
            exec { commandLine("adb", "shell", "chmod", "755", "/data/local/tmp/install.sh") }
            exec { commandLine("adb", "shell", "su", "-c", "/data/local/tmp/install.sh") }
        }
    }

    val installMagiskTask = task<Exec>("installMagisk$variantCapped") {
        group = "module"
        dependsOn(pushTask)
        commandLine("adb", "shell", "su", "-c", "magisk --install-module /data/local/tmp/$zipFileName")
    }

    task<Exec>("installKsuAndReboot$variantCapped") {
        group = "module"
        dependsOn(installKsuTask)
        commandLine("adb", "reboot")
    }

    task<Exec>("installMagiskAndReboot$variantCapped") {
        group = "module"
        dependsOn(installMagiskTask)
        commandLine("adb", "reboot")
    }
}
