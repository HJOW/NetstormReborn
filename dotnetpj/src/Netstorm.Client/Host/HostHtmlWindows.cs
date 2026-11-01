using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host; // 실제 메뉴 트리와 입력 큐를 공유하는 HTML 창 소유 영역이다.

/// <summary>HTML 부모·돌 버튼의 실제 공통 등록, 입력, 모달 및 자식 우선 수명을 소유한다.
/// 본문과 전체 배치·파서는 별도 장치이며 설치 전 해당 경로에서 명시 오류를 낸다.
/// 근거: cpppj/src/client/RawMenuHtml.inc의 BindHtml·AllocateHtml·CloseHtml.</summary>
public sealed class HostHtmlWindows
{
    /// <summary>메인 메뉴와 같은 공통 트리.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>할당되었으나 아직 반환되지 않은 HTML 부모와 직접 할당 자식.</summary>
    private readonly HashSet<uint> _owned = [];
    /// <summary>완전 생성된 HTML 부모. 부분 생성 시 소멸자에서 창 개수를 감소시키지 않는다.</summary>
    private readonly HashSet<uint> _constructed = [];
    /// <summary>실제 HTML 파생 처리기가 쓰는 현재 선택 장치.</summary>
    private readonly HtmlWindowGumpHooks _hooks;
    /// <summary>클라이언트 수명의 현재 HTML 창 전역.</summary>
    public HtmlWindowGumpState State { get; } = new();
    /// <summary>같은 실제 등록/입력/소멸 장치를 쓰는 HTML 부모 처리기.</summary>
    public HtmlWindowGump Window { get; }

    /// <summary>메뉴 트리와 실제 큐·벽 시각·링크 실행 경계를 연결한다.</summary>
    /// <param name="menu">같은 공통 트리와 실제 자산을 소유하는 메인 메뉴.</param>
    /// <param name="queue">명령 사건을 받는 실제 입력 큐.</param>
    /// <param name="wallSeconds">현재 단조 벽 시각.</param>
    /// <param name="link">죽음 표시 직후 링크 주소를 소비하는 현재 처리기.</param>
    public HostHtmlWindows(HostMainMenu menu, InputQueue queue, Func<double> wallSeconds, Action<uint> link)
    {
        ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(queue);
        ArgumentNullException.ThrowIfNull(wallSeconds); ArgumentNullException.ThrowIfNull(link);
        _tree = menu.Tree;
        _hooks = new HtmlWindowGumpHooks
        {
            Node = token => _owned.Contains(token) ? _tree.Node(token) as HtmlWindowGumpNode : null,
            Base = _tree.Node, Button = token => _tree.Node(token) as ButtonGumpNode,
            ConstructDecoration = token => menu.Decoration.Construct(token, 0, 0),
            DestructBase = _tree.Lifetime.DestructBase, Push = _tree.Tree.Push, Kill = _tree.Lifetime.Kill,
            Allocate = AllocateChild,
            ConstructButton = (token, label) => _tree.Stones.Construct(token, 0, 0, Encoding.Latin1.GetBytes(label + '\0')),
            Attach = (token, x, y, parent, mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode),
            Reference = _tree.Reference, WallSeconds = wallSeconds,
            Queue = (code, first, second) => queue.Push(code, unchecked((int)first), unchecked((int)second)),
            Link = link, BaseHandle = _tree.EventHooks.Handle,
            Release = token => _tree.LifetimeHooks.Release!(token)
        };
        Window = new HtmlWindowGump(State, _hooks);
        HtmlWindowGump.BindHtmlWindow(_tree.EventHooks, _tree.InputHooks, _tree.FrameHooks, _tree.LifetimeHooks,
            token => _constructed.Contains(token) ? Window : null);
        Action<uint> release = _tree.LifetimeHooks.Release!;
        _tree.LifetimeHooks.Release = token => { release(token); _owned.Remove(token); _constructed.Remove(token); };
    }

    /// <summary>원본 크기별 HTML 소유 버튼 또는 본문 노드를 실제 공통 저장소에 할당한다.</summary>
    private uint AllocateChild(int size)
    {
        GumpCreateNode node = size switch
        {
            0x170 => new StoneButtonGumpNode(),
            0xd8 => new ScrollingTextGumpNode(),
            _ => throw new ArgumentOutOfRangeException(nameof(size), "HTML 자식 원본 크기 미지원")
        };
        uint token = _tree.AllocateNode(node); _owned.Add(token); return token;
    }

    /// <summary>실제 다중 줄 생성·파싱과 스크롤 소유자를 연결한다. 각 콜백은 같은 공통 저장소를 사용해야 한다.</summary>
    public void BindBody(Action<uint> construct, Action<uint, string> parse, Action<uint, int> scrollable)
    {
        ArgumentNullException.ThrowIfNull(construct); ArgumentNullException.ThrowIfNull(parse); ArgumentNullException.ThrowIfNull(scrollable);
        _hooks.ConstructBody = construct; _hooks.ParseBody = parse; _hooks.Scrollable = scrollable;
    }

    /// <summary>같은 공통 트리에 HTML 파생 부모를 생성하고 토큰을 돌려준다.</summary>
    /// <param name="width">원본 요청 폭.</param><param name="height">원본 요청 높이.</param>
    public uint Create(uint width, uint height)
    {
        uint token = _tree.AllocateNode(new HtmlWindowGumpNode()); _owned.Add(token);
        uint result = Window.Construct(token, width, height); _constructed.Add(token); return result;
    }

    /// <summary>등록된 직접 소유 노드를 삭제 요청하고 같은 공통 삭제 큐를 처리한다. 부분 생성 객체도 반환한다.</summary>
    public void CloseAll()
    {
        // 먼저 공통 등록표 순서로 살아 있는 소유 루트와 미부착 자식을 삭제한다.
        foreach (uint token in _tree.Create.Entries.ToArray())
            if (token != 0 && _owned.Contains(token) && _tree.Node(token) is { } node && (node.Flags & 0x10000) == 0)
                _tree.Lifetime.Kill(token);
        _tree.Lifetime.Cleanup();
        // 기본 생성 전에 실패한 객체만 남을 수 있다. 생성된 미등록 객체는 조용히 버리지 않는다.
        foreach (uint token in _owned.ToArray())
        {
            if (_tree.Create.Entries.Contains(token)) throw new InvalidOperationException("HTML 정리 후 등록 노드 잔류");
            _tree.ReleaseUnregistered(token); _owned.Remove(token); _constructed.Remove(token);
        }
    }
}
