// PATCH 소유자 색 표와 섬 받침·종유석의 현재 프레임을 갱신한다.
namespace Netstorm.Core.Original;

/// <summary>원본 Player 색 변경의 표 쓰기·SID 순회·현재 타입 재조회를 복원한다.</summary>
public static class RawPlayerColor
{
    /// <summary>색 표를 먼저 쓰고 SID2부터 free만 건너뛰며 실제 프레임 소비자를 부른다.</summary>
    public static void ChangePlayerColor(RawSidPool pool, BridgeConnectState state,
        uint owner, uint color, Action<int, int, uint>? setFrame)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        if (pool.Edition != OriginalEdition.Patch1078 || owner >= state.OwnerColors.Length)
            throw new ArgumentOutOfRangeException(nameof(owner), "PATCH 소유자 색 변경 입력 범위 오류");
        int limit = pool.Capacity;
        state.OwnerColors[owner] = unchecked((int)color);
        uint index = 2;
        // 원본 iterator의 최초 증가 뒤 free 비트만 검사한다.
        while (index < limit && (pool.Slot(unchecked((ushort)index))[11] & 1) != 0) index++;
        // 콜백 뒤에는 현재 풀 상한·타입·소유자 색 표를 다시 읽는다.
        while (index >= 1 && index < pool.Capacity)
        {
            int sid = unchecked((ushort)index);
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            if (raw[10] == state.IslandType || raw[10] == state.StalagType)
            {
                if (setFrame is null) throw new InvalidOperationException("실제 소유자 색 변경의 프레임 소비자 누락");
                setFrame(sid, RawBridgeConnect.OwnerColorFrame(state, raw[34]), 0u);
            }
            index++;
            // iterator의 끝은 진입 당시 길이이며 dead·void·abstract는 제외하지 않는다.
            while (index < limit && (pool.Slot(unchecked((ushort)index))[11] & 1) != 0) index++;
        }
    }
}
