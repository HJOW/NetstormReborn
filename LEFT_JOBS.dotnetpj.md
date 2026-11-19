# LEFT_JOBS.dotnetpj — 현행 상태와 남은 작업

**2026-11-19 · 마지막 커밋 구현 0.638 · 다음 구현 0.639(미션 시작 CreateAiBrains live 연결·brain 행 대조) · 현재 단계 R4-2·R4-5 병행 · cpppj 제품 1322 / 11.01 · 전체 목표 미완료**

현행 인계는 이 문서가 기준이다. 0.600~0.635의 단위별 완료 문장, 단위마다 누적한 검사 수와 L2 보관 경로 나열은 [0.635 정리 전 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)에 있다. 0.613까지의 경과, 파일 계열별 체크 목록과 옛 집계는 [0.613 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에 있다. 단위별 근거는 `docs/dotnet-canon-0NNN-*.md`이다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), 웹은 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 따른다.

<a id="dotnet-overview"></a>

## 0. 현재 상태와 다음 작업

| 경로 | 실행·플레이 | 원본과의 동일성 |
| --- | --- | --- |
| 기존 배포 `Netstorm.Game` (`Netstorm.exe`) | 메인 메뉴와 캠페인 1-1·1-2를 플레이할 수 있다. 한국어 선택, 와이드 시야 확장, 테두리 없는 전체화면을 제공한다. 실행 오류만 수정하는 동결 상태다. | RGBA 합성·자체 UI·근사 `BattleSession`으로 동작한다. |
| 새 경로 `Netstorm.Client` + `Netstorm.Client.Host` | 타이틀·메뉴·도움말·Options·Credits·캠페인 선택·Edit 목록과 이름 창이 동작한다. 개발용 `--mission <키> --handle <이름>`으로 일반 31미션의 시작 상태 일부를 조립한다. 월드 표시·입력·규칙의 실제 경로가 남아 있어 플레이 불가다. | 1024×768 메뉴 **12장면**의 논리 BMP가 cpppj와 바이트 일치했다(0.381~0.388). L2는 **0/34**, 데모 3경로는 DN 진입 미연결이며 L3·L4b·L5는 미착수다. |

- **마지막 커밋 구현 0.638.** 직접 AI 지갑·파라미터 세션 결선과 세션 소유 brain 메모리(첫 토큰 `0x20000000`)를 연결했다. L1 집중 4건 통과. [0.638](docs/dotnet-canon-0638-20261119.md)
- **마지막 커밋 구현 0.637.** 직접 AI 본체 기록기(`CrankAiMoney`·`SetAiParameter`) 본체를 이식하고 원문 1940행 전수로 대조했다. L1 집중 3건 통과. [0.637](docs/dotnet-canon-0637-20261119.md)
- **마지막 커밋 구현 0.636.** 펜스 Pop·정기 세션 결선(`BuildFenceDevices`·가상표 관문·`range` 파싱)을 마감했다. L1 집중 6건 통과, `guardmyback` L2 재수집(DN +2행·공통 +4행·타입 +21행·`dn_only` 해소). [0.636](docs/dotnet-canon-0636-20261119.md)
- **검사 기준선.** 마지막 전체 회귀는 0.630이다. Assets 366/366·Core 2,936/2,937·Client 1,297/1,297이 통과했다. Core 1건은 `PlayerTechnology`의 `NETSTORM_DATA` 미설정으로 인한 환경 요인이며, 지정 뒤 단독 재실행에서 통과했다. 이후 Client 전체는 0.635에서 1,308 통과다. 2026-11-19 사용자 지시로 전체 회귀는 4단위당 1회만 수행한다. 이번 단위는 집중 검사만 수행했고 다음 전체 회귀는 0.640 시점에 수행한다. 단위별 집중 검사 수는 각 단위 문서에 있다. [0.630](docs/dotnet-canon-0630-20261119.md) · [회귀 목록](docs/dotnet-regression-inventory-20261115.md)
- **L2 기준선.** 일반 31경로의 양쪽 덤프와 데모 3경로의 cpp 덤프를 확보했고 전체 대조는 `extracted/dotnet-l2-0567/`에 있다. 최근 재측정은 3경로(`thewarbegins`·`tutorial1`·`bc1menu76`)가 0.625, `surrounded`가 0.623, `guardmyback`이 0.636(`extracted/dotnet-l2-0636/`)이다. 단위별 수집은 `extracted/dotnet-l2-0NNN/`에 둔다. 남은 차이는 정기 60행·`object` 행 부재·AI·cpp 전용 슬롯·농지 가장자리·공장 뒤 4건 sid 재사용 순서다. [일괄 수집](docs/dotnet-l2-batch-20261114.md) · [격차 분석](docs/dotnet-l2-0606-20261117.md) · [3경로 재측정](docs/dotnet-canon-0623-20261119.md)

<a id="now"></a><a id="09-바로-다음-작업"></a><a id="r4-board"></a>

### 0.1 다음 구현 0.639

1. **0.639 미션 시작 `CreateAiBrains` live 연결과 brain 행 대조(R4-5).** 실제 `Construct`·`InitialUpdate` 효과와 월드 본체를 시작 흐름에 연결해 brain·SP·Player 행을 대조한다. [0.638 결과](docs/dotnet-canon-0638-20261119.md)
2. **나머지 틱 난수원과 정기 연결(R4-4).** 장식 재생·연소 등 나머지 틱 난수를 잇는다. 정기 부착 시점 정렬(R4-6 로그인·시작 흐름)과 미션 시작 Pop 19+1건(R6 내용물·생산)도 남아 있다.
3. **R4-2 나머지 차이와 R4-3~R4-8.** 농지 프레임, 전용 요새, 표시 등록, 그래프 번호, 장부를 처리한 뒤 R4-3~R4-8을 진행한다. 통합 조각을 마칠 때 같은 조건의 일괄 덤프로 M1 진척을 확인한다. [R4 체크](#r4-detail)

<a id="first-run"></a>

### 0.2 최초 실행 상태 — DN 적용 대기

원본 관찰과 cpppj 1314 구현이 완료됐다. DN은 빈 사용자 루트·무옵션 데이터 사본에서 아래 세 항목을 메뉴/설정 결선과 함께 확인한다. [원본 관찰](docs/exe/original-first-run-20261116.md) · [cpppj 1314](docs/exe/cpp-first-run-1314-20261116.md)

- [ ] Welcome(OK) → Welcome2(Play Early Missions / Main Menu) → 메뉴 버튼. Welcome2가 열릴 때 `welcomeGiven=1`을 사용자 루트에 저장하고 재실행 시 Tips 분기를 확인한다. `HostStartupScene.SaveOptions`의 설치 데이터 쓰기도 사용자 루트로 이동한다.
- [ ] Struggle For Freedom 최초 상태는 1번만 선택 가능하고 1~6번 모두 체크가 없다. 자기 `Done*`는 체크, 직전 `Done*`는 해금에 쓰고 저장·재실행을 확인한다. 실제 미션 결과 연결은 R6/R7에서 검증한다.
- [ ] 최초 Options는 소리·음악·바람·가장자리 스크롤·Auto-Demo·팁 ON, 스피커 교환 OFF, 효과음·음악 각각 5, 800×600·전체화면 요청이다. 기존 사용자 값과 진행을 보존하고 전체화면 재실행을 확인한다. 메뉴 일치 시험의 1024×768 창 모드는 시험 조건이다.

<a id="korean-font-change"></a>

### 0.3 한국어 글꼴 — DN 적용 대기

AGENTS.md의 현행 지정은 갈무리11이다. cpppj 1322의 외부 비트맵 출력·배포가 완료됐고, DN 배포판은 아직 D2Coding 1.3.2를 사용한다. 새 호스트의 한국어 화면 연결은 R8 6차다. [공통 기준](LEFT_JOBS.md#korean-font-change) · [cpppj 1322](docs/exe/cpp-korean-galmuri-1322-20261117.md)

- [ ] `KoreanFontFamily` 공통 상수와 글꼴 파일·라이선스 설정을 마련해 로딩·측정·그리기·한글 대체·개발 표시가 같이 사용하도록 한다.
- [ ] 기존 배포의 한국어 출력과 새 경로의 8비트 출력에 `Galmuri11Bitmap-Regular-2.40.4.ttf`·`Galmuri11Bitmap-Bold-2.40.4.ttf`(family `Galmuri11Bitmap`)의 비트맵 글리프를 적용한다. D2Coding용 TTC face 선택은 전환 대상이며, TTC 판독기 검사는 실제 TTC로 유지한다.
- [ ] `Netstorm.Game.csproj`와 `package-dotnetpj.ps1`의 복사·필수 파일·고지·안내를 맞춘다. 두 TTF와 `Galmuri11-LICENSE.txt`를 exe 옆 `fonts/`에 두고, R5 배포 전환 시 설정을 승계한다.
- [ ] 작은 메뉴·도움말·브리핑과 영한 혼합 제목에서 일반/굵은 획·폭·기준선·줄 높이·잘림을 확인한다. 영어 `.chfnt`와 일본어 Noto Sans JP를 함께 대조한다.

<a id="exe-icon"></a>

### 0.4 실행 파일 아이콘 — DN 적용 대기

AGENTS.md 2026-11-19 추가(`d047bf9a3`)로 `Netstorm.exe`의 아이콘은 기존 게임 실행 파일 아이콘과 같아야 한다. 원본은 `originals/Netstorm.exe`의 아이콘 그룹 102(32×32·4bpp)와 144(16×16·4bpp)이며 탐색기 파일 아이콘은 102다. 현재 `Netstorm.Game.csproj`에는 `ApplicationIcon`·아이콘 리소스가 없다. 빌드의 `.exe` 혼입 검사 때문에 cpppj처럼 원본 실행 파일을 실행 중에 읽을 수 없으므로, 추출한 `.ico`를 저장소 자산으로 쓴다. [공통 기준](LEFT_JOBS.md#exe-icon)

- [ ] 원본 두 그룹을 그대로 담은 공통 `.ico`를 쓴다. cpppj나 webpj가 먼저 만들었으면 그 자산을 재사용하고, 없으면 만들어 생성 도구·해시를 단위 문서에 기록한다.
- [ ] 현재 `Netstorm.exe`를 만드는 `Netstorm.Game.csproj`에 `ApplicationIcon`을 지정한다. DesktopGL 창·작업 표시줄 아이콘에 내장 리소스가 따로 필요한지는 사용하는 MonoGame 버전에서 확인한다. R5에서 새 경로가 `Netstorm.exe`를 이어받을 때 같은 설정을 승계한다.
- [ ] 빌드된 exe의 아이콘 리소스를 원본 바이트와 대조하고, `package-dotnetpj.ps1` 산출 ZIP의 `Netstorm.exe`에도 적용됐는지 확인한다. 탐색기 표시는 사용자 확인으로 둔다.

<a id="dotnet-stage-board"></a><a id="stage-criteria"></a>

## 1. 단계별 작업과 완료 기준

| 단계 | 현행 상태와 완료 기준 |
| --- | --- |
| R0 기준선·동기화 | 완료. cpppj 기준선·PORT_MAP·`cpp_sync.py`. |
| R1 화면 호스트·입력 큐 | 완료. 8비트 호스트·입력 큐·비교 도구. 실제 월드 입력 소비는 R6. |
| R2 그리기·팔레트·글꼴 | 원시 본체 완료. 실자산 어댑터·수집/정렬·종류별 분배·`SpriteDraw`/그림자 공유와 `owner-capture` C행은 R4/R5에서 연결. |
| R3 Gump UI·메뉴 → D1 | 메뉴 L4a 12장면 일치. 메뉴 상위 소유자·모달·등록·대화창·텍스트 입력과 하위 화면 연결 잔여. 미룬 장면은 Language 자식 CJK(R8), Multiplayer 섬(R5/R11), Edit 이동행(R7). [UI 잔여](#r3-r2-detail) |
| **R4 월드 세션 조립 → M1** | **R4-2·R4-4 진행 중.** 34경로(Early 6·Training 9·캠페인 16·데모 3)의 같은 시드 Load/Start `session-dump`가 cpppj와 일치해야 한다(L2). [R4 체크](#r4-detail) |
| R5 월드 화면·카메라·HUD → D2 | 렌더러 수집·정렬·dirty/팔레트, 타입별 Draw와 효과, 카메라·전체화면 가장자리 스크롤, HUD·덱·미니맵·인벤토리. 진입 직후와 N프레임 뒤 34경로 L4a 차이 0 및 압축 해제 실행 후 `Netstorm.exe`와 배포 대상을 새 호스트로 넘긴다. |
| R6 플레이·규칙·AI·음향 → M2/M3 | Tutorial1 → Early Missions → 1-1·1-2 순서로 입력·커서·선택·메뉴, 사제/경로/운반/수확, 건설/생산/경제/다리, 전투, 의식/승패, AI, 소리/음악/동영상, 채팅/경고를 연결한다. M2는 고정 입력열 L3와 대표 캡처 일치, M3는 1-1·1-2가 기존 경로 수준 이상으로 진행되는 것. M3 뒤 `NetstormLegacy`와 근사 검사를 제거한다. [선행 완료와 잔여](#r6-preports) |
| R7 미션 전체·Edit·설정·배포 → M4 | 34경로 진입·진행·저장, Retry/Next/Abort, 데모 2주기, Edit 왕복 저장·Save as·Test Battle/복귀, `options.cfg` 시점·전체화면 재실행, A~F 중 C·D·F 판정. ZIP 전수 대조와 압축 해제 실행, `Netstorm.exe` 외 실행 파일 사용법 문서화. 배포본 `Netstorm.exe`의 아이콘이 원본 실행 파일 아이콘과 같은지 확인한다(AGENTS 2026-11-19 추가, [0.4](#exe-icon)). |
| R8 AGENTS 4~15차 | 아래 차수별 요구에 도달할 때마다 기능을 검증한다. 공통 파일을 옮기는 과정에서 이미 들어온 부분도 실제 경로에서 확인한다. [요구사항](#dotnet-r8-requirements) |
| R9 60/120 FPS | Frame Rate 60/75(원본)/120과 양보 대기의 프레임 시간 대조. 비행체 표시 보간은 cpppj D2 결정에 따른다. |
| R10 Linux | DesktopGL와 파일 장치의 경로·권한·원자 저장을 확인한다. `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`를 사용한다. |
| R11 로컬 멀티플레이·Outpost | TCP/IP 로컬만 지원한다. `Netstorm.Net` 일부 사전 구현. 루트 서버 독립 모듈은 로컬 실행에 필요한 범위를 먼저 판정하고 실동작은 사용자 확인으로 둔다. |

<a id="r4-detail"></a>

### 1.1 R4 세션 조립 체크 (1/8 완료)

0.600~0.635 단위별 완료 문장은 [0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)과 각 단위 문서에 있다. 아래에는 하위 단계별 완료 범위와 남은 일만 둔다.

- [x] **R4-1 객체 수명 공통층(0.546~0.600).** 미션에서 `HostSession` 별도 풀을 제거했다. 풀·공장·공간·건물 저장소 공유는 live 슬롯 바이트 대조로 확인했다. Dais·Player·Aviary 개별 몸체, 장기 Unpop·Pop·Display·Graph·Host 실행기, 실제 전송·파생 Runtime pin은 R6 장치 결선에서 잇는다. [마감 감사](docs/dotnet-r4-1-close-20261117.md)
- [ ] **R4-2 적재 Pop 전체 상태 일치.** cpp 근거는 `RawNormalTerrainWorldBinding`·`RawRegionPaint`·`RawRegionOwnership`·`Islandbuilder`·`SquidPostPop`이다.
  - 완료(섬·지형): 3×3 스탬프와 섬 `Prefix` 발화, 적재 깊이(0.601~0.604), 그래프 per-Pop 결선(0.605). [섬 후처리](docs/dotnet-session-island-postpop-20261117.md) · [그래프](docs/dotnet-session-terrain-graph-20261117.md)
  - 완료(요새·농지): 지원 타입 요새 일반 Pop(0.607), 농지 어댑터와 Generate 실결선(0.612~0.614)을 마쳤다. 대표 3경로의 농지 선택 집합·타입·소유자가 100% 일치했다. [요새 Pop](docs/dotnet-fort-general-pop-20261118.md) · [Generate](docs/dotnet-canon-0614-20261118.md)
  - 완료(지역·신전): 지역 칠하기·소유권 본체와 세션 연결(0.618~0.619), 신전 가상 Owner 본체와 결선(0.620~0.621), 사원 Pop 파생(0.622). [0.619](docs/dotnet-canon-0619-20261119.md) · [0.622](docs/dotnet-canon-0622-20261119.md)
  - 완료(공장): 후처리 본체(0.624), 정기 트리거 세션 연결과 발화 확인(0.625~0.626), Fort 경로 L2 대조(0.627), 정기 본체(0.628), 정기 분배(0.629), 초기 레벨 적재(0.630). [0.630](docs/dotnet-canon-0630-20261119.md)
  - 남음(일치 대조): 농지 프레임 일치(대표 3경로 0/152)와 그래프 번호(`surrounded` 0.623 기준 215/255)를 맞춘다. 표시 등록·스탬프·장부 `totals`/`totalmade`도 덤프와 대조한다.
  - 남음(요새·공장): 비용이 있는 전용 요새 8종(71·75·78·89·122·123·124·158)은 R6 파생 본체에 의존하고, 지원표 밖 처리도 남았다. 공장 뒤 4건의 sid 재사용 순서와 업그레이드 그림 의존 고리도 맞춘다.
- [ ] **R4-3 신전·소용돌이 → Player 첫 지역.**
  - 완료: 신전 Pop → 소용돌이 → 섬 소유자 → Player `+0x78` 경로값이 일치했다(0.622, `thewarbegins`·`tutorial1`). `surrounded` 혼합 경로에서도 첫 지역 9행이 일치했다(0.623).
  - 남음: AI 뇌 경로(`bc1menu76` 사원 없는 6행)는 R4-5 뒤에 닫는다. 농지 가장자리 432칸은 R6·R11 투표 입력이다. 거주 구역 5칸 전파는 0.623 기준이며, 0.635 거주 구역 연결 뒤 `surrounded`는 다시 재지 않았다.
- [ ] **R4-4 정기 처리·프로세스 초기 등록.** 입력은 `RawAssetRegular`과 Player·Dais·Dude·Disc·장식 정기 처리다.
  - 완료: 미션 틱 `AdvanceMissionFrame`(0.629), 고정 시각 1프레임 1틱과 덤프 시계(0.631), 매 프레임 `UpdateAmbientWind`(0.632)와 주변음(0.633), 간헐천 Pop과 사건 0 정기(0.634), 거주 구역 Pop과 사건 0·1·3 정기(0.635), 펜스 Pop·정기 결선(0.636). [0.631 격차 분해](docs/dotnet-canon-0631-20261119.md)
  - 남음: Kernel/Process 초기 등록과 초기 덤프 비교, Player·Dais·Dude·Disc·장식 정기, AI 초기화·장식 재생·연소 등 나머지 틱 난수원을 잇는다. 정기 부착 시점(0.631에서 분해한 RNG 25,611 격차의 틱 구간)도 맞춘다. 거주 구역 사건 2(주민 생성)는 R6이다. 펜스 표식 생성·삭제·연결선 생성·이웃 새로고침도 R6이다.
- [ ] **R4-5 AI 초기화.** 실제 생성자·`SetAiParameter`(0.637 본체·0.638 세션 결선 완료, live 연결 잔여)·`CrankAiMoney`(0.637 본체·0.638 세션 결선 완료, live 연결 잔여)·`ResetAiForState`·`ReturnAiForState`, 600바이트 검사·준비 `00416eb0`·258 완료 판정, 설정 `+0x23c` 동맹 판정을 연결해 brain·SP·Player 행을 대조한다.
- [ ] **R4-6 내부 로그인·전송·Client 전역.** 메뉴 핸들 출처와 이름 꼬리 16~19바이트, 로컬 Login→Ready→Start 실제 패킷, 요새 이미지·`Bind*`·State 수명을 연결한다. 실제 handshake 검사와 메뉴/`--mission`의 진입 덤프를 대조한다.
- [ ] **R4-7 데모와 보류 검사.** `StartDemo` 3경로 덤프, `player-world-x86.tsv` 1,680행, F3에서 미룬 Fort/지형 바인딩·미션 Fort 경로·로그인 객체 전송·미션 재적재 검사를 잇는다.
- [ ] **R4-8 M1 판정.** 같은 시드·고정 프레임·빈 사용자 루트로 34/34 상태 일치. 로드 뒤 프레임 진행에서만 생기는 차이는 cpp 후속 프레임 덤프로 입증한 뒤 R6으로 넘긴다.

<a id="r6-preports"></a>

### 1.2 후속 단계에서 이미 이식한 범위

- [x] **R6-3 배치 본체(0.609~0.611).** 권한·관계, 지형 메이커 2종, Player 앵커·서라운딩·파이프라인을 이식했고 MayPlace 3건 전수 일치를 확인했다. 실제 농지 호출은 R4-2(0.614)에서 결선했다. 사용자 건설 입력 통합은 R6-3에서 검증한다. [0.611](docs/dotnet-canon-0611-20261118.md)
- [x] **펜스 본체 8종(0.587~0.592).** `RawFenceLifetime`·`Neighbors`·`Line`·`MarkerClear`·`Rebuild`·`Create`·`PostPop`·`Regular`과 해당 저장소 갈고리를 이식했다. 세션 Pop·정기 결선은 0.636에서 마감했다. R6의 타깃·타입 부모·Damageable 정기 라우팅과 표식 생성·연결선 생성·이웃 새로고침은 열려 있다. [펜스 후처리와 정기 처리](docs/dotnet-surface-fence-postpop-20261117.md)
- [x] **간헐천 수명과 벌룬 본체(0.583~0.584).** 세션 Pop·사건 0 정기는 0.634에서 결선했다. 채굴 `RawGeyserMined`과 `RawBalloonFrame`은 R6, AI 등록은 R4-5에서 잇는다. [간헐천](docs/dotnet-surface-geyser-20261117.md) · [벌룬](docs/dotnet-surface-balloon-20261117.md)
- [x] **Outpost 전처리 본체(0.582).** 장기 삭제 분배·목록 압축·부모 호출을 집중 검사했다. 전체 Outpost 수명과 LAN 통합은 R11에서 이어간다. [Outpost](docs/dotnet-surface-outpost-20261117.md)
- [ ] **공통 장치의 실제 미션 연결.** `RawSurfaceAssetDeletion/Notice`·`Content/Reward`·`Lifetime/Deletion`·`PriestDeletion`·`Damageable/Fence/Balloon`은 대응 본체 또는 분배 장치가 있다. 큰 파일의 미이식 분기와 호출자 장치는 R4-2~R6에서 확인한다. `RawIceTower`의 성장 정기·완성형 파괴·재생성/삭제와 Native 설치도 남아 있다. [R4-1 감사](docs/dotnet-r4-1-close-20261117.md)

<a id="r3-r2-detail"></a><a id="carried-limits"></a>

### 1.3 앞 단계에서 이월한 작업

- [ ] **R3 UI.** `ParseDialogPage`·`ConfigOverwriteAndQueueLifetime` 잔여와 MessageGump, 맵/파일/기술 목록·Guide·모달·명령 소유자, HTML 그림과 QuickHelp TYPE/AST, Console/클립보드/Save as/숫자 창/ChatWindow, 배경 GIF 노드·창, 하위 화면 라벨과 실제 명령 소비.
- [ ] **R2 화면 통합.** 실자산·그리기 결선과 구름 카메라·시간 이동·dirty 갱신·구름 파일 fallback(R5). 빠른 물리 입력 이력과 옵션 변경 중 소리 수명은 R6.
- [ ] **파일·호스트 경계.** `--set` 중복 처리 차이, `MatchDisk` 절대/긴 경로, Linux `WriteAtomic`·`ReadOnly`, 이름 정규화·옛 인코딩, Fort 이름/Template 소유자·Save as는 해당 소비자를 옮길 때 대조한다. CD 슬롯 폭·경계의 하네스와 기존 단위의 예외·할당·복사 경계는 보관본 2.6과 단위 문서를 참조한다.

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a><a id="04-기존-dotnetpj-소스의-재수정-판정"></a><a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a><a id="dotnet-user-decisions"></a>

## 2. 계속 유효한 결정

- **원본 10.78 경계 보존.** 화면은 8비트 인덱스 버퍼·256색 팔레트, MonoGame은 창·입력·오디오 장치·최종 출력이다. cpppj의 파일/클래스·호출 순서·부작용·정수 폭·float32 저장·목록·난수·PATCH 50바이트 슬롯 배치를 보존한다. 플랫폼 장치는 .NET/MonoGame으로 대체하고 OS 전용 동작은 호스트에 둔다. cpppj 미복원·보류 경계는 명시 진단으로 드러낸다. CD 10.72 제품 복원은 cpppj 범위다.
- **D1(사용자 결정).** cpppj HEAD의 요구사항 코드가 공통 파일에 섞여 있으면 함께 옮긴다. 기본 영어·보통·75 FPS·4:3 창 모드에서 원본 동작을 먼저 대조하고 후순위 기능은 R8에서 검증한다. 파일 장치는 사용자 루트 정책을 초기 이식부터 적용한다.
- **D2(사용자 결정).** 새 경로가 `Netstorm.exe`를 이어받고 완료 후 기존 경로를 제거한다. 적용 시점인 R5 전환/R6 삭제는 기존 에이전트 해석이다. 기존 경로의 `settings.json`은 새 경로로 가져오지 않는다. 근사 검사는 해당 코드와 함께 은퇴시키고 L1 대조로 확보한 검사는 새 모듈에서 잇는다.
- **D3~D5(사용자 결정).** R8에서 로컬 MCP와 한국어·일본어 화면을 검증한다. Linux는 `$HOME/.netstorm/` 아래에 저장한다. Template 판독·433 Fort 왕복 검사의 기존 결정은 [Template 계획](docs/dotnet-template-storage-plan-20261101.md)에 있다.

<a id="original-parity-plan"></a>

### 2.1 원본과의 의도적 차이(9-1 결정)

같은 입력의 원본 근거를 확보해 동작 차이를 판정한다. 현 구조로 원본 경험을 재현할 수 있으면 유지하고 필요하면 구조를 바꾼다. [2026-10-10 결정과 A~F 상세](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)

| 항목 | 완료 범위와 다음 작업 |
| --- | --- |
| A 시계·고정 틱 | 프레임 명령·붕괴 스캔·정지 시각 첫 단계 완료. 나머지 정수 틱·보간·대본/브리핑 지연·틱 기반 검사 이전은 R4/R6. [첫 단계](docs/dotnet-frame-parity-20261011.md) |
| B 입력 큐 / E 부분 다시 그리기 | 40칸 입력 큐 본체는 R1 완료, 실제 사건 소비는 R6. dirty 영역 표시 경로는 R5. |
| C 설정 / D 프레임 대기 / F 실행 환경 | 사용자 루트 `options.cfg` 저장 시점과 전체화면 재실행, 양보 대기 프레임 시간, 중복 실행·창 크기 등 관측 동작은 R7에서 판정한다. |

<a id="dotnet-r8-requirements"></a><a id="agents-reflection"></a>

## 3. AGENTS 4~15차 — R8 반영 범위

현행 AGENTS 변경은 갈무리11 지정 `2c8ca3bf8`, 15차 추가 `faecb9cbd`, 실행 파일 아이콘 `d047bf9a3`([0.4](#exe-icon)), webpj favicon `11b16b778`(DN 해당 없음)까지 확인했다. 아래는 새 경로 기준이다.

| 차수 | 처리 및 검증 |
| --- | --- |
| 4 로컬 MCP·와이드 | stdio JSON-RPC로 모든 조작·상태 조회·화면 캡처·소리 포함 녹화. 4:3·16:9·16:10, 16:9 최대 1920×1080의 논리 해상도와 그래픽 크기 유지. [MCP](docs/exe/cpp-mcp-server-20261106.md) · [와이드](docs/exe/cpp-wide-resolution-20261106.md) |
| 5 사용자 데이터 | Windows `%USERPROFILE%\Saved Games\Netstorm\` 아래 모든 런타임 생성물. 설정·진행은 `Saves/`, 맵은 설치 경로와 `CustomMaps/` 양쪽 읽기, 새 Edit 맵은 `CustomMaps/` 저장. 목록 무제한 스크롤과 읽기 전용 설치 확인. [cpppj 사용자 루트](docs/exe/cpp-user-data-root-20261105.md) |
| 6 한국어 | 외부 갈무리11과 한국어 명칭 규칙. 표시용 부호화·`DisplayTextInput`의 선행 이식을 화면·번역과 연결한다. [전환 체크](#korean-font-change) |
| 7 오류·로그 | 공통 오류/예외 처리와 상세 로그(`logs/yyyyMMddHHmmss.log`), 시작 실패부터 추적 가능해야 한다. [cpppj 로그](docs/exe/cpp-error-log-20261111.md) |
| 8 AA·FSR 1.0 | Options에서 선택, 둘 다 기본 OFF. 지원 환경의 2560×1440·3840×2160은 내부 1920×1080에서 자동 FSR. [cpppj 출력 계약](docs/exe/cpp-aa-fsr-20261106.md) |
| 9 시나리오 Edit | 새 맵 기본 `.english` 자동 생성, 누락 시 Test Battle 종료 오류 교정, 편집 UI에서 초기 SP·테크·플레이어·동맹 수정. [cpppj 편집](docs/exe/cpp-scenario-s3-ui-20261111.md) |
| 10 알려진 버그 | cpppj의 34건 판정과 수정 10건을 해당 파일의 이식·검증에 반영하고 NB 번호를 기록한다. [판정표](docs/exe/cpp-known-bugs-status-20261111.md) |
| 11 난이도 | Options 행과 AI 배율 부품 선행. 기본은 보통(원본), 쉬움은 적 AI의 다리·생산 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 적 AI의 주기 ×½. 동맹 판정 포함. [cpppj 난이도](docs/exe/cpp-difficulty-d-20261111.md) |
| 12 스커미시 | Campaign에서 시작/취소, 적 수·AI별 난이도/동맹, 금지 유닛 최대 5개(Outpost 포함). 멀티플레이 규칙의 임의 섬·간헐천. [cpppj 스커미시](docs/exe/cpp-skirmish-sk-20261111.md) |
| 13 일본어 | 외부 Noto Sans JP, 일본어 UI와 본문 영어 대체. 번역 범위는 cpppj D3. [cpppj 일본어](docs/exe/cpp-japanese-l1-l3-20261111.md) |
| 14 QuickJS | quickjs-ng 엔진 탑재·자체 검사·라이선스 동봉. 추가 연동은 별도 사용자 결정 대상이다. [cpppj 엔진 계약](docs/exe/cpp-quickjs-engine-20261111.md) |
| **15 Reset All** | **미착수.** Options 맨 아래에 Reset All을 제공한다. 취소 시 현재 설정·진행을 보존하고 확인 시 `options.cfg`를 포함한 설정·게임 진행을 초기화한 뒤 종료한다. 다시 실행해 최초 상태를 검증한다. 2026-11-19 소스 확인 기준으로 cpppj에도 아직 기준 구현이 없다. [AGENTS.md](AGENTS.md) |

**공통 완료 사항.** 기존 배포의 `Netstorm.exe` 이름과 압축 스크립트, 11.01·Credits 3항목·HJOW/AI 크레딧 및 카피라이트(0.150·0.342), 폐기 클라이언트 유효성 검사 비활성·외부 접속 문서화(0.151). 새 소비자가 생기면 외부 접속 목록을 갱신한다. `Netstorm.exe`의 원본 아이콘은 아직 미완료다([0.4](#exe-icon)). [배포 스크립트](dotnetpj/package-dotnetpj.ps1) · [외부 접속](docs/dotnet-external-endpoints-20261103.md)

<a id="resync-scope"></a><a id="cpppj-requests"></a><a id="dotnet-inventory"></a>

## 4. cpppj 동기화와 이식 목록

- **cpppj 1322가 현행 제품.** 0.606의 오라클 재빌드 기준도 1322다. 마지막으로 문서화한 변경 분류는 0.530의 1312·1313이다. `979d65b5d..HEAD`의 `cpppj/src` 변경 23파일 중 PORT_MAP과 겹치는 것은 7파일이다(2026-11-19 확인). 겹치는 파일은 `BitmapFont.cpp/.h`(1322 글꼴), `ClientMain.cpp`, `LocalTcpTransport.cpp`·`LocalUdpTransport.cpp`·`LocalMissionTransport.cpp/.h`(1320 미션 전환)다. 해당 소비자를 옮길 때 재대조를 마친 뒤 PORT_MAP 기준 커밋을 올린다. 1314(최초 실행)는 0.2, 1322는 0.3과 함께 반영한다. [1312·1313 처분](docs/dotnet-ai-alliance-delta-20261115.md) · [1320](docs/exe/cpp-mission-transition-1320-20261117.md) · [cpppj 현행](LEFT_JOBS.md)
- **기존 재대조 규칙.** CPP 소스는 커밋된 HEAD를 사용한다. 해당 소비자를 옮길 때 바뀐 cpp 소스·fixture·검사를 함께 확인하고, cpppj 테스트 자동 플레이와 관전 경계는 R5/R6에서 잇는다. [오라클 자동 플레이](docs/testing/cpp-autoplay-test-guide.md)
- **현재 집계(2026-11-19).** PORT_MAP 물리 1,043줄(데이터 1,038행, L1 467·port 571), 연결 fixture 329개(`cpp_sync.py` 종료 0). 현행 cpppj 1,239개 소스 단위 중 448개 대응·791개 미대응(89,775줄). 대응 수는 `tools/port_inventory.py`가 인식한 단위 기준이다. 기존 대응 파일의 미이식 분기·호스트 결선은 별도 작업이다.
- **파일 계열 목록.** 현행 [PORT_MAP](dotnetpj/PORT_MAP.tsv)과 `python -X utf8 tools/port_inventory.py --families dn`을 사용한다. 예전 단계 배정 및 대응 파일의 미완 부분은 [보관본 2.4~2.7](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에서 찾는다. 단위 작업은 해당 계열이나 큰 파일 하나를 옮기고 검사·호스트 결선까지 확인한다.
- **cpppj 전달 항목.** 상태 덤프와 고정 시각 요청은 해결됐다. fixture 헤더·모듈 연결 상태표, `GraphTests` 슬롯 열 범위와 `ClearProcesses` 뒤 정기 조회 진단의 이전 요청은 보관본 1.4에 있다. 변경이 필요하면 사용자에게 전달한다.

<a id="verify-levels"></a><a id="user-checks"></a><a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a><a id="dotnet-commands"></a><a id="dotnet-ops"></a>

## 5. 검증과 작업 규칙

- **L1** 원본 x86 fixture, **L2** 시작 상태 덤프, **L3** 고정 시계·입력열 뒤 상태 해시, **L4a** 논리 화면 픽셀, **L4b** 소리 시점·곡 선택, **L5** 자연 완주·저장·재실행. 파일 이식·집중 검사·실제 세션 결선·전체 일치를 각각 기록한다. `snapshot`과 `snapshot-output`을 구별하고 실행 근거에 SHA·시드·고정 시각·프레임·스크립트·옵션을 남긴다.
- **사용자 확인.** L5와 전체 화면/청취, 실제 사제 포획·희생·특정 적 공격·승리·MP/LAN, 공격형 공중 3종 실제 생산/조작은 사용자 확인 범위다. 창 실행은 AGENTS.md의 명시 확인·예외 PC 규칙을 따른다. Windows 10에서 통과하면 Windows 11 실기와 사용자 관찰만 남은 상태에서도 다음 단계로 진행한다. 물리 DPI·4K FSR·UAC는 R8 이후, `DSERR_NODRIVER` 실장치 재검사는 출력 장치가 있는 PC에서 한다. [소리 장치 기록](docs/dotnet-sound-device-20261114.md) · [공통 실행 범위](LEFT_JOBS.md#rules-verify)
- **공통 기능 경계.** CHEAT 메뉴는 Edit에서만 표시한다. 연녹색 직사각형 분석은 보류다. 폐기된 인터넷 master/root·검증·ping/진단은 비활성 상태를 유지한다. 도움말 링크와 LAN은 해당 사용자가 선택한 경로를 따른다. [공통 계약](LEFT_JOBS.md#rules-verify)
- **오라클과 자료.** `cpppj/build/bin/Release/Netstorm.exe`를 PowerShell에서 빌드한다. 실행은 `extracted/dotnet-oracle-data/` 사본과 분리한 사용자 루트로 진행하며 `originals/` 해시를 전후 확인한다. 0.606의 VM 소프트웨어 GPU 환경은 [재측정 기록](docs/dotnet-l2-0606-20261117.md)에 있다. 일반 덤프 비교와 최초 실행 검증의 옵션을 구별한다.
- **검사 명령.** `dotnet build dotnetpj/Netstorm.sln -c Release`, `dotnet test dotnetpj/Netstorm.sln -c Release`, `python -X utf8 dotnetpj/tools/cpp_sync.py`를 쓴다. 실제 TYPE 검사는 `NETSTORM_DATA`를 보호 자료 사본으로 지정한다. 2026-11-19 사용자 지시로 전체 회귀는 4단위당 1회만 수행한다(다음 전체 회귀는 0.640 시점). fixture는 사본 없이 한 소비자 프로젝트에 링크하고 `cpp_sync.py --update --add`로 매니페스트에 등록한다. GUI 명령·변이 검사·등록 규칙 전문은 보관본 6.1에 있다. [L2 도구](docs/dotnet-l2-batch-20261114.md) · [L4a 도구](dotnetpj/tools/l4a_compare.ps1) · [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md)
- **착수·보호·커밋.** AGENTS.md·LEFT_JOBS.md와 이 문서, git status/log를 확인한다. dotnetpj 이식에 디컴파일은 필요 없다. AGENTS.md·기존 원본/CD 파일·README 개요·기존 fixture를 보호한다. 작업 단위마다 진행판·체크와 단위 문서를 갱신하고 해당 파일만 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다. push 금지. 새 코드·문서는 UTF-8, AGENTS 범위의 코드 주석은 설명·사용법부터 한국어로 적는다. 작업 트리 줄바꿈은 본문 LF, 보관본 CRLF다.

<a id="dotnet-done-index"></a><a id="dotnet-unit-records"></a><a id="dotnet-archive-log"></a>

## 6. 완료 기록과 보관

- **0.546~0.600** 객체 수명 공통층과 미션 저장소 공유 완료. **0.601~0.608** 섬·지형·지원 요새 Pop과 그래프 후처리, 3경로 L2 비교. **0.609~0.613** 실제 MayPlace 본체·전수 대조와 농지 월드훅. **0.614~0.622** 농지 Generate 실결선과 지역 본체·세션 연결, 신전 가상 Owner 결선, 사원 Pop 파생, 3경로 L2 재측정. **0.623** `surrounded` 혼합 경로 첫 지역 9행 일치 대조. **0.624~0.630** 공장 후처리·정기·초기 레벨 적재와 `guardmyback` L2 대조. **0.631~0.636** 미션 틱 결정론, 매 프레임 바람·주변음, 간헐천·거주 구역·펜스 Pop과 정기. **0.637** 직접 AI 명령 상태 본체 이식과 원문 1940행 대조. **0.638** 직접 AI 명령 세션 결선과 세션 소유 brain 메모리. 단위별 근거는 `docs/dotnet-*.md` 및 Git 기록에 있다.
- **2026-11-19 이관.** 정리 직전 0.635 인계 전문을 보존했다. [0.635 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)
  - 옮긴 것: R4 체크의 단위별 `[x]` 문장 22건(0.601~0.635)을 R4-2·R4-3·R4-4 하위 목록의 완료 범위로 줄였다. 0절의 단위별 검사 수 나열과 L2 보관 경로 나열도 최근 기준선 요약으로 바꿨다.
  - 바로잡은 것: 0.631~0.635는 단위 문서상 R4-4 작업이므로 R4-4 완료 범위로 옮겼고, 머리·1절 단계를 R4-2·R4-4 병행으로 고쳤다. R4-2 잔여에서 끝난 지역 칠하기·소유자 세션 연결(0.619·0.621)을 뺐다. R4-4 설명에서 연결이 끝난 `UpdateAmbientWind`(0.632)를 완료로 옮겼다. 간헐천 세션 결선(0.634)도 1.2절에 반영했다. 마지막 전체 회귀를 0.566에서 0.630으로 정정했고, 4절 집계를 재집계했다(442→448 대응, fixture 324→329, PORT_MAP 1,031→1,043줄).
  - 새로 넣은 것: 미커밋 0.636 펜스 작업 상태, 15차 cpppj 기준 구현 부재, `979d65b5d..HEAD` 교집합 7파일.
- **2026-11-18 이관.** 정리 직전 0.613 인계 전문을 보존했다. 완료/중단 단위의 경과, 중복 체크, 이전 이식량·AGENTS 이력, 파일별 목록과 명령 예시는 보관본을 참조한다. [0.613 전체 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)
- **이전 이관.** [0.545 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)과 보관 파일 머리의 날짜별 색인을 참조한다. 아래는 이전 문서에서 참조하는 앵커다. 같은 이름의 상세는 보관본에 있다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
