# State1 정리를 위한 월드 저장소 장기화 계획

**2026-10-25 읽기 전용 구현 초안**이다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 수행일은 **2026-10-23**이다. 대상은 originals의 PATCH 10.78이다. 현재 root의429 동결23소스와430 revision 작업은 수정하지 않았다. 이 문서만 작성했으며 신규 디컴파일/Native oracle/게임/GUI/빌드/검사를 실행하지 않았다. 아래 내용은 **431 이후 후보 계획**이며 구현·실행 완료 보고가 아니다.

주 근거는 [원문 C](../../extracted/refined/originals/Netstorm.c), [함수 목록](../../extracted/refined/originals/functions.tsv), 보호 PE의 메모리상 함수 바이트 정적 판독, [RawGraph](../../cpppj/src/o/RawGraph.cpp), [RawSurfaceStorage](../../cpppj/src/client/RawSurfaceStorage.cpp), [RawSurfaceWorld](../../cpppj/src/client/RawSurfaceWorld.cpp), [RawCampaignSession](../../cpppj/src/client/RawCampaignSession.cpp)이다. 기존 fixture/문서는 아래에서 그 증거 범위를 구별한다.

## 후속 정적 대조의 주소 정정

Graph431과 TotalMade433의 실제 적용/집중 검증은 각 [Graph 문서](cpp-graph-storage-owner-20261025.md)와 [TotalMade 문서](cpp-totalmade-storage-owner-20261025.md)에 기록한다. 아래 예상 파일/설계 표는 작성 당시 후보 이력이며 현재 구현 소유자를 다시 조사한 표가 아니다.

433 진행 중 보호PE의12명령/초기값을 직접 확인한 기록은 `extracted/cpp-state-input-address-review-20261025.json`이다. 원본 실행이나 새 디컴파일이 아닌 같은HJOW-Athlon 정밀본/PE의 읽기 대조다. 아래 후보 계획의 잘못된 두 주소 해석을 정정한다.

- cleanup의 실제 선택 해제는004b3077에서 호출하는 **004d5c60**이다. enabled005434e4 검사 뒤ClearMultiple004d5490의count005caf34=0,selected005caea0=0,Highlight004d5bb0(0)을 실행한다. 커서 취소는 앞004b2eb3의00446e70 호출이다.
- State7의004b74fd가 호출하는 **004978d0은 렌더러 상태 선택**이다. 인자0이면00540ce0을 사용하고 현재00540ce4와다르면00540ce4=값 뒤0059a8b0=1을 쓴다. PE의두store는004978e5/004978ea이며 C표시 순서와 구별한다. 현재RawLocalMissionStateHooks의clearSelection 이름/fixture의Selection 사건은 오명명 후보다. 이 원문 하위를 hook한 기존fixture를 실제선택해제 본문의 검증으로 세거나 여기에UserSelectionClear를 연결하지 않는다. 코드·fixture 수정은 이번에 하지 않았다.
- 커서hotspot보정 좌표는 **0055a2f8/fc**,원시마우스 좌표는 **005cae94/98**이다. PE00446b8a는`a3fca25500`,00446b95는`890df8a25500`으로보정좌표에저장한다. 두소유자를합치지않는다.
- server00540bc0/boss00540bc4의 PE최초DWORD는 모두 **1**이다. 이후cleanup의0쓰기와구별하며 새Client owner를BSS0으로시작하지않는다. 전체권한owner접합은아직후속이다.

## 권고하는 분리

**431은 Graph의 실제 기록/스택을 장기 저장소로 분리하는 한 단위부터 진행**하는 것을 권고한다. World의 실행 모듈을 전부 Storage로 이동하지 않는다. `RawGraph`에는 Session callback이 없으나 TYPE/FrameCode 스냅샷과 계산 검증 정책이 있다. `RawIslandRuntime`과 `RawCampaignCursor`에는 실제 Session/World 참조와 callback이 있으므로 데이터만 장기화하고 실행기는 현재 Runtime 수명에 남긴다.

| 대상 | 현재 지역 소유자·필수 데이터 | 최소 장기 owner / 실행기 경계 |
| --- | --- | --- |
| Graph | RawSurfaceWorld::Impl::graph 안 records255×6BYTE, stack4096DWORD | RawSurfaceStorage 안 `RawGraphState`의 고정 두 배열. RawGraph는 이 상태를 명시 대여하는 World 수명의 실행기로 남긴다. |
| TotalMade | World::Impl::bookkeeping의 local/localSecondary/global 각256DWORD | 같은 Storage의 실제 세 배열이 필요하다. 현재 `SquidPostPopState` 전체를 옮기는 경우의 바인딩/로딩 관문 위험은 아래 참조. |
| Island | Session::Impl::islandBytes3204BYTE | Storage에 같은3204BYTE만 보관. RawIslandRuntime은 Session의 finishState/residences/revision/ownerChanged/소유권 hook/현재 Player span을 빌리는 실행기로 유지한다. |
| ChunkMap | Session::Impl::chunks의256×12BYTE·폭/높이 | Storage의 같은 ChunkMap. 현재 클래스에 없는 첫 mode DWORD와 Reset00432360을 추가한다. GameWorld의 legacy chunks_/IslandList는 실제 Raw Session 지도와 별개다. |
| 사제 장부 | Session::priestState.priests의 ownerState9DWORD와 SID 목록 | ClientGameplayGlobals 또는 Storage의 실제 `PriestPostPopState`. RawPriestWorldBindings/RawPriestWorldState의 다른 권한·예약·이동 상태를 무조건 함께 이동하지 않는다. |
| 건설 커서 | RawCampaignCursor::Impl의 CursorLifecycleState/PlacementState | 기존 ClientGameplayGlobals.cursor와 같은 수명의 두 상태가 우선 필요하다. RawCursorLifecycle/Placement/Preview/Position 실행기는 실제 장치/Session 효과를 그때 빌린다. |
| 사용자 선택 | GameWorld의 selected_/mouseType_/highlightMode_/mousePoint_, GameWorld::Input의 enabled/remembered/multiple/count | ClientGameplayGlobals의 같은 선택/강조 전역. 원시 mousePoint005cae94/98과 cursor.screenX/Y의hotspot보정0055a2f8/fc는 별개로 소유한다. RawUserSelection/RawUserHighlight/RawCarrierInput의 callback은 Runtime/UI 수명에 남긴다. |

## Graph의 원문 장기 물리와 C++ 정책

| 필드 | 원문 근거 / 수명 | 431 설계 |
| --- | --- | --- |
| records | `00562c18`은 **0x5fa=1530BYTE=255×6BYTE** heap을 가리키는 전역 pointer다. 시작00462eb0이 new하고, reset00462b20/Graph 연산이 같은 표를 계속 사용한다. | `std::array<GraphRecord,255>`를 RawGraphState에 한 번 확보하여 World 교체로 주소/나머지 필드를 잃지 않는다. |
| stack | `00562c20`은 flood stack 전역 pointer다. **00462f10의 최초 flood에서 pointer0이면 new0x4000→4096DWORD0**을 한 번 수행한다. 이후 같은 스택의 오래된 값도 남는다. | 고정4096DWORD를 같은 State에 보관한다. Native의 lazy allocation/null/failure까지 재현하려면 별도 최초 확보 장치를 설계해야 하며, 고정 배열을 즉시 확보하는 현재 모델과 같다고 주장하지 않는다. cleanup reset은 스택 확보/0쓰기를 추가하지 않는다. |
| recovery_ | `GraphRecovery::Reject/FullPool`은 C++의 **지역 snapshot 또는 전체 Pool 검증 계약**이다. [기존 복구 문서](cpp-graphrecovery-reconstruction.md)와 Graph.h가 그 범위를 명시한다. 대응 Native DWORD는 없다. | 실제 Native 상태인 것처럼 RawGraphState에 넣지 않는다. 현재 RawGraph 실행기의 선택 정책으로 유지하고 ResetTable/AcquirePrepared로 바꾸지 않는다. |
| rebuildGraph | Native `00562c1c`은004637b0의 분할 정책이다. 기록 pointer00562c18과 stack pointer00562c20 사이의 별도 전역이다. | 위 recovery_와 다르다. 기존 Detach의 rebuild 인자/현재 정책 연결은 별도이며431에서 두 값을 합치지 않는다. |
| pool/hash/spots | RawGraph는 reference/span만 보유한다. 세 실제 배열은 이미 RawSurfaceStorage 수명이다. | 같은 저장소의 정체성 검사를 유지한다. spots 크기/주소 변경, 다른 해시/Pool 조합은 거부한다. |
| types_/frames_ | RawGraph 생성자가 TYPE/FrameCode를 복사한다. RawGraph 안에 owner/Kernel/통지 callback은 없다. | 실행기를 World마다 새로 만들면 기존 현재 snapshot 경계를 유지한다. RawGraph 객체 전체를 Storage에 영구 보관할 경우 늦은 mutable TYPE/FrameCode 수정이 오래된 사본으로 가려질 수 있다. |

원문 pointer 두 개는 각각 별도 할당이다. `GraphRecord`의1530BYTE와 flood16384BYTE가 Native에서 한 연속 구조라는 주장은 하지 않는다. `RawGraphState`는 호스트 소유권 묶음이며 인접 메모리 주소를 가장하지 않는다.

### 시작00462eb0과 cleanup00462b20은 다르다

보호 PE와 C 본문을 대조한 **시작00462eb0**의 순서는 `new(0x5fa)`→255행 surfaces/inUse 두WORD0→전역pointer 저장→00462b20→**255행 reserved의 low BYTE0**이다. reserved의 high BYTE는 이 몸체에서 쓰지 않는다. 따라서 시작 직후 Graph0의 reserved low BYTE는0이며, cleanup reset 후의1과 다르다. 현재 C++의 `records_{}`가 모든 high BYTE까지0으로 만드는 것은 기존 호스트 초기값이다. 이를 원문 할당 입력의 복원 완료로 세지 않는다. 최초 할당 내용을 정확히 재현하는 확장은 명시 실제 할당 장치/독립 근거가 필요하다.

**cleanup00462b20(85BYTE)**의 정확한 범위는 다음과 같다.

- 행0..250의 `surfaces` WORD와 `inUse` WORD만0. 각 행 reserved WORD는 그대로 둔다.
- 행0의 `inUse` WORD1, reserved **low BYTE만1**. high BYTE 보존.
- sentinel 행254의 surfaces/inUse 두WORD를 각각0x7dfd로 쓴다. sentinel reserved는 보존.
- 행251..253 전체, 나머지 reserved, flood4096DWORD, Pool, hash, spots는 보존한다.
- 할당/해제/소유자 callback/게임 시각/난수 조회가 없다.

PE의 범위는 `00462b2f cmp eax,0x5e2`의6BYTE stride 반복, `00462b48 WORD[base+2]=1`, `00462b52 BYTE[base+4]=1`, `00462b60 WORD[base+0x5f4]=7dfd`, `00462b6d WORD[base+0x5f6]=7dfd`다. 기존 [GraphResetTests](../../cpppj/tests/GraphResetTests.cpp)의 Native16은 표1530BYTE의 전체 출력 및 Pool/stack 보존을 대조한다. 이 자료는 작은 reset의 증거이며 실제 Client/준비 대여 연결 완료의 증거는 아니다.

### CalculatePostDestroy의 clone을 장기 상태와 섞지 않는다

[RawGraph::CalculatePostDestroy](../../cpppj/src/o/RawGraph.cpp)는 `auto nextPool=pool_`과 **현재 records/stack의 사본**을 받는 `RawGraph next(nextPool,hash_,spots_,types_,frames_,records_,stack_,recovery_)`를 만든 뒤 여러 `next.Add(candidate)`를 수행한다. `ValidatePostDestroy`도 이 계산을 실행한다. 실제 변경은 나중 Plan→Commit에서만 기록해야 한다.

따라서 새 State 대여 overload는 **World 조립이 명시 선택할 때만** 사용한다. 기존 records/stack 입력 constructor는 계속 독립 snapshot을 소유해야 한다. `next`가 원래 Storage의 State를 빌리면 Validate 중 실제 표/stack이 바뀌고 예외 시 부분 변화가 남으므로 명확한 회귀다. `Calculate`, `CalculatePostDestroy`의 Plan 기록/stack은 계속 값 사본이고 Commit만 실제 대여 State에 저장한다.

## 431의 예상 파일 변경안

새 파일을 꼭 만들 필요는 없다. 다음 **기존8파일** 안에서 한 단위를 만들 수 있다. 아래 함수/타입명은 제안이며 아직 작성하지 않았다.

| 파일 | 변경안 |
| --- | --- |
| [o/RawGraph.h](../../cpppj/src/o/RawGraph.h) | records/stack의 명시 `RawGraphState`를 정의한다. 기존 독립 snapshot constructor와 별도로 같은 State를 빌리는 overload를 추가한다. recovery는 실행기 필드에 둔다. State 대여 객체의 이동/복사에 의한 참조 변경을 허용하지 않는다. |
| [o/RawGraph.cpp](../../cpppj/src/o/RawGraph.cpp) | 기존 기록/stack 접근을 같은 State에 연결한다. 기존 ResetTable 몸체는 `ResetRawGraphTable(RawGraphState&)` 같은 공통 작은 함수에 위임해 Session 없는 cleanup에서도 동일 본문을 사용하게 한다. 기존 snapshot/Plan→Commit/clone 계산은 독립 상태로 유지한다. |
| [client/RawSurfaceWorld.h](../../cpppj/src/client/RawSurfaceWorld.h) | RawSurfaceStorage에 단일 GraphState를 추가한다. Storage 자체의 복사/이동을 금지해 그 필드 주소가 바뀌지 않게 한다. RawWorldCore의 기존 graph reference API는 유지한다. |
| [client/RawSurfaceStorage.cpp](../../cpppj/src/client/RawSurfaceStorage.cpp) | 최초 구성, 기존 일반 Acquire, 명시 AcquirePrepared의 정책을 구별한다. prepared의 획득/반납/부분 실패에서는 Graph 기록/stack을 reset하거나 초기값으로 바꾸지 않는다. |
| [client/RawSurfaceWorld.cpp](../../cpppj/src/client/RawSurfaceWorld.cpp) | 새 지역 실행기가 같은 Storage의 GraphState를 빌리도록 한다. prepared 경로의 무조건 ResetTable 호출을 제거한다. 기존 일반 World의 fresh 초기화 의미를 보존할 경우 그 명시 호환 준비는 한 곳에서만 실행하며 Native partial Reset과 구별한다. |
| [tests/RawGraphTests.cpp](../../cpppj/tests/RawGraphTests.cpp) | 같은 State를 빌리는 실행기·독립 snapshot의 정체성과 반영 차이, ValidatePostDestroy 성공/실패 모두의 실제 입력 불변, 정상 PostDestroy의 Commit만 반영하는 검사를 추가한다. |
| [tests/GraphResetTests.cpp](../../cpppj/tests/GraphResetTests.cpp) | 기존 Native16을 공유 State 경로에도 적용한다. reserved high BYTE/틈3행/sentinel reserved/4096stack의 비영 흔적을 각각 유지하고 별도 State에는 쓰지 않는지 확인한다. |
| [tests/RawSurfaceStorageTests.cpp](../../cpppj/tests/RawSurfaceStorageTests.cpp) | World가 사라져도 같은 records/stack 주소·내용을 유지하는지, 준비 대여 전후/부분 실패에서 보존하는지, 두 번째 실제 World도 같은 State를 쓰는지 검사한다. 다른 Storage 간 독립성/중복 대여 거부와 기존 일반 설치 의미도 유지한다. |

기존 세 검사 파일을 보강하면 새 CMake 등록은 필요하지 않다. 초기 소유권 검사를 별도 새 파일로 나눌 경우 root가 명시 등록한다. 독립 fixture를 제품에 맞춰 수정하지 않는다. 검사 필터 후보는 `RawGraph`, `GraphReset`, `GraphForms`, `DestroyGraph`, `RawSurfaceStorage`이며 실제 실행 범위는 root의 최종 변경과 증분 빌드 결과에 따라 결정한다. 이 문서에서는 실행하지 않았다.

### 일반 Acquire와 prepared의 구체 위험

현재 일반 Acquire는 두 번째 대여부터 Pool/Hash/spot을 reset하고 새 World가 새 Graph 표/stack을 만든다. Graph를 장기화한 뒤 단순 ResetTable만 추가하면 이전 stack/reserved가 남아 기존 일반 경로와 달라진다. 반대로 prepared에서도 전체 State를 새로0으로 만들면 원문 partial reset의 보존 영역을 잃는다. **기존 일반 설치의 호환 준비**와 **앞 Native 부모를 마친 prepared 대여**를 분리해야 한다. 전자를 원문 cleanup 실행이라고 부르지 않는다.

대여 중 Graph를 바인딩한 뒤 후속 모듈 생성이 실패해도 State를 조용히 초기화하지 않는다. 현재 Use 반납은 borrowed만 해제한다. 새 State를 옮기거나 뒤에서 rollback해 다른 생성 전 효과를 숨기지 않는다. World가 보유한 `shared_ptr<RawSurfaceStorage>`/Use는 모든 모듈보다 먼저 선언되어 마지막에 반납되는 기존 수명을 유지한다. 후속 모듈은 Graph 실행기를 먼저 참조하므로 현재 World 내부의 실행기 선언/소멸 순서도 유지한다.

## Graph 뒤의 작은 단위들

1. **TotalMade와 나머지 장부 수명:** 세256DWORD를 같은 장기 물리로 만들고 기존 RawTotalMade::ResetCounts를 그곳에 연결한다. 원문은005c94d0/005c98d0/005c9cd0의 첫255DWORD만 쓴다. SquidPostPopState 전체에는 graphsEnabled/suppressed/pendingPlacement/localOwner/totalCost/depth/productionDirty/AI관문과 여러 목록이 있으므로 전체 `{}` 또는 무조건 보존을3표 reset과 같다고 세지 않는다. 새 World는 현재 fresh graphsEnabled=false이고 FinishLoadAfterGraph에서만 true가 되므로 이전 true를 prepared의 최초 Pop 전에 남기는 변화도 검토해야 한다.
2. **Island·Chunk:** Storage에3204BYTE와 실제 ChunkMap을 두고 Session은 reference로 빌린다. Island Reset의 필드 쓰기를 공통 data-only helper로 분리해 기존 RawIslandRuntime::Reset도 같은 본문을 호출하게 하면, Session 없는 cleanup에서 가짜 mode/residence/소유권 hook으로 임시 실행기를 만들 필요가 없다. Chunk Reset은 mode 보관→1→현재 폭/높이의 각12BYTE0/첫BYTE7f→mode복구다. 이후 cleanup과 State1 prefix가 각각 mode1을 쓰는 순서를 유지한다. 현재 constructor만으로 이 두 reset이 실행됐다고 세지 않는다.
3. **사제·커서·선택:** 사제 ownerState9 및 원래 SID목록, CursorLifecycle/Placement, 선택/강조의 실제 현재 데이터를 장기화한다. 모듈과 callback은 기존 Runtime/UI의 정확 반납 시점까지 유지한다. 사제 reset00492240은 owner1..8의 한DWORD만 쓰며 SID목록/count를 지우지 않는다. 해당 목록을 어떤 원문 leaf가 정리하는지와 현재 Player/Pool 재구성의 접합을 먼저 확인해야 한다.

`SquidPostPopState` 전체를 Storage에 옮기는 안에는 추가 함정이 있다. 현재 `ownerFactories[9].CurrentBinding`은 ClientPlayerRecords*를 캡처하며 Player Use를 직접 캡처하지는 않는다. 그러나 `BindCurrent`가 이미 연결된 목록의 재바인딩을 거부하므로 새 Session ctor의 무조건9회 BindFactory가 두 번째부터 실패한다. 같은 ClientPlayerRecords owner에 최초 한 번 연결하고 정체성을 확인하거나, 실제 대여 반납 뒤 명시 재연결해야 한다. raw pointer만 남겨놓고 그 Player owner가 Storage보다 먼저 소멸하도록 만들면 안 된다. 이 이유로3표의 좁은 공유 물리를 먼저 분리하는 안도 비교할 가치가 있다.

RawIslandRuntime 전체를 Storage에 옮기면 현재 `players_` span이 Player 재할당 뒤 이전 물리를 가리키고, `setResidenceOwner=[this]`가 옛 Session을 부를 수 있다. RawCampaignCursor 전체 역시 data.world/forms/contained/reward/construction과 `[this]` callback, 실제 Kernel의 pendingTimer를 빌리므로 장기화 대상이 아니다. 커서의 바깥 `Cancel()`은 원문 raw body 앞에 game.prepare를 추가하므로 Native cleanup에서는 정확한 RawCursorLifecycle::Cancel 경계도 구별한다.

첫 메뉴의 선택 enabled005434e4는 보호 PE에서 **1**이다. `world_`가 없다는 이유로 enabled0을 반환해서 Clear를 통째로 생략하지 않는다. selected0/다중 count0이면 객체 통지만 자연 생략하고, selected0 store와 실제 Highlight(0) 본문은 남는다. Highlight는 현재 mode가 이미0이면 자체 조기 반환한다. 다중 선택 vector를 장기화할 때 현재 UserSelectionState의 span은 재할당된 vector를 자동으로 따라가지 않으므로 고정 물리/현재 pointer 조회 계약도 필요하다.

## 431 완료로 세지 않을 범위

위 Graph 단위를 마쳐도 실제 Tick State1 cleanup/prefix, 전체 Client 전역 adapter, 사제/커서 소멸, 전체 입력/편집·저장, SceneMusic 공통 RNG, 원문 전체 Graph/동적 할당 수명과 모든 미션 동일성이 완료되는 것은 아니다. 다음430의 revision005949d4와 server/boss 조사는 별도다. 이 문서는 그 동결 소스의 실행 결과를 변경하거나 새 완료 수치를 추가하지 않는다.
