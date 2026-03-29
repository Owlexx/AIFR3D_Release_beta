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
    namespace = "com.coresynth.admin"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.coresynth.admin"
        minSdk = 29
        targetSdk = 35
        versionCode = 224
        versionName = "2.2.4"

        val configuredBaseUrl = (project.findProperty("CORESYNTH_BASE_URL") as String?)
            ?.trim()
            ?.ifEmpty { "https://www.north3rnlight3r.com" }
            ?: localProps.getProperty("coresynthBaseUrl")
                ?.trim()
                ?.ifEmpty { "https://www.north3rnlight3r.com" }
            ?: "https://www.north3rnlight3r.com"

        val configuredToken = (project.findProperty("CORESYNTH_API_TOKEN") as String?)
            ?.trim()
            ?.ifEmpty { null }
            ?: localProps.getProperty("coresynthApiToken")
                ?.trim()
            ?: ""

        val defaultAdminUsername = "North3rnLight3r"
        val defaultAdminPassword = "Poohbe@r2009\$0826"

        val configuredAdminUsername = (project.findProperty("CORESYNTH_ADMIN_USERNAME") as String?)
            ?.trim()
            ?.ifEmpty { null }
            ?: localProps.getProperty("coresynthAdminUsername")
                ?.trim()
                ?.ifEmpty { null }
            ?: defaultAdminUsername

        val configuredAdminPassword = (project.findProperty("CORESYNTH_ADMIN_PASSWORD") as String?)
            ?.trim()
            ?.ifEmpty { null }
            ?: localProps.getProperty("coresynthAdminPassword")
                ?.trim()
                ?.ifEmpty { null }
            ?: defaultAdminPassword

        buildConfigField("String", "CORESYNTH_BASE_URL", "\"${configuredBaseUrl}\"")
        buildConfigField("String", "CORESYNTH_API_TOKEN", "\"${configuredToken}\"")
        buildConfigField("String", "CORESYNTH_ADMIN_USERNAME", "\"${configuredAdminUsername.replace("\"", "\\\"")}\"")
        buildConfigField("String", "CORESYNTH_ADMIN_PASSWORD", "\"${configuredAdminPassword.replace("\"", "\\\"")}\"")
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
    
    // Google Drive API
    implementation("com.google.android.gms:play-services-drive:17.0.0")
    implementation("com.google.api-client:google-api-client-android:1.32.1")
    implementation("com.google.oauth-client:google-oauth-client-jetty:1.35.0")
    implementation("com.google.apis:google-api-services-drive:v3-rev20240521-2.0.0")
    
    // JSON parsing
    implementation("org.json:json:20231013")
    
    // File handling
    implementation("commons-io:commons-io:2.11.0")
}
