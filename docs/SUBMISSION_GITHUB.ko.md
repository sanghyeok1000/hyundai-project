# GitHub 저장소 만들기와 제출 파일 올리기

이 안내는 **사용자가 직접** 업로드하는 절차다. Codex는 저장소 생성·커밋·업로드를 하지 않는다.

## 1. 올릴 폴더와 파일

- 소스 저장소: `submission/github-ready/` **폴더 안의 내용 전체**. 이 폴더가 Git 저장소의 루트가 된다.
- Release 첨부: `submission/release/BEYOND_PHYSICS_release.apk`, `SHA256SUMS.txt`, `RELEASE_NOTES.ko.md`.
- 올리지 않는 것: 원래 `hyundai/` 폴더 전체, `private/`, `builds/`, 서명 키/비밀번호, `local.properties`, `Intermediate/`, `Saved/`, `.gradle/`, 외부 SDK 추출 소스.

APK는 Git에 추가하지 않는다. GitHub는 일반 Git 파일 100MiB 초과를 차단하므로 큰 APK는 **Releases**에 첨부한다. 이번 소스 폴더의 개별 파일은 이 제한보다 작고 Git LFS 없이 올릴 수 있다. 앞으로 큰 자산을 추가하면 별도로 LFS를 설정한다.

## 2. 웹에서 빈 저장소 만들기

1. GitHub 로그인 → 오른쪽 위 `+` → `New repository`.
2. 이름 예: **beyond-physics**. 설명: `Pleos Connect 기반 물리 주행 체험 시뮬레이터`.
3. 공개 가능한 제출 소스는 Public으로 설정하면 심사자가 쉽게 접근할 수 있다. Private을 선택한다면 공모전에서 안내한 심사 계정을 직접 초대하고 접근을 확인한다.
4. **Add a README / Add .gitignore / Choose a license는 선택하지 않는다.** 준비한 파일과 충돌하지 않도록 빈 저장소로 만든다. 별도 오픈소스 라이선스를 임의로 붙이지 않는다.
5. `Create repository` 후 HTTPS 주소를 복사한다. 형식: `https://github.com/본인아이디/beyond-physics.git`.

## 3. PowerShell에서 소스 올리기

Windows에 Git이 없다면 [Git for Windows](https://git-scm.com/download/win)를 먼저 설치하고 PowerShell을 다시 연다. 아래 명령은 **직접 실행**한다.

```powershell
Set-Location 'C:\Users\parks\Desktop\hyundai\submission\github-ready'
git --version
git init
git branch -M main
git status --short
git add .
git status --short
git commit -m "Prepare BEYOND PHYSICS competition prototype"
git remote add origin https://github.com/본인아이디/beyond-physics.git
git push -u origin main
```

주소의 `본인아이디`를 실제 값으로 바꾼다. 처음 커밋할 때 신원을 요구하면 이 저장소에만 다음을 설정한 뒤 `git commit`부터 다시 한다. 공개를 원치 않으면 GitHub Settings → Emails의 **noreply** 이메일을 사용한다.

```powershell
git config user.name "본인 표시 이름"
git config user.email "본인의 GitHub 이메일 또는 noreply 이메일"
```

로그인 창이 뜨면 본인 GitHub 계정으로 인증한다. GitHub 비밀번호나 토큰을 README나 코드에 적지 않는다.

## 4. Release에 APK 첨부

1. 저장소 웹페이지 → `Releases` → `Create a new release`.
2. Tag는 **v0.1.0-prototype**, Target은 **main**, 제목은 `BEYOND PHYSICS Pleos Prototype v0.1.0`.
3. `submission/release/RELEASE_NOTES.ko.md`의 내용을 설명에 붙인다.
4. 첨부 영역에 APK, SHA256SUMS.txt, RELEASE_NOTES.ko.md를 넣는다. **Source code ZIP만으로는 APK 제출이 되지 않는다.**
5. 공모전 파일명은 `팀명_앱이름_release.apk` 형식이다. 팀명이 정해지면 APK를 그 이름으로 바꾸고 SHA256SUMS.txt의 **파일명 부분만** 함께 바꾼다. 파일 내용을 바꾸지 않으므로 해시는 같다.
6. `Set as a pre-release`로 프로토타입임을 표시하고 `Publish release`. 업로드 완료 후 첨부 APK가 실제 다운로드되는지 확인한다.
7. README의 APK 경로를 본인 Release URL로 바꾸고, 필요하면 아래처럼 다시 커밋한다.

```powershell
git add README.md apk/README.md
git commit -m "Add downloadable prototype release link"
git push
```

Release 주소 형식: `https://github.com/본인아이디/beyond-physics/releases/tag/v0.1.0-prototype`.

## 5. 공모전 접수에 전달할 것

저장소 URL과 위 **Release 페이지 URL**을 접수양식에 적는다. GitHub 공개만으로 접수가 완료되는 것은 아니다. 공식 접수양식·서명/동의서·제출 방법 및 기한은 [공식 제출 안내](https://appdevchallenge.auto-hmg.io/guide/submission)를 최종 확인한다. 기획서와 시연 영상은 이 작업에서 작성하거나 제출하지 않았다.

## 6. 이후 수정할 때

원래 개발 폴더에서 Unreal C++/맵/Kotlin을 수정하고 Release 스크립트로 다시 빌드한다. 공개 사본은 자동 동기화되지 않는다. `tools/prepare_submission.py`로 내보낼 때 기존 제출 폴더에 덮어쓰지 않도록 새 목적지를 지정한 후 필요한 소스 변경을 저장소에 반영한다. 이미 업로드한 Git 저장소의 `.git/`을 지우거나, APK를 Git에 넣거나, 다른 키로 매번 서명하지 않는다. 업데이트 앱을 만들려면 **동일한 개인 서명 키와 비밀번호**를 보관해야 한다.

참고: [GitHub 대용량 파일](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github), [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases).
