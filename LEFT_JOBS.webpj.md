# LEFT_JOBS.webpj — 현행 상태와 남은 작업

webpj(HTML + JavaScript 웹 클론)의 **현재 진행 상황·남은 작업(단계별)·유효한 결정**만 둔다. cpppj/공통은 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)가 기준이다. 지난 이력·E1~E242 단위 링크 색인·만료된 계획은 [보관본](LEFT_JOBS.webpj.old.md)에 원문 그대로 있다. 단위별 상세(근거 커밋·fixture·최초 실패와 교정·남은 경계)는 각 `docs/webpj-*.md` 단위 문서와 `webpj/PORT_MAP.tsv` 메모가 기준이다.

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-10, WB04-E251 완료)

| 항목 | 현행 기준 |
| --- | --- |
| 실행·플레이 | 클릭하여 시작→원본 자산 메인 메뉴·캠페인/데모 선택 목록·도움말/옵션·Credits 세 항목(11.01 Patch Credits 포함)과 개발용 지형 미리보기(`?map=`)까지 동작한다. **실제 미션 플레이는 아직 불가**다. 옮긴 월드 부품은 아직 앱 수명에 연결하지 않았고(`PORT_MAP` session 0), `MissionBegin`/`StartDemo`는 미션 월드에 연결되지 않았다 |
| 원본과의 동일성 | 메뉴는 **cpppj 오라클(L4a) M1 8장면**(Welcome·메인 메뉴 8버튼·Campaign·Demo·Credits·Help/Options 펼침 메뉴·General Help, 영어 4:3, 원본 비교 모드)이 기록한 원인 밖 픽셀 차이 0이다(구름 배경 등 원인 기록은 4절). Fort/지형·그림·SID와 월드 부품은 원본 10.78 x86 fixture 또는 독립 계산과 대조했다. 실제 미션의 L2/L3·전체 화면/음향·자연 완주는 미검증이다 |
| 마일스톤 | **M1 완료** · **M2 진행 중**(현재 단계 **3-C**, 0.2절) · M3~M5 미착수 |
| 이식 진행도 | 대상 895단위(제외 89 별도) 중 **port 467(52.2%)·partial 91(10.2%)·todo 337(37.7%)**. 기준선 소스 줄 수로는 129,541줄 중 port 52,600(40.6%)·partial 25,388(19.6%)·todo 51,553(39.8%). 영역별 남은 양은 0.3절 |
| 마지막 구현 커밋 | **WB04-E251** (이 커밋, `20261110-62 WB`) Damageable 삭제 전처리 RawDamageablePreDestroy(붕괴/폭발 효과·소리·spot 6 비트 내용물 순회의 사제 해방 소리·해방·공통 body, 기준선 뒤 Native 몸체)를 원본 10.78 damageablepredestroy 1632행을 typed·Native 두 경로로 대조해 port 로 올렸다([문서](docs/webpj-damageable-pre-destroy-20261110.md)). 그 앞은 **WB04-E250** (`59736d10 / 20261110-61 WB`). 단위 색인은 2절·3.1절 |
| 다음 작업 | **3-C 넷째 항목 RawDamageableRelease**(3.1절, 162줄, +38) [RawDamageablePreDestroy 뒤 — 충족]: 해방 몸체를 기준선 판으로 옮기고 기준선 뒤 변경을 3.6절로 판정해 port 로 올린다. 이후 RawDamageablePostDestroy → RawAssetPostPop → RawAssetRegular → RawLocalGeyser 순으로 진행한다 |
| 마지막 전체 검사 | WB04-E251의 Node v24.21.0, **2244/2244 통과·실패/건너뜀0**(신규 damageable-pre-destroy 5; WB04-E250 2239), 약383초 |
| 마지막 e2e | WB02-H의 Chromium 초기 페이지 스모크 **1/1 통과**. 별도 임시 Playwright 스크립트로 시작→Welcome 닫기→Help 클릭(정지 포인터 아래 펼침 메뉴 첫 행 강조)→General Help 창→Credits 세 행을 실제 클릭 확인(오류 0). 미션/월드 검사가 아니다 |

### 0.2 단계 진행판

순서별 할 일과 완료 기준은 3.2절, 체크 항목이 있는 단계의 목록은 3.1절이다. **진행** 칸의 `n/m`은 3.1절 체크 항목 수다. **규모**는 2026-11-10 정리 때 잰 착수 전 크기로, 기준선 `1f00a25a` 소스 줄 수(partial은 파일 전체 줄이라 상한)이며 `(+N)`은 그중 기준선 뒤 cpppj 변경 줄 수다. client 영역 문자는 0.3절 영역 표를 가리킨다.

| 순서·마일스톤 | 단계 | 내용 | 진행 | 규모 |
| --- | --- | --- | --- | --- |
| 1·2 선행 | WB02-G·H | 이식 대응표·기준선 재반영, 무브라우저 실행·cpppj 오라클 기반 | 완료 (2026-11-07) | — |
| 3 M2 선행 부품 | 3-A | WB04-A~D5c·E1~E242: SID 풀·해시·Kernel·Form/Process 층과 종류별 월드 부품(2절) | 완료 (2026-11-10 마감) | 242+단위 |
| 〃 | 3-B | 다리·표면·영역 공통 수명 | 완료 8/8 | `o/` 8단위 · 1,288줄 (+68) |
| 〃 | 3-C | 자산 정기 처리·피해/삭제 수명 | **진행 중** 3/8 | `o/` 8단위 · 1,931줄 (+269) |
| 〃 | 3-D | 공통 후처리(SquidPostPop)·Graph 조합 마감 | 대기 0/2 | `o/` 2단위 · 1,305줄 (+36) |
| 〃 | 3-E | 기준선 뒤 cpppj 변경 판정(3.6절) — 순서3 마감 관문 | 대기 0/6 | 판정 82단위 |
| 4 M2 | 4-A | 자료·파일·설정·타입 표 | 미착수 | `o/` 15단위 · 3,135줄 (+24) + client M |
| 〃 | 4-B | Fort·지형·섬 적재 | 미착수 | `o/` 26단위 · 3,916줄 (+47) + client B |
| 〃 | 4-C | 패킷·로그인·내부 미션 전송 | 미착수 | `o/` 20단위 · 2,578줄 (+548) + `zacket/` 1 + client A 일부 |
| 〃 | 4-D | Session 조립·Tutorial1 Load/Start(L2) | 미착수 | client A 나머지(RawCampaignSession 5,715줄 등) |
| 5 M2 | 5-A | 렌더러·Draw·시각 시스템 연결 | 미착수 | `o/` 4단위 · 216줄 + client C·J 일부 |
| 〃 | 5-B | 카메라·HUD·미니맵·커서 | 미착수 | `o/` 1단위 · 89줄 + client J 나머지 |
| 〃 | 5-C | 미션 진입 명령과 **M2 판정**(L4a) | 미착수 | — |
| 6 M3 | 6-P | 경로 부품(순서3에서 이월) | 병행 가능 0/3 | `o/` 4단위 · 937줄 (+163) |
| 〃 | 6 | 입력·경제·건설 연결 | 미착수 | client K·E·L |
| 7 M4 | 7-P1 | 비행 유닛 부품(이월) | 병행 가능 0/7 | `o/` 7단위 · 1,401줄 (+92) |
| 〃 | 7-P2 | 총·투사체 부품(이월) | 병행 가능 0/6 | `o/` 6단위 · 1,600줄 (+100) |
| 〃 | 7-P3 | 신전·제단·의식 부품(이월) | 병행 가능 0/7 | `o/` 7단위 · 1,360줄 (+73) |
| 〃 | 7 | 전투·의식·AI 연결 | 미착수 | client D·F |
| 8 M4 | 8 | 미션 수명·안내·소리·채팅 | 미착수 | `o/` 1단위 + client G·H·I |
| 9 M5 | 9 | 표시 품질·배포 | 미착수 | — |

**순서3 마감 조건은 3-B~3-E**다. 3-A 뒤 남은 순서3은 `o/` 18단위·4,524줄과 판정 82단위다(RawBridgeLifecycle 은 E247 에서 판정). 6-P·7-P1~P3은 M2에 직접 필요하지 않아 순서3에서 이월한 부품이며 순서3 마감 조건이 아니다(1절 단계 분할 결정, 3.1절).

### 0.3 남은 이식량 (`webpj/PORT_MAP.tsv`, 2026-11-10 WB04-E251 기준)

줄 수는 기준선 `1f00a25a`의 단위 파일(`.cpp/.h/.inc`) 줄 합계다(주석 포함). partial 줄 수는 일부만 옮긴 파일의 전체 줄이므로 남은 양의 상한이다. 단위·줄 수는 대략적인 척도일 뿐이며, 순서4~5의 연결·통합(실제 세션에서 부품을 같은 월드로 묶고 cpppj 오라클과 비교)은 이 수치에 드러나지 않는 큰 작업이다.

| 범위 | 대상 | port | partial | todo |
| --- | --- | --- | --- | --- |
| 전체 | 895단위 / 129,541줄 | 467 (52.2%) / 52,600줄 (40.6%) | 91 (10.2%) / 25,388줄 (19.6%) | 337 (37.7%) / 51,553줄 (39.8%) |
| `o/` 월드·규칙 | 516단위 / 67,480줄 | 417 (80.8%) / 49,426줄 (73.2%) | 29 (5.6%) / 8,105줄 (12.0%) | 70 (13.6%) / 9,949줄 (14.7%) |
| `client/` 세션·UI·표시 | 378단위 / 61,622줄 | 50 (13.2%) / 3,174줄 (5.2%) | 62 (16.4%) / 17,283줄 (28.0%) | 266 (70.4%) / 41,165줄 (66.8%) |
| `zacket/` 내부 미션 전송 | 1단위 / 439줄 | 0 | 0 | 1 / 439줄 |

남은 428단위(todo 337·partial 91)를 단위 이름 규칙으로 대략 나눈 영역이다. 영역 경계는 근사이며 실제 이식 순서는 0.2절 단계와 cpppj 의존 관계로 정한다. `*`는 partial이다.

| 영역 | 주 단계 | todo 단위 / 줄 | partial 단위 / 줄 | 큰 단위(줄) |
| --- | --- | --- | --- | --- |
| A 세션·로그인·패킷·전송·클라이언트 전역 | 4-C·4-D | 67 / 13,127 | 2 / 1,907 | RawCampaignSession 5,715 · ClientMain* 1,797 · CampaignTemplateStorage 695 · LocalMissionTransport 439 |
| B 월드·Fort 적재·지형/섬·객체 공통 수명 | 3-B~3-D·4-B | 23 / 4,719 | 12 / 3,758 | GameWorld 1,153 · RawSurfaceWorld 804 · SquidPostPop* 675 · RawGraph* 626 |
| C 시각 효과·입자 | 5-A | 5 / 678 | 0 | RawCampaignParticles 513 · ParticlePaletteMemory 65 · ConstructionVisualRenderer 57 · ThunderProcess 32 |
| D 전투·유닛 이동·경로 | 6-P·7-P·7 | 29 / 4,098 | 5 / 1,246 | RawGunProcess 421 · RawGunTarget* 399 · RawMissile* 392 · RawPathProcess 360 |
| E 경제·생산·건설·자원 | 3-C·6 | 16 / 3,194 | 1 / 142 | RawProductionControls 393 · RawDaisRegular 372 · RawAssetRegular 352 · RawCampaignFactoryDisplay 329 |
| F 사제·신전·의식·지식·AI | 7-P3·7 | 13 / 2,123 | 5 / 353 | RawPriestWorldBindings 488 · RawTempleRitual 264 · RawAltarSacrifice 261 · RawPriestMenu 249 |
| G 미션 진행·튜토리얼·안내·대본 | 8 | 13 / 1,273 | 4 / 323 | DialogScript 296 · Mission 150 · RawCampaignTutorialOneBindings 136 · MissionGuideInput 120 |
| H 소리·음악·동영상 | 8 | 10 / 1,666 | 4 / 2,041 | Sound* 824 · ClientAudio* 467 · SoundMusic* 455 · SoundDevice 372 |
| I 채팅·메시지 | 8 | 18 / 1,959 | 0 | ChatWindow 272 · ClientChatRuntime 191 · WarningMessages 181 · RawChatSubmit 142 |
| J 렌더러·카메라·HUD·미니맵 | 5-A·5-B | 44 / 5,142 | 8 / 2,232 | Screen* 816 · Renderer* 492 · SpriteDraw* 468 · PreviewDraw 363 |
| K 게임 입력·커서·선택·객체 메뉴 | 6 | 36 / 4,806 | 2 / 149 | RawCursorMana 289 · RawCampaignCursor 283 · RawCursorRange 283 · CampaignObjectMenuClient 240 |
| L 메뉴·대화상자·Gump·글자 UI | 5·6 | 39 / 6,948 | 37 / 10,315 | UberGump* 2,207 · RawMenuControls 1,341 · RawButtonControls 797 · StyleText* 519 |
| M 자료·파일·설정·언어(웹 적응 포함) | 4-A | 21 / 1,536 | 11 / 2,922 | BaseFile* 586 · Config* 586 · RiftTypeTable* 348 · ConfigInterface* 343 |
| N 진단·추적 | 각 단계 | 3 / 284 | 0 | GifScreenshot 114 · ClientDiagnostics 99 · RawTraceLog 71 |

분류(class)별로는 port 851(이식 대상)·adapt 44(웹 대체가 필요한 플랫폼 경계, port 2·partial 9·todo 33)·제외 89(exclude-mp 6·exclude-edit 55·exclude-host 27·exclude-legacy 1)이다. 이와 별도로 기준선 뒤 cpppj 변경 판정 대기가 있다(3.6절).

### 0.4 기준 값

| 항목 | 현행 기준 |
| --- | --- |
| cpppj 기준선 | **`1f00a25a`(제품985·표시 11.01).** 이식 원본은 언제나 기준선 판(`git show 1f00a25a:경로`)이고, 기준선 뒤 cpppj 변경은 3.6절 절차로만 반영한다. 같은 작업 트리에서 CP 세션이 cpppj를 계속 커밋하므로(LAN 3차·`Actual*` 실제 연결 경로·`shared*` 공유 전역 추가 등) cpppj 상태는 커밋과 소스로 판단한다. **오라클 실행 파일**은 기준선을 따로 빌드한 `extracted/webpj-oracle/1f00a25a/Netstorm.exe`(SHA-256 `03fa4df8…cbc1e`, Git 제외·HJOW-Athlon 전용)다 |
| 이식 범위 | **`webpj/PORT_MAP.tsv` 984단위**(기준선 `o/`555·`client/`428·`zacket/`1, 약 13만 7천 줄). 분류: **port 851·adapt 44·exclude-mp 6·exclude-edit 55·exclude-host 27·exclude-legacy 1, 미분류 0**. 상태: **port 467·partial 91·todo 337·제외(-) 89**, session/oracle 0. 웹 모듈 509개 중 504개 대응·5개 웹 전용. 웹 소스는 `www/` JS 약 6만 4천 줄(`www/data` 제외), 검사 파일 399개, 도구(`webpj/tools`) 오라클 관련 4개다 |
| 이식 추적 | `webpj/tests/fixtures/manifest.json` 518항목(original-x86 487·independent 7·screenshot 13·source 6·original-binary 1·**cpppj-oracle 2**; 최근 WB04-E251에서 damageablepredestroy 1개 추가). original-x86 487개는 모두 `cpppj/tests/fixtures`를 직접 읽는다. cpppj-oracle은 `tests/fixtures/cpppj/m1/oracle.json`(PNG 8장·보고서 8개의 해시를 담은 메타)과 `m1-scenarios.json`이다. `fixture-manifest.mjs`는 대응표 base(판정 커밋) 뒤 변경만 보고하고, `port-map.mjs`는 기준선 뒤 cpppj 변경(3.6절)과 새 파일을 보고한다 |
| 마지막 정밀 디컴파일 | 공통 인계 기준 **HJOW-Athlon / 2026-11-03**(R 전용은 2026-11-07, 같은 호스트). 웹은 cpppj 소스·fixture를 근거로 하므로 디컴파일 산출물을 직접 읽지 않는다 |

### 0.5 매 단위 착수/종료

1. AGENTS.md·이 문서·공통/dotnetpj 인계를 읽고 `git status`·`git fetch`를 확인한다. **같은 작업 트리에서 다른 세션이 cpppj·dotnetpj·공통 문서를 동시에 커밋할 수 있다(2026-11-06 실제 관측).** 공통 인계와 cpppj 파일은 고치지 않고, cpppj 상태는 커밋 기록과 소스로 판단한다.
2. `node webpj/tools/port-map.mjs`로 대응표 검사와 기준선(행 base) 뒤 cpppj 변경을 본다. 이번 단위가 옮길 항목은 **3.1절의 현재 단계 목록에서 위에서부터** 고른다(대괄호 선행 관계를 지킨다). **기준선 판**의 소스(`git show 1f00a25a:경로`)·그 파일을 대상으로 하는 cpppj 검사·fixture·단위 문서를 함께 읽는다. 기준선 뒤 그 파일이 바뀌었는지는 저장소 루트에서 `git diff --numstat 1f00a25a HEAD -- <파일>`로 확인하고, 바뀌었으면 3.6절로 판정해 판정 커밋을 base와 단위 문서에 적는다. 디컴파일이 꼭 필요하면 마지막 수행 PC와 현재 PC를 먼저 비교하고 AGENTS.md의 재디컴파일 규칙을 따른다.
3. 원본/클론 게임(cpppj 오라클 수집 포함)·analyzeManager·별도 창 실행은 AGENTS.md의 확인/예외 규칙을 적용한다. 단위 검사나 정적 확인을 실제 실행으로 기록하지 않는다.
4. 3.0절 방식으로 구현한다. 필요한 공유 장치가 없으면 먼저 이식하고, 빈 성공 함수·별도 근사 월드로 우회하지 않는다. 멀티플레이 분기는 1절의 경계 규칙을 따른다.
5. 근거 커밋·연결 fixture/오라클(SHA/행 수)·검사와 최초 실패/교정·실제 연결/미연결·남은 경계·다음 시작점은 **단위 문서**에 남긴다. `PORT_MAP.tsv`의 web·base·status·메모와 manifest(`node webpj/tools/fixture-manifest.mjs --write --only <아이디,…>`)를 갱신하고 `node webpj/tools/port-map.mjs` 오류 0을 확인한다. 이 문서는 **0.1절(머리글·진행도·마일스톤의 현재 단계·마지막 구현 커밋·다음 작업·마지막 전체 검사)·0.2절 진행 칸·0.3절 수치·0.4절 개수·3.1절의 해당 항목(`- [x]`와 단위 링크)·단계 머리글 `(n/m)`**만 고친다. 단계를 마치면 0.2절에서 다음 단계를 **다음**으로 바꾸고, 순서3을 마감하면(3-E 완료) 3.1절의 완료 목록을 보관본으로 옮긴다. 단위별 긴 설명과 기록 줄은 이 문서에 쌓지 않는다(7절). 이번 단위 파일만 이름으로 stage하고, 커밋 직전에 `git log`로 번호가 겹치지 않는지 확인한 뒤 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. push하지 않는다.

## 1. 범위와 유지되는 결정

- **계획 기준 (2026-11-06 재계획, 사용자 요청):** cpppj는 싱글플레이 범위(비멀티 UI·Edit·미션/내부 수명, 4·5·7차)의 구현·대조를 마치고 3차 LAN 멀티플레이 사전 구현으로 넘어갔다. 웹은 멀티플레이와 Edit를 제외하므로 웹이 옮길 cpppj 코드는 사실상 고정됐다. 그래서 ① cpppj 기준선 고정(`1f00a25a`, 이후 변경은 3.6절 필터) ② cpppj 파일(클래스) 단위 이식과 해당 검사 이식(`PORT_MAP.tsv`로 범위·진행 관리, 3.0절) ③ 원본 x86 fixture 대조(L1)에 더한 cpppj 오라클 비교(L2~L4, 3.3절) ④ 내부 미션 전송·패킷·로그인 경로는 이식하고 LAN TCP·멀티플레이 UI·일반 전투·Outpost 등만 제외(아래)로 진행한다.
- **단계 분할 (2026-11-10, 사용자 요청):** 순서3(M2 선행 부품)이 E1~E242까지 길어져 M2 착수가 늦어졌다. 그동안 남은 순서3 후보 대부분에 기준선 뒤 cpppj 변경이 있어, 변경 없는 단위를 찾아 순서4~8 몫(Fort/섬·튜토리얼 판정·번역 장부 등, E220~E242 대부분)을 앞당겨 옮기는 일이 반복됐다. 그래서 ① 지금까지의 순서3을 **3-A로 마감**하고 ② 남은 순서3을 유한한 목록의 **3-B~3-E**로 나누며 ③ M2(Tutorial1 Load/Start/표시)에 직접 필요하지 않은 경로·비행·총·의식 부품은 **6-P·7-P1~P3로 이월**하고 ④ 순서4·5도 **4-A~4-D·5-A~5-C**로 나눴다. 항목은 현재 단계 목록에서만 고르며, 기준선 뒤 변경이 있다는 이유로 건너뛰지 않고 E213처럼 기준선 판을 옮긴 뒤 변경을 판정한다. 이월 부품은 순서4 단계가 그 부품을 요구하면(예: Tutorial1 Fort에 있는 TYPE의 생성·정기 처리) 그 항목만 앞당긴다. 이 분할은 3.0절의 수직 진행 원칙을 따르기 위한 것이다.
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

## 2. 완료 범위 요약

여기 있는 항목은 해당 부품의 완료이며 실제 캠페인/전체 동일성 완료가 아니다. 월드 부품은 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 단위별 상세·최초 실패·남은 경계는 각 단위 문서와 `PORT_MAP.tsv` 메모의 "남음:"을 따른다.

### 2.1 기반·메뉴·자료·월드 기초 (WB00~WB04-D5c)

| 단위 | 완료 범위와 현재 경계 | 근거 |
| --- | --- | --- |
| WB00·WB01 | 계획·ES 모듈 진입·클릭 대기/실패 재시도·Canvas/기본 입력·앱 수명 | [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a) |
| WB02 메뉴 기반·WB02-D | 자산 준비/manifest·TAFF/Config/GameFiles/GameData/Tell 조회·영어 글꼴. 미션 조회는 4-A·4-D에서 연결 | [데이터층](docs/webpj-data-layer-20261028.md) |
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
| WB03-AB | 값형 Fort 객체/내용물·Terr→Chaff 순회 로더. 실제 월드에는 미연결(4-B) | [객체 로더](docs/webpj-fort-object-load-20261102.md) |
| WB03-AC | 제품 표기 `v11.01`·`Patch1101` 등록 객체·메인 Credits 세 번째 행과 Patch Credits 창(영어/한국어). 원본 대조는 `GameData.build({product:false})`로 10.78·두 행 유지. AI 문구/한국어 문구는 개발자 검수 대상 | [제품11.01](docs/webpj-product-1101-20261105.md) |
| WB03-AD | 바깥 링크 새 탭/창 열기: 판정 `ui/external-link.js`·열기 `platform/external-opener.js`(숨은 앵커 click, `noopener noreferrer`). 실제 브라우저의 새 탭 열림·팝업 차단은 미확인(3.5절 4번) | [외부 접속7절](docs/webpj-external-access-20261105.md) |
| WB04-A | 실제 50바이트 SID 풀·할당/반납/재초기화·20개 삭제 기록. 원본 10.78의 1584전이 대조 | [SID 풀](docs/webpj-sid-pool-20261104.md) |
| WB04-B | `SquidHash`(4단계 WORD 머리)와 `RawSquidFinder`(Begin/Next)를 같은 SID 풀에 연결 | [공간 해시/탐색기](docs/webpj-squid-finder-20261105.md) |
| WB04-C1~C3 | 장기 난수 전역·게임 시계·Kernel / 게임 보조 전역 `GameAuxState`(보호 PE 초기값 51칸 대조) / 원본 40칸 입력 큐와 메인 루프 순서 실행기 `MainLoop` | [세션 장기 상태1](docs/webpj-session-runtime-20261105.md)·[세션 장기 상태2](docs/webpj-game-aux-state-20261105.md)·[큐/메인 루프](docs/webpj-main-loop-20261105.md) |
| WB04-D1~D5c | 공통 공간 해제 부모 / `SquidUnpop` 처리기·삭제기 / 공통 Form 층 / `SquidFactory`(생성자 188칸) / Depend·Gump·Content Form 호스트 / Form 분배·Content 수명·보상/SP 저장소 / Process 호스트·Regular 공통 | [공통 공간 해제](docs/webpj-squid-unpop-20261105.md)·[Unpop·삭제기](docs/webpj-squid-destroy-20261105.md)·[공통 Form 층](docs/webpj-squid-forms-20261106.md)·[SquidFactory](docs/webpj-squid-factory-20261106.md)·[Form 호스트](docs/webpj-form-hosts-20261106.md)·[분배기·Content 수명](docs/webpj-content-lifetime-20261107.md)·[Process 호스트](docs/webpj-process-host-20261108.md) |

### 2.2 WB04-E 월드 부품 (단계 3-A, E1~E242)

E1~E242 단위 문서 링크 색인은 [보관본의 2.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)에 있다. 단위 문서는 `grep -l "WB04-E123" docs/webpj-*.md`처럼 번호로 찾는다(E1~E242 모두 검색됨). E243 이후 단위는 3.1절 항목 끝에 링크한다.

| 영역 | 단위 수 | 단위 번호 |
| --- | --- | --- |
| Player·소유자·State | 13 | E2~E9·E11·E13·E55~E57 |
| 객체 공통 수명·표시·명령·trace | 17 | E1·E14~E15·E24·E26·E41·E58~E61·E123·E213·E230·E235~E236·E240~E241 |
| 기반 유틸리티·입력 분배·패킷 길이·번역 장부 | 15 | E18~E23·E25·E27~E28·E33~E36·E232·E242 |
| 대본 명령·미션 설정·튜토리얼 판정 | 11 | E30~E32·E37~E38·E231·E233~E234·E237~E239 |
| Graph·다리·섬·Fort 메타 | 21 | E12·E39~E40·E42~E47·E161·E214·E220~E229 |
| 자원(너겟·간헐천·오벨리스크·마나·에너지) | 9 | E10·E16·E29·E71~E72·E75·E105·E107~E108 |
| 주문·Bomb·특수 건물 | 11 | E17·E48~E54·E109~E111 |
| 사제·신전·의식 | 17 | E62~E70·E78~E80·E106·E120·E162·E212·E219 |
| 운반·경로 | 20 | E73·E90~E103·E144~E145·E159~E160·E209 |
| 바리케이드·총·투사체 | 21 | E74·E81~E83·E85~E89·E112~E117·E124~E127·E210~E211 |
| 건설·배치(일반/사제) | 30 | E104·E128~E143·E146~E158 |
| AI | 21 | E76·E163~E182 |
| 유닛 생산·Dude 행동 | 10 | E121~E122·E185·E206~E208·E215~E218 |
| 시각 효과·구름·주변음 | 26 | E77·E84·E118~E119·E183~E184·E186~E205 |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식 (2026-11-06 재계획)

- **단위 크기:** 한 단위는 cpppj 파일 하나 또는 함께 움직이는 묶음(같은 클래스의 `.cpp/.h/.inc`와 생성 자료)과, 그 파일을 대상으로 하는 cpppj 검사로 이룬다. 함수 하나씩 나누지 않는다. 큰 파일은 공개 API 경계에서 나누고 남은 부분을 단위 문서와 PORT_MAP 비고에 적는다.
- **대응표 `webpj/PORT_MAP.tsv`:** 열은 `cpp`(저장소 기준 cpppj 경로)·`class`(분류)·`web`(웹 모듈 경로, 여럿이면 `;`)·`base`(이식·대조를 마친 cpppj 커밋)·`status`·`note`다. 분류는 `port`(이식 대상), `adapt`(창·입력·소리·파일·시계·캡처처럼 웹 대체가 필요한 플랫폼 경계), `exclude-mp`(1절의 LAN/멀티플레이 제외 목록), `exclude-edit`(Edit·UberEditor·Editor 계열), `exclude-host`(DirectDraw·Direct3D 11/FSR·MCP·CD 탐지·레지스트리·설치·Win32 진단), `exclude-legacy`(raw 풀 이전의 옛 단독 모델로 production 소비자가 없고 raw 구현이 대체한 단위)다. 상태는 `todo` → `partial`(웹 모듈이 파일 일부만 가짐) → `port`(이식, L1/cpppj 검사 통과) → `session`(앱 수명 연결) → `oracle`(cpppj 오라클 비교 통과) 순서로 올리고 제외 분류는 `-`다. **분류는 `tools/port-map.mjs`의 `RULES`가 정하며 대응표에서 손으로 바꾸지 않는다**(규칙과 사유를 고친 뒤 `--write`, 사유 조회는 `--why 이름`). `base`는 웹과 대조·판정한 cpppj 커밋이며 `todo` 행에는 기준선 뒤 변경을 판정했을 때만 적는다. 이식 원본은 언제나 기준선 판이다. 새 `www/core/*.js` 모듈은 반드시 대응 행의 web 칸(여럿이면 `;`)이나 `WEB_ONLY`에 넣는다(빠지면 전체 검사 실패). dotnetpj의 [PORT_MAP](dotnetpj/PORT_MAP.md) 형식을 참고하되 별도 파일로 둔다.
- **검사 이식:** cpppj `tests/`의 대상 검사를 같은 이름의 Node 검사로 옮긴다. 원본 x86 fixture는 `cpppj/tests/fixtures`를 직접 읽고 복사하지 않는다. 웹 적응·제외 경계를 확인하는 웹 전용 검사는 따로 표시한다.
- **생성 자료:** 생성자·vtable·타입·패킷 길이처럼 큰 표는 cpppj 소스에서 생성 스크립트로 만들고 `--check`로 재생성 결과가 같은지 검사한다(WB04-D4 `squid-constructors.js` 방식).
- **수직 진행:** 범위를 넓히기 전에 Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 통과시킨다. 그다음 Early Missions·캠페인 1-1/1-2, 34경로 순으로 넓히며, 각 단계에 필요한 파일만 PORT_MAP에서 골라 옮긴다. 2026-11-10 단계 분할(1절)은 이 원칙으로 돌아가기 위한 것이다.

### 3.1 단계별 항목 목록

순서3의 남은 단계(3-B~3-E)와 순서3에서 이월한 순서6/7 선행 부품(6-P·7-P1~P3)의 항목이다.

- **진행 방법:** 단계는 0.2절 순서대로, 단계 안에서는 위에서부터 진행하고 대괄호 `[… 뒤]` 선행 관계를 지킨다. 항목을 끝내면 `- [ ]`를 `- [x]`로 바꾸고 끝에 ` → [E번호](단위 문서) 요약`을 붙인다. 한 항목을 여러 단위로 나누면 링크를 이어 붙이고 마지막 단위에서 체크한다. 단계 머리글의 `(n/m)`과 0.2절 진행 칸을 함께 고친다.
- **수치:** 줄 수는 기준선 소스 줄(partial은 파일 전체 줄), `(+N)`은 기준선(또는 행 base) 뒤 cpppj 변경 줄 수다. partial 항목은 PORT_MAP 메모의 "남음:" 부분만 해당한다.
- **기준선 뒤 변경:** `(+N)`이 있는 항목은 **기준선 판을 옮기고** 바뀐 부분을 3.6절 ①/②/③으로 판정해 단위 문서·PORT_MAP base·메모에 남긴다(E213 선례: 기준선 typed 경로를 옮기고 Native 분기는 3.6절 ③ 표에 적음). 변경이 있다는 이유로 건너뛰지 않는다.
- **목록 밖 단위:** 다른 단계 몫을 끼워 넣지 않는다. 항목이 선행 미완료로 막히면 사유를 그 항목에 적고 같은 단계의 다음 항목으로 간다. 단계 전체가 막히면 사유를 0.1절 다음 작업에 적고, 선행이 갖춰진 이월 부품(6-P·7-P) 항목을 진행할 수 있다. 목록에 없던 단위가 꼭 필요하면 해당 단계 목록 끝에 항목을 추가하고 이유를 적는다.

#### 3-B 다리·표면·영역 공통 수명 (8/8)

완료 기준: 8항목이 PORT_MAP `port`(또는 대응 없음 사유를 남긴 분류 판정)이고 해당 cpppj 검사·fixture를 통과한다. 예외: `Squid` 는 옛 struct 잔여(GroundGrid 의존)를 4-B 로 이월하고 partial 을 유지한다([E246](docs/webpj-squid-legacy-20261110.md)).

- [x] **RawSquidNeighbors** (partial 296줄) — 남은 시험(일반 해시 flag 0/4 의 다리 이벤트 장면, 울타리 장면)이 E43·E88 에서 이미 재생됐는지 확인하고 남은 장면만 옮겨 port 로 올린다 → [E243](docs/webpj-neighbor-first-20261110.md) 일반 해시 첫 이웃·다리 끝 칸 변환 원본 10.78 576행 대조
- [x] **RawBridgeFallParent** (partial 48줄) — 남은 ScheduleBridgeFallChecked(SquidProcessHost 연결) → [E244](docs/webpj-bridge-fall-parent-host-20261110.md) 다리 낙하 진단 부모의 실제 Host 연결
- [x] **RawBridgeDraw** 38줄 → [E245](docs/webpj-bridge-draw-20261110.md) 다리 캡처 가상 그림 몸체와 원본 1200행 대조
- [x] **Squid** (partial 115줄) — 저장 필드·공간 단계의 나머지 → [E246](docs/webpj-squid-legacy-20261110.md) Squid 소비자 확인(옛 struct 4-B 이월)과 SquidSpatial exclude-legacy 분류
- [x] **SquidSpatial** 301줄 — 옛 구조 모델이라 웹 대응이 없다고 판단해 둔 단위다. 실제 소비자를 확인해 port 하거나, 대응이 필요 없으면 `tools/port-map.mjs` 규칙·사유를 고쳐 분류한다(3.0절) → [E246](docs/webpj-squid-legacy-20261110.md) Squid 소비자 확인(옛 struct 4-B 이월)과 SquidSpatial exclude-legacy 분류
- [x] **RawBridgeLifecycle** (partial 180줄, +8) — WB04-D2 삭제 훅 뒤 남은 다리 가상 삭제 분배(E214 RawSquidDestroyDispatch 위) → [E247](docs/webpj-bridge-lifecycle-20261110.md) 다리 삭제 전후 훅 원본 2806행 대조와 Native 연결 TYPE 경로
- [x] **RawSurfaceLifecycle** 127줄(+6) [RawBridgeLifecycle 뒤] → [E248](docs/webpj-surface-lifecycle-20261110.md) 표면 삭제 훅 분배기와 통합 장면 9건
- [x] **RawRegionOwnership** 183줄(+54) — outpost 분기는 RawOutpostLifecycle(exclude-mp)로 명시 웹 제외 → [E249](docs/webpj-region-ownership-20261110.md) 지역 소유 투표 원본 440행 대조와 Native 몸체

#### 3-C 자산 정기 처리·피해/삭제 수명 (3/8)

완료 기준: 8항목이 PORT_MAP `port`이고 해당 cpppj 검사·fixture를 통과한다.

- [x] **EnergyProviderVtables** 87줄 → [E250](docs/webpj-vtable-tables-20261110.md) 에너지 공급·캠페인 피해 가상 표 자료와 생성기
- [x] **CampaignDamageVtables** 175줄 → [E250](docs/webpj-vtable-tables-20261110.md) 에너지 공급·캠페인 피해 가상 표 자료와 생성기
- [x] **RawDamageablePreDestroy** 141줄(+36) → [E251](docs/webpj-damageable-pre-destroy-20261110.md) Damageable 삭제 전처리 원본 1632행 대조와 Native 몸체
- [ ] **RawDamageableRelease** 162줄(+38) [RawDamageablePreDestroy 뒤]
- [ ] **RawDamageablePostDestroy** 216줄(+19) — RawCarrierArrival 의 `genusAt` 실제 연결 포함
- [ ] **RawAssetPostPop** (partial 525줄, +30) — Gun·제네레이터·거주 구역·바리케이드·간헐천·신전 분배 본체와 AssetPostPopHooks
- [ ] **RawAssetRegular** 354줄(+72) [RawAssetPostPop 뒤]
- [ ] **RawLocalGeyser** 271줄(+74)

#### 3-D 공통 후처리·Graph 조합 마감 (0/2)

완료 기준: 3-A~3-C 에서 옮긴 몸체까지 연결·대조한다. 아직 없는 몸체(비행·Dais·Player 등 이월/순서4 몫)는 PORT_MAP "남음:"으로 남기고 partial 을 유지해도 이 단계는 완료로 본다.

- [ ] **RawGraph** (partial 628줄) — 남은 조합 대조(rawgraph·regiongraph·graphremove·graphrecovery)는 E41 SquidPostPop 시험이 재생했다고 메모돼 있으므로 확인 후 port 로 올린다(Commit 은 실제 실행이 쓰지 않아 제외)
- [ ] **SquidPostPop** (partial 677줄, +36) — 표시 dirty 열, 3-A~3-C 몸체의 setBody/set*Prefix 연결, 섬/표면/Teleport 접두

#### 3-E 기준선 뒤 cpppj 변경 판정 — 순서3 마감 관문 (0/6)

2026-11-10 `node webpj/tools/port-map.mjs` 보고(HEAD `d4b82c30`)의 "②/③ 판정 필요" 87단위 가운데 `o/` 82단위와 `client/RawChatChannels`다. 나머지 client UI 4단위(ClientMain·UberGump·MenuGumpWidgets·ScrollingTextGumpWidgets)는 4-D·5 연결 때 판정한다. 착수 때 판정 기준 커밋을 하나 정해 그 커밋까지의 변경만 판정하고 행 base 를 그 커밋으로 올린다(CP 세션이 계속 커밋하므로 그 뒤 변경은 다음 단계 착수 때 다시 본다). 3-B~3-D 에서 옮기며 판정한 단위는 빠진다. 대부분 CP 의 `Actual*`/`shared*` 장기 연결이라 ③(4-D 연결 때)일 가능성이 높지만, 단독 결과가 바뀌는 ②는 `재반영`을 명시한 단위로 옮긴다. 묶음마다 결과를 3.6절 표와 [대응표 문서](docs/webpj-port-map-20261106.md) 판정 절에 남긴다.

- [ ] **판정-공통층** 24단위 — ConfigInterface·ContentFormHost·ContentFormLifetime·DependFormHost·GumpFormHost·RawFortObjectLoad·RawGameAuxState·RawMissionBattleOptions·RawSquidContainedUnpop·RawSquidDestroy·RawStateSidLists·RiftType·RiftTypeTable·SidPool·SquidDestroyLifecycle·SquidDisplay·SquidFactory·SquidFormDispatch·SquidFrame·SquidPostPop·SquidPostPopPatch·SquidProcess·SquidUnpop·Xlat
- [ ] **판정-Player·사제·신전** 12단위 — RawPlayerContentNotice·RawPlayerGeyserBounds·RawPlayerIdentity·RawPlayerParticipation·RawPlayerWorld·RawPriestDamage·RawPriestForcefield·RawPriestPreDestroy·RawPriestRegen·RawVortexAscend·RawVortexOwner·RawVortexPreDestroy
- [ ] **판정-섬·다리·자원** 7단위 — RawBridgeEvents·RawIslandPostPop·RawIslandRuntime·RawGeyserLifetime·RawNuggetRegular·RawNuggetReplenish·RawAssetPostPop
- [ ] **판정-건설·운반·Dude** 15단위 — RawBuildingSpace·RawCanonPlacementCollision·RawConstructionClear·RawConstructionConfirm·RawConstructionPlace·RawConstructionProcess·RawConstructionRequest·RawCarrierFalling·RawCarrierPreDestroy·RawCarrierRegular·RawCarrierTransfer·RawDudeBehavior·RawDudeNotice·RawDudeRegular·RawDudeTask
- [ ] **판정-전투** 18단위 — RawBombEffect·RawBombEffectTimer·RawBombRegular·RawBombStrike·RawBombTargets·RawBombTimerPacket·RawArcherAnimation·RawCannonFire·RawCrossbowAnimation·RawGunAnimation·RawGunHit·RawGunTarget·RawIceCannonFire·RawMissile·RawAcidFence·RawFenceCreate·RawFenceMarkerClear·RawFenceNeighbors
- [ ] **판정-기타** 6단위 — RawFlare·RawIceTower·RawPathRuntime·RawWindAviaryProcess·RawWindAviaryState·client/RawChatChannels

#### 순서3에서 이월한 선행 부품 (순서3 마감 조건 아님)

M2(Tutorial1 Load/Start/표시)에 직접 필요하지 않아 순서6/7 몫으로 넘긴 `o/` 부품이다. 순서4 단계가 이 부품을 요구하면(예: Tutorial1 Fort 에 있는 TYPE 의 생성·정기 처리) 그 항목만 앞당긴다. 그 밖에는 3-E 를 마친 뒤나 현재 단계가 선행 대기로 막혔을 때 진행한다. 공중3종(훨리기그·맨오워·더스트 데빌)과 사제 포획/희생은 1절 사용자 확인 경계를 지킨다.

#### 6-P 경로 부품 (0/3)

- [ ] **RawPathProcess** 362줄(+10)과 **RawPathSearch** partial 240줄 — 서로 의존하므로 한 항목으로 옮긴다. E159 경로 정리와의 한 장면 통합 시험 포함
- [ ] **RawPathStart** 199줄(+2) [RawPathProcess 뒤]
- [ ] **RawPathData** 136줄(+151) [RawPathProcess 뒤] — 기준선 뒤 변경이 커서 판정 비중이 크다

#### 7-P1 비행 유닛 부품 (0/7)

- [ ] **RawFlyerProcess** 238줄(+53) — 비행 프로세스(RawFlyerFrame 이 FlyerProcessFields 를 요구)
- [ ] **RawSunAviaryProcess** 214줄(+20) [RawFlyerProcess 뒤]
- [ ] **RawRainAviaryProcess** 180줄(+19) [RawFlyerProcess 뒤]
- [ ] **RawFlyerFrame** 259줄 [RawFlyerProcess·RawSunAviaryProcess 뒤]
- [ ] **RawFlyerTarget** 194줄 [RawFlyerProcess 뒤]
- [ ] **RawSunFlyerFrame** 184줄 [RawFlyerProcess 뒤]
- [ ] **RawWindFlyer** 132줄 [RawFlyerProcess 뒤]

#### 7-P2 총·투사체 부품 (0/6)

- [ ] **RawGunProcess** 423줄(+33) — MakeGunTargetHooks 포함. `o/Player.h` 를 쓰므로 필요한 부분이 없으면 4-C 의 Player 뒤로 미룬다
- [ ] **RawGunTarget** (partial 401줄, +2) — MakeGunTargetHooks·보호 SHP 자산 sunCannon 사정선 48행(cannon-asset-ray) [RawGunProcess 뒤]
- [ ] **RawMissile** (partial 394줄, +24) — Host 팩토리(MakeMissileFactoryHooks/CreateMissile/MakeGunMissileHooks)와 호스트 자기 삭제 시험 [RawGunProcess 뒤]
- [ ] **RawProjectileGeometry** (partial 176줄) — muzzle 71 외·hotspot·directionFrame·아이스/썬더(129/131) 사정선의 소비자 fixture 대조와 실제 SHP 헤더 연결
- [ ] **RawGunImpact** 136줄(+12) [RawMissile·RawAssetRegular(3-C) 뒤]
- [ ] **RawDiscRegular** 70줄(+29) — 썬 디스크 Regular

#### 7-P3 신전·제단·의식 부품 (0/7)

- [ ] **RawTempleRitual** 266줄(+18)
- [ ] **RawRitualChoice** 27줄 — cpppj 에서 MP 6b 입력/수신 사전 구현이라 단독 도달(웹 범위)을 먼저 확인
- [ ] **RawDaisRegular** 374줄(+12) [RawAssetRegular(3-C)·RawPathStart(6-P)·RawTempleRitual 뒤]
- [ ] **RawAltarSacrifice** 263줄(+24) [RawDaisRegular·RawDamageablePreDestroy(3-C) 뒤]
- [ ] **RawDaisLifecycle** 192줄(+15) [RawAltarSacrifice 뒤]
- [ ] **RawVortexAscend** (partial 181줄, +4) — RawIslandRuntime 을 직접 읽는 두 번째 생성자
- [ ] **RawForeignRitualReward** (partial 57줄) — mode 1(SP) 180행의 실제 분산 SP 저장소/정책 대조. E167·E181 단위 문서에서 미확정으로 남긴 분산 SP 정책 연결과 함께 정한다

### 3.2 실행 순서 (순서 4~9)

순서 1~3은 0.2절·3.1절이다. 아래 단계는 착수할 때 3.1절과 같은 체크 항목 목록으로 펼치고, 큰 단위(예: RawCampaignSession 5,715줄)는 공개 API 경계로 나눈다. 한 단계를 한 번에 완료 처리하지 않고 cpppj 파일 단위로 나누어 기록한다.

| 단계 | 할 일 | 선행 | 완료 기준 |
| --- | --- | --- | --- |
| **4-A** 자료·파일·설정·타입 표 | `o/` 15단위: adapt 8(BaseFile*·RawBaseFileIO·RawBaseFileOpenPolicy·RawBaseFileReadPolicy·RawBaseFileWholeRead·FileSpecQuery·StartupArchiveFiles·RawConfigFileLoad, 브라우저 자료 조회로 대체)·Config*·ConfigInterface*·OriginalText*·Xlat*·RiftType*·RiftTypeTable*·TypeParser*, client M 영역. XlatFileRead(TARC 읽기)와 E242 RawXlat 연결(xlat-file-read fixture 253행). 웹 타입 로더가 `zorder`/생성자 주소를 Factory 레코드에 채움(4절 "타입 표 생성 메타")과 3.6절 ③ `RiftTypeTable` defaultFrame | 3-E | 미션 자료(Tell·`.fort`·타입 표)를 cpppj 와 같은 순서로 조회하고, cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata` 대응 검사 통과 |
| **4-B** Fort·지형·섬 적재 | `o/` 26단위: RawFortImage·RawFortMoneyLoad·RawFortObjectLoad*·RawFortObjectStream·RawFortRandomPlacement·RawFortWorldLoad·RawCampaignWorldLoad·RawTemplateLoad·Template*·RawDataManager 5종(Image·Lifetime·List·Resize·Selection)·RawMemStream·FortSerializationGlobals·Islandbuilder*·TerrainBuilder*·RawTerrainBitmap·RawTerrainContext*·RawTerrainGrowth[RawTerrainContext 뒤]·RawTerrainStamp*·RawFarmCluster·RawRegionPaint*·ChunkMap*·RawFrameChoose*(WB03-X `chooseTerrainFrame` 에 시험 보강), client B 영역(GameWorld·RawSurfaceWorld 등). WB03-U~AB 판독/지형과 E220~E228 섬/Fort 부품을 같은 월드에 연결. `o/Squid` 의 옛 struct 저장 필드·Advance/Face(GroundGrid/CellPoint·FortContentList 의존, 소비자는 옛 GameWorld/UberGump)는 GameWorld 이식 여부 판정과 함께 이 단계에서 닫는다(E246). 3.6절 ③ RawTerrainContext(군도 접근자) | 4-A | Tutorial1 Fort 적재 결과(객체·지형·섬)가 cpppj 와 같음(`--inspect-fort`·session-dump 범위) |
| **4-C** 패킷·로그인·내부 미션 전송 | `o/` 20단위: PacketRegistrationTable·RawPacketDispatch·RawConnectionActivity·RawConnectionCleanup·RawConnectionRegistration·RawConnectionRegistry·RawTransportPolling·RawMissionClientPackets·RawMissionServerPackets·RawMissionLogin·RawMissionLoginPackets·RawMissionLoginTimer·RawMissionSettingsPacket·RawLocalMissionState·RawBattleBroadcast·RawFormPacket*(패킷 writer, 3.6절 ③)·RawSquidTransmission·Player·RawConstructionCommand·RawGeyserMined*(RegisterGeyserMined), `zacket/LocalMissionTransport`, client A 영역의 전송/로그인 서비스. 분배기 등록이 미뤄진 부품(E108 간헐천 채집·E217 RegisterFactoryResearchCommand·processform Dispatch 행)을 함께 연결. 1절 단독 경로 경계 | 4-A | 단독 미션의 로그인→Ready→Start 패킷 순서와 내용이 cpppj 와 같음 |
| **4-D** Session 조립·Tutorial1 Load/Start | RawCampaignSession·ClientMain*·client 전역, `RawSurfaceWorld`/`RawWorldCore`, Player/동맹/기술/SP/Deck/AI 초기화, Start/Guide·State 전환을 cpppj 부모 호출 순서로 조립. 앞 단위가 미룬 연결: 보상의 삭제 장부(`reward_lifecycle_*`, D5b)·World 장기 호스트 대여와 Client 접합 장면(`ProcessHostStorage`·Kernel procSumm, D5c)·3.6절 ③ 표의 WB05 행(TellSection·SidPool·ClientMain·UberGump·GameClock·GameAuxState·SquidDestroyLifecycle 등)·client UI 4단위 판정. 웹 Session 덤프와 오라클 시나리오 형식(3.5절 1번) | 4-B·4-C | **L2:** Tutorial1 Load/Start 관측(보고서·session-dump·첫 프레임 논리 화면)이 cpppj 와 같음. 임시 Player/AI/색 번호로 시작 성공을 만들지 않음 |
| **5-A** 렌더러·Draw·시각 시스템 연결 | `Renderer`·`DrawSort`·타입별 Draw·팔레트 전환, 입자/표시 client 단위(RawCampaignParticles·ConstructionVisualRenderer·ParticlePaletteMemory(adapt)·ThunderProcess·SpellFrameVisibility), `o/` 4단위(RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture·RawColorMapCache(adapt), 캡처 픽셀 대조는 E199 `render/raw-shape-to-buffer.js` 위 E202 방식), cpppj `RainDrawTests` Renderer 합성 2건, KenCloud 메뉴 연결(4절 구름 차이 0). 전용 시험 없이 웹 조립 검사만 한 E197·E198·E201 시스템을 cpppj Session 시험(`DamageableBurningSessionTests` 등)으로 재대조 | 4-D | Tutorial1 첫 프레임 논리 화면 일부(지형·객체)가 cpppj `snapshot` 과 같음 |
| **5-B** 카메라·HUD·미니맵·커서 | 실제 카메라·가장자리 스크롤, `UberHud`·덱·미니맵·`FloatingSpGump`·커서 표시, `o/` RawManaBoltRegular(클라이언트 카메라 투영 선행) | 5-A | 화면 요소별 cpppj 대조 |
| **5-C** 미션 진입 명령·M2 판정 | `MissionBegin`·`StartDemo`·환영 창 명령, 영어 4:3 다음 16:9/16:10·한국어 | 5-B | **M2:** Tutorial1 진입 직후와 N프레임 뒤 논리 화면이 cpppj와 차이 0(L4a). `?map=` 개발 화면과 구분 |
| **6-P** 경로 부품 | 3.1절 목록 | 3-C(권장) | 3.1절 |
| **6** 입력·경제·건설 (WB06~08) | `UserInput`·선택/커서·QWASZX·ESC 메뉴·우클릭 메뉴, SP 저장/비용/회수/지갑·Deck/재충전/에너지, 사제 상태/이동/경로/수확/운반, 다리/건물 배치·빌더/생산·취소/실체화(client K·E·L 영역). E215 RawProductionDeck 의 재계산/Factory 상품 추가 시험 3건은 client RawCampaignProductionBindings 이식 때 대조 | 5-C·6-P | 같은 입력 단계표에서 cpppj와 같은 상태·화면(L3). **M3:** Tutorial1·2와 캠페인 1-1·1-2를 사용자가 브라우저에서 검증할 수 있음 |
| **7-P1~P3** 비행·총·의식 부품 | 3.1절 목록 | 3-C·6-P(7-P3 일부) | 3.1절 |
| **7** 전투·의식·AI (WB09~10·WB12) | 총/투사체/피해·벽/타워·유닛/파생 효과, 신전/제단·지식/의식/보상/승패 조건, 공통 전략 AI 전체 행동·탐색/전략·종료 수명(client D·F 영역) | 6·7-P | 현재 Player/기술/SP/명령/난수로 연결하고 cpppj 복원 범위를 타입별로 기록. 임시 방어 AI를 만들지 않음. 포획/희생·특정 적 공격·승리 재현과 공중3종 전용 검사는 1절 경계를 지킴 |
| **8** 미션 수명·안내·소리 (WB11·WB13·WB15) | 튜토리얼 게이트/명령/정지·현재 미션 Tell·도움말/지식·한국어 미션, Retry/Next/Abort/메뉴 복귀·실제 진행 저장, 데모 360초 반복/Auto-Demo(cpppj 946~948과 같은 조건), 위치 음향/음악 수명·Wind Noise/좌우 교환, 채팅·메시지(`o/` RawMissionGuide, client G·H·I 영역) | 7 | **M4:** 일반31+데모3의 진입·진행·저장 수명. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분. 스크립트 도움말 Back은 원본 명령 재등록에 연결 |
| **9** 표시 품질·배포 (WB16) | 원본 간격 유지 뒤 60/120 표시·성능/메모리, Chrome/Edge/Firefox·GUI Linux, 정적 배포/하위 경로·새로고침·캐시/자산 버전/저장 호환 | 8 | **M5:** 개발 서버 없이 같은 공개 자료로 실행. 확인한 브라우저/OS·DPI·소리/저장 환경만 기록. 배포 제품이 정해지면 README 등 공통 문서 일괄 갱신 |

**진행 기준:** M1 완료 → M2(순서 3 마감 뒤 4-A~5-C, Tutorial1의 실제 시작 상태와 표시) → M3(순서 6) → M4(순서 7~8, 범위 내 전체 싱글플레이) → M5(순서 9). M3/M4의 자연 완주·전체 동일성은 사용자 확인으로 남기며 AI 작업을 무기한 막는 완료 조건으로 두지 않는다. 사용자 제보는 해당 단계의 수정 작업으로 올린다.

### 3.3 cpppj 오라클 사용법

| 수단 (cpppj 기준선 실행 파일) | 얻는 것 | 창 |
| --- | --- | --- |
| `--run <자료> --window --mute-audio --frames N --set "k=v;…" --ui-script steps.tsv --ui-report report.tsv` (+ 환경 `NETSTORM_USER_DATA=<빈 폴더>`) | 프레임별 입력(click/right/key/esc/pointer 등)과 `snapshot`(팔레트를 적용한 논리 화면의 32비트 BMP)·`report`(메뉴/세션 관측 TSV). `tools/cpp-oracle.mjs`가 이 형태로 실행한다. `--no-audio`·`NETSTORM_CPP_NO_AUDIO`는 소리 장치가 없는 상태라 Options 소리 항목이 비활성으로 그려지므로 쓰지 않는다(WB02-H) | 뜸 |
| `--run <자료> --frames N --screenshot out.bmp` | N프레임 뒤 논리 화면 | 뜸 |
| `--mcp`: `netstorm_input`·`status`·`report`·`screenshot`·`wait_frames`·`record_start`/`record_stop` | 대화형 조작·관측·소리 포함 녹화(L4b 소리 시점·곡 선택) | 뜸 |
| `--dump-types`·`--inspect-fort`·`--inspect-mission`·`--dump-territories`·`--config-*`·`--inspect-*`·`--dump-font`·`--dump-gif` | 자료 해석 결과 | 없음 |
| `cpppj/tests/fixtures`(원본 x86 대조 자료) | L1 기대값 | — |

- **제한:** `--dump-world`는 저장된 Tutorial 미션만 받는 옛 `GameWorld` 보고서라 실제 Session 비교에 쓰지 않는다. 실제 Session 상태는 cpppj `session-dump`(3.5절 1번)로 볼 수 있으나 기준선 오라클 실행 파일에는 없다. 관측할 수 없는 범위는 단위 문서에 한계로 적는다.
- **같은 조건:** 같은 자료(`assets/game-data` 기준으로 cpppj 실행 폴더와 `webpj/www/data`의 해시를 대조), 같은 언어·화면 크기·설정(`--set`), 같은 시드와 프레임 단위 단계표, 같은 입력을 쓴다. 맞출 수 없는 조건(소리 장치 시각, 브라우저 시계 등)은 비교에서 빼고 기록한다.
- **실행 규칙:** 창이 뜨는 수집은 AGENTS.md의 실행 확인 대상이다(예외: HJOW-Athlon·VM-W11-Codex·vm-debian-codex). 수집은 해당 단위의 별도 단계에서만 하고, 결과 파일과 cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. `npm test`는 저장된 결과만 읽는다.
- **도구와 절차 (WB02-H):** 시나리오 묶음 `webpj/tests/fixtures/<묶음>-scenarios.json`에 단계표를 적고 `node webpj/tools/cpp-oracle.mjs --exe <기준선 EXE>`로 모은 뒤, `node webpj/tools/screen-compare.mjs --diff <폴더>`로 웹(무브라우저, 원본 비교 모드)과 비교한다. 차이는 고치거나 시나리오의 `expected`에 영역·픽셀 수·원인으로 적는다(영역 밖 0이어야 통과). 라벨 클릭은 cpppj `ControlPoint`처럼 스타일 명령을 뺀 일반 글자로 쓴다(`General Help - F1`). 기준선 EXE는 `cpppj/build`(다른 세션이 계속 다시 빌드)를 쓰지 않고 기준선 worktree에서 따로 빌드한다. 이 PC에는 `extracted/webpj-oracle/1f00a25a/`에 보존했다.
- **알려진 비교 차이:** 메인 메뉴 구름 배경은 원본(=cpppj) Kencloud 출력과 웹 GIF 단순 타일이 행별 1픽셀 어긋난다. 비교 도구는 이를 `background` 기준 장면으로 정밀 분류한다. Kencloud 부품(E118·E119)을 메뉴에 연결하는 5-A에서 0으로 바꾼다.
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

### 3.5 사용자 확인·요청이 필요한 것

1. **실제 Session 상태 비교 수단:** cpppj 는 2026-11-10 `--ui-script` 의 `session-dump` 단계(SID 풀 해시·사용 슬롯 원문·자산 해석·Kernel/Regular·Player 9·동맹 81·SP 비트·Graph·TotalMade·난수, 판번호 1 TSV)와 `--fixed-frame-ms N`(가상 시계)을 추가했다([덤프·고정 시각 문서](docs/exe/cpp-session-state-dump-20261109.md)). 웹 쪽 남은 일(4-D): `tools/cpp-oracle.mjs` 에 두 옵션을 넘기는 시나리오 형식, 같은 형식을 만드는 웹 덤프, 그리고 기준선 `1f00a25a` 오라클 실행 파일에는 이 기능이 없으므로 재기준선(3.6절) 여부 판단. 그 전까지는 `report`와 논리 화면 범위로 비교한다.
2. **한국어 미션 문구의 단일 원본:** cpppj 한국어 후속 자산(준비본 44개)이 제품에 반영되면 웹이 같은 파일을 `webpj/translations`로 가져와 쓸지(BOM·줄바꿈은 준비 도구에서 정규화), 웹 번역을 따로 둘지 정해야 한다. 기본값은 "cpppj 확정본과 통일"이다.
3. **오라클 수집의 실행 허용:** 예외 호스트가 아닌 PC에서 cpppj 창 실행이 필요하면 단위마다 확인을 받는다.
4. **바깥 링크의 실제 브라우저 확인:** WB03-AD의 새 탭/창 열림과 팝업 차단 여부는 실제 브라우저에서 확인하지 않았다(검사는 판정·앵커 생성까지).

### 3.6 cpppj 변경 재반영

- **기준선:** `1f00a25a`(제품985). 분류는 ① 웹 결과 영향 없음(구조 정리·진단·웹 제외 기능) ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫이다. cpppj fixture가 바뀌었으면 해당 웹 검사를 먼저 다시 돌린다.
- **기준선 이후의 변경:** 매 단위 착수 때 `node webpj/tools/port-map.mjs`의 `cpppj 변경` 줄을 본다(행 base 뒤 커밋을 단위별로 보고하고 안내를 붙인다). `exclude-*` 행은 ①로 기록만 하고, `port`/`adapt` 행은 ②/③을 판정해 ②는 `재반영`을 명시한 WB 단위로 반영한다. 판정을 마치면 그 행 base를 판정한 커밋으로 올리고 근거를 메모(①이라도)와 단위 문서에 남긴다. `fixture-manifest.mjs`도 같은 base 뒤 변경만 보고한다. 기준선 뒤 새로 생긴 파일은 `새 파일`로 보고되며 재기준선 때 분류한다.
- **판정 대기 (2026-11-10, HEAD `d4b82c30` 의 `port-map.mjs` 보고):** `cpppj 변경` 196건 — **②/③ 판정 필요(웹에 이식분 있음) 87단위**(`o/` 82·client 5), 이식 때 판정(아직 웹에 없음) 100단위(`o/` 37·client 62·zacket 1), ① 자동(제외 분류) 9단위. 새 파일은 253파일(145단위, 대응표 밖)이다. 2026-11-09의 54단위에서 늘었다(웹 이식이 진행되는 동안 CP 세션이 같은 파일에 `Actual*` 실제 연결·`shared*` 공유 전역·진단 복귀를 계속 더함). 87단위 처리는 3-E(`o/` 82·RawChatChannels)와 4-D·5(client UI 4)로 나눴다. 목록은 `node webpj/tools/port-map.mjs`의 `②/③ 판정 필요` 줄이다.
- **판정을 마치고 반영 시점만 남은 ③ (아래 표):** 해당 단위에서 반영하면 이 표에서 지우고 대응표 메모를 고친다. 판정 완료(①·②반영)된 9곳(WB02-G~WB04-E59 판정)과 그 근거는 [2026-11-09 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)과 [대응표 문서5절](docs/webpj-port-map-20261106.md)에 있고, base는 `PORT_MAP.tsv`에 기록돼 있다.

| cpppj 소스(웹 기준 → 기준선 사이 커밋) | 웹 모듈 | ③ 반영 시점과 내용 |
| --- | --- | --- |
| `client/TellSection` `6dd8d803` → `2ca39d0b`·`5a2cd047` | `ui/tell-parameters.js`·`game-data.js` | 대체 tell 적재가 `config.LoadFile` 경로 → **4-D 현재 미션 Tell 연결 때 반영**. `DisplayText` 교체는 cpppj 6차 표시 부호화라 웹은 ① |
| `o/RiftTypeTable`(+`RiftType.h`) `700475b5` → `fbc27346` | `assets/type-flags.js` 외 | 타입 레코드 `defaultFrame`(+0x128). 웹 type-table 정의에는 있으나 Factory용 레코드에 없음 → **4-A 타입 레코드·5-A Draw 연결 때** |
| `o/RawTerrainContext` `552656c0` → `3eec0830`·`1fe5fae7` | `assets/terrain-mask.js`·`terrain-fringe.js` | `Edition()`·`ArchipelagoMode()` 접근자 → **4-B 군도 지형 이식 때** |
| `o/SidPool.cpp/.h` `6035f810`/`b7065914` → `90ffe7e5`·`7b78cc20`·`435cfa1d` | `core/sid-pool.js` | (WB04-D3 판정 유지) 패치판 할당 진단 정상 복귀(`disallowSquidAllocation`·각 free 진단)·소진 시 boss의 현재 World 다리 회수(최대 50, `BindAllocation`)·물리 슬롯 friend. 정상 할당 결과는 같음. 웹은 소진/진단에서 오류로 멈춘다 → **4-D World 수명 때** |
| `9cb0547f`(일반 전투섬·중앙섬/군도): `client/UberGump.cpp` | `ui/main-menu.js` 등 partial | 로그인 중 "Creating Islands." 안내(일반 전투섬 생성 경로) → **4-C/4-D 로그인 이식 때 단독 도달 여부 확인**. base `9cb0547f` |
| 기준선/판정 뒤 ~`44e53bdc`: `client/ClientMain.cpp` | `core/main-loop.js`·`netstorm.js` | (2026-11-07 WB04-D5b) 메인 루프의 연결 감시(439ad8)·전투 자원 프레임(439b34)·재접속 주기 저장 단계, 기록 재생 전역·커서 기록기 식별자, 시간 보정 시계 바인딩 → **4-D 메인 루프 세션 연결 때 단독 도달·단계 순서 재확인**. 힙 진단 로그·Monitor AutoRestart 는 ①. base `44e53bdc` |
| `9cb0547f` 뒤 ~`11fbb988`: `client/UberGump.cpp` | `ui/main-menu.js` 등 partial | ①: LAN 대기실·재접속·Multiplayer 옵션/메뉴(웹 제외). ③: `ShowLoginProgress` 단독 안내(Initializing./Starting Mission...)·`PollBattleResources`(486bc0)·F7/Ctrl+F6 건설 기록 재생 입력 → **4-C/4-D 로그인·6 입력 때**. base `11fbb988` |
| `1f00a25a` 뒤 ~`3e9dc2c7`: `o/GameClock.cpp/.h` | `core/game-clock.js` | 시간 동기화 보정(0055b4ac) 바인딩과 `(ms−origin−보정)` 감산. 보정 0 이면 기준선과 같은 결과 → **4-C 시간 패킷(5번) 단독 도달 확인 때**. base `3e9dc2c7` |
| `1f00a25a` 뒤 `b8cadde1`: `o/RawFormPacket.cpp/.h` | `core/squid-vtables.js`(VT 칸만) | 삭제 패킷 writer 의 헤더 선기록·client SID 진단 정상 복귀 → **4-C 패킷 writer 이식 때**. VT 4종 칸은 ①. base `b8cadde1` |
| `9cb0547f` 뒤 ~`11fbb988`: `o/RawGameAuxState.h` | `core/game-aux-state.js` | 새 장기 전역 칸(StateNotifyType 82·Spell/Income·프로세스 TYPE·건설 헤더·미션 설정 7칸·Quick 건설 등)과 지정 칸 의미 갱신 → **각 소비자 이식 단위에서 보호 PE 초기값과 함께 추가**. 기존 웹 칸의 값/폭 변화 없음. base `11fbb988` |
| 기준선 뒤 `02b00b12`: `o/SquidProcess.cpp/.h` | `core/squid-process.js` | (2026-11-08 WB04-D5c) 발사/회전 애니메이션 프로세스 TYPE 두 칸(`fireAnimationType`·`rotateAnimationType`, 0x005410fc/00541100) → **애니메이션 소비자(`RawCampaignArcherBindings`) 이식 때 `GameAuxState`와 함께 추가**. 나머지 변경은 ①. base `02b00b12` |
| 기준선 뒤 `541624a9`·`417f22d2`·`7ba0130b`: `o/SquidDestroyLifecycle.cpp/.h` | `core/squid-destroy-lifecycle.js` | (2026-11-10 WB04-E213) 실제 PATCH(Native) 전용 분기(`destroy_.HasPatchDiagnostics()`): 현재 전역 getter(`selectedRaw`·`localOwner`·loading 깊이·fort 모드·silent/특수/대체/대형 표면 TYPE)·자산 pre `!isForm()` 0x771 진단·`rememberPlacement`(UI 기억점 원시 좌표 비트)·0040ea00 DWORD 감김 압축 제거·현재 TYPE 표 빌림·최초 로딩 비용 표 소비·선행 `ValidatePre/Post` 생략 → **4-D World 수명·삭제 UI 전역 연결 때** 같은 시기 `RawSquidDestroy`(+143/−2)·`SquidPostPop`(+35/−1)·`SquidReward`(+6/−3) 변경과 함께 반영. 웹은 기준선 typed 경로만 옮겼다. base `1f00a25a` |

## 4. 이관해도 남겨야 할 미해결 한계

아래는 아직 해소되지 않은 항목이다. 완료한 Tell/링크/도움말 번역/아이콘을 다시 미완료로 만들지 않는다.

| 항목 | 현재 한계와 처리 단계 |
| --- | --- |
| 실제 지형/소유권 | 미리보기의 중립색·가운데 카메라·고정 구름은 개발 정책. 저장 noIsland.owner를 최종 owner로 오해하지 않음. 신전 원소 투표/themeMode·도장/flood 순서·실제 카메라 시작/제한/스크롤은 4-B~5-B에서 연결 |
| 지형 난수/색표 | `buildTerrainFrames`와 절벽의 게임/CRT 난수는 실제 세션에서 공유. 절벽 제외 0x10/0x18의 선택, 최초 팔레트/ColorSort 시점·`useBanners` 기본값, 소유자→색 번호를 4-B·5-A에서 확정 |
| edgeFarm | spots·finder·MayPlace·지면 Retype 뒤 실제 월드에 구현. `RawFarmCluster`의 3×77 별도 시드/30000실패 한도 등 원본 순서를 유지. 타입121에 isle 색표가 있다고 추정하지 않음([근거6절](docs/webpj-terrain-extras-20261102.md)) |
| 타입 표 생성 메타 | 2026-11-06 WB04-D4: Factory는 `{constructorAddress, zOrder, maxHitPoints, flags1, flags2, footX/Y, name}` 레코드 배열을 받는다. 웹 타입 로더(`buildTypeRecords`)는 아직 `zorder`(cpppj `RiftTypeTable.cpp`의 23개 이름 상수+오프셋)와 `constructorAddress`(`typeConstructorAddress`)를 채우지 않는다. 실제 미션 TYPE 표를 Factory에 넘기는 4-A에서 cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata`와 같이 연결·검사한다 |
| 보상/SP 세션 연결 | 2026-11-07 WB04-D5b: `SquidReward`·`ScrambledSpStore`·`ContentFormLifetime`은 부품 대조까지다. SP 재시드 원천(004559d0 시각)·사제 추적 offset 소유자(0054d4e0)·실제 지갑(`bindPatchWallet`, brain+2d8)·수입 알림/원격 패킷/위치 소리 장치·Client 커서/boss/현재 너겟 getter·`SquidRewardState.bindGameAux`와 World 절반 HP 모드·같은 World TYPE/로딩 비용표 대여는 4-D·6에서 연결한다. 장부는 `SquidPostPopState`(WB04-E)와 같은 객체여야 한다 |
| 영역 회전 | 기존 실제 파일은 방향0. 회전2/3의 실제 저장 사례가 생기면 원본 결과와 추가 대조. 독립 기하 계산 통과를 실제 파일 관측으로 세지 않음 |
| 입력/커서 | 커서2~14·17·18은 쓰는 미션 입력과 함께 추가. 본문 끌기/캡처 중 Overlay의 원문 입력(`004d62b0` 계열) 계약은 미확정. 2026-11-07 WB02-H: 원본처럼 매 프레임 마지막 포인터 위치를 메인 메뉴에 다시 알리게 했다(정지 포인터 아래 펼침 메뉴 행 강조, cpppj 오라클 일치). 미션 입력(UserInput)도 같은 폴링 계약으로 이식한다 |
| 메인 메뉴 구름 배경 | 2026-11-07 WB02-H 오라클 대조: 원본/cpppj는 Kencloud 출력(`ClientKencloudGlobals::Draw`·`o/RawKencloud*`), 웹 메뉴는 GIF 단순 타일이라 일부 행이 가로 1픽셀 어긋난다(4:3에서 37,054픽셀, 원본 스크린샷도 cpppj 쪽). Kencloud 부품(E118·E119)을 메뉴에 연결하는 5-A에서 해소하고 `m1-scenarios.json`의 구름 차이 기록을 0으로 바꾼다 |
| Options/저장된 값 | Help0/Options−40은 측정값이며 일반 위치 규칙/와이드 메뉴 배치는 미확정. Wind Noise·Speaker Swap·Edge Scroll·Auto-Demo는 해당 시스템과 실제 연결 확인 필요. 시작 팁·설정→GameData 동기화/Done* 저장은 이미 구현 |
| 도움말 고급 표시 | `<info>` 머리글/능력치, `#`/`&` 상자 맞춤·색 치환은 미확인. 레이어0/그림자 없는 도움말 그림 경계를 유지한다. 스크롤 화살표는 웹의 검정이 원본과 맞다(cpppj 도 2026-11-10 원문대로 현재 팔레트 검정 005a4058 로 고침, [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md) 3.1절) |
| 에너지 기호 | `mana.8~11`은 현재 10.78 클러스터 8개 밖이라 0크기. 실제 원본 F1 모습 관측은 미확인. 원본 실행이 필요하면 AGENTS.md의 확인/예외 규칙 적용 |
| Tell/언어 | `<background>`는 이름만 보관(`nsisles` 자료/사용 근거 미확정). 현재 미션·`{@미션.키}`·브리핑/안내 `.korean`·스크립트 도움말 Back은 4-D·8에서 연결. 한국어 영어대체/긴 줄/고DPI도 해당 화면과 확인 |
| 메뉴 확장 | 두 열 메뉴는 범위 내 실제 소비자가 필요할 때 원본 계약으로 이식. `%{userSpec}`·User Made Campaigns는 제외이며 새 과제로 돌리지 않음 |
| 입력/오디오/환경 | 실제 스피커 청취·모든 브라우저/GUI Linux·고DPI/모니터 이동은 미확인. 일부 Edge/Chromium 기록을 전체 플랫폼 지원으로 쓰지 않음 |
| README webpj 상태 | README.md·README.en.md의 webpj 상태는 2026-11-06 일괄 갱신(WB04-D5a, 검사 429개) 기준에 머물러 있고 "바깥 링크 미구현" 같은 지난 서술이 남아 있다. README는 공통 문서이므로 다음 공통 문서 일괄 갱신 때 WB02-G 이후 상태로 고친다(README.md "개요" 단락은 수정 금지) |
| Credits AI 문구 | 2026-11-05 사용자 결정: 현행 문구(`OpenAI Codex, ChatGPT and Anthropic Claude Code`, 한국어 동일 취지)를 **유지하고 나중에 수정**한다. cpppj/dotnetpj 문구(Codex·ChatGPT만)와 다르며, 수정 지시가 오면 `www/assets/product.js`의 영어/한국어 섹션과 `tests/product.test.mjs`의 문구 검사를 함께 고친다. 한국어 행 이름 `NetStorm 11.01 패치 크레딧`은 확정 |

## 5. 검증과 완료 판정

| 단계 | 확인 방법과 책임 |
| --- | --- |
| L1 모듈 | 원본10.78 fixture(`cpppj/tests/fixtures`)와 이식한 cpppj 검사로 반환/전체 슬롯·전역·정수/float/double 비트·효과/난수 순서를 대조. Adler 일치는 체크섬 비교이며 바이트 직접 비교와 구분 |
| L2 시작 상태 | 같은 Fort/언어/옵션/시드와 같은 Load/Start 시점에서 cpppj 오라클과 비교. Session 덤프를 쓸 수 있으면 SID·타입·owner·좌표/HP/프레임·Player/동맹/SP·Graph·장부·난수, 그 전에는 `report`와 논리 화면 |
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
- [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md): 웹 이식 중 발견해 cpppj 에서 처리한 시험 결함·원본 차이(해소된 항목 포함)

## 7. 문서 정리 기록

- **기록 방식:** 단위마다 0.5절 5번의 칸만 고치고 단위별 기록 줄은 이 절에 쌓지 않는다(단위 문서와 커밋 메시지가 기록이다). 이 절에는 문서 정리·사용자 결정·작업 중단 같은 단위 밖 사건만 적는다.
- **2026-11-10, HJOW-Athlon, 사용자 요청(인계 문서 정리·M2 선행 작업 단계 분할, 문서만):** `bf156eea / 20261110-46 WB` 시점 원문 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)에 바이트 그대로(94,455바이트, SHA-256 `5d518ed6…27d652`) 보존했다. 순서3을 3-A(완료, E1~E242)와 3-B~3-E로 나누고, M2에 직접 필요하지 않은 경로·비행·총·의식 부품을 6-P·7-P1~P3로 이월했으며, 순서4·5를 4-A~4-D·5-A~5-C로 나눴다(1절 단계 분할 결정). 남은 `o/` 109단위를 모두 단계에 배정했다(3-B~3-D 18·4-A~4-C 61·5-A/5-B 5·6-P 4·7-P 20·8 1). 2.2절 단위 링크 색인은 영역별 번호 범위로 줄이고 링크는 보관본에 남겼다. 3.1절 완료 표시·3.4절과 4절의 해소 행·7절의 2026-11-09 상세 기록을 보관본으로 옮기고, 3.6절 판정 대기를 2026-11-10 보고(87단위)로 새로 적었다. 규모 수치는 `PORT_MAP.tsv`·기준선 `1f00a25a` 소스·`git diff --numstat` 로 계산했다. 코드·검사·자산·PORT_MAP 은 바꾸지 않았고 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.
- **2026-11-10, HJOW-Athlon, 사용자 요청(webpj 를 위한 cpppj 작업, CP 세션 일시 중지 중):** 웹 이식 중 "cpppj 몫"으로 남긴 항목을 cpppj 에서 처리했다(시험 결함 4건 수정·커서 폭 시험 확인, 원본 차이 2건 수정, `session-dump`·`--fixed-frame-ms` 추가). 웹 코드·검사·PORT_MAP 은 바꾸지 않았다. [cpppj 처리 기록](docs/exe/cpp-webpj-requests-20261109.md)
- **2026-11-09, HJOW-Athlon, 사용자 요청(인계 문서 정리):** 진행 상황·남은 작업 중심으로 처음 재구성했다. 상세는 [2026-11-10 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)의 7절에 있다.

**오래된 내용은 [2026-11-10 단계 분할 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261110-through-wb04-e242)·[2026-11-09 정리 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261109-through-wb04-e187)·[2026-11-06 재계획 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261106-through-wb04-d5a)·[WB04-A까지의 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a)에 보관했다. 그 문서의 “현재/다음/미구현”은 당시 기록이며, 이후 작업은 이 현행 문서를 기준으로 진행한다.**
