# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 [E1288 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)과 그 아래 날짜별 보관본을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → 3.1(단계별 항목) → 0.5(단위 착수·종료 절차) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 칸으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-23 저녁 정리, 마지막 단위 WB04-E1292)

| 항목 | 현행 상태 |
| --- | --- |
| 실행·플레이 | 브라우저에서 ‘클릭하여 시작’, 시작 title 그림, 메인 메뉴·캠페인/데모 목록·도움말·옵션(Reset All 게임 안 확인 창 포함)·Credits와 개발용 지형 미리보기가 동작한다. **미션:** 캠페인 목록 31개(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)와 데모 3개가 시작되고, 장시간 진행과 임의 조작(몽키) 시험에서 오류·먹통이 없다. Campaign ▸ Skirmish는 무작위 섬·간헐천 월드에서 AI와 진행되며 AI 생산·공격으로 사제가 죽으면 패배 안내까지 이어진다(E1262, 약 18분 관찰). Tutorial2는 시작부터 완료 창·Tutorial3 시작까지 자연 진행으로 확인했고, 승패 안내 뒤 흐름(Continue·Replay Mission·Leave/Next Mission·진행 저장)도 확인했다. 미션 프레임은 다리 붕괴 검사·전투 자원·제단 결과 알림·주변음/공유 바람·장면 음악·미니맵 갱신을 원본 주 루프 순서로 돈다(E1230~E1234·E1266). **조작:** 사제 선택·이동, 가장자리·Alt·가운데 버튼 스크롤, 미니맵 클릭·끌기, 생산 격자 클릭·단축키(D 포함)·조각 회전(C·회전 방향 옵션)·다리 배치, 사제·워크샾·제단·신전·배터리·발전기·운반체 우클릭 메뉴와 회수·기도·Cast/Drop, 객체 명령(Detonate·Meltdown·신전 명령), 화면 위치 키(F4/H·F5·TAB·U·Shift+숫자 저장/숫자 복귀)와 방향키 카메라 이동(32픽셀, 조각을 든 ←/→는 회전, E1291), 미니맵·스톰 파워 표시·생산 창·채팅 도우미 단추의 우클릭 빠른 도움말(E1236·E1253)이 동작한다. **게임 메뉴:** ESC 또는 화면 우측 상단 버튼(웹 전용)으로 Game·View·Options·Players·About 메뉴막대를 열고, Leave/Restart Mission(ABORT 확인 창), F1·F2·F6·F7·F8·F9·Shift-F3·T, Pause(Shift-F9·Pause 키·메뉴), Shift-F7 화면 GIF 저장, Players 메뉴의 송금(Send X 숫자 입력 창 포함)·동맹이 실행된다. ESC는 열린 Tell 창(브리핑·로딩 제외)을 원본처럼 닫는다(E1264). 브리핑·완료 창의 Review Knowledge·GetTechnology 단추와 Priest Training 7의 Training Menu(원본 오타 별칭, E1245)도 동작한다. **HUD 틀·채팅 창:** 실제 Uber 루트가 좌측 사이드바(SP·생산 덱)·하단 미니맵 칸·구분선을 원본 위치로 도킹하고 지도 영역이 `{85,0,W,H}`다(4:3·16:9·16:10·1920×1080·창 크기 변경 확인). F3 채팅 창의 로그·입력줄·제출(로컬 서버 경유 `You> 본문`)·점 명령·Tab 이름 완성(동맹 AI)·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창이 동작한다(E1169~E1179). **미완성:** 주문(스펠) 사용(Early Missions에서 스크립트 오류·일시 정지)과 주문을 얻은 골렘의 적 사제 포획(사용자 제보 2026-11-23, 원인 미확인), 다른 미션의 목표 수행과 자연 완주, Archer 계열 특수 행 우클릭과 짐을 실은 운반체의 Carrying/Drop 실제 확인, 메인 메뉴 구름 배경(정적 GIF, Kencloud 미연결). 지식 NSInfo 패널·Players 하위의 Ignore chat·Whisper To·정보 보기·의식 보상 선택 UI·인트로 영상은 웹 범위 밖이다(E1167·E1171·E1180·E1202). |
| 원본과의 동일성 | 영어 4:3 메뉴 M1 8장면은 cpppj 오라클과 알려진 구름 배경 등 차이 영역 밖에서 픽셀 차이 0이다. **L2(E1283 닫음):** 검사 전용 고정 시각 모드(`?fixed=25`)에서 Tutorial1 Load/Start의 Game 난수열(90프레임 6,851회)·논리 화면·Session 덤프가 cpppj 기준선 오라클과 같다 — 4:3과 16:9(1366×768)의 90프레임 화면 0픽셀, 90·130프레임 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다르다. **L3(진행 중):** cpp `--ui-script` 입력 단계표를 그대로 적용한 Tutorial1 첫 400프레임(안내 창 MORE/OK·F4·다리 조각 집기·놓기)이 200·300·345·364·400프레임 모두 화면 0픽셀·덤프 일치다(E1284~E1286). 월드·UI 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다(원본 공통 메뉴 788행·Factory 메뉴 39행·Uber 루트 160행·장면 음악 3,113행, 채팅 입력줄 대상 720행(E1265), 파편·건물 캡처 4,474행(E1275~E1276) 등). 대응표 1,007단위 중 732행은 Tutorial1 실행에서 웹 모듈의 함수가 모두 실행된 `session`이다(E1283). 좌측 HUD 하단 85×82픽셀은 원본 스크린샷과 일치한다. 한국어 글자는 갈무리11 12px 격자로 그리되 제목 배율·굵기(사용자 결정으로 보통체)·한글 기준선을 cpppj와 다르게 둔다([0.7절](#korean-font-change)). 미검증: 16:10·한국어 화면 대조, 400프레임 뒤의 L3 구간, L4b(소리)와 자연 완주. |
| 현재 작업 | **L3 입력 진행 대조와 남은 표시·조작 확인 단계.** 4-D는 4D-4b(Tutorial1·3~6 단계 진행)만 남았다. L2를 닫은 뒤(E1283) Tutorial1 L3 단계표를 늘려 가며(6단계 M3) 어긋나는 첫 프레임을 고치고, 5-A 메인 메뉴 Kencloud·E198/E201 재대조, 6단계 객체 메뉴·운반체 실제 확인, 대응표 partial 12행의 메모 정리·승격을 병행한다. E1238~E1292는 감사 차이 닫기, partial 34→12행, L2·L3, 원본 캡처 대조, 한국어 글자 품질을 다뤘다([진행 요약](#e951-e1081)). 순서는 아래 [다음 작업](#next)을 따른다. |
| 전체 회귀 | **Node 26.13.0:** E1290 기준 6,805건 모두 통과(로그 `webpj/extracted/webpj-e1290-node26.log`, 989초; E1288이 놓친 `korean.test.mjs` 래스터라이저 시험 1건을 이 회귀 준비 중 찾아 고침). **Node 22.23.3:** E1292 기준 6,807건 모두 통과(로그 `webpj/extracted/webpj-e1292-node22.log`, 528초). **e2e:** E1281 전체 `npx playwright test` 162건 통과·11건 건너뜀(옵트인)·1건 실패(월드 진입 정지로 바뀐 Fury War 창 순서; 시험을 고쳐 통과, 로그 `webpj/extracted/webpj-e1281-e2e-all.log`, 30.7분). 이후 단위는 새 e2e와 관련 e2e를 단위마다 돌렸다. **HJOW-Athlon 주기(0.5절 5):** v26 2/4(E1290에서 실행, 이후 E1291~E1292) — 다음은 E1294 완료 뒤·v22 0/8(E1292에서 실행) — 다음은 E1300 완료 뒤이며 겹치면 순차로 돈다. 실패와 교정을 포함한 이전 회귀 이력은 [E1237 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)과 [E1288 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다. |
| 브라우저 검사 | 미션 화면은 단위마다 Chromium e2e로 확인한다(최근 전체 실행 E1281, 고정 시각 L2·L3 `e2e/fixed-time.spec.js`·`e2e/l3-tutorial1.spec.js`, 한국어 글자 배치 `e2e/korean-glyph-layout.spec.js` E1288). 메뉴는 2026-11-17 Chromium 2/2(갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체), 최초 실행·진행 잠금/체크·저장 재실행은 E616 Chromium 2/2. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md) |

<a id="next"></a>

#### 다음 작업

2026-11-23 저녁 정리에서 번호를 다시 매겼다. 단위 문서가 가리키는 이전 번호(다음 작업 1 ①~⑩·2~6)의 원문은 [E1288 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다.

1. **Tutorial1 L3 단계표 확장(6단계 M3/L3).** E1286의 400프레임 뒤로 이어지는 다리 놓기·튜토리얼 안내 진행 → 사제 선택·이동 → 건설·생산 순으로 단계표(`tests/fixtures/cpppj/candidate-225553df/tutorial1-l3/steps.json`)를 늘리고, 체크포인트마다 화면 0픽셀·덤프 `sp`/`player` 외 일치를 확인한다. 어긋나는 첫 프레임의 원인을 고친다. 이 확장이 4D-4b의 Tutorial1 단계 진행과 6단계 '건설·생산·경제'의 다리 조각 연속 배치 확인을 겸한다. cpppj 오라클 수집은 HJOW-Athlon에서 실행 확인 없이 가능하고 다른 PC는 AGENTS 실행 확인 규칙을 따른다(3.3절). [L3](docs/webpj-l3-tutorial1-20261123.md)
2. **5-A 표시 잔여.** 메인 메뉴 구름 배경을 정적 GIF 타일에서 Kencloud 출력으로 바꿔 M1 구름 차이 37,054픽셀을 없애고(구름 구축 난수 801회는 이미 State 3 설치에서 소비, E1280), E197 계열 남은 캡처와 E198(EvilRay)·E201(Burning)을 cpppj 시험과 대조한다. 세부 단위는 3.1절 5-A.
3. **객체 메뉴·운반체 실제 확인(6단계).** Archer 계열 특수 행(폭탄·Turn To Obelisk!)의 실제 Chromium 우클릭과, 짐을 실은 운반체의 Carrying/Drop·줍기·내려놓기를 e2e로 확인한다. 운반체 시험 하나로 '객체 우클릭 메뉴'와 '사제·운반·경로 바인딩'의 앱 조작 부분을 함께 닫는다.
4. **다른 미션의 목표 수행(4D-4b).** Tutorial2([E1086](docs/webpj-tutorial2-complete-20261120.md))처럼 Tutorial3~6과 세 캠페인 묶음 미션의 단계별 목표를 수행해 완료 창까지 이어 본다. Early Mission 3 단계 A·6 단계 B의 너겟 납품(`0x020000b4`) 진행도 이때 확인한다. 자연 완주는 1절 사용자 확인 범위다.
5. **대응표 partial 9행 정리.** 메모가 E1223~E1283 결과를 반영하지 않은 행을 갱신하고, 닫는 조건이 충족된 행은 port로 올린다. 행별 닫는 조건은 0.2절 둘째 표에 있다(E1263 점검이 찾은 낡은 메모, [점검 기록](docs/webpj-left-jobs-audit-20261123.md)).
6. **12차 스커미시 나머지.** 승리 경로(적 전멸), 개별 난이도/동맹의 N프레임 동작 정량 비교, AI가 다리를 멈추는 시점(관찰에서 소유자별 360~600초)의 원본 대조. 비미션 너겟 보충은 스커미시가 미션 모드라 도달하지 않는다(E1270). [관찰](docs/webpj-skirmish-watch-20261123.md) · [게임 진행](docs/webpj-skirmish-play-20261120.md) · [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)
7. **주문(스펠) 사용·골렘 사제 포획(사용자 제보 2026-11-23, 차후 작업).** 사용자 지시로 지금은 구현하지 않고 기록만 했다. Early Missions에서 주문을 쓰면 스크립트 오류가 나고 게임이 잠시 멈추며, 주문을 얻은 골렘으로 적 사제를 포획할 수 없다. 원인 미확인(사용자 추정: 멀티플레이 미지원 범위 판정과 관련). 세부 단위와 완료 기준은 3.1절 7단계의 같은 이름 항목.
8. **결정·원본 확인 대기.** 사용자 결정과 원본 실행·사람 확인이 필요한 항목은 3.5절에 모았다.
9. **14차 모딩 API 기준 구현(사용자 결정 2026-11-23).** dotnetpj 계약 설계(MOD-1) 뒤 webpj를 기준 구현으로 맞춘다. [3.7절](#modding-api).

- **로컬 산출물.** `webpj/test-results-e1072-rerun`~`webpj/test-results-e1081`의 10개 폴더는 빈 폴더이거나 Git 제외 `*.zip` trace만 들어 있어 Git 상태에 나타나지 않는다(2026-11-23 정리 때도 남아 있음). 다시 볼 필요가 없으면 삭제해도 된다. E633~E640 단위 문서가 적은 루트 로그는 2026-11-21에 `extracted/webpj-e633-e640-root-logs/`로 옮겼으므로 그 폴더에서 읽는다.
- **미션 식별.** 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이고, 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이다(E753 재대조). [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md)

<a id="e951-e1081"></a>

#### E951~E1292 진행 요약

단위별 완료 문단·검증 수·회귀 이력은 [E1081 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081)의 0.1절(E951~E1081), [E1166 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)의 0.1·3.1·3.7절(E1082~E1166), [E1237 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)의 0.1·3.1절(E1167~E1237), [E1288 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)의 0.1·3.1절(E1238~E1288)과 각 단위 문서에 있다.

| 주제 | 단위 | 연결한 것 | 남은 것 | 대표 문서 |
| --- | --- | --- | --- | --- |
| 미션 로딩·Start | E951~E958, E968, E970, E974~E976 | 로그인 HUD·진행 창, 최초 Fort 요청 자동 처리, Ready→Start→첫 안내와 시계 정지/재개, 공유 Kernel 프레임 | — (원본 Gump serial은 TellWindow 어댑터로 동작 동등, E1252) | [로딩](docs/webpj-app-mission-loading-20261119.md) · [Start](docs/webpj-app-tutorial-start-frame-20261120.md) |
| 음향·음악 | E957, E959~E966, E997, E1059, E1061, E1119~E1120, E1153, E1238~E1239, E1258 | 효과음·음악 적재와 Web Audio 재생, 위치 음향·발소리, 장면 음악(미션 중 순환·첫 곡 난수·180초 기한)·희생 의식 곡, Wind Noise 판정, Sound On 끄기의 음악 무음·천둥 경로, 승리/패배 곡 범위 판정 | 실제 청취 | [앱 음향](docs/webpj-app-campaign-audio-20261119.md) · [장면 음악 닫기](docs/webpj-scene-music-close-20261123.md) |
| AI·채팅·Tell·승패 | E967, E969, E971~E973, E1058 | AI 이름·난이도·채팅 기록, 미션 Tell 조회, Gump 팝업 차단·닫기, 승패 안내, NSInfo·도움말 팝업 저장소 | — | [AI](docs/webpj-app-campaign-ai-20261120.md) · [승패](docs/webpj-app-mission-outcome-20261120.md) |
| Renderer·화면 | E977~E989, E1081~E1082 | 지면·객체·입자·커서·Gump 수집과 출력, Screen DIB·구름 합성·Present, Gump 이동 dirty, 도킹된 지도 영역 | 메인 메뉴 Kencloud 구름 | [출력](docs/webpj-campaign-renderer-drawing-20261120.md) · [Present](docs/webpj-app-renderer-present-20261120.md) |
| 미션 진입 | E985~E986, E1010~E1016, E1092 | Tutorial1~6·세 캠페인 묶음 진입·브리핑, 세 데모 시작·45초 Auto-Demo | 미션별 목표 수행·자연 완주 | [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md) · [데모](docs/webpj-demo-start-20261120.md) |
| 입력·카메라 | E990~E1001, E1009, E1084 | 선택·이동·포인터 폴링, 가장자리 스크롤·카메라 키(F4·P/R·F5·N), 조각 회전, ESC 메뉴 토글 | 여러 유닛 순환(Ctrl-F5·N은 사제 1명 상태로 확인) | [포인터](docs/webpj-app-mission-pointer-frame-20261120.md) · [사제 키](docs/webpj-browser-priest-shortcuts-20261120.md) |
| 생산·건설 | E1002~E1008, E1019~E1022, E1026~E1036, E1047, E1074, E1076~E1079, E1083, E1086 | 생산 패널·단축키·다리 배치, 회수·비용 표시, Factory 생산·업그레이드, Tutorial2 발사기 배치·회수 | 다리 조각 연속 배치·자연 완주 | [생산 패널](docs/webpj-client-production-panel-20261120.md) · [워크샾 생산](docs/webpj-browser-tutorial2-factory-20261120.md) |
| HUD 틀 | E1082, E1128, E1236, E1248, E1255, E1257 | Uber 루트 HUD 모드 3 도킹·화면 크기 변경 재배치, 미니맵 Gump와 클릭·끌기 e2e, HUD 우클릭 빠른 도움말, 미니맵·HUD·생산 컨트롤 행 닫기(표시 차이 도달성 판정) | — | [좌측 HUD 틀](docs/webpj-uber-hud-left-dock-20261120.md) · [HUD 닫기](docs/webpj-uber-hud-close-20261123.md) |
| 채팅 창 | E1169~E1179, E1253, E1265 | F3 표시·로그·입력줄·제출·수신·점 명령, Tab 이름 완성·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창, 도우미 단추 우클릭 도움말, 허브 fixture 두 개 | Format 입력창 아래 가로 슬라이더의 원본 화면 대조, 미션 간 로그 유지 결정(3.5절) | [채팅 조립](docs/webpj-chat-window-assembly-20261122.md) · [허브 fixture](docs/webpj-chat-hub-fixtures-20261123.md) |
| 게임 메뉴 | E1084~E1085, E1087~E1090, E1093~E1097, E1100, E1167~E1168, E1235, E1251, E1264, E1289~E1290 | ESC 메뉴막대·우측 상단 버튼, Leave/Restart, F6·F7·F9·T, Pause, Frame Rate·Language·Difficulty, Shift-F7 GIF, Players 하위 항목 실측(채팅·정보 항목은 단독 미션 도달 불가), Send X 숫자 입력 창, F1과 Tell/브리핑 창, Reset All 게임 안 Tell 확인 창, ESC Tell 닫기, 정보 ▸ 버전 창의 11.01 패치·Copyright 줄, 한국어 ESC 메뉴 전수 점검(프레임 속도 라벨·Player 정보 창 이름 해독) | — | [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md) · [ESC Tell](docs/webpj-tell-escape-20261123.md) · [버전 창](docs/webpj-about-version-copyright-20261123.md) · [ESC 한국어](docs/webpj-esc-menu-korean-20261123.md) |
| 스커미시(12차) | E1101~E1107, E1262, E1270 | 설정 화면·저장, 시작 섬·섬 생성 부모, 세션 월드 생성·앱 시작, AI 진행·금지 유닛, AI 생산·공격과 사제 사망 패배 안내 관찰, 미션 모드 판정 | 승리 경로·AI 다리 멈춤 시점 원본 대조 | [시작과 월드 생성](docs/webpj-skirmish-start-20261120.md) · [관찰](docs/webpj-skirmish-watch-20261123.md) |
| Gump·메뉴 UI | E1017~E1018, E1023~E1025, E1037~E1045, E1064~E1072, E1075, E1086 | Form 통지, 글자·Slider 출력, 중첩 메뉴, 사제·Player·공통 객체·운반체 메뉴, 신전 건설·완공 | — (객체 메뉴 잔여는 '객체 메뉴·명령' 행) | [Client 메뉴](docs/webpj-client-menu-gump-20261120.md) · [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) |
| 도움말 | E1046, E1048~E1055, E1229, E1240 | 생산 Help, TYPE 정보 창, 요구 에너지·도움말 그림, 링크 이동·월드 없는 메인 메뉴 Help의 TYPE 정보 | 타입별 원본 화면 비교 | [앱 TYPE](docs/webpj-app-help-type-info-20261120.md) · [메인 메뉴 Help](docs/webpj-main-menu-help-type-20261123.md) |
| 한국어 문구 | E1056~E1057, E1062~E1063, E1073, E1080, E1108~E1118, E1140~E1142, E1166, E1246, E1249~E1250 | 모든 미션 대본, 게임 안 메뉴·객체 메뉴·지식 문구, 지식 카드 줄 나눔, Player 이름 디코딩, 생산 창·스톰 파워 도움말·채팅 채널 이름, 커서 경고·생산 힌트·객체 정보·Send 줄·도움말 없음 안내, 채팅 도우미 라벨 59개 | 실제 화면 확인(미션 본문 줄 나눔 등), 사람 검수 | [캠페인 미션](docs/webpj-korean-campaign-missions-20261120.md) · [미션 화면 15개](docs/webpj-korean-gameplay-xlat-20261123.md) |
| 한국어 글자 | E1244, E1254, E1259~E1261, E1288 | 굵은 글꼴 부족 문자의 일반 글리프 대체, 큰 슬롯 정수 배율·기준선, Tell 본문 폭 맞춤, 12px 격자·4배 그리기와 숫자·영문·공백의 원본 글리프 | 고해상도 글자 층(제안, 사용자 결정) | [12px 격자](docs/webpj-korean-glyph-grid-20261123.md) · [제목 판독](docs/webpj-korean-title-legibility-20261123.md) |
| 조작키 | E1145~E1146, E1148, E1151, E1154~E1155, E1159, E1162~E1163, E1291 | Alt·가운데 버튼 스크롤, H, C·`rotmode`, TAB·U·Shift+숫자/숫자, D·F 키 점검, F3 구분선, Pause 키, 방향키 카메라 이동 | — | [Alt 스크롤·조작키](docs/webpj-alt-scroll-keys-20261121.md) · [화면 위치 키](docs/webpj-camera-keys-20261121.md) · [방향키](docs/webpj-arrow-camera-20261124.md) |
| 객체 메뉴·명령 | E1126~E1127, E1135~E1138, E1150, E1225 | Players 송금·동맹, 파생 객체·제단·운반체·Archer 메뉴, 메뉴 닫기·비활성 색, 객체 명령·정체 안전장치, 사제 기도·Cast/Drop·Factory 순환 명령 소비 | Archer 특수 행·짐을 실은 운반체 Carrying/Drop 실제 확인 | [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [객체 명령](docs/webpj-object-commands-20261121.md) |
| 장시간·실사용 점검 | E1121~E1125, E1128, E1132~E1134, E1139, E1143~E1144, E1147, E1158, E1160, E1165 | 전 미션 시작·장시간 진행, 몽키, 시계 정지 누수·죽은 Gump 정리 수정, 실사용·넓은 화면 e2e, 성능 측정 | 자연 완주 | [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md) |
| 승패·미션 흐름 | E1129~E1131, E1152, E1157, E1161 | 시작 덤프 구조 대조, 안내 창 배치, 승패 뒤 흐름·진행 저장, Priest Training 시작 연결 | — (L2·L3는 아래 행) | [L2 대조](docs/webpj-l2-tutorial1-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) |
| 분석·판정 | E1149, E1153, E1156, E1163, E1241 | 난이도별 건설 속도, Wind Noise, 빨리 감기 한계, 미연결 모듈 68개 분류, cpppj 1322·1325·1326 변경 아홉 행 판정(②는 파생 피해 진단 문구 하나) | 쉬움 채취 절반 실관찰 | [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md) · [미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md) · [1322·1325·1326 판정](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md) |
| 회귀 교정 | E1060, E1078 | 관측 자료 누락·메뉴 전용 번역 검사 범위, 외부 접속 감사의 비공개 필드 오탐 | — | [회귀 교정](docs/webpj-regression-translation-fixtures-20261120.md) · [접속 감사](docs/webpj-external-audit-private-field-20261120.md) |
| 웹 범위 판정 | E1167, E1171, E1176, E1180, E1182, E1187~E1188, E1192, E1202~E1204, E1238, E1242~E1243 | Players 하위 3항목·지식 NSInfo·의식 보상 선택 UI·HTML `$Input=`·패킷 원천/지연·메타 앞단·Sprite 갱신·FortCoreDataLoad·승리/패배 곡·건설 기록 재생·Uber 배경 자식을 웹 범위 밖으로, 인트로 영상은 사용자 결정으로 웹 제외 | — | [NSInfo](docs/webpj-nsinfo-scope-20261122.md) · [동영상 결정](docs/webpj-intro-movie-decision-20261122.md) · [건설 기록 재생](docs/webpj-build-replay-scope-20261123.md) |
| 대본·기술·타이틀 | E1184, E1189~E1190, E1197, E1199~E1200, E1227, E1245 | 시작 title 그림, Fury War 기술 지급, 대본 82개 명령 점검(`ViewSpot`·`Salvage`), 도움말·Tell 남은 두 행, Detonate·GetTechnology 원본 관측, Review Knowledge·GetTechnology 단추, Priest Training 7 Training Menu 별칭 | — | [대본 명령](docs/webpj-script-commands-audit-20261122.md) · [Fury War](docs/webpj-fury-war-technology-20261122.md) |
| Native 대응 대조 | E1193~E1195, E1205~E1221, E1273~E1274 | Native 고정 수신기 네 묶음·채팅 수신기, 건설 여섯 행·AssetDeletionVtables, RawSurface 훅 대조와 표시 후 계속 진단·17종 가상 삭제·unit-lost 억제, RawSurfaceDeletion·Lifetime 판정, Client 기본 수신기 범위 | DestroyPatch 표시 후 계속(사용자 결정) | [수신기](docs/webpj-native-receivers-audit-20261122.md) · [RawSurface](docs/webpj-native-surface-audit-20261122.md) · [DestroyPatch](docs/webpj-destroy-patch-decision-20261123.md) |
| 읽기 전용 감사·수정 | E1223~E1237 | Client 상태·입력(제단 희생 메뉴 훅)·UI 컨트롤(와이드 메인 메뉴 버튼) 감사, 객체 메뉴 명령 소비, 아이스 반사 0점 기본값, 도움말·음향·설정 partial 15행 닫기와 Tell Warning, 다리 붕괴·전투 자원·Player 결과 알림·주변음/바람 프레임, F1·HUD 우클릭 도움말·Back | — | [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md) |
| 감사 잔여·partial 닫기 | E1240, E1247~E1248, E1252, E1255~E1257, E1261, E1263~E1270, E1292 | 예산 칸 우클릭 판정, 미니맵·메뉴·HUD·커서·생산·BitmapFont·ClientAudio 행 닫기, 미완료 46항목 전수 점검(20개 닫음), 프레임 순서, 입력 큐 코드 전수, 이탈 State 정리, 사제 배치 검사 Session 설치, 4-D 세 행(SessionLogin·ClientCampaignTransport·UberReconnect) port 승격 | 예산 칸 원본 우클릭, 대응표 메모 정리(9행) | [점검](docs/webpj-left-jobs-audit-20261123.md) · [프레임 순서](docs/webpj-frame-order-20261123.md) |
| L2·고정 시각 | E1271~E1272, E1278, E1280~E1283 | 고정 시각 모드(`?fixed=25`)와 시작 그림 구간, cpp 난수 호출 추적으로 State 설치(SP 정리·구름 구축·주변음 기한) 맞춤, 월드 진입 Guide 정지, 4:3·16:9 오라클 0픽셀, 대응표 `session` 732행 승격 | 16:10·한국어 화면 대조 | [고정 시각](docs/webpj-fixed-time-mode-20261123.md) · [난수·시각](docs/webpj-state-install-rng-20261123.md) · [session 승격](docs/webpj-port-map-session-20261123.md) |
| L3 | E1284~E1286 | cpp `--ui-script` 단계표의 웹 적용(클릭 N+1·키 N−1 프레임), Tutorial1 400프레임까지 화면 0픽셀·덤프 일치 | 단계표 확장(다음 작업 1) | [L3](docs/webpj-l3-tutorial1-20261123.md) |
| 원본 캡처 대조 | E1275~E1276 | 신전·제네레이터·타워 파편 481행, 윈드 아비어리·워크샾·제단 3,993행의 크기·기준점·전체 픽셀 일치 | `golem`·`support-color` 캡처, E198·E201 | [파편 캡처](docs/webpj-temple-shrapnel-capture-20261123.md) |

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분(세부 단위 `- [x]`)은 중복 가산하지 않는다. 남은 단위와 줄 수는 `PORT_MAP.tsv`의 partial·todo 기준이며 partial에는 파일 전체 줄 수를 센다. 2026-11-24 E1292 때 `node webpj/tools/port-map.mjs`와 `python -X utf8 tools/port_inventory.py`로 다시 세어 **partial 9·todo 0, 9단위 / 11,959줄**이다(2026-11-23 저녁 정리의 12단위 / 12,748줄에서 E1292의 세 행 승격으로 줄었고 UberGump은 E1291의 방향키 분기로 11줄 늘었다; 오전 정리의 34단위 / 20,876줄에서 E1240~E1273의 행 닫기로 줄었다; 단위별 증감은 [E1288 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)). 단계 배정은 각 행이 남은 일로 걸린 3.1절 항목 기준이다. 4-D 머리 수는 오전 정리 61/85에서 E1263 점검(15개)·E1266~E1274(7개)·E1283(4D-10b)으로 84/85가 됐다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 |
| --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료(4-C 통합 잔여는 4D-3에서 닫음) | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **84/85, L2 닫음(Tutorial1 4:3·16:9)** | 3 / 7,602 |
| 5 | 5-A 렌더러·Draw | 44/46 | 1 / 394 |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 12/13 | 0 / 0 |
| 5 | 5-C 화면 허브·미션 진입 | 4/6 → M2 | 3 / 3,573 |
| 6 | 입력·게임 메뉴·경제·건설 | 6/10 → M3 | 1 / 229 |
| 7 | 전투·의식·AI | 3/7 | 1 / 161 |
| 8 | 미션 수명·대본·오디오·도움말 | 8/9 → M4 | 0 / 0 |
| 9 | 표시 품질·브라우저/OS·배포 | E945 하위 경로·설정 복원 검증, M5 미완료 | 0 / 0 |
| 10 | AGENTS 후순위 웹 요구 | 일부 선행, 3.7절 참조 | 0 / 0 |
| **합계** | | | **9 / 11,959** |

남은 partial 행의 단계 배정과 닫는 조건(줄 수 큰 순, 대응표 메모와 3.1절 항목 기준; [다음 작업 5](#next)):

| 단계 | 대응표 partial 행(줄 수) | 닫는 조건 |
| --- | --- | --- |
| 4-D | RawCampaignSession 7,130 | 메모 마지막 기록이 E1241이다. 메모의 '잔여' 문장을 4-D 닫기(E1263~E1283)와 대조해 남은 갭이 없으면 승격 |
| 4-D | CampaignClientServices 373 | ricochet 값의 비결정성(E1226, 원문과 동등 재현 불가)을 웹 적응으로 인정할지 판정 |
| 4-D | RawClockSynchronization 99 | 원문 CRT 정렬의 동률 RTT 동등성 확인(원본 필요) 또는 적응 판정 |
| 5-A | ClientRendererFrame 394 | 메인 메뉴 Kencloud 연결(3.1절 5-A) |
| 5-C | UberGump 2,714 | 메모에 E1245·E1251 반영. Kencloud 출력(`UberGump.cpp:2059`)은 ClientRendererFrame과 함께 닫는다 |
| 5-C | RawButtonControls 811 | 메모에 E1236~E1253 반영, 예산 칸 원본 우클릭과 채팅 창 'Chat Window' 우클릭 확인(5-C 메뉴·버튼 컨트롤) |
| 5-C | RawMenuChat 48 | 채팅 로그 미션 간 초기화 결정(3.5절) |
| 6 | CampaignObjectMenuClient 229 | Archer 특수 행·운반체 Carrying/Drop 실제 확인 뒤 메모 갱신·승격 |
| 7 | RawCampaignRitual 161 | AddRitualRegular의 shared 플래그 차이(E1228, 원문은 `processes.Attach`로 false·웹은 true, 영향 미확인) 판정 |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,007단위는 **port 이상 998(99.1%; `port` 266·`session` 732)·partial 9·todo 0**이다(2026-11-24 E1292 재집계). 제외 153단위(멀티 73·Edit 42·호스트 37·구판 1)를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session` 732행은 E1283 에서 `e2e/session-coverage.spec.js`(고정 시각 Tutorial1 4:3 130프레임 V8 함수 커버리지)와 `tools/port-map-session.mjs` 로 올렸다: **행의 웹 모듈이 모두 그 실행에서 함수를 하나 이상 실행했을 때만** 올리는 보수적 판정이며, 남은 `port` 263행(커버리지에 없음 62·함수 미실행 201)은 다른 시나리오(전투·AI·의식·편집 아닌 UI 등)에서 쓰이는 단위다. 오라클 대조를 뜻하는 `oracle`은 0이다(승격 규칙은 이후 시나리오가 늘 때 같은 도구로 다시 돌린다; [근거](docs/webpj-port-map-session-20261123.md)). E1098~E1237의 재판정 내역은 대응표 메모와 [E1166 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)·[E1237 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에, E1238~E1288의 행 닫기는 대응표 메모와 [E1288 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 572 / 573 | 1 | 0 |
| `client/` 세션·UI·표시 | 425 / 433 | 8 | 0 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 9단위 중 기존 기준선은 8단위/11,860줄, 재기준선 추가분은 1단위/99줄(RawClockSynchronization)이다(메모 머리 태그 `[rebaseline-port]`·`[rebaseline-adapt]` 기준). 재기준선 분류·영역별 상세는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)과 `python -X utf8 tools/port_inventory.py --families wb`에서 확인한다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 863항목을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

### 0.5 단위 착수와 종료

1. AGENTS.md와 공통·dotnetpj·웹 인계를 읽고 Git 상태·원격 변경을 확인한다. 같은 작업 트리의 다른 세션 변경을 고려해 파일명을 지정해 stage한다.
2. 대응표 검사와 기준선 뒤 변경을 확인한다. 3.1절의 단계 순서와 선행 조건에 맞춰 기준선 cpppj 코드, 기존 시험, fixture, 단위 문서를 읽는다. 큰 단위는 공개 API 경계로 나눈다.
3. 원본/클론·오라클 수집·별도 창 실행에는 AGENTS.md의 확인·예외 규칙을 적용한다. 웹 이식에는 디컴파일 소스 분석이 필요 없다.
4. 단위 문서에 근거 커밋·fixture/오라클·검사·실패와 교정·미연결 경계를 기록한다. 대응표의 web·base(8자리)·status·메모와 해당 manifest를 갱신한다. 현행 인계에는 현재 값·다음 작업·검증과 해당 체크만 갱신하고 완료 항목 상세는 정리 때 이관한다.
5. 각 작업 단위마다 해당 변경의 집중 검사와 연관 검사를 수행한다. HJOW-Athlon에서는 완료된 작업 단위 4회마다 Node v26 전체 회귀를, 8회마다 Node v22 전체 회귀를 수행한다. 두 버전의 전체 회귀가 함께 필요한 경우 순차 실행하며 동시에 실행하지 않는다. 다른 PC에서는 사용 가능한 최신 Node 버전으로 완료된 작업 단위 4회마다 전체 회귀를 수행한다. 필요하면 변경의 위험이나 통합 범위에 맞춰 추가 검사를 수행한다. 신호 NaN 비트 대조 시험은 맨 위에 `import './nan-exact.mjs'`를 둔다. (2026-11-19 사용자 지시)
6. 완료 후 당일 최대 번호를 확인하고 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. 기존 커밋의 Codex 공동 작성자 표기와 AGENTS.md의 push 금지 규칙을 따른다.

### 0.6 제품 공통 요구

11.01 표기·Credits 세 번째 항목과 HJOW/AI 문구·`Copyright 2026 HJOW`, 시작 클릭 화면, 공개 루트 내 실행 자산, 외부 접속 문서화·유효성 검사 비활성화는 반영했다. Credits AI 문구는 사용자가 다시 지시할 때 수정한다. **15차 Reset All은 E863에서 구현·검증했다.** [제품 11.01](docs/webpj-product-1101-20261105.md) · [외부 접속](docs/webpj-external-access-20261105.md) · [초기화](docs/webpj-reset-all-20261119.md)

<a id="favicon"></a>

- [ ] **favicon 탭 시각 확인.** E894에서 공통 ICO를 복사하고 상대 경로 링크를 추가했다. 원본 두 이미지의 바이트 대조와 Chromium의 HTTP 200·해독을 확인했다. 2026-11-20 Chrome의 localhost NetStorm 탭은 접근성 트리에서 확인했으나 Windows 창 캡처가 반복 시간 초과하여 탭 그림의 시각 확인이 남았다(E1263 점검: 탭 아이콘은 페이지 화면 캡처에 나오지 않는 브라우저 창 부분이라 자동 확인이 안 되고, 사용자 눈 확인 범위이다). [작업·검증](docs/webpj-favicon-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

<a id="about-version"></a>

- [x] **정보(About) ▸ 버전 창의 Copyright(사용자 요구 2026-11-23) — E1289 완료.** 게임 메뉴 About ▸ Version과 메인 메뉴 Help ▸ Version의 Tell `About`에 원작 저작권·`10.78 Patch by Ticonderoga Entertainment.` 줄은 그대로 두고 확인 단추 앞에 `NetStorm 11.01 Patch by HJOW.`(한국어 `NetStorm 11.01 패치: HJOW.`)와 `Copyright 2026 HJOW`를 한 번만 넣었다(`assets/product.js` `productTellBody`, cpppj 1330과 같은 문구·위치). Ticonderoga 치환 추측은 실제로 재현되지 않는다(`gamemaster`·`gameminor`는 10·78). 일본어 줄은 13차에서 더한다. Node 시험·e2e 2건·변형 확인 통과. [근거](docs/webpj-about-version-copyright-20261123.md) · [공통](LEFT_JOBS.md#user-requests-20261123)

<a id="korean-font-change"></a>

### 0.7 한국어 갈무리11

E896에서 실제 미션 설치의 You→당신 번역과 Player 표시 바이트를 연결했다. [설치](docs/webpj-app-install-display-20261119.md) 제공된 일반·굵은 WOFF2와 라이선스를 공개 데이터에 포함했고 로딩·Canvas·자료 준비에서 같은 글꼴 설정을 공유한다. 작은 글씨와 영어 대체를 Chromium 메뉴·도움말에서 확인했다. [구현·검증](docs/webpj-korean-galmuri11-20261117.md) 실제 미션 화면에서도 Tutorial1 한국어 브리핑 두 페이지의 갈무리11·혼합 제목·줄바꿈과 당시 미번역이던 Tutorial2의 영어 대체를 Chromium으로 확인했다. [한국어 미션](docs/webpj-korean-tutorial1-20261120.md)

**글자 품질(E1244·E1254·E1259~E1261·E1288).**
- **E1288(사용자 점검 요청):** 한국어 글자를 알아보기 어렵던 원인은 12px 설계 격자인 갈무리11 WOFF2를 11px로 그려 가로획이 빠진 것(`가`→`기`)과 Canvas 가로 번짐이 1px 간격(ㅔ)을 메운 것이었다. 크기 기준을 12px(제목 24·슬롯 4 48px)로, 래스터라이저를 4배 그리기·칸 평균으로 바꿨다. 숫자·영문·공백은 cpppj처럼 원본 비트맵 글리프로 그려 SP 숫자가 영어와 픽셀까지 같고 Tutorial3 D1의 제네레이터 그림 세 개가 한 줄에 놓인다. 슬롯 1·6의 한글 기준선은 `max(원본, 배율×11)`이다. [근거](docs/webpj-korean-glyph-grid-20261123.md)
- 게임 안 글자는 브라우저 최소·기본 글꼴 설정과 무관한 고정 크기임을 실측했다(예외: 웹 전용 ☰ 메뉴 DOM 버튼). D2Coding·Noto Sans KR로 바꾸는 것은 8비트 단색 출력에서 효과가 없다.
- 큰 슬롯(제목)은 원문 정수 배율과 윗부분이 잘리지 않는 기준선(E1259), Tell 본문 폭은 가장 긴 낱말 이상(E1260)이다. 굵은 글꼴에 없는 8,271자(호환 자모·한자·가나 등)는 일반 갈무리11로 대체한다(E1244). [제목 판독](docs/webpj-korean-title-legibility-20261123.md) · [굵은 글꼴 대체](docs/webpj-galmuri-bold-fallback-20261123.md)
- **사용자 결정:** 굵기는 원문 Bold 대신 지금의 보통체를 유지한다(2026-11-23 "지금은 그대로"). 글자만 출력 해상도로 덧그리는 고해상도 글자 층은 제안으로 남겼고 채택은 사용자 결정이다(3.5절). cpppj와 다르게 둔 점(제목 배율·굵기·한글 기준선)은 근거 문서에 있다. 같은 점검의 cpppj·dotnetpj 후속은 사용자 지시로 작업을 보류하고 [LEFT_JOBS.md](LEFT_JOBS.md)·[LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)에만 기록했다(`92ea7bfd1`).

## 1. 범위와 유지되는 결정

- **범위.** 일반 31경로와 데모 3경로를 대상으로 한다. 단독 미션의 같은 프로세스 안 내부 전송·패킷·Login·Zone·서버/클라이언트는 포함한다. LAN TCP·멀티플레이 UI·재접속/메타, 공식 검증 서버, Edit·커스텀 맵·User Made Campaigns, 설치/CD·Quit, 옵션 해상도/전체화면은 웹 범위에서 제외한다. 스커미시의 MP 시작 규칙과 Outpost는 12차에 포함한다.
- **이식.** cpppj의 파일과 API 경계, 부작용과 호출 순서, 정수 폭과 저장 시 정밀도, 난수 소비를 보존한다. 단독에서 도달하는지 확인해 웹 제외 분기에는 명시 오류를 유지한다. 현재 단계가 직접 요구하는 부품만 앞당긴다.
- **웹 구조.** `netstorm.js`가 앱 수명을 맡는다. `index.html`은 최소 변경하고 스타일은 JS에서 관리한다. 기존 CommonJS 서버와 `www/` ES 모듈 경계를 유지한다. 코어를 DOM·Canvas·Web Audio에서 분리하고 8비트 논리 화면과 최종 표시를 구분한다.
- **시간·입력·수명.** 시작 입력에서 오디오를 활성화한다. 탭이 숨으면 진행을 멈추고 복귀 시 시계를 다시 잡는다. `requestAnimationFrame`은 표시 기회다. 4:3부터 확인하고 16:9·16:10, 입력 역변환·DPI를 비교한다. 가장자리 스크롤은 활성 미션과 게임 영역 안의 포인터에서 동작한다.
- **자산·설정·언어.** `assets/game-data`·`fonts`·`webpj/translations`에서 `webpj/www/data`를 준비하며 느슨한 파일→TAFF 조회 순서를 유지한다. 버전 있는 `netstorm.settings`·`netstorm.progress` JSON은 localStorage에 두며 오류 시에도 계속할 수 있게 한다. 한국어 누락 파일·섹션·주제는 영어로 대체하고 미션 문구는 cpppj 확정본을 기본으로 삼는다. 사용자 검수와 언어별 글꼴은 3.5·3.7절을 따른다.
- **외부 링크.** Tell/도움말에서 치환된 `http:`·`https:`·`mailto:` 링크는 새 탭/창으로 열고 다른 주소의 제외 사유는 console에 기록한다. 개발 서버는 모든 인터페이스에서 수신한다. [외부 접속 7절](docs/webpj-external-access-20261105.md)
- **사용자 확인 범위.** 자연 완주·전체 외관/음향 동일성·실제 포획/희생·특정 적 공격·승리 재현은 사용자 확인 또는 별도 요청 대상으로 둔다. 공중 3종(훨리기그·맨오워·더스트 데빌)은 정적 이식 범위다.
- **보호·분담.** AGENTS.md·원본/CD·기존 fixture·공통 인계·dotnetpj 파일을 보존한다. cpppj 동시 작업은 사용자의 목표 지시를 기준으로 하며 별도 언급이 없으면 필요한 cpppj 수정이 가능하다. README 공통 상태는 일괄 갱신 때 반영한다. 새 문서와 코드는 UTF-8, 새 코드의 한국어 주석은 설명·사용법을 먼저 쓰고 추가·수정 시점과 이유를 뒤에 쓴다.

## 2. 완료 범위 요약

| 범위 | 확인한 결과 |
| --- | --- |
| WB00~WB04-D5c | 시작·메뉴·설정/진행·도움말·언어/제품 표기, 자산/TAFF/Fort 조회, SID·난수·Kernel·Form/Process 공통층. [기반과 단위 링크](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E1~E328 | 월드 규칙 부품, 자료·타입 표, Fort·지형·섬과 단독 미션 전송 앞단. [순서 3 및 4-A~4-C](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E329~E632 | 이동·비행·전투·의식 선행 부품, 렌더러·Gump·카메라/HUD, 제품 1313으로 재기준선, typed Session 생성자·공유 snapshot writer 선행. 최초 실행(E616)과 내부 미션 전환 flush/종료 수신(E620) 반영. [선행 부품과 재기준선](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E633~E702 | 외곽 생성자·snapshot/콜백과 typed `Load()` 완료분. 실제 1-1 Fort 적재, Battle snapshot 재적재, 기술·로컬/세계 간헐천 및 단일 미션 너겟 관문 확인. [typed Load](docs/webpj-session-load-20261118.md) · [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) |
| E703~E774 | 실제 typed Login/Ready·Start, 출하 스크립트 32종의 Fort 로그인, 같은 Session의 사제·골렘·벌룬 이동과 워크샾 배치·격리 완공. [Login](docs/webpj-session-login-ready-20261118.md) · [Start](docs/webpj-session-mission-start-20261118.md) · [워크샾](docs/webpj-workshop-isolated-completion-20261118.md) |
| E775~E801 | Session 카메라·입자·튜토리얼 게이트, AI/Player/Process 반납 경계, 지연 GameWorld·팔레트·자산 보기·선택. World 입력 부품과 UserInput 폴링·단축키·우클릭까지 연결. [공통 게이트](docs/webpj-session-tutorial-gate-20261118.md) · [지연 GameWorld](docs/webpj-campaign-game-world-front-20261118.md) · [입력](docs/webpj-campaign-selection-keys-20261118.md) |
| E802~E950 | 입력 클릭·NB20 취소, GameWorld 프레임·세 운반 유닛 도착, 워크샾 Kernel 완공·연구 동기화, 전투 완료 관문, Client 진단·프레임 시계, Session 덤프 형식과 오라클 시나리오(4D-9b), 앱 Guide·State7·Gump 루트·팔레트·Screen·입력 큐·TextEntry/IME·갈무리11 편집, Reset All(E863), 16:9 최대 1920×1080(E943), 하위 경로 배포(E945). [E1081 보관본 0.1·3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081) |
| E951~E1237 | 미션 앱 프레임의 로딩·Start·음향·Renderer/Gump 출력, 브라우저 입력·카메라와 원본 도움말 조작키, 31개 미션·데모·스커미시 진입과 장시간 진행, 생산 패널·건설, 객체 메뉴·명령, 게임 메뉴막대, 승패 뒤 흐름, 도움말, 한국어 대본·게임 안 문구, HUD 도킹·채팅 창, 웹 범위 판정과 Native 대응 대조, 감사로 찾은 프레임·도움말 차이 수정. 주제별 표는 [0.1절 요약](#e951-e1081). |
| E1238~E1288 | 감사 차이와 partial 행 닫기(34→12행), 인계 미완료 항목 전수 점검, 고정 시각 모드와 Tutorial1 L2(4:3·16:9, E1283)·L3 첫 400프레임, 원본 캡처 대조, 대응표 `session` 승격, 한국어 글자 품질. 주제별 표는 [0.1절 요약](#e951-e1081). |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다. 여러 독립 작업을 묶은 항목은 `- [ ]` 세부 단위로 나누고 세부마다 완료 기준을 적는다(세부 단위는 머리 수에 넣지 않는다, 0.2절). 닫힌 체크는 단계마다 '완료한 체크' 줄이나 표로 줄이고 점검·완료 근거 원문은 보관본에 둔다.

#### 4-D Session 조립·Tutorial1 Load/Start (84/85)

**완료 기준 L2는 E1283에서 닫았다.** 같은 시점·난수에서 Tutorial1 Load/Start의 덤프·첫 프레임 논리 화면이 cpppj 오라클과 같다(영어 4:3·16:9, 90·130프레임: 화면 0픽셀, 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다름 — `player`의 +1·+2 바이트는 주소 잔재, Player 1 +0x68·+0x69는 Deck 할당 토큰의 표현·시점 차이). 기준선 구간 지도와 E633~E801의 완료 체크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), E802~E1166의 항목별 완료 나열은 [E1166 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166), 아래 닫힌 항목의 점검·완료 근거 원문(E1263~E1283)은 [E1288 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다. cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다.

- [ ] **4D-4b Tutorial 게이트·표시 장치** — Tutorial 1~6 판정·공통 게이트와 신전의 현재 슬롯 재조회(E781~E782), 다리 수명 숫자·Renderer(E1230), 승패 판정 소비(E1152·E1161·E1262)는 끝났다. 남은 것은 실제 앱의 단계 진행이다(자연 진행 e2e는 Tutorial2뿐, E1086). 스커미시 승리 실전은 1절 사용자 확인 범위다. [E781](docs/webpj-session-temple-draw-20261118.md) · [E782](docs/webpj-session-tutorial-gate-20261118.md)
  - [ ] Tutorial1 — 6단계 M3/L3의 단계표 확장으로 완료 창까지 간다. 완료 기준: 단계표 끝 체크포인트가 cpppj와 화면 0픽셀·덤프 일치이고 완료 창이 열린다.
  - [ ] Tutorial3 — Tutorial2 e2e(`e2e/english-tutorial2.spec.js`) 방식으로 단계 목표를 수행한다(단계 A 너겟 납품 `0x020000b4` 포함). 완료 기준: 완료 창과 다음 미션 해제까지 e2e 통과.
  - [ ] Tutorial4 — 완료 기준: 위와 같다.
  - [ ] Tutorial5 — 완료 기준: 위와 같다.
  - [ ] Tutorial6 — 단계 B 너겟 납품 포함. 완료 기준: 위와 같다.

완료한 체크(머리글 수에 포함):

| 항목 | 닫은 단위 | 닫은 근거 요지 | 대표 근거 |
| --- | --- | --- | --- |
| 4D-1 외곽·snapshot | E946·E1270 | 저장 writer·Core·부모 callback·생성자/Publish, 4D-1b 전체 snapshot(E946), 4D-1c 외곽 callback — 비미션 너겟 배치·보충은 스커미시가 미션 모드라 도달하지 않음(E1270) | [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md) · [미션 모드](docs/webpj-skirmish-world-mode-20261123.md) |
| 4D-2 typed Load | E697 | 건설·이동·AI·의식·파괴/주문·운반·배치/커서 설치, 1-1 저장 객체 적재와 `loaded` 관문 | [E697](docs/webpj-session-load-20261118.md) |
| 4D-3a Login | E1263 | Begin/Pump·Ready·출하 Fort 32종 로그인(E752~E765), World 교체는 이탈 뒤 재시작(E1085), 일반(LAN) 설치 가지는 도달하지 않음 | [E765](docs/webpj-session-login-commit-audit-20261118.md) · [출하 Fort](docs/webpj-session-login-shipped-forts-20261118.md) |
| 4D-3b ClientCampaignTransport | E1274 | Client 기본 수신기 표는 단독 내부 전송에 그 패킷이 오지 않아 미설치와 같음(e2e `internal-transport-packets.spec.js`) | [근거](docs/webpj-client-default-receivers-scope-20261123.md) |
| 4D-3c 전송 앞단 | E1268 | 공통 `RawPacketFrame`(E820)·시간 표본과 Pump(E947), 이탈 때 시간 동기화 초기화 | [이탈 정리](docs/webpj-mission-leave-state-reset-20261123.md) |
| 4D-3d Squid 수신 | E1263 | Squid·Dais·Gun·Flyer·Aviary 수신 등록과 시험, `RawPeerWorldSync`(E822~E823) | [E823](docs/webpj-peer-world-session-20261119.md) |
| 4D-3e 명령 등록 | E1267 | 입력 큐 발행처 전수와 입력 루프 허용 목록 대조, 등록 5262..5436 완료 | [근거](docs/webpj-input-queue-codes-audit-20261123.md) |
| 4D-3f 통합 시험 | E1263 | 같은 Session의 자연 도착·완공(E826~E836), Tutorial2 워크샾 생산→Archer 배치 e2e | [E836](docs/webpj-game-world-frame-20261119.md) |
| 4D-4a 카메라·프레임 | E1266 | 주 루프 순서(주변음·바람→다리 붕괴→장면 음악→미니맵 갱신→Gump 정리·Idle→Kernel), 대응표 ClientMain 행 port | [근거](docs/webpj-frame-order-20261123.md) |
| 4D-5a 종료 수명 | E1263 | `Session.dispose` 반납과 Leave/Restart·31개 시작 e2e(E786·E789) | [E786](docs/webpj-session-state-withdraw-20261118.md) |
| 4D-5b Client State 정리 | E1268 | 이탈 효과를 `_releaseCampaignWorld`로 옮김(채팅 채널 정리·시간 동기화 초기화), 나머지는 영향 없음 판정, ClientState 계열 다섯 행 port | [근거](docs/webpj-mission-leave-state-reset-20261123.md) |
| 4D-5c 이탈·전투 판정 | E1263 | 전투 종료 프레임은 `missionPost` 관문(E1238), 4209..4279 함수 분류, 전투 복귀 State6(E842~E847) | [E844](docs/webpj-battle-completion-frame-20261119.md) |
| 4D-6a GameWorld | E1263 | 지연 Session 소유부터 앱 프레임 결선까지(E790~E989), 날씨 `tint`는 웹 화면 모드에서 도달하지 않음, GameWorld 행 port | [E908](docs/webpj-app-gump-root-20261119.md) |
| 4D-6b Client 서비스 | E1263 | 앱 Client 장치 결선(E837~E1122), 입력 큐 소비 전수(E1225·E1231). CampaignClientServices 행은 ricochet 비결정성(E1226)으로 partial 유지(0.2절) | [E967](docs/webpj-app-campaign-ai-20261120.md) |
| 4D-6c Session 공개 조회·장치 | E1263 | 기준선 줄 구간별 대응 확인(확신 낮음), Native 훅 11종은 RawSurface 감사 | [E848](docs/webpj-session-public-movement-20261119.md) |
| 4D-7a~7d ClientMain·Client 전역 | E1263 | 장기 소유자(E878~E883), 메인 루프(E976·E1230~E1234), 메타 앞단·Sprite 제외(E1188·E1203), 렌더러 전역(E977~E989). 프레임 예외 때 렌더 루프 정지 설계 유지 여부는 3.7절 7차 | [E880](docs/webpj-app-world-storage-20261119.md) · [E877](docs/webpj-app-frame-clock-20261119.md) |
| 4D-8a RawSurface 삭제·수명 | E1273 | 의도된 차이 유지: DestroyPatch 열두 지점·0x771은 보고 후 중단(원문은 표시 후 계속, 사용자 결정으로 열어 둠), `rememberPlacement`는 cpppj도 막음 | [근거](docs/webpj-destroy-patch-decision-20261123.md) |
| 4D-8b~8f Native 분기 | 2026-11-23 오전 정리 | 자원·생산·사제, 전투·의식, 건설·Vortex, 고정 수신기, 공통 후처리·Fort 가상 — 해당 대응표 행이 모두 port 또는 제외(E1193~E1221) | [Native 수신기](docs/webpj-native-receivers-audit-20261122.md) · [Fort CoreData](docs/webpj-fort-core-data-gate-20261122.md) |
| 4D-8g Native 분기 재대조 | E1263 | `UsesNativeTransmissionDevices()`를 쓰는 cpppj 파일의 행이 모두 port 또는 제외 | [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md) |
| 4D-9a·9b 덤프·오라클 | E869·E1263 | 덤프 형식·수집(E865~E867), 오라클 시나리오(E868~E869), 앱 `sessionDump()`(E1129) | [E869](docs/webpj-tutorial1-cpp-oracle-20261119.md) · [E867](docs/webpj-session-dump-world-20261119.md) |
| 4D-10a 실제 소유자 재대조 | E1263 | 앱·Session 실제 연결과 대응표 행 port 확인, Guide 해제(E882) | [E882](docs/webpj-client-guide-release-20261119.md) |
| 4D-10b L2 판정 | E1283 | 고정 시각 모드(E1271)·State 설치 난수(E1280)·월드 진입 정지(E1281)·16:9 오라클(E1282)로 4:3·16:9 0픽셀 | [난수·시각](docs/webpj-state-install-rng-20261123.md) · [고정 시각](docs/webpj-fixed-time-mode-20261123.md) |

#### 5-A 렌더러·Draw·시각 시스템 (44/46)

- [ ] **ClientRendererFrame** — 미션 화면 출력의 원본 순서는 앱에 연결했다(수집·정렬·출력 E978~E980, Screen DIB·구름·Present E981~E984, Gump 수집·투영 E988~E989, dirty E1081, 지도 영역 E1082). 날씨 색(`tint`)은 웹 화면 모드에서 도달하지 않고 진단·Meta·Rig 표시(`special` 개발자 관문·Meta 수신기 전용)는 웹 범위 밖이다. [E981](docs/webpj-campaign-renderer-drawing-20261120.md) · [E984](docs/webpj-app-mission-display-frame-20261120.md) · [E989](docs/webpj-app-gump-world-projection-20261120.md) · [화면 문서](docs/webpj-screen-20261113.md)
  - [ ] 메인 메뉴 Kencloud — `ui/main-menu.js`의 정적 GIF 타일(`surface.tile(clouds)`)을 Renderer의 Kencloud 출력(`ClientKencloudGlobals.draw`, 원문 `UberGump.cpp:2059`)으로 바꾼다(미션 쪽 구름은 이미 Kencloud). 완료 기준: M1 시나리오(`tests/fixtures/cpppj/m1-scenarios.json`)의 4:3 구름 차이 37,054픽셀이 0이 되거나 남는 차이의 원인이 기록되고, 대응표 ClientRendererFrame·UberGump 메모가 갱신된다.
- [ ] **E197·E198·E201 재대조** — 웹 조립으로만 확인한 시각 시스템을 실제 cpppj 시험·원본 관측과 대조한다. 7단계 '전용 시험이 없었던 시스템'과 같은 일이라 여기서 한 번만 한다. 원본 실행 없이 읽기·Node 시험만으로 가능하다. cpppj 시험이 참조하지만 웹 manifest에 없는 fixture는 E1275 기준 158개이며 편집기·파일·CD·시작·음악 장치 등 웹 제외가 다수다.
  - [x] E197 Shrapnel 파편 캡처 — 신전 181·제네레이터 180·타워 120행의 크기·기준점·전체 픽셀 일치(E1275, `temple-shrapnel-native-fixtures.test.mjs`, 변형 확인). [근거](docs/webpj-temple-shrapnel-capture-20261123.md)
  - [x] 같은 계열 건물 캡처 — 윈드 아비어리 54·워크샾 2,115·제단 1,824행 일치(E1276, `altar-capture-native-fixtures.test.mjs`).
  - [ ] `golem-capture`(67행)·`support-color-capture`(199행) — GameWorld 스프라이트 렌더링(카메라 이동 전후) 대조. 완료 기준: manifest 등록과 웹 시험의 전 행 일치.
  - [ ] E198 EvilRay — `EvilRayVisualSystem`의 Host/Kernel 부착·실제 입자 월드 결선을 대조한다(`EvilRayFrameTests`·`EvilRayLifetimeTests`는 웹 시험이 이미 있다. cpppj 쪽 대조 후보는 실제 Session 의 `EvilRayVisualProcess` 를 찾는 `CampaignObjectMenuTests.cpp:1083` `CurrentEvilRay` 사용 시험이다). 완료 기준: 대응 cpppj 시험의 기대값을 실제 Session을 쓰는 웹 Node 시험이 재현하고 변형 확인. [E198](docs/webpj-evil-ray-visual-20261109.md)
  - [ ] E201 Burning — 피해 연소 생성 호출(`RawCampaignParticles`)의 cpppj 시험(`cpppj/tests/DamageableBurningSessionTests.cpp`의 `DamageableBurningChecking_ActualClientSessionRecoveryAndLifetime`, `CampaignDamageTests.cpp`의 Burning 훅 등)과 대조. 완료 기준: 위와 같다. [E201](docs/webpj-burning-visual-20261109.md)
  - [ ] 섬 투하 지지·입자 — `CampaignSession_ActualDropIslandSupportAndParticles`(`cpppj/tests/CampaignSessionTests.cpp`) 대조. 완료 기준: 위와 같다.

#### 5-B 카메라·HUD·미니맵 (12/13)

완료한 체크(머리글 수에 포함, 보관한 9개 외): **UberHud**(E1255, E1224 감사의 표시 차이 여섯 가지를 도달성으로 판정, 지식 NSInfo 공장은 웹 범위 밖 E1171, [근거](docs/webpj-uber-hud-close-20261123.md)), **RawMinimapControls**(E1248, ToggleMinimap 도달 불가·클릭/끌기 e2e·우클릭 도움말 E1236, [근거](docs/webpj-minimap-controls-close-20261123.md)), **FloatingSpGump·RawProductionControls**(E1257, 생산 패널 도킹·UpdateContents 뒤 화면 갱신 픽셀 e2e, [근거](docs/webpj-production-controls-close-20261123.md)).

- [ ] **Cursor·RawCursor 표시** — 커서 부품의 실 화면 연결(Cursor 계열 행 전부 port, CampaignCursorClientEffects E1256)과 가장자리 스크롤(E990~E1001)·마나 볼트 Session 연결(`raw-campaign-session-arrivals.js`)은 끝났다. 남은 것은 실제 화면 대조다. [근거](docs/webpj-cursor-client-effects-close-20261123.md)
  - [x] 카메라 투영·커서 표시 — L2/L3 고정 시각 대조에서 F4 카메라 이동 뒤(300프레임)와 다리 조각을 든 커서(345프레임) 화면이 cpppj와 0픽셀이다(E1280~E1286).
  - [ ] 가장자리 스크롤 뒤 화면 — cpp `--ui-script`에는 포인터 이동 동작이 없어(`click`·`right`·`key`·`keyup` 등) 입력 방법부터 정한다. 완료 기준: 같은 스크롤 입력 뒤 체크포인트 화면이 cpppj와 0픽셀.
  - [ ] 마나 볼트 정기 표시 — 완료 기준: 마나 볼트가 그려지는 장면의 고정 시각 화면이 cpppj와 0픽셀이거나 남는 차이의 원인이 기록된다.

#### 5-C 게임 화면 허브·미션 진입 (4/6)

완료한 체크(머리글 수에 포함): **Uber 허브**(E912~E917·E1082, 채팅 창 E1169~E1179, 0xb0 배경 GIF·NSInfo 자식은 웹 범위 밖 E1243, [판정](docs/webpj-uber-root-background-scope-20261123.md) · [E912](docs/webpj-uber-root-construct-20261119.md)), **미션 진입 명령**(E1263, 31개 미션·데모 시작, 환영 창 `MissionBegin` 경로와 로딩 창 취소, [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md)), **허브 fixture**(E1265, `chat-input-recipients` 720행 일치·`chat-callbacks` 29행 반환 0 대조와 적응 판정, [근거](docs/webpj-chat-hub-fixtures-20261123.md)), **메인 메뉴·타이틀 상태**(E1184, NativeIntroState·TitleBitmapLifetime·NativeMainMenuState와 title 그림 세 장, [문서](docs/webpj-title-screens-20261122.md)).

- [ ] **메뉴·버튼 컨트롤** — 남은 대응표 행은 RawButtonControls·RawMenuChat이다. RawMenuHtml(E1230)·RawMenuControls(E1252, Tell 모달 스택·제목/request/closed 사건 도달성 판정)는 닫았고 RawMenuLoading·RawTextEntryControls는 port다. 우클릭 도움말은 HUD(E1236)와 채팅 도우미 단추(E1253)까지 연결했다. 알림 serial 0 고정·도우미 점멸 없음은 채팅 패킷 발생원이 사용자 say뿐이라 도달하지 않는다(`f66401a24`). [메뉴 컨트롤](docs/webpj-menu-controls-close-20261123.md) · [채팅 도우미](docs/webpj-chat-helper-help-20261123.md)
  - [ ] 채팅 로그 미션 간 초기화 — 원본은 Client 수명 동안 유지하고 웹은 미션 끝에 초기화한다(E1170 적응, 채팅 전역이 미션마다 새로 만들어짐). 완료 기준: 사용자 결정(3.5절)대로 수명을 분리해 원본처럼 고치거나 적응으로 기록하고 RawMenuChat 메모를 갱신한다.
  - [ ] `storm-power-refresh` 재부착 — 원문의 Push/Attach 재부착을 앱이 쓰지 않고 `refreshSp`를 `invalidateAll+menu.refresh`로 대체했다(예산 칸과 겹치는 Gump가 없어 보이는 차이가 없을 가능성, 확신 낮음). 완료 기준: 겹침 여부를 확인해 연결하거나 무해 판정을 문서화한다.
  - [ ] 예산 칸(스톰 파워 표시, 노드 (7,2) 66×21) 실제 우클릭 — 웹은 `input().route(0x20000, 40, 12)`가 0이다. 예산 텍스트 노드에 HELP 플래그(0x200)가 없어 원본 판정 논리와 같으므로 웹 결함이 아닐 가능성이 높다(E1247). 완료 기준: 원본 실행 파일에서 우클릭이 팝업을 여는지 확인(AGENTS 실행 규칙) — 열면 웹이 막는 지점을 고치고, 안 열면 닫는다. [근거](docs/webpj-korean-production-chat-xlat-20261123.md#budget-rightclick)
  - [ ] 채팅 창 'Chat Window' 실제 우클릭 — Uber 루트 채팅 노드의 도움말(`Chat Window`·`chatView`, `ui/uber-root-gump.js`). 완료 기준: 실제 Chromium 우클릭으로 팝업과 Help 항목이 열리는 e2e.
  - [ ] 대응표 메모 갱신 — RawButtonControls에 E1236~E1253, UberGump에 E1245·E1251·E1264를 반영한다. 완료 기준: 위 항목을 닫은 뒤 남은 갭이 없으면 port로 올린다(0.2절 둘째 표).
- [ ] **M2 / L4a** — 4-D·5-A·5-B 조립 뒤 Tutorial1 진입 직후와 N프레임 뒤의 논리 화면 차이 0을 확인한다. 영어 4:3부터 16:9/16:10과 한국어로 넓힌다. [고정 시각](docs/webpj-fixed-time-mode-20261123.md) · [난수·시각](docs/webpj-state-install-rng-20261123.md) · [L3](docs/webpj-l3-tutorial1-20261123.md)
  - [x] 영어 4:3 — 90프레임 화면 0픽셀(E1280), 90·130프레임 덤프 일치(E1281). N프레임 뒤 화면은 L3의 200~400프레임 0픽셀(E1284~E1286)이 대신한다(130프레임 화면 캡처가 없다는 남음 문구를 이것으로 정정).
  - [x] 16:9(1366×768) — 90프레임 화면 0픽셀, 90·130프레임 덤프 일치(E1282, `tests/fixtures/cpppj/candidate-225553df/tutorial1/oracle-1366x768/`).
  - [ ] 16:10 — cpppj 선택지는 1280×800이고 웹 16:10 논리 크기는 높이 768 기준(약 1229×768)이라 직접 대조할 수 없다. 완료 기준: 크기 정책 결정(3.5절) 뒤 같은 크기 오라클로 90프레임 화면 0픽셀.
  - [ ] 한국어 — cpppj 한국어 설정으로 같은 오라클을 수집해 대조한다. 완료 기준: 차이가 [0.7절](#korean-font-change)의 의도된 차이(제목 배율·굵기·한글 기준선) 영역에만 있음을 기록한다.

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (6/10)

완료한 체크(머리글 수에 포함, 보관한 1개 외): **입력 본체**(E1263, UserInput 상위 메뉴 E1084·브라우저 이벤트 E919~E934·특수 진단 로그 장치, GameWorldInput E795~E796, NB20 E804, 편집 우클릭 메뉴·모드 1/6 좌클릭은 웹 제외, [E802](docs/webpj-campaign-user-input-movement-click-20261118.md) · [E796](docs/webpj-campaign-game-world-menu-cursor-20261118.md)), **게임 메뉴**(E1264, 메뉴막대 E1084와 하위 명령 E1085~E1235, Ignore chat·Whisper To·정보 보기 단독 도달 불가 E1167, Tell 모달 스택 E1252, ESC Tell 닫기, [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md) · [ESC](docs/webpj-tell-escape-20261123.md)), **공장·지식 메뉴와 기술**(E1076~E1079 Factory 생산·지식·업그레이드, 연구 명령 0x03000080 소비, E1189 Fury War 기술 지급; 제단 업그레이드·`Get 5,000` 행의 HTML 기술 목록은 비미션 전용, [E1180](docs/webpj-ritual-reward-scope-20261122.md) · [E1189](docs/webpj-fury-war-technology-20261122.md)).

- [ ] **객체 우클릭 메뉴** — CampaignObjectMenuClient의 건물·운반체·특수 대상 메뉴와 소유자 명령 연결. 중첩 메뉴·사제·Player·공통 객체·운반체 메뉴와 사제 건설(E1064~E1072), 워크샾 생산·업그레이드(E1076~E1079), 파생 본체 일곱 주소(E1127), 팝업 닫기·비활성 색·제단 위임·운반체/Archer 열기(E1135~E1138), 명령 소비(E1225)는 끝났다. Factory 파생은 VT `0x509148`(앱 연결)과 `0x509210`(멀티 Outpost 래퍼)뿐이고 특수 대상 메뉴는 Meta·편집·AI 디버그 전용, 채팅 분기는 단독 미션에서 만들어지지 않는다(E1167). 완성된 발사기(TYPE 71) 우클릭→Salvage는 `english-tutorial2.spec.js`가 확인한다. [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) · [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [운반체·Archer 메뉴](docs/webpj-carrier-archer-menu-open-20261121.md)
  - [ ] Archer 계열 특수 행 — 폭탄·Turn To Obelisk! 행을 가진 Archer 계열의 실제 Chromium 우클릭. 완료 기준: 특수 행 표시와 행 명령 소비 e2e 통과.
  - [ ] 짐을 실은 운반체 Carrying/Drop — 열기·작성은 Node 시험(`carrier-menu.test.mjs`)으로 확인했고 Drop 명령 소비 e2e는 사제 대상이다. 완료 기준: 내용물을 실은 Bulf·Walker의 Carrying 행과 Drop 동작 e2e 통과('사제·운반·경로 바인딩'의 줍기·내려놓기와 한 시험).
  - [ ] 대응표 승격 — 위 둘 뒤 CampaignObjectMenuClient 메모("다른 객체 메뉴는 남음" 등 낡은 문구)를 갱신하고 port로 올린다.
- [ ] **사제·운반·경로 바인딩** — 캠페인 Session 운반 입력과 일반 사제 이동(E794), 실제 마우스의 사제 선택→이동(E1139 e2e), 단독 미션 제단 희생 메뉴 훅(E1223), 기도·Cast/Drop·Factory 순환 명령 소비(E1225), Review Knowledge·GetTechnology 단추와 기술 획득 Fort snapshot 장치(E1227)는 끝났다. MetaMap은 도달하지 않고(E1188), 저장 미션의 전체 초기화와 경로 루프는 31개 시작 e2e·`raw-campaign-session-load.test.mjs`·`mission-priest-move.spec.js`·`raw-campaign-motion-bindings.test.mjs`로 확인했다. [E794](docs/webpj-session-carrier-input-20261118.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md)
  - [ ] 운반체 줍기·내려놓기 앱 조작 — 본체·Session은 `raw-campaign-session-motion-runtime`·`-carrier-drop` Node 시험으로 확인했다. 완료 기준: 실제 Chromium에서 운반체가 줍고 내려놓는 e2e(위 Carrying/Drop과 같은 시험).
  - [ ] 저장 미션 경로 L3 — cpppj `CampaignMotionTests.cpp:120~126`의 저장 미션 경로 시험을 고정 시각 단계표로 대조한다. 완료 기준: 체크포인트 덤프의 경로·위치 관련 행 일치.
- [ ] **건설·생산·경제** — 생산 유지·동기화·다리 붕괴(E831~E835·E1230), 앱 생산 패널·단축키·다리 배치(E1002~E1035), 회수(E1047)·비용 표시(E1074)·Factory 명령(E1076~E1079), 다리 조각·사제 건설 위치 지정·제단 회수의 실제 마우스 e2e(E1139)는 끝났다. 건설 기록 재생(FILE/최신 rec·Host·Ctrl+F7)은 개발자 전용이라 웹 범위 밖이다(E1242). Tutorial2 외 미션의 자연 완주는 1절 사용자 확인 범위다. [E1035](docs/webpj-app-production-panel-20261120.md) · [E1079](docs/webpj-browser-tutorial2-factory-20261120.md) · [건설 기록 재생](docs/webpj-build-replay-scope-20261123.md)
  - [ ] 다리 조각 연속 배치 — 발사기 연속 배치는 Tutorial2 e2e로 확인했고 다리 조각은 한 조각 e2e(`mission-build-placement.spec.js`)와 L3 한 조각(E1286)뿐이다. 완료 기준: L3 단계표의 이어지는 다리 놓기 체크포인트 화면 0픽셀(다음 작업 1).
- [ ] **M3 / L3** — Tutorial1 이동·수확·건설·생산의 입력 단계표로 cpppj 상태·화면을 대조한다. Tutorial1·2와 캠페인 1-1·1-2의 브라우저 검증은 끝났다(`l2-tutorial1`·`english-tutorial2`·`korean-mission`·`tutorial-menu-start`·`mission-priest-move` e2e). 웹 입력 주입 규칙(클릭 N+1·키 N−1 프레임)은 실측으로 정해 시험에 고정했다. 아래 남은 세부의 완료 기준은 모두 "새 체크포인트의 화면 0픽셀·덤프 `sp`/`player` 외 일치, 어긋나면 첫 프레임의 원인을 고쳐 일치"이다. [L3](docs/webpj-l3-tutorial1-20261123.md)
  - [x] 틀과 첫 구간 — `e2e/l3-tutorial1.spec.js`·단계표·오라클(`tests/fixtures/cpppj/candidate-225553df/tutorial1-l3/`), MORE→OK→F4→Bridges OK→다리 조각 집기·섬 남쪽 가장자리에 놓기, 200·300·345·364·400프레임 일치(E1284~E1286).
  - [ ] 이어지는 다리 놓기·튜토리얼 안내 진행.
  - [ ] 사제 선택·이동.
  - [ ] 건설·생산(수확 포함).
- [x] **방향키 카메라 이동(사용자 요구 2026-11-23) — E1291 완료.** 게임 중 ↑↓←→가 누름·브라우저 자동 반복마다 논리 화면 32픽셀씩 카메라 목표에 누적된다(cpppj 1328과 같은 계약). 활성 조각의 ←/→는 회전이 먼저이고 ↑/↓는 조각을 들어도 카메라이며, 대화창·도움말·메뉴막대·팝업·지식 창·채팅 입력줄·Send X 창이 열려 있거나 정지·키 뗌이면 움직이지 않는다. 키마다 확인하는 e2e(4:3·16:9)와 Node 시험·변형 확인 통과. [근거](docs/webpj-arrow-camera-20261124.md) · [공통](LEFT_JOBS.md#user-requests-20261123)
- [x] **ESC 게임 메뉴 한국어 점검(사용자 제보 2026-11-23, webpj 재현) — E1290 완료.** 한국어 ESC 메뉴 전수 e2e(`e2e/korean-esc-menu.spec.js`)가 찾은 영어 행은 프레임 속도 `75 FPS (Original)` 하나였다(Frame Rate 라벨이 번역 함수를 거치지 않았음 — 번역 경로 연결과 `75 FPS (원본)` 번역 추가, cpppj 1329와 같음). "플레이어" 메뉴의 글자 깨짐은 Player 정보 창 제목(`Player %s`)과 동맹 줄의 이름이 표시 부호화 바이트 그대로 그려진 것이었고 `decodeName` 장치로 해독하게 고쳤다(`간수` 확인). 같은 단위에서 E1288이 놓친 `korean.test.mjs` 래스터라이저 시험의 낡은 가정도 고쳤다. [근거](docs/webpj-esc-menu-korean-20261123.md)

#### 7 전투·의식·AI (3/7)

완료한 체크: **파괴·주문 효과**(E1199, DetonateInput 원본 관측 768행·RawObjectConversion 128+36행), **의식**(E1180, 보상 선택 UI는 비미션 전용이라 웹 범위 밖), **AI 잔여·전투 이탈**(E1198, AiTypeRequiredEnergy 실제 Session 연결·RawBattleLeave 부품 이식). [의식 판정](docs/webpj-ritual-reward-scope-20261122.md)

- [ ] **전투 바인딩** — Hit·GunQueries·Archer·Motion·BombBlast·Damageable 여섯 표 행은 모두 port이고 Session에 결선돼 있으며(`raw-campaign-session-{hits,guns,gun-views,combat,flyers,damageable}.js`와 각 Node 시험) 실제 Chromium에서 AI 혼전이 약 18분 오류 없이 돈다(E1262). Archer의 초기 배치·비행 및 factory/clock loop는 1절의 검증 범위를 적용한다.
  - [ ] 전투 장면 오라클 — AI가 공격해 오는 미션의 고정 시각 cpppj 오라클을 수집한다(3.3절). 완료 기준: 시나리오·결과와 출처가 manifest에 등록된다.
  - [ ] 교전 N프레임 대조 — 완료 기준: 피격·사격이 일어나는 체크포인트의 화면·덤프가 cpppj와 같거나 남는 차이의 원인이 기록된다.
- [ ] **AI 세션 결선** — Start의 일반/데모 AI·동맹과 typed 프레임, 현재 Player·SP·명령·난수와 장기 RawSurface 소비자를 연결했다(`raw-campaign-session-ai*.js`·`netstorm-campaign-ai.test.mjs`, E1122·E1124·E1198·E1206~E1216). DestroyPatch 진단을 표시 후 계속으로 바꿀지는 4D-8a(E1273)에서 사용자 결정으로 열어 두었다(3.5절).
  - [ ] AI 행동 N프레임 대조 — 위 전투 장면 오라클로 AI의 다리·생산 결정 시점을 비교한다. 완료 기준: AI 행동 체크포인트의 덤프(`random`·`regular`·`object` 행) 일치.
- [ ] **전용 시험이 없었던 시스템** — 5-A 'E197·E198·E201 재대조'와 같은 일이라 그 세부 단위로 한 번만 한다(E1263 점검).
- [ ] **주문(스펠) 사용·골렘 사제 포획 미구현(사용자 제보 2026-11-23)** — 사용자 지시로 지금은 기록만 하고 차후 작업으로 둔다(2026-11-23 저녁 정리 때 추가, 머리 수 3/6→3/7).
  - 증상 ①: Early Missions에서 주문을 사용하면 스크립트 오류가 나고 게임이 잠시 멈춘다.
  - 증상 ②: 주문을 획득한 골렘으로 적 사제를 포획할 수 없다.
  - 원인 미확인 — 사용자 추정: 웹판의 멀티플레이 미지원 범위 판정(멀티 전용 제외)과 관련. 관련 기록: 운반체 Cast 메뉴 열기(E1138), 객체 메뉴 Cast/Drop 명령 소비(E1225), 전투 자원 프레임의 스펠 소비(E1232, E1230~E1234 묶음), 파괴·주문 효과 원본 관측(E1199), 의식 보상 선택 UI 웹 범위 밖 판정(E1180)과 0.1 진행 요약의 '웹 범위 판정' 행. [운반체·Archer 메뉴](docs/webpj-carrier-archer-menu-open-20261121.md) · [RawSurface 감사(E1225·E1232)](docs/webpj-native-surface-audit-20261122.md) · [Detonate 관측](docs/webpj-detonate-gettech-fixtures-20261122.md) · [의식 보상 범위](docs/webpj-ritual-reward-scope-20261122.md)
  - [ ] 원인 찾기 — 주문 사용 경로(획득·Cast·대상 지정·효과·소비)의 미연결 장치와 명시 오류(웹 제외 분기의 throw 포함) 지점을 cpppj와 대조해 찾는다. 완료 기준: 증상을 재현하는 시험(e2e 또는 Node)과 원인 지점·cpppj 대응 위치가 단위 문서에 기록된다.
  - [ ] 주문 사용 오류·멈춤 없음 — 완료 기준: 단독 미션(해당 Early Mission 단계)에서 주문을 써도 스크립트 오류·정지 없이 효과가 나는 e2e 통과. 웹 범위 밖으로 잘못 판정한 분기가 있으면 대응표와 판정 문서를 정정한다.
  - [ ] 골렘 사제 포획 — 완료 기준: 주문을 얻은 골렘의 적 사제 포획이 cpppj와 같게 동작함을 상태 단언 e2e(가능하면 고정 시각 오라클의 Session 덤프 대조)로 확인.

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (8/9)

완료한 체크(머리글 수에 포함): **대본·대화창**(E1263, 관련 행 전부 port, 대본 82개 명령 전수 E1190·소비자 감사 E1231, 한국어 대본·Tell 그림, [E1190 점검](docs/webpj-script-commands-audit-20261122.md)), **소리 본체**(E1261, Web Audio 적응 E957~E997, Sound On 끄기·천둥 경로 E1239, Wind Noise는 원본 기본 설치에서 무동작 E1153), **음악**(E1258, 장면 음악 첫 곡의 공유 난수·소리 끈 동안 180초 기한, 승리/패배 곡은 단독 도달 불가 E1238, [근거](docs/webpj-scene-music-close-20261123.md) · [장면 음악 구동기](docs/webpj-scene-music-driver-20261120.md)), **글자·설정·그림**(E1261, BitmapFont 한국어 큰 슬롯 E1259·굵기는 사용자 결정으로 보통체, GifScreenshot E1100, [근거](docs/webpj-korean-title-legibility-20261123.md) · [화면 저장](docs/webpj-screenshot-gif-20261120.md)), **적응 fixture**(E1263, `renderer-projection` 90행·`scenemusic-x86` 3,113행은 소비 시험이 있고 오디오·GDI 글꼴 관측은 적응·제외). 실제 청취 판정은 사용자 확인 범위이다.

- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.
  - 완료: 캠페인 목록 31개 미션의 연속 시작(E1121, Priest Training 9개는 E1157에서 실제 연결하고 Auto-Demo가 대신 돈 e2e 거짓 통과를 정정), 캠페인 16개 장시간 진행과 AI 시점 미연결 서비스·좌표 오류 네 가지 수정·소리/안내 서비스 여섯 개 연결(E1122), Early Missions·Priest Training 15개 100초 진행(E1123)과 Priest Training 9개 150초씩 26분(E1158), 스커미시 AI 7명 300초(E1124), 데모 420초 진행과 360초 재시작(E1125), 후반 4개·중반 13개 장시간 진행으로 시계 정지 누수 수정(E1132~E1133), 몽키 시험(E1128·E1134·E1165, 죽은 Gump 정리 수정 포함), 승패 안내 뒤 흐름과 진행 저장·다음 미션 해제(E1152·E1161).
  - 남음: 실제 자연 승패에 따른 진행 저장·자연 완주, 한국어 전체 확인. (E1263 점검: 둘 다 사용자 확인 범위(L5·3.7절 6차)라 코드로 더 줄일 일은 없다.)
  - 근거: [모든 미션 시작](docs/webpj-all-missions-start-20261120.md) · [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md) · [시계 정지 누수](docs/webpj-mission-leave-pause-20261121.md) · [Gump 정리](docs/webpj-gump-cleanup-frame-20261121.md) · [몽키·미니맵](docs/webpj-monkey-minimap-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)

완료한 체크: **도움말·Tell**(QuickHelpWindow·TellFileMenus E1197, ExternalHelpClient E1181, HtmlLinkText E1212, QuickHelpControl E1237, QuickHelpHtml·QuickHelpRequest E1240까지 해당 대응표 행이 모두 port, HtmlConditionEffect·HelpLinkPolicy 대응표 port, RawNsInfoContext 는 NSInfo 범위 밖 제외 E1182; 생산 메뉴 Help·TYPE 정보 본문·요구 에너지 그림·도움말 그림 E1046·E1048~E1055, 링크 이동·Back 의 TYPE 조회 E1229, HUD 우클릭 빠른 도움말 E1236, 월드 없는 메인 메뉴 Help 의 TYPE 정보 E1240; [근거](docs/webpj-main-menu-help-type-20261123.md) · [E1197](docs/webpj-help-tell-leftovers-20261122.md)), **동영상**(E1202 사용자 결정, CD 전용 인트로 영상을 웹 빌드에서 제외하고 다섯 단위를 이식 대상에서 뺌, [결정 기록](docs/webpj-intro-movie-decision-20261122.md)), **미션 수명**(튜토리얼 완료·정지·Retry/Next/Abort·메뉴 복귀·진행 저장·데모 반복, E1085~E1161; 자연 승패 뒤 저장→새로고침→해제는 L5 사용자 확인, [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md)).

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다. E945에서 Windows의 Chromium·Chrome·Edge·Firefox로 하위 경로의 정적 모듈·자산 로드와 같은 URL 새로고침 뒤 음량 설정 복원을 확인했다. HTTP 캐시, GUI Linux와 실제 미션 진행 저장은 별도 검증한다. [E945](docs/webpj-static-deployment-20261119.md)

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`·`session-dump`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 웹은 검사 전용 고정 시각 모드(`?fixed=25`, `stepFixedFrames`, E1271)로 같은 프레임을 만든다. L2는 `e2e/fixed-time.spec.js`(E1283 닫음), L3는 같은 단계표를 웹에 적용하는 `e2e/l3-tutorial1.spec.js`(E1284)이며 웹 입력은 클릭 N+1·키 N−1 프레임에 넣는다. 난수 소비 순서는 cpp 임시 난수 추적(호출 스택 RVA와 링크 맵)으로 대조할 수 있다(E1280). L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md) · [난수 추적](docs/webpj-state-install-rng-20261123.md) · [L3](docs/webpj-l3-tutorial1-20261123.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 제품은 1326(1327 M8 관문)이며 웹 비교 기준선은 제품 1313이다. 1314 최초 실행과 1320 미션 전환을 웹에 반영했다(E616·E620). 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목(1316~1319·1321·1324)은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 소스 영향 판정은 3.6절에 있고, 1323 아이콘은 favicon(0.6절)으로 따로 반영했다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·특정 적 공격·승리·전체 화면/음향은 1절 범위에 따라 확인한다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- 웹 링크 브라우저 검증은 E942에서 마쳤다. Chromium에서 외부 출처 탭의 `opener`와 `referrer`가 비어 있고, 브라우저가 팝업을 차단해도 현재 문서가 유지된다. [E942](docs/webpj-external-access-20261105.md#71-브라우저-검증)
- 와이드 전투 시야와 카메라 재배치를 연결할 때 16:9의 논리 크기 전환도 검증한다. WebMCP 대상 브라우저·조작/캡처/녹화 범위는 착수 시 확정한다.
- **결정 대기(2026-11-23 저녁 정리 기준).**
  - 채팅 로그를 미션 끝에 초기화하는 적응(E1170)을 인정할지, 원본처럼 Client 수명 동안 유지할지(5-C 메뉴·버튼 컨트롤).
  - DestroyPatch 진단 열두 지점을 원문처럼 표시 후 계속으로 바꿀지(현재 보고 후 중단, 4D-8a·E1273).
  - 16:10 대조를 위해 웹 16:10 논리 크기를 cpppj 1280×800에 맞출지(5-C M2, 와이드 해상도 요구와 함께 판단).
  - 한국어 고해상도 글자 층(글자만 출력 해상도로 외곽선 글꼴 덧그리기, E1288 제안)을 채택할지. 굵기는 보통체 유지로 결정됐다(0.7절).
  - 프레임 예외 한 번에 렌더 루프가 멈추는 설계를 유지할지(3.7절 7차).
- **원본 실행·사람 확인.** 예산 칸 실제 우클릭의 원본 동작(5-C, AGENTS 실행 규칙), favicon 탭 그림(0.6절), 주변음·효과음·음악 실제 청취, 한국어 사람 검수와 미션 본문 줄 나눔 실제 화면.

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다. cpppj 1322(`635906357`, 갈무리11)·1325(`710b9647b`, Reset All)·1326(`baf141ebe`, 테스트 AI 진단)의 변경 아홉 행은 E1241에서 판정해 base를 `63590635`·`710b9647`·`baf141eb`로 올렸다. ② 재반영은 RawDamageableHit의 파생 피해 진단 문구 하나(`core/raw-damageable-hit.js`)이고, ③ 두 건은 E1251(Reset All 게임 안 Tell 확인 창)·E1261(BitmapFont)에서 처리했다. 2026-11-23 저녁 정리 때 `node webpj/tools/port-map.mjs`의 변경 보고는 0행이었으나 cpppj 1328~1334 변경 파일이 새로 보고됐다: `ProductPresentation.cpp`는 E1289(버전 창 11.01 패치·Copyright, base `5a6c2d4d`), `RawGameMenus.cpp`는 E1290(프레임 속도 라벨 번역, base `840d92a8`), `UberGump.cpp`·`UserInput.cpp`는 E1291(방향키 카메라 이동, base `d2bcefeb`)에서 ② 반영했고, 남은 `KoreanResources.cpp`·`StyleTextBuffer.cpp`·`HtmlTextParser.cpp`·`Markup.cpp`·`StyleTextMarkup.cpp`는 일본어(13차) 작업이라 그 단계에서 판정한다. 대응표 밖 새 파일 EmbeddedBitmapFont·KoreanFontSettings·ResetAllSettings 다섯 개는 모두 ①이며 재기준선 때 등록한다. [판정표](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md) SoundMusicSelection 행의 base는 기준선보다 앞선 `50ca1acf`였으나 그 뒤 두 커밋(`225f2322c`·`07885f694`)이 `SoundMusicSelection.h`의 주석·멤버 설명만 바꾼 것을 확인해(웹 영향 없음) 2026-11-22에 base를 `225553df`로 바로잡았다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | (E1263 점검, 저녁 정리 갱신) 이탈 State 정리 효과(E1268, 4D-5b)·Client 기본 수신기 표(E1274, 4D-3b)·분리 Poll과 `pollClientConnectionHealth` 미호출(단독은 하트비트 외 관측 효과 없음으로 확정, 4D-7b)은 닫혔다. 남은 것은 RawClockSynchronization의 CRT 동률 RTT 동등성(원본 필요)이다. PacketDelays(실소켓)·RawPacketCopies(LAN)·RawMonitorTime(옵션 UI가 없는 숨은 키 `monitor`·`alert`)은 도달하지 않는다. 아비어리·공용 Form writer 경계는 E594·E1210으로 대부분 닫혔고 확인하지 못한 칸이 남았다(확신 낮음). |
| 4D-8 | DestroyPatch 진단 열두 지점·`rememberPlacement`는 E1273에서 판정했고(표시 후 계속은 사용자 결정) 기본 수신기는 E1274에서 닫았다. 나머지 Native 경계는 E1193~E1221에서 port로 끝났다. SidPool 소진 회수·진단과 GameAuxState의 새 전역 칸은 코드와 행이 있으나 소비 여부를 직접 확인하지 못했다. |
| 5-A~5-C | 메인 메뉴 Kencloud 구름 배경(5-A). ‘Creating Islands.’ 로그인 진행 안내는 앱이 `creatingIslands` 훅을 소비하지 않는다. 원문은 `RawMissionServerPackets.cpp:60-63`의 `createArchipelago`(Zone 0x100 비트·이름 `$`) 경로에서만 부르므로 단독·스커미시가 이 경로를 타는지 판정이 남았다(확신 낮음). ESC Tell 닫기(E1264)와 메타 Sprite 제외(E1203)는 끝났다. |
| 6~7 | (E1263 점검) 건설 ActualConfirm/ActualPlace(E1139 e2e)·동맹 협상 입력(`registerAllianceNegotiationCommands`, `player-menu-actions.spec.js`)·RawGunHit Rain override(`raw-gun-hit.js`)는 끝났고 F7/Ctrl+F6 건설 기록은 개발자 전용으로 제외했다(E1242). RawPathRuntime/RawPathData 는 두 행 port 이고 사제·운반·경로 바인딩 항목이 이어받는다. NB20 실제 취소는 E804에서 검증했다. |
| 8~10 | (E1263 점검) TellSection(행 port, E461·E971)·Frame Rate·Difficulty·NB10(`frame-rate.spec.js`·`difficulty.spec.js`, E1096·E1149)·스커미시 일반 시작·중앙섬·자기 Fort(E1103~E1107·E1262)는 끝났다. 한국어/일본어 자원 우선순위·입력은 13차(일본어)와 한국어 사람 검수가 남았다. |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했고 Native 대여는 E1193~E1194에서 끝났다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | E943에서 16:9 논리 화면을 최대 1920×1080까지 지원하고 창 크기 변경과 입력 좌표를 확인했다. 전투 카메라·HUD와 WebMCP 조작·화면 캡처·소리 포함 녹화가 남았다. |
| 5 | localStorage 설정·진행 저장과 오류 대응을 구현했다. 실제 미션에서 진행값이 발생하는 8단계와 15차 초기화에서 저장 수명을 확인한다. |
| 6 | 메뉴·Tell·도움말과 갈무리11, 튜토리얼·캠페인·데모·훈련 미션 대본 전부(E1108~E1116)와 게임 안 메뉴·객체 메뉴 문구(E1118·E1140~E1141·E1166)까지 구현했다. 생산 창·스톰 파워 도움말과 채팅 채널 이름(E1246, 미니맵 도움말은 원본이 번역하지 않음), 미션 화면 문구 15개(E1249), 채팅 도우미 라벨 59개(E1250)를 더했고 글자 품질은 E1244·E1254·E1259~E1261·E1288에서 바로잡았다([0.7절](#korean-font-change)). 사람 검수와 실제 화면 확인(미션 본문 줄 나눔, 5-C M2 한국어 오라클)이 남았다. |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. 실제 미션 수명에서의 오류 진단이 남았다(프레임 예외 한 번에 렌더 루프가 멈추는 설계를 유지할지의 결정 포함, 4D-7b). [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | 10.78 알려진 버그 중 웹 적용분을 cpppj 판정과 대조한다. NB04/29 간헐천 계산·callback, NB18 요청/수신 관문과 NB20 실제 Session 선택/취소는 선행했다. 실제 스커미시와 브라우저 조작 검증은 남았다. |
| 11 | 쉬움/보통/어려움(기본 보통). 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반. E554·E555 계산과 AiCreate 연결, E1096 Options·ESC 메뉴 Difficulty 선택과 저장을 마쳤다. E1149에서 실제 스커미시로 난이도별 AI 다리·섬 건설 속도(쉬움 ≪ 보통 < 어려움)를 확인하고 옵트인 e2e로 고정했다([난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)). 쉬움의 채취 유닛 절반(최소 1)은 실제 진행에서 편차가 커 구분되지 않아(E1156, AI가 프레임 구동이라 빨리 감기 부적합, [빨리 감기](docs/webpj-fast-forward-clock-20261121.md)) 설정 단위 Node 시험과 AI 생성 연결을 근거로 둔다. |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천. Outpost 목록 수명·분류, 규칙·설정 화면·저장·시작 섬 fort 묶음·섬 생성 부모·세션 월드 생성과 시작·어려움 AI 진행·금지 유닛 반영(E1101~E1107), AI 생산·공격과 사제 사망 패배 안내의 실제 시간 관찰(E1262), 미션 모드라 비미션 너겟 배치·보충이 도달하지 않고 간헐천은 무작위로 놓임(E1270)을 마쳤다. 승리 경로와 AI 다리 멈춤 시점의 원본 대조가 남았다([다음 작업 6](#next)). [규칙](docs/webpj-skirmish-setup-20261120.md) · [UI](docs/webpj-skirmish-ui-20261120.md) · [시작](docs/webpj-skirmish-start-20261120.md) · [관찰](docs/webpj-skirmish-watch-20261123.md) |
| 13 | 일본어 등 추가 언어. WOFF2가 없는 글꼴은 Google Fonts CDN을 사용하고(AGENTS.md가 가리키는 `fonts/webfonts.css` 예시는 `96d81381`에서 추가됨) 외부 접속 문서와 소스 감사를 함께 갱신한다. 일본어 Patch Credits에도 카피라이트를 넣는다. |
| 14 | 웹 자체 JS 사용으로 QuickJS 탑재 불필요. **2026-11-23 사용자 결정:** dotnetpj가 QuickJS로 제공할 모드 API를 webpj 이름 체계의 공통 계약으로 만들고 webpj가 기준 구현이 된다([모딩 API](#modding-api)). |
| 15 | Reset All은 E863에서 구현·검증했고 E1251에서 확인을 브라우저 `confirm()` 대신 게임 안 Tell 확인 창(옵션 최하단 → `Patch1101.ResetAll` → 확정 단추 `Patch1101.ResetAllConfirmed`)으로 바꿨다. 확정 뒤 현재 게임 소유 설정·진행 키만 지우고 앱 정리 후 다시 로딩한다. 취소·확정·가드·오류·최초 실행과 다른 앱 저장값 보존을 검사했고 Chromium 실제 메뉴(영어·한국어)에서도 확인했다. 영구 저장 키가 추가되면 초기화 목록도 갱신한다. 일본어 본문은 13차에서 더한다. [E863](docs/webpj-reset-all-20261119.md) · [E1251](docs/webpj-reset-all-tell-20261123.md) |

- **12차 Outpost 잔여.** 배치/거리 후보 목록의 실제 소유 관계 확인, 지역 소유 통지·Factory/연구/운반·AI·전투·Client 연결, 건설·금지 목록·삭제를 확인한다. 일반 캠페인/데모에서의 제외도 확인한다. [Outpost 부품](docs/webpj-raw-outpost-lifecycle-20261115.md) · [이관 전 범위](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)

<a id="modding-api"></a>

- **14차 모딩 API 기준 구현(2026-11-23 사용자 결정, 계약 설계 뒤 착수).** dotnetpj가 QuickJS로 제공할 모드 API는 webpj의 함수·클래스 이름 체계를 따르는 공통 계약이며 webpj가 기준 구현이다([DN 3.1](LEFT_JOBS.dotnetpj.md#dotnet-modding)). 계약 설계(DN MOD-1) 뒤 webpj도 그에 맞게 수정할 수 있다. cpppj 모딩은 아직 진행하지 않고, 게임 로직을 JS로 옮겨 dotnetpj가 webpj 코드를 재사용하는 안은 채택하지 않았다.
  - [ ] 공개 계약 층 — `Raw*`·원문 주소식 내부 모듈을 그대로 노출하지 않고 계약 v0의 이벤트·읽기 전용 조회·명령(원본 입력 큐 경유)을 감싸는 얇은 층을 둔다. 완료 기준: 계약 문서의 모든 항목이 webpj에서 동작하고 내부 모듈 직접 노출이 없다.
  - [ ] 이름 정렬 — 계약에서 정한 이름과 다른 webpj 클래스·함수는 계약 이름에 맞춘다(내부 이름 변경이 크면 단위로 나눈다). 완료 기준: 계약 이름 목록 대조 시험 통과.
  - [ ] 모드 로드 — 브라우저에서 모드 스크립트를 불러오는 방식(파일 선택·localStorage 보관, 5차 대응)과 기본 꺼짐. 외부 URL에서 불러오는 기능을 두면 외부 접속 문서화 규칙을 따른다. 완료 기준: 켜기·끄기·오류 격리 e2e 통과.
  - [ ] 공통 적합성 시험 — 같은 모드 스크립트를 webpj와 dotnetpj에서 돌려 결과를 비교하는 시험 묶음. 완료 기준: webpj 쪽 시험 통과와 DN MOD-4 대조 결과 일치.
  - 근거(2026-11-23 검토): QuickJS가 webpj ES 모듈을 고치지 않고 실행한다. QuickJS는 V8 JIT보다 약 41배 느려 모드 API는 이벤트·명령 단위로 한다. Math 함수 double 결과의 엔진 차이는 DN이 fdlibm 구현으로 맞춘다.

### 3.8 원본 관측 연결 이월

완료한 체크: **입력·장치**(E1269, 화면 위치 키·D 키·객체 명령·회전 키·Pause 키 분배기 연결 E1150~E1163, `raw-priest-placement-*` 6개를 Session에 설치해 갇힌 사제 해방 경로의 오류를 고침, E1163 미판정 모듈 판정 완료, [사제 배치](docs/webpj-priest-placement-session-20261123.md) · [화면 위치 키](docs/webpj-camera-keys-20261121.md) · [Pause 키·미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md)), **렌더러·음악**(E1263, `renderer-projection-x86.tsv` 90행·`scenemusic-x86.tsv` 3,113행을 `renderer-collection-geometry.test.mjs`·`scene-music-native-fixtures.test.mjs`가 소비).

완료한 체크: **채팅 허브**(E1169~E1175, `chat-callbacks`·`chat-input-recipients`를 실제 Client/RawMenuControls·RawChatRuntime과 연결, [채팅 조립](docs/webpj-chat-window-assembly-20261122.md)), **점 명령/HTML**(E1192, HTML `$Input=`은 전역 tell의 길드·관리자 섹션 전용이라 SaveFortNameDialog·RawMenuSaveFort의 남은 가지를 웹 범위 밖으로 판정, [근거](docs/webpj-html-input-scope-20261122.md)).

[원본 fixture 감사](docs/webpj-fixture-audit-20261115.md)

## 4. 남은 차이와 검증 한계

| 영역 | 후속 확인 |
| --- | --- |
| 지도·지형 | 중립색·가운데 카메라·고정 구름은 개발 미리보기 정책이다. 실제 지도에서 owner·원소 투표/themeMode·도장/flood·카메라 제한/스크롤·공유 난수·색 번호·팔레트/ColorSort와 `useBanners`를 확인한다. 절벽 외 0x10/0x18 지형 선택 및 회전2/3의 실제 저장 사례도 남아 있다. |
| edgeFarm·보상/SP | edgeFarm의 spots/finder·MayPlace·Retype를 실 월드와 맞추고, 보상/SP의 재시드·실제 지갑·알림/소리·보상/후처리 장부 공유는 4D-8·6단계에 연결한다. [지형 추가 관측](docs/webpj-terrain-extras-20261102.md) · [보상과 Content 수명](docs/webpj-content-lifetime-20261107.md) |
| 메뉴·입력 | 구름 비교 차이는 4:3의 37,054픽셀이다. Kencloud 연결 뒤 M1 시나리오를 갱신한다. Help/Options 위치·와이드 배치, Overlay 드래그/캡처·미션 커서는 미확인이다. Edge Scroll(가장자리 스크롤)과 Auto-Demo(메인 메뉴 45초 무활동)의 실제 동작은 E990~E1001·E1092에서 확인했다([가장자리](docs/webpj-app-edge-scroll-frame-20261120.md) · [데모](docs/webpj-demo-start-20261120.md)). |
| 도움말·Tell | `<info>` 머리글/능력치, `#`/`&` 맞춤·색 치환, mana.8~11의 실제 원본 F1 모습, `<background>`/nsisles 사용, `{@미션.키}`·스크립트 도움말 Back 및 필요 시 두 열 메뉴를 확인한다. 도움말 그림은 레이어0/그림자 없음 경계를 유지한다. |
| 소리·환경 | 실제 청취·위치 음향, 다양한 브라우저·GUI Linux·고DPI/모니터 이동은 미확인이다. |
| Credits | 영어 AI 문구 `OpenAI Codex, ChatGPT and Anthropic Claude Code`와 한국어 문구는 사용자 수정 지시까지 유지한다. 수정 시 `www/assets/product.js`와 `tests/product.test.mjs`를 함께 갱신한다. |

## 5. 검증과 완료 판정

| 단계 | 판정 |
| --- | --- |
| L1 모듈 | 원본 10.78 x86 fixture와 cpppj 검사로 반환값·슬롯·정수/float/double 비트·부작용·난수 순서를 대조한다. 체크섬 비교와 바이트 비교를 구분한다. |
| L2 시작 상태 | 같은 Fort·언어·옵션·시드 및 Load/Start 시점의 Session 덤프와 논리 화면을 비교한다. |
| L3 입력 진행 | 같은 프레임·입력 단계표의 상태·보고서·화면을 비교한다. 표시 주사율·비동기 자산 로딩과 게임 시계/난수를 분리한다. |
| L4 화면·소리 | L4a는 논리 화면과 cpppj snapshot, L4b는 소리 시점·곡 선택과 MCP 녹화를 비교한다. 전체 외관과 청취는 사용자 확인 범위다. |
| L5 자연 플레이 | 사용자가 자연 승패 → 새 완료값 저장 → 새로고침/재실행 → 다음 미션 해제를 확인한다. |

실제 브라우저·OS·논리/물리 해상도·DPI·음향/저장 환경과 검사 범위를 기록한다. 원본 fixture를 덮어쓰지 않고 미연결 장치, 웹 적응과 누락 fixture를 단위 문서에 남긴다.

## 6. 작업 명령과 참고

```powershell
npm --prefix webpj start
npm --prefix webpj test
node webpj/tools/port-map.mjs
node webpj/tools/fixture-manifest.mjs
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
node webpj/tools/screen-compare.mjs --diff <폴더>
npm --prefix webpj run check-assets
npm --prefix webpj run test:e2e
```

- 개발 서버는 `http://localhost:9803/`이다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 규칙 없는 개발용 미리보기다.
- 필요한 항목만 `node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>`로 갱신한다. 자산 변경 시 `npm --prefix webpj run prepare-assets`를 쓴다.
- Node v22는 HJOW-Athlon의 webpj 폴더에서 `& "C:\Program Files\nodejs22\node.exe" --test "tests/*.test.mjs"`로 순차 실행한다.
- [이식 대응표 사용법](docs/webpj-port-map-20261106.md) · [실행 자산](docs/runtime-resources.md) · [형식 목록](docs/formats/README.md) · [공유 명칭](assets/unit-names.korean.json) · [cpppj 소스 지도](cpppj/SOURCE_MAP.md) · [외부 접속 정책](docs/webpj-external-access-20261105.md)

## 7. 정리 기록

**2026-11-23 저녁.** E1288 직후 문서 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 이관했다(AGENTS.md `1fc63cb1` 규칙: 만료 내용 이관과 요약 유지·완료 미표시 점검·큰 단위 분할).
- **뺀 것(보관본에 원문):** 다음 작업 1의 닫힌 ①~⑧·⑩ 연쇄와 2~6의 단위별 경과, 4-D 닫힌 체크 23개의 점검·완료 하위 목록과 4D-10b의 E1271~E1282 경과, 0.2절 partial 행 수 연쇄(34→12), 5-B·5-C·6·8·3.8절 닫힌 체크의 본문, 0.1절 회귀 이력(E1276~E1282), 오전 정리의 7절 기록.
- **새로 넣은 것:** 0.1 진행 요약의 E1238~E1288 주제 행(감사 잔여·L2·L3·원본 캡처·한국어 글자), 0.2절 partial 행별 닫는 조건, 0.7절 글자 품질과 사용자 결정(E1288), 3.5절 결정 대기 목록, 세부 단위(4D-4b, 5-A 두 항목, 5-B Cursor, 5-C 메뉴·버튼 컨트롤과 M2/L4a, 6단계 네 항목, 7단계 두 항목).
- **바로잡은 것:** 0.3절 `client/` 414/433·partial 19 → 422/433·11, 재기준선 분배 8단위/12,515줄·4단위/233줄 → 10/12,616·2/132, fixture manifest 840 → 863항목, 회귀 주기 v26 1/4·v22 3/8 → 2/4·4/8(E1288 미반영분), 최근 전체 e2e E1147 → E1281, 4-D 머리 문단의 '83개' → 84개, 0.1 동일성의 'L2·L3·L4 화면 비교 미검증' → L2 닫음·L3 진행, 3.3·3.6절의 낡은 '4D-10b 남음'·③ 결정 대기·4D-3b/4D-8a 잔여, 7단계 AI 세션 결선의 'DestroyPatch 바꾸는 일' → 사용자 결정, 0.1 요약표 '남은 것' 칸의 끝난 항목(채팅 창 공장·World 교체, HUD 표시 차이, Tell 모달 스택·ESC, 스커미시 AI 생산·공격·너겟, 미판정 모듈, 대기실 곡 등).
- **세부 `[x]`:** 5-B Cursor의 '카메라 투영·커서 표시'와 5-C M2/L4a의 '영어 4:3'·'16:9'는 L2·L3 0픽셀(E1280~E1286)을 근거로 세부 완료로 적었다. 상위 체크와 머리 수는 바꾸지 않았다.
- **사용자 제보 기록(2026-11-23 지시, 구현 보류):** 주문(스펠) 사용 시 스크립트 오류·일시 정지와 골렘 사제 포획 불가를 7단계 새 미완료 항목(세부 단위 셋)과 다음 작업 7로 넣어 7단계 머리 수를 3/6에서 3/7로 바꿨다.
- 다음 작업은 1~8로 다시 매겼다. 단위 문서가 가리키는 이전 번호(다음 작업 1 ⑤·⑦·⑧·⑩, 3, 5 등)는 보관본 0.1절에서 찾는다.

현재 AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음), `11b16b77`(webpj favicon, [0.6절](#favicon))과 `1fc63cb1`(LEFT_JOBS 정리 요청 시 만료 내용의 old 이관·완료 미표시 점검·큰 단위 분할 — 이번 정리에 적용)까지 반영했다. `96d81381`은 Noto Sans KR 글꼴 파일과 AGENTS.md가 가리키는 `fonts/webfonts.css`(Google Fonts CDN 예시)를 더한 것이며 한국어 게임 글꼴 규칙은 갈무리11 그대로이다(3.7절 13차 참조). 오전 정리(E1237 이관)와 그 이전의 이관 기록은 보관본의 날짜별 원문 7절에서 확인한다.
