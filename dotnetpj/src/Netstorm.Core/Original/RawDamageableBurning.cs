// 현재 HP/TYPE·진단 복귀·위치 소리 뒤 생성 경계를 원문 순서로 연결한다.
namespace Netstorm.Core.Original;

/// <summary>
/// 실제 원본 burning process 14 조회·생성·삭제 경계(원본 DamageableBurningHooks).
/// </summary>
public sealed class DamageableBurningHooks
{
    /// <summary>현재 부모의 burning process 14 에서 index 번째 Form 번호를 찾는다. 0 은 자연스러운 없음이다.</summary>
    public Func<int, uint, uint, int>? Find { get; set; }

    /// <summary>실제 burning 생성자가 만든 Form 번호를 돌려준다.</summary>
    public Func<int, int>? Create { get; set; }

    /// <summary>찾아낸 burning Form 을 같은 월드의 실제 삭제기로 제거한다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>실제 위치의 bulfHurt.wav 를 재생한다. 다른 타입에는 요구하지 않는다.</summary>
    public Action<int, float, float>? BulfSound { get; set; }

    /// <summary>중복 프로세스 원문 식/파일/행을 보고하며 정상 복귀하면 저장한 판정을 이어간다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>함수 객체는 값 복사하고 실제 월드 수명은 캡처한 같은 owner를 빌린다.</summary>
    internal DamageableBurningHooks Copy() => new()
    { Find = Find, Create = Create, Destroy = Destroy, BulfSound = BulfSound, Assertion = Assertion };
}

/// <summary>
/// 원본 디버그 검사와 현재 타입 전역(원본 DamageableBurningState).
/// </summary>
public sealed class DamageableBurningState
{
    /// <summary>패치판만 두 번째 burning 이 없는지 확인하는 원본 전역이다.</summary>
    public RawCheckingFlag Checking { get => _checking; set => _checking = new(value); }

    /// <summary>원문 진단 뷰는 값 복사하고 실제 현재DWORD owner는 같은 주소로 빌린다.</summary>
    private RawCheckingFlag _checking = new();

    /// <summary>원본 process 14 타입의 현재 값.</summary>
    public uint BurningType { get; set; } = 14;

    /// <summary>원본 bulf 135 타입의 현재 값.</summary>
    public uint BulfType { get; set; } = 135;
}

/// <summary>
/// 피해가능객체의 현재 HP·최대 HP 절반 비교와 실제 burning 프로세스 생성·제거 분기를 복원한다(원본 RawDamageableBurning, 0044c060).
/// free·피격 효과 0 은 자연 반환하고, 저 HP 면 생성·기존 유지, 회복 후에는 기존을 삭제한다.
/// PATCH signedDWORD/CD signedWORD HP와 진단 선택을 구별한다.
/// 근거: cpppj/src/o/RawDamageableBurning.cpp (W03 삭제·수명 여덟째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageableBurning
{
    /// <summary>슬롯 안 HP 필드의 위치.</summary>
    private const int HealthOffset = 26;

    /// <summary>객체 슬롯을 읽는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 타입의 피격 효과·최대 HP 자료.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>최대 HP 조회 몸체.</summary>
    private readonly Func<uint, int> _typeHitPoints;

    /// <summary>현재 검사·프로세스 타입 전역.</summary>
    private readonly DamageableBurningState _state;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly DamageableBurningHooks _hooks;

    /// <summary>
    /// burning 판정 몸체를 만든다. HP 조회 몸체의 풀과 타입 표 크기를 확인한다.
    /// </summary>
    /// <param name="pool">피격 대상과 burning Form 이 공유하는 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드.</param>
    /// <param name="hp">최대 HP 조회 몸체. 같은 풀을 써야 한다.</param>
    /// <param name="state">현재 검사·프로세스 타입 전역.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    public RawDamageableBurning(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawHitPoints hp,
        DamageableBurningState state, DamageableBurningHooks hooks)
        : this(pool, types, hp?.Pool ?? throw new ArgumentNullException(nameof(hp)), hp.TypeHitPoints, state, hooks) { }

    /// <summary>같은 실제 보상 HP의 현재 TYPE 조회를 빌린다. 부모와 TYPE/상태는 더 오래 살아야 한다.</summary>
    public RawDamageableBurning(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawSquidReward hp,
        DamageableBurningState state, DamageableBurningHooks hooks)
        : this(pool, types, hp?.Pool ?? throw new ArgumentNullException(nameof(hp)), hp.TypeHitPoints, state, hooks) { }

    /// <summary>선택된 실제 HP 부모와 현재 자료를 구성 때 확인하고 함수 사본을 보관한다.</summary>
    private RawDamageableBurning(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawSidPool hpPool,
        Func<uint, int> typeHitPoints, DamageableBurningState state, DamageableBurningHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(hpPool, pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new ArgumentException("burning HP·타입·풀 연결 오류", nameof(types));
        }
        _pool = pool;
        _types = types;
        _typeHitPoints = typeHitPoints;
        _state = state;
        _hooks = hooks.Copy();
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// burning 전체 판정을 수행한다. 저 HP 면 생성·기존 유지, 회복 후에는 기존을 삭제한다.
    /// </summary>
    /// <param name="victim">피격 대상 슬롯 번호.</param>
    /// <returns>남아 있는 burning Form 번호. 없으면 0.</returns>
    public int Refresh(int victim)
    {
        if (victim <= 0 || victim >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(victim), victim, "burning 대상 번호 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(victim);
        if ((raw[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0)
        {
            return 0;
        }
        if (raw[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(victim), raw[RawSidPool.TypeOffset], "burning 대상 타입 오류");
        }
        if (_types[raw[RawSidPool.TypeOffset]].DamageEffect == 0)
        {
            return 0;
        }
        if (_hooks.Find is null)
        {
            throw new InvalidOperationException("burning 조회 콜백이 연결되지 않았습니다");
        }
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int current = patch ? unchecked((int)Read(raw, HealthOffset)) : unchecked((short)Read(raw, HealthOffset, 2));
        int maximum = _typeHitPoints(raw[RawSidPool.TypeOffset]);
        int doubled = unchecked((int)unchecked((uint)maximum * 2u));
        int half = doubled / 4;
        int found = _hooks.Find(victim, _state.BurningType, 0);
        if (patch && _state.Checking && _hooks.Find(victim, _state.BurningType, 1) != 0)
        {
            if (_hooks.Assertion is null) throw new InvalidOperationException("burning 두 번째 프로세스 원본 진단 장치 미연결");
            _hooks.Assertion("findProcess (mySid, burningSystemProcessType, 1 ) == 0", @"\Ns\O\Damageable.cpp", 0x210);
        }
        if (current < half)
        {
            if (found == 0)
            {
                if (raw[RawSidPool.TypeOffset] == _state.BulfType && _hooks.BulfSound is null)
                {
                    throw new InvalidOperationException("burning 생성·소리 경계가 연결되지 않았습니다");
                }
                if (raw[RawSidPool.TypeOffset] == _state.BulfType)
                {
                    _hooks.BulfSound!(victim,
                        BitConverter.UInt32BitsToSingle(Read(raw, 14)),
                        BitConverter.UInt32BitsToSingle(Read(raw, 18)));
                }
                if (_hooks.Create is null) throw new InvalidOperationException("burning 실제 생성 몸체 미연결");
                found = _hooks.Create(victim);
            }
        }
        else if (found != 0)
        {
            if (_hooks.Destroy is null)
            {
                throw new InvalidOperationException("burning 삭제 경계가 연결되지 않았습니다");
            }
            _hooks.Destroy(found, 0);
            return 0;
        }
        return found;
    }

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 요청한 원본 필드 폭만 낮은 바이트부터 조합한다.
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (i * 8);
        }
        return value;
    }
}
