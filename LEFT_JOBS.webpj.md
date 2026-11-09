# LEFT_JOBS.webpj — 현행 상태와 남은 작업

webpj(HTML + JavaScript 웹 클론)의 **현재 진행 상황·남은 작업·유효한 결정**만 둔다. cpppj/공통은 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)가 기준이다. 지난 이력과 만료된 계획은 [보관본](LEFT_JOBS.webpj.old.md)에 원문 그대로 있다. 단위별 상세(근거 커밋·fixture·최초 실패와 교정·남은 경계)는 각 `docs/webpj-*.md` 단위 문서와 `webpj/PORT_MAP.tsv` 메모가 기준이다.

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-10, WB04-E213 완료)

| 항목 | 현행 기준 |
| --- | --- |
| 실행·플레이 | 클릭하여 시작→원본 자산 메인 메뉴·캠페인/데모 선택 목록·도움말/옵션·Credits 세 항목(11.01 Patch Credits 포함)과 개발용 지형 미리보기(`?map=`)까지 동작한다. **실제 미션 플레이는 아직 불가**다. WB04에서 옮긴 월드 부품은 아직 앱 수명에 연결하지 않았고(`PORT_MAP` session 0), `MissionBegin`/`StartDemo`는 미션 월드에 연결되지 않았다 |
| 원본과의 동일성 | 메뉴·대화상자 일부는 원본 스크린샷과 대조했다. **cpppj 오라클(L4a) M1 8장면**(Welcome·메인 메뉴 8버튼·Campaign·Demo·Credits·Help/Options 펼침 메뉴·General Help, 영어 4:3, 원본 비교 모드)은 기록한 원인 밖 픽셀 차이 0이다(구름 배경 등 원인 기록은 4절). Fort/지형·그림·SID와 WB04 부품은 원본 10.78 x86 fixture 또는 독립 계산과 대조했다. 실제 미션의 L2/L3·전체 화면/음향·자연 완주는 미검증이다 |
| 마일스톤 | **M1 완료**(시작→메인 메뉴, cpppj 화면 대조 완료) · **M2 진행 중**(순서3 월드 부품 단계, 0.2절) · M3~M5 미착수 |
| 이식 진행도 | 대상 896단위(제외 88 별도) 중 **port 425(47.4%)·partial 95(10.6%)·todo 376(42.0%)**. 기준선 소스 줄 수로는 129,840줄 중 port 47,338(36.5%)·partial 26,035(20.1%)·todo 56,467(43.5%). 영역별 남은 양은 0.3절 |
| 마지막 구현 커밋 | **WB04-E213** (이 커밋, `20261110-06 WB`) 공통 삭제 장부 `squid-destroy-lifecycle.js`(preDestroy 004b0950 의 선택 복구·배치 기억·Graph 분할/Free·소유자 작업장 제거·보상·현재/전역 수·소리/손실 좌표·공급원 제거·현재 AI 통지·전체 작업장 제거와 postDestroy 004b0840 의 주변 표면 Add·패치 비용 인코딩 차감·DWORD 감김, 사전 검사의 쓰기 전 거부)를 원본 10.78 destroylifecycle 576행·destroygraph 576행·island-graph-frame 행과 cpppj `DestroyLifecycle_*`·`DestroyGraph_*`·`GameAux_Deletion*` 계약으로 대조하고, E212 가 미룬 보호막 조합 시험(`ForcefieldLifecycle_ComposesSpaceProcessesAndSelfDeletion`)을 실제 Factory/Owner/Pop/Unpop/Display/Frame/ProcessHost/Kernel 조립으로 닫음([문서](docs/webpj-squid-destroy-lifecycle-20261110.md)). 기준선 뒤 cpppj 3커밋의 실제 PATCH(Native) 분기는 3.6절 ③ 표로 올림. 그 앞은 **WB04-E212** (`9d39ef38 / 20261110-03 WB`). 완료 단위 전체는 2.2절 |
| 다음 작업 | **WB04-E214** — 3.1절 "총·피해·삭제 수명" 묶음에서 기준선 뒤 변경이 없는 RawSquidDestroyDispatch 를 먼저 권한다(RawFrameChoose·RawGeyserMined·RawRitualChoice 도 변경 없음). RawDamageablePreDestroy·RawDamageablePostDestroy·RawDamageableRelease·RawGunImpact·RawDiscRegular 와 "비행·이동 유닛" 묶음의 선행 RawFlyerProcess 는 기준선 뒤 변경이 있어 옮길 때 E213 문서 3절과 같은 Native 분기 판정(3.6절)을 함께 한다. "그림·캡처" 묶음의 남은 RawPlainAssetDraw·RawPlainWorkshopCapture·RawPlainUnitCapture·RawColorMapCache(adapt)는 순서5 Session 연결과 함께 판단한다. 순서3을 마감하기 전에 3.6절의 기준선 뒤 변경 판정 대기(웹 이식분 54단위)를 처리한다 |
| 마지막 전체 검사 | WB04-E213의 Node v24.21.0, **2056/2056 통과·실패/건너뜀0**(신규 squid-destroy-lifecycle 17·forcefield-lifecycle-integration 1; WB04-E212 2038), 약136초 |
| 마지막 e2e | WB02-H의 Chromium 초기 페이지 스모크 **1/1 통과**. 별도 임시 Playwright 스크립트로 시작→Welcome 닫기→Help 클릭(정지 포인터 아래 펼침 메뉴 첫 행 강조)→General Help 창→Credits 세 행을 실제 클릭 확인(오류 0). 미션/월드 검사가 아니다 |

### 0.2 실행 순서 진행판

순서별 할 일과 완료 기준의 상세는 3.2절이다. "부품 일부"는 해당 순서가 쓸 `o/` 부품 일부를 순서3에서 미리 옮겼다는 뜻이며, 그 순서의 연결·검증은 아직 시작하지 않았다.

| 순서 | 단위 | 상태 | 마일스톤 | 현재 진행 |
| --- | --- | --- | --- | --- |
| 1 | WB02-G 이식 대응표·기준선 재반영 | **완료** (2026-11-07) | 선행 | 984단위 분류·미분류 0, 3.6절 9곳 판정 |
| 2 | WB02-H 무브라우저 실행·cpppj 오라클 기반 | **완료** (2026-11-07) | M1 대조 | M1 8장면 기록 밖 차이 0 |
| 3 | WB04 월드 핵심 부품 | **진행 중** | M2 선행 | A~D5c·E1~E213 완료(2절). `o/`는 단위 72.5%·줄 65.2%가 port. 남은 후보 3.1절 |
| 4 | WB05-A/B 월드 조립과 미션 시작 | 미착수 | M2 | 세션·로그인·전송(0.3절 A 영역)과 Fort 적재·지형/섬(B 영역) 대부분 todo |
| 5 | WB03-M2/WB05-C 월드 표시·카메라·HUD | 미착수 | M2 | 렌더러·카메라·HUD(J 영역) todo. 메뉴/대화상자 UI(L 영역)는 M1 범위만 partial |
| 6 | WB06~08 입력·경제·건설 | 부품 일부 | M3 | 건설/배치·운반 부품(E90~E160)은 port. 커서·객체 메뉴(K)·생산 UI는 todo |
| 7 | WB09~10·WB12 전투·의식·AI | 부품 일부 | M4 | AI 몸체(E163~E182)·총/투사체·사제/신전 부품은 port. Gun/Flyer 프로세스·의식 본체는 todo |
| 8 | WB11·WB13·WB15 미션 수명·안내·소리 | 미착수 | M4 | 튜토리얼·미션 결과·소리/음악·채팅(G·H·I 영역) 대부분 todo |
| 9 | WB16 표시 품질·배포 | 미착수 | M5 | 60/120 표시·브라우저/OS 확인·정적 배포 |

### 0.3 남은 이식량 (`webpj/PORT_MAP.tsv`, 2026-11-10 WB04-E213 기준)

줄 수는 기준선 `1f00a25a`의 단위 파일(`.cpp/.h/.inc`) 줄 합계다(주석 포함). partial 줄 수는 일부만 옮긴 파일의 전체 줄이므로 남은 양의 상한이다. 단위·줄 수는 대략적인 척도일 뿐이며, 순서4~5의 연결·통합(실제 세션에서 부품을 같은 월드로 묶고 cpppj 오라클과 비교)은 이 수치에 드러나지 않는 큰 작업이다.

| 범위 | 대상 | port | partial | todo |
| --- | --- | --- | --- | --- |
| 전체 | 896단위 / 129,840줄 | 425 (47.4%) / 47,338줄 (36.5%) | 95 (10.6%) / 26,035줄 (20.1%) | 376 (42.0%) / 56,467줄 (43.5%) |
| `o/` 월드·규칙 | 517단위 / 67,779줄 | 375 (72.5%) / 44,164줄 (65.2%) | 33 (6.4%) / 8,752줄 (12.9%) | 109 (21.1%) / 14,863줄 (21.9%) |
| `client/` 세션·UI·표시 | 378단위 / 61,622줄 | 50 (13.2%) / 3,174줄 (5.2%) | 62 (16.4%) / 17,283줄 (28.0%) | 266 (70.4%) / 41,165줄 (66.8%) |
| `zacket/` 내부 미션 전송 | 1단위 / 439줄 | 0 | 0 | 1 / 439줄 |

남은 471단위(todo 376·partial 95)를 단위 이름 규칙으로 대략 나눈 영역이다. 영역 경계는 근사이며 실제 이식 순서는 cpppj 의존 관계로 정한다. `*`는 partial이다.

| 영역 | 주 순서 | todo 단위 / 줄 | partial 단위 / 줄 | 큰 단위(줄) |
| --- | --- | --- | --- | --- |
| A 세션·로그인·패킷·전송·클라이언트 전역 | 4 | 71 / 13,580 | 2 / 1,907 | RawCampaignSession 5,715 · ClientMain* 1,797 · CampaignTemplateStorage 695 · LocalMissionTransport 439 |
| B 월드·Fort 적재·지형/섬·객체 공통 수명 | 3·4 | 37 / 6,638 | 13 / 4,052 | GameWorld 1,153 · RawSurfaceWorld 804 · SquidPostPop* 675 · RawGraph* 626 |
| C 시각 효과·입자 | 3·5 | 5 / 678 | 0 | RawCampaignParticles 513 · ParticlePaletteMemory 65 · ConstructionVisualRenderer 57 · ThunderProcess 32 |
| D 전투·유닛 이동·경로 | 3·7 | 31 / 4,309 | 7 / 1,470 | RawGunProcess 421 · RawGunTarget* 399 · RawMissile* 392 · RawPathProcess 360 |
| E 경제·생산·건설·자원 | 3·6 | 24 / 4,139 | 1 / 142 | RawProductionControls 393 · RawDaisRegular 372 · RawAssetRegular 352 · RawCampaignFactoryDisplay 329 |
| F 사제·신전·의식·지식·AI | 3·7 | 13 / 2,123 | 6 / 482 | RawPriestWorldBindings 488 · RawTempleRitual 264 · RawAltarSacrifice 261 · RawPriestMenu 249 |
| G 미션 진행·튜토리얼·안내·대본 | 8 | 17 / 1,936 | 4 / 323 | DialogScript 296 · RawMissionOutcome 248 · RawTutorialLater 191 · Mission 150 |
| H 소리·음악·동영상 | 8 | 10 / 1,666 | 4 / 2,041 | Sound* 824 · ClientAudio* 467 · SoundMusic* 455 · SoundDevice 372 |
| I 채팅·메시지 | 8 | 19 / 2,109 | 0 | ChatWindow 272 · ClientChatRuntime 191 · WarningMessages 181 · RawChatManagerPrefix 150 |
| J 렌더러·카메라·HUD·미니맵 | 5 | 44 / 5,142 | 8 / 2,232 | Screen* 816 · Renderer* 492 · SpriteDraw* 468 · PreviewDraw 363 |
| K 게임 입력·커서·선택·객체 메뉴 | 6 | 40 / 5,230 | 2 / 149 | RawCursorMana 289 · RawCampaignCursor 283 · RawCursorRange 283 · CampaignObjectMenuClient 240 |
| L 메뉴·대화상자·Gump·글자 UI | 5·6 | 39 / 6,948 | 37 / 10,315 | UberGump* 2,207 · RawMenuControls 1,341 · RawButtonControls 797 · StyleText* 519 |
| M 자료·파일·설정·언어(웹 적응 포함) | 4 | 22 / 1,637 | 11 / 2,922 | BaseFile* 586 · Config* 586 · RiftTypeTable* 348 · ConfigInterface* 343 |
| N 진단·추적 | 각 단계 | 4 / 332 | 0 | GifScreenshot 114 · ClientDiagnostics 99 · RawTraceLog 71 · RawCheckingDiagnostics 48 |

분류(class)별로는 port 852(이식 대상)·adapt 44(웹 대체가 필요한 플랫폼 경계, port 2·partial 9·todo 33)·제외 88(exclude-mp 6·exclude-edit 55·exclude-host 27)이다. 이와 별도로 기준선 뒤 cpppj 변경 판정 대기가 있다(3.6절).

### 0.4 기준 값

| 항목 | 현행 기준 |
| --- | --- |
| cpppj 기준선 | **`1f00a25a`(제품985·표시 11.01).** 이식 원본은 언제나 기준선 판(`git show 1f00a25a:경로`)이고, 기준선 뒤 cpppj 변경은 3.6절 절차로만 반영한다. 같은 작업 트리에서 CP 세션이 cpppj를 계속 커밋하므로(LAN 3차·`Actual*` 실제 연결 경로·`shared*` 공유 전역 추가 등) cpppj 상태는 커밋과 소스로 판단한다. **오라클 실행 파일**은 기준선을 따로 빌드한 `extracted/webpj-oracle/1f00a25a/Netstorm.exe`(SHA-256 `03fa4df8…cbc1e`, Git 제외·HJOW-Athlon 전용)다 |
| 이식 범위 | **`webpj/PORT_MAP.tsv` 984단위**(기준선 `o/`555·`client/`428·`zacket/`1, 약 13만 7천 줄). 분류: **port 852·adapt 44·exclude-mp 6·exclude-edit 55·exclude-host 27, 미분류 0**. 상태: **port 425·partial 95·todo 376·제외(-) 88**, session/oracle 0. 웹 모듈 471개 중 466개 대응·5개 웹 전용. 웹 소스는 `www/` JS 약 6만 4천 줄(`www/data` 제외), 검사 파일 362개, 도구(`webpj/tools`) 오라클 관련 4개다 |
| 이식 추적 | `webpj/tests/fixtures/manifest.json` 478항목(original-x86 450·independent 7·screenshot 13·source 5·original-binary 1·**cpppj-oracle 2**; 최근 WB04-E213에서 destroylifecycle·destroygraph·island-graph-frame 3개 추가). original-x86 450개는 모두 `cpppj/tests/fixtures`를 직접 읽는다. cpppj-oracle은 `tests/fixtures/cpppj/m1/oracle.json`(PNG 8장·보고서 8개의 해시를 담은 메타)과 `m1-scenarios.json`이다. `fixture-manifest.mjs`는 대응표 base(판정 커밋) 뒤 변경만 보고하고, `port-map.mjs`는 기준선 뒤 cpppj 변경(3.6절)과 새 파일을 보고한다 |
| 마지막 정밀 디컴파일 | 공통 인계 기준 **HJOW-Athlon / 2026-11-03**(R 전용은 2026-11-07, 같은 호스트). 웹은 cpppj 소스·fixture를 근거로 하므로 디컴파일 산출물을 직접 읽지 않는다 |

### 0.5 매 단위 착수/종료

1. AGENTS.md·이 문서·공통/dotnetpj 인계를 읽고 `git status`·`git fetch`를 확인한다. **같은 작업 트리에서 다른 세션이 cpppj·dotnetpj·공통 문서를 동시에 커밋할 수 있다(2026-11-06 실제 관측).** 공통 인계와 cpppj 파일은 고치지 않고, cpppj 상태는 커밋 기록과 소스로 판단한다.
2. `node webpj/tools/port-map.mjs`로 대응표 검사와 기준선(행 base) 뒤 cpppj 변경을 본다. 이번 단위가 옮길 cpppj 파일을 3.1절과 `PORT_MAP.tsv`에서 고르고, **기준선 판**의 소스(`git show 1f00a25a:경로`)·그 파일을 대상으로 하는 cpppj 검사·fixture·단위 문서를 함께 읽는다. 기준선 뒤 그 파일이 바뀌었는지는 `git diff --numstat 1f00a25a HEAD -- <파일>`로 확인하고, 바뀌었으면 3.6절로 판정해 판정 커밋을 base와 단위 문서에 적는다. 디컴파일이 꼭 필요하면 마지막 수행 PC와 현재 PC를 먼저 비교하고 AGENTS.md의 재디컴파일 규칙을 따른다.
3. 원본/클론 게임(cpppj 오라클 수집 포함)·analyzeManager·별도 창 실행은 AGENTS.md의 확인/예외 규칙을 적용한다. 단위 검사나 정적 확인을 실제 실행으로 기록하지 않는다.
4. 3.0절 방식으로 구현한다. 필요한 공유 장치가 없으면 먼저 이식하고, 빈 성공 함수·별도 근사 월드로 우회하지 않는다. 멀티플레이 분기는 1절의 경계 규칙을 따른다.
5. 근거 커밋·연결 fixture/오라클(SHA/행 수)·검사와 최초 실패/교정·실제 연결/미연결·남은 경계·다음 시작점은 **단위 문서**에 남긴다. `PORT_MAP.tsv`의 web·base·status·메모와 manifest(`node webpj/tools/fixture-manifest.mjs --write --only <아이디,…>`)를 갱신하고 `node webpj/tools/port-map.mjs` 오류 0을 확인한다. 이 문서는 **0.1절(머리글·진행도·마지막 구현 커밋·다음 작업·마지막 전체 검사)·0.2절 순서3의 완료 범위·0.3절 수치·0.4절 개수·2.2절 해당 영역 행에 단위 링크 추가·3.1절에서 완료 항목 삭제**만 고친다. 단위별 긴 설명과 기록 줄은 이 문서에 쌓지 않는다(2026-11-09 정리 결정, 7절). 이번 단위 파일만 이름으로 stage하고, 커밋 직전에 `git log`로 번호가 겹치지 않는지 확인한 뒤 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. push하지 않는다.

## 1. 범위와 유지되는 결정

- **계획 기준 (2026-11-06 재계획, 사용자 요청):** cpppj는 싱글플레이 범위(비멀티 UI·Edit·미션/내부 수명, 4·5·7차)의 구현·대조를 마치고 3차 LAN 멀티플레이 사전 구현으로 넘어갔다. 웹은 멀티플레이와 Edit를 제외하므로 웹이 옮길 cpppj 코드는 사실상 고정됐다. 그래서 ① cpppj 기준선 고정(`1f00a25a`, 이후 변경은 3.6절 필터) ② cpppj 파일(클래스) 단위 이식과 해당 검사 이식(`PORT_MAP.tsv`로 범위·진행 관리, 3.0절) ③ 원본 x86 fixture 대조(L1)에 더한 cpppj 오라클 비교(L2~L4, 3.3절) ④ 내부 미션 전송·패킷·로그인 경로는 이식하고 LAN TCP·멀티플레이 UI·일반 전투·Outpost 등만 제외(아래)로 진행한다.
- **사용자 결정 (2026-11-05, 유효):** Credits AI 문구는 현행 유지 후 나중에 수정한다(4절). 한국어 행 이름 `NetStorm 11.01 패치 크레딧`은 확정이다. 원본 자료의 바깥 링크는 새 탭/창으로 연다(WB03-AD 구현). 개발 서버의 다른 PC 접속은 제한하지 않는다. 같은 날의 "착수 시점 cpppj + 사후 재반영" 결정은 위 기준선 고정으로 대체됐다.
- **목표:** 원본10.78 싱글플레이를 HTML+JavaScript로 복원한다. 대상은 Early Missions6·Priest Training9·캠페인16의 **일반31경로와 데모3경로**다. 같은 자산을 쓰는 일반/데모도 별도로 취급한다([공통 인계1.2절](LEFT_JOBS.md)).
- **웹 범위와 멀티플레이 경계 (2026-11-06 정리):**
  - **이식 대상(싱글플레이에 필요):** 단독 미션·데모가 쓰는 Client/Session·World·State와, **내부 미션 전송(zlocal 양끝)·패킷 등록/길이/분배·미션 로그인·서버/클라이언트 미션 패킷·Zone 초기화**를 옮긴다. 원본 싱글플레이도 같은 프로세스 안에서 이 경로로 진행하므로, 이것을 빼면 미션이 돌지 않는다. 로그인 Ready·Player 번호·초기 SP 지정처럼 이름은 멀티플레이 같아도 단독 경로에서 실행되는 코드는 이식 대상이다.
  - **웹 제외:** LAN TCP 전송(`LocalTcpTransport`, 복합 전송의 TCP 원격끝, `CampaignTransportServices`의 TCP backend), Multiplayer 메뉴·LAN UI·참가자 목록·Commander, 일반 전투(State6·`saveBattle`/`!state.dat` 복구·Reconnect·서버 이전), 메타맵·Challenge·중앙섬·랭크, Outpost, 공식 서버·클라이언트 유효성 검사. 또 Edit·커스텀 맵 작성/가져오기/저장·TEST01/TEST02 플레이·User Made Campaigns(`offical6`/`%{userSpec}` 목록), Quit/Exit, 게임 옵션의 해상도/전체화면, 설치/CD 탐지·레지스트리, Win32/DirectDraw/GDI/WINMM·Direct3D 11/FSR·MCP. 메뉴에도 노출하지 않으며 Edit 전용 CHEAT·`userCheat` 우회도 제공하지 않는다. 공통 Fort/지형 코드가 Edit에도 쓰인다는 이유로 캠페인용 이식을 막지는 않는다.
  - **경계 처리:** cpppj 함수 안의 멀티플레이 분기는 원본의 단독 판정(`inSingle`·demo·server 조건)을 그대로 옮긴다. 단독에서 도달하지 않는 분기는 조용한 성공이나 no-op로 바꾸지 않고 명시적인 웹 제외 오류로 둔다(`MainLoop`의 웹 제외 단계 사유와 같은 방식). 직접 단독 관문이 없다고 해서 도달 불가로 단정하지 않으며, 근거를 단위 문서에 남긴다.
  - cpppj가 3차 작업에서 공유 파일(로그인 부모·Session·Player/SP setter·패킷 분배 등)을 바꾸면 단독 경로에 영향이 있는지 3.6절로 확인한다.
- **cpppj 기준 (2026-11-06 재정의):** 목표는 원본 10.78 싱글플레이 복원이며, 그 근거로 원본 기계어와 대조된 cpppj 기준선(`1f00a25a`)을 옮긴다. cpppj의 파일·클래스·함수 경계와 이름, 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 보존한다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 `DataView`/TypedArray로 같은 배치를 쓴다. cpppj의 미복원·부분 연결·명시 미연결·실패 경계는 임의로 채우지 않고 같은 지점의 명시 오류·진단으로 옮긴다. dotnetpj의 근사를 정답으로 복사하지 않는다. 기준선 이후의 cpppj 변경은 3.6절로만 반영하고 미커밋 변경은 쓰지 않는다.
- **바깥 링크 (2026-11-05 사용자 결정, 2026-11-07 WB03-AD 구현):** Tell/도움말 링크의 바깥 주소는 원본의 셸 실행 대신 **브라우저 새 탭/창으로 연다**. 치환이 끝난 `http:`/`https:`/`mailto:`만 열고 `file:`·미치환 자리표시 등은 이유와 함께 콘솔로 알린다. 조건과 판정은 [외부 접속 문서7절](docs/webpj-external-access-20261105.md)을 따른다. 개발 서버(`server.js`)는 다른 PC 접속을 제한하지 않는다(모든 인터페이스 수신 유지).
- **구조:** `index.html` 수정은 최소화하고 `netstorm.js`가 앱 수명을 맡는다. `.css` 파일 없이 JS로 스타일을 지정한다. Canvas의8비트 인덱스/팔레트 합성과 최종 표시를 분리한다. 코어는 DOM/Canvas/Web Audio에 의존하지 않는다. 기존 CommonJS 서버와 `www/package.json`의 ES 모듈 경계, `www/{assets,core,render,ui,audio,platform}`를 유지한다. C++/C# 서버나 WebAssembly를 필수 런타임으로 추가하지 않는다.
- **시작/수명:** 사용자 시작 입력 안에서 오디오를 활성화한다. 미션 재시작·메뉴 복귀·`destroy/init`의 타이머/선택/음악/이벤트 소유권을 구분한다. 숨겨진 탭은 진행을 정지하고 복귀 때 시계 기준을 다시 잡아 부재 시간을 따라잡지 않는다(웹 적응).
- **화면/시간:** 영어4:3부터 비교하며 게임 영역의4:3·16:9·16:10, 입력 역변환·픽셀 비율을 다룬다. 현재768높이/1024·1228·1366너비는 임시 화면 정책이다. `requestAnimationFrame`은 표시 기회이고 게임 틱이 아니다. 요소별 원본 간격을 먼저 맞추고60/120프레임은 후순위로 둔다. 가장자리 스크롤은 미션이 입력을 받으며 포인터가 게임 영역 안에 있을 때만 동작하고 이탈/포커스 상실 시 멈춘다.
- **자산/저장:** 기본 소스는 `assets/game-data`·`fonts`·`webpj/translations`이며 실행 자료는 `webpj/www/data`에 준비한다. 느슨한 파일→TAFF 조회·타입/프레임/팔레트 의미·manifest 출처/해시를 보존한다. 웹 실행에서 보호 원본이나 로컬 설치를 요구하지 않는다. 설정/진행은 버전이 있는 브라우저 저장소에 두고 저장 불가/손상 때도 계속할 수 있게 한다. 큰 저장소 도입은 실제 필요가 생길 때 판단한다.
- **언어:** 영어 원본 `.chfnt`, 한국어는 사용자 지시대로 `fonts/D2Coding.woff2`·`D2CodingBold.woff2`를 복사해 사용한다. TTC 추출로 되돌리지 않는다. 한국어 파일/섹션/주제가 없으면 영어로 대체한다. 번역 문구 검수는 **개발자가 발견하는 대로 요청**한다(2026-11-01). 메뉴/도움말 번역 완료를 미션 한국어 완료로 세지 않는다. 명칭은 AGENTS.md와 [공유 명칭](docs/korean-unit-names.md)을 따른다. 미션 브리핑·안내의 한국어는 cpppj 한국어 후속 확정본과 같은 원문을 쓰는 것을 기본값으로 한다(3.5절 2번).
- **사용자 확인 경계:** [공통 인계0.3·1.1·3절](LEFT_JOBS.md)에 맞춰 **모든 자연 완주·전체 동일성·실제 사제 포획/희생·특정 적 공격·승리 재현/자동 공략은 사용자 확인 또는 별도 요청 대상**이다. 구현·정적 대조·허용된 부품 검사는 계속하되 이 항목만 남으면 다음 단계로 간다. 공중3종(훨리기그·맨오워·더스트 데빌)은 정적/코드 이식만 하며 새 전용 생산/생성/우클릭/행동 관측·검사를 추가하지 않는다.
- **보호/분담:** AGENTS.md·보호 원본/CD를 수정하지 않는다. cpppj·dotnetpj·공통 인계와 다른 세션 변경은 이 작업에서 수정/커밋하지 않는다. README 등 공통 파일은 기존 사용자 결정대로 일괄 갱신 때 반영한다. 새 문서/코드는 UTF-8, 새 상수·함수·자료형·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다. **공통 인계 LEFT_JOBS.md와 cpppj 파일은 다른 세션이 갱신 중이므로 WB 단위에서 수정하지 않는다(2026-11-06 사용자 주의).** 같은 작업 트리의 다른 세션 커밋과 섞이지 않도록 stage할 파일은 이름으로 지정한다.

## 2. 완료 범위 색인

여기 있는 항목은 해당 부품의 완료이며 실제 캠페인/전체 동일성 완료가 아니다. WB04 부품은 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 단위별 상세·최초 실패·남은 경계는 각 단위 문서와 `PORT_MAP.tsv` 메모의 "남음:"을 따른다.

### 2.1 기반·메뉴·자료·월드 기초 (WB00~WB04-D5c)

| 단위 | 완료 범위와 현재 경계 | 근거 |
| --- | --- | --- |
| WB00·WB01 | 계획·ES 모듈 진입·클릭 대기/실패 재시도·Canvas/기본 입력·앱 수명 | [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a) |
| WB02 메뉴 기반·WB02-D | 자산 준비/manifest·TAFF/Config/GameFiles/GameData/Tell 조회·영어 글꼴. 미션 조회는 WB05에서 연결 | [데이터층](docs/webpj-data-layer-20261028.md) |
| WB02-E | 웹 런타임 외부 접속 목록: 요청은 `./data/` 같은 출처 자산뿐, UserNotUpdated·Technical Help·서버 진단·`$Zsend`/`$Input` 미연결, 소스 정적 감사 검사 | [외부 접속](docs/webpj-external-access-20261105.md) |
| WB02-F | 이식 추적 목록 `manifest.json` 형식 v1·계산/검사 도구 `tools/fixture-manifest.mjs`. 새 대조 자료는 같은 단위에서 등록 | [이식 추적](docs/webpj-fixture-trace-20261105.md) |
| WB02-G | 이식 대응표 `PORT_MAP.tsv`(기준선 984단위, 미분류 0)·`tools/port-map.mjs`(생성/검사/기준선 뒤 변경 보고/`--why`)·분류 규칙 31개·3.6절 9곳 판정 | [이식 대응표](docs/webpj-port-map-20261106.md) |
| WB02-H | 무브라우저 실행 `tools/headless.mjs`·cpppj 오라클 수집 `tools/cpp-oracle.mjs`·비교 `tools/screen-compare.mjs`. M1 8장면 대조(기록 밖 차이 0, 웹 결함 2건 수정) | [오라클 비교](docs/webpj-cpp-oracle-20261107.md) |
| WB03-M1·D~L | 원본 메인 메뉴, Tell/Campaign/Demo/Credits 목록, Help/Options·설정, F1/링크/스크롤/커서, 시작 환영/팁, `Done*` 쓰기/저장. 미션 명령·Auto-Demo·현재 미션 문맥은 미연결 | [메인 메뉴](docs/webpj-main-menu-20261023.md)·[대화상자](docs/webpj-tell-dialog-20261028.md)·[목록](docs/webpj-campaign-lists-20261029.md)·[진행 저장](docs/webpj-progress-storage-20261030.md)·[시작 창](docs/webpj-startup-dialogs-20261030.md) |
| WB03-I / 제외 결정 | Edit 버튼과 User Made Campaigns 행 제거. 캠페인 최상위 6행이며 두 기능은 재구현하지 않음 | [캠페인 목록7절](docs/webpj-campaign-lists-20261029.md) |
| WB03-N~S | D2Coding·xlat·메뉴/Tell/캠페인 목록/도움말 한국어와 영어 대체. `serverHelp` 미완성 틀·그림뿐인 `map`과 미션 본문은 구분 | [글꼴/메뉴](docs/webpj-korean-20261031.md)·[Tell](docs/webpj-korean-tell-20261031.md)·[도움말 완료](docs/webpj-korean-help-types-20261101.md) |
| WB03-T | 자산 타입 116개의 프레임/그림 해석. `<info>`·`#`/`&`와 에너지 기호 관측은 남음(4절) | [도움말 그림](docs/webpj-help-pictures-20261101.md) |
| WB03-U~Y | `.fort`/188타입 플래그·영역 배치·본섬 마스크·지면/절벽·작은 받침·소유자 색표 계산. 465맵 독립 대조는 실제 미션 적재 증거가 아님 | [Fort](docs/webpj-fort-reader-20261101.md)·[영역](docs/webpj-territory-layout-20261101.md)·[마스크](docs/webpj-terrain-mask-20261101.md)·[지면](docs/webpj-terrain-tiles-20261102.md)·[절벽/받침/색](docs/webpj-terrain-extras-20261102.md) |
| WB03-Z·AA | CRT 정렬·64바이트 카메라·공통 월드 그림/그림자 출력과 개발용 중립 지형 화면. 실제 Draw/소유자·배경/카메라 정책은 미연결 | [출력](docs/webpj-world-render-20261102.md)·[지형 장면](docs/webpj-terrain-scene-20261102.md) |
| WB03-AB | 값형 Fort 객체/내용물·Terr→Chaff 순회 로더. 실제 월드에는 미연결(WB05) | [객체 로더](docs/webpj-fort-object-load-20261102.md) |
| WB03-AC | 제품 표기 `v11.01`·`Patch1101` 등록 객체·메인 Credits 세 번째 행과 Patch Credits 창(영어/한국어). 원본 대조는 `GameData.build({product:false})`로 10.78·두 행 유지. AI 문구/한국어 문구는 개발자 검수 대상 | [제품11.01](docs/webpj-product-1101-20261105.md) |
| WB03-AD | 바깥 링크 새 탭/창 열기: 판정 `ui/external-link.js`·열기 `platform/external-opener.js`(숨은 앵커 click, `noopener noreferrer`). 실제 브라우저의 새 탭 열림·팝업 차단은 미확인(3.5절 4번) | [외부 접속7절](docs/webpj-external-access-20261105.md) |
| WB04-A | 실제 50바이트 SID 풀·할당/반납/재초기화·20개 삭제 기록. 원본 10.78의 1584전이 대조 | [SID 풀](docs/webpj-sid-pool-20261104.md) |
| WB04-B | `SquidHash`(4단계 WORD 머리)와 `RawSquidFinder`(Begin/Next)를 같은 SID 풀에 연결 | [공간 해시/탐색기](docs/webpj-squid-finder-20261105.md) |
| WB04-C1~C3 | 장기 난수 전역·게임 시계·Kernel / 게임 보조 전역 `GameAuxState`(보호 PE 초기값 51칸 대조) / 원본 40칸 입력 큐와 메인 루프 순서 실행기 `MainLoop` | [세션 장기 상태1](docs/webpj-session-runtime-20261105.md)·[세션 장기 상태2](docs/webpj-game-aux-state-20261105.md)·[큐/메인 루프](docs/webpj-main-loop-20261105.md) |
| WB04-D1~D5c | 공통 공간 해제 부모 / `SquidUnpop` 처리기·삭제기 / 공통 Form 층 / `SquidFactory`(생성자 188칸) / Depend·Gump·Content Form 호스트 / Form 분배·Content 수명·보상/SP 저장소 / Process 호스트·Regular 공통 | [공통 공간 해제](docs/webpj-squid-unpop-20261105.md)·[Unpop·삭제기](docs/webpj-squid-destroy-20261105.md)·[공통 Form 층](docs/webpj-squid-forms-20261106.md)·[SquidFactory](docs/webpj-squid-factory-20261106.md)·[Form 호스트](docs/webpj-form-hosts-20261106.md)·[분배기·Content 수명](docs/webpj-content-lifetime-20261107.md)·[Process 호스트](docs/webpj-process-host-20261108.md) |

### 2.2 WB04-E 월드 핵심 부품 (E1~E213, 영역별)

새 단위를 마치면 해당 영역 행 끝에 `· [E번호](단위 문서) 제목`을 붙이고 단위 수를 고친다. 맞는 영역이 없으면 행을 새로 만든다.

| 영역 | 단위 수 | 단위(단위 문서 링크) |
| --- | --- | --- |
| Player·소유자·State | 13 | [E2](docs/webpj-squid-owner-20261108.md) 소유자/레벨/섬 번호/작은 setter · [E3](docs/webpj-player-core-20261108.md) Player Core 묶음 · [E4](docs/webpj-player-identity-20261108.md) Player 식별자 · [E5](docs/webpj-player-participation-20261108.md) Player 참여 · [E6](docs/webpj-chat-channels-20261108.md) Chat 채널 기록 · [E7](docs/webpj-player-initialization-20261108.md) Player 전체 초기화 · [E8](docs/webpj-player-notices-20261108.md) Player 정리·알림 · [E9](docs/webpj-player-content-notice-20261108.md) Player 내용물 통지 · [E11](docs/webpj-state-resets-20261108.md) State 정리 소형 몸체 · [E13](docs/webpj-technology-color-20261108.md) 기술 획득·테마 기술·소유자 색 · [E55](docs/webpj-player-world-regular-20261108.md) Player 월드 연결·Regular 처리기·Template State · [E56](docs/webpj-player-technology-contained-20261108.md) 내용물 검색기·Carrier 삭제 준비·Player 기술 목록 · [E57](docs/webpj-player-geyser-anchor-20261108.md) Player 간헐천 범위·배치 기준점 |
| 객체 공통 수명·표시·명령 | 12 | [E1](docs/webpj-squid-pop-20261108.md) 공통 Pop · [E14](docs/webpj-small-bodies-20261108.md) 소형 몸체 네 개 · [E15](docs/webpj-small-parents-20261108.md) 소형 부모 다섯 개 · [E24](docs/webpj-squid-postpop-parent-20261108.md) 공통 postPop 부모 · [E26](docs/webpj-region-sid-lists-20261108.md) 표면 소유자·SID 건전성·정적 SID 목록·TYPE 요구 에너지 · [E41](docs/webpj-squid-postpop-20261108.md) 공통 후처리 처리기 SquidPostPop · [E58](docs/webpj-squid-display-name-selection-20261108.md) 객체 표시 부모·이름·방어·선택 · [E59](docs/webpj-squid-display-frame-20261108.md) 객체 표시·프레임 지정·프레임 연결 · [E60](docs/webpj-post-pop-buried-carrier-fence-20261108.md) buried·Carrier·썬 바리케이드 후처리 · [E61](docs/webpj-rune-factory-post-pop-20261108.md) Rune·Factory 후처리·자산 후처리 공유 상태 · [E123](docs/webpj-object-command-20261108.md) 공통 객체 명령·Template 수명 · [E213](docs/webpj-squid-destroy-lifecycle-20261110.md) 공통 삭제 장부(SquidDestroyLifecycle)와 보호막 조합 시험 |
| 기반 유틸리티·입력 분배 | 13 | [E18](docs/webpj-static-factory-20261108.md) 임시 문자열 고리·Factory 입력 · [E19](docs/webpj-data-manager-names-20261108.md) 토큰·SNART·Template 이름표·DataManager 초기화 · [E20](docs/webpj-key-inputs-20261108.md) 일반 입력 분배 세 개 · [E21](docs/webpj-script-command-inputs-20261108.md) 대본 일반 입력 분배 · [E22](docs/webpj-selection-keys-20261108.md) 선택 관련 키 입력 · [E23](docs/webpj-client-input-commands-20261108.md) 선택 키·AI 일반 명령·편집 객체 취득 · [E25](docs/webpj-view-utility-inputs-20261108.md) 표시·유틸리티·글꼴/음질·시나리오 표시 입력 · [E27](docs/webpj-command-inputs-20261108.md) 설정·송금·AI 진단·시나리오 객체·저장 카메라 입력 · [E28](docs/webpj-file-name-20261108.md) 경로 이름 작성기 · [E33](docs/webpj-menu-small-inputs-20261108.md) 소형 일반 입력·메뉴 부모 여섯 개 · [E34](docs/webpj-language-title-time-20261108.md) 언어 요청·창 제목·게임 시간 표시 · [E35](docs/webpj-salvage-ritual-technology-20261108.md) 회수 명령·GetTechnology·의식 선택 입력 · [E36](docs/webpj-production-shortcuts-20261108.md) 생산 단축키 |
| 대본 명령·미션 설정 | 5 | [E30](docs/webpj-add-random-destroy-at-20261108.md) AddRandom·DestroyAt 부모 · [E31](docs/webpj-create-at-20261108.md) CreateAt 생성 부모 · [E32](docs/webpj-make-ai-20261108.md) 일반 MakeAi·AI 설정 · [E37](docs/webpj-pool-reset-battle-options-20261108.md) 월드 Pool 정리·미션 전투 옵션 · [E38](docs/webpj-tutorial-init-20261108.md) 미션 Tutorial Init |
| Graph·다리·섬 | 10 | [E12](docs/webpj-graph-20261108.md) 표면 이웃 탐색·Graph · [E39](docs/webpj-squid-neighbors-20261108.md) raw 풀 연결 이웃 탐색 · [E40](docs/webpj-raw-graph-20261108.md) 월드 Graph 연결 RawGraph · [E42](docs/webpj-bridge-connect-20261108.md) 다리/섬 연결 RawBridgeConnect · [E43](docs/webpj-bridge-events-20261108.md) 다리 이벤트 RawBridgeEvents · [E44](docs/webpj-bridge-decay-20261108.md) 다리 붕괴 순수 논리 Bridge · [E45](docs/webpj-raw-bridge-decay-20261108.md) raw 풀 위 다리 붕괴 RawBridgeDecay · [E46](docs/webpj-bridge-carriers-20261108.md) 다리 위 이동체 대피 RawBridgeCarriers · [E47](docs/webpj-island-post-pop-20261108.md) noIsland 후처리 RawIslandPostPop · [E161](docs/webpj-island-runtime-20261109.md) 섬 런타임 |
| 자원(너겟·간헐천·오벨리스크·마나·에너지) | 9 | [E10](docs/webpj-mana-registration-20261108.md) 마나 공급원 등록 · [E16](docs/webpj-nugget-gun-20261108.md) 너겟 PostPop·Gunanim 방향 · [E29](docs/webpj-energy-supply-20261108.md) 에너지 공급 · [E71](docs/webpj-nugget-asset-owner-gun-mode-20261108.md) 너겟 보충·자산 소유자·총 공격 모드 · [E72](docs/webpj-nugget-regular-carrier-fall-start-20261108.md) 너겟 Regular·Carrier task·낙하 시작 · [E75](docs/webpj-buried-regular-geyser-lifetime-20261108.md) 오벨리스크 Regular·간헐천 수명 · [E105](docs/webpj-gem-reward-20261108.md) 오벨리스크 보상·Deck 추첨 · [E107](docs/webpj-world-geyser-20261108.md) 세계 간헐천 생성 · [E108](docs/webpj-contained-geyser-mined-20261108.md) 종속 조회·간헐천 채집 통지 |
| 주문·Bomb·특수 건물 | 11 | [E17](docs/webpj-bomb-wind-20261108.md) 주문 활성화·공유 바람 · [E48](docs/webpj-acid-zap-20261108.md) 애시드 포획 명령 RawAcidZap · [E49](docs/webpj-arc-spire-20261108.md) 아크 스파이어 RawArcSpire · [E50](docs/webpj-acid-fence-20261108.md) 애시드 울타리 RawAcidFence · [E51](docs/webpj-bomb-pre-post-pop-20261108.md) Bomb 전처리·주문 효과 접두 · [E52](docs/webpj-bomb-effect-timer-20261108.md) Bomb 상태 타이머·패킷 · [E53](docs/webpj-bomb-status-regular-20261108.md) Bomb 상태 주문·Regular · [E54](docs/webpj-bomb-strike-family-20261108.md) Bomb 대상 검색·폭발·효과·직접 피해 · [E109](docs/webpj-ice-tower-20261108.md) 아이스 타워 · [E110](docs/webpj-flare-20261108.md) 플레어 · [E111](docs/webpj-meltdown-20261108.md) 제네레이터 Meltdown |
| 사제·신전·의식 | 16 | [E62](docs/webpj-priest-owner-state-path-20261108.md) 사제 소유자·사제 상태·경로 도착 애니메이션 · [E63](docs/webpj-priest-postpop-shield-20261108.md) 사제 후처리 접두·삭제 준비·보호막 조회/확보 · [E64](docs/webpj-priest-postpop-tail-20261108.md) 사제 후반 후처리·사제 Pop 통합 · [E65](docs/webpj-frame-carrier-priest-fall-20261108.md) 프레임 진행·Carrier 지면 조회·사제 낙하 · [E66](docs/webpj-priest-small-parts-20261108.md) 사제 소형 부품 네 개 · [E67](docs/webpj-priest-damage-burning-20261108.md) 사제 피격·burning 판정 · [E68](docs/webpj-priest-spawn-participation-20261108.md) 사제 생성·참여·포획 안내 · [E69](docs/webpj-priest-regen-20261108.md) 사제 회복 · [E70](docs/webpj-priest-ritual-fort-prep-20261108.md) 사제 의식·Fort 사제 준비 · [E78](docs/webpj-geyser-ai-vortex-owner-diagnostic-20261108.md) 간헐천 AI 등록·신전 소유자 진단 · [E79](docs/webpj-vortex-owner-20261108.md) 신전 소유자 · [E80](docs/webpj-vortex-pre-destroy-20261108.md) 신전 PreDestroy · [E106](docs/webpj-object-conversion-20261108.md) 객체 전환·비로컬 의식 보상 · [E120](docs/webpj-vortex-ascend-20261108.md) 신전 Ascend · [E162](docs/webpj-priest-arrival-20261109.md) 사제 도착 · [E212](docs/webpj-forcefield-lifecycle-20261110.md) 보호막 수명(예약 접두·Regular) |
| 운반·경로 | 20 | [E73](docs/webpj-carrier-bodies-20261108.md) Carrier 낙하·포획·내용물 통지·너겟 전달 · [E90](docs/webpj-carrier-candidate-sort-20261108.md) 운반 후보 비교기/정렬 · [E91](docs/webpj-carrier-regular-20261108.md) 골렘 Regular · [E92](docs/webpj-carrier-steal-20261108.md) 크리스탈 크랩 포획 · [E93](docs/webpj-carrier-drop-20261108.md) 골렘 Cast/Drop · [E94](docs/webpj-balloon-body-20261108.md) 벌룬 그림자 몸체 · [E95](docs/webpj-carrier-drop-point-20261108.md) 운반물 내려놓기 위치 · [E96](docs/webpj-carrier-menu-20261108.md) 운반물 메뉴 · [E97](docs/webpj-carrier-input-20261108.md) 운반물 입력 · [E98](docs/webpj-path-runtime-20261108.md) 경로 런타임 · [E99](docs/webpj-carrier-move-20261108.md) 운반체 이동 · [E100](docs/webpj-carrier-target-20261108.md) 운반 대상 접근 · [E101](docs/webpj-carrier-escape-20261108.md) 운반체 회피 · [E102](docs/webpj-carrier-choice-20261108.md) 운반 후보 선택 · [E103](docs/webpj-path-retarget-20261108.md) 경로 대상 재확인 · [E144](docs/webpj-path-halt-parent-20261109.md) 공통 경로 Halt 부모 · [E145](docs/webpj-path-arrival-parent-20261109.md) 공통 경로 도착 부모 · [E159](docs/webpj-path-cleanup-20261109.md) 경로 정리 · [E160](docs/webpj-carrier-arrival-20261109.md) 운반체 도착 · [E209](docs/webpj-path-unit-frames-20261109.md) 이동 유닛 경로 프레임 3종(벌룬·벌프·워커) |
| 바리케이드·총·투사체 | 21 | [E74](docs/webpj-fence-line-lifetime-20261108.md) 썬 바리케이드 연결선·수명 · [E81](docs/webpj-fence-rebuild-20261108.md) 썬 바리케이드 재생성 · [E82](docs/webpj-gun-target-20261108.md) Gun 표적 · [E83](docs/webpj-fence-create-marker-clear-20261108.md) 썬 바리케이드 표식·포스 필드 생성·표식 삭제 · [E85](docs/webpj-projectile-geometry-20261108.md) 투사체 포구/치수 기하 · [E86](docs/webpj-fence-target-20261108.md) 썬 바리케이드 표적 · [E87](docs/webpj-gun-blocker-20261108.md) 스톤 타워 표적 통지 · [E88](docs/webpj-fence-neighbors-20261108.md) 썬 바리케이드 이웃 검색 · [E89](docs/webpj-fence-regular-20261108.md) 썬 바리케이드 Regular · [E112](docs/webpj-gun-hit-20261108.md) 총 피해 시각/전달 · [E113](docs/webpj-gun-animation-20261108.md) 썬 캐논 애니메이션 · [E114](docs/webpj-cannon-fire-20261108.md) 캐논 공통 발사·아이스 캐논 발사 · [E115](docs/webpj-archer-animation-20261108.md) 아처 애니메이션 · [E116](docs/webpj-crossbow-animation-20261108.md) 크로스보우 발사 · [E117](docs/webpj-lightning-path-20261108.md) 빔 경로 · [E124](docs/webpj-ice-ricochet-20261108.md) 아이스 반사·Missile 필드 이미지 · [E125](docs/webpj-missile-20261108.md) Missile 생성자·프레임 · [E126](docs/webpj-missile-remaining-20261108.md) Missile 잔여 관측: 썬더·초기 아이스·플로터 · [E127](docs/webpj-archer-type-data-20261108.md) 아처 TYPE 자료 · [E210](docs/webpj-damageable-hit-20261109.md) 일반 자산 피격 · [E211](docs/webpj-gun-ground-target-20261109.md) 별도 지상 표적 검색 |
| 건설·배치(일반/사제) | 30 | [E104](docs/webpj-board-approach-special-damage-20261108.md) 건설 접근점·특수 피해 · [E128](docs/webpj-construction-visual-20261108.md) 건설 표시 · [E129](docs/webpj-construction-lifetime-20261108.md) 표시 목록 수명 · [E130](docs/webpj-construction-frame-20261108.md) 건설 표시 프레임 · [E131](docs/webpj-render-collection-20261108.md) 표시 수집기 · [E132](docs/webpj-construction-process-20261108.md) 건설 프로세스 · [E133](docs/webpj-construction-arrival-20261108.md) 건설 도착 · [E134](docs/webpj-construction-packet-notice-20261108.md) 건설 패킷·배치 통지 · [E135](docs/webpj-construction-effects-20261108.md) 건설 효과·선택 어댑터 · [E136](docs/webpj-canon-type-20261108.md) 타입별 Canon 디코더 · [E137](docs/webpj-construction-clear-20261109.md) 건설 예측 정리 · [E138](docs/webpj-construction-place-20261109.md) 건설 배치 · [E139](docs/webpj-construction-confirm-20261109.md) 건설 확정·건설 계열 통합 · [E140](docs/webpj-construction-request-local-20261109.md) 건설 요청·로컬 건설 · [E141](docs/webpj-construction-data-20261109.md) 건설 상태 메시지·기준선 판 정정 · [E142](docs/webpj-construction-builder-20261109.md) 건설 사제 이동 · [E143](docs/webpj-mana-bolt-arrival-20261109.md) 마나 볼트 도착 · [E146](docs/webpj-canon-placement-geometry-20261109.md) 일반 배치 모양 기하 · [E147](docs/webpj-canon-pixel-shape-20261109.md) 일반 픽셀 모양 getter · [E148](docs/webpj-canon-placement-terrain-20261109.md) 일반 배치 지형 · [E149](docs/webpj-canon-placement-permission-20261109.md) 일반 배치 권한 · [E150](docs/webpj-canon-placement-relations-20261109.md) 일반 배치 최종 관계 · [E151](docs/webpj-canon-placement-surrounding-20261109.md) 일반 배치 주변 표면 순회 · [E152](docs/webpj-canon-placement-preview-20261109.md) 일반 배치 접두와 미리보기 · [E153](docs/webpj-canon-placement-collision-20261109.md) 일반 배치 충돌 · [E154](docs/webpj-canon-placement-pipeline-20261109.md) 일반 배치 파이프라인 · [E155](docs/webpj-priest-placement-20261109.md) 사제 배치 접두 · [E156](docs/webpj-priest-placement-geometry-20261109.md) 사제 배치 기하와 픽셀 모양 · [E157](docs/webpj-priest-placement-collision-20261109.md) 사제 배치 미리보기와 충돌 · [E158](docs/webpj-priest-placement-terrain-20261109.md) 사제 배치 지형 |
| AI | 21 | [E76](docs/webpj-ai-command-burning-victim-dude-task-20261108.md) AI 직접 명령·연소 앞단·Dude task · [E163](docs/webpj-ai-dry-geometry-20261109.md) AI 가짜 부족 좌표 탐색 · [E164](docs/webpj-ai-brain-20261109.md) AI 본체 생성/소멸 · [E165](docs/webpj-ai-runtime-20261109.md) AI 런타임 · [E166](docs/webpj-ai-wallet-economy-20261109.md) AI 지갑·경제 · [E167](docs/webpj-ai-build-movement-20261109.md) AI 건설 선택·이동 · [E168](docs/webpj-ai-initial-update-20261109.md) AI 초기 갱신 · [E169](docs/webpj-ai-build-search-20261109.md) AI 배치 탐색 · [E170](docs/webpj-ai-dry-spots-20261109.md) AI 부족 좌표 목록 · [E171](docs/webpj-ai-priest-actions-20261109.md) AI 사제 행동 · [E172](docs/webpj-ai-fake-dry-drop-targets-20261109.md) AI 가짜 부족 좌표·배치 표적 · [E173](docs/webpj-ai-drop-evaluation-scan-20261109.md) AI 배치 평가·스캔 · [E174](docs/webpj-ai-drop-selection-actions-20261109.md) AI 배치 TYPE 선택·행동 · [E175](docs/webpj-ai-deployment-resource-20261109.md) AI 배치 실행·자원 선택 · [E176](docs/webpj-ai-bridge-candidates-planning-20261109.md) AI 다리 후보·계획 · [E177](docs/webpj-ai-bridge-path-20261109.md) AI 다리 경로 · [E178](docs/webpj-ai-bridge-search-strategies-20261109.md) AI 다리 검색·전략 · [E179](docs/webpj-ai-collectors-factory-production-20261109.md) AI 수집자·시설 생산 · [E180](docs/webpj-ai-damage-notice-registration-20261109.md) AI 피해·통지·월드 등록 · [E181](docs/webpj-ai-carrier-actions-preparation-20261109.md) AI 운반 행동·준비 · [E182](docs/webpj-ai-strategy-setup-debug-menu-20261109.md) AI 전략 준비·디버그 메뉴 |
| 유닛 생산·Dude 행동 | 6 | [E121](docs/webpj-wind-aviary-state-20261108.md) 데빌메이커 상태·윈드 원주 점·소환 주문 · [E122](docs/webpj-dude-behavior-20261108.md) Dude 부모 행동·정기 처리 · [E185](docs/webpj-wind-aviary-process-20261109.md) Wind 데빌메이커 프로세스·원주 위치 · [E206](docs/webpj-dude-spawn-20261109.md) Dude 생성·최초 Pop 후처리 · [E207](docs/webpj-dude-notice-20261109.md) Dude 충돌 반응 · [E208](docs/webpj-dude-drop-20261109.md) Dude 드롭 부모 |
| 시각 효과·구름·주변음 | 26 | [E77](docs/webpj-ambient-sound-meta-zone-colorsort-20261108.md) 주변음·메타구역 정리·ColorSort 시작 · [E84](docs/webpj-dais-draw-auxiliary-draw-20261108.md) 다이스 추가 그림·메타 배율/추상색 Draw · [E118](docs/webpj-kencloud-20261108.md) 구름 캐시 · [E119](docs/webpj-kencloud-render-20261108.md) 구름 표시 · [E183](docs/webpj-burning-visual-sources-20261109.md) Burning 표시·연소점 · [E184](docs/webpj-burning-lifetime-draw-20261109.md) Burning 출력·소멸·프레임 판정 · [E186](docs/webpj-lightning-gradient-20261109.md) 입자 색 띠·번개 프레임/수명 · [E187](docs/webpj-fiery-tail-20261109.md) 불꽃 꼬리 프레임/수명 · [E188](docs/webpj-shrapnel-20261109.md) 픽셀 파편 프레임/수명 · [E189](docs/webpj-dissolve-20261109.md) Dissolve(분해) 프레임/수명 · [E190](docs/webpj-evil-ray-20261109.md) EvilRay 프레임/수명 · [E191](docs/webpj-smoke-effect-20261109.md) 냉각 연기 · [E192](docs/webpj-render-process-smoke-visual-20261109.md) 공통 표시 저장소·연기 표시 시스템 · [E193](docs/webpj-dissolve-visual-20261109.md) 분해 표시 시스템 · [E194](docs/webpj-construction-visual-system-20261109.md) 건설 표시 시스템(+E192/E193 부착 예외 보강) · [E195](docs/webpj-fiery-tail-visual-20261109.md) 불꽃 꼬리 표시 시스템 · [E196](docs/webpj-lightning-visual-20261109.md) 빔 표시 시스템 · [E197](docs/webpj-shrapnel-visual-20261109.md) 파편 표시 시스템 · [E198](docs/webpj-evil-ray-visual-20261109.md) EvilRay 표시 시스템 · [E199](docs/webpj-shape-to-buffer-20261109.md) ShapeToBuffer 캡처·VFX 그리기·Squid 그림 헤더 · [E200](docs/webpj-burning-frame-20261109.md) Burning 프레임·BurningFrameCapture · [E201](docs/webpj-burning-visual-20261109.md) Burning 표시 시스템 · [E202](docs/webpj-derived-capture-20261109.md) 파생 그림·캡처 4종 · [E203](docs/webpj-temple-mana-draw-20261109.md) 신전·마나 추가 그림 · [E204](docs/webpj-minimap-raster-20261109.md) 미니맵 셀 그림 · [E205](docs/webpj-color-sort-primary-20261109.md) ColorSort primary |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식 (2026-11-06 재계획)

- **단위 크기:** 한 단위는 cpppj 파일 하나 또는 함께 움직이는 묶음(같은 클래스의 `.cpp/.h/.inc`와 생성 자료)과, 그 파일을 대상으로 하는 cpppj 검사로 이룬다. 함수 하나씩 나누지 않는다. 큰 파일은 공개 API 경계에서 나누고 남은 부분을 단위 문서와 PORT_MAP 비고에 적는다.
- **대응표 `webpj/PORT_MAP.tsv`:** 열은 `cpp`(저장소 기준 cpppj 경로)·`class`(분류)·`web`(웹 모듈 경로, 여럿이면 `;`)·`base`(이식·대조를 마친 cpppj 커밋)·`status`·`note`다. 분류는 `port`(이식 대상), `adapt`(창·입력·소리·파일·시계·캡처처럼 웹 대체가 필요한 플랫폼 경계), `exclude-mp`(1절의 LAN/멀티플레이 제외 목록), `exclude-edit`(Edit·UberEditor·Editor 계열), `exclude-host`(DirectDraw·Direct3D 11/FSR·MCP·CD 탐지·레지스트리·설치·Win32 진단)다. 상태는 `todo` → `partial`(웹 모듈이 파일 일부만 가짐) → `port`(이식, L1/cpppj 검사 통과) → `session`(앱 수명 연결) → `oracle`(cpppj 오라클 비교 통과) 순서로 올리고 제외 분류는 `-`다. **분류는 `tools/port-map.mjs`의 `RULES`가 정하며 대응표에서 손으로 바꾸지 않는다**(규칙과 사유를 고친 뒤 `--write`, 사유 조회는 `--why 이름`). `base`는 웹과 대조·판정한 cpppj 커밋이며 `todo` 행에는 기준선 뒤 변경을 판정했을 때만 적는다. 이식 원본은 언제나 기준선 판이다. 새 `www/core/*.js` 모듈은 반드시 대응 행의 web 칸(여럿이면 `;`)이나 `WEB_ONLY`에 넣는다(빠지면 전체 검사 실패). dotnetpj의 [PORT_MAP](dotnetpj/PORT_MAP.md) 형식을 참고하되 별도 파일로 둔다.
- **검사 이식:** cpppj `tests/`의 대상 검사를 같은 이름의 Node 검사로 옮긴다. 원본 x86 fixture는 `cpppj/tests/fixtures`를 직접 읽고 복사하지 않는다. 웹 적응·제외 경계를 확인하는 웹 전용 검사는 따로 표시한다.
- **생성 자료:** 생성자·vtable·타입·패킷 길이처럼 큰 표는 cpppj 소스에서 생성 스크립트로 만들고 `--check`로 재생성 결과가 같은지 검사한다(WB04-D4 `squid-constructors.js` 방식).
- **수직 진행:** 범위를 넓히기 전에 Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 통과시킨다. 그다음 Early Missions·캠페인 1-1/1-2, 34경로 순으로 넓히며, 각 단계에 필요한 파일만 PORT_MAP에서 골라 옮긴다.

### 3.1 순서3(WB04) 남은 후보 — E214부터

`o/`의 남은 단위(todo 145·partial 33) 가운데 순서4 이후 몫(아래 마지막 항목)을 뺀 것이 순서3의 남은 범위다. 묶음 안에서는 적힌 순서와 대괄호의 선행 관계를 따른다. 완료한 항목은 이 목록에서 지운다. `*`는 partial이며 PORT_MAP 메모의 "남음:" 부분만 해당한다. 순서3 완료 기준은 3.2절 순서3 행이다.

- **입자 표시(E183~E201 완료: 공통 표시 저장소·연기 E192, 분해 E193, 건설 E194, 불꽃 꼬리 E195, 빔 E196, 파편 E197, EvilRay E198, ShapeToBuffer·VFX 그리기·Squid 그림 헤더 E199, Burning 프레임 E200, Burning 표시 시스템 E201):** 일반 Host 부착 `*VisualSystem` 8종을 모두 옮겼다. 남은 입자/표시 client 단위(`RawCampaignParticles`·`ConstructionVisualRenderer`·`ParticlePaletteMemory`(adapt)·`ThunderProcess`·`SpellFrameVisibility`)는 실제 Session 의 소유자와 dirty/선/픽셀/팔레트 소비자 연결이므로 순서5(WB03-M2/WB05-C) 표시 연결에서 옮기고, 그때 cpppj Session 시험(`DamageableBurningSessionTests` 등)으로 전용 시험 없이 웹 조립 검사만 한 파편(E197)·EvilRay(E198)·Burning(E201) 시스템을 다시 대조한다.
- **비행·이동 유닛(E209 완료: RawBalloonFrame·RawBulfFrame·RawWalkerFrame):** RawFlyerProcess·RawFlyerFrame·RawFlyerTarget → RawSunAviaryProcess·RawRainAviaryProcess·RawSunFlyerFrame·RawWindFlyer[RawFlyerProcess의 비행 프로세스 선행].
- **경로:** RawPathProcess·RawPathStart·RawPathData(기준선 뒤 변경 있음, 이식 때 판정)·RawPathSearch*. E159 경로 정리와의 한 장면 통합 시험이 남아 있다.
- **총·피해·삭제 수명(E210 완료: RawDamageableHit, E211 완료: RawGunGroundTarget, E212 완료: RawForcefieldLifecycle, E213 완료: SquidDestroyLifecycle[기준선 typed 경로, Native 분기는 3.6절 ③]):** RawGunProcess(+MakeGunTargetHooks)·RawGunImpact·RawGunTarget* → Missile Host 팩토리[RawGunProcess 뒤]·RawMissile*·RawProjectileGeometry*. RawDamageablePreDestroy·RawDamageablePostDestroy(RawCarrierArrival의 `genusAt` 실제 연결 포함)·RawDamageableRelease·CampaignDamageVtables, RawSquidDestroyDispatch, RawDiscRegular·RawFrameChoose*.
- **다리:** RawBridgeLifecycle*·RawBridgeFallParent*·RawBridgeDraw.
- **자원·생산·건설 명령:** RawLocalGeyser·RawGeyserMined*·RawAssetRegular·RawAssetPostPop*(Gun·제네레이터·거주 구역·바리케이드·간헐천·신전 분배 본체와 AssetPostPopHooks)·RawDaisRegular·RawDaisLifecycle·RawDaisContentNotice·EnergyProviderVtables. RawProductionDeck·RawProductionSync·RawFactoryResearch·RawFactoryUpgrade·RawFactoryUpgradeCommand·RawSpStoreNoticeBindings. RawPacketDispatch → RawConstructionCommand. RawManaBoltRegular[클라이언트 투영 선행].
- **의식·신전:** RawTempleRitual·RawAltarSacrifice·RawRitualChoice·RawVortexAscend*·RawForeignRitualReward*.
- **그림·캡처(E202 완료: Factory·썬더 캐논·훨리베이스·간헐천 파생 캡처, E203 완료: 신전·마나 추가 그림, E204 완료: 미니맵 셀 그림, E205 완료: ColorSort primary. 자산 시험 장치는 `webpj/tests/capture-assets-support.mjs`·`player-scene-support.mjs` 재사용):** RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture·RawColorMapCache(adapt). 각 캡처의 ShapeToBuffer 픽셀 대조는 E199 `render/raw-shape-to-buffer.js` 위에서 E202 `tests/shape-to-buffer-derived.test.mjs` 방식으로 한다. cpppj `RainDrawTests` 의 Renderer 합성 2건은 client `Renderer` 이식 때 옮긴다.
- **공통 수명·그 밖:** SquidPostPop*·RawGraph*·RawSquidNeighbors*·Squid*·SquidSpatial·RawSurfaceLifecycle·RawRegionOwnership·RawTotalMade, RawObjectTrace·RawObjectTraceFields·RawObjectTraceMain·RawContentPreviewProcess·RawCheckingDiagnostics·RawMissionAiSettings*, RawSquidTransmission(기준선 뒤 변경 있음), 분산 SP 정책 연결(E167·E181 단위 문서에서 미확정으로 남김).
- **순서4 이후로 넘기는 `o/` 단위:** 세션·로그인·패킷(RawMission* 로그인/패킷/시작 계열·RawConnection*·PacketRegistrationTable·RawPacketLengths·RawLocalMissionState·RawStateWorldCleanup·RawBattleBroadcast·RawTransportPolling·RawMemStream·FortSerializationGlobals·Player·RawFormPacket*), Fort 적재·지형/섬(RawFort*·RawCampaignWorldLoad·RawTemplateLoad·Template*·RawIsland*·Islandbuilder*·TerrainBuilder*·RawTerrain*·RawFarmCluster·RawRegionPaint*·ChunkMap*·RawDataManager*), 미션/튜토리얼(RawMissionGuide·RawMissionOutcome·RawTutorial*·RawChatManagerPrefix, 순서8), 자료/설정 adapt 경계(Config*·ConfigInterface*·BaseFile*·RawBaseFile*·RiftType*·RiftTypeTable*·TypeParser*·OriginalText*·Xlat*·RawXlat·FileSpecQuery·StartupArchiveFiles·RawConfigFileLoad).

### 3.2 실행 순서

| 순서·상태 | 단위 | 할 일 / 선행 관계 | 완료 기준 |
| --- | --- | --- | --- |
| 1 · [x] | **WB02-G 이식 대응표·기준선 재반영 (2026-11-07 완료)** | 기준선 트리에서 단위 984개 생성·규칙 31개 분류, 기존 웹 모듈 대응, 3.6절 9곳 판정([문서](docs/webpj-port-map-20261106.md)) | 달성: 미분류 0·웹 모듈 미대응 0 |
| 2 · [x] | **WB02-H 무브라우저 실행·cpppj 오라클 기반 (2026-11-07 완료)** | `tools/headless.mjs`·`tools/cpp-oracle.mjs`·`tools/screen-compare.mjs`, M1 8장면 대조([문서](docs/webpj-cpp-oracle-20261107.md)) | 달성: 기록 밖 차이 0·원인 기록 5종·웹 결함 2건 수정. 웹 검사는 저장된 오라클만 읽는다 |
| 3 · [ ] | **WB04 월드 핵심 부품 마감** (A~D5c·E1~E213 완료, E214 이후 3.1절) | 3.1절 후보. 앞 단위가 미룬 연결: 보상의 삭제 장부 연결(`reward_lifecycle_*`, D5b)은 E 부품 연결 때, World 장기 호스트 대여·Client 접합 장면(`ProcessHostStorage`·Kernel procSumm, D5c)은 WB05, 패킷 분배(processform Dispatch 행)는 RawPacketDispatch 단위. 순서3을 마감하기 전에 3.6절 판정 대기(웹 이식분 54단위)를 정리한다 | 해당 cpppj 검사와 원본 fixture 통과, PORT_MAP `port`. 효과 뒤 현재 TYPE/VT/슬롯·진단 복귀·부분 쓰기 순서 보존 |
| 4 · [ ] | **WB05-A/B 월드 조립과 미션 시작** | `RawSurfaceWorld`/`RawWorldCore`·`RawCampaignSession`과 1절의 내부 미션 전송·패킷·미션 로그인 경로, `RawFortWorldLoad`·지형/섬 Builder 계열, Player/동맹/기술/SP/Deck/AI 초기화, Start/Guide·State 전환을 cpppj 부모의 호출 순서로 조립한다. 기존 판독/영역/지형(WB03-U~AB)과 WB04 부품을 같은 월드에 연결하고, 웹 타입 로더의 `zorder`/생성자 주소를 Factory에 넘긴다. 첫 대상은 Tutorial1 | Tutorial1에서 cpppj와 같은 Load/Start 관측(보고서·첫 프레임 논리 화면). Session 덤프가 없으면 3.5절 1번이 정해질 때까지 보고서 범위로 기록한다. 임시 Player/AI/색 번호로 시작 성공을 만들지 않는다 |
| 5 · [ ] | **WB03-M2/WB05-C 월드 표시·카메라·HUD** | `Renderer`·`DrawSort`·타입별 Draw·시각 시스템·`KenCloud`·팔레트 전환, 실제 카메라·가장자리 스크롤, `UberHud`·덱·미니맵·`FloatingSpGump`·커서, `MissionBegin`·`StartDemo`·환영 창 명령 | **M2:** Tutorial1 진입 직후와 N프레임 뒤 논리 화면이 cpppj와 차이 0(L4a). 영어 4:3 다음 16:9/16:10·한국어. `?map=` 개발 화면과 구분 |
| 6 · [ ] | **WB06~08 입력·경제·건설** | `UserInput`·선택/커서·QWASZX·ESC 메뉴·우클릭 메뉴, SP 저장/비용/회수/지갑·Deck/재충전/에너지, 사제 상태/이동/경로/수확/운반, 다리/건물 배치·빌더/생산·취소/실체화 | 같은 입력 단계표에서 cpppj와 같은 상태·화면(L3). **M3:** Tutorial1·2와 캠페인 1-1·1-2를 사용자가 브라우저에서 검증할 수 있음 |
| 7 · [ ] | **WB09~10·WB12 전투·의식·AI** | 총/투사체/피해·벽/타워·유닛/파생 효과, 신전/제단·지식/의식/보상/승패 조건, 공통 전략 AI 전체 행동·탐색/전략·종료 수명 | 현재 Player/기술/SP/명령/난수로 연결하고 cpppj 복원 범위를 타입별로 기록. 임시 방어 AI를 만들지 않음. 포획/희생·특정 적 공격·승리 재현과 공중3종 전용 검사는 1절 경계를 지킴 |
| 8 · [ ] | **WB11·WB13·WB15 미션 수명·안내·소리** | 튜토리얼 게이트/명령/정지·현재 미션 Tell·도움말/지식·한국어 미션, Retry/Next/Abort/메뉴 복귀·실제 진행 저장, 데모 360초 반복/Auto-Demo(cpppj 946~948과 같은 조건), 위치 음향/음악 수명·Wind Noise/좌우 교환 | **M4:** 일반31+데모3의 진입·진행·저장 수명. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분. 스크립트 도움말 Back은 원본 명령 재등록에 연결 |
| 9 · [ ] | **WB16 표시 품질·배포** | 원본 간격 유지 뒤 60/120 표시·성능/메모리, Chrome/Edge/Firefox·GUI Linux, 정적 배포/하위 경로·새로고침·캐시/자산 버전/저장 호환 | **M5:** 개발 서버 없이 같은 공개 자료로 실행. 확인한 브라우저/OS·DPI·소리/저장 환경만 기록. 배포 제품이 정해지면 README 등 공통 문서 일괄 갱신 |

순서 3~5는 큰 덩어리로 한 번에 완료 처리하지 않고 cpppj 파일 단위로 나누어 기록한다.

**진행 기준:** M1 완료 → M2(순서 4~5, Tutorial1의 실제 시작 상태와 표시) → M3(순서 6) → M4(순서 7~8, 범위 내 전체 싱글플레이) → M5(순서 9). M3/M4의 자연 완주·전체 동일성은 사용자 확인으로 남기며 AI 작업을 무기한 막는 완료 조건으로 두지 않는다. 사용자 제보는 해당 단계의 수정 작업으로 올린다.

### 3.3 cpppj 오라클 사용법

| 수단 (cpppj 기준선 실행 파일) | 얻는 것 | 창 |
| --- | --- | --- |
| `--run <자료> --window --mute-audio --frames N --set "k=v;…" --ui-script steps.tsv --ui-report report.tsv` (+ 환경 `NETSTORM_USER_DATA=<빈 폴더>`) | 프레임별 입력(click/right/key/esc/pointer 등)과 `snapshot`(팔레트를 적용한 논리 화면의 32비트 BMP)·`report`(메뉴/세션 관측 TSV). `tools/cpp-oracle.mjs`가 이 형태로 실행한다. `--no-audio`·`NETSTORM_CPP_NO_AUDIO`는 소리 장치가 없는 상태라 Options 소리 항목이 비활성으로 그려지므로 쓰지 않는다(WB02-H) | 뜸 |
| `--run <자료> --frames N --screenshot out.bmp` | N프레임 뒤 논리 화면 | 뜸 |
| `--mcp`: `netstorm_input`·`status`·`report`·`screenshot`·`wait_frames`·`record_start`/`record_stop` | 대화형 조작·관측·소리 포함 녹화(L4b 소리 시점·곡 선택) | 뜸 |
| `--dump-types`·`--inspect-fort`·`--inspect-mission`·`--dump-territories`·`--config-*`·`--inspect-*`·`--dump-font`·`--dump-gif` | 자료 해석 결과 | 없음 |
| `cpppj/tests/fixtures`(원본 x86 대조 자료) | L1 기대값 | — |

- **제한:** `--dump-world`는 저장된 Tutorial 미션만 받는 옛 `GameWorld` 보고서라 실제 Session 비교에 쓰지 않는다. 실제 Session 상태 전체를 내보내는 명령이 없으므로(3.5절 1번) `report`로 관측할 수 없는 범위는 단위 문서에 한계로 적는다.
- **같은 조건:** 같은 자료(`assets/game-data` 기준으로 cpppj 실행 폴더와 `webpj/www/data`의 해시를 대조), 같은 언어·화면 크기·설정(`--set`), 같은 시드와 프레임 단위 단계표, 같은 입력을 쓴다. 맞출 수 없는 조건(소리 장치 시각, 브라우저 시계 등)은 비교에서 빼고 기록한다.
- **실행 규칙:** 창이 뜨는 수집은 AGENTS.md의 실행 확인 대상이다(예외: HJOW-Athlon·VM-W11-Codex·vm-debian-codex). 수집은 해당 단위의 별도 단계에서만 하고, 결과 파일과 cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. `npm test`는 저장된 결과만 읽는다.
- **도구와 절차 (WB02-H):** 시나리오 묶음 `webpj/tests/fixtures/<묶음>-scenarios.json`에 단계표를 적고 `node webpj/tools/cpp-oracle.mjs --exe <기준선 EXE>`로 모은 뒤, `node webpj/tools/screen-compare.mjs --diff <폴더>`로 웹(무브라우저, 원본 비교 모드)과 비교한다. 차이는 고치거나 시나리오의 `expected`에 영역·픽셀 수·원인으로 적는다(영역 밖 0이어야 통과). 라벨 클릭은 cpppj `ControlPoint`처럼 스타일 명령을 뺀 일반 글자로 쓴다(`General Help - F1`). 기준선 EXE는 `cpppj/build`(다른 세션이 계속 다시 빌드)를 쓰지 않고 기준선 worktree에서 따로 빌드한다. 이 PC에는 `extracted/webpj-oracle/1f00a25a/`에 보존했다.
- **알려진 비교 차이:** 메인 메뉴 구름 배경은 원본(=cpppj) Kencloud 출력과 웹 GIF 단순 타일이 행별 1픽셀 어긋난다. 비교 도구는 이를 `background` 기준 장면으로 정밀 분류한다. Kencloud 이식(구름 캐시/표시 부품은 E118·E119 완료, 메뉴 연결은 순서5) 때 0으로 바꾼다.
- **기준선 뒤 cpppj 가 고친 오라클 차이 (2026-11-10):** WB02-H 오라클 문서 5.1의 ③ 스크롤 화살표 색과 ④ Demo Tell 이름(`Demoville`)은 cpppj 가 원본대로 고쳤다. ② Options 의 Pause 행은 원문 조건(`00594fa4`/`005c85a4`)과 cpppj 가 같아 상태 차이(스크린샷은 미션 뒤 메뉴로 판단)로 남긴다. 저장된 M1 오라클은 기준선 빌드라 그대로이며, 재수집 때 ③④ 기록 차이를 지운다. [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md)
- **의미:** 오라클과 같다는 것은 "cpppj와 같다"는 뜻이다. 원본과의 동일성은 cpppj가 원본과 대조한 범위만큼만 보증되므로 L1 원본 fixture 대조를 대체하지 않고 함께 쓴다. cpppj가 미검증·사용자 확인으로 둔 동작(자연 완주·포획/희생·특정 적 공격·승리)은 웹도 같은 경계로 둔다.

### 3.4 cpppj 잔여와 webpj의 관계 (2026-11-06, 제품985·인계986 기준)

| cpppj 상태 | webpj 처리 |
| --- | --- |
| 비멀티 UI(929)·Edit(943)·미션 수명(956)·내부 수명/FILE(980)·와이드 덱(981) 구현·대조 마감 | 웹 이식의 기준 범위다. 이미 완료한 웹 키/reader/UI는 다시 복원 과제로 세지 않고 나머지만 옮긴다. Edit 관련은 제외 |
| 3차 LAN 멀티플레이 사전 구현(982~985) 이후 계속 진행 중 | 웹 제외. 공유 파일(로그인 부모·Session·Player/SP setter·패킷 분배)이 바뀌면 단독 경로 영향을 3.6절로 확인 |
| 6차 한국어 후속(미적용 준비본: 공식 미션 43개 `d/*.korean`·`config.korean` 등 자산 44개, WB 번역 11개를 입력으로 사용) | 미션 문구 한국어는 cpppj가 제품에 반영한 확정본과 통일한다(3.5절 2번). 반영 전에는 영어 대체 |
| 8차 Edit 시나리오 UI(미적용 준비본) | 웹 제외 |
| 4·5·7차(MCP·와이드 해상도·사용자 홈 저장·AA/FSR) | MCP·AA/FSR·Win32 저장 경로는 웹 제외(웹은 브라우저 저장소). 화면비는 웹의 4:3/16:9/16:10 정책을 따른다 |
| checking 잔여 8함수(MP 분류 포함)·전체 동일성/자연 완주 등 사용자 확인 | 웹도 같은 경계. 아직 없는 효과를 일반 성공으로 채우지 않음 |
| 공중 3종(훨리기그·맨오워·더스트 데빌)은 정적 분석·코드 작업만 | 웹도 정적/코드 이식만 하고 전용 생산·행동 검사를 추가하지 않음 |
| cpppj 시험 결함(웹 이식 중 발견, 2026-11-09 WB04-E207) — **해소(2026-11-10 cpppj 수정)** | `DudeNoticeTests` 원본 재생 2건은 종속 표식에 판본별 실제 ProcessForm 가상 표(0x512098/0x505990)를 넣어 cpppj 에서도 통과한다(웹 교정과 같은 방법). 같은 작업에서 4절의 옛 시험 3건도 고쳤다. [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md) |

### 3.5 사용자 확인·요청이 필요한 것

1. **cpppj 실제 Session 상태 덤프 — 구현됨(2026-11-10):** cpppj `--ui-script` 의 `session-dump` 단계가 그 프레임의 SID 풀 해시·사용 슬롯 원문·자산 해석·Kernel/Regular·Player 9·동맹 81·SP 비트·Graph·TotalMade·난수를 판번호 1 TSV 로 쓴다. 같은 입력의 두 실행을 같게 하려면 `--fixed-frame-ms N`(가상 시계)을 함께 쓴다(1-1 240프레임에서 미초기화 SP 소유자 0 외 일치). [덤프·고정 시각 문서](docs/exe/cpp-session-state-dump-20261109.md). 웹 쪽 남은 일: `tools/cpp-oracle.mjs` 에 두 옵션을 넘기는 시나리오 형식, 같은 형식을 만드는 웹 덤프, 기준선 `1f00a25a` 오라클 실행 파일에는 이 기능이 없으므로 재기준선(3.6절) 판단. 그 전까지는 `report`와 논리 화면 범위로 비교한다.
2. **한국어 미션 문구의 단일 원본:** cpppj 한국어 후속 자산(준비본 44개)이 제품에 반영되면 웹이 같은 파일을 `webpj/translations`로 가져와 쓸지(BOM·줄바꿈은 준비 도구에서 정규화), 웹 번역을 따로 둘지 정해야 한다. 기본값은 "cpppj 확정본과 통일"이다.
3. **오라클 수집의 실행 허용:** 예외 호스트가 아닌 PC에서 cpppj 창 실행이 필요하면 단위마다 확인을 받는다.
4. **바깥 링크의 실제 브라우저 확인:** WB03-AD의 새 탭/창 열림과 팝업 차단 여부는 실제 브라우저에서 확인하지 않았다(검사는 판정·앵커 생성까지).

### 3.6 cpppj 변경 재반영

- **기준선:** `1f00a25a`(제품985). 분류는 ① 웹 결과 영향 없음(구조 정리·진단·웹 제외 기능) ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫이다. cpppj fixture가 바뀌었으면 해당 웹 검사를 먼저 다시 돌린다.
- **기준선 이후의 변경:** 매 단위 착수 때 `node webpj/tools/port-map.mjs`의 `cpppj 변경` 줄을 본다(행 base 뒤 커밋을 단위별로 보고하고 안내를 붙인다). `exclude-*` 행은 ①로 기록만 하고, `port`/`adapt` 행은 ②/③을 판정해 ②는 `재반영`을 명시한 WB 단위로 반영한다. 판정을 마치면 그 행 base를 판정한 커밋으로 올리고 근거를 메모(①이라도)와 단위 문서에 남긴다. `fixture-manifest.mjs`도 같은 base 뒤 변경만 보고한다. 기준선 뒤 새로 생긴 파일은 `새 파일`로 보고되며 재기준선 때 분류한다.
- **판정 대기 (2026-11-09, `port-map.mjs` 보고):** `cpppj 변경` 147건 — **②/③ 판정 필요(웹에 이식분 있음) 54단위**, 이식 때 판정(아직 웹에 없음) 84단위, ① 자동(제외 분류) 9단위. 새 파일은 109단위(206파일, 대응표 밖)다. 54단위는 대부분 CP 세션의 `Actual*` 실제 연결·`shared*` 공유 전역·진단 복귀 같은 기준선 뒤 변경이며, 웹 이식 단위가 기준선 판을 옮긴 뒤에 쌓였다. 단독 결과가 바뀌는 ②가 있는지 묶음 단위로 판정해 base를 올리고, ②는 재반영 단위로 옮긴다. 목록은 `node webpj/tools/port-map.mjs`의 `②/③ 판정 필요` 줄이다.
- **판정을 마치고 반영 시점만 남은 ③ (아래 표):** 해당 단위에서 반영하면 이 표에서 지우고 대응표 메모를 고친다. 판정 완료(①·②반영)된 9곳(WB02-G~WB04-E59 판정)과 그 근거는 [보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)과 [대응표 문서5절](docs/webpj-port-map-20261106.md)에 있고, base는 `PORT_MAP.tsv`에 기록돼 있다.

| cpppj 소스(웹 기준 → 기준선 사이 커밋) | 웹 모듈 | ③ 반영 시점과 내용 |
| --- | --- | --- |
| `client/TellSection` `6dd8d803` → `2ca39d0b`·`5a2cd047` | `ui/tell-parameters.js`·`game-data.js` | 대체 tell 적재가 `config.LoadFile` 경로 → **WB05 현재 미션 Tell 연결 때 반영**. `DisplayText` 교체는 cpppj 6차 표시 부호화라 웹은 ① |
| `o/RiftTypeTable`(+`RiftType.h`) `700475b5` → `fbc27346` | `assets/type-flags.js` 외 | 타입 레코드 `defaultFrame`(+0x128). 웹 type-table 정의에는 있으나 Factory용 레코드에 없음 → **WB03-M2/WB05 Draw·타입 레코드 연결 때** |
| `o/RawTerrainContext` `552656c0` → `3eec0830`·`1fe5fae7` | `assets/terrain-mask.js`·`terrain-fringe.js` | `Edition()`·`ArchipelagoMode()` 접근자 → **군도 지형 이식 때** |
| `o/SidPool.cpp/.h` `6035f810`/`b7065914` → `90ffe7e5`·`7b78cc20`·`435cfa1d` | `core/sid-pool.js` | (WB04-D3 판정 유지) 패치판 할당 진단 정상 복귀(`disallowSquidAllocation`·각 free 진단)·소진 시 boss의 현재 World 다리 회수(최대 50, `BindAllocation`)·물리 슬롯 friend. 정상 할당 결과는 같음. 웹은 소진/진단에서 오류로 멈춘다 → **WB05 World 수명 때** |
| `9cb0547f`(일반 전투섬·중앙섬/군도): `client/UberGump.cpp` | `ui/main-menu.js` 등 partial | 로그인 중 "Creating Islands." 안내(일반 전투섬 생성 경로) → **WB05 로그인 이식 때 단독 도달 여부 확인**. base `9cb0547f` |
| 기준선/판정 뒤 ~`44e53bdc`: `client/ClientMain.cpp` | `core/main-loop.js`·`netstorm.js` | (2026-11-07 WB04-D5b) 메인 루프의 연결 감시(439ad8)·전투 자원 프레임(439b34)·재접속 주기 저장 단계, 기록 재생 전역·커서 기록기 식별자, 시간 보정 시계 바인딩 → **WB05 메인 루프 세션 연결 때 단독 도달·단계 순서 재확인**. 힙 진단 로그·Monitor AutoRestart 는 ①. base `44e53bdc` |
| `9cb0547f` 뒤 ~`11fbb988`: `client/UberGump.cpp` | `ui/main-menu.js` 등 partial | ①: LAN 대기실·재접속·Multiplayer 옵션/메뉴(웹 제외). ③: `ShowLoginProgress` 단독 안내(Initializing./Starting Mission...)·`PollBattleResources`(486bc0)·F7/Ctrl+F6 건설 기록 재생 입력 → **WB05 로그인·WB06 입력 때**. base `11fbb988` |
| `1f00a25a` 뒤 ~`3e9dc2c7`: `o/GameClock.cpp/.h` | `core/game-clock.js` | 시간 동기화 보정(0055b4ac) 바인딩과 `(ms−origin−보정)` 감산. 보정 0 이면 기준선과 같은 결과 → **WB05 시간 패킷(5번) 단독 도달 확인 때**. base `3e9dc2c7` |
| `1f00a25a` 뒤 `b8cadde1`: `o/RawFormPacket.cpp/.h` | `core/squid-vtables.js`(VT 칸만) | 삭제 패킷 writer 의 헤더 선기록·client SID 진단 정상 복귀 → **패킷 writer 이식 때**. VT 4종 칸은 ①. base `b8cadde1` |
| `9cb0547f` 뒤 ~`11fbb988`: `o/RawGameAuxState.h` | `core/game-aux-state.js` | 새 장기 전역 칸(StateNotifyType 82·Spell/Income·프로세스 TYPE·건설 헤더·미션 설정 7칸·Quick 건설 등)과 지정 칸 의미 갱신 → **각 소비자 이식 단위에서 보호 PE 초기값과 함께 추가**. 기존 웹 칸의 값/폭 변화 없음. base `11fbb988` |
| 기준선 뒤 `02b00b12`: `o/SquidProcess.cpp/.h` | `core/squid-process.js` | (2026-11-08 WB04-D5c) 발사/회전 애니메이션 프로세스 TYPE 두 칸(`fireAnimationType`·`rotateAnimationType`, 0x005410fc/00541100) → **애니메이션 소비자(`RawCampaignArcherBindings`) 이식 때 `GameAuxState`와 함께 추가**. 나머지 변경은 ①. base `02b00b12` |
| 기준선 뒤 `541624a9`·`417f22d2`·`7ba0130b`: `o/SquidDestroyLifecycle.cpp/.h` | `core/squid-destroy-lifecycle.js` | (2026-11-10 WB04-E213) 실제 PATCH(Native) 전용 분기(`destroy_.HasPatchDiagnostics()`): 현재 전역 getter(`selectedRaw`·`localOwner`·loading 깊이·fort 모드·silent/특수/대체/대형 표면 TYPE)·자산 pre `!isForm()` 0x771 진단·`rememberPlacement`(UI 기억점 원시 좌표 비트)·0040ea00 DWORD 감김 압축 제거·현재 TYPE 표 빌림·최초 로딩 비용 표 소비·선행 `ValidatePre/Post` 생략 → **WB05 World 수명·삭제 UI 전역 연결 때** 같은 시기 `RawSquidDestroy`(+143/−2)·`SquidPostPop`(+35/−1)·`SquidReward`(+6/−3) 변경과 함께 반영. 웹은 기준선 typed 경로만 옮겼다. base `1f00a25a` |

## 4. 이관해도 남겨야 할 미해결 한계

아래는 아직 해소되지 않은 항목이다. 완료한 Tell/링크/도움말 번역/아이콘을 다시 미완료로 만들지 않는다.

| 항목 | 현재 한계와 처리 단계 |
| --- | --- |
| 실제 지형/소유권 | 미리보기의 중립색·가운데 카메라·고정 구름은 개발 정책. 저장 noIsland.owner를 최종 owner로 오해하지 않음. 신전 원소 투표/themeMode·도장/flood 순서·실제 카메라 시작/제한/스크롤은 WB04-E~WB05(3.2절 순서 3~5)에서 연결 |
| 지형 난수/색표 | `buildTerrainFrames`와 절벽의 게임/CRT 난수는 실제 세션에서 공유. 절벽 제외 0x10/0x18의 선택, 최초 팔레트/ColorSort 시점·`useBanners` 기본값, 소유자→색 번호를 WB05(3.2절 순서 4~5)에서 확정 |
| edgeFarm | spots·finder·MayPlace·지면 Retype 뒤 실제 월드에 구현. `RawFarmCluster`의 3×77 별도 시드/30000실패 한도 등 원본 순서를 유지. 타입121에 isle 색표가 있다고 추정하지 않음([근거6절](docs/webpj-terrain-extras-20261102.md)) |
| 타입 표 생성 메타 | 2026-11-06 WB04-D4: Factory는 `{constructorAddress, zOrder, maxHitPoints, flags1, flags2, footX/Y, name}` 레코드 배열을 받는다. 웹 타입 로더(`buildTypeRecords`)는 아직 `zorder`(cpppj `RiftTypeTable.cpp`의 23개 이름 상수+오프셋)와 `constructorAddress`(`typeConstructorAddress`)를 채우지 않는다. 실제 미션 TYPE 표를 Factory에 넘기는 WB05-A(3.2절 순서 4, Fort Load)에서 cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata`와 같이 연결·검사한다 |
| 보상/SP 세션 연결 | 2026-11-07 WB04-D5b: `SquidReward`·`ScrambledSpStore`·`ContentFormLifetime`은 부품 대조까지다. SP 재시드 원천(004559d0 시각)·사제 추적 offset 소유자(0054d4e0)·실제 지갑(`bindPatchWallet`, brain+2d8)·수입 알림/원격 패킷/위치 소리 장치·Client 커서/boss/현재 너겟 getter·`SquidRewardState.bindGameAux`와 World 절반 HP 모드·같은 World TYPE/로딩 비용표 대여는 WB05/WB06 에서 연결한다. 장부는 `SquidPostPopState`(WB04-E)와 같은 객체여야 한다 |
| cpppj 커서 폭 검사 입력 — **해소** | CP 가 `cfbd6420`(20261109-110)에서 시험 입력에 실제 Content VT 를 넣었고, 2026-11-10 새 빌드에서 `CursorPreviewDword_ContentComparisonNative56KeepsAllBits` 통과를 확인했다. [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md) |
| cpppj 옛 검사와 기준선 소스 불일치 — **해소(2026-11-10 cpppj 수정)** | `process_attach_guards`·`ProcessForm_RejectsInvalidNamedFormsBeforeRegistration`(→ `…AfterRegistration` 으로 이름 변경)은 원문 0041bd20 순서(Kernel 등록·패치 회전 뒤 거부, 예약 머리 허용)를 단언하도록, `rawProcessRemoval_native_mixed_form_filter` 는 raw 노드에 실제 Form/자산 가상 표를 넣도록 고쳐 cpppj 에서 통과한다. 웹 시험의 같은 판단과 일치한다. 웹 시험 주석의 "cpppj 옛 검사는 …" 문구는 당시 기록이다. [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md) |
| 영역 회전 | 기존 실제 파일은 방향0. 회전2/3의 실제 저장 사례가 생기면 원본 결과와 추가 대조. 독립 기하 계산 통과를 실제 파일 관측으로 세지 않음 |
| 입력/커서 | 커서2~14·17·18은 쓰는 미션 입력과 함께 추가. 본문 끌기/캡처 중 Overlay의 원문 입력(`004d62b0` 계열) 계약은 미확정. 2026-11-07 WB02-H: 원본처럼 매 프레임 마지막 포인터 위치를 메인 메뉴에 다시 알리게 했다(정지 포인터 아래 펼침 메뉴 행 강조, cpppj 오라클 일치). 미션 입력(UserInput)도 같은 폴링 계약으로 이식한다 |
| 메인 메뉴 구름 배경 | 2026-11-07 WB02-H 오라클 대조: 원본/cpppj는 Kencloud 출력(`ClientKencloudGlobals::Draw`·`o/RawKencloud*`), 웹 메뉴는 GIF 단순 타일이라 일부 행이 가로 1픽셀 어긋난다(4:3에서 37,054픽셀, 원본 스크린샷도 cpppj 쪽). Kencloud 부품(E118·E119)을 메뉴에 연결하는 순서5에서 해소하고 `m1-scenarios.json`의 구름 차이 기록을 0으로 바꾼다 |
| Options/저장된 값 | Help0/Options−40은 측정값이며 일반 위치 규칙/와이드 메뉴 배치는 미확정. Wind Noise·Speaker Swap·Edge Scroll·Auto-Demo는 해당 시스템과 실제 연결 확인 필요. 시작 팁·설정→GameData 동기화/Done* 저장은 이미 구현 |
| 도움말 고급 표시 | `<info>` 머리글/능력치, `#`/`&` 상자 맞춤·색 치환, 원본 삼각형 색 해석 확인. 웹은 원본 스크린샷 기준 검정 화살표를 사용. 2026-11-07 WB02-H: cpppj 기준선은 붉은 갈색(99,38,22)이고 원본 스크린샷 확대는 검정이라 웹을 유지하고 cpppj 쪽 차이로 기록했다. 레이어0/그림자 없는 도움말 그림 경계 유지 2026-11-10: cpppj 가 원문(ScrollingText 의 Slider 에 현재 팔레트 검정 005a4058 을 넘김)대로 고쳐 화살표가 검정이 됐다([cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md) 3.1절). 웹 검정 유지가 맞다 |
| 에너지 기호 | `mana.8~11`은 현재 10.78 클러스터 8개 밖이라 0크기. 실제 원본 F1 모습 관측은 미확인. 원본 실행이 필요하면 AGENTS.md의 확인/예외 규칙 적용 |
| Tell/언어 | `<background>`는 이름만 보관(`nsisles` 자료/사용 근거 미확정). 현재 미션·`{@미션.키}`·브리핑/안내 `.korean`·스크립트 도움말 Back은 WB05·WB11/WB13(3.2절 순서 4·8)에서 연결. 한국어 영어대체/긴 줄/고DPI도 해당 화면과 확인 |
| 메뉴 확장 | 두 열 메뉴는 범위 내 실제 소비자가 필요할 때 원본 계약으로 이식. `%{userSpec}`·User Made Campaigns는 제외이며 새 과제로 돌리지 않음 |
| 입력/오디오/환경 | 실제 스피커 청취·모든 브라우저/GUI Linux·고DPI/모니터 이동은 미확인. 일부 Edge/Chromium 기록을 전체 플랫폼 지원으로 쓰지 않음 |
| README webpj 상태 | README.md·README.en.md의 webpj 상태는 2026-11-06 일괄 갱신(WB04-D5a, 검사 429개) 기준에 머물러 있고 "바깥 링크 미구현" 같은 지난 서술이 남아 있다. README는 공통 문서이므로 다음 공통 문서 일괄 갱신 때 WB02-G 이후 상태로 고친다(README.md "개요" 단락은 수정 금지) |
| Credits AI 문구 | 2026-11-05 사용자 결정: 현행 문구(`OpenAI Codex, ChatGPT and Anthropic Claude Code`, 한국어 동일 취지)를 **유지하고 나중에 수정**한다. cpppj/dotnetpj 문구(Codex·ChatGPT만)와 다르며, 수정 지시가 오면 `www/assets/product.js`의 영어/한국어 섹션과 `tests/product.test.mjs`의 문구 검사를 함께 고친다. 한국어 행 이름 `NetStorm 11.01 패치 크레딧`은 확정 |

## 5. 검증과 완료 판정

| 단계 | 확인 방법과 책임 |
| --- | --- |
| L1 모듈 | 원본10.78 fixture(`cpppj/tests/fixtures`)와 이식한 cpppj 검사로 반환/전체 슬롯·전역·정수/float/double 비트·효과/난수 순서를 대조. Adler 일치는 체크섬 비교이며 바이트 직접 비교와 구분 |
| L2 시작 상태 | 같은 Fort/언어/옵션/시드와 같은 Load/Start 시점에서 cpppj 오라클과 비교. Session 덤프가 생기면 SID·타입·owner·좌표/HP/프레임·Player/동맹/SP·Graph·장부·난수, 그 전에는 `report`와 논리 화면 |
| L3 입력 진행 | cpppj와 같은 단계표(프레임·입력)로 진행한 상태·보고서·화면을 비교. 표시 주사율·비동기 자산 로딩이 게임 난수/시계를 바꾸지 않도록 검사 |
| L4 화면/소리 | L4a: 같은 상태의 논리 8비트 화면이 cpppj `snapshot`과 차이 0. L4b: cpppj MCP 녹화와 소리 시점·곡 선택 비교. 전체 외관/청취 동일성은 사용자 확인. 원본 캡처의 창 테두리/레터박스/HDR·방송 음성을 분리 |
| L5 자연 플레이 | 사용자가 자연 승패→신규 완료값0→1 저장→새로고침/재실행→다음 미션 해제를 확인. 강제 승리/합성 완료값/옛 CP 완주로 대체하지 않음 |

- JS의 BYTE/WORD/DWORD 폭·signed/감김·리틀엔디언·float32 저장·double 시간·x87 중간 정밀도를 명시한다. `Math.imul`/`Math.fround`/`DataView`/BigInt는 필요한 위치에만 사용한다. 비트 차이를 허용 오차로 숨기지 않는다.
- 게임 난수·CRT 난수·SP 쓰기/예약/삭제·callback 뒤 현재 상태 재조회를 보존한다. 렌더링용 상태를 규칙의 별도 월드로 만들지 않는다. 사본 조회/쓰기 대여/양수reset 뒤 수명도 각 모듈에서 명시한다.
- 원본 fixture/보호 자료를 웹 출력으로 덮어쓰지 않는다. 최초 실패/교정·미연결 장치·원본과 다른 보호/웹 적응·누락 fixture의 skip을 기록한다. 검토 커밋과 실제 반영 커밋은 분리한다.
- 기존 Node 검사와 변경에 맞는 집중 검사를 사용한다. 브라우저/자산/통합 검사는 관련 입력·화면·수명이 바뀔 때 수행한다. 소수 단위마다 긴 전체 미션/자연 완주/배포 반복을 자동 실행하지 않는다.
- 확인한 OS/브라우저 버전·논리/물리Canvas·화면 배율/픽셀 비율·소리/저장 환경을 기록한다. UI 스모크와 실제 게임/동일성 증거를 합산하지 않는다.

## 6. 작업 명령과 참고

```powershell
# 저장소 루트에서 정적 개발 서버를 실행한다(공개 루트 webpj/www, 포트9803).
npm --prefix webpj start
# Node 내장 전체 검사. 부품 작업 중에는 관련 tests/*.test.mjs만 집중 실행할 수 있다(webpj 폴더에서 node --test 도 같다).
npm --prefix webpj test
# 이식 추적 목록 검사(해시 비교·대응표 base 뒤 cpppj 근거 소스 변경 보고). 전체 --write 는 수 분~십수 분이 걸리므로 단위마다 --only 를 쓴다.
node webpj/tools/fixture-manifest.mjs
node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>
# 이식 대응표 검사·분류/상태 개수·기준선(행 base) 뒤 cpppj 변경 보고. --write 는 행/분류를 다시 만들고 사람 칸을 보존한다. --why 이름 은 분류 사유.
node webpj/tools/port-map.mjs
node webpj/tools/port-map.mjs --why CompositeMissionTransport
# 웹 무브라우저 실행(논리 화면 BMP·보고서), 저장된 cpppj 오라클과 비교, 오라클 재수집(창이 뜸: AGENTS.md 실행 확인 규칙)
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
node webpj/tools/screen-compare.mjs --diff <폴더>
node webpj/tools/cpp-oracle.mjs --exe extracted/webpj-oracle/1f00a25a/Netstorm.exe [--id 시나리오]
# 준비 자료를 검사한다. 자료 변경이 필요한 단위에서만 prepare-assets로 재생성한다.
npm --prefix webpj run check-assets
npm --prefix webpj run prepare-assets
# 저장소에 있는 Playwright Chromium 초기 페이지 스모크이다.
npm --prefix webpj run test:e2e
```

개발 URL은 `http://localhost:9803/`이며 `?debug&mute&aspect=16:9&scaling=nearest&lang=ko`를 조합할 수 있다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 **게임 규칙 없는 개발용 지형 화면**이다. 시작 클릭 뒤 방향키/드래그로 점검하고 Esc로 메뉴 복귀한다. `previewMap(name)`도 같은 개발 경로다.

- [공통 인계](LEFT_JOBS.md)·[dotnetpj 인계](LEFT_JOBS.dotnetpj.md)·[cpppj 소스 맵](cpppj/SOURCE_MAP.md)
- [RawSurfaceWorld](cpppj/src/client/RawSurfaceWorld.h)·[RawCampaignSession](cpppj/src/client/RawCampaignSession.h)·[RawFortObjectLoad](cpppj/src/o/RawFortObjectLoad.h): 실제 소유자/세션 조립 경계
- [실행 자산](docs/runtime-resources.md)·[형식 목록](docs/formats/README.md)·[가상 파일 조회](docs/formats/vfs.md)·[공유 명칭](assets/unit-names.korean.json)
- [25종 복원 기록](docs/exe/cpp-unit-restoration-20261022.md)·[현재 checking 공백](docs/exe/cpp-checking-consumer-gaps-20261103.md)·[월드 장기 소유](docs/exe/cpp-state-world-owner-plan-20261025.md): 과거 범위는 최신 공통 인계와 함께 확인
- [스크린샷](docs/screens/README.md)·[녹화](docs/videos/README.md)·[애니메이션 시간](docs/videos/animation-timing.md)
- [cpppj 빌드·실행 옵션](docs/cpp-build.md)·[cpppj 로컬 MCP](docs/exe/cpp-mcp-server-20261106.md)·[dotnetpj 포트 맵 형식](dotnetpj/PORT_MAP.md): 오라클 실행과 대응표 형식
- [이식 대응표 문서](docs/webpj-port-map-20261106.md): 분류 규칙·단독 경로 판정·3.6절 판정 근거(WB02-G)
- [오라클 비교 문서](docs/webpj-cpp-oracle-20261107.md): 무브라우저 실행·오라클 수집/비교 규칙·M1 대조 결과(WB02-H)
- [분배기·Content 수명 문서](docs/webpj-content-lifetime-20261107.md): Form 분배·Content 삭제 수명·보상/SP 저장소·NaN 비트 규칙·3.6절 5행 판정(WB04-D5b)

## 7. 문서 정리 기록

- **2026-11-09, HJOW-Athlon, 사용자 요청(인계 문서 정리, 문서만):** `6ca819d6 / 20261109-62 WB` 시점 원문 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)에 바이트 그대로(493,313바이트, SHA-256 `6cf30616…261ccd`) 보존하고, 현행 문서를 진행 상황(0.1~0.3절)과 남은 작업(3.1절 후보·3.6절 판정 대기) 중심으로 다시 썼다. 0.1절 칸·3.2절 순서3 칸·7절에 누적된 단위별 이력, 모두 끝난 즉시 요구/3.1 선행 보완, 이미 끝난 "다음 후보"(사제 배치·RawCarrierArrival·RawPathCleanup·RawManaBoltArrival·RawIslandRuntime 계열·후보/지형 권한), 3.6절 판정 완료 행, 2026-11-08 일시 중지 기록을 보관본으로 옮겼다. 2절 단위별 상세 행은 영역별 색인(2.2절)으로 줄였다. 3.6절의 RawIslandPostPop ③ 행은 E47에서 이미 반영돼(PORT_MAP 메모 `WB04-E47(+3.6③)`) 표에서 뺐고, 이식 판정 대기(54단위)는 `port-map.mjs` 보고로 새로 적었다. 0.3절 수치는 `PORT_MAP.tsv`와 기준선 `1f00a25a` 소스 줄 수로 계산했다. 코드·검사·자산은 바꾸지 않았고 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.
- **이후 기록 방식:** 단위마다 0.5절 5번의 칸만 고치고 단위별 기록 줄은 이 절에 쌓지 않는다(단위 문서와 커밋 메시지가 기록이다). 이 절에는 문서 정리·사용자 결정·작업 중단 같은 단위 밖 사건만 적는다.
- **2026-11-10, HJOW-Athlon, 사용자 요청(webpj 를 위한 cpppj 작업, CP 세션 일시 중지 중):** 3.4·3.5·4절과 단위 문서에 "cpppj 몫"으로 남긴 항목을 cpppj 에서 처리했다(시험 결함 4건 수정·커서 폭 시험 확인, 원본 차이 2건 수정, `session-dump`·`--fixed-frame-ms` 추가). 웹 코드·검사·PORT_MAP 은 바꾸지 않았다. 판정과 검증은 [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md).

**오래된 내용은 [2026-11-09 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)·[2026-11-06 재계획 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261106-through-wb04-d5a)·[WB04-A까지의 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a)에 보관했다. 그 문서의 “현재/다음/미구현”은 당시 기록이며, 이후 작업은 이 현행 문서를 기준으로 진행한다.**
