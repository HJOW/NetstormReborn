# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 최신 [E1488 정리 전 전문](LEFT_JOBS.webpj.old.md#archive-webpj-20261129-through-wb04-e1488)·[E1380 정리 전 전문](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)(보관 문서 끝)과 보관 문서 머리의 [E1339 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261126-through-wb04-e1339)부터 이어지는 날짜별 보관본을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → 3.1(단계별 항목) → 0.5(단위 착수·종료 절차) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 항목으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-29, 마지막 완료 단위 WB04-E1493·회귀 검사 교정과 Node26/22 전수 완료)

- **실행·플레이**
  - **화면:** 브라우저 ‘클릭하여 시작’(첫 적재 뒤 최소 6초, E1308), 시작 title 그림, 메인 메뉴·캠페인/데모 목록·도움말·옵션·Credits와 개발용 지형 미리보기가 동작한다. 옵션에는 Reset All 게임 안 확인 창, 영어/한국어/일본어 선택(E1412), 모드 관리(E1483), 설정·진행 JSON 복사/불러오기(E1487)가 있다.
  - **스커미시 메뉴(E1489):** Campaign의 Skirmish 바로 위 구분선을 원본 offical5sep 파일로 복원했고, AI 수·개별 난이도·동맹·금지 유닛은 Options의 기존 선택 아이콘을 쓴다. 세 언어·스커미시 꺼짐·선택/저장·금지 목록 순서를 Node 93/93·Chromium 4/4로 확인하고 lint·운영 번들을 갱신했다. [변경·검증](docs/webpj-skirmish-selection-icons-20261129.md)
  - **미션:** 캠페인 목록 31개(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)와 데모 3개가 시작되고, 장시간 진행과 임의 조작(몽키) 시험에서 오류·먹통이 없다. Campaign ▸ Skirmish는 무작위 섬·간헐천 월드에서 AI와 진행되며 AI 생산·공격으로 사제가 죽으면 패배 안내까지 이어진다(E1262, 약 18분 관찰). Tutorial2는 시작부터 완료 창·Tutorial3 시작까지 자연 진행으로 확인했고, 승패 안내 뒤 흐름(Continue·Replay Mission·Leave/Next Mission·진행 저장)도 확인했다. 미션 프레임은 다리 붕괴 검사·전투 자원·제단 결과 알림·주변음/공유 바람·장면 음악·미니맵 갱신을 원본 주 루프 순서로 돈다(E1230~E1234·E1266). 공식 Tutorial1~6은 다음 단계 조건을 우측 하단에 표시하고([0.8절](#tutorial-objective-port)), 스커미시는 Outpost 허용·금지와 실제 건설까지 연결했다(E1447~E1448).
  - **조작:** 사제 선택·이동, 가장자리·Alt·가운데 버튼 스크롤, 미니맵 클릭·끌기, 생산 격자 클릭·단축키(D 포함)·조각 회전(C·회전 방향 옵션)·다리 배치, 사제·워크샾·제단·신전·배터리·제네레이터·운반체 우클릭 메뉴(회수·기도·Cast/Drop·Carrying/Drop·폭탄 Detonate!), 객체 명령(Detonate·Meltdown·신전 명령), 주문 시전과 주문 보유 골렘의 적 사제 포획·Drop(E1318~E1319), 화면 위치 키(F4/H·F5·TAB·U·Shift+숫자 저장/숫자 복귀)와 방향키 카메라 이동(32픽셀, 조각을 든 ←/→는 회전, E1291), 미니맵·스톰 파워 표시(원본처럼 About, E1324)·생산 창·채팅 도우미 단추의 우클릭 빠른 도움말이 동작한다.
  - **게임 메뉴:** ESC 또는 화면 우측 상단 버튼(웹 전용)으로 Game·View·Options·Players·About 메뉴막대를 열고(열 때 원본처럼 SP 검산 회전, E1327), Leave/Restart Mission(ABORT 확인 창), F1·F2·F6·F7·F8·F9·Shift-F3·T, Pause(Shift-F9·Pause 키·메뉴), Shift-F7 화면 GIF 저장, Players 메뉴의 송금(Send X 숫자 입력 창 포함, 수신 안내 E1326)·동맹이 실행된다. ESC는 열린 Tell 창(브리핑·로딩 제외)을 원본처럼 닫는다(E1264). 버전 창의 11.01 패치·`Copyright 2026 HJOW` 줄(E1289)과 한국어 메뉴 전수 점검(E1290)을 마쳤다. 브리핑·완료 창의 Review Knowledge·GetTechnology 단추와 Priest Training 7의 Training Menu(원본 오타 별칭, E1245)도 동작한다.
  - **HUD 틀·채팅 창:** 실제 Uber 루트가 좌측 사이드바(SP·생산 덱)·하단 미니맵 칸·구분선을 원본 위치로 도킹하고 지도 영역이 `{85,0,W,H}`다(4:3·16:9·16:10·창 크기 변경 확인; E1488부터 논리 화면은 화면비별 최대 1280×720·800·960·1024이고 더 큰 창은 CSS/DPR 표시만 확대). F3 채팅 창의 로그·입력줄·제출(로컬 서버 경유 `You> 본문`)·점 명령·Tab 이름 완성(동맹 AI)·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창이 동작하고(E1169~E1179) 로그는 원본처럼 앱 수명 동안 유지된다(E1320).
  - **미완성·범위 밖:** 1-3/1-4 승리·저장은 미확인이고, 자동 허용(1-1~1-5·스커미시) 밖 미션의 목표 수행·자연 완주는 사용자 직접 확인으로 남았다(3.1절 M4). 지식 NSInfo 패널·Players 하위의 Ignore chat·Whisper To·정보 보기·의식 보상 선택 UI·인트로 영상은 웹 범위 밖이다(E1167·E1171·E1180·E1202).
- **원본과의 동일성**
  - **M1 메뉴:** 영어 4:3 메뉴 8장면은 cpppj 오라클과 알려진 차이 영역(Campaign 창 크기·Options 팝업 위치) 밖에서 픽셀 차이 0이고 구름 배경도 같다(Kencloud 캐시, E1298).
  - **L2 시작 상태(E1283 닫음):** 검사 전용 고정 시각 모드(`?fixed=25`)에서 Tutorial1 Load/Start의 Game 난수열(90프레임 6,851회)·논리 화면·Session 덤프가 cpppj 기준선 오라클과 같다. 4:3·16:9(1366×768)·16:10(1280×800, E1322)의 90프레임 화면 0픽셀(16:9는 E1488부터 논리 1280×720이며 새 CP1384 오라클과 90프레임 0픽셀·90/130프레임 덤프 허용 행 밖 0), 90·130프레임 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다르다. 한국어(E1323)는 상태가 같고 첫 안내 창 밖 0픽셀이며 창 안 차이는 웹 독립 번역·글꼴 차이로 분리 확인했다([근거](docs/webpj-korean-m2-20261124.md)). 31개 미션의 시작 5·30·120프레임 덤프도 모두 같다(E1313).
  - **L3 입력 진행:** cpp `--ui-script` 입력 단계표를 그대로 적용해 Tutorial1(4,200프레임, E1284~E1286·E1299·E1301~E1304)·Tutorial2(5,400프레임, E1305~E1307)·Tutorial3(Capture The Priest, 6,200프레임, E1309·E1329~E1333)의 시작~'Mission Accomplished!' 완료 창·다음 미션 해제와 Tutorial4 단계 A→C→E1 `'Sacrificing Priests, Part 1'` 안내 창 진입 7,240프레임(E1334~E1335)·Tutorial5 단계 A2 실제 피격·B 안내까지 1,000프레임(E1339~E1340), ESC 메뉴 여닫기 360프레임(E1327)까지 체크포인트 화면 0픽셀·덤프 일치다(단계별 근거는 [E1380 보관본 0.1 진행 요약](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380) L3 행과 3.1절 4D-4b). 이 대조로 다리 붕괴 스캔 초기화 누락(`Reset422490`, E1301)·생산 패널 HUD 생성 순서(E1305)·로그인 중 간헐천 보충 기한(E1313)·메뉴 SP 추적(E1327)·주변음/장면음악 천둥의 `ThunderProcess`(TYPE 55) 미연결(E1329)과 `e2e/l3-common.js`의 `INJECT_SHIFT.right`(-1, E1333)를 고쳤다. E1336 마나 그림 보충 뒤 Tutorial4 `a220`의 생산 하위 메뉴 영역은 기준선 오라클과 8,656픽셀 달라 E1337부터 사유·영역·픽셀 수를 명시한 알려진 차이로 검사하며(영역 밖 0픽셀), E1336 이전의 0픽셀 결과를 그 뒤로 넓혀 읽지 않는다.
  - **L3 캠페인(16/16):** E1350~E1365에서 모든 미션의 브리핑·시작·대기1200프레임을 미션마다 화면3곳0픽셀·덤프3곳 허용 밖0으로 대조했다. 허용 미션의 실제 목표 수행·승리는 별도 단위다.
  - **부품:** 월드·UI 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다(원본 공통 메뉴 788행·Factory 메뉴 39행·Uber 루트 160행·장면 음악 3,113행, 채팅 입력줄 대상 720행(E1265), 파편·건물 캡처 4,474행(E1275~E1276)·골렘/받침 264행(E1312), 실제 Session의 EvilRay·Burning·섬 투하(E1314~E1316) 등). 대응표 1,007단위 중 732행은 Tutorial1 실행에서 웹 모듈의 함수가 모두 실행된 `session`이다(E1283). 좌측 HUD 하단 85×82픽셀은 원본 스크린샷과 일치한다.
  - **한국어 글자:** 갈무리11 12px 격자로 그리되 제목 배율·굵기(사용자 결정으로 보통체)·한글 기준선을 cpppj와 다르게 둔다([0.7절](#korean-font-change)).
  - **미검증:** 세 캠페인 묶음의 목표 수행 L3·자연 완주와 L4b 소리(16개 진입/대기는 완료, 1-1·1-2·1-5의 실제 아군 AI 승리는 오라클 대조가 아닌 자연 진행 확인). Tutorial4 E1~I1·Tutorial5 D 이후·Tutorial6 E 이후의 특정 적 공격·포획·희생·승리는 사용자 지시로 건너뛰고 사용자가 직접 확인한다. Tutorial6 A~D 생산·건설·납품은 E1343~E1346 완료다.
- **기준 값(2026-11-29 정리 재확인)**
  - **마지막 완료 단위:** E1493(검사 기대값/Config 준비5건 교정·최종 Node26/22 각각7,046/7,046, [근거](docs/webpj-regression-expectations-20261129.md)). 먼저 닫은 E1494는 최신 AI 검사 범위·1-3 패배/1-4 상한 관찰(Node2/2·Chromium3/3, [결과](docs/webpj-autoplay-scope-20261129.md)), E1491~E1492는 생산 격자 부모 소실 재현·보완(Node44/44·Chromium3/3, [근거](docs/webpj-production-grid-parent-loss-20261129.md)), E1490은 CP1384 메뉴 주석① 판정이다. 이전 완료 이력은 아래 요약과 보관본을 따른다.
  - **최신 cpppj:** 제품 1387(스커미시 선택 아이콘, E1489 반영)·1386(생산 격자 부모 소실 보완, E1492 반영)·1384(1280 폭 상한·화면비별 FSR), 최신 ZIP 1378, 제품 검증 1380이다. 웹 이식 기준선 `225553df`(제품 1313)와 고정 오라클은 자동 교체하지 않으며, E1488은 16:9 1280 확인용 CP1384 오라클(`3f12a2bd4`)만 따로 추가했다. 변경 판정은 [3.6](#cpp-delta)을 따른다.
  - **운영 번들·자료:** `www/dist/netstorm.bundle.js` 5,866,967바이트(E1492, 크기 관문 9,900,000바이트; E1478 격리 Worker 단일 번들부터 약 5.8MB), fixture manifest 1,337항목(E1488).
  - **작업 지시(2026-11-29 최신 goal):** 단위별 문서·로컬 커밋을 유지한다. 포획/희생·특정 적 공격·승리 자동 검사는 캠페인1-1~1-5·스커미시만 아군 AI 활성화로 허용하고 그 밖은 생략한다. 단순 진입/대기/생산/건설은 진행한다. E1494에서1-3/1-4 거부를 해제하고1-6은 공용 활성화기/CLI에서 거부했다. 1-3은 실제 패배,1-4는1509초 상한 미승리로 승리·저장은 미완료다. 외부 확인/버그 자료는 사용자가 “새 결과 없음 — 미완료 유지”를 선택했다. [범위·결과](docs/webpj-autoplay-scope-20261129.md).
  - **모딩 진행 범위(2026-11-28 사용자 결정):** DN MOD-1 선행 대기를 해제하고 WB가 공통 계약을 주도한다. 계약(E1470)과 WB 구현·관리·자동 검증(E1471~E1484)은 끝났고 DN MOD-2~4 구현 결과가 없어 양쪽 적합성 대조가 남는다([3.7절](#modding-api)).
  - **재적용 금지:** CP 오류 문맥/Residence/epoch 초안은 E1390~E1392 제품에 반영했다. `webpj/extracted/cpp-post1381-plan-20261127.json`의 clock 판정은 E1392로 대체됐다.
- **전체 회귀**
  - **최신 전체:** E1493 최종 Node26.13.0 **7,046/7,046**(877.123초) → Node22.23.3 **7,046/7,046**(843.407초), 순차 실행·모두 종료0·실패/취소/건너뜀/todo0이다. 작성 입력2,280개 원시 SHA-256 변경0이다. E1487~E1494 소스와 이번 교정4파일을 포함한다. 첫 중단4실패와 첫 전수7,045/7,046은 낡은 기대값/Config 준비를 교정하고 별도 로그로 보존했다. [최종 근거](docs/webpj-regression-expectations-20261129.md).
  - **주기:** E1494 완료 소스까지 포함한 E1493 전수 종료 뒤 **v26=0/4·v22=0/8**이다. 다음 완료 단위4개 뒤 Node26,8개 뒤 Node22 전체를 수행하고 겹치면 순차 실행한다(0.5절5).
  - **Chromium:** E1313 전 미션 시작 상태 옵트인 전체 32/32 통과(24.3분, `webpj/extracted/webpj-e1313-final-e2e.log`). 기존 전체 e2e는 E1312 199건 중 186 통과·12 옵트인 미실행·한국어 브리핑 1건 부하 중 시간 초과(단독·3회 반복 통과)다. 단위별 실패·교정은 [E1313](docs/webpj-l3-mission-starts-20261124.md)과 각 단위 문서, 이전 회귀 이력은 [E1327 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)을 따른다.
- **브라우저 검사:** 미션 화면은 단위마다 Chromium e2e로 확인한다(최근 전체 실행 E1312, 고정 시각 L2·L3 `e2e/fixed-time.spec.js`·`e2e/l3-tutorial1.spec.js`·`e2e/l3-tutorial2.spec.js`·`e2e/l3-tutorial3.spec.js`·`e2e/l3-tutorial4.spec.js`(E1337부터 345프레임까지만 재실행)·`e2e/l3-tutorial5.spec.js`(E1339), 프레임 예외 뒤 진행 `e2e/error-reporting.spec.js` E1338, 앱 서비스 결선 `e2e/session-services-wiring.spec.js` E1328, 한국어 글자 배치 `e2e/korean-glyph-layout.spec.js` E1288). 메뉴는 2026-11-17 Chromium 2/2(갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체), 최초 실행·진행 잠금/체크·저장 재실행은 E616 Chromium 2/2. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md)

<a id="next"></a>

#### 다음 작업

CP1384 메뉴 주석 판정(E1490)과 CP1386 생산 격자 재현·보완(E1491~E1492)은 끝났다. 이식 대상의 미판정 변경 행은0이며 제외된 ScreenResolutions.h의① 자동 기록만 남는다. 최신 AI 범위·두 미승리 관찰은 E1494, 검사 준비 교정과 양쪽 전수는 E1493에서 마쳤다. 사용자는1-3/1-4의 다른 AI 조건 시도를 요청했다.

1. **진행 — 캠페인1-3/1-4 다른 AI 조건.** 사용자는 E1494 결과 뒤 “다른 AI 진행 조건으로 계속 시도”를 선택했다. 원래 자금·기술·HP·목표/승패 규칙을 보존하면서 난수/시작 시점과 원래 AI 설정에서 가능한 조건을 조사하고 실제 승리·신규 저장·다음 미션 해제를 확인한다. 단순한 동일 실행 반복은 피하고 조건·결과를 단위로 기록한다.
2. **M4 미완료(3.1절).** 자동 허용1-3/1-4는 E1494에서 원래 자금·기술·HP를 보존한 실제 AI로 확인했으나 각각 패배·60,000프레임 상한이었다. 승리·신규 저장·Next는 미확인이다. 같은 검사를 반복해 완료로 바꾸지 않고 다음 작업1의 다른 조건으로 이어간다. 1-6/그 이후·튜토리얼 후반 목표 수행은 생략하고 사용자 직접 확인, 한국어 중간 안내/문장 검수도 사용자 확인으로 유지한다.
3. **물리 환경 대기 — M5(3.2절).** 실제 모니터60/120Hz(M5-a), 모니터 이동/OS 배율(M5-b), 와이드 전투 육안·효과음/음악/위치음향 청취(M5-c). 원격32Hz·headless·Xvfb 결과로 대체하지 않는다.
4. **DN 구현 의존 — 모딩 공통 적합성(3.7절14차).** DN MOD-2~4의 같은 패키지/정규화 입력 결과가 있어야 WB 결과와 대조할 수 있다. WB 쪽 검사는 완료 상태다.
5. **자료 대기 — 원본 알려진 버그14건(3.7절10차).** E1455의 자료 필요10·미재현4는 재현 자료나 사용자 제보가 있을 때 단위로 연다.

사용자는 이번 외부 확인/버그 자료 질문에 **“새 결과 없음 — 미완료 유지”**로 답했다. 따라서2~5의 미확인 부분을 자동 완료로 바꾸지 않는다. [검증 결과·남은 조건](docs/webpj-autoplay-scope-20261129.md).

**완료 범위:** Tutorial1~6의 자동 검사 허용 범위는 E1346, 캠페인16개 브리핑/시작/대기는 E1365, CP 변경16행 판정은 E1348, 튜토리얼 조건 안내는 E1389, 자동 허용 승리(1-1·1-2·1-5·스커미시)는 E1394·E1369·E1371·E1439, 스커미시 나머지(예약 주기·채취 절반·C++ 대조)는 E1418·E1436·E1437, WB 모딩은 E1483에서 마쳤다. 입력 좌표·단위별 오라클 폴더·로컬 산출물 경과와 오래된 다음 작업 번호는 [정리 전0.1](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)에 보존했다. 조사 자료·오라클은 Git 제외 `extracted/`·`webpj/extracted/`에 그대로 두며 지우거나 커밋하지 않는다.
- **미션 식별.** 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이고, 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이다(E753 재대조). [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md)

<a id="e951-e1081"></a>

#### 완료 이력 요약

E951~E1380의 단위별 완료 문장·근거 표는 [E1380 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)의 0.1 진행 요약, E1382~E1488의 단위별 완료 문장은 [E1488 정리 전 전문](LEFT_JOBS.webpj.old.md#archive-webpj-20261129-through-wb04-e1488)의 0.1절과 각 `docs/webpj-*.md`에 있다. 아래는 E1382~E1488의 주제별 요약이며 현재 범위는 위 실행/동일성·3.1 체크가 기준이다. 완료된 구현을 다시 만들지 않는다.

| 주제 | 단위 | 연결·확인한 것 | 남은 것 | 대표 문서 |
| --- | --- | --- | --- | --- |
| HUD 생성·State 수명 | E1382~E1385 | 앱 HUD 생성과 World 도킹 분리, State7 선행 HUD 생성, 공통 State 부모·Runtime 반납, 건설17 종류별 입자 메모리 입력 → 실제 AI 7덤프·7화면 0픽셀(7단계 7/7) | — | [HUD 생성](docs/webpj-hud-app-factories-20261127.md) · [State 부모](docs/webpj-app-state-world-parent-20261127.md) · [입자 입력](docs/webpj-construction-memory-input-20261127.md) |
| 튜토리얼 조건 안내 | E1386~E1389 | 40단계/39키 조건·문구, 투명 Gump, 실제 gate·State 수명, 영어/한국어 3화면비·전환 검증 | 후반 목표 수행은 사용자 확인 | [0.8절](#tutorial-objective-port) · [검증](docs/webpj-tutorial-objective-verification-20261127.md) |
| CP 변경 반영(정리1381 당시 9행·신규 10파일) | E1390~E1393 | 표시 상세·Regular 실패 객체 문맥, Residence 복구 상태 공유, 비고정 부팅의 epoch 시계·최초 Capture, Login 재접속 SP 훅 | CP1384 `RawGameMenus.cpp` 1행([다음 작업 1](#next))·CP1386 `RawProductionControls.cpp` 1행([다음 작업 2](#next)) | [epoch](docs/webpj-client-clock-epoch-20261127.md) · [재접속 SP·판정](docs/webpj-login-reconnect-sp-20261127.md) |
| 배포·브라우저·Linux | E1395·E1397·E1402·E1446·E1468·E1484 | HTTP 캐시·같은 URL 교체·네 브라우저 새로고침, Linux GUI 세 화면비 L2·저장 읽기·하위 경로, 비고정 시작 재시도, Linux 원래 6검사 6/6·모드 8/8, 실제 Chrome 탭 favicon 시각 확인 | Linux 반복 debug 탐색 60초 실패 2건은 원인 미확정(관측 한계로 보존) | [캐시](docs/webpj-http-cache-20261127.md) · [Linux 후속](docs/webpj-linux-startup-load-followup-20261128.md) · [favicon](docs/webpj-favicon-20261119.md) |
| ESLint·빌드 관문 | E1400·E1406 | flat ESLint와 번들 생성 전 검사, 미사용 선언 건수 예외(177→176, E1416), 동일 출력의 mtime 갱신 | — | [ESLint](docs/webpj-eslint-20261127.md) · [mtime](docs/webpj-webpack-fresh-mtime-20261127.md) |
| 캠페인 실제 아군 AI | E1394·E1396·E1398·E1399·E1401·E1408·E1417·E1419·E1423·E1424·E1427 | 1-1 완료값 저장·다음 미션의 네 브라우저 복원(E1394; 1-2·1-5는 E1369·E1371), 1-3·1-4·1-6 장시간 관찰(패배·상한·사용자 중단, 게임 오류 0), 1-3·1-4·1-6 자동 승리 거부 | 1-3·1-4·1-6 승리·저장은 사용자 직접 확인 | [1-1](docs/webpj-campaign1-1-progress-20261127.md) · [사용자 이월](docs/webpj-autoplay-scope-manual-campaigns-20261127.md) · [검사 안내](docs/testing/webpj-autoplay-test-guide.md) |
| 월드 수명·메모리 | E1404·E1413·E1416·E1419·E1431·E1433·E1435 | 공용 진단 팩터리 약한 참조, 제단 예약 피해 가상 함수, 그래프 소진 복구 정책과 실제 복구, 내용물 통지 getter 세션 참조 해제, SID 소진 다리 회수, 정상 rAF 30분 27회 World/Session 회수 | 모든 전투의 장시간 메모리·물리 120Hz는 M5 | [반납 진단](docs/webpj-mission-memory-release-20261127.md) · [그래프](docs/webpj-graph-exhaustion-recovery-20261127.md) · [SID](docs/webpj-sid-allocation-recovery-20261127.md) · [30분](docs/webpj-long-mission-memory-20261127.md) |
| 스커미시·간헐천·Outpost | E1418·E1434·E1436·E1437·E1439·E1443·E1445·E1447·E1448 | 난이도별 실제 예약 주기(쉬움 4배·어려움 0.5배), 쉬움 채취 유닛 절반(쉬움 3·보통 6기), C++1377 다리 정체 대조, 실제 아군 AI 승리·반납, 와이드 승리와 화면비 복귀(당시 1920×1080), 보충 간헐천 3709초 오류 0, Outpost 삭제·시작 기술·실제 건설 | 엄격 전상태 동등·원본 NB01 모든 조건 해결로 세지 않음 | [주기](docs/webpj-skirmish-ai-periods-20261127.md) · [채취 절반](docs/webpj-skirmish-collector-half-20261127.md) · [승리](docs/webpj-skirmish-ai-victory-20261127.md) · [Outpost](docs/webpj-outpost-construction-close-20261127.md) |
| Native GameAux·Form 경계 | E1440·E1444·E1451·E1457 | 실제 Client 건설 차단/충돌 마스크/지역 요청 수·공급자 순환 수명, 아비어리·공용 Form 46/46, 잔여 24칸 = typed 21·원격/개발자 제외 3 | — (Native 129 주소 동일성은 주장하지 않음) | [소비자](docs/webpj-client-gameaux-consumers-20261127.md) · [헤더](docs/webpj-gameaux-native-header-audit-20261127.md) · [역할](docs/webpj-gameaux-native-role-audit-20261127.md) |
| 표시 성능·고DPI | E1403·E1449·E1454·E1456·E1460·E1461·E1462 | 메뉴 60/120 시계·짧은 메모리 표본, 와이드 AI 미션 11.3~14.1 → 36.4~45.0 → 52.6~54.9FPS(부분 표시·전송 합산·색 번호 대조), 일반 Chrome GPU·원격 32Hz와 headless SwiftShader 구분, DPR 알림 누락 복구, 한국어 고DPI | 물리 60/120Hz 확인(M5-a) | [성능](docs/webpj-indexed-conversion-cache-20261127.md) · [표시 경로](docs/webpj-browser-display-path-20261127.md) · [고DPI](docs/webpj-high-dpi-fallback-20261127.md) |
| 언어 | E1410·E1412·E1430·E1485 | 일본어 공용 43파일·44 웹 자산, 선택/저장/자동 감지·Noto Sans JP CDN·일본어 줄바꿈·31미션 첫 안내(13차), 한국어 31미션 첫 안내 줄폭, 전역 공통 Tell 32곳(60섹션)·미니맵·채팅 문구 | 사람 문장 검수·중간 안내(M4) | [일본어](docs/webpj-japanese-browser-support-20261127.md) · [한국어 첫 안내](docs/webpj-korean31-initial-layout-20261127.md) · [전역 점검](docs/webpj-global-korean-audit-20261128.md) |
| 잔여 재판정 | E1455·E1465~E1467 | 알려진 버그 34개(웹 수정 7·자료 필요 10·미재현 4·제외 13), 지형·보상/SP·메뉴/도움말 잔여 문장을 현재 연결과 대조 | 자료 미확정 14건, 육안·청취(4절) | [버그](docs/webpj-known-bugs-scope-audit-20261127.md) · [지형](docs/webpj-terrain-residual-audit-20261128.md) · [메뉴·도움말](docs/webpj-menu-help-residual-audit-20261128.md) |
| 모딩 API(14차) | E1470~E1484 | 계약 v0.1.0, 공개 API·RNG, 실제 입력 큐 호스트, 패키지 검증·기본 꺼짐, 참가자·결과 사건, Worker 격리·단일 번들, 시계 장벽·관리자, 실제 Session·고정 JS 수학, 적재 순서 복구, Options 관리 화면, Linux GUI 8/8 | DN 재개 뒤 양쪽 적합성 | [계약](docs/netstorm-common-mod-api-v0-20261128.md) · [Session·수학](docs/webpj-mod-session-math-20261128.md) · [관리 화면](docs/webpj-mod-options-20261128.md) · [남은 관문](docs/webpj-mod-linux-gui-20261128.md) |
| 사용자 추가 요구(2026-11-28) | E1486~E1488 | F3 채팅 닫기·작성 문자열 보존, 정지 커서 에너지/공간 재판정(CP1382), 크로스보우 가격 변수(현 자료 550)·설정/진행 JSON 복사·불러오기·미션 중 난이도 차단(CP1383), 논리 최대 1280×720·800·960·1024와 CSS/DPR 확대(CP1384, 웹 FSR 제외) | 물리 클립보드/IME·전 브라우저 재검증은 하지 않음 | [F3·커서](docs/webpj-input-placement-fixes-20261128.md) · [가격·JSON·난이도](docs/game-user-followups-20261128.md) · [해상도](docs/resolution-policy-1280-20261128.md) |
| 스커미시 메뉴 사용자 수정(2026-11-29) | E1489 | 세 언어 Campaign 구분선 복원·Options 기존 선택 아이콘 통일, CP1387 세미콜론 인자/기존 쉼표 호환과 금지 화면 탐색 순서 유지 | — | [변경·검증](docs/webpj-skirmish-selection-icons-20261129.md) |
| 전체 회귀 | E1397~E1486 | E1397 양쪽 6,895 뒤 Node26 14회(E1405 6,897 ~ E1486 7,033)·Node22 7회(E1411 6,901 ~ E1485 7,032), 모두 최종 전부 통과. 첫 실행 실패(E1438 등록 누락·E1474 분류 누락·E1481 적재 순서)는 보완 뒤 재실행했다 | 주기는 위 '전체 회귀' | [최신 v26](docs/webpj-input-placement-fixes-20261128.md) · [최신 v22](docs/webpj-global-korean-audit-20261128.md) |

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분(세부 단위 `- [x]`)은 중복 가산하지 않는다. 남은 단위·줄 수는 `PORT_MAP.tsv`의 partial·todo 기준(partial은 파일 전체 줄 수)이며, 2026-11-29 정리 재집계(`node webpj/tools/port-map.mjs`·`python -X utf8 tools/port_inventory.py`)도 **partial 0·todo 0, 0단위 / 0줄**이다(마지막 partial은 E1328). 이 수치는 고정한 이식 대상(기준선 `225553df`)의 집계다. 기준선 뒤 cpppj 변경은 3.6절에서 판정하며 지금 판정이 남은 이식 대상 행은 없다(CP1384 메뉴 주석 E1490 ①·CP1386 생산 격자 E1492 ② 완료)(대응표 밖 신규 10파일은 E1393 판정). 튜토리얼 안내 WB-TUT-1~4(0.8절)와 모딩(3.7절 14차)은 이식 대상 밖 추가 기능이다. 남은 일은 표의 '남은 상위 체크'(3.1절 미체크 상위 항목), 3.2절 M5, 3.7절 후순위 요구다. 4-D는 E1346에서 자동 검사 허용 범위85/85를 마쳤다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 | 남은 상위 체크(3.1) |
| --- | --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료(4-C 통합 잔여는 4D-3에서 닫음) | — | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **85/85, L2·Tutorial 자동 검사 허용 범위 닫음** | 0 / 0 | 건너뛴 공격·포획·희생·승리는1절 사용자 확인 |
| 5 | 5-A 렌더러·Draw | 46/46 | 0 / 0 | — |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 13/13 | 0 / 0 | — |
| 5 | 5-C 화면 허브·미션 진입 | 6/6(E1325) | 0 / 0 | — |
| 6 | 입력·게임 메뉴·경제·건설 | 10/10 | 0 / 0 | — |
| 7 | 전투·의식·AI | 7/7 | 0 / 0 | E1385 전투 바인딩·AI7화면 완료 |
| 8 | 미션 수명·대본·오디오·도움말 | 8/9 → M4 | 0 / 0 | M4 |
| 9 | 표시 품질·브라우저/OS·배포 | 자동 검증 범위 완료(3.2절 목록), 물리 확인 미완료 | 0 / 0 | M5(3.2절: M5-a 물리 60/120Hz·M5-b 모니터 이동·M5-c 와이드 전투 육안·청취) |
| 10 | AGENTS 후순위 웹 요구 | 3.7절 참조(5·7·11·12·13·15차 끝남, 8·9차 제외) | 0 / 0 | 4차 와이드 종합(=M5-c)·6차 사람 검수·10차 자료 미확정 14건·14차 DN 적합성 |
| **합계** | | | **0 / 0** | **1**(3.1절 M4; M5·3.7절 남은 일은 위 두 행, 추가 기능 WB-TUT-1~4는 0.8절에서 완료) |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,007단위는 **port 이상 1,007(100%; `port` 275·`session` 732)·partial 0·todo 0**이다(2026-11-29 정리 재집계, E1328 이후 같음). 제외 153단위(멀티 73·Edit 42·호스트 37·구판 1)를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session` 732행은 E1283 에서 `e2e/session-coverage.spec.js`(고정 시각 Tutorial1 4:3 130프레임 V8 함수 커버리지)와 `tools/port-map-session.mjs` 로 올렸다: **행의 웹 모듈이 모두 그 실행에서 함수를 하나 이상 실행했을 때만** 올리는 보수적 판정이며, 남은 `port` 275행의 `session` 승격은 시나리오를 늘려 같은 커버리지 도구로 재검사한다(전투·AI·의식·편집 아닌 UI 등). 오라클 대조를 뜻하는 `oracle`은 0이다(승격 규칙은 이후 시나리오가 늘 때 같은 도구로 다시 돌린다; [근거](docs/webpj-port-map-session-20261123.md)). E1098~E1237의 재판정 내역은 대응표 메모와 [E1166 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)·[E1237 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에, E1238~E1288의 행 닫기는 대응표 메모와 [E1288 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 573 / 573 | 0 | 0 |
| `client/` 세션·UI·표시 | 433 / 433 | 0 | 0 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 partial·todo 단위는 0이다. 재기준선 추가분 RawClockSynchronization은 E1317에서 플랫폼 적응으로 닫았다. 메모 머리 태그와 이전 집계는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), 현행 계열은 `python -X utf8 tools/port_inventory.py --families wb`(남은 계열이 없어 지금은 출력이 없다)를 따른다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 16:9 1280 논리 화면 확인용 CP1384 오라클(`3f12a2bd4`, `webpj/tests/fixtures/cpppj/resolution-1280-20261128/`)은 E1488에서 따로 추가했다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 1,337항목(E1488, 현행 fixture-manifest 검사 기준)을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

### 0.5 단위 착수와 종료

1. AGENTS.md와 공통·dotnetpj·웹 인계를 읽고 Git 상태·원격 변경을 확인한다. 같은 작업 트리의 다른 세션 변경을 고려해 파일명을 지정해 stage한다.
2. 대응표 검사와 기준선 뒤 변경을 확인한다. 3.1절의 단계 순서와 선행 조건에 맞춰 기준선 cpppj 코드, 기존 시험, fixture, 단위 문서를 읽는다. 큰 단위는 공개 API 경계로 나눈다.
3. 원본/클론·오라클 수집·별도 창 실행에는 AGENTS.md의 확인·예외 규칙을 적용한다. 웹 이식에는 디컴파일 소스 분석이 필요 없다.
4. 단위 문서에 근거 커밋·fixture/오라클·검사·실패와 교정·미연결 경계를 기록한다. 대응표의 web·base(8자리)·status·메모와 해당 manifest를 갱신한다. 현행 인계에는 현재 값·다음 작업·검증과 해당 체크만 갱신하고 완료 항목 상세는 정리 때 이관한다.
5. 각 작업 단위마다 해당 변경의 집중 검사와 연관 검사를 수행한다. HJOW-Athlon에서는 완료된 작업 단위 4회마다 Node v26 전체 회귀를, 8회마다 Node v22 전체 회귀를 수행한다. 두 버전의 전체 회귀가 함께 필요한 경우 순차 실행하며 동시에 실행하지 않는다. 다른 PC에서는 사용 가능한 최신 Node 버전으로 완료된 작업 단위 4회마다 전체 회귀를 수행한다. 필요하면 변경의 위험이나 통합 범위에 맞춰 추가 검사를 수행한다. 신호 NaN 비트 대조 시험은 맨 위에 `import './nan-exact.mjs'`를 둔다. (2026-11-19 사용자 지시)
6. `www` 소스를 바꾼 단위는 `npm --prefix webpj run build`로 webpack 번들(`www/dist/netstorm.bundle.js`)을 다시 만들어 함께 stage 한다(번들 Git 추적, E1297; `tests/bundle-fresh.test.mjs`가 낡은 번들을 실패로 잡는다). JavaScript 수정은 `npm --prefix webpj run lint`를 통과시킨 뒤 번들을 만들며(빌드도 ESLint 선행 관문을 거친다, E1400), 기존 예외를 고친 경우 `--prune-suppressions`로 줄인다. 완료 후 당일 최대 번호를 확인하고 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. 기존 커밋의 Codex 공동 작성자 표기와 AGENTS.md의 push 금지 규칙을 따른다.

### 0.6 제품 공통 요구

11.01 표기·Credits 세 번째 항목과 HJOW/AI 문구·`Copyright 2026 HJOW`, 시작 클릭 화면, 공개 루트 내 실행 자산, 외부 접속 문서화·유효성 검사 비활성화는 반영했다. Credits AI 문구는 사용자가 다시 지시할 때 수정한다. **15차 Reset All은 E863에서 구현·검증했다.** 설정·진행 JSON 복사/불러오기(사용자 요구 2026-11-28)는 E1487에서 Options 최하단에 넣었다([근거](docs/game-user-followups-20261128.md)). [제품 11.01](docs/webpj-product-1101-20261105.md) · [외부 접속](docs/webpj-external-access-20261105.md) · [초기화](docs/webpj-reset-all-20261119.md)

<a id="favicon"></a>

- [x] **favicon 탭 시각 확인 — E1446 완료.** 실제Chrome전체창의localhost:9803 NetStorm탭왼쪽노랑/파랑NS아이콘을직접시각확인했다. 현재웹/공통ICO바이트동일과E894원본PE동일성도유지한다. 초기도구시간초과/반환창재선정은분리기록했다. [창증빙/근거](docs/webpj-favicon-20261119.md).

<a id="about-version"></a>

- [x] **정보(About) ▸ 버전 창의 Copyright(사용자 요구 2026-11-23) — E1289 완료.** 게임 메뉴·메인 메뉴의 버전 창에 `NetStorm 11.01 Patch by HJOW.`와 `Copyright 2026 HJOW`를 한 번만 넣었다(cpppj 1330과 같음). [근거](docs/webpj-about-version-copyright-20261123.md) · [공통](LEFT_JOBS.md#user-requests-20261123)

<a id="korean-font-change"></a>

### 0.7 한국어 갈무리11

E896에서 실제 미션 설치의 You→당신 번역과 Player 표시 바이트를 연결했고, 제공된 일반·굵은 WOFF2와 라이선스를 공개 데이터에 넣어 로딩·Canvas·자료 준비가 같은 글꼴 설정을 공유한다. Chromium 메뉴·도움말(영어 대체 포함)과 Tutorial1 한국어 브리핑 두 페이지에서 확인했다. [설치](docs/webpj-app-install-display-20261119.md) · [구현·검증](docs/webpj-korean-galmuri11-20261117.md) · [한국어 미션](docs/webpj-korean-tutorial1-20261120.md)

- **글자 품질(E1244·E1254·E1259~E1261·E1288):** 갈무리11은 12px 설계 격자로 그린다(제목 24·슬롯 4 48px, 4배 그리기·칸 평균, E1288). 숫자·영문·공백은 cpppj처럼 원본 비트맵 글리프, 슬롯 1·6의 한글 기준선은 `max(원본, 배율×11)`, 큰 슬롯은 원문 정수 배율(E1259), Tell 본문 폭은 가장 긴 낱말 이상(E1260), 굵은 글꼴에 없는 8,271자는 일반 갈무리11로 대체(E1244)한다. 게임 안 글자는 브라우저 최소·기본 글꼴 설정과 무관한 고정 크기다(예외: 웹 전용 ☰ 메뉴 DOM 버튼). [12px 격자](docs/webpj-korean-glyph-grid-20261123.md) · [제목 판독](docs/webpj-korean-title-legibility-20261123.md) · [굵은 글꼴 대체](docs/webpj-galmuri-bold-fallback-20261123.md)
- **사용자 결정:** 굵기는 원문 Bold 대신 지금의 보통체를 유지한다(2026-11-23 "지금은 그대로"). 글자만 출력 해상도로 덧그리는 고해상도 글자 층은 도입하지 않고 현재 픽셀 표시를 유지하기로 사용자가 결정했다(2026-11-24, 3.5절). cpppj와 다르게 둔 점(제목 배율·굵기·한글 기준선)은 근거 문서에 있다. 같은 점검의 cpppj·dotnetpj 후속은 사용자 지시로 작업을 보류하고 [LEFT_JOBS.md](LEFT_JOBS.md)·[LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)에만 기록했다(`92ea7bfd1`).

- [x] **전역 번역 누락 재점검(2026-11-28, E1485).** 공통 Tell의 영어 대체32곳·HUD/채팅 표시를 보완하고 소스 참조 기반 누락 검사·실제 창 배치를 확인했다. 기존 ESC 메뉴 행 점검(E1290)과 첫 안내31미션 점검(E1430)을 공통 확인창/경고까지 확장했다. 문장 사람 검수 완료를 뜻하지 않는다. [근거](docs/webpj-global-korean-audit-20261128.md).

<a id="tutorial-objective-port"></a>

### 0.8 사용자 추가 요구 — 튜토리얼 다음 단계 조건 안내

**2026-11-27 필수 반영, TUT-1~4 완료(E1386~E1389).** 공식 Tutorial1~6/40단계에서 다음 알림창·미션 종료창의 실제 조건을 우측 하단에 배경 없이 계속 표시한다. 실제 gate·예약/타이머는 읽기만 하고 진행/승패를 바꾸지 않는다. 기존 원본 이식의85/85·1,007단위 완료에 포함하지 않는 추가 기능이다. [공통 계약](docs/tutorial-objectives-porting-20261127.md)·[CP 화면/근거](docs/exe/cpp-tutorial-objective-display-1372-20261127.md).

- [x] **WB-TUT-1 조건·문구 — E1386.** 실제40단계/39키·읽기 전용 gate 조회와 영어/한국어 공개 자산을 반영했다. CP 카탈로그·예약/타이머·범위 제외·중복/누락0·번들 등11/11 통과. 일본어39키도 E1410/E1412에서 공용 확정본으로 반영했다. [근거](docs/webpj-tutorial-objective-catalog-20261127.md).
- [x] **WB-TUT-2 우측 하단 표시 — E1387.** 실제 Gump 글리프·여백8/최대폭440/간격2·보통체1픽셀 그림자, 배경 없음·입력 투과·크기 변경/부모 삭제 수명을 구현했다. 기존x86관측과3화면비/입력/픽셀 등28/28 통과. [근거](docs/webpj-tutorial-objective-gump-20261127.md).
- [x] **WB-TUT-3 진행·수명 — E1388.** 실제 Guide/World 토큰·gate·창/메뉴 가림·복원과 State/World/Client 정리에 연결했다. 덤프/조회 불변·실제 반납 등25/25, Chromium F4 A→B·F8·ESC1경로 통과. 화면비·전환 확대 검증은 TUT-4다. [근거](docs/webpj-tutorial-objective-lifetime-20261127.md).
- [x] **WB-TUT-4 검증·배포 — E1389.** 실제 F4/F8·영어/한국어3화면비·F1/F6·입력/리사이즈·재시작/전환/이탈/destroy와 실제39키 글꼴 부품·원본L2/M2 대조16경로를 확인했다. Node26/22 전체 각각6,878/6,878·번들/자산 검증 완료. 후반 목표 수행은 사용자 확인으로 남긴다. [근거/증빙](docs/webpj-tutorial-objective-verification-20261127.md).

## 1. 범위와 유지되는 결정

- **범위.** 일반 31경로와 데모 3경로를 대상으로 한다. 단독 미션의 같은 프로세스 안 내부 전송·패킷·Login·Zone·서버/클라이언트는 포함한다. LAN TCP·멀티플레이 UI·재접속/메타, 공식 검증 서버, Edit·커스텀 맵·User Made Campaigns, 설치/CD·Quit, 옵션 해상도/전체화면은 웹 범위에서 제외한다. 스커미시의 MP 시작 규칙과 Outpost는 12차에 포함한다.
- **이식.** cpppj의 파일과 API 경계, 부작용과 호출 순서, 정수 폭과 저장 시 정밀도, 난수 소비를 보존한다. 단독에서 도달하는지 확인해 웹 제외 분기에는 명시 오류를 유지한다. 현재 단계가 직접 요구하는 부품만 앞당긴다.
- **웹 구조.** `netstorm.js`가 앱 수명을 맡는다. 모든 스크립트는 webpack 으로 번들 하나(`www/dist/netstorm.bundle.js`, 진입 `www/bootstrap.js`)로 묶어 `index.html`이 그것만 불러온다(E1293, 번들은 사용자 결정으로 Git 에 추적한다 — `www` 소스를 고친 단위는 `npm --prefix webpj run build`로 번들을 다시 만들어 함께 커밋하며 `tests/bundle-fresh.test.mjs`가 낡은 번들을 잡는다). 번들 없이 이전 방식(ES 모듈 직접 적재)으로 구동하는 디버그 페이지는 `www/debug.html`이며, 앱과 같은 모듈 인스턴스가 필요한 e2e(`import()` 사용 11개 파일)도 이 페이지를 연다. `index.html`은 최소 변경하고 스타일은 JS에서 관리한다. 기존 CommonJS 서버와 `www/` ES 모듈 경계를 유지한다. 코어를 DOM·Canvas·Web Audio에서 분리하고 8비트 논리 화면과 최종 표시를 구분한다.
- **시간·입력·수명.** 시작 입력에서 오디오를 활성화한다. 탭이 숨으면 진행을 멈추고 복귀 시 시계를 다시 잡는다. `requestAnimationFrame`은 표시 기회다. 4:3부터 확인하고 16:9·16:10, 입력 역변환·DPI를 비교한다. 가장자리 스크롤은 활성 미션과 게임 영역 안의 포인터에서 동작한다.
- **자산·설정·언어.** `assets/game-data`·`fonts`·`webpj/translations`에서 `webpj/www/data`를 준비하며 느슨한 파일→TAFF 조회 순서를 유지한다. 버전 있는 `netstorm.settings`·`netstorm.progress` JSON은 localStorage에 두며 오류 시에도 계속할 수 있게 한다. 한국어 누락 파일·섹션·주제는 영어로 대체하고 미션 문구는 cpppj 확정본을 기본으로 삼는다. 사용자 검수와 언어별 글꼴은 3.5·3.7절을 따른다.
- **외부 링크.** Tell/도움말에서 치환된 `http:`·`https:`·`mailto:` 링크는 새 탭/창으로 열고 다른 주소의 제외 사유는 console에 기록한다. 개발 서버는 모든 인터페이스에서 수신한다. [외부 접속 7절](docs/webpj-external-access-20261105.md)
- **사용자 확인 범위.** 2026-11-29 goal로1-1~1-5·스커미시의 아군 AI 목표 수행 검사를 허용한다. 실제 승리·저장 확인은1-1·1-2·1-5·스커미시가 완료이고1-3/1-4는 E1494 패배/상한으로 미확인이다. 1-6/그 이후와 튜토리얼 후반의 공격·포획·희생·승리는 생략하고 사용자가 확인한다. 단순 진입/대기/생산/건설은 모든 미션에 허용한다. 전체 외관/음향 동일성과 한국어 사람 검수는 별도 사용자 확인 범위다.
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
| E802~E950 | 입력 클릭·NB20 취소, GameWorld 프레임·세 운반 유닛 도착, 워크샾 Kernel 완공·연구 동기화, 전투 완료 관문, Client 진단·프레임 시계, Session 덤프 형식과 오라클 시나리오(4D-9b), 앱 Guide·State7·Gump 루트·팔레트·Screen·입력 큐·TextEntry/IME·갈무리11 편집, Reset All(E863), 16:9 최대 1920×1080(E943; E1488에서 논리 상한 1280×720·800·960·1024로 변경), 하위 경로 배포(E945). [E1081 보관본 0.1·3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081) |
| E951~E1339 | 미션 앱 프레임(로딩·Start·음향·Renderer/Gump 출력)과 브라우저 입력·카메라·조작키, 31개 미션·데모·스커미시 진입과 장시간 진행, 생산·건설·객체 메뉴·게임 메뉴막대·승패 뒤 흐름·도움말, 한국어 대본·게임 안 문구·글자 품질, HUD 도킹·채팅 창, 웹 범위 판정과 Native 대응 대조, 감사와 partial 34→0행(E1328), 고정 시각 L2(E1283)·Tutorial1~3 L3 완주와 Tutorial4 단계 E1 진입(E1335), 원본 캡처·Session 대조, 사용자 추가 요구 세 건(버전 창 Copyright·한국어 ESC 메뉴·방향키 카메라), webpack 번들, 처음 방문 시작 화면 안정화, cpppj 1335 마나 그림 보충(E1336), Tutorial4 자동 검사 범위 제한(E1337)·일반 프레임 예외 뒤 계속 진행(E1338)·Tutorial5 단계 A1 L3(E1339). 주제별 표는 [E1380 보관본 0.1 진행 요약](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)에 있다. |
| E1340~E1488 | Tutorial5·6 허용 구간과 캠페인 16개 진입·대기 L3(E1365), CP 변경 16행 판정(E1348), 가장자리 스크롤·저장 사제 경로 L3(E1366~E1367), AI 행동 대조(E1370), WebMCP(E1372), HUD 생성·State 수명과 전투 7화면 0픽셀(E1385), 튜토리얼 조건 안내(E1389), 1-1·1-2·1-5·스커미시 실제 아군 AI 승리, 메모리·SID·그래프 회복, 성능·고DPI, 일본어(13차), Outpost(12차), 모딩 API(14차), 사용자 요구(E1486~E1488). E1382 이후 주제별 표는 [0.1 완료 이력 요약](#e951-e1081), E1340~E1380 행 원문은 [E1380 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)에 있다. |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다. 여러 독립 작업을 묶은 항목은 `- [ ]` 세부 단위로 나누고 세부마다 완료 기준을 적는다(세부 단위는 머리 수에 넣지 않는다, 0.2절). 닫힌 체크는 단계마다 '완료한 체크' 줄이나 표로 줄이고 점검·완료 근거 원문은 보관본에 둔다.

#### 4-D Session 조립·Tutorial1 Load/Start (85/85)

**완료 기준 L2는 E1283에서 닫았다.** 같은 시점·난수에서 Tutorial1 Load/Start의 덤프·첫 프레임 논리 화면이 cpppj 오라클과 같다(영어 4:3·16:9, 90·130프레임: 화면 0픽셀, 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다름 — `player`의 +1·+2 바이트는 주소 잔재, Player 1 +0x68·+0x69는 Deck 할당 토큰의 표현·시점 차이). 기준선 구간 지도와 E633~E801의 완료 체크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), E802~E1166의 항목별 완료 나열은 [E1166 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166), 아래 닫힌 항목의 점검·완료 근거 원문(E1263~E1283)은 [E1288 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에, 그 뒤 항목별 닫은 근거 표 원문은 [E1329 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)에, Tutorial3 단계별 완료 줄과 Tutorial4 단계 A~E1 상세는 [E1336 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261125-through-wb04-e1336)에 있다. cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다.

- [x] **4D-4b Tutorial 게이트·표시 장치의 자동 검사 허용 범위** — Tutorial1~3 기존 완주(E1304·E1307·E1333), Tutorial4 E1 안내(E1335·E1337), Tutorial5 D 안내(E1342), Tutorial6 E 안내(E1343~E1346)까지 완료했다. 이후 특정 적 공격·포획·희생·승리 자연 전환은 사용자 확인이다. E1336 뒤 Tutorial4 a220의 알려진 생산 메뉴 차이8,656픽셀을 유지하며 이전0픽셀 결과를 새 판에 확대하지 않는다. 단계별 화면/덤프 수치·입력 표는 [정리 전3.1](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)과 [E1346](docs/webpj-l3-tutorial6-crossbow-build-20261126.md)에 있다. 새 진행 조건 안내는 [WB-TUT-1~4](#tutorial-objective-port)로 별도 관리한다.

완료한 체크(머리글 수에 포함; 항목별 닫은 근거 표 원문은 [E1329 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)):
- **4D-1~4D-3:** 4D-1 외곽·snapshot(E946·E1270, 비미션 너겟은 미션 모드라 미도달), 4D-2 typed Load(E697), 4D-3a Login(E1263), 4D-3b ClientCampaignTransport(E1274), 4D-3c 전송 앞단(E1268), 4D-3d Squid 수신(E1263), 4D-3e 명령 등록(E1267), 4D-3f 통합 시험(E1263). [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md) · [기본 수신기](docs/webpj-client-default-receivers-scope-20261123.md)
- **4D-4a~4D-7:** 4D-4a 카메라·프레임(E1266, 주 루프 순서), 4D-5a 종료 수명·4D-5b Client State 정리(`_releaseCampaignWorld`)·4D-5c 이탈·전투 판정(`missionPost` 관문)(E1263·E1268), 4D-6a GameWorld·4D-6b Client 서비스(CampaignClientServices port E1296)·4D-6c Session 공개 조회(E1263), 4D-7a~7d ClientMain·Client 전역(E1263; 일반 프레임 예외 뒤 다음 프레임 진행은 E1338, 3.5절 2026-11-26 결정). [프레임 순서](docs/webpj-frame-order-20261123.md) · [이탈 정리](docs/webpj-mission-leave-state-reset-20261123.md)
- **4D-8~4D-10:** 4D-8a RawSurface 삭제·수명(E1273·E1321), 4D-8b~8f Native 분기(E1193~E1221)·4D-8g `UsesNativeTransmissionDevices()` 재대조(E1263), 4D-9a·9b 덤프·오라클과 앱 `sessionDump()`(E869·E1263), 4D-10a 실제 소유자 재대조(E1263), 4D-10b L2 판정(E1283). [정상 복귀](docs/webpj-destroy-patch-continue-20261124.md) · [Native 수신기](docs/webpj-native-receivers-audit-20261122.md) · [고정 시각](docs/webpj-fixed-time-mode-20261123.md)

#### 5-A 렌더러·Draw·시각 시스템 (46/46)

완료한 체크(머리글 수에 포함, 보관한 44개 외; 아래 두 개의 본문은 [E1327 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)):
- **ClientRendererFrame** — E1298. 미션 화면 출력의 원본 순서(E978~E1082)와 메인 메뉴 Kencloud 구름 캐시(M1 구름 차이 37,054픽셀 → 0). [Kencloud](docs/webpj-menu-kencloud-20261124.md)
- **E197·E198·E201 재대조** — E1316. 파편·건물(E1275~E1276)·골렘/받침(E1312) 캡처와 실제 Session의 EvilRay(E1314)·피해 연소(E1315)·섬 투하(E1316)를 C++ 시험과 대조했다. [마지막 근거](docs/webpj-drop-island-session-20261124.md)

#### 5-B 카메라·HUD·미니맵 (13/13)

완료한 체크(머리글 수에 포함, 보관한 9개 외): **UberHud**(E1255, [근거](docs/webpj-uber-hud-close-20261123.md)), **RawMinimapControls**(E1248, [근거](docs/webpj-minimap-controls-close-20261123.md)), **FloatingSpGump·RawProductionControls**(E1257, [근거](docs/webpj-production-controls-close-20261123.md)), **Cursor·RawCursor 표시**(E1366: L2/L3 카메라 투영·커서(E1280~E1286), 가장자리 스크롤 입력 대응(C++ `inspectPoll` 좌버튼 뗌·웹 N−1프레임)과 오른쪽 스크롤·정지 4화면 0픽셀, 마나 볼트(E1332~E1333), [근거](docs/webpj-l3-campaign-edge-scroll-20261126.md)).

#### 5-C 게임 화면 허브·미션 진입 (6/6)

완료한 체크(머리글 수에 포함): **Uber 허브**(E912~E917·E1082·E1169~E1179, 배경 GIF·NSInfo 자식은 웹 범위 밖 E1243, [판정](docs/webpj-uber-root-background-scope-20261123.md)), **미션 진입 명령**(E1263, [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md)), **허브 fixture**(E1265, [근거](docs/webpj-chat-hub-fixtures-20261123.md)), **메인 메뉴·타이틀 상태**(E1184, [문서](docs/webpj-title-screens-20261122.md)), **메뉴·버튼 컨트롤**(E1325, RawButtonControls·RawMenuChat·RawMenuHtml·RawMenuControls·RawMenuLoading·RawTextEntryControls port, [메뉴 컨트롤](docs/webpj-menu-controls-close-20261123.md)), **M2 / L4a**(E1323, 영어 4:3·16:9·16:10과 한국어, [한국어](docs/webpj-korean-m2-20261124.md)).

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (10/10)

완료한 체크(머리글 수에 포함, 보관한 1개 외): **입력 본체**(E1263, [E802](docs/webpj-campaign-user-input-movement-click-20261118.md)), **게임 메뉴**(E1264, [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md)), **공장·지식 메뉴와 기술**(E1076~E1079·E1189, [E1189](docs/webpj-fury-war-technology-20261122.md)), **객체 우클릭 메뉴**(E1310, [e2e](docs/webpj-object-menu-rows-20261124.md)), **방향키 카메라 이동(사용자 요구 2026-11-23)**(E1291, [근거](docs/webpj-arrow-camera-20261124.md)), **ESC 게임 메뉴 한국어 점검(사용자 제보 2026-11-23)**(E1290, [근거](docs/webpj-esc-menu-korean-20261123.md)), **건설·생산·경제**(다리 조각 연속 배치 E1299~E1307로 마감, 건설 기록 재생은 웹 범위 밖 E1242, [다리 조각](docs/webpj-l3-tutorial1-20261123.md#확장-다리-조각-연속-배치wb04-e1299-2026-11-24)), **M3 / L3**(Tutorial1 입력 단계표 세부 넷 E1284~E1307, Tutorial3~6 확장은 4D-4b, [L3](docs/webpj-l3-tutorial1-20261123.md)), **사제·운반·경로 바인딩**(운반 입력·사제 이동 E794·E1139, 제단 희생·기도·Cast/Drop·Factory 명령 E1223~E1227, Bulf 줍기/Drop E1311, 주문 포획 E1318~E1319, 저장 미션 경로 L3 E1367 4화면 0픽셀·덤프 일치; MetaMap은 도달하지 않음 E1188, [근거](docs/webpj-l3-campaign-priest-path-20261126.md) · [E794](docs/webpj-session-carrier-input-20261118.md)).

#### 7 전투·의식·AI (7/7)

완료한 체크(머리글 수에 포함): **파괴·주문 효과**(E1199), **의식**(E1180, 보상 선택 UI는 웹 범위 밖, [의식 판정](docs/webpj-ritual-reward-scope-20261122.md)), **AI 잔여·전투 이탈**(E1198), **전용 시험이 없었던 시스템**(E1316, [근거](docs/webpj-drop-island-session-20261124.md)), **주문(스펠) 사용·골렘 사제 포획(사용자 제보 2026-11-23)**(E1318~E1319, cpppj 1339도 같게 교정, [주문](docs/webpj-spell-cast-20261124.md) · [포획](docs/webpj-spell-priest-pickup-20261124.md)), **전투 바인딩**(Hit·GunQueries·Archer·Motion·BombBlast·Damageable 여섯 행의 Session 결선 `raw-campaign-session-{hits,guns,gun-views,combat,flyers,damageable}.js`, 교전 N프레임 대조 E1334~E1335, 전투 장면 오라클은 실제 AI 7덤프·7화면 0픽셀 E1385, [근거](docs/webpj-construction-memory-input-20261127.md)), **AI 세션 결선**(Start의 일반/데모 AI·동맹과 typed 프레임 연결 `raw-campaign-session-ai*.js`·`netstorm-campaign-ai.test.mjs` E1122~E1216, AI 행동 N프레임 대조 E1370, DestroyPatch 표시 후 정상 복귀 E1321,[근거](docs/webpj-campaign-ai-telemetry-20261126.md)).

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (8/9)

완료한 체크(머리글 수에 포함): **대본·대화창**(E1263, [E1190 점검](docs/webpj-script-commands-audit-20261122.md)), **소리 본체**(E1261), **음악**(E1258, [근거](docs/webpj-scene-music-close-20261123.md)), **글자·설정·그림**(E1261, [근거](docs/webpj-korean-title-legibility-20261123.md)), **적응 fixture**(E1263), **도움말·Tell**(E1197~E1240 해당 대응표 행 모두 port, [근거](docs/webpj-main-menu-help-type-20261123.md)), **동영상**(E1202 사용자 결정으로 웹 제외, [결정 기록](docs/webpj-intro-movie-decision-20261122.md)), **미션 수명**(E1085~E1161, [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md)). 실제 청취 판정은 사용자 확인 범위이다.

- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.
  - 완료: 캠페인 목록 31개 미션의 연속 시작(E1121, Priest Training 9개는 E1157에서 실제 연결하고 Auto-Demo가 대신 돈 e2e 거짓 통과를 정정), 캠페인 16개 장시간 진행과 AI 시점 미연결 서비스·좌표 오류 네 가지 수정·소리/안내 서비스 여섯 개 연결(E1122), Early Missions·Priest Training 15개 100초 진행(E1123)과 Priest Training 9개 150초씩 26분(E1158), 스커미시 AI 7명 300초(E1124), 데모 420초 진행과 360초 재시작(E1125), 후반 4개·중반 13개 장시간 진행으로 시계 정지 누수 수정(E1132~E1133), 몽키 시험(E1128·E1134·E1165, 죽은 Gump 정리 수정 포함), 승패 안내 뒤 흐름과 진행 저장·다음 미션 해제(E1152·E1161).
  - [ ] 자연 승패 진행 저장·자연 완주 — 허용 미션은 실제 아군 AI로 확인하고 나머지는 사용자가 확인한다.
    - [x] 승리·저장 확인 완료 — 1-1·1-2·1-5 실제 승리 → 신규 완료값 저장 → 새로고침 → 다음 미션 해제(E1394·E1369·E1371), 스커미시 승리/나가기/재실행(E1439).
    - [ ] 1-3·1-4 승리·저장 — 최신 지시로 자동 허용, E1494에서 실제1-3 패배(919.125초)·1-4 상한 미승리(1509.175초)를 확인했다. 게임 오류0이며 승리/저장/Next를 합성하지 않았다. 완료 기준: 실제 승리 → 신규 저장 → 새로고침 → 다음 미션 해제 결과. [근거](docs/webpj-autoplay-scope-20261129.md).
    - [ ] 1-6 승리·저장 — 목표 수행은 생략하고 사용자 직접 확인. 공용 아군 AI/승리 CLI가 시작 전에 거부한다(E1494). 완료 기준: 사용자 플레이 결과 기록.
    - [ ] 그 밖의 자연 완주 — 자동 허용 밖 미션(캠페인 2·3, Priest Training, Tutorial4 E1·Tutorial5 D·Tutorial6 E 이후 등)의 목표 수행·완주. 완료 기준: 사용자 확인 결과가 기록되고 지적 사항이 반영된다.
  - [x] 한국어31미션첫안내창/실제내용줄폭자동확인 — E1430. 중간안내/사람문장검수와분리한다. [범위/화면](docs/webpj-korean31-initial-layout-20261127.md).
  - [ ] 한국어 전체 확인 — 사람 검수(3.7절 6차)와 미션 본문 줄 나눔 실제 화면. 완료 기준: 사용자 검수 결과가 기록되고 지적 사항이 반영된다.
    - [ ] 중간 안내 실제 화면 — 미션 진행 중 나오는 안내의 줄 나눔(E1430 판정으로 사용자 확인 범위). 완료 기준: 안내별 화면 확인 기록.
    - [ ] 문장·용어 사람 검수 — 완료 기준: 지적 목록과 반영 단위가 기록된다(자동 점검 E1430·E1462·E1485는 완료).
  - 근거: [모든 미션 시작](docs/webpj-all-missions-start-20261120.md) · [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md) · [시계 정지 누수](docs/webpj-mission-leave-pause-20261121.md) · [Gump 정리](docs/webpj-gump-cleanup-frame-20261121.md) · [몽키·미니맵](docs/webpj-monkey-minimap-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다.

**M5 완료(자동 검증 범위):**
- 배포·저장: Windows 네 브라우저(Chromium·Chrome·Edge·Firefox)의 하위 경로 정적 모듈·자산 로드와 새로고침 뒤 설정 복원(E945), 조건부 HTTP 캐시·같은 URL 배포 교체(E1395), 1-1 완료값의 네 브라우저 복원(E1394). [E945](docs/webpj-static-deployment-20261119.md) · [캐시](docs/webpj-http-cache-20261127.md)
- Linux GUI: 세 화면비 L2·진행 저장 읽기·하위 경로(E1397), 비고정 시작 단독 재시도(E1402), 원래 6검사 6/6·동일 파일 번들 비교 12회(E1468), 최신 모드 8/8(E1484). [Linux 후속](docs/webpj-linux-startup-load-followup-20261128.md) · [최신 모드 Linux](docs/webpj-mod-linux-gui-20261128.md)
- 시계·메모리: 메뉴 60/120 선택의 게임 시계 경과(E1403), 짧은 반복(E1404)과 정상 rAF 30분 27회 반납/GC(E1435).
- 표시·성능: 최종 수집 정책의 와이드 실제 AI 승리와 16:10/4:3/와이드 복귀(E1443), 와이드 AI 미션 11.3~14.1 → 52.6~54.9FPS(E1449·E1454·E1456), 일반 Chrome GPU·원격 어댑터 32Hz와 headless SwiftShader 구분(E1460), 고DPI(E1461·E1462), 1280 논리 상한·미션 중 6크기 전환(E1488).

**M5 남음(물리 환경·사람 확인):**
- [ ] M5-a 물리 60/120Hz 표시 — 고주사율 실제 모니터에서 60/120 선택의 표시 프레임을 측정한다. 완료 기준: 물리 표시 환경의 측정 기록(원격 32Hz·headless·Xvfb 결과로 대체하지 않음). 이 환경의 60FPS 미달은 게임 계산만의 병목으로 단정하지 않는다(E1460).
- [ ] M5-b 물리 모니터 이동·OS 배율 변경 — 실제 창을 다른 모니터로 옮기거나 배율을 바꾸는 경우(E1461·E1462는 브라우저 배율 변경까지). 완료 기준: 사용자 확인 기록.
- [ ] M5-c 와이드 전투 종합 표시·소리 청취 — 모든 교전의 와이드 육안과 효과음·음악·위치음향 청취. 완료 기준: 사용자 확인 기록.
- 보존한 한계: E1468 반복 debug 탐색의 60초 실패 2건은 원인 미확정 관측 한계로 남기며 해결로 세지 않는다.

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`·`session-dump`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 웹은 검사 전용 고정 시각 모드(`?fixed=25`, `stepFixedFrames`, E1271)로 같은 프레임을 만든다. L2는 `e2e/fixed-time.spec.js`(E1283 닫음), L3는 같은 단계표를 웹에 적용하는 `e2e/l3-tutorial1.spec.js`(E1284)와 공통 틀 `e2e/l3-common.js`·`tools/l3-collect.mjs`(E1305)이며 웹 입력은 클릭 N+1·키 N−1 프레임에 넣는다. 난수 소비 순서는 cpp 임시 난수 추적(호출 스택 RVA와 링크 맵)으로 대조할 수 있다(E1280). L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md) · [난수 추적](docs/webpj-state-install-rng-20261123.md) · [L3](docs/webpj-l3-tutorial1-20261123.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 cpppj 제품은 1387(최신 ZIP 1378·검증 1380·M8 관문 1327)이며 웹 비교 기준선은 제품 1313이다. 1387 스커미시 선택 아이콘은 E1489에서 반영했고, 1386 생산 격자 부모 소실 보완은 E1491~E1492에서 재현·반영했다. 1371~1372 튜토리얼 안내는 E1386~E1389, 1382~1384 사용자 요구(정지 커서·튜토리얼 가격/난이도·1280 상한)는 E1486~E1488에서 웹에 반영했다(3.6절). 1314 최초 실행·1320 미션 전환(E616·E620), 1328~1330 사용자 추가 요구(방향키 E1291·ESC 한국어 E1290·버전 창 E1289), 1339 주문/포획 교정(웹 E1319와 같은 교정, 대응표 base `13e74579`)을 웹에 반영했다. 1331~1338은 시험 교정·배포 확인과 일본어(13차) 작업이며 소스 변경 판정은 3.6절에 있다(1335의 언어 무관 마나 그림 별칭·CD 25프레임 보충은 E1336에서 반영). 1340~1362는 일본어 본문 43개·도움말 128주제 번역과 최종 관문(13차)이며 그중 `cpppj/src` 변경은 1345의 AI 기본 이름 길이 제한뿐이다. 1363은 언어 무관 공통 도움말 플래그·숫자 칸 우클릭, 1364~1370은 TCP 상대 봇·LAN 멀티플레이(웹 제외 범위)이지만 port 분류 파일을 고쳐 둘 다 3.6절 판정 대상이다. 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목(1316~1319·1321·1324)은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 판정은 3.6절, 1323 아이콘은 favicon(0.6절)으로 따로 반영했다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·공격·승리·전체 화면/음향은 1절 범위를 따른다. 1-3/1-4는 최신 지시의 자동 허용 범위지만 E1494 패배/상한으로 승리는 미확인이고,1-6/그 이후 목표 수행은 사용자 직접 확인이다. 1-1·1-2·1-5·스커미시 실제 아군 AI 승리 검증은 끝났다. 단순 진입/첫 안내는 모든 캠페인에 허용한다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- 와이드 화면: 16:9 논리 크기 전환은 E1443(와이드 승리 뒤 화면비 복귀)·E1488(미션 중 6크기 전환)에서 확인했다. 모든 교전의 와이드 육안은 M5-c다. WebMCP는 E1372에서 Document API 지원 Chromium156의 실제 입력·캡처·소리 포함 녹화·다운로드로 완료했다. 미지원 브라우저의 일반 실행도 검증했다. [사용법](docs/webpj-webmcp-20261126.md)
- **사용자 결정 완료(2026-11-24).** 채팅 로그 앱 수명 유지(E1320, E1170의 미션 종료 초기화 적응 해제)·DestroyPatch 진단 열두 지점 표시 후 계속(E1321)·16:10 논리 크기 1280×800(E1322)·한국어 고해상도 글자 층 미도입과 보통체 유지(0.7절). 결정 문장 원문은 [E1339 보관본 3.5절](LEFT_JOBS.webpj.old.md#archive-webpj-20261126-through-wb04-e1339)에 있다.
- **사용자 결정 완료(2026-11-25, 인계 정리 뒤 질문에 답변).**
  - 사제 포획/희생이 들어간 L3 대조(Tutorial4 단계 E1~I1·Tutorial5 단계 E~G)는 작업 지시의 건너뛸 시험으로 보고 건너뛴다. 사용자가 직접 확인한다(3.1절 4D-4b).
  - 7단계 '교전 N프레임 대조'는 플레이어가 공격하는 교전 체크포인트 일치(E1334~E1335)로 완료 표시를 유지한다. AI가 공격해 오는 장면은 '전투 장면 오라클'·'AI 행동 N프레임 대조'에 남는다.
- **사용자 결정(2026-11-26).** 일반 프레임 예외는 상세 오류를 기록한 뒤 다음 프레임을 진행한다. E1338에서 구현하고 실제 미션의 한 프레임 실패 뒤 진행을 검증했다([근거](docs/webpj-frame-error-continue-20261126.md)). DestroyPatch 정상 복귀와 구별한다.
- **검사 환경 결정(2026-11-26, 해결).** 허용 미션의 검사 helper가 원래 Start 직후 실제 Session AI를 활성화한다(E1349, 현재 SP·이름·동맹·기술 보존, 제품 CampaignTestPlay 제외 분류 유지, [절차](docs/webpj-ally-ai-test-control-20261126.md)). `webpj/extracted/` 무시 규칙은 사용자 커밋 `6b9b11ccf`로 해결됐다.
- **원본 실행·사람 확인.** 도움말·안내 마나 아이콘(E1336) 육안 대조, 주변음·효과음·음악 실제 청취, 한국어 사람 검수와 미션 본문 줄 나눔 실제 화면, 시작 화면 안정화(E1300·E1308)의 배포본 처음 방문. 예산 칸 원본 우클릭은 E1324에서 확인했다.

<a id="cpp-delta"></a>

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다.
- **현재 재집계(2026-11-29 E1492):** 대응표1,160행/오류0·port275/session732·partial/todo0이다. CP1384 메뉴 주석은 E1490 ①·`RawGameMenus.cpp` base `3f12a2bd`, CP1386 생산 격자는 E1492 ②·`RawProductionControls.cpp` base `0cca2c8dd`로 닫았다. 남은 변경 보고는 제외 분류 `ScreenResolutions.h`의① 자동 기록뿐이다. [CP1384](docs/webpj-cpp1384-menu-judgment-20261129.md)·[CP1386](docs/webpj-production-grid-parent-loss-20261129.md). CP1382 커서 E1486·CP1383 튜토리얼/난이도 E1487·CP1384 논리 화면 E1488과 정리1381 당시 변경9행/신규10파일 E1387~E1393도 완료다. 고정 기준선225553df/대상1,007단위/오라클은 올리지 않았다. [이전 행·파일별 근거](docs/webpj-login-reconnect-sp-20261127.md)·[감사 JSON](docs/webpj-e1393-evidence/cpp-delta-judgments.json).
- **판정 끝:** cpppj 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 변경 아홉 행(E1241, base `63590635`·`710b9647`·`baf141eb`, [판정표](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md)), 사용자 추가 요구 1328~1330(E1289~E1291), 1339 주문/포획 교정(E1319, base `13e74579`), SoundMusicSelection base 정정(2026-11-22, `225553df`), 일본어(13차) 커밋 `5238ee833`(1334)·`87324232f`(1335)의 여섯 행(E1336: 마나 그림 별칭 `imana.8`~`11`·`17`→`Imana.4`~`7`·`9`와 CD 마나 25프레임 보충의 `HtmlTextParser`·`Markup`·`StyleTextMarkup`·`UberGump` 행은 ② 반영, `KoreanResources.cpp`(`.japanese` 번들 읽기)·`StyleTextBuffer.cpp`(가나·한자 줄바꿈)는 ③ 13차(E1410·E1412 반영), [근거](docs/webpj-mana-picture-supplement-20261125.md)). 커밋·행별 경과는 [E1329 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)과 [E1336 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261125-through-wb04-e1336)에 있다.
- **2026-11-26 변경 판정 완료(E1347~E1348):** 공통 도움말·확장 AI 이름은 E1347, 나머지16행은①11·②5로 E1348에서 닫았다. 미션2bh 채널 해제와 Fort 읽기 오류 문맥도 실제 웹 경로에 반영했다. 당시 기존 행 변경 보고0이었으며 E1393에서 마감한 후속9행/신규10파일과 구별한다. [16행 판정표](docs/webpj-cpppj-lan-change-audit-20261126.md)
  - [x] 언어 무관 공통 변경 — E1347: 도움말 LOOK/HELP·빈 값 규칙의 기존 일치를 확인하고 첫 NUL 절단·확장 AI 기본 이름31BYTE 제한을② 반영했다. 한국어에도 적용된다. [근거](docs/webpj-cpppj-common-help-ai-name-20261126.md)
  - [x] LAN 변경16행 — E1348,①11·②5·③0. 미션에도 선택되는2bh removeChat의 선언·장부·수신기·현재 앱과 공통 Fort 물리 읽기 진단을 반영했다. 나머지는 일반 TCP/LAN/외부 시각 보정·진단 전용 경로로 판정하고16행base를 갱신했다. [근거](docs/webpj-cpppj-lan-change-audit-20261126.md)
- **대응표 밖10파일의 판정:** E1393의 파일별 표/JSON을 따른다. TutorialObjective.cpp/.h/Catalog.inc 3파일은 필수 기능으로 E1386~E1389에서 반영했고, 외부 글꼴 Native 경계·Reset All은 플랫폼 적응, ManaPicture 자료 2파일은 E1336 반영이다. 다음 재기준선 때 등록하며 현재 이식 완료율에 혼합하지 않는다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | 없음. 아비어리 본체/Host 송신/실제 수신·공용 Form 경계는 E1444에서 현재 소스와 46/46으로 판정했다(E1210은 Fort 가상 조회 참조로 정정). 이탈 State 정리(E1268)·Client 기본 수신기(E1274)·분리 Poll과 `pollClientConnectionHealth`(4D-7b)·RawClockSynchronization 플랫폼 적응(E1317)은 닫혔고 PacketDelays·RawPacketCopies·RawMonitorTime은 도달하지 않는다. |
| 4D-8 | 없음. SID 소진 회수/진단(E1433), 건설 차단/충돌 마스크/지역 요청 수·공급자 순환(E1440), Native GameAux 잔여 24칸 = 현재 CPP typed 선택 대응 21·원격/개발자 제외 3(E1451·E1457; Native 129 주소 동일성은 제공하지 않고 이름 감사만으로 누락 판정하지 않음), DestroyPatch 정상 복귀(E1321)·`rememberPlacement`(E1273)·기본 수신기(E1274)·나머지 Native 경계(E1193~E1221). [헤더](docs/webpj-gameaux-native-header-audit-20261127.md)·[역할/이벤트/진단](docs/webpj-gameaux-native-role-audit-20261127.md) |
| 5-A~5-C | 없음. ‘Creating Islands.’ 안내(`creatingIslands`, `RawMissionServerPackets.cpp:60-63`의 `createArchipelago`)는 로컬 미션 Zone `0x82000004`(`local-mission-transport.js:189`)에 0x100 비트가 없어 도달하지 않는다(E1328). |
| 6~7 | 없음. 건설 ActualConfirm/ActualPlace·동맹 협상 입력·RawGunHit Rain override는 끝났고 건설 기록은 개발자 전용으로 제외했다(E1242). RawPathRuntime/RawPathData는 '사제·운반·경로 바인딩' 항목이 이어받는다. |
| 8~10 | 한국어 사람 검수(3.7절 6차)만 남았다. 일본어 자원 우선순위·입력(13차)은 E1410·E1412에서 반영했다. TellSection·Frame Rate·Difficulty·NB10·스커미시 일반 시작은 끝났다(E1263 점검). |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했고 Native 대여는 E1193~E1194에서 끝났다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | 웹 논리 화면은 E1488부터 화면비별 최대 1280×720(16:9)·1280×800(16:10)·1280×960(4:3)·1280×1024(5:4)다. 작은 4:3 창은 원본 1024×768 등 기존 크기를 유지하고 더 큰 창은 CSS/DPR 표시만 확대한다(FSR은 웹 제외; AGENTS `33aae129b` 반영, E943의 최대 1920×1080을 대체). E1366의 카메라·HUD 단계13/13과 E1372의 WebMCP 실제 입력·PNG·효과음/음악 WebM·다운로드를 완료했다. 와이드 전투의 종합 확인은 M5-c다. [해상도](docs/resolution-policy-1280-20261128.md) · [WebMCP](docs/webpj-webmcp-20261126.md) |
| 5 | localStorage 설정·진행 저장과 오류 대응, Reset All을 구현했다. E1394·E1369·E1371에서 실제1-1·1-2·1-5 승리의 신규 완료값 저장·새로고침 유지·다음 미션 해제를 확인했다. |
| 6 | 메뉴·Tell·도움말·갈무리11, 모든 미션 대본(E1108~E1116)과 게임 안 메뉴·객체 메뉴·미션 화면·채팅 문구(E1118~E1250), 글자 품질([0.7절](#korean-font-change)), 5-C M2 한국어 오라클(E1323), 31미션 첫 안내 줄폭(E1430), 고DPI(E1462), 전역 공통 창/경고 32곳·미니맵·채팅·사제 수 보완(E1485, 원본 미니맵 제목의 고정 영어는 웹 표시만 번역)까지 구현했다. 중간 안내의 실제 화면과 전체 문장의 사람 검수는 남았다(3.1절 M4). |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. E1338에서 사용자 결정에 따라 실제 미션 프레임 예외를 기록한 뒤 다음 프레임 진행을 구현·검증했다. 오류·시계 관련 Node13/13·실제 Chromium3/3 통과. 웹 7차의 남은 일은 없다(2026-11-26 정리). [계속 진행 정책](docs/webpj-frame-error-continue-20261126.md). [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | E1455에서 NB 34개 전부를 현재 웹과 대조했다: 웹 적용 수정 7 반영·자료 필요 10·기존 미재현 4·웹 제외 13. E1436/E1445의 실제 간헐천 집계 보완·보충 간헐천 3709초 오류 0과 스커미시/Outpost 실행 완료를 연결했다. 원본 의미 미확정 14건은 공통 D4와 사용자 제보/재현 자료 범위이며 전부 수정으로 세지 않는다. [개별 판정](docs/webpj-known-bugs-scope-audit-20261127.md) |
| 11 | 쉬움/보통/어려움(기본 보통; 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반)의 계산·AiCreate 연결(E554·E555), Options 선택·저장(E1096), 난이도별 실제 스커미시 진행과 옵트인 e2e(E1149), 같은 지도/자금 조건의 실제 예약 배율(E1418)과 쉬움 3·보통 6기 채취 유닛(E1436)으로 자동 검증을 마쳤다. E1487 사용자 요구로 미션 내 선택 행과 직접 Tell 변경을 차단하고 메인 Options만 유지한다. [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md) · [채취 절반](docs/webpj-skirmish-collector-half-20261127.md) |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천(E1101~E1107·E1270, 미션 모드라 비미션 너겟 배치·보충은 도달하지 않음), AI 생산·공격과 사제 사망 패배 안내(E1262), 실제 아군 AI 승리/나가기/재실행(E1439), AI 다리 정체 구간의 현재 C++ 관찰 대조(E1437, 원본 실행/엄격 전상태 동등성으로 세지 않음), Outpost 연결(아래)을 마쳤다. E1484 남은 관문 대조에서 자동 검증 범위의 남은 일은 없다. [규칙](docs/webpj-skirmish-setup-20261120.md) · [UI](docs/webpj-skirmish-ui-20261120.md) · [시작](docs/webpj-skirmish-start-20261120.md) · [관찰](docs/webpj-skirmish-watch-20261123.md) · [승리](docs/webpj-skirmish-ai-victory-20261127.md) |
| 13 | **E1410/E1412 구현·자동 검증 완료.** 공용43일본어파일/번역표·튜토리얼39키·선택/저장/자동감지·실제Google Fonts CDN·영어대체/재시도·Credits카피라이트·일본어줄바꿈과31미션첫안내를 확인했다. 영어/한국어 회귀와Chrome/Edge/Firefox도 통과했다. 모국어 교정·후반 목표/승리·원본일본어픽셀동일성을 입증한 것은 아니다. [범위·근거](docs/webpj-japanese-browser-support-20261127.md). |
| 14 | 웹 자체 JS 사용으로 QuickJS 탑재 불필요. **2026-11-23 사용자 결정:** dotnetpj가 QuickJS로 제공할 모드 API를 webpj 이름 체계의 공통 계약으로 만들고 webpj가 기준 구현이 된다([모딩 API](#modding-api)). |
| 15 | Reset All 완료: E863/E1251의 Options 최하단 확인 창·확정 가드를 유지한다. E1480에서 모드 저장 키를 삭제 목록에 넣었고 E1483에서 세 언어의 모드 삭제 안내·취소 때 보존/확정 때 설정·진행·모드 삭제/다른 앱 키 보존·새로고침을 확인했다. [모드 초기화/검증](docs/webpj-mod-options-20261128.md) · [E1251](docs/webpj-reset-all-tell-20261123.md)  E1487 사용자 요청의 JSON 두 행을 그 아래에 추가했으며 확인/확정 동작은 유지한다. |

- **12차 Outpost 연결 완료(E1434/E1447/E1448).** 실제두목록/지역소유통지·Factory/연구/운반·AI/전투/Client결선을확인하고실제건설/허용·금지메뉴를검증했다. 일반22/데모3에는자동추가하지않고원래PriestTraining3개의명시기술은보존한다. 전용실제전투/모든자연플레이동일성은별개다. [마감/예외/검사범위](docs/webpj-outpost-construction-close-20261127.md).

<a id="modding-api"></a>

- **14차 모딩 API 기준 구현(2026-11-28 사용자 결정).** WB가 공통 명세를 주도한다. **E1470 계약 확정 완료:** [공통 API v0.1.0](docs/netstorm-common-mod-api-v0-20261128.md)·[기계 판독 이름/한도](shared/modding/v0/contract.json)·[선언 스키마](shared/modding/v0/manifest.schema.json)·[동일 패키지 예제](shared/modding/v0/examples/session-observer.nsmod.json). DN MOD-1은 이 계약으로 완료 처리했고 DN 실제 구현은 중지한다([DN 3.1](LEFT_JOBS.dotnetpj.md#dotnet-modding)). 네 항목 중 세 항목과 WB 쪽 적합성 검사는 E1483까지 완료했으며 DN과의 실행 대조는 재개 후 남는다. CPP 모딩과 게임 로직의 JS 재작성은 추가하지 않는다.
  - 완료(E1471~E1483): **공개 계약 층**(순수 API/값·RNG·실제 사본/원래 큐·참가자/유계 결과·격리·시계/관리자·실제 앱/Session/입력/반납·고정 JS 수학, 원래 게임 루프/입력 경계 사용, Raw/주소/객체 비노출, [근거](docs/webpj-mod-session-math-20261128.md)), **이름 정렬**(공개 root10개/명령7개·사건/오류/한도를 공통 JSON과 정확 대조, 내부 Raw 이름 비노출), **모드 로드**(패키지/기본 꺼짐/명시 교체·실패 보존, 격리/단일 번들, Options 파일 선택/켜기·끄기/명시 교체·취소·삭제/오류 표시/Reset All과 실제 미션 메뉴, UI9/9, [사용법](docs/webpj-mod-options-20261128.md)).
  - [ ] **공통 적합성 시험** — 같은 모드 스크립트를 webpj와 dotnetpj에서 돌려 결과를 비교하는 시험 묶음. 완료 기준: webpj 쪽 시험 통과와 DN MOD-4 대조 결과 일치.
    - [x] WB 쪽 — 공통 패키지/정규화 상태·출력/RNG/수학 벡터와 순수 API 검사(E1471, [적합성 입력](shared/modding/v0/conformance/README.md)), 실제 Chromium/Firefox 격리의 같은 5경계 출력(E1478), 실제 Session 자동 사건/장벽과 두 브라우저 고정 JS 수학 285벡터(E1480).
    - [ ] DN 결과 수집 — DN 중지 해제 뒤 MOD-2~4 구현으로 같은 패키지·정규화 입력의 결과를 낸다(DN 인계 담당). 완료 기준: DN 결과가 공통 형식으로 남는다.
    - [ ] 양쪽 대조 — 완료 기준: 정규화 출력·RNG·수학 비트가 WB 결과와 일치하고, 차이가 있으면 계약 문서에 판정을 남긴다.
  - 검토 근거(2026-11-23): 해당 합성 벤치마크에서 QuickJS가 V8 JIT보다 합계 약41배 느렸다(일반 성능 보장 아님). API는 이벤트·명령 단위다. Math double 차이는 E1480에서 고정 JS 프로필로 WB를 교정했으며 DN MOD-3/4의 같은 프로필/비트 대조는 재개 후 남는다. 웹의 VM 메모리/스택 강제 상한 부재도 계약 §6에 명시했다.

### 3.8 원본 관측 연결 이월

완료한 체크: **입력·장치**(E1269, 화면 위치 키·D 키·객체 명령·회전 키·Pause 키 분배기와 `raw-priest-placement-*` Session 설치), **렌더러·음악**(E1263, `renderer-projection-x86.tsv`·`scenemusic-x86.tsv` 소비), **채팅 허브**(E1169~E1175, `chat-callbacks`·`chat-input-recipients` 실제 연결), **점 명령/HTML**(E1192, HTML `$Input=`은 웹 범위 밖). 본문과 근거 링크 원문은 [E1336 보관본 3.8절](LEFT_JOBS.webpj.old.md#archive-webpj-20261125-through-wb04-e1336)에 있다.

[원본 fixture 감사](docs/webpj-fixture-audit-20261115.md)

## 4. 남은 차이와 검증 한계

| 영역 | 후속 확인 |
| --- | --- |
| 지도·지형 | 중립색·가운데 카메라·고정 구름은 개발 미리보기 정책이다. 실제 Session의 지형 연결과 기존 L1/L2는 완료 근거를 따른다. E1465 전수 집계상 절벽 외 현재 지형에는 lit/unlit 후보가 없어 0x10/0x18 추가 선택은 현 자산 비적용이다. 기존 Fort465개의 활성2060영역은 방향0뿐으로 회전2/3의 실제 원본 저장 사례는 자료가 없어 미입증이다(합성 회전 검사는 별도). [현재 연결·한계](docs/webpj-terrain-residual-audit-20261128.md). |
| edgeFarm·보상/SP | 같은 실제 Pool/spots/finder·MayPlace·Retype/HP 연결은E302/E619·E1465에서 확인했다. 보상/SP 재시드·실제지갑·공유장부·알림/위치소리·의식Tell은E1466에서 현재연결과기존검증을재확인했다. 연결된원격5a와보상선택UI는웹제외이며청취와모든자연의식/플레이는사람확인범위다. [연결·한계](docs/webpj-reward-sp-residual-audit-20261128.md). |
| 메뉴·입력 | Kencloud/M1·기준점·Edge Scroll/Auto-Demo는기존완료다. E1461/E1462 고DPI 메뉴와E1467 네브라우저/세화면비의F1본문끌기·포인터붙잡기/반납12경로를확인했다. 미션커서18종/단순건설표시는E1007·기존L3완료다. 원본와이드위치·물리OS커서/터치의육안확인은사용자범위다. [분류/범위](docs/webpj-menu-help-residual-audit-20261128.md). |
| 도움말·Tell | TYPE정보/그림·임시미션치환·Back은후속구현/검증완료다(E1467감사). 현재대본138개에는#·&그림식0·nsisles/nsstartGIF도없어현자료비적용이다. 특수목록/다열은기존웹제외/한열적응을유지한다. mana8~11원본F1·정보패널외관의육안확인은사용자이며현행별칭/CD보충/레이어0·그림자없음정책을유지한다. [개별판정/한계](docs/webpj-menu-help-residual-audit-20261128.md). |
| 소리·환경 | 브라우저/GUI Linux는E1395/E1397/E1402, 영어고DPI125/200%·실제입력/창크기/DPR통합은E1461에서확인했다. 한국어메뉴/첫안내고DPI는E1462에서완료했다. 실제OS모니터이동·청취/위치음향과사용자문장검수는남는다. |
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

**2026-11-29 정리(E1488 뒤).** 정리 직전 전문을 [E1488 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261129-through-wb04-e1488)에 바이트 수·SHA-256과 함께 보관했다(AGENTS.md `1fc63cb1` 정리 규칙).
- **옮긴 것:** 0.1의 bullet 75개(E1385~E1488 단위별 완료 문장 대부분)·지난 전체 회귀 목록·끝난 다음 작업 1~3을 주제별 표와 짧은 다음 작업으로 요약했다. 3.1의 닫힌 `[x]` 네 항목(Cursor·RawCursor 표시, 사제·운반·경로 바인딩, 전투 바인딩, AI 세션 결선) 세부, 3.5의 끝난 결정(루트 `test-results/` 삭제·`webpj/extracted/` 무시), 3.6 표 두 행과 3.7 6·10·11·12차·모딩 완료 세 항목의 경과도 줄였다.
- **바로잡은 것:** 3.7 4차와 0.1 HUD·2절의 최대 1920×1080 → E1488 1280 논리 상한(AGENTS `33aae129b`), 0.1 L2에 16:9 1280×720 새 오라클 추가, 최신 cpppj 1377 → 1384, 운영 번들 E1433 3,510,896 → E1488 5,866,615바이트, fixture manifest 1,331 → 1,337, 0.2 합계 남은 상위 체크 3 → 1, 0.2 10단계 남은 차수에서 5·12차 제외, 3.6 재집계 E1393 '기존 행 변경0' → 2026-11-29 2행, 3.6 ③ 13차 두 행·일본어39키 → E1410·E1412 반영, 1절·3.5의 '스커미시는 계속' → E1439 완료, 0.1 L3의 진행 요약 링크 → E1380 보관본, 7절 'AGENTS 변경 없음' → `33aae129b`.
- **나눈 것:** M4의 '자연 승패…'(자동 허용 `[x]`·1-3/1-4/1-6·그 밖 완주)와 '한국어 전체 확인'(중간 안내 화면·문장 검수), 3.2 M5 남음(M5-a~c), 14차 '공통 적합성 시험'(WB `[x]`·DN 결과·양쪽 대조). 단계 머리 수는 그대로이고 `[x]`로 바꾼 상위 체크는 없다.
- **형식 밖 커밋:** `b9f389cc3`(`.vscode/settings.json`)·머지 2개·`33aae129b`(AGENTS 해상도) 중 WB 영향은 `33aae129b`뿐이며 E1488에서 반영했다.

**2026-11-27 정리1381.** [E1380 전문](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)을 바이트/SHA-256과 함께 보관했다. [정리/검증 근거](docs/handoff-cleanup-1381-20261127.md).

이전 정리(2026-11-26 E1339 이관과 그 앞)의 기록은 보관본의 날짜별 원문 7절에서 확인한다. AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음), `11b16b77`(webpj favicon, [0.6절](#favicon)), `1fc63cb1`(LEFT_JOBS 정리 규칙 — 이번에도 적용), `33aae129`(4차 와이드 최대 1280×720·8차 FSR 목록; 웹은 4차만 해당, E1488)까지 반영했다. `96d81381`은 Noto Sans KR 글꼴 파일과 AGENTS.md가 가리키는 `fonts/webfonts.css`(Google Fonts CDN 예시)를 더한 것이며 한국어 게임 글꼴 규칙은 갈무리11 그대로이다(3.7절 13차).

**E1342 이후 표시 검증 범위(2026-11-26).** 부분 invalidate가 전체 갱신으로 바뀌던 앱 연결을 교정하고 Tutorial5 C 화면5곳·덤프2곳, 4:3 반복 결정론·16:9/16:10 시작 상태를 Chromium5/5로 재검증했다. 기존 Tutorial1~3 완주 L3는 공격·포획·희생·승리가 포함돼 현재 사용자 제한에 따라 재실행하지 않았으며 당시 코드의 결과로 유지한다. 운영 번들은 E1342의3,446,009바이트다.
