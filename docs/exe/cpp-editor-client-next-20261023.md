# Edit의 Client 연결·파일명·이름 입력 후속 설계

2026-10-23 단위377 뒤 **제안·미구현** 자료다. AGENTS와 공통 최신 상단/닷넷 인계를 재독했고 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 기존 정밀 C와 보호 PE를 읽고 필요한 명령만 정적으로 역어셈블했다. 새 디컴파일·게임·GUI·빌드·검사·git은 실행하지 않았고 이 신규 문서 외 파일은 수정하지 않았다. 단위377의2442검사/실패0은 root가 보고한 전체검사 결과이며 이후 진입 GUI는 진행 중이다. [Objects 통합](cpp-fort-objects-runtime-20261023.md)의 부분 저장을 전체 Edit 완료로 확대하지 않는다.

`genericEditor`는 일반 Fort/Editor 설치를 구별하는 설명용 이름이며 원본에서 확인한 함수명·설정 키가 아니다. snapshot/Money/Mission/Badges/005c89c0 공유는 별도 [snapshot 후속 설계](cpp-editor-snapshot-next-20261023.md)가 상세를 소유한다. 이 문서는 실제 Client 연결과 이름/파일 수명에 집중한다.

## 현재 경로와 재사용 API

| 단계 | 현재 구현 | 남은 실제 연결 |
|---|---|---|
| 메인 Edit/목록 선택 | 돌 버튼, Tell, `TellMapMenus`, `SoloMapCatalog`, `FortSelection` | `UberGump.cpp:1093` Edit는 `Tell,NotImplemented`다. `DialogScriptCode:58`/`Supported:143`/`Execute:986`에 c9/ca가 없다. |
| Editor 설치 | 같은 Session의 Pool/Factory/Player/TYPE/지도, `RawFortWorldLoad`/`RawFortWorldLoadBinding` | `RawCampaignWorldRuntime.cpp:61` 일반 Money/Badges/Technology와 alternate populate가 명시 미연결이다. `Load:89`는 캠페인 부모만 호출한다. 생성 전 모드 입력이 필요하다. |
| 저장 | 실제 TypeNames/Technology/Objects/Territory/Deck writer, Storage의 stream/파일 writer, `EditorFiles` | 전체004bf9a0/장부·Notify/BindFileSave·현재 manager 파일명 setter를 Client에 연결해야 한다. |
| 이름 입력 | `TextEntryBuffer`/`TextEntryGump` 신규 모듈, 공통 Gump/문자 입력 | 전용004d3490/004d3250 부모·제출/취소는 없다. 위 TextEntry 모듈도 현재 제품 CMake/Client 미등록이다. Tell `$Input`과 다른 부모다. |
| Test Battle/Return | `MissionScript`, Guide 교체 부모, Tutorial/Battle 공장 | c2/cb 명령·미션+64 편집 표식·Editor 수명/namespace가 미연결이다. |

`OpenGameMenu`는 아직 `missionRuntime_ && phase==Mission`을 요구한다([UberGump.cpp:637](../../cpppj/src/client/UberGump.cpp)). 미션 객체가 없는 Editor에서도 같은 월드/메뉴/입력·커서·덱·미니맵을 사용할 수 있어야 한다. [WindowTitle.cpp:7](../../cpppj/src/client/WindowTitle.cpp)의 Editor/testBattle 분기는 이미 재사용 가능하지만 현재 UberGump는 미션 정보만 공급한다.

## 원본 상태·현재 이름의 계약

주소는 `extracted/refined/originals/Netstorm.c`와10.78 PE SHA256 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`를 기준으로 했다. C에서 인자가 빠진 명령은 PE push/call을 다시 확인했다.

| 대상 | 원본 주소/순서 | Client에 필요한 수명 |
|---|---|---|
| 이름 | 사용자00540ca8, 현재 Fort0055b490, pending0055b494 | Session 밖 Client 소유다. 초기 사용자 이름은 빈 문자열이며 기존 Fort Subscriber/새 Fort Unnamed/Editor Player로 갱신된다. 임의 userName 설정 키로 대체하지 않는다. |
| AskEditFort(c9) | 004d7408: mode005cae84=2 → PathSpec(lastEditFort) → pending00460bb0 → Refresh2 → Tell LoadEditable | current/제목/dirty/next/월드는 이 명령에서 바꾸지 않는다. |
| ReadFort(ca) | 004d745f: **event.y** 문자열 → pending → 현재mode 재조회 →0이면 next6(004b3550), 그 외 next1(004b3560) | 같은80칸 문자열 고리/40칸 입력큐와 기존 `FortSelection`을 연결한다. 선택 중 즉시 월드를 설치하지 않는다. |
| next1 | 004b3560: 먼저005c8878=1 → inBattle&&currentState14일 때 현재 Fort 재읽기 →State flags&~2 →현재시각 `_ftol` 하위DWORD를+c에 기록 →현재 owner섬<1이면flags&~4/Notify | callback 뒤 현재 slot/owner/State 재조회. 기존 대기 요청과 무관하게 덮어쓴다. `RequestFortEdit`가 이 본체를 보존한다. |
| next 소비 | 004b6dd0: 요청/부가자료를 작업·이전 기록에 복사 →next/작업 요청0 →State 분배 | 원본 next DWORD를 [State::Post](../../cpppj/src/client/State.cpp)의 선착순 무시 큐로 바꾸지 않는다. UI 처리와 실제 자원 교체 위치를 분리한다. |

입력 부모004d62b0의 Guide→공통 Gump 소비가 먼저다. 이후 release80000000을 거부하고 `code&03ffffff`로 비교하며 c9/ca에 Control/Shift 추가금지는 없다. [FortSelection.h](../../cpppj/src/client/FortSelection.h)의 실제 참조/getter/hook을 재사용한다.

**현재 이름 setter00460980**(`Netstorm.c:78378`)은 다음 순서를 가진다.

1. 현재 manager가 있으면 기존 current의 fortSpec 경로를256BYTE에 보관한다. 새 이름 점 진단 뒤, 현재 manager가 있고 헤더+8이0이면004bfac0을 호출한다.
2. current 문자열을 복사한다. 현재 manager가 있으면 그때의 헤더+8을 보관하고0으로 쓴다. 새 current 경로를004bf630에 전달한다.
3. **004bf630은 동일 manager+c 파일명 복사→004bd360 Notify**다. 이어00460980이 다시004bfac0을 호출한다.
4. 보관한+8이0이고 기존/새 경로의 대소문자 무시 비교가 다르면0041aef0에 이전 파일 삭제를 요청한다. 이후 제목00460730→목록dirty00542678증가→원본 조건의 lastEditFort/lastMultiplayerFort 설정이다.

004bfac0은 `if(battle) Battle snapshot; if(fort) Fort snapshot`의 두 독립 if다. 004bfb50은 최초 현재 manager 존재 관문만 더한다. 두 모드를 else-if로 합치거나 callback 전 slot을 공통 캐시하지 않는다. 파일명 setter 자체 Notify와 이어지는 snapshot Notify를 한 번으로 줄이지 않는다.

Storage에는 현재 `Name(slot)` getter만 있다. 후속에 **같은 manager 파일명 setter004bf630**를 추가하고 실제 Notify를 연결한다. `InstallFile`은 manager/stream을 반납·교체하므로 이름 변경에 쓰면 안 된다. `RenameSection`은 섹션 노드 이름으로 별개다. 원본 Save as는 writable 이전 파일의 경로가 달라지면 삭제 요청까지 하므로 이미 분리한 사용자 작업 Fort 안에서 이를 수행해야 한다.

## 일반 Editor와 신규 Fort 설치

004b6dd0 State1/2(`Netstorm.c:145526`, 주요145735~145805)는 공통 해제004b2df0 뒤 **inFortMode00594fb8=1, Editor005c85a4=1, missionPost00594fa4=1, fullDeck0054db14=1**을 첫 객체 생성 전에 설정한다. 캠페인 Load 후 Editor bool 하나만 켜는 방식으로 대신하지 않는다.

원본 설치는 current/pending 비교·필요한 Guide 종료→current setter→빈/점 이름이면 newFort 설정 이름→004b5ca0→Player00490250→CoreData읽기004beca0→회복Territory004b5ec0→일반월드004b5f70→Deck읽기004bf190→사제004923e0→Camera004340d0→생산UI0047d890→CoreData쓰기004beb30→**전체snapshot004bf9a0**→State bit0해제/Notify→Editor이면 lastEditFort/004904b0 순서다. Editor 가드의 CoreData/사제 자연 생략이 전체snapshot 생략을 뜻하지 않는다.

- **기존 Fort:**004b5ca0은 실제 파일004bf550/Subscriber를 읽는다. 004b5f70(alternate0)은 같은 Builder/Generate 뒤 Objects→Money004bf340→Badges004be8e0→Technology004beee0→Graph(false)다. 캠페인 전용 부모의 child 생략으로 대체하지 않는다.
- **신규 Fort:**004bfba0으로 실제 manager 생성→State bit0/Notify→004b3920→alternate1이다. 004bfba0은 대상이 이미 있으면 삭제하는 경계도 있으므로 보호 경로를 넘기지 않는다. alternate1은 저장 Objects/Money/Badges/Technology를 건너뛰고 섬0이 있으면 실제 populate를 선택한다.
- **004b3920:**subscriber=-1→번역 Unnamed/handle/config/title→등록ID→20×6 Territory 초기자료→현재 SP 제거/Player기술 초기화→CoreData→공유난수/비영WORD→Subscriber/Territory/Deck→State+11 kind→Notify다. Editor에서는 기본 첫 섬 생성이 생략된다. 임의 빈 Fort 이미지/고정 난수로 대신하지 않는다.
- **004904b0:**Human Player/Player 이름, owner2..8 Computer 이름/flags와 기술을 설정한다. 타입0부터 매번 현재count를 읽고 group!=10&&theme!=5마다 직접 `ContainedCreate(parent,type,0,4,0x10,0)`한다. 기존 테마 기술의 Find/Ensure/중복회피와 다르다.

제안 API는 Session 생성 입력의 `SessionInstallMode` 또는 별도 `LoadEditorFort`다. 기존 [RawFortWorldLoad](../../cpppj/src/o/RawFortWorldLoad.h)/[binding](../../cpppj/src/client/RawFortWorldLoadBinding.h), Player/지도/Graph/Factory와 실제UI를 재사용하고 일반 부모를 별도 진입에서 호출한다. 미연결 Money/Badges/Technology/populate를 성공 callback으로 채우지 않는다.

**005c89c0은 Client 수명 한 DWORD여야 한다.** manager 생성자004bcc60의0쓰기, 메인 Storage, SoloMapCatalog 임시 Storage, Session `objectState.version`이 직접 같은 객체를 참조해야 한다. 현재 각각 분리되어 있다. 버전 초기/최종 값 복사로 alias를 가장하지 않는다. [snapshot 설계](cpp-editor-snapshot-next-20261023.md)의 주입안을 먼저 적용한 뒤 신규manager 생성을 연결한다.

## NewFort79의 전용 입력 Gump

PE **004d7446~004d745a**는 NewFort02000079(005428dc)에서 `004d3490(event.x,1,event.y)`를 호출한다. 생성자004d3250은 첫 인자를+d4, 두 번째를+d8에 보관하며 마지막0이면 현재 Fort 이름을 초기문자열로 쓴다. Save as 메뉴는 인자1, Create New Map는0이다. `tell.english:679~681`의 Create는 Tell LoadEditable 뒤 NewFort0을 예약한다.

004d3250은 실제 TextEntry00452430, 제외문자00542680, **maxLength0x28(40)**, 단일줄+f4=0, 확인/취소 버튼을 구성한다.004d3490은 화면중앙부착/004652b0/00466600이다. 기존 TextEntry child를 사용한 **전용 부모**를 구현한다. Tell `$Input`을 먼저 완성해야 한다는 의존성은 없다.

제출004cf9a0은 전체문자256BYTE복사→첫 LF에서 자름→C로케일 alnum/밑줄/공백 밖 문자를 공백으로 변경→비어있지 않으면 fortSpec→004dd560(GetFileAttributesA)이며 **기존 파일/디렉터리가 있으면 NameRequired/실패**다.

- +d4 비영(Save as):현재 manager가 있으면 헤더+8=0→00460980(입력이름)→성공1.
- +d4=0(NewFort):80칸 고리004c7910에 저장→**ReadFort를 x/y 모두 같은 문자열 토큰으로 예약**→성공1. 입력 callback에서 Session을 직접 교체하지 않는다.
- 취소004cfab0:+d8=0이면 NameRequired/실패, 비영이면 성공. 일반 NewFort는+d8=1이므로 취소 가능하다. 빈이름에 임의 기본명을 자동 제출하거나 기존파일을 덮어쓰지 않는다.

## Test Battle·Return·Main Menu

| 명령 | PE 실제 순서 | 필요한 Client 연결 |
|---|---|---|
| Test Battle(c2,0) | **004d81d0**:004bfb50→그 뒤 현재 current Fort 이름으로 Guide Begin00463b70→새 미션+64=1(004639d0) | 실제 snapshot/Notify 뒤 기존 Guide finish/release/create를 사용한다. +64를 현재 MissionRuntime에 소유하고 메뉴/제목이 같은 값을 읽는다. |
| Return(c2,1) | **004d8202**:Guide Kill00463b20→그 뒤 현재 current 이름을 pending00460bb0→mode2→004b3560 | Guide 종료 전 이름을 고정하지 않는다. 테스트 중 변경을 편집 원본으로 자동 snapshot하지 않는다. next1의 전투/State14 분기는 그대로 보존한다. |
| SaveGoMain(cb) | **004d815b**:event.x비영이면004bfb50→004b3540(next3) | LeaveEditMode의 Yes1/No0/Cancel DoNothing(tell.english:699~706). No도 저장하게 합치지 않는다. |

[RawGameMenus.cpp:49](../../cpppj/src/client/RawGameMenus.cpp)는 Test0/Return1과 +64 조건을 이미 보존한다. 원본 Mission 공장00482fb0(`Netstorm.c:105648`)은 missionSpec/loadFort 파일 존재와 missionType 등록을 검사하고 실패하면 `<name>Missing(inBattle)`/MissingMission 안내 후0이다. 현재 `UberGump::CreateMission`은 기존 월드 부재를 요구하고 누락/미지원 형식을 예외로 보고한다. **Editor의 Guide release/create/실패수명**을 추가해야 하며 c2의 Begin 뒤+64 접근 실패도 독립 검증해야 한다. 누락파일을 기본 Battle로 만들어 성공으로 가장하지 않는다.

원본 이름 부모에는 `.english` 자동 생성·복사·이름변경이 없다. 시나리오는 AGENTS대로 사용자가 텍스트 편집기로 작성한다. Save as 뒤 동명 스크립트가 없는 경우도 실제 실패 경로로 검증한다. 별도 GUI 스크립트 IDE는 이번 복원 의존성이 아니다.

## 작업 namespace와 보호 경계

Client가 장기 `EditorContext`(가칭)에 current/pending/user/mode/currentState/next, 공유 Template9/serialization globals, `EditorFiles`, 편집 VFS/목록 캐시를 소유하는 안이다. Session은 실제 현재 전역을 빌린다. 파일 장치는 `BindFileSave` callback보다 오래 살아야 한다.

`EditorFiles`는 이미 Prepare의 **기존 파일 유지**, canonical 보호루트/작업루트 검사, 쓰기·권한변경 직전 재검사를 제공한다. 읽기/목록/Test Battle에도 같은 작업 namespace를 적용해야 한다. [BaseFileSystem](../../cpppj/src/o/BaseFile.h)은 기본디스크→archive→보조디스크로 읽지만 `MatchDisk`는 기본디스크만 열거한다. writer만 작업본에 연결하면 다음 목록/재로드가 원본을 읽는다.

제안 resolver는 `FortPath(name)`, `ScenarioPath(name,language)`, `Exists(path)`, `DeleteWorkFile(path)`다. 원본 PathSpec 계산을 재사용하고 **Fort/시나리오 파일장치 경계만** 안전한 실제 작업 경로로 매핑한다. DataDir/InstallDir 전역을 바꾸어 자산·설정쓰기를 함께 이동하지 않는다. manager.fileName은 같은 실제 쓰기 경로, current/pending은 확장자 없는 원본 이름이다.

초기 작업 디렉터리는 편집 대상 디스크 Fort/대응 언어파일을 **없는 파일만 원시 복사**해 준비한다. 기존 작업본은 원문과 달라도 유지하고 원본 hard link를 쓰지 않는다. 목록은 하나의 실제 작업디스크에서 원본50개 상한/OS열거/필터/캐시를 실행한다. 두 목록을 임의 정렬·합산하지 않는다. bootstrap 대상/개수/실패를 기록한다. 설치는 Prepare 결과의 **현재 작업바이트**를 InstallFile하고, loadFort alias도 같은 resolver를 거친다.

새로운 삭제/Exists/파일명 setter 장치도 기존 Checked를 공유하도록 확장한다. 원본00460980/004bfba0이 요구한 삭제는 **동작 직전 작업루트 안·보호 originals/originalCD 밖**으로 정규화한 경로에만 수행한다. Save as의 이전 Fort 외 언어파일은 자동 이동/삭제하지 않는다. 실패한 쓰기를 성공 안내/기존 파일 삭제로 이어가지 않는다. Template+8/+16, inMission, readOnly/hidden/currentOwner 저장 관문은 유지한다.

## 첫 구현 단위와 실제 사용자 검증

**첫 Client 단위는 persistent Fort 상태/경로/현재 이름 부모**로 자른다. 원본 next DWORD/소비 경계와 c9/ca 실제큐를 연결하고, 동일 manager 파일명 setter004bf630/current setter00460980/안전한 Exists·삭제 장치를 복원한다. 005c89c0 alias를 함께 닫는다. 전체snapshot이 아직 없으면 명시 필수 hook으로 두고 도달 시 실패시키며 메인 Edit를 성공 메뉴로 열지 않는다.

이 단위의 검사는 독립 PE의 이름변경 trace/slot·모드 callback 변경/두 번 snapshot·Notify 순서와, 임시 작업디렉터리의 실제 Storage 파일명변경·중복명거부·기존작업본유지·보호경로/원본해시 불변이다. 다음 단위에서 snapshot/장부가 준비되면 **기존 Fort 일반Editor→현재변경→실제저장→동일작업본재로드**까지 메인 Edit/phase/메뉴/제목/HUD를 함께 켠다. 그 뒤 신규alternate1/EditorPlayer/이름Gump/Save as/Test Battle·Return/Add Island·다리품질 순서로 진행한다.

향후 GUI 합격 기준은 다음과 같다. **이번 문서 작업에서 실행하지 않았다.** 각 실행의 최종 제품 SHA/원본·CD해시/작업파일목록과 최초실패·재실행을 구분한다.

1. 기존 작업맵에서 서로 다른 두Terr/Chaff 객체와 지형·덱·기술을 두 차례 변경/저장하고 Main Menu·재실행 후 실제월드에 보존됨을 확인한다. 부분 Objects만으로 전체snapshot 성공을 판정하지 않는다.
2. LeaveEditMode Cancel/No/Yes를 별도 작업본으로 비교한다. Cancel은 현재Editor 유지, No는 저장부모 미호출, Yes는 snapshot/Notify 뒤 next3이며 실제파일/재로드 차이로 확인한다.
3. NewFort 빈이름/금지문자/기존이름/취소/고유이름, 실제새섬·객체·다리품질, Save as의 새 파일·old 작업Fort 삭제조건·언어파일유지·제목/lastEditFort·목록갱신을 확인한다.
4. 사용자 동명 `.english`의 Player/AI/SP/브리핑으로 Test Battle→ESC Return→재Test를 실행한다. 같은 편집 Fort가 시작입력이며 +64/제목/메뉴와 Editor복귀가 맞고 테스트중변경이 편집원본으로 섞이지 않아야 한다. 누락/미지원형식/loadFort alias도 별도 검사한다.
5. 1024×768/1280×720에서 ESC Game/Edit/View/Options·덱/미니맵/QWASZX·입력창포커스·해상도변경을 확인한다. 입력창 문자가 월드명령으로 중복전달되거나 종료뒤 stale callback/이중Session·pause가 남지 않아야 한다. 모든 원본/CD 파일은 동일해야 한다.

이 이름/설치/저장/시나리오 왕복이 사용자 입력으로 완성된 뒤 해당 Edit 범위를 완료로 판정한다. Early Missions·전체캠페인·데모3 사용자완주, 나머지 비Multiplayer 메뉴·GUI/음향 동일성·배포는 계속 별도 목표다.10.72는 정밀 C의 대응주소 참고만 남기고 이번 상태/명령·분기설계는10.78 기준이다.
