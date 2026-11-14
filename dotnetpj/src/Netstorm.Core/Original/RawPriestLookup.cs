// 원본 사제 후처리와 조회가 같은 목록·소유자 상태를 공유하는 모듈이다.
namespace Netstorm.Core.Original;

/// <summary>공유 목록 순서에서 현재 소유자의 첫 살아 있는 사제를 조회한다.</summary>
public sealed class RawPriestLookup
{
    /// <summary>매 조회마다 현재 owner·dead 비트를 읽는 동일 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>등록·삭제가 즉시 반영되는 동일 상태다.</summary>
    private readonly PriestPostPopState _state;

    /// <summary>조회 객체보다 오래 유지할 실제 풀과 사제 상태를 연결한다.</summary>
    public RawPriestLookup(RawSidPool pool, PriestPostPopState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        _pool = pool; _state = state;
    }

    /// <summary>signed 개수 관문과 DWORD 소유자를 보존하고 dead 비트2만 제외한다.</summary>
    public uint Find(uint player)
    {
        if (unchecked((int)_state.GetPriests().Count) <= 0) return 0;
        int ownerOffset = _pool.Edition == OriginalEdition.Patch1078 ? 34 : 32;
        // 첫 일치 뒤에는 다음 항목의 범위·타입·상태를 읽지 않는다.
        foreach (uint sid in _state.GetPriests().Items())
        {
            if (sid >= _pool.Capacity)
                throw new ArgumentOutOfRangeException(nameof(sid), "소유 사제 조회 SID 범위 오류");
            ReadOnlySpan<byte> raw = _pool.Slot(unchecked((ushort)sid));
            if (raw[ownerOffset] == player && (raw[RawSidPool.StateOffset] & RawSidPool.DeadBit) == 0)
                return sid;
        }
        return 0;
    }

    /// <summary>건설·AI 어댑터가 같은 실제 월드인지 검증하는 풀 참조다.</summary>
    public RawSidPool Pool() => _pool;
}
