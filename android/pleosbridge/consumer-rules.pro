-keep class com.beyondphysics.pleos.PleosRuntime { public *; }
-keep class ai.pleos.playground.** { *; }

# Vehicle SDK capability parsing uses Gson TypeToken at runtime. R8 full mode
# needs the TypeToken hierarchy as well as Signature, even if the SDK is kept.
# https://google.github.io/gson/Troubleshooting.html
-keepattributes Signature,InnerClasses,EnclosingMethod
-keep class com.google.gson.reflect.TypeToken { *; }
-keep class * extends com.google.gson.reflect.TypeToken { *; }
