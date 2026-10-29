using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 Gump 트리의 저장·좌표·표시 수집 영역이다.

/// <summary>원본 Gump의 수집/Draw 공용 필드. 토큰은 원본 포인터 대신 쓰며 같은 노드의 캐시를 직접 갱신한다.
/// 근거: cpppj/src/client/GumpCollection.h. 파생 생성 노드도 이 저장소를 그대로 상속한다.</summary>
public class GumpCollectionNode
{
    /// <summary>+4 표시·삭제·월드 부착 플래그.</summary>
    public uint Flags { get; set; }
    /// <summary>+0xc 다음 형제 토큰.</summary>
    public uint Next { get; set; }
    /// <summary>+0x14 부모 토큰.</summary>
    public uint Parent { get; set; }
    /// <summary>+0x18 첫 자식 토큰.</summary>
    public uint Child { get; set; }
    /// <summary>+0x24 월드 부착 대상의 식별자.</summary>
    public uint Object { get; set; }
    /// <summary>+0x30 표시 깊이 DWORD. 제출할 때 하위 WORD만 쓴다.</summary>
    public uint Depth { get; set; }
    /// <summary>+0x44 배율 적용 가로 크기.</summary>
    public float Width { get; set; }
    /// <summary>+0x48 배율 적용 세로 크기.</summary>
    public float Height { get; set; }
    /// <summary>+0x4c 부모에 상대적인 가로 위치.</summary>
    public float X { get; set; }
    /// <summary>+0x50 부모에 상대적인 세로 위치.</summary>
    public float Y { get; set; }
    /// <summary>+0x6c/+0x70 일반 위치 조회의 캐시. 월드 부착 자신은 갱신하지 않는다.</summary>
    public ScreenPoint Cached { get; set; }
}

/// <summary>수집 시점에 읽는 현재 루트·임시 머리·판본 전역이다.</summary>
public sealed class GumpCollectionState
{
    /// <summary>현재 실제 루트 토큰.</summary>
    public uint Root { get; set; }
    /// <summary>0이 아니면 root.child 대신 쓸 수집 머리.</summary>
    public uint OverrideHead { get; set; }
    /// <summary>월드 부착 부모의 int→float 저장 차이를 구별하는 판본.</summary>
    public OriginalEdition Edition { get; set; } = OriginalEdition.Patch1078;
}

/// <summary>같은 노드 저장소의 조회와 월드 투영만 외부에서 공급하는 경계다.</summary>
public sealed class GumpCollectionHooks
{
    /// <summary>토큰의 안정된 현재 노드. 없는 토큰은 null을 반환한다.</summary>
    public Func<uint, GumpCollectionNode?>? Node { get; set; }
    /// <summary>월드 객체 식별자의 현재 화면 좌표. 해당 분기에서만 필요하다.</summary>
    public Func<uint, ScreenPoint>? Project { get; set; }
}

/// <summary>Gump 위치·경계·로컬 좌표·표시 목록을 원본 float 저장/부작용 순서로 계산한다.
/// 근거: cpppj/src/client/GumpCollection.cpp. Draw 분배 연결은 해당 모듈 이식 때 잇는다.</summary>
public sealed class GumpCollection
{
    /// <summary>현재 경로/형제 체인의 원본 최대 길이.</summary>
    private const int MaxGumps = 1000;
    /// <summary>현재 루트/override/판본을 보유한 장기 상태.</summary>
    private readonly GumpCollectionState _state;
    /// <summary>공유 노드와 투영의 조회 경계.</summary>
    private readonly GumpCollectionHooks _hooks;

    /// <summary>현재 상태/안정된 노드 저장소를 빌린다. 노드 조회 경계는 필수다.</summary>
    public GumpCollection(GumpCollectionState state, GumpCollectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Node is null) throw new ArgumentException("Gump 저장소 조회 필요", nameof(hooks));
        _state = state; _hooks = hooks;
    }

    /// <summary>없는 토큰은 유효 원본 포인터 계약 밖이므로 명시 오류로 거부한다.</summary>
    private GumpCollectionNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("Gump 토큰/저장소 없음");

    /// <summary>CRT signed64 절삭의 하위 DWORD. 비유한/범위 밖은 원본 불확정 정수의 하위0이다.</summary>
    internal static int Ftol(double number) => !double.IsFinite(number) || number < -9223372036854775808.0 || number >= 9223372036854775808.0 ? 0 : unchecked((int)(long)number);

    /// <summary>현재 경로와 형제 체인의 순환/길이 한도를 검사한다. 서로 다른 경로의 중복은 허용한다.</summary>
    private static void Enter(HashSet<uint> seen, uint token)
    {
        if (seen.Count >= MaxGumps || !seen.Add(token)) throw new InvalidOperationException("Gump 링크 순환 또는 한도 초과");
    }

    /// <summary>루트 직계 월드 부착의 투영+로컬 위치를 DWORD 감김으로 계산한다.</summary>
    private ScreenPoint Anchor(GumpCollectionNode node)
    {
        if (node.Parent != _state.Root || _hooks.Project is null) throw new InvalidOperationException("월드 부착 Gump의 루트/투영 필요");
        ScreenPoint point = _hooks.Project(node.Object);
        return new(unchecked(point.X + Ftol(node.X)), unchecked(point.Y + Ftol(node.Y)));
    }

    /// <summary>부모마다 float32로 저장하고 최종 CRT 절삭 후 일반 노드의 위치 캐시를 갱신한다.</summary>
    public ScreenPoint Position(uint token)
    {
        GumpCollectionNode node = Node(token);
        if ((node.Flags & 0x2000) != 0) return Anchor(node);
        float x = node.X; float y = node.Y;
        var seen = new HashSet<uint>(); Enter(seen, token);
        // 부모의 현재 위치를 더하며 부모 자신의 캐시는 사용하지 않는다.
        for (uint parent = node.Parent; parent != 0; parent = Node(parent).Parent)
        {
            Enter(seen, parent); GumpCollectionNode above = Node(parent);
            if ((above.Flags & 0x2000) != 0)
            {
                ScreenPoint point = Anchor(above);
                double px = _state.Edition == OriginalEdition.Patch1078 ? (double)point.X : (double)(float)point.X;
                double py = _state.Edition == OriginalEdition.Patch1078 ? (double)point.Y : (double)(float)point.Y;
                x = (float)(px + (double)x); y = (float)(py + (double)y);
            }
            else
            {
                x = (float)((double)above.X + (double)x); y = (float)((double)above.Y + (double)y);
            }
        }
        node.Cached = new(Ftol(x), Ftol(y));
        return node.Cached;
    }

    /// <summary>정수 위치에 크기를 더해 절삭한다. 합산 직후 float으로 다시 반올림하지 않는다.</summary>
    public ScreenRect Bounds(uint token)
    {
        ScreenPoint point = Position(token); GumpCollectionNode node = Node(token);
        return new(point.X, point.Y, Ftol((double)point.X + node.Width), Ftol((double)point.Y + node.Height));
    }

    /// <summary>사건 화면 좌표를 각 뺄셈 뒤 float32로 저장해 로컬 좌표로 바꾼다.
    /// 루트 위치는 생략하고 월드 부모 뒤 일반 부모는 건너뛴다. 위치 캐시도 갱신하지 않는다(004650a0).</summary>
    public (float X, float Y) ScreenToLocal(uint token, ScreenPoint point)
    {
        float x = point.X; float y = point.Y; bool anchored = false;
        var seen = new HashSet<uint>();
        // 현재 부모/위치를 읽으며 최상위 자신의 위치는 빼지 않는다.
        while (true)
        {
            Enter(seen, token); GumpCollectionNode node = Node(token);
            if (node.Parent == 0) return (x, y);
            GumpCollectionNode parent = Node(node.Parent);
            if ((parent.Flags & 0x2000) != 0)
            {
                ScreenPoint anchor = Anchor(parent);
                x = (float)((double)x - anchor.X); y = (float)((double)y - anchor.Y); anchored = true;
            }
            else if (!anchored)
            {
                x = (float)((double)x - node.X); y = (float)((double)y - node.Y);
            }
            token = node.Parent;
        }
    }

    /// <summary>좌표나 캐시를 갱신하지 않고 현재 플래그만 읽는다.</summary>
    public uint Flags(uint token) => Node(token).Flags;

    /// <summary>역전 사각형은 제외하고 맞닿은 가장자리는 교차로 세지 않는다. 내부의 점/선 영역은 허용한다.</summary>
    private static bool Intersects(ScreenRect a, ScreenRect b) => a.Left <= a.Right && a.Top <= a.Bottom && b.Left <= b.Right && b.Top <= b.Bottom &&
        a.Left < b.Right && b.Left < a.Right && a.Top < b.Bottom && b.Top < a.Bottom;

    /// <summary>지정 노드는 visible/dead를 직접 검사하지 않고 깊이 우선 수집한다. limit은 이 하위 트리의 제한이다.</summary>
    public int CollectNode(uint token, ScreenRect view, Span<RenderCollectionEntry> output, int limit) => CollectCurrent(token, view, output, limit, []);

    /// <summary>현재 재귀 경로를 유지하고 남은 출력/제한을 자식으로 전달하는 공통 수집 본체다.</summary>
    private int CollectCurrent(uint current, ScreenRect view, Span<RenderCollectionEntry> remaining, int available, HashSet<uint> active)
    {
        if (available < 1) return 0;
        Enter(active, current); bool visible = Intersects(Bounds(current), view); GumpCollectionNode node = Node(current);
        if (!visible && (node.Flags & 0x20) == 0) { active.Remove(current); return 0; }
        int count = 0;
        if (visible)
        {
            if (remaining.IsEmpty) throw new ArgumentException("Gump 표시 출력 용량 부족", nameof(remaining));
            remaining[0] = new(node.X, node.Y, current, unchecked((short)node.Depth), 2, 0);
            remaining = remaining[1..]; count = 1; available--;
        }
        var siblings = new HashSet<uint>();
        // 자식은 visible만 검사한다. dead 자식을 제외하는 필터는 원본에 없다.
        for (uint child = node.Child; child != 0; child = Node(child).Next)
        {
            Enter(siblings, child);
            if ((Node(child).Flags & 1) == 0) continue;
            int added = CollectCurrent(child, view, remaining, available, active);
            remaining = remaining[added..]; available -= added; count += added;
        }
        active.Remove(current); return count;
    }

    /// <summary>root.child/override 순서로 수집한다. limit은 최상위마다 같으므로 전체 수는 limit을 넘을 수 있다.</summary>
    public int Collect(ScreenRect view, Span<RenderCollectionEntry> output, int limit)
    {
        uint head = Node(_state.Root).Child;
        if (_state.OverrideHead != 0) head = _state.OverrideHead;
        int count = 0; var siblings = new HashSet<uint>();
        // 최상위 next는 재귀 전에 저장하지만 자식 next는 재귀 뒤 현재 값을 읽는다.
        while (head != 0)
        {
            Enter(siblings, head); uint current = head; GumpCollectionNode node = Node(current); head = node.Next;
            if ((node.Flags & 1) != 0 && (node.Flags & 0x10000) == 0)
            {
                int added = CollectNode(current, view, output[count..], limit);
                if (added > int.MaxValue - count) throw new ArgumentException("Gump 표시 개수 넘침");
                count += added;
            }
        }
        return count;
    }
}
