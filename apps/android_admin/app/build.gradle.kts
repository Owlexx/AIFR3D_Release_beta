import java.util.Properties

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

val localProps = Properties().apply {
    val file = rootProject.file("local.properties")
    if (file.exists()) {
        file.inputStream().use { load(it) }
    }
}

android {
    namespace = "com.dawai.admin"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.dawai.admin"
        minSdk = 29
        targetSdk = 35
        versionCode = 224
        versionName = "2.2.4"

        val configuredBaseUrl = (project.findProperty("DAWAI_BASE_URL") as String?)
            ?.trim()
            ?.ifEmpty { "https://www.north3rnlight3r.com" }
            ?: localProps.getProperty("dawaiBaseUrl")
                ?.trim()
                ?.ifEmpty { "https://www.north3rnlight3r.com" }
            ?: "https://www.north3rnlight3r.com"

        val configuredToken = (project.findProperty("DAWAI_API_TOKEN") as String?)
            ?.trim()
            ?.ifEmpty { null }
            ?: localProps.getProperty("dawaiApiToken")
                ?.trim()
            ?: ""

        val defaultAdminUsername = "North3rnLight3r"
        val defaultAdminPassword = "Poohbe@r2009\$0826"

        val configuredAdminUsername = (project.findProperty("DAWAI_ADMIN_USERNAME") as String?)
            ?.trim()
            ?.ifEmpty { null }
            ?: localProps.getProperty("dawaiAdminUsername")
                ?.trim()
                ?.ifEmpty { null }
            ?: defaultAdminUsername

        val configuredAdminPassword = (project.findProperty("DAWAI_ADMIN_PASSWORD") as String?)
            ?.trim()
            ?.ifEmpty { null }
            ?: localProps.getProperty("dawaiAdminPassword")
                ?.trim()
                ?.ifEmpty { null }
            ?: defaultAdminPassword

        buildConfigField("String", "DAWAI_BASE_URL", "\"${configuredBaseUrl}\"")
        buildConfigField("String", "DAWAI_API_TOKEN", "\"${configuredToken}\"")
        buildConfigField("String", "DAWAI_ADMIN_USERNAME", "\"${configuredAdminUsername.replace("\"", "\\\"")}\"")
        buildConfigField("String", "DAWAI_ADMIN_PASSWORD", "\"${configuredAdminPassword.replace("\"", "\\\"")}\"")
    }

    buildFeatures {
        compose = true
        buildConfig = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    packaging {
        resources {
            excludes += "/META-INF/{AL2.0,LGPL2.1}"
        }
    }
}

dependencies {
    val composeBom = platform("androidx.compose:compose-bom:2025.02.00")

    implementation(composeBom)
    androidTestImplementation(composeBom)

    implementation("androidx.core:core-ktx:1.15.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.7")
    implementation("androidx.activity:activity-compose:1.10.0")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")
    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.9.0")
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
}
