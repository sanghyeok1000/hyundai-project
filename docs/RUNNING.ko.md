# APK 설치와 실행

## 준비 환경

| 항목 | 기준 |
|---|---|
|에뮬레이터|Pleos Connect v2.1.0|
|Android|Android 14 / API 34|
|CPU ABI|x86_64|
|입력 공급|Pleos Connect Virtual Console|
|앱 패키지|`com.beyondphysics.simulator`|
|앱 버전|0.1.0 · versionCode 1|

에뮬레이터는 [공식 설치 안내](https://document.pleos.ai/docs/connect/guide/getting-started/application-development/setup-connect-sdk-emulator)에 따라 Android Studio의 Device Manager에 추가한다. 부팅 방식은 **Cold Boot**를 사용한다. APK 실행만 할 때는 Unreal Engine을 설치할 필요가 없다.

## 1. APK와 기기 연결

[GitHub Releases](https://github.com/sanghyeok1000/hyundai-project/releases/tag/v0.1.0-prototype)에서 `beyond_physics_BEYOND_PHYSICS_release.apk`와 `SHA256SUMS.txt`를 받는다. APK가 있는 폴더에서 PowerShell을 연다.

```powershell
Get-FileHash '.\beyond_physics_BEYOND_PHYSICS_release.apk' -Algorithm SHA256
$adb = Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'
& $adb devices
```

SHA256 결과는 `SHA256SUMS.txt` 및 [Release 검증](RELEASE_VERIFICATION.ko.md)의 값과 같아야 한다. Device Manager에서 시작한 가상기기가 `device`로 표시되면 연결된 상태다.

## 2. 설치와 Virtual Console

```powershell
& $adb install -r '.\beyond_physics_BEYOND_PHYSICS_release.apk'
& $adb forward tcp:8088 tcp:8088
```

설치 결과 `Success`를 확인하고 브라우저에서 **http://127.0.0.1:8088/** 을 연다. 콘솔을 다음 상태로 둔다.

- 기어 **P**, 차량 속도 **0km/h**.
- **Accelerator 0**, **Brake 해제**, **Steering 중앙**.
- 콘솔의 **DDS ONLINE** 상태 확인.

가상 차량 입력을 공급하는 브라우저를 계속 열어 둔다. [Virtual Console 공식 사용법](https://document.pleos.ai/docs/pleos-only/app-test-guide/virtual-console-guide).

## 3. 앱 시작

```powershell
& $adb shell am start -n com.beyondphysics.simulator/com.epicgames.unreal.SplashActivity
```

처음에는 직선 맵이 열린다. 입력 상태가 `READY`가 되면 자동 가속한다. 입력 대기에서 정상 상태로 복구한 뒤에는 **RESTART**를 누른다.

Android Studio에서 APK를 열려면 **File → Profile or Debug APK**에서 다운로드한 파일을 선택한다. 게임 원본을 편집할 때는 Unreal 프로젝트를 사용한다.

## 4. 조작

| 조작 위치 | 입력 | 동작 |
|---|---|---|
|Virtual Console|Steering|가상 차량 조향|
|Virtual Console|BRAKE 누르고 유지|가상 차량 제동|
|게임|MAP|직선 → 곡선 → 안개 전환|
|게임|RESTART|현재 맵 초기 상태로 재시작|
|게임|EFFECTS|주행을 멈추고 효과 설정 열기|
|설정창|AMBIENT LIGHT|앰비언트 조명 사용 선택|
|설정창|GENTLE CLIMATE|공조 효과 사용 선택|
|설정창|DRIVING AUDIO|주행 사운드 사용 선택|
|설정창|APPLY AND RESTART|선택을 적용하고 주행 재시작|

브레이크는 눌림/해제의 두 상태를 사용한다. Accelerator는 게임 조작에 사용하지 않는다. 맵별 가속 정책은 다음과 같다.

- **직선**: 정지 출발. 제동을 놓으면 자동 가속을 다시 요청한다.
- **램프**: 정지 출발. 초기 가속 목표 50km/h, 구동 요청 상한 12%. 제동한 뒤에는 목표 속도가 낮아진다. 내리막에서는 중력 때문에 속도가 더 올라갈 수 있다.
- **안개**: 80km/h 출발, 목표 100km/h. 움직이는 중 첫 제동으로 자동 가속을 해제하며 이후에는 타력과 제동으로 진행한다.

P 해제·UX 제한·신호 끊김·앱 이탈 때 주행과 효과를 중단한다. 조건이 정상으로 돌아오면 RESTART로 새로운 시도를 시작한다.

## 5. 권장 체험 순서

1. **직선**에서 자동 가속을 관찰하고 브레이크를 유지한다. 다시 시작해 조향과 제동을 함께 사용하며 앞·뒤 접지력 사용량을 비교한다.
2. **램프**를 달린 뒤 RESTART한다. 이번에는 곡선 이전에 제동하고, 오른쪽 패널에서 두 시도의 진입 속도와 궤적을 비교한다.
3. **안개**에서 앞차를 본 뒤 제동한 시도와 안개 진입 전에 감속한 시도를 비교한다. 결과 패널의 반응 구간·첫 제동 이후 거리·정지 간격을 확인한다.
4. EFFECTS에서 조명과 공조를 켠 뒤 각 맵을 다시 실행한다. 콘솔의 실제 색·풍량·설정 온도와 앱의 `APPLIED`를 확인한다.
5. 콘솔에서 D로 바꿔 이용 제한과 효과 복원을 확인하고, P로 돌아와 RESTART한다.

## 문제 해결

| 증상 | 확인할 내용 |
|---|---|
|`adb.exe`를 찾지 못함|Android Studio의 SDK Manager에서 Android SDK Location을 확인하고 `$adb`를 그 경로의 `platform-tools\adb.exe`로 지정한다.|
|`adb devices`가 비어 있음|Pleos AVD 부팅 완료와 Platform-Tools 설치를 확인한다.|
|`more than one device/emulator`|`adb devices`에 나온 대상 일련번호를 `-s` 뒤에 지정한다. 예: `& $adb -s emulator-5554 install -r '.\beyond_physics_BEYOND_PHYSICS_release.apk'`. 이후 모든 명령에도 같은 대상을 지정한다.|
|`INSTALL_FAILED_NO_MATCHING_ABIS`|x86_64 Pleos AVD인지 확인한다.|
|`INSTALL_FAILED_UPDATE_INCOMPATIBLE`|기존 앱과 서명이 다르다. 새 Pleos AVD에 설치하거나 필요한 데이터를 보관한 뒤 기존 앱을 제거하고 설치한다.|
|`INSTALL_FAILED_VERSION_DOWNGRADE`|더 높은 versionCode의 앱이 설치돼 있다. 새 AVD에 설치하거나 기존 앱 버전을 확인한다.|
|콘솔이 열리지 않음|에뮬레이터를 켜고 `adb forward tcp:8088 tcp:8088`을 다시 실행한다. 8088 포트의 다른 로컬 서버도 확인한다.|
|`WAITING FOR PARK / UX`|기어 P, 콘솔 연결, DDS ONLINE을 확인한다.|
|`STEERING UNAVAILABLE` / `BRAKE UNAVAILABLE`|Virtual Console과 DDS 연결을 확인하고 입력을 한 번 변경한다. 소스 재빌드 APK라면 Release의 Gson 보존 규칙을 확인한다.|
|`VEHICLE INPUT STALE`|브라우저 콘솔을 다시 연결하고 정상 입력을 확인한 뒤 RESTART한다.|
|`PARK RESTORED`|주차 상태는 복구됐지만 자동 재개하지 않는 상태다. RESTART를 누른다.|
|조명·공조 `UNSUPPORTED`|해당 기능의 capability와 기존 설정 조건을 확인한다. 공조는 전원 ON·AUTO OFF 조건을 사용한다.|
|공조가 팬만 바뀜|A/C가 꺼져 있거나 온도 설정 조건이 충족되지 않으면 팬만 적용한다.|
|소리가 나지 않음|DRIVING AUDIO, 에뮬레이터 미디어 음량, 다른 앱의 오디오 포커스를 확인한다.|

기존 앱 제거는 앱 데이터를 삭제한다. 설치 절차에는 자동 제거 명령을 포함하지 않는다.
