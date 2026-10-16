# 실제 Fort 초기 월드 설치의 연결 경계 감사

## 현재 결론과 출처

**실제 캠페인 설치 부모는 `PATCH004b5300/CD004322e0`다.** 미션의 Packet0x27 새 로그인 처리기가 이를 호출한다. 단위13의 저장 부모 `PATCH004b5f70/CD004330c0`는 별도 Fort 메뉴 State 경로의 부모다. War Begins stage1은 State7→Battle/packet 전송 경로를 요청한다. 기존 부모534행 및 같은 풀의 선택 Fort 소비자1개/Builder 골든2개의 성공을 실제 캠페인173개(저장172+별도Player152) 설치 완료로 확대하지 않는다.

이 문서는 현재/마지막 정밀 PC **HJOW-Athlon, 2026-10-14**의 `extracted/refined/originals/raw.c`, `extracted/refined/originalCD/raw.c` 및 각 좁은 정밀 내보내기를 읽은 정적 감사다. 기준 커밋은 단위12 `db9b626`이며 이후 단위13 새 연결 파일과 root/gun이 전담한 shared successor는 별도다. 이번 감사 자체의 새 PE 실행/GUI/게임 실행은0이다. 아래 최초 간헐천의 신규 정상168/미복원 거부436은 별도 보고서다.

## 실제 미션 State/Guide/AI 순서

| 단계 | 10.78 | CD10.72 | 확인한 실제 순서/전제 |
| --- | --- | --- | --- |
| 미션 stage1 | `004c3290` | `0040f6e0` | Templates clear→tutorial/mission번호→`loadFort` 문자열→미션 맵 요청→기술 허용 |
| 저장 맵 요청 | `004b51e0` | `004320a0` | 현재Fort 이름 설정→가상 Zone packet `0x82000004`/4명→pending State7 |
| pending State 실행 | `004b88c0`→`004b6dd0` | `00430200`→`0042d630` | pending를 current로 복사→pending0→State를 동기 실행 |
| State7 reset | `004b2df0` | `0042f5c0(1)` | owner별 기존 AI 삭제→Kernel/SID/섬/청크/Graph/Player reset |
| owner별 AI 삭제 | `00413ae0` | `004d7db0` | AI 해제 후 실제 Player `+b0/CD+a8=0` |
| Player9기록 재초기화 | `004910a0` | `00406200` | `9*0xb4/CD9*0xac` 전체0→목록/동맹/firstRegion 초기화 |
| State7 Zone 정보 설치 | `004b4be0` | `00431990` | State7의 저장 이미지가 존재할 때 Zone/Subscriber 정보 설치. 이 몸체의 AI 생성은 확인하지 못함 |
| Guide 실행 | `00463f70`→`00463bb0` | `004175e0`→`00417210` | pending State0과 GUIempty인 때만 단계 본문 실행, 아니면 가상+20 재지연 |
| pending 검사/GUIempty | `004b32e0`/`004c8e90` | `0042fa30`/`004c3940` | 한 ready bool로 임의 합치지 않는다 |
| 미션 stage3 AI 준비 | `004c2b20`→`004c2d40` | `0040f840` | myStartMoney/myTech→Player1..8 순회→사제 존재→AI 기본값/전역·owner 파라미터→AI 생성 |
| 현재 사제 존재 | `004921f0` | `0040d940` | 실제 로드된 사제 목록을 조회한다 |
| AI 생성/부착 | `00417e50`→`00416860`→`004911a0` | `004d7720`→`004e2950`→`004062f0` | 현재 AI0이고 noTemple 파라미터0일 때 생성→같은 Player 포인터/flags 갱신 |

PATCH 메인 루프 `00438dc0`/분리 함수 `004395df`와 CD `00485b20`은 **State update 후 Guide 실행** 순서를 가진다. State 함수가 pending0으로 바꾼 순간을 독립 ready 완료로 해석하면 안 된다. 맵 전송/설치 동안의 후속 State와 진행 GUI도 Guide의 조건이다. 정적 근거는 정상 새 미션 reset 직후 AI 포인터가0이며, 아래 저장173개 설치와 진행 GUI 종료 뒤 stage3가 사제의 존재를 확인하여 부착한다는 것이다. load 중 Guide를 중첩 실행하거나 이미 부착된 AI를 보존하는 별도 경로의 성공을 주장하지 않는다. 실제 소켓/GUI 실행과 시점 관찰은 아직 하지 않았다.

첫 간헐천 PostPop은 `00416720/CD004d8670(sid,1)`으로 현재 Player1..8의 AI 포인터를 조회한다. 새 `RawGeyserAiRegistration`은 같은9기록의 null AI를 실제 순회하며 nonnull 하위는 명시 거부한다. Packet0x5b 채집의 `RawGeyserMined`로 대신할 수 없다. [정상168/미복원거부436의 독립 자료](cpp-geyser-ai-registration-20261014.md).

## 저장 부모의 완료 범위와 캠페인 차이

기존 복원 저장 부모는 Builder ctor/attach(owner1/bounds1..15)→Territory120→PlaceAll→Generate→ObjectLoad→Money→Badges→Technology→Graph(false)다. 대체 분기는 island0 Exists일 때 PopulateIsland0→Graph(false)다. 각 하위 콜 전에 **현재 Template owner를 다시 읽는다**. 부모 원본534행은 lower Generate/Objects/Money/Meta/Populate/Graph 호출 경계를 관찰했고 전체게임/하위완료 근거가 아니다. 선택 C++ 소비자는 실제 기존 몸체들을 같은 풀에 연결하지만 실제 캠페인172 저장자료를 입력하지 않았다.

### 실제 War Begins 저장맵 설치와 Ready

Mission base ctor `00482bf0/CD00480b00`은 각각 **`00594fa0=1,00594fa4=1` / `0052e16c=1,0052e170=1`**을 설정하고 `global.inMission` 문자열을 쓴다. 두 원본 mode 전역을 하나의 임의 bool로 대체하지 않는다.

State7은 Zone 이미지 설치→local 전송(server/client)의 로그인으로 진행한다. Packet0x27 handler **`004a7800/CD004c1a20`**은 subscriber가 같은 현재 Player를 찾고, 이전 Player+50 bit2(`>>1&1`)를 저장한다. 이 값이0인 **새 로그인**에서만 실제 저장맵 부모를 호출한다. 콜 전 `004a6c50/CD004c12e0`은 server+현재local Player일 때 현재 Fort 이름의 파일을 해당 Template에 읽는다. 다른 owner/클라이언트는 전달받은 Fort image 경로를 사용한다.

| 실제 사슬 | 10.78 | CD10.72 | 필요한 의미 |
| --- | --- | --- | --- |
| Mission mode 설정 | `00482bf0` | `00480b00` | 두 mode 전역을 모두1로 설정 |
| Packet0x27 새 로그인 | `004a7800` | `004c1a20` | server·현재 subscriber/Player·기존 bit2 검사 |
| 현재 Fort Template 준비 | `004a6c50`→`004bf550` | `004c12e0`→`00427230` | 자기 server인 경우 current Fort 파일, 다른 경로는 실제 image |
| 미션 저장맵 부모 | **`004b5300`** | **`004322e0`** | mode fa0/CD52e16c!=0인 새 로그인 분기에서 호출 |
| Player152 첫 생성 | `00490250` | `00407400` | 현재 battle localOwner로 flags2 생성/Owner/frame0/Pop0 |
| Builder ctor/attach | `0046d9b0`/`0046da20` | `00490430`/`004904d0` | 같은 ChunkMap, Builder owner1, 원본 기본 bounds |
| Territory120/PlaceAll | `004bd660`/`0046dd70` | `00427c00`/`00490980` | zero120 후 현재Templateowner 선택·0..20 |
| Generate/Objects | `0046da70`/`004be610` | `00490530`/`00428d50` | 지형 먼저→실제Terr0..19/Chaff 저장172개 |
| 마지막 Graph/사제준비/Camera | `00463110(0)`/`004923e0`/`004340d0` | `0045bd60(0)`/`0040daf0`/`00488f50` | `923e0/40daf0`의 큰 두 분기는 inMission일 때 자연 반환 |
| 끝 이름 복사 | currentPlayer `+0xc`←`00540ca8` | currentPlayer `+0xc`←`0050f6d0` | 실제 현재 이름의 NUL까지 복사 |
| 로그인 Ready 표시 | currentPlayer `+50 |=0x40` | 동일 | 실제 부모 설치 후 handler가 갱신 |
| login 완료/GUI 종료 | `004b67c0`→`004c8d90` | `004336e0`→`004c3820` | 활성timer·connected/pending/watchers 또는 Begin 수신 조건→사제상태→timer 중지→진행GUI 종료 |
| 이후 AI/스크립트 SP·기술 | `004c2b20` | `0040f840` | Guide pending0/GUIempty→myStartMoney/myTech→사제확인→AI 부착 |

실제 미션 저장맵 부모는 **Money/Badges/Technology를 호출하지 않는다**. 해당 내용은 Fort 메뉴 부모나 비미션/다른 owner metadata 경로와 구분한다. 실제 미션 시작SP3000/myTech는 stage3의 별도 스크립트 몸체다. 저장Fort의 Money100000/Technology26BYTE를 이 부모에 임의로 연결하면 원본에 없는 효과를 추가한다. 반대로 Player152 생성 및 저장 객체 내부 Contents는 실제로 실행해야 한다.

Packet0x2a의 `004b3d30` owner별 FortDataPacket→Builder 배치와 Packet0x72의 `004b3bc0` 중앙섬/archipelago는 일반 Battle의 별도 경로다. 실제 중앙 이름이`$`일 때만 archipelago이며 `SURROUNDED.FORT`도 그 내부 조건이다. 이름 War Begins만으로 이 분기들을 선택하지 않는다. 초기 자신의 미션 설치는 Packet0x27의 mode 분기에서 확인했다.

Ready는 Player+50의0x40, 원본 login timer/Begin 수신, pending State 및 GUIempty의 별개 조건이다. `004b67c0/CD004336e0`은 mission fa4/52e170 또는 Begin(c7a74/CD53da04) 조건에서 owner1..8의 사제상태 `00495060/CD0040f0e0`를 지정한 뒤 진행 GUI를 닫는다. 추가 초기 간헐천 생성 `00486440/CD00415a30`은 fa0/CD52e16c==0 조건이어서 실제 미션에서 선택하지 않는다. 이런 자연 반환을 missing callback fallback과 구분한다.

새 좁은 spec `tools/ghidra/campaign-world-load-functions.json`은 실제부모2개와 로그인 선택 감사2개를 별도로 준비한다. 기존 generic534행과 같은 실행 관찰로 합산하지 않는다. **후속 완료 목표는 실제 이 부모/Player 및 같은 저장173개 전체 raw/장부/두 난수/예약의 독립 대조**다.

여기서173은 저장 객체172와 별도 Player152의 입력 대상 수다. 앞서 생성되는 지형/edgeFarm이나 실제 부착되는 ContentForm/ProcessForm/Regular를 제외한 최종 Pool 크기라고 해석하지 않는다. 이 추가 슬롯/목록/예약도 같은 원본 대조의 출력으로 보존해야 한다.

## 같은 월드가 장기 소유해야 할 현재 자료

| 실제 구분 | PATCH | CD10.72 | 현재 연결 시 주의 |
| --- | --- | --- | --- |
| 현재 Template owner | `00540cac` | `0050f6d4` | 저장 스트림/Template 선택용. Builder owner1과도 별개 |
| 현재 Player/battle owner | `00540c70` | `0050f6c8` | 사제 소유자/Player Technology/AI/GameRandom 소비 정책 |
| Template 배열/stride | `005c89f0` /20 | `00564bd0` /20 | 각 하위 효과 뒤 현재 owner 재조회 |
| Player 배열/stride | `0059534c` /0xb4 | `0050f834` /0xac | Money flags+50/기술 SID/AI pointer가 같은 실제9기록을 읽어야 함 |
| playerKnown | `00595344` | `0050f828` | Technology getter의0 자연반환 전제 |
| Player Technology SID | `+0x9c` | `+0x94` | 원본 Player152 Pop이 기록한다. parent0 fallback 금지 |
| Template loadingDepth | `005c89b8` | `005178d4` | Money/Meta/Carrier/Priest/Asset의 중첩 깊이 |
| 공통 Pop bookkeepingDepth | `005c8488` | `00539600` | 공통PostPop 감소 카운터. Template depth와 별개 |

`RawPriestOwner::PriestOwnerState`는 fortPlayer와 battlePlayer를 이미 분리한다. 그러나 현재 `RawPriestWorldBindings::SetLocalPlayer`는 두 값을 같이 지정한다. 신규 실제 composition은 두 원본 전역을 개별 공급해야 하며, 한 값을 둘에 반복 복사하는 초기화는 새 근거 없이 사용하지 않는다. 원래 owner BYTE와 Runtime 현재 owner DWORD도 별개다.

`RawWorldCore`의 Pool/Hash/spots/types/frames/shapes/Factory/Owner/Frame/Pop/Unpop/Post/장부/Graph/Forms/ProcessHost/Kernel을 참조로 재사용한다. `RawIslandRuntime`의3204BYTE와 현재 ChunkMap도 같은 저장소를 장기 소유한다. `GameRandom` 한 인스턴스는 FrameChoose/객체 랜덤프레임/SP 등 전역 난수 소비를 공유한다. 섬 성장/edgeFarm local seed 난수와 별도 CRT holdrand/cache/currentTypeShape의 상태를 하나로 합치지 않는다. 첫 Regular 전에 `Core.processState.now`에 실제 current game time을 공급한다. payload는 최초 실행 지연과 다른 필드다.

## 실제172 객체와 필수 생성/Pop 경계

대상 저장자료는 보호 `originals/netstorm.tarc`의 `\\d\\thewarbegins.fort`다. 이전 원본 포맷 감사의172는 Terr00의4개, Terr01의23개, Chaff145개다. Terr0..19 파일순→허용된 Chaff 순서가 실제 ObjectLoad이며 currentCtor/QA/QB/Owner/HP/Pop/Contents 효과가 즉시 뒤 객체에 영향을 준다. SHP/타입 플래그/초기 전역·두 난수·초기 풀 상태까지 같은 입력에서172 전체 raw/장부/예약을 대조하기 전에는 캠페인 동일성 완료로 세지 않는다.

| 대상 | 기존 복원/재사용 | 아직 필요한 설치/미복원 경계 |
| --- | --- | --- |
| 사제158×2 | 실제 ctor/Owner/HP/Priest PostPop/같은 Host 경로 | 동일 raw 사제 장부·firstRegion·보호막/경로·전송·실제 SHP를 상위 전역에 연결 |
| 거주 구역89×2 | `RawAssetPostPop::Residence`, `RawAssetRegular`, FinishOwner | 같은 residences 목록/Player fields/현재 지역·예약. 목록의 신전을 거주 구역으로 취급하지 않음 |
| 신전123/124×1씩 | Vortex 실제 ctor/Asset PostPop/Regular/지역·섬setter | 실제 impactSound/typeDword, 동일 Runtime·목록·Player; 내부 contained 슬롯은 별도 실제 Form |
| 간헐천122×14 | 실제 ctor/Asset PostPop/Regular, 새 nullAI 등록 | nonnull AI 거리/목록/후속 등록은 아직 거부, 실제 주변 받침 Unpop/Repop·장부를 연결 |
| 썬 디스크 발사기71×9 | 실제 ctor/Gun PostPop/RawGunProcess/Archer animation | 실제 GunProcess 생성/부착/초기 Frame/SHP/두 난수/전송·목록. 일반 공격 이후 모든 handler 완료와 구분 |
| 윈드 배터리75×3·썬 블로커78×8 | 실제 ctor/Asset PostPop 및 공통 장부 | 실제 타입 metadata와 로딩 depth로 선택된 경계를 확인. 비로딩 blocker start와 현재 초기 선택을 구분 |
| 다리82×15·noIsland157×117 | 같은 Core의 실제 표면/Owner/Frame/Pop/Hash/Graph | Chaff 저장순 그대로 설치, 수동 배열 정렬·ground support 대입 제거 |
| contained sunWalker133 | `RawContainedCreate`/ContentFormHost/Lifetime/같은 Form dispatch | Form6의 storedType WORD이며 공간 walker133 생성으로 대체하지 않음 |
| Player152(172 밖의 사전 객체) | ctor491660/CD407950의 Recipe는 이미 존재 | 생성 wrapper490250/CD407400 및 PostPop4905d0/CD407cc0의 실제Player SID 기록·event1/payload5.0f 예약 미복원 |

Technology getter `0048fc60/CD00406d90`은 playerKnown이0이면0, 그렇지 않으면 battle localOwner의 실제 Player+9c/+94를 읽는다. 이 SID는 Player152 첫 Pop에서 기록된다. 초기 Fort 메뉴/별도 자신의Fort 경로는 이 Player 생성 후 Technology를 읽는다. **실제Technology26BYTE를 parent0으로 강제하면 필요한 Form 생성/장부를 생략한다.** 실제미션의 Player152 사전 생성/현재PlayerKnown 순서를 Battle/packet caller에서 다시 확인해야 한다.

초기172에는 sunAviary72/Factory85/Outpost126/sunCannon128 공간 객체가 없다. 이것을 전체게임의 해당 타입 미필요로 확대하지 않는다. 타입152 생성자는 기존 Recipe를 재사용하고 새 PostPop을 공통Post만으로 덮지 않는다. 저장 QA129/2는 일반 레벨setter와 다른 WORD12 낮은BYTE다. 가상+84 level setter `004ad110/CD004ace20`를 임의 raw 대입으로 공급하기 전에 실제 type/version 분기가 선택되는지를 확인한다.

## 기존 GameWorld 우회와 최소 후속 API

현재 `GameWorld.cpp`는 벡터 IslandBuilder/IslandList→Chaff 우선 값 객체→Terr 값 객체→수동 HP/defaultFrame/ground 점유를 만들고, 지형/noIsland/다리를 seeds로 변환해 `RawSurfaceWorld::Load`에서 다시 정렬한다. 비표면 객체는 여전히 값 벡터의 `AdvanceObjects`로 진행한다. CRT 고정seed/preconsume와 좌표hash 프레임 선택도 실제 current GameRandom/CRT와 다른 초기 입력이다. 이 경로를172 동일성 근거로 사용하지 않는다.

신규 composition은 첫 Create 전에 모든 runtime/form/Post/regular/owner 연결을 설치한다. `MakeTerrainStampWorldHooks`, `RawNormalTerrainWorldBinding`, `RawFortObjectLoad`, `RawFortMoneyLoad`, `RawFortMetaLoad`와 기존 일반HP/Frame/Pop을 같은 Core에서 재사용하고 selected 필수 경계에 noop/default 효과를 넣지 않는다. Render/선택/카메라의 읽기 어댑터는 raw current SID를 조회하고 값 벡터를 두 번째 게임 상태로 유지하지 않는다.

현재 `RawSurfaceWorld::FinishLoad`는 `Graph::Rebuild(true)`를 추가한다. 저장 부모의 마지막 `Rebuild(false)` 뒤 이를 호출하면 원본에 없는 두 번째 재구성을 실행한다. 실제 미션/상위caller가 확정되면 **load flags/depth만 종료하는 최소 API** 또는 원본이 요구한 마지막 Graph 인자를 받는 API를 root가 whole-lineage로 추가해야 한다. Template depth와 공통Post bookkeepingDepth를 같은 정수로 지정하지 않는다. 모듈 해제 전 `ClearProcesses`로 예약을 정리해 Host/Kernel이 참조하는 모듈이 먼저 파괴되지 않게 한다.

이 감사의 남은 최우선은 실제 저장맵 Battle 설치 사슬, Player152, 선택된 타입 metadata/SHP·실제 Factory/Pop 연결의 독립172 전수다. 범위 검사나 기존 선택 소비자 성공만으로 이 전체 결과가 원본과 같다고 완료 처리하지 않는다.
