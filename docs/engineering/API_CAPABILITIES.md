# Pleos API 구성

기준 환경은 Vehicle SDK 2.3.2 / Pleos Connect Emulator v2.1.0이다. Kotlin 브리지가 Vehicle SDK와 AAOS 상태를 읽고 JNI를 통해 Unreal C++에 전달한다.

## 입력과 실행 제한

| 기능 | 사용 API / 경로 | 처리 |
|---|---|---|
|조향|`getSteeringWheel()`, capability 확인, `getCurrentSteeringWheelAngle`, register/unregister|raw 값과 갱신 시각을 보관하고 맵별 조향 모델에 전달|
|브레이크|Vehicle SDK Brake capability/get/callback|눌림 여부를 제동 요청 0/1로 매핑|
|기어|AAOS `CarPropertyManager`의 기어 상태|P 상태 확인|
|UX 제한|AAOS `CarUxRestrictionsManager`|주의분산 제한과 flags 확인|
|앱 상태|Android 생명주기|백그라운드에서 입력·효과 중단|
|신호 유효성|`InputGate`, `VehicleInputGate`|null·범위·갱신 시각 확인, 끊김 후 재시작 요구|

입력 유효 기간은 1800ms다. P·UX와 조향·브레이크가 모두 유효하고 앱이 전경에 있을 때 주행을 허용한다. 프로필 판별은 에뮬레이터 환경으로 한정한다. 가속페달은 입력으로 사용하지 않는다.

## 선택형 차량 출력

| 기능 | 사용 방식 | 에뮬레이터 확인 결과 |
|---|---|---|
|앰비언트 색·밝기|capability → 현재 값 → setter → getter 재확인|앞·뒷좌석 연동 영역 적용|
|풍량|지원 범위와 영향 영역을 확인해 1~2단계 요청|앞좌석 연동 적용|
|설정 온도|운전석 원래 값에서 최대1°C 낮춤, 20~26°C 범위|얼음 맵22→21°C, 안개 맵22°C 유지|
|공조 전원·AUTO·AC|현재 상태와 영향 영역 읽기|전원·AUTO·AC 강제 전환 없음|
|자동 조명색 모드|capability 확인|선택 속성의 명시적 미지원 응답 처리|

공조는 기존 전원 ON·AUTO OFF 조건에서 적용한다. A/C OFF이면 온도를 유지하고 팬만 사용한다. 색 비교는 에뮬레이터의 RGB24 getter와 ARGB32 요청을 불투명색 기준으로 정규화한다.

## 원상복원

`EffectOwnership`은 최초 쓰기 전에 원래 값과 쓰기 의도를 내부 저장소에 기록한다. 앱이 적용한 값을 유지하고 있는 동안만 해당 설정을 소유한다. 수동 변경이 확인되면 그 기능의 제어를 양보한다.

효과 OFF, 설정창, 맵 전환, 접촉 종료, P 해제, UX 제한, 앱 이탈 때 효과를 중단하고 소유한 설정을 복원한다. 출력 작업은 500ms 간격으로 직렬 처리하고, 1500ms 동안 새 주행 상태가 없으면 독립적으로 출력을 중단한다. 복원 실패 상태는 다음 SDK 연결에서 다시 확인한다.

## Manifest 권한

| 권한 | 사용 |
|---|---|
|`pleos.car.permission.CAR_DRIVE`|조향·브레이크 읽기|
|`android.car.permission.CAR_POWERTRAIN`|기어 상태 읽기|
|`pleos.car.permission.CAR_LIGHTS`|앰비언트 상태 읽기|
|`pleos.car.permission.CONTROL_CAR_LIGHTS`|앰비언트 색·밝기 제어|
|`pleos.car.permission.CAR_CLIMATE`|공조 상태 읽기|
|`pleos.car.permission.CONTROL_CAR_CLIMATE`|풍량·설정 온도 제어|

## 사운드

Unreal AudioMixer에서 주행음을 합성한다. Android 오디오 포커스의 최초 허용 결과와 포커스 변경을 확인해 재생을 제어한다. 이 기능은 앱 오디오 재생이며 Pleos TTS/STT SDK는 사용하지 않는다.

## 코드

- [`PleosRuntime.kt`](../../android/pleosbridge/src/main/java/com/beyondphysics/pleos/PleosRuntime.kt): SDK 세션, 입력, AAOS 제한, 생명주기.
- [`PleosEffects.kt`](../../android/pleosbridge/src/main/java/com/beyondphysics/pleos/PleosEffects.kt): 지원 영역, 읽기·쓰기·적용 확인.
- [`EffectOwnership.kt`](../../android/pleosbridge/src/main/java/com/beyondphysics/pleos/EffectOwnership.kt): 소유권, 수동 변경, 복원 기록.
- [`InputGate.kt`](../../android/pleosbridge/src/main/java/com/beyondphysics/pleos/InputGate.kt): 입력 유효성.
- [`Pleos_UPL.xml`](../../unreal/BeyondPhysics/Build/Android/Pleos_UPL.xml): AAR·권한·GameActivity의 JNI 연결.
- [`PleosVehicleInput.cpp`](../../unreal/BeyondPhysics/Source/BeyondPhysics/Driving/PleosVehicleInput.cpp): Unreal 측 JSON 스키마·입력 검증.

기능 지원 여부는 각 속성과 영향 영역의 capability에서 확인한다. API 연결과 에뮬레이터 적용 결과는 [Release 검증](../RELEASE_VERIFICATION.ko.md)에 있다.

공식 문서: [Vehicle SDK](https://document.pleos.ai/docs/pleos-only/getting-started), [Virtual Console](https://document.pleos.ai/docs/pleos-only/app-test-guide/virtual-console-guide).
