// 실제 피해가능 객체 후처리와 간헐천 고유 수명의 전역/본체를 같은 장기 물리에 연결한다.
// cpppj/src/client/RawSurfaceDamageable.cpp 의 BindDamageableDevices/DamageablePostGlobals/DamageablePost 장기 본문을 이식한다.
// 0.576에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Damageable 우선)
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
}
