using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 등록/수명 저장소의 프레임 통지·idle·재그리기 요청 영역이다.

/// <summary>현재 노드와 프레임 가상 함수 경계. 콜백은 같은 표/링크/플래그를 바꿀 수 있으며 해제는 지연 수명 단계에서 수행한다.</summary>
public sealed class GumpFrameHooks
{
    /// <summary>호출 동안 안정된 현재 생성 노드.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>가상 +0x30 idle. 추가 인자는 없다.</summary>
    public Action<uint>? Idle { get; set; }
    /// <summary>가상 +8 재그리기 요청. 이 프레임 API의 두 번째 인자는 항상 false다.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>가상 +0x2c의 대상/사건/두 인자 통지.</summary>
    public Action<uint, uint, uint, uint>? Broadcast { get; set; }
}

/// <summary>원본 등록 표 통지·idle 실행과 현재 자식의 재그리기 요청을 같은 생성/수명 저장소에서 처리한다.
/// 근거: cpppj/src/client/GumpFrame.h·GumpFrame.cpp, 10.78 00464100/00464170/00464c20/004660f0.
/// OS 큐/실제 파생 가상 함수/Client 자동 프레임 조립은 상위의 연결 대상이다.</summary>
public sealed class GumpFrame
{
    /// <summary>현재 등록 표/개수/루트/판본.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 실제 idle 표.</summary>
    private readonly GumpLifetimeState _lifetime;
    /// <summary>안정된 노드와 가상 통지/idle/요청 경계.</summary>
    private readonly GumpFrameHooks _hooks;

    /// <summary>같은 생성/수명 상태를 빌리고 노드 조회를 요구한다. 각 가상 경계는 실제 도달할 때만 필요하다.</summary>
    public GumpFrame(GumpCreateState create, GumpLifetimeState lifetime, GumpFrameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("Gump 프레임 저장소 필요", nameof(hooks));
        _create = create; _lifetime = lifetime; _hooks = hooks;
    }

    /// <summary>실제 필드 읽기/가상 호출에 도달한 유효 토큰만 검사한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 프레임 토큰/저장소 없음");

    /// <summary>현재 경로/형제 체인의 중복과 원본1000개 한도를 검사한다.</summary>
    private static void Enter(HashSet<uint> seen, uint token)
    {
        if (seen.Count >= GumpCreateState.RegistryCapacity || !seen.Add(token)) throw new InvalidOperationException("Gump 프레임 링크 순환/한도 초과");
    }

    /// <summary>등록 표1~999를 방문해 visible/notify/비 dead 노드에 통지한다. 필터에 실패한 비영 칸도 현재 signed count와 비교할 개수에 포함한다.</summary>
    public void Broadcast(uint eventCode, uint first, uint second)
    {
        int encountered = 0;
        // 콜백 뒤 바뀐 현재 signed count를 각 슬롯 진입마다 다시 읽는다.
        for (int i = 1; i < _create.Entries.Length; i++)
        {
            if (encountered >= unchecked((int)_create.Count)) return;
            uint token = _create.Entries[i]; if (token == 0) continue;
            uint flags = Node(token).Flags; encountered++;
            if ((flags & (1 | 0x100)) == (1 | 0x100) && (flags & 0x10000) == 0)
                (_hooks.Broadcast ?? throw new ArgumentException("Gump 가상 통지 경계 필요"))(token, eventCode, first, second);
        }
    }

    /// <summary>시작 판본의500/80칸 현재 idle 토큰을 표 순서로 호출한다. visible/dead/void/idle 비트나 부모는 재검사하지 않는다.</summary>
    public void Idle()
    {
        int limit = _create.Collection.Edition == OriginalEdition.Patch1078 ? _lifetime.Idle.Length : 80;
        // 이미 지나간 칸을 콜백이 채워도 되돌아가지 않고 각 현재 슬롯을 한 번만 읽는다.
        for (int i = 0; i < limit; i++)
        {
            uint token = _lifetime.Idle[i]; if (token == 0) continue;
            Node(token); (_hooks.Idle ?? throw new ArgumentException("Gump 가상 idle 경계 필요"))(token);
        }
    }

    /// <summary>독립 재귀 경로에서 자신과 현재 자식의 재그리기 요청을 처리한다.</summary>
    public void RedrawNode(uint token) => Redraw(token, []);

    /// <summary>요청 콜백 뒤 현재 자식 전파 비트를 읽으며 재귀 뒤 현재 next를 방문한다.</summary>
    private void Redraw(uint token, HashSet<uint> active)
    {
        GumpCreateNode node = Node(token); Enter(active, token);
        if ((node.Flags & 0x42000) == 0) (_hooks.Redraw ?? throw new ArgumentException("Gump 가상 재그리기 경계 필요"))(token, false);
        if ((node.Flags & 0x20) != 0)
        {
            var siblings = new HashSet<uint>();
            // visible 자식만 전달하며 dead/void는 추가 제외 조건으로 쓰지 않는다.
            for (uint child = node.Child; child != 0; child = Node(child).Next)
            {
                Enter(siblings, child); if ((Node(child).Flags & 1) != 0) Redraw(child, active);
            }
        }
        active.Remove(token);
    }

    /// <summary>실제 등록 루트의 자식부터 visible/비 dead만 처리한다. 다음 최상위는 콜백 전에 저장하고 override/루트 자신의 visible은 쓰지 않는다.</summary>
    public void RedrawRoots()
    {
        var siblings = new HashSet<uint>();
        // 하위 next 재조회와 달리 각 최상위 next는 요청 전에 보관한다.
        for (uint token = Node(_create.Entries[0]).Child; token != 0;)
        {
            Enter(siblings, token); GumpCreateNode node = Node(token); uint flags = node.Flags; uint next = node.Next;
            if ((flags & 1) != 0 && (flags & 0x10000) == 0) RedrawNode(token);
            token = next;
        }
    }
}
