using Netstorm.Client.Gump;

// 임시 미션 HUD가 공통 루트에서 받는 도킹 사건을 실제 처리기에 전달한다.
namespace Netstorm.Client.Host;

/// <summary>전체 UberRoot가 설치되기 전 채팅과 구분선 사건을 같은 공통 트리에 연결한다.</summary>
public sealed class HostMissionHudRouting : IDisposable
{
    /// <summary>등록 시점의 실제 공통 루트다.</summary>
    private readonly uint _root;
    /// <summary>루트 사건을 처리할 실제 도킹 본체다.</summary>
    private readonly HudDock _dock;
    /// <summary>설치 전 공통 이벤트 처리기다. 도킹 기본 처리로 다시 들어갈 때 사용한다.</summary>
    private readonly Func<uint, uint, uint, uint, uint> _base;
    /// <summary>반납 후에는 공통 체인이 기존 처리기에만 전달하도록 한다.</summary>
    private bool _disposed;

    /// <summary>현재 루트의 채팅과 구분선 사건만 도킹 본체로 연결한다.</summary>
    public HostMissionHudRouting(GumpButtonTree tree, HudDock dock)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(dock);
        _root = tree.Create.Entries[0];
        if (_root == 0 || tree.Node(_root) is null) throw new InvalidOperationException("미션 HUD의 실제 루트 없음");
        _dock = dock;
        _base = tree.EventHooks.Handle ?? throw new InvalidOperationException("미션 HUD의 공통 이벤트 처리기 없음");
        var previous = _base;
        var weak = new WeakReference<HostMissionHudRouting>(this);
        tree.EventHooks.Handle = (token, code, first, second) =>
            weak.TryGetTarget(out var owner) && !owner._disposed && token == owner._root && IsDockEvent(code)
                ? owner._dock.Handle(code, first, second) : previous(token, code, first, second);
    }

    /// <summary>완전한 HUD 부모를 요구하지 않는 채팅·크기·이동 사건만 선택한다.</summary>
    private static bool IsDockEvent(uint code) => (code & 0x03ffffff) is 0x02000088 or 0x02000005 or 0x0200003a or 0x02000046;

    /// <summary>도킹 본체가 위임하는 기본 사건을 설치 전 실제 공통 처리기로 전달한다.</summary>
    public uint HandleBase(uint code, uint first, uint second) => _base(_root, code, first, second);

    /// <summary>다른 파생 처리기를 끊지 않고 자기 사건 전달만 비활성화한다.</summary>
    public void Dispose() => _disposed = true;
}
