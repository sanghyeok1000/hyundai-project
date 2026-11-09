# 공개 파일 구성

## 프로젝트 실행과 검토에 사용하는 파일

| 위치 | 포함 내용 |
|---|---|
|`README.md`|기획 의도, 기능, 설치·조작, 구현 구조|
|`android/pleosbridge/`|Vehicle SDK 브리지와 단위 검사|
|`android/gradle/`, `gradlew*`, Gradle 설정|브리지 빌드 도구와 의존성 선언|
|`unreal/BeyondPhysics/BeyondPhysics.uproject`|Unreal 프로젝트 정의|
|`Source/`|차량 동역학·입력·감각 효과·HUD·시나리오 코드|
|`Config/`|Android, 렌더링, 입력, 맵 설정|
|`Build/Android/Pleos_UPL.xml`|AAR·Manifest·JNI 연결|
|`Content/`|맵·차량·도로·산·조명·재질·텍스처|
|`SourceArt/`|프로젝트 생성 메시, 도로 샘플, 텍스처 원본과 출처|
|`Scripts/`|맵·재질 생성 및 Release 패키징 도구|
|`Tests/`|물리 모델과 정책 검사|
|`docs/`|실행·빌드·모델·API·검증 문서와 실행 화면|
|`THIRD_PARTY_NOTICES.md`, `RIGHTS.md`|외부 자산·의존성 출처와 권리|

Unreal 하위 경로는 `unreal/BeyondPhysics/`를 기준으로 한다. `.umap`, `.uasset`은 프로젝트를 구성하는 원본 자산이다. `.obj`는 `SourceArt/`의 메시 원본이며 C++ 컴파일 중간 파일과 구분한다. Gradle Wrapper JAR도 브리지 빌드에 사용한다.

## 배포 파일

APK·체크섬·릴리스 노트는 [Releases](https://github.com/sanghyeok1000/hyundai-project/releases)에 둔다. SDK·JDK·NDK·Unreal Engine은 각 제공자의 설치 절차를 사용한다.

## 저장소에서 제외하는 파일

- 개인 서명 키와 비밀번호, API 토큰, `.env`, 로컬 SDK 경로.
- 개인 개발 메모, 대화 기록, 에이전트 설정, 접수용 개인정보·서명 서류.
- `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `.gradle/`, IDE 캐시.
- APK·AAR·디버그 심볼·빌드 로그·백업.
- Unreal Engine·편집 플러그인 바이너리와 외부 SDK 추출 소스.

기본 제외 규칙은 [`.gitignore`](../.gitignore)에 있다. 자산의 이용 조건은 [출처 문서](../THIRD_PARTY_NOTICES.md)를 따른다.
