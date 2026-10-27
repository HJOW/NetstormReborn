# LEFT_JOBS — cpppj 및 공통 인수인계

이 문서는 cpppj와 공통 사항의 **현재 상태, 앞으로 할 작업, 유효한 사용자 결정**을 담는다. 2026-10-27 단위504 완료 기준으로 정리했다. 완료된 단위별 상세·옛 재개 지시·중복 검증 기록은 [과거 이력](LEFT_JOBS.old.md), 이번 정리 직전 원문은 [단위504까지 추가 보관](LEFT_JOBS.old.md#archive-20261027-through-504)을 본다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)를 따른다.

## 0. 재개 전 확인 사항

- **개발 목표는 단위504 인수인계·커밋 후 사용자 요청으로 일시 중지한 상태다.** 이번 요청은 문서 정리이며 다음 개발 단위를 시작하지 않는다. 마지막 제품 커밋은 `98c3aea8 / 20261027-39 CP`, 사용자 목표 변경 커밋은 `2f2b5a46`이다.
- 개발 재개 시 [AGENTS.md](AGENTS.md), 이 문서, [닷넷 인수인계](LEFT_JOBS.dotnetpj.md), [TODO.md](TODO.md)를 다시 읽는다. **첫 작업은 현재 최종 게임의 전체34경로에 대한 짧은 진입 재검증**이다. 502의32경로 성공·503의bc5 성공·504의bc8 성공은 서로 다른 제품의 결과이므로 최종 제품의34경로 전체 성공으로 합산하지 않는다.
- **마지막 전체 정밀 디컴파일: HJOW-Athlon (192.168.0.94) / 2026-10-23.** 결과는 `extracted/refined/`(Git 제외)에 있다. cpppj 코드 작업·디컴파일 소스 분석 전 현재 PC가 다르면 AGENTS.md에 따라 다시 디컴파일한다. 이번 문서 정리에서는 산출물을 변경하지 않았다.
- 재디컴파일 절차는 `tools\ghidra\refine_all.ps1`이다. 힌트 생성이 cp949 오류로 멈추면 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`을 설정하고 재실행한다.
- 실제 게임/별도 창 실행은 AGENTS.md의 확인·예외 규칙을 따른다. HJOW-Athlon·VM-W11-Codex·vm-debian-codex는 예외 시스템이다. **자연 완주·사용자 개입 검증과 `.english` 누락 개선은 계속 보류**하며, 메뉴 표시·진입·수초 진행의 짧은 검증은 허용된 범위다.
- 빌드·검사·GUI 원시 로그와 드라이버는 작업 PC의 `extracted/`(Git 제외)에 있다. 커밋되는 보고서는 `cpppj/*.json`, 작업 문서는 `docs/exe/cpp-*.md`다. 다른 PC에 원시 자료가 있다고 가정하지 않는다.

## 1. 현행 목표 ([TODO.md](TODO.md))

- Early Missions 6개·Priest Training 9개·캠페인 전체16개·데모3개를 사용자가 플레이할 수 있고, 원본10.78과 완전히 같게 동작하도록 한다.
- Edit 메뉴의 커스텀 맵/시나리오, 메인 메뉴의 비멀티 항목, 좌측 덱·미니맵·다리 단축키 QWASZX·ESC 메뉴를 구현한다.
- Windows 10/11 호환성·전체화면 DirectDraw 오류와 종료 후 재실행 오류 해결도 현재 목표다. 원본과 다른 호환성 처리와 이유는 한국어 코드 주석에 명시한다. 전체화면·4:3/16:9/16:10·가장자리 스크롤 요구도 유지한다.
- 위 싱글플레이·UI·호환성 작업 뒤 **TCP/IP 로컬 멀티플레이·Outpust를 사전 구현**한다. 사용자 결정에 따라 이 단계는 기존 프로그램 디컴파일만 참고하고 신규 단위 검사·게임 실행·실제 통신 테스트 없이 작성한다. 사전 구현과 검증 완료를 구별한다.
- 개발 작업 단위마다 인수인계를 갱신하고 커밋한다. 커밋에 실패해도 작업은 이어 간다. 원격 push는 하지 않는다.

대상은 원본 `d/offical1~5.english`의 MissionBegin 순서와 `tell.DEMOVILLE`의 데모 순서다.

| 구분 | 미션 키 (원본 표시 이름) |
| --- | --- |
| Early Missions (offical1) | Tutorial1 (Bridge the Gap), Tutorial2 (Secret Workshop), Tutorial3 (Capture The Priest), Tutorial4 (Tactical Combat), Tutorial5 (Subtle Defense), Tutorial6 (Raw Power) |
| Priest Training (offical2) | bc1menu76, bc2bridging76, bc3acid_bar76, bc4bow_rush76, bc5walkers76, bc6mows76, bc7sun_bars76, bc8fury76, bc9ssun_bars76 |
| 캠페인 1장 (offical3) | TheWarBegins, MasterOfWhirligigs, SaveTheIsland, FragileFortune, ThunderingPower, DissolvedAlliance |
| 캠페인 2장 (offical4) | TheNoose, RainVsRain, RunForIt, GuardMyBack, Surrounded |
| 캠페인 3장 (offical5) | BreakingThrough, ToTheRescue, Vicious, EnemyTerritory, FinalConfrontation |
| 데모 | DemoStormRages (The Storm Rages), GuardMyBack (Demo of "Guard My Back"), DissolvedAlliance (Demo of "Dissolved Alliance") |

일반31경로(Early6 + Training9 + 캠페인16)와 데모3개를 합쳐34경로다. 같은 자원을 쓰는 GuardMyBack·DissolvedAlliance도 일반/데모를 따로 검증한다([대응 근거](docs/gameplay/demo-campaign-map.md)).

## 2. 현재 상태 (2026-10-27 기준)

### 2.1 마지막 통합 검증 기준

| 근거 | 확인한 범위와 결과 | 현재 판정의 한계 |
| --- | --- | --- |
| **504 / 최종 제품** | Release build1/2 경고·오류0, 집중37개/실패0, 전체C++ **3,334개/실패0**, CTest1개/실패0(361.99초). 게임 SHA `a284cb011db23dc04697bed8315e2df885d1bb913b726b0b4d77938e03095683` | 같은 게임의 bc8 짧은 실제 재생만 확인했으며 전체34경로는 재검증 전이다. |
| **504 / bc8fury76 수정** | ProcessFinder의 원본 타입10~62 predicate 복원. 원문64입력·두 정밀도128정상 반환/차이0. 실제179프레임·game24.012초·정상Start/Session·비정지·진단6·종료0 | 손상 목록·실제 프로세스 부재의 오류는 유지한다. CD 제거 경로는 명시적 미복원이다. [문서](docs/exe/cpp-process-removal-filter-20261027.md)·[보고서](cpppj/process-removal-filter-runtime-20261027.json) |
| **503 / bc5walkers76 수정** | Burning의 판본별 조회 순서 복원(10.78 픽셀 우선/CD 행 우선). 전체C++3,332개/실패0. 실제179프레임·game12.328초·정상Start/Session·종료0 | 이전 game9.812초의 실패 지점을 넘었으나 504 제품에서 다시 확인한 결과는 아니다. CD 추가 접근 바이트는 명시 입력이며 실제 caller 힙 안전성 복원이 아니다. [문서](docs/exe/cpp-burning-corner-20261027.md)·[보고서](cpppj/burning-corner-runtime-20261027.json) |
| **502 / 마지막 전체 진입 기준선** | 501 게임 `d17d9c2b…`에서 **32/34경로 성공**(Early6·Training7·캠페인16·데모3), bc5/bc8은 실제 오류. 성공 경로는 실제 gameTime 증가·Start/Session·전체화면 확인 | 실제37실행에는 데모 입력 시점 도구실패3회가 포함된다. 두 제품 오류는 이후503/504에서 각각 수정했다. [문서](docs/exe/cpp-solo-entry-baseline-20261027.md)·[보고서](cpppj/solo-entry-baseline-20261027.json) |
| **495·500·501 / 화면·입력** | Windows10 DIB 호환 전체화면, 실제 가장자리/카메라, 세 화면비 해상도 메뉴·원본 SETSIZE 경고·저장/종료/새 프로세스 적용을 짧은 실제 실행으로 확인 | Windows11·다른 모니터/DPI·실제 앱 포커스 전환·전체 UI 동일성은 미검증이다. |
| **492~494 / Edit 실제 흐름** | TEST01 Edit→Test Battle→짧은 진행→Return to Editing(365프레임/14.316초), 새 맵 생성·기존 맵 섬 추가·저장·재진입/프로세스 재시작 확인 | 원본50개 목록 상한에 맞춘 분리 자산 구성이다. 전체 편집 도구·시나리오·자연 플레이·원본 외관/음향 동일성의 완료 증거는 아니다. |

단위504의 기존 검사·fixture·AGENTS/원본 보호와 첫 실패 보존 내역은 해당 보고서에 있다. CTest wrapper1개와 내부 C++3,334개를 구별한다. **단위/집중 검사·짧은 진입·자연 완주·전체 동일성은 각각 별도 판정**한다.

### 2.2 미션별 상태

아래 장기 플레이 증거는 과거 해당 제품에서 얻은 것이며 최종504 제품의 자연 완주 회귀 결과가 아니다. 자연 완주·사용자 개입 검증은 보류 중이다.

| 대상 | 확보한 증거 | 남은 확인 |
| --- | --- | --- |
| Early1 | 북서 간헐천 경로 A~G, 채취/귀환·SP0→600·Tutorial2 진입 | 남동 경로 실패 원인, 실패/재시작/F8, 화면·음향 |
| Early2 | A~I, 신전·워크샾·지식·생산·발사기·회수(환불75), Tutorial3 진입. 마지막 자연 완주 회귀는467 제품 | 재시도·취소, 화면·음향 |
| Early3~6 | 진입/Start·단계 판정·짧은 진행. Early3은468 Source1에서 채취/귀환·SP2000→2200→2400·A→B→C, Early5는 과거 F(오벨리스크 획득)까지 | 각 자연 완주·다음 미션. Early3은 최종468 제품에서 완주한 기록이 아니며 attempt4의 종료1도 보존 |
| 캠페인 1-1 | 자동 AI 세션에서 실제 캐논으로 적 신전HP5000→0(322) | 적 사제 포획/운반/희생·자연 승리·1-2 진입·패배/Retry |
| 캠페인 1-2~1-6 | 진입/Start·자동AI/Kernel400프레임(321), 502에서 짧은 진입 재확인 | 자연 플레이·승패·후속 명령 |
| 캠페인 2·3장 | 502에서 전16개 캠페인 진입/Start·짧은 진행 확인 | 추가 TYPE 전체 수명·자연 플레이 |
| Priest Training9개 | 502에서7개 성공, 실패한bc5/bc8은503/504에서 각각 수정 후 짧은 재생 성공 | 최종 제품에서9개 재검증, 각 자연 완주·완료 안내/메뉴 복귀·bc8 원소 선택지 |
| 데모3개 | 502에서 실제 메뉴·15초 안내timeout·로컬AI 자동 진행 진입 | 최종 제품 재검증, `demoRestartTimer=360` 만료/반복·정리·안내 분기 |

Early3 후속 입력과 미실행 계획은 [부분 진행 보관](docs/exe/prepared/tutorial3-partial-468-20261027/ATTEMPT4-NEXT-C-H.md)을 본다. 다음 워크샾 메뉴의71/75 생산·공급·사제 무력화/포획은 미완료다.

### 2.3 기능 영역별 상태

- **메인 메뉴:** 원본8버튼과 Campaign/Demo/Help/Options, Credits18쪽, Quit, Tips/기본 Native3 복귀를 연결·확인했다(466~468). 승패·Intro39·기존 저장상태 복구·미션 이탈 전체 선택과 픽셀/음향은 남는다. Multiplayer는 앞 목표 뒤 사전 구현한다.
- **Edit:** 기존 맵 자연 State1 설치·전체 저장/Save as·새 이름 목록/재시작·Set All Bridge·SaveGoMain·신규 Fort/자동 섬 창·기존/새 맵 섬 추가를 연결했다. Test Battle의129 발사·130 투사체/반사·냉각 연기를 복원해492에서 짧은 전투 후 편집 복귀까지 확인했다. 남은 것은3절의 도구·시나리오·수명·원본 동일성이다.
- **게임 중 UI:** QWASZX 취득/회전/배치/보충(366·452), 원본 ESC 가로/하위 메뉴(369), 실제 HUD 루트·도킹/덱·미니맵/NSInfo/Chat(367·368·417·418)을 연결했다. 임시 Game 버튼은 제거했으며 ESC 소비 회귀는459에서 수정했다. 가장자리/카메라 실제 연결은500, 세 화면비 메뉴는501에서 확인했다. 전체 View/zoom·도킹/Chat 입력·미니맵 토글·픽셀 대조는 남는다.
- **공통 전략 AI:** 적·아군·데모가 같은 원본 전략을 쓰고 `RawCampaignSession::RunFrame`의 현재 시각→비정지AI→Kernel 순서에 연결된다(320~322). 정상 간격의 확장/채취/생산/공격을 자연 플레이로 대조하지 않았다.
- **유닛·주문·의식:** 워크샾25종의 생성/이동/공격/표시·소유자색(335~345), Devastation·포획/운반/희생·SP/미션완료, 은신/마비·윈드 소환을 복원했다. 의식 집중13검사에는 명시 시전원/포로/시계 입력이 포함됐으며 실제 UI 취득→시전→포획→희생의 자연 플레이는 미검증이다.
- **내부 수명:** Client/Session의 장기 전역·Pool/Graph/Player·Deck·권한·커서·반납 조정기와 cleanup, 기존 맵 Edit 설치, Guide/World 분리·작업본 reader·Login 재적재를 연결했다(425~486). 실제 잔여 State1/State7·연결/송신·파생 수명은3절에 남긴다.
- **배포:** [cpppj/package-cpppj.ps1](cpppj/package-cpppj.ps1)은 작성됐다(359). `-NoBuild` 없는 자동 빌드→묶기 분기는 실제 검증 전이다.

### 2.4 알려진 미해결 이슈

- **Early1 남동 경로:** D·SP0에서 Next Tutorial을 찾지 못해 종료1(417). 북서 경로는 통과하며 실패 원인은 미확정이다.
- **options.cfg 저장/복구:** GUI 음량 변경의 SaveOptions fopen errno22/doserrno1224(393), 도구 설정 열기/복구의 간헐적 OSError22(369·386). 격리 복사본은 통과했으며 H 볼륨 exFAT와의 인과는 확정하지 않았다.
- **옛 스모크6개:** `cpp_menu_smoke`, `cpp_rawbuttons_smoke`, `cpp_thunder_smoke`, `cpp_weather_smoke`, `cpp_audio_smoke`, `check_dialog_controls.check_mission`은 제거된 임시 Game 버튼을 전제로 한다. 재사용 전에 ESC→실제 메뉴 입력으로 바꾼다.
- **PlayerNoMake:** 경고255DWORD/정리004328a0의 장기 연결은453에서 완료했으나 패킷 부모00433c30의 실제 writer는 남았다. CD00487900은 spot 정리이므로 같은 함수로 취급하지 않는다.
- **섬 추가 후 Graph:** 배치 직후 Graph1/연결 표면694→재로드0 차이의 원본 전체 월드 대조는 미완료다(493~494). 저장 seed/Money/Deck의 난수 변화와 배치 레코드 보존을 구별한다.

## 3. 앞으로 할 작업 (권장 순서)

### 1. 재개 첫 작업 — 현재 제품의 짧은 진입 기준선

- [ ] 504 최종 게임과 자산/설정·실행 파일SHA를 확인하고34경로를 짧게 재검증한다. 정상 종료뿐 아니라 정확한 미션/데모 구분·Start/Session·비정지·실제 gameTime 증가를 확인한다. 502의 최초 오류/도구실패 로그와503/504의 수정 근거를 보존한다.
- [ ] 실제 오류가 나오면 해당 원본 몸체/호출 순서를 대조해 복원한다. guard 제거·배열 임의 확장·손상 후보 무조건 건너뛰기·진단6 강제·임시 속도로 숨기지 않는다.
- [ ] 이후 DD 조정UI·미니맵/덱/QWASZX/ESC·편집/시나리오의 남은 기능과 짧은 실제 입력을 진행한다. 완주/사용자 개입 검증은 보류를 유지한다.

### 2. Edit 메뉴·커스텀 맵/시나리오

- [ ] 남은 지형·오브젝트·소유자 도구와 LeaveEditMode/State7 전체 분기를 점검한다. 새 맵/섬 추가·저장/재진입·Test Battle 짧은 왕복은492~494에서 확인했으므로 미구현으로 되돌려 적지 않는다.
- [ ] Graph 재로드 차이를 원본 전체 월드와 대조하고, 전체 작업본 저장/재파싱·이름/목록·미션 전환 수명을 확인한다. 원문50개 목록 상한은 유지한다.
- [ ] 언어별 시나리오 파일의 사용자/AI·시작SP·브리핑을 실제 경로에서 대조한다. `originals/d/TEST01.fort`·`TEST01.english`와 TEST02는 읽기 참조로만 쓰고 저장은 작업본 경로에 한다.
- **판본 차이 유지:** 494에서 원본10.78 기본 설명에도 `{Text}` 리터럴이 남음을 확인했다. 임의로 제거하지 않는다. 10.72 자기 자료에는 이 표기가 없다.
- **언어 파일 부재 개선은 보류:** 대응 `.english`가 없을 때 Test Battle 종료는 원본 동작이다. 파일이 있는 TEST01 등으로 연결을 검증하고 임의 기본 시나리오로 대체하지 않는다.

### 3. 미션 수명·진행 저장·데모 반복

- [ ] 완료값0→1 디스크 저장·재실행 후 다음 미션 해제를 확인한다. 과거 회귀는 기존 완료값1을 보존한 조건이므로 신규 해제의 증거가 아니다.
- [ ] Retry/Restart·Next Tutorial/Next Mission·Abort/메뉴 복귀와 연속 전환의 참조/타이머·다리/AI 재고를 확인한다. 자연 승패와 사용자 개입이 필요한 부분은 보류 목록으로 유지한다.
- [ ] 데모3개의360초 만료→반복/정리와 안내 분기를 확인한다. GuardMyBack의 Demo15초 안내·DissolvedAlliance의 Demo 섹션 부재를 유지하고 Script `demo` 키만으로 실행 모드를 추정하지 않는다.
- [ ] 존재하는 startup 저장상태 복구·Intro39/승패를 포함한 전체 Native3 선택, 다른 State1과 비동기 State7/전투 정리(`004b2df0`)의 자연 Tick 경계를 연결한다.

### 4. 남은 내부 수명·실제 연결

- [ ] 캠페인 이탈→Edit의 mission/모드 수명, 일반 전투saveBattle·metamap/비mission phase14 저장·파생 프로세스 수명.
- [ ] 실제 Frame의 BridgeDecayScan 순회. 460의 장기 저장소/reset·원문 대조 완료를 프레임 실행 완료로 합산하지 않는다.
- [ ] 남은 카메라 전체 Frame/zoom·Renderer `004994b0` flags/override/dirty/시차 경계. 498→499의 Present 목표 제한/현재 카메라 적용과 Guide 뒤496 입력은500에서 실제 연결됐다. 이를 통째로 미연결이라고 쓰지 않는다.
- [ ] 연결활동 frame22 clear·주기64 송신/timeout/UI·endpoint 삭제·server/boss 전체 owner, 실제 리슨/수락·별도22칸 송신버퍼 writer.
- [ ] Chat manager/recipient·F3/클립보드/전송·GUI 드래그, 전체 팔레트 전환·디렉터리 이름 formatter·PlayerNoMake writer.
- [ ] 플로터TYPE146 회피와 반사 Generate 변위0의 미초기화 지역점 등 명시 미복원 경계를 현재 소스에서 재확인한다(490~492). 원문 Point 출력/난수 대조와 실제 Generate 전체 연결을 구별한다.
- 근거: [월드 장기 소유 계획](docs/exe/cpp-state-world-owner-plan-20261025.md), [State Client 공백](docs/exe/cpp-state-cleanup-client-gap-20261025.md), [직접 전역 소유/반납](docs/exe/cpp-state-cleanup-global-owners-20261025.md). 오래된 PLAY-1/2/5 목록은 후속 해결 여부를 확인한 뒤 사용한다.

### 5. 원본 동일성 대조

- [ ] 목표 TYPE의 대기/건설/발사/피격/파괴/포획/희생 프레임·방향·색표·그림자·발 기준점·가림 순서·파편/지형 효과. 캠페인2·3장 추가 TYPE의 전체 수명도 확인한다.
- [ ] 메뉴·브리핑·생산/지식·선택/사거리/비용·커서/회전/회수/업그레이드·우클릭/전체 단축키·Pause/Help·카메라/스크롤·도킹/View/미니맵 독립 토글·Chat/PlayerList.
- [ ] 건설/발사/적중/출격/귀환/연소/붕괴/포획/의식/승패/음악의 파일·시점·반복·위치, 정지 중 시간·난수 소비. 정상 간격의 공통 AI 행동도 별도 대조한다.
- [ ] 장시간 플레이·메뉴/데모 반복의 크래시/정지/자원 누적. 완주/사용자 개입이 필요한 검증은 보류한다.
- 녹화의 HDR 밝기·방송 음성을 게임 효과로 오인하지 않는다. 표시 변환의 픽셀 일치와 전체 게임 외관 동일성을 구별한다.

### 6. 자연 완주·테스트 지원 — 계속 보류

- [ ] 보류 해제 후 Early3→6, 캠페인1-1의 포획→운반→희생/승패→1-2, 나머지1장→2장→3장, Priest Training9개/선택지 순서로 넓힌다. 2.2절의 부분 진행과 미검증 경계를 따른다.
- [ ] TEST02용 명시 로컬owner AI 옵션(`--test-local-ai original` 등)은 아직 미구현이다. 일반demo0에서 원본 공통AI를 로컬owner에도 실행하며 `gate.demo=1` 전환은 쓰지 않는다(튜토리얼 생략·Demo 사건·종료타이머도 바뀌기 때문).
- 로컬AI의 StartMoney/Tech/이름/Off/NoTemple/동맹/능력은 적AI 설정과 따로 준비하고 공유 난수 소비 차이를 기록한다. 기본 게임·적AI·저장 프로필에 섞지 않으며 테스트 조작임을 주석/기록에 표시한다. 얻은 결과는 사람이 직접 입력한 자연 플레이와 구별한다.
- [ ] **TEST02 포획/희생은 보류 중:** 사용자 결정대로 위 옵션으로 아군AI가 플레이하게 한다. 사용자 수정판 `3c12b4e7`/Fort5,282BYTE의 현재 Fort·언어파일SHA/배치를 다시 확인하고 옛 좌표를 재사용하지 않는다. 기술은 크로스보우·썬 디스크 발사기·맨오워풀·데빌메이커 우선, 훨리베이스 제외 지시를 따른다.
- 우리 편AI/행동빈도 증가/시작SP 추가는 [가능성 검토](docs/exe/cpp-test-local-ai-and-sp-feasibility-20261021.md)만 했으며 구현 완료로 기록하지 않는다.

### 7. Windows 10/11 호환성과 전체화면

- [ ] Windows11·다른 모니터/DPI·실제 다른 앱 포커스 전환, DD 조정/확인UI의 남은 경계를 검증한다. 검증한 OS와 미검증 OS를 구별한다.
- 495의 Windows10 호환 출력/입력 역변환·커서 제한/반납·조정Cancel/Accept/15초 복구, 500의 가장자리/카메라, 501의4:3/16:9/16:10 메뉴·경고/저장/종료/재실행은 짧은 실제 확인을 마쳤다. 메시지로 주입한 활성/비활성을 실제 앱 포커스 전환으로 합산하지 않는다.
- [ ] 전체화면 종료 후 재실행 안정성과 화면비/스크롤 회귀를 유지하고, 원본과 다른 처리는 이유를 한국어 코드 주석에 명시한다.
- 근거: [호환 전체화면](docs/exe/cpp-modern-fullscreen-20261027.md), [가장자리 실제 연결](docs/exe/cpp-edge-scroll-runtime-20261027.md), [해상도 메뉴](docs/exe/cpp-screen-resolution-menu-20261027.md).

### 8. 기타 정리와 후속 단계

- [ ] 2.4절의 미해결 이슈·옛 스모크 입력을 처리한다.
- [ ] Credits의 nsstart 배경·20초 자동롤·픽셀/음향을 대조한다. Patch4쪽/Original14쪽·More/Back/Cancel/최종OK·메인복귀/Quit은466에서 확인했다([문서](docs/exe/cpp-main-quit-credits-20261026.md)).
- [ ] 최신 소스 PC에서 배포 스크립트를 `-NoBuild` 없이 실행해 빌드→묶기와 `BUILD_INFO.txt` 실행파일 시각/SHA를 확인한다.
- 국·영 README는 `20261027-18 CM`에서483까지 갱신됐다. 이번에는 인수인계만 정리했으며 README를 최신504 기준으로 갱신한 것으로 쓰지 않는다.
- [ ] 싱글플레이/UI/호환성 뒤 TCP/IP 로컬 멀티플레이·Outpust/전용 분기를 **디컴파일 근거로만 사전 구현**한다. 신규 검사·게임/통신 테스트 없이 작성하며 동작·동일성·안정성 미검증을 기록한다. 공식 서버 복구는 가정하지 않는다.
- 4차 이후는 한국어·MCP·60/120프레임·CD10.72 전체 복원이다. CD의 Windows10 구동 불가 해결책도 해당 작업 때 문서화한다.

## 4. 운영 규칙과 유효한 사용자 결정

- **긴 검사 묶음(2026-10-25):** 매 단위34경로·긴Early완주·전체CTest를 반복하지 않는다. 원문 근거·집중 검사·필요한 증분 빌드·인계/커밋을 하고, 기능 묶음 완성 또는3~5단위 뒤 같은 최종 제품으로 통합한다. 실제 입력/타이머/저장은 해당 장면을 짧게 먼저 확인한다. 자연 완주/사용자 개입 보류 결정은 계속 우선한다.
- **기록:** 정적 대조·명시 입력·컴파일·C++검사·GUI 관측을 구별한다. 최초 실패/교정 원시는 지우지 않고 기대값을 제품에 맞춰 약화하지 않는다. 서로 다른 제품의 성공을 합산하거나 준비본을 제품 통과 수에 넣지 않는다.
- **실제 실행(2026-10-27):** 메뉴/스테이지 진입·수초 진행의 짧은 검증은 가능하다. 스테이지 클리어/사용자 개입 검증은 뒤로 미룬다. 실행 전 확인 여부는 AGENTS.md의 해당 PC 예외·직접 요청 규칙을 따른다.
- **Test Battle 언어파일(2026-10-27):** 대응 `.english` 부재 시 종료는 원본 동작이며 개선은 보류다. 임의 원본/기본 시나리오 fallback은 추가하지 않는다.
- **테스트 조작:** `--test-bridge-patterns` 등은 원본 분석 결과와 구별해 주석/기록에 표시하고 기본 게임·적AI·저장 프로필에 섞지 않는다. 로컬AI/TEST02 방침은3절6번을 따른다.
- **보호:** AGENTS.md와 `originals/`·`originalCD/` 기존 파일은 수정/삭제하지 않는다. AGENTS.md가 허용한 `options.cfg`·`fullScreenStateFile.dat`는 예외다. 게임 실행 전후 원본 전체 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 Visual Studio18(2026)/MSBuild18.x를 쓴다. 최근 명령은 `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`이며 앞선 컴파일러/CMake 자식이 모두 끝난 뒤 시작한다([빌드 안내](docs/cpp-build.md)).
- **UTF-8:** 새 문서/코드와 한국어 주석은 UTF-8이다. PowerShell5.1의 Python stdin은 `$env:PYTHONUTF8=1`만으로 부족하므로 한국어를 넘길 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정하거나 UTF-8 파일을 사용한다. 원시 로그/Native fixture는 인코딩 정리로 덮어쓰지 않는다.
- **Git:** CRLF 보존을 위해 확인/stage에 `git -c core.autocrlf=false`와 필요 시 `-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정을 바꾸지 않고 해당 작업 파일만 stage한다. 커밋은 `YYYYMMDD-NN PJ 요약`이며 당일 마지막 번호를 `git log`로 확인한다(cpppj CP, 공통 CM). 원격push 금지다.
- **도구:** `tools/cpp_solo_entry_smoke.py`(34경로, 데모는 `--demos`), `tools/cpp_demo_start_smoke.py`, 자연Early1/2용 `tools/cpp_tutorial_harvest_smoke.py`·`tools/cpp_tutorial_two_smoke.py`, ESC용 `tools/cpp_game_menu_smoke.py`·`tools/cpp_game_menu_pause_smoke.py`. 실제 호출 인자/최근 batch는 작업 PC의 `extracted/` 드라이버를 따른다.

## 5. 주요 참고 문서

- [C++ 빌드](docs/cpp-build.md), [cpppj 로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md)
- [모듈 맵](docs/exe/modules.md), [정밀 디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-10-23 양판본 정밀 디컴파일](docs/exe/cpp-decompilation-20261023.md)
- [원본 자동 탐험 analyzeManager](docs/analyze-manager.md), [유닛25종 점검](docs/exe/cpp-unit-restoration-20261022.md), [의식 집중 검증](docs/exe/cpp-ritual-runtime-focused-20261021.md)
- [준비본 보관](docs/exe/prepared/README.md): 443/444는 통합됐고469 준비본의 Subscriber/공유표/NewFort/섬 추가는 후속 단위에서 채택됐다. 개별 보관본의 미적용/미빌드 문구는 보관 당시 상태다. 보관 base/patch/manifest/142파일 원시는 유지한다.
- [Edit 왕복/냉각](docs/exe/cpp-ice-cooling-20261027.md), [새 맵 실제 저장](docs/exe/cpp-editor-new-map-live-20261027.md), [기존 맵 섬 추가](docs/exe/cpp-editor-existing-islands-20261027.md)
- 단위별 상세/최초 실패/증거 해시: [과거 이력](LEFT_JOBS.old.md), [2026-10-27 정리 직전 원문](LEFT_JOBS.old.md#archive-20261027-through-504)

## 인수인계 문서 임시 분할

dotnetpj 와 병행 작업하기 위해,
이 LEFT_JOBS.md 에는 당분간 cpppj 및 디컴파일 관련 진행 상황과 그 인수인계 내용을 적는다.
dotnetpj 진행 상황과 인수인계 내용은 LEFT_JOBS.dotnetpj.md 에 적는다
cpppj 1차 목표 달성 및 윈도우 10, 11 호환성 작업, MCP 추가 작업까지 완료되어 dotnetpj 구현에 cpppj 를 활용할 수 있게 되면, 인수인계 문서를 다시 LEFT_JOBS.md 로 통합한다.

---

**이번 문서 정리 완료(2026-10-27):** 정리 전 원문 전체를 기존 보관본 뒤에 보존하고, 현행 본문은504 기준으로 갱신했다. 해결된 bc5/bc8 오류·완료된 준비본 병합/섬 추가/편집 왕복을 옛 “다음 작업”에서 제외했다. 개발 목표의 일시 중지와 미완료/보류 결정은 유지한다. 이번에는 인수인계 두 문서만 변경하고 원문 보존·문서 링크/앵커·UTF-8을 확인했다. 게임 실행·게임 빌드/C++검사·디컴파일은 수행하지 않았다.
