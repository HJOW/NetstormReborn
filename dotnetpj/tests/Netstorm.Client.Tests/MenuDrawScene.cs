using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 메뉴 생성/배경/표시/글리프를 같은 현재 클립과 픽셀로 결합한다.

/// <summary>원본 기대 연산 없이 실제 처리기와 명시 입력/관측 경계를 제공한다.</summary>
internal sealed class MenuDrawScene
{
    /// <summary>같은 실제 트리/메뉴/소유 조각/제목/장식.</summary>
    public MenuWidgetScene Ui { get; }
    /// <summary>실제 plain 글꼴 캐시.</summary>
    public FontStore Fonts { get; }
    /// <summary>현재 출력 색/배경/체크 전역.</summary>
    public MenuGumpDrawState State { get; } = new() { Foreground = 171, Disabled = 77, BackgroundType = 82, MarkerType = 82 };
    /// <summary>현재 실제 메뉴/체크 경계.</summary>
    public MenuGumpDrawHooks Hooks { get; }
    /// <summary>같은 실제 plain 글리프/색 경계.</summary>
    public GumpGlyphHooks GlyphHooks { get; }
    /// <summary>실제 메뉴 출력기.</summary>
    public MenuGumpDraw Draw { get; }
    /// <summary>실제 plain 글자 색 패스 관측.</summary>
    public List<byte> Colors { get; } = [];
    /// <summary>실제 plain 글리프와 현재 클립 관측.</summary>
    public List<(BitmapFont Font, byte Code, ScreenPoint Position, ScreenRect Clip)> Glyphs { get; } = [];
    /// <summary>실제 배경 셰이프 요청/현재 클립 관측.</summary>
    public List<(GumpBackgroundShapeRequest Request, ScreenRect Clip)> Shapes { get; } = [];
    /// <summary>현재 원색 체크 출력 관측.</summary>
    public List<(uint Type, ScreenPoint Point, ScreenRect Clip)> Markers { get; } = [];
    /// <summary>기존 실제 배경/StyleText와 plain 글리프·체크의 같은8비트 표면을 연결한다.</summary>
    public MenuDrawScene(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        Ui = new(edition); Fonts = StyleTextTestSupport.PixelFonts(true, true); GlyphHooks = GumpTextDraw.MakeGumpGlyphPixelHooks(new(Ui.Scene.Pixels, 64, 128, Ui.Scene.Clip), new()); var glyph = GlyphHooks.Glyph; var color = GlyphHooks.Color; GlyphHooks.Color = c => { Colors.Add(c); color!(c); }; GlyphHooks.Glyph = (f, c, p) => { Glyphs.Add((f, c, p, Ui.Scene.Clip.Clip)); glyph!(f, c, p); }; Hooks = new() { Node = Ui.Menu.MenuHooks.Node, Base = Ui.Menu.MenuHooks.BaseNode }; MenuGumpDraw.BindMenuGumpDrawAssets(Hooks, _ => 0, Ui.Decoration.Pixels); var marker = Hooks.Marker; Hooks.Marker = (t, p) => { Markers.Add((t, p, Ui.Scene.Clip.Clip)); marker!(t, p); }; var shape = Ui.Decoration.BackgroundHooks.Shape; Ui.Decoration.BackgroundHooks.Shape = r => { Shapes.Add((r, Ui.Scene.Clip.Clip)); shape!(r); }; Draw = new(Ui.Scene.Create, Ui.Scene.Clip, State, Ui.Scene.Collection, Ui.Decoration.BackgroundDraw, new(Fonts, GlyphHooks, edition), Ui.Menu.Style, Ui.Menu.Layout, Hooks); MenuGumpDraw.BindMenuGumpDraw(Ui.Scene.DrawHooks, t => Ui.Scene.Nodes.GetValueOrDefault(t) is MenuGumpNode ? Draw : null);
    }
    /// <summary>실제 생성/일반 루트 부착을 마치고 본문 배경을 명시 단색9로 선택한다.</summary>
    public void Begin(bool horizontal = false) { if (horizontal) Ui.Widgets.BeginHorizontal(); else Ui.Widgets.BeginVertical(); Ui.Menu.Node.BackgroundFlags = GumpBackgroundFlags.Solid | 9; }
    /// <summary>현재 실제 controlled 메뉴를 루트에 부착한다.</summary>
    public void Attach() => Ui.Menu.Menu.AttachCurrent(2, 3, 1);
}
