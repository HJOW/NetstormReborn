using System.Buffers.Binary;
using Netstorm.Core;
using Netstorm.Core.Simulation;
using Netstorm.Core.Original;

// 사제·보호막·Carrier가 같은 실제 월드 저장소와 프로세스를 공유하도록 조립한다.
namespace Netstorm.Client.Mission;

/// <summary>사제 모듈보다 오래 살아야 하는 공통 월드 자원이다. 배열은 실제 owner의 저장소를 빌린다.</summary>
/// <param name="Pool">모든 모듈의 같은 SID 풀이다.</param>
/// <param name="Hash">실제 공간 해시다.</param>
/// <param name="Types">현재 타입 표다.</param>
/// <param name="Frames">실제 방향별 프레임 표다.</param>
/// <param name="Spots">65536셀 점유 지도다.</param>
/// <param name="Surfaces">Hash의 단계0 실제 배열이다.</param>
/// <param name="Reward">같은 풀의 HP·보상 처리기다.</param>
/// <param name="Random">실제 게임 난수 스트림이다.</param>
/// <param name="Factory">실제 객체 생성기다.</param>
/// <param name="Owner">공통 소유자 처리기다.</param>
/// <param name="OwnerMode">같은 현재 소유자 정책이다.</param>
/// <param name="PostPop">공통 Pop 후처리기다.</param>
/// <param name="Frame">실제 프레임 지정기다.</param>
public sealed record RawPriestWorldCore(RawSidPool Pool, RawSquidHash Hash,
    IReadOnlyList<OriginalTypeRecord> Types, IReadOnlyList<RiftTypeFrames> Frames,
    byte[] Spots, ushort[] Surfaces, RawSquidReward Reward, GameRandom Random,
    RawSquidFactory Factory, SquidOwner Owner, SquidOwnerMode OwnerMode, RawPostPop PostPop, SquidFrame Frame);

/// <summary>미션의 조회·예약·삭제가 함께 소비하는 실제 사제 상태다.</summary>
public sealed class RawPriestWorldState
{
    /// <summary>공통 등록·삭제·미션 결과가 공유하는 목록이다.</summary>
    public PriestPostPopState Priests { get; } = new();
    /// <summary>사제 소유자 표식과 로딩 상태다.</summary>
    public PriestOwnerState Owner { get; } = new();
    /// <summary>현재 Player9와 권한을 빌리는 HP 모드다.</summary>
    public PriestHitPointMode HitPoints { get; } = new();
    /// <summary>보호막의 로컬 안내·로딩 상태다.</summary>
    public PriestShieldState Shield { get; } = new();
    /// <summary>회복·중립 처리 상태다.</summary>
    public PriestRegenState Regen { get; } = new();
    /// <summary>현재 낙하 권한이다.</summary>
    public PriestFallState Fall { get; } = new();
    /// <summary>현재 보호막 조회 타입이다.</summary>
    public PriestForcefieldState Forcefield { get; } = new();
    /// <summary>사제 생성·나선 후보 상태다.</summary>
    public PriestSpawnState Spawn { get; } = new();
    /// <summary>Carrier의 공통 후처리 상태다.</summary>
    public CarrierPostPopState Carrier { get; } = new();
    /// <summary>삭제·도착이 공유하는 실제 내용물 조회 상태다.</summary>
    public ContainedFinderState Contained { get; } = new();
    /// <summary>보호막 정기의 현재 사제TYPE·진단 상태다.</summary>
    public ForcefieldRegularState ForcefieldRegular { get; } = new();
    /// <summary>현재 장기 TYPE·프로세스 선택 상자다.</summary>
    public RawMovementProcessTypes Movement { get; }
    /// <summary>실제 Session은 같은 장기 상자를 대여하고 독립 구성은 기존 초기값을 소유한다.</summary>
    public RawPriestWorldState(UIntBox? sharedPriest = null, RawGameAuxState? sharedRoles = null)
        => Movement = new(sharedPriest, sharedRoles);
}

/// <summary>실제 공간·소리·안내 및 외부 규칙 경계다. 생성 시 함수 객체를 값 복사한다.</summary>
public sealed class RawPriestWorldEffects
{
    /// <summary>실제 공간 Pop이다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>실제 공간 Unpop이다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>실제 공간 Repop이다.</summary>
    public Action<int, uint>? Repop { get; set; }
    /// <summary>Form·공간·목록을 정리하는 실제 가상 삭제다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>지면 소유권을 갱신한다.</summary>
    public Action<float, float, byte>? ClaimGround { get; set; }
    /// <summary>사제 소유자 변경을 통지한다.</summary>
    public Action<int>? OwnerNotice { get; set; }
    /// <summary>표면 변경을 통지한다.</summary>
    public Action<int>? NotifySurface { get; set; }
    /// <summary>발견한 종속 객체를 처리한다.</summary>
    public Action? DependentFound { get; set; }
    /// <summary>실제 Regular 확보 성공 여부다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>보호막 소리·안내 경계다.</summary>
    public PriestShieldHooks Shield { get; set; } = new();
    /// <summary>현재 좌표에서 사제 낙하 소리를 요청한다.</summary>
    public Action<int, float, float>? FallSound { get; set; }
    /// <summary>회복의 아직 외부인 효과를 공급한다.</summary>
    public PriestRegenHooks Regen { get; set; } = new();
    /// <summary>사제 이외 이동 유닛의 추락이다.</summary>
    public Func<int, bool>? OtherWalkerFall { get; set; }
    /// <summary>보호막의 복귀 가능한 실제 진단이다.</summary>
    public Action<string, string, uint>? ForcefieldAssertion { get; set; }
    /// <summary>실제 좌표·타입 finder다.</summary>
    public Func<float, float, uint, int>? FindTypeAt { get; set; }
    /// <summary>사제 이외의 현재 가상 소유자 분배다.</summary>
    public Action<int, uint>? NonPriestOwner { get; set; }
    /// <summary>사제 소유자 진단이다.</summary>
    public Action<string, string, uint>? OwnerAssertion { get; set; }
    /// <summary>Native가 빌리는 같은 장기 Carrier 본문이다.</summary>
    public RawCarrierPreDestroy? SharedCarrier { get; set; }
    /// <summary>Native가 빌리는 같은 장기 보호막 조회다.</summary>
    public RawPriestForcefield? SharedForcefield { get; set; }
    /// <summary>Native가 빌리는 같은 장기 보호막 정기다.</summary>
    public RawForcefieldRegular? SharedForcefieldRegular { get; set; }
    /// <summary>Native가 빌리는 같은 장기 사제 전처리다.</summary>
    public RawPriestPreDestroy? SharedPriestPre { get; set; }

    /// <summary>캡처한 owner는 공유하되 바깥 객체와 중첩 훅 객체의 추후 재대입은 격리한다.</summary>
    internal RawPriestWorldEffects Copy() => new()
    {
        Pop = Pop, Unpop = Unpop, Repop = Repop, Destroy = Destroy, ClaimGround = ClaimGround,
        OwnerNotice = OwnerNotice, NotifySurface = NotifySurface, DependentFound = DependentFound,
        ReserveRegular = ReserveRegular, FallSound = FallSound, OtherWalkerFall = OtherWalkerFall,
        ForcefieldAssertion = ForcefieldAssertion, FindTypeAt = FindTypeAt, NonPriestOwner = NonPriestOwner,
        OwnerAssertion = OwnerAssertion, SharedCarrier = SharedCarrier, SharedForcefield = SharedForcefield,
        SharedForcefieldRegular = SharedForcefieldRegular, SharedPriestPre = SharedPriestPre,
        Shield = new() { Create = Shield.Create, SetOwner = Shield.SetOwner, Pop = Shield.Pop,
            ReserveSound = Shield.ReserveSound, LoadSound = Shield.LoadSound, AttachSound = Shield.AttachSound,
            NoticeSound = Shield.NoticeSound, Translate = Shield.Translate, Tell = Shield.Tell },
        Regen = new() { Refresh = Regen.Refresh, Damageable = Regen.Damageable, Shape = Regen.Shape,
            Occupied = Regen.Occupied, NeutralAction = Regen.NeutralAction, PatchAudit = Regen.PatchAudit,
            Measurement = Regen.Measurement },
    };
}

/// <summary>같은 실제 월드의 사제 모듈을 조립한다. 외부 Kernel을 먼저 비운 뒤 Dispose한다.</summary>
public sealed class RawPriestWorldBindings : IDisposable
{
    /// <summary>공통 자원은 같은 참조를 빌린다.</summary>
    private readonly RawPriestWorldCore _core;
    /// <summary>미션 결과까지 공유할 상태다.</summary>
    private readonly RawPriestWorldState _state;
    /// <summary>생성 시 복사한 실제 효과다.</summary>
    private readonly RawPriestWorldEffects _effects;
    /// <summary>로딩 전에 지연 연결할 실제 Host다.</summary>
    private SquidProcessHost? _host;
    /// <summary>삭제 전에 지연 연결할 실제 피해 전처리다.</summary>
    private RawDamageablePreDestroy? _damageable;
    /// <summary>해방 전에 지연 연결할 실제 배치 검사다.</summary>
    private RawPriestPlacement? _placement;
    /// <summary>사제 가상 소유자다.</summary>
    private readonly RawPriestOwner _owner;
    /// <summary>실제 보호막 확보 몸체다.</summary>
    private readonly RawPriestShield _shield;
    /// <summary>실제 낙하·공유 예약 몸체다.</summary>
    private readonly RawPriestFall _fall;
    /// <summary>실제 사제 전처리 또는 장기 대여다.</summary>
    private readonly RawPriestPreDestroy _priestPre;
    /// <summary>실제 보호막 정기 또는 장기 대여다.</summary>
    private readonly RawForcefieldRegular _fieldRegular;
    /// <summary>실제 회복 몸체다.</summary>
    private readonly RawPriestRegen _regen;
    /// <summary>동일 내용물 풀의 실제 조회다.</summary>
    private readonly RawContainedLookup _contained;
    /// <summary>등록 반납을 한 번만 수행한다.</summary>
    private bool _disposed;

    /// <summary>필수 자원을 먼저 검사하고 모든 모듈 생성 성공 뒤에만 가상 후처리를 등록한다.</summary>
    public RawPriestWorldBindings(RawPriestWorldCore core, RawPriestWorldState state, RawPriestWorldEffects effects)
    {
        ArgumentNullException.ThrowIfNull(core); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(effects);
        _core = core; _state = state; _effects = effects.Copy();
        if (!ReferenceEquals(core.Reward.Pool, core.Pool) || !ReferenceEquals(core.Factory.Pool, core.Pool)
            || !ReferenceEquals(core.Owner.Pool, core.Pool) || !ReferenceEquals(core.PostPop.Pool, core.Pool)
            || !ReferenceEquals(core.Frame.Pool, core.Pool) || core.Types.Count != core.Frames.Count
            || core.Spots.Length != 65536 || core.Surfaces.Length != 65536
            || !ReferenceEquals(core.Surfaces, core.Hash.BorrowEntries(0)) || _effects.Pop is null
            || _effects.Unpop is null || _effects.Repop is null || _effects.Destroy is null
            || _effects.ClaimGround is null || _effects.OwnerNotice is null || _effects.NotifySurface is null
            || _effects.DependentFound is null || _effects.ReserveRegular is null || _effects.FallSound is null)
            throw new ArgumentException("사제 월드의 풀/공간/필수 효과 연결 오류");
        _ = state.Priests.GetPriests().Items();
        var fallFrames = new PriestFallFrames[core.Frames.Count];
        // 실제 방향 J/E의 첫 프레임을 타입별 원문 순서로 읽는다.
        for (int index = 0; index < fallFrames.Length; index++)
            fallFrames[index] = new(core.Frames[index].Run((byte)'J').First, core.Frames[index].Run((byte)'E').First);
        Forcefield = _effects.SharedForcefield ?? new(core.Pool, core.Hash, core.Types, state.Forcefield);
        SamePool(Forcefield.Pool());
        PriestState = new(core.Pool, core.Reward, core.Types, core.Spots);
        if (core.Pool.Edition == OriginalEdition.Patch1078) PriestState.BindCurrentTypes(core.Types);
        _owner = new(core.Pool, core.OwnerMode, state.Owner,
            new() { Base = core.Owner.Set, Notify = _effects.OwnerNotice, Assertion = _effects.OwnerAssertion });
        PriestShieldHooks shieldEffects = RawPriestShield.MakePriestShieldCreationHooks(core.Pool, core.Factory, core.Owner, _effects.Shield);
        shieldEffects.Pop = _effects.Pop;
        _shield = new(core.Pool, Forcefield, state.Shield, shieldEffects);
        var advance = new RawFrameAdvance(core.Pool, core.Frames, core.Frame);
        PriestFallHooks falling = RawPriestFall.MakePriestFallFrameHooks(core.Frame, advance, new()
        {
            EnsureShield = _shield.Ensure, ReserveRegular = _effects.ReserveRegular,
            AddSharedRegular = (sid, ev, payload) => Host().AddSharedRegular(sid, ev, payload),
            Unpop = _effects.Unpop, Repop = _effects.Repop, Sound = _effects.FallSound, ClearShield = ClearShield,
        });
        _fall = new(core.Pool, PriestState, core.Frames, fallFrames, core.Surfaces, state.Fall, falling);
        CarrierCheck = new(core.Pool, core.Types, core.Spots, RawPriestFall.MakePriestFallWalker(core.Pool, _fall, _effects.OtherWalkerFall));
        var carrierPost = new RawCarrierPostPop(core.Pool, core.Types, state.Carrier,
            RawCarrierPostPop.MakeCarrierPostPopHooks(core.Pool, core.PostPop, sid => CarrierCheck.Check(sid), _effects.ClaimGround));
        if (core.Pool.Edition == OriginalEdition.Patch1078) carrierPost.BindCurrentTypes(core.Types);
        var prefix = new RawPriestPostPop(core.Pool, core.Reward, state.Priests,
            new() { FindRegular = (sid, ev) => Host().FindEvent(sid, ev) is not null,
                ReserveRegular = _effects.ReserveRegular, AddRegular = (sid, ev, payload) => Host().AddRegular(sid, ev, payload) },
            () => state.Movement.Priest);
        var tail = new RawPriestPostPopTail(core.Pool, prefix, carrierPost, PriestState, core.Frames, core.Spots,
            RawPriestPostPopTail.MakePriestPostPopTailShieldHooks(core.Pool, _shield, Forcefield, _effects.Destroy, _fall.Begin));
        Carrier = _effects.SharedCarrier ?? new(core.Pool, state.Contained, new()
        { ValidateDamageable = (sid, flags) => Damageable().Validate(sid, flags),
            DamageablePre = (sid, flags) => Damageable().PreDestroy(sid, flags), DependentFound = _effects.DependentFound });
        SamePool(Carrier.Pool);
        _priestPre = _effects.SharedPriestPre ?? new(core.Pool, state.Priests,
            RawCarrierPreDestroy.MakeCarrierPreDestroyHooks(core.Pool, Carrier,
                RawPriestForcefield.MakePriestForcefieldHooks(core.Pool, Forcefield, new() { DestroyForcefield = _effects.Destroy })));
        SamePool(_priestPre.Pool);
        var fieldPost = new RawForcefieldPostPop(core.Pool, new() { ReserveRegular = _effects.ReserveRegular,
            AddRegular = (sid, ev, payload) => Host().AddRegular(sid, ev, payload) });
        if (core.Pool.Edition == OriginalEdition.Patch1078)
        {
            if (state.ForcefieldRegular.SharedPriestType is { } current && !ReferenceEquals(current, state.Movement.PriestBox))
                throw new InvalidOperationException("보호막 Regular 현재 사제 TYPE 소유자 불일치");
            state.ForcefieldRegular.SharedPriestType = state.Movement.PriestBox;
        }
        _fieldRegular = _effects.SharedForcefieldRegular ?? new(core.Pool, core.Hash, core.Types, state.ForcefieldRegular,
            ForcefieldLifecycle.MakeForcefieldFrameHooks(core.Pool, advance,
                new() { Destroy = _effects.Destroy, Assertion = _effects.ForcefieldAssertion, FindTypeAt = _effects.FindTypeAt }));
        SamePool(_fieldRegular.Pool);
        _contained = new(core.Pool, _effects.Destroy);
        _regen = new(core.Pool, core.Reward, PriestState, state.HitPoints,
            new() { Unpop = _effects.Unpop, Repop = _effects.Repop }, core.Random, state.Regen,
            new() { Refresh = _effects.Regen.Refresh, Damageable = DamageableRefresh, Shape = MovementProcessType,
                Occupied = (sid, process) => _contained.Find(sid, process) != 0, NeutralAction = _effects.Regen.NeutralAction,
                PatchAudit = _effects.Regen.PatchAudit, Measurement = _effects.Regen.Measurement });
        Spawn = new(core.Pool, core.Types, core.Spots, state.Spawn, new()
        {
            MayPlace = query => Placement().MayPlace(query), Create = (type, flags) => core.Factory.Create(type, (SidAllocation)flags),
            SetOwner = SetOwner, Pop = _effects.Pop, NotifySurface = _effects.NotifySurface, CheckCarrier = sid => CarrierCheck.Check(sid),
        });
        core.PostPop.SetPriestPostPop(tail); core.PostPop.SetForcefieldPostPop(fieldPost);
    }

    /// <summary>공유할 모듈이 다른 실제 풀을 사용하면 거부한다.</summary>
    private void SamePool(RawSidPool pool)
    { if (!ReferenceEquals(pool, _core.Pool)) throw new ArgumentException("사제 월드 실제 Pool 불일치"); }
    /// <summary>도달한 시점에 실제 프로세스 연결을 요구한다.</summary>
    private SquidProcessHost Host() => _host ?? throw new InvalidOperationException("사제 월드 ProcessHost 미연결");
    /// <summary>목록 변경 전에 실제 피해 전처리를 요구한다.</summary>
    private RawDamageablePreDestroy Damageable() => _damageable ?? throw new InvalidOperationException("사제 월드 Damageable 삭제 몸체 미연결");
    /// <summary>해방 후보에 실제 배치 검사를 요구한다.</summary>
    private RawPriestPlacement Placement() => _placement ?? throw new InvalidOperationException("사제 월드 배치 검사 몸체 미연결");
    /// <summary>실제 현재 보호막의 범위만 검사하고 가상 삭제를 요청한다.</summary>
    private void ClearShield(int sid)
    { int found = Forcefield.Find(sid); if (found > 0 && found < _core.Pool.Capacity) _effects.Destroy!(found, 0); }
    /// <summary>같은 풀의 Host를 한 번 연결한다.</summary>
    public void BindProcesses(SquidProcessHost host)
    { ArgumentNullException.ThrowIfNull(host); SamePool(host.Pool()); if (_host is not null) throw new ArgumentException("사제 ProcessHost 중복 연결"); _host = host; }
    /// <summary>같은 풀의 피해 전처리를 한 번 연결한다.</summary>
    public void BindDamageable(RawDamageablePreDestroy damageable)
    { ArgumentNullException.ThrowIfNull(damageable); SamePool(damageable.Pool); if (_damageable is not null) throw new ArgumentException("사제 Damageable 중복 연결"); _damageable = damageable; }
    /// <summary>같은 풀의 실제 배치 검사를 한 번 연결한다.</summary>
    public void BindPlacement(RawPriestPlacement placement)
    { ArgumentNullException.ThrowIfNull(placement); SamePool(placement.Pool); if (_placement is not null) throw new ArgumentException("사제 Placement 중복 연결"); _placement = placement; }
    /// <summary>전투 틱·명령 전에 세 지연 연결을 순서대로 확인한다.</summary>
    public void ValidateReady() { _ = Host(); _ = Damageable(); _ = Placement(); }
    /// <summary>같은 로딩 깊이를 네 소비자에게 전달한다.</summary>
    public void SetLoadingDepth(uint depth)
    { _state.Owner.LoadingDepth = depth; _state.Shield.LoadingDepth = depth; _state.Carrier.LoadingDepth = depth; _state.Regen.NeutralBlocked = depth != 0; }
    /// <summary>공유 권한 연결 전의 독립 입력만 함께 변경한다.</summary>
    public void SetAuthority(bool authority)
    {
        if (_state.HitPoints.SharedAuthority is not null || _state.Fall.SharedAuthority is not null
            || _state.Carrier.SharedBoss is not null || _state.Contained.SharedBoss is not null)
            throw new InvalidOperationException("공유 권한을 대여한 사제 실행기의 값형 권한 변경 금지");
        _state.HitPoints.Authority = authority; _state.Fall.Authority = authority;
        _state.Carrier.Boss = authority; _state.Contained.Boss = authority;
    }
    /// <summary>네 소비자가 같은 owner의 ReadBoss를 읽게 한다. 실패하면 기존 대여를 보존한다.</summary>
    public void BindAuthority(RawAuthorityState authority)
    {
        ArgumentNullException.ThrowIfNull(authority); Func<bool> read = authority.ReadBoss;
        if ((_state.HitPoints.SharedAuthority is { } hp && hp != read) || (_state.Fall.SharedAuthority is { } fall && fall != read)
            || (_state.Carrier.SharedBoss is { } carrier && carrier != read) || (_state.Contained.SharedBoss is { } contained && contained != read))
            throw new ArgumentException("사제 네 소비자의 동일 공유 권한 소유자 필요");
        _state.HitPoints.SharedAuthority = read; _state.Fall.SharedAuthority = read;
        _state.Carrier.SharedBoss = read; _state.Contained.SharedBoss = read;
    }
    /// <summary>같은 판본 Player9 전체 배열만 빌리며 다른 주소 재연결은 거부한다.</summary>
    public void BindPlayerRecords(byte[] players)
    {
        ArgumentNullException.ThrowIfNull(players);
        if (players.Length != 9 * (_core.Pool.Edition == OriginalEdition.Patch1078 ? 0xb4 : 0xac)
            || (_state.HitPoints.PlayerRecords is { Length: > 0 } current && !ReferenceEquals(current, players)))
            throw new ArgumentException("사제 HP의 같은 Player9 고정 기록 필요");
        _state.HitPoints.PlayerRecords = players;
    }
    /// <summary>현재 로컬 플레이어를 소유자·보호막 안내에 함께 적용한다.</summary>
    public void SetLocalPlayer(uint player)
    { if (player > 8) throw new ArgumentOutOfRangeException(nameof(player)); _state.Owner.BattlePlayer = player; _state.Owner.FortPlayer = player; _state.Shield.LocalPlayer = player; }
    /// <summary>사제 재정의·현재 외부 분배·공통 부모 순서로 소유자를 지정한다.</summary>
    public void SetOwner(int sid, uint player)
    { if (_owner.Handles(sid)) _owner.Set(sid, player); else if (_effects.NonPriestOwner is { } other) other(sid, player); else _core.Owner.Set(sid, player); }
    /// <summary>실제 HP·지면 경계를 같은 Unpop/Repop에 연결한다.</summary>
    public void SetHitPoints(int sid, int value) => PriestState.SetHitPoints(sid, value, _state.HitPoints, new() { Unpop = _effects.Unpop, Repop = _effects.Repop });
    /// <summary>실제 Carrier 지면 검사를 실행한다.</summary>
    public bool CheckCarrier(int sid) => CarrierCheck.Check(sid);
    /// <summary>사제의 실제 낙하 또는 다른 이동 유닛의 외부 몸체를 실행한다.</summary>
    public bool FallWalker(int sid) => RawPriestFall.MakePriestFallWalker(_core.Pool, _fall, _effects.OtherWalkerFall)(sid);
    /// <summary>현재 TYPE·프로세스 상자를 호출마다 읽는다.</summary>
    public uint MovementProcessType(uint type) => RawMovementProcessTypes.Select(_core.Pool.Edition, _core.Types, _state.Movement, type);
    /// <summary>free·damageEffect0은 자연 반환하며 나머지는 원래 burning 전체 몸체를 요구한다.</summary>
    public void DamageableRefresh(int sid)
    {
        ReadOnlySpan<byte> raw = _core.Pool.Slot(sid); if ((raw[11] & 1) != 0) return;
        if (raw[10] >= _core.Types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Damageable 현재 타입 범위 오류");
        if (_core.Types[raw[10]].DamageEffect == 0) return;
        (_effects.Regen.Damageable ?? throw new InvalidOperationException("nonzero damageEffect의 burning 전체 몸체 미연결"))(sid);
    }
    /// <summary>회복602·낙하603의 실제 몸체만 선택한다.</summary>
    public float UniqueRegular(int sid, uint ev, uint count, float payload) => ev switch
    { 602 => _regen.Handle(sid, ev, count, payload), 603 => _fall.Handle(sid, ev, count, payload), _ => throw new ArgumentException("다른 사제 회복/추락 사건") };
    /// <summary>실제 TYPE·VT로 사제/보호막 정기를 분배하며 나머지는 기존 부모 또는 payload다.</summary>
    public RegularHandler RegularHandler(RegularHandler? fallback = null) => (sid, ev, count, payload) =>
    {
        ReadOnlySpan<byte> raw = _core.Pool.Slot(sid); bool patch = _core.Pool.Edition == OriginalEdition.Patch1078;
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
        if (raw[10] == 158 && table == (patch ? 0x0050f210u : 0x005003e0u))
            return RawPriestRegular.Run(sid, ev, count, payload, new() { Destroy = _effects.Destroy, Carrier = fallback,
                Unique = (parent, code, number, value) => code is 602 or 603 ? UniqueRegular(parent, code, number, value)
                    : (fallback ?? throw new InvalidOperationException("사제 월드 고유 정기 미연결"))(parent, code, number, value) });
        if ((patch || raw[10] == 167) && table == (patch ? ForcefieldLifecycle.PatchVtable : ForcefieldLifecycle.CdVtable))
            return _fieldRegular.Handle(sid, ev, count, payload);
        return fallback?.Invoke(sid, ev, count, payload) ?? payload;
    };
    /// <summary>기존 Form·선택·공통 효과를 보존하고 사제 전처리만 분배한다.</summary>
    public SquidDestroyHooks DestroyHooks(SquidDestroyHooks fallback) => _priestPre.WrapHooks(_core.Pool, fallback);
    /// <summary>해방과 같은 실제 나선 생성기다.</summary>
    public RawPriestSpawn Spawn { get; }
    /// <summary>조회·움직임이 공유하는 실제 사제 상태 계산기다.</summary>
    public RawPriestState PriestState { get; }
    /// <summary>실제 공간·현재TYPE의 보호막 조회다.</summary>
    public RawPriestForcefield Forcefield { get; }
    /// <summary>이동·도착·삭제가 공유하는 실제 Carrier다.</summary>
    public RawCarrierPreDestroy Carrier { get; }
    /// <summary>Halt·후처리와 같은 지면 검사다.</summary>
    public RawCarrierCheck CarrierCheck { get; }
    /// <summary>미션 결과까지 같은 목록과 소유자 상태다.</summary>
    public PriestPostPopState Priests => _state.Priests;
    /// <summary>외부 Kernel 정리 뒤 공통 후처리 등록과 Player9 대여를 한 번 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _core.PostPop.SetPriestPostPop(null); _core.PostPop.SetForcefieldPostPop(null);
        _state.HitPoints.PlayerRecords = null; _disposed = true;
    }
}
