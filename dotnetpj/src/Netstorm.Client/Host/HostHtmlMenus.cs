using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host; // HTML 창과 같은 실제 트리의 controlled 메뉴 열 소유 영역이다.

/// <summary>HTML 메타의 메뉴 열을 실제 글꼴·스타일·배경·공통 수명에 조립한다.
/// 근거: cpppj/src/client/RawMenuControls.cpp와 RawMenuHtml.inc.</summary>
public sealed class HostHtmlMenus
{
    /// <summary>메뉴와 창이 공유하는 실제 노드 저장소.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>생성 전 실패와 미부착 메뉴도 추적하는 HTML 소유자.</summary>
    private readonly HostHtmlWindows _windows;
    /// <summary>실제 controlled 스택과 인스턴스 수.</summary>
    public MenuGumpState State { get; } = new();
    /// <summary>현재 메뉴의 크기와 controlled 스택.</summary>
    public MenuGump Menu { get; }
    /// <summary>같은 실제 메뉴 생성 및 수명.</summary>
    public MenuGumpWidgets Widgets { get; }
    /// <summary>현재 열의 일반·체크 항목 작성기.</summary>
    public MenuGumpControls Controls { get; }
    /// <summary>공통 출력기가 같은 항목 측정 장치를 사용한다.</summary>
    internal MenuGumpLayout Layout { get; }
    /// <summary>한 번 연결한 실제 메뉴 입력 처리기.</summary>
    private MenuGumpInput? _input;

    /// <summary>같은 화면·시작 설정·본문 장치를 빌려 메뉴 열을 연결한다.</summary>
    public HostHtmlMenus(HostMainMenu menu, HostHtmlWindows windows, HostHtmlText text, HostStartupScene startup, Screen screen, Action<uint, bool> redraw)
    {
        ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(windows); ArgumentNullException.ThrowIfNull(text);
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(redraw);
        if (!text.BelongsTo(windows)) throw new ArgumentException("다른 HTML 창의 본문 장치", nameof(text));
        _tree = menu.Tree; _windows = windows;
        void Attach(uint token, float x, float y, uint parent, uint mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode);
        var hooks = new MenuGumpHooks { Node = token => _tree.Node(token) as MenuGumpNode, BaseNode = _tree.Node,
            Push = _tree.Tree.Push, Pop = token => { var node = _tree.Node(token) ?? throw new InvalidOperationException("메뉴 노드 없음"); Attach(token, node.X, node.Y, 0, 0); },
            Attach = Attach, Redraw = redraw, BaseRedraw = redraw };
        Menu = new MenuGump(_tree.Create, State, _tree.Lifetime, hooks);
        var layoutHooks = new MenuGumpLayoutHooks { Node = hooks.Node, BaseNode = _tree.Node, Push = _tree.Tree.Push, Attach = Attach };
        MenuGumpLayout.BindMenuLayoutText(layoutHooks, new GumpText(_tree.Fonts));
        StyleText.BindMenuLayoutStyleText(layoutHooks, text.Style); MenuGumpWidgets.BindMenuTitleLayout(layoutHooks, text.Text);
        var layout = Layout = new MenuGumpLayout(_tree.Create, Menu, layoutHooks);
        var palette = screen.Colors();
        var dim = palette.Named.First(color => color.Red == 47 && color.Green == 24 && color.Blue == 17);
        Widgets = new MenuGumpWidgets(_tree.Create, State, new() { BackgroundType = menu.BackgroundType, BackgroundFrame = 1,
            Foreground = palette.Basic[7], Shadow = dim.Index, ObjectFormType = 3 }, _tree.Backgrounds,
            menu.Decoration.CreateDevice(), text.Widgets, layout, text.Style, _tree.Lifetime, _tree.Collection, _tree.Tree,
            new() { Node = hooks.Node, Base = _tree.Node, Allocate = Allocate, Attach = Attach,
                SelectionNotice = (_, _) => throw new InvalidOperationException("메뉴 선택 통지 미연결") });
        Controls = new MenuGumpControls(_tree.Create, State, Menu, layout, text.Parser,
            new() { Node = hooks.Node, Base = _tree.Node, ReadInt = (string key, ref int value) => { startup.Configuration.ReadInt(key, ref value); } });
        MenuGumpWidgets.BindMenuGumpWidgets(_tree.TreeHooks, _tree.LifetimeHooks, token => _tree.Node(token) is MenuGumpNode ? Widgets : null);
        MenuGump.BindMenuGumpFrame(_tree.FrameHooks, token => _tree.Node(token) is MenuGumpNode ? Menu : null);
        windows.BindMenus(Menu);
    }

    /// <summary>원본 크기에 맞는 메뉴·장식·제목을 공통 트리에 할당하고 부분 생성 수명도 추적한다.</summary>
    private uint Allocate(int size)
    {
        uint token = _tree.AllocateNode(size switch
        {
            0xb90 => new MenuGumpNode(), 0xc4 => new GumpDecorationNode(), 0xd8 => new ScrollingTextGumpNode(),
            _ => throw new ArgumentOutOfRangeException(nameof(size), "HTML 메뉴 자식 크기 미지원")
        });
        _windows.TrackChild(token); return token;
    }

    /// <summary>HTML 메타의 일반·체크 열 작성이 이 소유자의 같은 controlled 스택을 사용하게 한다.</summary>
    public void BindMetadata(HtmlWindowMetadataHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        HtmlWindowMetadata.BindHtmlWindowMetadataMenus(hooks, Widgets, Controls);
    }

    /// <summary>같은 공통 사건·프레임과 큐, 이름 소리 및 재그리기에 메뉴 선택을 연결한다.</summary>
    public void BindInput(InputQueue queue, Action<string> sound, Action<uint, bool> redraw)
    {
        ArgumentNullException.ThrowIfNull(queue); ArgumentNullException.ThrowIfNull(sound); ArgumentNullException.ThrowIfNull(redraw);
        if (_input is not null) throw new InvalidOperationException("메뉴 입력이 이미 연결되었다");
        var hooks = new MenuGumpInputHooks
        {
            Node = token => _tree.Node(token) as MenuGumpNode, Base = _tree.Node,
            Broadcast = (token, code, first, second) => _tree.FrameHooks.Broadcast!(token, code, first, second),
            Handle = (token, code, first, second) => _tree.EventHooks.Handle!(token, code, first, second),
            Push = _tree.Tree.Push, Attach = (token, x, y, parent, mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode),
            Redraw = redraw, Command = (code, id, parameter) => queue.PushCommand(code, unchecked((int)id), unchecked((int)parameter)),
            Sound = sound, Invalidate = _ => redraw(_tree.Create.Entries[0], false)
        };
        _input = new MenuGumpInput(_tree.Create, new() { RequestEvent = 0x0200000d, ClosedEvent = 0x020000be },
            Menu, _tree.Collection, _tree.Events, _tree.Lifetime, _tree.Tree, hooks);
        MenuGumpInput.BindMenuGumpInput(_tree.EventHooks, _tree.InputHooks, _tree.FrameHooks,
            token => _tree.Node(token) is MenuGumpNode ? _input : null);
    }
}
