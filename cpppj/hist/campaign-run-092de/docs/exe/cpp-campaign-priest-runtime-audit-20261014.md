# 실제 캠페인 월드의 사제 연결 감사

## 기준과 범위

마지막 디컴파일 PC는 **HJOW-Athlon**, 날짜는 **2026-10-14**다. 현재 PC와 일치하며 작업 전에 최신 `AGENTS.md`와 두 `LEFT_JOBS`를 다시 읽었다. 이 문서는 단위14의 공유 빌드 중 기존 소스를 읽어 다음 실제 캠페인 설치 런타임의 연결 요구를 정리한 것이다. 신규 원본 관찰은 **0개**다. 기존 Halt864, CarrierArrival406, Bridge640, 사제 월드 연결부의 관찰은 재합산하지 않는다.

단위14의 세 생산 모듈과 세 검사는 별도 범위다. 실제 캠페인 부모 `RawCampaignWorldLoad`와 Player152를 사제 런타임에 연결하는 완료 근거로 현재 합성 도착 검사를 확대하지 않는다. 10.78을 우선 복원하며 CD10.72의 가상 표·필드 폭·조건 차이도 아래에 유지한다.

## 같은 저장소에서 재사용할 공개 연결

`RawSurfaceWorld::Core()`의 풀·해시·spot·타입·프레임·SHP·장부·Factory·Owner·Pop·Unpop·PostPop·ProcessHost·Kernel을 사용한다. 별도 사제 좌표 목록이나 공간 지도 사본을 만들 필요가 없다.

| 작업 | 기존 실제 API | 실제 런타임 연결 조건 |
| --- | --- | --- |
| 사제 소유자 | `RawPriestWorldBindings::SetOwner` 또는 `MakePriestOwnerDispatch` / `RawPriestOwner::Set` | 사제 실제 VT만 파생 몸체로 분배하고 다른 자산은 기존 소유자 분배기를 보존한다. |
| 첫 Pop 전체 후처리 | `RawPriestWorldBindings` 생성 시 `SetPriestPostPop` 등록 | 접두→Carrier→공통 장부→지면/HP→낙하/보호막 전체를 같은 풀에서 실행한다. |
| 첫 회복 예약 | `BindProcesses(core.processes)` | Pop 전에 연결한다. 첫 flags1 또는 저장 객체 Pop9는 실제 event `0x25a`를 같은 Host에 예약한다. |
| 회복·낙하·보호막 사건 | `RawPriestWorldBindings::RegularHandler` | 기존 자산 handler를 fallback으로 합성한다. 미복원 사제 사건을 payload 반환으로 덮지 않는다. |
| HP 변경 | `RawPriestWorldBindings::SetHitPoints` | 실제 사제 반절 HP 전환의 Unpop/Repop을 사용한다. 일반 자산 직접 HP 쓰기로 대체하지 않는다. |
| 지면 확인 | `RawPriestWorldBindings::CheckCarrier` | 기존 genus 제외·현재 지면·낙하 분배를 유지한다. |
| 표면 붕괴 낙하 | `RawPriestWorldBindings::FallWalker` | 같은 프레임/Host/보호막/실제 공간에 연결한다. 다른 walker도 실제 fallback이 필요하다. |
| 삭제 접두 | `DestroyHooks`와 `BindDamageable` | 실제 사제 목록/보호막→Carrier→Damageable 전처리를 사용한다. 마지막 전체 PostDestroy는 별도 기존 실제 분배에 연결한다. |
| 사제 해방 배치 | `BindPlacement` / `Spawn` | 기존 132개 나선 후보의 실제 배치 검사·생성·Pop을 재사용한다. |
| 이동 프로세스 선택 | `MovementProcessType` / `SelectMovementProcessType` | 실제 mutable 선택 상태를 공유한다. 사제 자산158은 프로세스54이며 두 번호를 혼동하지 않는다. |
| 부모 Halt | 새 `RawPriestHalt::Halt` | 기존 `RawCarrierCheck`, 사제 HP, Reward MaxHP, Finder와 실제 공간을 공급한다. |
| 부모 Arrival | 새 `RawPriestArrival::Arrival` | 같은 CarrierArrival·CanonTerrain·IslandRuntime·Footprint·ContainedCreate를 공급한다. |

사제 월드 연결부의 생성 순서는 **같은 Core 확보→연결부 생성→기존 ProcessHost 연결→Damageable/Placement 연결→runtime hook 설치→실제 저장 객체 생성/Pop**이다. `ValidateReady()`가 Host/삭제/배치 연결을 확인한다. 종료는 외부 연결부가 해제되기 전에 같은 Kernel의 `ClearProcesses()`를 실행해야 한다.

## 필요한 최소 표면 월드 API

현재 `RawWorldRuntimeHooks`는 자산 삭제·Regular·Form 송신·walkerFall·loadingDepth를 받으며 **소유자 callback은 없다**. `RawSurfaceWorld.cpp`의 `objects.setOwner`는 생성 시 일반 `MakeOwnerDispatch`를 저장하고, 다리 사건도 이 함수 객체를 복사해 사용한다. `RawWorldCore::owner`만 직접 부르면 사제 가상 `+0x74`의 원래 소유자 규칙을 지나치게 된다.

root가 표면 월드를 변경할 때 좁은 소유자 callback 하나를 runtime hook에 추가하고 기존 `objects.setOwner`가 호출 시 현재 runtime callback을 선택하게 하면 된다. callback은 사제에 대해 `RawPriestWorldBindings::SetOwner`를 실행하고 나머지는 현재 표면/섬 소유자 분배를 사용한다. callback 안에서 같은 외부 `objects.setOwner`를 재호출하면 재귀가 되므로, 기존 일반 분배기를 명시 fallback으로 보관해야 한다. 새 사제 Owner 몸체나 장부 쓰기는 필요하지 않다.

`RawFortObjectLoadHooks::setOwner`에도 같은 가상 분배를 공급한다. 실제 저장 객체의 flags9 Pop 전에 소유자·HP·프레임이 설정된다. 이 순서는 실제 타입158 생성자/가상 표를 선택한 뒤 유지해야 한다.

## 소유자와 로딩 상태

사제의 **raw WORD `+12` low7 원래 소유자**, **그 WORD bit7 로컬 표식**, **현재 owner BYTE**는 서로 다른 필드다. 현재 owner는 10.78 `+34`, CD10.72 `+32`이며 `+13` high BYTE는 사제 소유자 몸체가 보존한다.

`RawPriestOwner::Set`은 비전투 또는 loadingDepth!=0에서 원래 소유자를 바꾸고, 전투 중 loadingDepth==0에서는 이미 저장된 low7을 다시 사용한다. 패치 로딩의 player0은 원본 assert 경계다. 실제 Player의 battle owner, Template 슬롯의 기본 owner, fort local player를 동일 값으로 가정하면 안 된다.

`RawPriestWorldBindings::SetLoadingDepth(depth)`는 Owner·Shield·Carrier의 깊이와 Regen의 `neutralBlocked=depth!=0`를 같이 갱신한다. 새 `PriestArrivalState::loading`도 동일 깊이의 0 아님으로 동기화한다. `SetAuthority`는 HP/Fall/Carrier/Contained 권한을 갱신하므로 새 Halt/Arrival/Path 권한도 같은 원본 상태를 읽게 한다. Halt의 battle 분기는 authority와 별개다.

`SetLocalPlayer`는 현재 연결부의 fortPlayer와 battlePlayer를 함께 지정한다. 실제 두 전역이 다른 경우 공개 `RawPriestWorldState::owner`의 각 필드에 별도 실제 값을 전달한다. 새 불필요한 setter나 공통 상수 변경은 필요하지 않다.

## 첫 Pop 이후 실제 Regular

첫 Pop의 회복 payload는 **`float((TypeHP/6을 정수 절삭한 값)*50.0)/MaxHP`**이며 단순 고정 주기가 아니다. 원래 목록과 ownerState도 같은 `RawPriestWorldState::priests`를 MissionOutcome까지 공유한다.

10.78의 `RawPriestRegen::Handle(0x25a)`는 HP가 이미 가득해도 `Next(9)`와 추적 sequence를 갱신하고 **패치 `00491680`을 항상 호출한다**. 이 몸체는 기존 `ScrambledSpStore::Rotate()`로 이미 복원돼 있다. 같은 SP 저장소와 난수를 재사용하고 `state.regen.patch.observedSequence`에 실제 `store.Offset()`을 공급한다. 기존 `RawPriestWorldBindingsTests`가 이 연결을 사용한다. full HP를 이유로 patchAudit에 빈 함수를 넣으면 실제 SP/추적 효과가 사라진다. CD10.72에는 이 패치 경계가 없다.

회복이 필요한 경우 가상 refresh `+0x88`과 DamageableRefresh가 호출된다. 기존 연결부는 free 또는 실제 `damageEffect==0`을 자연 반환하고 nonzero에서는 caller의 전체 Burning 처리를 요구한다. `shape`는 그림 번호가 아니라 실제 이동 ProcessForm 번호이며, `occupied`는 같은 `RawContainedLookup`이 현재 부착 Form을 조회한다.

중립 사제가 반절 HP 이상이고 두 차단 전역이0이며 이동 Form이 없으면 `00491db0/CD0040d030`의 실제 중립 귀환이 호출된다. 상태를 임의 차단하거나 빈 callback을 주어 첫 회복 완료로 세면 안 된다. `remaining!=0`인 패치 측정은 `0040ee90`의 실제 측정 결과가 추가로 필요하다.

## 실제 데이터 지원

단위14의 같은 Host 검사는 **1×1, HP200, owner1, 지면6, 비포획, AI없음, 다이스없음, 수동 두 점 경로**다. 실제 캠페인 연결에서는 해당 합성 타입 표를 사용할 수 없다.

| 자료 | 현재 공개 공급 경로 | 런타임에서 유지할 값 |
| --- | --- | --- |
| 타입 | `GameAssets::TypeTable().Types()` | 판본의188/171개 원래 번호, flags1/flags2/genus, footprint, maxHP, cost, group, content/container mask, damageEffect, 실제 constructorAddress |
| 프레임 | `PriestPlacementAssets::frames` | 각 실제 `.type` FrameTable과 defaultFrame, 걷기 방향 A~P 및 낙하 J/지면 E의 실제 시작 번호 |
| 표시·배치 SHP | `PriestPlacementAssets::shapes` / `SquidRenderer::Shapes` | 원래 hotspot과 SHP 크기/추가 헤더; 프레임 번호만 맞춘 가짜1×1 그림 사용 금지 |
| 이동 속도 | `RawPathStartHooks::metadata` | 실제 `.type`의 `speed`와 `turningSpeed`; 현재 RiftTypeRecord에는 이 두 필드가 없으므로 caller의 자산 조회 또는 별도 같은 metadata 저장소를 사용한다. |
| 지도 | Core의 spot와 `hash.Entries(0)` | 지면6/통행/막힘과 실제 surface SID, 지도 사본을 쓰지 않는다. |
| 도착 대상 | 실제 Player·Island·Graph·현재 Form | AI 포인터 존재, 방향을 보존한9×9 동맹표, mutable 타입 번호, 같은 Host의 현재 Path 조회 |

사제 VT는 **10.78 `0050f210`**, **CD10.72 `005003e0`**다. 해당 판본의 `TypeConstructorAddress`를 로더가 공급해야 실제 Factory 생성자의 후속 가상 Pop/Unpop/Owner 분배가 성립한다. HP 변경은 사제 DWORD/CD signed WORD 규칙을 유지한다.

## 아직 선택되면 실제 구현이 필요한 경계

- Halt의 큰 carrier pickup `00429ed0/CD004e7980`와 패치 nugget transfer `0042a0d0`는 실제 큰 하위 몸체다. 단위14 원본864에서는 필수 callback의 호출/반환/변경을 대조했으며 포획 몸체 전체 완료로 세지 않았다.
- CarrierArrival의 큰 pickup `00428e00/CD004e5800`, escape `00429010/CD004e5c10`, 후보선택/새 이동요청, 너겟 mined packet와 CD SP/owner packet/위치 음향은 별도 실제 경계다. 원본406의 선택 범위는 이를 호출하지 않거나 명시 입력으로 공급한다.
- CD의 `priestPickupPriest`는 실제 named config `004a9400` 조회다. carrier가 사제일 때만 현재 설정을 읽는 순서를 유지한다.
- 중립 귀환 `00491db0/CD0040d030`은 현재 섬 Temple을 찾고 저HP/동일 소유자일 때 타입 speed를 잠시 두 배로 바꿔 Temple 이동을 시도한다. 두 PE 모두 저장 전 speed를 CRT `ftol`로 signed DWORD 절삭해 두고 이동 요청 뒤 `FILD`→float 저장으로 복구하므로, 소수 속도를 그대로 되돌리는 동작이 아니다. 실패하면 현재 좌표 주변 ±20 난수를 뽑아 섬 존재/같은 지역 조건에 맞는 이동을 요청한다. 비교 getter는 `0046f530/CD004bdc50`의 현재 지역 번호이며 Graph 번호가 아니다. 현재 module에 전체 복원은 없으며 임의 가까운 신전 선택으로 대체할 수 없다. 실제 mutable speed 저장소 연결은 이 후속 몸체에서 별도 다룬다.
- 낮은 HP/이동불가/지면없음 첫 Pop은 실제 사제 낙하·보호막 생성/Pop·소리·로컬 안내를 선택할 수 있다. 기존 `RawPriestWorldBindings`가 몸체를 묶지만 보호막 Pop/음향/UI caller의 실제 연결까지 필요하다.
- 기존 RawPathStart 거의올림 bias의 경계 차이는 단위14 완료 근거에 섞지 않는다. 경계 입력을 포함한 별도 원본 관찰과 이전 전체 source lineage로 후속 교정한다.

이 문서는 연결 설계와 미완료 경계를 기록한다. 게임/GUI 실행, 전체 캠페인 설치·걷기·포획·운반·의식 완주, UI/음향 완료를 주장하지 않는다.
