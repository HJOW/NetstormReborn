using Netstorm.Core.Original;

// 같은 실제 World/Core/Host에 기존 애니메이션·미사일 몸체를 조립한다.
namespace Netstorm.Client.Mission;

/// <summary>공간/Host로 대체하지 않는 실제 음향·배리어·피해 등 외부 효과다.</summary>
public sealed class RawCampaignArcherEffects
{
    /// <summary>현재 음향/방향/배리어 코너의 실제 아처 경계다.</summary>
    public ArcherAnimationHooks Animation { get; set; } = new();
    /// <summary>현재 포구/SHP/피해/충돌의 실제 경계다.</summary>
    public MissileHooks Missile { get; set; } = new();
    /// <summary>반더 즉시 빔의 실제 생성/갱신/피해 경계다.</summary>
    public CannonBeamHooks Beam { get; set; } = new();
    /// <summary>선택 시 요구하는 실제 아이스 냉각 표시다.</summary>
    public Action<ushort>? IceCooling { get; set; }
    /// <summary>원문 효과 묶음의 반사점 지역8BYTE 입력이다. 이 공장 부모에서는 아직 소비하지 않는다.</summary>
    public Action<Span<byte>>? RicochetPointBytes { get; set; }
}

/// <summary>RawCampaignArcherBindings.cpp의 같은 World/Host 수명을 빌리는 전투 공장 부모다. 반납 전 상위가 Kernel/callback 사용을 끝낸다.</summary>
public sealed partial class RawCampaignArcherBindings
{
    /// <summary>모든 공간/가상 삭제가 사용하는 같은 실제 월드다.</summary>
    private readonly RawSurfaceWorld _world;
    /// <summary>별도 배열/Pool을 만들지 않는 실제 월드 참조 보기다.</summary>
    private readonly RawWorldCore _core;
    /// <summary>같은 현재 표적/중심을 읽는 검색기다.</summary>
    private readonly RawGunTarget _target;
    /// <summary>다른 프로세스와 공유하는 단일 게임 난수다.</summary>
    private readonly GameRandom _random;
    /// <summary>외부 owner가 보유하는 같은 현재 아처 시계다.</summary>
    private readonly ArcherAnimationState _animationState;
    /// <summary>외부 owner가 보유하는 같은 현재 미사일 시계다.</summary>
    private readonly MissileState _missileState;
    /// <summary>같은 공장/공간/Host의 실제 미사일 몸체다.</summary>
    private readonly RawMissile _missile;
    /// <summary>같은 방향 자료와 현재 역할을 읽는 썬 아처 몸체다.</summary>
    private readonly RawArcherAnimation _animation;
    /// <summary>현재 썬 캐논 회전 시계/역할이다.</summary>
    private readonly GunAnimationState _cannonState = new();
    /// <summary>실제 썬 캐논 회전/원위치 몸체다.</summary>
    private readonly RawGunAnimation _cannon;
    /// <summary>TYPE131의 실제 공통 회전 상태다.</summary>
    private readonly GunAnimationState _thunderRotateState = new() { SunCannonType = 131 };
    /// <summary>TYPE131의 실제 공통 회전 몸체다.</summary>
    private readonly RawGunAnimation _thunderRotate;
    /// <summary>썬 캐논의 실제 발사 상태다.</summary>
    private readonly CannonFireState _sunFireState = new() { Kind = CannonFireKind.Sun, Type = 128, DisplayType = 80, EffectType = 11, Delay = .07f };
    /// <summary>썬 캐논의 실제 공통 발사 몸체다.</summary>
    private readonly RawCannonFire _sunFire;
    /// <summary>썬더 캐논의 실제 발사 상태다.</summary>
    private readonly CannonFireState _thunderFireState = new();
    /// <summary>썬더 캐논의 실제 공통 발사 몸체다.</summary>
    private readonly RawCannonFire _thunderFire;
    /// <summary>반더의 즉시 빔/현재 시각/float0.3 지연이다.</summary>
    private readonly CannonFireState _vanderFireState = new() { Kind = CannonFireKind.Vander, Type = 148, DisplayType = 80, EffectType = 19, Delay = .3f };
    /// <summary>일반 미사일과 구별하는 실제 반더 빔 발사 몸체다.</summary>
    private readonly RawCannonFire _vanderFire;
    /// <summary>아이스129/표시130/float0.08의 실제 상태다.</summary>
    private readonly CannonFireState _iceState = new() { Type = 129, DisplayType = 130, Delay = .08f };
    /// <summary>공통 접두/추가16BYTE를 함께 실행하는 아이스 발사 몸체다.</summary>
    private readonly RawIceCannonFire _ice;
    /// <summary>GunHooks가 값 복사한 현재 raw 발사 문맥 조회기다.</summary>
    private Func<ushort, uint[]>? _iceContext;
    /// <summary>TYPE140의 같은 현재 회전 시계다.</summary>
    private readonly ArcherAnimationState _crossbowRotateState = new() { Type = 140 };
    /// <summary>TYPE140의 실제 별도 발사 시계다.</summary>
    private readonly CrossbowAnimationState _crossbowState = new();
    /// <summary>아처 공통 본문을 재사용하는 크로스보우 회전 몸체다.</summary>
    private readonly RawArcherAnimation _crossbowRotate;
    /// <summary>별도 가상 발사를 실행하는 크로스보우 몸체다.</summary>
    private readonly RawCrossbowAnimation _crossbow;
    /// <summary>같은 Frame 지정기의 실제 썬 디스크 정기다.</summary>
    private readonly RawDiscRegular _disc;

    /// <summary>실제 Core와 같은 Pool/역할/난수를 검증하고 기존 파생 몸체를 원문 순서로 조립한다. 생성 중 게임 효과는 선택하지 않는다.</summary>
    public RawCampaignArcherBindings(RawSurfaceWorld world, RawGunTarget target, GameRandom random,
        IReadOnlyList<ArcherTypeData> animationTypes, IReadOnlyList<PriestPlainCanonType> frameTypes,
        IReadOnlyList<MissileTypeData> missileTypes, ArcherAnimationState animation, MissileState missile, RawCampaignArcherEffects effects)
    {
        ArgumentNullException.ThrowIfNull(world); ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(animation); ArgumentNullException.ThrowIfNull(missile); ArgumentNullException.ThrowIfNull(effects);
        _world = world; _core = world.Core(); _target = target; _random = random; _animationState = animation; _missileState = missile;
        if (!ReferenceEquals(target.Pool, _core.Pool)) throw new ArgumentException("캠페인 Archer 현재 표적 Pool 불일치", nameof(target));
        RawGameAuxState aux = world.Storage().GameAux();
        _animationState.CurrentType = aux.SunArcherTypeBox; _cannonState.CurrentSunCannonType = aux.SunCannonTypeBox;
        _sunFireState.CurrentType = aux.SunCannonTypeBox; _crossbowRotateState.CurrentType = aux.WindArcherTypeBox; _crossbowState.CurrentType = aux.WindArcherTypeBox;
        _disc = new(_core.Pool, animationTypes, _core.Frame);
        MissileHooks flight = effects.Missile.Copy();
        flight.RandomInteger = (low, high) => high <= low ? throw new ArgumentException("투사체 정수 난수 구간 오류") : unchecked(low + (int)_random.Next((uint)(high - low)));
        flight.RandomFloat = (low, high) => (double)low + (_random.Next(65536) * (1.0 / 65536.0)) * Math.Abs((double)high - low);
        flight.Owner = (sid, owner) => _core.Owner.Set(sid, owner);
        flight.Unpop = (sid, flags) => _world.Unpop(sid, flags); flight.Pop = (sid, x, y, flags) => _world.Pop(sid, x, y, flags);
        flight.Center = CurrentCenter; flight.Destroy = (sid, flags) => _world.Destroy(sid, flags);
        _missile = new(_core.Pool, _core.Types, missileTypes, _missileState, RawMissile.MakeFactoryHooks(_core.Pool, _core.Factory, flight));
        ArcherAnimationHooks hooks = effects.Animation.Copy(); hooks.Common.Center = CurrentCenter;
        hooks.Common.Unpop = (sid, flags) => _world.Unpop(sid, flags); hooks.Common.Repop = (sid, flags) => _world.Repop(sid, flags);
        hooks.Common.RandomInteger = (low, high) => unchecked(low + (int)_random.Next((uint)(high - low)));
        hooks.NotifySurface = sid => _core.Building.NotifySurface(sid);
        GunAnimationHooks cannonHooks = RawMissile.MakeGunHooks(_core.Processes, _missile, RawGunAnimation.MakeHostHooks(_core.Processes, hooks.Common));
        _thunderRotate = new(_core.Pool, _core.Types, _thunderRotateState, cannonHooks);
        _cannon = new(_core.Pool, _core.Types, _cannonState, cannonHooks);
        GunAnimationHooks fireHooks = RawMissile.MakeGunHooks(_core.Processes, _missile, RawCannonFire.MakeHostHooks(_core.Processes, hooks.Common));
        _sunFire = new(_core.Pool, _core.Types, frameTypes, _sunFireState, fireHooks);
        CannonBeamHooks beam = effects.Beam.Copy(); beam.Random = _random;
        _vanderFire = new(_core.Pool, _core.Types, frameTypes, _vanderFireState, fireHooks, beam);
        GunAnimationHooks iceCommon = RawMissile.MakeGunHooks(_core.Processes, _missile, RawIceCannonFire.MakeHostHooks(_core.Processes, hooks.Common));
        var iceHooks = new IceCannonFireHooks
        {
            Cooling = effects.IceCooling,
            Context = sid => (_iceContext ?? throw new InvalidOperationException("아이스 발사의 실제 현재 문맥 미연결"))(sid),
            MayTarget = (context, sid) => _target.MayTarget(GunTargetQuery.FromWords(context), sid),
        };
        _ice = new(_core.Pool, _core.Types, frameTypes, _iceState, iceCommon, iceHooks);
        _thunderFire = new(_core.Pool, _core.Types, frameTypes, _thunderFireState, fireHooks);
        GunAnimationHooks crossbowHooks = RawMissile.MakeGunHooks(_core.Processes, _missile, RawCrossbowAnimation.MakeHostHooks(_core.Processes, hooks.Common));
        _crossbow = new(_core.Pool, _core.Types, animationTypes, _crossbowState, crossbowHooks);
        hooks = RawArcherAnimation.MakeHostHooks(_core.Processes, hooks); hooks.Common = RawMissile.MakeGunHooks(_core.Processes, _missile, hooks.Common);
        _crossbowRotateState.Angles = _animationState.Angles;
        _crossbowRotate = new(_core.Pool, _core.Types, animationTypes, _crossbowRotateState, hooks);
        _animation = new(_core.Pool, _core.Types, animationTypes, _animationState, hooks);
    }
    /// <summary>현재 원문 중심을 부동소수 값 두 칸으로 전달하며 반환 배열은 소유권을 분리한다.</summary>
    private float[] CurrentCenter(ushort sid) { var point = _target.Center(sid); return [point.X, point.Y]; }
    /// <summary>후속 공장/검사가 같은 실제 아처 몸체를 재사용한다.</summary>
    public RawArcherAnimation Animation => _animation;
    /// <summary>후속 공장/검사가 같은 실제 미사일 몸체를 재사용한다.</summary>
    public RawMissile Missile => _missile;
    /// <summary>현재 같은 실제 Pool의 정체성이다.</summary>
    public RawSidPool Pool => _core.Pool;
    /// <summary>다음 전체 Kernel 실행 전에 같은 현재 게임 시각/경과를 모든 파생에 공급한다.</summary>
    public void PrepareFrame(FrameTime time)
    {
        if (!double.IsFinite(time.Game) || !double.IsFinite(time.Delta) || time.Delta < 0) throw new ArgumentException("Archer 현재 clock/경과 오류", nameof(time));
        _animationState.Now = time.Game; _animationState.Delta = time.Delta; _missileState.Now = time.Game; _cannonState.Now = time.Game;
        _crossbowRotateState.Now = time.Game; _crossbowRotateState.Delta = time.Delta; _crossbowState.Now = time.Game;
        _thunderRotateState.Now = time.Game; _sunFireState.Now = time.Game; _thunderFireState.Now = time.Game; _vanderFireState.Now = time.Game; _iceState.Now = time.Game;
    }
    /// <summary>같은 Kernel의 예약에서 실제 썬 디스크만 선택하고 다른 정기는 원래 부모로 넘긴다.</summary>
    public RegularHandler Handler(RegularHandler? fallback = null) => _disc.Handler(fallback);
}
