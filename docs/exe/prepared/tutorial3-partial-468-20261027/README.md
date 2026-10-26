# 468 준비: Tutorial3 자연 플레이 → Tutorial4 재현 계획

최초 계획 뒤 Source6 클론 GUI2회를 시행했으며 **채취/완주 성공은 아직 없다**. 자세한 결과와 좌표 오류는 [실행 결과](ATTEMPTS.md)를 우선한다. 아래 좌표는 실제 관찰에 따라 정정했고, 최초 문서는 README.before-observed-coordinates.md로 보존했다. **이번 정정 중에는 추가 게임/GUI·빌드를 실행하지 않았다.** 원본 게임·제품/검사 소스는 변경하지 않았다. `AGENTS.md`, 공통/닷넷 LEFT를 다시 읽었다. 조사 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 다음 실제 실행은 root의 새 제품 빌드 뒤 별도 시행으로 진행한다.

`original-tutorial3.english`는 보호 TAFF 안 `\\d\\tutorial3.english`를 바이트 그대로 읽은 조사 복사본이다. `static-evidence.json`은 최초 정적 조사 출처로 유지하고, `static-evidence-corrected.json`에 cell 니블 오류 정정과 실제 initial 좌표를 기록했다. 이 계획은 상태/SP/HP/owner/객체 삭제/다리 패턴 주입 없이 **click/right/when/esc의 실제 입력**으로 진행한다. report/snapshot은 읽기 전용 관찰에만 사용한다. 아직 고정 프레임의 무인 완주 스크립트가 아니며 합법 배치/카메라/crop/대화창 라벨과 소요 시간은 실제 실행에서 확정해야 한다.

## 원본 목표와 단계

`tutorial3.english`는 `loadFort="CaptureThePriest"`, tutorialNumber3, 시작 SP2000, myTech=`sunArcher;sunWalker`, myProd=`sunArcher`, aiOff1/aiNoTemple1이다. salvage/ascend는 금지한다. 기존 신전/워크샾을 사용하고, `.fort`나 언어 파일을 바꾸지 않는다.

**이 미션은 적 사제를 골렘이 포획하면 완료한다.** H는 희생을 다음 튜토리얼에서 배우겠다고 명시한다. 귀환/제단/희생을 추가 완료 조건으로 삼지 않는다. 운반 증거는 실제 골렘의 Priest 내용물/Carrying 메뉴와 지면 사제 제거를 함께 확인한다.

실제 단계 부모는 `RawTutorialLater::Three` ↔ PE `004c3f00`이다. 원본 대화 섹션과 현재 실제 판정은 다음과 같다.

| 단계 | 원문 안내/실제 입력 | 실제 판정과 확인 사항 |
| --- | --- | --- |
| A/A1 | 골렘을 배치하고 간헐천으로 보내 처음 채취/귀환한다. | `StormCrystalEaten` `0x020000b4`가 일반 큐에 도착하면 B 예약. `rangeCommand`라는 기존 필드명/주석을 사거리 버튼으로 오인하지 않는다. 실제 SP 수입/운반 경로를 확인한다. |
| B/B1 | 남쪽 적 섬까지 다리를 잇는다. | 현재 enemy Priest(owner local+1)와 own Priest의 Graph가 같아지면 C. 단순 다리 수나 화면상 근접으로 완료 판정하지 않는다. 원문의 사제 부재 예외를 삭제로 유발하지 않는다. |
| C | 워크샾의 Sun Disc Thrower 생산을 확인한다. | `productionContains(type71)`이면 D. Header myProd가 이미 제공하므로 C가 자연히 넘어갈 수 있다. 억지로 단계에 머무르게 하지 않는다. |
| D/D1 | 워크샾에 Wind Generator 생산을 추가한다. | 원본은 type75를 알리고 type75/76/77 중 하나가 생산에 있으면 E. 이번 자연 계획은 허용된 Wind Generator75를 선택한다. |
| E/E1 | 공급을 연결해 적의 원래 섬 위에 Sun Disc Thrower를 배치한다. | 실제 `PieceDropped` `0x030000b0`의 SID가 type71, 위치 지역이 로컬 Player+78과 다르고, 같은 점에 genus0x1000000(새 3×3 섬)이 없어야 F. 제네레이터가 만든 작은 받침 위 배치는 이 조건과 다르다. |
| F | 사거리에 들어간 적 사제에게 자동 사격한다. | `00493c00`은 포획 완료가 아니라 **현재 HP < 실제 MaxHP/2**다. 동적 최대 HP를 읽으며 기본 type의100만으로50을 고정하지 않는다. HP/owner는 실제 피해 부모가 바꾸게 둔다. |
| G | 빈 손의 골렘을 선택해 손 모양 커서인 적 사제 위를 좌클릭한다. | `CarrierAboutToPickup` `0x030000b1`, event.x=사제 SID, priest genus, 현재 owner0이면 H. 이벤트를 직접 발행하지 않는다. |
| H | 실제 Mission Accomplished에서 Next Tutorial을 누른다. | `[H.]`의 Config가 DoneTutorial3=1을 쓴다. Next Tutorial은 MissionBegin,Tutorial4. 새 실제 Tutorial4 A./Guide4/SP2000까지 관찰한다. |

H 조건의 명령 이름은 `CarrierAboutToPickup`이지만 현재 `RawCarrierPickup::Receive`는 queue → contained Create/종속 이동 → 원 지면 객체 Destroy를 같은 수신 안에서 수행한다. 따라서 안내만 보고 포획 성공으로 판정하지 말고, 같은 완료 프레임에서 실제 Form/메뉴도 대조한다. 이 원문의 정상 Destroy를 외부 `surface-delete` 주입과 구별한다.

## 실제 플레이 좌표와 정적 저장 좌표의 구별

다음은 **Source6 attempt1/2의 실제 initial Report에서 읽은 좌표**다. 두 실행 모두 같은 값이었다. 이후 실행에서는 이 표를 먼저 참고하고 해당 실행의 현재 Report를 다시 확인한다. 최초 준비본은 `FortObject.cell=(상위니블,하위니블)`을 y/x로 오독했다. 현재 C++ `Template.h`의 CellX는 `position>>4`, CellY는 `position&15`다. 최초 `static-evidence.json`은 실패 계획의 출처로 유지하고 정정은 `static-evidence-corrected.json`에 기록했다.

| 대상 | 실제 SID | actual x,y | 비고 |
| --- | --- | --- | --- |
| 로컬 Wind Temple(type123) | 15000 | (125,92) | owner1, actual HP5000. |
| 로컬 Sun Workshop(type85) | 15003 | (129,102) | owner1. 실제 생산 목록/stock을 확인한다. |
| 로컬 Priest(type158) | 15002 | (129,103) | owner1, actual HP100. 정적 저장 해석 (133,103)과 현재 설치 좌표를 혼동하지 않는다. |
| 북쪽 간헐천(type122) | 15016 | (135,73) | owner0. |
| 서북 간헐천(type122) | 15026 | (109,86) | owner0. |
| 서남 간헐천(type122) | 15036 | **(106,103)** | owner0. 다음 채취 목표. |
| 적 Priest(type158) | 15005 | **(138,153)** | owner2, actual HP100. 실제 움직이면 현재 좌표를 다시 읽는다. |
| 적 Wind Temple(type123) | 실제 initial에 없음 | 정적 저장 (138,152) | aiNoTemple1이 있는 미션이다. 지면 Temple이 있다고 가정하지 않는다. |
| 실제 배치한 로컬 Golem(type133) | 15037 | **(128,94)** | initial 이후 두 실행에서 같은 합법 위치에 생성. attempt2의 `click cell:128,94`가 선택 SID15037/모드5를 실제 만들었다. |

이 SID는 Source6 두 실행의 관측값이며 새 제품의 고정 핸들이 아니다. 다음 자동 입력은 현재 타입/owner/좌표를 읽고 actual click만 보낸다. 원본 슬롯/좌표를 직접 쓰지 않는다.

로컬 섬의 저장 청크는 (7,5),(8,5),(7,6),(8,6), 적 섬은 (6,8),(7,8),(8,8),(7,9),(8,9)다. 자연 지형 가장자리/장식·충돌은 실제 원본 생성 결과를 사용해야 한다.

- 채취 연결 목표는 **(106,103)**이다. 로컬 서남 가장자리에서 현재 합법 다리 조각을 이어 간다. 현재 `placement-reach:106,103`은 결과를 생성하지 않고 실제 원본 커서의 합법 후보를 탐색한 뒤 마우스 점만 반환한다.
- 적 섬 연결 목표는 우선 **(130,129)** 근방이다. 로컬 남단에서 적 북단으로 이어지는 경로를 만든다. 마지막 도달은 B→C의 실제 Graph 조건으로 확인한다.
- 공급 사슬의 검토용 목표는 **(127,104) → (127,116) → (129,128) → (133,140)**이다. `placement-near`의 실제 허용/공급/충돌을 통과한 후보만 클릭한다. 이 좌표가 모든 조각/지형에서 합법이라고 아직 검증하지 않았다.
- 첫 발사기 목표는 **적 원래 섬의 (137,146) 근방**이다. 저장된 적 사제(138,153)까지 기본 사거리9 안이지만, 실제 움직인 적과 현재 공급/지역을 다시 확인한다. F가 열리지 않으면 위 E의 실제 지역/3×3 조건부터 확인한다.
- 원문 Wind Generator cost400, Sun Disc Thrower cost300이다. 네 제네레이터와 발사기 한 개만으로1900 SP이므로 시작2000에 근접한다. 골렘 배치/추가 무기 비용은 실제 UI/TypeCost를 읽고, A를 넘긴 뒤에도 필요하면 **자연 채취를 더 기다린다.** 비용·SP 지원값을 넣지 않는다.

## 현재 입력 API로 실행하는 순서

시작은 root가 승인한 같은 제품/검사 자산 복사본을 사용한다. 보존 실행의 기존 DoneTutorial3 값을0으로 쓰지 않는다. 기본 `useProductTimers=1`/자연 bridge stock을 유지하고 테스트 패턴/순서 옵션은 전달하지 않는다. 직접 Tutorial3 선택으로 기능을 조사한 결과와 Early2의 Next Tutorial에서 이어 온 전체 연속 플레이 결과를 구별한다.

1. **초기 관찰**: mission Tutorial3, page A., gate3/A, 실제 Session/local owner, 시작 SP2000, 초기 객체/SID, `raw_production_content`/bridge 재고, `minimap` crop, 기존 Done값을 report/snapshot으로 남긴다. A의 MORE, A1의 OK를 실제 클릭한다.
2. **서남 간헐천 연결**: `click production:82`로 현재 실제 첫 다리 항목을 취득한다. active preview가 확인된 뒤 `click placement-reach:106,103`을 시도한다. 같은 방향에서 후보가 없으면 표시된 현재 preview를 `right cell:106,103`으로90도 회전하고 재탐색한다. 최대 네 방향을 보며 합법 조각을 놓는다. ESC 취소도 실제 입력으로만 수행한다. 재고가 없으면 타이머의 자연 보충을 기다린다. 성공하지 않은 클릭을 다리 생성으로 세지 않는다.
3. **골렘 배치/채취**: 실제 `production:133` 항목이 있을 때 취득하고 `placement-near:128,96`의 합법 위치에 배치한다. 새 Report에서 실제 생성 좌표를 읽는다. Source6 두 번째 시행의 성공 경로는 **`click cell:128,94` → 실제 selected15037/mode5 확인 → `click cell:106,103`**이다. 첫 시행의 `world:sunWalker` 선택은 성공하지 않았으므로 검증된 cell 경로를 우선한다. 운반 경로·자연 SP 수입과 B 안내를 기다린다. 골렘이 이미 전개되어 있으면 새로 중복 생성할 필요가 없다.
4. **B 안내와 남쪽 다리**: B의 MORE, B1의 OK를 누른다. 다시 실제 bridge 항목/회전/합법 위치만으로 `placement-reach:130,129`를 이어 간다. B→C가 실제 발생할 때까지 연결을 확인한다. 중간에 채취 골렘의 손을 비우거나 상태를 바꿀 필요는 없다.
5. **생산**: C의 OK 뒤 D가 바로 열리는지 확인한다. C에 머물면 `right world:sunFactory` → Put Knowledge into Production → Sun Disc Thrower를 실제 선택한다. D의 MORE/D1 OK 뒤 동일한 워크샾 메뉴에서 **Wind Generator**를 추가한다. 메뉴에 원문 항목이 없으면 기존 현재 기술/생산/권한을 조사하고 중단 원시를 보존한다. 직접 기술/owner를 주입하지 않는다.
6. **카메라와 공급**: 실제 캐리어 이동 선택을 해제한 상태에서 미니맵으로 남쪽을 본다. 선택 모드2/5에서 미니맵 좌클릭은 카메라 대신 이동 명령이므로 먼저 신전 같은 비캐리어를 실제 선택해 모드를 확인한다. `minimap:x,y`는 월드 좌표가 아니라 **현재 미니맵 로컬 픽셀**이다. report의 cropX/cropY를 읽어 목표 (wx,wy)에 대해 `(wx/2-cropX, wy/2-cropY)`를 계산하고 실제 미니맵 안 점을 클릭한다. 범위 밖이면 화면에서 보이는 합법 목표를 다시 선택한다. 카메라 값을 직접 쓰지 않는다.
7. **제네레이터/발사기 배치**: `production:75` → 위 공급 사슬 근처 `placement-near`를 각각 실제 클릭한다. stock/SP가 자연 준비될 때만 다음을 시도한다. 원래 적 섬 위에 `production:71`의 발사기를 놓아 E→F를 확인한다. 첫 발사기가 원래 적 사제에 닿지 않으면 실제 현재 위치를 보고 공급/발사기를 추가한다. 적을 삭제하거나 HP를 내리지 않는다.
8. **무력화/골렘 이동**: F의 OK 뒤 자동 사격이 실제 HP를 절반 미만으로 내리고 G가 나타날 때까지 기다린다. G의 실제 닫기 버튼은 원문 명시 버튼이 없어 공통 HTML 기본 버튼을 쓸 수 있으므로 **현재 control 라벨을 읽어 클릭한다.** 현재 빈 손 골렘을 선택해 적 사제의 현재 논리 칸으로 좌클릭한다. 손 커서(cursor0x0d)가 나오는 위치인지 화면으로 확인한다.
9. **손이 찬 경우**: 골렘을 우클릭해서 실제 Carrying: Storm Crystal/Drop… 메뉴를 확인한다. Drop Storm Crystal의 현재 실제 라벨로 클릭한 뒤 다시 선택/적 사제 클릭을 수행한다. 메뉴의 스타일/공백이 있는 전체 라벨을 추측하지 않는다. 특정 골렘을 고르려면 report의 현재 골렘 좌표에 `cell:` 클릭을 사용한다.
10. **완료와 다음 미션**: H 화면/정지/Done값, 실제 골렘 Carrying: Priest 또는 Form 내용을 저장한다. H의 Next Tutorial을 실제 클릭한다. **Tutorial4의 A.**, gate4/A·Guide sequence4, 새 Session(local owner/PlayerKnown1), 시작 SP2000, 새 TacticalCombat 객체·새 브리핑 캡처까지 확인한다. 다음 예약만 보고 완료로 처리하지 않는다.

각 click은 `main.cpp`에서 실제 mouse down+up을 같은 큐로 보낸다. `when`은 그 시점 Report에 대한 regex만 읽고 실제 입력을 선택한다. 조건은 `mission_gate\t3\t<단계ASCII>`, `page\tmission\n`, 실제 preview/stock·현재 객체·control을 조합한다. 대화창에서 MORE를 반복 누르는 전역 루프는 BACK/MORE 사이를 오갈 수 있으므로 **현재 page A./A1./B./B1./D./D1./E./E1./F./G.를 각각 제한한다.**

## 제품 연결이 있는 곳과 현재 관찰/선택 한계

다음은 읽기 전용 조사 결과다. 미복원이라고 단정할 범위와, 실제 GUI가 아직 검증하지 않은 범위를 구별한다.

| 경계 | 현재 상태/정확한 재사용처 | 다음 검증 또는 필요한 최소 API |
| --- | --- | --- |
| 사용자 골렘 선택/목적지 | GameWorld.Input → RawUserSelection/RawCarrierInput → `RawCarrierMove::Request`가 실제 Session/로컬 전송에 연결되어 있다. | 실제 `raw_input`의 선택SID/모드와 move packet64, 좌표 변화를 함께 관찰한다. |
| 포획 후보/손 커서 | `RawCarrierInput::Evaluate`가 실제 `CarrierArrival().PickupCandidate`, 같은 Graph, 기존 content/flags, 사제 무력화 상태를 읽는다. | 손이 안 나오면 연결/현재HP/빈손/좌표를 먼저 조사한다. 별도 owner 변경 명령으로 우회하지 않는다. |
| 포획 실제 수신 | `RawCarrierPickup` request4f → 실제 receiver → contained Create/종속 이동/원 사제 Destroy가 Session에 연결되어 있다. | request packet79(0x4f), H, 원 지면 사제 제거와 실제 carried Form을 같이 확인한다. |
| 골렘 Drop | RawCarrierMenu → CampaignObjectMenuClient → 같은 Session의 실제 carrier-drop 요청이 있다. | Carrying 헤더/Drop 라벨·허용 여부를 현재 메뉴에서 읽는다. 실제 input capture/모드가 끝났는지도 확인한다. |
| `world:` 목표 | **현재 owner1의 첫 selectable 타입**만 반환한다. owner2/neutral 사제나 두 번째 골렘을 고르는 문법은 없다. | 첫 정적 대상은 `cell:138,153`로 가능하다. 움직인 적/빈 골렘에는 현재 report 좌표를 수동 반영한다. 무인 동적 자동화가 필요하면 `world-sid:<sid>` 같은 **좌표 조회만 하는** helper를 추가한다. 실제 live/visible 객체 Bounds를 읽어 click점만 반환하며 소유권/선택/경로를 쓰지 않는다. |
| `cell:` 정확도/화면 | 정수 논리 칸 내부의 실제 화면점으로 역투영한다. 화면 밖의 점도 반환할 수 있다. | 미니맵 이동 뒤 실제 화면 안 여부와 적 위치를 확인한다. fraction 위치/이동 중 사제에는 현재 Bounds helper가 더 확실하다. |
| `placement-*` | 현재 real Cursor.Update/공간/공급으로 후보를 탐색하고 actual click이 건설한다. | 합법 후보 부재는 ControlPoint 없음으로 실패한다. `when`은 라벨 존재를 질의하는 기능이 없으므로 첫 GUI로 후보/시간을 확정하기 전 전면 자동 성공을 약속하지 않는다. |
| 첫 production click의 release | 현재 API click은 down+up이며 일반 생산 입력에 사용 중이다. 과거 Early2 계획에는 preview에 별도 up-if도 있었다. | click만으로 얻은 preview/버튼 capture가 정상 종료하는지 첫 실행에서 확인한다. 캡처가 남으면 원시를 보존하고 입력 API의 기존 up/down 분리 필요성을 root에 보고한다. 상태 lockDepth를 직접 고치지 않는다. |
| 운반 내용물 보고 | `GameWorld::SyncCampaignObjects`는 view.contents를 채우지 않아 현재 `object … contents count`는 실제 포획 Form 증거가 아니다. RawSurfaceWorld.Report도 Factory85/126만 내용물을 출력한다. | 제품 수정이 허용된 다음 단위에서 carrier1개에 대해 `Session.Priests().Carrier().FindContained(parent,0x20,158,0)`의 실제 Form/type/argument/flags를 읽는 `raw_carrier_content` 보고를 추가할 수 있다. 또는 현재 실제 Carrying: Priest 메뉴/스크린샷으로 확인한다. |
| 소유권/희생 | 무력화의 owner0은 실제 Priest 피해 부모가 결정한다. Tutorial3은 희생을 요구하지 않는다. | Editor SetOwner/테스트 HP/제단 수동 상태/외부 삭제를 사용하지 않는다. Tutorial4에서 배우는 기능을 이 미션의 장애로 간주하지 않는다. |

현재 조사로는 자연 포획에 반드시 필요한 게임 하위가 명백히 비어 있다는 근거는 발견하지 못했다. Source6 두 번째 시행은 실제 cell 선택·move64·경로 이동까지만 통과했다. 간헐천이 아닌 빈 칸을 잘못 목표로 삼았기 때문에 채취/귀환/포획의 미연결 여부는 이 결과로 판정할 수 없다. **적/특정 골렘의 동적 좌표 선택과 실제 carried Form 보고**도 다음 자연 플레이에서 계속 확인할 관찰 경계다.

## 기록과 판정

- 초기/각 단계/미션 전환의 같은 프레임 report와 snapshot을 남기고, A→B→C→D→E→F→G→H 순서를 확인한다. A1 등 문서 소절은 별도 stage가 아니다.
- SP2000 시작, 실제 자연 수입,400/300 건설 지출, 골렘/생산의 실제 비용과 stock을 관찰한다. 무력화 HP·손 커서·실제4f 수신·carried Priest와 H를 서로 다른 증거로 기록한다.
- 원본 보호 SHA와 실행 제품 SHA를 고정한다. 실행 후 기존 완료값을 복원/변경하는 기준은 root의 설정 보존 장치를 사용한다. 초기 Done값이1이면 메모리0→1 전이/영구 저장 검증으로 합산하지 않는다.
- 첫 시행이 멈추면 마지막 단계/원시 click/카메라/cursor/선택/HP/SP/메뉴를 보존한다. 후보/타이밍을 수정한 재시행은 별도 실행으로 기록한다. 무관한 Source/검사 기대를 바꾸어 통과시키지 않는다.
- Tutorial3에서 Tutorial4 A.까지의 자연 진입이 통과해도 Tutorial4 진행/전체 캠페인/원본 화면·음향 동일성 완료는 아니다.
