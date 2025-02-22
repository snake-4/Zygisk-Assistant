import android.databinding.tool.ext.capitalizeUS

plugins {
    id("com.android.library")
}

val moduleId: String by rootProject.extra
val moduleName: String by rootProject.extra
val verCode: Int by rootProject.extra
val commitHash: String by rootProject.extra
val verName: String by rootProject.extra
val abiList: List<String> by rootProject.extra

android {
    namespace = "com.example.library"
    compileSdkVersion = "android-34"
    ndkVersion = "28.0.13004108"
    defaultConfig {
        minSdk = 21
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

    val libOutDir = layout.buildDirectory.dir("intermediates/stripped_native_libs/$variantLowered/strip${variantCapped}DebugSymbols/out/lib").get()
    val moduleDir = layout.buildDirectory.dir("outputs/zip/$variantLowered").get()
    val zipOutDir = layout.buildDirectory.dir("outputs/zip/").get()
    val zipFileName = "$moduleName-$verName-$commitHash-$buildTypeLowered.zip".replace(' ', '-')

    val moduleFilesTask = task<Sync>("moduleFiles$variantCapped") {
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

    task<Zip>("moduleZip$variantCapped") {
        group = "module"
        dependsOn(moduleFilesTask)
        archiveFileName.set(zipFileName)
        destinationDirectory.set(zipOutDir)
        from(moduleDir)
    }
}
