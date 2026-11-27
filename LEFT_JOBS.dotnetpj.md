# LEFT_JOBS.dotnetpj — 현행 상태와 남은 작업

**2026-11-27 재개 · 마지막 완료 단위1.106 · R5-4c3 전체 검증 미완료 · 1.080a/b Game 재빌드·관련 검사·대표3경로 L2 복구 완료 · 1.106 공통 Native 부모 통지 분기·세션 연결과 중첩 통지 미완료 · 1.089 실제 키 입력 확인 미완료 · 다음 R5-4c3c 한 픽셀·전환 수명 및 R5-4c4 → R5-4d → R5-3 → R5-5 → R6 · R5 2/5, R4 2/8 · 최신 정기 회귀1.105(7,401통과·음향 하드웨어 1미실행·M1 0/34 유지), 마지막 완료 관문1.076(7,379통과), 중단 회귀1.080(7,385통과·L2 세 종료1) · cpppj 제품1377/ZIP1378·검증1380 · 튜토리얼 안내 DN-TUT-1~4 필수 미완료**

**2026-11-26 작업 중지 기록(R5-4c3 검증 미완료, 중간 커밋1015ca4a6).** 정기 전체 회귀는 통과했다(Assets 383·Core 4,113·Client 2,889, 합계 7,385·실패0). 그러나 대표 3경로(`thewarbegins`·`tutorial1`·`surrounded`) L2 재수집에서 DN 3건 모두 종료1이 나왔다(`extracted/dotnet-l2-1080`, `run.json`의 `exit_code` 1). 원인은 미션 진입 뒤 생산 루트 Idle이 R6 미연결 본체(`UpdateContents`·`Maintenance`)를 요구한 결함이다(종료 로그에 `생산 … 실제 … R6에서 잇는다` FATAL). 대응으로 `EnsureMissionProductionDeck`에서 루트 유지 Idle을 끄도록 수정했으나(예산 Idle은 유지해 SP 표시 갱신) Game 재빌드·대표 3경로 재수집 검증 전에 중지했다. 커밋1015ca4a6에 보존한 변경: `ClientHostGame.Minimap.cs`(미니맵 HUD 부착 `EnsureMissionMinimap`)·`ClientHostGame.ProductionDeck.cs`(루트 Idle 차단)·`ClientHostGame.cs`(두 Ensure 연결)·`HostMissionMinimapDeckTests.cs`(신규 2건 통과, 관련 11건 통과·Game 오류0·경고0·`cpp_sync.py` 종료0 확인). 2026-11-27 재검증(1.080a/b): Game Release 오류0·기존 경고8, 집중12/12·동기화 종료0; 대표3경로 양쪽 종료0·DN 덤프 0998 바이트 일치·원본1,392파일 불변이다. 최초 실패 수집은 보존했다. 미니맵 픽셀·수명 검증은 R5-4c3c에 남는다. [근거](docs/dotnet-canon-1080ab-20261127.md)

현행 인계는 이 문서가 기준이다. 정리 직전 전문·완료 상세·과거 집계는 [1.079/중단 기록 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261127-through-1079-checkpoint1080)에 보존했다. 단위 근거는 `docs/dotnet-canon-0NNN-*.md`·`docs/dotnet-canon-1NNN-*.md`, 이전 보관본은 [6절](#dotnet-archive-log)을 따른다. cpppj·공통은 [LEFT_JOBS.md](LEFT_JOBS.md), 웹은 [LEFT_JOBS.webpj.md](LEFT_JOBS.webpj.md)가 기준이다. 이번1381은 문서 정리이며 제품·빌드·실행을 진행한 단위가 아니다.

<a id="dotnet-overview"></a>

## 0. 현재 상태와 다음 작업

| 경로 | 실행·플레이 | 원본과의 동일성 |
| --- | --- | --- |
| 유일한 실행 경로 `Netstorm.Game` (`Netstorm.exe`) + 내부 `Netstorm.Client` | 0.807에서 구 근사 실행기를 제거하고 직역 호스트가 이름을 승계했다. 타이틀·메뉴·도움말·Options·Credits·캠페인 선택·Edit 목록과 이름 창이 동작한다. 개발용 `--mission <키> --handle <이름>`으로 일반31미션의 시작 상태 일부를 조립한다. 미션 화면은 카메라 프레임·타일 수집/출력·HUD 구분선까지 제품 루프에 이었으나(1.062~1.076) 객체·HUD 패널·입력·규칙의 실제 경로가 남아 플레이 불가다. DN-MP-01 도움말 setter는 1.077에서, 미션 중 메뉴 배경 숨김은 1.078에서, 생산 덱 틀 부착과 우클릭 메뉴 표시는 1.079에서 교정했다. | 1024×768 메뉴 **12장면**의 논리 BMP가 cpppj와 바이트 일치했다(0.381~0.388). L2는 **0/34**(데모 3경로 DN 진입은 0.998에서 연결해 34경로 모두 대조)이다. 미션 진입 L4a는 `thewarbegins` 100프레임 89.0096% 불일치(1.076)이며 L3·L4b·L5는 미착수다. |

- **검사 기준선.** 최신 정기 전체 1.105는 Assets383·Core4119·Client2899 합계7,401통과·실패0이며 음향 하드웨어 1건은 TRX상 미실행이다. 이전 정기 전체1.076은 Assets383·Core4113·Client2883 합계7,379통과/실패0이며 그 뒤 1.077~1.079는 집중 9건(5+2+2)을 통과했다. 전체 회귀의 순서 의존 실패 이력은 1.032의 `HostSelectionLifetimeTests` 1건(재실행·단독 실행 통과)이다. 0.880 이후 기존 이름·중복을 보존한다. NETSTORM_DATA 없이 Release 오류0·신규경고0이며 검사 사본은 실행본과 같고 원본1392파일은 전후 불변이다. 그 이전 단위별 집중 수는 각 단위 문서와 보관본에 있다. [1.076](docs/dotnet-canon-1076-20261126.md)·[1.077](docs/dotnet-canon-1077-20261126.md)·[1.078](docs/dotnet-canon-1078-20261126.md)·[1.079](docs/dotnet-canon-1079-20261126.md).1.080 중단 회귀의7,385통과·L2 세 종료1은 머리 중지 기록과 구분하며 성공한 제품 관문으로 올리지 않는다.
- **L2 기준선.** 마지막 수집은 extracted/dotnet-l2-0998이다. 전체34경로의68실행 모두 종료0·오류/누락0이며 데모3경로의 DN 덤프가 생겨 34경로 모두 대조에 들어간다. M1 **0/34**(31미션 동등 0·cpp_only997·dn_only21), 고정 수신26/26이다. CPP 차이는 기존 미초기화SP행3열 범위뿐이다. 세 설치 값(`_nativeLoaded`·`_editorUiInstalling`·`_editorPrefixUiInstalling`)은 false다. 0.999 이후 단위는 제품 변경이 덤프에 닿지 않음을 대표 경로 재수집으로 확인해 왔다(1.001·1.008·1.020·1.024, 1.031 대표 1경로, 1.032 이후 정기 회귀마다 대표 3경로). 마지막 대표 재수집 1.104(`extracted/dotnet-l2-1104-content-dispatch-final`)까지 DN 덤프가 0998과 바이트 일치해 기준선을 유지한다(M1 0/34·cpp 1행 비결정 SP행 제외). [0.998](docs/dotnet-canon-0998-20261125.md) · [격차 분석](docs/dotnet-l2-0606-20261117.md)

<a id="now"></a><a id="09-바로-다음-작업"></a><a id="r4-board"></a>

### 0.1 재개 순서와 최근 완료 요약

**다음 재개 선행 점검(2026-11-26 사용자 요청).** cpppj TCP/IP 상대 봇 검사에서 드러난 공통 결함/조립 누락을 DN에도 반영한다. DN-MP-01 도움말 setter·기존 검사 기대값은 1.077에서 교정했고 기존 R5 순서를 계속한다. 나머지는 해당 UI/로그인·수송을 연결하는 R6 SL-9/R11의 필수 선행 항목이다. DN-MP-08 기존 ID 복구도 필수 후속이다. 새 ID 전투 중 참가는1380 사용자 결정에 따라 추가하지 않는다. CP 재현을 DN 재현으로 세지 않는다. [건별 근거·대응 위치·완료 기준](docs/dotnet-cpp-lan-followup-20261126.md).

- [x] **DN-MP-01 공통 도움말 setter.** `GumpButtonTree.Ownership.cs`의 LOOK1000/HELP200·빈값/첫NUL·소유 문자열 해제를 CP1363과 맞췄고 `HostGumpOwnershipTests`의 빈 문자열 소유 기대도 교정했다(1.077, 집중 5건 통과·Release 오류0·신규경고0·`cpp_sync.py` 종료0). 생산 HUD 연결 뒤 숫자 칸/생산창 우클릭 실제 확인은 R5-4c2에서 수행했다(1.079, 예산·생산창 HELP 우클릭 메뉴 표시·주제·제목·부착·소리 확인).
- [ ] **DN-MP-02 일반 MP 옵션 수명/스타일.** CP1364의 루트 Push/Attach·자식 Move·스타일 파서 교정을 참고해 이름/서버 반복 갱신·형제 순환·닫기를 검사한다. DN 일반 옵션 창의 실제 연결은 별도 확인한다.
- [ ] **DN-MP-03 일반 로그인 안내 두 경계.** Client 장기 State7/즉시8의 Progress와 공통72h CreatingIslands를 실제 UI에 연결한다. 미션 전용 설정만으로 일반 LAN 완료를 판정하지 않는다(CP1365 안내 연결 완료, 전체 로그인은1367 확인).
- [ ] **DN-MP-04 Source/수신·공유 Fort 모드·17/18.** CP1366/1367의 비인계Fort 두끝·누락6수신/채널 해제·객체Chaff/장기후처리3모드를 대조한다. DN RawLocalMissionState.cs:127~131에도17/18이 없다. CP1369의 레벨1d 결과 로그·즉시14/출처0+next3·전체144BYTE 보존/예외를 복원한다. CP는 집중14/14·실제 호스트 종료 뒤 상대15→16→17→14/서버 권한1/1·수락 로그·상대 정상 종료를 확인했다. DN도 실제 인수/재접속을 검증하며 임의 성공/전역 표 삭제로 우회하지 않는다.
- [ ] **DN-MP-05 실제 두 참가자 TCP 검증.** R11에서 격리 사용자/새 Fort·실시간 상대 봇으로 명단→시작ACK→로그인14/PlayerKnown→월드·단순 생산/건설→이탈/종료를 확인한다. CP 봇은 DN 지원으로 세지 않으며 사용자 직접 확인 범위는 유지한다.
- [ ] **DN-MP-06 네트워크 시계/프레임 경과.** DN ArcherBindings.cs:135의 Delta<0와 RawSurfaceWorld.cs:786의 Game<이전Game 거부를 원문00460e90/00467c80에 맞춰 교정한다. CP1368은 두 조건 교정·집중21/21·실제60초 이상 두 참가자 반복2회/정상 종료를 확인했다. 유한성 거부·상태 보존은 유지하고 공유 현재 시각·보정 증가/감소·정지/재개·인수 경계를 검증한다. DN은 정적 확인만 했으며 실제 수정/검증은 다음 작업이다.
- [ ] **DN-MP-07 배치 비용 차감 뒤 복구 저장.** CP1370 LAN 신전 배치에서 누락된 역사적 battleRefresh의 실제004b54e0은 UI 갱신이 아닌 복구 상태 저장이다. R6 Drop/건설·R11 일반 LAN에서 비용 차감/표시 뒤 같은 SP/Player·공통 작성자를 연결하고 사용자 저장 파일을 검증한다. DN은 CursorDropState 데이터만 정적으로 확인했으며 실제 본체 결함 확인/교정으로 세지 않는다. [건별 근거](docs/dotnet-cpp-lan-followup-20261126.md). CP1370은 집중12/12·실제 신전 객체/양쪽 SP1500·저장owner1/SP1500·두 정상 종료를 확인했다. DN의 기존 HostSession.SavedState/RawSavedState와 실제 장치를 공유한다.

- [ ] **DN-MP-08 기존 ID 재시작·저장 복구.** CP1375의 epoch/DWORD 시계·State13 로컬 SP·Residence 실제 SavedGame 교정을 R11의 같은 Client/Player/Surface에 대응시킨다. 완료 기준: 같은 기존 ID의 실제 재시작·복구와 양쪽 월드/SP/정상 종료를 확인한다. 새 ID 중도 참가를 추가하지 않으며 CP 재현을 DN 결함/완료로 세지 않는다. [계약/CP1380 결정](docs/dotnet-cpp-lan-followup-20261126.md).

1. **다음 — R5-4c3c 픽셀·수명 검증 → R5-4c4 → R5-4d → R5-3 → R5-5 → R6.** R5-4c3 전체 미니맵 픽셀·수명 검증은 아직 완료되지 않았다. 재빌드·관련 검사·대표3경로 재수집은 1.080a/b에서 복구했다. 1.081에서 채팅 최초 위치를 구분선 기준으로 교정해 미니맵 가림을 해소했다. 1.082에서 WorldBits·CellPixels와 프레임 갱신을 이었다. 1.083에서 호스트 정상 종료 반납을 이었다. 1.084에서 공통 저장소 Pop/Unpop 알림을 연결하고 대표3경로를 확인했다(정기 회귀 7,386통과). 1.086에서 세션 지형·Player Pop/Unpop을 제품에 연결하고 단위 알림과 대표 미션 진입을 확인했다. 1.087에서 지형 그래프 재결선의 실행기 교체를 검증했다. 1.088에서 최초 채팅 숨김 뒤 미니맵 차이가 1픽셀로 줄었다(정기 회귀 7,388통과). 1.089에서 공통 루트의 채팅 재열기 사건을 이었고 단위 검사는 통과했다. 실제 창 자동 키 입력은 게임의 키 이벤트에 도달하지 않아 미확인이다. 1.090에서 장기 GumpForm의 현재 World/UI 통지 대여를, 1.091에서 제품 UI 등록표와 생산 패널 종료 정리를 연결했다. 1.093에서 GumpForm의 현재 TYPE/공장 본체를 복원했고 1.094에서 실제 Storage TYPE 주소와 가상 삭제 호출 경계를 연결했다. 1.095에서 세션 자원 삭제를 실제 가상 분배에, 1.096에서 공유 Surface의 GumpForm을 같은 삭제 깊이의 세션 분배에 이었다. 1.097에서 제품 PATCH 삭제 가상표 선택과 TYPE 별칭 Form 삭제를, 1.098에서 종속 UI 갱신 명령을 실제 입력 큐에 이었다. 1.099 정기 전체 회귀를 마쳤고 1.100에서 내용물 수명이 빌릴 현재 커서·Player 상자를 마련했다. 1.101에서 종속 해제기 생성 전 Gump TYPE 설치 순서도 보호했고 1.102에서 내용물 원격 수입 송신 본체를 복원하고 1.103에서 장기 ContentDevices를 제품에 연결했다. 1.104에서 세션 ContentLifetime 분배를 연결했고 1.105 정기 전체 회귀는 7,401건 통과했다. 1.106에서 공통 분리의 Native 부모 통지 분기를 복원했다. 다음은 현재 세션 대여 연결과 부모 삭제 중 UI 중첩 통지를 검증한 뒤 생산 Form 생성·부착을 연결한다. 다른 재생성 경로·미션 전환 수명과 실제 입력을 검증한다. R5 남은 세부는 [1.4](#r5-detail)에 있다. 튜토리얼 안내는 [0.5 DN-TUT-1~4](#tutorial-objective-port)를 R5 표시/R6 진행 소비자에 맞춰 구현한다.
   - 선행 완료: AI 프레임·신전 삭제1.031~1.033, WP·DI·SL1.034~1.061, 사용자 데이터1.041~1.044, R5 수집/타일/프레임·HUD 일부1.062~1.079를 다시 만들지 않는다. 상세 완료 식별자와 옛 소견은 [정리 전 보관본0.1](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261127-through-1079-checkpoint1080)에 있다.
   - 남은 Root Source 공개/제출·수송은 R6 세션 수명/R11 backend, AI Drops·Bridges와 호출자 내부 live 틱은 사용자 결정대로 R6이다. 기존83VT·현재 Runtime pin·공유 지도/색표·실제 분배를 유지하며 미설치 후크를 no-op이나 임시 표로 통과시키지 않는다. PORT_MAP 행 누락 보충은 [4절](#dotnet-inventory)을 따른다.

2. **R4 마감.** [1.1절](#r4-detail)의 R4-6 나머지(RU-3a·RU-3b·전송·수신 부모와 Root Source·WP-5·SL-9)와 R4-2~R4-5의 일치 대조를 닫고 R4-8(M1)로 넘어간다(R4-7은 1.003, R4-6 WP-1~WP-4는 1.040, DI는 1.050, SL-1~SL-8은 1.061에 마감). R5와 병행하며 이어서 R6~R11과 후순위 요구사항(3절)을 진행한다. R4-6 전송·수신의 R11·R6 의존 갈래는 해당 단계에서 잇는다.
3. **사용자 검사 범위(2026-11-22).** 캠페인1-1~1-5·스커미시에 한해 아군 AI 활성화로 포획/희생·특정 적 공격·시나리오 승리를 재현할 수 있다. 그 밖의 스테이지 해당 검사는 사용자 수동 확인으로 남긴다. 단순 진입/대기/생산/건설 검사는 진행한다. DN 자동 플레이의 실제 연결은 아직 없다. 원본1392파일은 마지막 정기 전체 회귀(1.076) 전후에 불변이다.
4. **사용자 추가 요구(2026-11-23, 기록만).** 세 프로젝트 공통 규칙은 cpppj가 1328~1330에서 정했다([공통](LEFT_JOBS.md#user-requests-20261123)). DN은 해당 소비자를 연결할 때 그 규칙을 따른다(2026-11-26 재확인: 셋 다 DN 코드에 아직 없다).
   - [ ] 방향키 카메라 이동 — 게임 중 ↑↓←→로 카메라를 옮긴다. 원본 ←/→ 조각 회전(`RawCampaignCursor.cpp:179~180`)과 TextEntry 커서 이동을 먼저 처리한다. 완료 기준: R5 카메라·R6 입력 연결 때 cpppj 규칙대로 구현하고 네 방향 이동·회전·입력 보존을 확인한다. 규칙: [cpppj1328](docs/exe/cpp-arrow-camera-1328-20261123.md)(`UserInput.cpp`·`UberGump.cpp`, 사건당 32픽셀, 입력 UI·조각 회전 우선).
   - [ ] ESC 게임 메뉴 한국어 — webpj에서 번역 누락 행과 "플레이어"(Players) 메뉴 글자 깨짐이 재현됐다. 완료 기준: DN 게임 메뉴 연결 때 Game·View·Options·Players·About 하위의 한국어 라벨과 Player 이름 표시를 화면으로 확인한다. 규칙: [cpppj1329](docs/exe/cpp-esc-menu-korean-1329-20261123.md)(공통 `assets/lang/korean/xlat.korean` 23키 보완·`RawGameMenus.cpp`). cpppj에서는 Players 글자 깨짐이 재현되지 않았다. DN 배포(`assets/game-data`)에는 아직 이 공통 번역 자산이 없다.
   - [ ] 정보(About) ▸ 버전 창 Copyright — 원본 Tell `[About]`에 11.01 패치와 `Copyright 2026 HJOW`(AGENTS.md)가 없다. 완료 기준: cpppj에서 정한 문구로 영어·한국어 창에 원본 저작권 줄·10.78 패치 줄·11.01 패치(HJOW)·`Copyright 2026 HJOW`를 표시한다. 문구: [cpppj1330](docs/exe/cpp-about-copyright-1330-20261123.md)(`ProductPresentation.cpp`의 About 확장, 원본 Tell 파일 불변). DN `ProductPresentation.cs`는 Credits 3항목·HJOW 카피라이트(0.342)만 반영했다.
   - 14차 모딩(QuickJS + webpj 이름 체계 공통 모드 API, 2026-11-23 사용자 결정)은 [3.1](#dotnet-modding)의 MOD-1~5다. 착수는 사용자 지시를 따른다.

**완료 묶음 요약.** 0.639~1.079 주제별 세부·집중 검사 수는 [정리 전 보관본0.1](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261127-through-1079-checkpoint1080)과 각 단위 문서에 있다. 남은 일의 기준은1.1·1.2·1.4다.

| 묶음 | 완료 범위 | 남은 연결 |
| --- | --- | --- |
| AI·월드·네트워크 본체(0.639~1.033) | AI 하위 동작·정기·건설/전투/의식·로그인/수신·Root/HUD/생산·파일 본체, 세션 호출자·AI 프레임·신전 삭제 | R4 나머지, R6 live 입력/규칙·R11 수송 |
| 월드/입력/수명(1.034~1.061) | WP-1~4·DI-1/3/4·SL-1~8, 5차 사용자 데이터 | WP-5·SL-9와 단계별 소비자 |
| 월드 표시(1.062~1.076) | 카메라·수집/타일·Submit·프레임, HUD 도크와 입장 카메라 | R5-3·R5-4·R5-5 |
| 공통 도움말·메뉴·생산 틀(1.077~1.079) | DN-MP-01·메뉴 배경/입력 숨김·생산 덱 틀/우클릭 확인 | 격자 내용물·유지/입력은 R6/R11 |
| R5-4c3 진행 | 미니맵 부착·Idle 차단 및 재빌드/관련 검사·대표 L2 복구(1.080a/b) | 실제 픽셀·수명 검증 미완료, 완료 수에 미포함 |

<a id="first-run"></a>

### 0.2 최초 실행 상태 — DN 적용 대기

원본 관찰과 cpppj 1314 구현이 완료됐다. DN은 빈 사용자 루트·무옵션 데이터 사본에서 아래 네 항목을 메뉴/설정 결선과 함께 확인한다. 2026-11-26 재확인: DN은 1.054(SL-5)부터 `NativeMainMenuState`가 `WelcomeGiven`을 읽어 Welcome/Tips로 나누지만, Welcome2가 열릴 때 `welcomeGiven=1`이 사용자 루트에 저장되는지와 첫 실행·재실행 흐름은 아직 확인하지 않았다. `HostStartupScene.SaveOptions`는 1.041부터 사용자 루트 `Saves/options.cfg`에 쓴다(아래 둘째 항목). [원본 관찰](docs/exe/original-first-run-20261116.md) · [cpppj 1314](docs/exe/cpp-first-run-1314-20261116.md)

- [ ] Welcome(OK) → Welcome2(Play Early Missions / Main Menu) → 메뉴 버튼. 완료 기준: Welcome2가 열릴 때 `welcomeGiven=1`이 사용자 루트에 저장되고 재실행하면 Tips 분기로 간다.
- [x] `HostStartupScene.SaveOptions`의 설치 데이터 쓰기를 사용자 루트로 옮긴다. 완료 기준: 설치 경로가 읽기 전용이어도 옵션·진행 저장이 사용자 루트에만 생긴다. — 1.041에서 `Saves/options.cfg` 읽기·쓰기로 옮겼고(`Done*` 진행 키도 같은 파일, 사용자 루트를 정할 수 없을 때만 종전 설치 경로) 1.044 읽기 전용 설치 시뮬레이션으로 설치 파일 불변을 확인했다. [1.041](docs/dotnet-canon-1041-20261126.md) · [1.044](docs/dotnet-canon-1044-20261126.md)
- [ ] Struggle For Freedom 최초 상태는 1번만 선택 가능하고 1~6번 모두 체크가 없다. 자기 `Done*`는 체크, 직전 `Done*`는 해금에 쓰고 저장·재실행을 확인한다. 실제 미션 결과 연결은 R6/R7에서 검증한다.
- [ ] 최초 Options는 소리·음악·바람·가장자리 스크롤·Auto-Demo·팁 ON, 스피커 교환 OFF, 효과음·음악 각각 5, 800×600·전체화면 요청이다. 기존 사용자 값과 진행을 보존하고 전체화면 재실행을 확인한다. 메뉴 일치 시험의 1024×768 창 모드는 시험 조건이다.

<a id="korean-font-change"></a>

### 0.3 한국어 글꼴 — DN 적용 대기

AGENTS.md의 현행 지정은 갈무리11이다.0.807에서 새 Game 빌드/게시/ZIP에 일반·굵은 TTF와 라이선스를 외부 fonts로 배치했고 D2Coding 배포 의존을 제거했다. 한국어 화면 연결은 R8 6차다. 2026-11-26 재확인: `KoreanFontFamily` 상수와 Game 화면의 갈무리11 글리프 출력은 DN 코드에 아직 없다(배포 복사만 있음). [공통 기준](LEFT_JOBS.md#korean-font-change) · [cpppj 1322](docs/exe/cpp-korean-galmuri-1322-20261117.md)

- [ ] `KoreanFontFamily` 공통 상수와 글꼴 파일·라이선스 설정을 마련해 로딩·측정·그리기·한글 대체·개발 표시가 같이 사용하도록 한다.
- [ ] 유일한 Game 경로의8비트 출력에 갈무리11의 비트맵 글리프를 적용한다. 구 실행기는 제거했으며 TTC 판독기 검사는 실제 TTC로 유지한다.
- [x] 0.807에서 Game.csproj/배포 스크립트의 복사·필수 파일·고지·안내를 맞추고 두 TTF/라이선스의 ZIP 전체 바이트를 검증했다.
- [ ] 작은 메뉴·도움말·브리핑과 영한 혼합 제목에서 일반/굵은 획·폭·기준선·줄 높이·잘림을 확인한다. 영어 `.chfnt`와 일본어 Noto Sans JP를 함께 대조한다.
- [ ] **WB E1288 점검에서 얻은 적용 조건(2026-11-23 기록).** 사용자 지시로 문서에만 반영하고 지금 작업하지 않는다. 위 둘째·넷째 항목을 구현할 때 지킨다. 근거: [WB E1288](docs/webpj-korean-glyph-grid-20261123.md)
  - 글리프: cpppj처럼 비트맵 TTF strike를 픽셀 그대로 정수배 복제하는 것을 우선한다(`EmbeddedBitmapFont`, 한글 칸 전진 폭 12·기준선 위 12줄). 외곽선을 래스터화한다면 설계 격자 1em = 12px로 그린다(11px이면 가로획이 빠져 `가`→`기`). 안티앨리어싱 결과를 문턱으로 자르면 ㅔ의 1px 간격이 메워지므로 4배로 그려 칸 평균을 쓰거나 비트맵을 쓴다.
  - 원본 글리프 혼합: Windows-1252로 나타낼 수 있는 글자(숫자·영문·공백·문장 부호)는 원본 `.chfnt`로, 확장 글자만 갈무리11로 그린다(cpppj `EncodeBytes`·`GumpTextDraw`). 모두 갈무리로 그리면 사이드바 SP 숫자(슬롯 3)가 커지고 공백 폭(5px 대 원본 3px) 차이로 Tutorial3 D1의 제네레이터 그림 세 개가 두 줄로 갈린다(webpj에서 실제로 난 결함).
  - 기준선·큰 슬롯: 확장 글자 기준선을 `max(원본 기준선, 배율×11)`(cpppj는 칸 기준 12) 이상으로 두어 슬롯 1·6(원본 기준선 9)에서 한글 윗줄이 잘리지 않게 한다. 큰 슬롯 배율·굵기는 cpppj(제목 1배·굵게)와 webpj(2배·보통체)가 다르며 cpppj D8 결정을 따른다.
  - 확인 기준: SP 표시가 영어 화면과 같다. Tutorial3 D1 그림 세 개가 한 줄에 놓인다. `가`의 잉크가 11줄이고 `에`의 ㅔ가 떨어져 있다. 슬롯 1·6 한글 윗줄이 잘리지 않는다.
  - 표시 확대: 현행 `ClientHostGame.Draw`는 `HostPresentation.IntegerFitRect`·`PointClamp`의 정수 배율이라 cpppj 전체화면 같은 획 불균일은 없다. 1080p에서 1024×768이 1배로 작게 보이는 문제의 고해상도 글자 층 제안은 [공통 0.7](LEFT_JOBS.md#korean-font-change)·cpppj D8 결정을 따른다.

<a id="exe-icon"></a>

### 0.4 실행 파일 아이콘 — EXE/ZIP 적용 완료, 창 표시 확인 남음

AGENTS.md 지정에 따라0.807에서 공통 [assets/Netstorm.ico](assets/Netstorm.ico)(1,078바이트, SHA-256 `bd4c18c5…f01e4f068`)를 Game.csproj ApplicationIcon으로 지정했다. 원본32×32/16×16의 두 원시 이미지가 개발/게시/ZIP EXE와 전수 일치한다. 빌드는 원본 EXE를 읽지 않는다. [0.807](docs/dotnet-canon-0807-20261122.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

- [x] 0.807에서 공통 ICO를 재사용하고 Game.csproj ApplicationIcon과 실제 EXE의 두 원시 이미지/그룹을 전수 대조했다. 해시는0.807 보고서에 기록했다.
- [x] 0.807에서 개발/게시 EXE와 최종 ZIP의 EXE가 전체 바이트 같고 동일 아이콘/11.01 버전임을 검증했다.
- [ ] DesktopGL 창·작업 표시줄의 추가 아이콘 설정 필요 여부를 확인한다. 탐색기 사람 확인은 사용자에게 남긴다. 2026-11-26 재확인: Game은 csproj `ApplicationIcon`만 지정하고 창 아이콘을 코드에서 따로 설정하지 않는다.

<a id="dotnet-stage-board"></a><a id="stage-criteria"></a>

<a id="tutorial-objective-port"></a>

### 0.5 사용자 추가 요구 — 튜토리얼 다음 단계 조건 안내

**2026-11-27 필수 반영, 아직 미구현.** 공식 초기6미션의40단계에서 다음 알림창/미션 종료창의 실제 조건을 지도 우측 하단에 배경 없이 계속 표시한다. 현재 gate는 읽기만 하고 예약/타이머 대기를 구분하며 원래 진행 판정을 바꾸지 않는다. 공통39키·입력 투과·가림/복원·수명·검증 범위는 [세 프로젝트 적용 계약](docs/tutorial-objectives-porting-20261127.md)을 따른다. CP1372 완료를 DN 완료로 세지 않는다.

- [ ] **DN-TUT-1 조건·문구.** 실제 세션40단계/39키와 영어·한국어 자산을 대응시킨다. 완료 기준: 누락·중복0, 일본어39키의 R8 13차 이월 명시.
- [ ] **DN-TUT-2 우측 하단 표시.** R5 HUD/지도 영역과 외부 갈무리11에 입력 투과 글자를 연결한다. 완료 기준: 배경/상자 없음·줄바꿈/여백·HUD 비겹침·입력 보존.
- [ ] **DN-TUT-3 진행·수명.** R6 실제 미션 진행에 연결한다. 완료 기준: 단계 변경·예약/타이머 대기·알림/도움말/메뉴 가림과 복원·종료/재시작/전환/이탈·크기 변경에서 이전 문구가 남지 않음.
- [ ] **DN-TUT-4 검증·배포.** 완료 기준: 조건 대응 집중 검사·허용 범위 실제 F4/F8/수명·영어/한국어·창/전체화면·4:3/16:9/16:10 확인, 배포 자산/인계 갱신. 튜토리얼 후반 포획/희생·특정 적 공격/승리는 사용자 확인으로 남김.

## 1. 단계별 작업과 완료 기준

| 단계 | 현행 상태와 완료 기준 |
| --- | --- |
| R0 기준선·동기화 | 완료. cpppj 기준선·PORT_MAP·`cpp_sync.py`. |
| R1 화면 호스트·입력 큐 | 완료. 8비트 호스트·입력 큐·비교 도구. 실제 월드 입력 소비는 R6. |
| R2 그리기·팔레트·글꼴 | 원시 본체 완료. 실자산 어댑터·수집/정렬·종류별 분배·`SpriteDraw`/그림자 공유와 `owner-capture` C행은 R4/R5에서 연결. |
| R3 Gump UI·메뉴 → D1 | 메뉴 L4a 12장면 일치. 메뉴 상위 소유자·모달·등록·대화창·텍스트 입력과 하위 화면 연결 잔여. 미룬 장면은 Language 자식 CJK(R8), Multiplayer 섬(R5/R11), Edit 이동행(R7). [UI 잔여](#r3-r2-detail) |
| **R4 월드 세션 조립 → M1** | **R4-2·R4-3·R4-4·R4-5·R4-6·R4-8 미마감(2/8)**(0.689~0.997·1.034~1.061은 주로 R4-6, 1.004~1.033은 R4-5, 1.062부터는 R5와 병행). 34경로(Early 6·Training 9·캠페인 16·데모 3)의 같은 시드 Load/Start `session-dump`가 cpppj와 일치해야 한다(L2). [R4 체크](#r4-detail) |
| R5 월드 화면·카메라·HUD → D2 | 렌더러 수집·정렬·dirty/팔레트, 타입별 Draw와 효과, 카메라·전체화면 가장자리 스크롤, HUD·덱·미니맵·인벤토리. 진입 직후와 N프레임 뒤34경로 L4a 차이0 및 압축 해제 실행을 대조한다. 1.062부터 진행 중(R5-1·R5-2 완료, 2/5, [R5 체크](#r5-detail)). |
| R6 플레이·규칙·AI·음향 → M2/M3 | Tutorial1 → Early Missions → 1-1·1-2 순서로 입력·커서·선택·메뉴, 사제/경로/운반/수확, 건설/생산/경제/다리, 전투, 의식/승패, AI, 소리/음악/동영상, 채팅/경고를 연결한다. M2는 고정 입력열 L3와 대표 캡처 일치, M3는 1-1·1-2가 기존 경로 수준 이상으로 진행되는 것. [선행 완료와 잔여](#r6-preports) |
| R7 미션 전체·Edit·설정·배포 → M4 | 34경로 진입·진행·저장, Retry/Next/Abort, 데모 2주기, Edit 왕복 저장·Save as·Test Battle/복귀, `options.cfg` 시점·전체화면 재실행, A~F 중 C·D·F 판정. ZIP 전수 대조와 압축 해제 실행, `Netstorm.exe` 외 실행 파일 사용법 문서화. 배포본 `Netstorm.exe`의 아이콘이 원본 실행 파일 아이콘과 같은지 확인한다(AGENTS 2026-11-19 추가, [0.4](#exe-icon)). |
| R8 AGENTS 4~15차 | 아래 차수별 요구에 도달할 때마다 기능을 검증한다. 공통 파일을 옮기는 과정에서 이미 들어온 부분도 실제 경로에서 확인한다. [요구사항](#dotnet-r8-requirements) |
| R9 60/120 FPS | Frame Rate 60/75(원본)/120과 양보 대기의 프레임 시간 대조. 비행체 표시 보간은 cpppj D2 결정에 따른다. |
| R10 Linux | DesktopGL와 파일 장치의 경로·권한·원자 저장을 확인한다. `$HOME/.netstorm/` 아래 `Saves/`·`CustomMaps/`·`logs/`를 사용한다. |
| R11 로컬 멀티플레이·Outpost | TCP/IP 로컬만 지원한다. `Netstorm.Net` 일부 사전 구현. 루트 서버 독립 모듈은 로컬 실행에 필요한 범위를 먼저 판정하고 실동작은 사용자 확인으로 둔다. |

<a id="r4-detail"></a>

### 1.1 R4 세션 조립 체크 (2/8 완료)

단위별 완료 문장은 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)·[1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)·[0.983 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)·[0.950 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0950)·[0.900 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0900)·[0.797 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)·[0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)·[0.635 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)과 각 단위 문서에 있다. 아래에는 하위 단계별 완료 범위와 남은 일만 둔다. 통합 조각을 마칠 때 같은 조건의 일괄 덤프로 M1 진척을 확인한다.

- [x] **R4-1 객체 수명 공통층(0.546~0.600).** 미션에서 `HostSession` 별도 풀을 제거했다. 풀·공장·공간·건물 저장소 공유는 live 슬롯 바이트 대조로 확인했다. Dais·Player·Aviary 개별 몸체, 장기 Unpop·Pop·Display·Graph·Host 실행기, 실제 전송·파생 Runtime pin은 R6 장치 결선에서 잇는다. [마감 감사](docs/dotnet-r4-1-close-20261117.md)
- [ ] **R4-2 적재 Pop 전체 상태 일치.** cpp 근거는 `RawNormalTerrainWorldBinding`·`RawRegionPaint`·`RawRegionOwnership`·`Islandbuilder`·`SquidPostPop`이다.
  - 완료(0.601~0.642): 섬·지형의 3×3 스탬프·섬 `Prefix` 발화·적재 깊이·그래프 per-Pop(0.601~0.605), 지원 타입 요새 일반 Pop과 농지 어댑터·Generate 실결선(0.607·0.612~0.614, 대표3 농지 선택 집합·타입·소유자 100% 일치), 지역 칠하기·소유권과 신전 가상 Owner·사원 Pop 파생(0.618~0.622), 공장 후처리·정기 트리거/본체/분배·Fort 경로 L2 대조·초기 레벨 적재(0.624~0.630), 장식 정기 본체와 세션 분배(0.642, 가상98 전체 몸체·world.Destroy 송신은 R6). [섬 후처리](docs/dotnet-session-island-postpop-20261117.md) · [그래프](docs/dotnet-session-terrain-graph-20261117.md) · [요새 Pop](docs/dotnet-fort-general-pop-20261118.md) · [Generate](docs/dotnet-canon-0614-20261118.md) · [0.622](docs/dotnet-canon-0622-20261119.md) · [0.630](docs/dotnet-canon-0630-20261119.md) · [0.642](docs/dotnet-canon-0642-20261119.md)
  - 남음(일치 대조): 농지 프레임/소유자를 맞춘다(0.728 프레임 일치 thewarbegins 107/152·tutorial1 56/69·surrounded 334/500, 소유자 일치는 합계 68/721). 그래프는 surrounded 255/255지만 thewarbegins 253/255 등 일반 31경로 중 26경로에 차이가 남는다. 표시 등록·스탬프·장부 `totals`/`totalmade`도 덤프와 대조한다.
  - 남음(요새·공장): 비용이 있는 전용 요새 8종(71·75·78·89·122·123·124·158)은 R6 파생 본체에 의존하고, 지원표 밖 처리도 남았다. 공장 뒤 4건의 sid 재사용 순서와 업그레이드 그림 의존 고리도 맞춘다.
- [ ] **R4-3 신전·소용돌이 → Player 첫 지역.**
  - 완료: 신전 Pop → 소용돌이 → 섬 소유자 → Player `+0x78` 경로값이 일치했다(0.622, `thewarbegins`·`tutorial1`). `surrounded` 혼합 경로에서도 첫 지역 9행이 일치했다(0.623).
  - 남음: AI 뇌 경로(`bc1menu76` 사원 없는 6행)는 R4-5 뒤에 닫는다. 농지 가장자리 432칸은 R6·R11 투표 입력이다. 거주 구역 5칸 전파는 0.623 기준이며, 0.635 거주 구역 연결 뒤 `surrounded`는 다시 재지 않았다.
- [ ] **R4-4 정기 처리·프로세스 초기 등록.** 입력은 `RawAssetRegular`과 Player·Dais·Dude·Disc·장식 정기 처리다. 정기 분배와 Kernel/Process 초기 등록은 0.894~0.895로 마감했고 아래 남음 두 단위가 닫히면 체크한다.
  - 완료(틱·정기 본체): 미션 틱 `AdvanceMissionFrame`(0.629), 고정 시각 1프레임 1틱과 덤프 시계(0.631), 매 프레임 `UpdateAmbientWind`(0.632)·주변음(0.633), 간헐천(0.634)·거주 구역(0.635)·펜스(0.636) Pop과 정기, 장식(0.642)·Player(0.643)·스프라이트(0.651)·매몰(0.659) 정기 본체와 세션 분배. [0.631 격차 분해](docs/dotnet-canon-0631-20261119.md)
  - 완료(펜스 생성·삭제): 표식·포스 필드 생성 본체 연결과 연결선·표식 삭제의 전체 파괴 결선(0.644). [0.644](docs/dotnet-canon-0644-20261119.md)
  - 완료(Dais·Dude·Disc·의식, 0.875~0.895): 디스크 정기(0.875·0.877), Dude 정기 본체(0.883), Rune 후처리·PostPop 전수대조(0.885·0.887), 사제 의식(0.886), TempleRitual(0.889), DaisRegular 본체와 공유 분기 RawRitualRegular(0.890), Dude·Disc·Dais 세션 typed 정기 분배(0.891·0.893·0.895), Kernel/Process 초기 등록 계약(0.894). [0.890](docs/dotnet-canon-0890-20261123.md) · [0.894](docs/dotnet-canon-0894-20261123.md)
  - [ ] **남음 1 — 정기 부착 시점과 나머지 틱 난수원.** 0.631에서 분해한 RNG 25,611 격차의 틱 구간, AI 전체 행동의 실제 매 프레임 호출, 장식 재생·연소 등 남은 난수원을 잇는다. 완료 기준: 대표 경로의 틱별 난수 소비가 cpp 덤프와 같아 0.631 분해의 격차가 0이 된다. [0.631](docs/dotnet-canon-0631-20261119.md)
     - 진행(0.996): 주변 바람(0.632)은 매 프레임 소비한다. 0.996에서 틱 소비를 바꾸지 않았음을 전체 34경로 DN 덤프 불변으로 확인했다. 남음: AI 매 프레임 호출(R4-5e)·장식·재생·연소(R6)·공장 사건 부착 시점과 미션 시작 Pop 정렬(R4-6 시작 흐름).
  - [ ] **남음 2 — Dais 실제 장치.** MakeDaisRegularHooks(실제 Frame/Host/Damageable/로컬 SP 결선)·실제0x307 Host 부착. 제단 분기 RunAltarRegular 본문·후크·저항 상태는 0.939에서 이식했다(후크 합성은 세션 설치에서 잇는다). 완료 기준: 세션 Dais 정기가 실제 hooks로 돌고 제단·0x307 분기에 미연결 진단이 남지 않는다.
     - 진행(0.996): 세션 가용분 완료 — `AddLocalSp` 실제 보상 가산(`reward.AddSp`)·신전 0x307 공유 예약·부착의 실제 Host 연결(`processes.AddSharedRegular`). 남음(R6): `Damageable` 전체 몸체(`RawAssetRegular` 이식)·음향·경로·의식 Runtime·제단 전체 후크 합성. [0.996](docs/dotnet-canon-0996-20261125.md)
  - 다른 단계로 넘긴 것: 거주 구역 사건 2(주민 생성), 펜스 이웃 새로고침(이벤트 1), 장식 가상98 전체 몸체·삭제 송신, Player 화면 로그·DataManager 통지는 R6이다. 펜스 표식·연결선 삭제의 송신·통지 일치는 R4-6에서 대조한다.
- [ ] **R4-5 AI 초기화.**
  - 완료(생성·본체): 생성·초기 갱신·해제·동맹 난이도(0.639~0.641), 준비·전략·전체 프레임·통지·월드 등록 본체와 장치 조립·세션 결선(0.645~0.650), 지역 경계·분산 SP·자금·지도 상한(0.653~0.654), 그래프 조회·경제 본체·신전 생산 TYPE(0.657~0.658), 경제 세션 연결(0.662). [최근 경제 결선](docs/dotnet-canon-0662-20261120.md)
  - 완료(하위 행동 본체): 수집자(0.655)·배치 선행(0.661)·시설 생산(0.663, 세션 0.665) 본체, 업그레이드 요청·수신·사건3 완료(0.666~0.667), 실제 업그레이드/연구·생산 Deck·내용물 통지의 세션 연결(0.682~0.686), 공유 분배기·58h 등록·연구 운반(0.687), 1.004~1.025의 20조각(부족 좌표·피해 반응·건설 선택·배치 탐색/표적/평가/스캔/선택/행동·가짜 좌표 탐색/가짜 부족 좌표·사제/운반 행동·자원 선택·다리 경로/후보/계획/검색/전략·이동). 20조각의 파일·대조 수는 [0.1 요약표](#now)의 AI 하위 행동 본체 행에 있다.
  - 남음 — 아래 다섯 세부가 닫히면 체크한다.
    - [ ] **R4-5a 제품 요청 양끝 전송.** AI 업그레이드·연구·생산 요청의 제품 요청/응답 양끝. 완료 기준: AI 요청이 실제 전송·Poll을 거쳐 기존 수신 본체(58h 등, 0.666~0.687)에 닿는다.
    - [x] **R4-5b 하위 행동 세션 호출자.** 수집자·시설과 나머지 하위 행동의 세션 호출자. 완료 기준: 각 본체(0.655·0.661·0.663 등)가 세션 AI 프레임의 실제 호출자에서 원문 순서로 불린다.
      - 완료(1.027~1.030, 호출자 b1~b4): `BuildAiRuntimeDevices`의 `AiRuntimeHooks`에 수집자(`Collectors`)·부족 좌표(`DryScan`·`DryNotice`)·운반(`Carriers`)·사제(`Priest`, 원문 SID 범위 검사 포함) 호출자를 실제 장치로 결선했다(`HostSession.AiCollectors`·`AiDrySpots`·`AiCarriers`·`AiPriests`). 호출자 안의 미연결 하위 갈고리(구매·참여·배치·생산·음향, 에너지 승인, 이동·자원 선택·하역, 탐색·건설 선택·제단·발자국·회수 명령 등)는 원문 순서 도달 시 명시 거부하며 단위 문서가 R6으로 적었다.
      - R6로 넘긴 것(2026-11-25 사용자 결정, 옛 R4-5b5·R4-5b6): 배치 호출자 `AiRuntimeHooks.Drops`(cpp `frameHooks.drops` → `RawAiDropActions.Run`(1.015), 하위 갈고리의 제품 조회·`RawAiDropSelection` 선택 등)와 다리 호출자 `AiRuntimeHooks.Bridges`(cpp `frameHooks.bridges` → `RawAiBridgeStrategies.Update`(1.024)), b1~b4 호출자 안의 미연결 하위 갈고리. 코드의 명시 실패 문구도 "(R6)"이다(`HostSession.cs`). R6 완료 기준: 장치 결선 단언과 미연결 하위의 명시 거부를 집중 검사하고 대표 3경로 L2 덤프가 불변이다.
    - [x] **R4-5c 신전 삭제.** 완료 기준: AI 신전 삭제가 세션에서 원문 순서·통지로 처리된다.
      - 완료(1.032): `BuildAiTempleDestroy()`가 원문 범위 검사(SID 범위, cpp `out_of_range`와 같은 순서) 뒤 전략 `setup.destroy`와 같은 실제 세션 파괴에 전달하고(`HostSession.AiBrains.cs`), 시작한 미션 흐름(`ClientHostGame`)에서 넘긴다. 원문 NoTemple 분기(첫 지역→섬 신전 조회→신전 제거→brain 없이 0 반환)의 순서·통지는 세션 파괴 분배를 그대로 쓴다. 기존 NoTemple 기록 관례(`AiBrains` 0 기록, `MissionPlayersLoadTests.CreatesAiBrainsNoTemplePathWithoutConstruct` 고정)는 유지한다. 근거: [1.032](docs/dotnet-canon-1032-20261125.md).
    - [ ] **R4-5d `SetAiParameter`·`CrankAiMoney` 실제 호출자.** cpp는 `UberGump.cpp`의 aiCommands(`crankMoney`·`parameter`)가 `RawCampaignSession`의 두 메서드를 부른다. 완료 기준: 두 명령이 세션 AI 메모리에 닿는다.
      - 진행(1.026): 명령 분기(`AiUtilityCommandInput` 상수·분류·분배)와 세션 조립(`DispatchAiUtilityCommand`→`CrankAiMoney`·`SetAiParameter`)을 이었다. 남음: 입력 꼬리 결선(UberGump 상당)·초기 간헐천 생성(`GenerateWorldGeysers`).
    - [ ] **R4-5e 매 프레임 실행과 L2 대조.** 완료 기준: AI 매 프레임 실행이 세션에 붙고 258 완료·brain·SP·Player 덤프 행이 cpp와 같다.
      - [x] **R4-5e1 프레임 부착(세션 결선 완료, live 틱 결선은 R6).** cpp `RawCampaignSession::PrepareFrame`은 일시정지가 아니면 `aiRuntime->Frame()`을 부른다. DN 세션에는 `RunAiFrame(bool paused)`을 두어 같은 조건·순서·설치 검사를 세션 API로 수행한다(1.031, `HostSession.PlayerWorld.cs`). 준비 완료 brain은 준비·목표·수집자·시설·부족·사제를 지나 배치에서 원문 순서 명시 실패(`AI 배치 행동 미연결(R6)`)한다. live 틱 결선(매 프레임 호출)은 Drops·Bridges와 함께 R6에서 잇는다 — `RunMissionFrame`에 넣으면 `thewarbegins` 90프레임 덤프가 종료 1로 끝나 L2 수집이 깨짐을 1.031에서 실측했다. DN `RawAiRuntime.Actions`는 사제 다음에 `Drops`·`Bridges`를 조건 없이 부르므로(`RawAiRuntime.cs:140~141`, cpp도 같은 순서) 그때까지 명시 실패한다. 근거: [1.031](docs/dotnet-canon-1031-20261125.md).
      - [ ] **R4-5e2 L2 대조.** 완료 기준: 대표 3경로 뒤 34경로 덤프에서 258 완료·brain·SP·Player 행이 cpp와 같다. R4-4 남음 1의 틱 난수 격차 중 AI 몫도 함께 확인한다.
      - 진행(1.033): 새 도구 `ai_frame_gap.py`로 0998 수집 34경로를 측정한 결과 brain 생성 parity 29/34(나머지 11 owner는 데모 진입 경로·tutorial5/6 적재 사제 결손으로 귀속 규명), SP는 owner 1만 34/34 drift(AI owner 일치), Player 947셀은 로그인·SID 체계, 난수는 34/34 divergence다. 258은 덤프에 행이 없어 양쪽 대조 불가다. 34/34 판정·258·틱 난수 AI 몫 분리·live 틱 결선은 R6에서 잇는다. 근거: [1.033](docs/dotnet-canon-1033-20261125.md).
- [ ] **R4-6 내부 로그인·전송·Client 전역.** 메뉴 핸들 출처와 이름 꼬리 16~19바이트, 로컬 Login→Ready→Start 실제 패킷, 요새 이미지·`Bind*`·State 수명을 연결한다. 실제 handshake 검사와 메뉴/`--mission`의 진입 덤프를 대조한다.
  - 완료(0.689~0.997·1.034~1.061): 묶음별 범위·핵심은 [0.1 요약표](#now)에, 단위별 완료 문장은 [0.797](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)·[0.837](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)·[0.900](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0900)·[0.950](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0950)·[0.983](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)·[1.030](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)·[1.076](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076) 보관본과 단위 문서에 있다.
  - 남음(Root 장치·UI): RU-1·RU-2·RU-3c·RU-3d·RU-4·RU-5·RU-6은 끝났고 RU-3a·RU-3b가 남았다. 둘 다 미션 진입 Root 결선은 0.987에서 이었고 남은 장치는 R5 표시·메뉴·카메라와 R11 수송에 달려 있다. 각 단위는 집중 검사와 대표3 덤프/지표 불변으로 닫는다.
    - 완료(0.901~0.997, 항목별 원문 `[x]` 7개는 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)): RU-1 Root 첫 장치(PostPop 선택 재검사·Resource·Reward·Deletion·PlayerRecords·현재 Menu 데모 getter, `CreateAiWallet` 실제 공급, 0.901)·RU-2 공급값·State와 `StartMission`의 `BindSquidPacketDevices` 생산 결선(0.902·0.903)·RU-3c NativeRoot/HUD 창 생성·Enter(0.989)·RU-3d HUD 작성자(`GumpButtonTree.RegisterChatAlert`의 같은 `ChatAlertId` 칸)·Root reply Tell(`HostMenuInput.TryShowTell`, 0.990)·RU-4 미니맵 설치·HUD 도킹 작성자·선택 수명(같은 `MapRevision`, 전체 viewport 대체 없음, 0.991·0.993·0.994)·RU-5 Chat 점 명령 효과(0.995)·RU-6 Load/Editor 세 설치 값 공개(0.997). 넘긴 것: Flare/Mana/AttackSound/DaisFinish·Gun/Flyer/Aviary 큰 본체·장면음악·색표7·zone 완전 결선(R5/R6), 인게임 HUD 등록·버튼 점멸 확인과 Run(R5·세션 State 부모), Chat 번쩍임(R6)·일반 송신(R11), Load 하위 본체·최종 공개(R5/R6/R9).
    - [ ] **RU-3 Root 공장·창.** 실제 게임 Root의 네 자식·Root/생산 공장·채팅/메시지 공장과 바깥 UI 장치, NativeRoot/HUD 창 생성·Enter, 알림 등록 ID의 실제 HUD 작성자, NSInfo HUD/메타 UI/Root reply 저장. 완료 기준: 미션 진입 Root가 이 공장·창을 소유하고 세 번째 NSInfo와 F6 RawKnowledgeMenu를 구별한다.
      - 완료(0.905~0.931, 묶음 핵심은 [0.1 요약표](#now)의 NSInfo 메타 UI 행): 메시지 공장 팔레트 프로브와 NSInfo 메타 UI 본체(명부·본문·패널·옵션 표·원문 입력·메뉴·입력 분배)의 `HostNsInfoWindows` 생산 결선, 메타맵 관문·특수 그림·입력 후크 분배 검증. 단위 문서는 `docs/dotnet-canon-0905`~`0931`이다.
      - [ ] **RU-3a NSInfo 외부 장치 실제 공급.** 송신·음향·메뉴·카메라·입력·배치 15종 묶음 공장(`HostNsInfoRootDevices`)은 0.986에서, 미션 진입 Root 결선(`ClientHostGame.EnsureRootFactories`)은 0.987에서 이었다. 남음: 참가자·전투 옵션/제외/선택값/제거 메뉴와 중앙 카메라(R5, 지금 명시 실패), 12BYTE 상태·87BYTE 옵션 송신 outbox 비우기(R11 수송). 완료 기준: 미션 진입 경로의 NSInfo가 장치 미연결 명시 실패 없이 동작한다.
      - [ ] **RU-3b 네 공장·바깥 UI 설치.** Root·메시지 공장 집합과 진입 결선은 0.987에서 이식했다(생산은 세션 장치, 채팅은 메뉴 경로 소유를 런타임 장치로 묶음). `HostNsInfoWindows`는 이제 미션 진입 Root도 소유한다(2026-11-25 grep). 남음: 생산·채팅 공장 객체 생성과 메시지 공장 팔레트·배경 생성·사건(R5 표시, 지금 명시 실패), 메뉴 왕복 정리(SL-7). 완료 기준: 위 RU-3 완료 기준을 미션 진입 Root에서 집중 검사로 확인한다.
    - 이 묶음 밖 후속: 실제 F7의 상위 일반 입력, Dais의 실제 current records 하위. NSInfo의 메타 배치/입력/특수 그림 조립은 RU-3(0.907~0.931)에서 끝났다.
  - 남음(전송·수신, 근거 `cpppj/src/client/ClientCampaignTransport.cpp`). `InstallClientPacketDefaults`(270행)의 R4 범위 수신기 설치는 0.979로 끝났다. 남은 것은 Root 생성 부모·Root Source 공개/제출과 수송 세션 갈래이며, 대부분 수송 backend(R11)·세션 수명(R6)이 필요하다. 단위별 완료 문장은 [0.983](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)·[1.030](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030) 보관본과 단위 문서에 있다. 종료·닫기·반납과 연결 활성 정리는 R11 수송에서 잇는다.
    - [ ] 전체 InstallClientPacketDefaults(270행)와 MakeClientCampaignTransport 부모. 아래 완료분은 재사용하고, 남은 설치는 2026-11-24 DN 코드 grep 기준이다.
      - [x] **고정 26F·메타 수신 본체·Build 연결(0.882~0.982).** `NativeGameplay26F` 26항목의 `BindSquidPacketDevices`·`ReceiveFixedGameplayPacket`, `ClientPacketDispatch.Build`/`InstallDefaults`의 41h(`Root.ReceiveNativePacketChat`·`ClientChatPacketReceiver`)·`NativeLoginPacketReceivers`·설정/버전/기술/변수·`NativeGameplayPacketReceivers` 6종·기본 18행, 메타 수신 본체·설치(0.937~0.963, 1a/1b `InstallClientStateBattleStartReceivers`·0x0f `InstallClientMetaGlobalReceivers`·2번 `InstallClientConnectionEndedReceiver`와 본체 `Netstorm.Net/ConnectionEnded.cs` 포함)와 Build 연결(0.964~0.979: Carrier 15행·건설/연구 8행 고정 F `NativeCarrierPacketReceivers`·`NativeConstructionPacketReceivers`, `PacketDelayGlobals` 지연FIFO·`PacketDispatchHooks.Delay` 자리, 0x7b `ClientChallengePacketDefaults`·0x7c `ClientMetaGameLogin`, 2번·1a/1b·0x66·0x26·0x1d·0x0f·0x71·0x21·0x88·0x67·0x20·0x22·0x23·0x24·0x25), 0x0f 서버 값·목록·저장의 NSInfo·Saves 결선(0.981)·0x20 검증(0.982). 후속 갈래(표시·NSInfo·Chat·음향·메뉴 Run 등)는 명시 실패로 남기고 Carrier/Construction 수신 본체는 RU-5/R5 후속에서 잇는다. 수송 팩토리(`ClientCampaignTransport.Make`)는 같은 표를 공유한다. 메타 참가자(0x20·0x21·0x22·0x88)는 남은 설치가 없다. 원래 세 `[x]` 항목의 수신기별 세부는 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)에 있다.
      - [ ] **MakeClientCampaignTransport 수송 팩토리.** 완료 기준: 호출자의 BindSession보다 먼저 같은 기본 표를 채우는 원문 순서로 전체 설치를 한 부모에서 호출하고, 임대 반납 뒤 C3/관찰기/metadata가 복구된다. 진행: 0.964~0.967에서 Carrier/Construction 설치 순서·복구, 지연FIFO, 0x7b·0x7c, 수송 생성·재결합·Publish와 `MissionNetwork`·`TransportGlobals`·`SharesNetwork`·`RebindWorld`·팩토리 후크 조립·기본표 설치 공유를 이었다. 남음: 세션 `RecordConnectionPacket` 갈래·`BindConnectionTransport` 부모, R11의 `BindMetaSource`·`MetaConnectionRuntime`·종료/닫기/반납과 연결 활성 정리.
    - [ ] Root의 실제 Source/Record/Sent·송신/제출과 HostSession 실제 송신 부모(구 `Mission.RawCampaignSession`의 sendChatMutable 입구로 대신하지 않음). Root Source 공개·제출 갈래는 수송 backend(R11)·세션 수명(R6)이 필요하다(0.979~0.983 단위 문서의 다음).
    - [x] 전체 435220 옵션 부모의 Requested만 0으로 재설정하는 호출(0.933, FILE/enabled를 닫지 않음). 재설정 호출을 받는 옵션 재해석 부모(전체 435220 재실행)는 R7 옵션 저장 시점에서 잇는다.
    - [ ] 나머지 순수 observer/장기 수신, 장기 CS/7bh의 나머지 Meta/UI/투영/배열 소비자와 Root 생성 부모. 26F 검토 행·누락 하위 모듈·독립 Path writer 자료는 extracted/dotnet-0788/next-0789.md다.
  - 월드·경로(WP-1~WP-4 완료, WP-5 남음). 순수 observer나 구 Mission 세션·일반 삭제 Pre/Post로 결선을 대신하지 않는다.
    - 완료(1.034~1.040, 원문 `[x]` WP-1~WP-4와 WP-4a~4d 8개는 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)): WP-1 Cleanup 상위 설치(`BuildPathCleanupRootHooks`: 예약·의식 실제 본체·가상 삭제 분배, AfterSpatial 네 분기)·WP-2 제품 Root Path/Start/Motion(`BuildMotionRootEffects`·`BuildMotionRootDevices`를 `ClientHostGame` 진입 경로에)·WP-3 전체 Native World 가상 삭제 분배(`BuildWorldDeletionRootDevices`, 원문 83 가상표, 미복원 표 명시 거부)·WP-4a 파생 수신·원격 삭제의 `VirtualDestroy` 배선·WP-4b `RawDudePostPop`·WP-4c 사제 삭제·지면 소유자의 같은 Host 결선·WP-4d `RunPinnedWorldFrame`. 넘긴 것: 의식 Rune 하위·0x6c 송신 backend(R6/R11), WP-2 효과 묶음의 Cd*·PlayAt·Robust(FillMissing 상당)와 입력·음향·포획·회피 명시 거부(R6), Gump·내용물 Form(R3), 자산 삭제의 Damageable·사제 전처리(R5/R6), 외부 Walker·안내·행동·측정·보충(R6). 근거: [1.034](docs/dotnet-canon-1034-20261125.md)~[1.040](docs/dotnet-canon-1040-20261126.md).
    - [ ] **WP-5 RawGunTarget 보호 자산 검사.** 실제 보호 자산 `cannon-asset-ray48` 검사(0.878 후속). 1.041~1.044 단위 문서 제목의 `WP-5a~d`(사용자 데이터 경로)와는 다른 항목이며 DN 검사에 `cannon-asset-ray48`은 아직 없다(2026-11-26 grep). 완료 기준: 실제 자산으로 표적 Finder의 보호 분기를 집중 검사한다.
  - 표시·입력(DI-1~DI-4 모두 완료). 0.838의 같은 현재 지도 getter를 재사용하고 실제 HUD 도킹이 그 저장소를 작성한다.
    - 완료(0.993·0.994·1.045~1.050, 원문 `[x]` DI-1~DI-4와 DI-4a~4d 8개는 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)): DI-1 미션 Root UI(생산·채팅·HUD)의 매 프레임 수집·그리기(메뉴 경로 불변)·DI-2 같은 지도 owner HUD 작성자(RU-4)·DI-3 커서 호출자(`BuildCursorDevices`: `TakeCursor`·`CancelCursor`·`QueryCursor`·`PrepareCursor`·`TryCancelCursor`, 제품 포인터·Esc 결선)·DI-4 카메라(4a `RawCameraCommit`의 `CommitCameraTarget`·4b `RawCameraPresent`의 `PresentCameraTarget`·4c 저장 카메라·4d Pan 본체와 세션 조립·메뉴 진입 결선). 넘긴 것: 전체 화면 대조(R5 D2), 커서 UI·음향·프로세스 하위(R6), 화면 이동·뒤집기·실제 지도 영역·소리·중심(R5·R6). 근거: [1.045](docs/dotnet-canon-1045-20261126.md)~[1.050](docs/dotnet-canon-1050-20261126.md).
  - 시작·수명(SL-1~SL-8 완료, SL-9 남음). 각 세부는 실제 제품 Root·세션 경로에서 원문 순서로 돌고 미연결 진단이 남지 않으면 닫는다.
    - 완료(1.051~1.061, 원문 `[x]` SL-1~SL-8과 하위 18개는 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)): SL-1 로그인·Root 등록·Clock 제품 순서·SL-2 intro 감지(`DetectIntroMovie`)·PlayIntro 분배(`DispatchPlayIntroInput`)·SL-3 EnergySupply 본체(`EnergySupplyState`·`RawEnergySupply`)와 세션 조립(`BuildEnergyDevices`)·주변음 실재생·SL-4 non-mine Notify 저장 부모(`BuildTemplateSaveDevices`)·SL-5 큰 메뉴 부모와 Source 재인계(`InstallNativeMainMenuState`, parent 3행)·SL-6 WorldRuntime 공유 소유자 행렬 고정·SL-7 영화 갱신 잠금(`PauseMovieUpdates`)·SL-8 RawFileName/RawStaticStrings(0.983·0.985)와 저장 쓰기·읽기 세션 조립(`BuildSavedStateDevices`). 넘긴 것(완료 기준 일부를 뒤 단계로 넘기고 닫음): 장면음악 구동·Guide 진입·메뉴 주변음·화면 번쩍임(R6), 큰 메뉴 부모의 제품 호출(R6 상태 분배), 앞선 Native 초기화 주소 세대(R4-8·M1), 나머지 루프 단계(연결 감시·SceneFrame·미니맵·전투 후처리)·전체 State/자산 재시작·메뉴↔미션 왕복(R5/R6/R7), 편집기 저장(1.057 문서의 R9 표기, 단계 확인 필요), SavedState 꼬리 3전역·파일 장치·폴링·소비자 경로(R6), 에너지 요구 문자열 원천·배치/AI 호출자(R6). 근거: [1.051](docs/dotnet-canon-1051-20261126.md)~[1.061](docs/dotnet-canon-1061-20261126.md).
    - [ ] **SL-9 미룬 세션 로그인 본체(R4-7c3c에서 이관, C안 확정).** 미션 Fort 경로 서버 설치·로그인 객체 전송·미션 재적재·미션 객체 stream·새 Fort 검사와 세션 `LoadLocalConnection`·`Load`·새 Fort 설치 본체. 수송 연결 갈래는 R11에서 함께 잇는다. 근거: [분석](docs/dotnet-session-login-gap-20261125.md). 완료 기준: 13검사가 실제 세션 경로에서 통과한다.
- [x] **R4-7 데모와 보류 검사(0.998~1.003).** `StartDemo` 3경로 DN 진입(R4-7a, 0.998), `player-world-x86.tsv` 중 originals 840행을 DN `RawPlayerWorld` 실제 장면으로 대조(R4-7b, 0.999, CD 840행은 cpppj 몫), F3 보류 검사의 패치 장면(`FortWorldLoadBindingTests`·`NormalTerrainWorldBindingTests`, R4-7c1, 1.000)·CD 장면과 CD 조립(R4-7c2, 1.001)·Template 진단 전체 Client(R4-7c3a, 1.002)·미션 Fort 경로 읽기(`CampaignMissionFortPath`, R4-7c3b, 1.003)를 마쳤다. 미룬 세션 로그인 본체(R4-7c3c)는 C안 확정(2026-11-25 사용자 결정)으로 SL-9·R11 수송에 이관했고 Tell 메타 1종은 대화 본체(R3k-11)로 넘겼다. 하위 체크 6개(`[x]` 5·이관한 `[ ]` R4-7c3c)의 원문은 [1.030 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)에 있다.
- [ ] **R4-8 M1 판정.** 같은 시드·고정 프레임·빈 사용자 루트로 34/34 상태 일치. 로드 뒤 프레임 진행에서만 생기는 차이는 cpp 후속 프레임 덤프로 입증한 뒤 R6으로 넘긴다.
  - 진행(2026-11-25): DN 프레임 1 덤프 3경로 + cpp 프레임 61·150 덤프(thewarbegins)로 분해했다.
    DN은 프레임 1→90에 슬롯을 하나도 만들거나 지우지 않으며, M1 슬롯 수 차이는 DN 프레임 1 시점에 이미
    전부 존재한다. 지형 bounds 34/34·정적 타입 개수는 DN-1과 cpp-90이 일치해 로드 구조물 패리티가 있다.
    cpp에만 있는 슬롯은 전부 생산·건설·채집·플레이어 계열이고 cpp 90→150에 type-6이 1개 더 생긴다.
    owner 불일치(농지 owner 2 대 0 등)는 프레임 61→90에 변하지 않아 로드 매핑·초기 AI 배치 문제다.
    근거: [분해](docs/dotnet-m1-load-vs-frame-20261125.md). 남은 일은 R4-5 AI 호출자·매 프레임 실행과
    R4-4 틱 RNG·장식·공장 사건 부착, R6 런타임·세션 수명이며 M1 재측정은 1.008·1.020·1.024와 1.032 이후 정기 회귀마다 대표 3경로로 이어 1.076까지 DN 3경로 0998 일치·M1 0/34 유지·기준선 0998 유지다.

<a id="r6-preports"></a>

### 1.2 후속 단계에서 이미 이식한 범위

- [x] **R6-3 배치 본체(0.609~0.611).** 권한·관계, 지형 메이커 2종, Player 앵커·서라운딩·파이프라인을 이식했고 MayPlace 3건 전수 일치를 확인했다. 실제 농지 호출은 R4-2(0.614)에서 결선했다. 사용자 건설 입력 통합은 R6-3에서 검증한다. [0.611](docs/dotnet-canon-0611-20261118.md)
- [x] **펜스 본체 8종(0.587~0.592).** `RawFenceLifetime`·`Neighbors`·`Line`·`MarkerClear`·`Rebuild`·`Create`·`PostPop`·`Regular`과 해당 저장소 갈고리를 이식했다. 세션 Pop·정기 결선은 0.636에서 마감했다. 표식·포스 필드 생성은 0.644에서 연결했다([0.644](docs/dotnet-canon-0644-20261119.md)). R6의 타깃·타입 부모·Damageable 정기 라우팅과 연결선 생성·이웃 새로고침은 열려 있다. [펜스 후처리와 정기 처리](docs/dotnet-surface-fence-postpop-20261117.md)
- [x] **시설 업그레이드 본체(0.666~0.667).** 요청·수신·공통58h writer와 사건3 그림/완료·실제 Host 정리·현재 AI 통지 본체를 이식했다. 가상/건설/표시/음향과 요청 API는0.682~0.683, 연구 내용물/Deck·부모 통지는0.686에서 실제 세션에 연결했다. 공유 분배기·58h 등록·연구 운반은 0.687에서 연결했다([0.687](docs/dotnet-canon-0687-20261120.md)). 제품 요청 전송과 UI, 세 비행장 홈 통지·게임 내 HUD 메시지(0.682)는 후속이다. [요청](docs/dotnet-canon-0666-20261120.md) · [그림/완료](docs/dotnet-canon-0667-20261120.md)
- [x] **플레어·건설 본체와 실제 장치(0.669~0.681).** 플레어 생성/정기, 건설 진행/완료/실제Host 수명, 밀어내기/실패 섬 정리(0.669~0.671), 건설 표시 생성/동기화/입자 선택1,776행·프레임/Draw1,317행과 실제10.78 Kernel/Form/공통 목록 수명12건(0.673~0.675), 공장 가상 그림·실제 SHP 캡처(0.677), 같은 WorldRendering/Host/Kernel·COL 팔레트·현재 카메라/dirty/8비트 출력의 제품 건설 표시 장치(0.678), 플레어의 실제 장치·정기·음향(0.679), 건설 진행/완료 장치(0.681)를 연결했다. private 효과의 DropIsland 낙하/전송, 실제 건설 요청, Carrier 플레이 효과, 나머지 캡처·전용 색표·R5 월드 뷰는 후속이다. [플레어](docs/dotnet-canon-0669-20261120.md) · [건설 진행](docs/dotnet-canon-0670-20261120.md) · [표시 프레임](docs/dotnet-canon-0674-20261120.md) · [0.678](docs/dotnet-canon-0678-20261120.md) · [0.681](docs/dotnet-canon-0681-20261120.md)
- [x] **간헐천 수명과 벌룬 본체(0.583~0.584).** 세션 Pop·사건 0 정기는 0.634에서 결선했다. 채굴 `RawGeyserMined`은0.803에서독립2312관측/Host5bh를연결했다. Root의전체음향/가상삭제와운반상위는남았다. `RawBalloonFrame`은0.795의Path본체에포함되며제품Root설치가남았다. AI 등록은 R4-5에서 잇는다. [간헐천](docs/dotnet-surface-geyser-20261117.md) · [벌룬](docs/dotnet-surface-balloon-20261117.md)
- [x] **Outpost 전처리 본체(0.582).** 장기 삭제 분배·목록 압축·부모 호출을 집중 검사했다. 전체 Outpost 수명과 LAN 통합은 R11에서 이어간다. [Outpost](docs/dotnet-surface-outpost-20261117.md)
- [ ] **공통 장치의 실제 미션 연결.** `RawSurfaceAssetDeletion/Notice`·`Content/Reward`·`Lifetime/Deletion`·`PriestDeletion`·`Damageable/Fence/Balloon`은 대응 본체 또는 분배 장치가 있다. 큰 파일의 미이식 분기와 호출자 장치는 R4-2~R6에서 확인한다. `RawIceTower`의 성장 정기·완성형 파괴·재생성/삭제와 Native 설치도 남아 있다. [R4-1 감사](docs/dotnet-r4-1-close-20261117.md)

<a id="r3-r2-detail"></a><a id="carried-limits"></a>

### 1.3 앞 단계에서 이월한 작업

- [ ] **R3 UI.** `ParseDialogPage`·`ConfigOverwriteAndQueueLifetime` 잔여와 MessageGump, 맵/파일/기술 목록·Guide·모달·명령 소유자, QuickHelp TYPE/AST, Console/클립보드/Save as/숫자 창, ChatWindow 하위 화면(상태·기하·전체 부모·공장은 0.767~0.774, NativeRoot 창 생성·Enter는 RU-3c 0.989에서 완료), 배경 GIF 노드·창, 하위 화면 라벨과 실제 명령 소비. HTML/스타일 그림의 실제 자산/본문·메뉴 출력은0.773 완료이며 Native 색표7/옵션 변경 부모는 후속이다.
- [ ] **R2 화면 통합.** 실자산·그리기 결선과 구름 카메라·시간 이동·dirty 갱신·구름 파일 fallback(R5). 빠른 물리 입력 이력과 옵션 변경 중 소리 수명은 R6.
- [ ] **파일·호스트 경계.** `--set` 중복 처리 차이, `MatchDisk` 절대/긴 경로, Linux `WriteAtomic`·`ReadOnly`, 이름 정규화·옛 인코딩, Fort 이름/Template 소유자·Save as는 해당 소비자를 옮길 때 대조한다. CD 슬롯 폭·경계의 하네스와 기존 단위의 예외·할당·복사 경계는 보관본 2.6과 단위 문서를 참조한다.

<a id="r5-detail"></a>

### 1.4 R5 월드 화면 (2/5 완료)

렌더러 수집·정렬·dirty/팔레트, 타입별 Draw와 효과, 카메라·전체화면 가장자리 스크롤, HUD·덱·미니맵·인벤토리. 완료 기준: 미션 진입 직후와 N프레임 뒤 34경로의 L4a 논리 화면 차이가 0이고 압축 해제 실행과 일치한다. R6와 겹치는 입력·음향·포획·회피·건설 도착·live Path 틱은 해당 단계에서 잇는다.

- [x] **R5-1 카메라 프레임 적용(1.062, 1.064 프레임 전용 교정).** 제품 루프에 `BindMissionCameraDevices`·`ApplyMissionCameraFrame`을 이어 매 프레임 실제 지도 영역으로 목표·현재값을 적용한다(전역·순서·노출·정지 3건 대조, 대표 3경로 덤프 불변). 1.064에서 Pan·저장 조립을 뺀 `BuildCameraFrameDevices`로 교정했다(매 프레임 포인터 폴링의 Pan 명시 실패 회귀, 미조립 통과 1건 대조). 뒤집기는 매 프레임 출력에 맡긴다. 근거: [1.062](docs/dotnet-canon-1062-20261126.md) · [1.064](docs/dotnet-canon-1064-20261126.md).
- [x] **R5-2 월드 그리기(수집 결선까지, 1.063~1.068).** viewport 환산·타일 묶음·객체 수집·분배·구름·배경·앞단·세션 수집 결선을 이었다(2a 5건·2b 6건·2c 11건·2d1 5건·2d2 5건·2d3 4건 대조, 정기 전체 회귀 2회 7,326·7,351, 대표 3경로 덤프 불변). L4a 타일 일치는 Submit·프레임 구동이 이는 R5-3에서 닫는다.
  - 완료 조각(원문 `[x]` 7개는 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)): 2a 수집 viewport 환산 `RawRendererViewport`(1.063)·2b 타일 묶음 `RawTileGroupCollection`(1.064, 정기 7,326)·2c 객체 수집·분배 `RawObjectCollection`·`RawDrawSubmission`(1.065)·2d1 구름·배경 `RawRendererClouds`(1.066)·2d2 앞단 `RawRendererPrelude`(1.067)·2d3 세션 수집 결선 `BuildWorldDrawingDevices`·`CollectWorldDrawing`(1.068, 정기 7,351). 근거: [1.063](docs/dotnet-canon-1063-20261126.md)~[1.068](docs/dotnet-canon-1068-20261126.md).
- [ ] **R5-3 타입별 Draw와 효과.** 타일 출력·Submit·프레임 구동과 일반 자산·건설 그리기. 완료 기준: 아래 남은 세 조각이 닫혀 Submit의 kind 0·1·3이 건너뜀·명시 실패 없이 그려지고 미션 진입 직후 월드 픽셀이 L4a와 일치한다(34경로 전체 대조는 R5-5). UI(kind 2)는 R5-4d, 미리보기(kind 4)는 R6이다.
  - 완료(1.069~1.073, 원문 `[x]` 5개는 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)): 3a 미션 진입 L4a 기준선(`thewarbegins` 100프레임 95.1090%)·3b 일반 타일 직접 합성(`RawTileSprites`)·3c1 소유자·그림자·죽음·추상 분기·3c2 세션 Submit(`SubmitWorldDrawing`, 수집·정렬·분배·타일 제출, 정기 7,360)·3c3 프레임 구동(`RenderMissionWorldFrame`, 수집 0건 원인 규명: 지도 영역 0 → 1.075, 메뉴 덮음 → 1.076·R5-4c1). 근거: [1.069](docs/dotnet-canon-1069-20261126.md)~[1.073](docs/dotnet-canon-1073-20261126.md).
  - [ ] **R5-3c4 메뉴 전환 뒤 L4a 타일 재측정.** 1.073에서 이름만 남긴 조각이다. R5-4c1 메뉴 배경 숨김 뒤 `thewarbegins` 100프레임을 다시 재어 타일 영역 차이를 분해한다(1.076 재측정 89.0096%). 완료 기준: 미션 진입 직후 타일 픽셀이 cpp L4a와 일치한다.
  - [ ] **R5-3d 일반 자산 그림(kind 0).** `SubmitWorldDrawing`의 Sprite 갈고리는 kind 1 타일만 그리고 일반 자산은 조용히 건너뛴다(`HostSession.WorldDrawing.cs`). 같은 SHP·색표·카메라로 건물·유닛 등 객체 그림을 잇는다. 완료 기준: 건너뛰는 kind 0이 없고 진입 직후 객체 픽셀이 L4a와 일치한다.
  - [ ] **R5-3e 건설 표시(kind 3).** Construction 갈고리는 지금 `월드 건설 그리기 미연결(R5-3)` 명시 실패다. [1.2](#r6-preports)의 건설 표시 장치(0.673~0.678)를 같은 출력기에 잇는다. 완료 기준: 건설 항목이 명시 실패 없이 그려지고 집중 대조가 통과한다.
- [ ] **R5-4 HUD·덱·미니맵·인벤토리.** 도크 배치·지도 영역·메뉴 전환과 생산 덱·미니맵·인벤토리·공통 HUD의 제품 경로. 완료 기준: 패널·미니맵·인벤토리 표시와 미션 진입 직후 타일이 L4a와 일치한다.
  - 완료(1.074·1.075, 원문 `[x]` 2개는 [1.076 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261126-through-1076)): 4a UberRoot 조립 진입점(`HostUberRoot`: 네 공장·메시지·전역, 제품 미결선)·4b HUD 구분선 셋과 도크 배치로 `MapView`를 실제 HUD 배치로 둠(`EnsureMissionHudDividers`, 실측 map=(85,0,1024,672)·수집 2건). 근거: [1.074](docs/dotnet-canon-1074-20261126.md) · [1.075](docs/dotnet-canon-1075-20261126.md).
  - [ ] **R5-4c 메뉴 전환·생산 덱·미니맵·인벤토리.** 메뉴↔미션 전환과 생산 덱·미니맵·인벤토리·공통 HUD의 제품 경로. 완료 기준: 패널·미니맵·인벤토리 표시와 미션 진입 직후 타일이 L4a와 일치한다.
    - 완료(1.076): 입장 카메라 고향(`RawCameraHome`·`ResetMissionCamera`·제품 `ResetCamera` 결선, 12+2건)·특수 셰이프 타일 조용한 건너뜀(1건)·미션 중 시작 화면 다시 그리기 차단(L4a 89.0096% 재측정·L2 3경로 덤프 불변·정기 전체 회귀 7,379). 근거: [1.076](docs/dotnet-canon-1076-20261126.md).
    - [x] **R5-4c1 메뉴 배경 숨김.** 메뉴↔미션 전환에서 메뉴 배경이 월드를 덮지 않게 했다(1.073은 R6 상태 분배·SL-5 잔여로 적었고 1.076이 R5-4c로 당겼다). 완료 기준: 미션 진입 화면에 메뉴 배경 픽셀이 남지 않는다.
      - 완료(1.078): 미션 중 메인 메뉴 돌 버튼·배경을 그리지 않고 입력도 소비하지 않는다(`HostMainMenu.IsMissionActive`·`HidesMainMenuForMission`, `HostGumpDrawing.IsMissionActive`, 제품 `ClientHostGame` 결선, 집중 2건·관련 3건 통과·Game 오류0·경고0·`cpp_sync.py` 종료0). 근거: [1.078](docs/dotnet-canon-1078-20261126.md). 미션 임무 UI는 같은 트리의 다른 토큰이라 그대로 둔다.
    - [x] **R5-4c2 생산 덱 틀.** 생산 패널 본체(0.825~0.837)의 틀을 미션 HUD에 부착해 그린다. 완료 기준: 진입 직후 틀 영역의 표시·우클릭 도움말이 일치한다. 격자 내용물·입력/유지는 R6/R11이다.
      - 완료(1.079 틀 부착): 생산 덱 틀 8노드·예산/`BridgeGrid`·배경 HELP와 `GumpHelp` 우클릭 1 반환·주제·제목·부착·소리를 확인하고(집중 2건·관련 6건 통과·Game 오류0·경고0·`cpp_sync.py` 종료0) 미션 진입에 왼쪽 틀을 부착해 그린다. 근거: [1.079](docs/dotnet-canon-1079-20261126.md). 격자 내용물·유지·입력·동기화 본체와 About 본문 표시는 R6/R11에서 잇는다.
    - [ ] **R5-4c3 미니맵.** RU-4 미니맵(0.991·0.993)의 부착 코드는1015ca4a6에 보존했지만 검증 미완료다. 아래 세 조각을 모두 통과해야 완료한다.
      - [x] R5-4c3a Game 재빌드·관련 검사 — 루트 Idle 차단 뒤 Release와 미니맵/생산 틀 관련 검사가 통과한다.
      - [x] R5-4c3b 대표 L2 재수집 — thewarbegins/tutorial1/surrounded 세 종료0·0998 기준선/M1 0/34 유지. 실패 원인과 첫 실행을 보존한다.
      - [ ] R5-4c3c 실제 픽셀 — 미니맵 영역이 cpp L4a와 일치하며 진입/정리의 장치 수명을 확인한다. 2026-11-27 실제 비교는 6,000/6,000 픽셀 불일치: 임시 채팅창 가림을 1.081에서 교정해 차이는 901/6,000으로 줄었다. 1.082 카메라 좌표·프레임 갱신 교정 뒤 388/6,000 차이가 남는다. 1.083에서 호스트 정상 종료 반납을 이었다. 공통 저장소 Pop/Unpop 알림은 1.084에서 연결했다. 1.086에서 세션 지형·Player 실행기도 연결했다. 그래프 재결선은 1.087에서 교정했다. 1.088 최초 채팅 숨김 뒤 미니맵 차이는 1픽셀이다. 1.089의 채팅 재열기는 단위 검사만 통과했고 실제 자동 키 입력은 미전달로 미확인이다. 다른 실행기 재생성·미션 전환 수명과 실제 이동 시험도 미완료다. [최초 관측](docs/dotnet-canon-1080c-observation-20261127.md) · [1.081](docs/dotnet-canon-1081-20261127.md) · [1.082](docs/dotnet-canon-1082-20261127.md) · [1.083](docs/dotnet-canon-1083-20261127.md) · [1.084](docs/dotnet-canon-1084-20261127.md) · [1.085](docs/dotnet-canon-1085-20261127.md) · [1.086](docs/dotnet-canon-1086-20261127.md) · [1.087](docs/dotnet-canon-1087-20261127.md) · [1.088](docs/dotnet-canon-1088-20261127.md) · [1.089](docs/dotnet-canon-1089-20261127.md)
    - [ ] **R5-4c4 인벤토리·공통 HUD.** 인벤토리와 공통 HUD(0.817~0.819)의 제품 경로. 완료 기준: 해당 패널 영역이 L4a와 일치한다.
  - [ ] **R5-4d UberRoot·패널 결선.** 1.074 조립 진입점을 제품 진입 경로에 잇고 네 자식·메시지 공장으로 패널을 만든다(RU-3b 남음과 같은 대상). 월드 UI 그리기(kind 2)의 `월드 UI 그리기 미연결(R5-4)` 명시 실패도 여기서 닫는다. 완료 기준: 미션 진입 Root가 UberRoot 패널을 소유하고 UI kind가 명시 실패 없이 그려진다.
- [ ] **R5-5 가장자리 스크롤·전체 대조(D2).** 전체화면 가장자리 스크롤과 34경로 진입·N프레임 뒤 L4a 차이 0. 완료 기준: D2 판정을 닫는다.
  - [ ] **R5-5a 전체화면 가장자리 스크롤.** AGENTS 요구대로 전체화면에서 커서가 화면 끝에 닿으면 카메라를 옮긴다(옵션 끔 존중, Pan 본체는 1.050). 완료 기준: 네 방향·옵션 끔·창 모드를 집중 검사한다.
  - [ ] **R5-5b 34경로 L4a 대조.** 진입 직후와 N프레임 뒤 34경로 논리 화면을 cpp와 비교한다. 완료 기준: 차이 0이고 압축 해제 실행과 일치한다(D2 판정).

<a id="dotnet-current-plan-20261028"></a><a id="05-실행-순서-r0r8"></a><a id="04-기존-dotnetpj-소스의-재수정-판정"></a><a id="decisions"></a><a id="08-사용자-확인이-필요한-것-확인-전까지-에이전트-기본값으로-진행"></a><a id="dotnet-user-decisions"></a>

## 2. 계속 유효한 결정

- **원본 10.78 경계 보존.** 화면은 8비트 인덱스 버퍼·256색 팔레트, MonoGame은 창·입력·오디오 장치·최종 출력이다. cpppj의 파일/클래스·호출 순서·부작용·정수 폭·float32 저장·목록·난수·PATCH 50바이트 슬롯 배치를 보존한다. 플랫폼 장치는 .NET/MonoGame으로 대체하고 OS 전용 동작은 호스트에 둔다. cpppj 미복원·보류 경계는 명시 진단으로 드러낸다. CD 10.72 제품 복원은 cpppj 범위다.
- **D1(사용자 결정).** cpppj HEAD의 요구사항 코드가 공통 파일에 섞여 있으면 함께 옮긴다. 기본 영어·보통·75 FPS·4:3 창 모드에서 원본 동작을 먼저 대조하고 후순위 기능은 R8에서 검증한다. 파일 장치는 사용자 루트 정책을 초기 이식부터 적용한다.
- **D2(사용자 결정,0.807 적용 완료).** 2026-11-22 사용자가 즉시 전환을 지시해 구 실행기를 제거하고 직역 호스트가 Game/Netstorm.exe를 승계했다. 기존 settings.json은 가져오지 않으며 홈의 기존 저장값을 삭제하지 않았다. 전용 자동 실행 검사16개는 은퇴시켰고 공통 부품/L1 검사는 유지한다. R5 전환/R6 삭제라는 이전 에이전트 해석은 폐기한다.
- **D3~D5(사용자 결정).** R8에서 로컬 MCP와 한국어·일본어 화면을 검증한다. Linux는 `$HOME/.netstorm/` 아래에 저장한다. Template 판독·433 Fort 왕복 검사의 기존 결정은 [Template 계획](docs/dotnet-template-storage-plan-20261101.md)에 있다.
- **사용자 지시.** 정기 전체 회귀는 4단위당 1회만 수행한다(2026-11-19). 포획/희생·특정 적 공격·승리 재현은2026-11-22 지시로캠페인1-1~1-5·스커미시에서만아군AI 활성화로허용하며나머지는사용자수동확인으로둔다.

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

현행 AGENTS 변경은 갈무리11 지정 `2c8ca3bf8`, 15차 추가 `faecb9cbd`, 실행 파일 아이콘 `d047bf9a3`([0.4](#exe-icon)), webpj favicon `11b16b778`(DN 해당 없음), 인계 문서 정리 규칙 `1fc63cb1a`(문서 관리만, 2026-11-23~26 정리에 적용)까지 확인했다(2026-11-26 재확인). 아래는 새 경로 기준이다.

| 차수 | 처리 및 검증 |
| --- | --- |
| 4 로컬 MCP·와이드 | stdio JSON-RPC로 모든 조작·상태 조회·화면 캡처·소리 포함 녹화. 4:3·16:9·16:10, 16:9 최대 1920×1080의 논리 해상도와 그래픽 크기 유지. [MCP](docs/exe/cpp-mcp-server-20261106.md) · [와이드](docs/exe/cpp-wide-resolution-20261106.md) |
| 5 사용자 데이터 | Windows `%USERPROFILE%\Saved Games\Netstorm\` 아래 모든 런타임 생성물. 설정·진행은 `Saves/`, 맵은 설치 경로와 `CustomMaps/` 양쪽 읽기, 새 Edit 맵은 `CustomMaps/` 저장. 목록 무제한 스크롤과 읽기 전용 설치 확인. [cpppj 사용자 루트](docs/exe/cpp-user-data-root-20261105.md) · DN 결선 완료: 1.041 옵션 경로·1.042 CustomMaps 읽기·저장·1.043 목록 스크롤·1.044 읽기 전용 확인(단위 문서 제목의 `WP-5a~d`는 [1.1](#r4-detail) WP-5 RawGunTarget과 다른 묶음) |
| 6 한국어 | 외부 갈무리11과 한국어 명칭 규칙. 표시용 부호화·`DisplayTextInput`의 선행 이식을 화면·번역과 연결한다. [전환 체크](#korean-font-change) |
| 7 오류·로그 | 공통 오류/예외 처리와 상세 로그(`logs/yyyyMMddHHmmss.log`), 시작 실패부터 추적 가능해야 한다. [cpppj 로그](docs/exe/cpp-error-log-20261111.md) |
| 8 AA·FSR 1.0 | Options에서 선택, 둘 다 기본 OFF. 지원 환경의 2560×1440·3840×2160은 내부 1920×1080에서 자동 FSR. [cpppj 출력 계약](docs/exe/cpp-aa-fsr-20261106.md) |
| 9 시나리오 Edit | 새 맵 기본 `.english` 자동 생성, 누락 시 Test Battle 종료 오류 교정, 편집 UI에서 초기 SP·테크·플레이어·동맹 수정. [cpppj 편집](docs/exe/cpp-scenario-s3-ui-20261111.md) |
| 10 알려진 버그 | cpppj의 34건 판정과 수정 10건을 해당 파일의 이식·검증에 반영하고 NB 번호를 기록한다. [판정표](docs/exe/cpp-known-bugs-status-20261111.md) |
| 11 난이도 | Options 행과 AI 배율 부품 선행. 기본은 보통(원본), 쉬움은 적 AI의 다리·생산 주기 ×4와 채취 유닛 절반(최소 1), 어려움은 적 AI의 주기 ×½. 동맹 판정 포함. [cpppj 난이도](docs/exe/cpp-difficulty-d-20261111.md) |
| 12 스커미시 | Campaign에서 시작/취소, 적 수·AI별 난이도/동맹, 금지 유닛 최대 5개(Outpost 포함). 멀티플레이 규칙의 임의 섬·간헐천. [cpppj 스커미시](docs/exe/cpp-skirmish-sk-20261111.md) |
| 13 일본어 | 외부 Noto Sans JP, 일본어 UI와 본문 영어 대체. 번역 범위는 cpppj D3. [cpppj 일본어](docs/exe/cpp-japanese-l1-l3-20261111.md) |
| 14 QuickJS | quickjs-ng 엔진 탑재·자체 검사·라이선스 동봉. **2026-11-23 사용자 결정: 모딩 지원** — webpj 이름 체계의 공통 모드 API를 QuickJS로 제공하고 게임 로직은 C#을 유지한다. 결정·세부 단위는 [3.1](#dotnet-modding). [cpppj 엔진 계약](docs/exe/cpp-quickjs-engine-20261111.md) |
| **15 Reset All** | **미착수.** Options 맨 아래에 Reset All을 제공한다. 취소 시 현재 설정·진행을 보존하고 확인 시 `options.cfg`를 포함한 설정·게임 진행을 초기화한 뒤 종료한다. 다시 실행해 최초 상태를 검증한다. cpppj1325에서 확인/취소·초기화·종료와 최초 상태 복원을 구현했다(2026-11-22). DN은 해당 Source를 옮길 때 대조한다. [AGENTS.md](AGENTS.md) |

**공통 완료 사항.** 기존 배포의 `Netstorm.exe` 이름과 압축 스크립트, 11.01·Credits 3항목·HJOW/AI 크레딧 및 카피라이트(0.150·0.342), 폐기 클라이언트 유효성 검사 비활성·외부 접속 문서화(0.151). 0.807에서 직역 호스트가 `Netstorm.exe` 이름을 승계하고 원본 아이콘을 EXE/ZIP에 적용했다(창 표시 확인은 [0.4](#exe-icon)). 새 소비자가 생기면 외부 접속 목록을 갱신한다. [배포 스크립트](dotnetpj/package-dotnetpj.ps1) · [외부 접속](docs/dotnet-external-endpoints-20261103.md)

<a id="dotnet-modding"></a>

### 3.1 14차 모딩 — QuickJS + webpj 이름 체계의 공통 모드 API (2026-11-23 사용자 결정)

**결정.**
- dotnetpj에 QuickJS(quickjs-ng, cpppj와 같은 v0.18.0 소스·provenance `cpppj/third_party/quickjs-ng`)를 탑재해 다른 사용자가 기능을 더하는 모딩을 지원한다.
- 모드 API는 webpj의 함수·클래스 이름 체계를 따른다. webpj 내부 모듈(`Raw*`·원문 주소식 이름 등)을 그대로 노출하지 않고, 이름·형태를 webpj에 맞춘 얇은 공개 계약을 정한다. webpj가 기준 구현이고(브라우저 JS라 엔진 불필요) dotnetpj는 같은 계약을 QuickJS 바인딩으로 제공한다. 계약 설계 뒤 webpj도 맞춰 바뀔 수 있다([WB 모딩 API](LEFT_JOBS.webpj.md#modding-api)).
- 게임 로직은 C#을 유지한다(로직을 JS로 옮겨 webpj 코드를 재사용하는 안은 채택하지 않음). cpppj 모딩은 아직 진행하지 않는다(LEFT_JOBS.md D7 그대로).
- 착수 시점은 사용자 지시를 따르며 현 재개 순서(R4)는 바꾸지 않는다. MOD-1~3은 월드 연결과 독립이라 먼저 할 수 있고, MOD-4는 해당 기능이 DN에 연결된 뒤 한다.

**검토 근거(2026-11-23 측정, scratchpad 임시 실행기·헤드리스 Chromium).**
- QuickJS-ng 0.18(cpppj Release 정적 라이브러리)이 webpj 실제 ES 모듈(상대 import 포함)을 고치지 않고 불러 실행했다.
- 속도: 같은 벤치마크(webpj `IndexedSurface.drawImage` 합성·float32 수치·private 필드 클래스/Map·BigInt)에서 V8 JIT보다 5~115배(합계 약 41배), V8 인터프리터보다 2.0~2.8배 느리다. 모드 API는 이벤트·명령 단위로 하고 매 프레임·매 유닛 반복 처리를 모드에 맡기지 않는다.
- 결정론: `Math.sin`·`cos`·`atan2`·`pow`의 double 결과가 V8과 각각 2.6%·2.7%·17.3%·0.15% 다르다(2만 입력, float32로 줄이면 0건). 엔진의 이 함수들을 V8과 같은 fdlibm 구현으로 바꾼다.

**세부 단위(각각 완료 기준).**
- [ ] **MOD-1 공통 모드 API 계약 v0.** `docs/`에 범위(이벤트 훅·읽기 전용 상태 조회·원본 입력 큐로 들어가는 명령), 이름(webpj 클래스·함수 이름과 camelCase), 버전 규칙, 오류 처리, 결정론 규칙(게임 시계·난수만 사용, Math 함수 동일 구현), 보안(파일·네트워크·OS 미노출, 메모리·시간·스택 한도), 모드 위치(`%USERPROFILE%\Saved Games\Netstorm\Mods\`, Linux `$HOME/.netstorm/Mods/`), 기본 꺼짐·원본 대조 시험은 모드 꺼짐 상태를 적는다. 완료 기준: webpj 기준 구현 범위와 함께 확정해 두 인계 문서에서 링크한다.
- [ ] **MOD-2 QuickJS 엔진 탑재.** quickjs-ng를 win-x64 네이티브 라이브러리로 빌드해(ARM64·Linux는 후순위) `Netstorm.Game` 배포에 넣고 얇은 C 래퍼 + P/Invoke로 런타임·컨텍스트·평가를 감싼다. quickjs-libc는 넣지 않고 한도는 cpppj `ScriptEngineLimits`와 같게 둔다. 완료 기준: cpppj `--quickjs-selftest` 6항목과 같은 자체 검사 통과, 배포 ZIP에 라이브러리·라이선스 포함.
- [ ] **MOD-3 모듈 로더·호스트 shim.** 모드 폴더 안 ES 모듈만 불러오는 로더, `console`/`print` → 7차 로그, 계약이 요구하는 최소 전역, `Math.sin/cos/atan2/pow`의 fdlibm 구현 주입. 완료 기준: 같은 수학 격자에서 V8과 비트 일치, 모드 폴더 밖 import 거부.
- [ ] **MOD-4 API 바인딩.** 계약 v0의 이벤트·조회·명령을 DN Client/Core에 연결한다(명령은 원본 입력 큐 경유, 원본 호출 순서 보존). 완료 기준: 공통 적합성 시험 모드 스크립트가 DN과 webpj에서 같은 결과를 낸다.
- [ ] **MOD-5 모드 관리·멀티플레이.** Options에서 모드 목록·켜기/끄기(기본 꺼짐), 모드 오류 시 그 모드만 끄고 로그, LAN(R11)에서는 참가자 모드 목록·해시 일치 검사. 완료 기준: 실제 GUI에서 켜기·끄기·오류 격리와 해시 불일치 시 접속 거부 확인.

<a id="resync-scope"></a><a id="cpppj-requests"></a><a id="dotnet-inventory"></a>

## 4. cpppj 동기화와 이식 목록

- **cpppj 현행은 제품1377/11.01·ZIP1378·검증1380이다.** DN 이식 기준1323/오라클1322를 임의로 올리지 않는다.1328~1330 추가 요구는0.1, 일본어43본문/help128주제는 R8 13차, 1345의 확장 AI 이름31BYTE 제한은 언어 무관 후속, 도움말/LAN1363~1375는 DN-MP-01~08, 튜토리얼1371~1372는 [DN-TUT-1~4](#tutorial-objective-port), 로그1377은 R8 7차/해당 소비자 재대조에서 확인한다. 기존 변경 파일/base별 상세는 [정리 전4절](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261127-through-1079-checkpoint1080)에 있다. 현재 재대조를 수행한 것으로 세지 않으며 각 소비자·fixture·검사까지 확인한 뒤 PORT_MAP base를 올린다. CP1380 사용자 결정대로 전투 중 새 ID 참가는 추가하지 않는다. [공통 현행](LEFT_JOBS.md)
- **기존 재대조 규칙.** CPP 소스는 커밋된 HEAD를 사용한다. 해당 소비자를 옮길 때 바뀐 cpp 소스·fixture·검사를 함께 확인하고, cpppj 테스트 자동 플레이와 관전 경계는 R5/R6에서 잇는다. [오라클 자동 플레이](docs/testing/cpp-autoplay-test-guide.md)
- **현재 집계(2026-11-27 정리1381, 읽기 전용 재집계).** PORT_MAP 물리1,901줄(자료1,896행, port1,408·L1 481·session7)로 1.026(`f7140a26b`) 뒤 바뀌지 않았고, 연결 fixture513개도 1.030 정리 뒤 그대로다. `python -X utf8 tools/port_inventory.py` 재집계로 현행 cpppj1244개 소스 단위170,238줄 중 DN 대응690개115,930줄·미대응554개54,308줄이다(1.076 당시 CP1242개/DN690개·미대응552개에서 CP 신규2단위가 더해졌으며 DN 이식 완료 수가 늘어난 결과는 아니다). 아래 행 누락만큼 DN 대응을 적게 센다. 부분 이식을 포함하므로 전체 분기·Host 결선이나 플레이 완료를 뜻하지 않는다. 1.076 정기 전체 회귀와 각 단위의 `cpp_sync.py`가 종료0이다.
- **PORT_MAP 행 누락(2026-11-26 재집계).** `5e173e3bb`(20261124-14 DN, RU-3 등록 명부) 뒤 새로 만든 C# 103파일 중 70파일에 행이 없다. 1.030 정리 때의 44파일(`5e173e3bb`~0.983의 38파일 — NSInfo 메타 입력·BattleOption·메타 수신기·Carrier/Construction 수신·`ClientCampaignTransport`·`MissionNetwork`·`RawPacketDelay`·`RawFileName.Query` 등 — 와 1.002 `ClientDiagnosticGlobals`·`ClientTraceLog`, 1.027~1.030 `HostSession.AiCollectors`·`AiDrySpots`·`AiCarriers`·`AiPriests`)에 1.031~1.076의 26파일(Display `RawCameraCommit`·`RawCameraPresent`·`RawDrawSubmission`·`RawRendererClouds`·`RawRendererPrelude`·`RawRendererViewport`·`RawTileSprites`, Input `RawCameraPan`·`RawSavedCameraKey`, Core `RawCameraHome`·`RawDudePostPop`·`RawEnergySupply`·`RawObjectCollection`·`RawTileGroupCollection`, `NativeMainMenuState`·`RawIntroMenu`·`HostUberRoot`, `HostSession`·`ClientHostGame` 부분 파일 9개)이 더해졌다. 38파일의 머리 주석이 근거로 드는 cpp 중 대응표 밖 단위는 0.983 정리 때 `NativeMeta*`·`NativeBattleInfo*`·`RawBattleOption*`·`RawStateBattleStart`·`RawRootLoginReply`·`RawLocalServerDiscovery`·`RawPacketDelay` 등 31단위·2,113줄이었다(참조만 하는 `State`·`RawMeltdown` 포함이라 행마다 대응 판정이 필요하다). 다음 PORT_MAP 갱신 때 행을 더하고 `port_inventory.py`를 다시 돌린다. 파일 목록: `git diff --name-only --diff-filter=A 5e173e3bb HEAD -- dotnetpj/src`를 PORT_MAP csharp 열과 대조한다.
- **파일 계열 목록.** 현행 [PORT_MAP](dotnetpj/PORT_MAP.tsv)과 `python -X utf8 tools/port_inventory.py --families dn`을 사용한다. 예전 단계 배정 및 대응 파일의 미완 부분은 [보관본 2.4~2.7](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)에서 찾는다. 단위 작업은 해당 계열이나 큰 파일 하나를 옮기고 검사·호스트 결선까지 확인한다.
- **cpppj 전달 항목.** 상태 덤프와 고정 시각 요청은 해결됐다. fixture 헤더·모듈 연결 상태표, `GraphTests` 슬롯 열 범위와 `ClearProcesses` 뒤 정기 조회 진단의 이전 요청은 보관본 1.4에 있다. 변경이 필요하면 사용자에게 전달한다.

<a id="verify-levels"></a><a id="user-checks"></a><a id="dotnet-constraints"></a><a id="07-계속-유효한-결정제약"></a><a id="dotnet-commands"></a><a id="dotnet-ops"></a>

## 5. 검증과 작업 규칙

- **L1** 원본 x86 fixture, **L2** 시작 상태 덤프, **L3** 고정 시계·입력열 뒤 상태 해시, **L4a** 논리 화면 픽셀, **L4b** 소리 시점·곡 선택, **L5** 자연 완주·저장·재실행. 파일 이식·집중 검사·실제 세션 결선·전체 일치를 각각 기록한다. `snapshot`과 `snapshot-output`을 구별하고 실행 근거에 SHA·시드·고정 시각·프레임·스크립트·옵션을 남긴다.
- **사용자 확인.** L5와 전체 화면/청취·MP/LAN·공격형 공중3종 실제 생산/조작은 사용자 확인 범위다. 포획·희생·특정 적 공격·승리는 캠페인1-1~1-5·스커미시에 한해 아군AI 활성화로 테스트할 수 있고 다른 미션은 사용자 확인으로 남긴다(2026-11-22). 창 실행은 AGENTS.md의 명시 확인·예외 PC 규칙을 따른다. Windows 10에서 통과하면 Windows 11 실기와 사용자 관찰만 남은 상태에서도 다음 단계로 진행한다. 물리 DPI·4K FSR·UAC는 R8 이후, `DSERR_NODRIVER` 실장치 재검사는 출력 장치가 있는 PC에서 한다. [소리 장치 기록](docs/dotnet-sound-device-20261114.md) · [공통 실행 범위](LEFT_JOBS.md#rules-verify)
- **공통 기능 경계.** CHEAT 메뉴는 Edit에서만 표시한다. 연녹색 직사각형 분석은 보류다. 폐기된 인터넷 master/root·검증·ping/진단은 비활성 상태를 유지한다. 도움말 링크와 LAN은 해당 사용자가 선택한 경로를 따른다. [공통 계약](LEFT_JOBS.md#rules-verify)
- **오라클과 자료.** `cpppj/build/bin/Release/Netstorm.exe`를 PowerShell에서 빌드한다. 실행은 `extracted/dotnet-oracle-data/` 사본과 분리한 사용자 루트로 진행하며 `originals/` 해시를 전후 확인한다. 0.606의 VM 소프트웨어 GPU 환경은 [재측정 기록](docs/dotnet-l2-0606-20261117.md)에 있다. 일반 덤프 비교와 최초 실행 검증의 옵션을 구별한다.
- **검사 명령.** `dotnet build dotnetpj/Netstorm.sln -c Release`, `dotnet test dotnetpj/Netstorm.sln -c Release`, `python -X utf8 dotnetpj/tools/cpp_sync.py`를 쓴다. 실제 TYPE 검사는 기본 데이터 또는 `NETSTORM_DATA`로 선택한 보호 자료 사본을 읽는다. 2026-11-19 사용자 지시로 정기 전체 회귀는 4단위당 1회만 수행한다(마지막 1.088, 다음 1.092). fixture는 사본 없이 한 소비자 프로젝트에 링크하고 `cpp_sync.py --update --add`로 매니페스트에 등록한다. GUI 명령·변이 검사·등록 규칙 전문은 보관본 6.1에 있다. [L2 도구](docs/dotnet-l2-batch-20261114.md) · [L4a 도구](dotnetpj/tools/l4a_compare.ps1) · [덤프 형식](docs/exe/cpp-session-state-dump-20261109.md)
- **착수·보호·커밋.** AGENTS.md·LEFT_JOBS.md와 이 문서, git status/log를 확인한다. dotnetpj 이식에 디컴파일은 필요 없다. AGENTS.md·기존 원본/CD 파일·README 개요·기존 fixture를 보호한다. 작업 단위마다 0.1절 재개 순서·요약표와 1.1절 체크, 단위 문서를 갱신하고 해당 파일만 `YYYYMMDD-NN DN 요약`으로 로컬 커밋한다. 단위별 완료 문장은 단위 문서에 쓰고 이 문서에는 누적하지 않는다(0.1은 재개 순서와 주제 표이며 단위마다 새 표 행을 만들지 않고 기존 주제 행의 단위 범위만 늘린다. 6절도 범위 행만 갱신하고 단위별 "진행" 글머리를 더하지 않는다. 0.1 재개 순서 1번에도 단위별 완료 문장을 잇지 않는다(1.031~1.076에 쌓인 것을 1.076 정리에서 보관본으로 옮김). 1.1 체크에도 단위별 `[x]` 항목을 새로 만들지 않고 기존 완료 묶음의 범위만 늘리며, 검사 기준선 줄에는 마지막 정기 뒤 단위의 집중 수만 둔다). push 금지. 새 코드·문서는 UTF-8, AGENTS 범위의 코드 주석은 설명·사용법부터 한국어로 적는다. 작업 트리 줄바꿈은 이 문서가 LF, 보관본이 CRLF다(저장소는 둘 다 LF, 2026-11-26 `git ls-files --eol`).

<a id="regression-audit-0640"></a>

### 5.1 0.640 회귀 지연과 실패 점검

`NETSTORM_DATA` 미지정에서도 Assets 배포 자료·Core 기술 표 검사가 기본 데이터 경로를 찾도록 고쳤고, `CanonMayPlaceTests`(약 20분 → 0.64초)와 `FortObjectSaveTests`(약 14~16분 → 0.66초)의 기록 누적 방식을 바꿔 단언을 유지한 채 Release 전체 검사를 프로젝트별 합계 약 7분으로 줄였다. 원인·시간표·TRX 위치·cpppj 대응 코드 비교는 [0.732 보관본](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)의 5.1절에 있다.

<a id="dotnet-done-index"></a><a id="dotnet-unit-records"></a><a id="dotnet-archive-log"></a>

## 6. 완료 기록과 보관

**2026-11-27 정리1381.** [정리/검증 근거](docs/handoff-cleanup-1381-20261127.md). [1.079/중단 기록 전문](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261127-through-1079-checkpoint1080)을 원문 바이트·SHA-256과 함께 보존했다.0.1의 오래된 주제별 상세/선행 완료 나열과6절 완료 연혁은 보관본으로 옮겨 요약했다. ‘미커밋’은 실제1015ca4a6 중간 커밋으로 교정했고 검증 미완료는 유지했다. R5-4c3을 재빌드/대표 L2/픽셀 세 단위로 나누고 DN-MP-08·DN-TUT-1~4를 추가했다. 이번 문서 작업은 DN 제품 완료 단위를 올리지 않는다.

| 마지막 묶음 | 현재 판정 |
| --- | --- |
| 1.034~1.076 | WP·사용자 데이터·DI·SL과 R5 표시/도크 일부, 정기7,379통과·대표3경로 M1 0/34 유지 |
| 1.077~1.079 | 공통 도움말·메뉴 배경 숨김·생산 덱 틀 부착 완료, 집중9건. [1.079](docs/dotnet-canon-1079-20261126.md) |
| 1.080a/b/R5-4c3 | 기존 중간 소스의 Release·집중12/12·동기화 통과, 대표3경로 종료0·DN 0998 바이트 일치. 픽셀·수명은 미완료. [근거](docs/dotnet-canon-1080ab-20261127.md) |
| 1.081/R5-4c3 | 채팅 최초 도킹 교정·실제 미니맵 차이 901/6,000, 집중11/11·대표1경로 덤프 불변. 카메라·수명 미완료. [근거](docs/dotnet-canon-1081-20261127.md) |
| 1.082/R5-4c3 | 카메라 월드 좌표·프레임 갱신 교정, 미니맵 차이 388/6,000·집중13/13·대표1경로 덤프 불변. 월드 알림·수명 미완료. [근거](docs/dotnet-canon-1082-20261127.md) |
| 1.083/R5-4c3 | 미니맵 호스트 종료 반납, 집중10/10·대표3경로 정상 종료/덤프 불변. 월드 알림·미션 전환 수명 미완료. [근거](docs/dotnet-canon-1083-20261127.md) |
| 1.084/R5-4c3 | 공통 저장소 Pop/Unpop 큐 연결·실패 시 반납, 신규1/1·정기7,386통과·대표3 덤프 불변. 세션 전용 알림·전환 수명 미완료. [근거](docs/dotnet-canon-1084-20261127.md) |
| 1.085/R5-4c3 | 같은 물리의 추가 실행기 대여·소유자 안전 반납 경계, 집중10/10·대표1 덤프 불변. 세션 제품 결선은 다음. [근거](docs/dotnet-canon-1085-20261127.md) |
| 1.086/R5-4c3 | 세션 지형·Player Pop/Unpop 알림과 일괄 검증 연결, 집중11/11·대표3 덤프 불변. 실행기 교체·전환 수명 미완료. [근거](docs/dotnet-canon-1086-20261127.md) |
| 1.087/R5-4c3 | 그래프 재결선 지형 실행기 교체 시 알림 대여 이동, 재현 실패 후 집중11/11·대표1 덤프 불변. 다른 재생성·전환 수명 미완료. [근거](docs/dotnet-canon-1087-20261127.md) |
| 1.088/R5-4c3 | 최초 채팅 숨김·지도 영역 연결, 미니맵 1/6,000픽셀 차이·정기7,388통과·대표3 덤프 불변. 채팅 재열기·전환 수명 미완료. [근거](docs/dotnet-canon-1088-20261127.md) |
| 1.106/R5-4c4 기반 | 공통 분리의 Native 부모 통지 분기·대여 반납, Core15/15·대표3 덤프 불변. 실제 세션 대여·중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1106-20261127.md) |
| 1.105/정기 회귀 | 제품 DLL 일치 후 전체 7,401통과·실패0, 음향 하드웨어1 미실행. 새 34경로 수집 없음. [근거](docs/dotnet-canon-1105-20261127.md) |
| 1.104/R5-4c4 기반 | 공유 세션 ContentForm 삭제 분배, Client10/10·대표3 덤프 불변. Native 부모/중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1104-20261127.md) |
| 1.103/R5-4c4 기반 | 장기 내용물 커서·입력·송신·음향 장치와 Form 삭제, Client17/17·대표3 덤프 불변. 세션 분배·부모/중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1103-20261127.md) |
| 1.102/R5-4c4 기반 | 내용물 원격 수입 현재 헤더·실제 내부 송신, Client11/11·대표3 덤프 불변. ContentDevices·중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1102-20261127.md) |
| 1.101/R5-4c4 기반 | 종속 해제기 이후 Gump TYPE 설치 거부, Core15/15·대표3 덤프 불변. ContentDevices·중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1101-20261127.md) |
| 1.100/R5-4c4 기반 | 내용물 수명의 실제 커서·Player 공유 주소, Client45/Core50 통과·대표3 덤프 불변. TYPE 설치 guard·ContentDevices·중첩 통지 미완료. [근거](docs/dotnet-canon-1100-20261127.md) |
| 1.099/정기 회귀 | 제품 DLL 일치 후 전체 7,397통과·실패0, 음향 하드웨어1 미실행. 새 34경로 수집 없음. [근거](docs/dotnet-canon-1099-20261127.md) |
| 1.098/R5-4c4 기반 | 현재 Gump TYPE의 종속 UI 명령을 실제 입력 큐에 연결, Core14/Client13 통과·대표3 덤프 불변. ContentDevices·중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1098-20261127.md) |
| 1.097/R5-4c4 기반 | 제품 PATCH 삭제 가상표 선택과 TYPE 별칭 Form 삭제, 집중4/4·대표3 덤프 불변. 부모 중첩 통지·생산 Form 미완료. [근거](docs/dotnet-canon-1097-20261127.md) |
| 1.096/R5-4c4 기반 | 공유 세션의 GumpForm 삭제 분배와 깊이 확인, 집중4/4·대표3 덤프 불변. 중첩 통지·TYPE 별칭·생산 Form 미완료. [근거](docs/dotnet-canon-1096-20261127.md) |
| 1.095/R5-4c4 기반 | 세션 자원 삭제를 실제 가상 분배에 연결, 집중8/8·대표3 덤프 불변. Gump 분배·생산 Form 미완료. [근거](docs/dotnet-canon-1095-20261127.md) |
| 1.094/R5-4c4 기반 | 장기 GumpForm에 실제 Client TYPE 주소 연결, Core11/Client16 통과·대표3 덤프 불변. Session 삭제 분배·생산 Form 미완료. [근거](docs/dotnet-canon-1094-20261127.md) |
| 1.093/R5-4c4 기반 | GumpForm 현재 TYPE·공장·가상 삭제 본체, 집중10/10·대표3 덤프 불변. Storage/Session/생산 패널 제품 결선 미완료. [근거](docs/dotnet-canon-1093-20261127.md) |
| 1.092/정기 회귀 | 제품 DLL 일치 후 전체 7,392통과·실패0, 음향 하드웨어1 미실행. 새 34경로 수집 없음. [근거](docs/dotnet-canon-1092-20261127.md) |
| 1.091/R5-4c4 일부 | 실제 UI 등록표의 GumpForm 통지·생산 패널 종료 수명, 집중7/7·대표3 덤프 불변. 실제 Form 부착·내용물 미완료. [근거](docs/dotnet-canon-1091-20261127.md) |
| 1.090/R5-4c4 기반 | 장기 GumpForm의 현재 World/UI 통지와 호출 수명 대여, 집중2/2·대표3 덤프 불변. 제품 UI 연결·생산 Form/내용물 미완료. [근거](docs/dotnet-canon-1090-20261127.md) |
| 1.089/R5-4d 일부 | 채팅·구분선 사건 라우터와 재열기 단위 검사, 집중18/18·대표3 덤프 불변. 실제 자동 키 입력 전달 미확인·전체 UberRoot 미완료. [근거](docs/dotnet-canon-1089-20261127.md) |

정기 회귀는4단위마다 수행한다. 1.088은 Assets383·Core4113·Client2892, 합계7,388통과·실패0이다(음향 검사 건너뜀 메시지와 Client의 건너뜀0 집계가 다름). DLL 사본 해시 확인 후 실행했으며 다음 정기 회귀는 1.092다. 1.080은 최초 L2 실패와 1.080a/b 재검증을 구분하며 실제 픽셀·수명 검증을 마쳐야 닫는다. 과거 회귀/65진입/L2 수집 이력은 [정리 전6절](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261127-through-1079-checkpoint1080)을 따른다.

- **이전 이관.** 전체 보관본은 [1.030](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261125-through-1030)(0.1 요약표 1.004~1.030 단위별 행·R4-5 하위 행동 나열·RU와 전송·수신 완료 세부·R4-7 하위 체크·6절 0.969~0.998 나열) · [0.983](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0983)(1.1 전송·수신 단위별 `[x]` 25개·0.1 선행 완료 나열·검사 기준선 집중 수·6절 0.969~0.983 행) · [0.950](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261124-through-0950)(0.1 요약표 0.901~0.950 단위별 행 38개·RU-3 단위별 완료 나열·6절 0.901~0.909 행) · [0.900](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0900)(0.1 재개 인계 연쇄·6절 0.845~0.900 행·R4-2 완료 문장과 1.2 플레어·건설 문장) · [0.837](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261123-through-0837)(0.1 중지 인계·R4-6 완료 bullet 12개·6절 0.798~0.822 행) · [0.797](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0797)(재개 순서·주제별 표·R4-6 완료 bullet 29건) · [0.732](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261121-through-0732)(0.1 단위별 완료 문장 88건) · [0.635](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261119-through-0635)(R4 체크 단위별 문장 22건) · [0.613](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261118-through-0613)(경과·중복 체크·이식량·AGENTS 이력·파일별 목록·명령 예시) · [0.545](LEFT_JOBS.dotnetpj.old.md#archive-dotnet-20261115-through-0545)이다. 각 이관에서 옮긴 것·바로잡은 것의 목록은 바로 다음 보관본의 6절에 있고(0.983 이관 기록은 1.030 보관본, 1.030 이관 기록은 1.076 보관본의 6절), 그 이전은 보관 파일 머리의 날짜별 색인을 본다. 아래는 이전 문서에서 참조하는 앵커다. 같은 이름의 상세는 보관본에 있다.

<a id="018-단위-기록--r0-4-네-번째-묶음-사제-panic기도와-회복-허용-2026-10-28-hjow-athlon"></a><a id="dotnet-current-plan-20261022"></a><a id="legacy-dotnet-031"></a><a id="legacy-dotnet-032"></a><a id="legacy-dotnet-033"></a><a id="legacy-dotnet-038"></a><a id="legacy-dotnet-039"></a><a id="legacy-dotnet-040"></a><a id="legacy-dotnet-041"></a><a id="legacy-dotnet-042"></a><a id="legacy-dotnet-043"></a><a id="legacy-dotnet-044"></a><a id="legacy-dotnet-045"></a><a id="legacy-dotnet-046"></a><a id="legacy-dotnet-047"></a><a id="legacy-dotnet-048"></a><a id="legacy-dotnet-049"></a><a id="legacy-dotnet-050"></a><a id="legacy-dotnet-052"></a><a id="legacy-dotnet-053"></a><a id="legacy-dotnet-055"></a><a id="legacy-dotnet-056"></a><a id="legacy-dotnet-057"></a><a id="legacy-dotnet-058"></a><a id="legacy-dotnet-059"></a><a id="legacy-dotnet-064"></a><a id="legacy-dotnet-065"></a><a id="legacy-dotnet-066"></a><a id="legacy-dotnet-067"></a><a id="legacy-dotnet-068"></a><a id="legacy-dotnet-069"></a><a id="legacy-dotnet-070"></a><a id="legacy-dotnet-071"></a><a id="legacy-dotnet-072"></a><a id="legacy-dotnet-073"></a><a id="legacy-dotnet-074"></a><a id="legacy-dotnet-075"></a><a id="legacy-dotnet-077"></a><a id="legacy-dotnet-078"></a><a id="legacy-dotnet-079"></a><a id="legacy-dotnet-080"></a><a id="legacy-dotnet-082"></a><a id="legacy-dotnet-083"></a><a id="legacy-dotnet-084"></a><a id="legacy-dotnet-088"></a><a id="legacy-dotnet-092"></a><a id="legacy-dotnet-093"></a><a id="legacy-dotnet-094"></a><a id="legacy-dotnet-096"></a><a id="legacy-dotnet-097"></a><a id="legacy-dotnet-098"></a><a id="legacy-dotnet-099"></a><a id="legacy-dotnet-100"></a><a id="legacy-dotnet-101"></a><a id="legacy-dotnet-102"></a><a id="legacy-dotnet-103"></a><a id="legacy-dotnet-104"></a><a id="legacy-dotnet-105"></a><a id="legacy-dotnet-106"></a><a id="legacy-dotnet-107"></a><a id="legacy-dotnet-history"></a><a id="sid-session-plan"></a>
