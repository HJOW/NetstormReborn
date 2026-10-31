# LEFT_JOBS — cpppj 및 공통 인수인계

이 문서는 cpppj와 공통 사항의 **현재 상태, 남은 작업, 유효한 사용자 결정**만 담는다. 단위별 상세와 증거는 `docs/exe/cpp-*.md`·`cpppj/*.json`, 과거 기록은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)에 있다.

단위를 마치면 0.1절의 값과 2.1절의 해당 영역 행을 덮어써 갱신한다. 단위별 진행 기록을 이 문서 위쪽에 쌓지 않고 단위 문서에 남긴다.

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-10-31, 단위661 완료 / 목표 진행 중)

| 항목 | 값 |
| --- | --- |
| 진행 상태 | **사용자 재개 지시로 진행 중.** 661은 Edit State1의 cleanup 전 출처/제목 첫BYTE/이전 Battle14 통지를 연결했다. State3과 같은 조건·52h 전송을 공유하며 집중60등록 중 최종 고유11개/실패0·기존 원문115행 일치·Release1회 경고/오류0이다. 최초 실패는 없다. 일반 전투 재접속은 정적으로 분류해 후순위MP에 기록했다. 새 게임/ZIP/전체34는 없으며 더 많은 기능 묶음 뒤 통합한다. 실행 중인 빌드/게임은 없다. 전체 목표는 미완료다. |
| 현재 제품 | 제품661이며 State1은 cleanup 전에 출처 선택/첫BYTE0/이전 Battle14 통지를 실행하고 State3 전용 승패/음악/전체 배경은 실행하지 않는다. 실제 단독 로컬 큐의 예약/지연 수신과 실패 중간 상태를 확인했다. 전체 Editor 설치/GUI 검증은 별도다. 정상 단독 State7 부모는660이며 확인된 일반 전투 저장/서버 이전은 후순위MP로 구분한다. 최신 전체34/ZIP은645의644제품과 구별한다. |
| EXE SHA-256 | `107eb70c9306c40fee1690272b311f0e8a8d97ccbb6957e5ac4cdf56e85cf96c` (661) |
| ZIP SHA-256 | 645에서 만든644 제품의 `6485febc36080dc90e9eec43965a842f85d39e81c82a003a96b83b34b3694b73` (1376항목/게임 자산1358개). 실제 한글😀 경로에 압축 해제해 최종34를 실행했다. |
| 등록 C++ 검사 수 | 661의 집중60등록 중 새2개/입력5개·기존9개, 최종 고유11개/실패0이다. State3의41입력/두 제어워드82관측과 State33행, 총115행을 대조했다. 다른49등록은 실행하지 않았다.4회 실행/11함수 호출이며 최초 실패는 없다. 정적129명령/신규MP 검사0과 구별한다. 주 검사 EXE는 재빌드하지 않아645의3566등록/집중37이 마지막 근거다. |
| 최신 전체34 | 단위645, 644 제품 기준 최종34/34. 최소 비정지 시각 증가20.843초·창DC RGB·Native Frame 일치. 최초 입력 누락34실패/재검증34통과와 준비2회를 포함한 총70프로세스를 구별한다. |
| 최신 반복 수명 | 단위640, 같은637 제품의 브리핑/중단 확인 바깥 뗌 취소2회·미션 재진입1회와 채팅 도킹2실행. 최종3실행 종료0. Restart4회/Leave 취소2회/메인 복귀2회는638의 별도 근거다. |
| 전체 C++ 마지막 실행 | 단위549. 3389개 중 7개 검사가 실패했다(580단언). 이후 입력 정정 뒤 집중 검사만 통과했고 전체 재실행 기록은 없다. 마지막 전체 통과는 단위510(508 제품, 3337개, 실패 0)이다. |
| 보호 기준 | 전체2632개/기존fixture773개는645에서 불변을 확인했다. 기준은 `extracted/cpp-solo-entry-645-20261031/protected-after-current.json`이다.661은 관련7입력(AGENTS/두PE/TAFF/기존fixture3개)을645 기준과 대조해 불변을 확인했다.648/649/651/652/653/654의 각1개 추가는 별도이며655~661의 새fixture는 없다. 전체 재감사와 구별한다. HJOW-X3D에는 없다. |
| **마지막 전체 정밀 디컴파일** | **HJOW-Athlon (192.168.0.94) / 2026-10-23** |

### 0.2 재개 절차

1. AGENTS.md, 이 문서, LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 LEFT_JOBS.webpj.md도 읽는다. `git fetch`로 다른 PC의 커밋을 확인한다. push는 하지 않는다.
2. `hostname`이 위 디컴파일 호스트와 다르면, cpppj 작업이나 디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 내역 문서에 갱신한다. cp949 오류가 나면 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`을 설정한다. HJOW-X3D의 `extracted/refined/originals/`는 2026-10-27에 중단된 불완전 자료(`raw.c`·`functions.tsv`·`conventions.tsv` 없음)이므로 완료 산출물로 쓰지 않는다.
3. 실제 게임 구동과 별도 창이 뜨는 작업은 AGENTS.md를 따른다. 확인 없이 진행할 수 있는 시스템은 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94)뿐이다. **HJOW-X3D를 포함한 그 밖의 PC에서는 실행 목적과 필요성을 설명하고 명시적 확인을 받는다.** 허용받으면 해당 작업 단계가 끝날 때까지 유지된다. 사용자가 원본 수동 컨트롤이나 녹화 분석을 직접 요청한 작업 단계도 확인이 필요 없다.
4. 빌드·검사·GUI 원시 자료는 작업 PC의 `extracted/`(Git 제외)에만 있다. 커밋되는 보고서는 `cpppj/*.json`, 작업 문서는 `docs/exe/cpp-*.md`다. 다른 PC에 원시 자료가 있다고 가정하지 않는다.
5. 다른 세션의 dotnetpj·webpj 변경, staged 파일, 실행 출력은 이 작업에 포함하지 않는다. 단위마다 인계를 갱신하고 로컬 커밋한다.
6. 645의 전체34 통합은 완료됐으며646~661에서 실제 게임을 다시 실행하지 않았다. 소수 단위 뒤 전체34를 반복하지 않고 더 많은 기능 묶음 뒤 필요한 통합을 수행한다. 새 작업은3절을 따른다.
7. 마지막 빌드/집중 검사 근거는0.1절과 단위 문서를 따른다.661은 `extracted/cpp-editor-entry-prelude-661-build-v1.log`·`focused-v1.log`·`menu/state/parent.log`와 보고서의 실제 경로를 따른다. 최종11개/원문115행·정적129명령과 전체60등록을 구별한다. 주 EXE3566/집중37은645의644제품이다. 신규MP 검사는 하지 않는다. 물리 캡처 환경이 바뀌지 않았으므로 WGC/물리 입력을 반복하지 않는다.

### 0.3 유지되는 제한과 보류

- **재현하지 않는 검사:** 사제 포획/희생, 특정 적 공격, 시나리오 승리를 재현해야 하는 테스트는 건너뛴다. 사용자가 나중에 직접 플레이해 확인한다([TODO.md](TODO.md)).
- **사용자 확인 대상:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상에서 제외한다. 메뉴·진입·수 초 진행의 짧은 검증과 기능별 원문 대조는 계속 AI가 한다. 단계 전환은 1.1절의 기준을 따른다.
- **공중 3종:** 훨리기그·맨오워·더스트 데빌은 정적 분석과 코드 작업만 한다. 새 전용 생산·생성·우클릭·행동 관측/검사는 하지 않는다.
- **멀티플레이:** 싱글플레이·UI·호환성 뒤 디컴파일만 참고해 사전 구현한다. 신규 단위 검사, 게임 실행, 실제 통신 테스트는 하지 않으며 사전 구현과 검증 완료를 구별한다.
- **사용자 보류 항목:** 스펠 효과 도움말 그림의 추가 분석(그림 임시 숨김 유지, 단위530), 연녹색 직사각형 분석(원본에서도 보였다는 사용자 확인, 원격 데스크톱 원인 여부 미확정), 대응 `.english` 부재 개선.
- **아직 근거가 없는 범위:** 다른 모니터/DPI, 실제 앱 포커스 전환, 물리 키 입력, 실제 PCM 출력, 사용자 활성 전체화면의 물리 출력, 실제 CD 삽입. Windows 11 실기도 미검증이지만 단계 전환 조건이 아니다(1.1절).
- **복원 한계:** 단위612의 반사 Generate 입력은 원본 32비트와 현재판의 스택 배치·미정값 자체 일치를 보장하지 않는다. 기존 VFX 커널의 signed WORD/정상 표본 범위 계약도 유지하며, 양수·각도0·원색 입력 근거를 음수/회전/전체 픽셀로 확대하지 않는다.

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **진행 중.** 34경로 진입·진행은 가능하다. AI 작업으로 남은 것은 UI/Edit/시나리오/수명이다. 자연 완주와 전체 동일성은 사용자 확인 대상이다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **진행 중.** Windows 10 호환 전체화면과 저장 재시작은 확인했다. 남은 Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다. 10.72의 Windows 10 구동 불가 해결책은 10.72 작업 때 문서화한다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | 미착수. 채팅의 MP 분기처럼 일부만 정적으로 작성했다. |
| 4차 | MCP. dotnetpj 개발을 위한 게임 분석에 쓴다. | 미착수 |
| 5차 | 설정·캠페인 진행 저장을 `%USERPROFILE%\Saved Games\Netstorm\Saves\`로 옮긴다. 커스텀 맵은 기존 디렉토리와 `%USERPROFILE%\Saved Games\Netstorm\CustomMaps\` 양쪽에서 읽고, Edit 신규 저장은 후자에 둔다. Edit 맵 목록에 스크롤을 넣고 출력 개수 제한을 없앤다. | 미착수 |
| 6차 | 한국어 지원 | 미착수 |
| 7차 | Options에서 안티앨리어싱과 FSR 1.0을 선택하게 하고 게임에 적용한다. 둘 다 기본값은 꺼짐이다. FSR은 [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)의 Super Resolution(해상도 업스케일링)을 참고한다. | 미착수 |

**단계 전환 기준 (2026-10-31 사용자 결정):**

- **자연 완주·전체 동일성:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상이 아니며, 단계에 남은 작업이 이것뿐이면 다음 단계로 넘어간다.
- **Windows 11:** Windows 10에서 오류 없이 구동되면 Windows 11에서도 문제가 없을 가능성이 높다고 본다. Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다.
- **제보 처리:** 사용자가 플레이 중 오류나 원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다.

60·120프레임 지원과 CD 10.72 전체 복원은 별도 후순위다. cpppj는 Windows용만 개발한다. dotnetpj 후순위 요구에도 4~7차가 포함되며, webpj는 Edit·멀티플레이를 제외한다.

### 1.2 현재 목표 범위 ([TODO.md](TODO.md))

- Early Missions 6개, Priest Training 9개, 캠페인 전체 16개, 데모 3개를 사용자가 플레이할 수 있고 원본 10.78과 완전히 같게 동작하도록 한다. 자연 완주와 전체 동일성의 확인은 사용자가 한다(1.1절).
- Edit 메뉴의 커스텀 맵/시나리오, 메인 메뉴의 비멀티 항목, 좌측 덱·미니맵·다리 단축키 QWASZX·ESC 메뉴를 구현한다.
- Windows 10/11 호환성을 해결한다. 원본과 다른 호환 처리와 이유는 한국어 코드 주석에 명시한다. 전체화면, 4:3/16:9/16:10, 가장자리 스크롤 요구를 유지한다.
- 그 뒤 멀티플레이를 무시험으로 사전 구현하고 4~7차를 진행한다.
- 개발 작업 단위마다 인수인계를 갱신하고 커밋한다. 커밋에 실패해도 작업은 이어 간다.

대상은 원본 `d/offical1~5.english`의 MissionBegin 순서와 `tell.DEMOVILLE`의 데모 순서다.

| 구분 | 미션 키 (원본 표시 이름) |
| --- | --- |
| Early Missions (offical1) | Tutorial1 (Bridge the Gap), Tutorial2 (Secret Workshop), Tutorial3 (Capture The Priest), Tutorial4 (Tactical Combat), Tutorial5 (Subtle Defense), Tutorial6 (Raw Power) |
| Priest Training (offical2) | bc1menu76, bc2bridging76, bc3acid_bar76, bc4bow_rush76, bc5walkers76, bc6mows76, bc7sun_bars76, bc8fury76, bc9ssun_bars76 |
| 캠페인 1장 (offical3) | TheWarBegins, MasterOfWhirligigs, SaveTheIsland, FragileFortune, ThunderingPower, DissolvedAlliance |
| 캠페인 2장 (offical4) | TheNoose, RainVsRain, RunForIt, GuardMyBack, Surrounded |
| 캠페인 3장 (offical5) | BreakingThrough, ToTheRescue, Vicious, EnemyTerritory, FinalConfrontation |
| 데모 | DemoStormRages (The Storm Rages), GuardMyBack (Demo of "Guard My Back"), DissolvedAlliance (Demo of "Dissolved Alliance") |

일반 31경로(Early 6 + Training 9 + 캠페인 16)와 데모 3개를 합쳐 34경로다. 같은 자원을 쓰는 GuardMyBack·DissolvedAlliance도 일반/데모를 따로 검증한다([대응 근거](docs/gameplay/demo-campaign-map.md)).

### 1.3 AGENTS.md 변경 반영 (2026-10-30까지)

| 커밋 | 변경 | 반영 상태 |
| --- | --- | --- |
| `bfccadf5`, `24959e75` | 3차 뒤 후속을 4차 MCP, 5차 저장 위치·커스텀 맵·Edit 목록, 6차 한국어로 재편 | 1.1절과 3절 8번에 반영 |
| `ef0c6b91` | cpppj·dotnetpj의 Windows 실행 파일 이름을 `Netstorm.exe`로 통일 | cpppj는 단위580에서 완료(CMake 대상 NetstormCpp 유지, OUTPUT_NAME 변경). dotnetpj는 `Netstorm.Game.csproj`의 AssemblyName이 아직 `NetstormClone`이며 LEFT_JOBS.dotnetpj.md에서 처리한다. |
| `9d501e29`, `4acd05dd`, `5ed4ef73`, `7dd1ab4e` | 7차 추가. FSR 3.1 → FSR 제거 → FSR 1.0으로 변경됐고 dotnetpj 후순위도 4~7차로 넓어졌다. | **최종 기준은 `7dd1ab4e`의 FSR 1.0이다.** 1.1절과 3절 8번에 반영. FSR 3.1과 FidelityFX-SDK 안은 쓰지 않는다. |

AGENTS.md의 `(4~6차 목표.)` 문구는 7차 추가 뒤에도 그대로지만, 바로 아래에 7차까지 나열되고 dotnetpj 항목도 4~7차로 적혀 있으므로 4~7차로 읽는다.

## 2. 현재 상태 (2026-10-31, 단위661 확인 기준)

- **게임 실행·플레이 가능 여부:** 인자 없는 `Netstorm.exe`로 GUI가 시작되고 34경로 모두 진입·비정지 진행·정상 종료를 확인했다. 자연 완주 근거는 과거 제품의 Early1/2 자동 완주와 사용자가 다른 PC에서 직접 완주한 캠페인 1-1뿐이다.
- **기존 게임과의 동일성:** 원문 관측과 대조를 마친 부품이 많다. 자연 완주와 전체 동일성은 사용자가 플레이하며 확인한다.

단위/집중 검사, 짧은 진입, 자연 완주, 전체 동일성은 각각 별도로 판정한다. 서로 다른 제품의 성공을 합산하지 않는다.

### 2.1 영역별 마지막 근거

| 영역 | 마지막 근거 | 확인한 범위 | 남은 한계 |
| --- | --- | --- | --- |
| 전체34 진입 | [645](docs/exe/cpp-solo-integrated-20261031.md) (644 제품) | 새 ZIP의 일반31+실제Demo3 최종34/34, 종료0·비정지 시각 증가·창DC 전체RGB·Native Frame·수집/분배. 최소 증가20.843초. 주3566등록/집중37통과 | 짧은 진입이다. 최초 입력 누락34실패/재검증34통과/첫 실행 준비2회/총70프로세스를 구별한다. 자연 완주·전체 동일성·물리 입력은 아니다. |
| 반복 미션 수명 | [638](docs/exe/cpp-state-repeat-20261031.md) (637 제품) | TEST02 4:3 창, 캠페인 1-1 16:9 전체화면. Restart·Leave 취소·메인 복귀·Quit, 창 DC RGB 4쌍 | 앱 내 검사 입력이다. 장시간 누수, 모든 전환, 자연 완주의 증거가 아니다. |
| 시작·휴대 배포 | [637](docs/exe/cpp-portable-startup-20261031.md) | 무인자 GUI 시작, 실제 Unicode EXE 경로 기준 `game/` 우선, cwd 독립, 자기 콘솔 분리, GUI 오류 표시. 집중 19/실패 0, 빌드 경고·오류 0 | 물리 입력 0건. 2.4절 참조 |
| 카메라 | [615](docs/exe/cpp-camera-pan-20261030.md), [617](docs/exe/cpp-camera-pan-presentation-20261030.md), [632](docs/exe/cpp-camera-center-20261031.md), [633](docs/exe/cpp-camera-immediate-20261031.md), [635](docs/exe/cpp-camera-viewport-20261031.md) | 원문 Alt/가운데 pan, 확대 전체화면 논리 단위, 직접 중심 목표, Home 계열 즉시 적용, viewport 변경 시 현재 값 보존 | 물리 키, 다른 DPI, 실제 포커스 전환 |
| Renderer·Native Draw | [618](docs/exe/cpp-renderer-prelude-20261030.md), [619](docs/exe/cpp-renderer-clouds-20261030.md), [620](docs/exe/cpp-renderer-frame-20261030.md), [622](docs/exe/cpp-native-scene-20261031.md), [628](docs/exe/cpp-mana-additional-20261031.md), [629](docs/exe/cpp-auxiliary-draw-20261031.md), [648](docs/exe/cpp-renderer-presentation-20261031.md), [655](docs/exe/cpp-mission-entry-20261031.md) | 앞단, 구름 4칸·배경 선택, 실제 Frame, 단일 표시 목록, 마나 103 추가 Draw, 배율·고정색, 부분 그리기/전체 출력 요청·첫 flags/count0 보존, 실제 Zone 뒤 초기화 | 전체 픽셀 동일성, 기존 VFX 계약·전체 비동기State7 |
| UI 갱신 영역 | [624](docs/exe/cpp-gump-redraw-20261031.md), [625](docs/exe/cpp-ui-dirty-20261031.md), [627](docs/exe/cpp-gump-derived-dirty-20261031.md), [630](docs/exe/cpp-inventory-blink-20261031.md), [631](docs/exe/cpp-minimap-dirty-20261031.md), [639](docs/exe/cpp-minimap-content-20261031.md) | Gump 기본 갱신, 지도 clamp, 기본 UI, 장식·메뉴 행, 생산 점멸, 미니맵 기본 영역·지면/소유자/삭제 픽셀. grid/debug 직접 writer와 생성 초기값 대조 | 일반 경로 활성 writer를 찾지 못한 grid/debug는 노출하지 않는다. 간접 alias 부재 전체 증명은 아니다. |
| Edit·시나리오 | [590](docs/exe/cpp-editor-scenario-20261030.md), [597](docs/exe/cpp-editor-rules-20261030.md), [609](docs/exe/cpp-world-geyser-20261030.md), [634](docs/exe/cpp-editor-catalog-current-20261031.md), [650](docs/exe/cpp-world-generation-session-20261031.md), [653](docs/exe/cpp-editor-default-name-20261031.md), [661](docs/exe/cpp-editor-entry-prelude-20261031.md) | 시작값·작업본 왕복·방향별 동맹·세계 생성·Fort433개 이관/필터와 기본 이름 선택·늦은 파일 준비/Session checkpoint·State1 공통 앞단/52h 통지 | 3절 2번.653의7개 통과는 전체State/GUI 검증이 아니다. |
| Edit Unicode 파일 | [646](docs/exe/cpp-editor-unicode-20261031.md) | UTF-8/native 경로·wide CRT, 실제 Unicode 파일 왕복·권한·목록 이관/장부 재열기·삭제 보존·source TAFF 격리. 집중24통과 | GUI/전체34/한국어 UI 검증이 아니다. 최신 실행/ZIP은644제품이며 더 많은 기능 묶음 뒤 통합한다. |
| 객체 메뉴·건설 | [524](docs/exe/cpp-object-hit-20261028.md), [528](docs/exe/cpp-factory-upgrade-runtime-20261028.md), [531](docs/exe/cpp-altar-salvage-runtime-20261028.md), [537](docs/exe/cpp-mission-cost-guard-20261028.md) | 픽셀 선택, 시험한 메뉴·업그레이드·회수, 미션 비용 관문 | 다른 타입·레벨·상태 |
| Factory 중복 경고 | [647](docs/exe/cpp-factory-duplicate-warning-20261031.md) | 실제 제공된 경고 callback 보존·Client 채널4 로그·명시 중복 입력의 경고 뒤 기존 객체 삭제. 최종1개 통과 | 일반 Pop 겹침 거부와 분리한 +20 장치 접합이다. 합법 GUI 배치/전체20/전체34 검증이 아니다. |
| HUD·단축키 | [509](docs/exe/cpp-hud-input-runtime-20261027.md), [511](docs/exe/cpp-player-list-runtime-20261028.md), [540](docs/exe/cpp-minimap-toggle-runtime-20261028.md), [547](docs/exe/cpp-send-money-runtime-20261028.md), [548](docs/exe/cpp-screenshot-runtime-20261028.md) | 미니맵, QWASZX, ESC 일부, Player List, 독립 미니맵, Send Money, F7·Shift F7 | 스크립트 입력이다. 3절 1번 |
| Chat | [640](docs/exe/cpp-ui-lifetime-current-20261031.md), [574](docs/exe/cpp-chat-completion-20261029.md), [586](docs/exe/cpp-chat-live-message-20261030.md), [614](docs/exe/cpp-chat-settings-20261030.md), [649](docs/exe/cpp-issue-commands-20261031.md) | 입력줄, 자동완성, 일반 Enter·중계·로컬 큐·공유 로그, 설정 재실행, 초기 점 명령/밝기·실제 오류 출력 | 3절 4번 |
| 미션 진입 UI·팔레트 | [656](docs/exe/cpp-mission-starting-tail-20261031.md), [657](docs/exe/cpp-mission-async-20261031.md), [658](docs/exe/cpp-mission-starting-commit-20261031.md), [659](docs/exe/cpp-mission-starting-prefix-20261031.md), [660](docs/exe/cpp-mission-starting-parent-20261031.md) | UI/색 수명·Tick/큐/완료 공개·안내·7/8 확정·앞단과 같은 부모의 최초 복사/로그/World 교체. 집중10개/Native33행 일치 | 재접속/Zone0·메타 Poll 장치·시계 패킷 등록과 실제 GUI 전체 교체는 별도다. |
| 영상·CD | [594](docs/exe/cpp-movie-runtime-20261030.md), [595](docs/exe/cpp-cd-movie-package-20261030.md), [654](docs/exe/cpp-cd-presence-20261031.md), [655](docs/exe/cpp-mission-entry-20261031.md) | 실제 MovieGump, 스킵·자연 끝, 완료 저장, CD 경로 탐색·별도 CD 유무/Client 캐시·State7/시작의 별도 결과 저장, 영문·독문 기본 영상 동봉 | 실제 CD 삽입/열거, PCM 장치, 활성 전체화면 물리 출력 |
| 데모 | [567](docs/exe/cpp-demo-repeat-20261029.md), [569](docs/exe/cpp-demo-repeat-two-20261029.md) | 세 데모의 첫 360초 만료 → 재시작 → Exit Demo → Quit | 두 번째 전체 360초 |
| 저장·진행 | [568](docs/exe/cpp-saved-state-file-20261029.md), [578](docs/exe/cpp-campaign-progress-20261030.md), [588](docs/exe/cpp-battle-leave-20261030.md) | 저장상태 파일 정리, 진행 키 저장과 다음 미션 해제, 일반 중단의 State3 복귀 | 실제 승리부터 자동 저장까지, 일반 전투 저장상태 복구는8번 후순위MP(661) |
| 메뉴·Help | [566](docs/exe/cpp-credits-roll-20261029.md), [600](docs/exe/cpp-cheat-menu-20261030.md), [603](docs/exe/cpp-help-menus-20261030.md) | Credits 자동 전환, ESC Cheat 직접 메뉴, Help·Technical Help·외부 링크 로컬 도우미 | 원본 픽셀·음향 |
| 의식 보상 | [598](docs/exe/cpp-new-technology-20261030.md), [599](docs/exe/cpp-ritual-refresh-20261030.md), [604](docs/exe/cpp-player-notices-20261030.md), [642](docs/exe/cpp-foreign-ritual-reward-20261031.md) | 로컬 기술·Core/snapshot·프레임말 결과와 비로컬 SP5000/별도144BYTE 기술 장부 실제 연결 | 실제 의식 UI, 비미션 전체 랭크 상승, MP 상태/사망 안내 |
| 파일/TAFF | [643](docs/exe/cpp-taff-secondary-20261031.md), [644](docs/exe/cpp-basefile-read-policy-20261031.md), [651](docs/exe/cpp-basefile-io-20261031.md), [652](docs/exe/cpp-basefile-read-key-20261031.md) | 두 TAFF/열거·Help·UTF-8/W·경로 정책과 BaseFile11몸체·실제 디스크/TAFF/객체 덤프 장치.652 원문235행·집중15개 통과 | 전체41b4c0 FILE/예외 ABI·다른 쓰기/키/스트림 소비자·실제 손상/실패 수명 |
| 투사체·수명 | [552](docs/exe/cpp-bridge-runtime-20261029.md), [611](docs/exe/cpp-missile-floater-20261030.md), [612](docs/exe/cpp-ice-ricochet-scratch-20261030.md), [613](docs/exe/cpp-fence-event3-20261030.md) | 다리 붕괴·대피, 플로터 회피, 반사 변위 0, 펜스 사건 3 | 실제 특정 적 공격은 생략했다. 0.3절의 복원 한계 |
| 전체화면·해상도 | [호환 전체화면](docs/exe/cpp-modern-fullscreen-20261027.md), [가장자리](docs/exe/cpp-edge-scroll-runtime-20261027.md), [해상도 메뉴](docs/exe/cpp-screen-resolution-menu-20261027.md), [635](docs/exe/cpp-camera-viewport-20261031.md) | Windows 10 DIB 호환 출력, 가장자리 이동, 세 화면비 메뉴, 저장·종료·새 프로세스 적용 | 3절 7번 |

기본 채팅 구분선/숨김/복원과 실제 브리핑·중단 확인의 캡처 취소/미션 재진입은 [640](docs/exe/cpp-ui-lifetime-current-20261031.md)에서 같은637 제품으로 다시 확인했다. 전체 Chat·물리 입력의 완료 근거는 아니다.

### 2.2 미션별 상태

아래 장기 플레이 증거는 과거 각 제품에서 얻은 것이며 현재 제품의 자연 완주 회귀 결과가 아니다. 남은 확인 열의 자연 완주·승패·화면·음향은 사용자 확인 대상이다.

| 대상 | 확보한 증거 | 남은 확인 |
| --- | --- | --- |
| Early1 | 북서 간헐천 경로 A~G, 채취/귀환, SP 0→600, Tutorial2 진입 | 남동 경로 실패 원인, 실패/재시작/F8, 화면·음향 |
| Early2 | A~I, 신전·워크샾·지식·생산·발사기·회수(환불 75), Tutorial3 진입. 마지막 자연 완주 회귀는 467 제품 | 재시도·취소, 화면·음향 |
| Early3~6 | 진입/Start, 단계 판정, 짧은 진행. Early3은 468 Source1에서 채취/귀환, SP 2000→2200→2400, A→B→C. Early5는 과거 F(오벨리스크 획득)까지 | 각 자연 완주와 다음 미션. Early3은 최종 468 제품에서 완주한 기록이 아니며 attempt4의 종료 1도 보존한다. |
| 캠페인 1-1 | 자동 AI 세션의 캐논 적 신전 HP 5000→0(322). 2026-10-28 사용자가 다른 PC에서 직접 완주하고 다음 스테이지 안내 정상을 확인. 638에서 Restart·Leave 반복 확인 | 사용자 실행의 PC/커밋/SHA는 미제공이며 현재 제품의 자연 완주 검증이 아니다. 보호막 중심·제단 포획 사제색(541)의 실제 확인은 사용자 담당. 1-2 진입과 패배/Retry는 별도 |
| 캠페인 1-2~1-6 | 진입/Start, 자동 AI/Kernel 400프레임(321), 645 전체34 재확인. 1-6 무입력 전투의 신전 파괴 뒤 섬 중립·기본 테마 복귀(575) | 자연 플레이, 승패, 후속 명령 |
| 캠페인 2·3장 | 645 전체34에서 진입/Start, 비정지 시각, 창 출력, 종료 0 확인 | 추가 TYPE 전체 수명, 자연 플레이 |
| Priest Training 9개 | 645 전체34에서 9/9 진입, 비정지 진행, 창 출력, 종료 0 확인 | 각 자연 완주, 완료 안내/메뉴 복귀, bc8 전체 원소 선택지 |
| 데모 3개 | 645 전체34에서 실제 메뉴 진입, 종료 0 확인. 첫 360초 만료 → 재시작 → Exit Demo → Quit 확인(567, 569) | 재시작 뒤 두 번째 전체 360초, 연속 반복/정리, 승패 |

Early3 후속 입력과 미실행 계획은 [부분 진행 보관](docs/exe/prepared/tutorial3-partial-468-20261027/ATTEMPT4-NEXT-C-H.md)을 본다. 다음 워크샾 메뉴의 71/75 생산·공급·사제 무력화/포획은 미완료다.

### 2.3 기능 영역별 상태

- **시작·메인 메뉴:** 무인자 `Netstorm.exe`가 GUI로 시작한다(637). 원본 8버튼과 Campaign/Demo/Help/Options, Credits 18쪽, Quit, Tips, 새 설치의 Native39 복귀(563), 실제 인트로 영상(594~595)을 연결했다. 승패 전환, 일반 전투 저장상태 복구(661에서 후순위MP로 분류), 미션 이탈 전체 선택, 픽셀/음향은 남는다. Multiplayer는 앞 목표 뒤에 사전 구현한다.
- **Edit:** 기존 맵 자연 State1 설치, 전체 저장/Save as, 새 이름 목록/재시작, Set All Bridge, SaveGoMain, 신규 Fort/자동 섬 창, 기존·새 맵 섬 추가를 연결했다. Test Battle → 짧은 전투 → Return to Editing 왕복(492), 시나리오 시작값(590), 방향별 동맹(597), 세계 간헐천(609)을 확인했다. 남은 것은 3절 2번이다.
- **게임 중 UI:** QWASZX 취득/회전/배치/보충, 원본 ESC 가로/하위 메뉴, 실제 HUD 루트·도킹/덱·미니맵/NSInfo/Chat, 독립 미니맵, Player List, Send Money, F2·F3·F7·F8·Shift F7·Ctrl Shift F7, 가장자리 이동과 원문 pan, 실제 Chat 입력·전달을 연결했다. 기본 Chat 구분선 드래그/숨김/복원은543과640에서 확인했다. 나머지 View/zoom·도킹 조합·모드별 표시·픽셀 대조는 남는다.
- **공통 전략 AI:** 적·아군·데모가 같은 원본 전략을 쓰고 `RawCampaignSession::RunFrame`의 현재 시각 → 비정지 AI → Kernel 순서에 연결된다(320~322). 정상 간격의 확장/채취/생산/공격은 자연 플레이로 대조하지 않았다.
- **유닛·주문·의식:** 워크샾 25종의 생성/이동/공격/표시·소유자색(335~345), Devastation, 포획/운반/희생, SP/미션 완료, 은신/마비, 윈드 소환을 복원했다. 의식 집중 검사에는 명시 시전원/포로/시계 입력이 포함되며, 실제 UI 취득 → 시전 → 포획 → 희생의 자연 플레이는 미검증이다.
- **내부 수명:** Client/Session의 장기 전역, Pool/Graph/Player, Deck, 권한, 커서, 반납 조정기와 cleanup, 기존 맵 Edit 설치, Guide/World 분리, 작업본 reader, Login 재적재를 연결했다(425~486). 정상 State7은 `CleanupNativeStateWorld(1)`, 정상 MainMenu는 전체 cleanup(0)에 이미 연결돼 있다(638). 남은 것은 3절 3·4번이다.
- **배포:** [cpppj/package-cpppj.ps1](cpppj/package-cpppj.ps1)의 기본 증분 빌드 → ZIP, BUILD_INFO, 전체 자료/런타임, 인자 경계를 확인했다(562). 637부터 최상위 EXE 직접 실행이 기본이고, 창 모드 강제와 추가 인자용 배치 2개를 유지한다. 전체 게임 배포 검증과 구별한다.

### 2.4 알려진 미해결 이슈

- **물리 입력 미검증(637, 640):** 인자 없는 EXE 실행은 실제 창 제목과 PID, 명령줄까지 확인했으나, 화면 캡처가 재선택·재시도 뒤에도 timeout이어서 입력을 보내지 않았다. QWASZX·ESC·Home·포커스는 물리 입력 0건으로 미완료다. 640도 창 재선택·활성화 후 캡처 timeout으로 입력 없이 종료했다. 641의 별도 VS Code 창도 재선택 포함2회 캡처 timeout이었다. 게임 고유 결함으로 단정하지 않는다. 환경 상태가 바뀌기 전 같은 시도를 반복하지 않는다.
- **Early1 남동 경로:** D·SP 0에서 Next Tutorial을 찾지 못해 종료 1(417). 북서 경로는 통과하며 실패 원인은 미확정이다.
- **options.cfg 저장/복구:** GUI 음량 변경의 SaveOptions fopen errno22/doserrno1224(393), 도구 설정 열기/복구의 간헐적 OSError22(369·386). 격리 복사본은 통과했으며 H 볼륨 exFAT와의 인과는 확정하지 않았다.
- **옛 스모크 5개:** `cpp_menu_smoke`, `cpp_thunder_smoke`, `cpp_weather_smoke`, `cpp_audio_smoke`, `check_dialog_controls.check_mission`은 제거된 임시 Game 버튼을 전제로 한다(2026-10-31 확인). 재사용 전에 ESC → 실제 메뉴 입력으로 바꾼다. `cpp_rawbuttons_smoke`는640에서 실제 브리핑/ESC·캡처 취소·재진입으로 교체하고 실행 통과했다.
- **PlayerNoMake:** 경고 255DWORD/정리 `004328a0`의 장기 연결은 453에서 완료했으나 패킷 부모 `00433c30`의 실제 writer는 남았다. CD `00487900`은 spot 정리이므로 같은 함수로 취급하지 않는다.
- **섬 추가 후 Graph:** 배치 직후 Graph1/연결 표면 694 → 재로드 0 차이의 원본 전체 월드 대조는 미완료다(493~494). 저장 seed/Money/Deck의 난수 변화와 배치 레코드 보존을 구별한다.

## 3. 앞으로 할 작업 (권장 순서)

재개하면 아래 순서로 진행한다. 체크 항목(`[ ]`)은 AI 작업 대상이고, 5·6번은 사용자 확인 목록이라 단계 전환을 막지 않는다. 완료한 항목의 근거는 2.1절과 [정리 직전 원문](LEFT_JOBS.old.md#archive-20261031-through-638)을 따른다.

1. 화면 캡처가 정상화되면 물리 입력 QWASZX·ESC·Home·포커스를 검증한다(7번).
2. 남은 비멀티 UI·Edit·시나리오·파일·수명·호환성을 1~4번과 7번으로 이어 간다.
3. 645의 같은644 제품/새 ZIP/전체34 통합 뒤646~661의 파일·메뉴·Renderer·명령·세계 생성·Edit/CD/미션 진입을 진행했다.661은 State1의 공통 앞단/이전 Battle 통지를 연결하고 일반 전투 저장/서버 이전의 확인된 정상 경로를 후순위MP로 분류했다. 다음은 State1/LeaveEditMode의 나머지 파일/지형/소유자/시나리오 조합, 남은 UI/연속 미션·파일 소비자/ABI를 원문과 대조한다. 더 많은 기능 묶음 뒤 스테이지 진입/대기 검사를 몰아서 한다(4절 최신 사용자 지시). 소수 단위마다 전체34를 반복하지 않는다.
4. 체크 항목을 마치면 5·6번과 Windows 11 실기를 기다리지 않고 멀티플레이 무시험 사전 구현과 4~7차로 넘어간다(8번).

### 1. 대상별 커서·오브젝트 우클릭·게임 UI

- [ ] 크리스탈의 나머지 상태/명령, 비석의 미검증 특수 명령, 제단/일반 건물형의 나머지 타입·소유자·레벨·손상·건설/생산·포획/희생 상태별 메뉴·명령 효과·반납과 선택기 잔여를 점검한다. 이미 확인한 비석 기본 메뉴/전환과 소유자 일부를 미구현으로 되돌리지 않는다.
- [x] Factory 중복 경고의 SessionMissing 덮어쓰기는647에서 실제 Client 로그와 경고 뒤 기존 삭제에 연결했다. 일반 배치 성공과 구별한 명시 +20 장치 접합1개를 통과했다([647](docs/exe/cpp-factory-duplicate-warning-20261031.md)).
- [ ] 운반체 전체 종류와 다중 선택을 포함한 전체 커서 조합을 점검한다(536, 576). 공격형 공중 3종은 정적 분석만 한다.
- [ ] 남은 ESC 하위/View·도킹 조합·DD 조정 표시·Chat 입력과 모드별 표시/픽셀을 확인한다. 완료된 단축키·독립 미니맵·Send Money는 새 근거 없이 반복하지 않는다.
- [x] 미니맵 내용물 변경과 grid/항목별 debug의 직접 쓰기·생성 초기값·일반 경로를 대조했다([639](docs/exe/cpp-minimap-content-20261031.md)). 내용물 실제 영역/픽셀 집중 검사를 통과했다. grid/debug는 일반 경로 활성 writer를 찾지 못했으므로 노출하지 않는다. 간접 alias 전체 부재의 증거로 확대하지 않는다.
- 메타맵 번개 `00479650`은 MP 전용 실제 기록 분기이므로 8번의 무시험 사전 구현에 포함한다. 일반 번개/진단(559~565)과 구별한다.
- **647 메뉴 정적 점검:** c2(Test Battle/편집 복귀)와c5(Leave)는 이미 처리기에 연결돼 있다. c6(Refresh Zones)/db(Declare a Draw)는 메타맵/일반 Battle 전용이므로 후순위MP로 잇는다. 명령 표에 있는 값만 보고 비멀티 미연결로 되돌리지 않는다.
- **오류 복원 원칙:** 실제 오류의 원문 몸체/호출 순서를 대조한다. guard 제거, 배열 임의 확장, 손상 후보 무조건 무시, 진단 6 강제, 임시 속도로 숨기지 않는다.

### 2. Edit 메뉴·커스텀 맵/시나리오

- [x] 세계 생성 옵션9 비영/단일 전체와9회 요청의 실제 Session 접합은650에서 확인했다. 로그인/원문 Start의 NoTemple 뒤 중립 지면798/장식50, 소유된1-1의 거부, 간헐천 실제 Pop/Regular를8입력과 최종5개로 확인했다([650](docs/exe/cpp-world-generation-session-20261031.md)). 제품 변경/게임 실행은 없으며 비미션9개 서로 다른 구역·옵션 writer 검증과 구별한다.
- [ ] 나머지 시나리오 조합과 실제 설정/UI writer를 허용 범위에서 확인한다.
- [ ] 남은 지형·오브젝트·소유자 도구와 LeaveEditMode/State1·비동기 State7의 전체 분기를 점검한다.
- [x] 빈/점 시작 이름의 newFort 선택과 선택 뒤 실제 작업 파일 준비는653에서 연결했다. 원문252행/1008관측·집중7개와 실제 Unicode 파일/Session checkpoint를 확인했다([653](docs/exe/cpp-editor-default-name-20261031.md)). 전체 State 확정/loaded/GUI와 구별한다.
- [ ] 섬 배치 직후와 재로드의 Graph 차이를 원본 전체 월드와 대조하고 전체 작업본 저장/재파싱·이름/목록·미션 전환 수명을 확인한다.
- [ ] 나머지 owner 조합/시나리오 조건·지형/오브젝트 도구·자동 언어 선택을 대조한다. 590의 명시 `.french`는 프랑스어 UI나 한국어 완료가 아니며, 6차의 메뉴 전체 경로/문자열/글꼴은 남는다. TEST01/TEST02 원본은 읽기 참조만 하고 저장은 작업본에 한다.
- **목록 상한:** 원문의 첫 50개 상한은 5차 전까지 유지한다. 실제 Fort 433개는 종류2가 432개, 종류0이 1개여서 옵션0만으로 TEST02를 앞 50개에 넣지 못한다(634). 실제 Edit 검사 방법은 590·633·634 문서를 따른다.
- **판본 차이:** 10.78 기본 설명의 원문 `{Text}` 리터럴은 유지한다(494). 10.72 자료와 혼동하지 않는다.
- **영문 파일 부재 개선은 보류:** 대응 `.english`가 없을 때 Test Battle 종료는 원본 동작이다. 임의 기본 시나리오 fallback을 넣지 않는다.

### 3. 미션 수명·진행 저장·데모 반복

- [ ] Retry·Next Tutorial·Next Mission·Abort의 나머지 연속 전환과 참조/타이머·다리/AI 재고를 확인한다. Restart와 Leave → 메인은 638에서 확인했다. 자연 승패가 필요한 부분은 사용자 확인 대상이다.
- [ ] 세 데모의 재시작 뒤 두 번째 전체 360초(다시 만료)와 추가 연속 주기·나머지 안내를 확인한다. GuardMyBack의 15초 안내와 DissolvedAlliance의 Demo 섹션 부재를 유지하며 Script demo 키만으로 모드를 추정하지 않는다.
- [ ] 승패를 포함한 나머지 Native3 선택과 State1/단독 State7·전투 정리004b2df0의 자연 Tick/연속 전환을 확인한다. 존재하는 일반 전투 상태 복구/StateReconnect·서버 이전은661의 정적 분류에 따라8번의 후순위MP로 잇는다. 질문/삭제 UI는 유지하며 reader/open만으로 재접속 완료로 세지 않는다. LeaveBattle의 나머지 이유/CD/원격 페널티와 State6도 별도 잔여다(588).
- [x] State7 정리 뒤 실제 CD 확인/005c85b8 저장과 Client 캐시는654에서 연결했다([654](docs/exe/cpp-cd-presence-20261031.md)). 최종 원문492행/1968관측·집중9개를 통과했다. 기계어440cf9의 AL 조회를 stack 쓰기로 옮기지 않으며 하위 임시 물리는 반환 직후 관측한다. 기존 CDDir 재귀와 구별한다.
- [x] 시작 CD0054db6c와 실제 Zone 설치/반납 뒤4993c0은655에서 기존 Client 몸체에 연결했다([655](docs/exe/cpp-mission-entry-20261031.md)). 같은 실제 Client/Player/Core/State/요청의 접합과 필수 장치 누락 중단·기존 카메라16/CD492행을 확인했다. 앞선 Player 정리를 채택한 실제 미션에서 Renderer 장치를 임의 생략하지 않는다.
- [x] 실제 UI2/3와 활동 기한+10·즉시8/인자20·현재 battlePal·생성 전256색 보관/이후 커서 적용은656에서 연결했다([656](docs/exe/cpp-mission-starting-tail-20261031.md)). 최종6개와 기존941행을 대조했다. 즉시144BYTE는 Zone 반납 전에 복사하며 current+80의0과 immediate+80의 이전 핸들을 합치지 않는다. 설치 전 Cursor 입력 관문은 유지한다.
- **661 State1/7 경계:** 정상 단독 State7 부모의 최초 복사/로그·World 교체·7/8 확정은660에서 연결했다.661은 State1의 공통 출처/첫BYTE/이전 Battle14 통지를 State3과 공유했다([661](docs/exe/cpp-editor-entry-prelude-20261031.md)). 확인한 current+84 직접 작성은 일반 전투 서버 이전이며 !missionPost/PlayerKnown 저장 writer와 연결된다. 해당 재접속·Zone0/메타 Poll/시계 패킷·외부 동기화는8번의 후순위MP로 구분한다. State1 나머지 설치/도구·실제 GUI 전체 연쇄는 남는다. 간접 alias 전체 부재나 모든 State7 분기 완료로 확대하지 않는다.

### 4. 내부 수명·실제 연결

- [ ] 캠페인 이탈 → Edit의 mission/모드 수명, 일반 전투 saveBattle·metamap/비mission phase14 저장과 나머지 파생 프로세스 수명을 복원한다. saveBattle 미연결 선택은 638의 Guide 경로 통과로 완료 처리하지 않는다.
- [x] Renderer의 카메라/배경 전체 출력과 dirty 부분 그리기 분리는648에서 복원했다. 첫 flags/count0·같은 표 clip·원문288행과 관련 집중30개를 확인했다([648](docs/exe/cpp-renderer-presentation-20261031.md)). 게임 실행은 추가하지 않았다.
- [ ] 카메라 zoom과 Renderer `004994b0` flags/override/시차의 나머지 writer/모드 경계를 확인한다. 앞단·Frame(618~622)과 목표·즉시 적용·viewport(632~635)는 연결을 마쳤으므로 다시 만들지 않는다.
- [x] 초기 점 명령 등록/flash·cheatorama·실제 오류 출력과 공통 Form 밝기 전달은649에서 연결했다. 원문122행/488관측·관련19개를 확인했다([649](docs/exe/cpp-issue-commands-20261031.md)). desc는 등록/경계만 준비했으며 실제 디렉터리/통신은 아래 MP 사전 구현으로 잇는다.
- [ ] 남은 Chat 모드별 표시·전체 팔레트 전환·디렉터리 이름 formatter를 확인한다. 원격·메타맵·시스템 뉴스의 실제 UI와 기타 dot 명령도 잔여다. 입력(573, 574)과 설정·재실행(614) 완료를 전체 Chat GUI 완료로 확대하지 않는다.
- **651 정적 경계:** 번개 색 범위228..245/현재228·18, 합법 현대 모드의 비flip과 색 소비자는 기존 연결을 확인했다. 전체 GUI/픽셀 완료는 아니다. PlayerNoMake writer `00433c30`은 객체 수신 생성/검증 부모라 후순위MP 무시험 사전 구현으로 잇는다.
- [x] BaseFile 위치/EOF/길이/Close/소멸/4096BYTE 암호 쓰기와 실제 객체 덤프 쓰기/소멸은651에서 연결했다. 원문322행/1288관측·집중5개와 새 Unicode 디스크 장치를 확인했다([651](docs/exe/cpp-basefile-io-20261031.md)). 전체 FILE ABI/읽기/쓰기 소비자 완료로 확대하지 않는다.
- [x] 생성/일반·부분 파일 열기/키 교체/암호 부분 읽기와 실제 디스크·TAFF 소비자는652에서 연결했다. 원문235행/940관측·집중15개·Unicode 일반/빈 파일과 두 암호 엔트리를 확인했다([652](docs/exe/cpp-basefile-read-key-20261031.md)). 명시 작업 버퍼의 잔여 BYTE와 실제 미정 스택 값 자체를 구별한다.
- [ ] 전체 BaseFile41b4c0의 FILE/예외 객체 ABI·다른 쓰기/키/스트림 소비자·실제 손상/할당·부분 I/O 실패 수명, 보조 음향과 비미션 전체 랭크 상승을 연결한다.0044fa00은004b54e0 재접속 저장까지 복원해야 한다. CD 일반 열기는4ba5d0/부분 파일은4ba630이며 자동 정밀 후보의 반대 대응을 그대로 따르지 않는다(652).
- [x] 장치 경로 직접 읽기 실패 뒤 재조회는644에서 실제 원문 정책으로 연결했다. FileSpec/접두 BYTE/모든 조회·실패1024사례/2048관측을 고정했고 실제 한글 경로/드라이브 상대·빈 파일·UTF-8 중간 절삭까지 집중14개를 통과했다. 기존 Help의 경로 조합도 같은459c60 몸체를 공유한다([644](docs/exe/cpp-basefile-read-policy-20261031.md)). 전체FILE ABI/쓰기 완료로 확대하지 않는다.
- [x] 기본/보조 TAFF는643에서 독립 목록·Win32 자동*.tarc 등록·네 단계 읽기/FileFinder·실제 Client CD/Help source4에 연결했다. 한글😀 경로의 기존 ANSI 열거 실패도 UTF-8/W 경계로 수정했다. PE27사례/54관측·최종집중11개·추가 tarc 배포 계획과 보호2630불변을 확인했다([643](docs/exe/cpp-taff-secondary-20261031.md)). 전체BaseFile/643 전체34/새ZIP 완료로 확대하지 않는다.
- [x] 비로컬 보상은642에서 실제 Session에 연결했다. 별도144BYTE Core 기술 비트와 getter→**5000 가산**→정수화→정책 setter를 사용한다. 디컴파일에서 빠진 fadd를 PE504관측으로 확인해599 설명을 정정하고 기존 직접 SP 쓰기 대체를 제거했다([642](docs/exe/cpp-foreign-ritual-reward-20261031.md)). 실제 포획/희생·신규 MP 검사는 생략했다.
- [ ] 조기 희생 안내를 확인한다. 실제 missionPost 0의 일반 전투 경로라 후순위로 남겼다(615).
- [ ] 원본 32비트와 현재판의 스택 배치·미정값 자체 일치 여부는 별도로 판단한다(612).
- [ ] 연결 활동 frame22/주기64·timeout/UI·endpoint/server/boss와 실제 리슨/수락·22칸 송신 writer는 멀티플레이 후순위의 무시험 사전 구현 범위로 잇는다.
- 근거는 [월드 장기 소유](docs/exe/cpp-state-world-owner-plan-20261025.md)·[State Client 공백](docs/exe/cpp-state-cleanup-client-gap-20261025.md)·[직접 전역 소유/반납](docs/exe/cpp-state-cleanup-global-owners-20261025.md)을 따른다. 오래된 PLAY 목록의 완료 여부는 현행 코드/후속 단위와 대조한다. ClearWorld의 오래된 ‘후속’ 주석과 과거 428 문서만으로 미연결 버그를 주장하지 않는다.

### 5. 원본 동일성 대조 — 사용자 확인 대상

전체 동일성은 사용자가 실행·플레이하며 확인한다. AI 작업 대상이 아니며 아래 항목이 남아 있어도 다음 단계로 넘어간다. 기능을 구현할 때 해당 부품을 원문과 대조하는 일은 계속 AI가 한다.

- 목표 TYPE의 대기/건설/발사/피격/파괴/포획/희생 프레임·방향·색표·그림자·발 기준점·가림/파편/지형 효과와 캠페인 2·3장 추가 TYPE의 전체 수명
- 메뉴·브리핑·생산/지식·선택/사거리/비용·커서/회전/회수/업그레이드·우클릭/단축키·Pause/Help·카메라/도킹/View/Chat/PlayerList의 남은 조합/픽셀. 좁은 기능 완료를 전체 동일성으로 세지 않는다.
- 건설/발사/적중/출격/귀환/연소/붕괴/포획/의식/승패/음악의 파일·시점·반복·위치, 정지 중 시간/난수와 정상 간격 공통 AI
- 장시간 플레이·메뉴/데모 반복의 자원 누적·정지/크래시
- Credits의 원본 픽셀/음향 전체 동일성. 566은 no-audio/일부 현재 캡처이며 원본 실행 비교가 아니다.
- 녹화의 HDR 밝기·방송 음성을 게임 효과로 오인하지 않는다. 표시 변환 픽셀 일치와 전체 외관 동일성은 별도다.

### 6. 자연 완주·테스트 지원 — 사용자 확인 대상

모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주는 사용자가 실행·플레이하며 확인한다. AI 작업 대상이 아니며 아래 항목이 남아 있어도 다음 단계로 넘어간다. 자동 공략과 테스트 지원은 별도 요청이 없으면 진행하지 않는다.

- Early3→6, 캠페인 1-1의 포획/운반/희생/승패 → 1-2, 나머지 1장 → 2/3장, Training 9개와 선택지, 데모의 자연 완주. 현재 제품의 자연 완주가 완료됐다고 쓰지 않는다.
- 실제 승리 뒤 완료값 0→1 자동 저장과 재실행 후 다음 미션 신규 해제. 명시 키의 저장/재실행 해제는 578에서 확인했다. 기존 완료값 1 보존은 신규 해제의 증거가 아니다.
- **549 자동공략 잔여:** `CampaignForwardBattle_NaturalTempleAssault`는 합법 사격 배치 지점 부재로 실패했다. 제품/전술 입력 원인은 미확정이고 준비 전략은 미채택이다. 원래 함수/전략/승리 단언은 유지하며 별도 요청이 있을 때 원문과 대조한다. 전체 C++ 통과로 기록하지 않는다.
- TEST02의 명시 로컬 owner AI 옵션은 미구현이다. 일반 demo 0에서 원본 공통 AI를 로컬 owner에도 쓰며 `gate.demo=1` 우회로 튜토리얼/Demo 사건/타이머를 바꾸지 않는다.
- **TEST02 포획/희생 보류:** 사용자 수정판 `3c12b4e7`/Fort 5282바이트의 현재 SHA/배치를 다시 확인하고 옛 좌표를 재사용하지 않는다. 기술은 크로스보우·썬 디스크 발사기·맨오워풀·데빌메이커 우선이며 훨리베이스는 제외한다.
- 로컬 AI의 SP/Tech/이름/Off/NoTemple/동맹/능력은 적 AI와 따로 준비하고 공유 난수 차이를 기록한다. 기본 게임/프로필에 섞지 않고 검사 조작임을 주석/기록에 표시한다. 우리 편 AI·빈도/SP 추가는 [검토](docs/exe/cpp-test-local-ai-and-sp-feasibility-20261021.md)만 했으며 구현 완료가 아니다.

### 7. Windows 10/11 호환성과 전체화면

- [ ] 화면 캡처 정상화 뒤 물리 입력 QWASZX·ESC·Home·포커스를 검증한다(637). 관측이 안 되면 입력을 보내지 않는다. 앱 내 검사 입력을 물리 입력으로 합산하지 않는다.
- [ ] Windows 10에서 다른 모니터/DPI, 실제 다른 앱 포커스 전환, DD 조정/확인 UI의 남은 경계를 검증한다. 메시지 주입을 실제 앱 전환으로 합산하지 않는다.
- [ ] 실제 PCM 출력과 사용자 활성 전체화면의 물리 출력을 확인한다. 594 세션에는 waveOut 장치가 없고 전경 창이 NULL이었다. 실제 CD 삽입 W 경로도 미검증이다(595).
- [ ] 전체화면 종료 후 반복 재실행과 화면비/스크롤 회귀를 유지한다. 원본과 다른 호환 처리는 이유를 한국어 주석에 남긴다.
- Windows 11 실기 검증은 단계 전환 조건이 아니다. 위 항목을 Windows 10에서 마치면 다음 단계로 넘어간다(1.1절).

### 8. 배포·문서·후속 단계

- [ ] 2.4절의 미해결 이슈와 제거된 임시 Game 버튼을 전제한 옛 스모크 입력을 처리한다.
- [ ] 싱글플레이/UI/호환성 뒤 TCP/IP 로컬 멀티플레이·Outpust/전용 분기를 디컴파일 근거로만 사전 구현한다. 신규 단위/게임/실제 통신 검사는 하지 않고 동작·동일성·안정성 미검증을 기록한다. 공식 서버 복구는 가정하지 않는다. 메타맵 번개 `00479650`의 실제 기록 owner/getter와 MP Challenge 참가자/전투방 이미지·텍스트 `478d10`도 이 범위에 포함한다. 일반 전투 !state.dat 저장/복구·startup StateReconnect/서버 이전·Zone0/메타 Poll/4c1e70 시계 패킷도661의 정적 분류를 따라 포함한다. 기존 질문/삭제 UI는 유지하며 신규 검사는 하지 않는다.
- [ ] **4~7차 후순위:** 3차 멀티플레이 뒤에 1.1절 순서대로 진행한다. 모두 미착수다.
  - **5차 착수 전 확인점:** 현재 저장 파일은 `<게임 자료>/d/options.cfg`와 `fullscreenStateFile.dat`이고(578) 배포 스크립트는 이 파일들을 압축에 넣지 않는다. Edit 목록은 원문 50개 상한이며 `originals/d`의 TEST01/TEST02 같은 사용자 맵은 읽기 참조로만 쓴다. 새 위치로 옮길 때 기존 `options.cfg`를 이어받을지, 두 위치의 우선순위를 어떻게 둘지는 착수 때 정하고 원본과 달라지는 이유를 한국어 주석에 남긴다. 저장 위치 이동은 전체화면 전환 후 재실행 오류가 없어야 한다는 요구와 함께 시험한다.
  - **7차:** 안티앨리어싱과 FSR 1.0은 각각 Options에서 켜고 끄며 기본값은 꺼짐이다. 참고 저장소는 1.1절의 FidelityFX-FSR이다.
- 60·120프레임과 CD 10.72 전체 복원은 별도 후순위다. CD의 Windows 10 구동 불가 해결책도 해당 작업 때 문서화한다.
- README는 제품이 바뀌면 해당 근거로 다시 갱신한다. DN/WB의 별도 상태와 cpppj 과거 기록은 보존한다.

## 4. 운영 규칙과 유효한 사용자 결정

- **스테이지 실행 검사 빈도(2026-10-31 최신 사용자 지시):** 현재 진행 중인645의 검사는 마저 진행한다. 다음 주기부터는 게임을 실행해 각 스테이지에 진입하고 기다리는 검사를 훨씬 드물게 하며, 더 많은 기능 묶음을 구현한 뒤 한꺼번에 수행한다. 소수의 작업 단위가 끝날 때마다 전체34 또는 동일한 진입 검사를 반복하지 않는다. 중간 확인은 빌드·정적 원문 대조·필요한 집중 검사 위주로 한다. 새 실제 실행은 변경과 직접 관련된 확인이 필요한지 먼저 판단하고, 이미 통과한 진입을 관례로 반복하지 않는다.
- **CHEAT 표시(2026-10-28 사용자 확인):** CHEAT 문구가 있는 항목은 게임 진행 중에 나타나지 않고 Edit 모드 편집에서만 나타날 수 있다. Edit 안에서도 각 항목의 원본 추가 조건을 적용한다.
- **긴 검사 묶음(2026-10-25, 주기는 위2026-10-31 지시로 대체):** 매 단위 34경로·긴 Early 완주·전체 CTest를 반복하지 않는다. 원문 근거·필요한 집중 검사/증분 빌드·인계/커밋을 하고 더 많은 기능 묶음 뒤 같은 최종 제품으로 통합한다. 이전의3~5단위 기준을 자동 실행 조건으로 사용하지 않는다. 실제 입력/타이머/저장은 변경과 관련된 확인이 필요할 때 수행한다. 자연 완주는 사용자 확인 대상이므로 통합 검사에 넣지 않는다.
- **기록:** 정적 대조·명시 입력·컴파일·C++ 검사·GUI 관측을 구별한다. 최초 실패/교정 원시는 지우지 않고 기대값을 제품에 맞춰 약화하지 않는다. 서로 다른 제품의 성공을 합산하거나 준비본을 제품 통과 수에 넣지 않는다.
- **실제 실행(2026-10-27):** 메뉴/스테이지 진입·수 초 진행의 짧은 검증은 가능하다. 스테이지 클리어는 사용자 확인 대상이다. 실행 전 확인 여부는 0.2절과 AGENTS.md를 따른다.
- **Test Battle 언어파일(2026-10-27):** 대응 `.english` 부재 시 종료는 원본 동작이며 개선은 보류다. 임의 원본/기본 시나리오 fallback은 추가하지 않는다.
- **테스트 조작:** `--test-bridge-patterns` 등은 원본 분석 결과와 구별해 주석/기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다. 로컬 AI/TEST02 방침은 3절 6번을 따른다.
- **실행 파일 이름:** Windows 게임 실행 파일은 `Netstorm.exe`다. CMake 대상은 NetstormCpp를 유지하고 OUTPUT_NAME으로 산출물 이름을 정하며, import library는 정적 `netstorm.lib`와 겹치지 않게 ARCHIVE_OUTPUT_NAME NetstormCpp로 분리한다(580, 581). 이전 단위의 보고서·원시 로그에 남은 `NetstormCpp.exe`는 당시 이름으로 보존한다. 원본 `Netstorm.exe`와 구별할 때는 프로세스 이름이 아니라 전체 경로를 쓴다.
- **실행 방식:** 인자 없는 실행은 GUI이고 `--help`와 명시적 `--run`·분석 명령은 유지한다(637). 자료는 실제 EXE 옆 `game/netstorm.tarc`를 우선하며 cwd의 다른 게임이나 저장소의 보호 원본을 자동 탐색하지 않는다.
- **보호:** AGENTS.md와 `originals/`·`originalCD/` 기존 파일은 수정/삭제하지 않는다. AGENTS.md가 허용한 `options.cfg`·`fullScreenStateFile.dat`는 예외다. 게임 실행 전후 원본 전체 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 Visual Studio 18(2026)/MSBuild 18.x를 쓴다. 최근 명령은 `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`이며 앞선 컴파일러/CMake 자식이 모두 끝난 뒤 시작한다([빌드 안내](docs/cpp-build.md)). HJOW-X3D는 Visual Studio 17(2022)/v143이고 cmake가 PATH에 없어 BuildTools 내장 cmake를 전체 경로로 부른다.
- **UTF-8:** 새 문서/코드와 한국어 주석은 UTF-8이다. PowerShell 5.1의 Python stdin은 `$env:PYTHONUTF8=1`만으로 부족하므로 한국어를 넘길 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정하거나 UTF-8 파일을 사용한다. 원시 로그/Native fixture는 인코딩 정리로 덮어쓰지 않는다.
- **Git:** CRLF 보존을 위해 확인/stage에 `git -c core.autocrlf=false`와 필요 시 `-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정을 바꾸지 않고 해당 작업 파일만 stage한다. 최초 관측 바이트가 중요한 원시 TSV는 `-text` 속성으로 고정한다(589). 커밋은 `YYYYMMDD-NN PJ 요약`이며 당일 마지막 번호를 `git log`로 확인한다(cpppj CP, dotnetpj DN, webpj WB, 공통 CM). 원격 push는 금지다.
- **도구:** `tools/cpp_solo_entry_smoke.py`(34경로, 데모는 `--demos`), `tools/cpp_demo_start_smoke.py`, 자연 Early1/2용 `tools/cpp_tutorial_harvest_smoke.py`·`tools/cpp_tutorial_two_smoke.py`, ESC용 `tools/cpp_game_menu_smoke.py`·`tools/cpp_game_menu_pause_smoke.py`. 실제 호출 인자/최근 batch는 작업 PC의 `extracted/` 드라이버를 따른다.

## 5. 주요 참고 문서

- [C++ 빌드](docs/cpp-build.md), [cpppj 로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md)
- [모듈 맵](docs/exe/modules.md), [정밀 디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-10-23 양판본 정밀 디컴파일](docs/exe/cpp-decompilation-20261023.md)
- [원본 자동 탐험 analyzeManager](docs/analyze-manager.md), [유닛 25종 점검](docs/exe/cpp-unit-restoration-20261022.md), [의식 집중 검증](docs/exe/cpp-ritual-runtime-focused-20261021.md)
- [준비본 보관](docs/exe/prepared/README.md): 443/444는 통합됐고 469 준비본의 Subscriber/공유표/NewFort/섬 추가는 후속 단위에서 채택됐다. 개별 보관본의 미적용/미빌드 문구는 보관 당시 상태다. 보관 base/patch/manifest/142파일 원시는 유지한다.
- [Edit 왕복/냉각](docs/exe/cpp-ice-cooling-20261027.md), [새 맵 실제 저장](docs/exe/cpp-editor-new-map-live-20261027.md), [기존 맵 섬 추가](docs/exe/cpp-editor-existing-islands-20261027.md)
- 단위별 상세/최초 실패/증거 해시: [과거 이력](LEFT_JOBS.old.md), [단위504까지](LEFT_JOBS.old.md#archive-20261027-through-504), [단위549까지의 0절](LEFT_JOBS.old.md#archive-20261029-section0-through-549), [단위561까지의 3절](LEFT_JOBS.old.md#archive-20261029-section3-through-561), [단위638까지의 전체 원문](LEFT_JOBS.old.md#archive-20261031-through-638)
- **2026-10-31 문서 정리:** HJOW-X3D에서 문서만 변경했다. 빌드·검사·게임 실행·디컴파일은 하지 않았고 제품과 마지막 디컴파일 호스트는 그대로다. 보관본의 “다음”이나 “중지”는 당시 이력이며 현재 작업 순서로 재사용하지 않는다.

## 인수인계 문서 임시 분할

dotnetpj 와 병행 작업하기 위해,
이 LEFT_JOBS.md 에는 당분간 cpppj 및 디컴파일 관련 진행 상황과 그 인수인계 내용을 적는다.
dotnetpj 진행 상황과 인수인계 내용은 LEFT_JOBS.dotnetpj.md 에 적는다
cpppj 1차 목표 달성 및 윈도우 10, 11 호환성 작업, MCP 추가 작업까지 완료되어 dotnetpj 구현에 cpppj 를 활용할 수 있게 되면, 인수인계 문서를 다시 LEFT_JOBS.md 로 통합한다.

webpj 진행 상황과 인수인계 내용은 AGENTS.md에 따라 LEFT_JOBS.webpj.md에 적는다.
