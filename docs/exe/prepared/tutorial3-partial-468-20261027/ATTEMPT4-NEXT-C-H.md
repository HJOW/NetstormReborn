# attempt4 이후 Tutorial3 C~H 실제 입력 준비

2026-10-26, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽었다. 마지막 정밀 디컴파일 HJOW-Athlon/2026-10-23은 변경하지 않았다. 이번 파일은 읽기 전용 코드/기존 실행 자료 분석에 따른 **미실행 계획**이다. 제품 src/tests, 원본/기존 fixture, attempt1~4 원시는 변경하지 않았고 빌드·게임·GUI를 실행하지 않았다. 직접 상태·SP·HP·소유자·다리 패턴·기술을 주입하지 않는다.

## attempt3 정지와 attempt4의 새 근거

attempt3는 frame550 이후 다리35칸이 그대로이고, 골렘15037은 선택 mode5→move64 한 번 뒤에도 frame4000까지(128,94)였다. `paused=0`, SP2000도 그대로였다. 다리의 서쪽 마지막 접근 칸은(108,103)이며 간헐천 쪽(107,103)에 다리가 없다. f550.bmp에도 연결 앞 빈 간격이 보인다. attempt2의 잘못된(103,106) 목적지는 실제 경로/이동/도착이 됐지만 그 실행의 다리31칸은 다른 모양이므로 같은 연결 증거가 아니다.

`RawCarrierMove::Receive`는 간헐천 클릭에서 현재 genus→`GoGeyser`의 접근점 한 번→실패 시 일반 Start를 실행한다. `RawPathProcess::InitializePath`/`RawPathSearch`는 현재 지면과 같은 Graph/BFS 경로가 필요하며, `RawPathStart`는 count<1이면 새 경로를 부착하지 않고 반납한다. 따라서 요청64만으로 실제 이동/채취 성공이 아니다. attempt3는 미연결 경로에 맞는 관찰이다. root의 attempt4는 추가8조각 뒤 frame1035 골렘(127.598,94.401) 자연 이동, 이후 SP2200/B1→C를 확인하여 이 설명을 강화한다. 정확한 실패 Graph/경로 배열은 기존 Report에 없으므로 attempt3에서 해당 내부 값까지 직접 관찰했다고 주장하지 않는다.

## C 정지의 정확한 판정

attempt4 initial의 `raw_production_content 30 71 4 ...`는 현재 기술 부모의 **기술 소유 flags4**다. Factory의 생산 flags0x10 또는 `RawProductionDeck::Contains(71)`을 뜻하지 않는다. 현재 최종 C(67)·SP2400에서 왼쪽 실제 생산 항목은 다리82와 골렘133이며 71은 없다. `RawTutorialLater::Three`의 C 판정은 같은 실제 Deck의 Contains71이다. Header의 `myProd=sunArcher` 또는 기존 준비 문서의 “C가 자연히 넘어갈 수 있다”를 이 실행의 완료 증거로 쓰지 않는다.

`GameWorld::Report`의 `raw_production_content`는 CurrentTechnologyParent 체인이다. Factory15003의 내용물 전용 보고가 아니다. `object ... contents count`도 현재 SyncCampaignObjects가 실제 Carrier 내용물을 채우지 않으므로 포획 완료 증거로 사용하지 않는다.

## 단계별 실제 메뉴·입력

아래 프레임 간격은 새 실행에서 정한다. 각 단계의 실제 page/gate/메뉴/생산 항목을 읽어 다음 입력을 수행하며, 아직 확인되지 않은 라벨을 성공으로 가정한 고정 반복을 만들지 않는다. `right`와 `click`은 기존 실제 마우스 down/up 큐를 사용한다.

| 단계/관문 | 실제 입력 | 다음 확인 |
| --- | --- | --- |
| C, gate67, page mission | `right world:sunFactory` 또는 현재 셀 `right cell:129,102` → `click Put Knowledge into Production` → `click Sun Disc Thrower` | 실제 원문 Factory 메뉴의 하위 항목이다. 생산 Deck에71이 추가되고 D 안내로 넘어가는지 확인한다. 기술 소유만 보고 생략하지 않는다. |
| D/D1, gate68 | page D.에서 MORE, D1.에서 OK → 같은 Workshop 우클릭→`Put Knowledge into Production`→`Wind Generator` | D가 현재 기술75를 알린다. 실제75 생산 추가 후 E(69). 생산 슬롯 부족이면 실제 Current Production/Upgrade 상태를 읽고 원문 UI로 처리하며 직접 슬롯을 늘리지 않는다. |
| E/E1, gate69 | page E. MORE, E1. OK → 왼쪽 `production:75` 취득→현재 공급/공간을 통과하는 `placement-near:<x>,<y>` 실제 클릭 | Wind Generator 여러 개로 현재 신전부터 공급을 남쪽으로 연장한다. 새75 객체의 실제 좌표/소유자·SP400 감소와 다음 위치의 sourceAllowed를 관찰한다. |
| E, 실제 적 섬 공급 후 | `production:71` 취득→`placement-fire:137,146,15005` | 현재 적 Priest SID가15005일 때의 입력이다. 새 실행에서 현재 SID를 다시 읽는다. 실제 MayPlace/MayTarget을 통과한 화면 안 좌표만 반환하고 건설은 뒤 실제 클릭이 수행한다. 생성 후 F(70)를 확인한다. |
| F, gate70 | F. OK 뒤 자연 사격/피해를 기다린다. 필요하면 현재 적 위치 근처에 같은 실제 공급/발사기 배치를 추가한다. | 적 Priest HP 감소와 현재 MaxHP/2 미만 판정, G(71), 실제 owner0/보호막을 확인한다. HP나 owner를 직접 바꾸지 않는다. |
| G, gate71 | 현재 로컬 골렘을 실제 셀로 선택→mode5 확인→현재 적 Priest 셀 클릭 | 손 모양 커서0x0d와 같은 Graph/빈손/무력화가 필요하다. 기존 채취 골렘은 실제 반환 뒤 빈손인지 확인하거나 새 `production:133` 골렘을 합법 배치해 사용한다. |
| H, gate72 | Mission Accomplished/현재 carried Priest 메뉴 확인→`click Next Tutorial` | Tutorial4의 실제 A./Guide4/SP 등 새 미션 진입을 관찰한다. Tutorial3은 희생을 완료 조건으로 요구하지 않는다. |

원본 자산을 읽은 타입/설명: Sun Disc Thrower=`sunArcher`71, cost300/range9/foot3×3; Wind Generator=`windBattery`75, cost400/foot3×3; Golem=`sunWalker`133; Sun Workshop=`sunFactory`85; Storm Crystal=`nugget`154. 원문 라벨은 대소문자/공백을 그대로 사용한다. 현재 `RawFactoryMenu`는 `Put Knowledge into Production` 하위에 `Sun Disc Thrower`, `Wind Generator`를 작성하고, 생산 중인 종류는 하위 후보에서 숨긴다. 같은 이름을 다시 클릭해도 생산이 추가됐다는 증거가 되지 않는다.

## 공급 사슬과 카메라 후보

검토용 순서는 **(127,104) → (127,116) → (129,128) → (133,140)**, 첫 발사기는 **(137,146)** 근방이다. 이 좌표들은 배치 가능하다고 아직 실행 검증하지 않았다. 현재 연결 다리의 끝/다리 off-end 규칙·3×3 공간·실제 에너지 원을 통과하는 위치를 써야 한다. 원문의 생성기가 만드는 작은 섬 위에 발사기를 놓으면 Tutorial E의 “원래 적 섬” 조건과 달라질 수 있다.

`placement-near`/`placement-fire`는 요청 주변24칸에서 **화면 안의 합법 후보**를 찾는다. 특히 near는 원래 섬/전진 방향을 고정하지 않으므로 실제 배치 좌표를 report로 다시 읽어 공급이 앞으로 연장됐는지 확인한다. 계획 좌표를 클릭했다고 그 위치에 생성됐다고 기록하지 않는다. 생성 후 다음75 stock을 자연 보충까지 기다리며, 기존 미리보기와 새 생산 취득을 구별한다.

초기 camera(1536,738)에서는(137,146)이 화면 밖이다. 미니맵으로 실제 카메라를 먼저 이동해야 한다. **생산 미리보기 취소 및 Carrier 선택 mode2/5 해제** 뒤 클릭한다. 미니맵은 mode2/5일 때 카메라 대신 운반 이동 명령을 내므로 먼저 신전/빈 화면 등 실제 입력으로 mode0을 확인한다. 생성기 미리보기 중 미니맵으로 옮기지 않는다.

`minimap:x,y`는 세계 칸이나 퍼센트가 아니라 현재 미니맵 Gump 안의 **픽셀 오프셋**이다. 실제 목표는 `(cropX+x)*2,(cropY+y)*2`다. attempt4 초기의 crop25,13이 여전히 같을 때만 다음 예가 유효하다.

- `click minimap:40,51` → 세계 중심 근방(130,128), 중간 공급/적 북단 확인.
- `click minimap:44,63` → 세계 중심 근방(138,152), 적 사제/마지막 공급/발사기 확인.

카메라를 바꾸면 crop도 변할 수 있으므로 이후에는 현재 `minimap` report를 다시 읽어 계산한다. 같은 숫자를 반복하여 카메라가 계속 다른 곳으로 이동하는 입력을 피한다. 메뉴를 위해 원래 Workshop으로 돌아갈 때도 현재 crop 기준으로 신전/워크샾 쪽을 계산한다.

`placement-fire:137,146,15005`는 현재 SID15005에 대해 실제 사거리/필터를 통과하는71 후보만 고른다. 그러나 “적 원래 섬”이라는 Tutorial E의 지역 조건을 자동으로 제한하지 않는다. E가 계속되면 새71의 좌표와 현재 로컬 지역/생성된3×3 섬 존재를 조사한다. 단순히 적에게 사격 가능하다는 이유로 E 통과를 가정하지 않는다.

## 포획의 실제 입력과 메뉴

현재 적 사제의 관찰 좌표는(138,153), SID15005이지만 실제 이동/피해 이후에는 현재 report를 사용한다. `world:priest`는 owner1의 첫 선택 가능 객체만 찾으므로 적 선택용으로 사용할 수 없다. 현재 화면에서 **`cell:138,153`**처럼 실제 적의 현재 좌표를 클릭한다. `world:sunWalker` 역시 로컬 첫 골렘만 찾으며 어느 골렘인지 보장하지 않는다. 골렘의 현재 좌표를 report에서 읽고 `cell:`로 선택하는 것이 기존 실제 성공 경로다.

원문 G는 이미 든 Storm Crystal을 내려놓으라고 안내한다. 실제 골렘 우클릭 메뉴는 내용물 flags가 일반 운반일 때 `Carrying: Storm Crystal` 헤더와 **`Drop Storm Crystal`** 항목을 작성한다(원문 서식/스타일 제거 뒤의 보이는 이름). 입력은 우클릭 현재 골렘 셀→해당 Drop 라벨의 실제 control 확인→클릭이다. disabled/메뉴 부재를 성공으로 보지 않는다. 포획 후에는 `Carrying: Priest` 또는 현재 description의 실제 헤더를 관찰한다. Type/owner/내용물을 만들어서 메뉴를 띄우지 않는다.

골렘이 간헐천 채취를 반복 중이면 이동 목표를 바꾸기 전 실제 빈손/메뉴 상태를 확인한다. 빈 골렘을 새로 제작하는 선택도 자연133 생산·에너지/공간·현재 SP를 통과해야 한다. 포획 목표를 향해 이동하면 빈손과 같은 Graph에서 커서0x0d가 나오고 실제 move64→도착/포획79(0x4f)→원 지면 사제 제거/실제 내용물 생성→H의 증거를 구별한다. H만 보거나 현재 `object.contents count`만 읽어 운반 성공을 판단하지 않는다.

## 다음 시행의 짧은 구조

1. attempt4까지 성공한 더 긴 간헐천 연결/골렘 채취/남쪽 연결 입력을 그대로 별도 새 원시에 재사용한다. C/67을 읽고 위 실제 Workshop 생산71 세 입력을 추가한다.
2. D에서75 생산을 추가하고 E의 각 생성기1개/카메라 이동을 따로 report/snapshot으로 확정한다. 기존 무조건 반복은 추가 생성/자원 낭비를 만들 수 있으므로 새75 객체 수와 SP/stock을 보고 다음 입력을 정한다.
3. 실제 적 섬71 한 개→F 피해→G의 빈 골렘/현재 사제 클릭을 단계별로 실행한다. 첫 실패는 그 지점의 메뉴·cursor·현재 위치·SP·적HP와 stderr를 보존하고 다음 시도로 분리한다.
4. 완료 시 같은 제품 SHA·원본/자산복사본 불변·Tutorial4 실제 진입을 기록한다. 전체 캠페인/다른 튜토리얼 완주나 원본 음향/화면 동일성으로 합산하지 않는다.

실제 원문은 같은 디렉터리의 `original-tutorial3.english`, 단계 판정은 `RawTutorialLater.cpp::Three`, 실제 Factory/Carrier 메뉴는 `RawFactoryMenu.cpp`/`RawCarrierMenu.cpp`, 배치 좌표 검색은 `GameWorld.cpp::Point`, 카메라 변환은 `MinimapGeometry.cpp`다. 위 메뉴·생산·배치·포획은 모두 기존 제품 API를 재사용하며 새 제품 구현을 요구하는 성공 stub을 두지 않는다.
