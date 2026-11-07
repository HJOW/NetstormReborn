# webpj 튜토리얼/캠페인 미션 Init(TutorialInit) 이식 — WB04-E38

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른여덟 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/TutorialInit`(10.78, 원문 004c3290)를 옮겼다. [E37](webpj-pool-reset-battle-options-20261108.md)의 `resetMissionBattleOptions`를 `resetOptions` 훅의 실제 몸체로 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/tutorial-init.js` — `initializeTutorialMission(state, flags, demo, globals, hooks)`

1. `clearTemplates()` → `tutorialNumber = atol(get('tutorialNumber','0'))`(DWORD)이고 0이 아니면 `tutorialSeen = 1` → `missionNumber = atol(get('missionNumber','0'))` → `get('demo', null)`의 첫 NUL 앞이 비어 있지 않을 때만 `demo = atol` → `resetOptions()`.
2. 큰 효과 뒤의 **현재** `state.name`을 첫 NUL 앞까지 읽는다(80바이트 이상이면 원본 오버런이라 `RangeError`). 지역 80바이트(`scratch`)에 이름+NUL을 쓰고, ASCII 소문자 이름을 NUL 없이 덮어쓴 뒤 첫 글자가 a..z면 대문자로 만든다(원본 C locale 첫 글자 정규화; `state.name` 자체는 불변).
3. `namePrefix` = scratch 첫 4바이트의 리틀엔디언 DWORD(짧은 이름의 NUL 뒤 caller 입력 꼬리를 지우지 않음) → `set('fileName', scratch 이름 길이만큼)` → `loginSubscriber = 현재 initialSubscriber()` → `fort = get('loadFort', 현재 state.name)`의 첫 NUL 앞(256바이트 이상 `RangeError`) → `requestFort(fort)` → `header()` → `flags |= 1`.

도달한 효과를 빈 성공으로 대체하지 않는다(누락은 거부, 앞선 효과는 보존). 현재 구독자는 getter, flags/demo/전역은 `{ value }` 보관 객체이다.

## 2. 검증 (`tests/tutorial-init.test.mjs`, 2건)

- `tutorial-init-x86.tsv` **152행**: 입력 JSON(`\u00xx`는 한 원본 바이트)로 장면을 만들고 전체 필드(flags/demo/이름 접두/튜토리얼·미션 번호/seen/구독자), 지역 80바이트, 원래 이름, **옵션 87바이트**(`resetMissionBattleOptions`), 큰 효과 호출 순서(`C`/`G,키,대체,결과`/`O`/`S`/`L`/`H`)를 원문 열과 전수 비교. 효과가 현재 이름·구독자·flags를 바꾸는 변형 4종(clear 뒤 이름 변경, 옵션 뒤 이름+구독자, set 뒤 이름+구독자, 로드 요청/Header가 flags 변경)을 그대로 재생.
- 계약: 이름 80바이트·로드 256바이트 초과 거부(초과 시점까지의 효과 `C`,`O`[,`S`] 보존), 정상 경로의 첫 글자 대문자화·구독자 복사·`flags|=1`·효과 순서, Header 효과 누락 거부(앞선 4효과 보존).

최초 실패와 교정: 2건이 처음부터 통과했다.

## 3. 경계

- 실제 보호 Script 11개(`ProtectedElevenBeforeQueuedLoad`)와 `State` 큐의 다음 루프 로드(`MissionScript`·`MissionFortRequest`·`MissionTechnology`·`CaptureMissionFortZone`)는 해당 단위를 옮길 때 대조한다. `State`/`DialogAction` 큐(`PendingRequestReleasedWithOwner`)는 호스트 전환 큐라 원본 대응이 없다.
- 설정 조회(`get`/`set`)와 템플릿 정리·기술 Header는 훅으로만 주입했다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`HtmlInputDialog`(Gump 생성 몸체 필요)·`DialogCommandLifetime`·`LanguageOptions`·`TraceChat`(Chat 노드 필요). 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
