using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 사제 회복 패치 처리 중 조회·갱신하는 상태 묶음(원본 <c>PriestRegenPatchState</c>).
/// </summary>
public sealed class PriestRegenPatchState
{
    /// <summary>원본 Next(9)의 결과.</summary>
    public uint Sample { get; set; }

    /// <summary>3씩 증가하고 9600을 한 번 빼는 추적 값.</summary>
    public int Sequence { get; set; }

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
}

/// <summary>
/// 사제 이벤트 0x25a의 실제 HP 회복과 중립 후처리·패치 추적 순서를 복원한다(원본 <c>RawPriestRegen</c>).
/// 회복 이벤트 분기만 실행한다. 분배기(MakePriestRegenHandler)와 실제 저장소 읽기는 R4 세션 조립 때 연결한다.
/// 기준 판본은 10.78 뿐이다.
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

    /// <summary>원본 전역 난수.</summary>
    private readonly NetstormRandom _random;

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
        PriestHitPointMode mode, PriestHitPointHooks space, NetstormRandom random,
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
            || hooks.PatchAudit is null || hooks.Measurement is null)
        {
            throw new ArgumentException("사제 회복 풀/외부 효과 연결 오류", nameof(hooks));
        }
        _pool = pool;
        _hp = hp;
        _priest = priest;
        _mode = mode;
        _space = space;
        _random = random;
        _state = state;
        _hooks = hooks;
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
        int current = _priest.CurrentHitPoints(sid);
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[10] != 158 || Vtable(raw) != 0x0050F210u)
        {
            throw new InvalidOperationException("사제 회복 타입·가상 표 오류");
        }
        PriestRegenPatchState tracking = _state.Patch;
        tracking.Sample = (uint)_random.Next(9);
        tracking.Sequence = Add(tracking.Sequence, 3);
        if (tracking.Sequence > SequenceLimit)
        {
            tracking.Sequence = Add(tracking.Sequence, -SequenceLimit);
        }
        if (current < _hp.MaxHitPoints(sid))
        {
            int candidate = Add(_priest.CurrentHitPoints(sid), _hp.TypeHitPoints(158) / 6);
            int value = candidate > _hp.MaxHitPoints(sid)
                ? _hp.MaxHitPoints(sid)
                : Add(_priest.CurrentHitPoints(sid), _hp.TypeHitPoints(158) / 6);
            _priest.SetHitPoints(sid, value, _mode, _space);
            _hooks.Refresh!(sid);
            _hooks.Damageable!(sid);
        }
        _hooks.PatchAudit!();
        // HP setter·외부 효과가 바꾼 현재 소유자·HP·전역을 이 시점에 다시 읽는다
        byte owner = _pool.Slot(sid)[34];
        if (_mode.GetAuthority() && !_state.NeutralBlocked && owner == 0 && !_state.ModeBlocked &&
            _priest.CurrentHitPoints(sid) >= _hp.MaxHitPoints(sid) / 2)
        {
            uint shape = _hooks.Shape!(_pool.Slot(sid)[10]);
            if (!_hooks.Occupied!(sid, shape))
            {
                _hooks.NeutralAction!(sid);
            }
        }
        int observed = tracking.ObservedSequence;
        int sequence = tracking.Sequence;
        // 중립·외부 효과 뒤 같은 현재 추적 전역을 다시 읽는다
        if (observed != sequence)
        {
            tracking.ChangedSample = tracking.Sample;
        }
        if (tracking.Remaining != 0)
        {
            if (_hooks.Measurement!() > MeasurementLimit)
            {
                tracking.ChangedSample = tracking.Sample;
            }
            tracking.Remaining--;
        }
        // 원본은 두 독립 if다. 첫 배열이 없어도 뒤의 signed WORD를 반드시 현재 읽는다
        if (tracking.Gate == 0)
        {
            tracking.ChangedSample = tracking.Sample;
        }
        if (tracking.Sentinel == -1)
        {
            tracking.ChangedSample = tracking.Sample;
        }
        return payload;
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
