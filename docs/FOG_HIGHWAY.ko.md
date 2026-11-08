# 안개 고속도로 맵 사용·수정 안내

2026-11-04 최신: 사용자 피드백으로80km/h 출발·약30m 뒤 안개·첫 차량까지약255m·세 차선 차량3대·기준30m·4접지점/Ackermann으로 변경했다. 아래 최초 설계 설명은 이력이며 현재 실행/수정 기준은 FOG_FEEDBACK_PHYSICS.ko.md를 참조한다.

2026-11-04. 같은 APK의 세 번째 맵 `/Game/Maps/FogHighwayDrive`이다. 직선·곡선 얼음 맵은 μ0.12를 유지한다.

## 플레이

상단 MAP 버튼을 두 번 누르면 01 직선 → 02 램프 → 03 안개로 전환한다. PC에서는 M으로 전환한다. Unreal Content Browser의 Content/Maps/FogHighwayDrive를 열고 Play해도 된다.

정지 출발 후 가상 구동력이 자동으로 100km/h를 목표로 가속한다. PC A/D 조향·S 브레이크, Pleos에서는 기존 Steering/Brake 신호를 쓴다. 실제 가속페달은 읽지 않는다. 제동 중 구동력은0이며, 움직인 뒤 첫 제동은 자동 가속을 해제한다. 이후 브레이크를 놓아도 재가속하지 않는다. RESTART 또는 R로 새로 출발한다. 출발 전 브레이크를 잡고 있다가 놓으면 출발한다.

4km·3차로 가상 고속도로의 중앙 차로에 앞차가 처음부터 정차해 있다. 안개에 진입하면 거리에 따라 앞차 대비가 낮아지고 접근하면서 드러난다. 눈에 보이면 제동해 충돌 전에 정지하는 체험이다. 목표100km/h는 게임 조건이며 안개 속 권장속도가 아니다.

결과는 정지·앞차 접촉·벽 접촉·미정지 통과·측방 정지·중단으로 구분한다. 옆 차로에서 정지한 것을 앞차 뒤 정지 성공이라고 표시하지 않는다. 정상적인 100km/h 전제동의 이상 기준거리는 약49.16m다. 이 값에는 사람의 반응거리가 없다.

## 숫자의 의미

HUD의 `100M EXERCISE REFERENCE`는 범퍼 간격100m·전방 시야각 조건을 통과한 시각이다. 사람의 실제 발견 순간을 측정하지 않는다. `REF -> BRAKE`는 이 기준부터 첫 제동 입력까지의 시뮬레이션 시간이다. 미리 제동했으면 별도로 표시한다. `AFTER BRAKE`는 첫 제동 이후 종료까지 이동한 경로 길이이며 브레이크 해제·타력주행도 포함할 수 있다.

일시정지·입력 단절·앱 이탈·긴 프레임 정지는 진행 중 시도의 측정을 중단한다. 복귀 후 RESTART한다. 앞차를 보기 전에 일찍 제동해 멈춘 경우에도 RESTART로 새 시도를 시작할 수 있다.

## 수정할 파일

- `unreal/BeyondPhysics/Source/BeyondPhysics/Physics/FogHighway.h`: μ0.8, 목표100km/h, 앞차 위치, 자동 가속 정책, 차체 충돌 판정.
- `unreal/BeyondPhysics/Source/BeyondPhysics/Experiment/FogTrial.h`: 기준 통과·첫 제동 시각·거리·종료 결과.
- `unreal/BeyondPhysics/Source/BeyondPhysics/Environment/FogHighwayScene.cpp` 및 `.h`: 도로 타일, 차선·벽·가로등·앞차, 안개 반경/밀도·색. 거리 단위는 코드 주석을 확인한다. 이미 저장된 액터 속성은 에디터에서 함께 바꿔야 한다.
- `unreal/BeyondPhysics/Scripts/build_fog_highway.py`: 새 맵 생성·재질·조명·노출. 맵이 있으면 기본적으로 덮어쓰지 않는다. 재생성은 `FOG_REFRESH=True`를 명시한다.
- Content/Driving/Fog: 이 맵 전용 재질. 전용 M_FogSurface와 MI_Fog*는 인스턴싱 셰이더 지원을 명시해 저장했다. configure_fog_materials.py로 연결과 플래그를 복구할 수 있다. 기존 공용 재질을 수정하면 다른 맵에도 영향을 줄 수 있으므로 복사본을 사용한다.
- `Driving/DrivingPawn.cpp`, `UI/DrivingHUD.cpp`: 맵 전환·SDK 적용·결과 저장과 화면 표시.

별도 차량 방정식은 기존 `Physics/VehicleModel.cpp`를 그대로 사용한다. 마찰계수는 구동·제동·횡력의 접지 한계에 들어가며, 외형 재질의 Roughness와 다른 값이다.

C++ 수정은 에디터를 닫고 Editor/Android를 다시 빌드한다. 콘텐츠까지 바꿨으면 `Scripts/package_pleos_emulator.ps1 -Scene Drive`로 전체 cook한다. 콘텐츠가 그대로인 C++ 변경만 `-SkipCook`을 사용할 수 있다. `Scripts/run_pleos_emulator.ps1 -Scene Drive`는 빌드된 APK를 설치·실행한다.

## 근거와 범위

[NHTSA DOT HS809561](https://www.nhtsa.gov/sites/nhtsa.gov/files/nhtsalvabs5.2-5.3final.pdf) §3.3.8의 시험 건조 아스팔트 peak약0.9/sliding약0.8 중 0.8을 유효 마찰 한계의 대표값으로 선택했다. 타이어·온도·노면마다 달라지는 실측 곡선이나 현대차 보정값은 아니다. 보고서 시험속도72km/h를 근거로100km/h 실차 정밀도를 주장하지 않는다. ABS·하중 이동·휠 잠김·공력은 현재 모델에 없다.

안개는 [Epic Local Fog Volumes](https://dev.epicgames.com/documentation/unreal-engine/local-fog-volumes-in-unreal-engine)의 거리 적분 표현이다. 모바일에서는 영화 수준의 다중 산란·체적 그림자를 제공하지 않는다. 하늘은 구름 텍스처 배경이다. 100m 기준은 [Met Office의 기상 시정 설명](https://weather.metoffice.gov.uk/guides/observations/how-we-measure-visibility)을 참고한 게임 측정 기준이며, 실제 사람의 발견거리나 화면에서 계측한 기상 시정이 아니다.

시도 종료 시 `Saved/FogTrials/fog_*.csv`와 같은 이름의 `.summary.csv`를 저장한다. summary에는 정확한 기준/제동 시각·속도·거리·결과·미리 제동/제동 해제 여부·μ가 있다. Android 경로는 앱 전용 저장소이며 Windows의 프로젝트 Saved 폴더와 다르다.


