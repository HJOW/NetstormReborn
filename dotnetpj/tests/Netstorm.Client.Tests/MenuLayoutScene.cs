using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 메뉴 항목 소유/글꼴/픽셀/기본 등록·idle를 결합하는 입력 장면이다.

/// <summary>원본 기대값을 계산하지 않는 같은 실제 저장소 조립.</summary>
internal sealed class MenuLayoutScene
{
    /// <summary>실제 기본/트리/수명/픽셀 장면.</summary>
    public ScrollWidgetScene Scene { get; }
    /// <summary>실제 메뉴 토큰40.</summary>
    public MenuGumpNode Node { get; } = new() { MenuFlags = 0x80000 };
    /// <summary>현재 controlled 메뉴 표.</summary>
    public MenuGumpState State { get; } = new();
    /// <summary>현재 실제 크기/부착/재그리기 경계.</summary>
    public MenuGumpHooks MenuHooks { get; }
    /// <summary>같은 메뉴 본체.</summary>
    public MenuGump Menu { get; }
    /// <summary>현재 실제 글자/제목 레이아웃 경계.</summary>
    public MenuGumpLayoutHooks Hooks { get; }
    /// <summary>같은 전체 레이아웃.</summary>
    public MenuGumpLayout Layout { get; }
    /// <summary>실제 소유 조각 표.</summary>
    public StyleTextState Fragments { get; }
    /// <summary>실제 스타일 조각/글리프 처리기.</summary>
    public StyleText Style { get; }
    /// <summary>같은 실제 제목/항목이 공유하는 글꼴/팔레트/대괄호 경계.</summary>
    public StyleTextHooks StyleHooks { get; }
    /// <summary>실제 파일 팔레트 별칭 입력.</summary>
    public PaletteColorTable Palette { get; } = new();
    /// <summary>원본 소유 조각 반납 관측.</summary>
    public List<uint> Released { get; } = [];
    /// <summary>현재 가상 호출 관측.</summary>
    public List<string> Calls { get; } = [];
    /// <summary>실제 메뉴 기본 생성 뒤 같은 글꼴/조각 소유/측정과 픽셀을 연결한다.</summary>
    public MenuLayoutScene(OriginalEdition edition = OriginalEdition.Patch1078, ScrollingWidgetScene? title = null, bool construct = true)
    {
        Scene = title?.Scene ?? new(edition); Fragments = title?.Fragments ?? new(); StyleHooks = title?.StyleHooks ?? new(); Scene.Nodes.Add(40, Node); if (construct) Scene.Factory.Construct(40, 0, 0, 0, 1); Node.TextContext[2] = 171; Node.TextContext[6] = Node.TextContext[7] = 1; State.Index = 1; State.Entries[1] = 40;
        MenuHooks = new() { Node = t => Scene.Nodes.GetValueOrDefault(t) as MenuGumpNode, BaseNode = t => Scene.Nodes.GetValueOrDefault(t), Push = t => Calls.Add("push:" + t), Pop = t => Calls.Add("pop:" + t), Attach = (t, x, y, p, m) => Scene.Tree.Attach(t, x, y, p, m), BaseRedraw = (t, e) => Calls.Add($"base:{t},{e}"), Redraw = (t, e) => Calls.Add($"redraw:{t},{e}") }; Menu = new(Scene.Create, State, Scene.Lifetime, MenuHooks); MenuGump.BindMenuGumpFrame(Scene.FrameHooks, t => t == 40 ? Menu : null);
        var fonts = StyleTextTestSupport.PixelFonts(title is not null); var glyph = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(Scene.Pixels, 64, 128, Scene.Clip), new()), edition); StyleHooks.Release = t => Released.Add(t); StyleText.BindStyleTextFonts(StyleHooks, new(fonts, edition), glyph); Palette.Basic[0] = 9; Palette.Basic[1] = 171; Palette.Basic[7] = 77; Palette.Named.Add(new(47, 24, 17, 81)); StyleText.BindStyleTextPalette(StyleHooks, Palette); Style = title?.Style ?? new(Fragments, StyleHooks, edition);
        Hooks = new() { Node = MenuHooks.Node, BaseNode = MenuHooks.BaseNode, Attach = MenuHooks.Attach }; MenuGumpLayout.BindMenuLayoutText(Hooks, new(fonts, edition)); StyleText.BindMenuLayoutStyleText(Hooks, Style); Layout = new(Scene.Create, Menu, Hooks);
    }
}
