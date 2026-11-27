namespace Netstorm.Client.Gump; // 현재 예산 창의 실제 등록과 가상 부착을 공유한다.

/// <summary>같은 등록표의 노드와 실제 Push·Attach 장치다.</summary>
public sealed class StormPowerRefreshHooks
{
    /// <summary>같은 토큰의 실제 노드를 조회한다.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; init; }
    /// <summary>현재 노드를 실제 가상 Push로 분리한다.</summary>
    public Action<uint>? Push { get; init; }
    /// <summary>현재 좌표와 원본 인자로 실제 가상 Attach를 실행한다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; init; }
}

/// <summary>원본 00464020과 0043dad0의 예산 창 조회·재부착을 수행한다.</summary>
public static class StormPowerRefresh
{
    /// <summary>도달한 현재 토큰의 실제 노드만 요구한다.</summary>
    private static GumpCreateNode Current(Func<uint, GumpCreateNode?>? node, uint token)
        => (node ?? throw new InvalidOperationException("SP 표시의 실제 노드 조회 미연결"))(token)
           ?? throw new ArgumentOutOfRangeException(nameof(token), "SP 표시의 현재 노드 없음");

    /// <summary>등록 1~999에서 같은 serial의 첫 생존 노드를 찾는다.</summary>
    public static uint Find(ReadOnlySpan<uint> entries, uint serial, Func<uint, GumpCreateNode?>? node)
    {
        if (entries.Length < GumpCreateState.RegistryCapacity) throw new ArgumentException("SP 표시의 실제 등록표 크기 부족", nameof(entries));
        // 루트 0은 제외하고 실제 고정 표를 현재 순서대로 조회한다.
        for (int index = 1; index < GumpCreateState.RegistryCapacity; index++)
        {
            uint token = entries[index];
            if (token == 0) continue;
            var current = Current(node, token);
            if (current.Serial == serial && (current.Flags & 0x10000) == 0) return token;
        }
        return 0;
    }

    /// <summary>현재 예산 창을 분리·재부착하고 정상 복귀 뒤 원래 깊이를 복원한다.</summary>
    public static void Refresh(ReadOnlySpan<uint> entries, uint serial, StormPowerRefreshHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint token = Find(entries, serial, hooks.Node);
        if (token == 0) return;
        var current = Current(hooks.Node, token);
        uint depth = current.Depth;
        (hooks.Push ?? throw new InvalidOperationException("SP 표시의 실제 Push 미연결"))(token);
        (hooks.Attach ?? throw new InvalidOperationException("SP 표시의 실제 Attach 미연결"))(token, current.X, current.Y, 0, 0);
        current.Depth = depth;
    }
}
