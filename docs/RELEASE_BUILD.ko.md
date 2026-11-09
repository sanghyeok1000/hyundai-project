# 소스 빌드와 Unreal 편집

## 개발 환경

| 구성 요소 | 버전 / 설정 |
|---|---|
|Unreal Engine|5.8 · Android 플랫폼과 x86_64 Shipping 바이너리 포함|
|Windows C++|Visual Studio 2022 C++ 워크로드 · Windows SDK|
|Android SDK / Build Tools|API 36 / 36.0.0|
|Android NDK|27.2.12479018|
|Unreal 패키징용 Java|JDK 21|
|브리지 Gradle|Wrapper 9.6.0 · daemon JVM 설정 25|
|Vehicle SDK|2.3.2|
|Kotlin 표준 라이브러리|2.2.10|
|Android 출력|x86_64 · ETC2 · Shipping / Distribution|

Gradle Wrapper는 설정된 JVM과 의존성을 자동으로 받으므로 최초 빌드에는 인터넷 연결이 필요하다. Vehicle SDK 저장소는 `android/settings.gradle.kts`와 `Pleos_UPL.xml`에 선언돼 있다.

## 저장소 받기

```powershell
git clone https://github.com/sanghyeok1000/hyundai-project.git
cd hyundai-project
```

## Unreal에서 실행

`unreal/BeyondPhysics/BeyondPhysics.uproject`를 Unreal Engine 5.8로 연다. 첫 실행에서 C++ 모듈 빌드가 필요하다. 프로젝트를 연 뒤 Play로 실행한다. 기본 레벨은 `IceTrackDrive`다.

| PC 입력 | 기능 |
|---|---|
|A / D|좌 / 우 조향|
|S|제동|
|1 / 2|두 얼음 맵에서 일반 노면 / 저마찰 노면 비교|
|R|재시작|
|M|맵 전환|
|F|효과 설정 열기 / 적용 후 재시작|
|L / H / K|설정창에서 조명 / 공조 / 사운드 전환|
|V|카메라 전환|

PC에서는 차량 조명·공조 출력을 프리뷰하며, Pleos SDK의 실제 장치 연동은 에뮬레이터 APK에서 실행한다.

## Android Release 패키징

Unreal Editor에서 변경한 맵과 자산을 저장하고 에디터를 닫는다. JDK 21을 `JAVA_HOME`으로 설정한 뒤 저장소 루트에서 다음을 실행한다.

```powershell
& '.\unreal\BeyondPhysics\Scripts\package_submission_release.ps1' `
  -EngineDir 'C:\Program Files\Epic Games\UE_5.8' `
  -AndroidSdk (Join-Path $env:LOCALAPPDATA 'Android\Sdk') `
  -JavaHome $env:JAVA_HOME
```

설치 위치가 다르면 `EngineDir`, `AndroidSdk`, `JavaHome` 인수에 해당 경로를 지정한다. 스크립트는 다음 순서로 실행한다.

1. Kotlin 브리지의 Release AAR 생성.
2. Unreal C++ Shipping 빌드.
3. 직선·램프·안개 맵 cook 및 게임 데이터 패키징.
4. 전용 키로 Android Distribution APK 서명.
5. APK 서명·파일 정렬·디버깅 비활성 검사와 SHA256 생성.

| 출력 | 위치 |
|---|---|
|Release APK|`submission/release/BEYOND_PHYSICS_release.apk`|
|체크섬|`submission/release/SHA256SUMS.txt`|
|상세 빌드 로그|`builds/release-logs/`|
|로컬 서명 자료|`private/signing/`|

서명 키는 최초 빌드 때 생성한다. 업데이트 APK에는 같은 키를 사용하고 배포 버전에 맞춰 `Config/DefaultEngine.ini`의 `StoreVersion`을 증가시킨다. 제작자의 서명 키는 저장소에 포함하지 않는다.

소스와 브리지만 변경하고 기존 cook 결과를 재사용할 때는 `-SkipCook`을 추가할 수 있다. 맵·재질·텍스처·패키징 설정을 변경한 경우 전체 cook으로 빌드한다.

## 수정 위치

아래 경로는 `unreal/BeyondPhysics/`를 기준으로 한다.

| 대상 | 위치 |
|---|---|
|차량 힘·슬립·적분|`Source/BeyondPhysics/Physics/VehicleModel.cpp`|
|질량·관성·마찰 등 파라미터|`Physics/VehicleTypes.h`, `Driving/DrivingPawn.h`|
|안개 초기 속도·목표 속도·장애물|`Physics/FogHighway.h`|
|핸들각·전륜각|`Physics/SteeringGeometry.h`|
|램프 형상|`Physics/VirtualRamp.h`, `SourceArt/Ramp/road.csv`|
|자동 가속|`Driving/AutoDriveInput.h`, `Driving/RampAutoDrive.h`, `Physics/FogHighway.h`|
|입력·맵 전환·카메라|`Driving/DrivingPawn.cpp`|
|HUD와 버튼|`UI/DrivingHUD.cpp`|
|감각 효과|`Feedback/FeedbackPolicy.cpp`, `Feedback/DrivingAudioComponent.cpp`|
|그래픽|`Content/Maps/`, `Content/Driving/`, `Content/Vehicles/`|

`Physics/`, `Driving/`, `UI/`, `Feedback/` 축약 경로는 `Source/BeyondPhysics/` 아래다. 차량 SDK 원본은 저장소 루트의 `android/pleosbridge/src/main/java/com/beyondphysics/pleos/`에 있다.

`.umap`과 `.uasset`은 Unreal Editor에서 편집한다. 도로 메시를 다시 생성할 때는 `Scripts/`의 생성 코드와 `Physics/VirtualRamp.h`의 형상을 함께 확인한다. `Intermediate/Android`는 빌드 과정에서 생성되는 파일이다.

## Release의 SDK 보존 규칙

SDK capability 응답은 Gson의 제네릭 타입 정보를 사용한다. `android/pleosbridge/consumer-rules.pro`는 R8에서 `Signature`와 `TypeToken` 계층, Vehicle SDK 및 JNI 진입점을 보존한다. 관련 규칙을 제거하면 Release에서 조향·브레이크 capability 읽기가 실패할 수 있다.

## 계산 검사 실행

Visual Studio 2022 Community의 기본 설치 경로를 사용하는 검사 스크립트가 `Tests/`에 있다. 다른 에디션은 각 스크립트의 `vcvars64.bat` 경로를 조정한다.

```powershell
& '.\unreal\BeyondPhysics\Tests\run-model-tests.cmd'
& '.\unreal\BeyondPhysics\Tests\run-contact-patch-checks.cmd'
& '.\unreal\BeyondPhysics\Tests\run-feedback-checks.cmd'
```

브리지 단위 검사는 JDK와 Android SDK 설정 후 실행한다.

```powershell
Push-Location android
.\gradlew.bat :pleosbridge:testDebugUnitTest
Pop-Location
```

계산 검사는 Unreal과 독립된 C++ 코어를 검증한다. APK의 SDK 연결과 화면·장치 동작은 [실행 안내](RUNNING.ko.md)의 절차로 별도 확인한다.
