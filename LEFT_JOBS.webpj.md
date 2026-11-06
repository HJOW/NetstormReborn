# LEFT_JOBS.webpj — 현행 상태와 개발 계획

webpj의 **현재 구현·개발 계획(남은 작업 순서)·유효한 결정**을 정리한다. cpppj/공통은 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)가 기준이다. 단위별 과거 이력은 보관 문서와 각 `docs/webpj-*.md`를 따른다.

**2026-11-06 재계획(사용자 요청):** cpppj는 싱글플레이 범위(비멀티 UI·Edit·미션/내부 수명, 4·5·7차)의 구현·대조를 마치고 3차 LAN 멀티플레이 사전 구현으로 넘어갔다. 웹은 멀티플레이와 Edit를 제외하므로 웹이 옮길 cpppj 코드는 사실상 고정됐고, cpppj를 기준으로 webpj를 끝까지 완성할 수 있다. 이에 따라 계획을 다음과 같이 바꿨다.

1. **이식 기준 고정:** cpppj 제품985의 소스 `1f00a25a`를 기준선으로 삼는다. 단위마다 새 cpppj 커밋을 기준으로 삼던 방식은 기준선으로의 일괄 재반영 한 번(WB02-G)과 이후 변경 필터로 바꾼다(3.6절).
2. **파일 단위 이식:** 원본 함수를 하나씩 옮기던 방식을 cpppj 파일(클래스) 단위로 옮기고 해당 cpppj 검사를 함께 옮기는 방식으로 바꾼다. 대응표 `webpj/PORT_MAP.tsv`로 전체 범위와 진행도를 관리한다(3.0절).
3. **cpppj 오라클 비교:** 원본 x86 fixture 대조(L1)에 더해, 같은 자료·설정·입력에서 cpppj 실행 결과(논리 화면·관측 보고서)와 웹 결과를 비교하는 L2~L4 검증을 도입한다(3.3절).
4. **싱글플레이와 멀티플레이의 경계:** 원본 싱글플레이도 같은 프로세스 안의 내부 미션 전송·패킷·로그인 경로로 진행하므로 이 경로는 이식한다. LAN TCP·멀티플레이 UI·일반 전투·Outpost 등만 제외한다(1절).

이전 계획 원문은 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261106-through-wb04-d5a)에 그대로 있다.

**2026-11-05 사용자 결정(유효):** ① Credits AI 문구는 현행 유지 후 나중에 수정 ② 한국어 행 이름 `NetStorm 11.01 패치 크레딧` 확정 ③ 원본 자료의 바깥 링크는 **새 탭/창으로 연다**(3.1절 미구현 항목) ④ 개발 서버의 다른 PC 접속 제한 불필요 ⑤ cpppj 현재 버전을 참고해 구현하고 이후 변경을 따로 재반영한다 — 2026-11-06 재계획에서 기준선 고정으로 바꿨고, 재반영 원칙은 3.6절에 남겼다.

## 0. 현재 기준과 재개 절차

### 0.1 현재 값 (2026-11-06, WB04-D5a 완료·재계획)

| 항목 | 현행 기준 |
| --- | --- |
| 마지막 구현 커밋 | **WB04-D5a** `4d5b486a / 20261106-56 WB` Form 호스트(Depend/Gump/Content)와 부모들. 그 앞은 `7dd335e0`(D4 SquidFactory·생성자 표)·`410beda4`(D3 공통 Form 층)·`3ef1d18b`(D2 Unpop 처리기·삭제기)·`41cb8da9`(D1 공통 공간 해제)·`c68e673b`(C3 입력 큐·메인 루프)·`e993112b`(C2 게임 보조 전역)·`176ef6e7`(C1 난수·시계·Kernel)·`0563c07f`(B 공간 해시·탐색기)·`0fd343a1`(A SID 풀). 이번 재계획은 문서만 바꾼 WB 커밋이다 |
| 실행·플레이 | 클릭하여 시작→원본 자산 메인 메뉴·캠페인/데모 선택 목록·도움말/옵션·Credits 세 항목(11.01 Patch Credits 포함)과 개발용 지형 미리보기까지다. **실제 미션 플레이는 아직 불가**다. `MissionBegin`/`StartDemo`는 미션 월드에 연결되지 않았다 |
| 원본과의 동일성 | 메뉴·대화상자 일부는 원본 스크린샷과 대조했다. Fort/지형·그림·SID와 WB04 부품은 원본 fixture 또는 독립 계산과 대조했다. **cpppj 오라클 비교는 0건**이며, 같은 실제 미션의 L2/L3·전체 화면/음향·자연 완주는 미검증이다 |
| 마일스톤 | M1(시작→메인 메뉴) 완료. M2(Tutorial1의 실제 시작 상태와 정적 표시)는 지형/공통 출력/값형 로더와 월드 핵심 부품(WB04-A~D5a)까지 부분 완료이며, 이 부품들은 아직 앱 수명에 연결하지 않았다. 다음은 3.2절 순서1(WB02-G) |
| 이식 범위 | cpppj `o/`·`client/`는 `.cpp` 914개·헤더 포함 약 13만 7천 줄이다(2026-11-06 측정). 웹 대상·제외 분류는 아직 없다(WB02-G에서 `PORT_MAP.tsv`로 작성). 웹 소스는 `www/` JS 약 1만 6천 5백 줄(자료 제외), 검사 파일 43개다 |
| 마지막 전체 검사 | WB04-D5a의 Node v24.21.0, **429/429 통과·실패/건너뜀0**(신규 form-hosts 4·form-hosts-contract 19; WB04-D4 406), 약66.67초(같은 PC의 다른 node 프로세스 부하가 있던 측정이며 신규 검사 단독은 약2.5초) |
| 마지막 e2e | WB03-AC의 Chromium 초기 페이지 스모크 **1/1 통과**. 별도로 임시 Playwright 스크립트로 Credits→세 번째 행→Back/Cancel을 영어/한국어 실제 클릭 확인(Chromium 156.0.8078.4, 오류0). 미션/월드 검사가 아니다 |
| cpppj 기준선 | **`1f00a25a`(제품985·표시 11.01).** 2026-11-06 확인: 그 뒤 커밋(`c68942cf`·`9e5f1ba8`·`4b8063a8` 등)은 문서만 바꿨으며 `cpppj/src`·`cpppj/tests` 변경은 0이다. 공통 인계(LEFT_JOBS.md)는 다른 세션이 갱신하므로 cpppj 상태는 커밋과 소스로 판단한다 |
| 이식 추적 | `webpj/tests/fixtures/manifest.json` 62항목(original-x86 38·independent 7·screenshot 13·source 3·original-binary 1). original-x86 38개는 모두 `cpppj/tests/fixtures`를 직접 읽는다. 2026-11-06 도구 보고: cpppj 변경 9곳(3.6절 표)과 `depend-create-parent`의 webCommit 갱신 필요 1건 |
| 마지막 전체 정밀 디컴파일 | 공통 인계 기준 **HJOW-Athlon / 2026-11-03**. 웹은 cpppj 소스·fixture를 근거로 하므로 디컴파일 산출물을 직접 읽지 않는다 |
| 즉시 요구 | 11.01 표기/세 번째 Patch Credits(WB03-AC)·외부 접속 목록(WB02-E)·이식 추적 목록(WB02-F) 완료. **바깥 링크 새 탭/창 열기**는 결정됐고 미구현이다(3.1절) |

### 0.2 매 단위 착수/종료

1. AGENTS.md·이 문서·공통/dotnetpj 인계를 읽고 `git status`·`git fetch`를 확인한다. **같은 작업 트리에서 다른 세션이 cpppj·dotnetpj·공통 문서를 동시에 커밋할 수 있다(2026-11-06 실제 관측).** 공통 인계와 cpppj 파일은 고치지 않고, cpppj 상태는 커밋 기록과 소스로 판단한다.
2. 이번 단위가 옮길 cpppj 파일을 `PORT_MAP.tsv`에서 고르고, 기준선의 소스·그 파일을 대상으로 하는 cpppj 검사·fixture·단위 문서를 함께 읽는다. 기준선 뒤 그 파일이 바뀌었으면 3.6절로 판단한다. 디컴파일이 꼭 필요하면 마지막 수행 PC와 현재 PC를 먼저 비교하고 AGENTS.md의 재디컴파일 규칙을 따른다.
3. 원본/클론 게임(cpppj 오라클 수집 포함)·analyzeManager·별도 창 실행은 AGENTS.md의 확인/예외 규칙을 적용한다. 단위 검사나 정적 확인을 실제 실행으로 기록하지 않는다.
4. 3.0절 방식으로 구현한다. 필요한 공유 장치가 없으면 먼저 이식하고, 빈 성공 함수·별도 근사 월드로 우회하지 않는다. 멀티플레이 분기는 1절의 경계 규칙을 따른다.
5. 근거 커밋·연결 fixture/오라클(SHA/행 수)·검사와 최초 실패/교정·실제 연결/미연결·남은 경계·다음 시작점을 단위 문서에 남긴다. `PORT_MAP.tsv` 상태와 manifest(`node webpj/tools/fixture-manifest.mjs --write`)를 갱신하고 0.1절 값과 3절 체크를 고친다. 이번 단위 파일만 이름으로 지정해 stage하고, 커밋 직전에 `git log -1`로 번호가 겹치지 않는지 확인한 뒤 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. push하지 않는다.

## 1. 범위와 유지되는 결정

- **목표:** 원본10.78 싱글플레이를 HTML+JavaScript로 복원한다. 대상은 Early Missions6·Priest Training9·캠페인16의 **일반31경로와 데모3경로**다. 같은 자산을 쓰는 일반/데모도 별도로 취급한다([공통 인계1.2절](LEFT_JOBS.md)).
- **웹 범위와 멀티플레이 경계 (2026-11-06 정리):**
  - **이식 대상(싱글플레이에 필요):** 단독 미션·데모가 쓰는 Client/Session·World·State와, **내부 미션 전송(zlocal 양끝)·패킷 등록/길이/분배·미션 로그인·서버/클라이언트 미션 패킷·Zone 초기화**를 옮긴다. 원본 싱글플레이도 같은 프로세스 안에서 이 경로로 진행하므로, 이것을 빼면 미션이 돌지 않는다. 로그인 Ready·Player 번호·초기 SP 지정처럼 이름은 멀티플레이 같아도 단독 경로에서 실행되는 코드는 이식 대상이다.
  - **웹 제외:** LAN TCP 전송(`LocalTcpTransport`, 복합 전송의 TCP 원격끝, `CampaignTransportServices`의 TCP backend), Multiplayer 메뉴·LAN UI·참가자 목록·Commander, 일반 전투(State6·`saveBattle`/`!state.dat` 복구·Reconnect·서버 이전), 메타맵·Challenge·중앙섬·랭크, Outpost, 공식 서버·클라이언트 유효성 검사. 또 Edit·커스텀 맵 작성/가져오기/저장·TEST01/TEST02 플레이·User Made Campaigns(`offical6`/`%{userSpec}` 목록), Quit/Exit, 게임 옵션의 해상도/전체화면, 설치/CD 탐지·레지스트리, Win32/DirectDraw/GDI/WINMM·Direct3D 11/FSR·MCP. 메뉴에도 노출하지 않으며 Edit 전용 CHEAT·`userCheat` 우회도 제공하지 않는다. 공통 Fort/지형 코드가 Edit에도 쓰인다는 이유로 캠페인용 이식을 막지는 않는다.
  - **경계 처리:** cpppj 함수 안의 멀티플레이 분기는 원본의 단독 판정(`inSingle`·demo·server 조건)을 그대로 옮긴다. 단독에서 도달하지 않는 분기는 조용한 성공이나 no-op로 바꾸지 않고 명시적인 웹 제외 오류로 둔다(`MainLoop`의 웹 제외 단계 사유와 같은 방식). 직접 단독 관문이 없다고 해서 도달 불가로 단정하지 않으며, 근거를 단위 문서에 남긴다.
  - cpppj가 3차 작업에서 공유 파일(로그인 부모·Session·Player/SP setter·패킷 분배 등)을 바꾸면 단독 경로에 영향이 있는지 3.6절로 확인한다.
- **cpppj 기준 (2026-11-06 재정의):** 목표는 원본 10.78 싱글플레이 복원이며, 그 근거로 원본 기계어와 대조된 cpppj 기준선(`1f00a25a`)을 옮긴다. cpppj의 파일·클래스·함수 경계와 이름, 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 보존한다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 `DataView`/TypedArray로 같은 배치를 쓴다. cpppj의 미복원·부분 연결·명시 미연결·실패 경계는 임의로 채우지 않고 같은 지점의 명시 오류·진단으로 옮긴다. dotnetpj의 근사를 정답으로 복사하지 않는다. 기준선 이후의 cpppj 변경은 3.6절로만 반영하고 미커밋 변경은 쓰지 않는다. (2026-11-05의 "착수 시점 cpppj + 사후 재반영" 결정을 대체한다.)
- **바깥 링크 (2026-11-05 사용자 결정):** Tell/도움말 링크의 바깥 주소는 원본의 셸 실행 대신 **브라우저 새 탭/창으로 연다**. 구현 전까지는 현행(콘솔 안내)이며, 구현 조건은 [외부 접속 문서7절](docs/webpj-external-access-20261105.md)을 따른다. 개발 서버(`server.js`)는 다른 PC 접속을 제한하지 않는다(모든 인터페이스 수신 유지).
- **구조:** `index.html` 수정은 최소화하고 `netstorm.js`가 앱 수명을 맡는다. `.css` 파일 없이 JS로 스타일을 지정한다. Canvas의8비트 인덱스/팔레트 합성과 최종 표시를 분리한다. 코어는 DOM/Canvas/Web Audio에 의존하지 않는다. 기존 CommonJS 서버와 `www/package.json`의 ES 모듈 경계, `www/{assets,core,render,ui,audio,platform}`를 유지한다. C++/C# 서버나 WebAssembly를 필수 런타임으로 추가하지 않는다.
- **시작/수명:** 사용자 시작 입력 안에서 오디오를 활성화한다. 미션 재시작·메뉴 복귀·`destroy/init`의 타이머/선택/음악/이벤트 소유권을 구분한다. 숨겨진 탭은 진행을 정지하고 복귀 때 시계 기준을 다시 잡아 부재 시간을 따라잡지 않는다(웹 적응).
- **화면/시간:** 영어4:3부터 비교하며 게임 영역의4:3·16:9·16:10, 입력 역변환·픽셀 비율을 다룬다. 현재768높이/1024·1228·1366너비는 임시 화면 정책이다. `requestAnimationFrame`은 표시 기회이고 게임 틱이 아니다. 요소별 원본 간격을 먼저 맞추고60/120프레임은 후순위로 둔다. 가장자리 스크롤은 미션이 입력을 받으며 포인터가 게임 영역 안에 있을 때만 동작하고 이탈/포커스 상실 시 멈춘다.
- **자산/저장:** 기본 소스는 `assets/game-data`·`fonts`·`webpj/translations`이며 실행 자료는 `webpj/www/data`에 준비한다. 느슨한 파일→TAFF 조회·타입/프레임/팔레트 의미·manifest 출처/해시를 보존한다. 웹 실행에서 보호 원본이나 로컬 설치를 요구하지 않는다. 설정/진행은 버전이 있는 브라우저 저장소에 두고 저장 불가/손상 때도 계속할 수 있게 한다. 큰 저장소 도입은 실제 필요가 생길 때 판단한다.
- **언어:** 영어 원본 `.chfnt`, 한국어는 사용자 지시대로 `fonts/D2Coding.woff2`·`D2CodingBold.woff2`를 복사해 사용한다. TTC 추출로 되돌리지 않는다. 한국어 파일/섹션/주제가 없으면 영어로 대체한다. 번역 문구 검수는 **개발자가 발견하는 대로 요청**한다(2026-11-01). 메뉴/도움말 번역 완료를 미션 한국어 완료로 세지 않는다. 명칭은 AGENTS.md와 [공유 명칭](docs/korean-unit-names.md)을 따른다. 미션 브리핑·안내의 한국어는 cpppj 한국어 후속 확정본과 같은 원문을 쓰는 것을 기본값으로 한다(3.5절 2번).
- **사용자 확인 경계:** [공통 인계0.3·1.1·3절](LEFT_JOBS.md)에 맞춰 **모든 자연 완주·전체 동일성·실제 사제 포획/희생·특정 적 공격·승리 재현/자동 공략은 사용자 확인 또는 별도 요청 대상**이다. 구현·정적 대조·허용된 부품 검사는 계속하되 이 항목만 남으면 다음 단계로 간다. 공중3종(훨리기그·맨오워·더스트 데빌)은 정적/코드 이식만 하며 새 전용 생산/생성/우클릭/행동 관측·검사를 추가하지 않는다.
- **보호/분담:** AGENTS.md·보호 원본/CD를 수정하지 않는다. cpppj·dotnetpj·공통 인계와 다른 세션 변경은 이 작업에서 수정/커밋하지 않는다. README 등 공통 파일은 기존 사용자 결정대로 일괄 갱신 때 반영한다. 새 문서/코드는 UTF-8, 새 상수·함수·자료형·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다. **공통 인계 LEFT_JOBS.md와 cpppj 파일은 다른 세션이 갱신 중이므로 WB 단위에서 수정하지 않는다(2026-11-06 사용자 주의).** 같은 작업 트리의 다른 세션 커밋과 섞이지 않도록 stage할 파일은 이름으로 지정한다.

## 2. 완료 범위 색인

`[x]`는 해당 부품의 완료이며 실제 캠페인/전체 동일성 완료가 아니다. 상세 숫자/과거 PC/최초 실패는 아래 기록 또는 보관 문서를 따른다.

| 상태·단위 | 완료 범위와 현재 경계 | 근거 |
| --- | --- | --- |
| [x] WB00·WB01 | 계획·ES 모듈 진입·클릭 대기/실패 재시도·Canvas/기본 입력·앱 수명. 초기 빈 골격/미추적 파일 설명은 만료 | 과거 원문 보관 |
| [x] WB02 메뉴 기반·WB02-D | 자산 준비/manifest·TAFF/Config/GameFiles/GameData/Tell 조회·영어 글꼴. 미션 조회와 제품11.01 보완은 남음 | [데이터층](docs/webpj-data-layer-20261028.md) |
| [x] WB03-M1·D~L | 원본 메인 메뉴, Tell/Campaign/Demo/Credits 목록, Help/Options·설정, F1/링크/스크롤/커서, 시작 환영/팁, `Done*` 쓰기/저장. 미션 명령·Auto-Demo·현재 미션 문맥은 미연결 | [메인 메뉴](docs/webpj-main-menu-20261023.md)·[대화상자](docs/webpj-tell-dialog-20261028.md)·[목록](docs/webpj-campaign-lists-20261029.md)·[진행 저장](docs/webpj-progress-storage-20261030.md)·[시작 창](docs/webpj-startup-dialogs-20261030.md) |
| [x] WB03-I / 제외 결정 | Edit 버튼과 User Made Campaigns 행 제거. 캠페인 최상위6행이며 두 기능은 재구현하지 않음 | [캠페인 목록7절](docs/webpj-campaign-lists-20261029.md) |
| [x] WB03-N~S | D2Coding·xlat·메뉴/Tell/캠페인 목록/도움말 한국어와 영어 대체. `serverHelp` 미완성 틀·그림뿐인`map`과 미션 본문은 구분 | [글꼴/메뉴](docs/webpj-korean-20261031.md)·[Tell](docs/webpj-korean-tell-20261031.md)·[도움말 완료](docs/webpj-korean-help-types-20261101.md) |
| [x] WB03-T | 자산 타입116개의 프레임/그림 해석. `<info>`·`#`/`&`와 에너지 기호 관측은 남음 | [도움말 그림](docs/webpj-help-pictures-20261101.md) |
| [x] WB03-U~Y | `.fort`/188타입 플래그·영역 배치·본섬 마스크·지면/절벽·작은 받침·소유자 색표 계산.465맵 독립 대조는 실제 미션 적재 증거가 아님 | [Fort](docs/webpj-fort-reader-20261101.md)·[영역](docs/webpj-territory-layout-20261101.md)·[마스크](docs/webpj-terrain-mask-20261101.md)·[지면](docs/webpj-terrain-tiles-20261102.md)·[절벽/받침/색](docs/webpj-terrain-extras-20261102.md) |
| [x] WB03-Z·AA | CRT 정렬·64바이트 카메라·공통 월드 그림/그림자 출력과 개발용 중립 지형 화면. 실제 Draw/소유자·배경/카메라 정책은 미연결 | [출력](docs/webpj-world-render-20261102.md)·[지형 장면](docs/webpj-terrain-scene-20261102.md) |
| [x] WB03-AB | 값형 Fort 객체/내용물·Terr→Chaff 순회 로더. 필수 생성/Pop/finder/소유자 장치가 없어 실제 월드에는 미연결 | [객체 로더](docs/webpj-fort-object-load-20261102.md) |
| [x] WB04-A | 실제50바이트 SID 풀·할당/반납/재초기화·20개 삭제 기록. 원본10.78의1584전이 대조. `slot/bytes/deletions`는 시점 사본, `allocated`만 쓰기 대여이며 양수reset 뒤 재조회 | [SID 풀](docs/webpj-sid-pool-20261104.md) |
| [x] WB04-D5a | `depend-form-host.js`(감시 쌍 생성 004b1510·DependFormHost: 현재 Depend TYPE 두 번 읽기, TYPE 번호 후보·VT 클래스 판정, preDestroy 상대 삭제·release 통지), `gump-form-host.js`(소유 ID DWORD·부모 삭제 UI 통지·VT/반납 TYPE 판정), `content-form-host.js`(정렬 부착 004aec10·통지 뒤 현재 TYPE 사제 포획 꼬리·Robust/Linked/Damage 재부착·BindCreate), `raw-form-linked.js`(004ae080), `content-parent-notice.js`(+0x24 분배), `contained-create.js`(004afa70), contained 종단 `…AtTable` 재반영. 원본 감시 쌍 생성810·정렬 부착41·Form 연결120·Form 수명456행 대조, 계약19, 음성 대조18종 | [Form 호스트](docs/webpj-form-hosts-20261106.md) |
| [x] WB04-D4 | `squid-constructors.js`(10.78 생성자 주소 188칸·자산 파생 recipe 82개, cpppj `.inc`에서 생성)와 `squid-factory.js`: `createRawSquid`(004af530)·`takeRawSquid`(004af610)·TYPE HP(0049fce0)·`SquidFactory`(create/take/construct/constructCurrent/constructByAddress/retype/postCreate/postTake/firstPopFlags와 Form 진입 createContained/createGumpForm/attachForm/popForm/unpopForm). 사제 1/4 HP·사제/다이스/fakeSurface TYPE은 `bindGameAux`로 같은 GameAuxState를 빌림. 원본 생성자188·자산생성자567·Create부모294·Take부모360·초기화288·firstPop256·생성482·파생3300·Unpop/Take/반납1809 대조, 음성 대조16종. D2/D3 합성 준비를 실제 Factory로 교체. 웹 타입 로더의 zorder/생성자 주소 연결은 WB05 | [SquidFactory](docs/webpj-squid-factory-20261106.md) |
| [x] WB04-D3 | 공통 Form 층 `squid-forms.js`: 생성자 VT 단일 저장·Create(VT→TYPE)·Take(서버 SID, 사용 중 Form Unpop 뒤 VT→TYPE→void)·`attachCommonForm`(004ac800)·`popCommonForm`(004ac560)·Form Unpop(004ae140)과 `SquidForms` 기본/대여 장치. `GameAuxState`에 Form 종속 전역 3칸(0054107c 3·00541088 6·005427dc 0x02000039)과 호출마다 읽는 `squidFormGlobals`. 원본 생성자28·부착432·Pop1280·Unpop864·Create/Take/Unpop/Reset300행 대조, 음성 대조12종. Form 호스트는 Factory 뒤(D5), Factory Form 진입은 D4 | [공통 Form 층](docs/webpj-squid-forms-20261106.md) |
| [x] WB04-D2 | `SquidUnpop` 처리기(현재 TYPE 표·진단 연결, 체인 해제 뒤 현재 VT 표시 지원 확인)·`squid-vtables` 지원 자산 VT 83칸/Form VT 4종, 미니맵 dirty 큐 `RawMetadisplayDirty`(고유 등록·Refresh·공유 전체 갱신 소유자), 공통 contained 종단 `RawSquidContainedUnpop`(진단 복귀·링크 해제·부모 통지·UI 큐)과 VT 기준 링크 getter/setter, 공통 삭제기 `RawSquidDestroy`, 다리 삭제 훅 `RawBridgeLifecycle`. 원본 vtable 83행·미니맵 520행·Destroy 600행·Bridge 64행 대조와 계약 검사. `unpop-x86.tsv`는 Factory 필요로 D4, 다리 가상 삭제 분배는 WB07 | [Unpop 처리기·삭제기](docs/webpj-squid-destroy-20261105.md) |
| [x] WB04-D1 | 공통 공간 해제 부모 `runSquidUnpopParent`(004afe50: 진단 복귀·void·미니맵·spot 해제·건물 옆면·체인 제거·표시 갱신, 부분 변경 보존), `effectiveGenus`(004afd30), `RawBuildingSpace` 옆면 해제/다리 통지, SID 풀 `physical()` 물리 슬롯 대여. 원본 1300행(풀/해시/spot Adler·요청 순서)·Genus 3216행 대조 | [공통 공간 해제](docs/webpj-squid-unpop-20261105.md) |
| [x] WB04-C3 | 원본 40칸 입력 큐 `InputQueue`(넣기·명령 예약·꺼내기, 39개 한도·DEL 변환·뗌 거르기)와 메인 루프 순서 실행기 `MainLoop`(main-loop.md 패치판 순서, 갱신 목록 20개, 영상 갱신 정지 분기, 입력 종료 절차, 미연결 단계 보고, 웹 제외 단계 사유). cpppj 단위 검사 이식과 시계·큐·Kernel 통합 순서 검사 | [큐/메인 루프](docs/webpj-main-loop-20261105.md) |
| [x] WB04-C2 | 게임 보조 전역 `GameAuxState`(현재 TYPE 26칸·Form/예약 번호·모드·장부·기한 등 47필드+장식 4칸, 원본 폭 검사). 보호 원본 PE `.data`/BSS 초기값과 51칸 직접 대조, 웹 타입 표 이름과 26칸 대조. Regular 공통은 Form 생성 의존으로 WB04-D로 이동 | [세션 장기 상태2](docs/webpj-game-aux-state-20261105.md) |
| [x] WB04-C1 | 클라이언트 장기 난수 전역(`ClientRandomGlobals`: CRT 1·게임 난수 55·모드 등)·CRT/게임 난수·시드 Initialize/Reseed, 게임 시계(32비트 밀리초·중첩 정지·Reset·프레임 고정), Kernel(1..39999 현재 슬롯 순서). 시드96·시계 공개114 대조. 지형 `CrtRandom`을 core 구현으로 통합. 앱/세션 연결 전 | [세션 장기 상태1](docs/webpj-session-runtime-20261105.md) |
| [x] WB04-B | 원본10.78 `SquidHash`(4단계 WORD 머리·주소·단계 선택)와 `RawSquidFinder`(Begin/Next, buried 제외·OR 교차·미리 읽은 next·현재 슬롯 재조회)를 같은 SID 풀에 연결. 초기화4·주소807·Level605·Find584 대조. 실제 타입 발자국·spot 등록·Fort 로더 연결은 남음 | [공간 해시/탐색기](docs/webpj-squid-finder-20261105.md) |
| [x] WB02-F | 이식 추적 목록 `webpj/tests/fixtures/manifest.json`(35항목: original-x86 12·independent 7·screenshot 13·source 3), 계산/검사 도구 `tools/fixture-manifest.mjs`, 형식·해시·적용 범위 검사3개. 해시는 텍스트 CRLF→LF 정규화 | [이식 추적](docs/webpj-fixture-trace-20261105.md) |
| [x] WB02-E | 웹 런타임 외부 접속 목록: 요청은 `./data/` 같은 출처 자산뿐, 바깥 링크11종은 콘솔 안내만, UserNotUpdated·Technical Help·서버 진단·`$Zsend`/`$Input` 미연결. 소스 정적 감사·manifest/적재 요청·링크·미연결 기능 검사5개 | [외부 접속](docs/webpj-external-access-20261105.md) |
| [x] WB03-AC | 제품 표기 `v11.01`(설정 version/MAJOR/MINOR)·`Patch1101` 등록 객체·메인 Credits 세 번째 행과 Patch Credits 창(영어/한국어, Back→목록·Cancel→메뉴). 원본 대조는 `GameData.build({product:false})`로 10.78·두 행 유지. AI 문구/한국어 문구는 개발자 검수 대상 | [제품11.01](docs/webpj-product-1101-20261105.md) |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식 (2026-11-06 재계획)

- **단위 크기:** 한 단위는 cpppj 파일 하나 또는 함께 움직이는 묶음(같은 클래스의 `.cpp/.h/.inc`와 생성 자료)과, 그 파일을 대상으로 하는 cpppj 검사로 이룬다. 함수 하나씩 나누지 않는다. 큰 파일은 공개 API 경계에서 나누고 남은 부분을 단위 문서와 PORT_MAP 비고에 적는다.
- **대응표 `webpj/PORT_MAP.tsv`:** 열은 `cpp`(저장소 기준 cpppj 경로)·`class`(분류)·`web`(웹 모듈 경로, 여럿이면 `;`)·`base`(이식·대조를 마친 cpppj 커밋)·`status`·`note`다. 분류는 `port`(이식 대상), `adapt`(창·입력·소리·파일·시계·캡처처럼 웹 대체가 필요한 플랫폼 경계), `exclude-mp`(1절의 LAN/멀티플레이 제외 목록), `exclude-edit`(Edit·UberEditor·Editor 계열), `exclude-host`(DirectDraw·Direct3D 11/FSR·MCP·CD 탐지·레지스트리·설치·Win32 진단)다. 상태는 `todo` → `port`(이식, L1/cpppj 검사 통과) → `session`(앱 수명 연결) → `oracle`(cpppj 오라클 비교 통과) 순서로 올린다. dotnetpj의 [PORT_MAP](dotnetpj/PORT_MAP.md) 형식을 참고하되 별도 파일로 둔다. 0.1절에 분류별 개수와 상태별 진행도를 기록한다.
- **검사 이식:** cpppj `tests/`의 대상 검사를 같은 이름의 Node 검사로 옮긴다. 원본 x86 fixture는 지금처럼 `cpppj/tests/fixtures`를 직접 읽고 복사하지 않는다. 웹 적응·제외 경계를 확인하는 웹 전용 검사는 따로 표시한다.
- **생성 자료:** 생성자·vtable·타입·패킷 길이처럼 큰 표는 cpppj 소스에서 생성 스크립트로 만들고 `--check`로 재생성 결과가 같은지 검사한다(WB04-D4 `squid-constructors.js` 방식).
- **수직 진행:** 범위를 넓히기 전에 Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 통과시킨다. 그다음 Early Missions·캠페인 1-1/1-2, 34경로 순으로 넓히며, 각 단계에 필요한 파일만 PORT_MAP에서 골라 옮긴다.

### 3.1 선행 보완 — 제품 요구와 이식 추적

- [x] **11.01/새 Credits (WB03-AC, 2026-11-05):** `www/assets/product.js`·GameData `product` 옵션·Tell 창 본문 적용으로 완료. 목록 세 번째 `Netstorm 11.01 Patch Credits`, 창 제목 `NetStorm 11.01 Patch Credits`, 한국어 `NetStorm 11.01 패치 크레딧`. 원본 자료/fixture는 그대로이며 원본 대조는 `product:false`([문서](docs/webpj-product-1101-20261105.md)).
- [x] **외부 접속 문서 (WB02-E, 2026-11-05):** [웹 외부 접속 목록](docs/webpj-external-access-20261105.md)과 `tests/external-access.test.mjs`로 완료. `UserNotUpdated` 생략, 검증 성공값 합성 없음. 이후 사용자 결정: 바깥 링크는 새 탭/창으로 열고(아래 미구현 항목), 개발 서버 접속 제한은 하지 않는다.
- [x] **이식 추적 (WB02-F, 2026-11-05):** `webpj/tests/fixtures/manifest.json` 형식 v1 확정과 기존 35항목 등록, `tools/fixture-manifest.mjs`(계산/검사), `tests/fixture-manifest.test.mjs`. 새 원본 대조 자료는 같은 단위에서 등록한다([문서](docs/webpj-fixture-trace-20261105.md)). dotnetpj 동기화 도구는 쓰지 않는다.
- [ ] **바깥 링크 새 탭/창 열기 (2026-11-05 사용자 결정, 미구현):** `Netstorm._followLink`의 external 훅과 `_openHelp`의 `onExternal`을 콘솔 안내에서 새 탭/창 열기로 바꾼다. 열 대상(http/https·mailto)과 열지 않을 대상(`file:`·치환 실패 주소), 사용자 입력 안에서의 동기 호출, `noopener`, `tests/external-access.test.mjs` 소스 감사의 허용 경로 갱신은 [외부 접속 문서7절](docs/webpj-external-access-20261105.md)을 따른다. 월드 의존성이 없어 WB04-C 전후 어느 때나 독립 단위로 진행할 수 있다.

### 3.2 실행 순서

| 순서·상태 | 단위 | 할 일 / 선행 관계 | 완료 기준 |
| --- | --- | --- | --- |
| 1 · [ ] | **WB02-G 이식 대응표·기준선 재반영 (다음)** | `webpj/tools/port-map.mjs`를 만들어 기준선 `1f00a25a`의 cpppj `o/`·`client/`·`zacket/` 소스 목록(각 `CMakeLists.txt`)에서 `PORT_MAP.tsv`를 만들고 3.0절 분류를 매긴다. 기존 웹 모듈(WB02~WB04-D5a)의 대응과 상태를 채운다. 3.6절 표 9곳을 기준선과 비교해 판정·반영하고 `depend-create-parent`의 webCommit을 `--write`로 갱신한다. 기준선 뒤 cpppj 변경을 보고하는 기능을 넣는다 | 미분류 0, 기존 웹 모듈 대응 기록, 9곳 판정(②는 같은 단위 또는 별도 재반영 단위로 반영), 분류별 개수를 0.1절에 기록 |
| 2 · [ ] | **WB02-H 무브라우저 실행·cpppj 오라클 기반** | ① `tools/headless.mjs`: 같은 자료로 core+render를 Node에서 실행해 8비트 논리 화면(cpppj `snapshot`과 같은 팔레트 BMP)과 관측 보고서를 낸다. DOM에 의존하는 모듈(render 1·ui 3·assets 1·audio·platform)은 표시 경계만 대체한다. ② `tools/cpp-oracle.mjs`: cpppj `--ui-script`로 같은 단계표를 실행해 BMP·보고서·명령줄·cpppj 커밋·EXE SHA-256·자료 해시를 `webpj/tests/fixtures/cpppj/`에 저장하고 manifest에 `kind: cpppj-oracle`로 등록한다. ③ 화면/보고서 비교 도구. 첫 대상은 이미 구현한 M1 화면(메인 메뉴·Campaign·Help·Options·Credits, 영어 4:3) | M1 화면의 cpppj 대조 결과(차이 0 또는 원인 기록). 웹 검사는 저장된 오라클만 읽고 cpppj를 실행하지 않는다 |
| 3 · [ ] | **WB04-D5b·D5c·E 월드 핵심 부품 마감** | D5b `SquidFormDispatch`(현재 VT 클래스 분배)·`ContentFormLifetime`(Content pre/post 삭제·경제·커서 취소)과 의존 `SquidReward`, `contentlifetime-x86.tsv`(10.78 198행). D5c `SquidProcessHost`·Process/BaseProcess 부착·Regular 공통(`AddRegular/AddSharedRegular/FindEvent`, `regular-current-*-x86.tsv`)과 Depend 생성의 Process 직렬화 부착 접두(`process-x86.tsv`·`processform-x86.tsv`·`process/baseprocess-attach-current-local-x86.tsv`). E 공간 spots/해시 등록·Graph/점유·Pop/PostPop·소유자/지역/장부·HP/프레임·내용물 생성과 소유자 마감. 착수 때 D5a 항목 4개의 webCommit을 갱신한다 | 해당 cpppj 검사와 원본 fixture 통과, PORT_MAP `port`. 효과 뒤 현재 TYPE/VT/슬롯·진단 복귀·부분 쓰기 순서 보존 |
| 4 · [ ] | **WB05-A/B 월드 조립과 미션 시작** | `RawSurfaceWorld`/`RawWorldCore`·`RawCampaignSession`과 1절의 내부 미션 전송·패킷·미션 로그인 경로, `RawFortWorldLoad`·지형/섬 Builder 계열, Player/동맹/기술/SP/Deck/AI 초기화, Start/Guide·State 전환을 cpppj 부모의 호출 순서로 조립한다. 기존 판독/영역/지형(WB03-U~AB)과 WB04 부품을 같은 월드에 연결하고, 웹 타입 로더의 `zorder`/생성자 주소를 Factory에 넘긴다. 첫 대상은 Tutorial1 | Tutorial1에서 cpppj와 같은 Load/Start 관측(보고서·첫 프레임 논리 화면). Session 덤프가 없으면 3.5절 1번이 정해질 때까지 보고서 범위로 기록한다. 임시 Player/AI/색 번호로 시작 성공을 만들지 않는다 |
| 5 · [ ] | **WB03-M2/WB05-C 월드 표시·카메라·HUD** | `Renderer`·`DrawSort`·타입별 Draw·시각 시스템·`KenCloud`·팔레트 전환, 실제 카메라·가장자리 스크롤, `UberHud`·덱·미니맵·`FloatingSpGump`·커서, `MissionBegin`·`StartDemo`·환영 창 명령 | **M2:** Tutorial1 진입 직후와 N프레임 뒤 논리 화면이 cpppj와 차이 0(L4a). 영어 4:3 다음 16:9/16:10·한국어. `?map=` 개발 화면과 구분 |
| 6 · [ ] | **WB06~08 입력·경제·건설** | `UserInput`·선택/커서·QWASZX·ESC 메뉴·우클릭 메뉴, SP 저장/비용/회수/지갑·Deck/재충전/에너지, 사제 상태/이동/경로/수확/운반, 다리/건물 배치·빌더/생산·취소/실체화 | 같은 입력 단계표에서 cpppj와 같은 상태·화면(L3). **M3:** Tutorial1·2와 캠페인 1-1·1-2를 사용자가 브라우저에서 검증할 수 있음 |
| 7 · [ ] | **WB09~10·WB12 전투·의식·AI** | 총/투사체/피해·벽/타워·유닛/파생 효과, 신전/제단·지식/의식/보상/승패 조건, 공통 전략 AI 전체 행동·탐색/전략·종료 수명 | 현재 Player/기술/SP/명령/난수로 연결하고 cpppj 복원 범위를 타입별로 기록. 임시 방어 AI를 만들지 않음. 포획/희생·특정 적 공격·승리 재현과 공중3종 전용 검사는 1절 경계를 지킴 |
| 8 · [ ] | **WB11·WB13·WB15 미션 수명·안내·소리** | 튜토리얼 게이트/명령/정지·현재 미션 Tell·도움말/지식·한국어 미션, Retry/Next/Abort/메뉴 복귀·실제 진행 저장, 데모 360초 반복/Auto-Demo(cpppj 946~948과 같은 조건), 위치 음향/음악 수명·Wind Noise/좌우 교환 | **M4:** 일반31+데모3의 진입·진행·저장 수명. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분. 스크립트 도움말 Back은 원본 명령 재등록에 연결 |
| 9 · [ ] | **WB16 표시 품질·배포** | 원본 간격 유지 뒤 60/120 표시·성능/메모리, Chrome/Edge/Firefox·GUI Linux, 정적 배포/하위 경로·새로고침·캐시/자산 버전/저장 호환 | **M5:** 개발 서버 없이 같은 공개 자료로 실행. 확인한 브라우저/OS·DPI·소리/저장 환경만 기록. 배포 제품이 정해지면 README 등 공통 문서 일괄 갱신 |

순서 1·2는 월드 의존성이 없어 순서 3과 병행할 수 있지만, 순서 4(WB05) 착수 전에 끝낸다. 순서 3~5는 큰 덩어리로 한 번에 완료 처리하지 않고 cpppj 파일 단위로 나누어 기록한다.

**진행 기준:** M1 완료 → M2(순서 4~5, Tutorial1의 실제 시작 상태와 표시) → M3(순서 6) → M4(순서 7~8, 범위 내 전체 싱글플레이) → M5(순서 9). M3/M4의 자연 완주·전체 동일성은 사용자 확인으로 남기며 AI 작업을 무기한 막는 완료 조건으로 두지 않는다. 사용자 제보는 해당 단계의 수정 작업으로 올린다.

### 3.3 cpppj 오라클 사용법

| 수단 (cpppj 기준선 실행 파일) | 얻는 것 | 창 |
| --- | --- | --- |
| `--run <자료> --window --no-audio --set "k=v;…" --ui-script steps.tsv --ui-report report.tsv` | 프레임별 입력(click/right/key/esc/pointer 등)과 `snapshot`(논리 8비트 화면 BMP)·`report`(메뉴/세션 관측 TSV) | 뜸 |
| `--run <자료> --frames N --screenshot out.bmp` | N프레임 뒤 논리 화면 | 뜸 |
| `--mcp`: `netstorm_input`·`status`·`report`·`screenshot`·`wait_frames`·`record_start`/`record_stop` | 대화형 조작·관측·소리 포함 녹화(L4b 소리 시점·곡 선택) | 뜸 |
| `--dump-types`·`--inspect-fort`·`--inspect-mission`·`--dump-territories`·`--config-*`·`--inspect-*`·`--dump-font`·`--dump-gif` | 자료 해석 결과 | 없음 |
| `cpppj/tests/fixtures`(원본 x86 대조 자료 980개) | L1 기대값 | — |

- **제한:** `--dump-world`는 저장된 Tutorial 미션만 받는 옛 `GameWorld` 보고서라 실제 Session 비교에 쓰지 않는다. 실제 Session 상태 전체를 내보내는 명령이 없으므로(3.5절 1번) `report`로 관측할 수 없는 범위는 단위 문서에 한계로 적는다.
- **같은 조건:** 같은 자료(`assets/game-data` 기준으로 cpppj 실행 폴더와 `webpj/www/data`의 해시를 대조), 같은 언어·화면 크기·설정(`--set`), 같은 시드와 프레임 단위 단계표, 같은 입력을 쓴다. 맞출 수 없는 조건(소리 장치 시각, 브라우저 시계 등)은 비교에서 빼고 기록한다.
- **실행 규칙:** 창이 뜨는 수집은 AGENTS.md의 실행 확인 대상이다(예외: HJOW-Athlon·VM-W11-Codex·vm-debian-codex). 수집은 해당 단위의 별도 단계에서만 하고, 결과 파일과 cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. `npm test`는 저장된 결과만 읽는다.
- **의미:** 오라클과 같다는 것은 "cpppj와 같다"는 뜻이다. 원본과의 동일성은 cpppj가 원본과 대조한 범위만큼만 보증되므로 L1 원본 fixture 대조를 대체하지 않고 함께 쓴다. cpppj가 미검증·사용자 확인으로 둔 동작(자연 완주·포획/희생·특정 적 공격·승리)은 웹도 같은 경계로 둔다.

### 3.4 cpppj 잔여와 webpj의 관계 (2026-11-06, 제품985·인계986 기준)

| cpppj 상태 | webpj 처리 |
| --- | --- |
| 비멀티 UI(929)·Edit(943)·미션 수명(956)·내부 수명/FILE(980)·와이드 덱(981) 구현·대조 마감 | 웹 이식의 기준 범위다. 이미 완료한 웹 키/reader/UI는 다시 복원 과제로 세지 않고 나머지만 옮긴다. Edit 관련은 제외 |
| 3차 LAN 멀티플레이 사전 구현(982~985, 다음 987 원격 Fort) | 웹 제외. 공유 파일(로그인 부모·Session·Player/SP setter·패킷 분배)이 바뀌면 단독 경로 영향을 3.6절로 확인 |
| 6차 한국어 후속(미적용 준비본: 공식 미션 43개 `d/*.korean`·`config.korean` 등 자산 44개, WB 번역 11개를 입력으로 사용) | 미션 문구 한국어는 cpppj가 제품에 반영한 확정본과 통일한다(3.5절 2번). 반영 전에는 영어 대체 |
| 8차 Edit 시나리오 UI(미적용 준비본) | 웹 제외 |
| 4·5·7차(MCP·와이드 해상도·사용자 홈 저장·AA/FSR) | MCP·AA/FSR·Win32 저장 경로는 웹 제외(웹은 브라우저 저장소). 화면비는 웹의 4:3/16:9/16:10 정책을 따른다 |
| checking 잔여 8함수(MP 분류 포함)·전체 동일성/자연 완주 등 사용자 확인 | 웹도 같은 경계. 아직 없는 효과를 일반 성공으로 채우지 않음 |
| 공중 3종(훨리기그·맨오워·더스트 데빌)은 정적 분석·코드 작업만 | 웹도 정적/코드 이식만 하고 전용 생산·행동 검사를 추가하지 않음 |

### 3.5 사용자 확인·요청이 필요한 것

1. **cpppj 실제 Session 상태 덤프:** L2(시작 상태) 비교에는 SID·TYPE·HP·좌표·소유자·Graph·Player·SP·난수 상태를 내보내는 cpppj 명령이 필요하다. cpppj 쪽 CP 작업이므로 웹 단위에서 만들지 않는다(dotnetpj R4도 같은 요청을 남겼다). 추가 여부와 시점이 정해질 때까지 `report`와 논리 화면 범위로 비교한다.
2. **한국어 미션 문구의 단일 원본:** cpppj 한국어 후속 자산(준비본 44개)이 제품에 반영되면 웹이 같은 파일을 `webpj/translations`로 가져와 쓸지(BOM·줄바꿈은 준비 도구에서 정규화), 웹 번역을 따로 둘지 정해야 한다. 기본값은 "cpppj 확정본과 통일"이다.
3. **오라클 수집의 실행 허용:** 예외 호스트가 아닌 PC에서 cpppj 창 실행이 필요하면 단위마다 확인을 받는다.

### 3.6 cpppj 변경 재반영

- **기준선:** `1f00a25a`(제품985). 웹 모듈의 cpppj 근거(`cpppjBasis`·PORT_MAP `base`)가 기준선보다 오래되면 WB02-G에서 일괄 판정해 기준선으로 올린다. 분류는 ① 웹 결과 영향 없음(구조 정리·진단·웹 제외 기능) ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫이다. cpppj fixture가 바뀌었으면 해당 웹 검사를 먼저 다시 돌린다.
- **기준선 이후의 변경:** 매 단위 착수 때 `port-map.mjs`(만들기 전에는 `fixture-manifest.mjs`)의 보고를 본다. `exclude-*` 파일만 바뀌었으면 ①로 기록만 하고, `port`/`adapt` 파일이 바뀌었으면 ②/③을 판정해 ②는 `재반영`을 명시한 WB 단위로 반영한다. 판정 근거는 ①이라도 남긴다.
- **아래 표:** 2026-11-05~06 도구가 보고한 9곳이다. WB02-G에서 기준선과 비교해 다시 판정한다.

| 상태 | cpppj 소스(웹 기준 → 변경 커밋) | 웹 모듈 | 1차 분류(2026-11-05, diff 크기·제목 기준, 미검토) |
| --- | --- | --- | --- |
| [ ] | `o/Config.cpp` `0ded2f07` → `2ca39d0b` | `assets/config.js` | ① 가능성: LoadBytes 분리·`@파일` 적재 훅. 의미 동일 여부 확인 필요 |
| [ ] | `o/ConfigInterface.cpp` `2ede4027` → `8d2f81a1`·`1ba89851`·`2ca39d0b` | `assets/game-data.js` | ③ 가능성: 설정 Load/FileSpec 수명·Startup 파일명 순서. WB05 현재 미션 Tell 연결 때 검토 |
| [ ] | `client/TellSection.cpp` `6dd8d803` → `2ca39d0b` | `assets/game-data.js`·`ui/tell-parameters.js` | ③ 가능성: fallback tell 적재가 `config.LoadFile`로 변경. WB05 현재 미션 Tell 연결 때 반영 |
| [ ] | `o/RiftTypeTable.cpp` `700475b5` → `fbc27346` | `assets/type-flags.js` | ② 또는 ③: 타입 레코드에 기본 외형 프레임(+128) 보관. WB03-M2/WB05 Draw 연결 때 반영 |
| [ ] | `o/CanonDecoder.cpp` `5bdb5bed` → `167866d9` | `assets/territory.js` | ① 가능성: 현재 프레임 진단·커서 배치 소비자. 영역 해석 결과 영향 확인 필요 |
| [ ] | `o/RawTerrainContext.cpp` `552656c0` → `3eec0830`·`1fe5fae7` | `assets/terrain-mask.js`·`terrain-fringe.js` | ③: 판본·군도 모드 접근자. 군도 지형 이식 때 반영 |
| [ ] | `o/RawIslandPostPop.cpp` `deaf73f8` → `0c06fa66` | `assets/island-supports.js` | ③: 섬/표면 현재 후처리·진단 복귀(64+/29−). WB04-E Pop/PostPop 이식 때 반영 |
| [ ] | `o/SidPool.cpp/.h` `6035f810`/`b7065914` → `90ffe7e5`·`7b78cc20`·`435cfa1d` | `core/sid-pool.js` | ③(2026-11-06 WB04-D3 확인): 패치판 할당의 진단 정상 복귀(`disallowSquidAllocation`·각 free 진단)와 소진 시 boss의 현재 World 다리 회수(최대 50, `BindAllocation`), 물리 슬롯 friend 추가. 정상 할당 결과는 같음(formtake 300행 통과). 웹은 소진/진단에서 오류로 멈춘다. 현재 World 삭제 대여가 필요하므로 D4 Factory·WB05 World 수명 때 반영 |
| [x] | `o/RawSquidContainedUnpop.cpp/.h` `d678fcad` → `f83db66c` | `core/squid-contained-unpop.js` | ②→반영(2026-11-06 WB04-D5a): 정렬 Content 부착용 `commonContainedPreviousAtTable`/`SetPreviousAtTable`(저장한 VT로 배치 선택)을 추가하고 정렬 부착 41행·Form 수명 456행으로 확인. 기존 getter/setter 의미는 같음 |

표의 커밋은 2026-11-05 `0563c07f` 시점이며 마지막 두 행(SidPool·contained 종단)은 2026-11-06 `8db4af6f` 시점에 추가했다. 이후 cpppj 커밋으로 대상이 늘 수 있으므로 재반영 착수 때 도구 보고로 다시 만든다.

## 4. 이관해도 남겨야 할 미해결 한계

아래는 과거 “다음”에서 아직 해소되지 않은 항목이다. 완료한 Tell/링크/도움말 번역/아이콘을 다시 미완료로 만들지 않는다.

| 항목 | 현재 한계와 처리 단계 |
| --- | --- |
| 실제 지형/소유권 | 미리보기의 중립색·가운데 카메라·고정 구름은 개발 정책. 저장 noIsland.owner를 최종owner로 오해하지 않음. 신전 원소 투표/themeMode·도장/flood 순서·실제 카메라 시작/제한/스크롤은 WB04-E~WB05(3.2절 순서 3~5)에서 연결 |
| 지형 난수/색표 | `buildTerrainFrames`와 절벽의 게임/CRT 난수는 실제 세션에서 공유. 절벽 제외0x10/0x18의 선택, 최초 팔레트/ColorSort 시점·`useBanners` 기본값, 소유자→색 번호를 WB05(3.2절 순서 4~5)에서 확정 |
| edgeFarm | spots·finder·MayPlace·지면 Retype 뒤 실제 월드에 구현. `RawFarmCluster`의3×77 별도 시드/30000실패 한도 등 원본 순서를 유지. 타입121에 isle 색표가 있다고 추정하지 않음([근거6절](docs/webpj-terrain-extras-20261102.md)) |
| 타입 표 생성 메타 | 2026-11-06 WB04-D4: Factory는 `{constructorAddress, zOrder, maxHitPoints, flags1, flags2, footX/Y, name}` 레코드 배열을 받는다. 웹 타입 로더(`buildTypeRecords`)는 아직 `zorder`(cpppj `RiftTypeTable.cpp`의 23개 이름 상수+오프셋)와 `constructorAddress`(`typeConstructorAddress`)를 채우지 않는다. 실제 미션 TYPE 표를 Factory에 넘기는 WB05-A(3.2절 순서 4, Fort Load)에서 cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata`와 같이 연결·검사한다 |
| 영역 회전 | 기존 실제 파일은 방향0. 회전2/3의 실제 저장 사례가 생기면 원본 결과와 추가 대조. 독립 기하 계산 통과를 실제 파일 관측으로 세지 않음 |
| 입력/커서 | 커서2~14·17·18은 쓰는 미션 입력과 함께 추가. 본문 끌기/캡처 중 Overlay의 원문 입력(`004d62b0` 계열) 계약은 미확정 |
| Options/저장된 값 | Help0/Options−40은 측정값이며 일반 위치 규칙/와이드 메뉴 배치는 미확정. Wind Noise·Speaker Swap·Edge Scroll·Auto-Demo는 해당 시스템과 실제 연결 확인 필요. 시작 팁·설정→GameData 동기화/Done* 저장은 이미 구현 |
| 도움말 고급 표시 | `<info>` 머리글/능력치, `#`/`&` 상자 맞춤·색 치환, 원본 삼각형 색 해석 확인. 웹은 원본 스크린샷 기준 검정 화살표를 사용. 레이어0/그림자 없는 도움말 그림 경계 유지 |
| 에너지 기호 | `mana.8~11`은 현재10.78 클러스터8개 밖이라0크기. 실제 원본 F1 모습 관측은 미확인. 원본 실행이 필요하면 AGENTS.md의 확인/예외 규칙 적용 |
| Tell/언어 | `<background>`는 이름만 보관(`nsisles` 자료/사용 근거 미확정). 현재 미션·`{@미션.키}`·브리핑/안내 `.korean`·스크립트 도움말 Back은 WB05·WB11/WB13(3.2절 순서 4·8)에서 연결. 한국어 영어대체/긴 줄/고DPI도 해당 화면과 확인 |
| 메뉴 확장 | 두 열 메뉴는 범위 내 실제 소비자가 필요할 때 원본 계약으로 이식. `%{userSpec}`·User Made Campaigns는 제외이며 새 과제로 돌리지 않음 |
| 입력/오디오/환경 | 실제 스피커 청취·모든 브라우저/GUI Linux·고DPI/모니터 이동은 미확인. 일부 Edge/Chromium 기록을 전체 플랫폼 지원으로 쓰지 않음 |
| [x] README 일괄 갱신 | **2026-11-06 완료(CM 커밋):** README.md·README.en.md의 webpj 상태를 WB04-D5a(검사 429개) 기준으로 갱신했다(WB03-P~AC·WB02-E/F·WB04-A~D5a 반영). 이후 단위의 README 반영은 다음 공통 문서 일괄 갱신 때 한다 |
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
# Node 내장 전체 검사. 부품 작업 중에는 관련 tests/*.test.mjs만 집중 실행할 수 있다.
npm --prefix webpj test
# 이식 추적 목록 검사(해시 비교·cpppj 근거 소스 변경 보고). 자료/모듈을 바꾼 단위는 --write 로 계산 칸을 갱신한다.
node webpj/tools/fixture-manifest.mjs
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
- 추가할 도구(WB02-G/H): `tools/port-map.mjs`(대응표 생성·기준선 이후 cpppj 변경 보고), `tools/headless.mjs`(무브라우저 실행·논리 화면/보고서), `tools/cpp-oracle.mjs`(cpppj 오라클 수집). 만들면 위 명령 목록에 추가한다.

## 7. 이번 정리와 과거 이관

- **2026-11-06, HJOW-Athlon, 사용자 요청(재계획, 문서만):** cpppj가 싱글플레이 범위를 마감하고 멀티플레이로 넘어간 것을 반영해 기준선 고정·파일 단위 이식·cpppj 오라클·싱글/멀티 경계를 정했다(머리말·1절·3.0~3.6절). 실행 순서를 WB02-G/H 선행으로 다시 세우고 0.1·0.2·5·6절을 갱신했다. 2절 완료 색인과 4절 미해결 한계는 유지했으며, 4절의 옛 순서 번호만 WB 영역 이름으로 바꿨다. 이전 원문은 보관본에 바이트 그대로 옮겼다. 코드·검사·자산 변경, npm/브라우저/게임 실행, 디컴파일은 하지 않았다. 같은 작업 트리에서 다른 세션이 cpppj 인계를 갱신하고 있어 LEFT_JOBS.md 등 공통·cpppj 파일은 수정하지 않았다.
- 그 전 정리(2026-11-04 원문 이관·계획 재작성, 2026-11-05 사용자 결정 반영)의 기록은 보관본에 있다.

**오래된 내용은 [2026-11-06 재계획 전 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261106-through-wb04-d5a)과 [WB04-A까지의 원문](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a)에 보관했다. 그 문서의 “현재/다음/미구현”은 당시 기록이며, 이후 작업은 이 현행 문서를 기준으로 진행한다.**
