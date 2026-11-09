param(
    [string]$EngineDir = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$AndroidSdk = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$JavaHome = $env:JAVA_HOME,
    [string]$SigningDirectory = '',
    [string]$OutputDirectory = '',
    [switch]$SkipCook
)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path $PSScriptRoot -Parent
$workspaceDir = Split-Path (Split-Path $projectDir -Parent) -Parent
if (!$SigningDirectory) { $SigningDirectory = Join-Path $workspaceDir 'private\signing' }
if (!$OutputDirectory) { $OutputDirectory = Join-Path $workspaceDir 'submission\release' }
$SigningDirectory = [IO.Path]::GetFullPath($SigningDirectory)
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
if (!$JavaHome) { throw 'Pass -JavaHome with a JDK 21 directory.' }
$env:JAVA_HOME = $JavaHome
$env:ANDROID_HOME = $AndroidSdk
$env:ANDROID_SDK_ROOT = $AndroidSdk
$env:NDKROOT = Join-Path $AndroidSdk 'ndk\27.2.12479018'
$env:PATH = "$JavaHome\bin;$env:PATH"
foreach ($required in @("$JavaHome\bin\keytool.exe", "$env:NDKROOT\source.properties", "$EngineDir\Engine\Binaries\Android\UnrealGame-Android-Shipping-x64.so")) {
    if (!(Test-Path -LiteralPath $required)) { throw "Missing prerequisite: $required" }
}
New-Item -ItemType Directory -Force -Path $SigningDirectory, $OutputDirectory | Out-Null
$privateConfig = Join-Path $SigningDirectory 'release-signing.json'
$keystore = Join-Path $SigningDirectory 'beyondphysics-release.jks'
if (!(Test-Path -LiteralPath $privateConfig)) {
    if (Test-Path -LiteralPath $keystore) { throw 'Keystore exists without its signing configuration; restore the matching private backup.' }
    $bytes = New-Object byte[] 32
    $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
    try { $rng.GetBytes($bytes) } finally { $rng.Dispose() }
    $password = [Convert]::ToBase64String($bytes)
    $env:BP_RELEASE_STOREPASS = $password
    try {
        & "$JavaHome\bin\keytool.exe" -genkeypair -keystore $keystore -storetype JKS -alias beyondphysics -keyalg RSA -keysize 3072 -validity 10000 -dname 'CN=BEYOND PHYSICS, O=BEYOND PHYSICS, C=KR' -storepass:env BP_RELEASE_STOREPASS -keypass:env BP_RELEASE_STOREPASS *> (Join-Path $SigningDirectory 'keytool-private.log')
        if ($LASTEXITCODE -ne 0) { throw 'Signing key generation failed; inspect the private keytool log.' }
        @{ alias = 'beyondphysics'; password = $password } | ConvertTo-Json | Set-Content -LiteralPath $privateConfig -Encoding UTF8
    } finally { Remove-Item Env:BP_RELEASE_STOREPASS -ErrorAction SilentlyContinue }
}
$signing = Get-Content -LiteralPath $privateConfig -Raw | ConvertFrom-Json
if (!(Test-Path -LiteralPath $keystore)) { throw 'Private keystore is missing; restore the matching private backup.' }
$logDir = Join-Path $workspaceDir 'builds\release-logs'
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$bridgeLog = Join-Path $logDir 'bridge-release.log'
Push-Location (Join-Path $workspaceDir 'android')
try {
    & '.\gradlew.bat' :pleosbridge:assembleRelease *> $bridgeLog
    if ($LASTEXITCODE -ne 0) { throw "Release bridge build failed. See $bridgeLog" }
} finally { Pop-Location }
Copy-Item -LiteralPath (Join-Path $workspaceDir 'android\pleosbridge\build\outputs\aar\pleosbridge-release.aar') -Destination (Join-Path $projectDir 'Build\Android\pleosbridge.aar')
# Signing passwords are transient local configuration, never command-line arguments.
$configDir = Join-Path $projectDir 'Config\Android'
New-Item -ItemType Directory -Force -Path $configDir | Out-Null
$androidConfig = Join-Path $configDir 'AndroidEngine.ini'
$previousConfig = if (Test-Path -LiteralPath $androidConfig) { [IO.File]::ReadAllBytes($androidConfig) } else { $null }
$originalText = if ($null -ne $previousConfig) { [Text.Encoding]::UTF8.GetString($previousConfig) } else { '' }
$keyPath = ([IO.Path]::GetFullPath($keystore)).Replace('\', '/')
$releaseSettings = @"

[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
KeyStore=$keyPath
KeyAlias=$($signing.alias)
KeyStorePassword=$($signing.password)
KeyPassword=$($signing.password)

[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]
bEnablePlugin=False
bIncludeInShipping=False
bAllowExternalStartInShipping=False
"@
$buildLog = Join-Path $logDir 'unreal-shipping.log'
try {
    [IO.File]::WriteAllText($androidConfig, $originalText + $releaseSettings, (New-Object Text.UTF8Encoding($false)))
    $uatArgs = @('BuildCookRun', "-project=$projectDir\BeyondPhysics.uproject", '-nop4',
        '-platform=Android', '-cookflavor=ETC2', '-clientconfig=Shipping', '-distribution',
        '-build', '-cook', '-stage', '-pak', '-package', '-archive', '-nodebuginfo',
        "-archivedirectory=$OutputDirectory\archive", '-UbtArgs=-MaxParallelActions=4',
        '-map=/Game/Maps/IceTrackDrive+/Game/Maps/IceRampDrive+/Game/Maps/FogHighwayDrive', '-unattended', '-utf8output')
    if ($SkipCook) { $uatArgs = $uatArgs | ForEach-Object { if ($_ -eq '-cook') { '-skipcook' } else { $_ } } }
    & "$EngineDir\Engine\Build\BatchFiles\RunUAT.bat" @uatArgs *> $buildLog
    $buildExit = $LASTEXITCODE
} finally {
    if ($null -ne $previousConfig) { [IO.File]::WriteAllBytes($androidConfig, $previousConfig) }
    else { Remove-Item -LiteralPath $androidConfig -ErrorAction SilentlyContinue }
    # Keep logs useful without retaining signing credentials if an external tool echoed them.
    if (Test-Path -LiteralPath $buildLog) {
        $log = [IO.File]::ReadAllText($buildLog).Replace($signing.password, '[REDACTED]')
        [IO.File]::WriteAllText($buildLog, $log)
    }
}
if ($buildExit -ne 0) { throw "Shipping build failed ($buildExit). See $buildLog" }
$apk = Get-ChildItem -LiteralPath (Join-Path $OutputDirectory 'archive') -Filter '*.apk' -File -Recurse |
    Where-Object { $_.Name -notmatch 'AFS|universal' } | Select-Object -First 1
if (!$apk) { throw 'UAT finished without an APK.' }
$destination = Join-Path $OutputDirectory 'beyond_physics_BEYOND_PHYSICS_release.apk'
Copy-Item -LiteralPath $apk.FullName -Destination $destination
$buildTools = Join-Path $AndroidSdk 'build-tools\36.0.0'
& "$buildTools\apksigner.bat" verify --verbose --print-certs $destination *> (Join-Path $logDir 'signature-verification.txt')
if ($LASTEXITCODE -ne 0) { throw 'Release APK signature verification failed.' }
& "$buildTools\zipalign.exe" -c -P 16 4 $destination *> (Join-Path $logDir 'alignment-verification.txt')
if ($LASTEXITCODE -ne 0) { throw 'Release APK alignment verification failed.' }
$badging = & "$buildTools\aapt2.exe" dump badging $destination
if ($LASTEXITCODE -ne 0 -or ($badging -match 'application-debuggable')) { throw 'APK is invalid or still debuggable.' }
$badging | Set-Content -LiteralPath (Join-Path $logDir 'apk-badging.txt') -Encoding UTF8
$hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  beyond_physics_BEYOND_PHYSICS_release.apk" | Set-Content -LiteralPath (Join-Path $OutputDirectory 'SHA256SUMS.txt') -Encoding ASCII
Write-Output "Shipping APK: $destination"
Write-Output "SHA256: $hash"
Write-Output "Private signing key (never upload): $SigningDirectory"
