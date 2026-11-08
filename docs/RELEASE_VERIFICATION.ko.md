# 제출용 Release APK 검증 — 2026-11-08

대상: `BEYOND_PHYSICS_release.apk`, 버전0.1.0 / versionCode1, 패키지 `com.beyondphysics.simulator`.

| 항목 | 결과 |
|---|---|
|Unreal 빌드|UE5.8 Shipping + Distribution 성공, Release AAR 사용|
|APK 크기|121,751,742bytes (약122MB /116.1MiB)|
|SHA256|`2ba0f6ca2c93408d07b6d83c72e65c3c213899d5f663260b1264cbf07ee47c00`|
|서명|전용 RSA3072 키, APK Signature Scheme v2 검증 성공|
|서명 인증서 SHA256|`177ecc0f423d109885c8773c366c4d6a2c112695e4849d3dbcfd4f30f63f8c0c`|
|디버깅 비활성|aapt2에 application-debuggable 없음, 설치된 패키지 DEBUGGABLE 플래그 없음, run-as 거부 확인|
|정렬/무결성|zipalign 16KiB 페이지/4byte 정렬 검사 통과, APK 및 내장 OBB ZIP CRC 오류 없음|
|ABI / SDK|x86_64만 포함, minSdk28 / targetSdk36|
|실행|Pleos Connect v2.1.0, Android14 API34 에뮬레이터에서 설치·실행 성공|
|입력|실제 Vehicle SDK 조향/Brake 수신 READY, 가속페달0에서 자동 가속, 제동·조향 화면 변화 확인|
|맵|실제 터치로 직선→램프→안개→직선 전환|
|감각 효과|세 맵 SDK APPLIED. 얼음 팬2/운전석21°C, 안개 팬1/22°C. 조명 밝기20%와 맵별 색 적용|
|복원/제한|설정창·D 전환·OFF에서 원래 조명/팬3/22°C 복원. AAOS 주행 제한 화면 및 P 복귀 후 재시작 잠금 확인|
|오디오|Android 오디오 포커스 요청 및 48kHz OpenSL ES 재생 track started 확인. 이번 점검에서 청취 품질 평가/새 WAV 녹음은 하지 않음|
|개인 설정 제외|최종 APK의82개 INI에서 SecurityToken/KeyStore/KeyStorePassword/KeyPassword 항목과 실제 서명 비밀번호 없음|

Release에서만 나타난 첫 실패도 기록한다. R8이 Gson TypeToken 계층의 제네릭 정보를 제거해 SDK capability 읽기가 실패했다. Google Gson/Android 공식 안내에 따라 `android/pleosbridge/consumer-rules.pro`에 Signature와 TypeToken 계층 보존 규칙을 추가했다. 동일한 Shipping 빌드를 다시 설치한 뒤 오류가 사라지고 위 실제 입력·장치 적용 검사를 통과했다. 코드 축소 전체를 끄거나 게임 입력을 모의 값으로 대체하지 않았다.

세 맵과 차량 물리 원본은 이 작업에서 바꾸지 않았다. 새 물리 정밀도 검증이나 실차 시험을 수행한 것은 아니다. 기존22개 Kotlin/10개 효과 정책 검사는2026-11-05 개발 검증 이력이며 이번 Release 스모크 테스트와 구분한다. 원본 개발 앱 데이터는 교체 전에 로컬 백업했다.

현재 한계: x86_64 에뮬레이터/인포테인먼트용 프로토타입. ARM64 실차·운전대 뒤 계기판·제조사 승인·특정 차종 실측 보정·장시간 성능/발열 검증은 완료하지 않았다. 도로/산/안개는 현재 프로토타입 그래픽이며 실사 품질 인증을 의미하지 않는다.

증거 요약: [release-verification.json](release-verification.json), 실제 화면은 `images/`에 있다. 비공개 전체 빌드 로그·개발 앱 백업·개인 서명 자료는 GitHub 사본에서 제외했다.

참고: [Gson 문제 해결](https://google.github.io/gson/Troubleshooting.html), [Android R8 full mode](https://developer.android.com/topic/performance/app-optimization/full-mode), [Unreal Release 서명](https://dev.epicgames.com/documentation/unreal-engine/signing-android-projects-for-release-on-the-google-play-store-with-unreal-engine).
