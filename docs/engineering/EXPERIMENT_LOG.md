# BEYOND PHYSICS 실험 기록

## FEEDBACK-001 세 맵 조명·공조·사운드 (2026-11-05)

효과 정책10개검사 실패0, Kotlin 소유권/복원/수동변경/심박/영역/색호환과 기존 입력 검사22개 실패0. 검토의 복원 재시도·Android오디오 포커스·영역 전체 guard 보장 문제를 수정했고 실패 회귀→통과를 확인했다. 실제 SDK에서 팬/전원/온도 영역 차이, LightImpl 추가제어권한, 명시적 ambient-auto미지원, RGB24 getter/ARGB 요청 차이를 관측해 반영했다. 물리코어7파일·맵3개 해시 동일.

최종 APK와 Editor 빌드/설치 성공. 실제 SDK getter+콘솔 비교: 얼음01/02는 밝기20%,청록~호박,팬2/운전석21°C/동승석22°C. 안개03은청회색/밝기20%,팬1/온도22°C유지. 원본은흰색70%/팬3/양쪽22°C이며 OFF·맵 이동·접촉 종료·P 해제 후 복원을 확인했다. 수동색#123456/팬4는 앱이 그대로 유지하고 앱소유 밝기/온도만 복원했다. AC OFF는 팬만 적용/복원했다. AAOS HOME은 앱을RESUMED로 두어 무효 시험으로 분류하고 Settings전환에서 실제배경 복원/복귀restart=1을 확인했다. 실제터치설정열기/개별토글/적용재시작/설정창복원/최종OFF 대기를 검증했다.

최종APK의 마스터AudioMixer녹음3개를 자기앱 디버그run-as로 읽었다.48kHz/2채널,2.94~3.96초,비영PCM/클리핑없음. 측정은 디지털출력 확인이며 실차스피커·열체감·주관음질 검증이 아니다. 경쟁 외부앱의 오디오포커스 손실을 강제로 만드는 시험은 이번에 하지 않았고 플랫폼초기허용/콜백 구현을 소스로 검토했다. 강제종료/서비스장애 복원은 journal 기반최선노력이며 즉시보장이 아니다.

APK198819176bytes/SHA256 b7ff5a73d4129c9181271bc7f59719e1e1d1c7789b4ecb624361a5ae7314533e. 증거 docs/research/map-feedback-final-evidence.json 및 2026-11-05-map-feedback-results.ko.md. 콘솔원래설정복원/P/차량속도0/가속0/브레이크0/핸들0,게임설정창열림·차량효과OFF·소리ON으로마무리했다.

## FOG-002 짧은 안개·3대 차량·네 접지점 (2026-11-04)

휴대 가능한 C++ 검사에서 Ackermann/접지점 속도/바퀴별 마찰원/힘·모멘트 합산/영윤거 bicycle 비교/좌우·월드 회전 대칭/에너지 소산을 통과했다.5m/s 정상선회 r=0.00900785rad/s(선형 기준0.00900826),25m/s r=0.0269557(기준0.0272879).100km/h 직선 제동49.1594m. 세 차량 OBB·연속 접촉·기준 대상 고정·예방 정지 검사를 통과했다. 기록 대상 혼합 회귀는 RED→GREEN으로 확인했다. 기존 물리/램프/학습 검사도 통과했다.

같은 코어 시나리오에서100m 부근 예방 제동은49.1568m 후 정지,30m 기준0.503524초 뒤 제동은접촉,미제동/늦은 최대 조향도 접촉했다. 이는 지정 입력에 대한 결과이며 모든 회피가 불가능하다는 증거는 아니다.

최종 APK198708712bytes/SHA256 `2a88c9fc9374519c15cbcda1becee38f462b5653c3d50ab2659e5956ef955abd`. Editor/Android cook 빌드 성공·ADB 설치 성공. 실제 Pleos SDK에서초기80→100,raw핸들-90°→중앙-5°/FL-4.852923°/FR-5.156246°,미제동9.468143초접촉,근접최대조향9.477018초접촉을 확인했다. SDK 예방 제동은49.156398m 후 정지/범퍼간격52.620302m였다. 자동 시험이며 사람 반응 측정이 아니다. D 허용0/P 복귀허용1·restart1, 실제 차량 신호P/속도0/가속0을 확인했다. CSV/summary 저장 성공 로그는 있으나 private 파일 내용 추출 검증은 아니다.

실제 HTTP Unreal MCP의 현재 맵 조회 결과 `/Game/Maps/FogHighwayDrive`, 재접속과 뷰 캡처 성공. 기존 두 얼음 맵 해시 동일. 모든 근거·로그·한계는 docs/research/2026-11-04-fog-feedback-results.ko.md 및 fog-feedback-final-evidence.json을 참조한다.

## P1-001 Pleos 연동 준비와 제출 가능성 조사 (2026-10-29)

adb에서 기존 Pleos_Connect_v2.1.0 AVD를 실행하여 ro.product.cpu.abilist=x86_64, ro.dalvik.vm.native.bridge=0을 확인했다. 설치 엔진의 Android 플랫폼/NDK 구성은 누락돼 있다. Epic5.4 릴리스 노트 및5.8 Android Emulator 문서와 이전 설정 문서의 상충을 확인했고 '소스 빌드 필수' 단정을 정정했다. 공모전 규정의 Unreal 명시 허용/금지는 확인하지 못했으며 운영사 답변은 아직 없다.

공용 pleosbridge 모듈의 입력 유효성/신선도 및 에뮬레이터 프로파일 정책을 준비했다. :pleosbridge:testDebugUnitTest와 기존 :app:assembleDebug가 BUILD SUCCESSFUL(종료코드0),60 tasks executed. 로그 docs/research/pleos-bridge-preparation-build.log. 새 모듈의 Vehicle2.3.2/TTS3.3.1 의존성도 해결됐다. 기존 진단 앱 빌드 성공이며 Unreal APK·SDK 런타임/JNI·PC 중계의 완성을 뜻하지 않는다. 사용자 후속 요청에 따라 세부 연동 구현보다 최소 Unreal APK의 Pleos 실행 검증과 공식 출품 조건 확인을 우선한다.

## V1-004 자동 가속·브레이크 중심 조작과 Alpine V2 (2026-10-29)

최종 시각 조정: MI_AlpineRockSnow_V2의 차가운 암석 색상, Sun 고도24도/중립광, 노출보정-1.25로 노란빛과 눈부심을 줄였다. 실제1600×900 실행 캡처는 docs/research/auto-brake-alpine-20261029.png, 최종 실행 로그는 auto-brake-alpine-verified.log다. 이전 검사에서 자동 주행84km/h, 벽 접촉 종료, R 재시작 화면을 확인했다. 첫 캡처는 렌더링 준비 중이라 속도0일 수 있다.

사용자 결정으로 일반 조작의 W/가속페달 입력을 제외했다. AutoDriveInput은 브레이크0일 때 Throttle1, 브레이크>0이면 Throttle0으로 만든다. 기존 힘 모델은 유지한다. 네이티브 검사에서 자동 출발·부분/전체 제동 시 구동 차단·제동 정지·해제 재가속과 기존 물리 검사 failures=0. 로그 docs/research/auto-brake-model-tests.log. Unreal Win64 Development Editor 전체 빌드 Succeeded, 종료코드0(auto-brake-unreal-build.log).

게임에서 W 입력 없이 자동 가속하여84km/h가 표시되는 것을 확인했다. 벽 접촉 후 정지 및 R 재시작에서 속도0·시작 위치로 복원하는 화면도 확인했다. BP_WALL_CONTACT/BP_STATE로 crashed=1·paused=1이 기록됐다. S 키를 길게 누르는 동작의 별도 계측은 하지 않았으며 제동 정책·정지는 동일 순수 C++ 계산으로 검사했다. 초기 렌더링 로딩 때문에 시작 직후 FRAME STALL 정지가 발생하여, 시뮬레이션 시작 전0.75초의 안정된 프레임 준비 구간을 추가했다. 주행 중 프레임 지연 정지는 유지한다.

도로 양옆60m 간격의9m 가로등68개, 발광 패널과 거리 제한 스포트라이트를 저장했다. 산을321×321/204800삼각형의 능선 지형으로 교체하고 CC0 실사 암석2K 색·DirectX 노멀·거칠기 맵, 세 방향 색 투영과 경사·높이별 눈 마스크를 적용했다. 미세 노멀은 완만한 표면에 가중 적용한다. 원본 산은 숨겨 보존했고 예전 얼음 시험장용 Reflection_Ice 캡처는 백업을 남기고 레벨에서 제거했다. 산/가로등 재질은 광학 표현이며μ를 바꾸지 않는다.

오프라인 에디터 스크립트 upgrade_alpine_road.py와 finalize_alpine_look.py 실행 성공. Saved/alpine-v2-report.json 참조. 최초 재질 노드 연결에서 UE의 Coordinates→UVs/Input→빈 이름 변환으로 실패했고 공식 설치 엔진 소스 확인 후 수정했다. 첫 게임 캡처의 체크 패턴은 새 셰이더 준비 중이었으며 완료 후 실사 텍스처 표시를 확인했다. 숨긴 원본 M_Alpine의 잘못된 과거 연결 경고는 남지만 새 산 재질은 정상 표시된다.

지침 PROJECT_GUIDELINES.md(v1.1), 설계 docs/superpowers/specs/2026-10-29-auto-brake-alpine.md, 실행 가이드와 API/모델/결정 기록을 갱신했다. 외부 자산 출처는 SourceArt/AlpineV2/ASSET_SOURCES.md. 복구 사본은 backups/auto-brake-alpine-20261029. PC 화면 검증이며 실차 입력 안전 승인, Pleos APK 성능, 사진과 같은 완성도를 보장하는 검증은 아니다.

## V1-003 가속 증가와2km 도로 (2026-10-29)

저마찰 기본 프로파일을μ0.12→0.18로 조정했다. `Tests/run-model-tests.cmd` 실행 종료코드0, failures=0. 새 가속1.7658m/s²,10초 직선 정지 출발63.5688km/h 및 기존 프로파일 대비1.5배 검사가 통과했다. μ0.18에서 초기15m/s 제동거리63.7104957011m, 분석해63.7104994903m. 이전μ0.12 및 일반μ0.85 제동, 회전 불변성, 마찰원, 안정성/재생 검사도 유지했다. 로그 docs/research/longer-faster-model-tests.log.

MCP 저장 후 실제 도로·갓길·실선·양옆 벽·상단부의 X스케일2000(기본1m 큐브 기준 길이2km), 차선442개와 반사 표지162개를 재조회·검증했다. docs/research/longer-faster-road-verification.json. C++의 출발점-440m/끝 제한±995m를 함께 수정했으며 옛495m 제한을 넘어 주행 가능한 경계 검사도 통과했다.

LiveCoding 성공 후 에디터를 저장·종료하고 Win64 Development Editor 전체 빌드 Succeeded(종료코드0). 독립 게임 재실행에서 BP_STATE time=0, x=-440, speed=0, mu=0.180, paused=0, crashed=0 확인. 1280x720 실제 게임 화면의3차로/벽/μ0.18/속도0을 확인했다. 로그 docs/research/longer-faster-unreal-build.log, longer-faster-game.log, 화면 longer-faster-road-20261029.png. 실제 게임에서10초 전가속을 측정한 것은 아니며 해당 속도는 동일 C++ 코어의 네이티브 검사 결과다. 차량 힘 방정식 소스 SHA256 32CEEB6627C019976730DC37256D1130A0FEA1661BF50000BDB26F776C9E46E2는 변경하지 않았다.

## P0-001 조향 및 브레이크 상태 조회와 변경 알림

실험일: 2026-10-28 KST. 목적: 가상 차량 상태가 공개 Vehicle SDK를 거쳐 앱에 도달하는지 확인한다. 물리 모델 정확도를 평가하는 실험은 아니다.

### 환경

- Windows / Android Studio의 Kotlin·Compose 프로젝트.
- Pleos Connect Emulator 2.1.0, API 34, `emulator-5554`, Android 사용자 10.
- Vehicle SDK 2.3.2. 실제 Vehicle Service 버전은 별도 미확인.
- Manifest 권한: `pleos.car.permission.CAR_DRIVE`.
- 앱 ID: `com.example.beyondphysics`.
- 빌드: `gradlew.bat :app:assembleDebug --offline --console=plain` 성공.
- 산출물: `android/app/build/outputs/apk/debug/app-debug.apk`.
- APK SHA-256: `42836BF312CDE06F02B9AA0732C3BD6A3ED5F78B9E9DB65A113072F46097E877`.

### 방법과 관측

앱에서 initialize → 도메인 capability 확인 → 초기값 조회 및 register를 호출했다. Virtual Console의 Emulation Controls/Motion에서 입력을 변경하고, 앱 화면과 해당 프로세스의 SDK 로그를 비교했다. 기어는 P로 유지했다.

| 조작 / 항목 | 관측 |
| --- | --- |
| 조향 capability | READ, 범위 -470~500 degree |
| 브레이크 capability | READ |
| 초기 조향 | 0.0 또는 -0.0 degree |
| 콘솔 조향 +90 degree | SDK PERF_STEERING_ANGLE = -90.0 |
| 콘솔 조향 -90 degree | SDK PERF_STEERING_ANGLE = +90.0 |
| 콘솔 조향 0으로 복원 | SDK -0.0 |
| 브레이크 초기 0% | false |
| 브레이크 100% | true |
| 브레이크 0%로 복원 | false |

브레이크 변경 알림은 false→true→false 순으로 로그에 남았다. 앱 화면에서도 해제(false) 및 변경 알림 횟수 증가를 확인했다. 조향은 동일 값의 주기적 콜백도 관측되어 화면의 알림 횟수는 고유 입력 변경 횟수가 아니다.

원본 근거: [SDK 응답 로그](docs/research/phase0-input-probe-log.txt). AVD 내부 로그 시계는 호스트 KST 일시와 다르므로 로그에 표시된 날짜를 실제 실험일로 사용하지 않는다.

### 사용자 추가 관측 조향 방향

2026-10-28 KST에 사용자가 콘솔 Steering Angle 음수에서 화면의 바퀴가 오른쪽, 양수에서 왼쪽으로 돌아간다고 보고했다. 앞서 확인한 SDK 부호 대응과 합치면 다음과 같다. 시각적 방향은 사용자 관찰이며 전륜각 크기를 측정한 결과는 아니다.

| 콘솔 Steering Angle | 사용자 관측 방향 | 기존 실험에서의 SDK 부호 |
| --- | --- | --- |
| 음수 | 오른쪽 | 양수 |
| 양수 | 왼쪽 | 음수 |

이 대응은 SDK 문서의 왼쪽 음수 규칙과 일치한다. 출처: [조향각 명세](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/steering-wheel/get-current-steering-wheel-angle). 따라서 현재 진단 화면은 SDK 원본 부호를 유지한다. 콘솔의 핸들 각도와 SDK 전륜각의 크기·조향비 대응 문제는 여전히 남아 있다.

### 결론과 한계

이 에뮬레이터에서 두 공개 입력 API의 capability/get/callback 경로가 동작한다. 부호를 뒤집거나 조향비를 적용하지 않았으며, 콘솔 수치와 SDK 수치가 위처럼 관측되었다는 사실만 확인한다. 전륜각이라는 문서 정의가 에뮬레이터의 핸들 각도와 어떻게 대응하는지, 실차에서는 어떤 의미인지 아직 검증되지 않았다.

브레이크 중간값·임계값, 정확한 지연·지터, 범위 경계, null/권한 거부/미지원 기능, 서비스 강제 단절, Activity 전환 후 복구, release 빌드 및 실차는 미검증이다. 물리 검증이나 게임 실행 안전 조건의 통과를 뜻하지 않는다. 테스트가 끝난 뒤 콘솔 조향 0·브레이크 0%로 복원했다.

### 사용자가 재현하는 방법

1. Android Studio에서 `app`을 Pleos 에뮬레이터에 Run한다.
2. 화면에 두 지원 상태가 READ로 나타나는지 확인한다.
3. Virtual Console의 Emulation Controls → Motion을 연다.
4. Steering Angle을 좌우로 바꾸고 앱의 원본 수치와 알림 횟수를 확인한다.
5. Brake Pedal을 0→100→0%로 바꾸고 해제→눌림→해제를 확인한다.
6. 마지막에 조향 0·브레이크 0%로 되돌린다.

## P0-002 주차 및 플랫폼 UX 제한 사전조건

실험일: 2026-10-28 KST. 목적: 기어 P와 플랫폼 주행 제한을 함께 확인하고, 상태 미확인 시 허용하지 않는 판정 경로를 검증한다. 실제 게임·실차 테스트는 아니다.

### 환경과 변경

P0-001의 동일 Android/Emulator 환경에서 공개 AAOS `android.car` 라이브러리를 추가했다. 기어 읽기에 `android.car.permission.CAR_POWERTRAIN`을 선언했다. 기어·UX 상태를 읽기만 하며, DO 메타데이터를 선언하지 않았다.

최종 debug APK SHA-256: `F298E89A2ACF273B1826E3739DEF15B9A13D7D8F34D3E46F65787C9D05B5C73D`.

판정 규칙은 모든 신호가 알려져 있고 P/DO=false/flags=0이면 ELIGIBLE, 확인된 제한 또는 non-P가 있으면 BLOCKED, 그 밖의 누락은 UNKNOWN이다. ELIGIBLE은 플랫폼 사전조건 충족이며 장치 사용 승인·배포 심사 통과를 의미하지 않는다.

### 자동 테스트

UNKNOWN만 반환하는 stub에서 7개 정책 테스트 중 6개가 의도대로 실패했다. 정책 구현 후 7개와 기존 예제 1개, 총 8개가 모두 통과했다. 검사 범위는 정상 P, 각 필수 신호 누락, non-P, DO만 활성, 알려진·미래 플래그, 부분 정보에서의 차단, 초기화된 상태의 허용 취소다. 서비스 연결 자체를 모의로 검증한 테스트는 아니다.

`gradlew.bat :app:testDebugUnitTest :app:assembleDebug --offline --console=plain` 성공. Kotlin 컴파일 경고는 API34 호환 콜백 사용 위치를 명시한 뒤 최종 빌드에서 발생하지 않았다.

### 에뮬레이터 관측

| 순서 | 기어 | DO 요구 | UX flags | 판정 |
| --- | --- | --- | --- | --- |
| 초기 연결 | 미확인 | null | null | UNKNOWN |
| UX 초기값 수신 | 미확인 | false | 0 | UNKNOWN |
| P 수신 | P | false | 0 | ELIGIBLE |
| D 기어 수신, UX 갱신 전 | D | false | 0 | BLOCKED |
| D UX 갱신 | D | true | 80 | BLOCKED |
| P 복원, UX 해제 전 | P | true | 80 | BLOCKED |
| P UX 해제 | P | false | 0 | ELIGIBLE |

근거: [AAOS 진단 로그](docs/research/phase0-driving-safety-log.txt). 마지막 기어는 P로 복원했다. 사용자가 설정한 조향값 240도 등 다른 콘솔 설정은 변경하지 않았다. AVD 내부 시계는 실제 KST 날짜와 다르다.

### 검토와 남은 범위

별도 코드 검토에서 수정이 필요한 정확성 결함은 발견되지 않았다. 실제 서비스 강제 단절·재연결, 지연 콜백, 권한 거부·미지원 속성, API28·android.car 미탑재 환경, Binder read가 끝나지 않는 경우의 worker 종료는 실행 검증하지 않았다. epoch/active 방어 로직의 존재와 순수 판정 테스트를 이러한 통합 시험의 대체 근거로 사용하지 않는다.

D에서 이 실험의 진단 Activity는 foreground에 남는 것을 확인했다. 플랫폼이 게임 UI를 자동으로 숨겨 준다고 가정하지 않는다. 향후 게임 구현 시 BLOCKED/UNKNOWN에서 시뮬레이션·게임 화면·사운드를 중단하는 동작을 별도로 연결·검증해야 한다.

## P0-003 앱 정지 및 재진입 후 최신 입력 복구

실험일: 2026-10-28 KST. Pleos Emulator 2.1.0/API34, emulator-5554, 사용자10, 앱 PID3538. 코드 수정·재빌드 없이 현재 설치된 앱을 시험했다. 설치 base.apk SHA-256은 `20C343CB2034C0BBD9FB215413DA92E570615567A52D55B82A5C02B8F99E30B1`이다. P0-002 산출물 해시와 다르며 같은 바이너리라고 주장하지 않는다. 로컬의 기존 debug APK 경로에는 파일이 없었다.

### 방법과 관측

P·조향0에서 진단 앱을 열었다. Pleos에서 홈 버튼 및 HOME intent는 오른쪽 앱을 정지시키지 않아, 같은 화면의 내비게이션 Activity로 전환해 실제 onStop 경로를 시험했다. 앱 강제 종료나 데이터 삭제는 하지 않았다.

| 단계 | 관측 |
| --- | --- |
| 초기 진단 화면 | P, DO=false, flags=0, 조향 -0.00°, 브레이크 false |
| 내비게이션 전환 | SDK 조향·브레이크 removeRequest, Vehicle release 및 clearAllListeners 로그 확인 |
| 정지 중 콘솔 조향 +90° | 앱 재개 전 로그 조회에서 release 이후 조향 콜백은 관측되지 않음 |
| 진단 앱 재실행 | 같은 PID 유지. capability 재확인·재구독, 초기 조회 -90.0 및 변경 알림 수신. 화면 -90.00° |
| 복귀 후 콘솔 -45° | SDK 콜백 +45.0, 화면 +45.00°, 알림 횟수 증가 |
| 콘솔0 복원 | SDK -0.0 콜백 확인. 기어 P 유지 |
| 주행 조건 재확인 | 복귀 시 UNKNOWN → UX 수신(기어 미확인) → P 확인 후 ELIGIBLE |

원본 근거: [앱 프로세스 로그](docs/research/phase0-reentry-log.txt), [복원 후 화면](docs/research/phase0-reentry-restored.png). 로그의 AVD 날짜 10-27은 실제 실험일과 다르다. 주요 구간은 AVD 시계 21:35:04 초기 진입, 21:36:01 구독 해제·release, 21:36:11 재진입 및 -90 수신, 21:36:16 +45 수신, 21:36:36 0 복원이다.

### 판정과 한계

동일 프로세스에서 화면 전환으로 정지한 뒤 재진입하는 1회 시험은 통과했다. 이전 조향값을 계속 표시하지 않고 최신 값을 읽으며 이후 변경 알림도 받았다. 프로세스 종료·재생성, 서비스 장애, 장시간 반복·자원 누수, 정지 중 브레이크 변경, 지연 콜백·무응답은 이 시험의 범위가 아니다. Phase 0 전체 완료를 뜻하지 않는다.

## P0-004 D 상태에서 새 프로세스로 앱 시작

실험일: 2026-10-28 KST. P0-003과 같은 설치 앱·에뮬레이터에서 수행했다. 코드 수정·재빌드·데이터 삭제는 하지 않았다. 내비게이션으로 이동하고 진단 앱만 `am force-stop --user current`으로 종료했다. PID가 없어졌음을 확인한 뒤 Virtual Console에서 D로 전환하고 진단 앱을 명시적으로 실행했다. 새 PID는 6570(이전 3538)이다. 가속은 입력하지 않았으며 콘솔과 차량 화면 속도는 0 km/h였다.

| 단계 | 기어 | DO | UX flags | 판정·화면 |
| --- | --- | --- | --- | --- |
| 새 프로세스 초기 상태 | 미확인 | null | null | UNKNOWN |
| UX 초기 수신 | 미확인 | true | 80 | BLOCKED |
| 기어 초기 수신 | D | true | 80 | BLOCKED. 플랫폼 화면: 주행 중에는 사용할 수 없습니다. |
| P 복원, UX 해제 전 | P | true | 80 | BLOCKED |
| UX 해제 후 | P | false | 0 | ELIGIBLE. 정상 진단 화면 복귀 |

근거: [새 프로세스 로그](docs/research/phase0-d-start-log.txt), [D 시작 제한 화면](docs/research/phase0-d-start.png), [P 복원 화면](docs/research/phase0-d-start-restored.png). AVD 시계 기준 초기 진입은 21:44:04, P 복원은 21:44:19이며 로그 날짜는 실제 KST 실험일과 다르다. D 상태의 기록에서 ELIGIBLE 전이는 관측되지 않았다.

판정: 에뮬레이터에서 D 상태로 새로 시작하는 1회 시험은 통과했다. 앱 내부 초기 판정과 플랫폼 차단 화면을 함께 확인했으며 마지막 기어는 P로 복원했다. 조향·브레이크·차량 설정은 변경하지 않았다. 플랫폼은 앱 프로세스를 실행한 뒤 제한 화면을 표시하므로, 이를 앱 코드 자체가 실행되지 않는다는 근거로 사용하지 않는다. 실제 게임·사운드는 없으며 플레이 중단·사용자 명시 재개 정책은 별도 구현·검증 대상이다. 서비스 장애·권한 오류·미지원 환경은 여전히 미검증이다.

## P0-005 SDK 서비스 연결 장애와 수동 복구 — 수정 필요

실험일: 2026-10-28 KST. P0-004와 같은 설치 앱(원래 PID6570)·Pleos AVD/API34. 실제 SDK 전용 서비스 패키지 `ai.pleos.playground.service.vehicle`의 versionName은 **2.3.22**였다. 시스템 AAOS CarService는 종료하지 않았다.

1. P·조향0에서 SDK 전용 서비스만 사용자10에 대해 force-stop했다. 서비스 PID가 사라지고 앱 로그에 `service is reconnect isAbnormal reason`이 기록됐다(AVD 시계 21:48:16.974).
2. 서비스가 없는 동안 콘솔 조향을 +90도로 바꿨다. 앱 화면은 조향 -0.00°·브레이크 false·기존 READ 상태를 그대로 표시했으며 조향 알림 횟수는 82회에 머물렀다. 입력 연결 단절 표시가 없었다. AAOS 연결은 별개이므로 P·UX 사전조건 표시는 유지됐다.
3. shell에서 직접 서비스를 시작하는 시도는 VEHICLE_BINDING 권한 오류로 거부됐다. 권한을 추가하거나 우회하지 않았다. 내비게이션 전환·즉시 복귀 시도에서도 복구를 확인하지 못해 앱을 새 프로세스로 다시 시작했다.
4. 새 앱 PID31814가 정상 SDK bind 경로로 서비스(PID31942)를 다시 만들었다. ServiceConnected, 최신 조향 -90.0 및 변경 알림 수신을 확인했다(21:49:13~14). 조향을0으로 복원했다.

**판정:** 연결 장애 시 입력 무효화·단절 표시가 없어 수정 필요. 수동 앱 재시작을 통한 복구는 확인했으나 자동 재연결 성공으로 판정하지 않는다. 현 진단 화면의 ELIGIBLE은 AAOS 사전조건만 나타내며 입력 유효성까지 보장하지 않는다. 게임 구현 시 입력 유효성도 실행 조건에 포함해야 한다.

근거: [단절 로그](docs/research/phase0-sdk-disconnect-log.txt), [단절 중 오래된 값 화면](docs/research/phase0-sdk-disconnected.png), [앱 재시작 복구 로그](docs/research/phase0-sdk-recovery-log.txt). cached SDK 소스의 BaseServiceConnector는 연결 단절 시 binder를 비우지만 현재 VehicleInputProbe는 공개 연결 상태·입력 신선도를 별도로 추적하지 않는다. 따라서 SDK 내부 로그만으로 앱 표시가 무효화되지 않는다. 마지막 값이 변하지 않는 경우와 연결이 끊긴 경우를 구분할 정책은 아직 미구현이다.

## P0-006 조향·브레이크·기어 권한 없는 설치 — 통과(명시된 범위)

일반 스위치로 취소할 수 있는 런타임 권한이 아니므로 현재 소스를 빌드 전용 사본에 복사해 CAR_DRIVE/CAR_POWERTRAIN을 manifest에서 제거했다. 앱 ID는 `com.example.beyondphysics.nopermissions`, 화면 이름은 BP Permission Test, namespace·동작 코드는 원본과 같다. 원본 앱 소스·설치는 변경하지 않았다.

재현 스크립트: [Build-NoPermissionFixture.ps1](tools/phase0/Build-NoPermissionFixture.ps1). 빌드 위치 `android/build/phase0-no-permissions`, 실행 명령 `:app:assembleDebug --offline --console=plain --no-configuration-cache`. 빌드 성공 후 별도 앱으로 설치했다. 시험 APK SHA-256: `4DAA23AF3A1F20A08DBD1042BE10A2415941E64B193DB622F490DB703A61BBE8`. PID23077.

| 확인 항목 | 관측 |
| --- | --- |
| 설치 권한 | CAR_DRIVE/CAR_POWERTRAIN 요청·허용 없음. SDK VEHICLE_BINDING은 유지 |
| 조향·브레이크 | capability 조회의 need Permission 오류를 화면에 표시. 입력 수신0회, 정상값으로 대체하지 않음 |
| 기어 | 알림 등록 실패, 기어 미확인 |
| UX | DO=false/flags=0 수신 |
| 최종 판정 | UNKNOWN, 상태 미확인·게임 실행 불가 |

근거: [권한 덤프](docs/research/phase0-no-permissions-package.txt), [오류 로그](docs/research/phase0-no-permissions-log.txt), [화면](docs/research/phase0-no-permissions.png). 모의 오류가 아니라 실제 Android manifest 권한 누락으로 발생한 오류다. 두 권한을 각각 독립적으로 제거한 조합·실행 도중 권한 변경은 시험하지 않았다. 시험용 앱은 종료·제거했고 APK·사본·스크립트는 보관했다. 원본 앱으로 돌아왔다.

## P0-007 재진입 반복 및 순수 판정 회귀 검사 — 통과(명시된 범위)

- [Test-Reentry.ps1](tools/phase0/Test-Reentry.ps1)로 같은 PID31814에서 내비게이션 전환→STOPPED 확인→SDK release 확인→앱 복귀→새 조향 get 응답을 **5회** 반복했고 5회 모두 통과했다. [기계 판정 JSON](docs/research/phase0-repeat-reentry-results.json), [원본 로그](docs/research/phase0-repeat-reentry-log.txt). 단순 시작 명령 성공을 실제 정지·재연결 성공으로 대체하지 않았다.
- 현재 소스에서 `:app:testDebugUnitTest --rerun-tasks --offline --console=plain` 실행 성공. [실행 로그](docs/research/phase0-batch-unit-test-build.txt), [정책 테스트7개 XML](docs/research/phase0-batch-safety-tests.xml), [기존 예제1개 XML](docs/research/phase0-batch-example-test.xml). 총8개 실패0·오류0. 신호 null 시험은 순수 정책 검증이며 SDK/AAOS가 null을 실제로 전달하는 통합 시험이 아니다.
- 최종 확인: 서비스 PID31942 실행 중, 원본 앱만 설치, 기어 P·조향0·브레이크 false·DO=false/flags=0. [복원 화면](docs/research/phase0-batch-restored.png). APK 구분은 P0-003/P0-006 기록을 따른다. 원본 런타임 시험 APK와 현재 소스의 바이너리 일치를 주장하지 않는다.

### 이번 묶음 시험에서 완료하지 못한 항목

설치된 AVD는 Pleos_Connect_v2.1.0 한 개다. android.car 미탑재·API28 환경, 조향/브레이크 capability NONE, SDK 실제 null 전달, AAOS 서비스 단절·재연결, 지연 콜백·영구 무응답, Binder read 중단, 장시간 누수는 미검증이다. 권한 오류를 미지원 오류로 취급하거나 순수 정책 테스트를 통합 시험으로 대체하지 않는다. 이런 환경은 별도 테스트 장치 또는 의존성 주입 시험 구성이 필요하다. 조향각 크기·조향비 의미 및 물리 선행 연구도 남아 있으므로 Phase 0 완료가 아니다.

## U1-001 첫 Unreal 프로젝트 생성·C++ 빌드·PC 실행

실험일: 2026-10-28 KST. 사용자는 추가 예외 시험보다 구현을 우선하도록 지시했고 Unreal 프로젝트를 시작하도록 요청했다. 이 시험은 첫 3D 시제품의 생성·빌드·실행 확인이며 차량 물리 검증이 아니다.

### 환경

- Unreal Engine5.8.4, 설치 경로 `C:\Program Files\Epic Games\UE_5.8`.
- Windows PC. Visual Studio Community2022를 17.6.5에서 17.14.43(설치 버전17.14.37824.6)으로 업데이트.
- 실제 빌드 도구: MSVC14.44.35229, Windows SDK10.0.26100.0, .NET Framework4.8 SDK/TargetingPack.
- 프로젝트 `unreal/BeyondPhysics/BeyondPhysics.uproject`, 레벨 `/Game/Maps/PhysicsLab`.

### 구현과 실행 결과

1. 콘텐츠 장면을 먼저 생성했다. 평면 도로·차선·Cube/Cylinder 임시 차량·카메라·조명 및 단색 재질을 저장했다. 생성 보고는 액터67개다. 초기 생성 명령 종료 코드0, 저장 로그 확인: [생성 로그](docs/research/unreal-first-scene.log).
2. 초기 콘텐츠 프로젝트 상태에서 Play 실행을 화면과 PIE 로그로 확인했다. 차량 도형은 정지해 있으며 물리 시뮬레이션은 꺼져 있다.
3. C++ 기본 모듈과 Game/Editor 빌드 타깃을 추가했다. 첫 빌드는 NetFxSDK 누락으로 실패했다. 필요한 공식 구성 요소만 추가 후 `Build.bat BeyondPhysicsEditor Win64 Development -Project=... -WaitMutex -NoHotReloadFromIDE` 재실행이 **Succeeded, 종료 코드0**을 반환했다. [최종 빌드](docs/research/unreal-first-cpp-build.log), [첫 실패 로그](docs/research/unreal-first-cpp-build-attempt1.log).
4. 프로젝트 파일 생성도 종료 코드0. `BeyondPhysics.sln`과 `.slnx` 생성: [생성 로그](docs/research/unreal-project-files.log).
5. 시각 확인 중 바퀴 회전이 잘못된 것을 발견했다. Python Rotator 위치 인자가 C++ FRotator와 달라 yaw90으로 저장돼 있었다. 키워드 인자로 roll90/yaw0으로 수정해 네 바퀴를 세웠다. [수정 전후](docs/research/unreal-scene-finalization.json), [저장 로그](docs/research/unreal-scene-finalize.log). 도형 수정이며 물리 모델 보정이 아니다.
6. 빌드된 C++ 모듈을 로드하고 `/Game/Maps/PhysicsLab -game`으로 실제 PC 게임 화면을 실행했다. 로그에서 `UnrealEditor-BeyondPhysics.dll` 로드, PhysicsLab 월드 Play 시작, 1280×720 스크린샷 저장을 확인했고 이미지를 직접 검토했다. [실행 로그](docs/research/unreal-first-game.log), [첫 장면](docs/research/unreal-first-scene.png).

### 한계와 다음 작업

현재 C++ 모듈은 등록 코드만 있다. 차량 위치·속도 적분, 타이어 힘, 조향·제동, 서스펜션, 사운드, Pleos 입력은 아직 없다. 화면의 차량 크기는 시각적 임시값이며 실차 파라미터가 아니다. Unreal 거리는 cm이고 이후 물리 계산(m, kg, s)의 변환 지점을 명시해야 한다. Android APK·x86_64·Pleos 호환성·자원 사용량은 이 PC 실행으로 검증되지 않았다. 생성·빌드 로그의 장면 크기나 프레임 결과를 물리 정확성 근거로 사용하지 않는다.

## U1-002 첫 평면 차량 모델
무마찰 월드 운동, 일반/빙판 제동 분석해, 비포화 선회, 접지력 한도, 저속·좌우 대칭·반복성과 비유한 입력 거부 검사를 수행했다. 결과 failures=0. 제동거리 일반13.4918908921m/빙판95.5657379725m (초기15m/s). docs/research/low-friction-model-tests.log와 PHYSICS_MODELS.md 참조. Unreal Editor Win64 Development 빌드 Succeeded. 실제 게임 주행 검증은 이어서 기록한다.

U1-002 실행 확인: IceTrackDrive 생성(309개 액터), 독립 게임에서 상세 차량/HUD와 54km/h 자동 제동 시작 및 -0.12g 표시를 실행 화면으로 확인했다. docs/research/ice-track-first-drive.png 및 ice-track-game-verified.log. 자동 제동의 종료/CSV 저장까지는 이번 실행에서 확인하지 못했으므로 전체 통합 시험 통과로 기록하지 않는다. 그래픽은 첫 구성으로, 배경·반사·구도를 추가 개선할 여지가 있다.

## U1-003 사용자 교체 차량 모델 재빌드
교체본은 Evaluate 중간에서 잘려 C1075가 발생했다. 원본을 backups/VehicleModel-user-20261028-before-build.cpp로 보존했다. 작성된 Fiala 형태 횡력/방사형 마찰원 투영은 유지하고 누락 반환/괄호 및 0.5ms 이하 내부 세분화 RK4 Step을 복구했다. 기존 순수 모델 검사 failures=0. Unreal Editor Win64 Development 빌드 Succeeded. 로그 docs/research/user-model-rebuild-check.log, user-model-unreal-build.log. 검증 범위는 기존 검사이며 실측 타이어 모델 검증으로 해석하지 않는다.

## U1-004 방향·접지점·정지 출발 AWD·1인칭
기존 타이어 힘 계산에서 초기 월드 방향의 회전 불변성 및 전륜과 실제 이동 방향 일치 시 슬립각0이 통과했다. 바퀴 시각화의 동일 회전 속도 문제를 접지점 속도 투영으로 수정했다. 테스트 RED에서 바퀴 속도 2개 및 AWD 가속 검사가 실패하고 수정 후 전체 failures=0. 가상 AWD 직선 가속1.1772m/s²(기존후륜0.5232m/s²), μ0.12 및 정지출발 유지. LiveCoding 성공 및 CaptureEditorImage로 1인칭·속도0 표시를 확인했다. 로그 docs/research/steering-contact-tests.log. 차체/이동/바퀴 방향 표시와 모델 한계를 PHYSICS_MODELS.md에 반영했다.

## V1-001 도로 대비 개선 (2026-10-29)
Unreal MCP 배치 편집으로 넓은 빙판 표면을 폭10m 도로로 축소 표현하고, 차선폭3.5m 두 개와 흰색 차선/가장자리/노견선을 생성했다. 기존 표식298개는 삭제하지 않고 숨겨 보존했다. 노면 IceRoughness0.62, Specular0.22, IceTint(0.14,0.18,0.22), 노출보정-0.45, Bloom0.06. 도로와 주변 지면 대비를 1280x720 실제 1인칭 독립 실행 화면으로 확인했다. docs/research/road-contrast-20261029.png 및 road-contrast-game.log 참조. 새 실행의 화면 확인용 AA를FXAA로 사용했으며 최종 기본AA 설정 자체는 변경하지 않았다. 물리 소스 SHA256 32CEEB6627C019976730DC37256D1130A0FEA1661BF50000BDB26F776C9E46E2가 변경 전후 동일하다. 마찰계수/구동 배분/수치적분은 변경하지 않았다. 원본 레벨·재질은 backups/road-contrast-20261029에 보존했다.

## 2026-10-29 Unreal Android/Pleos 실제 실행

Epic Android 플랫폼과 SDK36/build-tools36.0.0/NDK27.2.12479018/CMake3.22.1을 설치하고 JDK21을 별도 사용한다. 최종 x86_64 APK 패키징 ExitCode0, adb install Success, 정상 SplashActivity→GameActivity 실행을 확인했다. Pleos API34 안에서 Android_Emulator/Vulkan 프로필 자동 선택, IceTrackDrive 로드, 화면의 도로/HUD/자동 가속과 실제 BP_STATE(time23.979167s, x67.667883204m, speed42.342412500m/s, paused0/crashed0)를 확인했다. 시작 x=-440m, speed0에서 진행한 관측이다.

이 결과는 Unreal 게임이 Pleos 에뮬레이터 안에서 실행된다는 검증이며 차량 SDK/JNI 연결이나 실차 동작 인증이 아니다. 모바일 하늘·조명과 숨겨둔 구형 산 재질 경고는 후속 정리가 필요하다. 물리 모델 원본은 변경하지 않았다. 기록: docs/research/2026-10-29-android-installation.ko.md, 최종 로그 unreal-android-final-logcat-20261029.log, 빌드 unreal-android-drive-build-latest.log. APK SHA256 B51F356E8F3575431524320BABFB94B722E58C9F5C8B0C222B12636B1252FF7A.

## 2026-10-29 Unreal APK의 Pleos 조향·브레이크 직접 연결

Vehicle SDK2.3.2/AAOS를 Kotlin AAR→JNI→Unreal C++에 연결해 실제 Pleos API34/x86_64 APK에서 확인했다. 콘솔-90→SDK+90→steer+0.166667/좌회전, 콘솔+90→SDK-90→steer-0.166667/우회전. 직진에서 브레이크 눌림은 brake1/throttle0으로 전달되고 속도4.11284m/s에서0으로 감속했다. P→D 시 시간·위치가 고정됐고 P 복귀 후에도 재시작 잠금이 유지됐다. 실제 화면 RESTART 클릭 후 정지 출발했고 다른 Activity 방문 후 복귀도 잠겼다.

Vehicle 서비스 force-stop 후 VEHICLE INPUT STALE/allowed0을 관측했다.5초 요청 deadline으로 SDK 세션을 교체한 뒤 READY가 돌아와도 물리 시간·위치와 재시작 잠금은 유지됐고, 명시적 재시작 후 새 브레이크 입력을 받았다. 대기 없이 D→P 연속 요청도 잠금 유지. Kotlin8개/실패0, C++ 모델·입력 검사 failures0, 최종 Unreal 패키징 ExitCode0, 설치/실행 성공.

첫 선회 뒤 제동 사례는 벽 접촉으로 중단되어 제동 근거에서 제외하고 직진 제동으로 다시 확인했다. 에뮬레이터 프로파일 보정이며 실차 조향비·압력 비례 브레이크 검증이 아니다. 원격 오류의 모든 타이밍·AAOS 서비스 단절·장기 누수는 미검증이다. 상세: docs/research/2026-10-29-native-vehicle-input.ko.md, 원문 native-input-final-cases.jsonl, native-input-final-logcat.log, 최종 상태 native-input-final-evidence.json. APK SHA256 64DAC10DBBAE013C13B6475C557E872A0DF152971F53521FB3073321E35EF87B. 최종 화면 native-input-final-emulator.png. 원본 VehicleModel.cpp 해시는 변경하지 않았다.

## 2026-10-29 저마찰0.12 복원 APK
사용자 요청으로 LowGripFriction0.18→0.12. 기존 모델 검사의 가속·10초 속도 기대값을1.1772m/s²/42.3792km/h로 맞추고 기존 검사1회 failures0. 새 검사는 추가하지 않았다. Unreal 전체 패키징 BUILD SUCCESSFUL/ExitCode0(1m54s), 설치 Success, GameActivity 실행 성공. 실제 BP_STATE mu=0.120을 확인했다. 로그: docs/research/friction-012-package-console.log, friction-012-install.log, friction-012-runtime.log, friction-012-model.log. APK SHA256 2BAE8CFC270C855B087E01E781AA9829F74CD191119EDA45A79231FEFEC8C365. 일반 노면0.85와 힘 계산/SDK 연결/자동 가속은 유지했다. 실제 얼음 실측 보정으로 해석하지 않는다.
Android Studio 후속 확인: 기존 프로젝트 Import new APK는 FileTooBigException으로 실패했다. BeyondPhysics-x641 새 실행 프로젝트로 가져오고 누락된 모듈 SDK를 Android API36으로 지정했다. Studio Run의 Commit Install/Activity 활성화 로그와 새 PID9199를 확인했고 BP_STATE mu=0.120, allowed1/READY, paused0으로 실행됐다. Studio 프로젝트의 APK 해시는 최종 원본 APK와 동일하다. 증거: docs/research/friction-012-studio-deploy.log, friction-012-studio-runtime.log, friction-012-evidence.json, friction-012-emulator.png.

## 2026-10-30 두 맵 APK·가상 램프
허용된 가상 설계: R150m 우회전90°, 완화곡선 각60m, 총635.619m, 3차로·포장폭13.5m, 최대 편경사4%·종단-3%, μ0.12. C++ 형상 노드와 Unreal 메시를 공유한다. 평면 회귀·램프 계산 검사는 failures0. 연속 C++ 입력으로 전구간630.63m/77.4417s, 최대 이탈0.0821557m를 기록했다.

최종 APK198,499,824바이트, SHA256 E7960DD1052377E2894D4C54DEB5ED865B0612AECCF9CACC7085F135F9724264. 전체 패키징 BUILD SUCCESSFUL/ExitCode0, 실제 Pleos x86_64 설치·실행 성공. SDK 곡선 진입과 brake1/throttle0 감속9.094074437→4.376857016m/s를 확인했다. 이후 station250.176979m에서 횡미끄러짐·벽 접촉으로 게임이 정지했다. 빠른 터치 누락을 이벤트 래치로 수정하고 최종 설치본에서 MAP02/RESTART/MAP01/MAP02 화면 클릭을 확인했다. 주행 중 D 차단 후 두 관측의 time22.683333s/x12.331023219m가 동일했고 P 복귀 재시작 잠금도 유지됐다.

전구간 SDK 자동 시험 운전자는 지연에 불안정해 벽에 닿았으므로 완주 성공으로 보고하지 않는다. 경사면 정지 유지와 실측 빙판/ABS 정확성도 검증하지 못했다. 자세한 실패·한계·원본 복구·MCP 연결은 docs/research/2026-10-30-virtual-ramp-results.ko.md, 최종 정량 증거는 virtual-ramp-final-evidence.json에 기록했다. 원본 직선 맵 해시는 백업과 동일하다.

## 2026-11-03 두 번째 맵의 완만한 자동 구동과 시도 비교
정책/측정 RED8건, 실험 입력 혼입 RED1건, μ 프로파일 보존 RED1건을 관측한 뒤 수정했고 최종 portable 검사0실패. 기존 평면·램프 검사도0실패. 같은 C++ 조향 시험 운전자에서 무감속 진입46.4653km/h는 station394.455m 벽 접촉, 사전 감속21.5964km/h는630.667m까지 통과(최대 이탈0.0881m). 위치/요각 강제 보정 없이 동일 타이어 힘을 적분했다.

최종 APK198,501,164바이트/SHA256 041F50CE021A994D05BA22C00C7D3268C071385C4108787CF0620FAD1283D5AC. Win64/Android 빌드·패키징/설치 성공. 실제 Pleos SDK 시험에서 map01 throttle1 유지, map02 throttle<=.12, 진입46.4652792→21.0269196km/h 및 이전 entry/outcome/trace 보존을 확인했다. 중립 조향 SDK smoke이며 전구간 SDK 완주 주장이 아니다. 결과 화면과 saved1 CSV 반환을 확인했으나 사용자10 외부 저장소 권한으로 CSV 원문은 추출하지 않았다.

보존 대상4파일 해시 동일. 자료: docs/research/2026-11-03-ramp-learning-results.ko.md, ramp-learning-apk-result.json, ramp-learning-preserved-files.json, ramp-learning-result.png/ramp-learning-comparison.png. 정지마찰·ABS·실차 검증의 기존 한계는 남아 있다.

## 안개 맵 검증 진행 기록 — 2026-11-04

휴대 가능한 실제 차량 코어의 FogChecks/시나리오 및 기존 차량·램프·학습 회귀검사는 failures=0이다. 분석식49.1593m에 대해100km/h·μ0.8의 제동 적분49.1594m를 확인했다. 기존 두 맵 자산·VehicleModel.cpp·두 기존 가속 정책 파일5개 해시는 동일했다. 검토에서 발견한 코너 접촉 누락·평행 근접통과 오판정·측방 정지·사전 제동 이력·결과 시각 누락을 수정하고 회귀에 포함했다.

첫 실제 APK의 자연 주행은75.174810s에100km/h로 앞차 접촉, 범퍼간격0으로 종료했다. 동일 APK 실제 Vehicle SDK 브레이크 시험은 기준 속도100km/h, 선언된100m 기준 후1.558524s/43.292326m에 제동, 이후49.159367m, 잔여7.548307m·속도0.032543m/s로 정지했다. 사람이 플레이한 반응 시간으로 해석하지 않는다. trace/summary 저장 성공을 로그로 확인했으며 Android 앱 전용 저장소의 CSV 파일 내용 자체를 추출해 검증한 것은 아니다.

실제 APK 화면에서 하얀 기본 격자 재질을 발견했고, 로그에서 기존 MI들의 InstancedStaticMeshes 사용 플래그 누락을 확인했다. 새 M_FogSurface와 MI_Fog* 복사본에 명시적 사용 플래그를 저장해 원본 공용 재질을 수정하지 않고 해결했다. 이 문단은 초기 코드/SDK 시험 이력이며 최종 화면·APK 확인 결과는 별도 fog-highway-results 기록을 참조한다.

최종 안개 APK(2026-11-04, SHA256 21100001E4B2DE81D9C908D972F4BBC92D10510E9BBEE33CC532DFF5EFD2804D)에서 실제 SDK 제동을 재확인했다. 기준 속도100km/h, 자동 명령 반응0.171024s/4.750660m, 제동 이후49.159367m, 최종 간격46.089973m·속도0.032543m/s/Stopped1. D 중단/P 복귀의 재시작 잠금도 확인했다. 정확한 이벤트·저장 성공·화면·해시와 이전 시험/실패를 docs/research/2026-11-04-fog-highway-results.ko.md에 구분해 보존했다. 최종 실제 노출 설정은 extended=0의 일반 휘도1024이며 EV13을 그대로 넣었던 단위 오류를 수정했다.

## 2026-11-08 — Shipping APK
실제 Release에서 Gson TypeToken 메타데이터 소실을 재현하고 R8 Signature/TypeToken 계층 보존 규칙으로 수정했다. 전용 서명·비디버그·정렬·CRC와 세 맵 실제 터치/SDK 입력·효과·P 제한/복원을 확인했다. 세 맵 및 물리 원본 변경 없음. 상세 docs/RELEASE_VERIFICATION.ko.md.

