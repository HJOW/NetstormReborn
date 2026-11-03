# LEFT_JOBS — cpppj 및 공통 인수인계

cpppj와 공통 사항의 **현재 기준, 완료 범위, 남은 작업, 유효한 사용자 결정**을 정리한다. dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md), webpj는 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다.

**2026-11-03 문서 정리:** 단위 808까지의 코드·보고서·커밋과 대조했다. 정리 전 전체 원문과 단위별 상세 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md#archive-20261103-through-808)에 보존했다. 완료 범위는 2절, 재개 후 작업은 3절을 따른다. 2026-11-03 사용자 `/goal` 재개 요청으로 cpppj 구현 목표를 다시 진행한다. 단위 809는 `a8f87f62 / 20261103-223 CP`로 완료했다. 단위 810→811에서 CanonDecoder의 현재 제품 소비자 연결과 집중 검사를 완료했다. checking 잔여30과 나머지 객체 메뉴/커서·Edit·수명·후속 단계는 계속 진행 중이다.

## 0. 재개 전 확인 사항

### 0.1 현재 값 (2026-11-03, 단위 813 완료)

| 항목 | 현행 기준 |
| --- | --- |
| 진행 상태 | **전체 목표 진행 중.** 812는 `5d714fe8 / 20261103-226 CP`, 813은 문서와 함께 로컬 커밋한다. [813 기록](docs/exe/cpp-squid-level-diagnostic-20261103.md) 참조 |
| 현재 제품 | **813 / 표시 11.01.** 기본/Factory 레벨 변경의 실제 현재 Client 진단 복귀·상태 BYTE 저장·표시 갱신 연결 |
| 실행·플레이 가능 여부 | 무인자 GUI 시작과 미션 플레이 경로가 있다. 일반 31+데모 3의 마지막 전체 진입 검증은 **645의 644 제품**이며 현재 813의 전체 회귀 결과가 아니다 |
| 원본과의 동일성 | 부품별 원문 대조와 집중 검사를 누적했다. **전체 동일성·모든 시나리오 자연 완주는 미확인**이며 사용자 확인 대상이다 |
| EXE SHA-256 | `f31db0a9136c9a00002104e92411c570ed29ab671e7c3cad55d84bf9d9d87b73` (813 / 11.01) |
| 최신 빌드·집중 검사 | 813 Release 빌드3회 경고·오류0, 집중4개(새1) 최종 실패0. 기존 양판본 레벨 Native96행의 전체슬롯/진단 식·파일·행 대조. 최초 검사 입력 free비트 오류1건 교정·로그 보존·제품/fixture 불변. 게임·GUI·공격·MP0 |
| checking 잔여 | **30함수 = 부모 21 + 부분 6 + MP 분류 3.** `00425860`은 810→811에서 현재 제품 소비자까지 연결 완료. 다음은 우선순위1 객체 메뉴/커서 상태 효과·수명과 나머지 부모 |
| 마지막 GUI | **794에서 793 제품**의 시작/메인 메뉴를 확인. 813 GUI 통과로 합산하지 않는다 |
| 전체 검사 기준 | 실행 대상66/전체 빌드 대상71. 전체71 마지막 컴파일은779. 전체 C++ 마지막 실행은549의3389개 중7개 실패(580단언); 마지막 전체 통과는510의508제품3337개. 이후 집중 통과를 전체 통과로 쓰지 않는다 |
| 전체 34·ZIP | 645의644 제품, 최종34/34 진입·비정지 시각 증가·창DC RGB·Native Frame 확인. ZIP SHA-256 `6485febc36080dc90e9eec43965a842f85d39e81c82a003a96b83b34b3694b73`, 1376항목/게임 자산1358개, 한글😀 경로 압축 해제 실행 확인 |
| 보호 기준 | **2739개/fixture919개 전부 불변.** 새 fixture0, 다음 기준은 `extracted/cpp-resume-813/protected-after.json` |
| 마지막 전체 정밀 디컴파일 | **HJOW-Athlon (192.168.0.94) / 2026-10-23**. 이번 정리는 같은 호스트에서 했고 디컴파일 산출물은 변경하지 않았다 |

### 0.2 재개 절차

1. 사용자 재개 요청 뒤 AGENTS.md·이 문서·LEFT_JOBS.dotnetpj.md를 다시 읽는다. webpj 작업이면 해당 인계도 읽는다. `git status`와 `git fetch`로 다른 PC/세션 변경을 확인하며 push는 하지 않는다.
2. 현재 호스트가 마지막 디컴파일 호스트와 다르면 cpppj 작업/디컴파일 분석 전에 `tools/ghidra/refine_all.ps1`로 다시 디컴파일하고 완료 호스트를 이 문서와 작업 문서에 갱신한다. HJOW-X3D의 2026-10-27 중단 산출물은 불완전하므로 쓰지 않는다.
3. 실제 게임·별도 창 실행은 AGENTS.md의 확인 규칙을 따른다. 예외는 vm-debian-codex(10.0.0.15), VM-W11-Codex(10.0.0.17), HJOW-Athlon(192.168.0.94) 및 사용자가 직접 요청한 원본 수동 컨트롤/녹화 분석의 해당 단계다. 다른 PC는 실행 목적·필요성 설명 후 명시적 확인이 필요하다.
4. 최신 원시는 `extracted/cpp-resume-813/`이며 Git 제외·작업 PC 전용이다. 커밋되는 증거는 `cpppj/*.json`·`docs/exe/cpp-*.md`다. 다른 PC에 원시가 있다고 가정하지 않는다.
5. 다른 세션의 dotnetpj/webpj·staged 파일·실행 출력은 포함하지 않는다. 한 단위를 마치면 0.1절·해당 완료/잔여 항목·단위 문서를 갱신하고 해당 파일만 로컬 커밋한다.
6. 네 Gun PostPop 진단은809, CanonDecoder 현재 진단/소비자는810→811, 오벨리스크/스펠 전환 실제 타입 변경 진단 복귀는812, 기본/Factory 레벨의 실제 진단 복귀·저장/표시는813에서 완료했다. 다음은 우선순위1의 나머지 객체 메뉴·커서 상태별 효과와 수명, 남은 checking 부모다.

### 0.3 유지되는 제한과 보류

- **사용자 직접 확인:** 모든 시나리오의 자연 완주·전체 동일성, 실제 사제 포획/희생·특정 적 공격·시나리오 승리 재현. AI는 별도 요청 없이 자동 공략/테스트 지원을 진행하지 않는다. 짧은 메뉴·진입·기능 검증과 부품 원문 대조는 가능하다.
- **공중 3종:** 훨리기그·맨오워·더스트 데빌은 정적 분석·코드 작업만 한다. 새 전용 생산·생성·우클릭·행동 관측/검사는 하지 않는다.
- **멀티플레이:** 비멀티·UI·Windows 10 호환성 뒤 원문 근거로 사전 구현한다. 신규 단위 검사·게임·실제 통신 검사는 하지 않는다. 사전 구현을 동작/안정성 검증 완료로 쓰지 않는다.
- **사용자 보류:** 스펠 효과 도움말 그림 추가 분석(530, 임시 숨김 유지), 연녹색 직사각형 추가 분석(원본에도 관찰, RDP 원인 미확정). `.english` 부재 Test Battle 개선은 영구 보류가 아니라 **8차 요구**다.
- **환경 한계:** 물리 입력/캡처는762의 새 부팅·RDP세션1에서도 두 번 timeout이었다. 실제 환경 변화 전 재시도하지 않는다. 다른 모니터/DPI·실제 앱 전환·실제 PCM/청취·사용자 활성 전체화면의 물리 출력·실제 CD 삽입은 미검증이다.
- **원문/호스트 경계:** 32비트 FILE/FileSpec/SEH·스택 배치/미정값 자체 일치, 부분 I/O/실제 할당 실패와 전체 진단 UI는 별도 한계다. 612의 반사 입력, VFX signed WORD/정상 표본·양수/회전0/원색의 근거를 전체 입력·픽셀로 확대하지 않는다.

## 1. 현행 목표

### 1.1 cpppj 단계 (AGENTS.md 기준)

| 단계 | 내용 | 현재 상태 |
| --- | --- | --- |
| 1차 | 원본을 디컴파일해 C++로 최대한 똑같이 복원한다. 기준은 10.78이며 멀티플레이·Outpust는 제외한다. 문서화할 때 10.72 관련 내용도 기록한다. | **진행 중.** 34경로 진입·진행은 가능하다. AI 작업으로 남은 것은 UI/Edit/시나리오/수명이다. 자연 완주와 전체 동일성은 사용자 확인 대상이다. |
| 2차 | Windows 10/11에서 오류 없이 실행한다. 전체화면 DirectDraw 오류와 전체화면 종료 후 재실행 오류를 수정한다. | **진행 중.** Windows 10 호환 전체화면과 저장 재시작은 확인했다. 남은 Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다. 10.72의 Windows 10 구동 불가 해결책은 10.72 작업 때 문서화한다. |
| 3차 | 멀티플레이·Outpust. 공식 서버는 없어졌으므로 TCP/IP 로컬 네트워크만 구현한다. | 전용 단계는 미착수. 채팅 MP 분기·707의 공유 의식 선택/비미션 UI·709의Zsend/송신장치처럼 일부만 정적으로 작성했고 신규 MP 검사는 하지 않았다. |
| 4차 | **로컬MCP**로 dotnetpj 분석을 위한 모든 게임 조작·화면 캡처·소리 포함 녹화. **16:9 최대 1920×1080 지원**이며 이 단계에서 그래픽 크기 스케일 조정은 불필요하다. | 미착수. 기존 화면비 처리/원문BMP 수집과 별도의 완료 조건이다. |
| 5차 | **일반 권한·읽기 전용 설치 정상 동작.** 설정/진행은`%USERPROFILE%\Saved Games\Netstorm\Saves\`, 신규 맵은`CustomMaps\`다. 모든 자동 생성 파일/디렉터리(임시/백업/잠금/장부/캡처/진단 포함)와 대응 reader도 같은Netstorm하위로 옮긴다. 원래/사용자 맵 병행 읽기와 Edit 스크롤/무제한도 포함한다. | **구현 미착수.** 사용자aaafbbc4를733에서 반영하고12파일군/50정적후보를 고정했다. 실제 권한/전체 경로 검증은 남는다. |
| 6차 | 한국어 지원 | 미착수 |
| 7차 | Options의AA/FSR1.0 선택은 기본 off. FSR 지원 환경에는2560×1440/3840×2160을 추가하고 해당 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 목표로 업스케일한다. [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)를 참고한다. | 미착수 |
| 8차 | Edit 전용 시나리오 구성 UI(초기 자본/테크·적·동맹 등), 새 맵의 기본 english 자동 생성, english 부재 Test Battle 종료 개선 | **미착수.** 사용자98b2f20c의 AGENTS/TODO 갱신을728에서 반영했다.1~7차 뒤 진행한다. |

**단계 전환 기준 (2026-10-31 사용자 결정):**

- **자연 완주·전체 동일성:** 모든 시나리오(Early Missions, Priest Training, 캠페인, 데모)의 자연 완주와 전체 동일성은 사용자가 직접 실행·플레이하며 테스트한다. AI 작업 대상이 아니며, 단계에 남은 작업이 이것뿐이면 다음 단계로 넘어간다.
- **Windows 11:** Windows 10에서 오류 없이 구동되면 Windows 11에서도 문제가 없을 가능성이 높다고 본다. Windows 10 확인을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다.
- **제보 처리:** 사용자가 플레이 중 오류나 원본과의 차이를 제보하면 해당 항목을 AI 작업으로 올린다.

60·120프레임 지원과 CD 10.72 전체 복원은 별도 후순위다. cpppj는 Windows용만 개발한다. dotnetpj 후순위 요구에도 4~8차가 포함되며, webpj는 Edit·멀티플레이를 제외한다.

### 1.2 현재 목표 범위 ([TODO.md](TODO.md))

- Early Missions 6개, Priest Training 9개, 캠페인 전체 16개, 데모 3개를 사용자가 플레이할 수 있고 원본 10.78과 완전히 같게 동작하도록 한다. 자연 완주와 전체 동일성의 확인은 사용자가 한다(1.1절).
- Edit 메뉴의 커스텀 맵/시나리오, 메인 메뉴의 비멀티 항목, 좌측 덱·미니맵·다리 단축키 QWASZX·ESC 메뉴를 구현한다.
- Windows 10/11 호환성을 해결한다. 원본과 다른 호환 처리와 이유는 한국어 코드 주석에 명시한다. 전체화면, 4:3/16:9/16:10, 가장자리 스크롤 요구를 유지한다.
- 그 뒤 멀티플레이를 무시험으로 사전 구현하고 4~8차를 진행한다(98b2f20c의 AGENTS/TODO 갱신).
- 버전 표기는11.01이며 Credits 목록에 세 번째 Netstorm 11.01 Patch Credits와 동일한 형식의 메시지 창(제목NetStorm 11.01 Patch Credits, HJOW 개발/AI 사용 설명)을 추가한다(추가 AGENTS.md).
- 외부 도메인/IP 접속 기능을 로컬 서버와 구별해 문서화하고, 클라이언트 유효성 검사는 비활성화한다(갱신 AGENTS.md,684 적용).
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

<a id="agents-reflection-20261102"></a>

### 1.3 AGENTS.md 요구 반영 현황 (2026-11-03 정리)

AGENTS.md는 수정하지 않는다. 단계별 요구는 1.1절이 기준이며, 즉시 적용 요구의 cpppj 상태는 다음과 같다. DN/WB의 구현 완료 여부는 각 프로젝트 인계에서 확인한다.

| 요구 | cpppj 상태·근거 |
| --- | --- |
| Windows 실행 파일 `Netstorm.exe` | **완료(580·581).** CMake 대상명 NetstormCpp와 import library 이름은 유지·분리한다 |
| 11.01 표기·Credits 세 번째 항목 | **완료(685).** `Netstorm 11.01 Patch Credits`, 창 제목 `NetStorm 11.01 Patch Credits`, HJOW 개발·AI 사용 안내, 기존과 같은 Back/Cancel. [기록](docs/exe/cpp-product-1101-20261101.md) |
| 외부 도메인/IP 접속 문서화·클라이언트 유효성 검사 비활성화 | **완료(684).** [문서](docs/exe/cpp-external-services-20261101.md)·[주소 목록](cpppj/external-endpoints-20261101.json). 구 서버 검증/security.dat writer·검증 getter·Not Validated 안내는 연결하지 않고 성공1도 합성하지 않는다. 사용자 선택 Help 링크와 로컬 LAN은 별도다 |
| Windows 10/11 AMD64·Windows 11 ARM 고려 | 현재 EXE AMD64(PE0x8664). ARM 빌드/실기 미검증이며 Windows 11 실기는 단계 전환 조건이 아니다 |
| 4차 로컬 MCP·1920×1080, 5차 모든 자동 생성 파일/reader 이관·무제한 목록, 6차 한국어, 7차 AA/FSR1.0·고해상도, 8차 시나리오 UI | **구현 미착수.** 기존 화면비·BMP·원문 언어 처리와 구별한다. 5차는733의 [12파일군/50정적후보](docs/exe/cpp-phase5-file-inventory-20261102.md) 조사만 완료 |
| 한국어·UTF-8·주석·용어 | 응답·새 코드 주석은 한국어, 새 문서/코드는 UTF-8. 한국어 글꼴은 `fonts/`의 D2Coding. 새 네임스페이스·상수·함수·구조체·클래스·멤버·반복문에는 설명/사용법 중심의 한국어 주석을 단다. 명칭은 AGENTS.md의 번역/발음 표기를 따른다 |

## 2. 현재까지 완료한 범위와 검증 한계

### 2.1 완료 목록 — 부분 완료 범위를 명시

`[x]`는 아래에 적힌 범위의 완료다. 전체 기능·전체 동일성 완료를 뜻하지 않는다. 오래된 “다음/미연결” 대신 후속 단위의 실제 연결 근거를 따른다. 단위별 검사 수·실패/교정·당시 해시는 [정리 전 2.1절](LEFT_JOBS.old.md#archive-20261103-through-808)과 링크한 단위 문서에서 확인한다.

- [x] **시작·메인 메뉴:** 무인자 GUI·EXE 옆 `game/` 우선·cwd 독립(637), 기본 8/Intro 조건부 9버튼·Help/Options 팝업·실제 부모/모달/HTML 수명·45초 autoDemo(697~700), Credits 자동 전환(566), Help/Technical Help(603). [메인 부모](docs/exe/cpp-main-menu-parent-20261102.md), [휴대 실행](docs/exe/cpp-portable-startup-20261031.md)
- [x] **게임 UI·단축키:** QWASZX·독립 미니맵·Player List·Send Money, Shift 다중 선택/30칸 그룹 이동(701), N/Shift+N(702), P/R(703), H/F4/F5/Ctrl+F5(704), C/c(717), Tab 최근 5칸(718), 숫자/Shift 저장 10칸(719), U/u 손실 위치(751), F6 정지 경계(741), ESC 선택 정리/취소(742), 현재 demo 관문(743), 전체화면 F12(744), Pause(753), T/t(754), D/d(756), outside/모달/정지 분배(761). [선택 키](docs/exe/cpp-selection-key-input-20261102.md), [저장 카메라](docs/exe/cpp-saved-camera-key-20261102.md), [입력 분배](docs/exe/cpp-outside-input-routing-20261103.md). 앱 내 입력·집중 검사이며 물리 키 전체 검증은 아니다.
- [x] **객체 메뉴·편집 입력:** 83VT의 작성 13/비멀티 열기 5/특수 명령 9 연결(681), CHEAT 편집 전용(725), 실제 owner 변경/월드 갱신·Ctrl 다리 순환·모드1 취득/우클릭 취소·가상 ac/70·성공 음향 부모(747~750). [연결 표](docs/exe/cpp-object-menu-wiring-20261101.md), [편집 입력](docs/exe/cpp-editor-primary-selection-20261102.md), [메뉴 부모](docs/exe/cpp-object-menu-open-parent-20261102.md). 나머지 타입/상태별 명령 효과는 남아 있다.
- [x] **오벨리스크/스펠 전환 실제 진단 복귀(812):** 현재 Client 서비스와 실제 Factory/frame DWORD/QA WORD 바인더를 연결했다. 원문128+36행·집중6개(새3) 통과이며 공간/상위 입력은 명시 장치다. [812 기록](docs/exe/cpp-object-conversion-diagnostic-20261103.md). 전체 상태별 효과/GUI 완료와 구별한다.
- [x] **기본/Factory 레벨의 실제 진단 복귀(813):** 두 Session 연결점을 현재 Client 서비스로 바꾸고 원문 BYTE 저장→Factory 표시 순서를 유지했다. 양판본 원문96행·집중4개(새1) 최종 통과. 최초 검사 입력 오류1건 교정·제품/fixture 불변. [813 기록](docs/exe/cpp-squid-level-diagnostic-20261103.md). 전체 상태별 메뉴/GUI 완료는 아니다.
- [x] **Edit 대표 수명·저장:** 기존/새 Fort·섬 추가·전체 저장/Save as·이름/목록, 시나리오 시작값·방향별 동맹(590·597), 실제 Objects 스트림·손상 파일 정리(663~665), Early1 중단→Edit→시험 전투/Restart/복귀·Cancel/No(716), 같은 워크샾 owner0/2/8·Yes 저장/재적재(726). [Edit 수명](docs/exe/cpp-mission-editor-lifetime-20261102.md), [소유자 저장](docs/exe/cpp-editor-owner-save-20261102.md). 전체 도구/분기/실패를 완료한 것은 아니다.
- [x] **Edit Graph·지형 캐시:** 재적재 소속(721), 같은 후보 배치 직후 장부678/실제670·noGraph4(723), 두 섬/다리 연결·owner 변경·절단의 성분2→1→2와 원본 전체 출력(724), 지면99cache Client 수명(722). [대표 Graph](docs/exe/cpp-editor-graph-world-20261102.md). 원본에도 있는 장부 차이8은 정규화하지 않는다.
- [x] **시나리오 명령 연결:** 객체/송금·연기/번개·AddRandom/DestroyAt·기술 지급·음향/ViewSpot·HTML Input·기술 목록(676·682·683·686~690·705~708), 명시549문서/43이름·Menu 둘째 인자/큐 조사(715). [이름/메뉴 조사](docs/exe/cpp-dump-squids-20261102.md). 명시 목록 일치를 동적/TARC/Config 실행 전체 완료로 세지 않는다. Zsend/비미션 의식 요청(707·709)은 MP 정적 연결만 했다.
- [x] **공통 월드·AI·유닛의 복원 범위:** 공통 전략 AI의 현재 시각→비정지 AI→Kernel(320~322), 워크샾 25종 생성/이동/공격/표시(335~345), 의식·로컬/비로컬 보상·프레임말 통지(598·599·604·642), 세계 간헐천(609), 투사체/다리/펜스의 명시 입력 대조(552·611~613). [25종 점검](docs/exe/cpp-unit-restoration-20261022.md), [의식 검증](docs/exe/cpp-ritual-runtime-focused-20261021.md). 자연 플레이·공중 3종 새 전용 검사를 뜻하지 않는다.
- [x] **Client/Session·Guide·State의 연결 범위:** 장기 전역/Pool/Graph/Player·권한·커서·cleanup(425~486), 정상 State7 cleanup(1)/메인 cleanup(0)·Restart/Leave(638), 미션 진입 UI·7/8 확정(655~660), Guide 반납(671), 현재 Guide Fort 요청(713)·실제 Early1의 State1→7→8→14(714). [State 반복](docs/exe/cpp-state-repeat-20261031.md), [Guide 요청 실기](docs/exe/cpp-guide-fort-state-20261102.md). 다른 State/비동기 분기는 남는다.
- [x] **Renderer·카메라·HUD:** 실제 Frame·단일 표시 목록·구름/배율/부분 갱신·마나103 추가 Draw, 도킹/미니맵 내용·grid/debug 직접 writer 조사(618~639·648), ViewSpot·pan/Home/viewport(615·617·632·633·635·676), 부착 SP 투영(745), DrawWater(768), Hash/Fire/Spot/Board 진단과 BFS 배열 수명(776). [Renderer](docs/exe/cpp-renderer-frame-20261030.md), [진단 표시](docs/exe/cpp-renderer-diagnostics-20261103.md). 활성 caller 없는 grid/debug·관리 메뉴는 기본 UI에 노출하지 않는다.
- [x] **Chat·원문 언어:** 입력/자동완성/전달·공유 로그·기본 구분선 드래그/숨김/복원·설정 재실행(543·574·586·614·640), 일반 dot 명령/도킹 연결(desc는 MP 잔여), 언어 번호·다중 Xlat/요청·Chat 알림(710~712). [Chat 수명](docs/exe/cpp-ui-lifetime-current-20261031.md), [언어 요청](docs/exe/cpp-language-request-20261102.md). 전체 Chat 모드/번역 UI·한국어는 미완료다.
- [x] **파일 경로·시작·BaseFile:** 공유 256BYTE/10칸 이름 고리와 Client·VFS/저장/Options/Capture/CD/Help/Config(727~732), 디렉터리/모듈·6후보 setup/가상CWD/아카이브 등록/반납(734~737), Edit 조회/저장/삭제·직접 chmod 경계(738·740), MatchDisk/FileSpec(752), 전체 열기 부모·키/TAFF/current FILE·객체 덤프(755·757·763). [경로 고리](docs/exe/cpp-file-name-formatter-20261102.md), [BaseFile](docs/exe/cpp-basefile-open-policy-20261103.md), [Edit 쓰기](docs/exe/cpp-editor-mutation-ring-20261102.md). 손상/부분 I/O·다른 소비자·32비트 ABI는 남는다.
- [x] **파일 reader·자산 수명:** 존재조회/세 소비자(765), Help HTML stream writer(766), QuickHelp(767), DataManager/Template(769), Config(771), TYPE(777), GIF/타이틀 State42(781·782), 보조 효과음 현재 root(785), 팔레트(786), Xlat(787), 글꼴 캐시 순수부모→실제 FontStore(788→789), SHP 매핑부모→실제 공유 자산(790→791). [직접 74caller 분류](docs/exe/cpp-secondary-sound-root-20261103.md), [실제 SHP](docs/exe/cpp-native-shape-file-20261103.md). Script 캐시 직접 caller는792에서 CD/호스트의 비활성 10.78 경로로 분류했으며 기존 snapshot/캐시 정책을 유지한다.
- [x] **일반 명령·글꼴/음향 설정:** press 후처리(784), CreateAt/AI 직접 명령/MakeAi/조건부 AI 메뉴(770·772·773·775), Config/우선순위 명령(762), 글꼴·음질/글리프 캐시(764), raw ACP 31BYTE↔UTF-8 Config/디스크 경계(774). [일반 후처리](docs/exe/cpp-recent-command-tail-20261103.md), [글꼴 이름](docs/exe/cpp-font-face-encoding-20261103.md). MakeAi의 owner0 이전/삭제·전략·실제 설치 글꼴/다른 ACP/구형 무BOM 설정은 별도 한계다.
- [x] **checking 공유 owner와 후속 21부모:** 779의 기존 41관문 공유와 별도로, 접근점(783)·다리 낙하(793)/붕괴(795)·Gump 표시/소멸(796)·연소 3소비자(797→798)·건설 Form(800)·커서 취소(801)·펜스 2타입(802)·Gunanim(803)·Squid 표시(804→805)·Gump 부착(806)·Gun 통지(808)·네 Gun PostPop(809)·CanonDecoder(810→811)를 실제 소비자에 연결했다. [함수별 색인](docs/exe/cpp-checking-consumer-gaps-20261103.md), [808](docs/exe/cpp-gun-notify-checking-20261103.md). 순수부모 준비본을 실제 연결 완료로 중복 계산하지 않는다. 807의 Gun/바리케이드 서버 관문 4곳 owner 분리와 809의 네 PostPop 진단 위치·복귀까지 완료다.
- [x] **Windows 10 화면 전환의 확인 범위:** DIB 호환 전체화면·가장자리 이동·세 화면비·저장/종료/새 프로세스 적용, F12(744), 화면 변경/불법 요청 복구·DD 마우스 조정/취소/확인/15초 만료·저장 재시작(746). [화면 전환](docs/exe/cpp-screen-mode-transition-20261102.md). 다른 장치/DPI·물리 입력/출력은 미검증이다.
- [x] **배포 스크립트·빌드 선택:** `cpppj/package-cpppj.ps1`의 인자/대화형 경로·빈 값 취소·실행 파일/자산 동봉(562), 최상위 EXE 실행(637), Game/Tests/All 선택/기본 테스트 OFF(739). [빌드 모드](docs/exe/cpp-build-modes-20261102.md). 최신 제품 813의 새 배포 ZIP/전체 회귀는 아직 없다.
- [x] **사용자 제보 수정:** 덱 재활성 지연은 원문 Fast 1초로 복원(677), 간헐천 그림자는 원문 대상 변환/색표로 수정(678). [덱](docs/exe/cpp-deck-reactivation-20261101.md), [그림자](docs/exe/cpp-translated-shadow-20261101.md). 사용자의 자연 플레이 재확인은 별도다.
- [x] **설정 매핑 저장 실패 대응:** 제품 674의 오류1224·백업/교체/지연 정리, 도구 675의 복구를 완료했다. [제품](docs/exe/cpp-options-mapped-save-20261101.md), [도구](docs/exe/cpp-tool-settings-writer-20261101.md). 과거 개별 원인 프로세스·1176/1177·부분 I/O와 전체 audio 재실행은 미확인이다.
- [x] **11.01/Credits·외부 검증 비활성화:** 684·685 적용 완료. 1.3절의 문구·처리 경계를 따른다.

### 2.2 미션별 확보한 증거

모든 행은 **과거 해당 제품의 증거**다. 현재 813의 자연 완주 회귀로 합산하지 않는다. 전체 34 최신 기준은645/644 제품이다.

| 대상 | 확보한 증거 | 남은 사용자 확인 |
| --- | --- | --- |
| Early1 | 북서 간헐천 A~G·채취/귀환·SP0→600·Tutorial2 진입 | 남동 경로 실패 원인(417의 D/SP0에서 Next Tutorial 미발견), 실패/재시도/F8·화면/음향 |
| Early2 | A~I·신전/워크샾/지식/생산/발사기/회수·환불75·Tutorial3 진입. 마지막 자연 완주 회귀는467 제품 | 재시도/취소·화면/음향 |
| Early3~6 | 진입/Start·단계 판정·짧은 진행. Early3의468 Source1은 채취/귀환·SP2000→2400·A→C, Early5는 과거F까지 | 각 자연 완주/다음 미션. Early3 최종468 제품 완주 기록은 없고 attempt4 종료1은 보존 |
| 캠페인1-1 | 322의 자동 AI 적 신전 HP5000→0, 2026-10-28 사용자 완주·다음 안내 확인, 638 Restart/Leave | 현재 제품 완주·패배/Retry·보호막/포로색. 사용자 실행 PC/커밋/SHA 미제공 |
| 캠페인1-2 | 321의400프레임·645 진입. **2026-11-01 사용자 완주·적 사제 포획/희생·Next Missions로1-3 진입** | 현재 제품 회귀·패배/Retry. 제보 2건은677/678 수정 완료이며 사용자 재확인은 별도 |
| 캠페인1-3~1-6 | 진입/Start·321/645 진행, 575의1-6 무입력 신전 파괴 후 섬 중립/테마 복귀. 1-3 사용자 진입 | 자연 플레이·승패·후속 명령 |
| 캠페인2·3장 | 645에서 진입/Start·비정지 시각·창 출력·종료0 | 추가 TYPE 전체 수명·자연 플레이 |
| Priest Training 9개 | 645에서9/9 진입·비정지 진행·창 출력·종료0 | 자연 완주·완료 안내/복귀·bc8 전체 원소 선택지 |
| 데모 3개 | 645 실제 메뉴 진입. 567/569 첫 360초 만료→재시작→Exit Demo→Quit | 승패·자연 완주. 두 번째 360초 반복 검사는 3절 3번의 별도 잔여 |

### 2.3 음향·영상·캡처의 확인 범위

현재 위치 음향 조회/장치 순서(672), 곡 만료의 현재 battle/정리 순서(673), MovieGump/스킵·끝·CD 경로/캐시(594·595·654·655·670), 연속8비트BMP 수집(693~695)은 연결했다. [위치 음향](docs/exe/cpp-sound-current-view-20261101.md), [영상 owner](docs/exe/cpp-movie-override-20261101.md), [화면 수집](docs/exe/cpp-capture-client-20261102.md)을 따른다.

실제 PCM/청취·CD 삽입·활성 전체화면 물리 출력, 전체 영상 동일성, 수집 중 모드 변경/1000프레임 GUI/반복 주기·소리 녹화는 미검증이다. 672 전체audio 실패 원시는 `live/audio-v2`에 보존하며 기능별 후속 성공으로 지우지 않는다. 기존 BMP 수집은4차 MCP·소리 포함 녹화 완료가 아니다.

### 2.4 현재 이슈의 처리 위치

| 항목 | 현재 처리 |
| --- | --- |
| 물리 캡처/입력 | 환경 대기.762의 FrameArrived/window capture timeout 뒤 실제 환경 변화 전 반복 금지(3절7번) |
| Early1 남동 경로 | 자연 플레이에서의 미확정 실패. 사용자 확인/제보 후 후속 작업(3절6번) |
| 옵션 저장 | 1224 대응은 완료. 공유 삭제 거부 시 이전 파일/changed/복구 파일을 보존하고 실패 안내. 외부 매핑이 남으면 성공 백업/교체 폴더도 남을 수 있음. 부분 I/O·1176/1177 잔여(3절4번) |
| PlayerNoMake | 경고255DWORD/정리 `004328a0` 연결은453 완료. 수신 생성/검증 패킷 부모 `00433c30` writer는 후순위 MP(3절8번). CD `00487900`은 spot 정리이며 같은 함수가 아님 |
| Graph/글꼴/사용자 제보 | 대표 Graph와 캐시 수명은721~724, 비ASCII 글꼴 경계는774, 제보 2건은677/678 완료. 2.1절에 명시한 나머지 레이아웃/글꼴/플레이 한계만 남김 |

## 3. 앞으로 할 작업 (재개 후 권장 순서)

**현재는 사용자 `/goal` 요청으로 진행 중이다.** 재개 첫 작업 네 PostPop은809, CanonDecoder는810→811에서 완료했다. 비멀티1~4번·Windows 10 호환성7번을 마친 뒤8번으로 간다. 5·6번은 사용자 확인 목록이며 단계 전환을 막지 않는다. 아래 `[ ]`만 실제 잔여이며 완료한 하위 범위는 2절로 분리했다.

### 1. 대상별 커서·오브젝트 우클릭·게임 UI

- [ ] 크리스탈/비석 특수 명령·제단/일반 건물의 나머지 타입·소유자·레벨·손상·건설/생산·포획/희생 상태별 메뉴·명령 효과·반납·선택기 잔여를 대조한다. 실제 포획/희생·공격 재현은 건너뛴다.
- [ ] 운반체 전체 종류와 나머지 커서 조합을 점검한다(536·576). 기본 Shift 그룹 선택·이동과 N/P/R/손실 위치 선택은 완료이며 공중 3종 새 전용 검사는 하지 않는다.
- [ ] ESC 하위/View·도킹 조합·DD 조정의 남은 표시·Chat 모드별 입력/표시와 전체 Console/모달 수명을 대조한다. 메인 메뉴의 전체 예외/할당 실패·CD 대응도 남는다. F9는 정지 앞, F6는 정지 뒤 예약이다. 이미 연결된 단축키·일반명령 후처리/글꼴/음질/파일 reader를 반복 완료 과제로 만들지 않는다.
- 활성 caller 없는 관리명령·grid/debug는 기본 UI에 임의 노출하지 않는다. 퇴역명령 `AdvanceRank/SpawnScreenChange/Stall/BridgeLikeCrazy/1/CreateRandomGuns/DumpProcessList/DumpSquids/DestoryAllPost`는 대조한 무효과를 유지한다. special `0055a480`은 현재값만 읽고 개발자키 `0055a484`를 켜지 않는다.
- 객체메뉴의 Dig Up/전환/다리/Meltdown/Detonate, 독립 미니맵/Send Money와 c2(Test Battle/편집 복귀)·c5(Leave)는 연결 완료다. RequestMenu 서버 전송·메타맵 열기2개·c6(Refresh Zones)/db(Declare a Draw)는8번이다.

### 2. Edit 메뉴·커스텀 맵/시나리오

- [ ] 다른 시나리오 조합·실제 설정/UI writer와 TARC/동적/Config 확장·남은 명령 부모를 대조한다. 687/715의 명시 목록·이름/큐 연결은 완료이며 모든 동적 실행의 증거가 아니다.
- [ ] 남은 지형·오브젝트·소유자 도구, 작업본 저장/재파싱·이름/목록·미션 전환의 다른 실패 수명, LeaveEditMode/State1/비동기 State7 분기를 점검한다. 716의 연속 왕복·726의 워크샾 owner 저장·724의 대표 Graph는 반복 미구현으로 세지 않는다.
- [ ] 원문 언어 UI/폰트 재생성·다른 언어 파일 실패 수명을 대조한다. 번호/Xlat/요청/기존 Chat 알림은 710~712·787 완료다. 590의 명시 `.french`는 프랑스어 UI 완료가 아니고 한국어는 6차다.
- 원문 목록 첫 50개 상한은 5차 전까지 유지한다(634). 실제 Fort 433개는 종류2가 432·종류0이 1개다. TEST01/TEST02 원본은 읽기만 하고 저장은 작업본에 한다. 10.78 기본 설명의 `{Text}` 리터럴(494)을10.72와 혼동하지 않는다.
- `.english` 부재 Test Battle 종료는 원본 복원 단계에서 유지하며 기본파일 자동 생성/시나리오 UI·부재 종료 개선은8차에서 한다.

### 3. 미션 수명·진행 저장·데모 반복

- [ ] Retry·Next Tutorial·Next Mission·Abort의 나머지 연속 전환과 참조/타이머·다리/AI 재고를 확인한다. 자연 승패가 필요한 검증은 사용자에게 남긴다.
- [ ] 세 데모의 재시작 뒤 두 번째 전체 360초 만료·추가 주기/정리·나머지 안내를 확인한다. 짧은 진입의 관례적 반복은 하지 않는다. GuardMyBack의 15초 안내와 DissolvedAlliance의 Demo 섹션 부재를 유지하며 Script demo 키만으로 모드를 추정하지 않는다.
- [ ] 비멀티 Native3의 남은 선택·State1/단독State7·전투 정리 `004b2df0`의 Tick/연속 전환, LeaveBattle 나머지 이유/CD·State6을 대조한다. State3 특이 분기는 도달 미입증/미연결이다.
- 정상 State7의 복사/로그·World 교체·7/8 확정(660), State1 공통 앞단(661), 현재 Guide 요청과 실제 Early1 전환(713·714)은 완료다. 일반 전투 저장/복구·StateReconnect·서버 이전·Zone0/메타 Poll·시계 패킷은 8번이며 기존 질문/삭제 UI를 유지한다.

### 4. 내부 수명·파일·checking 실제 연결

**재개 첫 작업 — 네 Gun PostPop 완료(809):** 생성 뒤 진단 위치·현재 전역 재조회·진단 복귀를 복원했다. [809 기록](docs/exe/cpp-gun-postpop-checking-20261103.md). 810→811에서 CanonDecoder 본체·현재 제품 소비자도 연결 완료했다. [811 기록](docs/exe/cpp-canon-consumers-checking-20261103.md). 다음은 우선순위1의 객체 메뉴/커서 상태 효과와 수명이다.

- [ ] **checking 잔여 30함수**를 [함수별 공백](docs/exe/cpp-checking-consumer-gaps-20261103.md)에서 이어 간다. 부모 21/부분 6/MP 분류 3이며 779 당시 51개 중 2.1절의 21부모는 완료다. 부분 항목은 `0048fb30/004aec10/004b54e0/004b8cb0/004c9350/004d62b0`이다. `004b54e0` 재접속 저장은 부분 6의 집계에 남지만 작업은 8번MP로 이월한다. MP 분류 3(`0042bc10/004b4df0/004b8b20`)도 8번이다.
- [ ] 비멀티 파생 프로세스·State/Session의 남은 수명과 전체 팔레트 전환·Chat 모드별 표시·시스템뉴스 UI를 연결한다. 원격/메타맵·desc는8번이다.
- [ ] 경로 고리/FileSpec·BaseFile의 **분류만 했거나 아직 연결하지 않은 소비자**, 최초 FILE/리소스 I/O·다른 쓰기/키/스트림·TAFF/보조 폴백·손상/할당/부분 I/O 실패 수명을 대조한다. 이미 실제 연결한 reader/FontStore/SHP·Edit 쓰기는 2.1절을 따른다. [785의 직접 74caller 분류](docs/exe/cpp-secondary-sound-root-20261103.md)는 모든 부모 구현 완료가 아니다. 등록 snapshot/무진단 optional·OS 직접 ReadFileBytes 계약을 보존한다.
- [ ] options.cfg의 실제 부분 I/O·1176/1177 실패 경계, 원문 FILE/FileSpec/SEH와 호스트 소유 경계의 남은 차이를 판정한다. `0044fa00`의 재접속 저장 소비자는 `004b54e0`과 함께8번이다. CD 일반 열기는 `004ba5d0`, 부분 파일은 `004ba630`이며 자동 정밀 후보의 반대 대응을 따르지 않는다(652). 실제 UNC 서버/다른 드라이브 상대CWD 등 미관측 경계도 보존한다.
- 원본32비트 스택/미정값 자체 일치(612)·원문 범위 밖 메모리·전체SEH/진단UI는 호스트 한계와 별도로 판단한다. guard 제거·임의 배열 확대·손상 후보 무조건 무시·진단6 강제·임시 속도로 오류를 숨기지 않는다.
- 비미션 전체 랭크 상승·조기 희생 안내(missionPost0,615)·연결 활동/리슨/수락/송신 writer와 일반 전투 saveBattle/metamap·비mission phase14 저장은 8번 후순위다.
- 내부 근거는 [장기 소유](docs/exe/cpp-state-world-owner-plan-20261025.md)·[State 공백](docs/exe/cpp-state-cleanup-client-gap-20261025.md)·[전역 반납](docs/exe/cpp-state-cleanup-global-owners-20261025.md)을 따른다. 오래된 PLAY 목록·ClearWorld 주석·428 문서만으로 후속 단위의 연결을 미구현으로 되돌리지 않는다.

### 5. 원본 전체 동일성 — 사용자 확인 대상

AI 미완료 체크 항목이 아니며 단계 전환을 막지 않는다. 부품을 구현할 때 원문과 대조하는 일과 사용자 제보 수정은 AI가 계속 맡는다.

- TYPE 전체 대기/건설/발사/피격/파괴/포획/희생 프레임·방향·색표·그림자·기준점·가림/파편/지형 효과, 캠페인2·3장 추가 TYPE 전체 수명
- 메뉴/브리핑·생산/지식·선택/사거리/비용·커서/회수/업그레이드·키·Pause/Help·카메라/View/도킹/Chat/PlayerList·Credits의 전체 조합/픽셀
- 건설/발사/적중/출격/귀환/연소/붕괴/의식/승패/음악의 파일·시점·반복·위치, 정지 중 시간/난수·정상 간격 AI, 장기 자원 누적/정지/크래시
- 녹화의 HDR 밝기·방송 음성은 게임 효과로 판단하지 않는다. 표시 변환 픽셀 일치와 전체 외관 동일성을 구별한다.

### 6. 자연 완주·자동 공략/테스트 지원 — 사용자 확인 또는 별도 요청 대상

모든 Early Missions·Training·캠페인·데모 자연 완주와 실제 승리→신규 완료값0→1 저장→재실행/다음 미션 해제는 사용자가 확인한다. 578의 명시 진행 키 저장과 기존 완료값1 보존을 자연 승리/신규 해제로 합산하지 않는다. 캠페인1-1/1-2 사용자 완주는2.2절의 당시 확인 범위다.

자동 Early3 후속·TEST02 로컬 AI/포획/희생 전략·좌표 준비 등은 별도 요청 전 진행하지 않는다. 옛 시도/계획은 [보관본](LEFT_JOBS.old.md#archive-20261103-through-808)에 있다. `CampaignForwardBattle_NaturalTempleAssault`의549 실패는 미해결이며 원래 함수/전략/승리 단언을 유지한다. TEST02 로컬 owner AI는 원문 누락이 아닌 테스트 지원 제안이다. 원문 `004c2b20`의 `owner!=myPlayerId || demo!=0` 조건을 유지하고 `gate.demo=1`로 우회하지 않는다. 별도 지원을 요청받으면 현재 원본 SHA/배치를 다시 확인하고 기본 게임/프로필과 분리하며 공유 난수 차이를 기록한다.

### 7. Windows 10/11 호환성과 전체화면

- [ ] 캡처가 정상화된 환경에서 물리 입력 QWASZX·ESC·Home·포커스를 검증한다. 762의 새 부팅/RDP세션1·전경비NULL에서도 기존 VSCode fresh선택/재선택2회 timeout이므로 실제 환경 변화 전 반복하지 않는다. 관측 없이 입력을 보내거나 앱 내 입력을 물리 입력으로 세지 않는다.
- [ ] 다른 모니터/DPI·실제 다른 앱 포커스 전환·DD 조정의 다른 비트/실제 장치 실패 경계를 확인한다. 746의 단일 마우스 조정/취소/확인/15초 만료·불법 모드 복구·저장 재시작은 완료다. 메시지 주입은 실제 앱 전환이 아니다.
- [ ] 실제 PCM/청취·사용자 활성 전체화면의 물리 출력과 실제 CD 삽입 W 경로를 확인한다. 594의 waveOut0/전경NULL,762의 원격오디오 1개/전경비NULL은 실제 PCM 열기/출력 성공이 아니다.
- 전체화면 종료 후 재실행·세 화면비·가장자리 스크롤은 확인한 범위에서 완료다. 관련 변경 뒤 필요한 회귀로 유지하며 무기한 `[ ]` 작업으로 반복하지 않는다. 원본과 다른 호환 처리·이유는 한국어 주석에 남긴다.
- Windows 10의 남은 AI 작업을 마치면 Windows 11 실기 검증 없이 다음 단계로 넘어간다(1.1절). ARM 실기 미검증도 명시한다.

### 8. 멀티플레이·배포·후속 단계

- [ ] **3차 TCP/IP 로컬 멀티플레이·Outpust**를 원문 근거로 사전 구현한다. 신규 단위/게임/통신 검사는 하지 않는다. 공식 서버 복구·구 클라이언트 유효성 검사는 연결하지 않는다.
- MP 이월 목록: RequestMenu 전송 `004d712e..004d717c`, 메타맵 열기 `004789b0/00479390`·번개 `00479650`·Challenge 이미지/텍스트 `00478d10`/참가자 삭제 `0042bc10`, c6/db·ChalSailPos 모드6·선행클릭/항해 메뉴·desc, 비미션 의식/랭크·조기 희생 안내, PlayerNoMake writer `00433c30`, frame22/주기64·timeout/UI·endpoint/server/boss·리슨/수락/22칸 송신.
- 일반 전투 `!state.dat` 저장/복구·saveBattle/metamap/비mission phase14·StateReconnect/서버 이전·Zone0/메타 Poll/`004c1e70` 시계 패킷·`004b54e0` 파일명/count 진단과 `0044fa00` 소비자·LeaveBattle 원격 페널티를 포함한다(661의 분류). 기존 질문/삭제 UI는 유지한다. 정적 사전 구현과 검증 완료를 구별한다.
- [ ] **4차 로컬 MCP·해상도:** 모든 게임 조작·캡처·소리 포함 녹화, 16:9 최대 1920×1080을 제공한다. 그래픽 크기 스케일 조정은 이 단계의 요구가 아니다.
- [ ] **5차 사용자 저장 경로·무제한 맵 목록:** 모든 자동 생성 파일/디렉터리와 reader를 `%USERPROFILE%\Saved Games\Netstorm\` 아래로 옮긴다. 설정/진행은 Saves, 신규 맵은 CustomMaps. 원래/사용자 맵 병행 읽기·목록 스크롤/무제한을 구현한다. [733 조사](docs/exe/cpp-phase5-file-inventory-20261102.md)의 12파일군/50후보와 이후 추가 writer(HTML PieceList·옵션 임시/복구·!state/전체화면 표식·Edit 장부/잠금/pending·GIF/BMP·squids/procSumm·MCP 생성물)까지 포함한다. 이관 우선순위·두 설치 충돌/장부 identity·명시 export 정책을 정하고 일반 권한/읽기 전용 설치·다른 CWD/Unicode·저장/재적재/중단/교체/전체화면 재실행을 확인한다.
- [ ] **6차 한국어:** 메뉴/문자열/시나리오·글꼴을 지원한다. 글꼴은 D2Coding이며 명칭은 AGENTS.md를 따른다.
- [ ] **7차 AA/FSR1.0:** Options에서 각각 선택·기본 off. 지원 환경은 2560×1440/3840×2160을 추가하고 이 해상도 선택 시 FSR 자동 켜기·실질 1920×1080에서 업스케일한다.
- [ ] **8차 Edit 시나리오 UI:** 초기 자본/테크·적·동맹 등을 별도 편집 UI로 수정하고 새 맵의 기본 english 자동 생성·english 부재 Test Battle 종료를 개선한다. 기존 수동 대본·저장/재적재를 보존한다.
- 배포 스크립트 자체는 완료다. 배포할 제품이 정해지면 README/BUILD_INFO를 갱신하고 그 제품의 ZIP·압축 해제 실행을 확인한다. 현재 검증 ZIP은644 제품이며808 배포 확인으로 쓰지 않는다.
- 60/120프레임·CD10.72 전체 복원/Windows 10 구동 불가 해결책은 별도 후순위다. Linux는dotnetpj 후순위이며cpppj는Windows 대상이다.

## 4. 운영 규칙과 유효한 사용자 결정

- **현재 진행 지시:** 2026-11-03 사용자 `/goal`로 일시중지를 해제하고 모든 AI 작업을 순차 진행한다. 사제 포획/희생·특정 적 공격·승리 및 사용자 직접 확인 검사는 건너뛰며 MP는 원문 근거 구현 후 검사를 건너뛴다. 단위마다 이 문서 갱신과 로컬 커밋을 수행한다.
- **단계 전환(2026-10-31):** 모든 자연 완주·전체 동일성은 사용자 확인이며 AI 잔여가 이것뿐이면 다음 단계로 간다. Windows 10 확인 뒤 Windows 11 실기 없이 넘어간다. 사용자가 오류/차이를 제보하면 해당 건을 AI 작업으로 올린다.
- **실행 빈도(2026-10-31):** 더 많은 기능 묶음 뒤 필요한 통합 확인을 한다. 소수 단위마다 전체 34·긴 Early 완주·전체 CTest/71대상/ZIP을 반복하지 않는다. 옛 3~5단위 기준은 자동 실행 조건이 아니다. 새 실제 실행은 변경과 직접 관련된 필요성을 먼저 판단한다.
- **검증 기록:** 정적 대조·명시 입력·컴파일·C++ 검사·GUI/물리 관측을 구별한다. 최초 실패/교정 원시를 보존하고 기대값을 제품에 맞춰 약화하지 않는다. 다른 제품의 성공·준비본을 현재 제품 통과 수로 합산하지 않는다. GUI/분석기 실행 720의 범위는 [720 기록](docs/exe/cpp-editor-graph-native-20261102.md)을 따른다.
- **CHEAT/테스트 조작:** CHEAT은 Edit 편집에서만 각 원문 조건으로 노출한다. `--test-bridge-patterns`·명시 로컬 AI/SP 등은 검사 조작임을 주석/기록에 표시하고 기본 게임·적 AI·저장 프로필에 섞지 않는다.
- **실행 파일/자료:** Windows 실행 파일은 `Netstorm.exe`다. CMake 대상명 NetstormCpp·OUTPUT_NAME Netstorm·ARCHIVE_OUTPUT_NAME NetstormCpp를 유지한다. 옛 `NetstormCpp.exe` 로그는 당시 이름으로 보존하고 원본/클론 프로세스는 전체 경로로 구별한다. 무인자 GUI·`--help/--run`·분석 CLI를 유지하며 EXE 옆 `game/netstorm.tarc`를 우선한다. cwd의 다른 게임/보호 원본을 자동 탐색하지 않는다.
- **보호:** AGENTS.md·originals/originalCD 기존 파일은 수정/삭제하지 않는다(AGENTS의 명시 예외만 허용). 게임 실행 전후 원본 해시를 비교하고 허용 설정을 복원한다.
- **빌드:** HJOW-Athlon은 VS18(2026)/MSBuild18.x를 쓴다. `cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true`로 빌드하며, 선행 CMake/MSBuild 자식이 끝난 뒤 한 번에 하나만 실행한다. exit_code0 확인 후 의존 검사를 한다. HJOW-X3D는 VS17(2022)/v143이며 BuildTools cmake의 전체 경로를 쓴다. [빌드 안내](docs/cpp-build.md) 참조.
- **UTF-8:** PowerShell 5.1에서 Python stdin으로 한국어를 보낼 때 `$OutputEncoding = [System.Text.UTF8Encoding]::new($false)`도 설정한다. cp949 오류 시 `PYTHONUTF8=1/PYTHONIOENCODING=utf-8`을 설정한다. 원시 로그/Native fixture를 인코딩 정리로 덮어쓰지 않는다.
- **Git:** 이 문서/CPP 작업 파일은 기존 줄바꿈을 유지하고 확인/stage에 필요 시 `git -c core.autocrlf=false`·`-c core.whitespace=cr-at-eol`을 쓴다. 저장소/전역 설정은 바꾸지 않고 최초 관측 TSV의 `-text`를 보존한다. DN/WB는 해당 문서·`.gitattributes` 규칙을 따른다. 단위별 인계 후 `YYYYMMDD-NN PJ 요약`으로 로컬 커밋한다. 당일 마지막 번호를 확인하며 cpppj=CP·dotnetpj=DN·webpj=WB·공통=CM이다. push는 금지다.

## 5. 참고 문서와 이관 기록

- [C++ 빌드](docs/cpp-build.md), [로드맵](docs/cpp-roadmap.md), [플레이 가능 계획](docs/cpp-playable-plan.md), [소스 맵](cpppj/SOURCE_MAP.md), [모듈 맵](docs/exe/modules.md)
- [디컴파일 신뢰도](docs/exe/decompile-reliability.md), [2026-10-23 양판본 디컴파일](docs/exe/cpp-decompilation-20261023.md), [analyzeManager](docs/analyze-manager.md)
- [준비본 보관](docs/exe/prepared/README.md): 443/444와 469의 일부는 후속 단위에 통합됐다. 보관본의 미적용/미빌드 문구를 현재 상태로 쓰지 않으며 base/patch/manifest/142파일 원시는 보존한다.
- 과거 전체 원문: [504까지](LEFT_JOBS.old.md#archive-20261027-through-504), [549의 0절](LEFT_JOBS.old.md#archive-20261029-section0-through-549), [561의 3절](LEFT_JOBS.old.md#archive-20261029-section3-through-561), [638까지](LEFT_JOBS.old.md#archive-20261031-through-638), [695까지](LEFT_JOBS.old.md#archive-20261102-before-cleanup), **[808까지·이번 정리 직전](LEFT_JOBS.old.md#archive-20261103-through-808)**.
- **이번 정리(2026-11-03, HJOW-Athlon):** 정리 전 원문을 보관 파일 끝에 그대로 추가하고 기존 보관 바이트를 유지했다. 대체된 실행/재개 지시·단위별 중간 수치/실패 나열·해소된 다음 작업·별도 요청 전 자동 공략 준비는 보관본으로 이관했다. 완료 항목은 2절의 `[x]`로, 미완료/부분·MP 이월·환경 대기·사용자 확인은 3절의 해당 위치로 정리했다. 제품 808/11.01·EXE 해시·마지막 디컴파일 호스트·일시중지는 유지했다. 문서/링크/보관 원문·해시·차이만 확인했으며 빌드/게임/기능 검사/디컴파일은 하지 않았다.

## 인수인계 문서 분담

cpppj/공통/디컴파일은 이 문서, dotnetpj는 LEFT_JOBS.dotnetpj.md, webpj는 LEFT_JOBS.webpj.md에 기록한다. cpppj 1차·Windows 10/11 호환·MCP가 완료돼 dotnetpj 구현에 활용할 수 있게 되면 인수인계를 LEFT_JOBS.md로 다시 통합한다. 현재는 프로젝트별 문서를 유지한다.
