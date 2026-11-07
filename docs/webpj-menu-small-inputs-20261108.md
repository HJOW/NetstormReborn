# webpj 소형 일반 입력·메뉴 부모 여섯 개 이식 — WB04-E33

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른세 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/CreateAtInput`·`client/DetonateInput`·`client/MoveDialog`·`client/MovieTimer`·`client/IntroMenu`·`client/MainMenuActivity`(10.78)를 옮겼다. 모두 짧은 훅 스타일 분배/상태 갱신이다(도달한 장치만 요구하고 앞선 부분 효과는 되돌리지 않음). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

| 모듈 | 원문 | 내용 |
| --- | --- | --- |
| `www/core/create-at-input.js` | 004d84f8..851b | `createAtScriptCode`(첫 NUL 앞 이름, ASCII 대소문자 무시, `0x020000e0`), `dispatchCreateAtInput(event, editor, mission, create)`: 뗌이거나 편집기도 미션도 아니면 효과 없이 true, 아니면 y 전체 DWORD(문자열 토큰)를 현재 런타임 생성 부모에 전달. 부모 실패는 전파 |
| `www/core/detonate-input.js` | 4d6eca..4da477 | `dispatchDetonateInput`: DetonateBomb(`0x03000074`)의 대상은 y 하위 WORD(0이면 소비만), 0이 아니면 `objectInput(sid, 사건 코드 전체)`, 반환 무시 |
| `www/core/move-dialog.js` | 004c8df0 | `moveDialog(current, where, screenWidth, screenHeight, hooks)`: 캡처한 노드에 `push()` 뒤 b/t/r/l 순서로 y/x를 float로 저장(b보다 t, r보다 l 우선, m/c 무변경), Push 뒤의 화면 크기를 getter로 읽고 `attach(x, y, 0, 0)` |
| `www/core/movie-timer.js` | SDK 00403000..00403137 | `waitMovieTimer(state, hooks)`: fast/ready면 시계 비접근, 밀리초×100(32비트 감김) 기준 최초 기한·대기(yield)·late 재설정·간격 0 처리 |
| `www/core/intro-menu.js` | 00435de0, 004d8xxx→004b3520 | `detectIntroMovie(holder, path, probe)`(선행 0 쓰기·빈 경로는 파일 장치 비요구·출처 번호는 1로 정규화), `dispatchPlayIntroInput`(뗌은 소비만, 그 밖에는 State `next[0]=39`·`next[35]=1`만) |
| `www/core/main-menu-activity.js` | 004cee80, 004cebf0 | `idleMainMenuActivity`(autoDemo 꺼짐이면 끝, HTML 양수면 팝업 비조회, 기한·포인터 사본 갱신 뒤 같은 프레임에 만료 검사 → `DemoStormRages` 큐 후 Kill), `handleMainMenuActivity`(활동 사건은 뗌/autoDemo와 무관하게 기한 = 벽시각+45) |

## 2. 검증 (`tests/menu-small-inputs.test.mjs`, 11건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음) 2540행:

- `create-at-input-x86.tsv` 408행(Guide/Gump 선행 소비 제외, 편집기/미션 모드, 정지/Console 독립), `create-at-names-x86.tsv` 15행(이름→번호·첫 NUL만).
- `move-dialog-x86.tsv` 73행(문자열·화면·창 치수·Push가 바꾸는 현재 값 → 노드 float 비트, Push/Attach 순서), `movie-timer-x86.tsv` 14행, `intro-menu-x86.tsv` 84행(경로 첫 NUL·출처·초기값·조회 횟수), `main-menu-activity-x86.tsv` 1944행(기한 double 비트·포인터·`p`/`n`/`q`/`k` 호출 순서).
- `DetonateBomb`의 768행 관측은 실제 객체 변환 몸체(`RawObjectConversion`의 Spell→Object→Form 반환)가 필요해 그 단위에서 대조한다. 여기서는 뗌/낮은 WORD 0/선택된 가상 6c/미연결 거부 계약만 검사했다.

계약 검사: 도달한 부모만 요구, 생성 부모 실패 전파(token `0xffffffff` 보존), 노드 없음은 Push/Attach 비요구·누락은 거부, 이동 문자 우선순위, fast/ready 시계 비접근, 감지 중단 뒤 선행 0 보존, PlayIntro의 수정키 허용·뗌/다른 명령의 State 비변경, 메인 활동의 지연 현재 조회(now가 바꾼 모달/포인터를 뒤에서 읽음)와 Kill 누락이 큐 효과를 되돌리지 않음.

최초 실패와 교정: 11건이 처음부터 통과했다.

## 3. 경계

- 실제 Client 설정 소유자(`AutoDemo` DWORD·`introMovie` 설정·Config 변경 알림)와 `ProbeBaseFileByPolicy`(네 출처 조회 순서)·`BaseFileSystem` 존재 조회·TAFF는 해당 단위에서 대조한다(`IntroMenu_PresencePolicy…`·`ActualFileProbe…`·`MainMenuActivity_ClientCurrentOptionOwner`).
- 대본 큐(`QueueScriptCommand`)·실제 Gump 트리(`GumpCreateNode`, 캡처 노드 분리/재부착)·메인 메뉴 HTML/모달은 해당 단위에서 한다. CD 10.72는 범위 밖이다.

## 4. 다음 시작점

`ProductionShortcuts`·`RitualChoiceInput`·`ThemeTechnologyCommand`·`RawSalvageCommand`, `LanguageRequest`·`WindowTitle`·`HtmlInputDialog`·`DialogCommandLifetime`, `RawSquidPoolReset`. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
