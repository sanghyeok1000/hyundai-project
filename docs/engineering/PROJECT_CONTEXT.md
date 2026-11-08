# 2026-11-08 제출용 Release 현황

Shipping + Distribution / Release AAR로 전용 RSA3072 키 서명 APK를 만들었다. package=com.beyondphysics.simulator, versionName=0.1.0, versionCode=1, x86_64 에뮬레이터 전용. 121751742bytes, SHA256 2ba0f6ca2c93408d07b6d83c72e65c3c213899d5f663260b1264cbf07ee47c00. 서명/정렬/CRC/디버깅 비활성 및 실제 Pleos 세 맵·입력·효과·복원·D 제한을 확인했다. Release에서 발견한 Gson TypeToken/R8 오류를 consumer 규칙으로 수정했다. 맵과 물리 모델은 바꾸지 않았다.

APK: submission/release/BEYOND_PHYSICS_release.apk. 공개 소스: submission/github-ready/. 빌드: unreal/BeyondPhysics/Scripts/package_submission_release.ps1. 개인 키: private/signing/ (절대 공개 금지). 공개 소스는 선택한 파일의 별도 스냅샷이며 원본 변경이 자동 동기화되지 않는다. GitHub 저장소 생성·커밋·업로드는 사용자가 직접 수행한다. 안내 docs/SUBMISSION_GITHUB.ko.md, 실행/빌드 docs/RELEASE_BUILD.ko.md, 검증 docs/RELEASE_VERIFICATION.ko.md. 기획서·서명/동의서·시연영상·공식 접수는 별도다.

아래는 날짜별 기존 개발 이력이다. 현재 실행 정보는 위 Release 안내를 우선한다.
# BEYOND PHYSICS 프로젝트 현황

갱신: 2026-11-05 KST. 현재 작업 기준은 PROJECT_GUIDELINES.md(v1.5)이다.

현재 방향: 사용자 요청으로 새 맵 확대를 멈추고 세 맵의 사용 경험을 완성한 뒤 접수양식(기획서)·시연 영상·GitHub 소스/APK·공모전 제출을 준비한다. 첫 우선 작업인 맵별 조명·공조·사운드와 개별 선택·원상복원을 구현했다. 최종 APK에서 세 맵 모두 실제 SDK 적용/getter·콘솔 값 비교, OFF·맵 이동·접촉·P 해제 복원을 확인했다. 수동 설정 우선권·AC OFF 팬 전용·Settings 배경 전환/재시작 잠금도 확인했다.22개Kotlin검사/10개효과정책 검사, 최종Editor/Android빌드와 실제터치 설정→토글→적용/재시작도 통과했다. 실제게임 마스터 오디오3개WAV를 저장했다. 세 맵 자산과 물리코어7개파일 해시는 작업 시작과 같다. 최종APK SHA256 b7ff5a73d4129c9181271bc7f59719e1e1d1c7789b4ecb624361a5ae7314533e,198819176bytes. 앱은P·설정창열림·조명/공조OFF·소리ON으로 준비됐고 차량 원래값을 복원했다. Unreal Editor는 메모리 여유를 위해 닫힌 상태이며 Windows용 모듈도 최신 빌드했다. 안내 docs/MAP_FEEDBACK.ko.md, 결과 docs/research/2026-11-05-map-feedback-results.ko.md, 증거 map-feedback-final-evidence.json. 과거 Unreal README의 ‘물리·APK 미구현’ 문구는 초기 이력이며 현재 상태와 다르다.

최신 안개 피드백 반영: 사용자 선택에 따라 특정 차종 대신 근거·가정을 기록한 일반 승용차 모델을 적용했다. 맵03은80km/h로 시작해100km/h로 자동 가속한다. 시작x=-260m, 안개 진입까지 약30m, 첫 차량 범퍼까지255.04m이며 중앙/좌/우 차로에 정차 차량을45m·40m 간격으로 배치했다. 반경450m·밀도0.27의 짧고 짙은 안개와 선언 기준30m를 쓴다. 실제 발견거리의 광학 보정은 아니다. 맵03만18:1 핸들/랙, Ackermann 좌우각, 네 접지점별 현재 속도·힘·요모멘트 모델을 사용하고 기존 얼음 맵은 기존 모델을 유지한다. APK 재빌드/설치와 실제 SDK 조향,80→100,미제동 접촉,근접 급조향 접촉,예방 제동49.156398m·잔여간격52.620302m를 확인했다. D→P 후 재시작 잠금도 유지한다. 상세 모델/수정 파일은 docs/FOG_FEEDBACK_PHYSICS.ko.md, 결과는 docs/research/2026-11-04-fog-feedback-results.ko.md, 증거는 fog-feedback-final-evidence.json이다. 아래 첫 안개 맵 설명은 수정 전 이력이다.

최신 변경(2026-11-04): 세 번째 FogHighwayDrive를 같은 APK에 추가했다. 건조 아스팔트 대표 μ0.8,4km 직선3차로, 정지 출발·게임 목표100km/h, 공간 안개와 처음부터 정차한 앞차다. 움직인 뒤 제동하면 자동 구동을 취소하며 RESTART로 새 시도를 시작한다. 기존 두 얼음 맵·공유 VehicleModel.cpp·기존 두 입력 정책 파일의 해시는 동일하다. 실제 최종 Pleos APK에서100km/h→SDK 제동→49.159367m 후 정지/앞차 간격46.089973m를 확인했다. 자동 입력 시험이며 사람의 반응 시간은 아니다. 미제동/늦은 제동 접촉, D 중단/P 복귀 재시작 잠금, CSV+summary 저장 성공도 확인했다. 기본 노출 단위는 EV가 아닌 일반 휘도(extended=0)로1024를 사용하고, 전용 인스턴싱 재질을 만들어 기존 공용 재질은 수정하지 않았다. 맵 순환은01 직선→02 곡선→03 안개→01이다. 사용·수정: docs/FOG_HIGHWAY.ko.md, 증거: docs/research/2026-11-04-fog-highway-results.ko.md 및 fog-final-evidence.json. 아래 두 맵 관련 문단은 과거 이력이다.
최신 변경: 두 번째 곡선 맵에만 RampAutoDrive(최대 throttle0.12·제동 후 가속 목표 유지)를 적용하고, 진입 속도/현재·직전 궤적/μ/종료 결과를 표시·저장한다. 첫 번째 맵 입력 함수·공유 물리 모델·두 도로 자산은 해시가 같다. 같은 타이어 힘 계산에서 감속 없는46.5km/h 진입은 벽 접촉, 미리 감속한21.6km/h 진입은 전구간 통과했다. 실제 SDK APK에서는 첫 맵 throttle1 유지와 두 번째 맵46.5→21.0km/h 진입 비교·이전 궤적 보존을 확인했다. SDK 전구간 완주는 주장하지 않는다. 사용법 docs/RAMP_LEARNING.ko.md, 결과 docs/research/2026-11-03-ramp-learning-results.ko.md. 아래 전체가속·이전 SDK 실패 설명은 과거 이력이다.

최신 맵 구성: 기존 IceTrackDrive와 두 번째 IceRampDrive를 같은 APK에 넣었다. 사용자가 허용한 공식 설계 기준 참고 가상 램프이며 특정 실도로 복제는 아니다. 우회전90°/R150m/완화곡선 각60m/총635.619m, 3차로·포장폭13.5m, 최대 편경사4%·내리막3%다. 기본 μ0.12, Pleos 조향·브레이크·자동 가속 정책을 공유한다. 상단 MAP 버튼 또는 PC M으로 전환한다. 형상·사용법·수정 경로·축소3자유도 한계는 docs/VIRTUAL_RAMP.ko.md를 참조한다. 원본 직선 맵은 백업과 SHA256이 같다. 아래 문단은 이전 단계 기록을 포함한다.

최종 두 맵 APK 설치·실행 확인: 화면 버튼 맵 왕복/재시작, 실제 SDK 곡선 진입·감속, 벽 접촉 종료, 주행 중 D 전환 정지/P 복귀 잠금을 확인했다. C++ 연속 입력 전구간 계산은 통과했지만 SDK 외부 자동 시험 운전자 완주는 실패했다. 경사면 정지 유지·ABS·실측 얼음 정밀도는 미완료다. 실제 결과와 실패를 docs/research/2026-10-30-virtual-ramp-results.ko.md 및 virtual-ramp-final-evidence.json에 기록했다. 현재 에뮬레이터는 곡선 시작점에서 RESTART를 기다린다.

최신 물리 설정: 기본 저마찰 μ=0.12로 복원했고 최종 APK 재빌드·설치 후 BP_STATE mu=0.120을 확인했다. 일반 노면0.85 및 Pleos 입력 연결은 유지한다. 기존0.18 기록은 과거 이력이다. 백업 backups/friction-012-20261029, 빌드/실행 docs/research/friction-012-package-console.log와 friction-012-runtime.log.

**최신 상태: Unreal Android APK의 Pleos 조향·브레이크 직접 연결 완료(에뮬레이터 검증).** Kotlin Vehicle SDK/AAOS → AAR/JNI → 기존 C++ 차량 모델로 연결했다. 자동 가속을 유지하고 브레이크 Boolean true는 제동1/구동0으로 매핑하며 실제 가속페달 API는 사용하지 않는다. P·UX·foreground·입력 신선도를 검사하고 중단 후에는 화면 RESTART로 정지 출발한다. 실제 APK에서 좌우 선회·직진 제동·P→D→P·화면 재시작·백그라운드 복귀·SDK 서비스 중단/복구 후 새 입력을 확인했다. Kotlin8개 및 C++ 모델/입력 검사 실패0. VehicleModel.cpp는 변경하지 않았다. 사용법: docs/ANDROID_RUN.ko.md, 상세 증거·한계: docs/research/2026-10-29-native-vehicle-input.ko.md. 실차 ARM64/제조사 승인·조향 보정·클러스터 출력·조명/공조는 미완료다. 아래 초기 단계의 'SDK/JNI 미연결' 서술은 과거 이력이며 이 문단이 최신 상태다.

Android 실행 기반(2026-10-29): Epic 런처 Android 플랫폼, SDK36/build-tools36.0.0/NDK r27c/CMake3.22.1 및 프로젝트 전용 JDK21을 갖췄다. Unreal x86_64 개발 APK를 빌드·설치했고 Pleos API34 에뮬레이터 내부에서 IceTrackDrive와 HUD·자동 가속을 실행 확인했다. 로그에서 Android_Emulator 프로필 자동 선택과 VulkanRHI will be used를 확인했다. 실제 모델명은 Unreal 안에서 `Connect on x86_64 emulator`로 읽히므로 전용 매칭 규칙을 추가했다. APK: builds/android-drive/BeyondPhysics-x64.apk. 모바일 하늘·조명은 PC 화면과 차이가 남아 있어 후속 조정 대상이다. 아래 미확인 서술은 이전 조사 이력이다.

Pleos 연동 착수 조사: 현재 AVD는 adb 실측 x86_64/native bridge=0, 설치 Unreal에는 Android 플랫폼/NDK 구성이 없다. 다만 'x86_64는 반드시 소스 빌드가 필요하다'는 이전 설명은 정정한다. Epic5.4 릴리스 노트는 런처 빌드의 x86_64 바이너리를 명시하고5.8 문서도 에뮬레이터 실행을 안내한다. 공모전 공개 규정에는 Unreal 명시 허용/금지를 찾지 못했고 운영사 승인 답변은 아직 없다. 최소 Unreal x86_64 APK를 Pleos 안에서 실제 실행하는 검증을 먼저 수행해야 한다. 조사: docs/research/2026-10-29-unreal-pleos-feasibility.ko.md. 공유 Android 모듈 pleosbridge에는 입력 유효성 정책과 검사를 준비했지만 SDK 런타임/JNI/PC 릴레이는 아직 구현되지 않았다. 의존성 확인 성공을 게임 연동 완료로 보고하지 않는다.

이번 변경: 시작/재시작 시 렌더링이0.75초 안정된 후 정지 상태에서 자동 가속하고 S로 제동한다. W와 실제 가속페달 입력은 제외했다.2km 3차로에 가로등68개를 추가하고,321×321 능선 지형·실사 CC0 암석 재질·경사/높이별 눈 분포로 배경을 개선했다. 원본 산은 숨겨 보존했다. 정식 Unreal 빌드와 자동 가속/제동 계산 검사는 통과했으며, 차량 APK 자원 예산·실차 사용 승인은 별도 후속 작업이다.

## 목표와 작업 환경

사용자가 제공한 개발 지침 v1.0을 따른다. 물리 정확성과 차량 인터페이스의 결합을 우선하며, 분석해 및 실험 검증 없이 다음 단계의 범위를 확대하지 않는다. 개발자가 코드와 모델을 설명할 수 있어야 한다.

주 작업 환경은 Windows 데스크탑이다. 저장소 루트는 `<WORKSPACE>`, Android 프로젝트는 `android/`이다. Android Studio로 만든 Kotlin·Jetpack Compose Empty Activity를 사용한다. 앱 ID는 개발용 `com.example.beyondphysics`, minSdk는 28이다. Vehicle SDK 2.3.2를 추가했다.

## 현재 단계

**Unreal의2km 3차로 저마찰 주행과 Pleos 에뮬레이터 입력 연결을 구현했다.** 자체 C++ 차량 운동, 정지 출발, 일반/저마찰 비교, HUD·기록·벽 접촉 종료가 있다. PC는 A/D·S, Android는 SDK 조향·브레이크로 조작한다. 자동 가속이며 W와 실제 가속페달은 사용하지 않는다. 가속·제동 힘과 고정 적분은 기존 물리 모델이 계산한다. SDK 서비스 단절 복구는 실제 시험했지만 장시간 누수·모든 원격 오류의 정리는 미검증이다. Phase 0 잔여 조사·게임 사운드·실차 사용 승인은 후속 작업이다. 아래 첫 장면 관련 설명은 초기 구현 이력이다.

사용자는 C++에 익숙하지만 Unreal 및 앱 설계는 처음이므로 도구 조작과 코드의 역할을 단계별로 설명한다. 첫 시제품은 평면 도로·임시 차량·카메라가 있는 작은 3D 장면이다. 물리 계산은 C++로 분리하고 Pleos SDK 연결은 Kotlin/Java와 JNI를 사용한다는 방향을 설명했다. Unreal APK의 Pleos 호환성은 아직 실제로 확인하지 않았다. 현재 AVD는 API34/x86_64이며 Epic Android 설정 문서는 x86_64 지원에 GitHub 소스가 필요하다고 명시한다.

2026-10-28 도구 갱신: Unreal 5.8.4 설치 경로는 `C:\Program Files\Epic Games\UE_5.8`이다. Visual Studio 2022 Community를 17.14.43(설치 버전17.14.37824.6)으로 업데이트했고 MSVC14.44.35229·Windows SDK10.0.26100.0·.NET Framework4.8 SDK를 확인했다. Android Studio는 3D 편집 중 열어둘 필요가 없지만 Android SDK/NDK·에뮬레이터 개발 환경은 유지한다.

## 첫 Unreal 프로젝트

- 프로젝트: `unreal/BeyondPhysics/BeyondPhysics.uproject` / 레벨: `/Game/Maps/PhysicsLab`.
- `Content`: 평면 도로·차선·도형으로 조립한 임시 차량·카메라·재질. 차량 도형의 물리 시뮬레이션은 꺼져 있다. 차량 크기는 시각적 임시값이며 실차 데이터가 아니다.
- `Source/BeyondPhysics`: C++ 기본 모듈 등록. 현재 차량 운동 코드는 없다. 고정 시간 적분·타이어·서스펜션은 아직 추가하지 않았다.
- `Scripts`: Unreal Editor Python으로 첫 장면을 생성하고 바퀴 방향을 수정한 편집 도구다. 게임의 물리 계산이나 실시간 실행을 Python에 의존하지 않는다.
- `.sln`: Visual Studio용 프로젝트 파일 생성 성공. `BeyondPhysicsEditor Win64 Development` C++ 빌드 성공. 로그는 `docs/research/unreal-first-cpp-build.log`.
- 콘텐츠 프로젝트 상태에서 첫 레벨의 Play 실행을 화면과 PIE 로그로 확인한 뒤 C++ 모듈을 추가·빌드했다. 이어 C++ 모듈을 로드한 독립 게임 모드(`UnrealEditor -game`)에서도 PhysicsLab이 실행되는 것을 확인했고 1280×720 화면을 저장했다. `docs/research/unreal-first-scene.png`, `unreal-first-game.log`를 참조한다.
- Unreal 거리는 cm이며 향후 SI 물리 계산(m, kg, s)과의 변환 지점을 명시한다. APK 빌드·Pleos 연결은 아직 하지 않았다. 사용법은 `unreal/BeyondPhysics/README.md`.

## 다음 구현에 대한 사용자 요구

사용자는 같은 프로젝트를 확장해 저마찰 운전을 만들고, 기본 도형 수준을 넘는 사실적이고 화려한 그래픽을 차량 사양에 맞춰 구현하길 요청했다. 중앙 인포테인먼트와 운전대 뒤 계기판을 모두 최종 화면 목표로 선택했다. 실제 클러스터 출력 API·권한·지원은 아직 미확인이다. PC 레이아웃 미리보기와 실제 차량 출력 지원을 구분한다.

사용자는 그래픽 작업 초보자로, 어느 파일을 수정하면 화면이 바뀌고 어디에 C++를 작성하는지 자세한 안내를 원한다. `docs/UNREAL_EDITING_GUIDE.ko.md`를 작성했다. 현재 파일과 향후 제안 파일을 구분하며, 재질 Roughness와 타이어 마찰계수는 다른 값임을 설명한다. 고급 그래픽과 저마찰 주행 코드는 아직 추가하지 않았다.

사용자는 후속 구현 방식으로 **별도 C++ 차량 모델 + Unreal 화면 표현**을 선택했다. 구현 전 설계안은 `docs/superpowers/specs/2026-10-28-low-friction-driving-design.md`에 작성했다. 이 방식의 선택과 세부 설계안 승인은 구분하며, 현재 게임 코드는 아직 확장하지 않았다. 설치된 Unreal의 `Templates/TemplateResources/Standard/Vehicles/Content/SportsCar`에 차체·유리·바퀴 메시와 재질이 있어 외형 재사용 후보로 확인했다. 개발 PC는 i5-12400F·RTX3060Ti(약8GB VRAM)·RAM32GB이며 실제 차량 사양과 다르다. 필요한 라이브러리·드라이버 설치는 사용자가 허용했지만 현재 드라이버 교체 필요성을 확인한 것은 아니다.

실행 중인 Pleos Emulator는 2.1.0/API 34, ADB 기기는 `emulator-5554`이다. Virtual Console은 ADB의 8088 포워딩 후 `http://127.0.0.1:8088/`에서 연다. 에뮬레이터나 ADB를 재시작하면 포워딩이 사라질 수 있다.

## 현재 코드의 역할

- `android/app/src/main/java/com/example/beyondphysics/MainActivity.kt`: Compose 화면과 Activity 생명주기. 시작 시 진단 객체를 만들고 정지 시 구독을 정리한다.
- 같은 폴더의 `VehicleInputProbe.kt`: Vehicle 초기화, capability 확인, 초기값 조회, 변경 알림 구독, 오류·null 표시. SDK 콜백은 메인 스레드로 전달한다. 종료 시 대기 중인 조회 콜백이 끝난 뒤 release한다.
- 같은 폴더의 `SafetyEligibility.kt`: P 확인·DO=false·UX flags=0을 모두 만족할 때만 사전조건 충족으로 판정한다. 누락 신호는 허용하지 않는다.
- 같은 폴더의 `DrivingSafetyProbe.kt`: 공개 AAOS CarUxRestrictionsManager와 GEAR_SELECTION을 읽는다. foreground마다 다시 연결하고 서비스 단절·화면 정지 시 이전 상태를 폐기한다. minSdk28을 유지하며 이 진단 리더는 API29+에서만 동작한다.
- `API_CAPABILITIES.md`: 문서 근거, 권한, 검증 상태.
- `EXPERIMENT_LOG.md`: 에뮬레이터 관측 결과와 재현 방법.
- `DECISION_LOG.md`: 이번 기술 선택과 제한.
- `docs/research/CONTEST_RESEARCH.md`: 2026-10-27 기준 공모전·개발 문서 조사 및 출처.

## 설명 가능한 범위와 남은 문제

현재 화면은 SDK 연결 진단용이다. 차량 제어 API를 호출하지 않으며 실제 페달 압력·게임 운동을 측정하거나 생성하지 않는다. 브레이크는 Boolean이고 조향은 SDK 원본 degree 값을 보존한다. 원운동 등 물리 검증은 아직 수행하지 않았다.

콘솔과 SDK 조향의 부호가 반대이며 capability 범위도 전륜각으로 해석하기 어려운 수치다. 임의 조향비·부호 보정은 도입하지 않았다. 내비게이션 전환으로 앱을 정지한 뒤 복귀하는 시험에서 구독 해제·release, 최신 조향값 재조회 및 이후 변경 알림 수신을 확인했다(P0-003, P0-007). SDK 단절 시험은 P0-005에서 수정 필요로 판정했다. 자세한 지연 측정·미지원 환경·콜백 무응답은 검증 대기다. 특히 SDK가 콜백을 영원히 반환하지 않는 경우의 timeout/자원 회수 정책은 아직 없다.

주행 제한 리더의 순수 판정 테스트 7개와 기존 예제 1개가 통과했다. 에뮬레이터는 P에서 DO=false/flags=0, D에서 DO=true/flags=80을 전달했다. D 직후 UX 콜백 이전에도 기어 신호로 차단됐으며, P 복원 뒤 UX 해제까지 기다린 후 조건 충족으로 전환됐다.

D 상태에서 앱을 새 프로세스로 시작한 1회 시험도 확인했다(P0-004). 초기 UNKNOWN에서 DO=true/flags=80 수신 후 BLOCKED가 됐고 플랫폼이 주행 중 사용 불가 화면을 표시했다. P로 복원하면 UX 해제 후 정상 진단 화면으로 돌아왔다. 게임·사운드 중단이나 자동 재개 정책의 검증을 뜻하지 않는다.

Phase 0의 남은 과제는 입력 의미·단위·지원 범위 정리와 SDK 단절 시 입력 무효화 보완·미지원 환경 등이다. 권한 누락은 P0-006에서 시험했다. 안전 리더의 epoch 방어 코드는 검토했으나 AAOS 서비스 강제 종료·재연결 및 Binder read 중단은 실행하지 않았다. 사용자 지시에 따라 이를 구현을 막는 조건으로 두지 않는다. 3D 표현은 Unreal로 시작했으며 차량 물리 모델은 아직 구현하지 않았다.

## 후속 묶음 시험 P0-005~007

권한 누락은 실제 권한을 제거한 별도 APK에서 오류 표시·UNKNOWN을 확인했다. 재진입 반복5회와 현재 소스 정책7개·기존 예제1개도 통과했다. 시험용 APK는 제거했으며 스크립트와 로그·화면은 보관했다.

**수정 필요:** SDK 전용 서비스를 강제 종료하면 조향·브레이크 화면에 이전 값과 READ 표시가 남는다. 현재 입력 연결의 유효성·신선도 표시가 없다. AAOS P·UX 상태는 별도 연결이므로 정상으로 남는다. 앱 새 프로세스로 재시작하면 서비스와 입력은 복구되지만 자동 복구 통과로 보지 않는다. 원본 동작 코드는 이번 묶음 시험에서 수정하지 않았다.

이 후속 기록이 앞선 '서비스 단절·권한 거부 검증 대기' 상태를 갱신한다. SDK 서비스2.3.22에서 시험했으며 AAOS 서비스 자체 단절·미지원 환경·실제 null·지연 콜백·영구 무응답·장시간 누수는 여전히 대기다. Phase 0 완료로 판단하지 않는다.

## 2026-10-28 첫 저마찰 주행 구현 중간 기록
C++ 평면 차량 운동, 고정 1/240초 RK4, WASD와 제동/선회 실험·CSV·HUD·계기판 미리보기를 추가했다. C++ 빌드와 독립 물리 검사는 통과했다. 새 레벨은 /Game/Maps/IceTrackDrive이며 상세 Epic 스포츠카·빙판 텍스처·설산·노면 표식과 조명을 생성했다. 기존 PhysicsLab은 보존했다. 실제 실행 화면을 확인하며 구도·모델 축을 수정 중이다. 실행 가이드는 unreal/BeyondPhysics/OPEN_AND_PLAY.ko.md. APK·Pleos·실제 계기판 출력은 아직 연동하지 않았다.

### 주행 피드백 반영
정지출발 유지(사용자 선택), 가상 AWD(앞5/9·뒤4/9), 기본1인칭 및 V 외부시점. 타이어 힘의 초기방향 종속 가설은 회전불변성 검사로 재현되지 않았다. 개별 바퀴의 시각적 회전 속도를 접지점 강체속도 투영으로 보완했다. 방향 진단 BODY/COURSE/FRONT WHEEL을 화면에 추가했다. 전체 모델 검사와 LiveCoding 통과. 바퀴 회전 동역학·휠스핀·서스펜션은 여전히 미구현.

### 2026-10-29 도로 식별성 개선
IceTrackDrive의 차도를 폭10m의 어두운 아스팔트 계열 빙판 노면으로 표현하고 흰색 차선·가장자리·노견을 구분했다. 표면 반사와 노출을 낮추고 얇은 얼음 패턴을 남겼다. 실제 1인칭 게임 화면에서 대비 개선을 확인했다. 차량 물리 코드는 변경하지 않았다. 편집 기록은 Scripts/improve_road_contrast.tool.py, 확인 화면은 docs/research/road-contrast-20261029.png.

### 2026-10-29 3차로 아스팔트·벽 접촉 종료
사용자 요청으로 얼음 텍스처와 노멀맵을 노면에서 완전히 제외했다. `MI_AsphaltRoad`와 별도 갓길/콘크리트 재질,3.5m×3차로와 양옆1.5m 갓길,1.2m 높이 벽을 IceTrackDrive에 저장했다. 기본 정지 출발은 가운데 차로이며 μ0.12/0.85 프로파일과 차량 힘 모델은 유지한다. 차체 외곽과 벽의 접촉으로 주행을 동결하고 R로 속도·위치·시간을 초기화한다. 그래픽 편집 기록은 `Scripts/build_three_lane_road.tool.py`, 접촉 기하 판정은 `Physics/RoadBoundary.h`, 게임 상태 전환은 `Driving/DrivingPawn.cpp`다. 이는 접촉 종료 규칙이며 충돌 반발/변형 물리는 아니다.

### 2026-10-29 속도·도로 길이 조정
후속 요청으로 저마찰 기본값을μ0.18로 조정했다(일반0.85 유지). 정지 전가속1.7658m/s²,10초 약63.6km/h이며 기존0.12 대비가속1.5배다. 접지 한도·질량·차량 힘 방정식과 고정 적분은 유지한다. 3차로 도로·벽·차선·반사 표지를2km로 연장하고 출발점-440m/끝 제한±995m로 맞췄다. 이전값μ0.12 실험·문서 기록은 역사적 결과이며 현재 프로파일과 구분한다. 복구 사본은 backups/longer-faster-road-20261029, 계산 검사 로그는 docs/research/longer-faster-model-tests.log다.

