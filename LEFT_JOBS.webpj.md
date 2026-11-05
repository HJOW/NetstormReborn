# LEFT_JOBS.webpj — 현행 상태와 남은 작업

webpj의 **현재 구현·남은 작업 순서·유효한 결정**을 정리한다. cpppj/공통은 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)가 기준이다. 단위별 과거 이력은 보관 문서와 각 `docs/webpj-*.md`를 따른다.

**2026-11-04 정리:** WB04-A까지의 상세 이력과 대체된 초기 계획을 이관했다. 이후 작업은 이 문서의 3절 순서를 따른다.
**2026-11-05:** 3.1절 선행 보완 세 항목(WB03-AC 11.01 표기·세 번째 Patch Credits, WB02-E 외부 접속 목록, WB02-F 이식 추적 목록)과 3.2절 순서1 WB04-B(공간 해시·일반 사각형 탐색기), 순서2 WB04-C 전체(C1 난수·게임 시계·Kernel, C2 현재 TYPE·모드·장부 전역, C3 입력/예약 큐·메인 루프 순서)를 완료했다. 3.2절 순서3 WB04-D는 하위 단위 D1(공통 공간 해제·spot genus)을 완료했고 다음은 D2다.
**2026-11-05 사용자 결정(문서만 반영, 코드 미변경):** ① Credits AI 문구는 현행 유지 후 나중에 수정 ② 한국어 행 이름 `NetStorm 11.01 패치 크레딧` 확정 ③ 원본 자료의 바깥 링크는 **새 탭/창으로 연다**(3.1절 미구현 항목) ④ 개발 서버의 다른 PC 접속 제한 불필요 ⑤ cpppj는 진행 중이므로 **현재 버전을 참고해 구현하고, 나중에 cpppj 변경 사항을 다시 반영하는 재반영 과정을 따로 진행**한다(1절 결정·3.5절).

## 0. 현재 기준과 재개 절차

### 0.1 현재 값 (2026-11-05, WB04-D1 완료)

| 항목 | 현행 기준 |
| --- | --- |
| 마지막 구현 커밋 | **WB04-D1** 공통 공간 해제·spot genus(`20261105` WB 커밋, 해시는 `git log -- webpj/www/core/squid-unpop.js`). 그 앞은 `c68e673b / 20261105-54 WB` 입력 큐·메인 루프, `e993112b / 20261105-53 WB` 게임 보조 전역, `176ef6e7 / 20261105-51 WB` 난수·시계·Kernel, `0563c07f / 20261105-47 WB` 공간 해시·탐색기 |
| 실행·플레이 | 클릭하여 시작→원본 자산 메인 메뉴·캠페인/데모 선택 목록·도움말/옵션·Credits 세 항목(11.01 Patch Credits 포함)과 개발용 지형 미리보기까지다. **실제 미션 플레이는 아직 불가**다. `MissionBegin`/`StartDemo`는 미션 월드에 연결되지 않았다 |
| 원본과의 동일성 | 메뉴·대화상자 일부는 원본 스크린샷과 대조했다. Fort/지형·그림·SID 부품은 원본 fixture 또는 독립 계산과 대조했다. **같은 실제 미션의 L2/L3·전체 화면/음향·자연 완주는 미검증**이다 |
| 완료/미완료 | M1(시작→메인 메뉴) 완료. M2(같은 실제 월드의 정적 화면)는 지형/공통 출력/값형 로더까지 부분 완료. WB04는 SID 풀(A)·공간 해시/일반 탐색기(B)·세션 장기 상태 C(C1 난수·게임 시계·Kernel, C2 현재 TYPE·모드·장부 전역, C3 입력/예약 큐·메인 루프 순서) 완료, D는 D1(공통 공간 해제·spot genus·건물 옆면 해제)까지 완료이며 D2~D4(Unpop 처리기·공통 삭제기·Form·Factory/Regular)와 E(Pop/소유자 마감)는 남는다. C/D의 장치들은 아직 앱 수명에 연결하지 않았다(첫 미션 소비자와 함께) |
| 마지막 전체 검사 | WB04-D1의 Node v24.21.0, **323/323 통과·실패/건너뜀0**(신규 squid-unpop5; WB04-C3 318), 약31.57초 |
| 마지막 e2e | WB03-AC의 Chromium 초기 페이지 스모크 **1/1 통과**. 별도로 임시 Playwright 스크립트로 Credits→세 번째 행→Back/Cancel을 영어/한국어 실제 클릭 확인(Chromium 156.0.8078.4, 오류0). 미션/월드 검사가 아니다 |
| cpppj 검토 기준 | 공통 인계의 **861 / 11.01**, 마지막 구현 `0c06fa66`. cpppj 문서의 일시 정지 기록을 webpj 중단 지시로 해석하지 않는다. 다음 착수 때 최신 커밋/인계를 다시 확인한다 |
| 이식 기준의 의미 | WB04-A가 검토한 트리는 `4fa7e89c`; **cpppj 전체를 그 커밋까지 반영했다는 뜻이 아니다.** 파일별 근거/반영 범위는 단위 문서에 있다 |
| 마지막 전체 정밀 디컴파일 | 공통 인계 기준 **HJOW-Athlon / 2026-11-03**. 이번 정리 PC도 HJOW-Athlon이며 디컴파일 산출물은 읽지 않았다 |
| 즉시 요구 | 11.01 표기/세 번째 Patch Credits(WB03-AC)·외부 접속 목록(WB02-E)·이식 추적 목록(WB02-F) 완료. **바깥 링크 새 탭/창 열기**는 결정됨·미구현(3.1절). 이식 추적 도구가 보고한 cpppj 변경 7곳은 3.5절 재반영 대상이다 |

### 0.2 매 단위 착수/종료

1. AGENTS.md·공통/dotnetpj/이 문서, `git status`·`git fetch`와 해당 cpppj 파일의 마지막 변경을 확인한다. 다른 세션의 미커밋 코드는 이식 기준으로 사용하지 않는다. 이번 단위가 쓰는 cpppj 소스가 3.5절 재반영 대상이면 그 변경을 함께 반영할지 판단해 기록한다.
2. cpppj는 **커밋된 복원 소스·원본 fixture·단위 문서·실제 바인딩**을 함께 읽는다. 디컴파일이 꼭 필요하면 마지막 수행 PC와 현재 PC를 먼저 비교하고 AGENTS.md의 재디컴파일 규칙을 따른다.
3. 원본/클론 게임·analyzeManager·별도 창 실행은 AGENTS.md의 확인/예외 규칙을 적용한다. 단위 검사나 정적 문서 확인을 실제 게임 실행으로 기록하지 않는다.
4. 현재 단계에 필요한 작은 구현 단위와 같은 입력/기대값을 먼저 고른다. 필요한 공유 장치가 없으면 그것을 선행 이식하고 빈 성공 함수·별도 근사 월드로 우회하지 않는다.
5. 근거 파일/커밋·연결 fixture/SHA/행 수·검사/실패와 교정·실제 연결/미연결·남은 경계·다음 시작점을 기록한다. 새 원본 대조 자료는 `webpj/tests/fixtures/manifest.json`에 등록하고 `node webpj/tools/fixture-manifest.mjs --write`로 계산 칸을 채운다(2026-11-05 WB02-F). 완료 체크 뒤 이번 파일만 `YYYYMMDD-NN WB 요약`으로 로컬 커밋하며 push하지 않는다.

## 1. 범위와 유지되는 결정

- **목표:** 원본10.78 싱글플레이를 HTML+JavaScript로 복원한다. 대상은 Early Missions6·Priest Training9·캠페인16의 **일반31경로와 데모3경로**다. 같은 자산을 쓰는 일반/데모도 별도로 취급한다([공통 인계1.2절](LEFT_JOBS.md)).
- **웹 제외:** Edit·커스텀 맵 작성/가져오기/저장·TEST01/TEST02 플레이·User Made Campaigns(`offical6`/`%{userSpec}` 목록), 멀티플레이/LAN/Outpost·공식 서버, Quit/Exit, 게임 옵션의 해상도/전체화면, 설치/CD 탐지·레지스트리다. 메뉴에도 노출하지 않으며 Edit 전용 CHEAT·`userCheat` 우회도 제공하지 않는다. 공통 Fort/지형 코드가 Edit에도 쓰인다는 이유로 캠페인용 이식을 막지는 않는다.
- **원본 우선:** cpppj가 복원/검증/연결한 범위부터 옮긴다. cpppj의 미복원/부분 연결/실패를 임의로 채우지 않고 해당 경계를 기록한다. dotnetpj의 근사를 정답으로 복사하지 않는다.
- **진행 중인 cpppj 기준 (2026-11-05 사용자 결정):** cpppj는 계속 변경되므로 각 단위는 **착수 시점의 커밋된 cpppj 현재 버전**을 참고해 구현하고, 그 근거 커밋을 단위 문서와 [이식 추적 목록](webpj/tests/fixtures/manifest.json)의 `cpppjBasis`에 남긴다. 이후 cpppj 변경은 단위 작업 중에 쫓아가지 않고 **3.5절의 재반영 과정**에서 따로 반영한다. cpppj의 미커밋 변경은 기준으로 쓰지 않는다.
- **바깥 링크 (2026-11-05 사용자 결정):** Tell/도움말 링크의 바깥 주소는 원본의 셸 실행 대신 **브라우저 새 탭/창으로 연다**. 구현 전까지는 현행(콘솔 안내)이며, 구현 조건은 [외부 접속 문서7절](docs/webpj-external-access-20261105.md)을 따른다. 개발 서버(`server.js`)는 다른 PC 접속을 제한하지 않는다(모든 인터페이스 수신 유지).
- **구조:** `index.html` 수정은 최소화하고 `netstorm.js`가 앱 수명을 맡는다. `.css` 파일 없이 JS로 스타일을 지정한다. Canvas의8비트 인덱스/팔레트 합성과 최종 표시를 분리한다. 코어는 DOM/Canvas/Web Audio에 의존하지 않는다. 기존 CommonJS 서버와 `www/package.json`의 ES 모듈 경계, `www/{assets,core,render,ui,audio,platform}`를 유지한다. C++/C# 서버나 WebAssembly를 필수 런타임으로 추가하지 않는다.
- **시작/수명:** 사용자 시작 입력 안에서 오디오를 활성화한다. 미션 재시작·메뉴 복귀·`destroy/init`의 타이머/선택/음악/이벤트 소유권을 구분한다. 숨겨진 탭은 진행을 정지하고 복귀 때 시계 기준을 다시 잡아 부재 시간을 따라잡지 않는다(웹 적응).
- **화면/시간:** 영어4:3부터 비교하며 게임 영역의4:3·16:9·16:10, 입력 역변환·픽셀 비율을 다룬다. 현재768높이/1024·1228·1366너비는 임시 화면 정책이다. `requestAnimationFrame`은 표시 기회이고 게임 틱이 아니다. 요소별 원본 간격을 먼저 맞추고60/120프레임은 후순위로 둔다. 가장자리 스크롤은 미션이 입력을 받으며 포인터가 게임 영역 안에 있을 때만 동작하고 이탈/포커스 상실 시 멈춘다.
- **자산/저장:** 기본 소스는 `assets/game-data`·`fonts`·`webpj/translations`이며 실행 자료는 `webpj/www/data`에 준비한다. 느슨한 파일→TAFF 조회·타입/프레임/팔레트 의미·manifest 출처/해시를 보존한다. 웹 실행에서 보호 원본이나 로컬 설치를 요구하지 않는다. 설정/진행은 버전이 있는 브라우저 저장소에 두고 저장 불가/손상 때도 계속할 수 있게 한다. 큰 저장소 도입은 실제 필요가 생길 때 판단한다.
- **언어:** 영어 원본 `.chfnt`, 한국어는 사용자 지시대로 `fonts/D2Coding.woff2`·`D2CodingBold.woff2`를 복사해 사용한다. TTC 추출로 되돌리지 않는다. 한국어 파일/섹션/주제가 없으면 영어로 대체한다. 번역 문구 검수는 **개발자가 발견하는 대로 요청**한다(2026-11-01). 메뉴/도움말 번역 완료를 미션 한국어 완료로 세지 않는다. 명칭은 AGENTS.md와 [공유 명칭](docs/korean-unit-names.md)을 따른다.
- **사용자 확인 경계:** [공통 인계0.3·1.1·3절](LEFT_JOBS.md)에 맞춰 **모든 자연 완주·전체 동일성·실제 사제 포획/희생·특정 적 공격·승리 재현/자동 공략은 사용자 확인 또는 별도 요청 대상**이다. 구현·정적 대조·허용된 부품 검사는 계속하되 이 항목만 남으면 다음 단계로 간다. 공중3종(훨리기그·맨오워·더스트 데빌)은 정적/코드 이식만 하며 새 전용 생산/생성/우클릭/행동 관측·검사를 추가하지 않는다.
- **보호/분담:** AGENTS.md·보호 원본/CD를 수정하지 않는다. cpppj·dotnetpj·공통 인계와 다른 세션 변경은 이 작업에서 수정/커밋하지 않는다. README 등 공통 파일은 기존 사용자 결정대로 일괄 갱신 때 반영한다. 새 문서/코드는 UTF-8, 새 상수·함수·자료형·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다.

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
| [x] WB04-D1 | 공통 공간 해제 부모 `runSquidUnpopParent`(004afe50: 진단 복귀·void·미니맵·spot 해제·건물 옆면·체인 제거·표시 갱신, 부분 변경 보존), `effectiveGenus`(004afd30), `RawBuildingSpace` 옆면 해제/다리 통지, SID 풀 `physical()` 물리 슬롯 대여. 원본 1300행(풀/해시/spot Adler·요청 순서)·Genus 3216행 대조 | [공통 공간 해제](docs/webpj-squid-unpop-20261105.md) |
| [x] WB04-C3 | 원본 40칸 입력 큐 `InputQueue`(넣기·명령 예약·꺼내기, 39개 한도·DEL 변환·뗌 거르기)와 메인 루프 순서 실행기 `MainLoop`(main-loop.md 패치판 순서, 갱신 목록 20개, 영상 갱신 정지 분기, 입력 종료 절차, 미연결 단계 보고, 웹 제외 단계 사유). cpppj 단위 검사 이식과 시계·큐·Kernel 통합 순서 검사 | [큐/메인 루프](docs/webpj-main-loop-20261105.md) |
| [x] WB04-C2 | 게임 보조 전역 `GameAuxState`(현재 TYPE 26칸·Form/예약 번호·모드·장부·기한 등 47필드+장식 4칸, 원본 폭 검사). 보호 원본 PE `.data`/BSS 초기값과 51칸 직접 대조, 웹 타입 표 이름과 26칸 대조. Regular 공통은 Form 생성 의존으로 WB04-D로 이동 | [세션 장기 상태2](docs/webpj-game-aux-state-20261105.md) |
| [x] WB04-C1 | 클라이언트 장기 난수 전역(`ClientRandomGlobals`: CRT 1·게임 난수 55·모드 등)·CRT/게임 난수·시드 Initialize/Reseed, 게임 시계(32비트 밀리초·중첩 정지·Reset·프레임 고정), Kernel(1..39999 현재 슬롯 순서). 시드96·시계 공개114 대조. 지형 `CrtRandom`을 core 구현으로 통합. 앱/세션 연결 전 | [세션 장기 상태1](docs/webpj-session-runtime-20261105.md) |
| [x] WB04-B | 원본10.78 `SquidHash`(4단계 WORD 머리·주소·단계 선택)와 `RawSquidFinder`(Begin/Next, buried 제외·OR 교차·미리 읽은 next·현재 슬롯 재조회)를 같은 SID 풀에 연결. 초기화4·주소807·Level605·Find584 대조. 실제 타입 발자국·spot 등록·Fort 로더 연결은 남음 | [공간 해시/탐색기](docs/webpj-squid-finder-20261105.md) |
| [x] WB02-F | 이식 추적 목록 `webpj/tests/fixtures/manifest.json`(35항목: original-x86 12·independent 7·screenshot 13·source 3), 계산/검사 도구 `tools/fixture-manifest.mjs`, 형식·해시·적용 범위 검사3개. 해시는 텍스트 CRLF→LF 정규화 | [이식 추적](docs/webpj-fixture-trace-20261105.md) |
| [x] WB02-E | 웹 런타임 외부 접속 목록: 요청은 `./data/` 같은 출처 자산뿐, 바깥 링크11종은 콘솔 안내만, UserNotUpdated·Technical Help·서버 진단·`$Zsend`/`$Input` 미연결. 소스 정적 감사·manifest/적재 요청·링크·미연결 기능 검사5개 | [외부 접속](docs/webpj-external-access-20261105.md) |
| [x] WB03-AC | 제품 표기 `v11.01`(설정 version/MAJOR/MINOR)·`Patch1101` 등록 객체·메인 Credits 세 번째 행과 Patch Credits 창(영어/한국어, Back→목록·Cancel→메뉴). 원본 대조는 `GameData.build({product:false})`로 10.78·두 행 유지. AI 문구/한국어 문구는 개발자 검수 대상 | [제품11.01](docs/webpj-product-1101-20261105.md) |

## 3. 남은 작업의 실행 순서

아래는 **선행 관계에 따른 순서**다. 기존 WB번호는 영역 식별자로 유지하며 WB14(Edit)는 제외다. 소리·화면·한국어·실패 수명은 각 규칙에 필요한 만큼 함께 연결하고 마지막 단계로 전부 미루지 않는다. cpppj에서 필요한 몸체가 미완료라면 의존 항목만 대기하고 다른 확정된 부품을 진행한다.

### 3.1 선행 보완 — 제품 요구와 이식 추적

- [x] **11.01/새 Credits (WB03-AC, 2026-11-05):** `www/assets/product.js`·GameData `product` 옵션·Tell 창 본문 적용으로 완료. 목록 세 번째 `Netstorm 11.01 Patch Credits`, 창 제목 `NetStorm 11.01 Patch Credits`, 한국어 `NetStorm 11.01 패치 크레딧`. 원본 자료/fixture는 그대로이며 원본 대조는 `product:false`([문서](docs/webpj-product-1101-20261105.md)).
- [x] **외부 접속 문서 (WB02-E, 2026-11-05):** [웹 외부 접속 목록](docs/webpj-external-access-20261105.md)과 `tests/external-access.test.mjs`로 완료. `UserNotUpdated` 생략, 검증 성공값 합성 없음. 이후 사용자 결정: 바깥 링크는 새 탭/창으로 열고(아래 미구현 항목), 개발 서버 접속 제한은 하지 않는다.
- [x] **이식 추적 (WB02-F, 2026-11-05):** `webpj/tests/fixtures/manifest.json` 형식 v1 확정과 기존 35항목 등록, `tools/fixture-manifest.mjs`(계산/검사), `tests/fixture-manifest.test.mjs`. 새 원본 대조 자료는 같은 단위에서 등록한다([문서](docs/webpj-fixture-trace-20261105.md)). dotnetpj 동기화 도구는 쓰지 않는다.
- [ ] **바깥 링크 새 탭/창 열기 (2026-11-05 사용자 결정, 미구현):** `Netstorm._followLink`의 external 훅과 `_openHelp`의 `onExternal`을 콘솔 안내에서 새 탭/창 열기로 바꾼다. 열 대상(http/https·mailto)과 열지 않을 대상(`file:`·치환 실패 주소), 사용자 입력 안에서의 동기 호출, `noopener`, `tests/external-access.test.mjs` 소스 감사의 허용 경로 갱신은 [외부 접속 문서7절](docs/webpj-external-access-20261105.md)을 따른다. 월드 의존성이 없어 WB04-C 전후 어느 때나 독립 단위로 진행할 수 있다.

앞의 세 항목은 월드 의존성이 없는 즉시 요구/기반 보완이며 2026-11-05에 끝났다. 이어 3.2절 순서1(WB04-B)도 같은 날 끝났다. 네 번째 항목은 같은 날 사용자 결정으로 추가된 미구현 작업이다.

### 3.2 WB04 최소 월드 → M2 → 미션 진입

| 순서·상태 | 할 일 / 선행 관계 | 완료 기준·근거 |
| --- | --- | --- |
| 1 · [x] WB04-B (2026-11-05) | **`SquidHash`→`RawSquidFinder`**를 WB04-A의 같은 풀에 연결. 실제 버킷/next·단계/y/x 순서·buried/교차식·현재 타입/좌표 재조회 보존 | 초기화4·주소807·ObjectLevel605·10.78 Find584 일치([문서](docs/webpj-squid-finder-20261105.md)). Genus(spot)·Pre/Post(다리 삭제 훅)는 D/E. 타입 발자국은 `{footX,footY}` 배열 계약이며 실제 타입 표 연결은 D/E |
| 2 · [x] WB04-C (2026-11-05) | 세션이 장기 소유하는 TYPE/모드·난수/CRT 난수·게임 시계·Kernel/Process/Regular·입력/예약 큐를 필요한 순서로 이식. 표시용 `FrameClock`과 분리. **하위 단위:** [x] C1 난수·시드·클라이언트 난수 전역·게임 시계·Kernel(2026-11-05, [문서](docs/webpj-session-runtime-20261105.md)) / [x] C2 현재 TYPE·모드·장부 전역(2026-11-05, [문서](docs/webpj-game-aux-state-20261105.md); Regular 공통은 Form/Process 생성 의존이라 WB04-D로 이동) / [x] C3 `InputQueue`·명령 예약과 메인 루프 순서(2026-11-05, [문서](docs/webpj-main-loop-20261105.md); 대화상자 명령 수명은 WB03 UI가 처리하므로 재복원하지 않음, 창 프로시저/폴링은 미션 입력과 함께) | 같은 시각/시드/입력에서 상태·소비·등록/취소 순서가 일치. 이후 생성/Pop이 요구하는 공유 전역을 여기서 빌림. 앱 수명 연결(`ClientRandomGlobals` 생성·GameClock 입력)은 첫 실제 소비자와 함께 |
| 3 · [ ] WB04-D | **하위 단위:** [x] D1 공통 공간 해제·spot genus·건물 옆면 해제(2026-11-05, [문서](docs/webpj-squid-unpop-20261105.md)) / [ ] **D2 (다음)** `SquidUnpop` 처리기(표시 갱신 vtable 지원표 `unpop-vtables.tsv`·미니맵 dirty 큐·contained 종단)와 공통 삭제기 `RawSquidDestroy`(`unpop-x86.tsv`·`destroy-x86.tsv` 등) / [ ] D3 Form 부착·생성자 / [ ] D4 Factory Create/Take·ProcessHost·Regular. 착수 때 `node webpj/tools/fixture-manifest.mjs --write`로 D1 항목(`squid-unpop-parent`·`squid-hash`·`sid-pool`)의 `webCommit`(`worktree`)을 실제 커밋으로 갱신. 다른 세션이 Form/Factory를 활발히 바꾸는 중이므로 착수 시점의 커밋본을 기준으로 하고 이후 변경은 3.5절에서 반영. 최신 생성자/가상 배치·Create/Take·Unpop/내용물 해제·Form·종속 삭제/반납. 효과 뒤 현재 TYPE/VT/슬롯과 진단 복귀·부분 쓰기 순서 보존. **Regular 공통**(`SquidProcessHost.AddRegular/AddSharedRegular/FindEvent`, `regular-current-*-x86.tsv`)도 여기서 이식(2026-11-05 C2에서 이동). C2 `GameAuxState`의 Form/예약 번호를 빌림 | `SquidFactory`·실제 World 바인딩과 해당 원본 fixture 대조.841/836의 연결을 Factory 전체 완료로 확대하지 않음. 풀의 상위reset/대여 정리/실패점도 연결 |
| 4 · [ ] WB04-E | 공간spots/해시 등록·Graph/점유·Pop/PostPop·소유자/지역/장부·HP/프레임·내용물 생성과 소유자 마감. 필요한 Player/기술/SP/Deck 기반은 WB06에서 선행 | `RawSurfaceWorld`와 같은 공유 저장소에 실제 필수 효과를 연결. 공통 Pop/파생 가상 진입은 cpppj 최신 부분 완료 경계를 유지. 필요한 장치를 빈 함수로 채우지 않음 |
| 5 · [ ] WB05-A/B | **실제 Fort Load→Start**: 기존 판독/영역/지형과 WB03-AB를 같은 월드에 조립. 지형 생성/저장 객체·섬/소유자/원소 마감·Player/동맹/기술/SP/Deck/AI 초기화·Start/Guide를 현재 cpppj 부모의 호출 순서로 연결 | 같은 파일/옵션/시드·같은 Load/Start 지점의 SID/타입/HP/좌표/owner/Graph/장부/난수 L2 비교. 임시 Player/AI/색 번호로 미션 시작 성공을 만들지 않음 |
| 6 · [ ] WB03-M2 / WB05 표시 | 같은 월드의 가상 Draw/현재 프레임·소유자 색/그림자·원소/배경·실제 카메라를 공통 출력기에 연결. 좌측덱/미니맵/선택/비용·게임 커서/키/우클릭·ESC 입력을 실제 명령으로 연결 | `?map=` 개발 화면과 구분한 실제 미션 표시. 영어4:3 같은 상태 비교 후 한국어/와이드/입력 좌표 확인. `MissionBegin`·`StartDemo`·환영 창 명령 연결, 미지원 경계는 명시 |

순서2~4는 큰 덩어리로 한 번에 완료 처리하지 않는다. 실제 호출 관계에 맞춰 생성자·가상 분배·Graph·각 파생 Pop 등으로 나누고 파일별 현재 계약과 검사를 기록한다. M2에 필요한 기반을 옮기는 것이며 이 단계에서 WB04 전체가 끝났다고 쓰지 않는다.

### 3.3 플레이 규칙 → 미션 수명 → 배포

| 순서·상태 | 작업 | 완료 기준 |
| --- | --- | --- |
| 7 · [ ] WB06~08 | SP 저장/비용/회수/지갑·Deck/재충전/에너지, 사제 상태/이동/경로/수확/운반, 다리/건물 배치·빌더/생산·취소/실체화 | 동일 입력/시각에서 위치·SP/난수·점유/Graph·요청→확정/통지→Process→완료가 같은 실제 상태를 사용. 일률 시간/즉시 완성으로 근사하지 않음 |
| 8 · [ ] WB09~10·WB12 | 총/투사체/피해·벽/타워·유닛/파생 효과, 신전/제단·지식/의식/보상/승패 조건, AI 전체 행동·탐색/전략·종료 수명 | 현재 Player/기술/SP/명령/난수로 연결. 임시 방어 AI를 만들지 않고 cpppj 복원/연결/검증 범위를 타입별로 기록. 실제 포획/희생·특정 적 공격·승리 재현과 공중3종 전용 검사는 1절의 사용자/보류 경계를 지킴 |
| 9 · [ ] WB11·WB13·WB15 | 튜토리얼 게이트/명령/정지·현재 미션 Tell·도움말/지식·한국어 미션, Retry/Next/Abort/메뉴 복귀·실제 진행 저장 연결, 데모 재시작/360초/Auto-Demo, 위치 음향/음악 수명·Wind Noise/좌우 교환 | 일반31+데모3의 범위 내 초기화/입력/저장/수명을 단계적으로 제공. `Done*` 쓰기 기능과 자연 승리 경로 연결을 구분. 스크립트 도움말 Back은 원본 명령 재등록에 연결; Tell에서 열었다고 이전 Tell을 다시 열지 않음 |
| 10 · [ ] WB16 | 원본 간격 유지 후60/120표시·성능/메모리, Chrome/Edge/Firefox·GUI Linux, 정적 배포/하위 경로·새로고침·캐시/자산 버전/저장 호환 | 개발 서버 없이 같은 공개 자료로 실행. 브라우저/OS·DPI·소리/저장 환경을 실제 확인한 범위만 기록. 배포 제품 결정 시 README 등 공통 문서 일괄 갱신 |

**진행 기준:** M1 완료 → M2(실제 시작 상태/정적 표시) → M3(Tutorial1·2/캠페인1-1·1-2를 사용자가 검증 가능한 플레이·진행 저장으로 제공) → M4(범위 내 전체 싱글플레이 제공) 순이다. M3/M4의 자연 완주/전체 동일성 확인은 사용자에게 남기며 AI 작업을 무기한 막는 완료 조건으로 두지 않는다. 사용자 제보는 해당 구현 단계의 수정 작업으로 올린다.

### 3.4 cpppj 잔여와 webpj의 관계

| cpppj 공통 인계 | webpj 처리 |
| --- | --- |
| 2절의 장기World/Player/AI·Unpop/Take·현재 TYPE/가상 분배/파생 후처리 | 이미 복원된 선행 근거다. 옛 “25종 전체 연결 미완료/TYPE139 실패”를 현재 상태로 복사하지 않고 각 모듈의 최신 단위 문서/커밋/fixture를 확인해 이식 |
| 3절4번의 checking12·공통 Pop/가상20·Factory/Teleport 나머지·사제 후처리 후보 | 웹도 미복원/부분 연결 경계를 유지. CP의 새 커밋이 확정된 뒤 영향 검토하여 반영. 아직 없는 효과를 일반 성공 처리하지 않음 |
| 3절1·3번의 객체 메뉴/커서·ESC/View·모달·Retry/Next·데모 반복·State cleanup | 싱글플레이 해당 경로를 순서6~9에서 이식. 이미 완료한 키/reader/UI는 재복원 과제로 세지 않고 나머지만 확인 |
| 3절2번 Edit·3절8번 MP/Outpost·일반 전투 saveBattle/metamap/Reconnect | 웹 제외. 캠페인 공용 생성/Load/소유자 코드는 유지하되 Edit UI·MP State·패킷/구 서버를 만들지 않음 |
| 3절4번 군도 첫 시드/상위·TARC/Config 동적 실행/파일 실패 | 공식 미션/데모의 실제 소비 경로부터 확인. 미복원 부모가 필요한 기능만 대기. 본섬 일반 성장에 별도 군도 검사를 임의 삽입하지 않음 |
| 3절5·6번 전체 동일성·자연 완주 | 사용자 확인/제보 영역. 과거 CP 완주를 웹 성공으로 합산하지 않음 |
| 3절7·8번 Windows/DD/CD·Saved Games·후순위4~8차 | OS 전용 처리는 웹으로 복제하지 않음. 적용 가능한 브라우저 수명/입력/저장/한국어 요구만 설계. CP의RDP 물리입력 실패를 웹 전체의 실패로 간주하지 않음. 별도 지시 없는 웹 MCP·AA/FSR/편집기 요구를 추가하지 않음 |

### 3.5 cpppj 변경 재반영 (2026-11-05 사용자 결정)

cpppj는 아직 진행 중이므로 webpj 단위는 착수 시점의 cpppj 현재 버전으로 구현하고, 그 뒤의 cpppj 변경은 이 절의 **재반영 과정**으로 따로 반영한다. 재반영은 3.2·3.3절 순서의 단위 작업을 막는 선행 조건이 아니다. 실행 시점은 개발자 지시나 해당 모듈을 다시 다루는 단위에서 정하며, 큰 단계(예: M2·M3 진입 전)마다 한 번은 목록을 정리한다.

1. **대상 찾기:** `node webpj/tools/fixture-manifest.mjs`의 "cpppj 변경" 보고(이식 추적 목록의 `cpppjBasis` 이후 커밋)와, 목록에 fixture가 없는 웹 모듈은 단위 문서의 근거 커밋 이후 `git log <근거>..HEAD -- <cpppj 소스>`로 찾는다.
2. **분류:** 각 변경을 ① 웹 결과에 영향 없음(구조 정리·진단·웹 제외 기능) ② 웹 반영 필요 ③ 아직 웹에 없는 후속 단계 몫으로 나눈다. cpppj fixture가 바뀌었으면 해당 웹 검사를 먼저 다시 돌린다.
3. **반영:** ②는 별도 WB 단위(`재반영` 명시)로 웹 모듈·검사·단위 문서를 고치고 커밋한다. ③은 해당 단계 항목에 근거 커밋과 함께 옮겨 적는다. ①도 판단 근거를 남긴다.
4. **갱신:** 반영/판단이 끝난 항목은 `node webpj/tools/fixture-manifest.mjs --write`로 `cpppjBasis`를 새 기준으로 올리고, 아래 표에서 상태를 바꾼다.

| 상태 | cpppj 소스(웹 기준 → 변경 커밋) | 웹 모듈 | 1차 분류(2026-11-05, diff 크기·제목 기준, 미검토) |
| --- | --- | --- | --- |
| [ ] | `o/Config.cpp` `0ded2f07` → `2ca39d0b` | `assets/config.js` | ① 가능성: LoadBytes 분리·`@파일` 적재 훅. 의미 동일 여부 확인 필요 |
| [ ] | `o/ConfigInterface.cpp` `2ede4027` → `8d2f81a1`·`1ba89851`·`2ca39d0b` | `assets/game-data.js` | ③ 가능성: 설정 Load/FileSpec 수명·Startup 파일명 순서. WB05 현재 미션 Tell 연결 때 검토 |
| [ ] | `client/TellSection.cpp` `6dd8d803` → `2ca39d0b` | `assets/game-data.js`·`ui/tell-parameters.js` | ③ 가능성: fallback tell 적재가 `config.LoadFile`로 변경. WB05 현재 미션 Tell 연결 때 반영 |
| [ ] | `o/RiftTypeTable.cpp` `700475b5` → `fbc27346` | `assets/type-flags.js` | ② 또는 ③: 타입 레코드에 기본 외형 프레임(+128) 보관. WB03-M2/WB05 Draw 연결 때 반영 |
| [ ] | `o/CanonDecoder.cpp` `5bdb5bed` → `167866d9` | `assets/territory.js` | ① 가능성: 현재 프레임 진단·커서 배치 소비자. 영역 해석 결과 영향 확인 필요 |
| [ ] | `o/RawTerrainContext.cpp` `552656c0` → `3eec0830`·`1fe5fae7` | `assets/terrain-mask.js`·`terrain-fringe.js` | ③: 판본·군도 모드 접근자. 군도 지형 이식 때 반영 |
| [ ] | `o/RawIslandPostPop.cpp` `deaf73f8` → `0c06fa66` | `assets/island-supports.js` | ③: 섬/표면 현재 후처리·진단 복귀(64+/29−). WB04-E Pop/PostPop 이식 때 반영 |

표의 커밋은 2026-11-05 `0563c07f` 시점이다. 이후 cpppj 커밋으로 대상이 늘 수 있으므로 재반영 착수 때 도구 보고로 다시 만든다.

## 4. 이관해도 남겨야 할 미해결 한계

아래는 과거 “다음”에서 아직 해소되지 않은 항목이다. 완료한 Tell/링크/도움말 번역/아이콘을 다시 미완료로 만들지 않는다.

| 항목 | 현재 한계와 처리 단계 |
| --- | --- |
| 실제 지형/소유권 | 미리보기의 중립색·가운데 카메라·고정 구름은 개발 정책. 저장 noIsland.owner를 최종owner로 오해하지 않음. 신전 원소 투표/themeMode·도장/flood 순서·실제 카메라 시작/제한/스크롤은 순서4~6에서 연결 |
| 지형 난수/색표 | `buildTerrainFrames`와 절벽의 게임/CRT 난수는 실제 세션에서 공유. 절벽 제외0x10/0x18의 선택, 최초 팔레트/ColorSort 시점·`useBanners` 기본값, 소유자→색 번호를 순서5~6에서 확정 |
| edgeFarm | spots·finder·MayPlace·지면 Retype 뒤 실제 월드에 구현. `RawFarmCluster`의3×77 별도 시드/30000실패 한도 등 원본 순서를 유지. 타입121에 isle 색표가 있다고 추정하지 않음([근거6절](docs/webpj-terrain-extras-20261102.md)) |
| 영역 회전 | 기존 실제 파일은 방향0. 회전2/3의 실제 저장 사례가 생기면 원본 결과와 추가 대조. 독립 기하 계산 통과를 실제 파일 관측으로 세지 않음 |
| 입력/커서 | 커서2~14·17·18은 쓰는 미션 입력과 함께 추가. 본문 끌기/캡처 중 Overlay의 원문 입력(`004d62b0` 계열) 계약은 미확정 |
| Options/저장된 값 | Help0/Options−40은 측정값이며 일반 위치 규칙/와이드 메뉴 배치는 미확정. Wind Noise·Speaker Swap·Edge Scroll·Auto-Demo는 해당 시스템과 실제 연결 확인 필요. 시작 팁·설정→GameData 동기화/Done* 저장은 이미 구현 |
| 도움말 고급 표시 | `<info>` 머리글/능력치, `#`/`&` 상자 맞춤·색 치환, 원본 삼각형 색 해석 확인. 웹은 원본 스크린샷 기준 검정 화살표를 사용. 레이어0/그림자 없는 도움말 그림 경계 유지 |
| 에너지 기호 | `mana.8~11`은 현재10.78 클러스터8개 밖이라0크기. 실제 원본 F1 모습 관측은 미확인. 원본 실행이 필요하면 AGENTS.md의 확인/예외 규칙 적용 |
| Tell/언어 | `<background>`는 이름만 보관(`nsisles` 자료/사용 근거 미확정). 현재 미션·`{@미션.키}`·브리핑/안내 `.korean`·스크립트 도움말 Back은 순서5/9에서 연결. 한국어 영어대체/긴 줄/고DPI도 해당 화면과 확인 |
| 메뉴 확장 | 두 열 메뉴는 범위 내 실제 소비자가 필요할 때 원본 계약으로 이식. `%{userSpec}`·User Made Campaigns는 제외이며 새 과제로 돌리지 않음 |
| 입력/오디오/환경 | 실제 스피커 청취·모든 브라우저/GUI Linux·고DPI/모니터 이동은 미확인. 일부 Edge/Chromium 기록을 전체 플랫폼 지원으로 쓰지 않음 |
| README 일괄 갱신 | 기존 문서의 WB03-O 이후 상태가 미반영. WB03-P~AB·WB04-A와 이후 확정 범위를 공통 문서 갱신 시 반영; 이번에는 수정하지 않음 |
| Credits AI 문구 | 2026-11-05 사용자 결정: 현행 문구(`OpenAI Codex, ChatGPT and Anthropic Claude Code`, 한국어 동일 취지)를 **유지하고 나중에 수정**한다. cpppj/dotnetpj 문구(Codex·ChatGPT만)와 다르며, 수정 지시가 오면 `www/assets/product.js`의 영어/한국어 섹션과 `tests/product.test.mjs`의 문구 검사를 함께 고친다. 한국어 행 이름 `NetStorm 11.01 패치 크레딧`은 확정 |

## 5. 검증과 완료 판정

| 단계 | 확인 방법과 책임 |
| --- | --- |
| L1 모듈 | 원본10.78 fixture로 반환/전체 슬롯·전역·정수/float/double 비트·효과/난수 순서를 대조. Adler 일치는 체크섬 비교이며 바이트 직접 비교와 구분 |
| L2 시작 상태 | 같은 Fort/언어/옵션/시드와 같은 Load/Start 시점의 SID·타입·owner·좌표/HP/프레임·Player/동맹/SP/Graph/장부 비교 |
| L3 입력 진행 | 같은 명시 시각/입력열로 허용된 경로를 진행하고 상태/효과 순서 비교. 표시 주사율·비동기 자산 로딩이 게임 난수/시계를 바꾸지 않도록 검사 |
| L4 화면/소리 | 변경과 관련된 짧은 메뉴/진입/기능·동일 상태의 비교. 전체 외관/청취 동일성은 사용자 확인. 원본 캡처의 창 테두리/레터박스/HDR·방송 음성을 분리 |
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

## 7. 이번 정리와 과거 이관

- **2026-11-04, HJOW-Athlon, 사용자 요청:** `0fd343a1` 직후 원문 전체를 보관 문서에 그대로 남기고, 현행 문서는 현재 기준·완료 색인·의존 관계에 따른 순서·미해결 한계 중심으로 다시 작성했다. 빈 골격/미추적 파일/없는 자산·자리표시자 npm test 설명, 이미 끝난 “다음”, 과거 호스트/검사 수 나열과 거대한 진행 이력을 이관했다.
- **계획 갱신:** 공통 인계861의 완료/부분 연결/잔여를 분리해 반영했다. 소스에서 확인한11.01/Credits 누락·외부 접속 문서·이식 추적을 선행 보완으로 넣고 SID 다음의 실제 월드→Fort→M2→플레이/수명 순서를 정리했다. 옛 AI 자연 완주/실제 포획·희생 완료 조건은 공통 인계의 사용자 확인 결정으로 대체했다. 글꼴 WOFF2·제외 기능·번역 검수·README 일괄 갱신과 미해결 지형/메뉴/도움말 항목은 유지했다.
- **검증 범위:** 원문 보존 바이트/SHA-256·정리한 문서의 UTF-8/로컬 링크/참조 앵커·공백/충돌 표식과 변경 파일 범위를 확인했다. 이번 문서 작업에서는 게임 코드/자산 수정·npm/브라우저/게임 실행·디컴파일을 하지 않았다. 다른 세션의 cpppj 미커밋 변경은 보존하고 커밋에 넣지 않는다.

- **2026-11-05, HJOW-Athlon, 사용자 결정 반영(문서만):** Credits AI 문구 유지·나중 수정(4절), 한국어 행 이름 확정, 바깥 링크 새 탭/창 열기(1절 결정·3.1절 미구현 항목), 개발 서버 접속 제한 불필요(1절), 진행 중인 cpppj 기준과 재반영 과정(1절·3.5절)을 기록했다. 코드·검사·자산은 바꾸지 않았다.

**오래된 내용은 [LEFT_JOBS.webpj.old.md — WB04-A까지의 원문 보관](LEFT_JOBS.webpj.old.md#archive-webpj-20261104-through-wb04-a)으로 이관했다. 그 문서의 “현재/다음/미구현”은 당시 기록이며, 이후 작업은 이 현행 문서를 기준으로 진행한다.**
