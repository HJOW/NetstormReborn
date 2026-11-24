# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 [E1329 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)과 그 아래 날짜별 보관본을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → 3.1(단계별 항목) → 0.5(단위 착수·종료 절차) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 항목으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-25 정리, 마지막 단위 WB04-E1334)

- **실행·플레이**
  - **화면:** 브라우저 ‘클릭하여 시작’(첫 적재 뒤 최소 6초, E1308), 시작 title 그림, 메인 메뉴·캠페인/데모 목록·도움말·옵션(Reset All 게임 안 확인 창 포함)·Credits와 개발용 지형 미리보기가 동작한다.
  - **미션:** 캠페인 목록 31개(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)와 데모 3개가 시작되고, 장시간 진행과 임의 조작(몽키) 시험에서 오류·먹통이 없다. Campaign ▸ Skirmish는 무작위 섬·간헐천 월드에서 AI와 진행되며 AI 생산·공격으로 사제가 죽으면 패배 안내까지 이어진다(E1262, 약 18분 관찰). Tutorial2는 시작부터 완료 창·Tutorial3 시작까지 자연 진행으로 확인했고, 승패 안내 뒤 흐름(Continue·Replay Mission·Leave/Next Mission·진행 저장)도 확인했다. 미션 프레임은 다리 붕괴 검사·전투 자원·제단 결과 알림·주변음/공유 바람·장면 음악·미니맵 갱신을 원본 주 루프 순서로 돈다(E1230~E1234·E1266).
  - **조작:** 사제 선택·이동, 가장자리·Alt·가운데 버튼 스크롤, 미니맵 클릭·끌기, 생산 격자 클릭·단축키(D 포함)·조각 회전(C·회전 방향 옵션)·다리 배치, 사제·워크샾·제단·신전·배터리·발전기·운반체 우클릭 메뉴(회수·기도·Cast/Drop·Carrying/Drop·폭탄 Detonate!), 객체 명령(Detonate·Meltdown·신전 명령), 주문 시전과 주문 보유 골렘의 적 사제 포획·Drop(E1318~E1319), 화면 위치 키(F4/H·F5·TAB·U·Shift+숫자 저장/숫자 복귀)와 방향키 카메라 이동(32픽셀, 조각을 든 ←/→는 회전, E1291), 미니맵·스톰 파워 표시(원본처럼 About, E1324)·생산 창·채팅 도우미 단추의 우클릭 빠른 도움말이 동작한다.
  - **게임 메뉴:** ESC 또는 화면 우측 상단 버튼(웹 전용)으로 Game·View·Options·Players·About 메뉴막대를 열고(열 때 원본처럼 SP 검산 회전, E1327), Leave/Restart Mission(ABORT 확인 창), F1·F2·F6·F7·F8·F9·Shift-F3·T, Pause(Shift-F9·Pause 키·메뉴), Shift-F7 화면 GIF 저장, Players 메뉴의 송금(Send X 숫자 입력 창 포함, 수신 안내 E1326)·동맹이 실행된다. ESC는 열린 Tell 창(브리핑·로딩 제외)을 원본처럼 닫는다(E1264). 버전 창의 11.01 패치·`Copyright 2026 HJOW` 줄(E1289)과 한국어 메뉴 전수 점검(E1290)을 마쳤다. 브리핑·완료 창의 Review Knowledge·GetTechnology 단추와 Priest Training 7의 Training Menu(원본 오타 별칭, E1245)도 동작한다.
  - **HUD 틀·채팅 창:** 실제 Uber 루트가 좌측 사이드바(SP·생산 덱)·하단 미니맵 칸·구분선을 원본 위치로 도킹하고 지도 영역이 `{85,0,W,H}`다(4:3·16:9·16:10·1920×1080·창 크기 변경 확인). F3 채팅 창의 로그·입력줄·제출(로컬 서버 경유 `You> 본문`)·점 명령·Tab 이름 완성(동맹 AI)·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창이 동작하고(E1169~E1179) 로그는 원본처럼 앱 수명 동안 유지된다(E1320).
  - **미완성·범위 밖:** 다른 미션의 목표 수행과 자연 완주가 남았다. 지식 NSInfo 패널·Players 하위의 Ignore chat·Whisper To·정보 보기·의식 보상 선택 UI·인트로 영상은 웹 범위 밖이다(E1167·E1171·E1180·E1202).
- **원본과의 동일성**
  - **M1 메뉴:** 영어 4:3 메뉴 8장면은 cpppj 오라클과 알려진 차이 영역(Campaign 창 크기·Options 팝업 위치) 밖에서 픽셀 차이 0이고 구름 배경도 같다(Kencloud 캐시, E1298).
  - **L2 시작 상태(E1283 닫음):** 검사 전용 고정 시각 모드(`?fixed=25`)에서 Tutorial1 Load/Start의 Game 난수열(90프레임 6,851회)·논리 화면·Session 덤프가 cpppj 기준선 오라클과 같다. 4:3·16:9(1366×768)·16:10(1280×800, E1322)의 90프레임 화면 0픽셀, 90·130프레임 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다르다. 한국어(E1323)는 상태가 같고 첫 안내 창 밖 0픽셀이며 창 안 차이는 웹 독립 번역·글꼴 차이로 분리 확인했다([근거](docs/webpj-korean-m2-20261124.md)). 31개 미션의 시작 5·30·120프레임 덤프도 모두 같다(E1313).
  - **L3 입력 진행:** cpp `--ui-script` 입력 단계표를 그대로 적용해 Tutorial1 시작~'Mission Accomplished!' 완료 창 4,200프레임(체크포인트 20곳, E1284~E1286·E1299·E1301~E1304), Tutorial2 시작~완료 창 5,400프레임(체크포인트 41곳, E1305~E1307), Tutorial3(Capture The Priest) 시작~'Mission Accomplished!' 완료 창 및 Tutorial4 해제 6,200프레임(체크포인트 화면 31곳·덤프 20곳, E1309·E1329~E1333, [단계 A→B](docs/webpj-l3-tutorial3-stage-ab-20261124.md) · [단계 B→C](docs/webpj-l3-tutorial3-stage-bc-20261124.md) · [단계 C→D](docs/webpj-l3-tutorial3-stage-cd-20261124.md) · [단계 D~E→F](docs/webpj-l3-tutorial3-stage-def-20261124.md) · [단계 F~H 완료](docs/webpj-l3-tutorial3-complete-20261125.md)), Tutorial4 단계 A→B→단계 C `'Upgrading Workshops, Part 1'` 안내 창 진입 3,210프레임(체크포인트 화면 14곳·덤프 10곳, E1334, [단계 A~B](docs/webpj-l3-tutorial4-stage-ab-20261125.md))과 ESC 메뉴 여닫기 360프레임(E1327)까지 화면 0픽셀·덤프 일치다. 이 대조로 다리 붕괴 스캔 초기화 누락(`Reset422490`, E1301)·생산 패널 HUD 생성 순서(E1305)·로그인 중 간헐천 보충 기한(E1313)·메뉴 SP 추적(E1327)·주변음/장면음악 천둥의 `ThunderProcess`(TYPE 55) 미연결(E1329)과 `e2e/l3-common.js`의 `INJECT_SHIFT.right`(-1, E1333)를 고쳤다.
  - **부품:** 월드·UI 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다(원본 공통 메뉴 788행·Factory 메뉴 39행·Uber 루트 160행·장면 음악 3,113행, 채팅 입력줄 대상 720행(E1265), 파편·건물 캡처 4,474행(E1275~E1276)·골렘/받침 264행(E1312), 실제 Session의 EvilRay·Burning·섬 투하(E1314~E1316) 등). 대응표 1,007단위 중 732행은 Tutorial1 실행에서 웹 모듈의 함수가 모두 실행된 `session`이다(E1283). 좌측 HUD 하단 85×82픽셀은 원본 스크린샷과 일치한다.
  - **한국어 글자:** 갈무리11 12px 격자로 그리되 제목 배율·굵기(사용자 결정으로 보통체)·한글 기준선을 cpppj와 다르게 둔다([0.7절](#korean-font-change)).
  - **미검증:** Tutorial4 단계 C~I1·Tutorial5~6·캠페인 묶음 미션 L3 진행, L4b(소리)와 자연 완주.
- **현재 작업:** E1334에서 Tutorial4 단계 A~B(Wind Generator·Sun Disc Thrower 생산 등록 → 중앙 회랑 다리 + 무피해 Wind Generator 2기 → Thrower 3기로 양쪽 적 Sun Cannon 격파 → 단계 C `'Upgrading Workshops, Part 1'` 안내 창) 3,210프레임 L3 대조(화면 14곳 0픽셀·덤프 10곳 허용 행 외 차이 0)를 마쳤다. 다음은 Tutorial4 단계 C~E(워크샾 업그레이드·Sun Cannon 으로 적 발사기 6기 파괴 → 단계 E1 진입) L3 대조다. 순서는 [다음 작업](#next)을 따른다.
- **전체 회귀**
  - **Node 22.23.5 E1332:** 6,847/6,847 통과, 실패·취소·미실행 0(469.0초, 8/8 주기 갱신). 로컬 Node22 실행 경로는 `extracted/webpj-node22/node_modules/node-win-x64/bin/node.exe`다.
  - **Node 26.13.0 E1333:** 6,847/6,847 통과, 실패·취소·미실행 0(402.4초, 4/4 주기 갱신).
  - **주기(E1334 기준):** v26은 E1333 전체 뒤 1/4, v22는 E1332 전체 뒤 2/8.
  - **Chromium:** E1313 전 미션 시작 상태 옵트인 전체 32/32 통과(24.3분, `webpj/extracted/webpj-e1313-final-e2e.log`). 기존 전체 e2e는 E1312 199건 중 186 통과·12 옵트인 미실행·한국어 브리핑 1건 부하 중 시간 초과(단독·3회 반복 통과)다. 단위별 실패·교정은 [E1313](docs/webpj-l3-mission-starts-20261124.md)과 각 단위 문서, 이전 회귀 이력은 [E1327 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)을 따른다.
- **브라우저 검사:** 미션 화면은 단위마다 Chromium e2e로 확인한다(최근 전체 실행 E1312, 고정 시각 L2·L3 `e2e/fixed-time.spec.js`·`e2e/l3-tutorial1.spec.js`·`e2e/l3-tutorial2.spec.js`·`e2e/l3-tutorial3.spec.js`·`e2e/l3-tutorial4.spec.js`, 앱 서비스 결선 `e2e/session-services-wiring.spec.js` E1328, 한국어 글자 배치 `e2e/korean-glyph-layout.spec.js` E1288). 메뉴는 2026-11-17 Chromium 2/2(갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체), 최초 실행·진행 잠금/체크·저장 재실행은 E616 Chromium 2/2. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md)

<a id="next"></a>

#### 다음 작업

번호는 2026-11-24 첫 정리(E1327 이관)에서 1~6으로 다시 매겼고, E1328(`f6e6d8cd9`)이 1번(RawCampaignSession partial 닫기, [근거](docs/webpj-session-boundary-close-20261124.md))을 끝내 2~6번이 지금의 1~5번이 됐다(그 사이 번호는 커밋 `bcabc8025`의 문서). 단위 문서가 가리키는 더 앞 번호(다음 작업 0·0-a·1~9)의 원문은 [E1327 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327), 그보다 앞 번호(다음 작업 1 ①~⑩·2~6)는 [E1288 보관본 0.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다.

1. **다음 미션 L3 대조(4D-4b Tutorial4~6).** Tutorial1(E1304, 4,200프레임), Tutorial2(E1305~E1307, 5,400프레임), Tutorial3(E1309·E1329~E1333, 6,200프레임)는 시작부터 완료 창·다음 미션 전환까지 화면 0픽셀·덤프 일치로 끝났다([Tutorial1](docs/webpj-l3-tutorial1-complete-20261124.md) · [Tutorial2](docs/webpj-l3-tutorial2-20261124.md) · [Tutorial3](docs/webpj-l3-tutorial3-complete-20261125.md)). 같은 틀(`e2e/l3-common.js`·`tools/l3-collect.mjs`, 미션 이름·폴더만 바꿈)로 **Tutorial4~6**(전투·방어물·AI)과 세 캠페인 묶음 미션의 시작~완료를 대조한다. 이 확장이 4D-4b의 Tutorial 단계 진행을 겸한다.
   - 현재 위치: Tutorial3 단계 A→H 전 구간 6,200프레임 완료(E1333) 및 **Tutorial4**(`TacticalCombat`) 단계 A~B(3,210프레임, [단계 A~B](docs/webpj-l3-tutorial4-stage-ab-20261125.md)) 완료(E1334). 다음은 **Tutorial4** 단계 C~E(`Upgrading Workshops`·`Sun Cannon` 으로 적 발사기 파괴 → 단계 E1 진입) L3 대조이며, Tutorial4~6의 대본 단계별 세부 단위와 완료 기준은 3.1절 4D-4b 아래에 있다.
   - 좌표 기준(E1329): 1024×768 기본 카메라 `cameraX=1552, cameraY=749`, 1칸=16×11px, 생산 창 다리 조각 `60,44`(우클릭 회전)·골렘 `40,180`. 단계 A의 입력 순서 전체는 단위 문서 표와 `tutorial3-l3/steps.json`에 있다.
   - 방법: 폴더 `tests/fixtures/cpppj/candidate-225553df/tutorialN-l3/` 와 스펙 `e2e/l3-tutorialN.spec.js`(Tutorial2 스펙 복제). 입력 위치는 cpp 오라클을 5초 안팎으로 여러 번 돌려 정하고(격자 탐색 50~230회가 5~21분이라 백그라운드로), 단계는 cpp `report` 의 `mission_gate` 두 번째 값으로 판정한다. 전투가 있는 미션은 체크포인트를 짧은 간격으로 두고 어긋나는 첫 프레임을 `mission_gate`·`bridge_scan_global` 과 프레임별 덤프(`runL3({ compare: false })`·`L3_DUMP_DIR`)로 좁힌다([탐색 기록](docs/webpj-l3-extension-probe-20261124.md)).
   - cpppj 오라클 수집은 HJOW-Athlon에서 실행 확인 없이 가능하고 다른 PC는 AGENTS 실행 확인 규칙을 따른다(3.3절).
2. **다른 미션의 목표 수행(4D-4b).** Tutorial2([E1086](docs/webpj-tutorial2-complete-20261120.md))처럼 Tutorial3~6과 세 캠페인 묶음 미션의 단계별 목표를 수행해 완료 창까지 이어 본다. 너겟 납품(`0x020000b4`) 진행 조건은 Early Mission 3 단계 A를 E1329 L3에서 확인했고 6 단계 B가 남았다. 자연 완주는 1절 사용자 확인 범위다.
3. **12차 스커미시 나머지.** 비미션 너겟 보충은 스커미시가 미션 모드라 도달하지 않는다(E1270). [관찰](docs/webpj-skirmish-watch-20261123.md) · [게임 진행](docs/webpj-skirmish-play-20261120.md) · [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)
   - 승리 경로(적 전멸) — 완료 기준: 적 AI를 모두 없앤 스커미시에서 승리 안내와 그 뒤 흐름이 동작한다(실전 재현은 1절 사용자 확인 범위).
   - 개별 난이도/동맹의 N프레임 동작 정량 비교 — 완료 기준: 같은 시드·설정에서 AI별 다리/생산 주기를 프레임 수로 비교한 결과가 단위 문서에 기록된다.
   - AI가 다리를 멈추는 시점(관찰에서 소유자별 360~600초)의 원본 대조 — 완료 기준: 같은 조건의 cpppj 오라클 시점과 비교한 결과·차이 원인이 기록된다.
4. **결정·원본 확인 대기.** 사용자 결정과 원본 실행·사람 확인이 필요한 항목은 3.5절에 모았다.
5. **14차 모딩 API 기준 구현(사용자 결정 2026-11-23).** dotnetpj 계약 설계(MOD-1) 뒤 webpj를 기준 구현으로 맞춘다. [3.7절](#modding-api).

- **로컬 산출물.** `webpj/test-results-e1072-rerun`~`webpj/test-results-e1081`의 10개 폴더는 빈 폴더이거나 Git 제외 `*.zip` trace만 들어 있어 Git 상태에 나타나지 않는다(2026-11-24 정리 때도 남아 있음). 다시 볼 필요가 없으면 삭제해도 된다. E633~E640 단위 문서가 적은 루트 로그는 2026-11-21에 `extracted/webpj-e633-e640-root-logs/`로 옮겼으므로 그 폴더에서 읽는다.
- **미션 식별.** 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이고, 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이다(E753 재대조). [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md)

<a id="e951-e1081"></a>

#### E951~E1329 진행 요약

단위별 완료 문단·검증 수·회귀 이력은 [E1081 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081)의 0.1절(E951~E1081), [E1166 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)의 0.1·3.1·3.7절(E1082~E1166), [E1237 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)의 0.1·3.1절(E1167~E1237), [E1288 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)의 0.1·3.1절(E1238~E1288), [E1327 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)의 0.1·3.1절(E1289~E1327), [E1329 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)의 0.1·3.1·3.6절(이 표를 줄이기 전 칸 문장·E1328~E1329·완료 체크 근거)과 각 단위 문서에 있다.

| 주제 | 단위 | 연결한 것 | 남은 것 | 대표 문서 |
| --- | --- | --- | --- | --- |
| 미션 로딩·Start | E951~E958, E968, E970, E974~E976 | 로그인 HUD·진행 창, 최초 Fort 요청 자동 처리, Ready→Start→첫 안내와 시계 정지/재개, 공유 Kernel 프레임 | — | [로딩](docs/webpj-app-mission-loading-20261119.md) · [Start](docs/webpj-app-tutorial-start-frame-20261120.md) |
| 음향·음악 | E957, E959~E966, E997, E1059, E1061, E1119~E1120, E1153, E1238~E1239, E1258 | 효과음·음악 Web Audio 재생, 위치 음향·발소리, 장면 음악(순환·첫 곡 난수·180초 기한)·희생 의식 곡, Wind Noise, Sound On 끄기·천둥 경로, 승리/패배 곡 범위 판정 | 실제 청취 | [앱 음향](docs/webpj-app-campaign-audio-20261119.md) · [장면 음악 닫기](docs/webpj-scene-music-close-20261123.md) |
| AI·채팅·Tell·승패 | E967, E969, E971~E973, E1058 | AI 이름·난이도·채팅 기록, 미션 Tell 조회, Gump 팝업 차단·닫기, 승패 안내, NSInfo·도움말 팝업 저장소 | — | [AI](docs/webpj-app-campaign-ai-20261120.md) · [승패](docs/webpj-app-mission-outcome-20261120.md) |
| Renderer·화면 | E977~E989, E1081~E1082 | 지면·객체·입자·커서·Gump 수집과 출력, Screen DIB·구름 합성·Present, Gump 이동 dirty, 도킹된 지도 영역 | — | [출력](docs/webpj-campaign-renderer-drawing-20261120.md) · [Present](docs/webpj-app-renderer-present-20261120.md) |
| 미션 진입 | E985~E986, E1010~E1016, E1092 | Tutorial1~6·세 캠페인 묶음 진입·브리핑, 세 데모 시작·45초 Auto-Demo | 미션별 목표 수행·자연 완주 | [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md) · [데모](docs/webpj-demo-start-20261120.md) |
| 입력·카메라 | E990~E1001, E1009, E1084 | 선택·이동·포인터 폴링, 가장자리 스크롤·카메라 키(F4·P/R·F5·N), 조각 회전, ESC 메뉴 토글 | 여러 유닛 순환(Ctrl-F5·N은 사제 1명 상태로 확인) | [포인터](docs/webpj-app-mission-pointer-frame-20261120.md) · [사제 키](docs/webpj-browser-priest-shortcuts-20261120.md) |
| 생산·건설 | E1002~E1008, E1019~E1022, E1026~E1036, E1047, E1074, E1076~E1079, E1083, E1086 | 생산 패널·단축키·다리 배치, 회수·비용 표시, Factory 생산·업그레이드, Tutorial2 발사기 배치·회수 | 자연 완주(다리 조각 연속 배치는 L3 E1299~E1304) | [생산 패널](docs/webpj-client-production-panel-20261120.md) · [워크샾 생산](docs/webpj-browser-tutorial2-factory-20261120.md) |
| HUD 틀 | E1082, E1128, E1236, E1248, E1255, E1257, E1324~E1325 | Uber 루트 HUD 모드 3 도킹·화면 크기 변경 재배치, 미니맵 Gump·클릭/끌기 e2e, HUD 우클릭 빠른 도움말, 미니맵·HUD·생산 컨트롤 행 닫기, 스톰 파워 칸 우클릭 About(E1324)·예산 Gump 재부착(E1325) | — | [좌측 HUD 틀](docs/webpj-uber-hud-left-dock-20261120.md) · [HUD 닫기](docs/webpj-uber-hud-close-20261123.md) · [예산 우클릭](docs/webpj-budget-help-20261124.md) |
| 채팅 창 | E1169~E1179, E1253, E1265, E1295, E1320 | F3 표시·로그·입력줄·제출·수신·점 명령, Tab 이름 완성·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창, 우클릭 도움말, 허브 fixture, 로그 앱 수명 유지(E1320, 사용자 결정) | Format 입력창 아래 가로 슬라이더의 원본 화면 대조 | [채팅 조립](docs/webpj-chat-window-assembly-20261122.md) · [로그 수명](docs/webpj-chat-client-lifetime-20261124.md) |
| 게임 메뉴 | E1084~E1085, E1087~E1090, E1093~E1097, E1100, E1167~E1168, E1235, E1251, E1264, E1289~E1290, E1326~E1327 | ESC 메뉴막대·우측 상단 버튼, Leave/Restart, F6·F7·F9·T, Pause, Frame Rate·Language·Difficulty, Shift-F7 GIF, Players 하위(채팅·정보 항목은 단독 도달 불가)·Send X, F1·Tell/브리핑, Reset All Tell 확인 창, ESC Tell 닫기, 버전 창 Copyright, 한국어 ESC 메뉴, 송금 수신 MessageBoard(E1326), 메뉴 SP 검산 회전(E1327) | — | [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md) · [ESC Tell](docs/webpj-tell-escape-20261123.md) · [메뉴 SP](docs/webpj-menu-sp-tracking-20261124.md) |
| 스커미시(12차) | E1101~E1107, E1262, E1270 | 설정 화면·저장, 시작 섬·섬 생성 부모, 세션 월드 생성·앱 시작, AI 진행·금지 유닛, AI 생산·공격과 사제 사망 패배 안내 관찰, 미션 모드 판정 | 승리 경로·AI 다리 멈춤 시점 원본 대조 | [시작과 월드 생성](docs/webpj-skirmish-start-20261120.md) · [관찰](docs/webpj-skirmish-watch-20261123.md) |
| Gump·메뉴 UI | E1017~E1018, E1023~E1025, E1037~E1045, E1064~E1072, E1075, E1086 | Form 통지, 글자·Slider 출력, 중첩 메뉴, 사제·Player·공통 객체·운반체 메뉴, 신전 건설·완공 | — (객체 메뉴 잔여는 '객체 메뉴·명령' 행) | [Client 메뉴](docs/webpj-client-menu-gump-20261120.md) · [사제 우클릭](docs/webpj-browser-priest-menu-20261120.md) |
| 도움말 | E1046, E1048~E1055, E1229, E1240 | 생산 Help, TYPE 정보 창, 요구 에너지·도움말 그림, 링크 이동·월드 없는 메인 메뉴 Help의 TYPE 정보 | 타입별 원본 화면 비교 | [앱 TYPE](docs/webpj-app-help-type-info-20261120.md) · [메인 메뉴 Help](docs/webpj-main-menu-help-type-20261123.md) |
| 한국어 문구 | E1056~E1057, E1062~E1063, E1073, E1080, E1108~E1118, E1140~E1142, E1166, E1246, E1249~E1250 | 모든 미션 대본, 게임 안 메뉴·객체 메뉴·지식 문구와 줄 나눔, Player 이름 디코딩, 생산 창·스톰 파워 도움말·채팅 채널 이름, 미션 화면 문구 15개, 채팅 도우미 라벨 59개 | 실제 화면 확인(미션 본문 줄 나눔 등), 사람 검수 | [캠페인 미션](docs/webpj-korean-campaign-missions-20261120.md) · [미션 화면 15개](docs/webpj-korean-gameplay-xlat-20261123.md) |
| 한국어 글자 | E1244, E1254, E1259~E1261, E1288 | 굵은 글꼴 부족 문자의 일반 글리프 대체, 큰 슬롯 정수 배율·기준선, Tell 본문 폭 맞춤, 12px 격자·4배 그리기와 숫자·영문·공백의 원본 글리프 | — (고해상도 글자 층은 도입하지 않기로 사용자 결정, 2026-11-24) | [12px 격자](docs/webpj-korean-glyph-grid-20261123.md) · [제목 판독](docs/webpj-korean-title-legibility-20261123.md) |
| 조작키 | E1145~E1146, E1148, E1151, E1154~E1155, E1159, E1162~E1163, E1291 | Alt·가운데 버튼 스크롤, H, C·`rotmode`, TAB·U·Shift+숫자/숫자, D·F 키 점검, F3 구분선, Pause 키, 방향키 카메라 이동 | — | [Alt 스크롤·조작키](docs/webpj-alt-scroll-keys-20261121.md) · [화면 위치 키](docs/webpj-camera-keys-20261121.md) · [방향키](docs/webpj-arrow-camera-20261124.md) |
| 객체 메뉴·명령 | E1126~E1127, E1135~E1138, E1150, E1225, E1310~E1311, E1318~E1319 | Players 송금·동맹, 파생 객체·제단·운반체·Archer 메뉴, 메뉴 닫기·비활성 색, 객체 명령·정체 안전장치, 기도·Cast/Drop·Factory 순환 명령 소비, Detonate!·Carrying/Drop·너겟 줍기 e2e, 주문 시전(E1318)과 주문 보유 골렘의 사제 포획·Drop(E1319, cpppj 1339도 같게) | — | [파생 객체 메뉴](docs/webpj-object-menu-variants-20261120.md) · [메뉴 행 e2e](docs/webpj-object-menu-rows-20261124.md) · [포획](docs/webpj-spell-priest-pickup-20261124.md) |
| 장시간·실사용 점검 | E1121~E1125, E1128, E1132~E1134, E1139, E1143~E1144, E1147, E1158, E1160, E1165 | 전 미션 시작·장시간 진행, 몽키, 시계 정지 누수·죽은 Gump 정리 수정, 실사용·넓은 화면 e2e, 성능 측정 | 자연 완주 | [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md) |
| 승패·미션 흐름 | E1129~E1131, E1152, E1157, E1161 | 시작 덤프 구조 대조, 안내 창 배치, 승패 뒤 흐름·진행 저장, Priest Training 시작 연결 | — (L2·L3는 아래 행) | [L2 대조](docs/webpj-l2-tutorial1-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) |
| 분석·판정 | E1149, E1153, E1156, E1163, E1241 | 난이도별 건설 속도, Wind Noise, 빨리 감기 한계, 미연결 모듈 68개 분류, cpppj 1322·1325·1326 변경 아홉 행 판정(②는 파생 피해 진단 문구 하나) | 쉬움 채취 절반 실관찰 | [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md) · [미연결 모듈](docs/webpj-pause-key-unwired-modules-20261121.md) · [1322·1325·1326 판정](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md) |
| 시작 화면 안정화 | E1300, E1308 | 처음 방문의 요청 실패 재시도(파일·매니페스트 3회, 동시 12개, 번들 로더), 글꼴·메뉴 음악 받기(상한 20초)와 첫 적재 뒤 최소 6초(`?startwait=밀리초`, 자동화 브라우저 0)가 지난 뒤 `클릭하여 시작`, 언어 탐지 `navigator.languages[0]` 우선 | 배포본 재배포 후 실제 처음 방문 확인(사용자) | [안정화](docs/webpj-start-screen-robustness-20261124.md) |
| 번들 빌드 | E1293, E1297 | `index.html`은 webpack 운영 번들(E1329 3,433,729바이트, 9.9MB 기준 통과), `debug.html`은 번들 없는 ES 모듈 구동, 크기 한도·최신 검사(`tests/bundle-fresh.test.mjs`), 번들 Git 추적(사용자 결정 E1297) | — | [번들](docs/webpj-webpack-bundle-20261124.md) |
| 회귀 교정 | E1060, E1078 | 관측 자료 누락·메뉴 전용 번역 검사 범위, 외부 접속 감사의 비공개 필드 오탐 | — | [회귀 교정](docs/webpj-regression-translation-fixtures-20261120.md) · [접속 감사](docs/webpj-external-audit-private-field-20261120.md) |
| 웹 범위 판정 | E1167, E1171, E1176, E1180, E1182, E1187~E1188, E1192, E1202~E1204, E1238, E1242~E1243 | Players 하위 3항목·지식 NSInfo·의식 보상 선택 UI·HTML `$Input=`·패킷 원천/지연·메타 앞단·Sprite 갱신·FortCoreDataLoad·승리/패배 곡·건설 기록 재생·Uber 배경 자식을 웹 범위 밖으로, 인트로 영상은 사용자 결정으로 웹 제외 | — | [NSInfo](docs/webpj-nsinfo-scope-20261122.md) · [동영상 결정](docs/webpj-intro-movie-decision-20261122.md) · [건설 기록 재생](docs/webpj-build-replay-scope-20261123.md) |
| 대본·기술·타이틀 | E1184, E1189~E1190, E1197, E1199~E1200, E1227, E1245 | 시작 title 그림, Fury War 기술 지급, 대본 82개 명령 점검(`ViewSpot`·`Salvage`), 도움말·Tell 남은 두 행, Detonate·GetTechnology 원본 관측, Review Knowledge·GetTechnology 단추, Priest Training 7 Training Menu 별칭 | — | [대본 명령](docs/webpj-script-commands-audit-20261122.md) · [Fury War](docs/webpj-fury-war-technology-20261122.md) |
| Native 대응 대조 | E1193~E1195, E1205~E1221, E1273~E1274, E1321 | Native 고정 수신기·채팅 수신기, 건설 여섯 행·AssetDeletionVtables, RawSurface 훅 대조와 17종 가상 삭제·unit-lost 억제, RawSurfaceDeletion·Lifetime 판정, Client 기본 수신기 범위, DestroyPatch 열두 지점·0x771 진단 후 정상 복귀(E1321, 사용자 결정) | — | [수신기](docs/webpj-native-receivers-audit-20261122.md) · [RawSurface](docs/webpj-native-surface-audit-20261122.md) · [정상 복귀](docs/webpj-destroy-patch-continue-20261124.md) |
| 읽기 전용 감사·수정 | E1223~E1237 | Client 상태·입력(제단 희생 메뉴 훅)·UI 컨트롤(와이드 메인 메뉴 버튼) 감사, 객체 메뉴 명령 소비, 아이스 반사 0점 기본값, 도움말·음향·설정 partial 15행 닫기와 Tell Warning, 다리 붕괴·전투 자원·Player 결과 알림·주변음/바람 프레임, F1·HUD 우클릭 도움말·Back | — | [RawSurface 감사](docs/webpj-native-surface-audit-20261122.md) |
| 감사 잔여·partial 닫기 | E1240, E1247~E1248, E1252, E1255~E1257, E1261, E1263~E1270 | 예산 칸 우클릭 판정, 미니맵·메뉴·HUD·커서·생산·BitmapFont·ClientAudio 행 닫기, 미완료 46항목 전수 점검(20개 닫음), 프레임 순서, 입력 큐 코드 전수, 이탈 State 정리, 사제 배치 검사 Session 설치 | — | [점검](docs/webpj-left-jobs-audit-20261123.md) · [프레임 순서](docs/webpj-frame-order-20261123.md) |
| partial 마감(12→0행) | E1292, E1294, E1296, E1298, E1310, E1317, E1320, E1324~E1328 | port 승격: SessionLogin·ClientCampaignTransport·UberReconnect(E1292), RawCampaignRitual(E1294), CampaignClientServices(웹 적응, E1296), ClientRendererFrame·UberGump(Kencloud, E1298), CampaignObjectMenuClient(E1310), RawClockSynchronization(플랫폼 적응, E1317), RawMenuChat(E1320), RawButtonControls(E1324~E1325), RawCampaignSession(송금 MessageBoard E1326·메뉴 SP E1327·앱 서비스 5건 `kencloudGlobals`·`htmlWindowGlobals`·`factoryHelp`·`removeConnectionStatus`·`templateStateBytes` 연결과 공개 경계 전수 감사 E1328) | — | [partial 세 행](docs/webpj-port-map-partial-close-20261124.md) · [시간 동기화](docs/webpj-clock-sync-close-20261124.md) · [세션 경계 마감](docs/webpj-session-boundary-close-20261124.md) |
| L2·고정 시각 | E1271~E1272, E1278, E1280~E1283, E1322~E1323 | 고정 시각 모드(`?fixed=25`)·시작 그림 구간, cpp 난수 호출 추적으로 State 설치 맞춤, 월드 진입 Guide 정지, 4:3·16:9·16:10(1280×800, E1322) 오라클 0픽셀과 한국어 M2(E1323), 대응표 `session` 732행 승격 | — | [고정 시각](docs/webpj-fixed-time-mode-20261123.md) · [난수·시각](docs/webpj-state-install-rng-20261123.md) · [session 승격](docs/webpj-port-map-session-20261123.md) · [16:10](docs/webpj-aspect-16x10-20261124.md) |
| L3 | E1284~E1286, E1299, E1301~E1307, E1309, E1313, E1329 | cpp `--ui-script` 단계표의 웹 적용(안내 단추 클릭 N+1·키와 화면 좌표 클릭 N−1 프레임), Tutorial1 시작~완료 창 4,200프레임·Tutorial2 5,400프레임·Tutorial3 단계 A→B 1,700프레임의 화면 0픽셀·덤프 일치, 31개 미션 시작 덤프(E1313). 고친 차이: 다리 붕괴 스캔 초기화(E1301), 생산 패널 HUD 생성 순서(E1305), 로그인 중 간헐천 보충 기한(E1313), 천둥 `ThunderProcess`(TYPE 55) 연결(E1329) | Tutorial3 단계 B→H·Tutorial4~6·캠페인 미션 단계표([다음 작업 1](#next)) | [L3](docs/webpj-l3-tutorial1-20261123.md) · [Tutorial1 완료](docs/webpj-l3-tutorial1-complete-20261124.md) · [Tutorial2](docs/webpj-l3-tutorial2-20261124.md) · [Tutorial3 단계 A→B](docs/webpj-l3-tutorial3-stage-ab-20261124.md) · [전 미션 시작](docs/webpj-l3-mission-starts-20261124.md) |
| 원본 캡처·Session 대조 | E1275~E1276, E1312, E1314~E1316 | 신전·제네레이터·타워 파편 481행, 윈드 아비어리·워크샾·제단 3,993행, 골렘 66·받침 198행 캡처의 크기·기준점·전체 픽셀 일치, 실제 Session의 EvilRay(E1314)·피해 연소(E1315)·섬 투하 받침/입자(E1316)를 C++ 통합 시험과 대조 | — | [파편 캡처](docs/webpj-temple-shrapnel-capture-20261123.md) · [골렘·받침](docs/webpj-golem-support-capture-20261124.md) · [섬 투하](docs/webpj-drop-island-session-20261124.md) |

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분(세부 단위 `- [x]`)은 중복 가산하지 않는다. 남은 단위·줄 수는 `PORT_MAP.tsv`의 partial·todo 기준(partial은 파일 전체 줄 수)이며, 2026-11-24 두 번째 정리 재집계(`node webpj/tools/port-map.mjs`·`python -X utf8 tools/port_inventory.py`)도 **partial 0·todo 0, 0단위 / 0줄**이다(E1328에서 마지막 RawCampaignSession 7,130줄을 port로 승격, [세션 경계 마감](docs/webpj-session-boundary-close-20261124.md)). 그래서 남은 일은 대응표 행이 아니라 표의 '남은 상위 체크'(3.1절 미체크 상위 항목, 정리 스크립트가 모음)와 3.6절 판정 6행·3.7절 후순위 요구다. 4-D 머리 수는 E1283의 L2 닫기 뒤 84/85이며 Tutorial 단계 진행이 남았다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 | 남은 상위 체크(3.1) |
| --- | --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료(4-C 통합 잔여는 4D-3에서 닫음) | — | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **84/85, L2 닫음(Tutorial1 4:3·16:9·16:10·한국어)** | 0 / 0 | 4D-4b Tutorial 게이트·표시 장치 |
| 5 | 5-A 렌더러·Draw | 46/46 | 0 / 0 | — |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 12/13 | 0 / 0 | Cursor·RawCursor 표시 |
| 5 | 5-C 화면 허브·미션 진입 | 6/6(E1325) | 0 / 0 | — |
| 6 | 입력·게임 메뉴·경제·건설 | 9/10 → M3 | 0 / 0 | 사제·운반·경로 바인딩 |
| 7 | 전투·의식·AI | 5/7 | 0 / 0 | 전투 바인딩 · AI 세션 결선 |
| 8 | 미션 수명·대본·오디오·도움말 | 8/9 → M4 | 0 / 0 | M4 |
| 9 | 표시 품질·브라우저/OS·배포 | E945 하위 경로·설정 복원 검증, M5 미완료 | 0 / 0 | M5(3.2절: 60/120 표시·성능·메모리, 브라우저·GUI Linux, HTTP 캐시·저장 호환) |
| 10 | AGENTS 후순위 웹 요구 | 일부 선행, 3.7절 참조 | 0 / 0 | 3.7절 표의 남은 일(4·5·6·7·10·12·13·14차) |
| **합계** | | | **0 / 0** | **6** |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,007단위는 **port 이상 1,007(100%; `port` 275·`session` 732)·partial 0·todo 0**이다(2026-11-24 두 번째 정리 재집계, E1328 이후 같음). 제외 153단위(멀티 73·Edit 42·호스트 37·구판 1)를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session` 732행은 E1283 에서 `e2e/session-coverage.spec.js`(고정 시각 Tutorial1 4:3 130프레임 V8 함수 커버리지)와 `tools/port-map-session.mjs` 로 올렸다: **행의 웹 모듈이 모두 그 실행에서 함수를 하나 이상 실행했을 때만** 올리는 보수적 판정이며, 남은 `port` 275행의 `session` 승격은 시나리오를 늘려 같은 커버리지 도구로 재검사한다(전투·AI·의식·편집 아닌 UI 등). 오라클 대조를 뜻하는 `oracle`은 0이다(승격 규칙은 이후 시나리오가 늘 때 같은 도구로 다시 돌린다; [근거](docs/webpj-port-map-session-20261123.md)). E1098~E1237의 재판정 내역은 대응표 메모와 [E1166 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)·[E1237 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에, E1238~E1288의 행 닫기는 대응표 메모와 [E1288 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 573 / 573 | 0 | 0 |
| `client/` 세션·UI·표시 | 433 / 433 | 0 | 0 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 partial·todo 단위는 0이다. 재기준선 추가분 RawClockSynchronization은 E1317에서 플랫폼 적응으로 닫았다. 메모 머리 태그와 이전 집계는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), 현행 계열은 `python -X utf8 tools/port_inventory.py --families wb`(남은 계열이 없어 지금은 출력이 없다)를 따른다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 1,047항목(E1334 Tutorial4 단계 A~B 오라클 26항목 추가)을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

### 0.5 단위 착수와 종료

1. AGENTS.md와 공통·dotnetpj·웹 인계를 읽고 Git 상태·원격 변경을 확인한다. 같은 작업 트리의 다른 세션 변경을 고려해 파일명을 지정해 stage한다.
2. 대응표 검사와 기준선 뒤 변경을 확인한다. 3.1절의 단계 순서와 선행 조건에 맞춰 기준선 cpppj 코드, 기존 시험, fixture, 단위 문서를 읽는다. 큰 단위는 공개 API 경계로 나눈다.
3. 원본/클론·오라클 수집·별도 창 실행에는 AGENTS.md의 확인·예외 규칙을 적용한다. 웹 이식에는 디컴파일 소스 분석이 필요 없다.
4. 단위 문서에 근거 커밋·fixture/오라클·검사·실패와 교정·미연결 경계를 기록한다. 대응표의 web·base(8자리)·status·메모와 해당 manifest를 갱신한다. 현행 인계에는 현재 값·다음 작업·검증과 해당 체크만 갱신하고 완료 항목 상세는 정리 때 이관한다.
5. 각 작업 단위마다 해당 변경의 집중 검사와 연관 검사를 수행한다. HJOW-Athlon에서는 완료된 작업 단위 4회마다 Node v26 전체 회귀를, 8회마다 Node v22 전체 회귀를 수행한다. 두 버전의 전체 회귀가 함께 필요한 경우 순차 실행하며 동시에 실행하지 않는다. 다른 PC에서는 사용 가능한 최신 Node 버전으로 완료된 작업 단위 4회마다 전체 회귀를 수행한다. 필요하면 변경의 위험이나 통합 범위에 맞춰 추가 검사를 수행한다. 신호 NaN 비트 대조 시험은 맨 위에 `import './nan-exact.mjs'`를 둔다. (2026-11-19 사용자 지시)
6. `www` 소스를 바꾼 단위는 `npm --prefix webpj run build`로 webpack 번들(`www/dist/netstorm.bundle.js`)을 다시 만들어 함께 stage 한다(번들 Git 추적, E1297; `tests/bundle-fresh.test.mjs`가 낡은 번들을 실패로 잡는다). 완료 후 당일 최대 번호를 확인하고 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. 기존 커밋의 Codex 공동 작성자 표기와 AGENTS.md의 push 금지 규칙을 따른다.

### 0.6 제품 공통 요구

11.01 표기·Credits 세 번째 항목과 HJOW/AI 문구·`Copyright 2026 HJOW`, 시작 클릭 화면, 공개 루트 내 실행 자산, 외부 접속 문서화·유효성 검사 비활성화는 반영했다. Credits AI 문구는 사용자가 다시 지시할 때 수정한다. **15차 Reset All은 E863에서 구현·검증했다.** [제품 11.01](docs/webpj-product-1101-20261105.md) · [외부 접속](docs/webpj-external-access-20261105.md) · [초기화](docs/webpj-reset-all-20261119.md)

<a id="favicon"></a>

- [ ] **favicon 탭 시각 확인.** E894에서 공통 ICO를 복사하고 상대 경로 링크를 추가했다. 원본 두 이미지의 바이트 대조와 Chromium의 HTTP 200·해독을 확인했다. 2026-11-20 Chrome의 localhost NetStorm 탭은 접근성 트리에서 확인했으나 Windows 창 캡처가 반복 시간 초과하여 탭 그림의 시각 확인이 남았다(E1263 점검: 탭 아이콘은 페이지 화면 캡처에 나오지 않는 브라우저 창 부분이라 자동 확인이 안 되고, 사용자 눈 확인 범위이다). [작업·검증](docs/webpj-favicon-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

<a id="about-version"></a>

- [x] **정보(About) ▸ 버전 창의 Copyright(사용자 요구 2026-11-23) — E1289 완료.** 게임 메뉴 About ▸ Version과 메인 메뉴 Help ▸ Version의 Tell `About`에 `NetStorm 11.01 Patch by HJOW.`(한국어 `NetStorm 11.01 패치: HJOW.`)와 `Copyright 2026 HJOW`를 한 번만 넣었다(cpppj 1330과 같음, 일본어 줄은 13차). [근거](docs/webpj-about-version-copyright-20261123.md) · [공통](LEFT_JOBS.md#user-requests-20261123)

<a id="korean-font-change"></a>

### 0.7 한국어 갈무리11

E896에서 실제 미션 설치의 You→당신 번역과 Player 표시 바이트를 연결했고, 제공된 일반·굵은 WOFF2와 라이선스를 공개 데이터에 넣어 로딩·Canvas·자료 준비가 같은 글꼴 설정을 공유한다. Chromium 메뉴·도움말(영어 대체 포함)과 Tutorial1 한국어 브리핑 두 페이지에서 확인했다. [설치](docs/webpj-app-install-display-20261119.md) · [구현·검증](docs/webpj-korean-galmuri11-20261117.md) · [한국어 미션](docs/webpj-korean-tutorial1-20261120.md)

- **글자 품질(E1244·E1254·E1259~E1261·E1288):** 갈무리11은 12px 설계 격자로 그린다(제목 24·슬롯 4 48px, 4배 그리기·칸 평균, E1288). 숫자·영문·공백은 cpppj처럼 원본 비트맵 글리프, 슬롯 1·6의 한글 기준선은 `max(원본, 배율×11)`, 큰 슬롯은 원문 정수 배율(E1259), Tell 본문 폭은 가장 긴 낱말 이상(E1260), 굵은 글꼴에 없는 8,271자는 일반 갈무리11로 대체(E1244)한다. 게임 안 글자는 브라우저 최소·기본 글꼴 설정과 무관한 고정 크기다(예외: 웹 전용 ☰ 메뉴 DOM 버튼). [12px 격자](docs/webpj-korean-glyph-grid-20261123.md) · [제목 판독](docs/webpj-korean-title-legibility-20261123.md) · [굵은 글꼴 대체](docs/webpj-galmuri-bold-fallback-20261123.md)
- **사용자 결정:** 굵기는 원문 Bold 대신 지금의 보통체를 유지한다(2026-11-23 "지금은 그대로"). 글자만 출력 해상도로 덧그리는 고해상도 글자 층은 도입하지 않고 현재 픽셀 표시를 유지하기로 사용자가 결정했다(2026-11-24, 3.5절). cpppj와 다르게 둔 점(제목 배율·굵기·한글 기준선)은 근거 문서에 있다. 같은 점검의 cpppj·dotnetpj 후속은 사용자 지시로 작업을 보류하고 [LEFT_JOBS.md](LEFT_JOBS.md)·[LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)에만 기록했다(`92ea7bfd1`).

## 1. 범위와 유지되는 결정

- **범위.** 일반 31경로와 데모 3경로를 대상으로 한다. 단독 미션의 같은 프로세스 안 내부 전송·패킷·Login·Zone·서버/클라이언트는 포함한다. LAN TCP·멀티플레이 UI·재접속/메타, 공식 검증 서버, Edit·커스텀 맵·User Made Campaigns, 설치/CD·Quit, 옵션 해상도/전체화면은 웹 범위에서 제외한다. 스커미시의 MP 시작 규칙과 Outpost는 12차에 포함한다.
- **이식.** cpppj의 파일과 API 경계, 부작용과 호출 순서, 정수 폭과 저장 시 정밀도, 난수 소비를 보존한다. 단독에서 도달하는지 확인해 웹 제외 분기에는 명시 오류를 유지한다. 현재 단계가 직접 요구하는 부품만 앞당긴다.
- **웹 구조.** `netstorm.js`가 앱 수명을 맡는다. 모든 스크립트는 webpack 으로 번들 하나(`www/dist/netstorm.bundle.js`, 진입 `www/bootstrap.js`)로 묶어 `index.html`이 그것만 불러온다(E1293, 번들은 사용자 결정으로 Git 에 추적한다 — `www` 소스를 고친 단위는 `npm --prefix webpj run build`로 번들을 다시 만들어 함께 커밋하며 `tests/bundle-fresh.test.mjs`가 낡은 번들을 잡는다). 번들 없이 이전 방식(ES 모듈 직접 적재)으로 구동하는 디버그 페이지는 `www/debug.html`이며, 앱과 같은 모듈 인스턴스가 필요한 e2e(`import()` 사용 11개 파일)도 이 페이지를 연다. `index.html`은 최소 변경하고 스타일은 JS에서 관리한다. 기존 CommonJS 서버와 `www/` ES 모듈 경계를 유지한다. 코어를 DOM·Canvas·Web Audio에서 분리하고 8비트 논리 화면과 최종 표시를 구분한다.
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
| E1289~E1329 | 사용자 추가 요구 세 건(버전 창 Copyright·한국어 ESC 메뉴·방향키 카메라), webpack 번들, 처음 방문 시작 화면 안정화, Tutorial1·2 L3 시작~완료 창과 Tutorial3 단계 A→B(천둥 `ThunderProcess` 연결), 31개 미션 시작 상태, 5-A 표시 재대조 마감(캡처·EvilRay·Burning·섬 투하), 객체 메뉴 특수 행·운반체 줍기, 주문·골렘 사제 포획 교정, 16:10 1280×800·한국어 M2, DestroyPatch 정상 복귀, 채팅 로그 앱 수명, partial 12→0행(RawCampaignSession 공개 경계 감사, E1328). 주제별 표는 [0.1절 요약](#e951-e1081). |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다. 여러 독립 작업을 묶은 항목은 `- [ ]` 세부 단위로 나누고 세부마다 완료 기준을 적는다(세부 단위는 머리 수에 넣지 않는다, 0.2절). 닫힌 체크는 단계마다 '완료한 체크' 줄이나 표로 줄이고 점검·완료 근거 원문은 보관본에 둔다.

#### 4-D Session 조립·Tutorial1 Load/Start (84/85)

**완료 기준 L2는 E1283에서 닫았다.** 같은 시점·난수에서 Tutorial1 Load/Start의 덤프·첫 프레임 논리 화면이 cpppj 오라클과 같다(영어 4:3·16:9, 90·130프레임: 화면 0픽셀, 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다름 — `player`의 +1·+2 바이트는 주소 잔재, Player 1 +0x68·+0x69는 Deck 할당 토큰의 표현·시점 차이). 기준선 구간 지도와 E633~E801의 완료 체크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), E802~E1166의 항목별 완료 나열은 [E1166 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166), 아래 닫힌 항목의 점검·완료 근거 원문(E1263~E1283)은 [E1288 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에, 그 뒤 항목별 닫은 근거 표 원문은 [E1329 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)에 있다. cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다.

- [ ] **4D-4b Tutorial 게이트·표시 장치** — Tutorial 1~6 판정·공통 게이트와 신전의 현재 슬롯 재조회(E781~E782), 다리 수명 숫자·Renderer(E1230), 승패 판정 소비(E1152·E1161·E1262)는 끝났다. 남은 것은 실제 앱의 단계 진행이다(자연 진행 e2e는 Tutorial2뿐, E1086; Tutorial1·2는 L3로 시작~완료 창을 cpp와 대조해 끝남, E1304·E1307; Tutorial3은 단계 A→B까지, E1329). 스커미시 승리 실전은 1절 사용자 확인 범위다. [E781](docs/webpj-session-temple-draw-20261118.md) · [E782](docs/webpj-session-tutorial-gate-20261118.md)
  - [x] Tutorial1 — E1304에서 완료 창까지 4,200프레임 화면 0픽셀·덤프 일치로 완료했다. E1313에서 미표시 체크를 바로잡았다. [완료 근거](docs/webpj-l3-tutorial1-complete-20261124.md)
  - [x] Tutorial3 — E1333에서 시작~'Mission Accomplished!' 완료 창 및 `DoneTutorial3=1`·`Tutorial4` 전환까지 6,200프레임(체크포인트 화면 31곳 0픽셀·덤프 20곳 허용 행 외 차이 0)으로 완료했다. [완료 근거](docs/webpj-l3-tutorial3-complete-20261125.md)
    - [x] 단계 A(간헐천 다리 직결·골렘·첫 크리스털 신전 납품 `0x020000b4`, SP 1600→1800) → 단계 B 안내 창 — E1329, 1,700프레임 화면 9곳·덤프 7곳 일치. [기록](docs/webpj-l3-tutorial3-stage-ab-20261124.md)
    - [x] 단계 B 남쪽 적 섬에 다리 직결 — E1330, 2,200프레임(8조각 다리로 적 섬 `(135,130)` 연결 → 단계 C `'Build Sun Disc Throwers'` 안내 창·`OK` 뒤 자유 진행까지 화면 12곳 0픽셀·덤프 10곳 일치). [기록](docs/webpj-l3-tutorial3-stage-bc-20261124.md)
    - [x] 단계 C Sun Disc Thrower 생산 — E1331, 2,360프레임(Sun Workshop 우클릭 메뉴→지식 생산 투입→Sun Disc Thrower 선택 → 단계 D `'Generators, Part 1/2'` 안내 창·`OK` 뒤 자유 진행까지 화면 17곳 0픽셀·덤프 12곳 일치). [기록](docs/webpj-l3-tutorial3-stage-cd-20261124.md)
    - [x] 단계 D~E Wind Generator 생산·적 섬까지 에너지 확장 — E1332, 3,460프레임(Wind Generator 생산 등록 → 단계 E `'Extending Energy, Part 1/2'` → 아군 남단 `(129,110)`·적 섬 `(134,131)` Wind Generator 2기 연결 → 적 섬 `(132,134)` 첫 `Sun Disc Thrower` 완공 → 단계 F `'Incapacitating the Enemy Priest'` 안내 창·`OK` 뒤 자유 진행까지 화면 25곳 0픽셀·덤프 17곳 일치). [기록](docs/webpj-l3-tutorial3-stage-def-20261124.md)
    - [x] 단계 F~H 발사기로 적 사제 무력화·골렘 포획 → 'Mission Accomplished!' 완료 창 — E1333, 6,200프레임(적 섬 `(137,151)` 두 번째 발사기 배치→단계 G `'Capturing the Enemy Priest'`→이동 중 골렘 우클릭 `Drop Storm Crystal`→적 사제 `(138,153)` 포획→단계 H 완료 창, 화면 31곳 0픽셀·덤프 20곳 일치, `DoneTutorial3=1`·`Tutorial4` 전환 통과). [기록](docs/webpj-l3-tutorial3-complete-20261125.md)
  - [ ] Tutorial4 — 완료 기준: 위와 같다. 대본 단계 A~I1(아래 묶음은 진행하며 조정한다):
    - [x] 단계 A~B 고급 전투·썬 디스크 발사기와 썬 캐논 — E1334, 3,210프레임(Sun Disc Thrower·Wind Generator 생산 등록 → 남쪽 카메라 `(1504,760)`·Wind Generator 2기 `(130,91)`/`(136,110)`·중앙 회랑 다리·Thrower 3기 `(136,127)`/`(127,127)`/`(124,129)` → 적 `Sun Cannon` 2기 파괴 → 단계 C `'Upgrading Workshops, Part 1'` 안내 창 진입까지 화면 14곳 0픽셀·덤프 10곳 허용 행 외 차이 0). [기록](docs/webpj-l3-tutorial4-stage-ab-20261125.md)
    - [ ] 단계 C~E 워크샾 업그레이드·썬 캐논으로 적 발사기 파괴 — 완료 기준: 단계 E1 '사제 희생, 1부' 안내 창까지 L3 일치.
    - [ ] 단계 E1~I1 사제 희생 1~4부·멀티/싱글플레이어 안내 → 완료 창 — 완료 기준: 완료 창까지 L3 일치와 다음 미션 해제.
  - [ ] Tutorial5 — 완료 기준: 위와 같다. 대본 단계 A~G:
    - [ ] 단계 A~B 방어·워크샾 추가 건설 — 완료 기준: 단계 C 안내 창까지 L3 일치.
    - [ ] 단계 C~D 적의 섬·공중 공격 방어·캐논 방어·다리 품질 — 완료 기준: 단계 E '주문' 안내 창까지 L3 일치.
    - [ ] 단계 E~G 주문·데버스테이션 주문·사제 희생 → 완료 창 — 완료 기준: 완료 창까지 L3 일치와 다음 미션 해제.
  - [ ] Tutorial6 — 단계 B 너겟 납품 포함. 완료 기준: 위와 같다. 대본 단계 A~F:
    - [ ] 단계 A~B 원소 계열 유닛·에어 쉽·원소 에너지·공중 수송 유닛(너겟 납품 `0x020000b4`) — 완료 기준: 단계 C 안내 창까지 L3 일치.
    - [ ] 단계 C~F 크로스보우·바리케이드 → 완료 창 — 완료 기준: 완료 창까지 L3 일치와 다음 미션 해제.

완료한 체크(머리글 수에 포함; 항목별 닫은 근거 표 원문은 [E1329 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)):
- **4D-1~4D-3:** 4D-1 외곽·snapshot(E946·E1270, 비미션 너겟은 미션 모드라 미도달), 4D-2 typed Load(E697), 4D-3a Login(E1263), 4D-3b ClientCampaignTransport(E1274), 4D-3c 전송 앞단(E1268), 4D-3d Squid 수신(E1263), 4D-3e 명령 등록(E1267), 4D-3f 통합 시험(E1263). [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md) · [기본 수신기](docs/webpj-client-default-receivers-scope-20261123.md)
- **4D-4a~4D-7:** 4D-4a 카메라·프레임(E1266, 주 루프 순서), 4D-5a 종료 수명·4D-5b Client State 정리(`_releaseCampaignWorld`)·4D-5c 이탈·전투 판정(`missionPost` 관문)(E1263·E1268), 4D-6a GameWorld·4D-6b Client 서비스(CampaignClientServices port E1296)·4D-6c Session 공개 조회(E1263), 4D-7a~7d ClientMain·Client 전역(E1263; 프레임 예외 때 렌더 루프 정지 설계는 3.7절 7차). [프레임 순서](docs/webpj-frame-order-20261123.md) · [이탈 정리](docs/webpj-mission-leave-state-reset-20261123.md)
- **4D-8~4D-10:** 4D-8a RawSurface 삭제·수명(E1273·E1321), 4D-8b~8f Native 분기(E1193~E1221)·4D-8g `UsesNativeTransmissionDevices()` 재대조(E1263), 4D-9a·9b 덤프·오라클과 앱 `sessionDump()`(E869·E1263), 4D-10a 실제 소유자 재대조(E1263), 4D-10b L2 판정(E1283). [정상 복귀](docs/webpj-destroy-patch-continue-20261124.md) · [Native 수신기](docs/webpj-native-receivers-audit-20261122.md) · [고정 시각](docs/webpj-fixed-time-mode-20261123.md)

#### 5-A 렌더러·Draw·시각 시스템 (46/46)

완료한 체크(머리글 수에 포함, 보관한 44개 외; 아래 두 개의 본문은 [E1327 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)):
- **ClientRendererFrame** — E1298. 미션 화면 출력의 원본 순서(E978~E1082)와 메인 메뉴 Kencloud 구름 캐시(M1 구름 차이 37,054픽셀 → 0). [Kencloud](docs/webpj-menu-kencloud-20261124.md)
- **E197·E198·E201 재대조** — E1316. 파편·건물(E1275~E1276)·골렘/받침(E1312) 캡처와 실제 Session의 EvilRay(E1314)·피해 연소(E1315)·섬 투하(E1316)를 C++ 시험과 대조했다. [마지막 근거](docs/webpj-drop-island-session-20261124.md)

#### 5-B 카메라·HUD·미니맵 (12/13)

완료한 체크(머리글 수에 포함, 보관한 9개 외): **UberHud**(E1255, [근거](docs/webpj-uber-hud-close-20261123.md)), **RawMinimapControls**(E1248, [근거](docs/webpj-minimap-controls-close-20261123.md)), **FloatingSpGump·RawProductionControls**(E1257, [근거](docs/webpj-production-controls-close-20261123.md)).

- [ ] **Cursor·RawCursor 표시** — 커서 부품의 실 화면 연결(Cursor 계열 행 전부 port, CampaignCursorClientEffects E1256)과 가장자리 스크롤(E990~E1001)·마나 볼트 Session 연결(`raw-campaign-session-arrivals.js`)은 끝났다. 남은 것은 실제 화면 대조다. [근거](docs/webpj-cursor-client-effects-close-20261123.md)
  - [x] 카메라 투영·커서 표시 — L2/L3 고정 시각 대조에서 F4 카메라 이동 뒤(300프레임)와 다리 조각을 든 커서(345프레임) 화면이 cpppj와 0픽셀이다(E1280~E1286).
  - [ ] 가장자리 스크롤 뒤 화면 — cpp `--ui-script`에는 포인터 이동 동작이 없어(`click`·`right`·`key`·`keyup` 등) 입력 방법부터 정한다. 완료 기준: 같은 스크롤 입력 뒤 체크포인트 화면이 cpppj와 0픽셀.
  - [ ] 마나 볼트 정기 표시 — 완료 기준: 마나 볼트가 그려지는 장면의 고정 시각 화면이 cpppj와 0픽셀이거나 남는 차이의 원인이 기록된다.

#### 5-C 게임 화면 허브·미션 진입 (6/6)

완료한 체크(머리글 수에 포함): **Uber 허브**(E912~E917·E1082·E1169~E1179, 배경 GIF·NSInfo 자식은 웹 범위 밖 E1243, [판정](docs/webpj-uber-root-background-scope-20261123.md)), **미션 진입 명령**(E1263, [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md)), **허브 fixture**(E1265, [근거](docs/webpj-chat-hub-fixtures-20261123.md)), **메인 메뉴·타이틀 상태**(E1184, [문서](docs/webpj-title-screens-20261122.md)), **메뉴·버튼 컨트롤**(E1325, RawButtonControls·RawMenuChat·RawMenuHtml·RawMenuControls·RawMenuLoading·RawTextEntryControls port, [메뉴 컨트롤](docs/webpj-menu-controls-close-20261123.md)), **M2 / L4a**(E1323, 영어 4:3·16:9·16:10과 한국어, [한국어](docs/webpj-korean-m2-20261124.md)).

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (9/10)

완료한 체크(머리글 수에 포함, 보관한 1개 외): **입력 본체**(E1263, [E802](docs/webpj-campaign-user-input-movement-click-20261118.md)), **게임 메뉴**(E1264, [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md)), **공장·지식 메뉴와 기술**(E1076~E1079·E1189, [E1189](docs/webpj-fury-war-technology-20261122.md)), **객체 우클릭 메뉴**(E1310, [e2e](docs/webpj-object-menu-rows-20261124.md)), **방향키 카메라 이동(사용자 요구 2026-11-23)**(E1291, [근거](docs/webpj-arrow-camera-20261124.md)), **ESC 게임 메뉴 한국어 점검(사용자 제보 2026-11-23)**(E1290, [근거](docs/webpj-esc-menu-korean-20261123.md)), **건설·생산·경제**(다리 조각 연속 배치 E1299~E1307로 마감, 건설 기록 재생은 웹 범위 밖 E1242, [다리 조각](docs/webpj-l3-tutorial1-20261123.md#확장-다리-조각-연속-배치wb04-e1299-2026-11-24)), **M3 / L3**(Tutorial1 입력 단계표 세부 넷 E1284~E1307, Tutorial3~6 확장은 4D-4b, [L3](docs/webpj-l3-tutorial1-20261123.md)).

- [ ] **사제·운반·경로 바인딩**
  - 완료: 캠페인 Session 운반 입력과 일반 사제 이동(E794), 실제 마우스의 사제 선택→이동(E1139 e2e), 단독 미션 제단 희생 메뉴 훅(E1223), 기도·Cast/Drop·Factory 순환 명령 소비(E1225), Review Knowledge·GetTechnology 단추와 기술 획득 Fort snapshot 장치(E1227), Bulf 너겟 줍기(0x030000b1)·메뉴 Drop(0x030000b2) Chromium e2e(E1311), 주문 Cast와 주문 보유 골렘의 사제 포획·Drop(E1318~E1319). MetaMap은 도달하지 않고(E1188), 저장 미션의 전체 초기화와 경로 루프는 31개 시작 e2e·`raw-campaign-session-load.test.mjs`·`mission-priest-move.spec.js`·`raw-campaign-motion-bindings.test.mjs`로 확인했다.
  - [ ] 저장 미션 경로 L3 — cpppj `CampaignMotionTests.cpp:120~126`의 저장 미션 경로 시험을 고정 시각 단계표로 대조한다. 완료 기준: 체크포인트 덤프의 경로·위치 관련 행 일치.
  - 근거: [E794](docs/webpj-session-carrier-input-20261118.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md) · [줍기·Drop e2e](docs/webpj-object-menu-rows-20261124.md) · [포획](docs/webpj-spell-priest-pickup-20261124.md)

#### 7 전투·의식·AI (5/7)

완료한 체크(머리글 수에 포함): **파괴·주문 효과**(E1199), **의식**(E1180, 보상 선택 UI는 웹 범위 밖, [의식 판정](docs/webpj-ritual-reward-scope-20261122.md)), **AI 잔여·전투 이탈**(E1198), **전용 시험이 없었던 시스템**(E1316, [근거](docs/webpj-drop-island-session-20261124.md)), **주문(스펠) 사용·골렘 사제 포획(사용자 제보 2026-11-23)**(E1318~E1319, cpppj 1339도 같게 교정, [주문](docs/webpj-spell-cast-20261124.md) · [포획](docs/webpj-spell-priest-pickup-20261124.md)).

- [ ] **전투 바인딩** — Hit·GunQueries·Archer·Motion·BombBlast·Damageable 여섯 표 행은 모두 port이고 Session에 결선돼 있으며(`raw-campaign-session-{hits,guns,gun-views,combat,flyers,damageable}.js`와 각 Node 시험) 실제 Chromium에서 AI 혼전이 약 18분 오류 없이 돈다(E1262). Archer의 초기 배치·비행 및 factory/clock loop는 1절의 검증 범위를 적용한다.
  - [ ] 전투 장면 오라클 — AI가 공격해 오는 미션의 고정 시각 cpppj 오라클을 수집한다(3.3절). 완료 기준: 시나리오·결과와 출처가 manifest에 등록된다.
  - [ ] 교전 N프레임 대조 — 완료 기준: 피격·사격이 일어나는 체크포인트의 화면·덤프가 cpppj와 같거나 남는 차이의 원인이 기록된다.
- [ ] **AI 세션 결선** — Start의 일반/데모 AI·동맹과 typed 프레임, 현재 Player·SP·명령·난수와 장기 RawSurface 소비자를 연결했다(`raw-campaign-session-ai*.js`·`netstorm-campaign-ai.test.mjs`, E1122·E1124·E1198·E1206~E1216). DestroyPatch 진단은 사용자 결정에 따라 E1321에서 표시 후 정상 복귀로 고쳐 실제 앱에 연결했다(3.5절, [근거](docs/webpj-destroy-patch-continue-20261124.md)).
  - [ ] AI 행동 N프레임 대조 — 위 전투 장면 오라클로 AI의 다리·생산 결정 시점을 비교한다. 완료 기준: AI 행동 체크포인트의 덤프(`random`·`regular`·`object` 행) 일치.

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (8/9)

완료한 체크(머리글 수에 포함): **대본·대화창**(E1263, [E1190 점검](docs/webpj-script-commands-audit-20261122.md)), **소리 본체**(E1261), **음악**(E1258, [근거](docs/webpj-scene-music-close-20261123.md)), **글자·설정·그림**(E1261, [근거](docs/webpj-korean-title-legibility-20261123.md)), **적응 fixture**(E1263), **도움말·Tell**(E1197~E1240 해당 대응표 행 모두 port, [근거](docs/webpj-main-menu-help-type-20261123.md)), **동영상**(E1202 사용자 결정으로 웹 제외, [결정 기록](docs/webpj-intro-movie-decision-20261122.md)), **미션 수명**(E1085~E1161, [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md)). 실제 청취 판정은 사용자 확인 범위이다.

- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.
  - 완료: 캠페인 목록 31개 미션의 연속 시작(E1121, Priest Training 9개는 E1157에서 실제 연결하고 Auto-Demo가 대신 돈 e2e 거짓 통과를 정정), 캠페인 16개 장시간 진행과 AI 시점 미연결 서비스·좌표 오류 네 가지 수정·소리/안내 서비스 여섯 개 연결(E1122), Early Missions·Priest Training 15개 100초 진행(E1123)과 Priest Training 9개 150초씩 26분(E1158), 스커미시 AI 7명 300초(E1124), 데모 420초 진행과 360초 재시작(E1125), 후반 4개·중반 13개 장시간 진행으로 시계 정지 누수 수정(E1132~E1133), 몽키 시험(E1128·E1134·E1165, 죽은 Gump 정리 수정 포함), 승패 안내 뒤 흐름과 진행 저장·다음 미션 해제(E1152·E1161).
  - [ ] 자연 승패 진행 저장·자연 완주 — L5 사용자 확인 범위(1절). 완료 기준: 사용자가 자연 승패 → 새 완료값 저장 → 새로고침 → 다음 미션 해제를 확인한다.
  - [ ] 한국어 전체 확인 — 사람 검수(3.7절 6차)와 미션 본문 줄 나눔 실제 화면. 완료 기준: 사용자 검수 결과가 기록되고 지적 사항이 반영된다.
  - E1263 점검: 두 세부 모두 사용자 확인 범위라 코드로 더 줄일 일은 없다.
  - 근거: [모든 미션 시작](docs/webpj-all-missions-start-20261120.md) · [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md) · [시계 정지 누수](docs/webpj-mission-leave-pause-20261121.md) · [Gump 정리](docs/webpj-gump-cleanup-frame-20261121.md) · [몽키·미니맵](docs/webpj-monkey-minimap-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다. E945에서 Windows의 Chromium·Chrome·Edge·Firefox로 하위 경로의 정적 모듈·자산 로드와 같은 URL 새로고침 뒤 음량 설정 복원을 확인했다. HTTP 캐시, GUI Linux와 실제 미션 진행 저장은 별도 검증한다. [E945](docs/webpj-static-deployment-20261119.md)

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`·`session-dump`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 웹은 검사 전용 고정 시각 모드(`?fixed=25`, `stepFixedFrames`, E1271)로 같은 프레임을 만든다. L2는 `e2e/fixed-time.spec.js`(E1283 닫음), L3는 같은 단계표를 웹에 적용하는 `e2e/l3-tutorial1.spec.js`(E1284)와 공통 틀 `e2e/l3-common.js`·`tools/l3-collect.mjs`(E1305)이며 웹 입력은 클릭 N+1·키 N−1 프레임에 넣는다. 난수 소비 순서는 cpp 임시 난수 추적(호출 스택 RVA와 링크 맵)으로 대조할 수 있다(E1280). L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md) · [난수 추적](docs/webpj-state-install-rng-20261123.md) · [L3](docs/webpj-l3-tutorial1-20261123.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 cpppj 제품은 1339(M8 관문 1327)이며 웹 비교 기준선은 제품 1313이다. 1314 최초 실행·1320 미션 전환(E616·E620), 1328~1330 사용자 추가 요구(방향키 E1291·ESC 한국어 E1290·버전 창 E1289), 1339 주문/포획 교정(웹 E1319와 같은 교정, 대응표 base `13e74579`)을 웹에 반영했다. 1331~1338은 시험 교정·배포 확인과 일본어(13차) 작업이며 소스 변경 판정은 3.6절에 있다. 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목(1316~1319·1321·1324)은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 판정은 3.6절, 1323 아이콘은 favicon(0.6절)으로 따로 반영했다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·특정 적 공격·승리·전체 화면/음향은 1절 범위에 따라 확인한다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- 와이드 전투 시야와 카메라 재배치를 연결할 때 16:9의 논리 크기 전환도 검증한다. WebMCP 대상 브라우저·조작/캡처/녹화 범위는 착수 시 확정한다.
- **사용자 결정 완료(2026-11-24, 비동기 질문에 명시 답변).**
  - 채팅 로그는 원본처럼 Client/앱 수명 동안 유지한다. E1320에서 미션 종료 초기화 적응(E1170)을 해제하고 미션 간 유지·앱 재시작 초기화를 검증했다(5-C).
  - DestroyPatch 진단 열두 지점은 원본처럼 표시 후 계속 진행하도록 바꾼다. E1321에서 실제 앱의 정상 복귀 경로를 구현·검증했다(4D-8a).
  - 16:10 논리 크기는 cpppj와 같은 1280×800으로 맞춘다. E1322에서 크기 변경·입력 좌표와 실제 선택/이동·동일 크기 오라클을 검증했다(5-C M2).
  - 한국어 고해상도 글자 층은 도입하지 않고 현재 갈무리11 픽셀 표시를 유지한다. 굵기도 기존 결정대로 보통체다(0.7절).
- **결정 대기.** 프레임 예외 한 번에 렌더 루프가 멈추는 설계를 유지할지(3.7절 7차). DestroyPatch의 진단 정상 복귀와 일반 예외 복구 정책을 구별한다.
- **원본 실행·사람 확인.** favicon 탭 그림(0.6절), 주변음·효과음·음악 실제 청취, 한국어 사람 검수와 미션 본문 줄 나눔 실제 화면, 시작 화면 안정화(E1300·E1308)의 배포본 처음 방문. 예산 칸 원본 우클릭은 E1324에서 확인했다.

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다.
- **판정 끝:** cpppj 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 변경 아홉 행(E1241, base `63590635`·`710b9647`·`baf141eb`, [판정표](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md)), 사용자 추가 요구 1328~1330(E1289~E1291), 1339 주문/포획 교정(E1319, base `13e74579`), SoundMusicSelection base 정정(2026-11-22, `225553df`). 커밋·행별 경과는 [E1329 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)에 있다.
- **판정 남음(2026-11-24 두 번째 정리 때도 `port-map.mjs` 재실행 결과 같은 6행):** 일본어(13차) 커밋 `5238ee833`(1334)·`87324232f`(1335)가 바꾼 `KoreanResources.cpp`·`StyleTextBuffer.cpp`·`HtmlTextParser.cpp`·`Markup.cpp`·`StyleTextMarkup.cpp`와 `UberGump.cpp`(base `d2bcefeb` 뒤 `87324232f`). 이 중 `87324232f`의 `BindHtmlPictureText(…, Patch1078)`(Help·안내 HTML의 `imana.8`~`11`·`17`을 `Imana.4`~`7`·`9`로 바꿔 그림)와 `TextPictureResources.h`·`ManaPictureData.inc`(원본 10.78 자료에 없는 마나 아이콘을 CD 원본 25프레임으로 보충)는 언어와 무관한 표시 변경이라 13차와 별도로 ②·③을 판정해야 한다. 웹에는 같은 별칭·보충이 없다(4절 '도움말·Tell').
- **대응표 밖 새 파일:** EmbeddedBitmapFont(.cpp/.h)·KoreanFontSettings.h·ResetAllSettings(.cpp/.h)는 ①이고 `TextPictureResources.h`·`ManaPictureData.inc`는 위 판정을 따른다. 재기준선 때 등록한다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | 아비어리·공용 Form writer 경계에서 확인하지 못한 칸(확신 낮음, E594·E1210). 이탈 State 정리(E1268)·Client 기본 수신기(E1274)·분리 Poll과 `pollClientConnectionHealth`(4D-7b)·RawClockSynchronization 플랫폼 적응(E1317)은 닫혔고 PacketDelays·RawPacketCopies·RawMonitorTime은 도달하지 않는다. |
| 4D-8 | SidPool 소진 회수·진단과 GameAuxState 새 전역 칸의 소비 여부(코드와 행은 있음). DestroyPatch 정상 복귀(E1321)·`rememberPlacement`(E1273)·기본 수신기(E1274)·나머지 Native 경계(E1193~E1221)는 끝났다. |
| 5-A~5-C | 없음. ‘Creating Islands.’ 안내(`creatingIslands`, `RawMissionServerPackets.cpp:60-63`의 `createArchipelago`)는 로컬 미션 Zone `0x82000004`(`local-mission-transport.js:189`)에 0x100 비트가 없어 도달하지 않는다(E1328). |
| 6~7 | 없음. 건설 ActualConfirm/ActualPlace·동맹 협상 입력·RawGunHit Rain override는 끝났고 건설 기록은 개발자 전용으로 제외했다(E1242). RawPathRuntime/RawPathData는 '사제·운반·경로 바인딩' 항목이 이어받는다. |
| 8~10 | 한국어/일본어 자원 우선순위·입력(13차와 한국어 사람 검수). TellSection·Frame Rate·Difficulty·NB10·스커미시 일반 시작은 끝났다(E1263 점검). |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했고 Native 대여는 E1193~E1194에서 끝났다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | E943에서 16:9 논리 화면을 최대 1920×1080까지 지원하고 창 크기 변경과 입력 좌표를 확인했다. 전투 카메라·HUD와 WebMCP 조작·화면 캡처·소리 포함 녹화가 남았다. |
| 5 | localStorage 설정·진행 저장과 오류 대응을 구현했다. 실제 미션에서 진행값이 발생하는 8단계와 15차 초기화에서 저장 수명을 확인한다. |
| 6 | 메뉴·Tell·도움말과 갈무리11, 튜토리얼·캠페인·데모·훈련 미션 대본 전부(E1108~E1116)와 게임 안 메뉴·객체 메뉴 문구(E1118·E1140~E1141·E1166)까지 구현했다. 생산 창·스톰 파워 도움말과 채팅 채널 이름(E1246, 미니맵 도움말은 원본이 번역하지 않음), 미션 화면 문구 15개(E1249), 채팅 도우미 라벨 59개(E1250)를 더했고 글자 품질은 E1244·E1254·E1259~E1261·E1288에서 바로잡았다([0.7절](#korean-font-change)). 5-C M2 한국어 오라클은 E1323에서 마쳤다. 전체 미션 대본의 사람 검수·줄 나눔 확인은 남았다. |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. 실제 미션 수명에서의 오류 진단이 남았다(프레임 예외 한 번에 렌더 루프가 멈추는 설계를 유지할지의 결정 포함, 4D-7b). [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | 10.78 알려진 버그 중 웹 적용분을 cpppj 판정과 대조한다. NB04/29 간헐천 계산·callback, NB18 요청/수신 관문과 NB20 실제 Session 선택/취소는 선행했다. 실제 스커미시와 브라우저 조작 검증은 남았다. |
| 11 | 쉬움/보통/어려움(기본 보통). 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반. E554·E555 계산과 AiCreate 연결, E1096 Options·ESC 메뉴 Difficulty 선택과 저장을 마쳤다. E1149에서 실제 스커미시로 난이도별 AI 다리·섬 건설 속도(쉬움 ≪ 보통 < 어려움)를 확인하고 옵트인 e2e로 고정했다([난이도 진행](docs/webpj-skirmish-difficulty-20261121.md)). 쉬움의 채취 유닛 절반(최소 1)은 실제 진행에서 편차가 커 구분되지 않아(E1156, AI가 프레임 구동이라 빨리 감기 부적합, [빨리 감기](docs/webpj-fast-forward-clock-20261121.md)) 설정 단위 Node 시험과 AI 생성 연결을 근거로 둔다. |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천. Outpost 목록 수명·분류, 규칙·설정 화면·저장·시작 섬 fort 묶음·섬 생성 부모·세션 월드 생성과 시작·어려움 AI 진행·금지 유닛 반영(E1101~E1107), AI 생산·공격과 사제 사망 패배 안내의 실제 시간 관찰(E1262), 미션 모드라 비미션 너겟 배치·보충이 도달하지 않고 간헐천은 무작위로 놓임(E1270)을 마쳤다. 승리 경로와 AI 다리 멈춤 시점의 원본 대조가 남았다([다음 작업 3](#next)). [규칙](docs/webpj-skirmish-setup-20261120.md) · [UI](docs/webpj-skirmish-ui-20261120.md) · [시작](docs/webpj-skirmish-start-20261120.md) · [관찰](docs/webpj-skirmish-watch-20261123.md) |
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
| 메뉴·입력 | 메인 메뉴 구름은 Kencloud 캐시로 cpppj 와 같다(E1298, M1 구름 차이 0). Help/Options 위치·와이드 배치, Overlay 드래그/캡처·미션 커서는 미확인이다. Edge Scroll(가장자리 스크롤)과 Auto-Demo(메인 메뉴 45초 무활동)의 실제 동작은 E990~E1001·E1092에서 확인했다([가장자리](docs/webpj-app-edge-scroll-frame-20261120.md) · [데모](docs/webpj-demo-start-20261120.md)). |
| 도움말·Tell | `<info>` 머리글/능력치, `#`/`&` 맞춤·색 치환, mana.8~11의 실제 원본 F1 모습(cpppj 1335는 이 번호를 현행 마나 그림으로 바꿔 그린다, 3.6절 판정 남음), `<background>`/nsisles 사용, `{@미션.키}`·스크립트 도움말 Back 및 필요 시 두 열 메뉴를 확인한다. 도움말 그림은 레이어0/그림자 없음 경계를 유지한다. |
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
npm --prefix webpj run build
npm --prefix webpj run build:dev
npm --prefix webpj run check-bundle
npm --prefix webpj run test:e2e
```

- 개발 서버는 `http://localhost:9803/`이다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 규칙 없는 개발용 미리보기다.
- 필요한 항목만 `node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>`로 갱신한다. 자산 변경 시 `npm --prefix webpj run prepare-assets`를 쓴다.
- Node v22는 HJOW-Athlon의 webpj 폴더에서 `& "C:\Program Files\nodejs22\node.exe" --test "tests/*.test.mjs"`로 순차 실행한다.
- [이식 대응표 사용법](docs/webpj-port-map-20261106.md) · [실행 자산](docs/runtime-resources.md) · [형식 목록](docs/formats/README.md) · [공유 명칭](assets/unit-names.korean.json) · [cpppj 소스 지도](cpppj/SOURCE_MAP.md) · [외부 접속 정책](docs/webpj-external-access-20261105.md)

## 7. 정리 기록

**2026-11-24(두 번째).** E1329 직후 문서 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)에 이관했다(AGENTS.md `1fc63cb1` 정리 규칙).
- **옮긴 것(보관본에 원문):** 다음 작업 5 아래에 잘못 붙어 있던 '(완료) RawCampaignSession partial 1행 닫기' 줄(E1328), 단계 A 확인 좌표의 입력 순서, 진행 요약 표 칸의 단위별 문장, 3.1 완료 체크의 근거(4-D 완료 표 21행, 5-A~8단계 완료 본문), 0.7 글자 품질 경과, 3.6 판정 끝 문단의 커밋별 경과와 표의 끝난 칸, 지난 7절 기록.
- **바로잡은 것:** 4D-4b Tutorial3의 'L3 대조는 시작 240프레임(E1309)까지' → 단계 A→B 1,700프레임(E1329), 다음 작업 2의 Early Mission 3 단계 A 너겟 납품 → E1329에서 확인, 0.1 '미검증'의 'Tutorial3 이후' → 'Tutorial3 단계 B 이후', 2절 'partial 12→1행' → 12→0행, 3.6 표 5-A~5-C 행 → 남은 것 없음(E1328). 재집계·실측(partial 0·todo 0, `port` 275·`session` 732, 판정 남음 6행, manifest 986, 번들 3,433,729바이트, 회귀 주기 v26 0/4·v22 5/8)은 문서 값과 같았다.
- **나눈 것:** 4D-4b의 Tutorial3을 대본 단계 기준 다섯(단계 A→B `[x]` E1329, B, C, D~E, F~H)으로, Tutorial4~6을 대본 단계 묶음 2~3개씩으로, 다음 작업 3(스커미시)을 완료 기준이 있는 세 갈래로 나눴다.
- **새로 넣은 것:** 0.2 표의 '남은 상위 체크' 열(3.1 미체크 상위 항목 6개). `[x]`로 바꾼 상위 항목은 없다.

이전 정리(같은 날 첫 정리 E1327 이관, 2026-11-23 E1288·E1237 이관과 그 앞)의 기록은 보관본의 날짜별 원문 7절에서 확인한다. AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음), `11b16b77`(webpj favicon, [0.6절](#favicon)), `1fc63cb1`(LEFT_JOBS 정리 규칙 — 이번에도 적용)까지 반영했고 그 뒤 AGENTS.md 변경은 없다. `96d81381`은 Noto Sans KR 글꼴 파일과 AGENTS.md가 가리키는 `fonts/webfonts.css`(Google Fonts CDN 예시)를 더한 것이며 한국어 게임 글꼴 규칙은 갈무리11 그대로이다(3.7절 13차).
