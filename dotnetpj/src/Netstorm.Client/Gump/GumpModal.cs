namespace Netstorm.Client.Gump; // 원본 모달 등록과 이전 창 전체의 최소 깊이 배치를 연결한다.

/// <summary>cpppj/src/client/RawMenuHtml.inc의 FindModalLowest/SetModal.기존16칸 dialog 표를 그대로 쓴다.</summary>
public sealed class GumpModal
{
    /// <summary>같은 현재 기본/파생 노드.</summary>
    private readonly Func<uint, GumpCreateNode?> _node;
    /// <summary>같은 실제 모달 표.</summary>
    private readonly GumpLifetimeState _state;
    /// <summary>기존 실제 모달 추가.</summary>
    private readonly GumpInput _input;
    /// <summary>기존 실제 모달 제거.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>같은 실제 자식 우선 깊이 덧셈.</summary>
    private readonly GumpTree _tree;
    /// <summary>성공한 설정 뒤의 실제 변경 통지.</summary>
    private readonly Action _changed;
    /// <summary>검색 중 같은 원문 최소 깊이 전역.</summary>
    private int _minimum;
    /// <summary>같은 최소 깊이에서 먼저 찾은 현재 토큰.</summary>
    private uint _lowest;
    /// <summary>실제 공통 저장소/등록/깊이/변경 장치를 빌린다.</summary>
    public GumpModal(Func<uint, GumpCreateNode?> node, GumpLifetimeState state, GumpInput input, GumpLifetime lifetime, GumpTree tree, Action changed) { ArgumentNullException.ThrowIfNull(node); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(changed); _node = node; _state = state; _input = input; _lifetime = lifetime; _tree = tree; _changed = changed; }
    /// <summary>현재 같은 기본 노드를 요구한다.</summary>
    private GumpCreateNode Node(uint token) => _node(token) ?? throw new InvalidOperationException("모달 현재 실제 노드 없음");
    /// <summary>visible/alive/nonvoid 자식의 깊이를 재귀 전에 비교하고 자신은 자식 뒤 비교한다.</summary>
    private void FindLowest(uint token, HashSet<uint> active)
    {
        if (active.Count >= GumpCreateState.RegistryCapacity || !active.Add(token)) throw new InvalidOperationException("모달 최소 깊이 트리 순환/한도"); var seen = new HashSet<uint>();
        // 같은 깊이면 기존 토큰을 유지하며 현재 형제/플래그를 읽는다.
        for (uint child = Node(token).Child; child != 0; child = Node(child).Next) { if (!seen.Add(child)) throw new InvalidOperationException("모달 최소 깊이 형제 순환"); var node = Node(child); if ((node.Flags & 1) != 0 && (node.Flags & 0x10008) == 0) { int depth = unchecked((int)node.Depth); if (depth < _minimum) { _minimum = depth; _lowest = child; } FindLowest(child, active); } }
        int own = unchecked((int)Node(token).Depth); if (own < _minimum) { _minimum = own; _lowest = token; } active.Remove(token);
    }
    /// <summary>이전 창의 표시 자식 전체에서 최소 깊이를 찾아 새 창을3단계 앞에 둔다.중복 추가는 깊이를 다시 바꾸지 않는다.</summary>
    public void SetModal(uint token, bool enabled) { if (!enabled) _lifetime.RemoveDialog(token); else if (_input.AddDialog(token) && _state.DialogCount > 1) { _minimum = 10000; _lowest = 0; FindLowest(_state.Dialogs[_state.DialogCount - 2], []); if (_lowest == 0) throw new InvalidOperationException("원문 모달 최소 깊이 assertion504"); _tree.AddDepth(token, unchecked(Node(_lowest).Depth - Node(token).Depth - 3)); } _changed(); }
}
