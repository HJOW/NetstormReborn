# cpppj 아군 AI 자동 플레이 테스트 — AI 에이전트용 사용 안내

작성 2026-11-14(HJOW-Athlon). 대상: 이 저장소에서 작업하는 AI 에이전트(Claude Code·Codex 등)와 개발자. 구현·검증 근거는 [cpp-campaign-test-play-20261114.md](../exe/cpp-campaign-test-play-20261114.md)에 있다.

## 0. 이 방식이 무엇인가

캠페인/스커미시 플레이는 난수·AI 판단에 좌우되어 사람이 같은 결과를 재현하기 어렵다. cpppj `Netstorm.exe`에는 **테스트 전용 명령줄 옵션**이 있어 **로컬 플레이어(아군)에도 게임 내장 AI를 붙여 자동으로 플레이**시키고, 아군/적 AI 난이도와 시작 SP(스톰 파워)를 조절할 수 있다. **관전 카메라**를 켜면 화면이 주요 전투 위치, 사제 희생 의식 중에는 제단으로 자동 이동한다(4.1절). 실행 도구 `tools/cpp_campaign_autoplay.py`가 게임을 띄우고 안내 창만 닫아 주며, 주기적으로 상태를 기록해 승패를 요약한다.

- **일반 사용자 실행에서는 켤 수 없다.** 무인자 `Netstorm.exe`(배포본 기본 실행)·게임 메뉴·`options.cfg`에는 없고, `--run`/`--mcp` 뒤 명령줄 인자로만 켜진다.
- 적용 대상: **캠페인/일반 미션, 스커미시.** 편집 Test Battle·데모·LAN/멀티플레이에는 적용되지 않는다.
- cpppj 전용이다. dotnetpj/webpj에는 이 옵션이 없다. 다만 cpppj 자동 플레이 결과(`session-dump`·화면 캡처)는 다른 프로젝트의 동작 비교 자료로 쓸 수 있다.

## 1. 실행 전 확인 (반드시)

1. **AGENTS.md 실행 확인 규칙**: 게임 창이 뜨는 작업이다. 현재 호스트가 AGENTS.md의 예외 시스템(`vm-debian-codex`·`VM-W11-Codex`·`HJOW-Athlon`)이 아니면 **실행 목적과 필요성을 설명하고 사용자 확인을 받은 뒤** 실행한다. 호스트명은 `hostname`으로 확인한다.
2. **빌드 확인**: `cpppj/build/bin/Release/Netstorm.exe`가 현재 소스로 빌드돼 있어야 한다. 필요하면 LEFT_JOBS.md 6절의 빌드 명령(`cmake --build cpppj/build --config Release --target NetstormCpp ...`)을 PowerShell로 실행한다. 빌드 중에는 제품 소스를 수정하지 않는다.
3. **게임 자료**: 기본값 `assets/game-data`(`--game`으로 변경). originals/originalCD는 보호 대상이므로 출력/사용자 데이터 위치로 쓰지 않는다.
4. **사용자 저장 격리**: 도구는 출력 폴더의 `user-data`를 `NETSTORM_USER_DATA`로 지정해 실제 `%USERPROFILE%\Saved Games\Netstorm`을 바꾸지 않는다. 도구 없이 직접 실행할 때도 반드시 빈 폴더를 `NETSTORM_USER_DATA`로 지정한다.
5. 버튼 이름이 영어 기준이므로 `--language english`(기본값)를 유지한다.

## 2. 빠른 사용법

```powershell
# 캠페인 1-1: 아군 어려움·적 쉬움(기본값), 게임 시간 600초
python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins

# 승리 재현을 쉽게: 로컬 시작 SP 20000, 적 시작 SP 1000, 게임 시간 30분, 12000프레임마다 화면 캡처
python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins --start-sp 20000 --enemy-start-sp 1000 --frames 72000 --snapshot-every 12000

# 스커미시: AI 2명(소유자 2는 적, 소유자 3은 플레이어 편), 게임 시간 30분
python -X utf8 tools/cpp_campaign_autoplay.py --skirmish --skirmish-ai-count 2 --skirmish-allies 3 --frames 72000

# 아군 AI 없이 사람(또는 MCP 입력)이 플레이할 때 시작 SP만 바꾸기
python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins --ally-ai none --enemy-ai option --start-sp 50000
```

- 실행 시간: 고정 25ms/프레임에서 HJOW-Athlon 기준 약 100프레임/초(24000프레임 ≈ 4분, 72000프레임 ≈ 12분). Bash/PowerShell 도구의 제한 시간을 넘기면 **백그라운드 실행**으로 돌리고 완료 알림을 기다린다.
- 미션 키 목록: 캠페인 메뉴 파일(`offical*.english`의 `MissionBegin,<키>`)과 [미션 대응 문서](../gameplay/demo-campaign-map.md)를 참고한다. 예: `thewarbegins`(1-1), `dissolvedalliance`(1-6), `guardmyback`(2-4).

### 2.1 도구 인자

| 인자 | 기본값 | 뜻 |
| --- | --- | --- |
| `--mission <키>` / `--skirmish` | (둘 중 하나 필수) | 캠페인/일반 미션 키, 또는 메인 메뉴에서 스커미시 시작 |
| `--ally-ai` | `hard` | 아군(로컬 AI·동맹 AI) 난이도. `none`이면 로컬 AI를 붙이지 않는다 |
| `--enemy-ai` | `easy` | 적 AI 난이도. `option`이면 옵션 메뉴 값(스커미시는 스커미시 화면 값) |
| `--start-sp` / `--enemy-start-sp` | 미션 값 | 로컬 시작 SP / 적 AI 시작 SP(0..10000000). 동맹 AI 자금은 미션 값 유지 |
| `--frames` | 24000 | 총 프레임. 끝나면 게임이 종료된다 |
| `--frame-ms` / `--unix-seconds` | 25 / 0 | 고정 가상 시계(0이면 실제 OS 시계) / 가상 유닉스 초 |
| `--dump-every` / `--snapshot-every` / `--click-every` | 2000 / 0 / 20 | 덤프·보고 주기 / 화면 캡처 주기(0=끔) / 안내 버튼 클릭 주기 |
| `--skirmish-ai-count` | 1 | 스커미시 AI 수 1..7(소유자 2부터) |
| `--skirmish-allies` | 없음 | 플레이어 편 AI 소유자 목록(예: `3,4`) |
| `--skirmish-ai-diff` | 모두 보통 | 스커미시 화면 AI별 난이도(예: `2=hard,3=easy`). `--ally-ai`/`--enemy-ai`가 지정되면 그쪽이 우선 |
| `--skirmish-bans` | 없음 | 금지 유닛 TYPE 이름 쉼표 목록(최대 5개) |
| `--no-follow` | (관전 카메라 켬) | 관전 카메라(주요 전투 위치·희생 의식 중 제단으로 화면 이동)를 끈다 |
| `--out` | `extracted/cpp-campaign-autoplay/<대상>-<시각>` | 출력 폴더(`extracted/`는 git 제외) |

## 3. 결과 읽는 법

도구는 끝에 요약을 출력하고 출력 폴더에 `summary.json`을 남긴다.

| 항목 | 뜻 / 확인 기준 |
| --- | --- |
| `exit_code` | 0이어야 한다. 0이 아니면 `console.txt`와 `user-data/logs/*.log`(7차 오류 로그)를 본다. |
| `local_ai_attached` | `--ally-ai`를 쓴 경우 true여야 한다(콘솔 `테스트 자동 플레이: 로컬 owner N AI 생성 완료`). |
| `ai_diagnostics` | 콘솔의 `AI 원본 진단` 횟수. 0이 정상이며 아니면 원문 불변식 위반 후보로 기록한다. |
| `camera_moves` | 관전 카메라 이동 횟수(`battle`/`ritual`). `ritual`이 1 이상이면 그 실행에서 사제 희생 의식이 있었다는 뜻이다. |
| `dumps[]` | 프레임별 `ai_owners`(AI가 붙은 소유자), `objects_by_owner`(소유자별 객체 수: 섬·다리·건물·유닛 포함), `sp`(소유자별 SP). 아군 AI가 일하면 로컬 소유자 객체 수가 늘고 SP가 오르내린다. |
| `reports[]` | 프레임별 `page`(현재 안내/화면 이름), `paused`, `mission_progress`(`Done<미션>` 설정 값) |
| `outcome` / `outcome_frame` | `victory`: `Done<미션>`≠0 또는 page가 `BadTeamDead`/`Succeeded`/`Success`. `defeat`: page가 `GoodTeamDead`/`Failed`. 그 외 `undecided`. 프레임은 처음 관측된 보고 시점(덤프 주기 단위)이다. |

- 승리/패배 창의 버튼(`Next Mission`·`Leave Missions`·`Leave`)은 누르지 않으므로 결과 이후에는 그 창에서 멈춘 상태가 기록된다.
- 객체 TYPE 번호는 `Netstorm.exe --dump-types assets\game-data`로 이름(예: 82=bridge, 72=sunAviary)을 찾는다. `dump-*.tsv`의 `object` 행(SID·TYPE·상태·소유자·프레임·x·y·HP) 형식은 [session-dump 문서](../exe/cpp-session-state-dump-20261109.md) 2절을 따른다.
- 화면 캡처(`snap-*.bmp`)는 PNG로 바꿔 보면 편하다(PowerShell `System.Drawing`).

## 4. 도구 없이 직접 실행하기 (MCP·다른 스크립트에서)

```powershell
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj\build\bin\Release\Netstorm.exe --run assets\game-data --mission thewarbegins --window --test-autoplay --test-start-sp 30000 --test-enemy-start-sp 2000
# MCP(4차): Netstorm.exe --mcp --test-autoplay ...  처럼 같은 옵션을 뒤에 붙인다.
```

| 게임 옵션 | 뜻 |
| --- | --- |
| `--test-autoplay` | 지정하지 않은 아군 AI=어려움, 적 AI=쉬움으로 채우는 프리셋 |
| `--test-ally-ai <easy\|normal\|hard>` | 로컬 플레이어에 AI 부착 + 아군(로컬·동맹 AI) 난이도 |
| `--test-enemy-ai <easy\|normal\|hard>` | 적 AI 난이도(옵션 메뉴·스커미시 화면 값보다 우선) |
| `--test-start-sp <N>` / `--test-enemy-start-sp <N>` | 로컬 / 적 시작 SP(0..10000000) |
| `--test-follow-battle` | 관전 카메라. `--test-autoplay` 프리셋에 포함된다 |

- 시작 시 stderr에 `테스트 전용 캠페인 자동 플레이 활성: ...`이 찍힌다. 잘못된 값이면 시작 단계에서 오류로 끝난다.
- 결정론 재현이 필요하면 `--fixed-frame-ms 25`를 함께 쓴다(같은 인자·같은 가상 시계). 2026-11-14 확인: 같은 인자 두 실행은 난수·풀 해시까지 같았다. **단 화면 위치가 공용 난수 소비에 영향을 주므로 관전 카메라를 켠 실행과 끈 실행, `--set` 내용이 다른 실행은 서로 전개가 달라진다.** 비교할 실행끼리는 모든 인자를 맞춘다.
- 스커미시는 `--mission`으로 시작할 수 없다. 메뉴 `Campaign → Skirmish → Start`를 누르거나(도구의 `--skirmish`), `--set "skirmishAiCount=2;skirmishAi3Ally=1;..."`로 화면 설정을 미리 넣는다.

### 4.1 관전 카메라 (사람이 지켜볼 때)

`--test-follow-battle`(도구는 기본 켬)을 켜면 카메라가 자동으로 움직인다. 게임 상태는 읽기만 하며 게임 계산·난수·입력에는 관여하지 않는다.

- **전투 위치**: 0.5초(게임 시각)마다 소유 객체의 HP 감소와 사라진 객체(파괴, 하나당 최대 60)를 모아, 최근 6초 피해 합이 가장 큰 곳(반경 8칸)의 가중 중심으로 옮긴다. 피해 합 20 미만은 무시하고, 한 번 옮기면 4초 동안 유지하며, 6칸 안의 같은 전투로는 다시 옮기지 않는다. 섬·다리(붕괴로 HP가 줄어듦)와 중립 객체는 제외한다.
- **사제 희생 의식**: 제단에서 의식 중에만 나는 사슬/몸부림 소리(`chainRattle23~27.wav`, `priestStruggleFade01~03.wav`)의 재생 위치를 그대로 받아 **즉시 제단으로** 옮기고, 마지막 소리 뒤 5초 동안은 제단을 유지한다. 의식이 끝나면 다시 전투를 따라간다.
- 이동할 때마다 콘솔(stderr)에 `테스트 관전 카메라: 전투|희생 의식 제단 (x, y) 피해합 W @ 시각`이 남고, 도구 요약의 `camera_moves`가 횟수를 센다.
- **실제 속도로 보기**: 도구 기본값(`--frame-ms 25` 고정 시각)은 가능한 한 빨리 진행해 사람 눈에는 빨리 감기처럼 보인다. 지켜보려면 `--frame-ms 0`(실제 OS 시계)을 쓰거나, 도구 없이 직접 실행한다(브리핑의 `Play Mission`은 직접 누른다).

```powershell
python -X utf8 tools/cpp_campaign_autoplay.py --mission thewarbegins --frame-ms 0 --frames 200000 --start-sp 20000 --enemy-start-sp 1000
# 또는 직접 실행
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj\build\bin\Release\Netstorm.exe --run assets\game-data --mission thewarbegins --window --test-autoplay --test-start-sp 20000 --test-enemy-start-sp 1000
```

## 5. 기록·보고 규칙

- 이 방식의 결과는 **"테스트 자동 플레이(아군 AI)" 결과**로 구분해 기록한다. 사람 플레이나 원본 게임과의 동일성 검증을 대신하지 않는다.
- 인계 문서에는 사용한 인자(미션/스커미시 설정·난이도·SP·프레임·고정 시각), `exit_code`, `outcome`/`outcome_frame`, 오류 로그·AI 원본 진단 수, 출력 폴더를 남긴다.
- 출력 폴더(`extracted/`)는 커밋하지 않는다. originals/originalCD·AGENTS.md는 수정하지 않는다.
- 이 안내서나 도구를 바꾸면 [구현 문서](../exe/cpp-campaign-test-play-20261114.md)와 LEFT_JOBS.md의 관련 행도 함께 갱신한다.

## 6. 문제 해결

| 증상 | 확인 |
| --- | --- |
| `local_ai_attached`가 false | 콘솔에 `AI 생성 생략`이 있으면 로컬 사제가 없거나 이미 AI가 있는 경우다. 편집 Test Battle·데모는 적용 대상이 아니다. |
| 미션이 시작되지 않음(page가 계속 `main`/`loading`) | `--mission` 키 철자, `assets/game-data` 경로, 스커미시는 메뉴 버튼 이름(영어)·`--language english`를 확인한다. `report.tsv`의 `page`/`popup`을 본다. |
| 승패가 `undecided` | 시간을 늘리거나(`--frames`), `--start-sp`를 올리고 `--enemy-start-sp`를 낮춘다. 미션마다 승리 조건(사제 포획/희생, 적 전멸 등)이 다르다. |
| 실행이 도구 제한 시간 안에 끝나지 않음 | 백그라운드로 실행하고 완료를 기다린다. `--timeout`(초)을 늘린다. |
| 실제 사용자 저장이 바뀐 것 같음 | 도구는 항상 `NETSTORM_USER_DATA`를 출력 폴더로 지정한다. 직접 실행했다면 환경 변수를 확인한다. |
