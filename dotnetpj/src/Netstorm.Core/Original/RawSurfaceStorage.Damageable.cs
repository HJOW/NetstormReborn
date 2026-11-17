// 실제 피해가능 객체 전/후처리·해방과 간헐천 고유 수명의 전역/본체를 같은 장기 물리에 연결한다.
// cpppj/src/client/RawSurfaceDamageable.cpp 의 BindDamageableDevices/DamageablePreGlobals/DamageableReleaseGlobals/DamageablePostGlobals/DamageableRelease/DamageablePre/DamageablePost/BombPre/BindCarrierReplenishment/CarrierPre/FlyerPre 장기 본문을 이식한다.
// 0.576에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Damageable 우선)
// 0.578에서 전처리·해방·폭탄 본체를 이었다.
// 0.579에서 운반체·비행체 전처리 본체를 이었다.
// 0.583에서 간헐천 고유 수명 본체를 이었다.
namespace Netstorm.Core.Original;

/// <summary>첫 실제 모듈 전 현재 finder/허용 genus owner와 현재 장치를 연결하는 묶음이다.</summary>
public sealed class RawSurfaceDamageableDevices
{
    /// <summary>같은 GameplayGlobals의 현재 검색 상태이며 값 사본이 아니다. 0.576에서 추가됐다.</summary>
    public ContainedFinderState? Finder { get; set; }

    /// <summary>실제 00542640 owner를 별칭 소유권으로 보존한다. 0.576에서 추가됐다.</summary>
    public UIntBox? AllowedGenus { get; set; }

    /// <summary>선택된 실제 파편/폭발 모듈만 현재 전체 Runtime을 빌린다. 0.576에서 추가됐다.</summary>
    public Action<int>? Collapse { get; set; }

    /// <summary>선택된 실제 파편/폭발 모듈만 현재 전체 Runtime을 빌린다. 0.576에서 추가됐다.</summary>
    public Action<int>? Explosion { get; set; }

    /// <summary>실제 같은 Client 소리 장치로 전역/위치 소리를 재생한다. 0.576에서 추가됐다.</summary>
    public Action<DamageableSoundEvent>? Sound { get; set; }

    /// <summary>실제 해방 사제 생성에 원문 type/WORD를 전달한다. 0.576에서 추가됐다.</summary>
    public Action<float, float, uint, ushort>? SpawnPriest { get; set; }

    /// <summary>실제 같은 Dude 드롭 생성기의 현재 호출이다. 0.576에서 추가됐다.</summary>
    public Action<int, int, bool>? DropDude { get; set; }

    /// <summary>선택된 실제 받침 정리의 좌표/flags/owner를 전달한다. 0.576에서 추가됐다.</summary>
    public Action<float, float, uint, uint>? DropIsland { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.576에서 추가됐다.</summary>
    internal RawSurfaceDamageableDevices Copy() => new()
    {
        Finder = Finder,
        AllowedGenus = AllowedGenus,
        Collapse = Collapse,
        Explosion = Explosion,
        Sound = Sound,
        SpawnPriest = SpawnPriest,
        DropDude = DropDude,
        DropIsland = DropIsland,
    };
}

/// <summary>World 없이 같은 장기 Pool/TYPE/장부로 피해 후처리를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>실제 Client와 장기 전역만 소유하며 Session 주소를 저장하지 않는다. 0.576에서 추가됐다.</summary>
    private RawSurfaceDamageableDevices _damageableDevices = new();

    /// <summary>같은 사제 TYPE의 장기 전역 뷰다. 0.576에서 추가됐다.</summary>
    private readonly DamageablePreDestroyState _damageablePreState = new();

    /// <summary>같은 장기 허용 genus를 실제 소비자가 공유하는 상태다. 0.576에서 추가됐다.</summary>
    private readonly DamageableReleaseState _damageableReleaseState = new();

    /// <summary>실제 Client 보충 부모를 최초 연결만 하며 호출하지 않는다. 0.579에서 추가됐다.</summary>
    private Action? _carrierReplenishment;

    /// <summary>현재 finder·Damageable과 보충 부모를 같은 장기 물리에서 사용하는 실제 운반체 전처리다. 0.579에서 추가됐다.</summary>
    private RawCarrierPreDestroy? _carrierPre;

    /// <summary>현재 WORD 대상 삭제 뒤 같은 장기 Damageable 전처리를 실행하는 실제 전처리다. 0.579에서 추가됐다.</summary>
    private RawFlyerPreDestroy? _flyerPre;

    /// <summary>실제 현재 모드/목록/사건의 장기 owner다. 0.576에서 추가됐다.</summary>
    private readonly DamageablePostDestroyState _damageablePostState = new();

    /// <summary>같은 HP/목록/공통 Post를 빌리는 실제 후처리 본체다. 0.576에서 추가됐다.</summary>
    private RawDamageablePostDestroy? _damageablePost;

    /// <summary>첫 실제 모듈 전 현재 owner/장치만 결합한다. 0.576에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 Damageable 장치 묶음이다.</param>
    public void BindDamageableDevices(RawSurfaceDamageableDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송·공통 삭제 뒤, 첫 실제 모듈 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _deletionDevices.UnitLost is null || _borrowed
            || _damageableDevices.Finder is not null || _damageablePost is not null
            || devices.Finder is null || devices.AllowedGenus is null || devices.Collapse is null
            || devices.Explosion is null || devices.Sound is null || devices.SpawnPriest is null
            || devices.DropDude is null || devices.DropIsland is null)
        {
            throw new ArgumentException("장기 Damageable의 실제 Native 최초 장치 계약", nameof(devices));
        }

        _damageableDevices = devices.Copy();
        _damageablePreState.SharedPriestType = GameAux().PriestTypeBox;
        _damageableReleaseState.SharedAllowedGenus = _damageableDevices.AllowedGenus;
        _damageablePostState.SharedExcludedDropTypes[0] = GameAux().TempleProductTypeBox;
        _damageablePostState.SharedExcludedDropTypes[1] = GameAux().RainWalkerTypeBox;
        _damageablePostState.SharedExcludedDropTypes[2] = GameAux().RainFlyerTypeBox;
        _damageablePostState.SharedExcludedDropTypes[3] = GameAux().WindFlyerTypeBox;
        _damageablePostState.SharedExcludedDropTypes[4] = GameAux().EmptyCarrierTypeBox;
        _damageablePostState.SharedSpecialDropType = GameAux().BulfTypeBox;
        _damageablePostState.SharedBattle = () => OwnerMode.Battle;
        _damageablePostState.SharedBoss = () => Authority().Boss;
        _damageablePostState.SharedSilentSurfaceType = GameAux().RainBlockerTypeBox;
        _damageablePostState.SharedDudePopulation = SidLists.Dudes;
        _damageablePostState.SharedDamageEvent = GameAux().DamageRegularEventBox;
        _damageablePreState.SharedPriestType = GameAux().PriestTypeBox;
        _damageableReleaseState.SharedAllowedGenus = _damageableDevices.AllowedGenus;
    }

    /// <summary>같은 사제 TYPE의 장기 전역 뷰다. 0.576에서 추가됐다.</summary>
    public DamageablePreDestroyState DamageablePreGlobals()
    {
        if (_damageableDevices.Finder is null)
        {
            throw new InvalidOperationException("장기 Damageable 실제 finder 미연결");
        }

        return _damageablePreState;
    }

    /// <summary>같은 장기 허용 genus를 실제 소비자가 공유한다. 0.576에서 추가됐다.</summary>
    public DamageableReleaseState DamageableReleaseGlobals()
    {
        if (_damageableDevices.Finder is null)
        {
            throw new InvalidOperationException("장기 Damageable 실제 해방 owner 미연결");
        }

        return _damageableReleaseState;
    }

    /// <summary>같은 실제 population/mode/event의 장기 후처리 owner다. 0.576에서 추가됐다.</summary>
    public DamageablePostDestroyState DamageablePostGlobals()
    {
        if (_damageableDevices.Finder is null)
        {
            throw new InvalidOperationException("장기 Damageable 실제 후처리 owner 미연결");
        }

        return _damageablePostState;
    }

    /// <summary>같은 HP/목록/공간/Host로 드롭·피해 예약·다리 약화·공통 Post를 연결한다. 0.576에서 추가됐다.</summary>
    public RawDamageablePostDestroy DamageablePost()
    {
        // 실제 같은 장기 전역이 있어야 한다.
        DamageablePostDestroyState state = DamageablePostGlobals();
        if (_damageablePost is null)
        {
            var hooks = new DamageablePostDestroyHooks
            {
                DropDude = _damageableDevices.DropDude,
                DropIsland = _damageableDevices.DropIsland,
                Regular = (sid, @event, payload, count) =>
                {
                    RegularProcess? process = Processes().Host().AddRegular(sid, @event, payload);
                    if (process is null)
                    {
                        throw new OutOfMemoryException("장기 피해 예약 Regular 확보 실패");
                    }

                    process.SetInitialCount(count);
                },
                HasBridgeFall = sid => SquidProcessHost.HasScheduledBridgeFall(Processes().Host(), sid),
                NotifySurface = sid => BuildingExecutor().NotifySurface(sid),
                DisplayUpdate = sid => DisplayExecutor().Update(Pool.Slot(sid)),
                Destroy = (sid, flags) => DestroyAsset(sid, flags),
                Base = (sid, flags) => CommonDeletion().PostDestroy(sid, flags),
            };
            _damageablePost = new RawDamageablePostDestroy(
                Pool,
                Hash,
                Types,
                Frames,
                Reward(),
                NeighborsExecutor(),
                state,
                hooks);
        }

        return _damageablePost;
    }

    /// <summary>같은 현재 TYPE/내용물과 실제 Factory/Owner/Pop의 해방 몸체다. 0.578에서 추가됐다.</summary>
    private RawDamageableRelease? _damageableRelease;

    /// <summary>실제 Native 효과 순서와 현재 권한/Bridge TYPE를 사용하는 전처리 본체다. 0.578에서 추가됐다.</summary>
    private RawDamageablePreDestroy? _damageablePre;

    /// <summary>실제 Point Blast의 자연 관문 뒤 현재 소리와 공통 부모를 같은 장기 본체에서 선택한다. 0.578에서 추가됐다.</summary>
    private RawBombPreDestroy? _bombPre;

    /// <summary>같은 현재 TYPE/내용물과 실제 Factory/Owner/Pop의 해방 몸체다. 0.578에서 추가됐다.</summary>
    public RawDamageableRelease DamageableRelease()
    {
        // finder 역참조보다 먼저 실제 owner 결합을 확인한다.
        DamageableReleaseState state = DamageableReleaseGlobals();
        if (_damageableRelease is null)
        {
            var hooks = new DamageableReleaseHooks
            {
                SpawnPriest = _damageableDevices.SpawnPriest,
                Create = (type, flags) => CreateAsset(type, flags),
                SetOwner = (sid, owner) => SetAssetOwner(sid, owner),
                Pop = (sid, x, y, flags) => PopAsset(sid, x, y, flags),
                NotifySurface = sid => BuildingExecutor().NotifySurface(sid),
            };
            _damageableRelease = new RawDamageableRelease(
                Pool,
                Types,
                Spots,
                _damageableDevices.Finder ?? throw new InvalidOperationException("장기 Damageable 실제 finder 미연결"),
                state,
                hooks);
        }

        return _damageableRelease;
    }

    /// <summary>실제 Native 효과 순서와 현재 권한/Bridge TYPE를 사용하는 전처리 본체다. 0.578에서 추가됐다.</summary>
    public RawDamageablePreDestroy DamageablePre()
    {
        // 구성되지 않은 장치를 물리 포인터로 대신하지 않는다.
        DamageablePreDestroyState state = DamageablePreGlobals();
        if (_damageablePre is null)
        {
            var hooks = new DamageablePreDestroyHooks
            {
                Collapse = _damageableDevices.Collapse,
                Explosion = _damageableDevices.Explosion,
                Sound = _damageableDevices.Sound,
                ReleaseContained = sid => DamageableRelease().ReleaseNative(sid, () => OwnerMode.Battle, () => Links.BridgeType),
                ValidateBase = (sid, flags) => CommonDeletion().ValidatePre(sid, flags),
                Base = (sid, flags) => CommonDeletion().PreDestroy(sid, flags),
            };
            var body = new RawDamageablePreDestroy(
                Pool,
                Spots,
                _damageableDevices.Finder ?? throw new InvalidOperationException("장기 Damageable 실제 finder 미연결"),
                state,
                hooks);
            body.EnableNativeBody();
            _damageablePre = body;
        }

        return _damageablePre;
    }

    /// <summary>실제 Point Blast의 자연 관문 뒤 현재 소리와 공통 부모를 같은 장기 본체에서 선택한다. 0.578에서 추가됐다.</summary>
    public RawBombPreDestroy BombPre()
    {
        // 동일 finder/실제 Client 효과의 최초 결합만 확인하며 값을 초기화하지 않는다.
        DamageablePreDestroyState state = DamageablePreGlobals();
        if (_bombPre is null)
        {
            var hooks = new BombPreDestroyHooks
            {
                Explosion = sid => (_damageableDevices.Explosion ?? throw new InvalidOperationException("장기 Damageable 실제 폭발 장치 미연결"))(sid),
                Sound = (x, y) => (_damageableDevices.Sound ?? throw new InvalidOperationException("장기 Damageable 실제 소리 장치 미연결"))(new DamageableSoundEvent(DamageableSound.Explosion, x, y)),
                Base = (sid, flags) => CommonDeletion().PreDestroy(sid, flags),
            };
            _bombPre = new RawBombPreDestroy(Pool, hooks);
        }

        return _bombPre;
    }

    /// <summary>실제 Client 보충 부모를 최초 연결만 하며 권한/지도/모드 관문을 앞당겨 실행하지 않는다. 0.579에서 추가됐다.</summary>
    /// <param name="replenish">연결할 실제 Client 보충 부모다.</param>
    public void BindCarrierReplenishment(Action replenish)
    {
        ArgumentNullException.ThrowIfNull(replenish);
        // Native 전송 뒤, 첫 운반체 본체/대여 전에만 실제 Client 보충 부모를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _carrierPre is not null || _carrierReplenishment is not null)
        {
            throw new ArgumentException("장기 Carrier 보충 부모의 최초 Native 계약", nameof(replenish));
        }

        _carrierReplenishment = replenish;
    }

    /// <summary>현재 finder·Damageable과 보충 부모를 같은 장기 물리에서 사용하는 전처리 본체다. 0.579에서 추가됐다.</summary>
    public RawCarrierPreDestroy CarrierPre()
    {
        // 구성되지 않은 장치를 물리 포인터로 대신하지 않는다.
        _ = DamageablePreGlobals();
        if (_carrierPre is null)
        {
            var hooks = new CarrierPreDestroyHooks
            {
                ValidateDamageable = (sid, flags) => DamageablePre().Validate(sid, flags),
                DamageablePre = (sid, flags) => DamageablePre().PreDestroy(sid, flags),
                DependentFound = () => (_carrierReplenishment ?? throw new InvalidOperationException("장기 Carrier 내용물의 실제 너겟 보충 부모 미결합"))(),
            };
            var body = new RawCarrierPreDestroy(Pool, _damageableDevices.Finder ?? throw new InvalidOperationException("장기 Damageable 실제 finder 미연결"), hooks);
            body.EnableNativeBody();
            _carrierPre = body;
        }

        return _carrierPre;
    }

    /// <summary>현재 WORD 대상 삭제 뒤 같은 장기 Damageable 전처리를 실행한다. 0.579에서 추가됐다.</summary>
    public RawFlyerPreDestroy FlyerPre()
    {
        // 장치의 최초 결합만 확인하고 게임 값이나 현재 대상을 읽지 않는다.
        DamageablePreDestroyState state = DamageablePreGlobals();
        if (_flyerPre is null)
        {
            _flyerPre = new RawFlyerPreDestroy(Pool, new FlyerPreDestroyHooks
            {
                Destroy = (target, flags) => DestroyAsset(target, flags),
                Damageable = (sid, flags) => DamageablePre().PreDestroy(sid, flags),
            });
        }

        return _flyerPre;
    }

    /// <summary>같은 지면 목록/기본 프레임/현재 편집 모드를 사용하는 실제 간헐천 고유 수명이다. 0.583에서 추가됐다.</summary>
    private RawGeyserLifetime? _geyserLifetime;

    /// <summary>같은 지면 목록/기본 프레임/현재 편집 모드를 사용하는 실제 간헐천 고유 수명이다. 0.583에서 추가됐다.</summary>
    public RawGeyserLifetime GeyserLifetime()
    {
        // 실제 editor/finder owner 결합 뒤에만 구성한다.
        DamageablePreDestroyState state = DamageablePreGlobals();
        if (_geyserLifetime is null)
        {
            var hooks = new GeyserLifetimeHooks
            {
                DefaultFrame = sid =>
                {
                    uint number = Pool.Slot(sid)[10];
                    if (number >= Types.Count)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "Native 간헐천 현재 기본 프레임 TYPE 범위");
                    }

                    return unchecked((uint)Types[(int)number].DefaultFrame);
                },
                Update = sid => DisplayExecutor().Update(Pool.Slot(sid)),
                RegisterAi = (sid, operation) => (_resourceDevices.GeyserRegistration ?? throw new InvalidOperationException("장기 간헐천의 실제 Player 등록 미연결"))(sid, operation),
                DamageablePre = (sid, flags) => DamageablePre().PreDestroy(sid, flags),
                DamageablePost = (sid, flags) => DamageablePost().PostDestroy(sid, flags),
                DropIsland = (x, y, flags, owner) => (_damageableDevices.DropIsland ?? throw new InvalidOperationException("장기 간헐천의 실제 받침 정리 미연결"))(x, y, flags, owner),
            };
            var body = new RawGeyserLifetime(
                Pool,
                SidLists.Geysers,
                _rewardDevices.Editor ?? throw new InvalidOperationException("장기 간헐천의 실제 editor owner 미연결"),
                hooks);
            body.EnableNativeBody();
            _geyserLifetime = body;
        }

        return _geyserLifetime;
    }

    /// <summary>일반 거주 구역의 장기 50칸 목록을 압축한 뒤 공통 Post를 실행한다. 0.580에서 추가됐다.</summary>
    /// <param name="sid">삭제할 거주 구역 번호다.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void ResidencePostDestroy(int sid, uint flags)
    {
        // 현재 extra9 관문은 목록을 읽기 전에 적용한다.
        if ((Pool.Slot(sid)[40] & 9u) == 0)
        {
            SidLists.Residences.RemoveAll(unchecked((uint)sid));
        }

        CommonDeletion().PostDestroy(sid, flags);
    }

    /// <summary>기존 30칸 population 제거 본체를 같은 장기 목록에 연결한다. 0.580에서 추가됐다.</summary>
    /// <param name="sid">삭제할 Dude 번호다.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트다.</param>
    public void DudePostDestroy(int sid, uint flags)
    {
        if (_edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("Native Dude 후처리 판본 오류", nameof(sid));
        }

        if ((Pool.Slot(sid)[40] & 9u) == 0)
        {
            SidLists.Dudes.RemoveAll(unchecked((uint)sid));
        }

        CommonDeletion().PostDestroy(sid, flags);
    }
}
