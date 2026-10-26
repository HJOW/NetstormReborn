# New Fort·Add Island 후속 연결 조사

**2026-10-27 후속:** [469](cpp-new-map-storage-runtime-20261027.md)에서 Subscriber 저장·공유 이름표/빈 Image를, [470](cpp-new-fort-manager-runtime-20261027.md)에서 NewFort 세 부모와 실제 manager 장치를 채택했다. 아래는2026-10-26 조사 범위이며, 신규 자료의 Core clear/Money 저장 해석은 실제 PE 재현 근거로 정정했다. NewEditor 전체 설치·Add Island 창·GUI는 아직 미완료다.

이 문서는 **HJOW-Athlon / 2026-10-26**의 읽기 전용 조사다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 다시 확인했다. 제품 코드·기존 fixture·보호 원본·디컴파일 산출물은 변경하지 않았고 게임·빌드는 실행하지 않았다. 단위467의 Save as/State3/장기 Deck 코드는 통합 검증 중인 현재 소스로 조회했으며 검증 완료로 합산하지 않는다.

입력 식별자는 다음과 같다.

- 보호 `originals/Netstorm.exe` SHA256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`.
- 현재 `extracted/refined/originals/Netstorm.c` SHA256: `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`.
- `tell.english`는 디스크에 같은 이름의 파일이 없으므로 보호 `originals/netstorm.tarc`의 `\d\tell.english`를 `tools.taff.TaffArchive.read`로 메모리에서 읽었다. 아래 행 번호는 그 원문과 현재 정밀 C 기준이다.

## 명령과 실제 GUI 진입

| 역할 | 실제 전역/초기 ID | 원문 위치와 호출 |
| --- | --- | --- |
| 메인 Edit | `00542a18 / 020000c9` AskEditFort | 현재 mode2, lastEditFort→pending, 목록 갱신, Tell LoadEditable. 현재 `FortSelection`이 연결한다. |
| 새 이름 창 | `005428dc / 02000079` NewFort | PE `004d7446~004d745a`: **`004d3490(event.y,1,0)`**. event.x는 이 부모의 인자로 사용하지 않는다. |
| 새 이름 확정 뒤 읽기 예약 | `00542a1c / 020000ca` ReadFort | `Netstorm.c:164202~164204`: 문자열80칸 고리004c7910→00452fe0(현재ReadFort,token,token). x/y 두 칸에 같은 토큰을 넣는다. |
| 섬 추가 창 | `00542780 / 02000022` Add Island | PE `004d7bab~004d7bf3`: 00480090→**가상+20(0,0,현재00563e38 root,2)**→00466600(this,1). |
| 창의 선택/목록 사건 | `0054270c / 02000004` | 0047fb90은 first의 목록 번호와 second 비영을 읽어 드래그를 시작한다. |
| 창의 dirty 사건 | `00542708 / 02000003` | 0047fb90은 창+d80=1을 쓴다. |
| 돌 버튼/대체 버튼 사건 | `005427a8 / 0200002c`, `005427b4 / 0200002f` | 0047fb90은 first와 닫기 버튼의 serial(+2c)을 비교해 창을 Kill한다. |

`tell.english:671~681`의 LoadEditable은 `Create New Map` 버튼을 Tell LOADLOADFORT에 연결한다. LOADLOADFORT는 **Tell LoadEditable을 먼저 다시 실행한 뒤 `<$NewFort,0>`을 일반 입력 큐에 예약**한다. `RawGameMenus::Edit`의 Save as는 같은 NewFort 명령에 숫자1을 넣는다. 따라서 모달의 첫 모드는 원문 명령의 y값이며 현재 inFortMode로 바꿔 읽을 수 없다. 일반 명령에서 canCancel은 항상1이고 마지막 초기 문자열 포인터는0이므로 생성자004d3250이 현재 Fort 이름을 초기 문자열로 사용한다.

이번에는 기존 `decomp_production_shortcuts.Oracle`의 Guide/Gump 장치를 유지하고 `004d3490` 경계의 실제 스택 세 인자만 메모리에서 관측했다. 새 fixture나 제품을 작성하지 않은 독립 실행 결과다.

| 명시 입력 x/y | 실제004d3490 인자 | 스택 변화 |
| --- | --- | --- |
| 13 / 0 | 0,1,0 | 0 |
| 13 / 1 | 1,1,0 | 0 |
| 0 / 7 | 7,1,0 | 0 |

이는 정밀 C에서 인자가 빠진 `Netstorm.c:168992~168994`를 보완한다. PE004d6dba/004d6dc2가 사건+8을 현재 지역 y에 보관하고, 004d744e/744f/7451이 0·1·그 y를 역순으로 push하는 것을 함께 확인했다. 과거 `cpp-editor-client-next-20261023.md`의 `event.x` 및 Add Island의 단순 `Attach(0,0,0)` 서술은 이 문서의 실제 PE 근거로 정정한다.

현재 제품의 확인된 미연결 지점은 다음과 같다.

1. `DialogScriptCode`가 부르는 `GameMenuScriptCode`/`FortSelectionScriptCode`에는 NewFort 이름 매핑이 없다. 원본 LOADLOADFORT의 큐 예약부터 연결해야 한다.
2. 현재 `UberGump::OpenSaveFortNameDialog(canCancel)`은 실제 Editor/World를 요구하고 현재 ownerMode.fort를 첫 모드로 사용한다. 현재 Event도 y를 canCancel로 넘긴다. Save as의 y1은 같은 표시가 나오지만 메인 Create의 y0은 원문 신규 모드/취소 가능1과 다르다. NewFort의 명시 모드·canCancel·기본 초기 문자열을 원문 인자 그대로 받는 접합이 필요하다.
3. 현재 `context.readFort`는 `PushCommand(kReadFortCommand,0,Store(name))`다. 원문 확정 부모는 두 인자가 같은 토큰이다. 현재 ReadFort 소비자는 y만 읽지만 원본 큐 동일성 검사는 x도 보존해야 한다.
4. Add Island는 `GameMenuCommands` 주소/ID와 Edit 행, 빈 섬 tail의 예약까지만 있다. 현재 Event에 02000022 전용 공장/모달 소비자가 없다.

## 새 Fort 설치의 원문 부모

이름 창은 단위467의 `SaveFortNameDialog`, `RawTextEntryControls`, `RawMenuSaveFort.inc`, 같은 Gump 트리·돌 버튼·문자열 고리·작업본 경로를 재사용할 수 있다. 모드0 확정의 원문은 256BYTE CopyText→첫LF 절단→CRT alnum/underscore/공백 외 문자를 공백으로 치환→fortSpec PathSpec→GetFileAttributesA다. 기존 파일과 디렉터리 모두 비영으로 거부하고 NameRequired를 띄운다. trim/collapse나 임의 이름 자동 제출은 없다. 모달이 성공하면 Kill하고 ReadFort가 다음 일반 입력에서 pending/mode/next를 처리한다. 입력 callback에서 즉시 World를 교체하지 않는다.

ReadFort는 현재 SelectionMode2이면 next1, mode0이면 next6을 예약한다. 현재 실제 State1 prefix의 연결/AI/Deck/Kernel/Core/Pool/HUD 정리·새 Player·Editor 전역/옵션/청크 준비는 `PrepareNativeFortState(true)`를 재사용할 수 있다. 그 뒤 신규 분기는 현재 기존 파일 경로의 parse 전에 구분해야 한다.

원문 `004b5ca0`의 파일 부재 분기(`Netstorm.c:144552~144568`)는 다음 순서다.

1. 현재 Fort 경로 조회/존재 검사. 0이면 현재 경로를 다시 읽어 `004bfba0(currentTemplate,path,0)`으로 실제 새 manager를 만든다.
2. 현재 State를 얻어 bit0을 켜고 Notify한다.
3. `004b3920`의 실제 신규 자료 부모를 실행하고 외곽 alternate를1로 쓴다.
4. Subscriber→현재 handle 설정→창 제목을 실행한 뒤 외곽에 복귀한다.

`004bfba0`(`Netstorm.c:152892` 이후)은 이미 열린 manager의 진단과 header 필드 쓰기·파일 존재 재조회가 있으며, 대상이 있으면 임시 manager를 만들어 **그 실제 파일을 삭제**하는 분기도 있다. 이름 창의 사전 존재 검사만으로 이를 제거하면 검사 후 외부 변경 경계가 달라진다. 파일 장치는 반드시 같은 `EditorFiles` 작업 namespace를 사용하고 쓰기·권한변경·삭제 직전에 보호 경로를 다시 검사해야 한다. 보호 originals/CD 파일을 이 분기에 직접 전달하지 않는다.

`004b3920`(`Netstorm.c:142702~142774`)의 순서는 다음과 같다.

1. 같은 MySubscriber DWORD를−1로 쓰고 Unnamed 번역을 실제 handle에 복사한다. fort.mySubHandle/제목을 갱신하고004b38d0으로 실제 registeredSubId를 확보한다. 설정 이름의 원문 철자 차이도 현재 설정 부모에서 그대로 처리해야 한다.
2. 로컬 **20×6BYTE Territory를0**으로 만든다. Editor0에서만 첫 Territory의 모양/seed/기본 위치를 채운다. Editor1인 새 맵은 기본 첫 섬 생성의 이 쓰기를 건너뛴다.
3. 현재 known이 비영일 때 실제 SP getter를 읽고 아니면+0.0을 사용한다. 그 음수를 차감한 뒤00595348=1→현재 Core getter→0042b200의10BYTE clear→CoreData 쓰기를 실행한다. 기술/Deck 초기화가 아니다.
4. 실제 게임 난수 준비004559d0을 호출한 뒤 `Next(65536)`을 **lowWORD가 비영일 때까지 반복**해 같은005c89b4.low16에 쓴다. DWORD 상위WORD는 유지한다.
5. Subscriber 저장→설정/제목→Territory120BYTE 저장→004beec0의 Money 저장을 실행한다. Money는 같은 게임 난수 `Next(1010)+333.0f`의float32이며 Deck 초기 저장이 아니다.
6. 현재 State를 다시 얻어 +0x11 kind를 Editor이면2, 일반이면0으로 쓰고 Notify한다.

이 뒤 `004b5f70(alternate1)`은 Builder/실제 지도 attach→Territory 읽기→PlaceAll→Generate를 실행한다. Objects/Money/Badges/Technology 읽기 네 단계는 alternate0에서만 실행한다. alternate1은 실제 Island0 Exists가 비영일 때 `0046e630` populate를 실행하고 Graph(false)를 한 번 실행한다. Editor 신규 자료가 자연 빈 지도이면 이 populate는 미선택이지만 Add Island 성공 경로의 population까지 완료됐다는 뜻은 아니다.

외곽은 같은 Player/Core/Territory/Deck/사제·카메라·생산 UI/Core 쓰기/전체 Fort snapshot/State bit0 해제/Notify/startupStateCheck/EditorPlayers를 기존 순서로 마쳐야 한다. 마지막 tail의 두 HUD 입력·음악·팔레트·State144BYTE 공통 확정/다음 Poll4도 생략할 수 없다. **실제 섬 count0이면 현재 Add Island 명령을 같은40칸 입력 큐에 예약**한다. 신규 설치 callback에서 창을 즉시 열어 큐/State 순서를 합치지 않는다.

현재 `UberEditor.inc::InstallExistingEditorState`는 보호 입력을 작업본에 준비한 뒤 `Read(working)`와 `FortTemplate::Parse`를 먼저 실행한다. 새 파일은 이 위치에서 실패하므로 빈 파싱 자료를 만들어 통과시키는 대신 신규 manager 설치를 명시 분리해야 한다. `CampaignInstallMode`도 Campaign/ExistingEditor 두 종류이며, `RawExistingEditorInstallOuter`의 NewFort/DefaultName은 정확한 outside 예외다. 이 guard를 완화해 기존 파일용 부모로 신규 성공을 가장하지 않는다.

## 현재 재사용할 수 있는 저장·게임 장치

| 현재 장치 | 재사용 범위와 남은 접합 |
| --- | --- |
| `CampaignTemplateStorage`, `RawTemplateLifetime`, manager 생성자 | 실제9헤더/manager/섹션 수명·소멸·생성자 효과를 보유한다. 신규004bfba0의 파일 존재/삭제/빈 manager 공개와 초기 자료 부모를 별도 연결해야 한다. 기존 Install/LoadFile을 임의 빈 파일 읽기로 가장하지 않는다. |
| `FortSerializationGlobals` | manager 생성자와 객체 소비자가 같은005c89c0을 빌리는 현재 구조가 있다. 과거 문서의 별도 version 사본 잔여는 그대로 반복하지 않는다. 신규 manager도 같은 owner를 주입해야 한다. |
| `ClientRandomGlobals` | 실제 게임/CRT 난수, ID 초기화,005c89b4 FortCoreSeed가 이미 장기 소유다. 신규 비영WORD 반복은 이 실제 DWORD와 같은 GameRandom에 연결할 수 있다. 별도 seed/고정 난수는 필요하지 않다. |
| `ClientSubscriberGlobals`, `RawFortSubscriberLoad` | 같은 MySubscriber/LoginSubscriber, 실제 Subscriber 읽기/등록ID 장치가 있다. 신규 handle/등록ID/Subscriber writer 호출 순서는 신규 부모에 연결해야 한다. |
| `RawPlayerWorld`, `ClientCoreDataGlobals`, `WriteExistingEditorCoreData` | 현재 Player/Core/실제 Core writer의 진단·자연 관문을 재사용한다. 신규 자료의00595348=1과 현재 Core10BYTE clear를 포함해야 한다. |
| `SeedFortTerritoryRecord`, `WriteFortTerritoryRecords`, `SnapshotFortTerritory` | 현재 seed·120BYTE stream writer·실제 지도 snapshot을 보유한다. 새20레코드 초기 저장과 Add Island 뒤 현재 owner/stream 재조회에 재사용한다. |
| `RawFortWorldLoad`, `RawFortWorldLoadBinding`, `RawNormalTerrainWorldBinding` | Builder/PlaceAll/실제 일반 지형/기존 네 후처리/Graph 연결이 있다. alternate1 Island0 population은 현재 명시 미연결 예외다. |
| `SnapshotFort`, `SaveCurrentMap`, `EditorFortFileDevices` | 새 manager가 원문 초기 섹션과 State를 만든 뒤 같은 전체 Fort/Notify/작업본 저장 부모에 연결할 수 있다. 최초 생성을 snapshot 한 번으로 대신하지 않는다. |
| `ClientProductionSync`, 장기 Player Deck | 단위467에서 World 없는 prefix/HUD1도 실제 덱을 확보한다. 신규 설치 역시 Native cleanup→현재 Player→같은 Deck 저장소 순서를 유지해야 한다. |

## Add Island 전용 GUI와 성공 경로

공장00480090(`Netstorm.c:103674~103697`)은 **0xd98BYTE**를 할당해0047f450(432,412)으로 생성하고 flags|0xc, signed 깊이 조정을 수행한다. 일반 입력 부모는 이를 현재 root에 **mode2**로 부착하고 modal arg1로 등록한다. 기존 문서처럼 parent0/mode0을 가정하면 실제 좌표/깊이/모달 수명이 달라진다.

0047f450(`Netstorm.c:103202~103368`)은 원문 장식 창,14×14 임시 ChunkMap, 이미지, 글자 둘, 목록0047ccb0, 닫기 돌 버튼, Help PlacingIslands를 만든다. 실제 refCount00593154의 단일 표시 관문과 초기 dirty1/drag0을 유지한다. 현재 `GumpDecoration`, `RawMenuControls`, 공통 lifetime/input/capture/collection, Text/StyleText/Stone/Help 부모를 재사용할 수 있다. **0047f2a0의 meta 이미지·0047ccb0/0047f360의 목록·전용0047fb90 입력·00480130 Idle은 현재 대응 모듈이 없다.** 기존 일반 SHP `ImageGump`나 미니맵으로 대체했다고 완료 처리하지 않는다.

00480130 Idle은 dirty가 비영일 때0047f9b0으로 현재 자료를 다시 만들고 실제 currentImage가 있으면 창 갱신/dirty0, 목록 갱신 통지를 수행한다.0047f9b0(`Netstorm.c:103403~103476`)은 현재 templateOwner의 실제 manager 첫DWORD가 비영일 때만 실행한다. 임시 지도/현재 image를 새로 만들고 이전 목록의 image·Gump를 실제 반납한다. 창의 +d00에는 **6BYTE 레코드21개**가 있으며 비활성 기록은004b37c0으로 현재 순번/공유 Next(254)+1 seed를 보충한다.21번째 행을 임의로 지우거나20 Territory 저장 배열로 축소하지 않는다. 저장 Territory는20개이고 전용 창의 목록은21개이므로 이 서로 다른 폭/최대값을 Native fixture로 고정해야 한다.

전체 지도 image는0046e4a0→14×14 로컬 ChunkMap→현재 Territory PlaceAll→0046e410 bitmap 생성→**3084BYTE 임시 지도 복사**로 만든다. 단일 목록 image0046e5a0은 새14×14 임시 지도에서 `Place(record,0,ignorePosition=true)` 후 같은 bitmap 부모를 호출한다. 단순 SVG/사각형 이미지나 같은 크기의 가짜 점유 배열을 공급할 수 없다. 임시 지도는 실제 world 지도와 주소 정체성이 달라야 하며, 임시 Place가 world IslandList를 추가해서는 안 된다.

0047fb90(`Netstorm.c:103485~103666`)은 실제 목록 클릭/마우스 상태를 사용한다.

1. 목록 사건02000004의 second가 비영이면 현재 목록 항목 image를 조회하고0047f2a0 드래그 표시를 만든다. 목록이 보관한 실제 record index로6BYTE를 복사한다. 현재 커서 화면 위치에 붙이고 depth−100, 부모 flags0x100/0x200 및 실제 capture/push 순서를 보존한다.
2. 마우스 사건0x10000의 **누름(상위 sign bit0)**과 drag 비영에서만 놓기를 시도한다. 목록 bounds 안에 놓으면 drag Kill·선택−1·목록 갱신만 실행하는 취소다.
3. 지도 image bounds 안이면 실제 좌표 차이를 signed /16으로 절삭하고 x/y 각각0..13을 검사한다. packed 위치BYTE는 `(y<<4)|(x&15)`다. 단순 화면 클릭점만 쓰지 말고 원문이 읽는 현재 커서 float/변환 시점을 고정해야 한다.
4. 현재 templateOwner의004bd300 반환 owner와 임시 지도로 Builder를 만들고 **`RawIslandPlacement::CanPlace`의 실제0046de50 판정**을 호출한다. 거짓이면 배치/후처리/저장 없이 drag를 유지한다.
5. 참이면 복사 record의 flag bit1을 지우고0046da20으로 **같은 Builder를 실제 world 지도에 attach**한다.0046ee50의 현재 IslandList total을 읽어0046e0d0에 전달한다.
6.0046e0d0(`Netstorm.c:90369~90382`, PE0046e0d0~0046e139)은 **Fort 모드 진단→Place→0046da70 실제 지형 생성→0046e000 population→Graph(false)→Builder의 현재 owner Template에004bf6d0 Territory snapshot** 순서다. Place나 저장만 실행해 사용자 섬 추가 성공으로 처리할 수 없다.
7. 성공 복귀 후 실제 새 위치로 카메라 이동→drag Kill/포인터0→현재 선택 image 반납→행 제거→선택−1/목록 갱신→dirty1→실제005949d4 증가를 수행한다. 마지막 전역은 이미 `GameplayGlobals::MapRevision()`과 미니맵 FullRefresh가 공유한다. 파일/표면 효과가 중간에서 실패하면 그 앞의 실제 변경을 되돌려 성공처럼 만들지 않는다.

`RawIslandBuilder`, `RawIslandRuntime`, `ChunkMap`, `CanonDecoder`, `RawIslandPlacement`는 완료된 같은 몸체를 재사용할 수 있다. 일반 지형은 현재 `RawNormalTerrainWorldBinding`의 실제 같은 Pool/난수/가상 Pop을 연결한다. 미니맵은 현재 raw bitmap/dirty 부모와 연결하되 별도 지도 복사/새 섬 vector를 실제 세계로 쓰지 않는다.

**필수 남은 하위는 population이다.**0046e000(`Netstorm.c:90330~90361`)은 현재 섬 bounds/record 읽기→현재 boss 진단/재조회→0046df80→0040e2b0→record flag0x800 반영을 수행한다.0046df80의0040e130 계열과0040e2b0의 전체 생성/가상 효과를 같은 World에 연결해야 한다. 기존 `RawLocalGeyser`가 있다는 사실만으로 이 전체 부모를 완료로 간주하지 않는다. 신규 alternate1용0046e630도 population 뒤 기존 사제 선택/가상삭제가 남아 있으며 현재 `RawCampaignWorldRuntime`은 이 선택에 정확한 예외를 낸다.

## 다음 구현 단위와 검증

1. NewFort 명령 이름/일반 입력 인자부터 독립 Native fixture로 고정한다. x/y를 다르게 하고 y0/1/상위비영·release·Guide/Gump 소비·원래 문자열 초기값을 교차한다. 원문 004cf9a0의 두 인자 동일 ReadFort 예약도 실제 큐로 검증한다. 기존 SaveFortName fixture는 변경하지 않는다.
2. 신규 manager004bfba0/초기 자료004b3920을 별도 원문 부모로 복원한다. 현재 manager/owner 변경, 파일 존재 재조회·삭제 경계, State/Notify/Subscriber/SP/Core 순서, 공유 난수 소비, 비영 lowWORD 재시도·상위WORD 유지, Editor0/1의 첫 섬 차이를 Native 원본과 대조한다. 실제 작업본 장치의 보호·기존파일 유지 회귀도 포함한다.
3. 기존 파일 외곽의 guard를 유지하며 신규 설치 모드를 명시 연결한다. Native State1 전체·startupStateCheck·tail·공통 State144BYTE 확정·다음 Poll4가 끝난 뒤 실제 섬 count0의 Add Island 예약을 관측한다. 모달 명령의 다음 큐 소비를 즉시 실행하지 않는다.
4. Add Island의 생성/배치/그림/list/Idle/drag 상태를 별도 부모에 복원한다.21행/14×14/3084BYTE/이미지 수명과 RNG를 독립 원문 fixture로 고정하고 실패·닫기·목록 취소에서 world/파일 불변을 검사한다. 실제 population을 연결한 성공은 Pool/IslandList/ChunkMap/Graph/표시/난수/저장 Territory를 함께 대조한다.
5. GUI는 메인 Edit→Create New Map→고유 이름 입력/Cancel/기존 이름 거부→Native 새 편집기→자동 Add Island→드래그 충돌/경계/취소/성공→닫기→전체 저장→메뉴 복귀→같은 작업본 재로드를 검증한다. 저장된 `.fort`를 파싱하는 검사와 별도 실제 Session 재설치에서 지형/섬/객체/생산 덱이 같은지 확인한다. 같은 화면의 원본432×412 창/도움말/목록/드래그 깊이/카메라/미니맵도 비교한다.

동명 `.english` 등 시나리오 스크립트는 원본대로 사용자가 텍스트 편집기로 작성한다. NewFort나 Save as에 자동 스크립트 작성·복사 기능을 추가할 필요는 없다. 후속 Test Battle/Return은 같은 작업 namespace에서 읽는 실제 Guide 수명과 별도 저장 경계를 검증해야 한다. NewFort/Add Island 문서 조사 완료는 이 기능이나 전체 사용자 플레이 목표의 완료 판정이 아니다.

10.72 참고 대응은 현재 정밀 export의 004b3920→004302e0,004b37c0→004300c0,004b5ca0→00432e30,004b5f70→004330c0,00480090→00477300,0046e0d0→00490d50,0046e000→00490c50,0046e630→004906c0이다. 이번 인자 Probe와 구현 제안 기준은10.78이며 CD의 전용 GUI/부모 인자를 실행 검증했다고 주장하지 않는다.
