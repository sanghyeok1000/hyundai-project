# 제출 Release 빌드와 실행

## 구성

최종 게임은 Unreal이 만드는 하나의 APK다. Kotlin은 프론트엔드가 아니라 Pleos Vehicle SDK·AAOS 상태·선택형 차량 효과를 C++에 연결하는 Android 라이브러리(AAR)다. Android Studio의 별도 진단 앱은 제출 APK가 아니다.

- UE 5.8 설치본의 Android x86_64 Shipping 구성.
- Android SDK36, Build Tools36.0.0, NDK27.2.12479018, JDK21.
- 브리지의 Gradle9.6은 `gradle-daemon-jvm.properties`에 따라 별도 JDK25를 자동 조달할 수 있다. 최초 빌드는 인터넷이 필요하다. Unreal Android 패키징에 사용하는 JDK21과 구분한다.
- Windows C++ 도구: Visual Studio2022 C++ 워크로드와 Windows SDK. 처음 빌드하려면 UE용 구성도 설치한다.
- Vehicle SDK2.3.2는 공개 Pleos Maven 저장소에서 Gradle로 받는다. Gradle/JDK/SDK/엔진을 소스 저장소에 올리지 않는다.
- 검증 대상은 **Pleos Connect v2.1.0 / Android14 API34 / x86_64 에뮬레이터**다. ARM64 실차 및 운전대 뒤 계기판 배포는 검증하지 않았다. 실차 작동을 활성화하지 않는다.

## 새 소스에서 빌드

저장소 루트에서 PowerShell을 연다. Java 경로는 본인의 JDK21 설치 경로로 바꾼다. 처음에는 Unreal Editor를 닫는다.

```powershell
& '.\unreal\BeyondPhysics\Scripts\package_submission_release.ps1' `
  -EngineDir 'C:\Program Files\Epic Games\UE_5.8' `
  -AndroidSdk "$env:LOCALAPPDATA\Android\Sdk" `
  -JavaHome 'C:\본인JDK21설치경로' `
  -SigningDirectory '.\private\signing' `
  -OutputDirectory '.\submission\release'
```

스크립트는 Release AAR → C++ Shipping → 세 맵 cook → Distribution APK 서명 → SHA256 순서로 처리한다. 개인 서명 키가 없으면 최초 한 번 생성한다. **기존 APK의 업데이트는 같은 키가 필요**하므로 자신의 `private/signing/`을 별도 안전한 곳에 백업한다. 키/비밀번호 파일을 GitHub에 올리지 않는다. 공개 소스에는 제작자의 서명 키가 포함되지 않아 다른 사람이 새로 빌드한 APK는 제작자 APK의 업데이트로 설치되지 않는다.

서명 설정은 빌드 중에만 `Config/Android/AndroidEngine.ini`에 작성하고 완료/오류 후 원래 상태로 복원한다. 강제 전원 종료/프로세스 강제 종료 시 임시 파일이 남을 수 있으므로 Git에서 항상 제외한다. 생성된 `Intermediate/`의 Gradle 파일과 빌드 로그도 공개하지 않는다. `.gitignore`가 방어하지만 개인 파일을 검토한 뒤 커밋한다.

빌드 산출물: `submission/release/BEYOND_PHYSICS_release.apk`, `SHA256SUMS.txt`. 상세 빌드 로그는 `builds/release-logs/`이며 공개 사본에는 포함하지 않는다.

## 실행

1. Android Studio Device Manager에서 **Pleos Connect v2.1.0**을 시작한다.
2. SDK의 platform-tools에서 다음을 실행한다. PATH에 adb가 없다면 `adb`를 SDK의 전체 경로로 바꾼다.

```powershell
adb devices
adb install -r '.\submission\release\BEYOND_PHYSICS_release.apk'
adb forward tcp:8088 tcp:8088
adb shell am start -n com.beyondphysics.simulator/com.epicgames.unreal.GameActivity
```

기존 개발 APK와 서명 키가 다르면 `INSTALL_FAILED_UPDATE_INCOMPATIBLE`이 나온다. 이는 잘못된 APK라는 뜻이 아니다. 개발 앱에 보관할 데이터가 있는지 확인한 뒤 기존 앱을 지우고 설치하거나 새 Pleos 가상기기를 사용한다. 앱 삭제는 앱의 저장 데이터를 지우므로 자동으로 수행하는 설치 스크립트는 제공하지 않는다.

3. [Virtual Console](http://127.0.0.1:8088/)을 열어 둔다. 콘솔 연결은 가상 차량 입력 공급에 필요하다.
4. 차량을 **P / 실제 차량 속도0 / accelerator0**으로 둔다. 게임 속도는 실제 차량 속도와 별개다. 조향과 Brake로 가상 차량을 조작한다. 가속페달을 밟을 필요가 없다.
5. MAP 버튼으로 세 맵을 순환하고 RESTART로 재시작한다. P 해제/입력 단절/백그라운드 이후에는 안전 상태를 회복하고 RESTART한다.
6. EFFECTS에서 AMBIENT LIGHT/GENTLE CLIMATE를 선택하고 APPLY AND RESTART한다. 조명·공조는 기본 OFF, 소리는 ON이다. SDK 지원·원래 상태에 따라 공조가 제한될 수 있으며 상태창의 APPLIED/OFF/UNSUPPORTED/ERROR/RESTORING을 확인한다. [상세 감각 효과 안내](MAP_FEEDBACK.ko.md).

Android Studio에서는 `File → Profile or Debug APK`로 **이번 Release APK**를 새로 선택해 설치/실행할 수 있다. Release는 디버깅 불가이며 개발용 콘솔 명령·run-as·중단점으로 검사하는 대상이 아니다. 코드 수정은 원본 Unreal C++/Kotlin에서 하고 APK를 다시 만든다. 예전에 가져온 APK 사본은 자동 갱신되지 않는다.

## 수정할 원본

| 목적 | 파일/폴더 |
|---|---|
| 힘·슬립·요 운동·수치적분 | `unreal/BeyondPhysics/Source/BeyondPhysics/Physics/VehicleModel.cpp`, `VehicleTypes.h` |
| 안개 속도·장애물·조향비 | `Physics/FogHighway.h`, `Physics/SteeringGeometry.h` |
| 입력·자동 가속·일시정지·카메라 | `Driving/DrivingPawn.cpp`, `Driving/PleosVehicleInput.cpp` |
| 조명/공조/사운드 표현 정책 | `Feedback/FeedbackPolicy.cpp`, `Feedback/DrivingAudioComponent.cpp` |
| 차량 SDK/설정 복원 | `android/pleosbridge/src/main/java/com/beyondphysics/pleos/` |
| 화면과 버튼 | `UI/DrivingHUD.cpp` |
| 실제 레벨·차량·재질·산·조명 | `unreal/BeyondPhysics/Content/Maps/`, `Content/Driving/`, `Content/Vehicles/` |

그래픽은 `BeyondPhysics.uproject`를 Unreal5.8로 열어 수정한다. 공개 사본은 편집 보조용 MCP 플러그인 의존성을 제거했으며 MCP가 없어도 열고 빌드할 수 있다. `.umap/.uasset`은 Unreal Editor에서 편집하는 바이너리 자산이고 `.cpp/.h`는 C++ 원본이다. `Scripts/*.py`는 과거 생성/정리 도구이므로 실행 전 경로와 덮어쓰기 동작을 읽는다. 맵을 다시 생성하는 작업과 기존 맵을 편집하는 작업을 구분한다.
