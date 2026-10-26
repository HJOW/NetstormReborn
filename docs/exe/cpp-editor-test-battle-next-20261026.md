# 다음 단위 준비: Test Battle과 커스텀 시나리오 편집

이 문서는 **읽기 전용 조사 결과이며 Test Battle 구현 완료 보고가 아니다.** 제품/검사 소스가 동결된 467 중에 작성했다. `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 재확인했으며 조사 PC와 마지막 전체 정밀 디컴파일 PC는 모두 **HJOW-Athlon / 2026-10-23**이다. 보호 PE/TAFF·기존 디컴파일·제품 소스를 읽었고 실제 게임 실행, 빌드, 원본/디컴파일 변경은 하지 않았다. 후속 구현·검증·LEFT/커밋은 root가 담당한다.

Test Battle은 작은 입력 분배기만으로 완료할 수 없다. 일반 Editor에는 Guide 미션 없이 실제 World가 살아 있는데, 현재 `CreateMission`/`InitializeMission`/`LoadMissionWorld`는 빈 World를 요구한다. 새 Guide 생성부터 실제 Native State7 교체까지의 수명과, 방금 저장한 작업본 Fort를 다시 읽는 파일 경계를 함께 연결해야 한다. 반면 시나리오의 `SetOwner` 확정은 이미 실제 객체 메뉴·HTML·소유권·CoreData 선택 하위가 있으므로 작게 완성할 수 있는 별도 단위다.

## 근거와 명령 경계

주요 원문은 `extracted/refined/originals/Netstorm.c`다. 필요한 인자/순서는 `originals/Netstorm.exe`의 실제 x86 바이트도 직접 확인했다. 정적 주소 확인을 독립 Native fixture 실행으로 주장하지 않는다.

| 대상 | 10.78 주소/값 | CD 10.72 기록 |
| --- | --- | --- |
| Game 메뉴 작성 | `004cbb80` | 이번 조사에서 대응 미확정 |
| TestBattle 명령 | 전역 `005429fc` = `0x020000c2` | 이번 조사에서 숫자 미확정 |
| 일반 입력 분기 | `004d81d0..004d822a` | 이번 조사에서 분기 미확정 |
| 현재 맵 snapshot | `004bfb50` | 정밀표 대응 없음 |
| Guide.Begin | `00463b70` | `004170f0` |
| 새 현재 Mission의 +0x64 쓰기 | `004639d0` | 정밀표 대응 없음 |
| Guide.Close | `00463b20` → `00463ab0` | `00417340` |
| pending Fort 이름 | `00460bb0` | 기존 FortSelection 문서/구현 참조 |
| nextState1과 Battle14 처리 | `004b3560` | `0042fe00` |
| 실제 미션 공장 | `00482fb0` | `00481020` |
| SetOwner 명령 | 전역 `00542a24` = `0x020000cc` | 이번 조사에서 숫자 미확정 |
| SetOwner 입력 | `004d8176..004d81cb` | 이번 조사에서 분기 미확정 |
| Editor의 변경 후 플레이어 선택 | `004d5290` → `0048f3b0` | 뒤 함수 `00405f40` |

`RawGameMenus::Game()`는 이미 Editor일 때 `Test Battle`에 인자0을 쓰고, Mission이며 현재 +0x64가 비영일 때 `Return to Editing`에 인자1을 쓴다. `GameMenuCommands.cpp`도 c2 숫자를 이미 보유한다. 메뉴 생성 완료와 선택 뒤 게임 전환 완료는 구별해야 한다. 현재 `UberGump::GameMenuCommand`/일반 Event에는 실제 c2 실행 부모가 없다.

## Test Battle 시작: event.y == 0

일반 입력은 Guide/Gump가 소비하지 않은 사건과 공통 release 거부 뒤 도달한다. 선택 숫자는 `event.x`가 아니라 **세 번째 DWORD인 event.y**다. `004d81d8 test eax,eax`의 EAX가 이 값이다.

1. `004d81dc`가 `004bfb50`을 호출한다.
2. snapshot이 정상 반환한 **뒤** `004d81e1`이 현재 Fort 이름 포인터 `0055b490`을 읽는다.
3. 그 이름으로 `00463b70(Guide=00562ca0,name)`을 실행한다.
4. Begin이 반환한 **뒤** `004639d0(Guide,1)`로 새 현재 Mission의 `+0x64=1`을 쓴다.
5. 공통 일반 입력 끝 `004d8bda`로 진행한다.

`004bfb50`은 현재 `00540cac` owner의 Template manager가 있을 때만 동작한다. Battle이면 `004bf880`을 수행하고, 그 뒤 owner를 다시 읽어 Fort 모드이면 `004bf9a0`을 수행한다. Editor에는 `RawCampaignSession::SaveCurrentMap()`와 `EditorSaveGoMain::SnapshotCurrentMap()`이 이미 같은 실제 snapshot/저장 하위에 연결되어 있다. 이름을 snapshot 전에 복사하거나, snapshot 실패 뒤 Guide 생성/모드 쓰기를 계속하면 안 된다.

`00463b70`은 기존 현재 Mission이 있을 때만 가상 `+0x1c`와 해제를 한 번 수행하고, 그 뒤 Guide 이름을 복사해 공장을 호출한다. 원문은 Editor World가 있다는 이유로 여기에서 `004b2df0`을 미리 호출하지 않는다. `BeginGuideMission()`의 기존 현재/finish/release/이름복사 순서를 재사용할 수 있다.

`004639d0`은 `Guide+0x800`의 현재 포인터를 읽고 바로 `+0x64`를 쓴다. 원문에 NULL guard나 demo 변경은 없다. 현재 `MissionRuntime::mode64`와 `MissionTransitionEffects().mode`가 이 필드의 실제 소비자다. `BeginMission(name,0,1)`처럼 생성자 인자만 전달하면 결과값은 같을 수 있지만, **공장 정상 반환 뒤 쓰는 시점**과 실패/재진입 경계는 별도로 검증해야 한다. gate.demo는 원래 일반 시나리오 값으로 남긴다.

## Return to Editing: event.y != 0

원문은 숫자1만 검사하지 않고 모든 비영 DWORD에 같은 경로를 선택한다.

1. `004d8207` Guide.Close를 실행한다. 현재 Mission이 없으면 이 함수는 아무 효과 없이 반환한다.
2. close가 반환한 **뒤** `004d820c`가 현재 Fort 이름을 다시 읽는다.
3. `00460bb0(currentName)`으로 pending 이름을 쓴다.
4. `005cae84=2`로 Fort 목록/설치 종류를 선택한다.
5. `004b3560`을 실행한다.

`004b3560`은 먼저 실제 `005c8878=1`(nextState1)을 쓴다. 이어 현재 Battle 비영이고 `005c8758==14`일 때만 현재 Template load·header bit 정리·금액/상태 보존의 기존 하위를 실행한다. `FortSelection` 계열에 이 큰 부모의 일부 계약이 있으나, 현재 Uber의 `FortSelectionEvent`는 Battle14 파일 보존 장치가 미연결임을 명시하고 있다. next1 대입만 별도로 복제해서 성공 처리하면 안 된다.

그 뒤에는 이미 복원한 State1의 cleanup → prefix/Editor·로컬 양끝 → pending/current 이름/실제 작업본 설치 → HUD/플레이어 → 공통 확정 → 다음 Poll4를 자연 소비해야 한다. `ClearWorld()` 후 호스트 Editor 화면을 직접 여는 것으로 복귀를 대신하지 않는다. 현재 실제 Session/생산 Form/HTML/Guide의 수명 및 native inMission0 쓰기를 원문 위치에 남겨야 한다.

## 실제 미션 공장과 작업본 파일

`00482fb0`은 이름/추가 설정을 구분해서 새 `mission` Config를 만들고, 추가 설정 → `missionSpec(name)` 파일 Load → `mission.loadFort` 조회를 수행한다. loadFort의 첫 글자가0이면 미션 이름을 사용한다. 이어 `00460650` FortPath와 `0041ab10` 파일 존재를 확인하고 `mission.missionType` 문자열을 실제 등록 factory 표에서 대소문자 무시로 찾는다. 공장이 반환한 객체의 `+4`에 이름, `+0x5c`에 Config를 넘긴다.

파일/형식이 없으면 먼저 `Tell("%sMissing(%d)",name,inBattle)`를 시도하고, Tell이0이면 `MissingMission(name)`을 시도한 뒤 Config를 해제하고0을 반환한다. 임의 Tutorial/Battle·빈 스크립트를 만들어 성공으로 바꾸지 않는다. 이 함수의 실제 실패 안내도 현재 호스트 `!Loaded()`/미지원 형식 예외와 별도의 남은 동일성 범위다.

현재 재사용점과 미연결 경계는 다음과 같다.

| 현재 코드 | 이미 있는 실제 하위 | 다음 연결에서 필요한 점 |
| --- | --- | --- |
| `Mission.cpp::MissionScript` | Config 등록, extraSettings, missionSpec, 현재 언어, loadFort/type/섹션 | 공장 이름 파싱·파일 존재·등록 factory·실패 안내까지 전부 복원되었다고 간주하지 않는다. |
| `UberGump::CreateMission` | Runtime/현재 Script, mode64/demo, Tutorial 또는 Battle 선택 | 살아 있는 Editor World 때문에 시작 시 throw한다. 공장과 State 교체 수명을 분리해야 한다. |
| `InitializeMission`/`TutorialInit` | 실제 Template clear, Header, loadFort 요청, 옵션/미션 전역 | 현재 World 없는 계약이 있다. 원문 Template clear가 살아 있는 Editor Session 참조에 미치는 위치까지 해결해야 한다. |
| `MissionFortRequest` | 실제 next[0]=7, 주소/포트/Zone 복제, 현재 이름 선택 | 이미 만든 같은 요청을 Native State7에서 소비해야 한다. 다른 State나 새 가짜 Zone으로 바꾸지 않는다. |
| `LoadMissionWorld` | 실제 Fort 파싱·Player·Session·Start/Guide 설치 | 현재 빈 World 계약과 자산 VFS 직접 Read를 Editor 출발에도 맞게 이어야 한다. |
| `EditorFortFileDevices`/`EditorFiles` | Config 경로/고정 FileSpec, 기존 작업본 보존, 실제 작업 namespace 읽기/쓰기 | TestBattle 이후 editor 플래그가0이어도 방금 저장한 같은 작업본을 읽는 **출발 수명**을 유지한다. |

특히 현재 `ClientMain.cpp`의 Config reader는 `files_.TryRead(path)`이고 `LoadMissionWorld`도 `client_.Files().Read(fortPath)`를 직접 사용한다. Editor가 저장한 형제 `*-editor/d/<name>.fort`를 우선하지 않으면 TestBattle이 오래된 원본 Fort를 읽거나 Save as 새 이름을 찾지 못한다. `editor==1` 여부만 파일 reader 선택 조건으로 쓰면 State7 cleanup 이후 연결이 끊길 수 있다.

권장 최소 파일 계약은 TestBattle 출발 시점의 작업본 장치/선택 Fort를 실제 요청 수명에 빌려 주고, 그 요청이 고른 Fort의 읽기에서 같은 `EditorFiles::ResolvePath`를 사용하도록 하는 것이다. 현재 EditorFiles의 절대 경로 VFS 읽기는 `EditorFortFileDevices::Read`가 이미 수행한다. Config의 `DataDir`, `fortSpec`, `missionSpec`를 전역으로 바꾸거나 보호 원본에 저장하지 않는다. 정상 캠페인 reader에는 작업본을 새로 끼우지 않는다.

언어 파일은 별도 검토가 필요하다. AGENTS의 계약은 `.fort`를 Edit에서 수정하고 `.english`는 사용자가 텍스트 편집기로 직접 작성한다는 것이다. 원본 `TEST01.english`는 `[Header] missionType="Tutorial"`, myStartMoney/myTech, ai2/ai3의 이름·SP·기술·동맹·능력, `[A.]` HTML, 승패 섹션과 MissionBegin 등을 실제 포함한다. 새 Save as는 Fort 이름 변경/저장이지 같은 이름의 `.english` 자동 생성/복사는 아니다. 작업본 이름만 새로 만들었을 때 언어 파일이 없는 실패도 원문 factory 규칙에 맞춰 드러나야 한다.

따라서 사용자 시나리오 파일을 어디에 놓는지와 reader 우선순위는 명시적으로 정의해야 한다. 최소 범위는 현재 자산 VFS의 같은 언어 스크립트를 읽되 Fort만 작업본을 사용하는 TEST01 재시험이다. 작업 namespace에서 직접 작성한 새 `.english`도 지원하려면 같은 TestBattle 수명에 제한된 script reader를 추가하고 원래 Config registry/언어 치환을 재사용한다. 원문에 없는 인게임 시나리오 텍스트 작성 UI를 만들 필요는 없다.

## 실제 시나리오 편집 명령: SetOwner

보호 `originals/netstorm.tarc`의 `\\d\\tell.english`에서 `[SetOwner]`를 직접 읽었다. Human Player→1, Computer #1..#7→2..8, No Owner→0, Cancel→DoNothing이다. 공통 객체 메뉴 `004b1270`은 Editor 또는 special 모드에서 `Tell("SetOwner")`를 추가한다. 현재 `RawObjectMenu::Open`과 실제 공통 HTML은 이 메뉴/창까지 재사용할 수 있다.

명령 이름 표의 `005431e8`은 문자열 `00512eac="SetOwner"`와 전역 `00542a24`를 짝지으며, PE의 초기 값은 **`0x020000cc`**다. 현재 `GameMenuScriptCode`/일반 Event/`CampaignObjectMenuClient::Command`에는 이 이름/cc 실행이 없다. 객체 메뉴가 보인다는 사실만으로 소유자 확정 완료라고 평가하지 않는다.

`004d8176..004d81cb`의 정확한 순서는 다음과 같다.

1. 현재 Editor가 비영이거나, 그 뒤 읽은 missionPost `00594fa4`가 비영일 때만 몸체를 수행한다.
2. 현재 cursor SID `005caea0`을 ESI에 캡처한다.
3. SID가 비영이면 같은 풀 슬롯의 가상 `+0x74(event.y)`를 실행한다. SID BYTE를 직접 쓰면 안 된다. 신전의 섬/지역/생산 효과와 사제의 원래 owner 처리를 유지해야 한다.
4. 가상 호출 후 Editor를 **다시** 읽고 비영이면 캡처한 SID로 `004d5290`을 실행한다. 호출 중 current cursor가 바뀌어도 이 SID를 바꾸지 않는다.
5. 마지막에 worldRedraw `0059a8b0`을1 증가한다. 처음 SID가0이어도 이 마지막 증가는 수행한다.

`004d5290(sid)`는 변경 후 실제 SID+0x22 owner BYTE를 읽는다. owner0이면 효과가 없고, 비영이면 **`0048f3b0(owner,1)`**를 실행한 뒤 worldRedraw를 추가1 증가한다. 이는 기술 지급이 아니라 현재 로컬 owner/Template owner와 CoreData alias를 고르는 실제 부모다. 현재 `ClientCoreDataGlobals::Select(player,metamap,owner,1)`과 `RawPlayerCoreData`가 정확한 재사용점이다. Editor에서 다른 Computer 소유자로 바꾸면 현재 플레이어 선택도 그 owner로 바뀐다는 점이 중요하다.

소유권은 현재 `RawCampaignOwnership::SetOwner`가 신전 `RawVortexOwner`와 나머지 실제 사제/common owner 분배를 이미 가진다. Session 내부 콜백에서도 같은 몸체를 사용한다. 새 공개 Session API는 이 몸체를 빌리고 현재 풀/공통 전역을 사용하면 된다. `Core().owner.Set`를 모든 TYPE에 그대로 호출하지 않는다. no-owner0에서의 현재 로컬 선택 유지, TemplateOwner=1, 같은 CoreData 포인터/known 상태를 함께 검사한다.

`CHEAT Destroy`/Editor `Destroy`의 `0x03000015`, `Make Yours`의 `0x030000af`, Hurt의 `0x030000e2`는 기존 공통 객체 명령 경로가 있다. 이들의 존재를 cc SetOwner 확정으로 대체하지 않는다. 개별 다리 Ctrl 입력은 원문 Editor cursor/모드 갱신 `004d5bb0`·`004d5c90`과 연결된 별도 범위이며 이번 조사로 전체 동일성을 확정하지 않았다. 전체 다리0/1/2는 이미 완료한 `RawBridgeEditor`를 재사용한다.

`Change Description...`은 원문 `004789b0`의 메타맵 전투 설명 메뉴와 전역 `00542ac0=0x020000f3`, 대화창 `004d3550/004d37a0`다. 인게임 `.english` 시나리오 편집기를 뜻하는 근거가 아니다. Multiplayer 제외 범위 판단을 할 때 이 메뉴를 커스텀 미션 스크립트 작성의 필수 부모로 오인하지 않는다.

## Local AI 테스트 옵션과 구별

[기존 Local AI 다음 문서](cpp-test-local-ai-integration-next-20261026.md)의 `--test-local-ai original`/SP/tech는 아직 제안이며 이번 조사 시점 제품에 없다. 공통 AI 생성·실제 프레임 루프는 이미 연결되어 있다. 일반 Start는 로컬을 건너뛰고 사제 있는 다른 owner의 AI를 만들며, demo에서만 원래 로컬도 생성한다.

Editor TestBattle은 위 c2/Guide/State 경로와 시나리오 Header의 실제 `my*`/`ai*`를 사용하는 일반 미션이다. 로컬 테스트 AI/지원 SP를 켜거나 gate.demo를1로 바꿔서 TestBattle 경로 성공을 대신하지 않는다. 추후 별도 명시 Local AI 옵션을 넣더라도 native Start 정상 반환 뒤의 독립 로컬 프로필/현재 owner/중복 방지 경계는 기존 문서대로 유지하고 결과를 별도로 표시한다.

원본 설정명 `testBattleAddress`/`tryToJoinTestBattle`도 이 c2와 다르다. `004359f0`은 tryToJoinTestBattle로 전역 `005427a4` 사건을 예약하며, 일반 입력은 `004b3620` 뒤 **`004b8be0` 네트워크 test battle 접속**으로 간다. 이 함수는 특정 개발자 이름을 검사하고 `testBattleAddress`를 복사해 phase29 접속을 요청한다. 커스텀 Editor TestBattle나 로컬 자동 AI 구현에 이 경로를 사용하지 않는다.

## 최소 완성 단위와 후속 검증

1. **독립 c2 부모**: 현재 이름은 getter로 각 원문 위치에서 읽고, save/Begin/mode 및 close/pending/mode2/requestState1 순서를 명시 효과로 관찰한다. event.y0/1/2/ffffffff, release, Guide/Gump 소비, snapshot/close 실패, 콜백 중 현재 이름/Guide 변경을 포함한다. 새 Native fixture는 실제 `004d81d0..822a`를 실행하고 큰 외부 하위만 명시 장치로 기록한다.
2. **SetOwner 작은 단위**: cc 이름/숫자, 실제 cursor SID/TYPE 가상 owner 분배, 후행 Editor 재조회, 캡처SID의 owner 읽기, 현재 CoreData 선택과 redraw1/2회를 연결한다. SID0, owner0/1..8, 가상 호출 중 cursor/Editor/owner 변경, 현재 Core alias, 사제와 신전의 파생 효과를 원본 및 실제 Session에서 검사한다. 기존 객체 메뉴/HTML을 그대로 사용한다.
3. **Editor→Mission 수명**: 실제 Guide 생성 때 Editor World를 유지할 위치와 Init의 Template clear를 확정하고, 요청한 같은 next7/Zone/이름을 원문 전체 State7 cleanup/설치로 이어 준다. 단순 `ClearWorld()` 선행이나 현재 guard 제거만으로 완료 처리하지 않는다. 기존 캠페인/재시작/Guide 교체/Native State1·3 집중 검사를 보존한다.
4. **작업본 실제 재로드**: TEST01 작업본을 수정/저장한 뒤 TestBattle에서 원본과 다른 작업본 바이트가 실제 로드되는지, 새 이름의 missing script가 원문 안내로 끝나는지, 원래 `mission.loadFort`가 다른 파일을 고르는지 확인한다. 보호 자산 SHA, 작업본 저장/언어 파일 reader 수명, Template 원본 이미지 및 실제 객체/Player 상태를 대조한다.
5. **자연 GUI 왕복**: 메인 Edit→TEST01→소유자 변경/기존 도구→Game/Test Battle→브리핑/Go/실제 local SP·기술·적 AI→ESC/Game/Return to Editing→같은 이름/내용/현재 owner·HUD 복귀를 실제 버튼/공통 입력으로 확인한다. State7/State1 직접 호출은 GUI 통과로 합산하지 않는다.
6. **완료 경계**: 독립 입력 fixture, 실제 Session, 자연 GUI, 원본 보호 및 기존 검사 집합을 각각 기록한다. TestBattle 시작만 통과한 결과로 복귀·새 시나리오·전체 Edit 또는 전체 미션 플레이 동일성을 완료 처리하지 않는다.

## Save as 일반 사건 계약 정정

[Save as 최초 조사](cpp-editor-save-as-next-20261026.md)의 메뉴 예시는 fortMode1/canCancel1이지만, 일반 사건 자체는 `004d744e push EDI(0); push1; push EAX(event.y); call004d3490`이다. 따라서 정확한 계약은 **`Show(fortMode=event.y,canCancel=1,initialName=0)`**이며 ownerMode.fort를 첫 인자에 대신 넣거나 event.y를 canCancel로 쓰지 않는다. 원본 Edit 메뉴가 y1을 발행하므로 정상 Save as는 `(1,1,0)`이 된다. NewFort/미션의 강제 이름입력 호출 계약은 이 일반 사건과 별도로 유지한다.
