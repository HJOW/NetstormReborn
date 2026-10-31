# EXE 기본 GUI 진입과 휴대 배포 경로 — 단위637

Netstorm.exe를 인자 없이 실행하면 게임 GUI를 시작하도록 변경했다. 이전에는 빌드 정보만 출력하고 배치 파일이 `--run game`을 붙여 실행했다. `--help`와 명시적 `--run`·분석 명령은 유지한다. 전체 목표와 원본 동일성은 미완료다.

현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon/2026-10-23이다. 원본/CD·정밀 산출물을 변경하지 않았다. 원시 경로는 `extracted/cpp-portable-startup-637-20261031/`다.

## 변경과 경계

`CurrentExecutableFile`은 Win32 W API로 실제 EXE의 UTF-16 절대 경로를 얻는다. 기본 시작은 그 옆의 `game/netstorm.tarc`를 우선하고, 원본식 EXE 옆 자료를 다음으로 찾는다. 실행 시 cwd의 다른 게임이나 저장소의 보호 원본을 자동 탐색하지 않는다. 자료가 없으면 오류를 표시하며 임의 자료/설정 폴더를 만들지 않는다.

탐색기가 이 프로세스만을 위해 만든 콘솔은 분리하고 이미 사용자가 연 터미널은 유지한다. 무인자 시작 실패는 콘솔이 없어도 오류창에 표시한다. `game/` 배치·Unicode·콘솔/오류창 처리는 원본과 다른 Windows10/11 휴대배포 어댑터임을 한국어 주석에 적었다. 실제 GUI 입력/게임 규칙은 기존 RunClient와 동일하다. 배포 안내는 최상위 EXE 직접 실행을 기본으로 바꾸고 창 모드 강제/추가 인자를 위한 두 배치를 유지했다.

## 검증과 확인하지 못한 부분

Release 제품/검사 빌드 종료0·경고/오류0, 집중19/실패0·등록3554다. 새2검사는 실제 한글/보충 평면 문자 임시 경로와 다른 cwd 자료·두 배치 우선순위·누락·archive 이름의 디렉토리·원본식 옆 자료를 확인한다. 기존 StartupStateCheck/ClientOptionsFile/ScreenPresentation/CampaignViewCommands를 함께 검사했다.

같은 새 ZIP을 한글과 이모지 경로에 풀어 Computer Use로 **인자 없이 EXE를 실행**했다. 실제 반환된 창 제목 `NetStorm Main Menu`·창ID787254와 PID19464/정확한 경로·인자 없는 CommandLine을 보존했다. 이 제목만으로 안내가 닫힌 메인 메뉴 본문/시각을 확인했다고 주장하지 않는다.

물리 화면 캡처는 `FrameArrived timed out: timed out waiting on channel`, 창 재선택/한 번 재시도 후 `window capture timed out: timed out waiting on channel`로 실패했다. computer-use SKILL.md와 guidance.md에 따라 이 오류를 알리고 관측 없는 입력을 보내지 않았다. **물리 입력0건이며 QWASZX·ESC·Home/포커스 검증은 미완료**다. 직접 실행 프로세스는 정확히 검증한 임시 EXE/PID만 비UI 방식으로 정리했고 정상 GUI 종료로 세지 않는다. 스킬은 `C:/Users/BUZZ/.codex/plugins/cache/openai-bundled/computer-use/26.1027.30824/skills/computer-use/SKILL.md`다.

별도 명시적 CLI 사본 실행은 실제 초기 안내→메인→Quit/정상 종료0, 같은 표시 프레임의 논리 버퍼와 실제 창DC 전체RGB 일치다. --help도 종료0/정보 출력을 유지한다. 이 앱 내 검사 입력을 물리 입력으로 합산하지 않는다. 최초 CLI 계획은 UserNotUpdated 안내를 빠뜨려 Quit를 찾지 못한 종료1을 보존했고, v2에서 실제 안내만 닫았다. v2 게임은 한 번만 실행했으며 분석 출력 폴더 누락은 같은 원시로 재분석했다. 최초 추출 준비의 cp949/이모지 print 오류도 모든 파일 감사·설정·경로 기록 뒤의 호스트 출력 오류이며 재추출/게임 오류로 세지 않는다.

## 보존과 후속

실행 뒤 원천31개·보호2629개/fixture770개가 불변이다. 압축1376항목/1358자산의 실행 뒤 사본·원본/CD 바이트가 같고 게임/디코더는 현재 빌드, VC 배포 런타임은 검증된635 동봉본과 같다. EXE `3923635e953752cbe09e4bfa9158ba401b5d73195e3157cf96f3740a12b2ed33`, ZIP `624ee85b91ca298d940792c35380177dc5a0f164b6e1668a8fbe138d45beb0f8`이다. [완결 집계](../../cpppj/portable-startup-runtime-20261031.json)를 따른다. 다음 보호 기준은 같은 원시의 protected-after-current.json2629개다.

최신 전체34는635제품을 검사한636의34/34·실패/재시도0이며637 제품으로 다시 실행하지 않았다. 물리 검증은 Windows Graphics Capture 도우미 정상화 뒤에 재개한다. 그동안 남은 비MP UI/Edit·시나리오/파일/수명 연결을 원문과 계속 대조하고 MP 무시험/4~7차를 잇는다. 금지 재현/공중3종 전용/신규MP 검사는 없고 Windows11·PCM·DPI/포커스·기존VFX/612스택미정값·자연완주/전체동일성 한계는 남는다.
