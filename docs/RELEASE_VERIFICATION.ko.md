# Release 검증 결과

빌드·실행 검증일: **2026-11-08**. 대상은 v0.1.0의 `BEYOND_PHYSICS_release.apk`다.

## 배포 파일

| 항목 | 값 |
|---|---|
|패키지|`com.beyondphysics.simulator`|
|버전|0.1.0 / versionCode 1|
|구성|Unreal5.8 Shipping · Android Distribution · Release AAR|
|크기|121,751,742bytes|
|ABI|x86_64|
|SDK|minSdk28 / targetSdk36|
|APK SHA256|`2ba0f6ca2c93408d07b6d83c72e65c3c213899d5f663260b1264cbf07ee47c00`|
|서명 인증서 SHA256|`177ecc0f423d109885c8773c366c4d6a2c112695e4849d3dbcfd4f30f63f8c0c`|

## 패키지 검사

| 검사 | 결과 |
|---|---|
|APK 서명|전용 RSA3072 키 · Signature Scheme v2 검증 통과|
|디버깅 비활성|aapt2 debuggable 항목 없음 · 설치된 패키지 DEBUGGABLE 없음 · run-as 차단|
|파일 정렬|zipalign의16KiB 페이지 /4byte 정렬 검사 통과|
|파일 무결성|APK와 내장 OBB의 ZIP CRC 검사 통과|
|설정 포함 검사|82개 패키지 INI에 개인 서명 값·서명 경로·File Server 토큰 없음|

## Pleos 실행 검사

환경: Pleos Connect Emulator v2.1.0 / Android14 API34.

| 항목 | 확인 결과 |
|---|---|
|설치·시작|APK 설치 Success, 앱 화면 표시|
|입력|Vehicle SDK 조향·브레이크 READY, 자동 가속·제동·조향 반응|
|맵 전환|실제 터치로 직선 → 램프 → 안개 → 직선|
|얼음 효과|SDK APPLIED, 조명 밝기20%, 팬2, 운전석22→21°C|
|안개 효과|SDK APPLIED, 청회색 조명·밝기20%, 팬1,22°C 유지|
|설정 복원|설정창·D 전환·OFF에서 원래 조명·팬3·22°C 복원|
|주행 제한|D 이용 제한 화면, P 복귀 후 RESTART 요구|
|오디오|포커스 요청과48kHz OpenSL ES 재생 track started 확인|

수치와 원래 상태는 [증거 요약 JSON](release-verification.json)에 있다. [실행 화면](images/release-straight.png)도 같은 Shipping APK에서 캡처했다. 이번 오디오 확인은 재생 경로와 포커스 상태 검사다.

## 물리·정책 검사

C++ 검사 코드는 [`Tests/`](../unreal/BeyondPhysics/Tests), 브리지 검사는 [`src/test/`](../android/pleosbridge/src/test/java/com/beyondphysics/pleos)에 있다. 2026-11-05에는 Kotlin22개 검사와 감각 정책10개 검사를 통과했다. 물리 기준식과 검사 대상은 [물리 모델](engineering/PHYSICS_MODELS.md)에 정리했다.

Release 패키징 과정에서는 R8의 Gson 제네릭 정보 보존 규칙을 적용하고 SDK 실동작을 재확인했다. 관련 설정은 [빌드 안내](RELEASE_BUILD.ko.md#release의-sdk-보존-규칙)에 있다.

## 적용 범위

위 결과는 에뮬레이터에서 확인한 프로토타입 결과다. 실차·ARM64·운전대 뒤 계기판과 장시간 성능·발열 평가는 후속 검증 범위다.
