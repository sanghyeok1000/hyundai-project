# 곡선 맵 전용 입력·비교 화면 검증

사용자가 승인한 변경: μ0.12와 접지 한계를 유지하고, 두 번째 맵의 자동 가속을 완만하게 하며 제동 후 낮춘 가속 목표를 유지한다. 이번/직전 시도의 진입 속도·궤적을 비교한다. 설명·수정 경로: ../RAMP_LEARNING.ko.md.

## 구현과 보존 범위

새 RampAutoDrive 입력 정책을 Model.bRamp에만 선택한다. 최대 throttle0.12, 초기 가속 목표50km/h, 제동 시 목표를 낮춰 유지하며 정지 후 출발 하한15km/h. 이 값은 게임 정책이며 실도로 안전속도가 아니다. 목표 초과 시 구동0/자동제동0이다. SDK 주차·중단·신선도 게이트는 그대로 유지한다.

VehicleModel.cpp, 기존 AutoDriveInput.h, IceTrackDrive.umap, IceRampDrive.umap은 작업 전 백업과 SHA256이 동일하다. ramp-learning-preserved-files.json 참조. 지형·μ·타이어/마찰원·수치적분을 바꿔 통과시킨 것이 아니다.

## 계산 검증

- 새 동작을 기존 입력으로 재현한 초기 RED: failures8(ramp-learning-red.log).
- 시험/리플레이가 일반 완료 기록에 섞이는 회귀 RED: failures1(ramp-learning-lifecycle-red.log), 수정 후0.
- 시도마다 μ를 보존하는 회귀 RED: failures1(ramp-learning-profile-red.log), 수정 후0.
- 최종 정책·측정 검사: failures0(ramp-learning-green.log).
- 기존 평면/입력 검사: failures0(ramp-learning-flat-regression.log).
- 기존 램프 형상/물리 검사: failures0(ramp-learning-geometry-regression.log).

같은 초기 정지 상태·동일한 조향 시험 운전자에서:

| 시도 | 진입 속도 | 결과 |
|---|---|---|
| 곡선 전 감속 없음 | 46.4653km/h | station394.455m에서 벽 접촉 |
| 곡선 전 미리 제동 | 21.5964km/h | station630.667m까지 통과, 최대 이탈0.0881m |

위치/요각을 강제로 맞추지 않고 같은 타이어 힘을 적분했다. 시험 운전자의 조향은 시험 코드에만 있으며 APK에 자동 조향을 넣지 않았다. 결과는 가상 모델의 특정 조건이며 실제 안전속도나 모든 운전자의 성공을 의미하지 않는다. 로그 ramp-learning-scenarios.log, 원시 궤적 ramp-learning-fast.csv/ramp-learning-slow.csv.

## 코드 검토

독립 검토에서 합성 입력/리플레이 이후 일반 결과가 잘못 완료될 가능성과, 완료된 결과의 재활성화·중복 저장 가능성을 발견했다. Exclude/IsRunning과 재시작 경계를 추가하고 실제 저장된 outcome을 사용한다. B/T/BPTestInput/BPWallTest/리플레이는 학습 결과에서 제외하며 R로 새 시도를 시작한다. 종료 후 SPACE만으로 새 결과를 쓰지 않는다. 비교 데이터에는 각 시도의 μ도 표시·저장한다.

## 실제 Pleos APK

Win64 Editor와 Android 코드 빌드, 패키징 BUILD SUCCESSFUL/ExitCode0. 맵/재질이 그대로이므로 첫 전체 패키징 이후 마지막 코드 수정은 SkipCook으로 기존 쿠킹 결과를 재사용했다. 최종 APK198,501,164바이트, SHA256 `041F50CE021A994D05BA22C00C7D3268C071385C4108787CF0620FAD1283D5AC`. adb install Success/GameActivity 실행 확인.

실제 Pleos API34/x86_64 APK의 SDK 입력만 이용해:

1. 직선 맵에서 브레이크 해제 throttle1/μ0.12 확인.
2. 곡선 맵의 throttle 상한0.12 확인. 첫 시도 진입46.4652792km/h, 벽 종료와 결과 패널 확인.
3. 재시작 시 직전 entry/outcome/trace 보존 확인.
4. SDK 브레이크로 미리 감속한 두 번째 시도 진입21.0269196km/h, 직전46.5km/h와 두 궤적이 화면에 함께 표시됨을 확인.
5. 첫 결과의 BP_RAMP_RESULT에서 saved1, μ0.12와 CSV 상대경로 확인. 앱은 Android 사용자10의 외부 저장소를 사용한다. 일반 adb shell/run-as에서 해당 외부 파일을 직접 읽는 것은 권한 거부되어 파일 원문 추출 검증은 하지 않았다. 저장 성공 반환과 로그를 근거로 보고한다.

SDK 시험은 조향을 중립으로 두어 UI·입력·진입 속도 비교를 확인한 것이며 두 번째 시도의 SDK 전구간 완주 시험이 아니다. 실제 전구간 통과 주장은 위 C++ 조향 시험 조건에 한정한다.

원시 기록: ramp-learning-apk-cases.jsonl, ramp-learning-apk-result.json, ramp-learning-saved-results.log. 실제 화면: ramp-learning-result.png, ramp-learning-comparison.png. 브라우저 가상 콘솔을 유지하고 마지막 에뮬레이터는 P 복귀 후 RESTART 대기로 정지했다. 새 코드로 다시 연 Unreal MCP get_current_level은 /Game/Maps/IceRampDrive를 반환했다.

## 남은 한계

정지마찰·차륜 회전·ABS, 동적 수직하중·서스펜션·6자유도·실측 얼음 타이어 검증은 이번에 추가하지 않았다. 결과 화면이 충돌 원인을 자동으로 속도로 단정하지 않는다. SDK 제동은 Boolean이며 완만한 가속 목표 변경과 별개의 입력 한계다.
