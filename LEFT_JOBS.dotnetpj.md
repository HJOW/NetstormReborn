# LEFT_JOBS.dotnetpj — dotnetpj(C# + MonoGame) 현행 상태와 남은 작업

**기준 단위 0.523 · 정리 2026-11-14(HJOW-Athlon) · 다음 단위 0.524 · cpppj 기준 제품 1310(14차 QuickJS-ng 엔진 탑재는 1301) · dotnetpj의 마지막 cpppj 영향 검토는 제품 1247(`4d4b3a928`, 이후 1302·1303은 CD 10.72 전용이라 영향 없음) · 전체 목표 미완료 — 지금 어디까지 왔는지는 [0.1](#dotnet-overview), 남은 일은 [0.4](#now), 사용자 결정(D1~D5, 2026-11-11 확정)은 [0.5](#decisions)**

이 문서는 dotnetpj의 **현재 상태·남은 작업·계속 유효한 결정과 규칙**만 둔다. cpppj와 공통 사항은 [LEFT_JOBS.md](LEFT_JOBS.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 이번 정리(2026-11-12) 직전의 이 문서 전체(단위 기록 0.342~0.452 111건, 0.2~0.4의 단위별 검사·진행 나열)는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 바이트 그대로 보관했고, 그 이전 보관본 5개와 0.341 이하 단위 기록도 같은 파일에 있다. [이식 계획](docs/dotnet-cpp-port-plan-20261016.md)의 W 번호·fixture 목록은 근거 목록으로만 쓰고, 순서·구조·완료 기준은 이 문서를 따른다.

**읽는 순서:** 0.1(지금 어디까지 왔나) → 0.4(남은 작업, 특히 R4 진행판) → 0.5(확정된 결정) → 1절(cpppj 마감분 반영과 4~14차 대응표) → 필요할 때 나머지 절.

<a id="dotnet-overview"></a>

## 0. 한눈에 보기

### 0.1 진행 상황 — 실행·플레이 가능 여부와 원본과의 동일성

**한 줄 요약:** 새 경로는 **메뉴12장면이cpppj와픽셀까지같다.** 개발진입`--mission`으로세션·Player·동맹·Zone·지형/요새객체슬롯·그래프초기화를조립했고일반31미션의덤프를확보했다. **객체Pop후속·규칙·AI시작·월드표시가미연결이라새경로로플레이할수없다.** 0.500~0.503의 AI 부품 원본 대조에 이어 0.504에서 실제 Player Pop·기술부모·초기 Regular를 호스트에 연결했다. AI 생성은 결선 중이다. 현재플레이가능한것은동결된근사기존경로다.

AGENTS.md의 진행 요약 기준(실행·플레이 가능 여부, 원본과의 유사성)으로 평가한다. dotnetpj에는 실행 경로가 둘이다([2.1](#dotnet-current-plan-20261028) 결정 5).

| 경로 | 실행·플레이 가능 여부 | 원본과의 동일성 |
| --- | --- | --- |
| **기존 경로** `Netstorm.Game`<br>(`Netstorm.exe`, 현재 배포 대상) | **실행·플레이 가능.** 메인 메뉴와 캠페인 1-1·1-2를 근사 규칙으로 플레이할 수 있다. 한국어 선택·와이드 시야 확장·테두리 없는 전체화면이 있다. 새 기능을 넣지 않는 동결 상태다(실행 오류 수정만). | **낮다.** 녹화·팬게임 기반 근사(RGBA 합성, 자체 UI 배치, 근사 세션 `BattleSession`)라 원본 동일성의 기준이 될 수 없다. 새 경로가 같은 범위를 대체하면 삭제한다. |
| **새 경로** `Netstorm.Client` + `Netstorm.Client.Host`<br>(개발 호스트, 아직 배포 안 함) | **메뉴와 미션 진입 일부만 실행.** 타이틀, 메인 메뉴 8버튼, Help·Options(해상도·음량·난이도 펼침 포함)·Credits(11.01 포함), 캠페인 선택, Edit 맵 목록·새 맵 이름까지의 메뉴, 그리고 개발 진입 `--mission <키> --handle <이름>`(세션·Player·동맹·Zone·가입/Ready 조립). **지형·요새객체슬롯·그래프초기화는조립됐으나객체후처리·장부·규칙/표시가미연결이라플레이할수없다.** | **부품은 높지만 전체는 미확정.** cpppj를 구조 그대로 직역했고 원본 x86 fixture275개로부품을대조했다(PORT_MAP L1 379행). **화면(L4a): 1024×768 메뉴 12장면이 cpppj와 BMP 바이트 동일**(메인 메뉴·Credits Tell·Credits 항목·Help 팝업·About·Options 팝업·Resolution 펼침·Difficulty 펼침·Campaign 목록·Campaign 하위·Demo 목록·Edit 목록, 0.381~0.388). **상태(L2): 0.499에서cpp기준선34경로·dn일반31미션을같은시드로확보/대조했고전체일치0/34다(데모3개dn진입미연결). 아래4경로초기비교는0.455당시기록이다. cpppj 초기기준선4경로**(thewarbegins·masterofwhirligigs·savetheisland·fragilefortune)를 확보했으나 새 경로 world-1 덤프는 1,955행(슬롯 1,679·플레이어 9, 0.455)이라 cpppj의 수천 행(슬롯 1,807~2,946·객체·프로세스·장부)에 한참 못 미친다. SID 집합·개수·SetRegion·비슬롯 행은 바이트 동일하고 슬롯 내용은 Pop·공장체·프레임·소유자가 다르다(0.455·0.456 분석). 고정 입력열 비교(L3)는 0건. 부품 대조(L1) 통과를 화면·플레이 동일성으로 합산하지 않는다. |

**이식 범위(2026-11-11 집계):** PORT_MAP은 cpppj `src`의 2,303파일 중 584파일, 약 16.8만 줄 중 약 6.0만 줄(36%)을 가리킨다. 행이 있다고 파일 전체를 옮긴 것은 아니다(예: `RawCampaignSession`은 관문·소유 상태 위주로 일부만). PORT_MAP에 없는 나머지 약 10.7만 줄(1,719파일) 중 큰 묶음은 `client/Client*`(ClientMain 등) 6.6천·`RawCampaign*` 4.9천·`Native*`(LAN) 3.7천·`Uber*` 3.7천·`Sound*` 2.7천·`RawCursor*` 1.7천·`Game*` 1.7천·`Editor*` 1.1천·`Hud*` 1.0천 줄, `o/RawAi*` 4.1천·`RawConstruction*` 3.4천·`RawPriest*` 2.4천·`RawCarrier*` 2.2천·`RawGun*` 1.8천·`RawPath*` 1.7천·`RawMission*` 1.6천·`RawBomb*` 1.4천·`RawFence*` 1.1천 줄이다(R4 조립·R5 표시·R6 규칙 범위). 집계 방법은 [1.2](#resync-scope)와 같은 PORT_MAP `cpp` 열 대조다. 이후 0.394에서 `Player.cpp/.h`가 PORT_MAP에 더해졌고(`RawMissionPlayers.cs`, port 2행) 그 밖의 변화는 없다.

### 0.2 단계별 진행 (R0~R8)

| 단계 | 내용 | 이정표 | 상태 |
| --- | --- | --- | --- |
| R0 | 기준선·동기화(cpppj 오라클, PORT_MAP, `cpp_sync.py`) | — | ✅ 완료. cpppj 제품 1248~1300의 영향 재검토도 종결했다(0.379, 0.4 1-2) |
| R1 | 8비트 화면 호스트·입력 큐·오라클 도구(`--ui-script`·`snapshot`·`session-dump`·`--fixed-frame-ms`) | — | ✅ 완료 |
| R2 | 그리기 원시·팔레트·글꼴 | — | ✅ 본체 완료(실자산·세션 연결은 R4·R5) |
| R3 | Gump UI와 메인 메뉴 계열 | D1 메뉴 픽셀 일치 | 🔶 위젯·호스트 대부분 이식. **메뉴 L4a 12장면 차이 0**(0.381~0.388, 장면은 0.1). 남은 메뉴 장면 3건은 사유와 함께 뒤로 미뤘다(Language 자식 CJK는 R8, Multiplayer 섬 화면은 R5·3차, Edit 이동행은 R7). R3 잔여 위젯·소유자는 [2.5](#r3-r2-detail) |
| R4 | 월드 세션 조립 | M1 시작 상태 L2 | 🔶 월드 아래층(Template·Fort·지형·섬·Player·저장소·프로세스·월드 본체)과 F3 세션의 관문·소유 상태 대부분 완료. **실제 호스트 조립 진행 중(0.391~0.523, [R4 진행판](#r4-board)):** 미션 Fort 적재 → 세션 홀더 → Player·SP·AI·동맹 → Zone·가입·Ready → 이름·요새 순번 → 월드 적재 시작 흐름 → 타입·프레임 → 지형·난수 저장소 → 섬 부모 → 입력 결선 → 바인딩 조립·드라이런 → Generate 라이브·덤프 CRT 행·난수 단일 스트림·프리-Generate 소비 확정·프레임 선택 대조·후보 테이블 per-type 대조·SP 353 구성과 동기 순서 확정·SID 단위 per-cell 대조·reseed 시점 근본 원인 확정·호스트 시작 1회 reseed 적용·Pop 공간 등록 live 연결·요새 객체 live 적재·4경로 dn 덤프 구조 대조·소유자 귀속 확정·그래프 재구성 live 연결·장부 귀속 확정·전투섬 배정 귀속 확정·Player 첫 지역 127 초기화·전투섬 배정 region live 기록·Player 월드 갈고리 live 연결·그래프 ResetTable 설치·5경로 dn f090 HEAD 재생성·단일 미션 +0x78 기록자 정정과 섬 런타임 바인딩·6번째 경로(dissolvedalliance) 기준선과 덤프·7번째 경로(thenoose) 기준선과 덤프·8번째 경로(rainvsrain) 기준선과 덤프·9번째 경로(runforit) 기준선과 덤프·10번째 경로(guardmyback) 기준선과 덤프·11번째 경로(surrounded) 기준선과 덤프·담긴 섬 판정(12번째 경로 breakingthrough·13번째 경로 totherescue 기준선·덤프 포함 14건 전수 일치)·14번째 경로(vicious) 기준선과 덤프·담긴 섬 2건 추가 확인(무신전 127 잔류 재확인)·15번째 경로(enemyterritory) 기준선과 덤프·담긴 섬 6건 확인·16번째 경로(finalconfrontation) 기준선과 덤프·담긴 섬 5건 확인(3장 Complete Victory 완료)·17번째 경로(tutorial1) 기준선과 덤프·담긴 섬 1건 확인(Early Missions 진입)·18번째 경로(tutorial2) 기준선과 덤프·담긴 섬 1건 확인(중립 신전)·19번째 경로(tutorial3) 기준선과 덤프·담긴 섬 2건 확인(Early 3 Capture The Priest)·20번째 경로(tutorial4) 기준선과 덤프·담긴 섬 2건 확인(Early 4 Tactical Combat)·21번째 경로(tutorial5) 기준선과 덤프·담긴 섬 2건 확인(Early 5 Subtle Defense)·22번째 경로(tutorial6) 기준선과 덤프·담긴 섬 2건 확인(Early 6 Raw Power — Early Missions 6개 완료)·23번째 경로(bc1menu76) 기준선과 덤프·담긴 섬 6건 확인(Priest Training 첫 경로 Menu — AI 이름은 양쪽 일치·+0x78은 소용돌이 사슬 R6)·24번째 경로(bc2bridging76) 기준선과 덤프·담긴 섬 2건 확인(Priest Training Bridging — 3섬 중 유신전 2섬·중앙섬 무신전·그래프 row0 일치 첫 사례)·25번째 경로(bc3acid_bar76) 기준선과 덤프·담긴 섬 2건 확인(Priest Training Acid Bars — 공격 개시 중이라 cpp_only 규모 큼)·26번째 경로(bc4bow_rush76) 기준선과 덤프·담긴 섬 4건 확인(Priest Training X-Bows — 4섬·4사제·4참가자 구조)까지. **Pop 후속(postPop·표시)·농지 배치·월드 본체(그래프 나머지·장부 집계) 조립은 미착수** |
| R5 | 월드 표시·카메라·HUD | D2 진입 화면 픽셀 일치 | ⬜ 미착수 |
| R6 | 플레이 입력·규칙·소리·AI | M2·M3 | ⬜ 미착수 |
| R7 | 미션 전체·Edit·설정·배포 | M4 | ⬜ 미착수(Edit 목록·`options.cfg` 저장 일부는 R3에서 선행) |
| R8 | AGENTS **4~14차**, 60/120프레임, Linux, LAN·Outpost | — | ⬜ 미착수(검증·마감은 이 단계). 단 결정 D1에 따라 일부 요구사항 코드는 앞서 이식했다: 5차 `UserDataRoot`(0.359), 6차 확장 문자 부호화·`DisplayTextInput`(0.350·0.351), 7차 `ErrorLog`·호스트 배선(0.360·0.361), 11차 `AiDifficulty`(0.347), Options의 Difficulty·Resolution 행(0.384·0.387). 즉시 요구인 11.01 표기·Credits 3항목·외부 접속 문서화는 완료, **`Copyright 2026 HJOW` 줄은 0.342에서 반영**(0.4 1-3 완료). LAN은 `Netstorm.Net` 사전 구현 일부(검사 없음) |

### 0.3 숫자로 보는 현재

| 항목 | 값 |
| --- | --- |
| 마지막 구현 커밋 | **0.523: [현재Carrier/내용물조회](docs/dotnet-carrier-predestroy-current-20261114.md).** 사제월드조립검토에서현재Native/권한/진단차이를확인해먼저갱신했다. 원본2880입력·새9검사·실제사제삭제→Carrier연결·현재boss/checking/복귀head·개수계약, 변이3/9검출/원복·전체비증분빌드0경고0오류·관련Core527·Client32통과. 직전0.522는824b4ea2d(20261114-80 DN). 다음0.524는피해전처리의현행Native/현재TYPE·flatSpot·어댑터대조후실제사제월드묶음/HostSession이다. 일반지역정책은잔여다 |
| 검사 | **0.523: 원복뒤전체비증분빌드0경고0오류·관련Core527/527·Client32/32통과(실패0·건너뜀0·각종료0). 원본2880입력·새9검사·현재boss변이3/9검출/바이트원복.** **0.522: 원복 뒤 전체 비증분 빌드0경고0오류·관련Core499/499·Client32/32통과(실패0·건너뜀0·각종료0). 원본3974입력·새Core3/Client7·signed count2/3/우선순위6/7변이검출/바이트원복.** **0.521: 원복 뒤 전체 비증분 빌드0경고0오류·관련Core429/429·Client21/21통과(실패0·건너뜀0·각종료0). 원본34200입력·새19검사·마스크변이3/19검출/바이트원복.** **0.520: 원복 뒤 전체 비증분 빌드0경고/0오류·관련Core410/410·Client21/21통과(실패0·건너뜀0·각종료0). 원본20039입력·새24검사·배열전치 변이6/24검출/바이트 원복.** **0.519: 관련Core386/386·Client21/21, 빌드0경고0오류·원본20398입력·새21검사·소유자비교 변이8/21검출/원복.** **0.518: 변이 바이트 원복 후 전체 비증분 Release 빌드0경고/0오류·관련Core279/279·Client21/21 통과(실패0·건너뜀0·각종료0). 원본 생성1236+Carrier PATCH676입력 일치, 실제 해방/서버 공장/소유자·현재 Carrier와132후보 후 checkingDWORD 분기 검증. spot 재조회 변이3/8실패 검출·SHA까지 원복. 실제 게임 실행 없이 부품/합성만 검증했다.** **0.517: 원복 뒤 제품 전체 비증분 Release 빌드0경고/0오류. Client일반전체1193 통과. Core전체2076중2075통과/과거 World 예외형 기대1실패는 현행 진단 계약으로 교정하고 실제 World 현재 표/정상복귀 부분효과3건 추가 뒤 최종 관련Core232/232·Client21/21 통과(실패0·건너뜀0·각종료0). 검사만의 빈 저장소 입력은 실제 자산 저장소로 교정/재빌드0경고0오류. 새원본7632입력·기존Owner1000입력 일치, 모드고정변이2/7검사 실패검출/바이트원복. 2미션 cpp/dn4회 단순 진입 종료0, originals1394파일 해시 동일·dn내용0.512와frame행 제외동일.** **0.516: 원복 뒤 전체 비증분 Release 빌드0경고/0오류·Client일반전체1192·메타데이터6 통과(실패0·건너뜀0·각종료0). WAVE원본1305입력·실제WAV218개 표본 일치. bits보정 생략 변이3/8실패 검출·SHA까지 원복. 실장치 Explicit1건은 DirectSoundCreate 0x88780078로 실패, cpp동일 검사도 초기화 실패(활성 출력endpoint 부재). 실재생 완료로 세지 않는다.** **0.515: 원복 뒤 전체 비증분 Release 빌드0경고/0오류·Client전체1184·메타데이터6 통과(실패0·건너뜀0·각종료0). 원본23788입력·실제 보호막/공간/프레임/Kernel/소리 수명과 현재 뷰/TYPE 재조회 합성 검증. 현재 뷰 고정 변이3/12검사 실패 검출·SHA까지 원복.** **0.514: 원복 뒤 전체 비증분 Release 빌드0경고/0오류·최종관련Core312·Client전체1172 통과(실패0·건너뜀0·각종료0). PATCH 사제상태6752+낙하456입력 일치. 실제 공유Form/Kernel·보호막 생성/조회/삭제·권한 재조회·프레임 어댑터 합성 통과. 권한 선행고정 변이2/7검사 실패 검출/바이트 원복.** **0.513: 비증분 Release 빌드0경고/0오류·최종관련Core305·Client전체1172 통과(실패0·건너뜀0). 원본1208입력 일치·첫확보 조기반환 변이2/6검사 실패 검출/원복. 첫 관련211중1실패는 과거 미연결Player 공통성공 기대를 현행cpp 명시실패로 교정했다.** **0.512: Assets366·Client1172 통과, Core2054중2053통과/유일한PORT_MAP 오타1실패는 수정 후 metadata6/6 통과로 해소했다. 제품코드 동일 빌드, 변경된 메타데이터만 재대조했다.** **0.511: 원복 뒤 전체 비증분 Release 빌드0경고/0오류·관련Core438(새부모/manifest40포함)·Client호스트116 통과(실패0·건너뜀0·각종료0). 원본좌표260입력·CRT9입력 일치. 깊이/선행SHP 변이12/34검사 실패 검출·원복.** **0.510: 변이 원복 후 전체 비증분 Release 빌드0경고/0오류·관련Core404·Client전체1172 통과(실패0·건너뜀0). 원본28800관측 일치, modulo변이5944관측 불일치 검출·원복.** **마지막 전체회귀 0.509: Assets366·Core1994·Client1172=3532건, 실패0·건너뜀0·종료0. 전체 비증분 Release 빌드0경고/0오류. 원본8448관측·실제 합성·대표변이 검출/원복 완료.** **0.508: 3판본원본7344입력·현재부모13계약·관련자산 포함집중64건 통과. 사전 검사 복귀 변이13/13 검출·원복 뒤 전체 비증분 빌드0경고/0오류.** **0.507: PATCH원본2048입력·일반Pop/장부/실제HP 공유 검증. 변이120/2048 검출·원복 후 전체 비증분 빌드0경고/0오류·관련집중34건 통과.** **0.506: PATCH원본2392입력·실제HP/목록/조회/커널 통과. 변이356/2392 검출·원복 후 전체 비증분 빌드0경고/0오류·관련집중24건 통과.** **0.505: 전체 비증분 빌드0경고/0오류·원복 뒤 관련집중27건 통과·원본5067입력 일치·대표변이 검출.** **0.504: Client23·관련Core134·맵/manifest6 통과, x+1 좌표 변이 검출·원복 후Client23 재통과.** **이전 전체회귀 0.499: Assets366·Core1945·Client1171=3482건, 실패0·건너뜀0·종료0(2026-11-14 종결). 0.500은 AI본체 원본160입력+수명 포함 집중14건 통과, 변이160/160검출·원복 후 재검증. 0.501은Player기술158입력·실제TYPE포함집중18건통과·변이42/158검출. 0.502는AI초기갱신149입력포함집중17건통과·변이기본67/133+theme5 5/16검출.** **그 이전 전체 회귀 0.453:** Assets 366·Client 1,130·Core 1,945(합계 3,441, 실패 0, 새 기준선). 전체 완주 이력은 0.345(3,273) → 0.369(3,375) → 0.381(3,389) → 0.432(3,422) → 0.453(3,441)이다. 0.454는 Client HostTerrain* 10건, 0.455는 Client 전체 1,134건+Core 지형 52건, 0.456은 Client Host·Dump 69건+SessionStateDump 11건, 0.457은 Client 전체 1,133건+Core 포팅 111건, 0.459는 Client Host·Dump 24건, 0.460은 Core RiftType·FrameChoose·AmbientWind·RawTerrain 33건, 0.461은 Core SpStore·FortMoney·SetPlayerSp 12건, 0.464는 Client 전체 1,134건(신규 공유 SP 1건 포함)+paired 라이브 재실행(예측 일치) 통과(실패 0, 0.458·0.462·0.463·0.465는 조사 단위라 별도 집중 없음 — 0.462·0.464·0.465·0.466·0.467·0.468은 paired 라이브/디스크 대조). 0.465는 revert 확인용으로 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,134건 통과(실패 0). 0.466은 빌드(경고 0·오류 0) 뒤 Client 전체 1,139건(신규 Pop 5건 포함, 실패 0)+paired 라이브 재실행(예측 일치) 통과. 0.467은 빌드(경고 0·오류 0) 뒤 Client 전체 1,144건(신규 FortObject 5건 포함, 실패 0)+paired 라이브 재실행(구조+결정성 일치) 통과. 0.468은 게임 코드 무변경(검사 2건 추가·측정 단위)으로 Client 전체 1,146건(신규 순서·null 프로브 2건 포함, 실패 0)+4경로 paired 구조 대조 통과. 0.469는 제품 코드 무변경 조사 단위로 새 집중 검사 없이 paired 디스크 대조로 소유자 귀속 확정(0.468 게이트 1,146건 유지). 0.470은 빌드(경고 0·오류 0) 뒤 Client 전체 1,149건(신규 그래프 3건 포함, 실패 0)+paired 라이브 재실행(난수·구조 중립, graph 120/255) 통과. 0.471은 제품 코드 무변경 조사 단위로 새 집중 검사 없이 paired 디스크 대조+소스 추적으로 장부 귀속 확정(0.470 게이트 1,149건 유지). 0.472는 제품 코드 무변경 조사 단위로 새 집중 검사 없이 paired 디스크 대조+소스 추적으로 전투섬 배정·+0x78 귀속 확정(0.471 게이트 1,149건 유지). 0.473은 빌드(경고 0·오류 0) 뒤 Client 전체 1,151건(신규 첫 지역 2건 포함, 실패 0)+paired 라이브 재실행(9기록 +0x78 전부 127 확인, Game·CRT·슬롯 구조 중립) 통과. 0.474는 빌드(경고 0·오류 0) 뒤 Client 전체 1,161건(신규 region 갈고리 10건 포함, 실패 0)+변이 검출 확인(선택 +1 변이에 신규 2건 실패 후 원복·재통과)+paired 라이브 재실행(p1 +0x78 127→0으로 cpp와 일치, 나머지 8기록 127 유지, 덤프 2,129행 중 player 1행만 변경·Game·CRT 중립) 통과. 0.475는 빌드(경고 0·오류 0) 뒤 Client 전체 1,168건(신규 Player 월드 7건 포함, 실패 0)+변이 검출 확인(소유자 +1 변이에 신규 2건 실패 후 원복·재통과)+paired 라이브 재실행(아바타 슬롯 1개만 공장 생성·소유자 기록으로 변경, SID 번호·Game·CRT·행 수 중립) 통과. 0.476은 빌드(경고 0·오류 0) 뒤 Client 전체 1,169건(신규 ResetTable 2건·기대값 정정 1건 포함, 실패 0)+변이 검출 확인(엔진 ResetTable 주석화에 신규 1건 실패 후 원복·재통과)+5경로 paired f090 재생성(thewarbegins·masterofwhirligigs·savetheisland·fragilefortune·thunderingpower 전부 종료 코드 0, Game·CRT·행 수 중립) 통과. 0.477은 빌드(경고 0·오류 0) 뒤 Client 전체 1,171건(신규 섬 첫 지역 2건 포함, 실패 0)+변이 검출 확인(바인딩 주석화에 신규 2건 실패 후 원복·재통과)+paired thewarbegins f090 재실행(p1 0→127 복원 외 2,129행 동일·슬롯·그래프·Game·CRT 중립) 통과. 0.478은 제품 코드 무변경 측정 단위로 새 집중 검사 없이 6번째 경로(dissolvedalliance) cpp 기준선·dn 덤프 생성과 5→6경로 구조 대조로 범위 확정(0.477 게이트 Client 전체 1,171건 유지 — 코드 무변경이라 재완주 없이 인용). 0.479는 제품 코드 무변경 측정 단위로 새 집중 검사 없이 7번째 경로(thenoose) cpp 기준선·dn 덤프 생성과 6→7경로 구조 대조로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.480은 제품 코드 무변경 측정 단위로 새 집중 검사 없이 8번째 경로(rainvsrain) cpp 기준선·dn 덤프 생성과 7→8경로 구조 대조+신전 파일 순서 8경로 대조로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.481은 제품 코드 무변경 측정 단위로 새 집중 검사 없이 9번째 경로(runforit) cpp 기준선·dn 덤프 생성과 8→9경로 구조 대조+신전 순서 3번째 역순 확인으로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.482는 제품 코드 무변경 측정 단위로 새 집중 검사 없이 10번째 경로(guardmyback) cpp 기준선·dn 덤프 생성과 9→10경로 구조 대조+신전 순서 4번째 역순·5인 미션 확인으로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.483은 제품 코드 무변경 측정 단위로 새 집중 검사 없이 11번째 경로(surrounded) cpp 기준선·dn 덤프 생성과 10→11경로 구조 대조+신전 없는 소유자 127 잔류 확인으로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.484는 제품 코드 무변경 측정·조사 단위로 새 집중 검사 없이 담긴 섬 판정 실험(신전 10건 전수 일치)과 12번째 경로(breakingthrough) cpp 기준선·dn 덤프 생성·11→12경로 구조 대조로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.485는 제품 코드 무변경 측정 단위로 새 집중 검사 없이 13번째 경로(totherescue) cpp 기준선·dn 덤프 생성·12→13경로 구조 대조와 담긴 섬 4건 추가 확인으로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 0.486은 제품 코드 무변경 측정 단위로 새 집중 검사 없이 14번째 경로(vicious) cpp 기준선·dn 덤프 생성·13→14경로 구조 대조와 담긴 섬 2건 추가 확인(무신전 잔류 재확인)으로 범위 확정(0.477 게이트 Client 전체 1,171건 유지). 단위별 집중 검사 내역과 변이 검출 기록은 8절과 [보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 있다 |
| PORT_MAP | `PORT_MAP.tsv` 793행(L1 379·port414, 0.523 Carrier헤더1행추가·현재3행재대조). 행은 파일 전체 완료를 뜻하지 않는다(SquidPostPop 목록 부분, SoundDevice 음악 경계와 RawCameraView 투영 외 부분 잔여). 이전 이력은 해당 단위와 보관본에 있다 |
| fixture | 연결275개(0.523 Carrier전처리2880입력; 0.522 사제정기384·이동선택3590=3974입력; 0.521 일반/사제후보충돌34200입력; 0.520 기하10319·미리보기9720=20039입력; 0.519 일반/사제배치6384·전체픽셀모양14014=20398입력; 0.518 priestspawn1236·carriercheck2028행 중PATCH676입력; 0.517 owner-current-local72·priestowner7416·priest-owner-current-local144입력; 0.516 sounddevice세판본1305입력; 0.515 soundprocess18381행 중PATCH6127·soundplay세판본14472·soundcontrol세판본3189입력; 0.514 prieststate20256행 중PATCH6752입력·priestfall1368행 중PATCH456입력; 0.513 forcefieldpostpop3624행 중PATCH1208입력; 0.511 pop-world-bounds260입력; 0.510 frameadvance23040관측; 0.509 priestforcefield10416행 중5952관측·priestshield2496관측; 0.507 priestpostpoptail6144입력 중PATCH2048입력; 0.506 priestpostpop7176입력 중PATCH2392입력; 0.505 priestlookup4851입력·player-color216입력; 0.503 type-required-energy2353행·2352입력; 0.502 ai-initial-update134행·133입력, theme5추가16행·16입력; 0.501 Player기술158입력, 0.500 AI생성160입력). 직전 연결 236개(0.381에서 `kencloud-v2-x86.tsv` 246행·`kencloud-render-x86.tsv` 944행을 Core에 더했다). `cpp_sync.py`는 미반영분 때문에 0종료가 아니다. `reflected-commit`·PORT_MAP floor는 `f95a8db2` 그대로다(영향 재검토를 마친 행만 올린다) |
| cpppj 기준 | 현재 cpppj는 제품 1310 / 11.01(M8 단위1311 완료, CD복원 사용자 보류)([LEFT_JOBS.md](LEFT_JOBS.md) 0.1). dotnetpj의 마지막 영향 검토는 0.281(제품 1247, `4d4b3a928`)이고 재검토 범위는 0.379에서 종결했다. 그 뒤 `cpppj/src` 변경은 1302·1303(CD 10.72 전용)뿐이다([1.2](#resync-scope)). 14차 엔진 본체(`ScriptEngine`·`third_party/quickjs-ng`)는 R8에서 옮긴다 |
| 디컴파일 | 마지막 정밀 디컴파일은 HJOW-Athlon 2026-11-03(R 전용 11-07, 같은 호스트)이다. dotnetpj 이식 근거는 cpppj 소스·fixture·문서라 디컴파일이 필요 없다. 산출물을 직접 읽어야 할 때만 AGENTS.md대로 호스트를 확인한다 |

<a id="now"></a><a id="09-바로-다음-작업"></a>

### 0.4 남은 작업 — 순서대로

체크 대상은 AI 작업이다. 자연 완주·전체 동일성·Windows 11 실기는 AI 작업이 아니며 단계 전환을 막지 않는다([2.3](#stage-criteria)).

| 순서 | 작업 | 할 일과 완료 기준 | 시작 조건 |
| --- | --- | --- | --- |
| **1-1** | **전체 회귀 재완주** | ✅ 0.453에서 완주: Assets 366·Client 1,130·Core 1,945(합계 3,441, 실패 0, 새 기준선. `NETSTORM_DATA=assets/game-data/` 지정, 종료 코드 0) | 완료 |
| **1-2** | **cpppj 영향 재검토(제품 1247→HEAD)** | ✅ 게임 실행 없는 범위 종결(0.379): 31항목 전수 처분(완료 22·부분 1·미착수 8). 정적 분류 0.343, 재대조·신규 이식 0.344~0.367, 구간 대조 0.368·0.370~0.378. 그 뒤 cpppj 변경 1302·1303은 CD 10.72 전용이라 영향 없음으로 정적 확인했다(2026-11-12, [1.2](#resync-scope)). **남은 9건**은 해당 단계에서 파일을 건드릴 때 HEAD 기준으로 처리한다: 1번 `main.cpp`(부분 — 14차 QuickJS 3종 잔여, R8) · 2번 `BitmapFont.cpp`(일본어 글리프, R2·R8 13차) · 3번 `ClientMain.cpp`(호스트 루프, R1·R8) · 12번 `RawCampaignSession`(F3·R4, 난이도·스커미시는 R8) · 13번 `RawMenuControls`(R3) · 16번 `RawSurfaceStorage`·17번 `RawSurfaceWorld`(R4·R6) · 30번 `SquidPostPop`(R4) · 31번 `LocalTcp/UdpTransport`(10단계). 근거: [재검토 문서](docs/dotnet-cpp-resync-20261111.md) | 완료(병행) |
| **2** | **R4 마무리 → M1** | 🔶 **진행 중.** 아래 [R4 진행판](#r4-board)이 현재 위치와 남은 조각·순서다. 완료 기준(M1)은 34경로의 Load/Start 뒤 상태가 cpppj `session-dump`와 같은 것(L2). 세부 범위는 [2.4](#r4-detail) | 1-1과 병행 |
| **3** | **R3 마무리 → 이정표 D1** | 🔶 메뉴 L4a는 12장면 차이 0으로 사실상 끝났다(0.381~0.388, 장면은 0.1). 남은 것: 미룬 메뉴 장면 3건(Language 자식 CJK는 R8, Multiplayer 섬 화면은 R5·3차, Edit 이동행 선택은 R7)과 R3 잔여 위젯·소유자([2.5](#r3-r2-detail)). 메뉴 기본 실행 교체는 4단계에서 미션과 함께 한다(결정 D2) | 2와 병행 |
| 4 | R5 → 이정표 D2, **`Netstorm.exe` 전환** | 월드 표시·카메라·HUD. 34경로 진입 직후와 N프레임 뒤 캡처 차이 0. 이 시점에 메뉴·미션 기본 실행을 새 경로로 바꾸고 `Netstorm.exe` 이름과 배포 대상을 새 호스트로 넘긴다. 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 두고 배포에서 뺀다(결정 D2) | 2·3 뒤 |
| 5 | R6 → M2·M3 | 입력·선택·커서, 게임 메뉴·우클릭 메뉴, 규칙(사제·운반·건설·생산·전투·의식·미션)·AI·소리. 고정 입력열 L3 상태 해시와 대표 장면 캡처 일치. 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로(`NetstormLegacy`)와 그 검사를 삭제한다(결정 D2·결정 5) | 4 뒤 |
| 6 | R7 → M4 | 34경로 전체·데모 반복·진행 저장·Retry/Next/Abort, Edit 전체, `options.cfg`(9-1 C·D·F 판정), 배포 스크립트 갱신과 압축 해제 실행 확인 | 5 뒤 |
| 7 | R8 — AGENTS 4~14차 | [1.3](#dotnet-r8-requirements) 표의 차수별 할 일. 결정 D1에 따라 요구사항 코드 상당 부분이 2~6 단계에서 함께 들어오므로 R8은 요구사항 전용 모듈(MCP·FSR 출력기·오류 로그·스커미시 화면·시나리오 편집기·외부 글꼴 출력·14차 QuickJS 엔진 탑재) 이식과 요구사항 검증이 중심이다. 14차는 cpppj가 1301에서 엔진 탑재(결정 D7 ① 범위)까지 끝냈으므로 같은 계약으로 옮기고, 연동 범위는 cpppj D7을 따른다 | 6 뒤 |
| 8 | 60/120프레임 | cpppj F1·F2(옵션 `Frame Rate` 60/75 원본/120·양보 대기). 비행체 표시 보간은 cpppj D2를 따른다 | 7 뒤 |
| 9 | Linux(DesktopGL) | 설정·진행 저장·커스텀 맵·로그는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`(결정 D5). Windows 전용 동작은 호스트 OS 분기에만 둔다 | 8 뒤 |
| 10 | TCP/IP 로컬 멀티플레이·Outpost | cpppj 3차(M1~M7)를 로컬 네트워크만으로 옮긴다. MP 실행 검사는 cpppj처럼 사용자 확인으로 둔다 | 최후순위 |
| — | 새 제보 처리 | 사용자가 오류·원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다 | 제보 시 |

**완료한 준비 작업(참고):**

- **11.01 카피라이트 ✅ 0.342:** 새 경로 `Netstorm.Client/ProductPresentation.cs`의 `PatchCreditsBody`와 기존 경로 `MainMenuView`의 11.01 Credits 상세에 `Copyright 2026 HJOW`를 넣었다. 기준은 cpppj `client/ProductPresentation.cpp`(제품 1291)다.
- **오라클 갱신 ✅ 0.380:** 오라클은 `cpppj/build/bin/Release/Netstorm.exe`(2026-11-11 11:20:25 빌드, SHA-256 `09042F35637459793784D46D4489B524D8E412C52294F7640FFFFDE5B2A8E996`, 제품 1301 소스와 같은 빌드)다. 데이터 사본 `extracted/dotnet-oracle-data/`(`originals/` 1,394파일 + 클론 `cursors/`)와 `originals/` 해시 기준선 `extracted/dotnet-r0-baseline/originals-hash-before-0380.txt`는 이 PC에만 있다(Git 제외). 0.382 이후의 L4a와 R4 기준선(0.390~)은 CD 10.72 변경(`93a589636`·`b833ecb51`)만 더한 11-12 재빌드 오라클로 실행했고 메뉴·미션 경로에 영향이 없어 비교가 유효하다. cpppj `src`가 더 새로 바뀌면 PowerShell로 다시 빌드한다.

<a id="r4-board"></a>

#### R4 진행판 — 실제 호스트·월드 조립 (순서 2, 0.390~0.523)

M1(34경로의 Load/Start 뒤 상태를 cpppj `session-dump`와 대조)까지의 현재 위치다. **0.499 갱신: 현재 cpp 기준선34/34, dn 일반 미션31/31 확보·대조, 데모3개는 StartDemo 미연결, 전체 일치0/34다. [같은 시드 일괄 수집 결과](docs/dotnet-l2-batch-20261114.md)가 아래 과거 경로별 집계보다 우선한다. 기준선 수집 반복은 끝났으며 다음은 실제 생성자·세션 결선이다.** **cpppj 기준선 26/34(M1 판정용 25/34+thunderingpower 구조 참조), 새 경로 전체 일치 0/34.** 새 경로의 world-1 덤프는 thewarbegins 기준 2,129행(슬롯 1,853, 0.467)이며, 0.468에서 baselined 4경로 중 나머지 3경로의 첫 실제 dn 미션 덤프(f090/f240)를 생성했고(masterofwhirligigs 1,743·savetheisland 2,618·fragilefortune 2,719슬롯, thunderingpower 구조 참조 2,410슬롯 별도), 0.476에서 5경로 dn f090을 HEAD로 재생성하고 0.478에서 dissolvedalliance(3,288슬롯), 0.479에서 thenoose(2,643슬롯), 0.480에서 rainvsrain(1,931슬롯), 0.481에서 runforit(2,068슬롯), 0.482에서 guardmyback(3,415슬롯), 0.483에서 surrounded(6,238슬롯), 0.484에서 breakingthrough(3,177슬롯), 0.485에서 totherescue(4,139슬롯), 0.486에서 vicious(2,052슬롯), 0.487에서 enemyterritory(6,171슬롯), 0.488에서 finalconfrontation(4,890슬롯), 0.489에서 tutorial1(832슬롯), 0.490에서 tutorial2(830슬롯), 0.495에서 bc1menu76(829슬롯), 0.496에서 bc2bridging76(4,184슬롯), 0.497에서 bc3acid_bar76(2,921슬롯), 0.498에서 bc4bow_rush76(3,272슬롯)의 cpp 기준선·dn 덤프를 더했다. 월드 본체(객체·그래프 재구성·장부 집계) 행이 없어 어느 경로도 전체 일치할 수 없다. 이 구간 107단위 중 코드를 바꾼 단위는 약 절반(45개)이고, 나머지는 기록자 추적·범위 확정·기준선 확보였다.

| 묶음 | 연결한 것 (실행 호스트 world-1 덤프·집중 검사로 확인) | 단위 |
| --- | --- | --- |
| 현재Carrier전처리·내용물조회 | 원본2880입력·Native부모/현재boss·checkingDWORD/복귀head·저장next/현재TYPE·개수/WORD-DWORD·실제사제삭제어댑터를대조했다. 피해준비현행Native·전체사제월드/HostSession은잔여다 | 0.523 |
| 사제 정기 분배·현재 이동 선택 | 원본3974입력·현재TYPE/프로세스/동일 owner·명시 사본/주소보존 대입·실제Form/Kernel601재예약/삭제를 확인했다. 전체 사제 월드 묶음/HostSession 연결은 잔여다 | 0.522 |
| 일반/사제 후보 충돌·실제 배치 조립 | 원본34200입력·현재 마스크/역할·저장한next·첫거부 고정·실제 패턴/모양·전체 사제 배치→서버 공장/Owner/Carrier 합성을 확인했다. 일반 지역 정책·사제 월드/HostSession 연결은 잔여다 | 0.521 |
| 일반/사제 기하·로컬 미리보기 | 원본20039입력·현재 발자국/프레임·144BYTE/표면/방향 관계, 실제 finder와 사제 픽셀/기하/미리보기/지형 합성을 확인했다. 후보 충돌은0.521완료, 일반 지역 정책·전체 사제 월드는 잔여다 | 0.520 |
| 일반/사제 배치 접두·전체 픽셀 모양 | 원본20398입력·현재 자료/함수 사본·실제 PATCH 모양→배치→나선/공장을 대조했다. 기하/미리보기는0.520완료, 후보 충돌·일반 지역 정책과 전체 사제 월드 조립은 잔여다 | 0.519 |
| 사제 생성·Carrier 지면 | 나선/캐시한spot/현재TYPE·생성 후WORD/소유자/Pop/Carrier를 원본1912입력과 실제 해방/서버 공장·사제 소유자 합성으로 대조했다. 배치 접두/픽셀 모양은0.519 완료, 후반 정책·전체 월드 조립은 잔여다 | 0.518 |
| 현재 공통/사제 소유자·목록 | 원본7632입력과 실제 현재 배열/개수 쓰기·효과 뒤 모드/TYPE 재조회·진단 부분효과를 대조했다. Host Player/도장 두 Owner와 RawSurfaceWorld에 같은 현재 표를 연결했다. 생성/Carrier는0.518에서 완료했고 배치 검사·전체 사제 조립은 잔여다 | 0.517 |
| Player 화신·공유 장부 | 실제 Pop/PostPop·Player+9c·초기 Regular·장부 저장소 공유·실제 프로세스 덤프. 2미션 cpp/dn에서 로컬 기술부모5·첫 PID/Form/부모1/6/5 일치. 프레임 실행·AI·다른 파생 PostPop은 잔여 | 0.504 |
| 사제·현재 Pop·보호막/낙하·소리 수명 부품 | 사제 조회/Pop/후반·보호막·프레임·현재 Pop/전송·예약/Regular·낙하/9기록/현재상태·이름표/위치 재생/소리 프로세스와 실제 Windows 파일/장치 경계를 구현했다. 실제 보호막/공간/Kernel 수명 대조 완료. 실장치 재생은 활성 출력endpoint 부재로 미확인, 파생별 캠페인 호스트 결선은 남는다 | 0.505~0.516 |
| 미션 Fort | 스크립트 목록·원문 읽기·Template 풀기를 호스트 장치로 연결(thewarbegins 목록이 cpp `inspect-fort`와 일치) | 0.391 |
| 세션 홀더·덤프 | `HostSession`(Kernel·난수·풀 공유)과 world-1 덤프 행(머리·kernel·pool·slot, 저장소 7종) | 0.392·0.393 |
| 플레이어·AI | `MissionPlayers.Load`, 시작 SP, 인공지능 설정 바인딩(600B)·생성 본체(2번 brain), 참가자·동맹 81칸 | 0.394~0.400 |
| 마스크·접속 | 동맹 마스크, Zone 설치, 가입·Ready·Ready packet 원형과 라이브 연결(1번 flags 0x4b) | 0.401~0.407 |
| 요새·이름 | 요새 순번(0x5c·0x60·0x64) 기록, 전투 이름 확정(NUL)과 핸들 버퍼 공유 | 0.420·0.428·0.430 |
| 월드 적재 흐름 | 공유 저장소 묶음 → 확정 갈고리 Load 드라이런 → SetConfig 영속화 확정·연결 → `StartMission`에 `BuildWorldLoad`+`Load` 연결(**Generate·Objects는 stub**) | 0.436~0.440 |
| 타입·프레임 | `.type` 정의 116개 적재, 프레임 188 채우기, 실제 타입 표 연결 | 0.442 |
| 지형 저장소 | 지형 상태 묶음, 지형 전용 난수(`TerrainRandom`)·CRT 상자 묶음, 지역 상자의 세션 승격 | 0.444·0.447·0.448 |
| 입력 결선 | 게임 보조(`TerrainEnabled`·밀도 3종)·`ownerMode.battle`·`bridgeState.mission`·전투 옵션 5·6번(`Layout`·`Spacing`)·구독자·`AiDebug` 결선과 `WorldGeyserSpacing` 표 이식 | 0.453 |
| 바인딩 조립 | 세션 바인딩 조립(`BuildTerrainBinding`: 세션 저장소 묶음+입력 바인딩, 시작 흐름 드라이런)과 기록 효과 Generate 드라이런(일반 분기·도장·결정성) | 0.454 |
| Generate 라이브 | Territory 실바이트·`noRegion=127`·조각 107 실제값 결선, Load.Generate 갈고리→`GenerateOriginal` 라이브 연결(실제 도장+기록 Pop). 슬롯 1,679행 출현·SID 집합 일치·비슬롯 행 무변화 확인 | 0.455 |
| 난수·덤프 분석 | 고정시각 paired 기준선(`extracted/dotnet-l2-fixed/`, cpp f090/f240), world-1 CRT 행을 실제 소비 스트림(`TerrainCrt`)으로. 체인 parity 확정(CRT 296677442 일치) | 0.456 |
| 난수 단일 스트림 | 포팅 모듈(RawSpStore·지형·농지·배치·오브젝트·프레임 선택 등 13종+호스트·테스트)을 공유 `GameRandom`으로 단일화(근거 cpppj 498행). `TerrainRandom`·`_spRandom` 삭제, `HostTerrainRandomTests` 은퇴 | 0.457 |
| 프리-Generate 소비 확정 | dn 측 감사(game0·SP후=advance^353·Generate전후 무변화·덤프=advance^963)·cpp 측 정적 인벤토리·shift 상관(±5000 무)·SP offset +6=2회전·CRT 동일 절차 판정. 프레임 미수렴(78/1679) — 선택 테이블 대조·메뉴 소비량 확정은 0.459 | 0.458 |
| Pop 공간 등록 live | 세션 Pop 장치 결선(`BuildTerrainPopDevices`: 공장·해시·spots·SHP 표시, `BuildTerrainStampHooks` Create/Pop live 분기+중립 보존, `StartMission` SHP 적재·장치 연결, `TerrainDisplayShapes.Build` 블록 순서=자산 순서 정정)과 `HostTerrainPopTests` 5건(중립 보존·개수 거부·공장 vtable·섬 좌표/단계0/체인/spot·프린지 실측 3단계). paired 재실행으로 Pop 중립성 확정(Game·CRT·타입 1526·프레임 80/1679·full 78 동일, pool 바이트만 변경) | 0.466 |
| 요새 객체 live 적재 | 세션 객체 장치 결선(`BuildFortObjectDevices`: 공장·HP·내용물·건물·Pop, `MakeFortObjectLoadHooks` Create/HP/내용물, `StartMission` 객체 장치·Fort/Campaign Objects live 연결, `HostFortObjectLiveTests` 5건(장치 거부·미연결 거부·실제 요새 구조/결정성·Pop 미지원 분류·내용물 전수)). paired 재실행으로 구조 확정(슬롯 1679→1853·cpp_only 261→87, 타입 1628·프레임 123/1853·full 78, Game 2317693072 새 기준·CRT parity 유지). Pop 공간 등록은 R6 postPop·표시·그래프 장치와 함께 잇는다(실측 미지원 vtable 71·75·78·89·122 등) | 0.467 |
| 4경로 dn 미션 덤프·구조 대조 | baselined 4경로 중 나머지 3경로의 첫 실제 dn 미션 덤프(f090/f240) 생성과 SID 단위 대조(masterofwhirligigs 1743·savetheisland 2618·fragilefortune 2719슬롯, 모두 dn_only 0·공통=dn 전체, f240 dn_only [6] 게임플레이 잔여). thunderingpower 구조 참조(2410슬롯, cpp 시드 미일치로 구조만) 별도. 적재 순서 패리티 확정(순서 엄격 대조·내용물 null 전수·부착 경로 검사) | 0.468 |
| 그래프 재구성 live | 세션 그래프 엔진 결선(`BuildWorldGraphDevices`: 해시·점유·프레임 공유 `RawGraph` 생성, `RebuildWorldGraph` 표·스택 복사, territory·campaign 두 갈고리 live, `HostWorldGraphTests` 3건). paired 재실행으로 중립성 확정(Game·CRT·타입 1628·프레임 123/1853·full 78 동일, 변경분은 오프셋 30만, graph 120/255) | 0.470 |
| 소유자 귀속 확정 | 적재 중 postPop 스탬프 vs 게임플레이 진화 분리 확정(코드 변경 없음, 0.469 8절 기록) | 0.469 |
| 첫 지역 127 초기화 | `StartPlayers`에서 9기록 +0x78을 원문 초기화와 같이 127로 둠(`HostPlayerFirstRegionTests` 2건, 비참가자 7기록 cpp 127과 일치·참가자는 전투섬 배정 갈고리에서 덮어쓸 예정). paired 재실행으로 9기록 전부 127·Game·CRT·슬롯 구조 중립 확인 | 0.473 |
| 배정 region live 기록 | 전투섬 배정 region 갈고리 live 연결(`HostSession.WriteBattleIslandRegion`+`BuildBattleIslandRegionHook`+`TryResolveBattleIslandRegion`, `RawBattleIsland.SelectBattleTerritory` 분리, `HostBattleIslandRegionTests` 10건). 0.474에서 시작 흐름에 연결했다가 0.477에서 단일 미션 경로가 아님을 확정하고 호출을 걷어냈다(MP 경로 장치로 유지). paired 재실행 기록은 8절 0.474에 있다 | 0.474 |
| 6번째 경로 기준선·덤프 | dissolvedalliance(1-6) cpp f090 기준선+dn f090 생성과 6경로 구조 대조(dn_only 0·아바타 +1·그래프 236~249/255·Game·CRT 중립, 3인 미션 p3 확인·신전 3개 실측). 0.477 정정 후 상태로 단일 미션 +0x78은 127 유지(R6 사슬 잔여) | 0.478 |
| 7번째 경로 기준선·덤프 | thenoose(2-1, 캠페인 순서 offical4.english) cpp f090 기준선+dn f090 생성과 7경로 구조 대조(dn_only 0·타입 2,289·그래프 row0/row254 일치·Game·CRT 중립, 2인 미션+신전 2개 실측). tarc 목록에서 offical3~6 확인 | 0.479 |
| 8번째 경로 기준선·덤프·신전 순서 대조 | rainvsrain(2-2) cpp f090 기준선+dn f090 생성과 8경로 구조 대조(dn_only 0·타입 1,747·프레임 78·그래프 일치·Game·CRT 중립, savetheisland형 p1=1·p2=0 + 신전 파일 순서 [2,1] 일치). 8경로 전부에서 신전 파일 순서→cpp +0x78 일치(thewarbegins·masterofwhirligigs·fragilefortune·thunderingpower·thenoose [1,2]→[0,1], savetheisland·rainvsrain·runforit [2,1]→[1,0], dissolvedalliance [1,3,2]→[0,2,1]). 판정 메커니즘(담긴 섬 vs 파일 순서)은 신전 파일 좌표·섬 bounds 대조(R6)에서 가린다 | 0.480 |
| 9번째 경로 기준선·덤프 | runforit(2-3) cpp f090 기준선+dn f090 생성과 9경로 구조 대조(dn_only 0·타입 1,823·프레임 107·그래프 row0/row254 일치·Game·CRT 중립, savetheisland형 p1=1·p2=0 + 덤프 내 신전 순서 [2,1] 일치). offical4.english 순서대로 진행 중 | 0.481 |
| 10번째 경로 기준선·덤프 | guardmyback(2-4) cpp f090 기준선+dn f090 생성과 10경로 구조 대조(dn_only 0·타입 3,041·그래프 row0/row254 일치·Game·CRT 중립, 5인 미션 p1=1·p2=0·p3=2·p4=3 + 덤프 내 신전 순서 [2,1,3,4] 일치). offical4.english 순서대로 진행 중 | 0.482 |
| 11번째 경로 기준선·덤프 | surrounded(2-5) cpp f090 기준선+dn f090 생성과 11경로 구조 대조(dn_only 0·타입 5,734·그래프 row0/row254 일치·Game·CRT 중립, 6인 미션·소유자 1 무신전 p1=127 잔류 + 덤프 내 신전 순서 [3,4,5,2] 일치). offical4.english 순서대로 진행 중 | 0.483 |
| 담긴 섬 판정·12번째 경로 | 신전 좌표·섬 bounds 대조로 담긴 섬 모델 확정(8건 전수 일치: savetheisland·thewarbegins 각 2건, surrounded 4건) + breakingthrough(3-1) cpp f090 기준선+dn f090 생성과 12경로 구조 대조(dn_only 0·타입 2,767·그래프 row0/row254 일치·Game·CRT 중립, savetheisland형 p1=1·p2=0 + 신전 순서 [2,1] 일치). offical5.english 순서대로 진행 중 | 0.484 |
| 13번째 경로 기준선·덤프 | totherescue(3-2) cpp f090 기준선+dn f090 생성과 13경로 구조 대조(dn_only 0·타입 3,518·그래프 row0/row254 일치·Game·CRT 중립, savetheisland형 p1=1·p2=0 + 덤프 내 신전 순서 [1,4,3,2] 일치·bounds 대조로 담긴 섬 4건 추가 확인). offical5.english 순서대로 진행 중 | 0.485 |
| 14번째 경로 기준선·덤프 | vicious(3-3) cpp f090 기준선+dn f090 생성과 14경로 구조 대조(dn_only 0·타입 1,900·소유자 1,157·프레임 342·그래프 row0/row254 일치·Game·CRT 중립, 소유자 1 무신전 p1=127 잔류·owner2 신전만 담김 + bounds 대조로 담긴 섬 2건 추가 확인). offical5.english 순서대로 진행 중 | 0.486 |
| 15번째 경로 기준선·덤프 | enemyterritory(3-4) cpp f090 기준선+dn f090 생성과 15경로 구조 대조(dn_only 0·타입 5,697·소유자 2,517·프레임 1,136·그래프 239/255·Game·CRT 중립, 5인 미션 p1=4·p2=5·p3=1·p4=0 + 신전 6개와 섬 bounds 6건 전수 일치·플레이어별 담긴 섬 대응). offical5.english 순서대로 진행 중 | 0.487 |
| 16번째 경로 기준선·덤프 | finalconfrontation(3-5) cpp f090 기준선+dn f090 생성과 16경로 구조 대조(dn_only 0·타입 4,104·소유자 1,695·프레임 511·그래프 218/255·Game·CRT 중립, 6인 미션 p1=0·p2=1·p3=2·p4=3·p5=4 + 신전 5개와 섬 bounds 5건 전수 일치·3장 Complete Victory 완료). offical5.english 순서대로 진행 중 | 0.488 |
| 17번째 경로 기준선·덤프 | tutorial1(Early 1 Bridge the Gap) cpp f090 기준선+dn f090 생성과 17경로 구조 대조(dn_only 0·타입 762·소유자 138·프레임 31·그래프 254/255·Game·CRT 중립, 2인 미션 p1=0 + 신전 1개와 섬 bounds 1건 일치·Early Missions 진입). offical1.english 순서대로 진행 중 | 0.489 |
| 18번째 경로 기준선·덤프 | tutorial2(Early 2 Secret Workshop) cpp f090 기준선+dn f090 생성과 18경로 구조 대조(dn_only 0·타입 761·소유자 829·프레임 280·그래프 254/255·Game·CRT 중립, 전원 +0x78 127·중립 신전 1개와 섬 bounds 1건 일치). offical1.english 순서대로 진행 중 | 0.490 |
| 19번째 경로 기준선·덤프 | tutorial3(Early 3 Capture The Priest) cpp f090 기준선+dn f090 생성과 19경로 구조 대조(dn 1,855·cpp 1,882·공통 1,854·dn_only 1·cpp_only 28·타입 1,700·소유자 1,135·프레임 366·그래프 249/255·Game·CRT 중립, p1=0·p2=1 + 신전 2개와 섬 bounds 2건 일치·dn 추가 123번 1건은 요새 원문 2개 중 cpp 게임플레이 제거 잔여). offical1.english 순서대로 진행 중 | 0.491 |
| 20번째 경로 기준선·덤프 | tutorial4(Early 4 Tactical Combat) cpp f090 기준선+dn f090 생성과 20경로 구조 대조(dn 1,990·cpp 2,032·공통 1,989·dn_only 1(SID 15022 요새 원문 자리·R6 잔여)·cpp_only 43(소규모 SID 게임플레이 잔여)·타입 1,764·소유자 1,262·프레임 380·그래프 247/255(row0 surfaces만 cpp -5 vs dn 0·row254 일치·R6 per-Pop 번호 잔여)·Game·CRT 중립, p1=0·p2=1·나머지 127(dn은 전부 127·소용돌이 사슬 R6) + 신전 2개(125,75·131,154)와 지형 군집 bounds 2건((113,64)-(143,94)·(96,128)-(143,159)) 전수 일치·31건→33건). offical1.english 순서대로 진행 중 | 0.492 |
| 21번째 경로 기준선·덤프 | tutorial5(Early 5 Subtle Defense) cpp f090 기준선+dn f090 생성과 21경로 구조 대조(dn 2,692·cpp 2,745·공통 2,692·dn_only 0·cpp_only 53(소규모 SID 게임플레이 잔여)·타입 2,403·소유자 445·프레임 83·그래프 248/255(row0 surfaces만 cpp -4 vs dn 0·row254 일치·R6 per-Pop 번호 잔여)·Game·CRT 중립, p1=0·p2=1·나머지 127(dn은 전부 127·소용돌이 사슬 R6, cpp p2 이름 Evil Duke·dn p2 이름 없음도 같은 사슬 잔여의 결과) + 신전 2개(127,83·146,139)와 지형 군집 bounds 2건((115,64)-(142,94)·(112,113)-(159,159)) 전수 일치·33건→35건). offical1.english 순서대로 진행 중 | 0.493 |
| 22번째 경로 기준선·덤프 | tutorial6(Early 6 Raw Power) cpp f090 기준선+dn f090 생성과 22경로 구조 대조(dn 2,102·cpp 2,366·공통 2,102·dn_only 0·cpp_only 264(소규모 SID 게임플레이 잔여 — 170번 165개·90번 51개 포함, 개시 공격 중이라 규모가 큼)·타입 1,868·소유자 390·프레임 92·그래프 248/255(row0 surfaces만 cpp -4 vs dn 0·row254 일치·R6 per-Pop 번호 잔여)·Game·CRT 중립, p1=0·p2=1·나머지 127(dn은 전부 127·소용돌이 사슬 R6, cpp p2 이름 Evil Duke·dn p2 이름 없음도 같은 사슬 잔여의 결과) + 신전 2개(122,61·170,111)와 지형 군집 bounds 2건((96,49)-(126,79)·(128,82)-(175,127)) 전수 일치·35건→37건·Early Missions 6개 완료). offical1.english 순서대로 진행 중 | 0.494 |
| 23번째 경로 기준선·덤프 | bc1menu76(Priest Training Menu) cpp f090 기준선+dn f090 생성과 23경로 구조 대조(dn 829·cpp 952·공통 829·dn_only 0·cpp_only 123(소규모 SID 게임플레이 잔여)·타입 677·소유자 193·프레임 55·그래프 253/255(row0 surfaces만 cpp -7 vs dn 0·row254 일치·R6 per-Pop 번호 잔여)·Game·CRT 중립, cpp p1=0·p2=0·p3=127·p4=0·p5=0·p6=0·p7=0·나머지 127(dn은 전부 127·소용돌이 사슬 R6, AI 이름 CrossBows·Bridges·Acid Bars·Walkers·Sun Barricade·Man o'Wars는 양쪽 일치) + 7신전 중 생성 지형 bounds((115,112)-(143,141)) 내 6건 일치·지형 밖 1건(owner3 (124,143) — cpp·dn 양쪽 지형 최대 y=141, 해당 소유자 Bridges p3가 cpp에서도 127이라 담긴 섬 모델과 일치)·37건→43건·Priest Training 진입). offical2.english 순서대로 진행 중 | 0.495 |
| 24번째 경로 기준선·덤프 | bc2bridging76(Priest Training Bridging) cpp f090 기준선+dn f090 생성과 24경로 구조 대조(dn 4,184·cpp 4,232·공통 4,184·dn_only 0·cpp_only 48(소규모 SID 게임플레이 잔여)·타입 3,807·소유자 2,671·프레임 1,017·그래프 250/255(row0 surfaces cpp 0 vs dn 0 일치 첫 사례·row254 일치·나머지 5행 R6 per-Pop 번호 잔여)·Game·CRT 중립, cpp p1=1·p2=2·나머지 127(dn은 전부 127·소용돌이 사슬 R6, AI 이름 Stray 양쪽 일치) + 3섬 중 유신전 2섬 bounds((96,208)-(143,236)·(113,16)-(141,46))에 신전 2개(owner1 (125,218)→p1=1·owner2 (133,41)→p2=2) 전수 일치·중앙섬((51,115)-(175,175)) 무신전·43건→45건). offical2.english 순서대로 진행 중 | 0.496 |
| 25번째 경로 기준선·덤프 | bc3acid_bar76(Priest Training Acid Bars) cpp f090 기준선+dn f090 생성과 25경로 구조 대조(dn 2,921·cpp 3,133·공통 2,921·dn_only 0·cpp_only 212(소규모 SID·155번 77개·94/95번 35개씩 게임플레이 잔여 — man o war 공격 개시 중) ·타입 1,988·소유자 753·프레임 257·그래프 245/255(row0 surfaces만 cpp -59 vs dn 0·row254 일치·나머지 9행 R6 per-Pop 번호 잔여)·Game·CRT 중립, cpp p1=1·p2=0·나머지 127(dn은 전부 127·소용돌이 사슬 R6, AI 이름 Jailer 양쪽 일치) + 2섬 bounds((112,81)-(159,127)·(112,192)-(158,223))에 신전 2개(owner2 (131,89)→p2=0·owner1 (146,210)→p1=1) 전수 일치·45건→47건). offical2.english 순서대로 진행 중 | 0.497 |
| 26번째 경로 기준선·덤프 | bc4bow_rush76(Priest Training X-Bows) cpp f090 기준선+dn f090 생성과 26경로 구조 대조(dn 3,272·cpp 3,376·공통 3,272·dn_only 0·cpp_only 104(소규모 SID·46번 50개·6번 17개·94/95번 16개씩 게임플레이 잔여)·타입 2,970·소유자 792·프레임 150·전체 108·그래프 234/255(row0 surfaces만 cpp -16 vs dn 0·row254 sentinel 32253 일치·나머지 20행 R6 per-Pop 번호 잔여)·Game·CRT 중립, cpp p1=0·p2=1·p3=3·p4=2·나머지 127(dn은 전부 127·소용돌이 사슬 R6, AI 이름 Lord Ice·Count Thunder·Mr Sun 양쪽 일치·p1 You 부분 일치 꼬리 pson vs son은 핸들 버퍼 공유 잔여) + 4섬 bounds((96,160)-(143,191)·(144,193)-(190,223)·(98,96)-(142,127)·(49,193)-(92,207))에 사제 4개(owner1 (126,175)→p1=0·owner2 (118,112)→p2=1·owner4 (160,214)→p4=2·owner3 (77,201)→p3=3) 전수 일치·47건→51건). offical2.english 순서대로 진행 중 | 0.498 |
| Player 월드 live | Player 월드 갈고리 live 연결(`HostSession.BuildPlayerWorldHooks`: Find→finder·Create→공장·SetOwner→SquidOwner, `StartMission`의 기록 스텁 교체·화신 사전 할당 제거, `HostPlayerWorldLiveTests` 7건). Pop 공간 등록·예약 갈고리는 R6 경계(152 가상표 미지원 실측). paired 재실행으로 아바타 슬롯 1개만 변경(SID 5·행 수·Game·CRT 중립) 확인 | 0.475 |
| 그래프 ResetTable·5경로 재생성 | 그래프 번호 소진 해소(`HostSession` 생성자+`BuildWorldGraphDevices`에 ResetTable 설치, `HostWorldGraphTests` 2건·`SessionStateDumpTests` 기대값 정정). 원인: 빈 표의 임시 번호 0이 기존 graph-0과 충돌해 251+ 무리를 재번호(=0.470 thewarbegins 120/255의 정체). 5경로 dn f090 HEAD 재생성+구조 대조(dn_only 0·아바타 +1·그래프 236~249/255·Game·CRT 중립, savetheisland·fragilefortune 통과) | 0.476 |
| +0x78 기록자 정정·섬 바인딩 | 단일 미션 +0x78 기록자는 전투섬 배정 갈고리가 아니라 소용돌이→섬 소유자→첫 지역 사슬임을 확정하고 시작 흐름 자동 기록을 걷어냈다(장치·검사는 MP 경로용으로 유지). 근거: missionWorld=true 바인딩→loadWorld 경로·savetheisland p1 불일치·fragilefortune/thunderingpower 생략·경로별 신전 2개(소유자 1·2) 실측. `BuildIslands`에 Player 기록 바인딩(`HostIslandFirstRegionTests` 2건: 공유 기록·첫 섬 유지·0번 생략). 소용돌이 호출 연결은 Pop·postPop 표시와 함께 R6에서 잇는다(0.484 판정 실험이 담긴 섬 모델을 확정: 신전 좌표·섬 bounds 10건 전수 일치) | 0.477 |
| 섬 | 섬 런타임 부모 조립(`BuildIslands`: 풀·기록·모드·목록·해시 공유) | 0.452 |

**조사로 확정한 것(코드 변경 없음, 잔여 지도):** Player 9기록·이름 바이트15·SP 오프셋·`+8`/`+0x78`/`0x5c~0x64`·장부 합계·AI 부착 recipe의 기록자 추적(0.408~0.431), 월드 적재(0.435)·지형/객체 효과(0.441)·Generate 진입점(0.443)·난수 출처(0.446)·생성 입력 15종(0.450)의 범위 확정, fragilefortune 2번 SP 불일치는 cpp 쪽 AI 경제 활동이라 시작값 오류가 아니라는 결론(0.451, R6 잔여). 상세는 [보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)의 단위 기록이다.

**cpp L2 기준선(오라클 `session-dump`, f090·f240):** thewarbegins(0.390: 슬롯 1,941·객체 1,881), masterofwhirligigs(0.433: 3,904행·슬롯 1,807·객체 1,755), savetheisland(0.445: 6,129행·슬롯 2,926·객체 2,831), fragilefortune(0.449: 6,191행·슬롯 2,946·객체 2,848). 산출물은 `extracted/dotnet-l2-baseline/<경로>/`에 있다(Git 제외, 이 PC에만).

**검증 방법:** 개발 호스트로 `--mission <키> --handle Youpson`을 260프레임 실행(종료 코드 0)하고 world-1 덤프를 cpp 기준선과 행 단위로 대조한다. 실행 전후 `originals/` 1,394파일 해시 차이 0을 확인하고, 호스트가 바꾼 추적 파일 `assets/game-data/d/options.cfg`는 내가 바꾼 것인지 가려 되돌린다. 대표 변이를 넣어 검사의 검출력을 확인한 뒤 원복한다([6.1](#dotnet-commands)).

**남은 조각과 순서(0.441·0.450에서 정한 분할):**

1. **전체 회귀 재완주(0.4 1-1)** — 0.453에서 0.453 포함으로 완주한다.
2. **바인딩 조립 설계 → 입력 값 연결 + Generate 드라이런.** ✅ 0.454에서 끝났다(조립·바인딩·드라이런, 덤프 282행 중립).
3. **Generate 라이브 연결 + 덤프 대조** ✅ 0.455에서 1차 연결·대조(슬롯 1,679행·SID 집합·비슬롯 행 무변화), 0.456에서 메커니즘 확정: ① 생성 순서·셀 집합 동일 증명(정렬 152 flips only·Pop 좌표 집합 동일) — 농지 121은 postprocess 배치 산물(동일 셀에 dn은 97, 배치 파이프라인 R6 필요) ② 소유자는 region+1을 적재 시점에 가짐(121은 0, writer는 Objects/AI-setup 잔여) ③ CRT parity 확정(체인 동일 출력 296677442) ④ Game/SP-offset/frame divergence는 단일 공유 스트림 분리가 원인(0.457에서 단일화), SP offset +6=게임플레이 2회전·tracking 미연결은 R6 잔여. 남은 차이: Pop 공간 등록(void·좌표·레벨·체인, vtable Supports·SHP dims·postPop이 발목) · 공장 생성자·vtable · `generate:127` 갱신(의도적 결선 변경, 0.455). 위험 ①(풀 수열)은 해소 — `predictable` 플래그를 그대로 풀에 전달해 예측 영역에 할당한다.
4. **난수 단일 스트림 연결** ✅ 0.457에서 포팅 모듈 13종+호스트·테스트 20건을 공유 `GameRandom`으로 단일화(`TerrainRandom`·`_spRandom` 삭제, `HostTerrainRandomTests` 은퇴). 라이브 덤프 구조 동일(1,955행·슬롯 1,679·SID 집합 동일·비슬롯 행 무변화) 확인. 프레임은 미수렴(78/1679) — 0.458에서 dn 측 소비를 감사로 확정(reseed→SP 353회→Generate 무변화→지형 610회, 합계 advance^353+610)하고 cpp 측 정적 인벤토리를 마쳤다(AI 생성·로그인·플레이어 생성·PlaceAll 무소비, 메뉴 ambient 제외). 레거시(`BattleSession` 등)는 결정 D2 삭제 때까지 `NetstormRandom`을 둔다.
5. **프레임 선택 대조(0.459~0.465)** ✅ 0.459에서 프레임 값 분포 대조로 후보 테이블 차이를 확정: dn 최댓값 0xd0·cpp 최댓값 0x13c(동일 셀 집합·동일 순서·동일 스트림 가정 하에 설명 불가). 0.460에서 per-type 정적 대조로 필터·구조 가설을 좁혔다: `ChooseTerrainFrame` 몸체 동일(cpp `o/RawFrameChoose.cpp` 8~33행 vs dn `RawFrameChoose.cs` 20~82행, NB23 포함), `RiftTypeFrames` 구조 동일(Codes·Find*·Run·LetterKinds), `FrameTable()` 양쪽 클러스터 순서 그대로(cpp `TypeParser.cpp` 225~231행 vs dn `TypeFrameTable`+`FillWorldFrames` 70+순서, 188칸·116자산), dn 호스트 ambient 0회(`Client.Host`에 `UpdateAmbient` 없음) vs cpp 매 프레임 Wind 1~2회(`Next(100)`+10% `Next(65536)`)·Sound 기한 통과 때만 1~2회(`Next(3000)`[+`Next(20000)`]), cpp 덤프는 메뉴 경유(click-if 5/30/60→dump 90, 약 90 메뉴 프레임) vs dn 직접 진입(`--mission`, 0 메뉴 프레임). 0.461에서 reseed-Generate 동기 순서와 SP 353 구성을 확정해 ambient를 프레임 원인에서 제외했다: SP 353회=`Initialize` 33회(`Next(100)` 1+`Next(32)` 32)+활성 소유자 2명의 `Set` 잡음 320회(32비트×5회×2명), 양쪽 모두 reseed→SP→Generate가 한 시작 흐름에 동기 실행되고 프레임 루프(`Update`)와 분리돼 있어 그 사이에 ambient가 낄 수 없다(dn `ClientHostGame` 270~383행 vs `Update` 393행~, cpp `ClientMain` 349행·`RawCampaignSession` 1090행 vs 루프 601~602행). 0.462에서 SID 단위 per-cell paired 대조로 발산 위치를 확정했다: 공통 SID 1,679개(dn_only 0·cpp_only 261=게임플레이 객체 잔여), 타입 일치 1,526개(불일치 153개 중 152개는 dn97 vs cpp121 농지 postprocess·1개는 dn0 vs cpp152, 모두 R6 잔여), 프레임 일치 78개=지형 5개+프린지 72개+기타 1개이며 섬 타입 97은 1,505개 중 0개 일치·프린지 102는 168개 중 72개 일치, 불일치 표본에 +6 오프셋 다수(141→147·149→155·33→39) — 테이블 내용 가설을 검증했으나 파서·난수 알고리즘이 양쪽 동일함을 확인하고, reseed 시점 차이(dn 매 미션 reseed vs cpp 시작 1회 초기화+공유 경로 무재초기화, cpp `--mission` 메뉴 경유 vs dn 직접 진입)를 근본 원인으로 확정했다(아래 8절 0.463 기록). 0.464에서 호스트 시작 1회 reseed+SP 초기화를 적용하고 paired 재실행으로 검증했다: 덤프 Game 2583299428 == 사전 예측 advance^1764(seed) 정확 일치(33+801+320+610, 중간 소비 0 증명), CRT parity 유지, SP offset 44 동일, 프레임 80/1679(여전히 미수렴). 0.465에서 early-dump N(F) 역산으로 sim 가설을 기각하고 sim을 회수했다: 필요 gap ≥ ~3500 draws(기동 3395+f3-f4 1282+적재-전) vs K≤90 ≈ ≤180 draws, 위치도 Generate 전이 아니라 적재 후·적재 사이 인터리브라 불일치. 0.466에서 Pop live 연결 후 paired 재실행으로 Pop 중립성을 확정했다: Game 2583299428·CRT parity·타입 1526·프레임 80/1679·full 78 모두 0.464와 동일하고 pool 바이트만 Pop 기록분 변경됐다. 0.467에서 요새 객체 live 적재 후 paired 재실행으로 구조를 확정했다: 슬롯 1679→1853(+174)·cpp_only 261→87, 타입 1628·프레임 123/1853·full 78, Game 2317693072(새 기준, 객체 RandomFrame·후보 추첨 소비분)·CRT parity 유지. 프레임 수렴은 파이프라인 구현(R4-6: Pop 후속·농지·소유자+기동/메뉴 경로 복원) 뒤 재시도한다. 이후 Objects 라이브 연결(`RawFortObjectLoad` + 갈고리 약 20종, 공장 할당 parity) + 덤프 대조.
6. **장부·그래프·농지·소유자**(`RebuildGraph` 실몸체, totals·totalmade, 농지 배치 파이프라인, 소유자 후속 지정) + 덤프 대조. 선행: 프레임 수렴(5). 요새 객체 Create·소유자·HP·프레임·내용물 부착은 0.467에서 live 연결됐고, 객체 Pop 공간 등록(postPop·표시·그래프)은 R6에서 잇는다. 0.469에서 소유자 귀속 확정: 지면 isle-97(1353건)·noIsland-157(23건) 스탬프는 적재 중 postPop(`IslandPostPop.SetSupportOwner` 3×3·`Prefix`, cpp f6=f65 분포 {1:853, 2:500} 일치로 입증) 소관이라 R6 postPop 파이프라인에서 해소하고, 교량-82·요새 객체(71/75/78/158) 차이는 게임플레이(f6→f65 진화, 적재 직후 cpp f6 == dn) 소관이라 R6 게임플레이에서 해소한다. 요새 적재 owner식·`islands.Owner`/소용돌이·`RegionPaint`는 원인에서 제외했다. 0.470에서 `RebuildGraph` live 연결(세션 엔진+territory·campaign 두 갈고리, `HostWorldGraphTests` 3건): paired 재실행으로 난수·구조 중립 확정(Game·CRT·타입·프레임·full 동일, 변경분 오프셋 30만, graph 120/255 — 나머지 135행은 풀 상태 차이로 R6 소관). 0.471에서 장부 귀속 확정: totals(TotalCost)·totalmade(3종 counts)는 Pop 성공 때 집계(`SquidPostPop::PostPopBase` 417~431행·`SquidPostPopPatch` 21~22행)라 dn 중립 Pop에서는 0이며 R6 postPop 파이프라인에서 잇는다.
7. **기타 잔여:** brain 실생성자(0.503 정정: 호스트 AI 호출 자체가 아직 없음), 요새 이미지 전송, 메뉴에서의 미션 시작(핸들 출처, 이름 꼬리 16~19 바이트 잔여), 전투섬 배정과 `+0x78` 첫 지역(0.472 귀속 확정·0.473 비참가자 127 초기화·0.474 전투섬 배정 갈고리 live 연결·0.477 정정: 배정 갈고리는 MP 전투 경로 장치로 유지하고 단일 미션 시작 흐름의 자동 기록은 걷어냈다. 단일 미션의 기록자는 소용돌이→섬 소유자→첫 지역 사슬이며 호출 연결은 신전 Pop·postPop 표시와 함께 R6에서 잇는다 — missionWorld=true 바인딩·11경로 대조(cpp 배정 vs dn 127 유지)·경로별 신전 실측·surrounded 무신전 소유자 127 잔류가 근거. 섬 런타임 Player 기록 바인딩은 0.477에서 연결했다), Player 월드 Pop 공간 등록·예약 갈고리(0.504 완료, 화면 표시·프레임 실행은 R5/R6 잔여), 그래프 per-Pop 번호 부여(R6 postPop 그래프 연결, 0.476에서 설치 ResetTable까지만 둠), 세션 `Bind*` 대리자의 실제 몸체 연결, 농지 배치 파이프라인.
8. **34경로 L2 대조(M1)** — 나머지 14경로의 cpp 기준선을 만들고 새 경로 덤프와 행 단위로 대조한다. 비교 실행에는 `--fixed-unix-seconds`를 양쪽에 동일하게 준다(0.456; 고정 시각만으로는 프레임이 맞춰지지 않고 시드는 같아진다). 0.476에서 baselined 5경로의 dn 미션 덤프를 HEAD로 재생성하고 0.478에서 dissolvedalliance, 0.479에서 thenoose(캠페인 2-1), 0.480에서 rainvsrain(캠페인 2-2), 0.481에서 runforit(캠페인 2-3), 0.482에서 guardmyback(캠페인 2-4), 0.483에서 surrounded(캠페인 2-5, 순서 offical4.english), 0.484에서 breakingthrough(캠페인 3-1), 0.485에서 totherescue(캠페인 3-2), 0.486에서 vicious(캠페인 3-3), 0.487에서 enemyterritory(캠페인 3-4), 0.488에서 finalconfrontation(캠페인 3-5, 순서 offical5.english·3장 Complete Victory 5개 완료), 0.489에서 tutorial1(Early 1 Bridge the Gap, 순서 offical1.english), 0.490에서 tutorial2(Early 2 Secret Workshop, 순서 offical1.english), 0.491에서 tutorial3(Early 3 Capture The Priest, 순서 offical1.english), 0.496에서 bc2bridging76(Priest Training Bridging, 순서 offical2.english), 0.497에서 bc3acid_bar76(Priest Training Acid Bars, 순서 offical2.english), 0.498에서 bc4bow_rush76(Priest Training X-Bows, 순서 offical2.english)의 cpp 기준선·dn 덤프를 더해 M1 판정용 25/34(thunderingpower 구조 참조 별도)를 갖췄다. cpp 기준선의 시드가 dn 고정시각과 달라 Game·CRT 행은 직접 대조하지 않고 구조(SID·타입·소유자·프레임 분포)만 대조했다 — 동일 시드 paired cpp 재실행은 34경로 본 대조 때 묶어서 한다. 경로별 +0x78 대조(0.477 정정 후 dn은 전 경로 127 유지, cpp는 경로별 배정 — thewarbegins·masterofwhirligigs·fragilefortune·thunderingpower·thenoose p1=0·p2=1, savetheisland·rainvsrain·runforit p1=1·p2=0, dissolvedalliance p1=0·p2=2·p3=1, guardmyback p1=1·p2=0·p3=2·p4=3, surrounded p1=127·p2=4·p3=0·p4=2·p5=3, vicious p1=127·p2=1, enemyterritory p1=4·p2=5·p3=1·p4=0, finalconfrontation p1=0·p2=1·p3=2·p4=3·p5=4, tutorial1 p1=0, tutorial2 전원 127(중립 신전), tutorial3 p1=0·p2=1, tutorial4 p1=0·p2=1, tutorial5 p1=0·p2=1(cpp, dn은 전부 127·p2 이름 없음 — 같은 소용돌이 사슬 잔여), tutorial6 p1=0·p2=1(cpp, dn은 전부 127·p2 이름 없음 — 같은 소용돌이 사슬 잔여), bc1menu76 p1=0·p2=0·p3=127(Bridges 지형 밖)·p4=0·p5=0·p6=0·p7=0(cpp, dn은 전부 127 — 같은 소용돌이 사슬 잔여), bc2bridging76 p1=1·p2=2(cpp, dn은 전부 127 — 같은 소용돌이 사슬 잔여), bc3acid_bar76 p1=1·p2=0(cpp, dn은 전부 127 — 같은 소용돌이 사슬 잔여), bc4bow_rush76 p1=0·p2=1·p3=3·p4=2(cpp, dn은 전부 127 — 같은 소용돌이 사슬 잔여)이다. 단일 미션 기록자가 소용돌이 사슬임을 가리킨다(R6). 9경로 전부에서 신전 파일 순서→cpp +0x78 일치(0.480~0.481 실측). 10번째 경로 guardmyback도 덤프 내 신전 순서 [2,1,3,4]가 cpp 배정 [1,0,2,3]과 일치한다(0.482). 11번째 경로 surrounded는 파일 순서 [3,4,5,2]가 그대로 섬 번호가 아니라 담긴 섬 [0,2,3,4](1번 섬 무신전·소유자 1 무신전→127)이며, 0.484 판정 실험(신전 좌표·섬 bounds 10건 전수 일치)이 담긴 섬 모델을 확정했다. 0.485에서 totherescue 섬 bounds 4건을 추가 확인해 14건 전수 일치가 됐다. 0.486에서 vicious 섬 bounds 2건을 추가 확인해 16건 전수 일치가 됐으며, 소유자 1 신전이 섬 밖에 있어 p1=127로 남는 두 번째 사례다. 0.487에서 enemyterritory 섬 bounds 6건(플레이어 4명+중립 2개, 6개 섬 전부 담김)을 추가 확인해 22건 전수 일치가 됐다. 0.488에서 finalconfrontation 섬 bounds 5건(참가자 5명+중립 1개, 5개 섬 전부 담김)을 추가 확인해 27건 전수 일치가 됐으며, 3장 Complete Victory 5개가 모두 담긴 섬 모델과 일치한다. 0.489에서 tutorial1 섬 bounds 1건(섬 0에 신전 1개 담김, p1=0 대응)을 추가 확인해 28건 전수 일치가 됐다. 0.490에서 tutorial2 섬 bounds 1건(섬 0에 중립 신전 1개 담김, 전원 +0x78 127 대응)을 추가 확인해 29건 전수 일치가 됐다. 0.491에서 tutorial3 섬 bounds 2건(owner1 (132,93)→섬 0·owner0 (117,139)→섬 1, p1=0·p2=1 대응)을 추가 확인해 31건 전수 일치가 됐다. 0.492에서 tutorial4 지형 군집 bounds 2건(owner1 (125,75)→(113,64)-(143,94)·owner2 (131,154)→(96,128)-(143,159), p1=0·p2=1 대응)을 추가 확인해 33건 전수 일치가 됐다. 0.493에서 tutorial5 지형 군집 bounds 2건(owner1 (127,83)→(115,64)-(142,94)·owner2 (146,139)→(112,113)-(159,159), p1=0·p2=1 대응)을 추가 확인해 35건 전수 일치가 됐다. 0.494에서 tutorial6 지형 군집 bounds 2건(owner1 (122,61)→(96,49)-(126,79)·owner2 (170,111)→(128,82)-(175,127), p1=0·p2=1 대응)을 추가 확인해 37건 전수 일치가 됐다(Early Missions 6개 완료). 0.495에서 bc1menu76 생성 지형 bounds((115,112)-(143,141), cpp·dn 682셀 일치) 내 신전 6건(owner1 (127,130)·owner2 (120,116)·owner4 (136,118)·owner5 (136,136)·owner6 (121,128)·owner7 (126,118) → p1·p2·p4·p5·p6·p7 = 0 대응)을 추가 확인해 43건 전수 일치가 됐다. 지형 밖 1건(owner3 (124,143) — cpp·dn 양쪽 생성 지형 최대 y=141)은 해당 소유자 Bridges p3가 cpp +0x78 127로 남아 담긴 섬 모델(담긴 섬만 배정)과 일치한다. 0.496에서 bc2bridging76 3섬 중 유신전 2섬 bounds((96,208)-(143,236)·(113,16)-(141,46), cpp·dn 셀 집합 일치)에 신전 2건(owner1 (125,218)→p1=1·owner2 (133,41)→p2=2 대응)을 추가 확인해 45건 전수 일치가 됐다(중앙섬 (51,115)-(175,175) 무신전). 0.497에서 bc3acid_bar76 2섬 bounds((112,81)-(159,127)·(112,192)-(158,223), cpp 지형 1,780+농지 191=dn 1,971 일치)에 신전 2건(owner2 (131,89)→p2=0·owner1 (146,210)→p1=1 대응)을 추가 확인해 47건 전수 일치가 됐다. 파일 순서는 섬별 열거의 부산물로 본다.

**진행 원칙:**

- **단위 크기(권고, 원하지 않으면 알려 달라):** cpppj가 1~13차를 마감해 소스가 안정됐으므로, 지금까지의 "함수 하나 + 검사 몇 건" 단위 대신 **cpppj 파일(모듈) 묶음 단위**로 옮긴다. 검증의 중심은 L2(`session-dump`)·L4a(캡처) 비교로 옮기고, 원본 fixture가 있는 모듈은 계속 L1을 연결한다. 0.330~0.341처럼 실제 조립과 무관한 관문을 하나씩 늘리는 단위는 줄인다.
- 단위마다 이 문서의 0절(0.2·0.3·0.4)과 해당 상세 절을 갱신하고, 작은 단위 기록은 [8절](#dotnet-unit-records)에 남긴 뒤 로컬 커밋한다([6.2](#dotnet-ops)).
- 실제 실행 검사에서 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 검사는 건너뛴다. MP 실행 검사는 하지 않는다. 단순 미션 진입·대기·생산·건설로 끝나는 비MP 검사는 수행한다(LEFT_JOBS.md 5.1과 같음).

<a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a>

### 0.5 사용자 결정 (2026-11-11 확정)

이번 정리에서 사용자 확인을 기다리던 항목을 사용자가 2026-11-11에 모두 정했다. **현재 결정을 기다리는 항목은 없다.** 새 결정이 필요해지면 이 절에 표로 더한다. 아래 결정 D1~D5는 이정표 D1(메뉴 픽셀 일치)·D2(진입 화면 픽셀 일치)와 다르다.

| ID | 결정 | 내용과 적용 |
| --- | --- | --- |
| **D1** | **cpppj HEAD를 요구사항 코드까지 그대로 옮긴다**(권고안) | cpppj가 원본 복원 코드 위에 같은 파일로 얹은 요구사항 코드(Options의 Difficulty·Frame Rate·Language, Campaign의 Skirmish, Edit의 Scenario 패널, 모든 파일 경로의 사용자 홈 루트, 한글·일본어 표시 부호화, Credits 카피라이트 등)를 결정 2대로 함께 옮긴다. 각 단계의 완료 판정은 먼저 **기본 설정(영어·보통 난이도·Frame Rate 75 원본·4:3·창 모드)에서 cpppj와 같음**으로 하고, 요구사항 기능 자체의 검증·마감은 R8에서 한다. AGENTS의 "일단은 기존 게임과 완전히 동일하게 동작하는 것을 목표로 개발 후 요구사항을 반영"은 이 검증 순서로 지킨다. 저장 위치는 옛 계획의 "R7까지 게임 폴더 `d/`, R8 5차에서 사용자 홈"이 아니라 처음부터 cpppj와 같은 사용자 루트다(시험은 `NETSTORM_USER_DATA` 빈 루트) |
| **D2** | **새 경로가 `Netstorm.exe` 이름을 이어받는다. 작업이 끝나면 기존 경로는 제거한다** | 적용(AI 해석): 이름과 배포 대상은 새 경로가 메뉴와 미션을 모두 맡을 수 있게 되는 R5(이정표 D2)에서 한 번에 넘긴다. 그 전에 넘기면 배포판에서 미션을 할 수 없거나, 새 메뉴에서 기존 경로 미션으로 넘기는 임시 연결이 필요하다. 넘길 때 `Netstorm.Client.Host`의 어셈블리 이름을 `Netstorm`으로, 기존 `Netstorm.Game`은 `NetstormLegacy`로 바꾸고 배포에서 뺀다. 기존 경로와 그 근사 검사는 새 경로가 1-1·1-2를 같은 수준 이상으로 진행하면(R6, 결정 5) 삭제한다. 그때까지 기존 경로는 실행 오류 수정만 한다 |
| D3 | **dotnetpj 클론에도 4차 로컬 MCP를 붙인다**(권고안) | R8 4차에서 cpppj MCP(935~937)와 같은 도구 표면(stdio JSON-RPC로 게임 내 모든 조작·상태 조회·PNG 화면 캡처·소리 포함 녹화)을 만든다. 그 전까지 dotnetpj 개발의 분석은 cpppj MCP와 `--ui-script`·`session-dump`로 한다 |
| D4 | **한국어(6차)·일본어(13차) 화면을 R8보다 앞당기지 않는다**(권고안) | 결정 D1에 따라 언어 메뉴·표시 부호화 경계는 R3 이식에 함께 들어오지만 검증은 영어로만 한다. 외부 `fonts/`의 D2Coding·Noto Sans JP 글리프 출력과 번역 본문 적재·검증은 R8에서 한다 |
| D5 | **Linux 저장 경로는 `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`**(권고안) | Windows의 `%USERPROFILE%\Saved Games\Netstorm\`(`Saves\`·`CustomMaps\`·`logs\`)에 대응한다. 글자대로 바꾼 `$HOME/.netstorm/Saved Games/Netstorm/...`는 쓰지 않는다 |

**이번 정리로 닫힌 옛 확인 항목:**

- **옛 0.8 7번(알려진 버그 34건의 dotnetpj 반영) — AGENTS 변경으로 해소.** `8661e1060`·`60226ea93`이 dotnetpj 후순위 범위를 4~13차로 넓혀 10차(알려진 버그)가 dotnetpj 범위에 들어왔다. dotnetpj는 34건을 따로 조사하지 않고, cpppj HEAD의 수정 10건(NB02·NB04·NB08·NB09·NB10·NB18·NB20·NB23·NB24·NB29)을 해당 파일을 옮길 때 함께 옮긴 뒤 단위 기록에 "10차 NBxx 포함"으로 적는다. cpppj가 새 수정을 넣으면(LEFT_JOBS.md D4·D5) 같은 방식으로 따라간다.
- 옛 0.8 1번(기존 경로 처리)·3번(cpppj 미복원 경로)·4번(프로젝트 배치)은 결정 5·6·3으로 확정돼 [2.1](#dotnet-current-plan-20261028)에 남긴다.

<a id="user-checks"></a>

### 0.6 사용자 확인 대기 (AI는 실행하지 않음)

| 영역 | 확인할 것 |
| --- | --- |
| 플레이 동일성(L5) | 모든 시나리오(Early Missions·Priest Training·캠페인·데모)의 자연 완주와 전체 화면·음향 동일성. 단계에 이것만 남으면 다음 단계로 넘어간다(2026-10-31 사용자 결정) |
| 기존 경로 | 1-1·1-2 근사 플레이 중 실행 오류 제보(실행 오류 수정만 한다) |
| 환경 | Windows 11 실기는 단계 전환 조건이 아니다(Windows 10에서 오류 없이 구동되면 넘어간다). 다른 모니터/DPI·실물 4K FSR·Program Files/UAC 설치는 R8 이후 |
| 소리 출력 환경 | 0.516 HJOW-Athlon 실장치 검사가 DSERR_NODRIVER로 초기화 실패(cpp도 동일). 활성 출력 장치가 있는 Windows PC에서 Explicit 음소거 재생/복제/정지/재초기화 검사를 재수행한다. 독립 구현/단계 진행은 계속한다([검사 명령](docs/dotnet-sound-device-20261114.md)) |
| MP·LAN | 실제 LAN 접속과 MP 경로 동작(cpppj와 같은 사용자 확인 범위) |

## 1. cpppj 마감분 반영 (2026-11-11)

### 1.1 cpppj 현황과 dotnetpj에 주는 의미

현재 cpppj는 제품1310이며 M8최종배포관문1311을 완료했고 CD복원은 사용자 보류다(LEFT_JOBS.md, 2026-11-14). 0.499에서1304~1310의 PATCH영향을 추가 확인했다. 다음 문단의 과거 AI잔여는 이 현행 상태로 대체한다.

cpppj는 제품 1300에서 AGENTS 1~13차 구현을 모두 마쳤고, 2026-11-11 사용자 커밋 `85551c6c2`가 더한 **14차(QuickJS 엔진 탑재)**도 제품 1301에서 엔진 탑재(cpppj 결정 D7 ① 범위)로 끝냈다. 연동 범위는 cpppj 결정 D7을 기다린다. 이후 1302·1303은 CD 10.72 전용 변경이다. cpppj에 남은 AI 작업은 CD 10.72 복원 C3~C5, M8 최종 관문 재실행, 비행체 표시 보간(F3, 선택)이다([LEFT_JOBS.md 0.1](LEFT_JOBS.md#now)). dotnetpj 범위는 4~14차다.

- **이식 기준이 안정됐다.** 그동안 cpppj가 같은 파일을 계속 고쳐 dotnetpj는 "파일을 건드릴 때 늦게 재대조"했다(0.281). 이제 한 번 재검토(0.4 1-2)한 뒤 HEAD 기준 파일 묶음 단위 이식이 가능하다.
- **요구사항(4~13차)의 해석과 계약이 cpppj 구현으로 확정됐다(14차는 엔진 탑재까지만 확정, 연동 범위는 cpppj D7 뒤).** dotnetpj는 같은 계약을 옮긴다([1.3](#dotnet-r8-requirements)). 옛 문서가 미정으로 둔 해석(MCP 범위·와이드 방식·사용자 홈 범위·Test Battle `.english` 처리)은 cpppj 문서를 따른다.
- **오라클 장치가 갖춰졌다.** `snapshot`(L4a)·`session-dump`(L2)·`--fixed-frame-ms`(L3)·MCP. dotnetpj 쪽 같은 장치는 0.283·0.284에서 만들었다(world 0 덤프까지, 세션 조립에 따라 행을 늘린다).
- **dotnetpj 비대상:** CD 10.72 복원은 cpppj 전용이다(AGENTS가 cpppj에만 둠). 단 cpppj가 C3에서 `Patch1078` 게이트(241파일 306곳)를 판본 분기로 바꾸면, dotnetpj가 이미 옮긴 판본 분기(PATCH/CD) 파일의 재대조가 필요하다. 그 커밋 이후의 영향 검토에서 확인한다. cpppj M8의 ZIP·검증 도구(`tools/verify_cpp_package.py`)는 dotnetpj 배포 검증(R7)의 참고 자료다.
- **cpppj 사용자 결정 중 dotnetpj에 닿는 것:** cpppj D2(비행체 보간) → dotnetpj 60/120프레임 범위, cpppj D3(일본어 본문 번역·다른 언어) → dotnetpj 13차 범위, cpppj D4·D5(알려진 버그 재현 조건·공식) → 수정되면 HEAD로 따라간다, cpppj D6(스커미시 후속) → dotnetpj 12차 범위.

<a id="resync-scope"></a>

### 1.2 영향 재검토 범위 (0.4 1-2의 입력)

| 항목 | 값 |
| --- | --- |
| 지난 검토 | 0.281: `f95a8db2..`(당시 HEAD, 제품 1247) — 자료 629행 중 458행 영향(L1 247·port 211), R4 우선 65행 목록. 재대조는 파일을 건드릴 때 하는 방식으로 미뤘다 |
| 이번 구간 | `4d4b3a928..HEAD`(제품 1247→1300): cpppj 47커밋, `cpppj/src` 184파일 변경(신규 39), 그중 PORT_MAP `cpp` 열에 있는 파일 47개 |
| 주요 변경 묶음 | M6 LAN·Native 등록(1258~1279), M7 5건(helpWord 캐시·메타 맵 커서·희생 안내·오류 로그, 1268~1273), K 한국어(1270·1276·1281), S 시나리오 편집(1277·1282~1284), B 알려진 버그 수정 10건(1285~1294·1300), L 일본어 UI(1288), 11.01 카피라이트(1291), D 난이도(1292), SK 스커미시(1293·1300), F 프레임(1296~1298), M8 배포(1274·1280·1295~1297) |
| 집계 방법 | `git diff --name-only 4d4b3a928..HEAD -- cpppj/src`와 `dotnetpj/PORT_MAP.tsv`의 `cpp` 열 교집합. 행별 `base` 이후 변경은 `git diff --name-only <base>..HEAD -- <cpp 파일>`로 본다(`cpp_sync.py --ports`는 0.147 이후 시간 초과라 쓰지 않는다) |
| 이후 변경(2026-11-12 정적 확인) | 제품 1302(`b833ecb51`: `CdDirectory.cpp/.h` 신규·`ClientMain.cpp` 1줄 — CD 자료 폴더 선택)와 1303(`93a589636`: `main.cpp` 8줄 — `--cd` 판본 옵션)은 모두 CD 10.72 전용이다. PORT_MAP에 있는 `ClientMain.cpp`(재검토 3번)·`main.cpp`(1번)에 닿지만 AGENTS가 CD 10.72를 cpppj에만 두므로 dotnetpj에는 영향이 없다. 두 행의 `base`는 올리지 않는다(두 파일의 잔여는 R1·R8에서 HEAD 기준으로 처리) |

<a id="dotnet-r8-requirements"></a>

### 1.3 R8 — AGENTS 4~14차 차수별 대응

AGENTS(`8661e1060`·`60226ea93`·`85551c6c2`)는 cpppj의 **4~14차**를 dotnetpj 후순위 요구로 둔다(webpj와 달리 제외 차수가 없다). 옛 문서의 4~8차 번호는 7차(오류 로그) 신설로 한 칸씩 밀렸다(옛 7차 AA·FSR → 8차, 옛 8차 시나리오 편집 → 9차, 옛 9차 알려진 버그 → 10차). 각 차수의 해석·세부 계약은 cpppj 구현을 기준으로 옮긴다(결정 2). 기존 경로에 이미 있는 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식으로 다시 만든다.

| 차수 | AGENTS 요구 | cpppj 구현·근거 | cpppj 주요 소스 | dotnetpj에서 할 일 | dotnetpj 현황 |
| --- | --- | --- | --- | --- | --- |
| **4차** | 로컬 MCP(게임 내 모든 조작·화면 캡처·소리 포함 녹화), 16:9 와이드 최대 1920×1080(해상도 지원만, 그래픽 크기 스케일 조정 없음) | 935~937 [MCP](docs/exe/cpp-mcp-server-20261106.md), [와이드](docs/exe/cpp-wide-resolution-20261106.md)·[와이드 덱 배경](docs/exe/cpp-wide-deck-background-20261106.md) | `app/McpServer`, `platform/Json`·`Png`·`AudioLoopback`·`FrameRecorder`, `client/ScreenPresentation`·`ScreenResolutions.h`, 도구 `tools/mcp_cpp_smoke.py`·`mcp_cpp_record_smoke.py` | MCP는 결정 D3대로 dotnetpj에도 붙인다. 와이드는 cpppj 논리 해상도 방식으로 만들고 기존 경로의 시야 확장(`WideScreenMode`)은 교체한다 | 기존 경로에 근사 와이드만 |
| **5차** | 설정·캠페인 진행을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`에, 커스텀 맵은 기존 폴더와 `...\CustomMaps\` 양쪽에서 읽고 Edit 신규 맵은 `CustomMaps\`에 저장, Edit 목록 스크롤·개수 무제한, 구동 중 자동 생성하는 **모든** 파일·디렉터리를 사용자 홈 아래로(읽기 전용 설치에서도 동작) | 932~934 [사용자 루트](docs/exe/cpp-user-data-root-20261105.md)(`CustomMaps\d\NAME.fort`·최초 Edit의 원본 맵 복사·한 화면 50행 스크롤), 1284 [읽기 전용 설치 확인](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) | `platform/UserDataRoot`·`PortableStartup` | 결정 D1에 따라 파일 장치 이식 때 함께 옮긴다(R7 이전부터 사용자 루트). 기존 경로는 R5에서 배포에서 빠지고 R6 뒤 삭제되므로 `settings.json`은 옮기지 않는다(결정 D2). Linux는 `$HOME/.netstorm/`(결정 D5) | 새 경로 파일 장치(0.126·0.127)는 게임 폴더 기준 |
| **6차** | 한국어(D2Coding, 한국어 명칭 규칙) | K1~K5 1270~1281 [K1](docs/exe/cpp-korean-k1-merge-20261111.md)·[화면·입력](docs/exe/cpp-korean-screens-input-verification-20261111.md)·[K4/K5](docs/exe/cpp-korean-k4-k5-closure-20261111.md)·[글꼴 경계](docs/exe/cpp-font-face-encoding-20261103.md) | `client/DisplayTextInput`, 표시 전용 `EncodeDisplayText`/`DecodeDisplayText`, `lang\korean` 번역표, 외부 `fonts\` 적재 | 8비트 화면에 D2Coding 글리프를 그린다. 원문 8비트 문자열과 화면 표시 부호화를 구분한다. cpppj 배포는 저장소의 최신 D2Coding(1.4.0)을 넣으므로, 이때 [5절](#dotnet-user-decisions) 7번의 1.4.0 교체 점검을 함께 한다 | 기존 경로만(D2Coding 1.3.2) |
| **7차** | 오류/예외 처리·상세 로그 공통 구조(`...\Netstorm\logs\yyyyMMddHHmmss.log`, 원인 추적에 충분한 정보) | 1269 [오류 로그](docs/exe/cpp-error-log-20261111.md) | `platform/ErrorLog` | .NET 처리되지 않은 예외(`AppDomain.UnhandledException` 등)와 명시 오류 경로를 같은 위치·이름 규칙으로 기록한다. Linux는 `$HOME/.netstorm/logs/`(결정 D5) | 없음 |
| **8차** | 안티앨리어싱·FSR 1.0 선택(기본 꺼짐), FSR 지원 환경이면 2560×1440·3840×2160 추가(자동 FSR, 실질 1920×1080 업스케일) | 939 [AA·FSR](docs/exe/cpp-aa-fsr-20261106.md) | `client/GpuPresenter`·`FsrConstants`(Direct3D 11 후처리 출력기) | MonoGame 최종 출력 단계의 셰이더로 같은 상수·조건을 옮긴다. FSR 3.1 안은 쓰지 않는다 | 없음 |
| **9차** | Edit에서 시나리오 자체 편집(편집 모드 전용 UI: 초기 자본·테크·적 구성·동맹 등), 새 맵의 `.english` 기본 내용 자동 생성, `.english`가 없을 때 Test Battle 종료 수정 | S1~S4 1277·1282~1284 [S1](docs/exe/cpp-scenario-s1-merge-20261111.md)·[S2](docs/exe/cpp-scenario-s2-test-battle-20261111.md)·[S3](docs/exe/cpp-scenario-s3-ui-20261111.md)·[S4](docs/exe/cpp-scenario-s4-readonly-home-20261111.md) | `client/EditorScenarioController`·`EditorScenarioFiles`·`EditorScenarioModel`·`ScenarioDocument`·`UberScenario.inc` | R7 Edit 이식 뒤 | 없음(Edit 목록·이름 모달까지만) |
| **10차** | 10.78 알려진 버그 34건 확인·수정 | 판정 34/34, 수정 10건 [판정 현황표](docs/exe/cpp-known-bugs-status-20261111.md)·[NB01~NB34 목록](docs/exe/cpp-known-bugs-plan-20261109.md) | 건별(판정표 참조) | 해당 파일을 옮길 때 함께 옮기고 단위 기록에 NB 번호를 적는다. 따로 조사하지 않는다(0.5 아래) | — |
| **11차** | Options 난이도(쉬움·보통·어려움, 기본 보통). 쉬움은 적 AI 다리 건설·유닛 생성 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 주기 ×½ | 1292 [난이도](docs/exe/cpp-difficulty-d-20261111.md)(AI 설정 +0x220/224/228 배율, AI 단위 값) | `o/AiDifficulty` | 세션 AI 설정 소유자(0.306)에 배율 필드를 HEAD대로 두고, 보통이 원본과 같은지 먼저 검증한다 | 없음 |
| **12차** | 스커미시(Campaign 메뉴 항목, 자체 UI: 적 AI 수·AI별 난이도/동맹·금지 유닛 최대 5개(Outpost 포함)·시작/취소, 섬 배치·간헐천은 MP처럼 랜덤) | 1293·1300 [스커미시](docs/exe/cpp-skirmish-sk-20261111.md)(미션 시작 전 `prepareWorld`가 군도 밀도→중앙섬→참가자별 전투섬 순서로 생성) | `client/SkirmishSetup`·`SkirmishMapList.inc`·`SkirmishStartIslands.inc` | 이미 옮긴 섬 생성(중앙·전투·군도 분리, 0.156)과 간헐천 생성을 MP 시작 순서로 묶는 경로를 옮긴다. 계약은 LEFT_JOBS.md 4.1 '스커미시 월드' | 없음 |
| **13차** | 기타 언어(일본어 — 추후 추가 예정) | 일본어 UI 1288 [일본어](docs/exe/cpp-japanese-l1-l3-20261111.md)(번역표 509키·외부 Noto Sans JP·`.japanese`가 없으면 영어 본문) | 번역표·표시 부호화·외부 `fonts\NotoSansJP-*` | 6차 다음. 본문 번역 범위는 cpppj D3을 따른다 | 없음 |
| **14차** | QuickJS 지원([quickjs-ng](https://github.com/quickjs-ng/quickjs)). 일단 엔진 탑재만 하고 어느 정도까지 연동할지는 추후 결정(AGENTS `85551c6c2`, 2026-11-11 추가) | **cpppj 1301 탑재 완료**([근거](docs/exe/cpp-quickjs-engine-20261111.md): QuickJS-ng v0.18.0 정적 링크, 엔진 생성·해제, 시작 자체 검사 6항목, `--quickjs-selftest`, 메모리 16MiB·스택 256KiB·1초 한도, 게임 상태·파일·네트워크 비노출). 연동 범위는 cpppj 결정 D7 대기 | `third_party/quickjs-ng/`, `platform/ScriptEngine` | cpppj의 탑재 계약(D7 ① 범위)을 같은 계약으로 옮긴다(결정 D1). AI 기본안: quickjs-ng 네이티브 라이브러리를 대상 플랫폼별(Windows x64 먼저, Linux는 9단계)로 빌드해 P/Invoke로 적재하고, 글꼴처럼 exe 밖 파일로 배포 ZIP에 라이선스(MIT)와 함께 넣는다. AGENTS가 QuickJS를 지정했으므로 Jint 같은 관리형 JS 엔진으로 대체하지 않는다. 엔진이 외부 접속을 하지 않음을 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)에 적는다 | 없음 |
| 후순위 | 60/120프레임 | F1·F2 1297·1298 [계획·결과](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)(옵션 `Frame Rate` 60/75 원본/120, 양보 대기로 CPU 점유 감소·프레임 시간 동일) | `client/FrameRateOption` | 0.4 8단계 | 기존 경로에 근사 `FramePacing`만 |
| 후순위 | Linux(DesktopGL) | — | — | 0.4 9단계, 경로는 결정 D5 | 없음 |
| 최후순위 | TCP/IP 로컬 멀티플레이·Outpost(3차) | M1~M7 [LEFT_JOBS.md 1절](LEFT_JOBS.md#done) | `client/Native*`, `src/zacket`, `o/RawPacket*`·`RawConnection*` 등 | 로컬 네트워크만(`LocalNetworkPolicy` 루프백·리슨) | 사전 구현(검사 없음): `Netstorm.Net` 0.152·0.287·0.307·0.321·0.324·0.330~0.333, 세션 에디터 전송 0.293 |

**단계와 무관한 AGENTS 공통 요구:** `Netstorm.exe` 이름(결정 D2: R5에서 새 호스트로 이전), 11.01 표기·Credits 3항목(완료 0.150), `Copyright 2026 HJOW`(0.4 1-3), 외부 접속 문서화·클라이언트 유효성 검사 비활성화(완료 0.151, 새 경로를 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md) 갱신), 글꼴 exe 내장 금지·exe 옆 `fonts/`(기존 경로 준수), 배포 스크립트 `package-dotnetpj.ps1`(인자 또는 입력받은 경로·빈 값이면 취소·실행 파일과 필요 에셋 전부 포함 — 현행 준수, 구성이 바뀔 때마다 압축 해제 실행 확인).

<a id="cpppj-requests"></a>

### 1.4 cpppj에 전달할 요청 (이식 계획 8절의 남은 것)

dotnetpj 작업에서는 cpppj·LEFT_JOBS.md를 수정하지 않는다. 아래 요청은 사용자에게 전달한다. 요청 1·2(`session-dump`·고정 시각 실행)는 cpppj `c91d9b84`와 dotnetpj 0.283·0.284로 해결됐다.

| 번호 | 요청 | 상태(2026-11-11) |
| --- | --- | --- |
| 3 | fixture TSV 첫 주석 줄에 열 이름 | 남음 |
| 4 | 모듈별 연결 상태표(독립 대조만 / 실제 Session 연결 / GUI 연결) | 남음. cpppj [SOURCE_MAP](cpppj/SOURCE_MAP.md)로 대신할 수 있는지 0.4 1-2에서 확인 |
| 5 | `cpppj/tests/GraphTests.cpp:105`가 `graph-x86.tsv` 객체 열 `v[11]`(범위 밖, 객체는 정수 11개)을 읽음 | 남음(HEAD에서도 같은 줄) |
| 6 | `world.ClearProcesses()` 뒤 `FindEvent(sid,1)`가 null 대신 미등록 진단(`PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular` 꼬리, 0.305) | 남음. dotnetpj는 원본 동작 그대로 둔다 |

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a>

## 2. 현행 계획 — cpppj를 오라클로 한 구조 보존 직역 (2026-10-28 수립, 2026-11-11 갱신)

**진단(2026-10-28):** 기존 구현은 화면이 RGBA 합성이고, UI가 원본 Gump 트리가 아닌 자체 배치이며, 규칙이 근사 세션(`BattleSession`)에 있었다. 옮겨 둔 `Original` 모듈은 세션에 연결되지 않아 플레이를 바꾸지 못했고 같은 입력에서 cpppj와 비교할 수단도 없었다. 그래서 아래 구조로 다시 세웠다.

### 2.1 구조 결정 (계속 유효)

- **결정 1 — 화면은 8비트 논리 화면으로 옮긴다.** cpppj의 `Screen`·`Renderer`·팔레트·셰이프·색표·Gump Draw를 C#으로 옮긴다. 그리기 대상은 팔레트 인덱스 버퍼(`byte[]`, 행 간격 포함)와 256색 팔레트다. MonoGame은 창·입력 장치·오디오 장치·**최종 출력**만 맡는다. 최종 출력은 인덱스를 RGBA로 바꿔 텍스처를 갱신하고, cpppj `ScaleScreenPresentation`과 같은 정수 픽셀 중심 최근접 확대와 검정 여백을 쓴다. 게임 화면 경로에서는 SpriteBatch 합성·실수 알파·`RenderTarget2D` 확대를 없앤다. 9-1 E(부분 다시 그리기)는 cpppj Renderer의 dirty 경로로 해소한다.
- **결정 2 — cpppj 구조를 보존해 직역한다.** cpppj의 파일·클래스·함수 경계와 이름, 부작용과 호출 순서, 정수 폭(BYTE/WORD 감김), float32 저장 지점, 목록 순서, 난수 소비를 그대로 옮긴다. cpppj가 원본 메모리 배치(슬롯·전역 DWORD)를 쓰는 곳은 C#도 같은 배치를 쓴다. Win32·DirectDraw·GDI·WINMM·파일 장치처럼 cpppj에서도 대체 경계인 부분만 .NET/MonoGame으로 바꾸고 차이와 이유를 한국어 주석에 적는다. Windows 전용 동작(커서 제한 등)은 호스트의 OS 분기에만 두어 Linux 작업을 막지 않는다. 요구사항 코드가 섞인 파일도 그대로 옮긴다(결정 D1).
- **결정 3 — 프로젝트를 cpppj 디렉터리와 짝지어 배치한다.**

  | 프로젝트 | 대응하는 cpppj | 내용 |
  | --- | --- | --- |
  | `Netstorm.Core`(`Original/` 등) | `src/o` | 원본 월드 코어·규칙 모듈. 기존 경로용 근사 코드(`Simulation`·`Rules`·`Bridges` 등)도 아직 같은 프로젝트에 있다 |
  | `Netstorm.Client` | `src/client` 중 플랫폼 독립 부분 | 화면 버퍼·Gump·Renderer·세션·HUD·소리 논리·Chat·Mission. MonoGame에 의존하지 않는다 |
  | `Netstorm.Client.Host` | `src/app`·`src/platform`의 대체 부분 | 새 경로 MonoGame 호스트(개발용) |
  | `Netstorm.Game` | — | 기존 경로 MonoGame 게임(동결, 결정 5) |
  | `Netstorm.Assets` | `GameAssets`·`RiftTypeTable`·`Template`·`Config*` 등 | 자료 판독. cpppj HEAD와 재대조한다 |
  | `Netstorm.Net` | `src/zacket`, 연결 계열 | 멀티플레이. 최후순위(현재 사전 구현만) |

  대응표는 [PORT_MAP.md](dotnetpj/PORT_MAP.md)·`PORT_MAP.tsv`(cpppj 파일 → C# 파일 → 이식 기준 cpppj 커밋 → 상태)이며 [cpppj/SOURCE_MAP.md](cpppj/SOURCE_MAP.md)와 짝을 이룬다.
- **결정 4 — cpppj 실행 파일을 판정 기준(오라클)으로 쓴다.** cpppj는 원본 기계어와 대조된 결과이므로 dotnetpj는 같은 입력에서 cpppj와 같아야 한다. 화면은 cpppj `--run … --ui-script <steps.tsv>`의 `snapshot` 논리 캡처(팔레트 변환 BMP)를, 상태는 헤드리스 출력(`--dump-world`·`--dump-types`·`--inspect-*`)과 ui-script `session-dump`(L2)·`--fixed-frame-ms`(L3)를 기준으로 쓴다([형식](docs/exe/cpp-session-state-dump-20261109.md)). dotnetpj 호스트도 같은 문법의 실행기와 같은 형식의 캡처·덤프를 낸다. 비교 도구는 `dotnetpj/tools/`에 둔다.
- **결정 5 — 기존 경로는 동결한 뒤 교체·삭제한다.** 기존 `BattleSession`·`FortMapViewer`·`MainMenuView` 등에는 새 기능 추가와 부분 수선을 멈춘다(실행 오류 수정만 예외). 새 경로가 메뉴와 미션을 모두 맡게 되는 R5에서 기본 실행과 `Netstorm.exe` 이름·배포 대상을 새 경로로 넘기고, 기존 경로는 `NetstormLegacy`로 내려 비교·개발용으로만 둔다(결정 D2). 1-1·1-2가 새 경로에서 같은 수준 이상으로 진행되면 기존 경로와 그 검사를 삭제한다. 근사를 기대하는 기존 검사는 기대값을 고치지 않고 대체되는 코드와 함께 은퇴시키며 은퇴 목록을 단위 기록에 남긴다. 원본 fixture를 쓰는 검사는 대상 모듈을 옮긴 뒤에도 유지한다.
- **결정 6 — cpppj가 미완·보류로 둔 범위는 추측으로 채우지 않는다.** cpppj가 명시 경계·미복원·보류로 둔 동작은 dotnetpj 새 경로에서도 같은 경계로 둔다. cpppj가 명시 오류로 멈추는 미연결 경로는 같은 지점의 명시 예외·진단으로 옮긴다(예: LEFT_JOBS.md 4.1의 NoWorld 경계). 옛 "미복원 동작은 근사를 유지한다"는 동결된 기존 경로에만 남는다.

<a id="04-기존-dotnetpj-소스의-재수정-판정"></a>

### 2.2 기존 dotnetpj 소스의 재수정 판정

"교체"는 cpppj 대응 모듈을 새로 옮긴 뒤 기존 파일을 지운다는 뜻이다. "재대조"는 cpppj HEAD와 같은 입력에서 결과·오류 처리를 비교해, 같으면 유지하고 다르면 cpppj 구조로 바꾼다는 뜻이다. 아직 은퇴시킨 기존 코드·검사는 0건이다.

| 기존 dotnetpj | 판정 | 기준이 되는 cpppj | 단계 |
| --- | --- | --- | --- |
| `Game/NetstormGame`·`DisplayManager`·`PerfMeter`·`FrameWallClock`, `Core/Display/ScreenLayout`·`WideScreenMode`·`FramePacing`·`DisplaySettings` | 교체. 호스트에는 창·입력·오디오 장치·출력만 남긴다. 와이드 시야 확장은 cpppj 논리 해상도 방식으로 바꾼다. 새 경로는 cpppj와 같은 사용자 루트의 `options.cfg`를 쓴다(결정 D1). 기존 경로의 `settings.json`은 기존 경로와 함께 삭제한다 | `ClientMain`, `Screen`, `ScreenPresentation`, `ScreenResolutions.h`, `ScreenPaletteApply`, `FrameRateOption` | R1(설정은 R7) |
| `Game/UiAutomation` | 교체 | `app/main.cpp`의 `--ui-script`·`snapshot` | R1 |
| `Core/Display/OriginalGameClock`·`FrameLoopClock`, `Simulation/BattleSessionFrameClock` | 유지 후 재대조(L1 검증 있음). `ClientMain` 루프에 맞춰 새 호스트에 다시 연결 | `ClientMain`, `o/GameClock`, [main-loop.md](docs/exe/main-loop.md) | R1 |
| `Game/SpriteAnimation`·`FortMapViewer.Sprites`, `Assets/ObjectColorRemap`·`IsleColorRemap`·`ProductionTintRemap`·`MapSpriteFrames`·`StructureAnimation`·`BitmapFontAtlas`, `Core/Display/DrawOrder`·`DisplayBounds` | 교체. `DrawOrder`·`DisplayBounds`의 L1 대조는 새 모듈의 검사로 흡수 | `VFXDraw`·`VFXMappedDraw`·`VFXLine`·`VFXTransform`, `RawShapeToBuffer`, `SpriteDraw`, `*ColorMaps`, `Palette*`, `DrawSort`, `o/SquidDisplay` | R2·R5 |
| `Game/MainMenuView`·`OriginalUiSkin`·`HelpWindow`·`OriginalFonts`·`OriginalCursor`, `Core/Rules/ButtonGump`, `Assets/CampaignMenu`·`StartupTips`·`HelpDocument`·`HelpTopics`·`InlinePicture` | 교체 | `Gump*`, `ButtonGump`·`StoneButtonGump`, `MenuGump*`, `ScrollGump*`, `TextGump`, `HtmlWindow*`·`HtmlBodyGump`·`Markup`·`StyleText*`, `QuickHelp*`, `TellTip`, `UberGump`·`UberRootGump`, `NativeMainMenuState`, `BitmapFont`, `Cursor` | R3 |
| `Assets/BitmapFont`(`.chfnt`)·`Palette`·`GifImage`·`ShapeDatabase`·`CursorBitmap`·`TaffArchive`·`XlatTable`·`XorCipher`·`WaveFile`·`PatternDecoder`·`Config*`·`Type*`·`OriginalText`·`GameFileSystem`·`GameDataLocator`·`GameResources`·`GameLanguage`·`GameUnitNames`·`TrueTypeCollection` | 재대조. L1/정적 대조 기록은 있으나 기준 시점이 10-07~10-10이다 | `GameAssets`, `RiftTypeTable`, `Template`, `Config*`, `Xlat`, `GifImage`, `BitmapFont`, `RawImage` | R0·R2 |
| `Assets/FortFile`·`FortMap` / `FortTerrainPreview`·`FortTerrainFringe`·`FortEdgeFarmPreview`·`FortIslandSupports` | Fort 판독은 재대조(`FortFile`은 연결하지 않은 옛 판독기라 R4 Fort 적재 연결 때 은퇴), 지형 미리보기 4종은 교체 | `RawFort*Load`, `RawCampaignWorldLoad`, `TerrainBuilder`, `RawTerrain*`, `Islandbuilder`·`RawIsland*`, `RawFarmCluster` | R4 |
| `Assets/MissionScript`·`MissionConditions`·`TutorialDialogScript` | 교체 | `TellSection`·`TellParameters`·`DialogScript`·`Html*`·`ScriptCommand`·`RawMissionGuide` | R3·R6 |
| `Core/Simulation/BattleSession.*`(15파일)·`BattleSessionFactory`·`GameEntity`·`GameCommands`·`SessionEvents`·`PlayerState`·`UnitProduction`·`*Animation`·`CombatImpact`·`FlyerFlight`·`MovementRate`·`UnitHeading`·`ConstructionTimes`·`TutorialStages`·`TutorialGeysers`·`FixedTimestep` | 교체 후 삭제 | `RawCampaignSession*`, `RawCampaign*Bindings`, `RawSurfaceWorld`, `State`, `Mission*`, 대응 `Raw*` | R4·R6 |
| `Core/Rules/*`(25파일: `BattleMap`·`SpotMap`·`Footprint`·`IslandOwnership`·`Energy*`·`StormPower`·`ProductionDeck`·`KnowledgeCatalog`·`PlacementCursor`·`RangeIndicator`·`SelectionMarks`·`GameCursor`·`MiniMapLayout`·`PlayerColors`·`MissionStart` 등) | 교체 후 삭제. `SpotRules`의 L1 대조는 새 해시 모듈 검사로 흡수 | `SquidHash`·`SquidSpatial`, `CanonDecoder`·`RawCanonPlacement*`, `RawEnergySupply`, `SpStore`, `RawProductionDeck`, `RawPlayerTechnology`, `RawCursor*`, `MinimapGeometry`, `RawPlayerColor`, `RawMissionStart` 등 | R4~R6 |
| `Core/Bridges/*`(13파일) | L1 대조된 `BridgeDecayRules`·`SurfaceFinder`·`BridgeSurfaceRules`·`SurfaceGraph`는 `Original` 쪽 대응 모듈로 합친다. 나머지는 교체 후 삭제 | `Bridge`, `Graph`·`RawGraph`, `SquidFinder`, `RawBridge*`, `CanonDecoder` | R4 |
| `Simulation/MissionFrameGate`·`PlayerParticipation`·`MissionParticipation`·`TotalMade`·`TutorialOne`·`NetstormRandom`·`MsvcRandom`·`SidPool` | 유지 후 재대조(L1 검증 있음). 세션 조립 때 cpppj와 같은 소유 위치로 재배선. `SidPool`은 `RawSidPool`과 하나로 정리 | `RawMissionFrameGate`, `RawPlayerParticipation`, `RawTotalMade`, `RawTutorialOne`, `CrtRandom`, `SidPool` | R0·R4 |
| `Core/Original/*` | 유지. 영향 재검토(0.4 1-2) 뒤 세션 조립에 쓴다 | 대응 `o/*` | R0·R4 |
| `Core/Audio/*`, `Game/AudioPlayer`, `FortMapViewer.Audio` | 교체. 장치 계층만 MonoGame | `Sound`·`SoundProcess`·`SoundMusic*`·`SoundSceneMusic`·`SoundDevice`·`ClientAudio` | R6 |
| `Game/FortMapViewer*.cs`(26파일) | 교체 후 삭제 | `Renderer`·HUD·커서·선택·메뉴, `RawCampaignSession`의 입력 | R5·R6 |
| `Game/SpriteBrowser` | 개발 도구로 유지하되 게임 경로와 분리 | — | 필요 시 |
| `package-dotnetpj.ps1` | 유지. 프로젝트·자산 구성이 바뀔 때마다 갱신하고 압축 해제 실행을 확인한다. 글꼴은 exe 옆 `fonts/`에 별도 파일로 넣는다. R5 전환 때 배포 대상을 새 호스트(`Netstorm.exe`)로 바꾸고 `NetstormLegacy`는 넣지 않는다(결정 D2) | `cpppj/package-cpppj.ps1`(참고) | 매 전환 |

<a id="stage-criteria"></a>

### 2.3 단계별 범위와 완료 기준 (R3~R7)

각 단계 괄호의 W 번호는 [이식 계획](docs/dotnet-cpp-port-plan-20261016.md) 5절의 근거 목록이다. R0~R2는 완료했다([7절](#dotnet-done-index)).

**단계 전환 기준(2026-10-31 사용자 결정, cpppj와 같음):** 모든 시나리오의 자연 완주와 전체 동일성은 사용자가 직접 플레이하며 확인하고, 단계에 남은 일이 이것뿐이면 다음 단계로 넘어간다. 각 단계의 L1~L4 대조는 AI가 한다. Windows 10에서 오류 없이 구동되면 Windows 11 실기 검증 없이 넘어간다. 사용자 제보는 해당 항목을 AI 작업으로 올린다.

- **R3 — Gump UI와 메인 메뉴 계열(W15) → 이정표 D1: 메뉴 화면 픽셀 일치.** 옮길 것: Gump 프레임워크 전체, 버튼·돌 버튼·메뉴·스크롤·텍스트·텍스트 입력·HTML 창·메시지·이미지·배경 GIF 위젯, `UberGump`·`UberRootGump`·`RawUberRootControls`·`NativeMainMenuState`, 로딩 화면(`MissionLoadingMessage`·`RawMenuLoading`), Tell·Dialog·StyleText·Markup·QuickHelp, 기본 커서. 범위: 메인 메뉴 8버튼, Campaign/Demo/Help/Options(해상도·음량·Window Mode)/Credits/Tips/Quit, 캠페인 목록, 브리핑 창(결정 D1에 따라 cpppj HEAD의 Difficulty·Frame Rate·Language·Skirmish 항목도 함께 옮기되 기본 설정 화면으로 비교). **완료 기준:** 같은 ui-script로 각 화면·하위 메뉴·Help 쪽·Credits 쪽을 캡처해 cpppj와 차이 0. 메뉴 기본 실행 교체는 R5에서 미션과 함께 한다(결정 D2).
- **R4 — 월드 세션 조립(W02·W03 잔여, W04~W07) → M1 시작 상태 L2.** 옮길 것: 시간·Kernel·Process·Regular, SP 저장소·보상·난수의 실제 연결, Fort 적재(객체·월드·메타·돈·소유자 마감·TypeNames·Deck), 지형·섬·농지·영역, 다리 연결, Player·기술·색, 미션 Start·로그인·타이머·Guide, AI 생성과 초기 갱신, 생산 Deck. `RawSurfaceWorld`/`RawWorldCore`와 `RawCampaignSession`의 소유 구조를 그대로 옮기고 기존 `Original` 모듈을 그 안에 끼운다. **완료 기준:** 34경로(Early6·Training9·캠페인16·데모3) 각각의 Load/Start 뒤 상태가 cpppj와 같다. cpppj 쪽은 ui-script `session-dump`(판번호 1)로 얻고 dotnetpj 호스트도 같은 형식을 낸다. 양쪽 모두 같은 `--fixed-frame-ms`와 빈 사용자 루트를 쓴다([6.1](#dotnet-commands)).
- **R5 — 월드 표시·카메라·HUD(W14 나머지, W15의 HUD) → 이정표 D2: 진입 화면 픽셀 일치.** 월드 표시(`Renderer`·`DrawSort`·`DrawSubmission`·`RawRenderCollection`·`ObjectCollection`·`TileGroupCollection`·`PreviewCollection`·`PreviewDraw`·`SquidRenderer`, 지형 바인딩, 타입별 Draw, 시각 시스템 Burning·Construction·Dissolve·FieryTail·Lightning·Shrapnel·Smoke·RenderProcess, KenCloud, 파티클, 번개 화면, Fort 팔레트 전환), 카메라(`RawCameraView`·`RawCameraCommit`·`RawCameraPresent`·`CameraHome`·`UberGumpCamera`·`RawEdgeScroll`), HUD(`UberHud`·`HudDock`·`HudNsInfo`·채팅 표시, 미니맵, `FloatingSpGump`·`ProductionHintGump`, 덱 `InventoryGump`). **완료 기준:** 34경로의 진입 직후와 N프레임 뒤 캡처가 cpppj와 차이 0. 애니메이션 장면은 같은 `--set`(maxFPS·sleepPerLoop 등)과 `--fixed-frame-ms` 조건에서 비교하고, 맞출 수 없는 장면은 비교 범위를 기록한다. 이 시점에 메뉴·미션의 기본 실행과 `Netstorm.exe` 이름·배포 대상을 새 경로로 넘긴다(결정 D2).
- **R6 — 플레이 입력·규칙·소리(W08~W13, W16, W19의 AI) → M2·M3.** 입력·선택·커서(`UserInput`·`GameWorldInput`·`RawUserPick`·`RawObjectHit`·`RawUserSelection`·`RawUserHighlight`·`RawCampaignCursor*`·`RawCursor*`), 게임 메뉴(QWASZX `RawProductionControls`·`ProductionShortcuts`, ESC 가로·하위 메뉴, Player List·F9, 우클릭 메뉴 전체, Warning·MessageBoard), 규칙(사제·경로·운반·수확, 배치·건설·생산, 전투·방어·파생 건물, 의식·신전·제단, 미션 진행·안내·승패, 공통 전략 AI `RawAi*`), 소리(소리 Process·음악 선택·흐름·장면 음악, 장치는 MonoGame). `arc-spire`·`acid-fence`·`ice-tower` fixture는 해당 몸체 클래스를 옮길 때 연결한다. **완료 기준:** 고정 입력열의 L3 상태 해시(프레임별 `session-dump`)와 대표 장면 캡처가 일치하고, 1-1의 건설 파이프라인이 원본 경로로 동작한다(M2). 1-1·1-2가 새 경로에서 진행되면 기존 경로와 그 검사를 삭제한다.
- **R7 — 미션 전체·Edit·설정·배포(W17·W18, 9-1 C·D·F) → M4.** 34경로 전체, 데모 3개의 반복, 진행 저장과 다음 미션 해제, Retry/Next/Abort. Edit(편집 메뉴·섬 추가·저장·Test Battle·SaveGoMain, `.english` 등 언어 파일; Edit 제외는 webpj에만 해당). `options.cfg`를 원본의 시점·형식으로 읽고 쓰되 전체화면 전환 뒤 재실행 오류는 허용하지 않는다. 9-1 D·F를 판정하고 Windows 10에서 확인한다. 저장 위치는 cpppj와 같은 사용자 루트다(결정 D1). 배포 스크립트를 갱신하고(`Netstorm.exe`, 글꼴은 exe 옆 `fonts/`) 압축 해제한 사본에서 실행되는지 확인한다. **완료 기준:** 34경로 짧은 진입과 핵심 장면이 cpppj와 일치한다. 자연 완주(L5)는 완료 기준에 넣지 않는다.
- **R8** — [1.3](#dotnet-r8-requirements) 표.

<a id="r4-detail"></a>

### 2.4 R4 진행 상세 (0.4 2단계)

현재 위치·남은 조각·순서는 [0.4 R4 진행판](#r4-board)이 기준이다. 여기는 완료 묶음과 남은 항목의 목록이다.

**완료 묶음** (단위별 상세는 보관본의 단위 기록과 [7절](#dotnet-done-index) 색인):

- Template/DataManager와 저장소 TS0~TS6(0.128~0.136), Fort 타입 표·적재·저장 F1·F2a~F2c-12(0.137~0.155).
- 지형·섬 생성 1·2차와 Fort 사제 준비(0.156~0.160), 일반지형 요청형(0.174), F2c 정상 생성·도장 본체(0.285), 월드 도장 훅(0.291), 지형 패턴 장면값 재생(0.297).
- 다리·섬 연결 본체와 판본별 전수 재생(0.161·0.162), 섬 수명주기·후처리 전수 재생(0.289·0.290, `islandlifecycle-x86.tsv` 2055행·`islandpostpop-x86.tsv` 1988행).
- F2d Player Core·초기화·Chat·참여·CoreData·Player 월드, 캠페인 월드 전역·적재, 서피스 삭제 분배기, 저장 나머지(0.163~0.171). Player 월드 CD 분기·후처리 순서 계약·잔여 순서 계약·가상 분배·월드 직접 조립 검사(0.298·0.299·0.303~0.305), Money 월드 공유(0.300), 공통 몸체 실패 순서(0.301), Subscriber 현재 읽기(0.302).
- 서피스 월드 기반·프로세스·호스트(0.172~0.225), 월드 본체 조립(0.226~0.264: `RawSurfaceWorld` 본체·`RunFrame`·수명 분배기 등).
- F3 세션: 껍데기·Fort 씨앗 적재·프레임/보고서 위임·섬 개수(0.265~0.271), Stub 관문 8종(0.272~0.279)의 본체 연결(0.286·0.288·0.292·0.294~0.296). 세션 소유 상태: Player 상태·Battle(0.286), outposts/temples 목록(0.288), AI 설정(0.306), 미션 시작 설정(0.313), 전투 옵션 87바이트·숨은 플레이어 뷰(0.319·0.320), 현재 인지(0.322), TYPE 표(0.327), 동맹 표 81칸(0.341). AI 관문: 생성·준비·초기화·다리 갱신·자금·다리 경로 전진(0.308~0.312·0.314). 동맹: 해제·협상 수신·무승부 마감·요청 입력(0.315~0.318). 객체 소유자 지정과 세션-월드 연결 검사(0.323·0.328), 가변 로컬 패킷 송신(0.326).
- 채팅 형식·추적·본문 조합·패킷·수신·이름 자동완성(0.334~0.339), Player 식별자(0.340), 아카이브 열거 검사(0.325).
- 재개 준비: 전체 회귀 재기준선(0.280), cpppj 영향 확인(0.281), cpppj 재반영 2건(Demo `Demoville`·Help 스크롤 화살표 색, 0.282), 고정 시각 실행·`session-dump` world 0(0.283·0.284), 전체 회귀(0.329).
- 사전 구현(검사 없음, 3차 범위): LAN TCP/UDP 루프백(0.287), 세션 에디터 전송(0.293), 로컬 미션 Zone(0.307), 연결 활성(0.321), 전투 방송(0.324), 패킷 등록 표(0.330), 연결 등록·정리·종료(0.331~0.333).
- **실제 호스트 조립 착수(0.390~0.459):** 미션 Fort 적재, 세션 홀더·world-1 덤프, 미션 플레이어·SP·AI·동맹, 마스크·Zone·가입·Ready, 요새 순번·이름 확정, 월드 적재 시작 흐름, 타입·프레임 채우기, 지형·난수 저장소, 섬 부모 조립, 지형 바인딩 입력 값 연결, 바인딩 조립·드라이런, Generate 라이브 연결, 난수·덤프 분석, 난수 단일 스트림 연결, 프리-Generate 소비 확정, 프레임 선택 대조(묶음별 단위는 [0.4 R4 진행판](#r4-board)). 같은 구간에 오라클 갱신·L4a 메뉴 12장면(0.380~0.389)과 cpppj 영향 재검토(0.343~0.379)가 있다. 전체 회귀 재완주(0.453: 3,441건, 실패 0).

**남은 항목:**

- [ ] **실제 호스트·월드 조립:** *(0.390~0.459에 Player·SP·AI 설정·동맹·Zone·가입/Ready·이름·요새 순번과 월드 적재 11단계 순서·타입/프레임·지형/난수 저장소·섬 부모·지형 바인딩 입력 값 연결을 호스트에 연결했다. 나머지는 [0.4 R4 진행판](#r4-board)의 2~6번.)* 미션 객체, Fort reader, Guide 교체·수명, 미션 제어 입력, 공통 Start, AI 설정·생성 관문, Guide 단계·Lifecycle은 이식과 L1 대조를 마쳤지만 검사에서는 명시 효과로만 연결돼 있다. 실제 호스트에 분배기를 설치하고 같은 Player·제한 표·메뉴·월드 효과에 연결한다(0.66·0.68·0.69·0.75~0.79). Tutorial Init의 실제 템플릿/옵션 소유자·요새 선택·상태 실행·실제 타입 조회·호스트 단계 연결(0.105~0.110), Start의 실제 Config·Player·SP·AI·Gump 효과(0.70), `PlayerParticipation`의 실제 세션 바인딩과 동맹 수신 `ReceiveAlliance`(0.74), 동맹 표의 `ClientPlayerRecords` 공유(0.341), 세션 TYPE 표의 메뉴·월드 연결(`HostEditorSelection`의 188 상수 교체, 0.327)이 남았다. 세션 대리자(`Bind*`) 묶음은 이 조립에서 실제 몸체에 연결한다.
- [ ] **AI 나머지:** `CrankAiMoney`·`SetAiParameter`(players·aiMemory 필요), `ResetAiForState`·`ReturnAiForState`(participation 필요), 실제 AI 몸체(`RawAiCreate.Create`의 600바이트 검사, 준비 00416eb0, 258 완료 판정 등). 11차 난이도 배율 필드(`o/AiDifficulty`)는 0.347에서 이식해 `RawAiCreate`에 재대조했다(`AiDifficultyTests`·`RawAiCreateTests` 21건). 2번 brain 생성·부착은 0.399에서 연결하고 부착 recipe는 0.412에서 확정했으며 실생성자0.500(160입력)·기술부모0.501(158입력)·초기갱신0.502(149입력)부품은완료했고 호스트 토큰/결선은아직스텁이다([R4 진행판](#r4-board) 6번).
- [ ] **미뤄 둔 전수 재생:** `player-world-x86.tsv` 1680행(월드 스택 하네스 필요), 0.154에서 옮기지 않은 cpp 검사(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`의 전체 월드 장면 등), 0.134~0.139에서 F3로 미룬 검사(`CampaignMissionFortPathTests`·`CampaignLoginObjectStreamTests`·`CampaignMissionReloadSourceTests`·`TemplateDiagnosticClientTests` 일부·`SoloMapCatalogTests`의 Tell 메타 1종·`CampaignNewFortTests`·`CampaignNewFortConfigTests`). 가상 분배 진입(`PostPopVirtual`)의 나머지 파생 분기.
- [ ] **M1 판정:** 34경로의 Load/Start 뒤 상태를 cpppj `session-dump`와 대조한다(L2). cpp 기준선은 4경로(thewarbegins 0.390·masterofwhirligigs 0.433·savetheisland 0.445·fragilefortune 0.449)를 확보했고 새 경로가 일치한 경로는 아직 0개다. 나머지 30경로의 기준선은 월드 행이 나온 뒤 만든다([R4 진행판](#r4-board)).

<a id="r3-r2-detail"></a>

### 2.5 R3·R2 잔여와 9-1 A

- [ ] **R3 잔여 → 이정표 D1:** 위젯 본체와 실제 호스트의 표시·입력 경로는 대부분 옮겼으나 R3 완료로 세지 않는다.
  - 대화창 분리(`ParseDialogPage`)와 `ConfigOverwriteAndQueueLifetime`의 남은 부분, MessageGump·대화창.
  - 맵/파일/기술 목록 owner, 실제 상위 메뉴 소유·분배, `RawMenuControls`의 전체 모달·등록 소유자와 링크·명령 소비자, Guide 우선 연결 등 기타 상위 명령·실제 맵/기술 목록 조립, HTML 그림 자산과 다른 파생·월드 통합 출력.
  - 전체 자산 조립, `ReadAssetQuickHelpType`의 TYPE/AST·요구 에너지 후처리, 음향.
  - `RawTextEntryControls`의 Console·부분 생성 정리·시스템 클립보드, Save as·숫자 창·ChatWindow, `RawBackgroundGifControls`의 노드·창 소유자.
  - 호스트 프레임·입력 큐, report·kernel-tell: 메인 메뉴 라벨 좌표는 실제 버튼에 연결했고(0.85) 하위 화면 라벨과 실제 호스트 명령 소비가 남는다.
  - 이정표 D1 메뉴 캡처 비교: ✅ 12장면 차이 0(0.381~0.388, 장면은 0.1). 남은 3장면(Language 자식 CJK·Multiplayer 섬 화면·Edit 이동행)은 R8·R5·R7로 미뤘다.
- [ ] **R2 잔여 세션 연결:** 실자산 어댑터, 수집·정렬·kind 분배, `SpriteDraw` 클래스·그림자 alias 공유, `owner-capture` C행 480개는 R4·R5에서 잇는다.
- [ ] **9-1 A 나머지([4절](#original-parity-plan)):** 정수 틱을 실제 시각·원본 타이머로 옮기는 일을 세션 교체와 함께 처리한다.

<a id="carried-limits"></a>

### 2.6 이월한 이식 한계 (이후 단위에서 해소됐다는 기록이 없는 것만)

- 0.80: 초기 구름 카메라 원점은 임시 범위이고 실제 카메라(R5)·시간별 이동·부분 dirty는 미연결이다. 512×512 구름 파일이 없거나 크기가 다를 때의 원본 생성 fallback은 이식하지 않아 명시 오류다.
- 0.81: 반복 `--set`은 C# 호스트가 병합하고 같은 이름은 마지막 값을 쓰지만 cpppj는 마지막 묶음만 보관한다. 오라클 연결 때 계약 차이를 정리한다.
- 0.86·0.87: 프레임 사이의 빠른 물리 입력 이력, 공통 음향·음악(R6 범위)과 옵션 변경 중 재생 관리가 남았다.
- 0.93: 부분 생성 중 돌 버튼 내부 배경 할당 실패의 모든 사례를 검증하지 않았다.
- 0.126: cpppj와의 이관 장부 교차 읽기, `MatchDisk`의 Win32 특수 경계(절대 경로·긴 경로)는 검증하지 않았다. 실제 Editor State1 월드·저장·Save as의 Template/현재 이름 세션 소유자는 미이식이다(F3 범위).
- 0.127: Windows 외의 `WriteAtomic`은 write-through·재시도가 없는 준비용 `File.Move`이며 durable 검증이 아니다. `ReadOnly`의 Windows 외 동작은 관측하지 않았다. 새 파일 장치의 이름 정규화(ASCII만 접음)와 기존 `GameFileSystem.NormalizePath`(비ASCII도 접음)의 차이는 고치지 않았다. cpppj `RelativePath`는 옮기지 않았다.
- 0.128: 기존 `Netstorm.Assets.FortFile`은 연결하지 않은 이전 독립 판독기로 남아 있으며 은퇴는 R4 Fort 적재 단위에서 한다.
- 0.132: `WriteStream`은 입력을 배열로 복사하므로 입력이 같은 섹션 버퍼의 구간이면 원본의 DWORD 순방향 복사와 다를 수 있다(현재 호출처에는 없음, 소비자를 옮길 때 확인).
- 0.134: `AllocateEmptyManager`의 할당 실패(NULL) 경로는 이식하지 않았다.
- 0.135: 짝이 맞지 않는 UTF-8(옛 ANSI) 이름은 manager에서 꺼낼 때 U+FFFD로 바뀌어 왕복하지 않는다. `Dump`의 파일명은 바이트 문자열 그대로 출력된다.
- 0.136: `HostFortNameConfirmation`의 `header`·`setName` 실제 연결(현재 Template 소유자·전체 snapshot)은 F3이다. `FortCurrentNameDevices`의 두 dirty DWORD 주소 비교는 생략했다(호출자가 `ClientFortGlobals.Catalog`에 연결해야 함). cpppj는 비ASCII 이름을 CP1252→UTF-8로 이중 인코딩하는 차이가 있다.
- 0.137·0.138: `ReadSections` 검사의 `Parse` 이식은 F3(세션 표 소유)이다. `from_chars` 지수 언더플로 거부는 MSVC 동작을 확인하지 못해 옮기지 않았다. `TypeRequiredEnergyTests`·`ArcherTypeDataTests`·`CanonTypeDecoder` 계열과 세션 통합 검사는 F2·F3에서 잇는다.
- 0.152·0.208·0.220·0.230·0.268: `LocalMissionTransport`·로컬 미션 패킷·Host 전송(`WriteFormPacket`)·월드 전송(`SynchronizePeer`·`TransmitDeletion`)·세션 연결 Stub은 검사 없는 사전 구현이거나 명시 오류다. Zone·로그인·분배 검사는 F3 세션 연결 때, TCP/IP 소켓과 송수신은 3차 로컬 구현 때 `LocalNetworkPolicy`의 루프백 전용으로 더한다.
- 0.153: `AddRandom_Native288`의 대본 GameMenu 후반부(`QueueScriptCommand`·`GameMenuScriptCode`)는 R6에서 옮긴다.
- 0.154: 판본 분기 잔여(원문 근거는 있으나 fixture에서 관측되지 않음) — `RawSquidFactory.Take`·`PostCreate`의 나머지 CD 분기, `RawPop`의 무효 void CD throw·겹침 CD 항상 쓰기, `RawUnpop`의 추가 비트 CD 위치, `RawPostPop`·`RawUnpop`의 표시 지원 판본 overload 연결. 탐색기 발자국 조건은 원문(`footX<0`) 대신 기존 C# 조건(`FootX<1`)을 유지했다(기존 직접 검사 `RawFinderTests.FootprintValidationRejectsBadFeet`와 충돌, 차이로 기록). `RawPathSearch` 전체·`RawCanonPlacementPipeline`·`MakeFortRandomPlacementHooks`는 R6이다.
- 0.159: 배치 지형의 파이프라인 어댑터·통합 검사는 R6 범위로 뺐다.
- 0.176: `RawSurfaceStorage.UpdateAmbientWind`의 F3 연결이 남았다.
- 0.182: `Kernel` 진단 5종은 Client·`SquidProcess`가 필요해 옮기지 않았다.
- 0.221~0.224: 공장 HP의 소비(`TypeHitPoints`)는 R6, Create/Take 부모 진단 전달은 월드·세션 조립 때 잇는다.
- 0.242: 월드 보고서의 효과·삭제 요청 계수기 증가 연결(수명주기 쪽)은 미연결이다.
- 0.305: 정리 뒤 정기 조회 꼬리는 오라클도 실패하므로 원본 동작 그대로 두었다([1.4](#cpppj-requests) 요청 6).
- 0.330~0.341의 C# 모양 차이: 문자열은 Latin1로 풀어 string으로 전달(ASCII 범위에서 원문과 동등), C++ 약한 참조·공유 포인터는 `WeakReference`·`IDisposable` pin으로, 세션의 cpp 공유 참조는 내부 소유 배열·대리자(`Bind*`) 주입으로 옮겼다. 실제 공유 바인딩은 실제 호스트 조립에서 잇는다.

## 3. 검증 단계와 계속 유효한 결정·제약

<a id="verify-levels"></a>

### 3.1 검증 단계 L1~L5와 기록 규칙

- **L1:** 원본 x86 fixture. **L2:** 같은 미션 Load/Start 상태가 cpppj와 같다(cpppj `session-dump`). **L3:** 같은 고정 시계·입력열로 N프레임 진행한 상태 해시가 같다(`--fixed-frame-ms` + 프레임별 `session-dump`). **L4a:** 같은 ui-script의 논리 캡처가 픽셀 차이 0이다. **L4b:** 소리 시점과 곡 선택이 같다. **L5:** 자연 완주·저장·재실행 — 사용자 확인 대상이라 AI 작업과 단계 완료 기준에 넣지 않는다.
- cpppj 캡처·덤프는 오라클 SHA·스크립트·`--set` 값·`--fixed-frame-ms` 값·프레임과 함께 기록한다. 논리 캡처(`snapshot`)와 실제 출력 캡처(`snapshot-output`)를 구별한다. 실행에는 `originals/`가 아니라 `extracted/` 아래의 데이터 사본과 빈 사용자 루트를 쓴다.
- 한 단위를 마칠 때 기록할 것: 옮긴 cpppj 파일과 기준 커밋, 연결 fixture와 행 수, 이식한 cpppj 검사 수, 은퇴시킨 기존 코드와 검사, 비교한 캡처·덤프 수와 차이, 함께 옮긴 10차 NB 수정, 남은 경계. L1 통과를 L2 이상으로 합산하지 않는다. 기존 근사 검사의 은퇴로 실패를 감추지 않으며, 원본 fixture 기대값을 C# 결과로 바꾸지 않는다.
- 클론·cpppj 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다(VM-W11-Codex·HJOW-Athlon·vm-debian-codex는 예외 PC, 그 밖의 PC는 실행 전 목적 설명과 명시적 확인). 이 계획은 원본 게임을 실행할 필요가 없다.

<a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a>

### 3.2 계속 유효한 결정·제약

- **목표 순서:** ① 원본 10.78과 동일한 동작 → ② 요구사항(AGENTS 4~14차, 화면비·60/120프레임·다국어·재실행 오류 방지) → ③ Linux. 이미 들어간 요구사항 기능(한국어 선택·와이드·전체화면)은 버리지 않고 새 경로에서 cpppj 방식(논리 해상도·호환 전체화면)으로 다시 만든다. cpppj HEAD에 섞인 요구사항 코드도 함께 옮기고 기본 설정에서 먼저 검증한다(결정 D1).
- **AGENTS.md 요구의 반영 상태** — [9.1](#agents-reflection) 표.
- **글꼴·명칭·주석:** 영어 화면은 원본 `.chfnt`, 한국어는 D2Coding, 일본어·기타 언어는 Noto Sans·Noto Sans JP다([5절](#dotnet-user-decisions) 2·7번). 글꼴은 exe(어셈블리 리소스 포함)에 내장하지 않고 배포판 `Netstorm.exe` 옆 `fonts/`에서 별도 파일로 읽으며 라이선스 파일도 함께 배포한다. 현재 기존 경로는 D2Coding 1.3.2를 출력 폴더 `fonts/`로 복사한다(`CopyToOutputDirectory`). AGENTS.md의 한국어 명칭 규칙을 따른다. 기존 주석은 대상을 고칠 때 정비한다(5절 8번).
- **cpppj 공통 제약을 준용한다.** CHEAT 항목은 Edit 편집에서만 나타난다. 공격형 공중 3종(훨리기그·맨오워·더스트 데빌)은 코드를 옮기되 실제 생산·생성·우클릭 재현과 추가 실행 관측은 하지 않는다. 연녹색 직사각형 분석은 보류한다(원본에서도 관찰됨). 대응 `.english`가 없을 때 Test Battle이 끝나는 원본 동작과 새 맵 `.english` 자동 생성은 9차(cpppj S2) 계약을 따른다. 실제 실행 검사에서 사제 포획/희생·특정 적 공격·시나리오 승리를 재현해야 하는 검사는 건너뛴다([TODO.md](TODO.md)). MP 실행 검사는 하지 않는다.
- **cpppj의 미완·경계 범위는 cpppj를 따라간다.** cpppj가 지금도 명시 경계·보류로 둔 것(LEFT_JOBS.md 4.1 NoWorld·외부 기능, 5.1 보류 분석, 5.2 기존 한계)만 dotnetpj도 경계로 둔다. 옛 목록(View/zoom, 도킹, DD 조정·확인 UI 잔여, Chat GUI 드래그, 미니맵 내용물·grid·항목별 debug, Renderer 시차 경계)은 cpppj 929 UI 마감 이전 기준이므로 0.4 1-2에서 항목별로 cpppj HEAD 구현 여부를 확인해, 구현된 것은 이식 대상으로 바꾼다. 독립 미니맵, Credits 자동 전환, Chat 입력·전달, 마나 103·배율 Draw는 이미 이식 대상이다.
- **근거 우선순위:** 원본 fixture → cpppj 보고서의 계약·경계 → cpppj 소스 → 녹화·스크린샷. cpppj는 HEAD만 쓰고 미커밋 변경은 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않고, 필요한 요청은 [1.4](#cpppj-requests)와 이식 계획 8절에 적어 사용자에게 전달한다(사용자가 두 문서를 함께 정리하라고 지시한 경우는 예외).
- **외부 접속:** 새 경로의 Help 링크(R3)·시작 화면의 검증 안내·`Netstorm.Net`(3차)을 옮길 때마다 [dotnet 외부 접속 문서](docs/dotnet-external-endpoints-20261103.md)의 소비자 목록을 갱신한다. 구 클라이언트 검증·master/root 인터넷 서버·ping/진단은 이식해도 연결하지 않고 성공을 합성하지 않는다. 사용자가 선택하는 도움말 링크 실행과 로컬 서버/LAN 접속(3차)은 별도 경계다(0.151).
- **보호·커밋:** 원본/CD 디렉터리·AGENTS.md·루트 README.md의 "개요"는 수정하지 않는다(README.en.md "Overview"는 번역으로만). push하지 않는다. 새 문서·코드는 UTF-8과 한국어 주석 규칙을 따른다.

<a id="original-parity-plan"></a>

## 4. 9-1 결정 — 의도적 차이는 원본처럼 동작하도록 바꾼다 (2026-10-10 사용자 결정)

**결정:** dotnetpj가 일부러 다르게 두었던 차이(아래 A~F)는 원본처럼 동작하도록 바꾼다. **지금 구조를 둔 채 원본 게임의 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다.** 요구사항·편의는 더 이상 차이를 그대로 두는 근거가 아니다. 목표 순서 ①에 속한다.

**항목마다 밟는 순서:** ① 그 차이 때문에 원본과 달라지는 동작을 구체적으로 적는다 → ② 원본 근거(cpppj의 x86 기대값·`--dump-*` 출력·분석 문서·녹화)와 같은 입력으로 비교한다 → ③ 차이가 없거나 플레이에서 구별할 수 없으면 구조를 유지하고 근거를 이 문서에 남긴다 → ④ 구별되면 먼저 지금 구조 안에서 고치고, 안 되면 구조를 원본 방식으로 바꾼다. **비교 근거 없이 "같다"고 적지 않는다.** 2026-10-28부터 MonoGame은 창·입력·오디오 장치·최종 출력 계층으로만 쓰고 게임 화면은 8비트 논리 화면에 그린다(결정 1). cpppj가 원본 배치를 쓰는 곳은 C#도 같은 배치를 쓴다(결정 2). CD 10.72·10.82 값을 쓰지 않는 것은 기준 판본(10.78)을 따르는 것이라 원본과의 차이가 아니다.

| # | 판정 시작 시 차이(원본) | 원본과 달라질 수 있는 동작 | 처리 시점·현황 |
| --- | --- | --- | --- |
| A | 24Hz 고정 틱 + 화면 보간(원본은 고정 틱 없이 프레임마다 한 번 돌고 "지금 + 간격" 타이머를 쓴다) | 사건이 틱 경계(41.7ms)에 맞춰 일어나고, 명령이 다음 틱 처음에 실행되며, 붕괴 스캔이 틱당 33개다(원본은 프레임당 11개) | **첫 단계 완료(2026-10-11):** 명령·프레임 붕괴 스캔·정지 시각 보존, 화면 루프·숫자 시계·세션 시계를 프레임 처리로 바꾸고 원본 10.78 Scan 1,948행·Init 4행과 일치시켰다([근거](docs/dotnet-frame-parity-20261011.md)). **나머지(R4 Kernel/Regular와 함께):** 이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션의 정수 틱을 실제 시각/원본 타이머로 이행, 과도 단계의 `FrameSeconds`·`Seconds`·8틱 상한·정지 중 보간 잔여 정리, 안내창 재개/브리핑 지연·FPS 표본/Capture·초 경계 초기화, 이벤트 Tick·RunTicks 기반 검사 이전. A 전체 완료로 처리하지 않는다 |
| B | 입력을 MonoGame으로 읽는다(원본은 사건 큐 40칸, 넘치면 새 사건을 버리고 한 바퀴에 한 번 처리) | 빠른 연속 클릭·끌기·키 반복의 처리 순서와 누락 | R1에서 입력 큐를 만들었고 전체 입력 처리(`client/UserInput`·`InputEvent`)는 R6에서 옮긴다 |
| C | 자체 `settings.json`을 쓰고 `options.cfg`는 읽기만(원본은 `d/options.cfg`를 시작·변경 시 저장하고 `fullscreenStateFile.dat`를 만들고 지운다) | 옵션 저장 시점과 위치 | R7. 새 경로는 0.114에서 변경 옵션의 `options.cfg` 저장을 선행했다. **전체화면 전환 뒤 재실행 오류는 클론에서 발생하지 않아야 한다**(AGENTS). 위치는 cpppj와 같은 사용자 루트(결정 D1) |
| D | 그리기 직전 제한의 대기를 Sleep/Yield로 양보(원본은 바쁜 대기) | 실제 간격·흔들림·초과 대기 | R7에서 판정. cpppj F2(1298)가 양보 대기로 바꾸고 **프레임 시간이 같음**을 확인했으므로([근거](docs/exe/cpp-post-plan-fps-cd1072-20261111.md)) 판정 때 이 근거와 같은 방식의 측정을 쓴다. 근거 없이 같다고 적지 않는다 |
| E | 매 프레임 전체를 다시 그린다(원본은 변경 사각형 100개 표로 부분만) | 화면 결과가 같아야 한다 | R1·R5에서 cpppj Renderer의 dirty 경로로 처리 |
| F | 뮤텍스·레지스트리·CD 찾기·창 크기 고정이 없다 | 창 크기 조절·중복 실행처럼 사용자가 보는 동작 | R7에서 판정. 사용자가 보는 동작만 원본대로 맞춘다([cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md)) |

판정 근거 문서: A는 [main-loop.md](docs/exe/main-loop.md) 7절·[cpp-process-reconstruction.md](docs/exe/cpp-process-reconstruction.md)·`process-x86.tsv`·`bridgedecay-x86.tsv`의 Scan 행, B는 [cpp-screen-reconstruction.md](docs/exe/cpp-screen-reconstruction.md) 6절, C는 [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md)와 options x86 기대값 16개, D는 main-loop.md 4·7절, E는 [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md). 옛 9절 표·옛 순서 제안은 [보관 파일](LEFT_JOBS.dotnetpj.old.md)에 있다.

<a id="dotnet-user-decisions"></a>

## 5. 사용자 결정·확인 기록 (옛 10절, 계속 유효한 것)

번호는 옛 10절 번호다. 해결·구현이 끝난 1·3·4번은 보관 파일에 있다. 2026-11-11에 확정한 D1~D5는 [0.5](#decisions)다.

2. **영어 UI 글꼴(2026-10-10 사용자 결정):** 판단 기준은 선택한 언어다. 영어이면 UI 글자를 원본 비트맵 글꼴(`.chfnt`)로 그리고 폭도 그 글꼴로 잰다. 한국어이면 D2Coding을 쓴다. 영어 화면에 원본 글꼴에 없는 글자(한글 등)가 섞이면 D2Coding으로 대신 그린다(에이전트 해석). 새 경로는 R2·R3의 `BitmapFont`·Gump 글리프로 영어를 그리고, 한국어는 R8(6차)에서 8비트 화면용으로 다시 만든다(결정 D4). 글꼴 파일은 exe 옆 `fonts/`에서 읽는다(AGENTS `75e4eed9`).
5. **"원본과 완전히 동일한 동작"과 의도적 차이(2026-10-10 사용자 결정):** 원본처럼 동작하도록 바꾼다. 구조를 둔 채 경험을 거의 동일하게 만들 수 있으면 구조는 유지해도 되지만, 그렇게 할 수 없으면 구조 자체를 원본처럼 바꾼다. 항목별 판정은 [4절](#original-parity-plan).
6. **이미 들어간 요구사항 기능(2026-10-10, 에이전트 해석):** 한국어 선택·와이드 화면 시야 확장·테두리 없는 전체화면 창은 되돌리지 않는다. 기본 실행(영어·4:3)에서 원본과 같은 동작을 먼저 맞추고, 새 경로로 옮길 때 cpppj 방식으로 다시 만든다.
7. **D2Coding 1.4.0 교체(2026-10-10 사용자 결정): 중요도가 낮다.** 당분간 1.3.2를 쓰고, 글꼴 코드를 고칠 일이 있거나 여유가 있을 때 한다. 교체 때 확인할 것: 코드 3곳의 파일명(`Netstorm.Game.csproj`·`NetstormGame.cs`·`TrueTypeCollectionTests.cs`), 배포 스크립트의 글꼴 경로와 고지문, TTC face 순서·글자 폭·줄 높이, `TrueTypeCollectionTests` 기대값, 한국어 화면 캡처, README 두 개의 파일명. 새 경로의 6차 작업은 글꼴 코드를 새로 만드는 일이라 이때 cpppj와 같은 최신 D2Coding으로 맞추는 것을 기본으로 한다([1.3](#dotnet-r8-requirements) 6차).
8. **기존 코드 주석 정비(2026-10-10 사용자 결정):** 주석이 설명하는 대상이 수정될 때 새 규칙(설명·사용법 먼저, 추가/수정 이력은 뒤)에 맞춘다. 고치지 않는 대상의 주석을 일괄 정리하지 않는다.
9. **cpppj 미복원 동작(2026-10-16, 에이전트 기본값):** 동결된 기존 경로에서만 근사를 유지하고 [core-rules.md](docs/core-rules.md)의 근사 목록에 "원본 근거 없음"으로 등록한다. 새 경로는 결정 6.
10. **원본 월드 코어 도입(2026-10-16, 에이전트 판단):** 5번 결정의 "구조 자체를 원본처럼 바꾼다"에 해당한다([이식 계획 4절](docs/dotnet-cpp-port-plan-20261016.md#4-구조-결정-c-원본-월드-코어-도입)). 원본 월드 코어는 R4에서 세션에 조립한다.
11. **원본 월드 코어의 저장 방식(2026-10-16, 에이전트 판단):** 코어 저장소는 슬롯 50바이트를 그대로 둔다(W02~W03 fixture가 슬롯 전체 바이트를 대조하고 파생 생성자 쓰기가 필드 이름 없는 오프셋이라 의미 모델로는 검증할 수 없다). 가상 표 주소는 클래스 식별 값으로만 쓴다. 결정 2로 범위를 넓혀, cpppj가 원본 배치를 쓰는 곳은 세션·화면 쪽도 같은 배치를 쓴다.
12. **Template 저장소 이식 방식(2026-11-01 사용자 결정):** 계획서 기본값 네 가지(`RawSectionBuffer`, `RiftTypeTable` 직역, `FortTemplate` 위치는 F1에서 확정, 저장소 cpppj 대조 대신 fort 433개 왕복 검사)로 진행했다([계획](docs/dotnet-template-storage-plan-20261101.md) 7절).

## 6. 검증 명령과 운영 규칙

<a id="dotnet-commands"></a>

### 6.1 검증 명령

```powershell
# 저장소 루트에서 기준선과 단계별 검증 (헤드리스)
dotnet build dotnetpj/Netstorm.sln -c Release
dotnet test dotnetpj/Netstorm.sln -c Release

# cpppj 콘솔 출력 대조 (헤드리스 명령만 사용. --run 등 게임/창 실행은 해당 PC의 규칙을 별도로 확인)
cpppj/build/bin/Release/Netstorm.exe --dump-types originals
cpppj/build/bin/Release/Netstorm.exe --dump-world originals thewarbegins
cpppj/build/bin/Release/Netstorm.exe --inspect-bridges originals

# cpppj 반영 범위 확인. 저장소 루트에서 실행. 마지막 반영 커밋은 cpp-fixture-manifest.tsv 의 reflected-commit
python dotnetpj/tools/cpp_sync.py                                    # 보고 (읽기 전용, 연결 fixture 가 어긋나면 종료 코드 1)
python dotnetpj/tools/cpp_sync.py --update --reflect HEAD            # HEAD까지 영향 검토·반영을 모두 마친 경우에만 갱신
python dotnetpj/tools/cpp_sync.py --update --add 이름:Core           # 새 fixture 를 연결할 때 목록에 등록 (Assets 도 가능)

# 영향 재검토(0.4 1-2): 지난 검토 이후 바뀐 cpppj 소스와 PORT_MAP 의 교집합
git diff --name-only 4d4b3a928..HEAD -- cpppj/src
```

```powershell
# cpppj 오라클 캡처. 창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다.
# 데이터는 originals/ 대신 extracted/ 아래 사본을 쓴다. steps.tsv 한 줄은 "프레임<TAB>동작<TAB>인자"다
# (click·click-if·right·key·esc·pointer·snapshot·session-dump 등. snapshot·session-dump 인자는 저장할 파일 경로).
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --window --no-audio --frames 120 `
  --set "SCREENW=1024;SCREENH=768;maxFPS=40;sleepPerLoop=25;autoDemo=0;tellTips=0" `
  --ui-script steps.tsv --ui-report states.tsv            # 미션 직행은 --mission thewarbegins 등을 더한다
# L4a 일괄 도구(0.380): 같은 ui-script 로 cpppj 와 dotnetpj 호스트를 각각 돌려 캡처를 만들고 바로 비교한다. 종료 코드 0 일치·1 차이·2 오류.
#   steps 템플릿의 캡처 경로 자리에 {OUT} 을 쓴다(예: dotnetpj/tools/l4a/menu-main.tsv). 캡처는 extracted/dotnet-oracle-captures/<Name>/cpp, dn 에 남는다.
#   powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj/tools/l4a_compare.ps1 -Steps dotnetpj/tools/l4a/menu-main.tsv -Name menu-main -Frames 130 [-Build] [-SkipCpp]
#   창이 뜨므로 AGENTS.md 확인·예외 규칙을 따른다. 실행 전후 originals/ 해시를 비교해 달라지면 실패로 끝낸다.
# 캡처 비교 도구: python dotnetpj/tools/cpp_frame_compare.py A.bmp B.bmp [--diff diff.png]  /  --dir 폴더A 폴더B (종료 코드 0 일치·1 차이·2 입력 오류)
# dotnetpj 쪽의 같은 실행 인자(--frames/--ui-script/--ui-report/--set/--fixed-frame-ms)는 Netstorm.Client.Host 가 받는다.
# PowerShell 5.1의 Start-Process -PassThru 는 종료 코드를 비우므로 `& exe 인자; $LASTEXITCODE` 로 읽는다.

# cpppj 실제 Session 상태 덤프(L2)·고정 시각 실행(L3).
# 결정론 비교에는 가상 시계(--fixed-frame-ms)와 빈 사용자 루트를 쓴다. 기본 OS 시계 두 실행은 게임 시각부터 달라 비교할 수 없다.
# WelcomeGiven=1 이 없으면 환영 창 때문에 메뉴 클릭이 실패하고, --mission 뒤 'Play Mission' 은 늦게 나타나므로 click-if 를 여러 프레임에 둔다.
$env:NETSTORM_USER_DATA = "<빈 폴더>"
cpppj/build/bin/Release/Netstorm.exe --run <데이터 사본> --mission thewarbegins --fixed-frame-ms 25 --window --mute-audio --frames 250 `
  --set "SCREENW=1024;SCREENH=768;WelcomeGiven=1;autoDemo=0;tellTips=0" --ui-script steps.tsv --ui-report report.tsv
# steps.tsv 예: "5<TAB>click-if<TAB>Play Mission", "90<TAB>session-dump<TAB><출력 경로>.tsv"
# 덤프 형식(판번호 1, ASCII·LF·탭, float/double 은 비트 정수): docs/exe/cpp-session-state-dump-20261109.md
```

- 클론 창을 띄우는 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 AGENTS.md의 확인·예외 규칙을 따른다.
- 원본 x86 기대값 대조만 돌릴 때: `dotnet test -c Release --filter "FullyQualifiedName~X86"`. 새 기대값 파일은 `tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj`에 링크로 추가하고(사본 금지) `cpp_sync.py --update --add`로 SHA-256 목록에 등록한다(빠뜨리면 `EveryLinkedFixtureIsRegistered`가 실패). 한 fixture는 한 소비자 프로젝트에서 검사한다(`config-x86.tsv`는 Assets.Tests). 포트 맵의 `port` 행은 fixture 대조가 없는 이식이며 L1이 아니다.
- **변이 검사:** 대표 변이를 수동으로 넣고 제품·검사 빌드 뒤 `--no-build` 집중 실행으로 검출을 확인한 다음 원복하고 집중 검사를 다시 통과시킨다. 원복 뒤 증분 빌드가 옛 결과를 쓰지 않게 필요하면 `--no-incremental`로 다시 빌드한다. 사본(`extracted/dn-mutation-ts0`)을 쓸 때는 다중 행 패턴 들여쓰기가 틀리면 일치 0개로 조용히 건너뛰므로 개수 assert로 잡고 복사한 파일은 반드시 `touch`한다. 무한 루프로 걸리는 변이도 검출로 기록한다.
- **전체 검사 완주:** 단위마다 집중 검사로 넘어가더라도 20단위 또는 한 작업일마다 솔루션 전체 검사를 끝까지 돌려 0.3의 기준선을 새로 적는다.
- **오라클 자료:** 데이터 사본 `extracted/dotnet-oracle-data/`, 캡처 `extracted/dotnet-oracle-captures/`, 기준선 `extracted/dotnet-r0-baseline/`은 Git 제외이며 작업 PC에만 있다. 실행 전후 `originals/` 해시를 비교한다.
- **options.cfg 변경 판별:** 호스트나 원본 게임을 실행한 뒤 추적 파일 `assets/game-data/d/options.cfg`가 바뀌어 있으면 누구의 실행 때문인지 먼저 가린다. 내가 만든 변경일 때만 되돌린다.

<a id="dotnet-ops"></a>

### 6.2 착수·커밋 운영 규칙

1. **착수:** AGENTS.md·이 문서·LEFT_JOBS.md를 다시 읽고 `git status`/`git log`로 다른 세션 변경을 확인한다. dotnetpj는 디컴파일 산출물을 쓰지 않으므로 호스트 확인은 산출물을 직접 읽을 때만 한다.
2. **단위 종료:** 머리말의 기준 단위·다음 단위 번호, 0.2·0.3·0.4와 해당 상세 절을 갱신한다. 문서가 없는 작은 단위만 [8절](#dotnet-unit-records)에 짧게 적고, 긴 경과는 `docs/dotnet-*.md` 단위 문서에 둔다. 끝난 상세가 길어지면 보관 파일로 옮기고 [7절](#dotnet-done-index) 색인만 갱신한다.
3. **커밋:** 인계 작성 뒤 **이 단위의 파일만** 경로로 지정해 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다(끝에 Co-Authored-By 트레일러). 커밋 번호는 다른 PC·세션과 겹칠 수 있으므로 `git log`로 당일 마지막 번호를 확인해 이어 쓴다. 같은 작업 트리에서 다른 세션이 cpppj·`LEFT_JOBS.md`를 자주 고치므로 CP/WB 파일·stage를 건드리지 않는다. **push하지 않는다.**
4. **줄바꿈:** `dotnetpj`·`docs`와 `LEFT_JOBS.dotnetpj*.md`는 저장소에 LF로 저장된다(이 문서는 작업 트리도 LF, 보관 파일은 작업 트리 CRLF). 기본 `autocrlf`로 add하고 `-c core.autocrlf=false`를 쓰지 않는다. CRLF 작업 트리 파일은 Edit 도구 대신 정확히 1회 일치하는 바이트 치환으로 고친다.
5. **빌드 환경:** Git Bash에서 `cmake --build … -- /p:…`를 실행하면 `/p:`가 경로로 바뀌어 MSBuild가 실패하므로 PowerShell로 실행한다. `cpp_sync.py` 보고가 cp949 콘솔에서 깨지면 `PYTHONUTF8=1` 또는 `python -X utf8`. 오라클에는 게임 실행 파일만 필요하며 cpppj 전체 빌드는 검사 대상까지 포함해 오래 걸린다.
6. **주석:** 새 코드의 네임스페이스·상수·함수·구조체·클래스·멤버함수·멤버변수·반복문에 한국어 주석을 단다. 주 내용은 설명과 사용법이고, 언제·왜 추가/수정했는지는 그 뒤에 적는다. C#에서는 `/// <summary>`에 무엇을 하는지와 쓰는 방법(인자·반환·호출 조건)을 쓰고 원본 주소·근거 문서·이력은 summary 뒤쪽이나 `<remarks>`에 둔다. 반복문은 바로 위 `//` 주석으로 무엇을 도는지 적는다. 원본 주소는 10.78 주소와 근거 문서(`docs/exe/cpp-*.md`)를 함께 적는다.

## 7. 참고 문서와 완료 범위 색인

<a id="dotnet-done-index"></a>

### 7.1 완료 범위 색인

단위별 상세는 [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md)와 아래 문서에 있다. 마지막 열은 현행 계획에서의 처리다([2.2](#04-기존-dotnetpj-소스의-재수정-판정)). 새 경로의 진행도는 Client 검사와 2.3의 실제 완료 기준을 함께 본다. Assets·Core의 기존 경로 검사 수를 새 경로 진행도로 세지 않는다.

| 기간·단위 | 완료 범위 | 근거 문서 | 처리 |
| --- | --- | --- | --- |
| 2026-09-27~10-05 | MonoGame 기반 초기 C# 구현(메인 메뉴·캠페인 1-1/1-2·다리·사제·전투·희생·덱 등 녹화·팬게임 기반), `dotnetpj/` 이동 | [보관 파일 14절](LEFT_JOBS.dotnetpj.old.md#legacy-dotnet-history) | 기존 경로(대부분 교체) |
| 2026-10-07~10-13 | 자료 계층 정정, 다리·표면·그래프·SID·점유·표시 경계 계산, `.chfnt`·커서 표·배경 GIF 색·미션 시작 값, 9-1 A 첫 단계·화면 루프·시계, 도움말·안내창·단계 프레임, 한국어 명칭 | [10-07](docs/dotnet-reconstruction-20261007.md)·[10-09](docs/dotnet-reconstruction-20261009.md)·[10-10](docs/dotnet-reconstruction-20261010.md)·[frame-parity](docs/dotnet-frame-parity-20261011.md)·[loop-parity](docs/dotnet-loop-parity-20261012.md)·[clock-kernel](docs/dotnet-clock-kernel-20261012.md)·[session-clock](docs/dotnet-session-clock-20261012.md)·[dialog-frames](docs/dotnet-dialog-frames-20261013.md)·[stage-frames](docs/dotnet-stage-frames-20261013.md) | 판독기·시계는 재대조, 계산은 `Original`로 흡수, 나머지 교체 |
| 2026-10-15~10-23 | 감사 D01~D16, W01 동기화 도구, W02·W03 원본 월드 코어(슬롯 풀·생성자·Pop/Unpop/PostPop·그래프·삭제·Damageable·Form·내용물 등), W05 선행(SP 저장소·보상), 배포 스크립트 | [감사](docs/dotnet-cpp-audit-20261015.md)·[이식 계획](docs/dotnet-cpp-port-plan-20261016.md)·[W02 첫 조각](docs/dotnet-original-core-20261016.md) 외 `docs/dotnet-original-core-*.md`·[SP 저장소·보상](docs/dotnet-original-core-spstore-reward-20261023.md) | 유지. R4 세션 조립 |
| R0~R2 (10-28~29, 0.13~0.28) | cpppj 오라클·재대조 영향표·PORT_MAP, `Netstorm.Client` 8비트 Screen·입력 큐·캡처·스크립트 도구·`cpp_frame_compare.py`·MonoGame 출력 호스트, R0-4 코어 보강, 팔레트·VFX·색 표·SpriteDraw | [영향표](docs/dotnet-cpp-resync-20261028.md)·[PORT_MAP.md](dotnetpj/PORT_MAP.md)·[R1 문서](docs/dotnet-client-screen-20261028.md)·`docs/dotnet-r04-*.md`·[SpriteDraw](docs/dotnet-spritedraw-20261029.md) | 새 경로 |
| R3a~R3k (10-29~31, 0.29~0.65) | Gump 기반·트리·수명·사건/입력·프레임·클립·배경·글꼴·버튼, StyleText·Slider·메뉴, 문자 입력·Console·Image·MetaImage, 배경 GIF·HTML·도움말·스크립트 명령·Config·MissionScript | `docs/dotnet-gump-*.md`·`docs/dotnet-style*.md`·`docs/dotnet-menu-*.md`·`docs/dotnet-text-entry-*.md`·`docs/dotnet-html-*.md`·`docs/dotnet-quick-help-*.md`·`docs/dotnet-config-*.md` 등 | 새 경로 |
| 0.66~0.127 (10-31~11-01) | 미션 조립 부품, 호스트 입력·타이틀·메인 메뉴, 공통 트리·HTML 창, 기존 경로 `Netstorm.exe` 이름·배포(0.96), 통합 표시·메뉴 입력, Tutorial Init·초기화, 호스트 Help·Options·옵션 저장, Edit 맵 목록·이름·작업본 | [보관 0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)·[보관 0.127~0.154](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) | 새 경로(L1 일부). 실제 조립은 R4 |
| 0.128~0.155 (11-01~07) | DataManager·Template 저장소 TS0~TS6, Fort 타입 표·적재·저장 F1~F2c-12, 11.01 표기·Credits(0.150), 외부 접속 문서화(0.151), 멀티플레이 사전 구현(0.152) | [보관 0.127~0.154](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)·[Template 저장소 계획](docs/dotnet-template-storage-plan-20261101.md)·[외부 접속](docs/dotnet-external-endpoints-20261103.md) | 대부분 L1, 세션 연결은 F3 |
| 0.155~0.279 (11-07~10) | 지형·섬 생성, 다리 연결, F2d Player·캠페인 월드, 서피스 월드 기반·프로세스·호스트, 월드 본체 조립, F3 세션 껍데기·Stub | [보관 0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) | 새 경로(L1·port) |
| 0.280~0.341 (11-10~11) | 재개 준비(재기준선·영향 확인·재반영 2건·`session-dump`·고정 시각), F2c 정상 생성, 섬 전수 재생, F3 본체·세션 소유 상태·AI 관문·동맹·채팅·Player 식별자, LAN 사전 구현, 전체 회귀(0.329) | [보관 0.280~0.341](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341), 요약 [2.4](#r4-detail) | 새 경로(port 위주) |
| 0.342~0.379 (11-11) | 11.01 `Copyright 2026 HJOW` 반영(0.342)과 cpppj 영향 재검토 31항목 전수 처분 — 정적 분류(0.343), 10차 NB 수정 재대조(0.344~0.348 등), 결정 D1에 따라 앞당긴 요구사항 코드의 신규 이식(11차 `AiDifficulty` 0.347, 6차 확장 문자 부호화·`DisplayTextInput` 0.350·0.351, 5차 `UserDataRoot` 0.359, 7차 `ErrorLog`·호스트 배선 0.360·0.361), 구간 대조(0.368·0.370~0.378), 전체 회귀(0.369), 범위 종결(0.379) | [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452), [재검토 문서](docs/dotnet-cpp-resync-20261111.md) | 새 경로(port 위주) |
| 0.380~0.389 (11-11~12) | 오라클 갱신, L4a 일괄 도구 `l4a_compare.ps1`, 메뉴 12장면 차이 0 — 돌 버튼 높이 교정(0.380), 구름 Kencloud 이식(0.381), 모달 게이트(0.382), Help·Options 실제 행·앵커(0.383·0.384), Edit 목록 무제한(0.386), 해상도 10행(0.387), 남은 장면 확정(0.389) | [L4a 문서](docs/dotnet-l4a-menu-cloud-20261111.md), [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452) | 새 경로 |
| 0.390~0.452 (11-12) | R4 실제 호스트·월드 조립 착수 — thewarbegins 기준선(0.390)부터 섬 부모 조립(0.452)까지, 묶음별 요약은 [0.4 R4 진행판](#r4-board) | [보관 0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452) | 새 경로(진행 중) |

### 7.2 참고 문서

| 주제 | 문서 | cpppj 기준 구현 |
| --- | --- | --- |
| **cpppj 현행 상태·계약·사용자 결정** | [LEFT_JOBS.md](LEFT_JOBS.md)(0.4 단계별 현황·1절 완료 묶음·4.1 계약·5절 실행 제외) | — |
| cpppj 실제 Session 덤프·고정 시각 실행(L2·L3 오라클) | [cpp-session-state-dump-20261109.md](docs/exe/cpp-session-state-dump-20261109.md) | `client/SessionStateDump`, `client/ClientTimeSource`, `app/main.cpp` |
| cpppj의 다른 프로젝트 요청 처리·재반영 대상 | [cpp-webpj-requests-20261109.md](docs/exe/cpp-webpj-requests-20261109.md) | `client/UberGump.cpp`, `client/RawMenuControls` |
| **AGENTS 4~14차의 cpppj 구현**(14차는 1301 엔진 탑재까지, 연동 범위 D7 대기) | [1.3](#dotnet-r8-requirements) 표의 근거 열 | 1.3 표의 소스 열 |
| cpppj 11.01 표기·Credits·카피라이트 / 외부 접속 목록 | [cpp-product-1101-20261101.md](docs/exe/cpp-product-1101-20261101.md), [cpp-external-services-20261101.md](docs/exe/cpp-external-services-20261101.md) | `client/ProductPresentation.cpp` |
| cpppj CD 10.72(dotnetpj 비대상, 게이트 영향만) | [cpp-cd1072-startup-and-gates-20261111.md](docs/exe/cpp-cd1072-startup-and-gates-20261111.md) | `Patch1078` 게이트 |
| cpppj 영향 재검토(제품 1247→1300) 31항목 처분 | [dotnet-cpp-resync-20261111.md](docs/dotnet-cpp-resync-20261111.md) | — |
| **L4a 첫 비교(메인 메뉴)·구름 Kencloud 이식** | [dotnet-l4a-menu-cloud-20261111.md](docs/dotnet-l4a-menu-cloud-20261111.md) | `o/RawKencloud`·`RawKencloudRender`, `client/ClientKencloudGlobals`·`ClientMain.cpp` 팔레트 전환 |
| 8비트 화면 출력·전체화면 호환 | [cpp-modern-fullscreen-20261027.md](docs/exe/cpp-modern-fullscreen-20261027.md), [cpp-screen-resolution-menu-20261027.md](docs/exe/cpp-screen-resolution-menu-20261027.md) | `client/Screen.cpp`, `client/ScreenPresentation.cpp`, `client/ClientMain.cpp` |
| cpppj → dotnetpj 이식 계획(W01~W20, 8절 요청) | [dotnet-cpp-port-plan-20261016.md](docs/dotnet-cpp-port-plan-20261016.md) | `client/RawCampaignSession`, `client/RawSurfaceWorld` |
| 원본 월드 코어 W02 첫 조각 | [dotnet-original-core-20261016.md](docs/dotnet-original-core-20261016.md) | `o/SidPool.cpp`, `o/SquidFactory.cpp`, `o/TypeConstructors.inc`, `o/DerivedConstructors.inc` |
| Template 저장소 이식 계획(TS0~TS6, F1~F3) | [dotnet-template-storage-plan-20261101.md](docs/dotnet-template-storage-plan-20261101.md) | `client/CampaignTemplateStorage` |
| dotnetpj 외부 접속 문서 | [dotnet-external-endpoints-20261103.md](docs/dotnet-external-endpoints-20261103.md) | — |
| 반영 점검 D01~D16 | [dotnet-cpp-audit-20261015.md](docs/dotnet-cpp-audit-20261015.md) | — |
| 표시 이름·내부 타입명 대응 | [unit-source-name-map.md](docs/unit-source-name-map.md) | `o/RiftTypeTable.cpp` |
| 데모·캠페인 대응 | [demo-campaign-map.md](docs/gameplay/demo-campaign-map.md) | `o/RawMissionStart` |
| 설정 조회·치환·저장 | [cpp-config-reconstruction.md](docs/exe/cpp-config-reconstruction.md), [cpp-options-reconstruction.md](docs/exe/cpp-options-reconstruction.md) | `o/Config.cpp`, `o/ConfigInterface.cpp` |
| 타입 표·요새 파일 | [cpp-fort-reconstruction.md](docs/exe/cpp-fort-reconstruction.md), [cpp-assets-reconstruction.md](docs/exe/cpp-assets-reconstruction.md) | `o/RiftTypeTable.cpp`, `o/Template.cpp` |
| 다리 표·추첨·열린 방향·수명 | [cpp-bridge-reconstruction.md](docs/exe/cpp-bridge-reconstruction.md), [bridge-pieces.md](docs/exe/bridge-pieces.md) | `o/Bridge.cpp`, `o/CanonDecoder.cpp` |
| 표면 이웃·연결·붕괴 방문 / 그래프 | [cpp-surface-reconstruction.md](docs/exe/cpp-surface-reconstruction.md), [cpp-graph-reconstruction.md](docs/exe/cpp-graph-reconstruction.md) | `o/SquidFinder.cpp`, `o/Graph.cpp`, `o/RawGraph.cpp` |
| 점유 비트·해시 / SID / 첫 등록 | [cpp-hash-reconstruction.md](docs/exe/cpp-hash-reconstruction.md), [cpp-spatial-reconstruction.md](docs/exe/cpp-spatial-reconstruction.md), [cpp-sid-reconstruction.md](docs/exe/cpp-sid-reconstruction.md), [cpp-postpop-reconstruction.md](docs/exe/cpp-postpop-reconstruction.md) | `o/SquidHash.cpp`, `o/SquidSpatial.cpp`, `o/SidPool.cpp`, `o/SquidPostPop.cpp` |
| 그리기 순서·글꼴·커서 / 메뉴·브리핑 | [cpp-renderer-reconstruction.md](docs/exe/cpp-renderer-reconstruction.md), [cpp-display-reconstruction.md](docs/exe/cpp-display-reconstruction.md), [cpp-menu-reconstruction.md](docs/exe/cpp-menu-reconstruction.md) | `client/Renderer.cpp`, `o/SquidDisplay.cpp`, `client/UberGump.cpp`, `client/Gump.cpp` |
| 지형·시작 값·조작 / 메인 루프 | [cpp-world-reconstruction.md](docs/exe/cpp-world-reconstruction.md), [main-loop.md](docs/exe/main-loop.md) | `o/TerrainBuilder.cpp`, `client/ClientMain.cpp` |
| 판본 비교(10.37·10.62·10.82·V12) | [cpp-reference-versions.md](docs/exe/cpp-reference-versions.md) | — |
| C# 규칙 코어의 현재 범위와 근사 목록(기존 경로) | [core-rules.md](docs/core-rules.md) | — |
| cpppj 소스 맵·디컴파일 신뢰도 | [SOURCE_MAP](cpppj/SOURCE_MAP.md), [decompile-reliability.md](docs/exe/decompile-reliability.md) | — |

<a id="dotnet-unit-records"></a>

## 8. 단위 기록 (0.453부터)

문서가 없는 작은 단위만 아래에 최신순으로 짧게 적는다. 한 기록에는 범위, 구현(새 파일·수정 파일·옮긴 cpppj 파일과 기준 커밋), C#에서 달라진 모양, 검증(집중·전체·변이·L2/L4a 비교), PORT_MAP·fixture 변화, 함께 옮긴 10차 NB 수정, 보호·실행·외부 접속 관점, 남은 것을 적는다([3.1](#verify-levels)). 0.11~0.452는 보관 파일에 있다: [0.11~0.65](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073) · [0.66~0.126](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126) · [0.127~0.153](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154) · [0.154~0.279](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279) · [0.280~0.341](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341) · [0.342~0.452](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452). 묶음별 요약은 [7.1](#dotnet-done-index)과 [0.4 R4 진행판](#r4-board)에 있다.

#### 0.523 — 사제 월드 선행 Carrier 전처리·현재 내용물 조회 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.522는824b4ea2d(20261114-80 DN)로 완료했다. 실제 사제월드 조립 검토에서 기존 RawCarrierPreDestroy/RawContainedFinder에 현행 Native 경로·현재 공유boss/checking/복귀진단·CountContained/사제어댑터가 빠진 것을 확인했다. 먼저 이 선행과 원본 Carrier2880입력·현재권한/진단/저장한next·사제/피해부모합성을 대조한다. C# bool checking 입력4곳은 원문 RawCheckingFlag뷰로 이행한다. 사제월드전체/HostSession조립·일반지역정책은 다음단계이며 실제금지플레이/MP는 하지않는다.

- 두현행모듈·전체DWORD checking/현재공유boss·복귀진단·Native선행검사생략·개수함수·사제어댑터를갱신했다. 원본세판본2880입력은처음부터일치했고 새9검사로실제사제PreDestroy→Carrier와Native부모뒤현재권한·진단뒤현재head·저장next/현재TYPE·모든후보상태·전체WORD/DWORD·함수/진단뷰사본을대조했다. 추가공유권한2실패는할당전에합성raw를써서free표식을지운입력오류라원문처럼할당후준비로교정했다. 현재boss를독립값으로고정한변이3/9검출·SHA까지바이트원복뒤전체비증분Release빌드0경고0오류·최종관련Core527/527·Client32/32통과(실패0·건너뜀0·각종료0). PORT_MAP793(L1 379+port414), fixture275. 문서 docs/dotnet-carrier-predestroy-current-20261114.md, 해당작업완료. 기존0.508문서명충돌은원문바이트복원/별도이름분리로해소했다. 실제게임/MP/금지플레이없음·보호원본/CP/WB미변경. M1전체0/34 유지. 다음0.524는 RawDamageablePreDestroy의EnableNativeBody/PreDestroyNative·공유PriestType/현재boss·감긴flatSpot·같은풀어댑터를현행화한뒤실제사제월드묶음조립을계속한다. 해시0표면동일저장소/네권한owner동일성·필수효과검증후등록/반납규칙과일반지역정책도잔여다.

#### 0.522 — 사제 정기 분배와 현재 이동 프로세스 선택 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.521은30523aaf2(20261114-79 DN)로 완료했다. 실제 사제 월드 선행 RawPriestRegular의601~605/부모 분배와 RawPriestWorldBindings의 RawMovementProcessTypes/선택 함수를 이식한다. 같은 현재TYPE/역할 상자와 명시 사본/목적지 주소 보존 대입을 구현하고, 원본 정기384·선택718입력 및 현재 전역/선택 진단·실제 Kernel 수명을 확인한다. 전체 사제 월드/호스트 및 일반 지역 정책은 다음 단계이며 실제 금지 플레이/MP검사는 하지 않는다.

- RawPriestRegular전체·현재 이동 선택/자료 부분·장기5역할 상자를 구현했다. 원본3974입력은처음부터일치했고 새Core3/Client7검사로 unsigned count·선택 경계·실제Form/Kernel재예약/삭제 재진입·현재owner/일반월드준비·독립사본·주소보존/교차별칭순차대입·전체DWORD·판본조회순서를확인했다. Kernel최초검사Form0의삭제거부를실제공장/부착으로보완했고 후속free확인의잘못된void4기대를실제Release의free1|dead2근거로교정했다(제품/golden무변경). signed count변이2/3·balloon선행변이6/7검출·두소스SHA까지바이트원복뒤전체비증분Release빌드0경고0오류·최종관련Core499/499·Client32/32통과(실패0·건너뜀0·각종료0). PORT_MAP792(L1 378+port414), fixture274. 문서 docs/dotnet-priest-regular-movement-20261114.md, 해당작업완료. 실제게임/MP/금지플레이없음·보호원본/CP/WB미변경. 사제월드전체/HostSession·일반지역정책/M1전체0/34는잔여다. 다음0.523에서같은실제모듈/장기전역을RawPriestWorldBindings묶음으로조립한다.

#### 0.521 — 일반/사제 후보 충돌과 실제 finder 조립 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.520은a127fb5cc(20261114-78 DN)로 완료했다. RawCanonPlacementCollision·RawPriestPlacementCollision과 원본 무시 helper를 실제 공유 전역·미리보기/해시/finder에 이식한다. 두 세판본 원본 fixture와 실제 현재TYPE/저장한next·거부 고정·지형/미리보기/생성 합성을 대조한다. 일반 지역 정책/전체 사제 월드·HostSession은 다음 단계이며 실제 금지 플레이/MP검사는 하지 않는다.

- 두 충돌 모듈과 원본 전체 무시 비트값/helper를 구현했다. 원본 세판본34200입력·실제finder14검사는 처음부터 통과했고 전체 사제 픽셀/접두/기하/미리보기/지형/충돌→나선→공장/실제Owner/Carrier와 다중 패턴의 두번째모양 거부·현재 공유 전역/자료계약까지 새19검사를 확인했다. 추가 계약2실패는 Client풀에 기본Server 할당을 요청한 입력 오류라 명시적Client 할당으로 교정했다(제품/golden무변경). PATCH마스크를CD값으로 바꾼 변이3/19검출·SHA까지바이트 원복 후 전체 비증분 Release빌드0경고0오류·최종관련Core429/429·Client21/21통과(실패0·건너뜀0·각종료0). PORT_MAP788(L1 376+port412), fixture271. 문서 docs/dotnet-placement-collision-20261114.md, 해당 작업 완료. 실제게임/MP/금지플레이 입력 없음. 보호원본/CP/WB미변경. 사제 배치 부품의 실제 합성은 완료했으나 실제Pop/사제월드/HostSession은 잔여이며 M1전체0/34 유지. 다음0.522에서 RawPriestRegular·현재이동선택 등 묶음 조립의 나머지 선행을 확인하고 실제 사제월드로 잇는다. 일반 지역 정책도 남는다.

#### 0.520 — 일반/사제 배치 기하와 로컬 미리보기 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.519는1b009866d(20261114-76 DN)로 완료했다. RawCanonPlacementGeometry·RawPriestPlacementGeometry와 일반/사제 Preview를 같은 현재 타입/프레임·지도·관계에 연결한다. 기존 CanonPlacementCell·기하 상태는 재사용하고, 충돌 갈고리 형식만 선행 복원해 기하 어댑터를 닫는다. 세판본4원본fixture, 실제기하/미리보기/표면조회·현재자료·조기거부·대표변이를 대조한다. 후보충돌 본체/지역정책과 전체 사제월드는 잔여이며 실제 금지 플레이/MP검사는 하지 않는다.

- 기하/미리보기4모듈과 충돌 함수 형식을 구현했다. 초기 검사using 누락1빌드오류를 보완했고 원본세판본20039입력·새24검사(실제 패턴/finder와 사제 픽셀→접두→기하→미리보기→기존 지형 조합, 현재자료/초기 프레임/값·함수 사본·조기 거부·빈 모양·양수/0발자국·방향 관계/표면 무필터·다른풀/범위 진단) 통과했다. 배열 x/y전치 변이6/24검출·SHA까지바이트 원복 후 전체 비증분 Release빌드0경고0오류·최종관련Core410/410·Client21/21통과(실패0·건너뜀0·각종료0). PORT_MAP785(L1 372+port413), fixture269. 문서 docs/dotnet-placement-preview-20261114.md, 해당 작업 완료. 실제게임/MP/금지플레이 입력 없음. 보호원본/CP/WB미변경. 후보충돌/일반지역정책·전체사제월드/HostSession과 M1전체0/34는 잔여다.

#### 0.519 — 일반/사제 배치 접두와 전체 픽셀 모양 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.518은7c3efe9bd(20261114-74 DN)로 완료했다. 다음 사제 월드 연결 선행으로 RawCanonPlacement·RawPriestPlacement·RawCanonPixelShape·RawPriestPlacementShape를 같은 현재 타입/프레임/SHP/진단 자료에 이식한다. 기존 CanonPlacementQuery·PriestPlacementQuery는 재사용한다. 원본 세판본 배치/모양4fixture와 실제 생성자 조합·현재 자료/함수 사본·다른풀 거부를 대조하고 대표변이 뒤 원복한다. 이번 범위는 접두와 픽셀 getter이며 후반 미리보기/충돌/지역·전체 월드/호스트는 다음 단계다. 실제 금지 플레이/MP 검사는 하지 않는다.

- 일반/사제 배치와 전체 픽셀 getter4모듈을 구현했다. 원본 세판본20398입력과 새21검사(현재 자료/공유DWORD/함수 사본/다른풀/기준점/프레임·해제표식/실제패턴/실제PATCH서버사제생성) 통과. 최초 CD 합성1실패는 실제 C# 공장이PATCH188타입만 받기 때문이라 CD 모양/배치 검사는 유지하고 공장합성만PATCH로 분리했다. 소유자 비교를 모양 뒤로 늦춘 변이는8/21실패를 검출했고 SHA까지바이트 원복했다. 원복 뒤 전체 비증분 Release빌드0경고0오류·최종관련Core386/386·Client21/21통과(실패/건너뜀0·종료0). PORT_MAP778(L1 364+port414), fixture265. 문서 docs/dotnet-placement-shape-20261114.md, 해당 작업 완료. 실제게임/MP/금지플레이 입력 없음. 보호 원본/CP/WB 미변경. 후반 미리보기·기하·충돌/지역·사제 월드/HostSession과 M1전체0/34는 잔여다.

#### 0.518 — 사제 생성·Carrier 지면 확인 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.517은dfdbcd655(20261114-73 DN)로 완료했다. RawPriestSpawn.h/.cpp·RawCarrierCheck.h/.cpp 전체와 해방/생성/후처리 어댑터를 이식했다. 기존 PriestPlacementQuery를 재사용해 초기 중복 선언 빌드 오류를 해소했다. 원본 생성1236(세판본)+Carrier PATCH676 =1912입력이 처음부터 일치했다. 실제 종속 해방→서버 사제 생성자→사제/공통Owner→현재Carrier 합성, 빈 후보의 배치 검사 생략, 함수 사본/다른풀 거부, 현재TYPE/지면·132후보 뒤 checking0x100/PATCH 실패/CD 미조회까지 새8검사 통과했다. 기존0.514/0.515 실제 공유Form/Kernel 수명 검증은 유지하며 이번 Pop은 기록 경계다.
- 캐시한spot을 배치 콜백 뒤 다시 읽는 변이는3/8검사(세판본 원본)가 검출했고 SHA256까지 바이트 원복했다. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·최종관련Core279/279·Client21/21 통과(실패0·건너뜀0·각종료0). PORT_MAP770(L1 356+port414), fixture261. 문서 docs/dotnet-priest-create-20261114.md, 해당 작업 완료. 실제 게임/MP/금지 플레이 입력 없음, 보호 원본/CP/WB 파일 미변경. Carrier의 CD 실제HP/세션은 미완성이므로 나머지1352행을 이번 대조로 세지 않는다. 전체 사제 월드·HostSession 결선과 배치 검사, M1 전체0/34는 잔여다.

#### 0.517 — 현재 공통/사제 소유자와 목록 조회 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.516은99449bd00(20261114-70 DN)로 완료했다. 실제 RawPriestWorldBindings 선행 소유자를 확인하니 기존 SquidOwner의 선행 TYPE/free/Player 전체 검사·타입 대여·현재 목록과 현행 cpp가 달랐다. 먼저 RawCommonOwner·SquidOwner·RawPriestOwner와 SquidPostPopList 현재 저장소/용량/개수 모듈을 묶어 이식한다. 생성/Carrier 확인은 다음0.518로 이어간다. 원래/요청/effective·현재 모드 재조회·진단 복귀와 부분 효과, 목록 쓰기 뒤 모드 변경을 독립 자료로 대조한다.
- 공통/사제 본체·타입 사본/대여·선택 진단·현재 목록 연결을 구현했다. 초기 빌드1경고(사제WORD의 int/uint OR)는 unsigned 명시로 교정했다. 원본 owner-current-local72·priestowner7416·priest-owner-current-local144 =7632입력과 실제 공통 몸체/현재 목록·타입대여/진단의 새7검사·기존Owner3검사(1000입력 포함), 합계10/10 통과했다. 최초4실패는 CD 원본 슬롯36BYTE를 C# 고정50BYTE와 비교한 검사 폭 오류였다. 원본36BYTE 전부와 남은14BYTE 입력 보존을 각각 대조하도록 판독기만 교정했다. 기존 도전20 선행 무변경 기대는 현행 원문의 제거/소유자 쓰기 뒤 선택 목록 실패로 교정했다. 호스트 Player/도장 두 Owner에도 같은 현재 TYPE/진단을 대여했다. PORT_MAP766(L1 351+port415), fixture259 연결. 전체빌드0경고/0오류, 호스트9/9 통과, 모드 선행 고정 변이2/7실패 검출·SHA까지 바이트 원복 뒤 비증분 전체빌드0경고/0오류다. Client일반전체1193/1193 통과. 2미션 cpp/dn4회 단순 진입 종료0, originals1394파일 해시 동일, dn 상태는0.512 대비 frame행 제외 동일·구조요약 동일이다. cpp는각미션 중립owner0 SP DWORD만 과거와 다르고 다른행 동일하다. Core전체회귀 중 기존 RawSurfaceWorld의 잘못된 예외형 기대1실패를 확인해 선택된 실제 진단 미연결의 InvalidOperationException으로 교정했다. 실제 RawSurfaceWorld Owner에도 현재 표/진단을 연결하고 현재World/진단 부분효과3검사를 추가했다. Core전체2076중2075통과/해당1실패만 확인했다. 실제 World 저장소 입력 교정/검사 재빌드0경고0오류 뒤 최종 관련Core232/232·Client21/21 통과(실패0·건너뜀0·각종료0)로 해소했다. 제품 변경 뒤 비증분 전체빌드0경고/0오류이며 새 World3검사도 포함한다. PORT_MAP766(L1 351+port415), fixture259. 문서 docs/dotnet-owner-current-20261114.md 작성, 해당 작업 완료. 생성/Carrier확인·전체 사제/HostSession 조립은 다음 작업이다. 전체 HostSession 조립 완료로 세지 않는다. 보호 원본/CP/WB 보존·실제 금지 입력 제외 유지, M1 전체0/34 유지.

#### 0.516 — 실제 Windows 소리 파일·장치 경계 (완료, 2026-11-14, HJOW-Athlon)

- 직전0.515는51b006882(20261114-68 DN)로 완료했다. cpp SoundDevice.h/.cpp의 WAVE/여덟 파일선택/감쇠/장치 초기화·종료/효과음 여덟 함수를 현재 Windows 호스트 경계로 옮긴다. 원문은 Winmm/DirectSound이고, C#에서도 같은 Win32 장치를 명시 토큰으로 연결한다. 기존 MonoGame 메뉴 임시 재생 교체는 후속 ClientAudio 수명 조립에서 수행한다. 음악 링 버퍼/worker는 후속 음악 모듈과 함께 잇고 실제 사제 HostSession 전체 결선도 잔여다.
- 독립 sounddevice-x86 세판본435×3자료와 파일선택/오프라인 수명, 실제 장치 음소거 재생·복제·정지 검사를 준비한다. WindowsSoundFiles·WindowsSoundDevice와 순수 SoundWave를 구현했고 독립1305입력·파일/오프라인 수명8건과 선행소리12건=20/20 통과했다. 복제 실패의 기존 출력을 보존하도록 out 대신 ref를 사용한다. 초기 UCRT sscanf export 오류는 SDK inline 구현의 __stdio_common_vsscanf 연결로 수정했고 파일 경로 기대값의 구분자는 GetFullPath로 교정했다. 실장치 명시 검사1건은 DirectSoundCreate 0x88780078(DSERR_NODRIVER)로 실패했고 cpp 동일 검사도 초기화 실패다. 출력 장치 활성 항목 부재(레지스트리 DeviceState에1없음)를 읽기 확인했다. 실재생 성공은 미확인으로 남기고 실패 위치/HRESULT 진단을 제공한다. 원문 짧은 fmt 비트 보정 생략 변이3/8검사 실패 검출·SHA까지 바이트 원복. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·Client일반전체1192/1192(실장치 Explicit만 선택 제외)·메타데이터6/6 통과(실패0·건너뜀0·각종료0). 문서 docs/dotnet-sound-device-20261114.md, PORT_MAP757(L1 346+port411), fixture256. 해당 이식 작업 완료. 실제 출력 성공은 확인 대기로 명시하며 음악/ClientAudio·메뉴 교체·사제 HostSession 결선은 잔여다. 실제 실행은 AGENTS 예외 호스트 HJOW-Athlon에서 수행하며 금지된 플레이 입력/MP는 제외한다. M1 전체0/34 유지.

#### 0.515 — 소리 이름/위치 재생·프로세스 수명 (완료, 2026-11-14, HJOW-Athlon)

- cpp Sound/SoundProcess와 공통 카메라 투영을 Audio 다섯 파일로 이식했다. 고정0x8000BYTE 이름표·현재 카메라 소비 순서·장치 복제/재생/정지·공유Kernel 부착/삭제·보호막/낙하 어댑터를 구현했다. PATCH 소리 프로세스6127 + 세 판본 재생14472/제어3189 = 원본23788입력이 일치했고 실제 보호막/공간/프레임/Kernel 수명·장치 효과 뒤 현재 뷰/TYPE 재조회 등 새12검사 통과했다. 첫 재생 판독기 OverflowException은 stoul식 하위DWORD 판독으로 교정했고 제품/자료 무변경이다.
- 음량 현재 뷰 재조회를 좌우용 사본으로 바꾼 변이는 새12검사 중3건이 검출했고 SoundPlayer SHA256까지 바이트 원복했다. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·Client전체1184/1184·메타데이터6/6 통과(실패0·건너뜀0·각종료0). PORT_MAP751(L1 346+port405), fixture255. 문서 docs/dotnet-sound-lifecycle-20261114.md 작성. 해당 작업 완료. 실제 파일/출력 장치 backend·캠페인 HostSession 전체 결선은 다음 작업이다. 실제 게임 실행 없음, 보호 원본/CP/WB 파일 미변경, 금지 플레이 입력 제외 유지. M1 전체0/34 유지.

#### 0.514 — 사제 낙하·현재 상태/Player 회복 계약 (완료, 2026-11-14, HJOW-Athlon)

- 이전0.513은92f395980(20261114-63 DN)으로 완료했다. 다음 원문 RawPriestFall.h/.cpp를 실제 프레임·보호막·공유Regular와 연결하기 위해 상태 조회 계약을 먼저 맞춘다. RawPriestState의 현재TYPE 대여·생성시 사본, Player9기록/owner0~8과 PATCH/CD 필드 폭을 현행 cpp로 교정 중이다. 원본 사제상태6752입력·낙하456입력의 PATCH 몫을 실제 HP/상태/낙하로 대조해 일치했다. 같은 실제 공유Form/Kernel·공장/owner·보호막 조회/삭제·현재권한/재예약/최종자기삭제 합성도 통과했다. 같은 실제 SHP/지정/진행기에 DWORD -1증분을 전달하는 어댑터와 분배/다른풀 거부까지 새7검사 통과했다.
- 초기 관련31/31·최종새7/7 통과, Core/Core.Tests 증분 빌드0경고/0오류다. 권한을 보호막/실제 공유 생성 전에 고정한 변이는2/7검사(낙하원본456·실제공유생성 합성)가 검출했고 SHA256까지 바이트 원복했다. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·최종관련Core312/312·Client전체1172/1172 통과(실패0·건너뜀0·각종료0). 문서는 `docs/dotnet-priest-fall-20261114.md`다. 해당 작업 완료. 실제 캠페인 HostSession의 낙하/소리 장치 조립은 다음 작업으로 유지한다. PORT_MAP739(L1 346+port393), 공유fixture252개(사제상태20256행/낙하1368행, 이번 대조는PATCH6752/456). CD의 필드 분기는 보존했지만 기존 실제 HP제공자/세션 CD 전체 복원과 CD 원본 재생을 완료로 세지 않는다. 실제 게임 실행 없음, 금지된 플레이 입력 제외 유지. 보호 원본/CP/WB 파일 미변경. M1 전체0/34 유지.

#### 0.513 — 보호막 최초 예약·Regular 수명과 현재 후처리 분배 (완료, 2026-11-14, HJOW-Athlon)

- 0.512 커밋5388f45e2(20261114-62 DN) 완료 뒤 분리 검증한 보호막 접두/Regular를 제품에 적용했다. 독립 두 확보·실제 finder·typed 현재DWORD/float 조회·진행/삭제·선택 checking 진단을 옮겼다. 일반 RawPostPop에 같은 풀 접두를 등록하고 실제VT로 분배해 공통 깊이를 한 번 끝낸다. PATCH 억제 관문은 raw/TYPE 선행 검사보다 먼저 반환하도록 원문 순서로 맞췄고 미연결 파생 가상 몸체를 공통 성공으로 넘기지 않는다.
- `forcefieldpostpop-x86.tsv`3624행을 공유하고 PATCH1208입력을 대조한다. 원본/CD 자료·해시는 바꾸지 않았다. 공유fixture250개·PORT_MAP736행(L1 346+port390). 적용 전 분리한 같은 코드의 golden/현재부모/CD계약8검사 통과. 제품 적용 뒤 전체 비증분 Release 빌드0경고/0오류·최종관련Core305/305·Client전체1172/1172 통과(실패0·건너뜀0·각종료0). 첫 관련211건 중1건은 과거 미연결Player 공통성공 기대였으며 현재cpp의 명시실패/깊이보존 계약으로 검사만 교정했다. 같은 제품 소스의 분리 컴파일 사본에서 첫확보 실패 조기반환 변이2/6 검출, SHA256까지 원복했다. 문서는 `docs/dotnet-forcefield-lifecycle-20261114.md`다. 해당 작업 완료. 실제 게임 실행 없음. 결과 `extracted/dotnet-forcefield-lifecycle-0513/`.
- 실제 월드/호스트에 보호막 전체 예약·Regular 분배를 붙이는 조립은 후속 R4 작업이며 해당 기능 완료로 세지 않는다. 다음 순서는 사제 낙하/소리 수명과 해당 장치 조립이다. M1 전체0/34 유지.

#### 0.512 — 일반 Pop 현재 부모·실제 프레임·필수 전송기 연결 (완료, 2026-11-14, HJOW-Athlon)

- RawPop PATCH를 현재 부모에 연결하고 PopCurrent·현재 진단/TYPE/SHP·공유 Activate 깊이·선택된 송신을 추가했다. CD는 기존 별도 몸체다. SquidDisplay의 Pop/Unpop 인터페이스와 같은 풀·표시·해시·spot 검증, SquidFrameBinding을 추가했다.
- 단독 캠페인에서도 실행되는 실제 RawSquidTransmission35BYTE/압축/분배를 이식했다. 새 C++ 정적 모듈 프로브256입력과 첫64BYTE·나머지1984BYTE 보존·누적·로그·콜백 횟수 대조는 통과했다. x86 원본fixture249개와 별도인 C++ 부품 자료다. 내보내기 도구 재실행 출력 SHA256 동일.
- 첫 전체 회귀는 Assets366통과, Core2051중2040통과/11실패, Client1172중1130통과/42실패였다. 새 Pop이 요구하는 실제 writer와 현재 grid 진단 로그를 기존 월드/검사 호출자가 공급하지 않은 원인을 확인했다. 현재 RawSurfaceWorld의 같은 표시/공간/전송 결선, 장기 GameAux 버퍼/약화 owner, 실제 HP getter와 삭제 writer를 연결했다. 독립 typed 월드는 cpp와 같이 실제 writer 뒤 방송에서 반환한다. Native graph/Fort 검사는 cpp의 진단로그/자식 도달 관측을 동일하게 연결했고 golden 기대값은 바꾸지 않았다. 원복 후 최종 비증분 빌드0경고/0오류, 집중177/177·최종Client1172/1172·Assets366/366 통과다. 최종Core전체는2054중2053통과/메타데이터1실패였으며 PORT_MAP 오타1건(RawSurfaceStorage.h는 별도 파일 없음)을 제거한 뒤 metadata6/6 통과했다.
- 첫 단순 미션 L2 실행은 두 미션×cpp/dn 모두 종료0, 원본1394파일 전후 동일이다. 0.504와 상태 내용 동일(세이브 디 아일랜드의 frame 행90/92만 다름). 전체 일치0/34 유지. 결과 `extracted/dotnet-pop-current-0512/`. 최종 프레임 DWORD→BYTE 판정 변이는 writer9건 중 golden1건이 검출, SHA256까지 동일하게 원복했다. 수정 뒤 단순 진입도4회 종료0·원본1394파일 동일·이전 상태 내용 동일이다. 문서는 `docs/dotnet-pop-current-20261114.md`, PORT_MAP734(L1 346+port388), 공유fixture249+별도CPP256입력1개. Core의 유일한 실패는 같은 PORT_MAP 오타였고 수정 후 metadata6/6 통과해 미해결 실패0이다. 제품 코드는 원복 뒤 비증분 빌드 상태 그대로다. 전체 회귀의 실패 이력을 숨기지 않으며 전체 재실행 대신 변경된 메타데이터6건을 재대조했다. 해당 작업 완료, M1 전체0/34는 유지한다. 후속 보호막 수명은0.513에서 제품 반영과 검증을 완료했다. 금지된 공격/포획/희생/승리 입력 없음. 보호 파일·CP/WB 보존.

#### 0.511 — 공통 Pop·Activate 현재 부모 ( 2026-11-14, HJOW-Athlon)

- `RawSquidActivate`·`RawSquidPopParent`의 PATCH 부모를 이식했다. [구현·검증 범위·한계](docs/dotnet-pop-activate-parents-20261114.md). 선택된 장치/진단 복귀 뒤 현재 조회·부분 raw/spot/hash 쓰기·저장한 후보/버킷·최초Pop/깊이 순서를 보존한다. 같은 TYPE 칸의 참조 보기는 배열 원소 교체 뒤에도 현재 필드를 읽는다. 기존 공통 장부 Depth를 같은 backing DWORD로 공개하는 ref 보기만 더했으며 기존 Activate/일반Pop 호출 교체는 아직 하지 않았다.
- 독립 cpp 부모 계약과 실제 공통 장부 합성 검사를 추가했다. 동일 MSVC18 도구 집합의 헤드리스 CRT 프로브로9개 로그 입력의 NaN/무한대/부호있는0/짝수 반올림을 확인했다. 프로브는 `extracted/dotnet-pop-parent-0511/`에 있고 게임을 실행하지 않는다.
- **완료 검증:** 초기집중39건 통과. 깊이 증가1→2와 SHP선행조회 삽입 변이에서34건 중12건 실패를 검출하고 두 소스를 바이트 원복했다. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·관련Core438(최종부모/manifest40포함)·Client호스트116 통과(실패0·건너뜀0·각종료0). 결과는 `extracted/dotnet-pop-parent-0511/`의 TRX에 보존한다. PORT_MAP721(L1 346+port375), fixture249(원본Point/Pop 앞단260입력 추가, 기존248해시 불변). 기존 Pop·AI·실제 호스트 결선은 완료로 세지 않는다. 다음0.512는 현재 부모의 일반 Pop 어댑터·표시/SHP 연결과 기존 원본fixture 대조→프레임 연결→낙하/포스필드 수명이다. PATCH에서 `BindDiagnostics`·`PopCurrent`·실제 Activate의 공유깊이를 잇고 CD 기존 몸체를 구별한다. `IRawPopPostPop` 구현은 실제 RawPostPop과 RawPopTests의 RecordingPostPop 두 곳이다. 기존 안전가드 기대는 최신 cpp의 도달/부분 실패 계약과 대조하고 원본fixture 기대값은 바꾸지 않는다. 서버 풀이 선택되면 미연결 송신을 성공 상수로 우회하지 말고 실제 no-peer writer 계약을 확인한다. M1 0/34.

#### 0.510 — 공통 프레임 진행·현재 TYPE와 지정기 계약 ( 2026-11-14, HJOW-Athlon)

- `RawFrameAdvance`와 실제 RiftTypeFrames의 signed 방향 조회를 옮겼다. [구현·관측·한계](docs/dotnet-frame-advance-20261114.md). `SquidFrame`은 생성 시 TYPE 깊은 사본·함수 객체 값 복사·현재TYPE 명시 대여/복귀와 PATCH DWORD/CD BYTE 배치를 최신 cpp대로 보정했다. 기존 서피스 월드 장기 지정기는 현재 표를 명시 대여한다.
- 첫 집중 검사에서 세 판본 진행23040관측·지정5760관측·실제 PATCH 공간·현재 표/관문은 통과했다. CD 실제 공간 합성은 현행 공장이PATCH전용이라 구성에서 거부했으며, 제품 비대상인 CD 공장을 이번 단위에서 확대하지 않고 원본 CD 관측만 별도 재생한다. PATCH 실제 합성은 계속 검사한다.
- **완료 검증:** 집중20건 통과. 한 번 감싸기→modulo 변이는 PATCH2040·CD1952·별도10.37 1952, 총5944관측 불일치로3/3검사에서 검출하고 원복했다. 원복 뒤 전체 비증분 Release 빌드0경고/0오류·관련Core404·Client전체1172 통과(실패0·건너뜀0·각종료0). 산출물은 `extracted/dotnet-frame-advance-0510/`다. 전체 회귀 기준선은0.509의3532건이다.
- PORT_MAP717(L1 346+port371), fixture248(새 frameadvance23040행, 기존247해시 불변). 다음0.511은 공통 `RawSquidActivate`·`RawSquidPopParent`의 도달 장치/현재 상태·부분 실패 순서부터 옮긴다. 일반 Pop의 표시/현재 SHP 조회 순서는 아직 오래된 계약이므로 실제 표시/공간 어댑터 전체 완료로 세지 않는다. 그 계약과 `SquidFrameBinding`→낙하·포스필드 수명→요새 Pop·AI 결선이 다음이다. 새 게임 실행·금지 플레이 재현·MP 없음, M1 0/34.

#### 0.509 — 보호막 조회·생성과 실제 사제 후반 연결 ( 2026-11-14, HJOW-Athlon)

- `RawPriestForcefield`·`RawPriestShield`와 사제 Tail 보호막 어댑터를 실제 finder·공장·소유자·HP·공통 장부/ProcessHost에 연결했다. 최신 finder의0×0 발자국 허용 차이도 교정했다. [범위·원본 대조·한계](docs/dotnet-priest-shield-20261114.md).
- 조회/사전파괴5952출력+생성2496출력, 총8448관측 일치. DWORD 역할326 잘림·존재 여부 반전 변이를 검출한 뒤 원복했다. 전체 비증분 Release 빌드는 경고0·오류0이다. PORT_MAP714(L1 346+port368), fixture247(기존245해시 불변).
- **원복 후 전체 회귀 완료:** Assets366·Core1994·Client1172=3532건, 실패0·건너뜀0·각종료0. Core는13분57초에 종결했다. 결과는 `extracted/dotnet-priest-shield-0509/regression/`의3개TRX에 보존한다. 최종 생성 전 보호 검사도 이 전체 회귀에 포함한다.
- 실제 합성은 포스필드 전체 Pop·낙하 대신 명시 외부 공간 입력·요청 관측이다. 호스트 전체 Pop·플레이 검증으로 세지 않는다. 새 게임 실행·금지 플레이 재현·MP·신규NB·외부접속 없음. 보호 파일·CP/WB 보존, M1 0/34 유지.
- 다음0.510: `SquidFrame` 최신 현재TYPE/함수객체 사본 계약과 공통 `RawFrameAdvance`를 먼저 맞추고 실제 표시/공간 연결의 미이식 계약을 이어 옮긴다. 이어 `RawPriestFall`·포스필드후처리·소리 소비자→실제 요새 Pop→AI 생성/초기갱신이다.

#### 0.508 — Carrier 실제 도달 순서·현재 전역·원본7344입력 (2026-11-14, HJOW-Athlon)

- 0.507에서 발견한 Carrier의 오래된 PATCH 선행 검사 계약을 최신 cpp `RawCarrierPostPop.h/.cpp`로 교정했다. PATCH는 도달 장치만 요구하고 앞선 효과를 늦은 실패에서 보존한다. 최초genus물리조회·간헐천/extra 관문·현재권한/로딩/TYPE 대여·생성시소유사본 복귀를 보존한다. CD의 기존 사전 검사/필드는 별도 유지했다. [근거·구현·한계](docs/dotnet-carrier-current-20261114.md).
- 기존 carrierpostpop 원본3판본7344입력 전수 일치. C++ 현재부모계약12건+공유boss1건을 옮겼고 사전 검사 복귀 변이에서13/13 실패를 검출해 원복했다. 이전 PATCH 안전가드 기대는 cpp처럼 CD 보호 검사로 옮겼고 공통 실패 전 지면 효과 기대도 최신 순서로 바꿨다. 원본 관측은 불변이다.
- 원복 뒤 전체 비증분 Release 빌드 경고0·오류0. 관련자산의 낡은 사전 검사 횟수 기대1건도 최신 계약0으로 교정했고 원본 기대값은 유지했다. 원복 뒤 관련집중64건 통과. 결과는 `extracted/dotnet-carrier-current-0508/`에 보존한다. PORT_MAP은기존CPP행기준갱신+헤더1행으로710(L1 346+port364), fixture245개 유지.
- 새 게임 실행·포획/희생/공격/승리·MP·신규NB·외부접속·기존경로은퇴 없음. 보호 파일·CP/WB 보존. M1 0/34 유지.
- 다음0.509: 보호막 조회 `RawPriestForcefield`·생성 `RawPriestShield`·Tail보호막 어댑터를 실제 공간/공장/소유자에 연결한다. 이어 낙하·포스필드후처리 소비자→실제 요새 Pop/AI 생성/초기갱신 순서다. 호스트 Driver에는 아직 사제/AI 호출이 연결되지 않았다.

#### 0.507 — 사제 후반·실제 HP 공유·일반 Pop 분배 (2026-11-14, HJOW-Athlon)

- `RawPriestPostPopTail`의 PATCH 전체 wrapper를 옮겼다. 접두→Carrier→현재 extra/Immobile→거의 올림 지면/방향→낙하/보호막 순서다. `RawPriestState`가 접두와 같은 실제 RawSquidReward를 빌리게 하고 `RawPostPop`에 같은 풀의 전체 사제 몸체를 명시 등록/분배한다. [근거·구현·한계](docs/dotnet-priest-tail-20261114.md).
- 원본2048입력의 순서·raw·목록·9DWORD 전수 일치. 일반 Pop 공간 등록·사제 조회·장부/깊이·직접VT분배·불필요한NaN/프레임 읽기 생략 포함 집중29건 통과. 배율0.9999→0.0001 변이는120/2048 불일치를 검출한 뒤 원복했다. 원복 뒤 전체 비증분 Release 빌드 경고0·오류0·관련집중34건 통과. 산출물은 `extracted/dotnet-priest-tail-0507/`에 보존한다.
- PORT_MAP2행(709=L1 346+port363), fixture1개 추가로245개. 기존244개 해시 불변. 새 게임 실행·포획/희생/공격/승리·MP·신규NB·외부접속·기존경로은퇴 없음. 보호 파일·CP/WB 보존. M1 0/34 유지.
- **다음0.508 우선:** 최신cpp Carrier는 PATCH 사전 검사 없이 도달한 효과/현재TYPE·권한·로딩을 조회하지만 기존C#은 무조건 사전 검사/구성 때 모든 효과 요구다. 이번 정상 입력 대조는 이 차이를 선택하지 않는다. 호스트 결선 전에 `RawCarrierPostPop`을 HEAD 계약으로 보정한다. 이후 보호막 조회/생성·낙하·Tail보호막 어댑터→실제 요새 Pop→AI 생성/초기갱신 순서다. 일반 분배 장치 추가만으로 실제 요새/AI 완료로 세지 않는다.

#### 0.506 — 사제 등록·회복 예약 접두와 PATCH 원본2392입력 (2026-11-14, HJOW-Athlon)

- `RawPriestPostPop`의 PATCH 접두·검사·소유자 상태 한DWORD 초기화·ProcessHost 어댑터를 cpp 동명 모듈에서 옮겼다. 기존 `PriestPostPopState`에 공유 소유자9DWORD를 더하고 GetPriests를 재사용한다. 현재 목록→검색→확보→현재TYPE/객체HP→예약→현재owner 상태 쓰기 순서를 유지한다. [구현·근거·한계](docs/dotnet-priest-postpop-20261114.md).
- 원본 두x87정밀도2392입력의 전체 목록/상태/효과/슬롯 일치. 실제 생성자·HP·조회·Form·커널로 중복 예약 방지·실행·삭제/재생성을 검증했다. 신규 포함 집중20건 통과. 나눗수6→7 변이는 원본356/2392 불일치와 실제payload8.25→7 실패를 검출한 뒤 원복했다.
- 원복 뒤 전체 비증분 Release 빌드 경고0·오류0. 원복 최종 집중24건 통과. 산출물은 `extracted/dotnet-priest-postpop-0506/`에 둔다. PORT_MAP3행(707=L1 346+port361), fixture1개 추가해244개. 기존243개 해시 불변. HP 모듈이PATCH전용이라 CD접두는 명시 거부한다.
- 새 게임 실행·포획/희생/공격/승리·MP 재현·신규NB·외부접속·기존경로은퇴 없음. 보호 파일·CP/WB 보존. M1은0/34 유지.
- 다음0.507: `RawPriestPostPopTail`(접두→Carrier→현재 Immobile/프레임/지면→낙하·보호막)을 원본대로 옮긴다. RawPriestState·RawPathAnimation·RawCarrierPostPop·RawFrame은 이미 있으므로 재사용하고 미이식 보호막/낙하 소비자를 채운다. 전체 가상 PostPop·실제 요새 Pop을 연결한 뒤 AI 생성/초기갱신으로 진행한다. 접두만 전체 사제 Pop 성공으로 등록하지 않는다.

#### 0.505 — 실제 AI 사제 조회·색 변경과 원본5067입력 (2026-11-14, HJOW-Athlon)

- `RawPriestLookup`·`RawPlayerColor`를 cpp 동명 모듈에서 직역했다. 기존 사제 상태의 실제 `GetPriests()`를 조회하고 signed 개수·DWORD owner·첫 일치 조기 종료·dead2만 제외를 보존한다. 색 표 선쓰기·SID2 iterator·free만 제외·현재 표 재조회도 보존한다. [구현·근거·한계](docs/dotnet-ai-world-lookups-20261114.md).
- 사제 3판본4851입력·색216입력 전수 일치, 공유 목록/손상 경계·소비자 실패/재조회 검증을 포함한 집중10건 통과. CD 관측 직렬화 폭은 cpp의 실제36바이트에 맞췄다(C# 공통50바이트 풀의 제품 코드 변경 없음). void 제외 변이는 판본별91/1617, 색의 dead 제외 변이는216/216 불일치를 검출하고 원복했다.
- 원복 후 전체 비증분 Release 빌드 경고0·오류0. 원복 뒤 관련집중27건 통과. 결과는 `extracted/dotnet-ai-lookups-0505/restored.trx`에 둔다. PORT_MAP3행(704=L1 346+port358), fixture2개 추가해243개. 기존241개 해시 불변.
- 새 게임 실행·신규NB·외부접속·기존경로은퇴 없음. 보호 파일·CP/WB 보존. M1은0/34 유지.
- 다음0.506: 사제 PostPop 접두(목록/회복 Regular/소유자 상태)를 기존 `PriestPostPopState`에 연결한다. 클래스는 이미 `RawPriestPreDestroy.cs`에 있으므로 중복 생성하지 않는다. 이후 공유 사제 목록·AI 메모리/생성/초기갱신·기술 Form·지역/색 프레임을 조립한다. 요새 사제 Pop이 아직 비어 있으므로 빈 목록 조회를 AI 완료로 세지 않는다.

#### 0.504 — 실제 Player 화신·기술부모·Regular·공유 장부 결선 (2026-11-14, HJOW-Athlon)

- R4 실제 AI 생성의 선행 결선 완료. 빈 화신 Pop을 실제 공간 등록·Player PostPop으로 교체하고 월드 적재/AI가 같은 Player 몸체를 공유한다. 실제 Regular 객체를 예약한 뒤 부모 체인·커널에 부착한다. TotalMade·비용·깊이·생산 알림도 단일 장부로 공유하고 실제 프로세스 정보를 덤프한다. [근거·구현·제한](docs/dotnet-player-world-20261114.md).
- 전체 비증분 Release 빌드 경고0·오류0. Client 집중23건·관련 Core134건 통과. 좌표 x+1 대표 변이에서 공간 조회 검사가 실패했고 원복했다. 원복 후Client23·PORT_MAP/manifest6 재통과. 산출물은 `extracted/dotnet-player-0504/`다.
- 같은 시드·고정25ms로 thewarbegins/savetheisland cpp/dn 4회 실행(각120프레임·종료0). 두 미션의 로컬 기술부모5·첫 PID/Form/부모1/6/5·이벤트/payload 비트가 일치한다. 전체 L2는 불일치, M1 0/34 유지. originals1394파일 해시 불변. 공격/포획/희생/승리·MP 실행 없음.
- PORT_MAP3행 추가(701=L1 346+port355), fixture241개 유지. 신규NB/외부접속/기존경로은퇴 없음. 보호 파일·CP/WB 보존.
- 다음0.505: 실제 AI 메모리·생성자·초기갱신·기술 Form·공유 사제 목록 조회·지역·색 변경을 같은 세션에 조립한다. 프레임 시계/실행, 다른 파생 PostPop·농지·장부 전체 집계·요새 이미지·메뉴 시작·M1 재대조는 계속 잔여다.

#### 0.503 — 실제 TYPE에너지·AI getter와 원본2352입력 (2026-11-14, HJOW-Athlon)

- R4 AI결선선행조회완료. `RawTypeRequiredEnergy`·`AiTypeRequiredEnergy`를cpp의같은모듈에서직역했다. 실제mana AST순서·9바이트/NUL·레벨/원소보충·ASCII화·현재188TYPE/116AST를보존한다. [단위문서](docs/dotnet-ai-energy-20261114.md).
- cpp검사3건이원본2352입력·실제188TYPE입력별원본결과·현재변경·경계를대조했다. 집중14건통과. level2→3변이에서22/2352불일치검출후원복. 전체비증분Release빌드 경고0·오류0, 원복뒤PORT_MAP포함집중16건재통과.
- PORT_MAP4행(698=L1 346+port352), fixture241개. 게임실행/신규NB/외부접속/은퇴없음·보호파일/CP/WB보존.
- **호스트상태정정:** 현재StartMission에는AI생성호출자체가없다. 이전'임시brain토큰사용'은기존경로/독립검사와혼동한표현이다. 다음0.504는실제Player Pop/PostPop·Player+9c기술부모·Regular확보/부착을공유상태에결선하고그뒤AI생성/초기갱신을호스트에서호출한다. 사제조회는실제공유postPop목록을사용한다. M1은0/34다.

#### 0.502 — 실제 AI초기갱신·기술/원소분류와 원본149입력 (2026-11-14, HJOW-Athlon)

- R4 AI초기갱신부품완료. `RawAiBrainInitialUpdate`를cppHEAD `bcad4b479`에서직역했다. 같은실제body/Player/기술Form·현재TYPE/전역·이름·누적빌더/원소목록·비용절삭·theme5공유계수를보존한다. 상세는[단위문서](docs/dotnet-ai-initial-update-20261114.md).
- 신규3건: 원본133+theme5추가16입력의전체body/빌더/Player/새Form/효과순서, 지갑보존·선택metadata누락후부분효과. 집중17건통과. battery증가1→2변이가67/133+5/16불일치를검출한뒤원복했다. 전체비증분Release빌드 경고0·오류0, 원복뒤PORT_MAP포함집중19건도통과했다.
- PORT_MAP2행(694=L1 344+port350), fixture240개·원본fixture불변. 게임실행·신규NB/외부접속/기존경로은퇴없음. 보호파일·CP/WB작업미수정.
- 다음0.503: HostSession에실제AI메모리/생성/초기갱신/Participation수명을소유하고사제·지역·TYPE에너지·언어이름·기술Form부착을결선해임시brain토큰을교체한다. 그뒤Pop후속/농지/장부·요새이미지·메뉴시작·M1재대조로진행한다. M1은0/34이며미션플레이불가상태를완료로세지않는다.

#### 0.501 — 실제 Player기술 내용물·40개목록과 원본158입력 (2026-11-14, HJOW-Athlon)

- R4 AI초기갱신의기술부모완료. `RawPlayerTechnology`를cppHEAD `f3a5c821f`에서직역했다. 같은Player/contained finder/create를공유하며타입진단·현재부모·가상삭제·Ensure·40개중복제거/all·무진행경계를보존한다. 상세는[단위문서](docs/dotnet-player-technology-20261114.md).
- 신규2건: 원본158입력전체raw/목록/새Form/효과순서대조·실제116자산188TYPE조회/미등록생성전거부. 집중16건통과. Find의flags4→1변이가42/158불일치를검출했고원복했다. 전체비증분Release빌드 경고0·오류0, 원복뒤PORT_MAP검사를포함한집중18건도통과했다.
- PORT_MAP2행(692=L1 342+port350), fixture238개. 원본fixture불변·게임실행없음·신규NB/외부접속/은퇴없음. 보호파일·CP/WB작업미수정.
- 다음: `RawAiBrainInitialUpdate`의133+theme5추가16입력대조→실제AI생성/초기갱신을호스트에같이결선. 라이브는아직임시토큰이며M1은0/34다.

#### 0.500 — 실제 AI본체 생성·역순 소멸과 원본160입력 (2026-11-14, HJOW-Athlon)

- R4 AI실생성자 첫 조각 완료. `RawAiBrainMemory`·`RawAiBrainConstructor`를cppHEAD `04c87c034`에서 직역했다. 실제0xb08/17목록·미초기화보존·같은Player/동맹/섬·예외정리·분리소멸이다. 초기갱신이 없으므로 라이브호스트에는 아직 연결하지 않았다. 상세는 [단위 문서](docs/dotnet-ai-brain-20261114.md).
- `AiBrainTests`4건: 원본160입력의 전체body/Player/동맹/섬/18할당/17해제·효과순서, Participation수명·미연결거부·현재그래프2회조회. 집중14건 통과. +0x3d8의1→2변이에서160/160불일치 검출 후 원복했다. 전체 비증분Release빌드 경고0·오류0, 원복 뒤 집중14건도 다시 통과했다.
- PORT_MAP2행추가(690=L1 340+port350), fixture237개(ai-brain161행 추가), 원본fixture불변. 신규NB·외부접속변화·기존경로은퇴 없음. 게임실행 없음·보호파일/CP/WB미수정.
- 0.499전체회귀 종결: Assets366·Core1945·Client1171=3482, 실패0·건너뜀0·종료0. 다음은 `RawPlayerTechnology`와 `RawAiBrainInitialUpdate` 이식 후 호스트의 임시brain토큰을 실제메모리/초기갱신으로 함께 교체한다. M1은0/34다.

#### 0.499 — 34경로 같은 시드 기준선·일괄 도구 (2026-11-14, HJOW-Athlon)

- R4 M1 자료 수집 완료, 구현 완료는 아님. 일반31경로 cpp/dn·데모3경로 cpp를120프레임씩 총65회 실행(종료0). 전체일치0/34·수집오류0·데모 StartDemo3개 dn미연결. 상세·명령·새 훈련5개 슬롯 집계는 [단위 문서](docs/dotnet-l2-batch-20261114.md).
- `l2_compare.py`와 검사4건 추가. 오라클SHA·명령·로그·빈 사용자 루트·originals전후1394파일 해시 감사 보존. cpp기준 제품1310의 CD분기 영향 정적 확인. 이전 기준선 보존. 제품코드·PORT_MAP·fixture·NB·외부접속 변화 없음.
- 솔루션Release빌드 경고0·오류0, Assets366·Client1171 실패0. Core전체회귀는 진행 중이며 다음 단위에서 종결 기록. 실제 창 실행 있음(AGENTS예외PC); 포획/희생·특정 적 공격·승리·MP재현 제외. 보호파일·CP/WB변경 미수정.
- 다음: AI실생성자와 초기갱신·세션 Bind·요새이미지·메뉴 미션 시작·postPop·농지·장부 연결. 같은 시드의31+3경로로 M1을 재판정한다.

#### 0.498 — 26번째 경로 bc4bow_rush76 기준선·덤프와 담긴 섬 4건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백일곱 번째 단위. M1 item 8의 다음 경로(Priest Training 4번째 경로 bc4bow_rush76 X-Bows, 순서 근거 `offical2.english` — Priest Training 9개 중 네 번째)의 cpp 기준선·dn 덤프를 만들고 26경로 구조를 대조하며, 4섬·4사제·4참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.497과 같은 절차(cpp 메뉴 경로 `--mission bc4bow_rush76` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission bc4bow_rush76 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 없음 — `git diff c909c2dfa..HEAD -- cpppj/src` 0건). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 열세 번째 경로다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 547,434바이트) + dn 덤프 생성(종료 코드 0, 374,172바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 3,272·cpp 3,376·공통 3,272·dn_only 0·cpp_only 104(소규모 SID·46번 50개·6번 17개·94/95번 16개씩 게임플레이 잔여)·타입 2,970·소유자 792·프레임 150·전체 108. Game·CRT 행은 시드가 달라 구조만 대조(cpp 2959533536/1451214786 vs dn 2906550144/296677442, CRT parity dn 측은 296677442 유지). totals cpp 84300 vs dn 0·totalmade cpp 22종 vs dn 0(R6 postPop 집계 잔여). 그래프 234/255 일치, row0 surfaces만 cpp -16 vs dn 0이고 row254 sentinel(32253)은 일치한다(나머지 20행 R6 per-Pop 번호 잔여). player 이름은 You·Lord Ice·Count Thunder·Mr Sun 양쪽 일치(p1 꼬리 pson vs son 차이는 핸들 버퍼 공유 잔여, 번호 이동 없음), +0x78은 dn 전부 127 유지·cpp p1=0·p2=1·p3=3·p4=2·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 3,272개 집합·바이트 동일, 두 실행 바이트 동일 frame 91 포함). paired 산출물은 `extracted/dotnet-l2-baseline/bc4bow_rush76/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 2,777셀(x,y slot +14/+18 float 비트, cpp 지형 2,479+농지 121번 298=2,777 일치)의 군집(4방향 연결) bounds 4건 — 동섬 (96,160)-(143,191) 977셀·북동섬 (144,193)-(190,223) 686셀·남섬 (98,96)-(142,127) 651셀·서섬 (49,193)-(92,207) 463셀에 cpp 사제 좌표(TYPE 158 object x,y 비트→float) 대조 — owner1 (126,175)→동섬 ✓(p1=0)·owner2 (118,112)→남섬 ✓(p2=1)·owner4 (160,214)→북동섬 ✓(p4=2)·owner3 (77,201)→서섬 ✓(p3=3)으로 4건 일치한다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건·tutorial4 2건·tutorial5 2건·tutorial6 2건·bc1menu76 6건·bc2bridging76 2건·bc3acid_bar76 2건에 4건을 더해 51건 전수 일치가 됐다.
- 소유자 귀속: 소유자 불일치 2,480건 중 2,479건은 지형 97번(cpp 소유자 1·2·3·4 vs dn 0, cpp 농지 121번 298셀은 dn에 없음)이라 적재 중 postPop 소유자 스탬프(0.469)와 농지 배치 파이프라인(0.455) 잔여로 귀속하고, 89번 1건 교차와 cpp_only 104건은 게임플레이 진화(0.469) 잔여로 둔다. AI 이름 Lord Ice·Count Thunder·Mr Sun 양쪽 일치·이웃 SID 타입·순서 일치는 번호 이동 없음을 뒷받침하며, 코드 변경 없이 문서화만 한다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다).
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 25/34).

#### 0.497 — 25번째 경로 bc3acid_bar76 기준선·덤프와 담긴 섬 2건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백여섯 번째 단위. M1 item 8의 다음 경로(Priest Training 3번째 경로 bc3acid_bar76 Acid Bars, 순서 근거 `offical2.english` — Priest Training 9개 중 세 번째)의 cpp 기준선·dn 덤프를 만들고 25경로 구조를 대조하며, 2섬·2신전·2참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.496과 같은 절차(cpp 메뉴 경로 `--mission bc3acid_bar76` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission bc3acid_bar76 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 없음 — `git diff 06cd2434b..HEAD -- cpppj/src` 0건). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 열두 번째 경로다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 506,557바이트) + dn 덤프 생성(종료 코드 0, 334,848바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 2,921·cpp 3,133·공통 2,921·dn_only 0·cpp_only 212(소규모 SID·155번 77개·94/95번 35개씩 게임플레이 잔여 — man o war 공격 개시 중)·타입 1,988·소유자 753·프레임 257·전체 95. Game·CRT 행은 시드가 달라 구조만 대조(cpp 2888346819/1451214786 vs dn 421656047/296677442, CRT parity dn 측은 296677442 유지). totals cpp 90600 vs dn 0·totalmade cpp 23종 vs dn 0(R6 postPop 집계 잔여). 그래프 245/255 일치, row0 surfaces만 cpp -59 vs dn 0이고 row254 sentinel(32253)은 일치한다(나머지 9행 R6 per-Pop 번호 잔여). player 이름은 You·Jailer 양쪽 일치, +0x78은 dn 전부 127 유지·cpp p1=1·p2=0·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 2,921개 집합·바이트 동일, frame 행만 90 vs 91 1줄 차이). paired 산출물은 `extracted/dotnet-l2-baseline/bc3acid_bar76/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 1,971셀(x,y slot +14/+18 float 비트, cpp 지형 1,780+농지 121번 191=1,971 일치)의 군집(4방향 연결) bounds 2건 — 남섬 (112,81)-(159,127) 1,058셀·북섬 (112,192)-(158,223) 913셀에 cpp 신전 좌표(object x,y 비트→float) 대조 — owner2 (131,89)→남섬 ✓(p2=0)·owner1 (146,210)→북섬 ✓(p1=1)로 2건 일치한다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건·tutorial4 2건·tutorial5 2건·tutorial6 2건·bc1menu76 6건·bc2bridging76 2건에 2건을 더해 47건 전수 일치가 됐다.
- 소유자 귀속: 소유자 불일치 2,168건 중 1,780건은 지형 97번(cpp 소유자 1·2 vs dn 0, cpp 농지 121번 191셀은 dn에 없음)이라 적재 중 postPop 소유자 스탬프(0.469)와 농지 배치 파이프라인(0.455) 잔여로 귀속하고, 교량-82·157번 등 공격 개시 산물(82번 1·2↔0·1·2 교차 156건·157번 178건)과 cpp_only 212건은 게임플레이 진화(0.469) 잔여로 둔다. AI 이름 Jailer 양쪽 일치·이웃 SID 타입·순서 일치는 번호 이동 없음을 뒷받침하며, 코드 변경 없이 문서화만 한다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 24/34).

#### 0.496 — 24번째 경로 bc2bridging76 기준선·덤프와 담긴 섬 2건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백다섯 번째 단위. M1 item 8의 다음 경로(Priest Training 2번째 경로 bc2bridging76 Bridging, 순서 근거 `offical2.english` — Priest Training 9개 중 두 번째)의 cpp 기준선·dn 덤프를 만들고 24경로 구조를 대조하며, 3섬·2신전·2참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.495와 같은 절차(cpp 메뉴 경로 `--mission bc2bridging76` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission bc2bridging76 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 없음 — `git diff 11eb7762a..HEAD -- cpppj/src` 0건). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 열한 번째 경로다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 683,543바이트) + dn 덤프 생성(종료 코드 0, 476,296바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 4,184·cpp 4,232·공통 4,184·dn_only 0·cpp_only 48(소규모 SID 게임플레이 잔여)·타입 3,807·소유자 2,671·프레임 1,017·전체 149. Game·CRT 행은 시드가 달라 구조만 대조(cpp 162635140/1451214786 vs dn 3026294607/296677442, CRT parity dn 측은 296677442 유지). totals cpp 11500 vs dn 0·totalmade cpp 8종 vs dn 0(R6 postPop 집계 잔여). 그래프 250/255 일치, row0 surfaces가 cpp 0 vs dn 0으로 처음 일치했고 row254 sentinel(32253)도 일치한다(나머지 5행 R6 per-Pop 번호 잔여). player 이름은 You·Stray 양쪽 일치, +0x78은 dn 전부 127 유지·cpp p1=1·p2=2·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 4,184개 집합·바이트 동일, frame 행만 90 vs 91 1줄 차이). paired 산출물은 `extracted/dotnet-l2-baseline/bc2bridging76/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 3,789셀(x,y slot +14/+18 float 비트, cpp 3,428+농지 121번 361=3,789 일치)의 군집(4방향 연결) bounds 3건 — 중앙섬 (51,115)-(175,175) 2,121셀(무신전)·북섬 (96,208)-(143,236) 904셀·남섬 (113,16)-(141,46) 764셀에 cpp 신전 좌표(object x,y 비트→float) 대조 — owner1 (125,218)→북섬 ✓(p1=1)·owner2 (133,41)→남섬 ✓(p2=2)로 2건 일치한다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건·tutorial4 2건·tutorial5 2건·tutorial6 2건·bc1menu76 6건에 2건을 더해 45건 전수 일치가 됐다.
- 소유자·이름 귀속: 소유자 불일치 1,513건 중 1,509건은 지형 97번(cpp 소유자 0·1·2 vs dn 0, cpp 농지 121번 361셀은 dn에 없음)이라 적재 중 postPop 소유자 스탬프(0.469)와 농지 배치 파이프라인(0.455) 잔여로 귀속하고, cpp_only 48건·교량·요새 객체 4건은 게임플레이 진화(0.469) 잔여로 둔다. AI 이름 Stray 양쪽 일치·이웃 SID 타입·순서 일치는 번호 이동 없음을 뒷받침하며, 코드 변경 없이 문서화만 한다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 23/34).

#### 0.495 — 23번째 경로 bc1menu76 기준선·덤프와 담긴 섬 6건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백네 번째 단위. M1 item 8의 다음 경로(Priest Training 첫 경로 bc1menu76 Menu, 순서 근거 `offical2.english` — Priest Training 9개 중 첫 번째)의 cpp 기준선·dn 덤프를 만들고 23경로 구조를 대조하며, 단일섬·7신전·7AI 이름 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.494와 같은 절차(cpp 메뉴 경로 `--mission bc1menu76` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission bc1menu76 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 없음 — `git diff 539f52032..HEAD -- cpppj/src` 0건). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 열 번째 경로다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 157,307바이트) + dn 덤프 생성(종료 코드 0, 100,530바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 829·cpp 952·공통 829·dn_only 0·cpp_only 123(소규모 SID 게임플레이 잔여)·타입 677·소유자 193·프레임 55·전체 30. Game·CRT 행은 시드가 달라 구조만 대조(cpp 3328468140/1451214786 vs dn 3632853768/296677442, CRT parity dn 측은 296677442 유지). totals cpp 11650 vs dn 0·totalmade cpp 24종 vs dn 0(R6 postPop 집계 잔여). 그래프 253/255 일치, row0 surfaces만 cpp -7 vs dn 0이고 InUse·Reserved 1과 row254 sentinel(32253)은 일치한다(R6 per-Pop 번호 잔여). player 이름은 You·CrossBows·Bridges·Acid Bars·Walkers·Sun Barricade·Man o'Wars 양쪽 일치, +0x78은 dn 전부 127 유지·cpp p1=0·p2=0·p3=127·p4=0·p5=0·p6=0·p7=0·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 829개 집합·바이트 동일, frame 행만 90 vs 91 1줄 차이). paired 산출물은 `extracted/dotnet-l2-baseline/bc1menu76/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 682셀(x,y slot +14/+18 float 비트, cpp·dn 셀 수·범위 일치)의 bounds((115,112)-(143,141), 단일 군집) 내 cpp 신전 좌표(object x,y 비트→float) 대조 — owner1 (127,130)→p1=0 ✓, owner2 (120,116)→p2=0 ✓, owner4 (136,118)→p4=0 ✓, owner5 (136,136)→p5=0 ✓, owner6 (121,128)→p6=0 ✓, owner7 (126,118)→p7=0 ✓로 6건 일치한다(단일섬이라 배정 섬 번호는 전부 0). 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건·tutorial4 2건·tutorial5 2건·tutorial6 2건에 6건을 더해 43건 전수 일치가 됐다.
- 지형 밖 신전 귀속: owner3 (124,143)은 cpp·dn 양쪽 생성 지형(최대 y=141) 밖에 있으며, 해당 소유자 Bridges p3가 cpp +0x78 127로 남는다(무신전 소유자 127 잔류인 surrounded·vicious 사례와 같은 담긴 섬 모델의 결과). 단일섬 7신전 중 유일한 미배정이라 모델 확인 강도가 높다. dn 미배정 전체 127·이름 일치·이웃 SID 타입·순서 일치는 번호 이동 없음을 뒷받침하며, 코드 변경 없이 문서화만 한다.
- 소유자 귀속: 소유자 불일치 636건 중 615건은 지형 97번(cpp 소유자 1 vs dn 0, cpp 농지 121번 67셀은 dn에 없음)이라 적재 중 postPop 소유자 스탬프(0.469)와 농지 배치 파이프라인(0.455) 잔여로 귀속하고, dn 요새 원문 자리 85건(타입 1000대·x,y 0,0)과 cpp_only 123건·교량·요새 객체 10건은 Pop 공간 등록 전(0.467)·게임플레이 진화(0.469) 잔여로 둔다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 22/34).

#### 0.494 — 22번째 경로 tutorial6 기준선·덤프와 담긴 섬 2건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백세 번째 단위. M1 item 8의 다음 경로(Early 6 tutorial6 Raw Power, 순서 근거 `offical1.english` — Early Missions 6개 중 마지막)의 cpp 기준선·dn 덤프를 만들고 22경로 구조를 대조하며, 2섬·2신전·2참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.493과 같은 절차(cpp 메뉴 경로 `--mission Tutorial6` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission Tutorial6 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 없음 — `git diff b8e61291a..HEAD -- cpppj/src` 0건). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 아홉 번째 경로(dissolvedalliance·enemyterritory·finalconfrontation·tutorial1~tutorial5에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 384,398바이트) + dn 덤프 생성(종료 코드 0, 243,112바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 2,102·cpp 2,366·공통 2,102·dn_only 0·cpp_only 264(소규모 SID 게임플레이 잔여 — 170번 165개·90번 51개 포함, 개시 공격 중이라 규모가 큼)·타입 1,868·소유자 390·프레임 92·전체 75. Game·CRT 행은 시드가 달라 구조만 대조(cpp 859686108/1451214786 vs dn 3993625835/296677442, CRT parity dn 측은 296677442 유지). totals cpp 23900 vs dn 0·totalmade cpp 17종 vs dn 0(R6 postPop 집계 잔여). 그래프 248/255 일치, row0 surfaces만 cpp -4 vs dn 0이고 InUse·Reserved 1과 row254 sentinel(32253)은 일치한다(R6 per-Pop 번호 잔여). player 이름은 p1 You 양쪽 일치, +0x78은 dn 전부 127 유지·cpp p1=0·p2=1·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 2,102개 집합·바이트 동일, frame 행만 90 vs 91 1줄 차이). paired 산출물은 `extracted/dotnet-l2-baseline/tutorial6/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 1,877셀의 x,y(slot +14/+18 float 비트) 군집(4방향 연결) bounds 2건 — (96,49)-(126,79) 759셀·(128,82)-(175,127) 1,118셀에 cpp 신전 좌표(object x,y 비트→float) 대조 — owner1 (122,61)→첫째 군집 ✓, owner2 (170,111)→둘째 군집 ✓로 2건 일치한다. 참가자 대응은 첫째→p1·둘째→p2이며 2섬·2신전·2참가자 구조다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건·tutorial4 2건·tutorial5 2건에 2건을 더해 37건 전수 일치가 됐다.
- 소유자·이름 귀속: 소유자 불일치 1,712건 중 1,702건은 지형 97번(cpp 소유자 1·2 vs dn 0, cpp 농지 121번 175셀은 dn에 없음)이라 적재 중 postPop 소유자 스탬프(0.469)와 농지 배치 파이프라인(0.455) 잔여로 귀속하고, cpp_only 264건(소규모 SID·170번·90번 등 개시 공격 산물)과 교량·요새 객체 10건은 게임플레이 진화(0.469) 잔여로 둔다. cpp p2 이름 Evil Duke·dn p2 이름 없음 차이는 +0x78 미배정과 같은 소용돌이 사슬 잔여의 결과(단일 미션 기록자 미연결, R6)로 두고 코드 변경 없이 문서화만 한다. 이웃 SID 타입·순서가 양쪽 일치해 번호 이동은 없다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 21/34).

#### 0.493 — 21번째 경로 tutorial5 기준선·덤프와 담긴 섬 2건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백두 번째 단위. M1 item 8의 다음 경로(Early 5 tutorial5 Subtle Defense, 순서 근거 `offical1.english` — Early Missions 6개 중 다섯 번째)의 cpp 기준선·dn 덤프를 만들고 21경로 구조를 대조하며, 2섬·2신전·2참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.492와 같은 절차(cpp 메뉴 경로 `--mission Tutorial5` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission Tutorial5 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 없음 — `git diff 59566dbd5..HEAD -- cpppj/src` 0건). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 여덟 번째 경로(dissolvedalliance·enemyterritory·finalconfrontation·tutorial1~tutorial4에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 446,003바이트) + dn 덤프 생성(종료 코드 0, 309,192바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 2,692·cpp 2,745·공통 2,692·dn_only 0·cpp_only 53(소규모 SID 8~24 게임플레이 잔여)·타입 2,403·소유자 445·프레임 83·전체 66. Game·CRT 행은 시드가 달라 구조만 대조(cpp 718062427/1451214786 vs dn 3340740519/296677442, CRT parity dn 측은 296677442 유지). totals cpp 22000 vs dn 0·totalmade cpp 16종 vs dn 0(R6 postPop 집계 잔여). 그래프 248/255 일치, row0 surfaces만 cpp -4 vs dn 0이고 InUse·Reserved 1과 row254 sentinel(32253)은 일치한다(R6 per-Pop 번호 잔여). player 이름은 p1 You 양쪽 일치, +0x78은 dn 전부 127 유지·cpp p1=0·p2=1·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 2,692개 집합·바이트·행 전체 동일). paired 산출물은 `extracted/dotnet-l2-baseline/tutorial5/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 2,472셀의 x,y(slot +14/+18 float 비트) 군집(4방향 연결) bounds 2건 — (115,64)-(142,94) 678셀·(112,113)-(159,159) 1,794셀에 cpp 신전 좌표(object x,y 비트→float) 대조 — owner1 (127,83)→첫째 군집 ✓, owner2 (146,139)→둘째 군집 ✓로 2건 일치한다. 참가자 대응은 첫째→p1·둘째→p2이며 2섬·2신전·2참가자 구조다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건·tutorial4 2건에 2건을 더해 35건 전수 일치가 됐다.
- 소유자·이름 귀속: 소유자 불일치 2,247건 중 2,241건은 지형 97번(cpp 소유자 1·2 vs dn 0, cpp 농지 121번 231셀은 dn에 없음)이라 적재 중 postPop 소유자 스탬프(0.469)와 농지 배치 파이프라인(0.455) 잔여로 귀속하고, 나머지 6건(교량·요새 객체 71·78·89·112·158·2080·2082)은 게임플레이 진화(0.469) 잔여로 둔다. cpp p2 이름 Evil Duke·dn p2 이름 없음 차이는 +0x78 미배정과 같은 소용돌이 사슬 잔여의 결과(단일 미션 기록자 미연결, R6)로 두고 코드 변경 없이 문서화만 한다. 이웃 SID 타입·순서가 양쪽 일치해 번호 이동은 없다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 20/34).

#### 0.492 — 20번째 경로 tutorial4 기준선·덤프와 담긴 섬 2건 확인 (2026-11-13, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백한 번째 단위. M1 item 8의 다음 경로(Early 4 tutorial4 Tactical Combat, 순서 근거 `offical1.english` — Early Missions 6개 중 네 번째)의 cpp 기준선·dn 덤프를 만들고 20경로 구조를 대조하며, 2섬·2신전·2참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.491과 같은 절차(cpp 메뉴 경로 `--mission Tutorial4` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission Tutorial4 --handle Youpson` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경은 webpj만이라 미션 경로에 영향 없음). 실행 전후 `originals/` 1,394파일 개수 대조 차이 0. `No Central Island` 안내가 뜨는 일곱 번째 경로(dissolvedalliance·enemyterritory·finalconfrontation·tutorial1·tutorial2·tutorial3에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 332,246바이트) + dn 덤프 생성(종료 코드 0, 230,568바이트). 게임 실행 없음(빌드·검사·덤프 실행만, AGENTS.md 예외 호스트 HJOW-Athlon). 빌드는 경고 0·오류 0. dn 1,990·cpp 2,032·공통 1,989·dn_only 1(SID 15022 type 1181 owner 0 — 요새 원문 자리·R6 잔여)·cpp_only 43(소규모 SID 8~24 게임플레이 잔여)·타입 1,764·소유자 1,262·프레임 380·전체 55. Game·CRT 행은 시드가 달라 구조만 대조(cpp 1398387083/1451214786 vs dn 3557236463/296677442, CRT parity dn 측은 296677442 유지). totals cpp 18400 vs dn 0·totalmade cpp 13종 vs dn 0(R6 postPop 집계 잔여). 그래프 247/255 일치, row0 surfaces만 cpp -5 vs dn 0이고 InUse·Reserved 1과 row254 sentinel(32253)은 일치한다(R6 per-Pop 번호 잔여). player 이름 일치(You — p1 참가·p0·p2~p8 비참가자 표시, dn p1도 You), +0x78은 dn 전부 127 유지·cpp p1=0·p2=1·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 1,990개 집합·바이트 동일, frame 행만 90 vs 91 1줄 차이). paired 산출물은 `extracted/dotnet-l2-baseline/tutorial4/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 지형(type 97) 1,778셀의 x,y(slot +14/+18 float 비트) 군집(4방향 연결) bounds 2건 — (113,64)-(143,94) 786셀·(96,128)-(143,159) 992셀에 cpp 신전 좌표(object x,y 비트→float) 대조 — owner1 (125,75)→첫째 군집 ✓, owner2 (131,154)→둘째 군집 ✓로 2건 일치한다. 참가자 대응은 첫째→p1·둘째→p2이며 2섬·2신전·2참가자 구조다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건·tutorial3 2건에 2건을 더해 33건 전수 일치가 됐다.
- dn_only 1건 귀속: dn SID 15022(type 1181 owner 0, x,y 0,0)는 요새 적재 live 연결(0.467) 상태에서 Pop 공간 등록 전이라 좌표·공장 타입이 붙지 않은 원문 자리이며, cpp에는 90프레임 게임플레이에서 제거·전환되어 없다. 이웃 SID(15021·15023) 타입·순서가 양쪽 일치해 번호 이동이 없고, dn 재실행에서 15022가 재현된다. cpp_only 43건(소규모 SID 게임플레이 생성)과 같은 R6 소관으로 두고 코드 변경 없이 문서화만 한다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 19/34).

#### 0.491 — 19번째 경로 tutorial3 기준선·덤프와 담긴 섬 2건 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 백 번째 단위. M1 item 8의 다음 경로(Early 3 tutorial3 Capture The Priest, 순서 근거 `offical1.english` — Early Missions 6개 중 세 번째)의 cpp 기준선·dn 덤프를 만들고 19경로 구조를 대조하며, 2섬·2신전·2참가자 구조를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.490과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0. `No Central Island` 안내가 뜨는 여섯 번째 경로(dissolvedalliance·enemyterritory·finalconfrontation·tutorial1·tutorial2에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 308,460바이트) + dn 덤프 생성(종료 코드 0, 215,448바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 1,855·cpp 1,882·공통 1,854·dn_only 1(SID 15006 type 123 owner 2)·cpp_only 28(소규모 SID 8~24 게임플레이 잔여)·타입 1,700·소유자 1,135·프레임 366·전체 63. Game·CRT 행은 시드가 달라 구조만 대조(cpp 3114386979/1451214786 vs dn 2448719692/296677442). totals cpp 11800 vs dn 0·totalmade cpp 11 vs dn 0(R6 postPop 집계 잔여). 그래프 249/255 일치, row0 surfaces만 cpp -3 vs dn 0이고 InUse·Reserved 1과 row254 sentinel은 일치한다(R6 per-Pop 번호 잔여). player 이름 일치(You — p1·p2 참가·p0·p3~p8 비참가자), +0x78은 dn 전부 127 유지·cpp p1=0·p2=1·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). dn 재실행 결정성 확인(슬롯 집합·바이트 동일, 15006 양쪽 존재, frame 행만 90 vs 91 1줄 차이). paired 산출물은 `extracted/dotnet-l2-baseline/tutorial3/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 섬 bounds(PlaceAll 직후, FillWorldFrames 적용, 0:(112,80)-(144,112), 1:(96,128)-(144,160), 나머지 2~19 비어 있음)에 cpp 신전 좌표 대조 — owner1 (132,93)→섬 0 ✓, owner0 (117,139)→섬 1 ✓로 2건 일치한다. 참가자 대응은 섬 0→p1·섬 1→p2이며 2섬·2신전·2참가자 구조다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건·tutorial2 1건에 2건을 더해 31건 전수 일치가 됐다.
- dn_only 1건 귀속: 요새 원문(CaptureThePriest)에 123번이 2개(terr7 owner1·terr16 owner2) 있어 dn이 둘 다 적재(sid 15000 owner1·15006 owner2)한 것은 원문대로이며, cpp f090에 owner1 1개만 남은 것은 90프레임 게임플레이에서 owner2 123이 제거·전환된 R6 잔여다. 이웃 SID(15005·15007~15010) 타입·순서가 양쪽 일치해 번호 이동이 없고, dn 재실행에서 15006이 재현된다. cpp_only 28건(소규모 SID 게임플레이 생성)과 같은 R6 소관으로 두고 코드 변경 없이 문서화만 한다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 18/34).

#### 0.490 — 18번째 경로 tutorial2 기준선·덤프와 담긴 섬 1건 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔아홉 번째 단위. M1 item 8의 다음 경로(Early 2 tutorial2 Secret Workshop, 순서 근거 `offical1.english` — Early Missions 6개 중 두 번째)의 cpp 기준선·dn 덤프를 만들고 18경로 구조를 대조하며, 중립 신전 경로를 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.489와 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0. `No Central Island` 안내가 뜨는 다섯 번째 경로(dissolvedalliance·enemyterritory·finalconfrontation·tutorial1에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 141,550바이트) + dn 덤프 생성(종료 코드 0, 100,655바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 830·cpp 838·공통 830·dn_only 0·cpp_only 8·타입 761·소유자 829·프레임 280·전체 27. Game·CRT 행은 시드가 달라 구조만 대조(cpp 2566394488/1451214786 vs dn 2926268868/296677442). totals 양쪽 0·totalmade cpp 5 vs dn 0(R6 postPop 집계 잔여). 그래프 254/255 일치, row0 InUse·Reserved 1과 row254 sentinel이 모두 일치한다(R6 per-Pop 번호 잔여 1행). player 이름 일치(You — p1 참가·p0·p2~p8 비참가자), +0x78은 dn·cpp 전부 127(0.477 정정 상태, 중립 신전이라 소용돌이 사슬 배정 없음·R6). paired 산출물은 `extracted/dotnet-l2-baseline/tutorial2/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 섬 bounds(PlaceAll 직후, FillWorldFrames 적용, 0:(32,32)-(64,64), 나머지 1~19 비어 있음)에 cpp 신전 좌표 대조 — owner0 (60,40)→섬 0 ✓로 1건 일치한다. 중립 신전이라 참가자 배정이 없고 전원 +0x78 127이 유지된다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건·tutorial1 1건에 1건을 더해 29건 전수 일치가 됐다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 17/34).

#### 0.489 — 17번째 경로 tutorial1 기준선·덤프와 담긴 섬 1건 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔여덟 번째 단위. M1 item 8의 다음 경로(Early 1 tutorial1 Bridge the Gap, 순서 근거 `offical1.english` — Early Missions 6개 중 첫 번째)의 cpp 기준선·dn 덤프를 만들고 17경로 구조를 대조하며, Early Missions 진입을 담긴 섬 모델로 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.488과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0. `No Central Island` 안내가 뜨는 네 번째 경로(dissolvedalliance·enemyterritory·finalconfrontation에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 142,238바이트) + dn 덤프 생성(종료 코드 0, 100,866바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 832·cpp 843·공통 832·dn_only 0·cpp_only 11·타입 762·소유자 138·프레임 31·전체 27. Game·CRT 행은 시드가 달라 구조만 대조(cpp 2627616803/1451214786 vs dn 2926268868/296677442). totals cpp 5000 vs dn 0·totalmade 6 vs 0(R6 postPop 집계 잔여). 그래프 254/255 일치, row0 InUse·Reserved 1과 row254 sentinel이 모두 일치한다(R6 per-Pop 번호 잔여 1행). player 이름 일치(You — 2인 미션 p1 참가·p0·p2~p8 비참가자), +0x78은 dn 전부 127 유지·cpp p1=0·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). paired 산출물은 `extracted/dotnet-l2-baseline/tutorial1/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 섬 bounds(PlaceAll 직후, FillWorldFrames 적용, 0:(32,32)-(64,64), 나머지 1~19 비어 있음)에 cpp 신전 좌표 대조 — owner1 (60,40)→섬 0 ✓로 1건 일치한다. 참가자 대응은 섬 0→p1이며 단일 섬·단일 신전·단일 참가자 구조다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건·finalconfrontation 5건에 1건을 더해 28건 전수 일치가 됐다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 16/34).

#### 0.488 — 16번째 경로 finalconfrontation 기준선·덤프와 담긴 섬 5건 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔일곱 번째 단위. M1 item 8의 다음 경로(캠페인 3-5 finalconfrontation, 순서 근거 `offical5.english` — Complete Victory 5개 중 마지막)의 cpp 기준선·dn 덤프를 만들고 16경로 구조를 대조하며, 3장 5개를 담긴 섬 모델로 마감한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.487과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0. `No Central Island` 안내가 뜨는 세 번째 경로(dissolvedalliance·enemyterritory에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 840,984바이트) + dn 덤프 생성(종료 코드 0, 555,360바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 4,890·cpp 5,239·공통 4,890·dn_only 0·cpp_only 349·타입 4,104·소유자 1,695·프레임 511·전체 150. Game·CRT 행은 시드가 달라 구조만 대조(cpp 2291589179/1451214786 vs dn 678793219/296677442). totals cpp 131100 vs dn 0·totalmade 31 vs 0(R6 postPop 집계 잔여). 그래프 218/255 일치, row0 surfaces만 cpp -65 vs dn 0이고 InUse·Reserved 1과 row254 sentinel은 일치한다(R6 per-Pop 번호 잔여). player 이름 일치(You+Lord of Thunder+Lord of Rain+Lord of Wind+Storm Lord — 첫 6인 미션, p1~p5 참가·p0·p6~p8 비참가자), +0x78은 dn 전부 127 유지·cpp p1=0·p2=1·p3=2·p4=3·p5=4(0.477 정정 상태, 소용돌이 사슬 R6). paired 산출물은 `extracted/dotnet-l2-baseline/finalconfrontation/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 섬 bounds(PlaceAll 직후, FillWorldFrames 적용, 0:(64,80)-(96,112), 1:(96,48)-(128,80), 2:(96,128)-(144,160), 3:(144,176)-(192,224), 4:(192,192)-(224,224))에 cpp 신전 좌표 대조 — owner1 (83,100)→섬 0 ✓, owner2 (122,65)→섬 1 ✓, owner3 (119,133)→섬 2 ✓, owner4 (189,202)→섬 3 ✓, owner0 (219,198)→섬 4 ✓로 5건 전수 일치한다. 참가자별 대응은 섬 0→p1·섬 1→p2·섬 2→p3·섬 3→p4·섬 4→p5이며 중립 신전 섬 4가 p5(Storm Lord)에 배정된다. 0.484의 10건·totherescue 4건·vicious 2건·enemyterritory 6건에 5건을 더해 27건 전수 일치가 됐다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 15/34).

#### 0.487 — 15번째 경로 enemyterritory 기준선·덤프와 담긴 섬 6건 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔여섯 번째 단위. M1 item 8의 다음 경로(캠페인 3-4 enemyterritory, 순서 근거 `offical5.english` — Complete Victory 5개 중 4번째)의 cpp 기준선·dn 덤프를 만들고 15경로 구조를 대조하며, 담긴 섬 모델을 6섬 체제로 확장 확인한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.486과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0. `No Central Island` 안내가 뜨는 두 번째 경로(dissolvedalliance에 이어)다.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 1,016,054바이트) + dn 덤프 생성(종료 코드 0, 698,851바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 6,171·cpp 6,320·공통 6,171·dn_only 0·cpp_only 149·타입 5,697·소유자 2,517·프레임 1,136·전체 165. Game·CRT 행은 시드가 달라 구조만 대조(cpp 1199769280/1451214786 vs dn 2136825127/296677442). totals cpp 52550 vs dn 0·totalmade 22 vs 0(R6 postPop 집계 잔여). 그래프 239/255 일치, row0 surfaces만 cpp -73 vs dn 0이고 InUse·Reserved 1과 row254 sentinel은 일치한다(R6 per-Pop 번호 잔여). player 이름 일치(Youpson 계열 5인 미션: p1 Youpson·p2 The Traitor·p3 Duke of Wind·p4 Duke of Rain, p0·p5~p8 비참가자), +0x78은 dn 전부 127 유지·cpp p1=4·p2=5·p3=1·p4=0·나머지 127(0.477 정정 상태, 소용돌이 사슬 R6). paired 산출물은 `extracted/dotnet-l2-baseline/enemyterritory/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 확인: dn 섬 bounds(PlaceAll 직후, FillWorldFrames 적용, 0:(128,112)-(176,160), 1:(64,112)-(112,160), 2:(16,128)-(64,144), 3:(176,128)-(224,144), 4:(112,192)-(144,224), 5:(128,32)-(160,64))에 cpp 신전 좌표 대조 — owner4 (171,145)→섬 0 ✓, owner3 (104,124)→섬 1 ✓, owner0 (23,139)→섬 2 ✓, owner0 (183,139)→섬 3 ✓, owner1 (133,216)→섬 4 ✓, owner2 (135,40)→섬 5 ✓로 6건 전수 일치한다. 참가자별 대응은 섬 4→p1·섬 5→p2·섬 1→p3·섬 0→p4이며 중립 신전 섬 2·3은 어느 참가자에도 배정되지 않는다. 요새 파일의 89번 6개 소유자 [4,3,1,1,1,0]와 dn 적재 소유자 [4,3,1,1,1,1](마지막 0→정규화 1)·cpp 게임플레이 소유자 [4,3,0,0,1,2]의 차이는 적재 owner식→postPop 스탬프→게임플레이 진화 분리(0.469 귀속)와 같은 결이며, 담긴 섬 판정에는 world 좌표·bounds를 쓴다. 0.484의 10건·totherescue 4건·vicious 2건에 6건을 더해 22건 전수 일치가 됐다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다(현재 webpj 미커밋 없음).
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 14/34).

#### 0.486 — 14번째 경로 vicious 기준선·덤프와 담긴 섬 2건 추가 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔다섯 번째 단위. M1 item 8의 다음 경로(캠페인 3-3 vicious, 순서 근거 tarc `offical5.english` — 1장 6개·2장 5개·3장 6개로 캠페인 17개가 아니라 3장 합계 확인 필요)와 담긴 섬 2건 추가 확인을 한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.485와 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 610,860바이트) + dn 덤프 생성(종료 코드 0, 390,187바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 2,052·cpp 2,156·공통 2,052·dn_only 0·cpp_only 104·타입 1,900·소유자 1,157·프레임 342·전체 64. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 34(R6 per-Pop 번호 잔여). player 이름 일치(You+Duke of the Furies — 두 번째 2인 미션), +0x78은 dn 127 유지·cpp p1=127·p2=1(0.477 정정 상태, 소용돌이 사슬 R6). 섬이 ISLAND 1개에 신전 1개(소유자 2)만 있고 소유자 1의 신전은 섬 밖에 있어 p1이 127로 남는다 — 무신전 잔류의 두 번째 사례(surrounded에 이어). paired 산출물은 `extracted/dotnet-l2-baseline/vicious/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. temp 탐침·스크립트는 삭제했다.
- 담긴 섬 추가 확인: vicious 섬 bounds(PlaceAll 직후, 0:(96,128)-(128,160), 1:(128,96)-(176,128))에 cpp 신전 좌표 대조 — owner2 (162,128)→섬 1 ✓, owner1 (89,125)→어느 섬에도 속하지 않음(가장 가까운 섬 0의 L=96 밖) → 미배정 ✓. 0.484의 10건과 totherescue 4건에 2건을 더해 16건 전수 일치(5경로: savetheisland·thewarbegins·surrounded·breakingthrough·totherescue·vicious... — 정확히는 savetheisland 2·thewarbegins 2·surrounded 4·breakingthrough 2·vicious 2 = 12건 bounds 확정 + totherescue 4건 파일 순서 일치 = 16건).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 13/34).

#### 0.485 — 13번째 경로 totherescue 기준선·덤프와 담긴 섬 4건 추가 확인 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔한 번째 단위. M1 item 8의 다음 경로(캠페인 3-2 totherescue, 순서 근거 tarc `offical5.english`)의 cpp 기준선·dn 덤프를 만들고 13경로 구조를 대조하며, 0.484 판정 실험을 totherescue 섬 bounds로 확장한다. 제품 코드 변경 없음(측정·조사 단위).
- 근거·방법: 0.478~0.484와 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 711,860바이트) + dn 덤프 생성(종료 코드 0, 471,267바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 4,139·cpp 4,420·공통 4,139·dn_only 0·cpp_only 281·타입 3,518·소유자 979·프레임 245·전체 127. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 34(R6 per-Pop 번호 잔여). player 이름 일치(You+Governor Cherok+Prince of Thunder+Duke of Thunder — 두 번째 4인 미션), +0x78은 dn 127 유지·cpp p1=0·p2=3·p3=2·p4=1(0.477 정정 상태, 소용돌이 사슬 R6). 신전 가상표 4종이 소유자 1·4·3·2당 1개씩 있고 cpp 좌표는 (101,104)·(157,109)·(126,155)·(180,164), dn은 Pop 미연결로 (0,0)이다. paired 산출물은 `extracted/dotnet-l2-baseline/totherescue/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 담긴 섬 추가 확인: totherescue 섬 bounds(PlaceAll 직후, 0:(80,80)-(112,112), 1:(128,80)-(176,128), 2:(96,128)-(128,160), 3:(144,144)-(192,176))에 cpp 신전 좌표를 대조 — owner1 (101,104)→섬 0, owner4 (157,109)→섬 1, owner3 (126,155)→섬 2, owner2 (180,164)→섬 3으로 4건 전부 cpp +0x78과 일치한다(0.484 10건과 합쳐 14건 전수 일치). 파일 순서 [1,4,3,2]는 섬 번호 [0,1,2,3]과 우연히 일치하므로 판정력이 없으며, surrounded 역순 사례와 합쳐 담긴 섬 모델이 확정된다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침·스크립트는 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 12/34).

#### 0.484 — 담긴 섬 판정 실험과 12번째 경로 breakingthrough 기준선·덤프 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔한 번째 단위. +0x78 기록 메커니즘의 판정 실험(신전 좌표·섬 bounds 대조)과 다음 경로(캠페인 3-1 breakingthrough, 순서 근거 tarc `offical5.english` — 1장 6개·2장 5개·3장 5개로 캠페인 16개)의 cpp 기준선·dn 덤프를 만든다. 제품 코드 변경 없음(조사·측정 단위).
- 근거·판정: dn 섬 런타임 bounds(PlaceAll 뒤, Generate 불필요)와 cpp 덤프 신전 좌표를 대조했다 — savetheisland owner2 (107,114)→섬 0·owner1 (187,108)→섬 1, thewarbegins owner1 (89,116)→섬 0·owner2 (156,124)→섬 1, surrounded owner3 (188,136)→섬 0·owner4 (137,189)→섬 2·owner5 (79,131)→섬 3·owner2 (141,74)→섬 4 — 10건 전부 cpp +0x78과 일치한다. 파일 순서 가설은 surrounded에서 깨진다(파일 순서 [3,4,5,2]→[0,1,2,3]이 아니라 실제 [0,2,3,4]). 담긴 섬 모델이 확정됐다: 소용돌이→islands.Owner→FirstRegion(미배정 때만). R6 연결 계약도 확정됐다: Pop 후처리에서 신전 Pop 좌표의 RegionAt→islands.Owner→FirstRegion(Player 기록 바인딩은 0.477 완료).
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 540,915바이트) + dn 덤프 생성(종료 코드 0, 363,512바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 3,177·cpp 3,340·공통 3,177·dn_only 0·cpp_only 163·타입 2,767·소유자 708·프레임 128·전체 67. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 35(R6 per-Pop 번호 잔여). player 이름 일치(You+Prince of Wind), +0x78은 dn 127 유지·cpp p1=1·p2=0(0.477 정정 상태, 네 번째 역순 경로). 신전 가상표 4종이 소유자 2·1당 1개씩 있고 cpp 좌표는 (184,128)·(90,125), dn은 Pop 미연결로 (0,0)이다. paired 산출물은 `extracted/dotnet-l2-baseline/breakingthrough/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만, 임시 탐침은 삭제했다). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→RegionAt→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 12/34).

#### 0.483 — 11번째 경로 surrounded 기준선·덤프·구조 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔한 번째 단위. M1 item 8의 다음 경로(캠페인 2-5 surrounded, 순서 근거 tarc `offical4.english` — 1장 6개·2장 5개·3장 5개로 캠페인 16개)의 cpp 기준선·dn 덤프를 만들고 11경로 구조를 대조한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.482와 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 1,003,359바이트 — 가장 큰 경로) + dn 덤프 생성(종료 코드 0, 706,364바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 6,238·cpp 6,420·공통 6,238·dn_only 0·cpp_only 182·타입 5,734·소유자 2,046·프레임 581·전체 210. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 42(R6 per-Pop 번호 잔여). player 이름 일치(You+Lord of Sun+Lord of Thunder+Lord of Wind+Lord of Rain — 첫 6인 미션), +0x78은 dn 127 유지·cpp p1=127·p2=4·p3=0·p4=2·p5=3(0.477 정정 상태, 소용돌이 사슬 R6). 신전 가상표 4종이 소유자 2·3·4·5당 1개씩 있고 cpp 좌표는 (171,99)·(188,136)·(137,62)·(79,131), dn은 Pop 미연결로 (0,0)이다. 소유자 1의 신전이 없어 p1이 127로 남는다 — 무신전 소유자는 기록되지 않는다는 소용돌이 모델의 예측과 일치한다. paired 산출물은 `extracted/dotnet-l2-baseline/surrounded/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 파일 순서 가설의 판정: surrounded 파일 순서 [3,4,5,2]를 그대로 섬 번호 [0,1,2,3]에 대응시키면 cpp 배정(p1=127·p2=4·p3=0·p4=2·p5=3)과 어긋난다. 담긴 섬 모델(소유자→섬: {3:0, 4:2, 5:3, 2:4}, 소유자 1은 신전이 없어 127 유지, 1번 섬 무신전)은 모든 값을 설명한다. 판정 실험(신전 파일 좌표·섬 bounds 대조)은 R6 과제에서 파일 좌표 판독과 함께 수행한다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6 postPop·표시와 함께 — 파일 좌표·섬 bounds 판정 포함), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 10/34).

#### 0.482 — 10번째 경로 guardmyback 기준선·덤프·구조 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 아흔 번째 단위. M1 item 8의 다음 경로(캠페인 2-4 guardmyback, 순서 근거 tarc `offical4.english`)의 cpp 기준선·dn 덤프를 만들고 10경로 구조를 대조한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.481과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 610,860바이트) + dn 덤프 생성(종료 코드 0, 390,187바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 3,415·cpp 3,792·공통 3,415·dn_only 0·cpp_only 377·타입 3,041·소유자 874·프레임 196·전체 106. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 34(R6 per-Pop 번호 잔여). player 이름 일치(You+Minister of Sun+Marquis de Thunder+Knight of Rain — 첫 5인 미션), +0x78은 dn 127 유지·cpp p1=1·p2=0·p3=2·p4=3(0.477 정정 상태, 소용돌이 사슬 R6). 신전 가상표 4종이 소유자 2·1·3·4당 1개씩 있고 cpp 좌표는 (138,109)·(141,121)·(137,62)·(136,184), dn은 Pop 미연결로 (0,0)이다. paired 산출물은 `extracted/dotnet-l2-baseline/guardmyback/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 신전 파일 순서 10경로 대조: 덤프 내 신전 SID 순서 [2,1,3,4]가 cpp 배정 [1,0,2,3]과 일치한다(네 번째 역순 경로, savetheisland·rainvsrain [2,1]→[1,0]·runforit [2,1]→[1,0]과 같은 유형). 판정 메커니즘(담긴 섬 vs 파일 순서)은 신전 파일 좌표·섬 bounds 대조(R6 과제)로 가린다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6 postPop·표시와 함께 — 파일 좌표·섬 bounds 판정 포함), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 9/34).

#### 0.481 — 9번째 경로 runforit 기준선·덤프·구조 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든아홉 번째 단위. M1 item 8의 다음 경로(캠페인 2-3 runforit, 순서 근거 tarc `offical4.english` — 1장 6개·2장 5개·3장 5개로 캠페인 16개)의 cpp 기준선·dn 덤프를 만들고 9경로 구조를 대조한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478~0.480과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 349,730바이트) + dn 덤프 생성(종료 코드 0, 239,304바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 2,068·cpp 2,144·공통 2,068·dn_only 0·cpp_only 76·타입 1,823·소유자 442·프레임 107·전체 64. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 15(R6 per-Pop 번호 잔여). player 이름 일치(You+Lord of Rain), +0x78은 dn 127 유지·cpp p1=1·p2=0(0.477 정정 상태, 소용돌이 사슬 R6 — 세 번째 역순 경로). 신전 가상표 4종이 소유자 2·1당 1개씩 있고 cpp 좌표는 (148,99)·(94,124), dn은 Pop 미연결로 (0,0)이다. paired 산출물은 `extracted/dotnet-l2-baseline/runforit/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 신전 파일 순서 9경로 대조(0.480 8경로 + runforit [2,1]→[1,0] 추가): [1,2]→[0,1] 5경로(thewarbegins·masterofwhirligigs·fragilefortune·thunderingpower·thenoose), [2,1]→[1,0] 3경로(savetheisland·rainvsrain·runforit), dissolvedalliance [1,3,2]→[0,2,1]. 판정 메커니즘(담긴 섬 vs 파일 순서)은 신전 파일 좌표·섬 bounds 대조(R6 과제)로 가린다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6 postPop·표시와 함께 — 파일 좌표·섬 bounds 판정 포함), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 8/34).

#### 0.480 — 8번째 경로 rainvsrain 기준선·덤프·신전 파일 순서 8경로 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든여덟 번째 단위. M1 item 8의 다음 경로(캠페인 2-2 rainvsrain, 순서 근거 tarc `offical4.english`)의 cpp 기준선·dn 덤프를 만들고 8경로 구조를 대조한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478·0.479와 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0.
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 329,817바이트) + dn 덤프 생성(종료 코드 0, 223,968바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 1,931·cpp 2,023·공통 1,931·dn_only 0·cpp_only 92·타입 1,747·소유자 1,028·프레임 78·전체 59. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치, dn InUse 2 대 cpp 16(R6 per-Pop 번호 잔여). player 이름 일치(You+Prince of Rain), +0x78은 dn 127 유지·cpp p1=1·p2=0(0.477 정정 상태, 소용돌이 사슬 R6). 신전 가상표 4종이 소유자 2·1당 1개씩 있고 cpp 좌표는 (171,124)·(91,124), dn은 Pop 미연결로 (0,0)이다. savetheisland에 이어 두 번째 p1=1 경로이며 신전 파일 순서는 [2,1]로 cpp 배정 [1,0]과 일치한다. paired 산출물은 `extracted/dotnet-l2-baseline/rainvsrain/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 8경로 신전 파일 순서→cpp +0x78 일치(임시 탐침 실측 후 삭제): [1,2]→[0,1] 5경로(thewarbegins·masterofwhirligigs·fragilefortune·thunderingpower·thenoose), [2,1]→[1,0] 2경로(savetheisland·rainvsrain), dissolvedalliance [1,3,2]→[0,2,1]. 판정 메커니즘(담긴 섬 vs 파일 순서)은 신전 파일 좌표·섬 bounds 대조(R6 과제)로 가린다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6 postPop·표시와 함께 — 파일 좌표·섬 bounds 판정 포함), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 7/34).

#### 0.479 — 7번째 경로 thenoose 기준선·덤프·구조 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든일곱 번째 단위. M1 item 8의 다음 경로(캠페인 2-1 thenoose, 순서 근거 tarc `offical4.english` — 1장 `offical3` 6개·2장 `offical4` 5개·3장 `offical5` 5개로 캠페인 16개 확인)의 cpp 기준선·dn 덤프를 만들고 7경로 구조를 대조한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 0.478과 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 전용). 실행 전후 `originals/` 1,394파일 해시 대조 차이 0(기준 파일 BOM은 `utf-8-sig`로 읽는다).
- 검증: cpp 기준선 생성(종료 코드 0, 덤프 456,351바이트) + dn 덤프 생성(종료 코드 0, 303,704바이트). 게임 실행 없음(빌드·검사·덤프 실행만). dn 2,643·cpp 2,814·공통 2,643·dn_only 0·cpp_only 171·타입 2,289·소유자 1,028·프레임 238·전체 62. Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치(0.476 ResetTable), dn InUse 2 대 cpp InUse(게임플레이 번호, R6 잔여). player 이름 일치(You+Chamberlain of Rain, +0x78은 dn 127 유지·cpp p1=0·p2=1 — 소용돌이 사슬 R6 잔여). 신전 가상표 4종이 소유자 1·2당 1개씩 있고 cpp 좌표는 (113,132)·(184,128), dn은 Pop 미연결로 (0,0)이다(R6 Pop 결핍과 같은 양상). paired 산출물은 `extracted/dotnet-l2-baseline/thenoose/`에 두었다(Git 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 6/34).

#### 0.478 — 6번째 경로 dissolvedalliance 기준선·덤프·구조 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든여섯 번째 단위. M1 item 8의 나머지 경로 중 1-4 dissolvedalliance의 cpp 기준선·dn 덤프를 만들고 6경로 구조를 대조한다. 제품 코드 변경 없음(측정 단위).
- 근거·방법: 기존 5경로와 같은 절차(cpp 메뉴 경로 `--mission` + click-if Play Mission → 90프레임 `session-dump`, dn 직접 진입 `--mission` + 90프레임 덤프, 둘 다 `--fixed-frame-ms 25`, dn만 `--fixed-unix-seconds 1760000000`). cpp 오라클은 11-12 12:24 빌드 그대로 쓴다(그 뒤 cpppj 변경 1302·1303은 CD 10.72 전용이라 미션 경로에 영향 없음, [1.2](#resync-scope)). 실행 전후 `originals/` 1,394파일 해시를 대조한다(기준 파일의 BOM 한 줄을 `utf-8-sig`로 읽어야 첫 줄이 일치한다).
- 검증: cpp 기준선 생성(종료 코드 0, `No Central Island`·빈 사용자 폴더의 Cache 미열기 안내のみ·덤프 560,070바이트) + dn 덤프 생성(종료 코드 0, 375,958바이트). 게임 실행 없음(빌드·검사·덤프 실행만). `originals/` 1,394파일 해시 차이 0(실행 전후 동일). 비교(`compare_slots.py` + player/temple 대조): dn 3,288·cpp 3,471·공통 3,288·dn_only 0·cpp_only 183·타입 2,769·소유자 666·프레임 163·전체 81, Game·CRT 행은 시드가 달라 구조만 대조. 그래프 row0 InUse·Reserved 1·row254 sentinel 일치(0.476 ResetTable), dn InUse 2 대 cpp 20(R6 per-Pop 번호 잔여). player 이름 전 경로 일치(You+AI 2명), +0x78은 0.477 정정 후 상태로 전원 127 유지. 신전 가상표 4종이 경로당 소유자별 1개씩(1·3·2) 있고 cpp 좌표는 (134,123)·(74,124)·(201,125), dn은 Pop 미연결로 (0,0)이다 — 소용돌이 사슬 입력 Pop 결핍의 증거(R6). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다. paired 산출물은 `extracted/dotnet-l2-baseline/dissolvedalliance/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 `extracted/` 산출물만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 5/34).

#### 0.477 — 단일 미션 +0x78 기록자 정정과 섬 런타임 Player 기록 바인딩 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든다섯 번째 단위. 0.474의 단일 미션 자동 기록을 정정하고 소용돌이 사슬의 전제(섬 런타임 Player 기록 공유)를 연결한다. 제품 코드 변경 있음(호출 제거+바인딩).
- 근거·판정: ① 단일 미션은 missionWorld=true라 로그인에서 loadWorld 분기를 타서 makeBattleIsland가 돌지 않는다(cpp `RawMissionLogin.cpp` 162~169행·`MissionLoginGlobals` 바인딩: 4번째가 `objectState.mission`=true, 5번째가 `missionPost`=true). ② 0.474 규칙은 savetheisland에서 깨진다(dn p1=0 대 cpp p1=1). ③ fragilefortune·thunderingpower는 선택 영역이 배치 불가라 생략되지만 cpp는 값을 가진다(배치 무시·단일 배치 경로의 기록). ④ 경로별 요새에 신전 가상표 객체가 소유자 1·2에 정확히 2개씩 있다(임시 탐침 실측 후 삭제). ⑤ 진짜 기록자는 소용돌이→섬 소유자→첫 지역 사슬이다(cpp `RawIslandRuntime.cpp` 172~175행의 FirstRegion 후속 지정·`RawCampaignOwnership.cpp` 57행의 setIslandOwner·신전 가상 Set 경로). 따라서 시작 흐름의 자동 기록을 걷어내고 배정 갈고리·장치는 MP 전투 경로용으로 둔다.
- 구현: `ClientHostGame.StartMission`에서 battleTerritory 해석·`WriteBattleIslandRegion` 호출을 제거하고 주석으로 경로를 밝혔다. `HostSession.BuildIslands`에 `islands.BindPlayerRecords(PlayerRecords)`를 연결했다(근거 cpp `RawCampaignOwnership.cpp` 61행). 관련 장치 주석 3종을 MP 경로·R6 사슬 기준으로 바로잡았다. 신규 `HostIslandFirstRegionTests` 2건(공유 기록·첫 섬 유지·0번 생략).
- 검증: 빌드(경고 0·오류 0) 뒤 신규 2건 통과(실패 0) + Client 전체 1,171건 통과(실패 0, NETSTORM_DATA 지정). 변이(바인딩 주석화)에 신규 2건 실패로 검출력 확인 후 원복·재통과. paired thewarbegins f090 재실행(예외 PC, `--mission thewarbegins --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`, 종료 코드 0): 덤프 2,129행 중 0.476 덤프와 player 1행(p1 +0x78 0→127 복원)과 프레임 지터 1행만 다르고 슬롯·그래프·Game·CRT·이름·동맹 전부 동일. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: 소용돌이 호출 연결(신전 Pop→islands.Owner→첫 지역, R6 postPop·표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 그래프 per-Pop 번호(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 4/34).

#### 0.476 — 그래프 ResetTable 설치와 5경로 dn f090 재생성 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든네 번째 단위. 큰 맵(savetheisland·fragilefortune)의 시작 흐름 번호 소진 crash를 고치고 5경로의 dn 덤프를 HEAD로 재생성한다. 제품 코드 변경 있음.
- 원인 확정(조사): `RebuildWorldGraph`가 빈 표에서 임시 번호 0을 잡으면 기존 graph-0 슬롯 2413개(savetheisland 실측: type 97/82/157, void 331 포함)를 전부 임시 번호로 보고 flood 재번호(=0.470 thewarbegins 120/255의 정체)하다 251개를 넘겨 소진된다. cpp는 세션 설치 ResetTable로 0번을 예약해 임시 번호가 1이 되므로 graph-0을 건너뛴다(근거 cpppj RawSurfaceWorld.cpp 161행·저장소 Acquire의 Graph ResetTable, dn `RawGraphStorage.ResetTable`). 덤프 대조로도 확인했다: 5경로 cpp 기준선 row0은 InUse·Reserved가 1이고 row254는 sentinel 32253(원본 설치 상태), dn은 row0·row254 모두 0이었다.
- 구현: `HostSession` 생성자에 `RawGraphStorage.ResetTable(Graph)`(세션 설치 의미)+`BuildWorldGraphDevices`에 `_worldGraph.ResetTable()`(엔진 설치 의미)을 연결했다(cpppj 적재 경로는 ResetTable을 호출하지 않으므로 Load 흐름에는 손대지 않는다). per-Pop 그래프 번호 부여는 postPop 그래프 연결과 함께 R6에서 잇는다. `HostWorldGraphTests`의 기존 기대(재구성이 표를 채운다)를 설치-유지 의미로 바로잡고 ResetTable 상태 검사 1건을 더했다. `SessionStateDumpTests`의 world-1 Graph 255칸 기대값도 설치 상태(0번 0/1/1·254번 sentinel)로 고쳤다.
- 검증: 빌드(경고 0·오류 0) 뒤 그래프·덤프 집중 15건 통과(실패 0) + Client 전체 1,169건 통과(실패 0, NETSTORM_DATA 지정). 변이(엔진 ResetTable 주석화)에 신규 1건 실패로 검출력 확인 후 원복·재통과. 5경로 paired f090 재생성(예외 PC, `--mission <키> --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`, 전부 종료 코드 0 — savetheisland·fragilefortune crash 해소): dn_only 0·Game·CRT·행 수 중립, 아바타분 타입 +1, 그래프 236~249/255 일치(0번 예약·254번 sentinel 일치, 차이는 게임플레이 번호 7~20행=R6), player 이름 전 경로 일치. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-baseline/<경로>/dn/`·`extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- 경로별 +0x78 대조(thewarbegins·masterofwhirligigs p1 일치, savetheisland p1 0 대 1 불일치, fragilefortune·thunderingpower 생략): 0.474 전투섬 배정 규칙의 적용 범위를 정정한다 — 단일 미션은 missionWorld=true라 loadWorld 경로를 타서 makeBattleIsland가 돌지 않으며(근거 cpppj RawMissionLogin 162~169행·MissionLoginGlobals 바인딩), 단일 미션의 기록자는 소용돌이→섬 소유자→첫 지역 사슬이다(근거 RawIslandRuntime.Owner 172~175행·vortex setIslandOwner, 경로마다 신전 2개(소유자 1·2) 실측, savetheisland p2→p1 순서와도 부합). 배정 갈고리·장치는 MP 전투 경로용으로 둔다. 소용돌이 사슬 연결(신전 위치→섬→첫 지역)은 Pop·postPop 표시와 함께 R6에서 잇는다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사·Core 포팅만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: 소용돌이 사슬 연결(R6: 신전 위치→섬 소유자→첫 지역, Pop·postPop 표시와 함께), 화신 Pop 공간 등록·예약 갈고리(R6), AI 참가자 첫 지역(R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 4/34).

#### 0.475 — Player 월드 갈고리 live 연결: 화신 공장 생성·소유자·찾기 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든세 번째 단위. R4 진행판 7번 중 Player 월드 갈고리를 live 연결한다. 제품 코드 변경 있음.
- 근거·판정: cpp `RawCampaignSession.cpp` 1235~1242행 playerHooks 결선(find→world.Find·create→world.Create·setOwner→core.owner.Set·pop→world.Pop)을 `HostSession.BuildPlayerWorldHooks`로 옮겼다(Find→`RawSquidFinder` 첫 일치·Create→세션 공유 공장·SetOwner→요새 객체와 같은 `SquidOwner`). Pop 공간 등록은 R6 경계로 남긴다: 임시 탐침으로 직접 `RawPop.Pop`(152) 호출 시 표시·파생 등록 미지원 throw를 확인하고 탐침을 삭제했다(0.467 요새 객체 Pop과 같은 경계). ReserveRegular·AddRegular는 정기 프로세스 몫이라 잇지 않으며(패치판 생성자는 요구하지 않는다), 화신 생성의 Pop 호출은 기록만 한다.
- 구현: `HostSession`에 `_playerOwner` 필드 보관(`BuildFortObjectDevices`의 지역 공유)+`BuildPlayerWorldHooks()`(장치 미연결 거부·플래그→Predictable/Client/Server 매핑), `ClientHostGame.StartMission`에서 기록 스텁 4종을 live 갈고리로 교체하고 화신 사전 할당(`Pool.Allocate`)을 제거했다(첫 할당=공장 생성이라 SID 번호 유지). 신규 `HostPlayerWorldLiveTests` 7건(장치 거부·빈 찾기 0·공장 생성 152/가상표·등록 전 찾기 빗나감·소유자 바이트·Pop 기록 경계·`RawPlayerWorld.Create` 전 순서).
- 검증: 빌드(경고 0·오류 0) 뒤 신규 7건 통과(실패 0) + Client 전체 1,168건 통과(실패 0, NETSTORM_DATA 지정). 변이(소유자 +1)에 신규 2건 실패로 검출력 확인 후 원복·재통과. paired 라이브 재실행(예외 PC, `steps-dn-f065-0475.tsv`, `--mission thewarbegins --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`, 종료 코드 0): 덤프 2,129행·random·CRT·player 9기록 모두 0.474와 동일하고 pool 1행+slot 5(아바타) 1행만 변경됐다(빈 할당→공장 생성 152·소유자 1, SID 번호·행 수·Game·CRT 중립). `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사·Core 포팅만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: 화신 Pop 공간 등록·예약 갈고리(R6 postPop·표시·정기 프로세스), AI 참가자 첫 지역(사슬·AI 설정, R6), 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 4/34).

#### 0.474 — 전투섬 배정 region 갈고리 live 연결: 전투 소유자 +0x78 기록 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든두 번째 단위. 0.472 귀속 확정·0.473 127 초기화에 이어 참가자(전투 소유자) 지역을 실제 기록한다. 제품 코드 변경 있음.
- 근거·판정: cpp `RawCampaignSession.cpp` 3243행 `hooks.region`(범위 밖 9 이상은 `out_of_range`, `Put(players, player*180+0x78, island)`)을 `HostSession.WriteBattleIslandRegion`+`BuildBattleIslandRegionHook`으로 live 연결했다. 시작 섬 번호는 `RawBattleIsland.SelectBattleTerritory` 분리(0046e2c0 선택 순서와 동일, `Build` 동작 그대로)에 `PlaceAll` 배치 순서(앞선 배치 가능 레코드 수)를 더한 `TryResolveBattleIslandRegion`으로 푼다. thewarbegins 실측 탐침(임시 검사로 Territory 120바이트 전수 출력 후 삭제): rec0·rec1만 배치 가능(b1=0x09)·배틀 플래그 없음→selected 0·앞선 배치 0→섬 0. 배치 불가 선택·120바이트 미만은 거짓으로 127을 유지한다.
- 구현: `RawBattleIsland.cs`에 `SelectBattleTerritory` 정적 분리+`Build` 호출로 교체(동작 동일), `HostSession.cs`에 `PlayerFirstRegionOffset` 재사용 기록·갈고리·해석 3종, `ClientHostGame.StartMission`의 `worldLoad.Load()` 뒤에 전투 소유자(`MissionPlayer.BattleOwner`) 기록 연결(AI 참가자는 AI 설정 사슬에서 덮어쓰므로 R6). 신규 `HostBattleIslandRegionTests` 10건(기록 LE·덮어쓰기·범위 거부·갈고리 관통·선택 실측/합성/짧은 입력·해석 실측/합성/불가).
- 검증: 빌드(경고 0·오류 0) 뒤 신규 12건(신규 10+0.473 2) 통과(실패 0) + Client 전체 1,161건 통과(실패 0, NETSTORM_DATA 지정). 변이(`selected = index + 1`)에서 신규 2건 실패로 검출력 확인 후 원복·재통과. paired 라이브 재실행(예외 PC, `steps-dn-f065-0474.tsv`, `--mission thewarbegins --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`, 종료 코드 0): 덤프 `thewarbegins-dn-f065-0474.tsv`(2,129행·215,228바이트, 0.473 덤프와 바이트 길이 동일)에 player 1 +0x78 127→0으로 cpp(p1=0)와 일치, 나머지 8기록 127 유지(비참가자 7기록 cpp 일치·p2 AI는 R6 잔여), 0.473 덤프와 대조해 player 1행 1줄만 변경·random 2317693072·CRT 296677442 중립 확인. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사·Core 포팅만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: AI 참가자 첫 지역(소용돌이→섬 소유자→첫 지역 사슬·AI 설정 지역 기록, R6 brain 실생성자·AI 설정과 함께), Player 월드 풀·공간 Pop·예약 갈고리, 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 4/34).

#### 0.473 — Player 첫 지역(+0x78) 9기록 127 초기화: 로그인 초기화 Start 순서 연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든한 번째 단위. 0.472에서 로그인·초기화 잔여로 남긴 비참가자 127을 Start 순서에 연결한다. 제품 코드 변경 있음.
- 구현: `HostSession`에 `PlayerFirstRegionOffset(0x78)` 상수 추가(근거 cpppj `o/RawPlayerInitialization.cpp` 46행·`o/RawFortFinishOwner.h` 9행 기본 127, 0.472 귀속 확정) + `StartPlayers`末尾에 9기록(0~8) +0x78 DWORD를 127(LE 7F 00 00 00)로 쓰는 반복문 추가(참가자는 전투섬 배정 갈고리에서 덮어쓸 예정). 신규 `HostPlayerFirstRegionTests` 2건(9기록 전부 127·이름/동맹 보존). C# 모양 차이 없음(세션 소유 바이트 배열에 원문과 같은 LE DWORD 쓰기).
- 검증: 빌드(경고 0·오류 0) 뒤 신규 2건 통과(실패 0) + Client 전체 1,151건 통과(신규 2건 포함, 실패 0, NETSTORM_DATA 지정, 110초). paired 라이브 재실행(예외 PC, `steps-dn-f065-0473.tsv`, `--mission thewarbegins --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`, 종료 코드 0): 덤프 `thewarbegins-dn-f065-0473.tsv`(2,129행 유지)에 player 9기록 +0x78 전부 127(7F) 확인, random·CRT·슬롯 구조는 0.470 덤프와 중립(길이 215,228 동일). 비참가자 7기록은 cpp 127과 일치, 참가자 2기록은 전투섬 배정 갈고리 후속에서 해소. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만, 작업 트리의 cpppj/tests·docs/exe 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: 전투섬 배정 region 갈고리 결선(R4, 배정 섬 확정 위치에서 참가자 +0x78 기록), 소용돌이→섬 소유자→첫 지역 사슬과 AI 설정 지역 기록(R6, brain 실생성자·AI 설정과 함께), Player 월드 풀·공간 Pop·예약 갈고리, 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 4/34).

#### 0.472 — 전투섬 배정·첫 지역(+0x78) 귀속 확정: 섬 배정 갈고리·AI 설정 미연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 여든 번째 단위. paired 덤프의 플레이어 기록 차이(+0x78 첫 지역: dn 전부 0 vs cpp 참가자 0/1·비참가자 127, 참가자 전용 필드군)의 기록자를 가린다. 제품 코드 변경 없음(조사·디스크 대조 단위).
- 근거·판정: ① cpp f6(적재 직후)==f65이므로 적재 시점 기록이고 dn은 전 구간 0이다. ② 참가자 지역(p1=0·p2=1) 기록자는 섬 배정·AI 설정 사슬이다: `BuildBattleIsland` hooks.region→`Put(players, player*180+0x78, island)`(cpp `RawCampaignSession.cpp` 3243행·`RawBattleIsland.cpp` 22행), `islands.Owner`→`FirstRegion`(cpp `RawIslandRuntime.cpp` 172~175·205~212행, 소용돌이 신전 사슬·AI 경로에서 호출), AI 설정 지역 기록(cpp `RawAiBrain.cpp` 85~89행·`RawMakeAi.cpp` 55행). dn은 `RawBattleIsland`를 포팅만 하고 호스트에서 호출하지 않으며(`Netstorm.Client*`에 호출 없음), 호스트 전역에서 `islands.Owner`를 호출하지 않고, brain은 스텁 토큰이라 AI 설정이 돌지 않는다 → 전부 0이다. ③ 비참가자 127(+0x78 단일 바이트만 0x7f)은 로그인·플레이어 기록 초기화 소관으로 남긴다(cpp 소스에 +0x78 기록자는 hooks.region 계열뿐이라 초기값 경로이며, dn 기록은 0 초기화 + 이름·마스크·동맹만 쓴다 — item 7 Start 순서 잔여). ④ 기록 0x1~0x4(전원, 포인터형)는 세션 주소라 대조 대상에서 제외한다.
- 검증: 제품 코드 무변경이라 새 집중 검사 없음(0.471 게이트 Client 전체 1,149건 통과를 그대로 인용, dn 제품·검사 코드 변경 없음). 디스크 대조만 수행했다: thewarbegins paired 덤프 플레이어 9기록 바이트 단위 대조(0x1~0x4 전원·+0x78 전원·참가자 전용 0x8/0x10~0x13/0x68~0x81/0x9c 등 확인) + cpp 소스 기록자 추적 + dn 호스트 미연결 확인. paired 산출물은 `extracted/`에만 둔다(Git 제외). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: 전투섬 배정 region 갈고리 결선(R4, 배정 섬이 확정되는 위치에서 +0x78 기록), 소용돌이→섬 소유자→첫 지역 사슬과 AI 설정 지역 기록(R6, brain 실생성자·AI 설정과 함께), 플레이어 기록 127 초기화(Start 순서·로그인 잔여, item 7), Player 월드 풀·공간 Pop·예약 갈고리, 세션 `Bind*` 실제 몸체, 요새 이미지 전송, 메뉴 미션 시작, 농지 배치 파이프라인, 34경로 L2 대조(M1, 4/34).

#### 0.471 — 장부(totals·totalmade) 귀속 확정: 적재·생성 Pop 집계 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔아홉 번째 단위. paired 덤프의 장부 차이(dn totals 0/0/0 vs cpp 45100/0/0, dn totalmade 행 없음 vs cpp 16행)의 기록자를 가린다. 제품 코드 변경 없음(조사·디스크 대조 단위).
- 근거·판정: cpp 장부는 Pop 성공 때 쓴다 — `SquidPostPop::PostPopBase` 417~431행(`TotalCost()=TotalCost(number)` 누적 + `LocalCounts`/`LocalSecondaryCounts`(로컬 소유자 일치 때) + `GlobalCounts` 증가, Patch판은 `SquidPostPopPatch` 21~22행 `localCount`/`globalCount` 갈고리 경유). dn은 지형 Pop이 중립 보존(0.466)·요새 객체 Pop이 미연결(0.467)이라 집계가 전혀 돌지 않는다 → R6 postPop 파이프라인에서 잇는다. cpp totalmade 16행은 타입별 census와 일치(97:1505·102:168·157:117 등)해 Generate+Objects 적재 Pop 누적인 것과 부합한다. `islands.Owner`/소용돌이·`RegionPaint`·적재 owner식과 무관하다(0.469에서 제외済, 장부는 Pop 집계라 별개 경로).
- 검증: 제품 코드 무변경이라 새 집중 검사 없음(0.470 게이트 Client 전체 1,149건 통과를 그대로 인용, dn 제품·검사 코드 변경 없음). 디스크 대조만 수행했다: thewarbegins paired 덤프 totals/totalmade 행 비교 + cpp 소스 기록자 추적. paired 산출물은 `extracted/`에만 둔다(Git 제외). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: postPop·표시·그래프 파이프라인 이식(R6, 지면/noIsland owner 스탬프·totals/totalmade 집계 해소), 게임플레이 규칙(R6, 교량/객체 프레임·owner 진화), 농지 배치 파이프라인, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.470 — 월드 그래프 재구성 live 연결과 paired 검증 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔여덟 번째 단위. territory·campaign 적재 갈고리의 `RebuildGraph`를 세션 엔진에 live 연결한다. 제품 코드 변경 있음.
- 구현: `HostSession.BuildWorldGraphDevices(types)`(지형 Pop 장치의 해시·점유 지도와 채운 `WorldFrames`를 공유해 `RawGraph` 엔진 생성, `RiftFrameCode`→`FrameCode` 변환 — 플래그 비트 동일) + `RebuildWorldGraph(mode)`(엔진 `Rebuild` 후 표·스택을 세션 `Graph` 저장소로 복사, `DumpGraph` 관측 유지) + `ClientHostGame`의 territory·campaign 두 갈고리 live화(cpppj `RawCampaignWorldRuntime.cpp` 65·83행·`RawFortWorldLoad.cpp` 42행 대응, `ResetTable`은 cpp 적재 경로에서도 호출되지 않아 연결하지 않는다). 신규 `HostWorldGraphTests` 3건(장치 거부·미연결 거부·실제 장면 재구성 표 채움/결정성/그래프 바이트 기록).
- 검증: 빌드(경고 0·오류 0) 뒤 신규 3건 통과(실패 0) + Client 전체 1,149건 통과(신규 3건 포함, 실패 0, 85초). paired 라이브 재실행(예외 PC, `steps-dn-f065-0470.tsv`, `--frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000`, 종료 코드 0): 덤프 `random 2317693072 296677442`(Game 새 기준 유지·CRT parity 유지, 재구성은 난수 무소비), 슬롯 1853·`compare_slots.py` 대 cpp-f065 결과 타입 1628·프레임 123/1853·full 78·owner 460·dn_only 0·cpp_only 87로 0.467과 동일하고 변경 슬롯 1637개는 오프셋 30(그래프 바이트)만 달라졌다. graph 행 255개 중 120개 일치, 135개 불일치(InUse 수 차이 — dn 풀 상태 차이 소관이라 R6에서 잇는다). `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 작업 트리에 있던 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: graph 나머지 135행(풀 상태 수렴 뒤 재대조, R6 postPop·객체 소관), totals·totalmade 장부, 농지 배치 파이프라인, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.469 — 소유자 후속 지정 귀속 확정: 지면·받침은 적재 postPop, 교량·요새 객체는 게임플레이 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2의 일흔일곱 번째 단위. owner 불일치(공통 SID 중 isle-97 1353건·noIsland-157 23건·교량-82 9건·요새 객체 5건 등)의 기록자를 가린다. 제품 코드 변경 없음(조사·디스크 대조 단위).
- 근거·판정: ① 요새 적재 owner식은 양쪽 동일하다(마스크 0x5d77cf00/0x10002000·255-저장 규칙·version 분기·island/noIsland 0, cpp `RawFortObjectLoad.cpp` 76~104행 vs dn `RawFortObjectLoad.cs` 474~552행) → 적재식 탓이 아니다. ② 지면 스탬프는 적재 중 postPop 소관으로 확정했다: cpp f6(적재 직후) isle-97 owner 분포 {1:853, 2:500}가 f65와 바이트 동일하고 dn은 전부 0이다. dn Pop은 중립 기록만이라 `IslandPostPop`(`SetSupportOwner` 3×3 무타입필터 스탬프·`Prefix` 문자 경로·`connect`, cpp `RawSurfaceStorage.cpp` 154·380행 경유)을 건너뛴다 → R6 postPop·표시·그래프에서 잇는다. `islands.Owner`/소용돌이 사슬은 residenceType=89 고정이라 제외하고, `RegionPaint`는 건설 UI 호출뿐이라 제외했다. ③ 교량·요새 객체 차이는 게임플레이(f6→f65) 소관으로 확정했다: cpp f6 교량은 dn f65와 동일(owner 2·저장 프레임 0x23/0x29 등)이나 f65에서 owner 0·재계산 프레임(2~8·16)으로 바뀐다. dn은 R6 규칙이 없어 적재값에 머문다 → R6 게임플레이에서 잇는다. noIsland owner 2는 f6에 이미 있어 ②와 같은 적재 postPop 소관이다. ④ 부수 1건: SID 5(type 0, dn 0 vs cpp 1)는 특수 슬롯 잔여로 남긴다.
- 검증: 제품 코드 무변경이라 새 집중 검사 없음(0.468 게이트 Client 전체 1,146건 통과를 그대로 인용, dn 제품·검사 코드 변경 없음). 디스크 대조만 수행했다: thewarbegins paired 덤프 SID 단위 owner 바이트(오프셋 34) 비교와 cpp early f6 적재 직후 대조. paired 산출물·비교 절차는 `extracted/`에만 둔다(Git 제외). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서만). 작업 트리의 webpj 미커밋 변경은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛴다.
- 남은 것: postPop·표시·그래프 파이프라인 이식(R6, 지면/noIsland owner 스탬프 해소), 게임플레이 규칙(R6, 교량/객체 프레임·owner 진화), 장부·그래프·농지 파이프라인, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.468 — 4경로 dn 미션 덤프 생성과 SID 단위 구조 대조·적재 순서 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔여섯 번째 단위. baselined 4경로 중 나머지 3경로의 첫 실제 dn 미션 덤프(f090/f240)를 생성하고 SID 단위로 대조하며, 요새 적재 순서 패리티를 검사로 확정한다. 제품 코드 변경 없음(검사 2건 추가·측정 단위).
- 구현: 신규 `HostFortObjectLiveTests` 2건(기존 5건 → 7건). `ProbeFortLoadVsDumpOrder`는 Load와 같은 순서·건너뜀·다리 매핑·내용물 끼워넣기로 기대 타입열(`CollectFortSection`·`CollectFortContents`: y 바깥·x 안쪽, 지역 `Matches` 건너뜀, 0xf6+→다리 타입, ContainerFlag 부착, factoryFlags→server/client/predictable 분기)을 모아 풀 번호순 실제와 엄격 대조한다. `ProbeFortContentsNulls`는 ContainerFlag 조건 내용물 null 위치를 전수 특정한다(`CollectNulls` 재귀, null이면 실패). 4경로 dn 미션 덤프 신규 생성(`*-dn-f090-0468.tsv`·`*-dn-f240-0468.tsv`, 고정시각 paired, `extracted/dotnet-l2-baseline/<경로>/dn/`, Git 제외). thunderingpower는 cpp 기준선 시드가 dn 고정시각과 달라 구조 참조용으로만 둔다.
- 검증: 빌드(경고 0·오류 0) 뒤 Client 전체 1,146건 통과(신규 2건 포함, 실패 0, NETSTORM_DATA 지정, 121초). f090 SID 대조(`compare_slots.py`, 직접 재실행 확인): masterofwhirligigs dn 1743·cpp 1807·공통 1743·타입 1591·프레임 75·full 53·Game 173420507, savetheisland 2618·2926·2618·2246·126·61·Game 4032020911, fragilefortune 2719·2946·2719·2205·195·78·Game 1491053939, thunderingpower 2410·2490·2410·2221·116·73·Game 297644596(구조만). 4경로 모두 dn_only 0·공통=dn 전체, CRT 296677442 parity 유지, dn frame max 208·cpp max 316. f240은 3경로 모두 dn_only [6] 1개(내용물 부착형, 게임플레이 잔여·R6). cpp_only SID는 클라이언트 블록 8~81+(플레이어·AI·셋업·프로세스, item 7/R6 잔여) + 타입 34 gun-process 인터리브 약 9개(postPop 생성, R6) + 5-SID 꼬리. dn 생성 순서는 fort 순서·cpp 순서와 modulo 9 인터리브를 제외하고 일치한다(순서 검사 통과). noIsland 소유자 차이(dn 0 vs cpp 2)는 미해결로 남기고 적재 후 배치(R6) 추정으로 적는다. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다(작업 트리 `git status`에 dotnetpj 2건만 남김).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서와 dotnetpj 새 경로 검사만). 작업 트리에 있던 webpj 미커밋 변경(geyser-mined 2건)은 이번 단위와 무관해 손대지 않고 커밋 범위에서 제외한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 장부·그래프·농지·소유자(R4 진행판 6번 잔여, noIsland 소유자 후속 지정 포함), 객체 Pop 공간 등록(postPop·표시·그래프, R6) + 덤프 대조, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34 — 동일 시드 paired cpp 재실행은 본 대조 때 묶어서).

#### 0.467 — 요새 객체 live 적재와 paired 구조 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔다섯 번째 단위. R4 진행판 6번 중 요새 객체 조각을 live 연결한다. 제품 코드 변경 있음.
- 구현: `HostSession.BuildFortObjectDevices`(공장·HP·내용물·건물·Pop 몸체, `MakeFortObjectLoadHooks`로 Create/HP/내용물 결선, `RawFortObjectLoad(Pool, 공유 해시·spots, types, frames, Game, state, hooks)`) + `LoadFortObjects(fort, 칸)` + `StartMission` Fort/Campaign Objects live 연결(기록 병행). 상태값은 미션 적재값(Mission=true·ChaffMode=true, Fort=false·Special=false·Version=0, cpppj 1093행 대응). SetState·FinishOwner는 기록만(R6 공장 후처리·거주 배치), Destroy·VirtualCount는 미션 적재에서 도달하지 않으므로 명시 경계, 내용물 부착은 bound Form 분배+서버 전송 기록으로 live 연결한다(cpppj `RawCampaignWorldRuntime.cpp` 43~53·61·82행 대응). Pop은 기록만 둔다(R6 postPop·표시·그래프 장치와 함께 잇는다).
- 근거·판정: 실측으로 범위를 확정했다. thewarbegins 요새 객체에 Pop 지원표 밖 가상 표(71·75·78·89·122 등)가 있어 공간 등록을 수행하면 파생 등록 효과 미지원으로 멈춘다 → Pop은 R6으로 미룬다. 내용물 보유 타입(123×1·124×1·158×2)이 있어 부착은 live로 잇는다. test 순서가 live와 달라 부분집합만 돌던 문제를 `BuildLiveTerrainFirst`(Territory 120B→PlaceAll→Generate) 도입으로 해소했다(같은 적재 부분집합, 같은 GameRandom 소비 순서).
- 검증: 신규 `HostFortObjectLiveTests` 5건 통과(장치 거부·미연결 거부·실제 요새 구조/결정성·Pop 미지원 분류·내용물 전수, 실패 0, NETSTORM_DATA 지정). 빌드(경고 0·오류 0) 뒤 Client 전체 1,144건 통과(신규 5건 포함, 실패 0, 76초). paired 라이브 재실행(예외 PC, `steps-dn-f065-0467.tsv`, `--frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000`, 종료 코드 0): 덤프 2,129행·슬롯 1,853(+174), `random 2317693072 296677442`(Game 새 기준·CRT parity 유지), `compare_slots.py` 대 cpp-f065 결과 공통 1853·타입 1628·프레임 123/1853·full 78·dn_only 0·cpp_only 87(261→87)·dn max 208·cpp max 316. `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 객체 Pop 공간 등록(postPop·표시·그래프, R6) + 덤프 대조, 장부·그래프·농지·소유자(R4 진행판 6번 잔여), brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.466 — 지형 Pop 공간 등록 live 연결과 paired 중립성 검증 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔세 번째 단위. R4 진행판 6번 중 Pop·공장체 조각을 live 연결한다. 제품 코드 변경 있음.
- 구현: `HostSession.BuildTerrainPopDevices`(공장·해시·spots·SHP 표시 결선, cpppj `MakeTerrainStampWorldHooks`의 core.factory·core.hash·core.shapes→FrameSize 대응) + `BuildTerrainStampHooks` Create/Pop live 분기(미연결 때 neutral 기록만이라 기존 드라이런 검사 그대로 통과) + `ClientHostGame.StartMission` SHP 적재·장치 연결(`d/_shapes.shp`→`ShapeDatabase`→`TerrainDisplayShapes.Build`→`BuildTerrainPopDevices`). 신규 `TerrainDisplayShapes.cs`(cpppj `SquidRenderer::Shapes` 이식: 70번부터 자산 순서·클러스터 수=프레임 수·SHP 물리 순서). `Build`는 정의 안의 typename이 아니라 SHP 블록 순서(=자산 로딩 순서)로 찾는다(typename 조회는 `Man` 같은 내부 이름에서 실패한다). `HostSession`에 검사·대조용 `TerrainPopHash`·`TerrainPopSpots`를 두었다. 중립 Create의 `raw.Clear()`가 VoidBit를 지우던 것을 `VoidBit` 재설정으로 고쳤다(공장 경로는 Allocate가 Void를 둔다). `ClientHostGame`의 `Netstorm.Assets` using 모호 참조(`MissionScript`·`ShapeDatabase`)는 using 제거+완전 한정으로 해소했다.
- 검증: 신규 `HostTerrainPopTests` 5건 통과(중립 보존·개수 거부·공장 vtable Supports·섬 좌표/화면좌표/단계0/섬127/void해제/체인/spot·프린지 실측 3단계, 실패 0). 기존 `HostTerrainBinding/Stores/Inputs` 9건 통과(중립 동작 보존). 빌드 경고 0·오류 0 뒤 Client 전체 1,139건 통과(신규 5건 포함, 실패 0, NETSTORM_DATA 지정, 95초). paired 라이브 재실행(예외 PC, `steps-dn-f065-0466.tsv`, `--frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000`, 종료 코드 0): 덤프 `random 2583299428 296677442`(Game 예측 일치·CRT parity 유지), 구조 1,955행·슬롯 1,679 동일, `compare_slots.py` 대 cpp-f065 결과 공통 1679·타입 1526·프레임 80/1679·full 78·dn max 208·cpp max 316으로 0.464와 동일하고 pool 해시만 Pop 기록분 변경됐다(Pop은 타입·프레임·난수 중립). `originals/` 미접촉, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Objects 라이브 연결(`RawFortObjectLoad` + 갈고리 약 20종, 공장 할당 parity) + 덤프 대조, 장부·그래프·농지·소유자(R4 진행판 6번 잔여) + 기동/메뉴 경로 복원 뒤 프레임 수렴 재시도, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.465 — early-dump N(F) 역산과 ambient-sim 가설 기각·회수 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔두 번째 단위. K-search 평탄(75~86/1679) 원인을 early-dump Game 역산으로 규명한다. 제품 코드 변경 없음(미커밋 `SimulateMenuAmbient` 34행을 회수해 작업 트리 clean, 조사·검증 단위).
- 증거 N(F) 역산(seed 4097495276 기준 advance 횟수, `extracted/dotnet-l2-fixed/thewarbegins/`, Git 제외): dn-f065 1764 · cpp f1 3395·f2 3397(+2)·f3 3398(+1)·f4 4680(+1282)·f6 8078(f4→f6 +3398)·f7 8079(+1)·f8 8081(+2)·f10 8084·f065 12077·f090 16168(f065→f090 +4091/25=+163.6/프레임)·f240 40719(f090→f240 +24551/150=+163.7/프레임)·dn-f065 1764. 메뉴율 1~2/프레임(Wind만)과 일치, 게임플레이율 ~163-164/프레임, AI 활성화 ~f31-f40(f29→f31 +2·f31→f40 +1155/9≈128·f40→f50 ≈164).
- Generate 구간 특정: f1-f3 world 0·슬롯 0, f4 world 1·kernel 0·슬롯 5(클릭 5 이전에 출현), f6 world 1·kernel 34·슬롯 1928. 클릭-5→덤프-6 사이 Terrain Generate+Objects 적재. CRT는 f1-f4 511160713→f6+ 296677442로 바뀌나 Game 사슬은 연속(N 단조 증가, 재시드 없음)이라 Game 무재시드(0.464 유지)·CRT만 적재 시 재설정(동일 고정 시각이라 같은 값) 또는 세션 스트림 교체로 판정. dn CRT 296677442와 f6+ parity는 유지된다.
- f4 이상치(+1282, 클릭 전 world-1·5슬롯): 비동기 적재 스캐폴딩 또는 메뉴 배경 세션 중 하나로 판정(어느 쪽이든 dn에 없는 적재-전 소비). 0.461 "양쪽 동기 실행" 서술은 정정한다: cpp 적재는 f4→f6에 걸쳐 프레임 경계를 넘나들며(메인루프 ambient가 적재 사이 약 4 draws 인터리브 가능), dn 적재는 `LoadContent` 동기라 인터리브가 없다. ambient 완전배제가 아니라 소규모 기여자로 정정한다.
- K-search 평탄 설명과 sim 기각: 지형-전 gap ≥ ~3500 draws(기동 3395+f3-f4 1282+적재-전 L_pre, dn 지형-전 1154=33+801+320 대비)인데 K≤90은 약 180 draws 이하라 범위를 벗어난다. sim 위치(`StartPlayerSp` 앞 Generate 전 일괄)도 cpp 순서(적재 후 매 프레임·적재 사이 인터리브)와 달라 magnitude·position 둘 다 불일치 → sim 회수. f4→f6 3398−Sets(320)−지형(610)−ambient ≈ 2400-2500 draws는 Objects·Pop·농지 배치·공장체·소유자 잔여(R4 진행판 6번)와 규모가 부합한다.
- 진입 비대칭 주의(M1 caveat): cpp paired 실행은 메뉴 경로(`--mission` 없음: f1-f3 world 0, 적재 f4-f6)이고 dn은 `--mission` 직접 진입(`LoadContent` 적재)이라 동일 프레임 번호가 적재-후 경과와 다르다(cpp f90≈84 vs dn f90=90 post-load). 지형 비교에는 무영향이나 Game/SP 행의 동등-프레임 cross-side 비교는 적재-후 정렬이 필요하다.
- 검증: 회수 뒤 작업 트리 clean(`git status` 빈 출력), 깨끗한 전체 리빌드(경고 0·오류 0, 69초) + Client 전체 1,134건 통과(0.464 게이트, 실패 0, NETSTORM_DATA 지정, 85초). 새 집중 검사 없음(조사 단위). 게임 실행 없음(디스크 덤프+정적 분석만). `originals/` 변경 없음, 추적 `options.cfg` 변경 없음. paired 산출물·역산 스크립트는 `extracted/dotnet-l2-fixed/`에만 둔다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(변경은 LEFT_JOBS.dotnetpj.md 문서만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Objects 라이브 연결(`RawFortObjectLoad` + 갈고리 약 20종, 공장 할당 parity) + 덤프 대조, 장부·그래프·Pop·농지·공장체·소유자(R4 진행판 6번) + 기동/메뉴 경로 복원 뒤 프레임 수렴 재시도, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.464 — 호스트 시작 1회 reseed+SP 초기화 적용과 paired 검증 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔한 번째 단위. 0.463 근본 원인(reseed 시점)을 수정한다. 제품 코드 변경 있음(호스트 공유 SP·CRT 상자, 미션별 reseed 제거).
- 구현: `ClientHostGame`에 `_spModeBox`·`_spCrtBox`·`_sharedSp`를 두고 생성자에서 reseed+`Initialize` 1회 실행(cpppj `ClientMain` 349행 대응, 원본 004391ed). `HostSession`에 공유 Sp·CRT 상자 빌림 overload를 추가했다(기존 3인자 호출은 세션 전용 상자로 그대로 동작한다). `StartMission`에서 미션별 `BindSpReseed`·재초기화·`TerrainCrt=1u`를 없애고 공유 저장소로 `StartPlayerSp`(Sets만) 한다. 신규 검사 `SharedSpStoreDoesNotReseedOnStartPlayerSp`(공유 사전 초기화 뒤 Sets 잡음만 160×활성 소유자임을 수계산으로 대조, SP 값 1번 1161527296 유지).
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,134건 통과(신규 1건 포함, 실패 0, NETSTORM_DATA 지정). paired 라이브 재실행(예외 PC, 0.462와 동일 인자+`steps-dn-f065-0464.tsv`): 덤프 Game=2583299428이 사전 예측 `advance^1764(seed)`와 정확히 일치했다(33+801+320+610, 중간 소비 0 증명), CRT 296677442 유지, SP offset 44 동일, 프레임 80/1679(여전히 미수렴 — 잔여는 메뉴 ambient, 0.465). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 산출물은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(제품 변경은 dotnetpj 새 경로·검사만). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 메뉴 ambient 시뮬레이션과 K 탐색·수렴 검증(0.465), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.463 — reseed 시점 차이 근본 원인과 수정 방향 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 일흔 번째 단위. 0.462 잔여(테이블 내용 직접 대조)를 확정한다. 제품 코드 변경 없음(조사·검증 단위).
- 테이블·난수 알고리즘 동등성 확정(정적 대조, 실행 없음): ① 프레임 표 파서 동등 — dn Core `RiftTypeDefinition`(토큰 렉서·선언 순서 보존·중복/플래그/GIF 순서 유지, `ReadFrameCode` 문자·변형·번호 분리, 7개 플래그 비트+base/gump/default/help 특수 프레임, `FrameTable()`=클러스터 순서 그대로)이 cpp `o/TypeParser.cpp`(`ReadFrameCode`·`Parse` 135~204행)와 일치한다. 양쪽 `Letter` 판정(A-Z·a-z·`_`)도 같다. ② `GameRandom::Next` 동일 — cpp `o/SpStore.cpp` 20~25행(0이면 `0x0BAD0BAD`, `state*0x10003+3`, 상위 16비트 `% limit`) vs dn `RawGameRandom.cs` 31~50행. 따라서 +6 오프셋·섬 전멸 패턴은 테이블·난수 알고리즘 탓이 아니다.
- reseed 시점 차이 확정(원본 충실도 판정 포함): dn은 미션 시작 때마다 reseed+SP 재초기화(`ClientHostGame` 272~274행, 0.395 도입)하지만 cpp는 클라이언트 시작 때 1회만 SP 초기화하고(`ClientMain` 349행, 원본 004391ed) 미션 Load에서는 공유 SP 경로를 다시 쓰지 않는다(`RawCampaignSession` 1090행은 독립 세션 전용). cpp `--mission`도 메뉴를 경유하고(`ClientMain` 556행 `UberGump::StartMission`), dn `--mission`은 메뉴를 우회하며, cpp ambient는 메뉴 포함 매 프레임 공유 난수를 소비하고(601~602행) dn 호스트에는 ambient 호출이 없다. 양쪽 미션 스트림은 reseed 정책·진입 경로·ambient 유무가 모두 달라 필연적으로 갈린다. 원본을 복원한 cpp가 기준이므로(004391ed) dn이 맞춰야 한다. CRT parity(296677442 양쪽 일치)와 모순되지 않는다: 양쪽이 같은 고정 시각으로 reseed하고 그 뒤 CRT 소비(Variants 99회 등)가 같으면 reseed 지점이 달라도 덤프 CRT가 일치한다.
- 수정 방향(0.464 구현): dn도 호스트 시작 때 1회만 reseed+SP 초기화(cpp 349행 대응)하고, 메뉴 ambient 차이(매 프레임 Wind·기한 Sound)는 메뉴 경로 연결(R3/R6) 또는 동일 메뉴 프레임 수 시뮬레이션으로 해소한 뒤 paired 재대조로 프레임 수렴을 확인한다.
- 검증: 제품 코드 무변경이라 새 집중 검사 없음(0.453 기준선 유지). 정적 대조만 수행했으므로 게임 실행 없음. `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: reseed-1회 수정과 paired 재대조(0.464), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.462 — SID 단위 per-cell 프레임 대조와 발산 위치 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순아홉 번째 단위. 0.461 잔여(SID 단위 per-cell 대조)를 확정한다. 제품 코드 변경 없음(조사·검증 단위, `compare_slots.py`는 Git 제외 `extracted/`에만 둔다).
- paired 라이브 대조(예외 PC): 깨끗한 빌드(경고 0·오류 0) 뒤 개발 호스트로 `--mission thewarbegins --handle Youpson --window --frames 90 --fixed-frame-ms 25 --fixed-unix-seconds 1760000000 --mute-audio`+`--ui-script steps-dn-f065.tsv`(65프레임 session-dump)를 실행(종료 코드 0)해 새 dn 기준선 `thewarbegins-dn-f065.tsv`(1,955행·frame 65·Game 3186620043·CRT 296677442, CRT는 cpp f065와 일치)를 얻었다. `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선(`extracted/dotnet-l2-fixed/`, Git 제외)에 dn 덤프·스텝·대조 스크립트를 추가했다.
- SID 대조 결과(`slot` 행 SID 결합, stride 50B·타입 +10·프레임 DWORD +36): dn 슬롯 1,679·cpp 슬롯 1,940·공통 1,679·dn_only 0·cpp_only 261(게임플레이 객체 잔여). 타입 일치 1,526개, 불일치 153개 중 152개는 dn97 vs cpp121(농지 postprocess, R6 잔여)·1개는 dn0 vs cpp152. 프레임 일치 78개=지형 5개+프린지 72개+기타 1개이며 섬 타입 97은 1,505개 중 0개 일치·프린지 102는 168개 중 72개 일치다. 소유자 일치는 325개(예상 잔여). 불일치 표본에 +6 오프셋이 다수(141→147·149→155·33→39)라 xAA 셀의 J-run base 차이(테이블 내용) 가설이 유력하며 0.463에서 dn `WorldFrames`와 cpp 프레임 표 내용을 직접 대조한다.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 프레임 표 내용 직접 대조(0.463)와 프레임 반영·검증, Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.461 — SP 353 구성과 reseed-Generate 동기 순서 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순여덟 번째 단위. 0.460 잔여(reseed 대비 ambient 순서)를 확정한다. 제품 코드 변경 없음(조사·검증 단위, 임시 계측 없음).
- SP 353 구성 확정(정적 계수+감사 일치): `RawSpStore.Initialize` 33회(`Next(100)` 1회+`Next(32)` 32회, `RawSpStore.cs` 149~197행) + 활성 소유자당 `Set` 160회(32비트×잡음 5회, 229~255행). `HostSession.StartPlayerSp`가 활성 소유자마다 `SetPlayerSpAmount`→`store.Set` 1회씩 호출하고 첫 `Set`이 `Initialize`를 촉발하므로 합계=33+160N. 감사 353회와 정확히 일치하는 정수해는 N=2(33+320)이며 1-1 미션(인간+적 AI 1명) 구조와 일치한다. cpp `o/SpStore.cpp` 45행 `Initialize`도 같은 순서라 양쪽 구성이 같다.
- 동기 순서 확정(정적 대조): dn `ClientHostGame.StartMission` 270~383행이 BindSpReseed→StartPlayerSp→…→`worldLoad.Load()`→`GenerateOriginal`을 한 시작 흐름에 동기 실행하고 프레임 루프(`Update` 393행~)와 분리돼 있다. cpp도 `ClientMain` 349행(시작 SP)·`RawCampaignSession` 1090행(독립 세션 SP)이 적재 흐름에서 동기 초기화하고 ambient(601~602행)는 바깥 루프에서만 돈다. 따라서 reseed→SP→Generate 사이에 ambient가 낄 수 없어 0.460의 메뉴 ambient 가설은 지형 프레임 원인에서 제외된다(ambient는 적재 후 게임플레이의 SP offset 등에만 영향, R6 잔여). 프레임 발산 원인은 성장 입력·CRT 쪽으로 좁혀졌다(0.462에서 성장 입력 대조).
- 검증: Core SpStore·FortMoney·SetPlayerSp 12건 집중 통과(실패 0, NETSTORM_DATA 지정). 제품 코드 무변경이라 전체 회귀는 0.453 기준선을 유지한다. 게임 실행 없음(정적 대조 단위). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음. 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 성장 입력 대조와 프레임 반영·검증(0.462), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.460 — 후보 테이블 per-type 대조와 메뉴 소비량 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순일곱 번째 단위. 0.459 잔여(per-type 대조·메뉴 소비량)를 확정한다. 제품 코드 변경 없음(조사·검증 단위, 임시 계측 없음·`git status` 제품 트리-clean 확인).
- per-type 정적 대조(실행 없음): ① `ChooseTerrainFrame` 몸체 동일 — cpp `o/RawFrameChoose.cpp` 8~33행 vs dn `RawFrameChoose.cs` 20~82행(수집→정규화→'A'→전체 폴백, theme -1·quartile, NB23 `high<=low` 전체 선택). ② `RiftTypeFrames` 구조 동일 — Codes·FindNumber·FindMasked·FindFlags·Run(A~P)·LetterKinds(cpp `o/RiftType.h` 24~46행 vs dn `RiftTypeFrames.cs` 22~156행). ③ `FrameTable()` 양쪽 클러스터 순서 그대로 — cpp `o/TypeParser.cpp` 225~231행 vs dn `TypeFrameTable`+`HostSession.FillWorldFrames`(70+순서, `WorldFrameCount`=188·`FirstAssetTypeNumber`=70·자산 116개→70..185, 0..69·186..187 빈칸, cpp `SessionAssets` 70+block·188칸과 같은 틀). 따라서 분포 차이(dn 최대 0xd0·100종 vs cpp 최대 0x13c·162종)는 필터·구조 차이가 아니라 스트림 위치 차이 쪽으로 좁혀진다.
- 메뉴 소비량 확정(정적 계수): cpp `ClientMain.cpp` 601~602행은 메뉴 여부와 무관하게 매 프레임 `UpdateAmbientSound`+`UpdateAmbientWind`를 공유 `GameRandom`으로 소비한다. Wind=`Next(100)` 1회+10% 확률 `Next(65536)` 1회(평균 약 1.1회, `o/RawAmbientWind`·dn `RawAmbientWind.cs` 48~73행 동일). Sound=기한 통과 때만 `Next(3000)` 1회[+kind 1·2 때 `Next(20000)` 1회], 기한은 벽시각+10초 후 120초 간격(`o/RawAmbientSound.cpp` 11~23행)이라 단기 메뉴(약 90 프레임)에서는 0~1회다. dn 호스트(`Client.Host`)에는 `UpdateAmbient` 호출이 없다(grep 무일치). cpp 덤프는 메뉴 경유(ui-script click-if 5/30/60→dump 90, 약 90 메뉴 프레임) vs dn 직접 진입(`--mission`, 0 메뉴 프레임)이라 적재 전 스트림이 갈린다. 단 dn 감사에서 적재 전 801회는 reseed로 무효이므로, reseed 대비 ambient 순서(적재 전후)는 0.461 paired 라이브 대조에서 확정한다.
- 검증: Core RiftType·TypeFrame·FrameChoose·AmbientWind·RawTerrain 33건 집중 통과(실패 0, NETSTORM_DATA 지정). 제품 코드 무변경이라 전체 회귀는 0.453 기준선을 유지한다. 게임 실행 없음(정적 대조 단위). `originals/` 미접촉, 추적 `options.cfg` 변경 없음.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: reseed 대비 ambient 순서 확정과 프레임 반영·검증(0.461 paired 라이브), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.459 — 프레임 선택 대조와 메뉴 소비 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순여섯 번째 단위. 0.458 잔여(프레임 미수렴 78/1679)의 원인을 확정한다. 제품 코드 변경 없음(조사·검증 단위, 임시 계측은 원복했고 `git status` 빈 출력으로 확인).
- dn 측 감사(임시 계측, 실행 후 제거): game0=4190983114(호스트 사전 소비 801회, reseed로 무효) → SP 후=1839122055(=advance^353(seed), 수계산 일치) → Generate 전후 무변화 → 덤프=advance^(353+610)(지형 610회 소비). reseed→SP 353회→Generate→지형 610회로 완전히 닫힌다.
- cpp 측 정적 인벤토리: 세션 조립·AI 생성·로그인·플레이어 생성·PlaceAll에 공유 난수 직접 소비 없음(모두 바인딩·지연·비난수 효과). 메뉴 ambient(`ClientMain.cpp` 601~602행, UpdateAmbientSound+UpdateAmbientWind)가 메뉴 여부와 무관하게 매 프레임 공유 `GameRandom`을 소비한다 — dn 직접 진입(`--mission`, 메뉴 없음)에는 이 구간이 없어 스트림이 갈린다.
- 프레임 값 분포 대조(paired 고정시각 덤프): dn 최댓값 0xd0·100종 vs cpp 최댓값 0x13c·162종. 동일 셀 집합·동일 생성 순서·동일 스트림 가정 하에 설명 불가이므로 `RiftTypeFrames` 내용·순서 또는 `ChooseTerrainFrame` 후보 필터가 다르다(0.460에서 per-type 대조). shift 상관 ±1~±5000 무응답, f065-vs-f090 분기가 적재 시점에 이미 존재(dn 1679행 동일·cpp 1921→1941행)함을 함께 확인했다.
- 검증: 깨끗한 빌드(경고 0·오류 0) 뒤 Client Host·Dump 24건 집중 통과(실패 0, NETSTORM_DATA 지정). 게임 실행 있음(예외 PC): cpp f065/f90 + dn f065/f90 paired 덤프(종료 코드 0). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선(`extracted/dotnet-l2-fixed/`)에 f065 덤프를 추가했다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(조사 단위라 제품 코드 변경 없음). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 후보 테이블 per-type 대조와 메뉴 소비량 확정(0.460) 후 프레임 반영·검증, Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.458 — 프리-Generate 소비 목록 확정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순다섯 번째 단위. 0.457 단일화 뒤에도 남은 프레임 미수렴(78/1679)의 원인을 확정한다. 제품 코드 변경 없음(조사·검증 단위이며 임시 계측은 원복했다).
- 조사(증거): ① dn 측 감사(임시 계측, 실행 후 제거): game0=4190983114(801회 사전 소비, reseed로 무효) → SP 후=1839122055(=advance^353(seed), 수계산 일치) → Generate 전후 무변화 → 덤프=advance^963 → 지형 소비 610회. ② cpp 측 절차: SP 초기화는 클라이언트 시작 시 1회(`ClientMain.cpp` 349행, 세션 독립분기는 `RawCampaignSession.cpp` 1090행) — dn과 같은 353회. ③ shift 상관 ±1~±5000 무응답 — 단순 오프셋 아님. ④ 메뉴 ambient가 매 프레임 공유 난수를 소비(`ClientMain.cpp` 601~602행 "메뉴 여부와 무관") — dn 직접 진입에는 메뉴 단계가 없어 이 소비가 빠진다. ⑤ SP offset +6 = 게임플레이 2회전(44→50·tracking 미연결은 R6 잔여)으로 seed 절차가 아님을 확정. ⑥ CRT 동일 절차·동일 출력(296677442 양쪽) — reseed 사슬 parity의 결정적 증거. ⑦ 프레임 65 vs 90 덤프 비교(cpp f065/f090, dn f065/f90): dn 1679행 동일(정적), cpp 1921/1941행(게임플레이 진행 중) — 분기가 적재 시점에 이미 갈려 있음을 확정. 결론: streams+order+tables는 동일하고, 프리-Generate 소비 목록에 cpp 특유 항목(메뉴 ambient 등)이 있어 0.459에서 수량을 확정한다.
- 검증: 깨끗한 빌드(경고 0·오류 0) 뒤 집중 검사로 확인했다(새 코드 없음). 임시 계측의 삽입·삭제로 제품 코드가 HEAD와 동일함을 `git status` 빈 출력으로 확인했다. 게임 실행 있음(예외 PC): cpp 고정시각 paired f065/f090 + dn f065/f90 4종 덤프(종료 코드 0). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선 `extracted/dotnet-l2-fixed/`에 f065·dn f065/f90 덤프를 추가했다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정(조사 단위라 제품 코드 변경 없음). 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 메뉴 ambient 소비량 확정·선택 테이블 대조(0.459), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.457 — 난수 단일 스트림 연결 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순네 번째 단위. 0.456 결론(공유 스트림 분리 탓)에 따라 포팅 모듈의 난수를 공유 `GameRandom`으로 단일화한다. 레거시 경로는 건드리지 않는다.
- 구현: `RawSpStore`·`RawTerrainStamp`·`RawNormalTerrainWorldBinding`(Generate/StampContext/GenerateOriginal)·`RawFortRandomPlacement`·`RawFarmCluster`·`RawFortObjectLoad`·`RawFrameChoose`·`RawFortFinishOwner`·`RawIslandPopulation`·`RawPriestRegen`·`RawTerrainBitmap`·`RawFortMoneySave`·`RawAddRandom`의 `NetstormRandom`을 `GameRandom`으로 교체(알고리즘 동일: 0x10003+3·상위 16비트·0이면 0x0BAD0BAD, 근거 cpppj `RawCampaignSession.cpp` 498행 `같은 실제 Client 객체`). 호출 측 `Next(int)`→`Next((uint))`+캐스트, `Next(min,max)`→동등식으로 맞췄다. `HostSession`의 `_spRandom`·`TerrainRandom`을 삭제하고 SP 저장소·Generate에 세션 공유 `Game`을 직접 연결했다. 은퇴: `HostTerrainRandomTests`(분리 단정 2건, 대체 없이 삭제 — 공유 전환이 전제 자체를 바꾸므로 은퇴 목록에 남긴다). `NetstormRandom` 클래스에 레거시 전용 주석을 달았다.
- C# 모양 차이: `GameRandom.Next(uint)`가 `uint`를 돌려주므로 인덱스·비트 연산 자리에 `unchecked((int)...)` 캐스트를 둔다. 동작·순서는 원문과 같다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,133건+Core 포팅 111건 통과(실패 0, NETSTORM_DATA 지정). L1 fixture 대조는 그대로 통과한다(동일 알고리즘). 게임 실행 있음(예외 PC): `--fixed-unix-seconds 1760000000` paired 실행(종료 코드 0) — 덤프 구조 동일(1,955행·슬롯 1,679·SID 집합 동일·비슬롯 행 무변화), Game 행이 시드 고정값에서 전진값으로 바뀌어 공유 소비가 동작함을 확인했다(4097495276→3186620043). 프레임은 미수렴(78/1679) — 프리-Generate 소비 목록이 덜 채워졌거나 선택 테이블 차이이며, 단일 스트림 자체는 전제 조건이다(0456 덤프와 1452행 동일·227행 변경으로 결선이 실제로 먹혔음을 확인). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 프리-Generate 소비 목록 확정(0.458: Objects 이전 소비·AI 생성·로그인 잔여와 선택 테이블 대조), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.456 — 고정시각 paired 대조와 덤프 CRT 행 수정 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순세 번째 단위. 0.455 잔여(농지 121·소유자·Pop·공장체·프레임 난수)의 메커니즘 확정, 덤프 CRT 행을 실제 소비 스트림으로 수정, `--fixed-unix-seconds` paired 대조 방법론 확립.
- 조사(코드 변경 없음, 증거): ① 생성 순서·셀 집합 동일 증명 — 정렬 1673행 중 121↔97 뒤집힘 152곳만 다르고 나머지 동일, Pop 좌표 집합 양쪽 1638개 바이트 동일. 농지 121은 postprocess 배치 산물(동일 셀에 dn은 97)이며 배치 파이프라인(R6 MayPlace 공간 판정)이 필요해 0.456에서 잇지 않는다. ② 소유자는 region+1을 적재 시점에 가짐(121은 0, (0,1) 853·(1,2) 500 대 (0,0) 101·(1,0) 51). writer는 Objects/AI-setup 잔여(보드 6번). f090=f240 동일이라 프레임 후반부가 아니다. ③ CRT parity 확정 — chain(1760000000)을 손으로 재계산해 dn TerrainCrt=296677442와 cpp 덤프 CRT가 일치함을 확인(수계산: seed 4097495276·offset 44·Variants 일치). ④ Game/SP-offset/frame divergence는 단일 공유 스트림 분리가 원인 — cpp는 SP·객체·지형·AI가 같은 GameRandom을 공유하고(`RawCampaignSession.cpp` 498행) dn은 Game·SP·지형 3분리라 SP 초기화 33회 소비 등이 어긋난다. 0.457 단일화 대상(NetstormRandom≡GameRandom 알고리즘 동일). ⑤ 33회 선소비 모방 실험·스트림 shift 상호상관(delta ±1·±33·±100 모두 무의미, delta 0이 79로 최대)으로 단순 오프셋 가설을 기각했다. ⑥ Pop 차단 조건 확정 — vtable Supports(0 거짓)·SHP HashLevel dims·postPop Validate가 없어 스텁 유지. ⑦ 프레임 선택 알고리즘·성장 L1은 이미 대조됐으므로(terraingrowth-x86 108행·GrowthBytes) 차이는 소비 순서 문제다.
- 구현: `HostSessionDump` world-1 난수 행을 세션 `TerrainCrt`로(미소비 호스트 Crt 상수 1 대신, 근거 cpppj `SessionStateDump.cpp` 66·116행). 신규 검사 `HostDumpWritesWorld1SessionStreams`(세션 스트림 덤프 단정).
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client Host·Dump 69건+SessionStateDump 11건 통과(실패 0, NETSTORM_DATA 지정). CRT 행 원복 변이에서 신규 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 게임 실행 있음(예외 PC): `--fixed-unix-seconds 1760000000` paired 실행(cpp f090/f240 + dn, 종료 코드 0). 덤프 CRT 행이 양쪽 296677442로 일치하고 SP 1번 값(1161527296)도 일치하나 SP offset(50 대 44)·Game 행·프레임(79/1679)은 갈린다. 비슬롯 행은 0.455와 동일(프레임 지터 1행 제외). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 되돌렸다. paired 기준선은 `extracted/dotnet-l2-fixed/`에 두었다(Git 제외).
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 난수 단일 스트림 연결(0.457), Pop·농지 배치·공장체·소유자 라이브 연결, Objects·장부·그래프, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.455 — Generate 라이브 연결과 첫 덤프 대조 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순두 번째 단위. Load.Generate 갈고리→`GenerateOriginal` 라이브 연결과 world-1 덤프 첫 대조를 한다. 공간 Pop·군집 배치·공장 생성자는 다음 단위다.
- 구현: `RawGameAuxState.PuzzlePieceType=107` 추가(근거 cpppj `RawCampaignSession.cpp` 621~622행 `localPiece`). `HostSession`에 세션 소유 `TerrainBookkeeping`+`BuildTerrainStampHooks`(실제 Create·SetOwner·SetRegion·ChunkRegion, Pop은 공간 등록 미연결로 기록만, 근거 `RawTerrainWorldBinding.cpp` 8~22행) 추가. `BuildWorldLoad`의 조각·지역없음 stub(`() => 0u`·`() => 0`)을 세션 실제값(`GameAux.PuzzlePieceType`·`TerrainGlobals.NoRegion`, 근거 cpppj 621·628행)으로 교체. `ClientHostGame.StartMission`에 Territory 실바이트·진단 결선·Generate 라이브 어댑터 연결(농지 place는 기록만, 섬 0개라 호출 없음). dryrun 기대값 `generate:0`→`generate:127` 갱신(의도적 실제값 결선 변경, 근사 수정 아님). 신규 검사 `HostTerrainBindingTests.GenerateLiveWritesTerrainSlotsDeterministically`(실슬롯 바이트·진단 무발화·결정성).
- C# 모양 차이: 원문의 공장 Create·소유자 분배·지역 지정을 세션 소유 풀·장부·타입 표로 직접 잇는다. Pop 공간 등록은 잇지 않는다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 Client 전체 1,134건+Core 지형 52건 통과(실패 0, NETSTORM_DATA 지정). 생성 타입 바이트 제거 변이에서 신규 검사 1건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 게임 실행 있음(예외 PC): `--mission thewarbegins --handle Youpson` 260프레임 실행(종료 코드 0)으로 덤프가 282→1,955행이 됐다(슬롯 1,679행 출현). cpp 기준선(f090, 4,174행)과 대조: ① SID 집합 완전 일치(내 것만 0개, 기준선만 262개=서버 186+저수준 76=Objects 잔여) ② 지형 개수 일치(1,673 양쪽) ③ SetRegion WORD 일치 ④ 비슬롯 행(플레이어·동맹·SP·그래프 239/255) 무변화. 남은 차이: Pop(void·좌표·레벨·체인), 공장 생성자·vtable, 프레임 난수 parity, 농지 121 152셀·소유자 후속 지정 메커니즘(0.456 조사), 그래프 16행(RebuildGraph 잔여). `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 `--set` 저장분만 바뀌어 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Pop 공간 등록·농지 배치·공장 생성자 라이브 연결, Objects·장부·그래프 라이브 연결, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.454 — 지형 바인딩 조립과 드라이런 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순한 번째 단위. 0.453 입력 결선에 이어 바인딩 본체 조립(`BindGenerationInputs`)과 기록 효과 Generate 드라이런까지 잇는다. Load.Generate 갈고리·도장 실제 효과 연결은 다음 단위다.
- 구현: `HostSession`에 세션 소유 `TerrainFarm`·`TerrainBinding`과 `BuildTerrainBinding(types, diagnostics, islandHooks)` 추가(세션 ChunkMap·섬 런타임·지형 전역·배치·스탬프 상태·타입 표·풀·조각 상자·프레임 188·진단 묶음, 입력 바인딩 후 보관, 근거 cpppj `RawCampaignSession.cpp` 1829~1853행). `ClientHostGame.StartMission`에 드라이런 조립 연결(기록용 섬 갈고리, 덤프 무영향). 신규 검사 `HostTerrainBindingTests` 3건(조립·입력 바인딩·장치 거부, 기록 효과 일반 분기 드라이런의 도장·결정성).
- C# 모양 차이: 원문의 바인딩 지역 변수·장기 참조를 세션 소유 상태+읽기 대리자로 둔다. 진단은 호출자가 준 묶음을 복사해 가진다(바인딩 기존 모양). 드라이런 도장 효과는 검사 전용 기록 갈고리이며 시작 흐름에는 잇지 않는다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 신규 포함 집중 10건 통과(Client HostTerrain* 10, 실패 0, NETSTORM_DATA 지정). 바인딩 묶음 제거 변이에서 신규 검사 2건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 게임 실행 있음(예외 PC): `--mission thewarbegins --handle Youpson` 260프레임 실행(종료 코드 0)의 world-1 덤프가 282행이며 0.453 덤프와 바이트 동일(재실행 1회는 프레임 90·재실행 2회는 91으로 0.440의 기존 1프레임 지터, 내용 행 281행 동일)해 조립이 중립임을 확정했다. `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 `--set` 저장분만 바뀌어 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: Load.Generate 갈고리 연결 + 도장 실제 효과 라이브 연결(Objects·장부·그래프는 그 다음), brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

#### 0.453 — 지형 바인딩 입력 값 연결 + 전체 회귀 재완주 (2026-11-12, HJOW-Athlon)

- 범위: 0.4 순서 2(R4 실제 호스트·월드 조립)의 예순 번째 단위. 0.450 대조표의 입력 15종 중 새 경로에 없던 11종 가운데 R6·성장 잔여 3종을 제외한 8종(`TerrainEnabled`·밀도 3종·`ownerMode.battle`·`bridgeState.mission`·전투 옵션 5·6번·`AiDebug` checking)을 세션 소유로 결선하고, 0.4 순서 1-1의 전체 회귀를 0.453 포함으로 재완주한다.
- 구현: `RawGameAuxState`에 `TerrainEnabled`(1)·`ArchipelagoMinimum`(15)·`ArchipelagoMaximum`(35)·`ArchipelagoPercent`(0) 추가(cpppj `o/RawGameAuxState.h` 초기값). 신규 `RawWorldGeyserSpacing.Spacing`(선택 0~3 → 3·4·5·6칸, 범위 밖 거부, 근거 `o/RawWorldGeyserBounds.cpp` 0041ca10). `HostSession`에 `GameAux`·`OwnerMode`(battle=true)·`BridgeState`(CampaignLinks 타입값·battle/mission=true)·`BattleOptions`(87바이트 `RawMissionBattleOptions.Reset` 초기화)·`AiChecking` 소유와 `BuildTerrainGenerationInputs()`(Enabled·Battle·Mission·Layout·Spacing·Span·Minimum·Maximum·Percent·Archipelago·Subscriber·Growth.Checking 결선, 근거 `RawCampaignSession.cpp` 1831~1853행) 추가. `PlayerBounds`(R6)·세션 로그 0x1d(R6)·성장 assertion은 미연결(null)로 두어 `GenerateOriginal`이 명시 거부한다. 신규 검사 `HostTerrainInputsTests` 4건(기본값 소유·간격표·공유 결선·값 추종).
- C# 모양 차이: 원문의 `uint32_t&`·`bool&` 장기 참조를 세션 소유 값+읽기 대리자(`RawCheckingFlag`·`Func`)로 빌린다. `Span` 등 상자는 `GenerationInputs` 소유 상자에 값을 동기화해 바인딩의 `BindGenerationInputs` 공유 방식과 맞춘다.
- 검증: 깨끗한 전체 리빌드(경고 0·오류 0) 뒤 신규 포함 집중 52건 통과(Client HostTerrainInputs 4·주변 7 + Core 지형 37·GameAux 4, 실패 0, NETSTORM_DATA 지정). 간격표 6→7 변이에서 신규 검사 2건 실패로 검출력을 확인한 뒤 원복하고 다시 통과했다. 전체 회귀 완주(백그라운드): Assets 366·Core 1,945·Client 1,130(합계 3,441, 실패 0, 종료 코드 0, NETSTORM_DATA 지정) — 새 기준선(직전 0.432는 3,422). 게임 실행 있음(예외 PC): `--mission thewarbegins --handle Youpson` 260프레임 실행(종료 코드 0)의 world-1 덤프가 282행으로 이전 실행분과 동일해 세션 상태 추가가 중립임을 확정했다. `originals/` 변경 없음, 추적 `assets/game-data/d/options.cfg`는 `--set` 저장분만 바뀌어 되돌렸다.
- PORT_MAP, fixture 변화 없음(대응행 추가 없음). 함께 옮긴 10차 NB 수정 없음. 외부 접속 목록 변경 없음.
- 보호: originals, originalCD, AGENTS.md, README 개요, cpppj, LEFT_JOBS.md 미수정. 사제 포획/희생·특정 적 공격·시나리오 승리 재현 검사는 건너뛰었다.
- 남은 것: 바인딩(`BindGenerationInputs`) 드라이런 + Generate 라이브 연결·덤프 대조(슬롯·객체 행 출현), Objects·장부·그래프 라이브 연결, brain 실생성자·요새 이미지·메뉴 미션 시작, 34경로 L2 대조(M1, 4/34).

## 9. AGENTS 반영 기준과 이관 기록

<a id="agents-reflection"></a>

### 9.1 AGENTS.md 반영 기준 (재확인 2026-11-12, 마지막 AGENTS 변경 `85551c6c2` 이후 변경 없음)

AGENTS.md는 수정하지 않는다. dotnetpj에 닿는 사용자 변경은 모두 현재 계획에 반영했다.

| 사용자 변경 | dotnetpj 반영 |
| --- | --- |
| `ef0c6b91` 실행 파일 이름 `Netstorm.exe` | 기존 경로 0.96 적용. 결정 D2: R5에서 새 호스트로 넘기고 기존 경로는 `NetstormLegacy`로 내린 뒤 삭제 |
| `462c9a64` 11.01 표기·Credits 3항목 | 0.150 완료(새 경로·기존 경로) |
| `0190ba95` 외부 접속 문서화·유효성 검사 비활성화 | 0.151 완료, 이식 때마다 갱신(3.2) |
| `5867f35e`·`990a3541` 4차 로컬 MCP(전체 조작·캡처·소리 포함 녹화)·와이드 최대 1920×1080 | [1.3](#dotnet-r8-requirements) 4차, 결정 D3(dotnetpj에도 MCP) |
| `98b2f20c`·`aaafbbc4` 시나리오 편집 추가·5차 확장(자동 생성 파일 전부 사용자 홈, 읽기 전용 설치) | 1.3 5차·9차 |
| `851054a8`·`3051892a` 기존 파일 재활용·README 개요 보호 | 3.2 |
| `b5197d48` 알려진 버그 34건(현 10차) | 1.3 10차. 옛 0.8 7번은 아래 범위 확대로 해소(0.5 아래) |
| `551d27ae` 세 프로젝트 평가 기준 | 0.1(실행·플레이 가능 여부와 원본 동일성) |
| `75e4eed9` 글꼴 exe 내장 금지·exe 옆 `fonts/`·일본어 등 다른 글꼴 | 3.2·5절 2·7번 |
| `8136df49e`·`b5117f524`·`8661e1060` 7차 오류/예외 상세 로그 신설(로그 경로·`yyyyMMddHHmmss.log`·원인 추적 정보)과 차수 재번호, Linux `$HOME/.netstorm` | 1.3 7차(옛 번호 대응은 1.3 머리), Linux 경로는 결정 D5 |
| `22a0877c3` webpj 예외(8·9차 등) | webpj 전용이라 dotnetpj 변화 없음 |
| `60226ea93` 11차 난이도·12차 스커미시·13차 기타 언어, dotnetpj 범위 **4~13차**, `Copyright 2026 HJOW` | 1.3 11~13차, 0.4 1-3 카피라이트 |
| `610670750` cpppj 플랫폼 명시 | cpppj 전용 |
| `85551c6c2` 14차 QuickJS 추가, dotnetpj 범위 **4~14차**, webpj 예외 변경(4차 WebMCP·5차 localStorage·14차 불필요·12차 대상) | 1.3 14차(cpppj 1301 탑재 계약을 같은 계약으로), 0.2 R8·0.4 7단계 범위. webpj 예외 변경은 dotnetpj 변화 없음 |

<a id="dotnet-archive-log"></a>

### 9.2 이관 기록 — LEFT_JOBS.dotnetpj.old.md

**2026-11-12 이관(HJOW-Athlon, 문서만, 사용자 요청):** 단위 기록 0.342~0.452와 0.2~0.4의 단위별 나열을 정리했다. 정리 직전(`315be9a59 / 20261112-72 DN`) 전체 원문을 [보관 파일 끝](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261112-through-0452)에 그대로 보관했다(원문 SHA-256 `654c0ee6c07e42b3059e7f644bf01fc5e4e8011a9250b3d775efe456a7d9b2ac`, 보관본을 LF로 되돌려 원문과 바이트 일치 확인). 보관 파일 머리의 보관본 목록에 새 링크를 더했다.

- **본문에서 뺀 것:** 단위 기록 0.342~0.452(111건) 전체, 0.3 '검사' 행과 0.4 1-1 행의 단위별 집중 검사 나열(0.345~0.452), 0.4 순서 2 행의 단위별 진행 이력(0.390~0.452)과 순서 3 행의 L4a 장면별 경과(0.380~0.389), 0.3 PORT_MAP 행의 행별 `base` 갱신 이력, 0.2 R4 행의 단위 나열, 0.4 1-3·1-4 행(완료 참고로 줄임), 9.2의 지난 11-11 이관 3건 상세(아래 요약).
- **새로 넣은 것:** 0.1 한 줄 요약, [0.4 R4 진행판](#r4-board)(묶음별 연결 현황·cpp 기준선·검증 방법·남은 조각 순서), 0.3의 전체 회귀 주기 도래 표시, 0.4 1-2의 남은 9건 목록, 7.1 색인 3행.
- **바로잡은 것:** cpppj 기준 제품 1301 → 1303, 14차 'cpppj 미착수' 표기(1.1·1.3·7.2·9.1) → 1301 엔진 탑재·연동 범위 D7 대기, PORT_MAP 행 수(691줄 → 688행 재집계, 0.394 `Player.cpp/.h` 반영), 0.2 R0·R8 행(재검토 종결·앞서 이식한 요구사항 코드), 1.2에 1302·1303 정적 확인 추가, 2.4·2.5의 낡은 진행 문장.
- 빌드·검사·게임 실행·디컴파일은 하지 않았다. `LEFT_JOBS.md`·cpppj·AGENTS.md는 수정하지 않았다.

**2026-11-11 이관 3건(요약, 원문은 [보관 파일 0.341까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261111-through-0341)):** ① 1300 반영 정리 — 단위 기록 0.280~0.341을 빼고 0.4 남은 작업 순서표·0.5 결정 표·1절 cpppj 마감분 반영·0.1 이식 범위 집계·9.1 AGENTS 반영 표를 새로 만들었다(원문 SHA-256 `698670bf211d9ec89d63ae2ffca976e9c28e87dd52c74288fb1b2ef58d785209`). ② 같은 날 결정 반영 — 사용자가 D1~D5를 정했고(D1·D3~D5 권고안, D2 "새 경로가 이름을 이어받고 작업이 끝나면 기존 경로 제거") 0.5와 관련 문장을 고쳤다. 전환 시점(R5)과 기존 경로 삭제 시점(R6)은 이 결정에 대한 AI 해석이다. ③ 같은 날 AGENTS 14차 반영 — `85551c6c2`(dotnetpj 범위 4~14차)를 머리말·0.2·0.4·1.1·1.3·3.2·7.2·9.1에 반영했다.

**이전 이관(요약, 상세는 각 보관본 머리):** [2026-11-10 — 0.279까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261110-through-0279)(HJOW-Athlon, AGENTS 8차 시나리오 편집·5차 확장·9차 알려진 버그·글꼴 외부 배포·평가 기준 반영), [2026-11-04 — 0.154까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261104-through-0154)(HJOW-Athlon), [2026-11-02 — 0.126까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261102-through-0126)(HJOW-Athlon), [2026-10-31 — 0.73까지](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261031-through-073)(HJOW-X3D), 2026-10-28 재수립 이관(VM-W11-Codex: 2026-10-22 이전 계획, 날짜별 완료·인계 절, 옛 0~11·14절).

**앵커:** 2026-10-28에 옮긴 절의 `<a id>`(`dotnet-current-plan-20261022`·`sid-session-plan`·`legacy-dotnet-history`·`legacy-dotnet-031`~`107` 등)는 보관 파일에 같은 이름으로 남아 있다. `LEFT_JOBS.old.md`·`LEFT_JOBS.webpj.md`·`docs/`의 기존 링크가 이 안내로 오도록 같은 이름의 빈 앵커를 바로 아래에 둔다. 그런 링크로 들어왔다면 보관 파일의 같은 앵커를 본다. 옛 절 제목 앵커(`04-…`·`05-…`·`07-…`·`08-…`·`09-…`)는 이 문서의 대응 절 앞에 남겼다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
