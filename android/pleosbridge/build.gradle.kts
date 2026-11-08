plugins { alias(libs.plugins.android.library) }
android {
    namespace = "com.beyondphysics.pleos"
    compileSdk = 36
    useLibrary("android.car", false)
    defaultConfig {
        minSdk = 28
        consumerProguardFiles("consumer-rules.pro")
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}
dependencies {
    implementation("ai.pleos.playground:Vehicle:2.3.2")
    testImplementation("junit:junit:4.13.2")
}
