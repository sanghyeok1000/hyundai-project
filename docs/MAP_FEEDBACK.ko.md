# 맵별 조명·공조·사운드 사용과 수정

2026-11-05 KST. 같은 APK의 세 맵에서 사용하는 선택형 차량 감각 효과다. Unreal의 물리 계산을 읽어서 표현하며 μ·조향·제동·도로 형상·접촉 판정에는 개입하지 않는다. 에뮬레이터 전용 프로필과 P/UX/입력 신선도 제한을 유지한다.

## 사용

화면 상단 **EFFECTS**(PC는F)를 누른다. 게임이 멈추며 **AMBIENT LIGHT**, **GENTLE CLIMATE**, **DRIVING AUDIO**를 각각 선택한다. PC는 설정창에서L/H/K로 전환할 수도 있다. **APPLY AND RESTART**를 누르면 선택을 적용하고 새 시도를 시작한다. 조명/공조는 앱을 처음 켤 때OFF, 소리는ON이다. 선택은 맵 전환 중 유지되지만 앱 재실행 시 차량 효과는OFF로 돌아간다. 효과를 끄고 다시 켜면 사용자의 수동 변경으로 양보한 기능을 다시 요청할 수 있다.

| 맵 | 조명 | 공조 | 소리 |
|---|---|---|---|
|01 직선 얼음|청록 기본, 실제 접지력 사용·횡슬립 증가 시 부드러운 호박색|약한 팬2, 조건이 맞을 때 원래 설정보다 최대1°C 낮춤|가상 전동 구동음·속도별 노면/바람·부드러운 슬립음|
|02 곡선 얼음|실제 코너 접지력 사용/슬립에 반응하는 같은 색 언어|일정한 약한 팬2와 선택적 냉감|회전각 자체가 아니라 실제 슬립/속도에 따른 마찰음|
|03 안개 고속도로|옅은 청회색, 선언된 시야30m 안에서 앞차가 시야각에 들어오면 호박색|온도 유지, 약한 팬1|고속 노면/바람, 시야 내 앞차 알림1회, 제동음, 접촉음1회|

안개에 가려진 멀리 있는 차량을 미리 경고하지 않는다.30m는 앱의 선언 기준이며 계측한 실제 사람의 인지거리가 아니다. 소리/색/차가운 공기는 교육용 감각 연출이고 실제 차량 경보·ABS/ESC·정밀 열환경/음향 모델이 아니다. 가상 구동음은 자체 합성 전동차 음향으로 실제 엔진RPM을 계측한 녹음이 아니다. 일반 소리 재생을 Pleos TTS SDK 활용으로 표시하지 않는다.

## 적용 상태와 장치 조건

설정창과 상단 상태줄은 SDK의 **APPLIED**, **OFF**, **UNSUPPORTED**, **ERROR**, **RESTORING**을 구분한다. PC는차량 쓰기 없이SIMULATED 프리뷰다. 원본 읽기/쓰기 capability·실제 영역·원래 값이 확인되지 않으면 차량을 수정하지 않는다. setter 콜백 후 실제getter를 다시 읽어APPLIED를 판단한다.

차량 조명의 최대 밝기는 SDK 지원 최댓값의20%로 제한한다. 미세한 색 변화는 완화하고 점멸하지 않는다. 색/밝기 영역은 장치가 실제 광고한 운전자·연동·전역 영역을 따르므로 전체가 함께 바뀔 수 있다. 주행모드 자동 색상 기능이 켜져 있으면 이를 강제로 끄지 않고 미지원 사유를 표시한다.

공조는 이미 전원ON·AUTO OFF인 지원 영역에서만 약한 팬1~2를 요청한다. 에어컨ON이면 지원 온도 단계와 원본값을 확인한 뒤20~26°C 범위에서 원래보다 최대1°C만 낮춘다. 에어컨OFF/온도 증분 미확인/범위 밖 원본은 온도를 바꾸지 않고 가능하면 팬만 사용한다. 원래 공조 전원·AUTO·AC·송풍 방향·제상은 강제로 바꾸지 않는다. 팬/온도/전원 등은 같은 이름의 영역을 공유한다고 가정하지 않고 각 API의 실제 지원 영역과 영향 범위에 맞춰 조회한다.

## 중단·사용자 설정·원상복원

앱은 처음 쓰기 전에 원본값과 쓰기 의도를 내부 복원 기록에 저장한다. 적용 중 사용자가 다른 값으로 변경하면 해당 그룹은 양보하고 사용자의 새 값을 덮어쓰지 않는다. 효과OFF·설정창·맵 전환·접촉/정지·P 해제·UX 제한·백그라운드·종료 때 효과를 멈추고 앱이 소유한 값만 복원한다.500ms 직렬 작업으로 중복 요청을 줄이고, 새 게임 상태가1.5초간 없으면 독립적으로 효과를 중단한다. 다시 시작하려면 비활성 확인/RESTART가 필요하다.

복원이 실패하면RESTORING과 원인을 유지하고 재시도한다. 앱 내부 기록을 사용해 다음 SDK 연결 때 남은 복원을 시도한다. 프로세스 강제 종료·전원 차단·SDK 서비스 장애에서 즉시 복원을 보장하지 않는다. 사용자가 바꾼 모드나 지원이 없어진 기능은 안전하게 복원할 수 없어 보류될 수 있다. 실제 차량 조작 허용·마켓 권한 승인·실차 장치/음향 품질 검증은 에뮬레이터 성공과 별개다.

소리는 별도Android 오디오 포커스를 확인한다. 다른 앱에 포커스를 넘기라는 요청이면 즉시 멈추고, 앱 화면이 보인다는 이유만으로 계속 재생하지 않는다. 앱이 다시 재생할 수 있을 때만 다음 유효 프레임부터 이어가며 오래된 발견/접촉 알림을 재생하지 않는다. 차량 마스터 음량은 수정하지 않는다.

## 코드를 수정하는 위치

- `unreal/BeyondPhysics/Source/BeyondPhysics/Feedback/FeedbackPolicy.cpp`: 맵 색·위험 표현·팬/냉감 요청·음향 강도. 물리식과 분리된 표현 정책이다.
- `Feedback/DrivingAudioComponent.h/.cpp`: 재사용 오디오 음색·스무딩·단발 알림·음량 상한·포커스.48kHz 모노 합성, 여러 이벤트를하나의 오디오 voice로 믹스한다.
- `Driving/DrivingPawn.h/.cpp`: 실제 물리값→효과프레임, 사용자 선택,0.25초 JNI 요청,맵 이동/중단 처리.
- `UI/DrivingHUD.cpp`: 효과 설정창·버튼·실제 SDK 상태 표시.
- `android/pleosbridge/src/main/java/com/beyondphysics/pleos/PleosEffects.kt`: SDK capability/영역/읽기/쓰기/실제 적용 확인.
- `EffectOwnership.kt`: 원본과 쓰기 의도·소유권·사용자 변경·복원 기록/온도 한도/영역·heartbeat 정책.
- `PleosRuntime.kt`: 기존 입력/실행 제한과 출력 세션 정리.
- `unreal/BeyondPhysics/Build/Android/Pleos_UPL.xml`: 권한·AAR/JNI·오디오 포커스 초기 확인.

Vehicle2.3.2 공개 Light 인터페이스와 실제 LightImpl 제어 권한이 달라 내부 구현의 `CONTROL_CAR_LIGHTS`도 선언했다. 읽기 `CAR_LIGHTS`, 공조 `CAR_CLIMATE`/`CONTROL_CAR_CLIMATE`와 기존 입력 권한을 구분한다. 이 권한 선언이 실제 차량이나 마켓 승인을 대신하지 않는다.

현재 에뮬레이터는 색을RGB24로 반환해 흰색16777215·청록3382463을 관측했다. ARGB32 요청과 비교할 때 불투명alpha를 동일하게 정규화한다. 이는 emulator-only 호환 처리이며 실차 데이터 형식으로 일반화하지 않는다. 명시적인 선택 API의 미지원 오류와 권한/타임아웃/실제getter 오류를 구분한다.

수정 후 기존 패키징/설치 스크립트를 사용한다. 콘텐츠가 바뀌지 않은 C++/Kotlin/권한 수정은 `package_pleos_emulator.ps1 -Scene Drive -SkipCook`로 기존 cook결과를 재사용할 수 있다. 맵/재질 자산이 바뀌면 전체cook을 수행한다. Android Studio의 기존 APK 사본은 새 원본으로 다시 가져온다.

공식 근거: [공모전 심사](https://appdevchallenge.auto-hmg.io/guide/judging-awards), [Pleos 조명](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/light/set-ambient-light-brightness-color), [공조 풍량](https://document.pleos.ai/docs/pleos-only/connect-sdk-pleos/Vehicle/vehicle/ai.pleos.playground.vehicle.api/h-v-a-c/set-hvac-fan-speed), 설치된 SDK2.3.2 배포 소스와 Unreal5.8 Android/AudioMixer 구현. 기능 개수별 별도 가산점은 확인되지 않았고 핵심 경험에 적절한 SDK 활용을 평가한다.
