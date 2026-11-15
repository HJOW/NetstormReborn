# 재기준선 신규 장기 월드 저장소 29단위 판정 (4-R ②-A)

기존 기준선 `1f00a25a`에는 없고 후보 `225553df`에 추가된 `client/RawSurface*` 29단위를 검토했다. `RawSurfaceWorld.h`의 같은 `RawSurfaceStorage` 멤버와 각 구현의 본문·`RawSurfaceWorld.cpp`의 Runtime 바인딩을 근거로 한다. 이들은 LAN 전용 세션이나 Edit UI가 아니라 단독 미션이 사용하는 Client 수명의 물리·월드 저장소를 구성하므로 **29개 모두 port**다. 개별 경로의 도달 여부는 소비자 이식 때 다시 확인한다. 전투·의식 등 뒤 단계에 필요한 멤버는 각 소비자 단계에서 연결하고, Tutorial1 시작에 필요한 부분만 4-D에서 먼저 옮긴다. 아직 이식하거나 세션에 연결했다는 뜻은 아니다.

각 행은 `RawSurfaceStorage`의 실제 메서드 이름과 해당 파일의 기능을 기록한다. 그 외 신규 단위 147개와 이미 이식한 변경 37단위의 판정이 남아 있으며, 기준선 전환과 `PORT_MAP.tsv` 재생성은 이를 마치기 전에는 하지 않는다. 기준선 소스는 고정 커밋의 판을 사용한다.

두 커밋의 단위 목록을 비교해 29개가 모두 신규임을 확인하고 각 파일의 저장소 메서드와 아래 기록을 대조했다. 현행 대응표 검사는 984행·오류 0건이며 Node v22.23.3·v24.22.3·v26.13.0 전체 검사는 각각 4,966건 통과·실패 0이다. 이 단위에서 제품 코드나 게임 실행은 변경하지 않았다.

| 신규 단위 (`cpppj/src/client/`) | 실제 저장소 메서드 | 기능 근거 |
| --- | --- | --- |
| `RawSurfaceAssetDeletion` | `BindRegionLifecycle`, `EmitDefaultAssetEffect`, `NotifyAssetDependency` | 현재 자산의 실제 VT+14/+18/+34를 같은 장기 삭제 부모에 연결한다. |
| `RawSurfaceAssetNotice` | `NotifyAviaryHome` | 실제 가상34의 세 홈 통지를 같은 장기 물리 저장소와 현재 Host에 연결한다. |
| `RawSurfaceAttack` | `AcidFence`, `AcidHeader`, `AcidState`, `AcidTypes`, `AcidZap`, `ActiveSounds`, `ArcSpire`, `AttackDamageRate`, `BindAttackRuntime`, `BindAttackSound`, `ConstructAttackRegular`, `ReserveAttackRegular`, `SoundState`, `WindFlyer` | 아크/애시드의 전체 정기·검색은 장기 물리에서 실행하고 큰 빔/피해/분해만 실제 Runtime을 빌린다. |
| `RawSurfaceBalloon` | `BalloonBody`, `BalloonRegular` | 벌룬의 그림자와 실제 Carrier 부모를 World·Session보다 오래 같은 저장소에 보유한다. |
| `RawSurfaceBomb` | `BindBombRuntime`, `BombCastTimes`, `BombPostPop`, `BombRegular`, `BombSearchReservations`, `BombSpawns`, `BombState`, `HasBombRegular` | 공통 주문의 실제 정기 본문과 현재 TYPE/권한을 Native Client 수명으로 보유한다. |
| `RawSurfaceCarrier` | `BindCarrierRuntime`, `CarrierRegular`, `HasCarrierRegular` | Carrier의 내용물/현재 TYPE와 미리보기 수명을 실제 Client 물리에 연결한다. |
| `RawSurfaceContent` | `BindContentDevices`, `BindContentRuntime`, `ContentForms`, `ContentLifetime`, `ContentState`, `ContentType`, `NoticeContent` | 내용물의 정렬·Regular·삭제를 같은 Client 풀/공장/현재 전역과 실제 장치에 연결한다. |
| `RawSurfaceDaisDeletion` | `BindDaisFinish`, `DaisDeletion`, `PriestParticipation` | 다이스·제단 삭제를 같은 장기 Pool/해시/현재 역할과 실제 Client 선택 효과에 연결한다. |
| `RawSurfaceDamageable` | `BindCarrierReplenishment`, `BindDamageableDevices`, `BombPre`, `CarrierPre`, `DamageablePost`, `DamageablePostGlobals`, `DamageablePre`, `DamageablePreGlobals`, `DamageableRelease`, `DamageableReleaseGlobals`, `FlyerPre`, `GeyserLifetime` | 실제 피해가능 객체 전/후처리와 간헐천 고유 수명을 같은 장기 물리/전역에 연결한다. |
| `RawSurfaceDecoration` | `BindDamageableRuntime`, `DecorationRegular` | 장식/가장자리 팜의 현재HP·권한과 선택된 가상98를 같은 장기 물리에 연결한다. |
| `RawSurfaceDeletion` | `BindDeletionDevices`, `BindDeletionRuntime`, `CommonDeletion`, `RestoreDeletionGlobals` | 공통 삭제의 현재 장부·Graph·보상을 Client 수명으로 공유하고 파생 종단은 짧게 빌린다. |
| `RawSurfaceDepend` | `BindDependRuntime`, `DeliverDependNotice`, `DependForms`, `NotifyDependency` | 감시 쌍·현재 Form 공장/TYPE·가상 삭제를 같은 Client 물리에 장기 연결한다. |
| `RawSurfaceDudeRegular` | `BindDudeRuntime`, `DudeAssert`, `DudeBehavior`, `DudePostPop`, `DudeRegular`, `DudeTask`, `SetDudeWord` | Dude의 순수 정기/지면/task는 장기 저장소를 공유하고 큰 이동만 현재 Runtime을 빌린다. |
| `RawSurfaceFactoryRegular` | `BindFactoryRegularRuntime`, `FactoryPostPop`, `FactoryRegular` | 두 Factory VT의 순수 프레임/난수는 장기 현재 상태를 쓰고 큰 효과만 현재 Runtime을 빌린다. |
| `RawSurfaceFence` | `BindFenceAlliances`, `FenceLifetime`, `FenceLine`, `FenceMarkerClear`, `FenceMarkers`, `FenceNeighbors`, `FencePostPop`, `FenceRebuild`, `FenceRegular`, `FenceShields` | 썬 펜스의 전체 이웃·연결선·표식·재생성·수명을 같은 장기 물리와 Host에 연결한다. |
| `RawSurfaceFlare` | `BindFlareDevices`, `Flare` | 현재 실제 플레어 생성·예약·주기·소멸은 Session을 소유하지 않고 같은 물리를 사용한다. |
| `RawSurfaceFrameRegular` | `DiscRegular`, `ImpactRegular` | 두 실제 프레임 Regular를 같은 장기 Pool/TYPE/Frame/현재 가상 삭제에 연결한다. |
| `RawSurfaceGump` | `BindGumpRuntime`, `BindGumpUiRuntime`, `DeliverGumpNotice`, `GumpForms` | UI Form은 실제 Client의 공장/현재 TYPE/물리와 실제 UI 등록표를 장기 공유한다. |
| `RawSurfaceIce` | `IceTower`, `IceTowerGlobals` | 아이스 타워의 파괴·재성장·주기를 같은 장기 물리/TYPE/Host와 가상 부모에 연결한다. |
| `RawSurfaceLifetime` | `BindSurfaceLifetimeRuntime`, `BridgeRegular`, `SurfaceLifetime`, `VirtualDestroy` | 같은 장기 Pool/Graph/공통 삭제로 표면 수명을 소유하고 큰 파생 효과만 현재 Runtime에서 빌린다. |
| `RawSurfaceMana` | `BindManaDevices`, `ManaArrival`, `ManaRegular`, `ManaState` | Fountain의 대상 검색·도착·정기를 같은 현재 물리/카메라/Host에 연결한다. |
| `RawSurfacePlayerRegular` | `BindPlayerRegularRuntime`, `BindPlayerRegularState`, `ConstructPlayerRegular`, `PlayerRegular`, `ReservePlayerRegular` | Player 정기는 같은 현재 장기 상태/Host를 쓰고 선택된 Template만 실제 Runtime을 빌린다. |
| `RawSurfacePriest` | `BindPriestRuntime`, `PriestRegular`, `PriestState` | 사제 정기 분기와 현재 상태를 Client 장기로 보관하고 큰 후단만 실제 Runtime을 pin한다. |
| `RawSurfacePriestDeletion` | `PriestForcefield`, `PriestPre` | 실제 사제 목록과 보호막 조회·가상 삭제를 World/Session보다 긴 같은 저장소에 연결한다. |
| `RawSurfaceRegistryDeletion` | `BindPlayerRecords`, `DudePostDestroy`, `PlayerPreDestroy`, `ResidencePostDestroy` | 실제 Player 기록과 정적 SID 목록의 고유 삭제 부모를 World·Session 없이 연결한다. |
| `RawSurfaceResidenceRegular` | `BindResidenceDevices`, `BindResidenceRuntime`, `ResidenceRegular` | Native 거주 구역은 같은 실제 지형 역할/신전/난수와 선택한 Dude Runtime을 빌린다. |
| `RawSurfaceReward` | `BindRewardDevices`, `Reward`, `RewardState` | Client의 실제 HP·보상·분산 SP를 World/Session 수명과 독립된 같은 장기 물리에 연결한다. |
| `RawSurfaceRitual` | `AltarState`, `BindRitualRuntime`, `ForcefieldRegular`, `RitualRegular` | 의식계 정기는 장기 현재 물리에서 실행하고 선택된 큰 후단만 실제 Runtime을 빌린다. |
| `RawSurfaceVortex` | `BindVortexDevices`, `BindVortexRuntime`, `VortexRegular`, `VortexState` | 신전 네 VT의 정기/현재 모드는 장기 물리에서 실행하고 큰 이탈/Descend만 실제 Runtime을 빌린다. |
