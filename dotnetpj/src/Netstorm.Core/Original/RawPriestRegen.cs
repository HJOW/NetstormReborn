namespace Netstorm.Core.Original;

/// <summary>
/// 사제 회복 패치 처리 중 조회·갱신하는 상태 묶음(원본 <c>PriestRegenPatchState</c>).
/// </summary>
public sealed class PriestRegenPatchState
{
    /// <summary>원본 Next(9)의 결과.</summary>
    public uint Sample { get; set; }

    /// <summary>3씩 증가하고 9600을 한 번 빼는 추적 값.</summary>
    public int Sequence { get => SequenceBox.Value; set => SequenceBox.Value = value; }

    /// <summary>SP와 회복이 실제 주소로 대여하는 독립 추적 DWORD다.</summary>
    public IntBox SequenceBox { get; } = new();

    /// <summary>독립 fixture의 입력. 실제 세션은 저장소를 직접 읽는다.</summary>
    public int ObservedSequence { get; set; }

    /// <summary>추적 조건이 참이면 현재 sample을 쓰는 진단 DWORD.</summary>
    public uint ChangedSample { get; set; }

    /// <summary>0이 아니면 측정 뒤 하나 줄인다.</summary>
    public uint Remaining { get; set; }

    /// <summary>독립 fixture의 입력. 실제 세션은 첫 분산배열 주소를 직접 읽는다.</summary>
    public uint Gate { get; set; }

    /// <summary>독립 fixture의 signed WORD 입력. 실제 세션은 같은 순열 배열을 읽는다.</summary>
    public short Sentinel { get; set; }

    /// <summary>실제 Client의 같은 현재 표본 주소이며 독립 Sample은 보존한다.</summary>
    public UIntBox? SharedSample { get; set; }
    /// <summary>실제 Client의 같은 현재 진단 주소이며 독립 ChangedSample은 보존한다.</summary>
    public UIntBox? SharedChangedSample { get; set; }
    /// <summary>각 원문 읽기 위치의 현재 표본이다.</summary>
    public uint ReadSample() => SharedSample?.Value ?? Sample;
    /// <summary>현재 표본 주소에만 쓴다.</summary>
    public void WriteSample(uint value) { if (SharedSample is { } shared) shared.Value = value; else Sample = value; }
    /// <summary>현재 진단 주소에만 쓴다.</summary>
    public void WriteChangedSample(uint value) { if (SharedChangedSample is { } shared) shared.Value = value; else ChangedSample = value; }
}

/// <summary>
/// 사제 회복 처리 중 조회·갱신하는 상태 묶음(원본 <c>PriestRegenState</c>).
/// </summary>
public sealed class PriestRegenState
{
    /// <summary>중립 사제 후처리를 차단하는 0 아님 조건.</summary>
    public bool NeutralBlocked { get; set; }

    /// <summary>두 번째 차단 전역. 게임 내 의미는 미확정이다.</summary>
    public bool ModeBlocked { get; set; }

    /// <summary>패치 상태.</summary>
    public PriestRegenPatchState Patch { get; } = new();

    /// <summary>증분마다 실제 현재 사제TYPE를 빌리며 Actor의 TYPE와 구별한다.</summary>
    public UIntBox? SharedPriestType { get; set; }
    /// <summary>연결된 장기 번호 또는 독립 초기158을 읽는다.</summary>
    public uint ReadPriestType() => SharedPriestType?.Value ?? 158;
}

/// <summary>실제 SP 전역을 원문 읽기 위치에서 조회하는 함수 사본이다.</summary>
public sealed class PriestRegenSpReads
{
    /// <summary>중립 효과 뒤 현재 비트 위치 signed DWORD다.</summary>
    public Func<int>? ObservedSequence { get; set; }
    /// <summary>측정 뒤 현재 첫 분산배열의 실제 주소다.</summary>
    public Func<uint[]?>? FirstPool { get; set; }
    /// <summary>앞의 null 검사와 독립된 현재 순열의 signed WORD다.</summary>
    public Func<short>? FirstPermutation { get; set; }
    /// <summary>실제 공통 시작에서 연결한 추적 owner이며 생략 시 독립 patch.SequenceBox다.</summary>
    public Func<IntBox>? TrackingSequence { get; set; }
    /// <summary>함수 객체를 복사하고 캡처한 장기 저장소는 그대로 빌린다.</summary>
    internal PriestRegenSpReads Copy() => new() { ObservedSequence = ObservedSequence, FirstPool = FirstPool, FirstPermutation = FirstPermutation, TrackingSequence = TrackingSequence };
}

/// <summary>
/// 사제 회복 외부 동작과 저장소 조회를 연결하는 콜백 묶음(원본 <c>PriestRegenHooks</c>).
/// 실제 저장소 읽기(spReads·BindPriestRegenSpStore)는 R4 세션 조립 때 연결하며, 독립 검사는 값형 입력을 쓴다.
/// 근거: cpppj/src/o/RawPriestRegen.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class PriestRegenHooks
{
    /// <summary>회복을 시도한 뒤 가상 +0x88을 부른다. HP가 거부되어도 호출한다.</summary>
    public Action<int>? Refresh { get; set; }

    /// <summary>후처리의 전체 효과. 외부 경계다.</summary>
    public Action<int>? Damageable { get; set; }

    /// <summary>타입의 모양 값을 공급한다.</summary>
    public Func<uint, uint>? Shape { get; set; }

    /// <summary>shape 자리에 중립 사제가 있으면 참을 돌려준다.</summary>
    public Func<int, uint, bool>? Occupied { get; set; }

    /// <summary>조건을 만족하고 점유 결과가 0일 때 부른다.</summary>
    public Action<int>? NeutralAction { get; set; }

    /// <summary>패치 전용 추적 갱신. 추적·SP 전역 효과는 호출자가 공급한다.</summary>
    public Action? PatchAudit { get; set; }

    /// <summary>패치 전용 측정 결과. remaining이 0이 아닐 때만 읽는다.</summary>
    public Func<double>? Measurement { get; set; }

    /// <summary>실제 세션의 현재 SP 조회기이며 독립 fixture는 비워 둔다.</summary>
    public PriestRegenSpReads? SpReads { get; set; }
    /// <summary>구성 시 중첩 조회기까지 값 복사한다.</summary>
    internal PriestRegenHooks Copy() => new() { Refresh = Refresh, Damageable = Damageable, Shape = Shape,
        Occupied = Occupied, NeutralAction = NeutralAction, PatchAudit = PatchAudit, Measurement = Measurement, SpReads = SpReads?.Copy() };
}

/// <summary>
/// 사제 이벤트 0x25a의 실제 HP 회복과 중립 후처리·패치 추적 순서를 복원한다(원본 <c>RawPriestRegen</c>).
/// 회복 이벤트·가상 분배·선택 SP 조회기를 제공한다. 제품 Session의 실제 효과 연결은 호출자가 공급한다.
/// typed 두 판본과 현재 SP·사제TYPE·추적 owner를 같은 실제 읽기 위치에서 사용한다.
/// 근거: cpppj/src/o/RawPriestRegen.cpp (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class RawPriestRegen
{
    /// <summary>사제 회복 이벤트 번호.</summary>
    private const uint PriestRegenEvent = 0x25a;

    /// <summary>패치 추적 순환 상한.</summary>
    private const int SequenceLimit = 9600;

    /// <summary>측정 비교의 원본 float 리터럴.</summary>
    private const double MeasurementLimit = 7000.0;

    /// <summary>동일 월드의 HP·사제 모듈과 전역 난수를 받는다. 외부 공간·표시 경계는 객체보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>객체의 HP를 조회·변경하는 처리기.</summary>
    private readonly RawSquidReward _hp;

    /// <summary>사제 상태를 조회·변경하는 처리기.</summary>
    private readonly RawPriestState _priest;

    /// <summary>동작 분기를 선택하는 모드.</summary>
    private readonly PriestHitPointMode _mode;

    /// <summary>공간 경계.</summary>
    private readonly PriestHitPointHooks _space;

    /// <summary>원본 전역 난수. 2026-11-12 0.457에서 단일 공유 스트림으로 바꿨다.</summary>
    private readonly GameRandom _random;

    /// <summary>처리 중 조회·갱신하는 상태.</summary>
    private readonly PriestRegenState _state;

    /// <summary>외부 저장소 조회와 가상 동작.</summary>
    private readonly PriestRegenHooks _hooks;

    /// <summary>
    /// 사제 회복 처리기를 만든다. 풀·모듈·난수·상태는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀.</param>
    /// <param name="hp">HP 조회·변경 처리기. 같은 풀을 써야 한다.</param>
    /// <param name="priest">사제 상태 처리기. 같은 풀을 써야 한다.</param>
    /// <param name="mode">동작 분기를 선택하는 모드.</param>
    /// <param name="space">공간 경계. unpop·repop이 있어야 한다.</param>
    /// <param name="random">원본 전역 난수.</param>
    /// <param name="state">처리 중 상태.</param>
    /// <param name="hooks">외부 동작. 7갈래가 모두 있어야 한다.</param>
    public RawPriestRegen(RawSidPool pool, RawSquidReward hp, RawPriestState priest,
        PriestHitPointMode mode, PriestHitPointHooks space, GameRandom random,
        PriestRegenState state, PriestRegenHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hp);
        ArgumentNullException.ThrowIfNull(priest);
        ArgumentNullException.ThrowIfNull(mode);
        ArgumentNullException.ThrowIfNull(space);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(hp.Pool, pool) || !ReferenceEquals(priest.Pool, pool)
            || space.Unpop is null || space.Repop is null
            || hooks.Refresh is null || hooks.Damageable is null || hooks.Shape is null
            || hooks.Occupied is null || hooks.NeutralAction is null
            || (pool.Edition == OriginalEdition.Patch1078 && (hooks.PatchAudit is null || hooks.Measurement is null))
            || (hooks.SpReads is { } reads && (reads.ObservedSequence is null || reads.FirstPool is null || reads.FirstPermutation is null)))
        {
            throw new ArgumentException("사제 회복 풀/외부 효과 연결 오류", nameof(hooks));
        }
        _pool = pool;
        _hp = hp;
        _priest = priest;
        _mode = mode;
        _space = new() { Unpop = space.Unpop, Repop = space.Repop };
        _random = random;
        _state = state;
        _hooks = hooks.Copy();
    }

    /// <summary>분배기의 동일 풀 보호에 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 회복 이벤트 분기만 실행한다. count는 원본도 읽지 않으며 반환 payload 비트를 유지한다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="event">사제 이벤트. 0x25a가 아니면 거부한다.</param>
    /// <param name="count">원본도 읽지 않는 인자.</param>
    /// <param name="payload">그대로 돌려주는 반환 payload.</param>
    public float Handle(int sid, uint @event, uint count, float payload)
    {
        if (@event != PriestRegenEvent)
        {
            throw new ArgumentException("미복원 사제 이벤트", nameof(@event));
        }
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (Vtable(raw) != (patch ? 0x0050F210u : 0x005003e0u))
            throw new InvalidOperationException("사제 회복 타입·가상 표 오류");
        PriestRegenPatchState tracking = _state.Patch;
        // 각 원문 참조 확보 위치에서 같은 현재 추적 주소를 선택한다.
        IntBox Sequence() => _hooks.SpReads?.TrackingSequence is { } select
            ? select() ?? throw new InvalidOperationException("사제 회복 실제 추적 owner 누락") : tracking.SequenceBox;
        if (patch)
        {
            tracking.WriteSample(_random.Next(9u)); IntBox sequence = Sequence();
            sequence.Value = Add(sequence.Value, 3);
            if (sequence.Value > SequenceLimit) sequence.Value = Add(sequence.Value, -SequenceLimit);
        }
        int? value = RawPriestRegenHitPoints.Read(_pool.Edition, new()
        { Current = () => _priest.CurrentHitPoints(sid), Maximum = () => _hp.MaxHitPoints(sid),
            PriestType = _state.ReadPriestType, TypeMaximum = _hp.TypeHitPoints });
        if (value is { } healed)
        {
            _priest.SetHitPoints(sid, healed, _mode, _space); _hooks.Refresh!(sid); _hooks.Damageable!(sid);
        }
        if (patch) _hooks.PatchAudit!();
        byte owner = _pool.Slot(sid)[patch ? 34 : 32];
        if (_mode.GetAuthority() && !_state.NeutralBlocked && owner == 0 && !_state.ModeBlocked &&
            _priest.CurrentHitPoints(sid) >= _hp.MaxHitPoints(sid) / 2)
        {
            uint shape = _hooks.Shape!(_pool.Slot(sid)[10]);
            if (!_hooks.Occupied!(sid, shape)) _hooks.NeutralAction!(sid);
        }
        if (patch)
        {
            int observed = _hooks.SpReads is { } reads ? reads.ObservedSequence!() : tracking.ObservedSequence;
            int sequence = Sequence().Value;
            if (observed != sequence) tracking.WriteChangedSample(tracking.ReadSample());
            if (tracking.Remaining != 0)
            {
                if (_hooks.Measurement!() > MeasurementLimit) tracking.WriteChangedSample(tracking.ReadSample());
                tracking.Remaining--;
            }
            // 두 독립 if이므로 첫 배열이 null이어도 뒤 순열 WORD는 반드시 현재 읽는다.
            if (_hooks.SpReads is { } poolReads ? poolReads.FirstPool!() is null : tracking.Gate == 0)
                tracking.WriteChangedSample(tracking.ReadSample());
            if ((_hooks.SpReads is { } permutationReads ? permutationReads.FirstPermutation!() : tracking.Sentinel) == -1)
                tracking.WriteChangedSample(tracking.ReadSample());
        }
        return payload;
    }

    /// <summary>독립 사제 추적 주소와 SP의 실제 대여 주소가 같아야 현재 조회기를 만든다.</summary>
    public static PriestRegenSpReads BindPriestRegenSpStore(RawSpStore store, PriestRegenPatchState patch)
    {
        ArgumentNullException.ThrowIfNull(store); ArgumentNullException.ThrowIfNull(patch);
        if (!ReferenceEquals(store.TrackingOffsetCurrent, patch.SequenceBox)) throw new ArgumentException("사제 회복과 SP 저장소의 추적 주소 불일치");
        return new() { ObservedSequence = () => unchecked((int)store.OffsetCurrent), FirstPool = () => store.FirstPoolCurrent,
            FirstPermutation = () => store.Permutation[0] };
    }
    /// <summary>공통 시작에서 연결한 같은 장기 추적 owner를 사본 없이 읽고 쓴다.</summary>
    public static PriestRegenSpReads BindPriestRegenSpStore(RawSpStore store)
    {
        ArgumentNullException.ThrowIfNull(store); _ = store.TrackingOffsetCurrent;
        return new() { ObservedSequence = () => unchecked((int)store.OffsetCurrent), FirstPool = () => store.FirstPoolCurrent,
            FirstPermutation = () => store.Permutation[0], TrackingSequence = () => store.TrackingOffsetCurrent };
    }
    /// <summary>실제 가상표의 회복602만 처리하고 다른 사제 사건은 부모 없으면 진단한다.</summary>
    public static RegularHandler MakePriestRegenHandler(RawSidPool pool, RawPriestRegen regen, RegularHandler? fallback = null)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(regen);
        if (!ReferenceEquals(pool, regen.Pool)) throw new ArgumentException("사제 회복 분배 풀 불일치");
        return (sid, ev, count, payload) =>
        {
            bool priest = Vtable(pool.Slot(sid)) == (pool.Edition == OriginalEdition.Patch1078 ? 0x0050f210u : 0x005003e0u);
            if (priest && ev == PriestRegenEvent) return regen.Handle(sid, ev, count, payload);
            if (fallback is not null) return fallback(sid, ev, count, payload);
            if (priest) throw new ArgumentException("미복원 사제 이벤트 분배"); return payload;
        };
    }

    /// <summary>정렬되지 않은 vtable DWORD를 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    private static uint Vtable(ReadOnlySpan<byte> raw)
    {
        uint value = 0;
        // 낮은 바이트부터 원본 주소 기록값을 조합한다
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)raw[i] << (8 * i);
        }
        return value;
    }

    /// <summary>원본 signed ADD의 32비트 감김을 C# 부호 오버플로 없이 표현한다.</summary>
    /// <param name="a">피가산수.</param>
    /// <param name="b">가산수.</param>
    private static int Add(int a, int b) => unchecked((int)((uint)a + (uint)b));
}
