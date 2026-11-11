using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 실제 호스트의 공통 Gump 수집·정렬·8비트 출력 영역이다.

/// <summary>메뉴와 HTML 창의 현재 트리를 원본 수집·CRT 정렬 순서로 같은 논리 표면에 출력한다.
/// 근거: cpppj/src/client/RawMenuControls.cpp의 CollectDrawing·WithDrawing.</summary>
public sealed class HostGumpDrawing
{
    /// <summary>원본 표시 작업 배열의 고정 용량.</summary>
    private const int Capacity = 5000;
    /// <summary>실제 트리와 자산 소유자.</summary>
    private readonly HostMainMenu _menu;
    /// <summary>실제 본문·슬라이더·공유 글리프 장치.</summary>
    private readonly HostHtmlText _text;
    /// <summary>같은 메뉴 항목 측정·스타일 장치.</summary>
    private readonly HostHtmlMenus _menus;
    /// <summary>현재 파일 팔레트와 논리 화면.</summary>
    private readonly Screen _screen;
    private readonly HostTextEntry? _entry;
    /// <summary>같은 스타일 저장소를 사용하는 한 줄 제목 출력기다.</summary>
    private readonly TextGump _single;

    /// <summary>같은 실제 메뉴 트리와 화면 장치를 빌린다.</summary>
    public HostGumpDrawing(HostMainMenu menu, HostHtmlText text, HostHtmlMenus menus, Screen screen, HostTextEntry? entry = null)
    { _menu = menu ?? throw new ArgumentNullException(nameof(menu)); _text = text ?? throw new ArgumentNullException(nameof(text)); _menus = menus ?? throw new ArgumentNullException(nameof(menus)); _screen = screen ?? throw new ArgumentNullException(nameof(screen)); _entry = entry;
        var tree = menu.Tree; _single = new TextGump(tree.Create, new(), tree.Factory, tree.Lifetime, tree.Collection, text.Clip, text.Style,
            new() { Node = token => tree.Node(token) as TextGumpNode, BaseNode = tree.Node }); }

    /// <summary>현재 표시 트리를 수집하고 원본 정렬 뒤 각 노드를 같은 픽셀·클립 문맥에서 출력한다.</summary>
    public void Draw()
    {
        var tree = _menu.Tree; var entries = new RenderCollectionEntry[Capacity]; var view = new ScreenRect(0, 0, _screen.Width, _screen.Height);
        int count = tree.Collection.Collect(view, entries, Capacity); DrawSort.Sort(entries.AsSpan(0, count), tree.Create.Collection.Edition);
        byte[] pixels = _screen.Lock();
        try
        {
            var clip = _text.Clip; GumpDraw.SetGumpDrawClip(clip, view);
            var target = new GumpPixelTarget(pixels, _screen.Pitch, _screen.Height, clip); var colors = new GumpGlyphColorState();
            var glyph = new GumpTextDraw(tree.Fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, colors));
            var backgroundState = new GumpBackgroundDrawState(); GumpBackgroundDraw.SetGumpBackgroundColors(backgroundState, _screen.Colors());
            var backgroundHooks = new GumpBackgroundDrawHooks { Node = token => tree.Node(token) as GumpBackgroundNode };
            GumpBackgroundAssets.BindGumpBackgroundAssets(new(), backgroundHooks, tree.BackgroundAssets);
            new GumpBackgroundPixelTarget(target, tree.BackgroundAssets, _screen.ShadeMaps(), colors.VfxColors).Hooks(clip, backgroundHooks);
            var background = new GumpBackgroundDraw(tree.Collection, clip, backgroundState, backgroundHooks);
            var cornerPixels = new GumpDecorationPixelTarget(target, _menu.Decoration.CornerAssets);
            var decoration = _menu.Decoration.Device(background, cornerPixels.Hooks());
            var palette = _screen.Colors(); var dim = palette.Named.First(color => color.Red == 47 && color.Green == 24 && color.Blue == 17);
            var menuHooks = new MenuGumpDrawHooks { Node = token => tree.Node(token) as MenuGumpNode, Base = tree.Node };
            MenuGumpDraw.BindMenuGumpDrawAssets(menuHooks, _ => _menu.MarkerFrame, cornerPixels);
            var menuDraw = new MenuGumpDraw(tree.Create, clip, new() { Foreground = palette.Basic[7], Disabled = dim.Index,
                BackgroundType = _menu.BackgroundType, BackgroundFrame = 1, MarkerType = _menu.BackgroundType }, tree.Collection,
                background, glyph, _text.Style, _menus.Layout, menuHooks);
            var dispatch = new GumpDraw(tree.Collection, clip, new() { Draw = token =>
            {
                switch (tree.Node(token))
                {
                    case MenuGumpNode: menuDraw.Draw(token); break;
                    case GumpDecorationNode: decoration.Draw(token); break;
                    case ScrollingTextGumpNode: _text.Text.Draw(token); break;
                    case TextGumpNode: _single.Draw(token); break;
                    case ScrollGumpArrowNode: _text.SliderWidgets.DrawArrow(token, tree.Collection, glyph, target); break;
                    case ScrollGumpThumbNode: _text.SliderWidgets.DrawThumb(token, tree.Collection, background); break;
                    case ScrollGumpNode: break;
                    case StoneButtonGumpNode:
                        // 모달 대화창(Tell 등)이 열리면 메인 메뉴 돌 버튼은 그리지 않는다. 대화창 안의 돌 버튼(Cancel 등)은 그대로 그린다.
                        // 근거: cpppj/src/client/UberGump.cpp 2029행(MainMenu 단계에서만 버튼 그림)과 2065행(Dialog 단계는 Tell 창만).
                        // L4a menu-credits(0.382)에서 cpppj와 비교해 확인했다. 팝업 메뉴는 모달이 아니라 제외하지 않는다.
                        if (tree.LifeState.CurrentDialog != 0 && _menu.HideWithModal(token)) break;
                        tree.Stones.Draw(token, tree.Collection, glyph, target); break;
                    case TextEntryGumpNode: (_entry ?? throw new InvalidOperationException("문자 입력창 출력 소유자 미연결")).Draw(token, glyph, background, target); break;
                    case GumpBackgroundNode:
                        // 메인 메뉴 돌 버튼의 배경 자식도 모달이 열리면 함께 건너뛴다(별도 수집 항목이다. 0.382).
                        if (tree.LifeState.CurrentDialog != 0 && _menu.HideWithModal(token)) break;
                        background.Draw(token); break;
                }
            } });
            _text.WithTextDraw(glyph, () =>
            {
                // 수집된 모든 표시 항목을 같은 정렬 결과의 순서대로 한 번씩 분배한다.
                for (int i = 0; i < count; i++) dispatch.Draw(entries[i].Token);
            });
        }
        finally { _screen.Unlock(); }
        _screen.Update(view);
    }
}
