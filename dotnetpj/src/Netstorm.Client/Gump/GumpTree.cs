using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 생성 노드의 부모/형제 링크와 배치를 관리하는 영역이다.

/// <summary>원본 지도 표시 영역과 패치판 Push 링크 진단 전역이다.</summary>
public sealed class GumpTreeState
{
    /// <summary>지도 중심 정렬에 쓰는 정수 화면 영역.</summary>
    public ScreenRect MapView { get; set; }
    /// <summary>3/4이면 원본 패치판의 의도적 형제/부모 링크 변경을 재현한다.</summary>
    public uint PushDebugMode { get; set; }
}

/// <summary>같은 저장소의 가상 Push/Attach/Redraw·삭제 요청·월드 투영 경계. 콜백은 현재 노드/전역을 바꿀 수 있다.</summary>
public sealed class GumpTreeHooks
{
    /// <summary>토큰의 안정된 기본/파생 생성 노드를 읽는다.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>현재 노드의 redraw 가상 함수. 두 번째 인자는 분리 여부다.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>자식 재배치/이동의 가상 Push 경계.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>대상/위치/부모/모드의 가상 Attach 경계.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>동종 교체 항목에 대한 지연 삭제 요청.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>월드 부착 객체의 현재 화면 투영.</summary>
    public Func<uint, ScreenPoint>? Project { get; set; }
}

/// <summary>원본 Gump 부착·밀어내기·정렬·깊이·이동 순서를 같은 노드에서 복원한다.
/// 근거: cpppj/src/client/GumpTree.h·GumpTree.cpp, 10.78 00466750·CD00493070.
/// 콜백 예외에도 이미 쓴 링크/좌표를 유지하며 가상 수명 경계는 상위 조립에서 연결한다.</summary>
public sealed class GumpTree
{
    /// <summary>트리에서 분리된 노드의 void 비트.</summary>
    private const uint Void = 8;
    /// <summary>월드 투영에 부착된 노드 비트.</summary>
    private const uint World = 0x2000;
    /// <summary>부모 아래 동종 교체 삭제 요청을 하는 비트.</summary>
    private const uint Replace = 0x4000;
    /// <summary>화면 제한을 생략하는 비트.</summary>
    private const uint Unclamped = 0x20000;
    /// <summary>부모 깊이를 상속하지 않는 비트.</summary>
    private const uint OwnDepth = 0x200000;
    /// <summary>드래그 기준점을 이동 좌표에서 빼는 비트.</summary>
    private const uint Grab = 0x10;
    /// <summary>현재 화면/루트와 같은 생성 전역.</summary>
    private readonly GumpCreateState _state;
    /// <summary>현재 지도 영역/Push 진단 전역.</summary>
    private readonly GumpTreeState _tree;
    /// <summary>현재 노드/가상 동작 경계.</summary>
    private readonly GumpTreeHooks _hooks;

    /// <summary>같은 생성/트리 상태와 안정된 노드 저장소를 빌린다. 공통 가상 경계는 필수이고 투영은 해당 분기에서만 요구한다.</summary>
    public GumpTree(GumpCreateState state, GumpTreeState tree, GumpTreeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null || hooks.Redraw is null || hooks.Push is null || hooks.Attach is null || hooks.Kill is null)
            throw new ArgumentException("Gump 트리 저장소/처리기 필요", nameof(hooks));
        _state = state; _tree = tree; _hooks = hooks;
    }

    /// <summary>없는 토큰은 원본 유효 포인터 계약 밖이므로 거부한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 트리 토큰/저장소 없음");

    /// <summary>원본 지도 중심의 signed DWORD 덧셈 감김 뒤 2로 나누기를 유지한다.</summary>
    private static int Center(int low, int high) => unchecked(low + high) / 2;

    /// <summary>현재 경로/형제 체인의 순환과 원본1000개 한도를 검사한다.</summary>
    private static void Enter(HashSet<uint> seen, uint token)
    {
        if (seen.Count >= GumpCreateState.RegistryCapacity || !seen.Add(token)) throw new InvalidOperationException("Gump 트리 링크 순환/한도 초과");
    }

    /// <summary>현재 부모 기본 크기/화면 크기로 종속 크기·중앙·끝·지도 중심을 순서대로 적용한다. 반환 위치는 직접 기록하지 않는다.</summary>
    public (float X, float Y) Align(uint token)
    {
        GumpCreateNode node = Node(token); GumpCreateNode parent = Node(node.Parent);
        float width = parent.BaseWidth; float height = parent.BaseHeight; float x = node.X; float y = node.Y;
        if (node.Parent == _state.Collection.Root) { width = _state.ScreenWidth; height = _state.ScreenHeight; }
        uint flags = node.AlignFlags;
        if ((flags & 0xc0) != 0 && node.Parent != _state.Collection.Root) throw new InvalidOperationException("지도 중심 정렬의 루트 부모 필요");
        if ((flags & 1) != 0) { node.BaseWidth = width; node.Width = (float)((double)width * node.Scale); }
        if ((flags & 2) != 0) { node.BaseHeight = height; node.Height = (float)((double)height * node.Scale); }
        if ((flags & 4) != 0) x = (float)(((double)width - node.BaseWidth) * .5);
        if ((flags & 8) != 0) y = (float)(((double)height - node.BaseHeight) * .5);
        if ((flags & 0x10) != 0) x = (float)((double)width - node.BaseWidth);
        if ((flags & 0x20) != 0) y = (float)((double)height - node.BaseHeight);
        if ((flags & 0x40) != 0) x = (float)((double)Center(_tree.MapView.Left, _tree.MapView.Right) - (double)node.BaseWidth * .5);
        if ((flags & 0x80) != 0) y = (float)((double)Center(_tree.MapView.Top, _tree.MapView.Bottom) - (double)node.BaseHeight * .5);
        return (x, y);
    }

    /// <summary>현재 형제/부모에서 분리하고 void를 세운 뒤 redraw(true)를 부른다. 자신의 부모/이전/다음 링크는 남긴다.</summary>
    public void Push(uint token)
    {
        GumpCreateNode node = Node(token);
        if ((node.Flags & Void) != 0) throw new InvalidOperationException("이미 void인 Gump 밀어내기");
        if (node.Previous == 0 && node.Parent == 0 && node.Id != 0) throw new InvalidOperationException("부모 없는 Gump의 id0 필요");
        node.Flags |= Void;
        if (node.Previous != 0)
        {
            Node(node.Previous).Next = node.Next;
            if (_state.Collection.Edition == OriginalEdition.Patch1078 && _tree.PushDebugMode == 3) Node(node.Previous).Next = node.Previous;
        }
        else if (node.Parent != 0)
        {
            Node(node.Parent).Child = node.Next;
            if (_state.Collection.Edition == OriginalEdition.Patch1078 && _tree.PushDebugMode == 4) Node(node.Parent).Child = node.Parent;
        }
        if (node.Next != 0) Node(node.Next).Previous = node.Previous;
        _hooks.Redraw!(token, true);
    }

    /// <summary>머리에 부착한 뒤 위치/크기 제한·정렬·redraw·현재 자식 재배치를 수행한다. 부모0은 기존 부모, 모드1은 제한 생략, 모드2는 정렬이다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        GumpCreateNode node = Node(token);
        if (_state.Collection.Edition != OriginalEdition.Patch1078 && (node.Flags & Void) == 0) throw new InvalidOperationException("CD Gump 부착의 void 필요");
        uint destination = parent != 0 ? parent : node.Parent; Node(destination);
        node.Flags &= ~Void; if (parent != 0) node.Parent = parent;
        if (node.Parent != _state.Collection.Root && (node.Flags & OwnDepth) == 0)
            node.Depth = unchecked(Node(node.Parent).Depth + (node.SecondaryDepth != 0 ? node.SecondaryDepth : 0xfffffffdU));
        if ((node.Flags & Replace) != 0)
        {
            var seen = new HashSet<uint>();
            // 삭제 요청 전에 next를 보관하며 dead 항목도 같은 교체 요청에 포함한다.
            for (uint current = Node(node.Parent).Child; current != 0;)
            {
                Enter(seen, current); uint next = Node(current).Next;
                if ((Node(current).Flags & Replace) != 0) _hooks.Kill!(current);
                current = next;
            }
        }
        node.Previous = 0; node.Next = Node(node.Parent).Child; Node(node.Parent).Child = token;
        if (node.Next != 0) Node(node.Next).Previous = token;
        if ((mode & 1) == 0 && (node.Flags & Unclamped) == 0)
        {
            ScreenPoint origin = default;
            if ((node.Flags & World) != 0)
            {
                if (_hooks.Project is null) throw new InvalidOperationException("월드 Gump 부착의 투영 필요");
                origin = _hooks.Project(node.Object); x = (float)((double)origin.X + x); y = (float)((double)origin.Y + y);
            }
            if (node.Parent == _state.Collection.Root)
            {
                bool patch = _state.Collection.Edition == OriginalEdition.Patch1078;
                double width = patch ? (double)_state.ScreenWidth : (double)(float)_state.ScreenWidth;
                double height = patch ? (double)_state.ScreenHeight : (double)(float)_state.ScreenHeight;
                if (width < (double)x + node.Width) x = (float)(width - node.Width);
                if (height < (double)y + node.Height) y = (float)(height - node.Height);
                if (patch ? x < 0F : x <= 0F) x = 0F;
                if (patch ? y < 0F : y <= 0F) y = 0F;
            }
            if ((node.Flags & World) != 0) { x = (float)((double)x - origin.X); y = (float)((double)y - origin.Y); }
        }
        node.X = x; node.Y = y;
        if ((mode & 2) != 0) { (float X, float Y) position = Align(token); node.X = position.X; node.Y = position.Y; }
        _hooks.Redraw!(token, false); var children = new HashSet<uint>();
        // redraw 뒤 현재 child를 읽고 각 자식의 next는 가상 Push 전에 보관한다.
        for (uint child = node.Child; child != 0;)
        {
            Enter(children, child); GumpCreateNode below = Node(child); uint next = below.Next;
            if (below.AlignFlags != 0 || below.SecondaryDepth != 0)
            {
                _hooks.Push!(child);
                if (below.SecondaryDepth != 0) below.Depth = unchecked(node.Depth + below.SecondaryDepth);
                (float X, float Y) position = Align(child); _hooks.Attach!(child, position.X, position.Y, 0, 0);
            }
            child = next;
        }
    }

    /// <summary>자식부터 깊이를 갱신하고 재귀 뒤 현재 next를 읽는다. visible/dead/void 필터는 추가하지 않는다.</summary>
    private void Depth(uint token, uint value, bool add, HashSet<uint> active)
    {
        Enter(active, token); GumpCreateNode node = Node(token); var siblings = new HashSet<uint>();
        // 모든 자식의 깊이를 자신의 깊이보다 먼저 쓴다.
        for (uint child = node.Child; child != 0; child = Node(child).Next)
        {
            Enter(siblings, child); Depth(child, value, add, active);
        }
        node.Depth = add ? unchecked(node.Depth + value) : value; active.Remove(token);
    }

    /// <summary>자식부터 모든 깊이 DWORD에 같은 값을 더한다.</summary>
    public void AddDepth(uint token, uint delta) => Depth(token, delta, true, []);

    /// <summary>자식부터 모든 깊이를 같은 DWORD 값으로 지정한다.</summary>
    public void SetDepth(uint token, uint depth) => Depth(token, depth, false, []);

    /// <summary>드래그 기준점 보정→현재 void 판정→가상 Push→가상 Attach 순서를 유지한다.</summary>
    public void Move(uint token, float x, float y, uint parent)
    {
        GumpCreateNode node = Node(token);
        if ((node.Flags & Grab) != 0) { x = (float)((double)x - BitConverter.UInt32BitsToSingle(node.Positioning[0])); y = (float)((double)y - BitConverter.UInt32BitsToSingle(node.Positioning[1])); }
        if ((node.Flags & Void) == 0) _hooks.Push!(token);
        _hooks.Attach!(token, x, y, parent, 0);
    }

    /// <summary>먼저 Push하고 그 뒤 현재 기준 노드/부모/크기를 읽어 위치와 anchor8/4의 폭/높이를 더한다.</summary>
    public void MoveRelative(uint token, uint reference, float x, float y, byte anchor, uint parent)
    {
        GumpCreateNode node = Node(token);
        if ((node.Flags & Void) == 0) _hooks.Push!(token);
        GumpCreateNode basis = Node(reference);
        if (reference != node.Parent) { x = (float)((double)x + basis.X); y = (float)((double)y + basis.Y); }
        if ((anchor & 8) != 0) x = (float)((double)x + basis.Width);
        if ((anchor & 4) != 0) y = (float)((double)y + basis.Height);
        _hooks.Attach!(token, x, y, parent, 0);
    }
}
