using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 메뉴 생성·장식·제목·버퍼·Slider·기본 트리/수명 조립 입력이다.

/// <summary>원본 기대값을 생성하지 않고 같은 실제 저장소/가상 분배를 조립한다.</summary>
internal sealed class MenuWidgetScene
{
    /// <summary>같은 실제 기본 트리/사건/수명/픽셀.</summary>
    public ScrollWidgetScene Scene { get; }
    /// <summary>아직 생성하지 않은 실제 제목 토큰30/버퍼/Slider 처리기.</summary>
    public ScrollingWidgetScene Title { get; }
    /// <summary>같은 실제 장식 토큰50/배경/픽셀.</summary>
    public DecorationScene Decoration { get; }
    /// <summary>같은 실제 항목/설정/제목 파서.</summary>
    public MenuControlScene Control { get; }
    /// <summary>같은 실제 메뉴 기본/레이아웃/소유 조각.</summary>
    public MenuLayoutScene Menu => Control.Menu;
    /// <summary>현재 실제 자산/종류 전역.</summary>
    public MenuGumpWidgetState State { get; } = new() { BackgroundType = 82, Foreground = 171, Shadow = 9 };
    /// <summary>현재 실제 할당/부착/선택 경계.</summary>
    public MenuGumpWidgetHooks Hooks { get; }
    /// <summary>실제 메뉴 생성/수명 처리기.</summary>
    public MenuGumpWidgets Widgets { get; }
    /// <summary>선택 통지 뒤 현재 항목 변경 같은 명시 외부 입력.</summary>
    public Action<uint, int>? OnSelection { get; set; }
    /// <summary>원본 요청 할당 크기.</summary>
    public List<int> Allocations { get; } = [];
    /// <summary>실제 선택/조각/기본 반납의 순서 관측.</summary>
    public List<string> Order { get; } = [];
    /// <summary>다음 별도 메뉴 토큰.</summary>
    private uint _nextMenu = 60;
    /// <summary>메뉴는 안정된 새 객체에,장식/제목은 이 장면의 전용 미생성 객체에 할당한다.</summary>
    private uint Allocate(int bytes)
    {
        Allocations.Add(bytes); if (bytes == 0xc4) return 50; if (bytes == 0xd8) return 30; if (bytes != 0xb90) throw new InvalidOperationException("메뉴 검사 할당 크기 오류"); if (!Scene.Create.Entries.Contains(40U)) return 40; uint token = _nextMenu++; Scene.Nodes.Add(token, new MenuGumpNode()); return token;
    }
    /// <summary>같은 기본 트리에 미생성 제목/장식/메뉴를 넣고 실제 현재 가상 분배를 연결한다.</summary>
    public MenuWidgetScene(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        Scene = new(edition); Title = new(edition, true, Scene); Decoration = new(edition, Scene); Control = new(edition, Title, false); Menu.State.Index = 0; Array.Clear(Menu.State.Entries);
        // 원본 Pop은 현재 좌표/부모 재사용의 실제 가상 Attach다.
        Menu.MenuHooks.Push = t => Scene.TreeHooks.Push!(t); Menu.MenuHooks.Pop = t => { var n = Scene.Nodes[t]; Scene.TreeHooks.Attach!(t, n.X, n.Y, 0, 0); }; Menu.MenuHooks.Attach = (t, x, y, p, m) => Scene.TreeHooks.Attach!(t, x, y, p, m); Menu.MenuHooks.Redraw = (t, e) => Scene.FrameHooks.Redraw!(t, e); Menu.Hooks.Attach = Menu.MenuHooks.Attach; Menu.Hooks.Push = Menu.MenuHooks.Push; MenuGumpWidgets.BindMenuTitleLayout(Menu.Hooks, Title.Text);
        Hooks = new() { Node = Menu.MenuHooks.Node, Base = Menu.MenuHooks.BaseNode, Allocate = Allocate, Attach = Menu.MenuHooks.Attach, SelectionNotice = (c, s) => { Order.Add($"selection:{c},{s}"); OnSelection?.Invoke(c, s); } }; Widgets = new(Scene.Create, Menu.State, State, Decoration.Background, Decoration.Decoration, Title.Widgets, Menu.Layout, Menu.Style, Scene.Lifetime, Scene.Collection, Scene.Tree, Hooks);
        MenuGumpWidgets.BindMenuGumpWidgets(Scene.TreeHooks, Scene.LifetimeHooks, t => Scene.Nodes.GetValueOrDefault(t) is MenuGumpNode ? Widgets : null); MenuGumpWidgets.BindMenuGumpObjectAttach(Menu.MenuHooks, t => Scene.Nodes.GetValueOrDefault(t) is MenuGumpNode ? Widgets : null); MenuGump.BindMenuGumpFrame(Scene.FrameHooks, t => Scene.Nodes.GetValueOrDefault(t) is MenuGumpNode ? Menu.Menu : null); Scene.TreeHooks.Redraw = (t, e) => { if (Scene.Nodes[t] is MenuGumpNode or GumpDecorationNode) Scene.FrameHooks.Redraw!(t, e); else Scene.Redraws.Add(t); };
        var release = Scene.LifetimeHooks.Release; Scene.LifetimeHooks.Release = t => { Order.Add("node:" + t); release!(t); }; var styleRelease = Menu.StyleHooks.Release; Menu.StyleHooks.Release = t => { Order.Add("style:" + t); styleRelease!(t); };
    }
}
