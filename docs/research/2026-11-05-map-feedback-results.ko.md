# 세 맵의 차량 감각 효과 — 구현과 실제 검증

2026-11-05 KST. 사용자 요청에 따라 새 맵을 늘리지 않고 기존 세 맵의 조명·공조·사운드를 연결했다. [사용·수정 안내](../MAP_FEEDBACK.ko.md), [상세 증거 JSON](map-feedback-final-evidence.json).

## 실제 동작

| 상황 | 조명·공조 실제 값 | 결과 |
|---|---|---|
|원래 설정|흰색70%,팬3,운전/동승석22°C,공조ON/AUTO OFF/AC ON|시작 기준|
|직선 얼음|청록→접지력 사용에 따른호박,밝기20%,팬2,운전석21°C/동승석22°C|SDK getter와콘솔APPLIED|
|곡선 얼음|같은색언어/실제슬립·접지력상태,밝기20%,팬2,운전석21°C|SDK getter와콘솔APPLIED|
|안개 고속도로|청회색,시야기준내앞차경고색,밝기20%,팬1,양쪽22°C|SDK getter와콘솔APPLIED|
|OFF·맵 이동·접촉 종료·P해제|원래흰색70%/팬3/양쪽22°C|복원 확인|
|사용자색#123456/팬4 변경|사용자값유지,앱소유밝기70%/온도22°C복원|USER OVERRIDE로양보|
|AC OFF|팬2만적용,온도22°C유지,OFF후팬3|냉방강제ON없음|
|Settings전환→앱복귀|원래값복원,복귀시restart=1|자동재개차단|

설정창을 실제터치로 열고 조명/공조 각각ON→APPLY AND RESTART→실제SDK APPLIED→다시설정창/복원→선택OFF를 확인했다. 최종상태는P,차량속도/가속/제동/핸들0,원래조명/공조복원,게임설정창열림·조명/공조OFF·소리ON이다. 효과는물리상태를표현하며기존물리코어7파일·도로맵3개해시는작업전백업과동일하다.

최종세맵/P시험: map-feedback-sdk-results.json, map-feedback-sdk-console.log, map-feedback-sdk-runtime.log. 실제터치: map-feedback-ui-evidence.json 및 map-feedback-ui-console.log. 보조수명주기시험: map-feedback-manual-evidence.json, map-feedback-fan-only-evidence.json, map-feedback-background-evidence.json. AAOS HOME은내장게임을RESUMED로두므로그시도는실제백그라운드검증에서제외하고Settings전환을썼다.

## 실제 API 차이와 수정 근거

Light.kt공개명세의읽기권한외에LightImpl.kt배포구현은CONTROL_CAR_LIGHTS를요구한다. 실제패키지권한을확인하고선언했다. 조명색/밝기는전체5좌석연동,팬/AUTO는앞좌석2개연동,온도는운전/동승석각영역,전원/AC는전체5좌석으로지원영역이다르다. 각API가광고한영역만골라쓰기영향전체의전원/AUTO/AC상태를확인한다.

주행모드자동색상capability는명시적으로ERROR_NOT_SUPPORTED_PROPERTY여서그선택기능만생략했다. 권한/타임아웃/지원후getter/setter오류는생략하지않는다. SDKgetter는흰색16777215/청록3382463의RGB24를반환하고ARGB32요청을받는것을앱의실제SDK로그에서확인했다. emulator-only불투명alpha정규화로읽기/요청/복원을비교한다. 소스폴더의해당호환로그와setter성공이후getter검증을유지했다.

원본값을모르면무쓰기,쓰기전journal저장,값검증,수동변경양보,직렬/중복제거,1.5초게임상태heartbeat,OFF/중단/다음세션복원으로구현했다. 검토에서실패한복원의재시도누락/Android오디오포커스/영역guard부족을찾아수정했다. 세부원인은코드와회귀검사에남겼다.

## 소리

별도C++오디오컴포넌트가자체합성가상전동차구동/바람/노면/슬립/제동과단발발견·접촉음을믹스한다. 바퀴실측녹음이나엔진RPM/ABS측정이아니다. 하나의재사용voice와고정상태,음량완화/리미터를쓰며기존물리에개입하지않는다. Android플랫폼오디오포커스의최초실제허용/손실을앱foreground와별도로처리한다.

최종APK의실제마스터AudioMixer출력을짧게녹음했다. `BPAudioCapture`진단명령은2~10초로제한하며자기앱private files/AudioProof로저장한다. 공식debugrun-as로복사했다. [오디오측정](map-feedback-audio-evidence.json):48kHz/2채널,각2.94~3.96초,0이아닌PCM과클리핑없음을확인했다. 저속램프출발녹음은속도가낮아조용하다. 이는디지털출력증거이며실차스피커/청취품질평가가아니다.

- [직선실제출력](map-feedback-audio-straight.wav)
- [곡선실제출력](map-feedback-audio-ramp.wav)
- [안개실제출력](map-feedback-audio-fog.wav)

## 빌드와 검증

효과정책C++검사10개실패0(미구현상태에서4개실패후구현통과). Kotlin기존입력+출력소유권/실패/복원/심박/영역/색호환검사총22개실패0. 로그 map-feedback-policy-red.log/green.log, map-feedback-bridge-final.log 및Gradle test-results. 최종WindowsEditor모듈빌드성공은 map-feedback-editor-build-final.log. Android전체cook후내용이같은자산을재사용해최종수정APK패키징/설치성공. 최종실제세맵SDK/P시험과터치설정/오디오읽기시험종료코드0.

원본APK: `<WORKSPACE>\builds\android-drive\BeyondPhysics-x64.apk`,198819176bytes,Development/x86_64. SHA256 `b7ff5a73d4129c9181271bc7f59719e1e1d1c7789b4ecb624361a5ae7314533e`. Android Studio의이전APK사본은자동갱신되지않으므로새원본을가져온다. 이전APK/Source/맵해시는backups/map-feedback-20261105에보존했다.

## 검증 범위

실차사용승인·차종별capability·물리적공조체감·스피커음질·장시간실차성능·경쟁외부앱포커스손실강제시험은미수행이다. 강제종료/전원차단/서비스장애복원은journal기반최선노력이고즉시복원보장은아니다. PC조명/공조는SIMULATED다. 새맵/물리정밀도/μ변경은없다. 현재Unreal Editor와메모리과다자동조작프로세스는닫힌상태를유지했고최초실제HTTP MCP조회는연결거부였다. 공식빌드도구로코드와APK를갱신했으며MCP편집을성공했다고주장하지않는다.

공식심사의기술활용성은핵심경험에적절한SDK사용이며API개수별가점표는확인되지않았다. 별도가산점은영상/GitHub소스+APK개발산출물이다. [공식심사기준](https://appdevchallenge.auto-hmg.io/guide/judging-awards).

![실제 APK 설정 화면](map-feedback-apk-settings.png)
