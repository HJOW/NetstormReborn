// 원본 Squid의 hasLevel/getLevel/setLevel을 실제 슬롯 상태 바이트에 연결한다.
namespace Netstorm.Core.Original;

// 레벨 조회 몸체를 담는 네임스페이스다. 2026-11-19 0.624에서 R4-2 공장 Pop 파생용으로 추가했다.

/// <summary>
/// 원본 setter는 범위 진단 후에도 바이트 저장을 계속한다.
/// 근거: cpppj <c>o/RawSquidLevel.h</c>의 <c>SquidLevelAssertion</c>.
/// </summary>
/// <remarks>2026-11-19 0.624에서 R4-2 공장 Pop 파생용으로 추가했다. 실제 signed n&lt;0 또는 n&gt;=8의 진단이다.</remarks>
public enum SquidLevelAssertion
{
    /// <summary>레벨이 0~7 범위를 벗어났다. 2026-11-19 0.624에서 추가했다.</summary>
    OutOfRange = 0,
}

/// <summary>
/// 같은 Client 진단 장치를 원문 식/파일/행으로 호출하며 선택되지 않은 누락은 요구하지 않는다.
/// 근거: cpppj <c>o/RawSquidLevel.cpp</c>의 <c>MakeSquidLevelAssertion</c>.
/// </summary>
/// <remarks>2026-11-19 0.624에서 R4-2 공장 Pop 파생용으로 추가했다.</remarks>
public static class SquidLevelDiagnostics
{
    /// <summary>양 판본의 범위 진단을 전달한다. 복귀하면 setter가 현재 상태 바이트를 다시 읽는다.</summary>
    /// <param name="edition">원본 판본이다. 진단 파일 경로가 갈린다.</param>
    /// <param name="assertion">원문 식/파일/행을 받는 실제 진단이다. null이면 최초 OutOfRange에서 거부한다.</param>
    /// <returns>레벨 범위 진단을 받는 장치다.</returns>
    public static Action<SquidLevelAssertion> MakeSquidLevelAssertion(OriginalEdition edition, Action<string, string, uint>? assertion)
    {
        return kind =>
        {
            if (kind != SquidLevelAssertion.OutOfRange || assertion is null)
            {
                throw new InvalidOperationException("Squid level 실제 범위 진단 장치 미연결");
            }

            assertion("0 <= n && n < 8", edition == OriginalEdition.Patch1078 ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", edition == OriginalEdition.Patch1078 ? 0x579u : 0x562u);
        };
    }
}

/// <summary>
/// PATCH004ad0a0/004ad0f0/004ad110 및 CD004acdc0/004ace00/004ace20의 base 몸체다.
/// 근거: cpppj <c>o/RawSquidLevel.h/.cpp</c>의 <c>RawSquidLevel</c>.
/// </summary>
/// <remarks>2026-11-19 0.624에서 R4-2 공장 Pop 파생용으로 추가했다. 가상 분배기는 실제 base+80/+84인지를 먼저 선택한다.</remarks>
public sealed class RawSquidLevel
{
    /// <summary>원문 예약/반납 영역까지 포함하는 실제 물리 슬롯 저장소다. 이 몸체보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 raw 타입의 원본 타입 조회 범위를 확인한다. 이 몸체보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>원본 진단 후 현재 슬롯 변경도 보존한다. 이 몸체보다 오래 살아야 한다.</summary>
    private readonly Action<SquidLevelAssertion> _assertion;

    /// <summary>풀·타입·진단 경계를 만든다.</summary>
    /// <param name="pool">같은 풀의 슬롯 저장소다.</param>
    /// <param name="types">판본별 전체 타입 표다. 패치판 188개·CD판 171개여야 한다.</param>
    /// <param name="assertion">원문 식/파일/행을 받는 실제 진단이다.</param>
    /// <exception cref="ArgumentException">판본 타입·진단 누락이다.</exception>
    public RawSquidLevel(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, Action<SquidLevelAssertion> assertion)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(assertion);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new ArgumentException("Squid level 판본 타입·진단 누락", nameof(types));
        }

        _pool = pool;
        _types = types;
        _assertion = assertion;
    }

    /// <summary>같은 월드의 실제 상태 setter 연결인지 확인할 풀이다.</summary>
    /// <returns>이 몸체가 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>원본 hasLevel의 OR 조건 때문에 정상 타입에서는 항상 참이다. intended AND로 수정하지 않는다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>타입 범위 안이면 항상 참이다.</returns>
    public bool Has(int sid)
    {
        if (_pool.Slot(sid)[RawSidPool.TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Squid level 현재 타입 범위 오류");
        }

        return true;
    }

    /// <summary>현재 상태 바이트의 상위 3비트를 zero-extend한다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>상위 3비트 값이다.</returns>
    public uint Get(int sid) => Has(sid) ? (uint)(_pool.Slot(sid)[RawSidPool.StateOffset] >> 5) : 0u;

    /// <summary>signed 범위 진단 뒤 현재 바이트의 하위 5비트와 n의 하위 3비트를 저장한다. 물리 범위 안 예약/free도 허용하며 풀 할당/반납은 하지 않는다.</summary>
    /// <param name="sid">쓸 슬롯 번호다.</param>
    /// <param name="value">쓸 레벨 값이다. 0~7이어야 한다.</param>
    public void Set(int sid, uint value)
    {
        _ = Has(sid);
        if (unchecked((int)value) < 0 || value >= 8)
        {
            _assertion(SquidLevelAssertion.OutOfRange);
        }

        Span<byte> raw = _pool.RawSlot(sid);
        raw[RawSidPool.StateOffset] = unchecked((byte)((raw[RawSidPool.StateOffset] & 31u) | ((value & 7u) << 5)));
    }
}
