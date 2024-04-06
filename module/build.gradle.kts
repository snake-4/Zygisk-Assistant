import android.databinding.tool.ext.capitalizeUS

plugins {
    alias(libs.plugins.agp.lib)
}

val moduleId: String by rootProject.extra
val moduleName: String by rootProject.extra
val verCode: Int by rootProject.extra
val commitHash: String by rootProject.extra
val verName: String by rootProject.extra
val abiList: List<String> by rootProject.extra

android {
    namespace = "com.example.library"
    compileSdkVersion = "android-31"
    defaultConfig {
        minSdk = 21
        ndk {
            abiFilters.addAll(abiList)
        }
        externalNativeBuild {
            ndkBuild {
                arguments("-j${Runtime.getRuntime().availableProcessors()}")
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

    val libOutDir = layout.buildDirectory.dir("intermediates/stripped_native_libs/$variantLowered/strip${variantLowered}DebugSymbols/out/lib").get()
    val moduleDir = layout.buildDirectory.dir("outputs/module/$variantLowered").get()
    val zipOutDir = layout.buildDirectory.dir("outputs/release").get()
    val zipFileName = "$moduleName-$verName-$commitHash-$buildTypeLowered.zip".replace(' ', '-')

    val prepareModuleFilesTask = task<Sync>("prepareModuleFiles$variantCapped") {
        group = "module"
        dependsOn("assemble$variantCapped")
        into(moduleDir)
        from("$projectDir/template") {
            include("module.prop")
            expand(
                "moduleId" to moduleId,
                "moduleName" to moduleName,
                "versionName" to "$verName ($commitHash-$variantLowered)",
                "versionCode" to verCode
            )
        }
        from("$projectDir/template") {
            exclude("module.prop")
        }
        from(libOutDir) {
            into("zygisk")
        }
        doLast {
            moduleDir.dir("zygisk").asFile.listFiles { f -> f.isDirectory }?.forEach { sourceDir ->
                val srcFile = file("$sourceDir/libzygisk.so")
                val dstFile = moduleDir.file("zygisk/${sourceDir.name}.so")
                srcFile.copyTo(dstFile.asFile, overwrite=true)
                delete(sourceDir)
            }
        }
    }

    task<Zip>("zip$variantCapped") {
        group = "module"
        dependsOn(prepareModuleFilesTask)
        archiveFileName.set(zipFileName)
        destinationDirectory.set(zipOutDir)
        from(moduleDir)
    }
}
