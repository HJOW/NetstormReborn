# 일반 HTML 메뉴 행의 등록 ID와 부모 종료 — 단위376 후속 분석

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 현재 정밀 `extracted/refined/originals/Netstorm.c`와 진행 중 `UberGump`·`RawMenuControls`·`MenuGumpInput`·`GumpTree`를 읽었다. 이 분석에서는 원문 관측기 생성/실행·게임/GUI·C++ 수정·커밋을 하지 않았다. 단위376 버튼/timeout 발화 복원과 일반 HTML 메뉴 행의 실제 등록 ID 복원은 별도 범위다.

## 원문에서 구별해야 하는 두 예약

| 발화 원점 | 실제 예약과 즉시 후속 처리 |
| --- | --- |
| HTML 버튼/Enter | c79b0 → 452f90(command,parameter,parameter) → 같은 HTML 부모 Kill |
| HTML 메뉴 행 | 76820 → 452fe0(command,**선택 메뉴의 등록 ID**,parameter) → openSubGump.wav → Close → 선택 메뉴 Kill → 부모에 MenuJustClosed Bubble → HTML 부모 Kill |
| 후반 Cleanup | 자식/부모 실제 소멸 → cf1c0에서 Guide 창 수 감소 → OnExit가 있으면52f90(command,parameter,parameter) |

메뉴의 첫 인자는 `Gump+1c`의 현재 등록 ID다. `+2c` serial, 호스트 token, 행 번호, 열 번호, 부모 ID와 다르다. `452fe0`은 명령 code를 `code & 0100ffff | 02000000`으로 정규화하는 원문 입구이며52f90의 그대로 예약과도 구별한다. 현재 `MenuGumpInput`은 동일 저장소의 `Base(token).id`를 이미 hooks.command에 전달하므로 새로운 메뉴 발화 계산기를 만들 필요가 없다.

메뉴 자체 Kill 뒤 `464e90(menu,MenuJustClosed,menu.serial,menu.serial)`은 부모의 가상 Handle을 **즉시 Bubble**한다. 이것을 일반 InputQueue에 새 MenuJustClosed 명령으로 늦게 넣으면 다른 페이지를 닫거나 순서가 달라질 수 있다. `GumpTree::Push`는 자기 parent/next/previous 값을 지우지 않으므로 Kill 뒤에도 부모 Bubble이 가능하다. 실제 HTML 부모가 이 통지를 받으면 c79b0은 선택 명령을 다시 예약하지 않고 부모만 Kill한다. sibling 열도 그 부모의 자식 우선 Kill에 포함되고 실제 해제/OnExit는 Cleanup까지 남는다.

일반 `$Menu`의 command0은 항목 flags의 command bit를 세우지 않으며 원문 입력에서 큐/선택 뒤 삭제가 일어나지 않는다. `$Checked`는 command0에도 command bit가 있으므로 다르다. 기존 `MenuGumpControls::AddText/AddChecked`가 이 차이를 이미 보존한다. 일반 행을 버튼 helper로 통일하면 이 차이도 잃는다. 왼쪽/오른쪽 **누름**은 실제 MenuGumpInput이 처리하며 release와 expanding은 거부한다. 기존 논리 GumpControlInput fallback은 왼쪽 누름만 지원한다.

## 현재 제품의 정확한 경계

`MapMetadata::begin`은 `menuColumnFlags`만 기록하고 실제 MenuGump를 만들지 않는다. `Compose`는 `menuColumn`에 따라 동일폭 사각형을 계산하고 논리 GumpControl/텍스트/체크를 만든다. `RawButtonControls::SetControls`는 `menu=true` 행을 원래부터 돌 버튼 생성에서 제외한다. 따라서 일반 HTML 메뉴 행에는 읽을 실제 menu.id가 **아직 없다**. 현재 논리 행 발화의0/parameter 복제/그 밖의 호스트 대체를 실제 메뉴 ID 복원으로 평가할 수 없다. 메뉴 행이 Tell처럼 세 번째 인자만 읽는 명령에서는 차이가 드러나지 않아도 첫 인자 동일성의 증거가 되지 않는다.

`RawMenuControls::Controls()`도 현재 단일 popup의 행 번호를 `GumpControl.id`로 반환하는 보고/구형 호스트 경계다. 이는 실제 Gump 등록 ID가 아니다. `RawMenuControls::Open`은 이전 전역 팝업 정리와 controlled 루트 한 칸을 요구하며, `OpenAttached`는 작성 종료 뒤 root직계 부착을 요구한다. HTML 부모 아래 여러 열을 보존하는 원문 경로에는 두 API의 계약이 맞지 않는다. `Open/Close`를 그대로 재사용하면 관련 없는 팝업/부모를 정리하거나 입력 도중 Cleanup할 수 있다.

## 가장 작은 다음 실제 연결 조각

1. 현재 공통 Gump 등록표에 HTML 부모의 실제 식별 노드를 만들고 논리 page 수명과 연결한다. 이 부모가 MenuJustClosed를 직접 받고 동일 페이지의 ClearPage/Kill을 한 번 요청하게 한다. 기존 SetExternalGumps는 text/owns/draw만 제공하고 Handle/할당을 제공하지 않으므로 그것만 추가해 입력 부모를 만들 수는 없다. RawMenuControls 안의 전용 Tell 부모 저장소/가상 Handle 분배가 가장 적은 새 경계다. 새 모달/등록/serial을 별도 사본으로 만들지 않는다.
2. `MapMetadata`의 실제 begin 시점에 `MenuGumpWidgets::BeginVertical(kTellMenuFlags=40000)`을 호출하고 반환 token을 현재 HTML 부모의 열 목록에 보관한다. 일반 Menu/Checked와 SOLO/FORT 목록 분할도 같은 begin/현재 controlled 메뉴에 AddText/AddChecked로 즉시 추가한다. 빈 파일 목록도 원문이 만든 실제 열을 보존한다. 이름/고리 토큰은 현재 단위376의 생성 시점 DWORD를 그대로 사용한다.
3. c7d40의 메뉴 처리처럼 현재 controlled 열을 **역순으로 AttachCurrent(x,y,HTML부모)** 하여 스택을0으로 돌린다. 원문은 모든 메타 처리 동안 여러 열을 controlled 스택에 남기고 Fit에서 꺼낸다. 생성 직후 매 열을 임의 pop하거나 첫 열에 모든 행을 합치면 ID/생성/배치 순서가 달라진다. 현재 logical panel 사각형과 실제 원문 c7d40 크기 계산의 차이는 별도 문서화하고, 행의 실제 입력/측정 경계는 MenuGumpLayout에서 읽는다.
4. 기존 MenuGumpInput을 그대로 분배해 실제 선택 메뉴의 현재 등록 ID로 예약하게 하고, 전용 HTML 부모 Handle에서 직접 MenuJustClosed→부모 Kill을 처리한다. 일반 HTML 행의 `input_.Event` fallback과 버튼용 FireDialogCommand 호출은 이 경로에서 제거한다. `RawMenuHooks.command`도 선택 명령의 원본 정수/인자를 허용하도록 현재 검증된 명령표로 연결한다.
5. 단순 Refresh/팔레트/Resize/Compose는 실제 메뉴 token/id/serial을 보존하고, 페이지 교체/Blank/Kill에서만 반납한다. 현재 `RawButtonControls::SetControls(...,preserveWorldUi=false)`는 root 자식 전체를 정리하므로 메인 메뉴 위 Tell 부모를 metadata 시점에 만든 직후 첫 Compose에서 다시 죽일 수 있다. 페이지 버튼 교체와 전체 월드/루트 반납의 소유권을 먼저 구별해야 한다. `RetirePageControls`도 새 메뉴 부모/열의 실제 수명을 알고 선택 부모만 반납하도록 연결한다.

이 조각의 확인 대상은 같은 페이지 두 열에서 서로 다른 실제 id/serial을 읽고 명령 첫 인자가 **선택된 열의 id**인지 확인하는 것이다. command0 일반/Checked, 오른쪽 누름/release, 비활성 행/빈 열, 메뉴 Kill 직후 부모 링크와 즉시 Bubble, sibling 반납, 같은 입력 드레인 재발화 거부, Cleanup 뒤 OnExit1회·Guide 창 수0을 함께 확인한다. RawMenu 생성/입력/수명 독립 fixture와 현재 실제 페이지의 보고서를 함께 대조해야 하며 기존 버튼/timeout fixture만으로 일반 메뉴 첫 인자를 검증할 수 없다.

기존 raw 메뉴가 제공하는 개별 계산은 대부분 재사용할 수 있다. 남는 핵심은 **생성 시점의 공통 식별값과 HTML 부모 소유권을 실제로 연결하는 것**이다. 전체 HTML 본문/모든 sentinel/배치/픽셀 동일성·Edit·사용자 완주는 계속 별도 잔여다.

## 구현 전 구체 API 제안

아래는 단위376 제품 검증/커밋에 섞지 않는 다음 단위의 **제안**이다. 선언/구현 파일은 아직 만들지 않았다. 일반 Tell이 쓸 API는 RawMenuControls에 모으고, 그 안에서 별도 `TellParentGump` 본체를 조립하는 구성이 적절하다. 새 RawMenuTree나 등록표/삭제큐를 만들 필요가 없다.

| 제안 API/경계 | 최소 계약 |
| --- | --- |
| `CreateTellParent(minWidth,minHeight)` → token | 현재 common.token으로0x21e8 소유 저장소를 준비하고 cf120을 수행한다. 같은 GumpDecoration::Construct→Tell VT/필드 초기화→공유 Guide 개수 증가다. 결과 token과 현재 Node(token).id/serial을 서로 구별한다. 아직 root에 붙이지 않는다. |
| `AddTellButton(parent,label,resolved,instant)` → child token | c7c60의20슬롯/실제 StoneButtonGump::Construct→부모 부착→해석된 command/parameter 저장→Instant bit4 순서를 보존한다. 슬롯과 child serial로 c79b0을 분배하며 logical `actions_` 인덱스를 원본 명령으로 쓰지 않는다. |
| `BeginTellMenu(parent,backgroundFlags)` → menu token | 동일 controlled 스택의 BeginVertical을 그 메타 시점에 실행하고 현재 부모의 열 목록에 보관한다. 부모별 별도 controlled 스택을 만들지 않으며 처음 진입 때 실제 전역 index0을 요구한다. |
| `AddTellMenuRow(parent,row)` | 현재 controlled 메뉴의 AddText/AddChecked를 호출한다. 이미 해석된 DWORD를 쓰고 이름을 재조회하지 않는다. Checked 종류와 checked 값은 별도다. SOLO/FORT의 행 추가와 분할 begin도 이 경로를 쓴다. |
| `SetTellTimeout(parent,seconds,resolved)` / `SetTellExit(parent,resolved)` | 메타 처리 시점의 timeout 절대 벽시각/명령·OnExit 두 DWORD를 해당 부모에 저장한다. 실제 원문 timeout idle의 기한 비교/발화는 별도 원문 근거를 확인해 연결한다. Tick의 페이지 bool만 실제 raw idle로 간주하지 않는다. |
| `SetTellBodyAndFit(parent,secondHtml)` | c7aa0의 기존 본문 Kill→실제 Text 생성/부착/Parse 뒤 c7d40의 Fit에 들어간다. 메뉴를 역순 AttachCurrent하여 controlled index0으로 만들고, 부모 최종 부착/모달 등록을 마친다. 기존 400폭 Loading 전용 Fit을 일반 Tell Fit으로 확대 사용하지 않는다. |
| `KillTellParent(parent)` | 부모가 이미 dead인지 해당 호출자 관문에서 검사하고 실제 common.lifetime.Kill만 호출한다. 프레임 Cleanup 전 저장소/등록을 없애거나 현재 다른 부모를 Kill하지 않는다. |
| `TellParentsAlive()` / `TellInstances()` | 전자는 해당 token의 현재 dead/부착/등록 상태를, 후자는 Kill 뒤 실제 소멸 전까지의 생성/소멸 개수를 읽는다. currentDialog 부재 판정과 Guide 인스턴스 수를 합치지 않는다. |

`RawTellRow` 같은 신규 구조체를 만든다면 label 원본 바이트, command/parameter, 일반/Checked 종류, checked/include/enabled만 필요하다. menuColumn은 표시용 추적 값이며 명령 첫 인자가 아니다. 부모/메뉴 token의 보고 조회는 기존 Node와 등록 ID/serial을 그대로 사용한다.

일반 HTML 부모는 이미 복원된 `GumpDecorationNode`를 상속하고,20개의 버튼 token/command/parameter, 현재 본문 token, 메뉴 열 token, 요청 최소 폭/높이, timeout, OnExit를 가진 안정된 저장소로 충분하다. RawMenuControls의 `Node/Background/Attach/Redraw/Draw/Handle/DeleteScalar/Release` 분배가 이 같은 주소를 보아야 한다. 현재 로딩도 decorations+LoadingFields로 같은 종류의 부모 일부를 표현하므로, 새 Tell 본체는 처음부터 로딩이 이후 합류할 수 있게 만들되 이번에 로딩을 자동 이관했다고 주장하지 않는다.

메타 생성의 원본 순서상 **메뉴만 실제 생성하는 것은 최종 설계가 아니다.** c83b0은 일반 Button 전체→Instant 전체→Menu/Checked 순서로 실제 new/등록하고, 본문 Text는 그 뒤 c7aa0에서 만든다. 현재처럼 버튼을 Compose에서 나중에 만들면 유효한 menu.id를 얻어도 원문 등록/serial 할당 순서는 달라진다. 따라서 다음 실제 조각은 부모·버튼·메뉴의 생성까지 함께 이관하고, Compose에서는 그 자식의 현재 label/크기/위치를 조회하게 하는 편이 일관된다. ParseDialogPage에 필요한 추가 훅은 새 원본 인자 해석기가 아니라 **종류 판별/해석이 끝난 Button 항목의 생성 callback** 하나다. 현재 appendMenu도 같은 방식으로 실제 열/행 생성을 호출한다.

## 공통 소유권과 첫 Compose 정리안

RawMenuTree는 이미 factory/collection/tree/lifetime/events/input/stones와 공통 token/node/attach를 제공한다. Tell 구현에 추가 상태 사본을 내줄 필요가 없다. 최소 외부 보완은 RawMenuControls의 전용 Tell 부모 분배와, 다음 두 수명 통지다.

- 부모 Kill이 확정되면 `onTellKilled(parent)`를 한 번 알린다. UberGump는 **현재 표시 부모 token과 일치할 때만** page/phase/report 입력 영역을 내려야 한다. 옛 부모의 callback이 새 페이지를 닫지 않게 한다. MenuJustClosed는 이 콜백으로 가며 일반 InputQueue에 중복 예약하지 않는다.
- 실제 scalar 소멸에서 `onTellDestroyed(parent)`를 알릴 수 있으나 원문 OnExit 예약 자체는 Tell 본체가 같은 RawMenuHooks.command/전용 exact52f90 예약 경계를 사용한다. 일반 메뉴의52fe0 정규화 입구와 OnExit의52f90 입구는 구별한다. 논리 retiredPageExits와 raw 부모가 같은 OnExit를 각각 예약해서는 안 된다.

현재 `RawButtonControls::SetControls`는 화면 갱신과 root 전체 정리 두 책임을 함께 가진다. 다음 단위에서는 메인 메뉴/Game 임시 버튼의 교체만 맡는 `ReplaceHostControls`(기존 자신의 controls token만 Kill)와, 명시 종료/월드 반납/소멸자가 쓰는 `ClearRootChildren`을 분리한다. 전자는 root의 Tell/Help/메뉴/HUD를 스캔해 지우지 않고, **Cleanup도 호출하지 않는다.** 후자는 원본 수명/소유자 종료 관문에서만 자식 전체 Kill과 허용된 Cleanup을 수행한다. 이 분리는 새 부모가 첫 Compose에서 죽는 문제와, 기존 Kill된 부모의 OnExit가 Compose 안의 SetControls→Cleanup으로 너무 일찍 실행될 문제를 함께 막는다.

HTML 버튼은 새 Tell 부모가 소유하므로 기존 host `controls` 맵에 다시 등록해 SetControls가 소유하는 척하지 않는다. 레거시 GumpControl/labels/actions는 자동 검사/보고 좌표의 어댑터로만 남길 수 있으나, 실제 HTML 입력은 common.input→자식→Tell Handle이 처리해야 한다. Resize/Refresh는 실제 부모/버튼/열을 새로 생성하지 않고 Fit/Draw만 갱신한다. 실제 world 해제는 Tell/Help/메뉴의 Session 의존 참조 반납을 먼저 마친 뒤 HUD와 월드를 해제한다.

현재 논리 `pageAlive_`·retiredPageExits는 일반 HTML 부모를 raw로 이관하는 그 순간 해당 부모에 대해 제거한다. GuideOpen은 raw Tell 생성/소멸 개수, dialogsClear는 common.life.currentDialog의 원문 관문을 읽게 한다. 일부 부모만 이관하는 과도기에는 raw 카운트와 논리 카운트가 소유자별로 겹치지 않는다는 표를 유지해야 하며 같은 부모를 합산하지 않는다. 무본문 cf270도 새 부모를 생성하고 c7aa0의 빈 본문/부착을 거친 뒤 Kill하므로, 그때의 임시 Text 생성/등록과 Cleanup까지의 인스턴스 수를 생략하지 않는다.

실제 본체로 먼저 분리할 수 있는 범위는 cf120 생성, c7c60 버튼 추가, c79b0 부모 Handle, cf1c0/d3230 소멸이다. 이 본체는 외부의 allocate/constructDecoration/constructButton/attach/queue/kill/free/currentWall 경계를 훅으로 받고 원문 fields/순서만 구현할 수 있다. 기존 GumpDecoration·StoneButtonGump·GumpLifetime을 재구현하지 않는다. c7d40 전체 Fit과 timeout idle은 별도 정밀 원문/독립 관측 범위를 확정한 뒤 같은 본체에 붙인다. 이번 분석에서는 이 helper 파일이나 새 원문 fixture를 만들지 않았다.
