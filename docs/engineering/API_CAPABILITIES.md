# Pleos API 지원 및 검증 현황

## 2026-11-05 최신 출력 연동

기존조향/브레이크·AAOS P/UX 읽기에 선택형 조명·공조 출력을 추가했다. 실제 APK에서 SDK capability/get/set와 getter 재확인을 사용하고, 콘솔의 실제값을 별도로 비교했다. 세 맵 모두APPLIED와 OFF·맵 전환·접촉 종료·P 해제 원상복원을 확인했다. 사용자 색/팬 변경은 덮어쓰지 않고 앱 소유 밝기/온도만 복원했다. Settings 전환으로 실제 백그라운드 복원과 복귀 후 restart=1도 확인했다. AAOS HOME은 내장 앱을 계속RESUMED로 두므로 백그라운드 시험으로 쓰지 않는다. 증거/한계는 docs/research/2026-11-05-map-feedback-results.ko.md와 docs/MAP_FEEDBACK.ko.md를 참조한다.

| 기능 | 실제 지원/사용 영역 | 권한/주의 |
|---|---|---|
|조명 색·밝기|ROW1좌/우+ROW2좌/중/우 연동 영역,READ_WRITE|CAR_LIGHTS + CONTROL_CAR_LIGHTS. 공개 Light.kt와 실제 LightImpl.kt 권한이 달라 내부 구현도 확인했다.|
|풍량|ROW1좌/우 연동,READ_WRITE,현재 앱1~2단계|CAR_CLIMATE + CONTROL_CAR_CLIMATE.0~8을 전 차종 표준으로 가정하지 않는다.|
|설정온도|운전석/동승석 각각READ_WRITE,앱은 운전석만|단위/증분/범위 확인.20~26°C 내에서 원래보다 최대1°C 낮춤.안개에서는 유지.|
|전원·AC·AUTO|전원/AC는 전체좌석,AUTO는ROW1|원본/guard 읽기만 사용하며 강제로 켜거나 끄지 않는다. 모든 출력 영향 영역을 guard가 덮는지 확인한다.|
|주행모드 자동 조명색|capability API가명시적 ERROR_NOT_SUPPORTED_PROPERTY 반환|이 선택 기능만 생략하며 색/밝기는 별도 지원을 확인했다. 권한·타임아웃·getter 오류와 구분한다.|

SDK getter는 조명RGB24를 반환(흰색16777215,청록3382463)하고 요청은ARGB32이므로 emulator-only 불투명색 정규화로 비교·복원한다. 콜백 성공만으로 실제 적용을 주장하지 않는다. 사운드는 Unreal 자체 합성/AudioMixer이며 Pleos TTS가 아니다. Android 오디오 포커스의 실제 최초 허용과 이벤트를 따로 처리한다. PC는SIMULATED이며 실차 활성화/허가·마켓 권한 승인·실차 체감검증은 미수행이다. 아래 예전 API 미구현/권한 설명은 과거 이력이다.

2026-10-29 최신 통합: Unreal x86_64 APK에 Vehicle SDK2.3.2 조향·브레이크 읽기와 AAOS P/UX 게이트를 AAR/JNI로 직접 연결했다. 실제 APK에서 좌우 선회, 제동/자동 가속 차단, P→D→P 및 앱 복귀 후 재시작 잠금, SDK 서비스 강제 종료 후 stale 차단·재연결·새 브레이크 입력을 확인했다. 현재 권한은 CAR_DRIVE와 CAR_POWERTRAIN 읽기이며 액추에이터 쓰기·가속페달 읽기는 없다. 에뮬레이터 raw/540 보정만 허용하고 실차에는 활성화하지 않는다. 증거: docs/research/2026-10-29-native-vehicle-input.ko.md. 아래 미검증 문구는 초기 진단 이력이며 장시간 정리·실차 승인과 혼동하지 않는다.

2026-10-29 사용자 결정: 실제 가속페달은 앱 입력에서 제외한다. 가상 차량은 자동 가속하고 브레이크를 누르는 동안 구동력을 끊고 제동한다. PC에서는 S로 확인한다. SDK 브레이크 눌림 여부를 가상 제동0/1로 매핑했으며 페달 압력 측정으로 설명하지 않는다. 실제 브레이크·핸들의 차량 영향은 남아 있으므로 제조사 승인 전 실차 게임 조작 테스트는 하지 않는다. 에뮬레이터 통합 성공과 실차 승인을 구분한다.

확인일: 2026-10-28 KST. 프로젝트: BEYOND PHYSICS.

현재 상태는 **공식 문서 확인 및 에뮬레이터에서 조향·브레이크 capability/get/callback 확인**이다. 실차 검증과 마켓 권한 승인은 수행하지 않았다. API 존재, 프로젝트 사용 상태, Manifest 권한, 마켓 승인, 차량 capability, 실제 동작을 각각 확인해야 한다. 조사 전체는 [공모전 조사](docs/research/CONTEST_RESEARCH.md), 실행 결과는 [실험 기록](EXPERIMENT_LOG.md)을 참조한다.

## 1. 기준 버전과 제공 상태

문서 기준 Vehicle SDK 2.3.2, Emulator 2.1.0. Vehicle 소개의 Service는 2.3.21이며 공식 샘플의 관측 환경은 2.3.22이다. 실행 환경 버전은 아직 미확인이다.

프로젝트 생성 가이드는 Vehicle State·NaviHelper를 즉시 사용 가능, Vehicle Control·FusedLocation·TTS·STT를 준비 중으로 안내한다. 계정별 실제 상태는 확인하지 않았다. [신청 안내](https://document.pleos.ai/docs/pleos-only/create-my-project)

## 2. 조향 입력

공개 API:

- `checkSteeringWheelAngleCapabilities`
- `getCurrentSteeringWheelAngle`
- `registerSteeringWheelAngle` / `unregisterSteeringWheelAngle`

getter는 `Float?`를 비동기로 반환한다. **전륜 조향각**, 단위 degree, 왼쪽 음수이며 핸들 회전각과 다르다는 명세를 확인했다. getter와 register의 요구 권한은 `pleos.car.permission.CAR_DRIVE`이다. [getter](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/steering-wheel/get-current-steering-wheel-angle) [register](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/steering-wheel/register-steering-wheel-angle)

콘솔 사용 문서는 같은 이름의 조절 항목을 스티어링 휠 각도 -540~540도로 설명한다. 이 범위를 실제 SDK 전륜각으로 간주하지 않는다. 콘솔값→getter→callback의 대응을 측정하고, 실제 의미 확인 전 조향비를 적용하지 않는다. 물리 엔진의 radian 및 좌표계 변환도 별도로 명시한다. [콘솔](https://document.pleos.ai/docs/pleos-only/app-test-guide/virtual-console-guide)

검증 대기: capability 범위, null·오류, 부호, 반복성, 갱신 빈도, 지연·지터, 입력 누락, 구독 해제. 갱신 주기나 실시간성이 보장된다는 근거는 확인하지 못했다.

## 3. 브레이크 입력

공개 `getCurrentBrakePedalStatus`는 `Boolean?`이며 true는 페달이 눌렸음을 뜻한다. `registerBrakePedalStatus`와 해제 API가 있다. getter·구독 권한은 `CAR_DRIVE`이다. 연속 페달 위치나 압력이 아니다. [getter](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/brake/get-current-brake-pedal-status) [register](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/brake/register-brake-pedal-status)

콘솔 Brake Pedal은 0~100%를 설정하고 작동 상태를 파생시킨다. 이 UI가 연속값 공개 API의 증거는 아니다. 눌림을 가상 제동 명령으로 변환한다면 입력 모델로 명시해야 하며, 실차 페달 압력을 측정한 것처럼 설명하지 않는다.

검증 대기: 콘솔 0/중간/100%에서 Boolean 변화, 임계값·null·반복 입력, listener 동작. 현재 단계에서 제동 모델을 선택하거나 구현하지 않았다.

## 4. 가속·속도·기어·주차

검토한 공개 Vehicle SDK 도메인·샘플 카탈로그에는 가속 페달이나 Gear 도메인을 확인하지 못했다. 이를 모든 플랫폼에서 불가능하다는 의미로 확대하지 않는다. 샘플도 Gear를 공개 카탈로그에서 제외한다고 설명한다. [공식 샘플](https://github.com/Pleos-Playground/pleos-sample-vehicle-app)

SDK 릴리즈 노트는 `VehicleSpeedListener` 삭제를 명시한다. 옛 예제를 재사용하지 않는다. [변경 이력](https://document.pleos.ai/docs/pleos-only/release-note/sdk-261015)

AAOS에는 `GEAR_SELECTION`과 `PERF_VEHICLE_SPEED`가 정의되어 있다. 하지만 Pleos 대상 환경의 지원·접근 가능 여부는 별도 확인해야 한다. Google 명세상 기어는 읽기 전용, `Car.PERMISSION_POWERTRAIN`이 필요하고 속도는 m/s이며 별도 권한이 필요하다. [AAOS 프로퍼티](https://developer.android.com/reference/android/car/VehiclePropertyIds) [Pleos 가용성](https://document.pleos.ai/docs/pleos-only/vehicle-app-planning-guide/api-compatibility-availability)

게임 허용 판단은 공식 `CarUxRestrictionsManager` 연동이 필요하다. `registerListener()`/`onUxRestrictionsChanged()`로 상태를 받고 최초 상태도 확인해야 한다. 게임은 운전 시작 시 중단·UI 숨김·재개 차단 대상이다. `NO_VIDEO` 하나가 모든 게임 규칙을 대체한다고 가정하지 않는다. 신호나 정책 상태가 없을 때 플레이를 허용하지 않는 것은 프로젝트 안전 원칙에 따른 방침이다. [개발 가이드](https://document.pleos.ai/docs/pleos-only/vehicle-app-developer-guide/driving-restriction-development-guide) [심사 규정](https://document.pleos.ai/docs/pleos-only/distribute/app-review-guide/review-driver-distraction-elements)

검증 대기: P/R/N/D 전환, 제한 콜백, 초기 상태 미확인, 서비스 단절, foreground 복귀 때 재확인. 실차 페달·핸들 사용 승인은 별도이며 테스트하지 않는다.

## 5. 앰비언트 조명

Light 도메인에 밝기·색상 capability/get/set/callback이 있다. `setAmbientLightBrightnessColor(area, value, ...)`는 SeatArea와 32-bit ARGB, alpha 0xFF를 사용한다고 명시한다. [색상 setter](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/light/set-ambient-light-brightness-color)

**개별 색상 setter 명세의 권한은 `CAR_LIGHTS`로 표시된다.** 일반적으로 setter가 `CONTROL_CAR_*`라는 설명만으로 이 값을 바꾸지 않는다. 실제 배포 SDK의 어노테이션·서비스 검증이 필요하다. 콘솔의 색상 4개는 콘솔 UI 선택지이며 SDK 전체 색상 지원 목록이라고 단정하지 않는다.

검증 대기: 읽기/쓰기 accessType, SeatArea, 밝기 범위, setter 후 getter/이벤트의 실제 반영, 종료 시 복원, 사용 승인. set 완료 콜백만으로 실제 장치 변화 완료를 주장하지 않는다.

## 6. 공조와 시트

`setHvacFanSpeed`는 SeatArea와 Int 풍량 단계를 받고 `CONTROL_CAR_CLIMATE`를 요구한다. 범위는 고정이 아니며 `checkHvacFanSpeedCapabilities`가 주는 영역별 `AreaBounds<Int>`를 사용한다. 콘솔의 0~8단계를 전 차종 규격으로 삼지 않는다. [풍량](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/h-v-a-c/set-hvac-fan-speed)

HVAC에는 목표온도 setter가 있고 Seat에는 위치·각도·열선·통풍 계열 API가 있다. `setHvacSeatTemperature`의 값은 온도 제어 단계이며 `CONTROL_CAR_SEATS`가 필요하다. 섭씨 온도값으로 해석하지 않는다. [HVAC](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/h-v-a-c/entry) [시트 온도](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/seat/set-hvac-seat-temperature)

전용 진동·충격·핸들 토크 피드백 API는 검토한 공개 목록에서 확인하지 못했다. 좌석 위치 이동은 진동 API의 근거가 아니다. 차량의 실제 물리 제어·센서 직접 접근은 금지된다. [하드웨어 정책](https://document.pleos.ai/docs/pleos-only/vehicle-app-planning-guide/platform-api-architecture)

승인·실행 검증 전까지 감각 효과는 모의 출력 후보이다. 기능을 도입할 경우 선택적 사용, 빈도 제한, 원래 설정 기록·복원, 사용자가 바꾼 설정과의 충돌 처리를 검증할 필요가 있다. 이는 사용자 프로젝트 지침에 따른 요구이지 SDK가 복원을 보장한다는 뜻이 아니다.

## 7. 연결 및 오류 처리

문서의 기본 흐름: Vehicle 생성 → initialize → getXxx 도메인 → capability → 조회/구독/허용된 제어 → unregister → release. 비동기 작업 종료 전에 release하지 않는다. [Vehicle 소개](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/intro)

capability의 AccessType은 NONE/READ/WRITE/READ_WRITE이며 영역별 허용 범위를 따로 확인한다. [Capability](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/check-capability)

`onFailed(Exception)`에서 VehicleException이면 errorCode를 확인하고, 일반 Exception도 처리한다. 주요 분기는 미지원 속성·영역, 권한 부족, 서비스 연결 실패, 주행 제한, 빈 결과다. [오류](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle-error-code)

SDK의 비동기 입력과 물리 고정 시간 적분의 결정론은 별개다. 향후 입력 기록을 시뮬레이션 tick에 대응시키고 재생 조건을 기록해야 한다는 것이 프로젝트 지침에 따른 검증 요구다. 이 SDK가 결정론적 차량 시뮬레이터를 제공한다는 의미는 아니다.

## 8. 실행 검증 현황

| 항목 | 상태 |
| --- | --- |
| Windows 개발 도구·GPU 적합성 확인 | 미수행 |
| Vehicle 2.3.2 의존성 resolve 및 빌드 | 캐시 SDK 확인 및 assembleDebug 성공 |
| Pleos AVD 2.1.0 설치·실행 | 사용자 화면 및 ADB emulator-5554 device 확인. Cold boot 설정은 별도 미확인 |
| Virtual Console 연결 | 8088 포워딩 및 브라우저 DDS ONLINE 확인 |
| 조향·브레이크 capability/get/callback | 두 기능 READ, 초기값·변경 알림 확인. 상세는 EXPERIMENT_LOG.md |
| 앱 정지 후 재진입 | 내비게이션 전환 1회: 구독 해제·release, 최신 조향 재조회 및 변경 알림 확인(P0-003). 프로세스 종료·장시간 반복은 미검증 |
| D 상태에서 새 프로세스 시작 | UNKNOWN→BLOCKED, DO=true/flags=80 및 플랫폼 제한 화면 확인. P 복원 후 정상 진단 화면 확인(P0-004) |
| SDK 서비스 단절·복구 | 실제 서비스 종료 시 이전 입력값 잔류: 수정 필요. 앱 새 프로세스로 재시작해 복구 확인. 자동 복구·AAOS 단절 시험은 미검증(P0-005) |
| 권한 없는 설치 | 별도 APK에서 CAR_DRIVE/CAR_POWERTRAIN 제거: 실제 오류 표시, 기어 미확인·UNKNOWN 확인(P0-006) |
| 재진입 반복·정책 회귀 | STOPPED/release/최신 get 5회 모두 통과. 정책7개+기존 예제1개 재실행 통과(P0-007) |
| 기능 미지원·실제 null·영구 무응답 | 미검증. 권한 오류 시험과 구분 |
| CarUxRestrictions 및 기어 전환 | AAOS 공개 API로 P→D→P 확인. 미확인·제한 판정 단위 테스트 통과. 실제 서비스 장애 경로는 미검증 |
| 조명·공조·시트 제어 권한 승인 | 미확인 |
| setter→조회·알림 반영 및 복원 | 미수행 |
| release APK 설치·화면·사운드 | 미수행 |
| 물리 모델의 분석해·수치 오차 검증 | 미수행 |

향후 실험 기록에는 APK/SDK/서비스/이미지 버전, capability, 입력·출력, 단위, 타임스탬프, 오류, 실제값 반영 여부를 포함한다. 이 목록은 검증 대기 현황이며 구현 완료나 Phase 0 통과를 뜻하지 않는다.

## 9. 에뮬레이터 연결 확인 기록

2026-10-27 KST에 사용자가 제공한 화면에서 Pleos Connect 2.1.0 실행 및 P 표시를 확인했다. 설치된 ADB는 `C:\Users\parks\AppData\Local\Android\Sdk\platform-tools\adb.exe`이며 `adb devices` 결과는 `emulator-5554 device`였다. 해당 기기에 `tcp:8088` 포워딩을 설정하고 `http://127.0.0.1:8088/`에서 Pleos Connect Virtual Console의 `DDS ONLINE`, `PARKED`, `P` 표시를 확인했다. 콘솔의 조작값을 변경하거나 SDK 앱에서 입력을 읽는 시험은 수행하지 않았다.

2026-10-28 KST 후속 실험에서는 진단 앱을 설치·실행하고 조향과 브레이크 입력을 변경했다. 콘솔 조향 +90/-90도에서 SDK는 각각 -90/+90도를 전달했다. 브레이크 0→100→0%에서 false→true→false를 확인했다. 조향 capability는 READ 및 -470~500도 범위를 보고했으나 이를 실차의 물리적으로 유효한 전륜각 범위라고 해석하지 않는다. 문서 의미와 콘솔 구현의 관계는 아직 미해결이다.

같은 날 사용자는 콘솔 음수에서 오른쪽, 양수에서 왼쪽으로 바퀴가 돌아간다고 관찰했다. 기존 SDK 수신 부호와 합치면 SDK 양수=오른쪽, 음수=왼쪽으로 문서 정의와 일치한다. 이는 방향에 대한 사용자 관측이며 실제 전륜각 크기나 조향비를 검증한 것은 아니다. 진단 코드는 변경하지 않았다.

후속 묶음 시험 P0-005에서 설치된 SDK 전용 Vehicle Service의 package versionName **2.3.22**를 dumpsys package로 확인했다. 클라이언트 Vehicle SDK 2.3.2와 서비스 버전은 서로 구분한다.

## 10. AAOS 주행 제한 및 P 기어 신호

2026-10-28 KST에 `android.car` 공유 라이브러리와 `android.car.permission.CAR_POWERTRAIN` 조회 권한을 추가했다. Pleos SDK의 비공개 Gear를 사용하지 않고 AAOS 공개 `GEAR_SELECTION`을 읽는다. 플랫폼 UX 신호는 `CarUxRestrictionsManager`의 초기값과 변경 콜백으로 읽는다. 실제 기어 제어 호출은 하지 않는다.

P 상태에서 DO=false/flags=0, D에서 DO=true/flags=80, P 복원 시 false/0을 확인했다. 관측 순서는 [실험 기록](EXPERIMENT_LOG.md)의 P0-002를 참조한다. 사전조건은 P 확인과 UX 무제한을 동시에 요구하며 알 수 없는 값은 허용하지 않는다. DO 선언·제한 우회는 추가하지 않았다. 이 화면에는 게임이 없으므로 실제 플레이 중단·게임 UI 숨김 검증은 아직 해당하지 않는다.

출처: [Pleos 주행 규제](https://document.pleos.ai/docs/pleos-only/vehicle-app-developer-guide/driving-restriction-development-guide), [AAOS UX API](https://developer.android.com/reference/android/car/drivingstate/CarUxRestrictionsManager), [기어 속성](https://developer.android.com/reference/android/car/VehiclePropertyIds#GEAR_SELECTION).
