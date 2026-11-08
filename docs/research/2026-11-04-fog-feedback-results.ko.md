# 안개 맵 사용자 피드백 반영 결과

2026-11-04 KST. 사용자 선택: 특정 차종 없이 근거를 기록한 일반 승용차 모델. 구현과 최종 APK 설치·실행 확인을 완료했다. [모델/가정/수정 경로](../FOG_FEEDBACK_PHYSICS.ko.md), [기계 판독 증거](fog-feedback-final-evidence.json).

## 변경 결과

- 맵03 가상 초기속도80km/h, 목표100km/h. 초기조건 이후 접지력/강체 적분으로 가속한다.
- 시작x=-260m, 공간 안개 시작x≈-230m. 첫 차량 범퍼까지255.04m로 종전1.9km 접근을 줄였다. 도로 전체4km는 유지한다.
- 중앙(0,0), 좌(45,3.5), 우(85,-3.5)m에 각 한 대.45/40m 간격으로 처음부터 존재한다. [저장된 컴포넌트/재질 검증](fog-feedback-scene.json).
- 안개 반경450m·중심220m·밀도0.27, 선언 기준30m. 실제 사람의 인지거리/시정계를 보정한 수치는 아니다.
- 맵03만18:1 조향비, Ackermann 좌우 앞바퀴각, 네 접지점 현재 속도/힘/요모멘트 계산. 같은 각도를 바퀴 렌더링에 사용한다. 기존 두 얼음 맵의 자산 SHA256은 작업 전 백업과 동일하다.

## 실제 Pleos APK 시험

SDK 시험은 실제 콘솔 DDS→Vehicle SDK→AAR/JNI→Unreal 경로이며 게임 상태 강제 주입이나 순간이동을 쓰지 않았다. BPReset/BPReportState는 재시작/측정만 요청했다. 에뮬레이터의 실제 차량 신호는 P/속도0/가속0이며 게임의 가상80→100과 구분한다. 각 자동 시험 종료코드0.

| 시험 | 측정 결과 | 근거 |
|---|---|---|
| 조향 연결 | SDK 핸들-90°→중앙-5°; FL-4.852923°/FR-5.156246°; four_contact=1 | fog-feedback-sdk-steer.json/.log |
| 미제동 | 초기80,최대99.999998km/h,9.468143초 앞차 접촉 | fog-feedback-sdk-missed.json/.log |
| 예방 제동 |100km/h 부근에서 실제 SDK 브레이크;49.156398m 후 정지,범퍼간격52.620302m;기준 전 제동이라 반응N/A | fog-feedback-sdk-early.json/.log |
| 근접 급조향 |30m 이내 보고 후 실제 핸들-540°/중앙-30° 요청,9.477018초 접촉 | fog-feedback-sdk-dodge.json/.log |
| 주차 제한 |D에서 allowed=0/PARK REQUIRED,P복귀 allowed=1/READY이나 restart=1 유지 | fog-feedback-sdk-final-gate.log |

자동 입력 결과는 사람 반응시간 측정이나 모든 회피 경로의 불가능성 증명이 아니다. SDK 자동 입력에는 샘플/HTTP 지연이 있으므로 정확히30m/0.5초 조건은 아래 순수 코어 시험과 구분한다. 앱은 최종 P·입력0·충돌 후 RESTART 대기 상태다. RESTART로 새80km/h 시도를 시작한다.

## 필요한 물리 검증과 검토

네 접지점 검사는 Ackermann 관계, 각 접지점 순수 원운동 속도, 바퀴별 μFz 한도, 힘/모멘트 합산, 영윤거에서 기존 bicycle과 일치, 좌우대칭/월드방향 불변성, 전·후진 에너지 소산을 통과했다.5m/s 정상선회 요레이트0.00900785rad/s 대 선형 기준0.00900826;25m/s 0.0269557 대0.0272879.100km/h·μ0.8 직선 제동49.1594m 대 해석49.1593m. 로그 fog-feedback-contact-green.log.

세 차량 충돌/연속 스윕/첫 기준ID 고정/다른 차량 접촉ID/예방 정지 검사 실패0. 검토에서 기준 차량 간격을 뒤의 실제 충돌 차량 간격으로 덮는 문제를 찾아 수정했다. 회귀 검사의 RED/GREEN은 fog-feedback-reference-red.log/green.log. 최종 FogChecks/FogTrafficChecks 실패0은 fog-feedback-final-run-fog-checks.log와 fog-feedback-final-run-fog-traffic-checks.log.

같은 코어로 선언 기준30m 후0.503524초에 제동한 경우 접촉했고, 기준 후0.5초 최대 조향도 접촉했다.100m 부근에서 예방 제동하면49.1568m 후 정지했다. 로그 fog-feedback-scenarios.log. 기존 물리/램프/학습 검사도 각각 한 차례 통과했다.

모델은 정적 하중·평면3자유도·준정상 타이어 힘이다. 타이어 휠 각운동·종슬립/ABS/ESC/서스펜션/동적 하중이동/공력/양산차 실측 보정은 없다. 자세한 가정과 출처는 모델 안내에 기록했다.

## 빌드·화면·연결

Editor C++ 빌드와 최종 Android BuildCookRun 성공, ADB 설치/실행 성공. 최종 패키징224.87초이며 로그 fog-feedback-package-console.log, unreal-android-drive-build-latest.log, fog-feedback-install.log. 원본은 `<WORKSPACE>\builds\android-drive\BeyondPhysics-x64.apk`,198708712bytes, SHA256 `2a88c9fc9374519c15cbcda1becee38f462b5653c3d50ab2659e5956ef955abd`.

실제 HTTP MCP 조회 반환은 `/Game/Maps/FogHighwayDrive`이며 fog-feedback-mcp-after.json에 저장했다. Codex native Unreal 도구 목록이 생겼다는 의미는 아니며 설치된 MCP HTTP helper로 호출했다. Editor는 새 맵을 열고 시작 카메라로 준비했다. 새 세 차량 컴포넌트는 공식 Unreal Python commandlet로 fog맵만 다시 저장했다. 실제 APK 안개/접촉/정지 화면과 Editor의 짙은 안개를 확인했다. 높은 상공의 배치 캡처는 안개로 차량이 가려져 시각적3대 검증으로 쓰지 않으며 저장 컴포넌트 기록/각 차로 충돌 검사를 근거로 삼는다.

CSV/summary 저장 성공 로그는 확인했으나 앱 private 파일을 추출해 내용을 직접 검사한 것은 아니다. Android Studio의 이전 APK 사본은 자동 갱신되지 않으므로 원본 새 APK를 다시 가져온다.

![실제 Pleos APK의 짙어진 안개](fog-feedback-apk-missed-fog.png)

![실제 SDK 급조향 후 앞차 접촉](fog-feedback-apk-dodge-result.png)
