namespace Netstorm.Core.Original;

/// <summary>
/// 피해가능객체의 HP 조회 전역(원본 SquidRewardState 의 HP 관련 부분).
/// </summary>
public sealed class RawHitPointState
{
    /// <summary>제단(altar) 타입 번호(원본 DAT_005412e8). HP 를 그대로 쓰지 않는다.</summary>
    public uint NoneType { get; set; } = 164;

    /// <summary>받침(dais) 타입 번호(원본 DAT_005412ec). 제단과 같은 HP 규칙을 쓴다.</summary>
    public uint DaisType { get; set; } = 165;

    /// <summary>사제(priest) 타입 번호(원본 DAT_005412d0). 사제 전용 HP 1/4 규칙의 대상이다.</summary>
    public uint PriestType { get; set; } = 158;

    /// <summary>사제 전용 HP 1/4 규칙을 켜는지(원본 DAT_005ca8f0).</summary>
    public bool PriestQuarterHp { get; set; }

    /// <summary>원본 0049fce0 의 디버그 assert 를 켜는지(원본 DAT_005e4794).</summary>
    public bool DebugAsserts { get; set; }
}

/// <summary>
/// 피해가능객체의 HP 조회(원본 SquidReward 의 HasHitPoints·MaxHitPoints·TypeHitPoints 부분).
/// 서버 생성 때 채워진 HP 를 읽고, 제단(dais)은 abstract 비트 여부로 HP 유무를 정한다.
/// SP·지갑·샘 풀 계산은 이 몸체에 포함하지 않는다(삭제 보상 계산 조각에서 옮긴다).
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/SquidReward.cpp (W03 삭제·수명 일곱째 조각, 2026-10-21).
/// </summary>
public sealed class RawHitPoints
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>타입 플래그1의 HP 보유 비트(원본 0x10).</summary>
    private const uint HasHitPointBit = 0x10;

    /// <summary>타입 플래그2의 제단(dais) 비트(원본 0x400000).</summary>
    private const uint DaisKind = 0x400000;

    /// <summary>객체 슬롯을 읽는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 최대 HP 를 조회하는 타입 표.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>제단·사제·사제 전용 HP 전역.</summary>
    private readonly RawHitPointState _state;

    /// <summary>
    /// HP 조회 몸체를 만든다. 타입 표 크기를 확인한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="state">제단·사제 HP 전역.</param>
    public RawHitPoints(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawHitPointState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"타입 레코드가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _state = state;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>원본과 같이 번호·타입 범위를 확인한 뒤 슬롯을 읽는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    private ReadOnlySpan<byte> Raw(int sid)
    {
        if (sid <= 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "HP 조회 SID 범위 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), raw[TypeOffset], "HP 조회 타입 범위 오류");
        }
        return raw;
    }

    /// <summary>타입 번호의 최대 HP 를 돌려준다. 사제 전용 HP 1/4 규칙을 적용한다.</summary>
    /// <param name="type">타입 번호.</param>
    public int TypeHitPoints(uint type)
    {
        if (type >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "HP 조회 타입 범위 오류");
        }
        OriginalTypeRecord record = _types[(int)type];
        // 서버가 아닌 타입은 HP 가 없지만 원본도 UI 경고만 낸다.
        if (_state.DebugAsserts && (record.Flags1 & HasHitPointBit) == 0 && type != _state.DaisType)
        {
            throw new InvalidOperationException("HP 없는 타입의 assert 경로");
        }
        return _state.PriestQuarterHp && type == _state.PriestType
            ? record.MaxHitPoints / 4
            : record.MaxHitPoints;
    }

    /// <summary>객체가 HP 를 갖는지 확인한다. 제단은 abstract 비트 여부로 정한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public bool HasHitPoints(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        OriginalTypeRecord type = _types[raw[TypeOffset]];
        return (type.Flags2 & DaisKind) != 0
            ? (raw[ExtraOffset] & 1) != 0
            : (type.Flags1 & HasHitPointBit) != 0;
    }

    /// <summary>객체의 최대 HP 를 돌려준다. 제단은 abstract 비트에 따라 0 또는 제단 타입의 HP 다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public int MaxHitPoints(int sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        OriginalTypeRecord type = _types[raw[TypeOffset]];
        if ((type.Flags2 & DaisKind) == 0)
        {
            return TypeHitPoints(raw[TypeOffset]);
        }
        // 제단 계열은 abstract 면 HP 를 갖지 않고, 그렇지 않으면 제단 타입의 HP 를 쓴다.
        return (raw[ExtraOffset] & 1) != 0 ? TypeHitPoints(_state.NoneType) : 0;
    }
}
