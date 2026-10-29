using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 같은 생성/수명 노드의 커서 선택과 재귀 사건 전달 영역이다.

/// <summary>원본 전체 트리 선택이 사용하는 현재 위치·후보 마스크·최소 signed 깊이·선택 전역이다.</summary>
public sealed class GumpEventState
{
    /// <summary>현재 커서의 정수 화면 위치.</summary>
    public ScreenPoint Point { get; set; }
    /// <summary>후보만 제한하고 자식 재귀는 막지 않는 플래그 마스크.</summary>
    public uint Mask { get; set; }
    /// <summary>현재 최소 깊이 DWORD. 비교할 때 signed로 읽는다.</summary>
    public uint BestDepth { get; set; }
    /// <summary>현재 선택한 노드 토큰.</summary>
    public uint Selected { get; set; }
}

/// <summary>가상 사건 필터/처리기와 같은 안정된 노드 저장소의 경계다. 콜백은 현재 링크/전역을 바꿀 수 있다.</summary>
public sealed class GumpEventHooks
{
    /// <summary>현재 토큰의 안정된 생성 노드.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>가상 +0x34 필터. 없으면 실제 기본 필터를 사용한다.</summary>
    public Func<uint, uint, uint, uint, bool>? Filter { get; set; }
    /// <summary>가상 +0x28 처리기. 전체 DWORD 반환은 API마다 성공 토큰/원값으로 구별한다.</summary>
    public Func<uint, uint, uint, uint, uint>? Handle { get; set; }
}

/// <summary>원본 커서 대상 선택과 자식 우선/부모 전파 사건을 같은 생성/위치/수명 저장소에서 처리한다.
/// 근거: cpppj/src/client/GumpEvents.h·GumpEvents.cpp. 표시 qsort와 별개로 같은 깊이는 먼저 방문한 노드가 우선이다.</summary>
public sealed class GumpEvents
{
    /// <summary>현재 실제 루트 등록 표.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 dialog를 보유한 수명 전역.</summary>
    private readonly GumpLifetimeState _lifetime;
    /// <summary>현재 선택 좌표/마스크/깊이/토큰 전역.</summary>
    private readonly GumpEventState _state;
    /// <summary>같은 노드의 현재 위치/경계/캐시를 갱신하는 수집기.</summary>
    private readonly GumpCollection _collection;
    /// <summary>현재 노드와 가상 사건 경계.</summary>
    private readonly GumpEventHooks _hooks;

    /// <summary>같은 생성/수명/선택/위치 저장소를 빌린다. 노드 조회만 필수이며 전달에서만 처리기를 요구한다.</summary>
    public GumpEvents(GumpCreateState create, GumpLifetimeState lifetime, GumpEventState state, GumpCollection collection, GumpEventHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("Gump 이벤트 저장소 필요", nameof(hooks));
        _create = create; _lifetime = lifetime; _state = state; _collection = collection; _hooks = hooks;
    }

    /// <summary>없는 토큰은 원본 유효 포인터 계약 밖이므로 거부한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 이벤트 토큰 없음");

    /// <summary>왼쪽/위 포함, 오른쪽/아래 제외의 원본 커서 경계다.</summary>
    private static bool Contains(ScreenRect rect, ScreenPoint point) => rect.Left <= point.X && point.X < rect.Right && rect.Top <= point.Y && point.Y < rect.Bottom;

    /// <summary>현재 재귀 경로/형제 체인에서 중복 토큰만 거부한다. 별도 경로의 정상 재방문은 허용한다.</summary>
    private static void Enter(HashSet<uint> active, uint token)
    {
        if (!active.Add(token)) throw new InvalidOperationException("Gump 이벤트 링크 순환");
    }

    /// <summary>현재 dialog는 모든 사건을 받는다. 그 외에는 도움말 경계·키보드·마우스 플래그를 원본 우선순위로 판정한다.</summary>
    public bool BaseFilter(uint token, uint eventCode, uint first, uint second)
    {
        if (token == _lifetime.CurrentDialog) return true;
        uint flags = Node(token).Flags; uint kind = eventCode & 0x03ffffff;
        if ((flags & 0x200) != 0 && (kind == 0x10000 || kind == 0x20000))
            return Contains(_collection.Bounds(token), new(unchecked((int)first), unchecked((int)second)));
        return ((flags & 0x400) != 0 && (eventCode & 0x02000000) == 0 && kind != 0x10000 && kind != 0x20000) ||
            ((flags & 0x800) != 0 && (eventCode & 0x02000000) != 0);
    }

    /// <summary>루트 직계 자식만 경계/visible/dead/signed 깊이로 선택한다. void는 제외하지 않고 동일 깊이는 기존 선택을 유지한다.</summary>
    public uint PickTopLevel(ScreenPoint point)
    {
        uint selected = 0; int depth = 1000; var siblings = new HashSet<uint>();
        // 경계 조회 뒤 현재 플래그/깊이를 읽고 다음 형제도 현재 링크로 방문한다.
        for (uint child = Node(_create.Entries[0]).Child; child != 0; child = Node(child).Next)
        {
            Enter(siblings, child); ScreenRect bounds = _collection.Bounds(child); GumpCreateNode node = Node(child);
            if (Contains(bounds, point) && unchecked((int)node.Depth) < depth && (node.Flags & 1) != 0 && (node.Flags & 0x10000) == 0)
            {
                depth = unchecked((int)node.Depth); selected = child;
            }
        }
        return selected;
    }

    /// <summary>현재 선택 전역을 초기화한 뒤 전체 자식을 탐색한다. 마스크는 후보만 제한하며 재귀를 막지 않는다.</summary>
    public uint Pick(ScreenPoint point, uint mask)
    {
        _state.Point = point; _state.Mask = mask; _state.BestDepth = 10000; _state.Selected = 0;
        PickChildren(_create.Entries[0], []); return _state.Selected;
    }

    /// <summary>부모 경계 밖 자식 통과와 visible/void/dead를 판정하고 재귀 뒤의 현재 next를 읽는다.</summary>
    private void PickChildren(uint token, HashSet<uint> active)
    {
        Enter(active, token); var siblings = new HashSet<uint>();
        // 위치 계산이 바꾼 현재 플래그/선택 전역은 같은 방문에서 바로 사용한다.
        for (uint child = Node(token).Child; child != 0; child = Node(child).Next)
        {
            Enter(siblings, child); ScreenRect bounds = _collection.Bounds(child); bool inside = Contains(bounds, _state.Point); GumpCreateNode node = Node(child);
            if (((node.Flags & 0x20) != 0 || inside) && (node.Flags & 1) != 0 && (node.Flags & (0x10000 | 8)) == 0)
            {
                if (inside && unchecked((int)node.Depth) < unchecked((int)_state.BestDepth) && (_state.Mask == 0 || (node.Flags & _state.Mask) != 0))
                {
                    _state.BestDepth = node.Depth; _state.Selected = child;
                }
                PickChildren(child, active);
            }
        }
        active.Remove(token);
    }

    /// <summary>직접 자식부터 재귀 전달하고 실패하면 자신의 가상 필터/처리기를 부른다. 비영 처리기 반환 대신 성공 노드 토큰을 반환한다.</summary>
    public uint Dispatch(uint token, uint eventCode, uint first, uint second, bool includeHidden = false)
    {
        if (_hooks.Handle is null) throw new ArgumentException("Gump 이벤트 처리기 필요");
        return Deliver(token, eventCode, first, second, includeHidden, []);
    }

    /// <summary>자식 성공을 먼저 반환하고 실패할 때만 현재 노드의 필터/처리기를 실행하는 재귀 본체다.</summary>
    private uint Deliver(uint token, uint eventCode, uint first, uint second, bool includeHidden, HashSet<uint> active)
    {
        Enter(active, token); var siblings = new HashSet<uint>();
        // 전달은 visible만 제한하며 dead/void 필터를 추가하지 않는다. 강제 모드는 visible도 무시한다.
        for (uint child = Node(token).Child; child != 0; child = Node(child).Next)
        {
            Enter(siblings, child);
            if ((Node(child).Flags & 1) != 0 || includeHidden)
            {
                uint result = Deliver(child, eventCode, first, second, includeHidden, active);
                if (result != 0) { active.Remove(token); return result; }
            }
        }
        bool accepts = _hooks.Filter is not null ? _hooks.Filter(token, eventCode, first, second) : BaseFilter(token, eventCode, first, second);
        uint response = accepts && _hooks.Handle!(token, eventCode, first, second) != 0 ? token : 0;
        active.Remove(token); return response;
    }

    /// <summary>자신을 제외한 현재 부모 체인으로 기본 필터 없이 전달한다. 첫 비영 처리기 반환의 전체 DWORD를 보존한다.</summary>
    public uint Bubble(uint token, uint eventCode, uint first, uint second)
    {
        if (_hooks.Handle is null) throw new ArgumentException("Gump 이벤트 처리기 필요");
        var seen = new HashSet<uint>();
        // 실패한 부모의 처리기가 바꾼 부모 링크를 다음 목적지로 사용한다.
        for (uint parent = Node(token).Parent; parent != 0; parent = Node(parent).Parent)
        {
            Enter(seen, parent); uint result = _hooks.Handle(parent, eventCode, first, second); if (result != 0) return result;
        }
        return 0;
    }
}
