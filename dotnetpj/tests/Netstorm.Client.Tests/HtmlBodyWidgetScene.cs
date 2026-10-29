using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 실제 HTML 부모와 스크롤/글꼴/링크·공통 입력/수명을 같은 저장소에 조립한다.

/// <summary>원본 기대값을 생성하지 않는 헤드리스 장치 입력.전체 메뉴 소유자는 후속이다.</summary>
internal sealed class HtmlBodyWidgetScene
{
    /// <summary>실제 공통 등록/트리/Slider/입력/수명/8비트 표면.</summary>
    internal ScrollWidgetScene Common { get; } = new();
    /// <summary>실제 HTML 부모40.</summary>
    internal HtmlBodyGumpNode Node { get; } = new();
    /// <summary>실제 다중 줄 자식30.</summary>
    internal ScrollingTextGumpNode TextNode { get; } = new();
    /// <summary>실제 소유 조각 표.</summary>
    internal StyleTextState Fragments { get; } = new();
    /// <summary>같은 실제 문자열/그림 가상 처리기.</summary>
    internal StyleText Style { get; }
    /// <summary>같은 실제 다중 줄 버퍼 소유자.</summary>
    internal StyleTextBufferStore Buffers { get; }
    /// <summary>현재 표시/적중/스크롤.</summary>
    internal ScrollingTextGump Text { get; }
    /// <summary>현재 실제 자식 생성/부착/입력/수명.</summary>
    internal ScrollingTextGumpWidgets Widgets { get; }
    /// <summary>현재 본문 T태그/StyleText 해석.</summary>
    internal ScrollingTextGumpMarkup Markup { get; }
    /// <summary>실제 HTML 부모 본체.</summary>
    internal HtmlBodyGump Body { get; }
    /// <summary>부모의 같은 실제 링크 소유자.</summary>
    internal HtmlLinkTextState Links { get; } = new() { Base = 0x63000000, Capacity = HtmlBodyGumpNode.Capacity };
    /// <summary>같은 현재 HTML 전역.</summary>
    internal HtmlTextParserGlobals Globals { get; } = new();
    /// <summary>현재 원문 명령/조건/그림 효과.</summary>
    internal HtmlTextParserHooks Effects { get; } = new();
    /// <summary>실제 버퍼 free 관측.</summary>
    internal List<uint> FreedBuffers { get; } = [];
    /// <summary>실제 자식 버퍼/Slider의 원문 크기 할당 입력.</summary>
    private uint Allocate(int bytes) { if (bytes == 0x38) return 500; if (bytes == 0xd8) { Common.Nodes.Add(31, new ScrollGumpNode()); return 31; } throw new ArgumentException("HTML 자식 원문 할당 크기"); }
    /// <summary>보호 캐시 글꼴과 실제 공통 UI/버퍼/표시/입력/수명을 연결한다.</summary>
    internal HtmlBodyWidgetScene()
    {
        Common.Nodes.Add(40, Node); Common.Nodes.Add(30, TextNode); var files = GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals")); var fonts = new FontStore(files.TryReadAllBytes, "Arial", (_, _, _, _) => throw new InvalidOperationException("검사에서 보호 캐시 외 GDI 생성 안 함")); var styleHooks = new StyleTextHooks { Color = code => code == (byte)'k' ? 9U : 77U }; StyleText.BindStyleTextFonts(styleHooks, new(fonts), new(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(Common.Pixels, 64, 128, Common.Clip), new()))); Style = new(Fragments, styleHooks); var bufferHooks = new StyleTextBufferHooks(); StyleTextBuffer.BindStyleTextBufferFonts(bufferHooks, new(fonts)); Buffers = new(Fragments, Style, bufferHooks, token => FreedBuffers.Add(token));
        var textHooks = new ScrollingTextGumpHooks { Node = t => t == 30 ? TextNode : null, BaseNode = t => Common.Nodes.GetValueOrDefault(t), Redraw = (t, _) => Common.Redraws.Add(t) }; StyleTextBufferStore.BindScrollingTextBuffers(textHooks, Buffers); ScrollGump.BindScrollingTextSlider(textHooks, Common.Slider); Text = new(Common.Create, Common.Collection, Common.Clip, Fragments, Style, textHooks); var widgetState = new ScrollingTextGumpWidgetState { SliderType = 77, SliderFrame = 171, ChangeEvent = Common.State.ChangeEvent, StepEvent = Common.State.StepEvent, RefreshEvent = 0x12501, CursorEvent = 0x12502, LeftTagEvent = 0x12503, RightTagEvent = 0x12504 }; var widgetHooks = new ScrollingTextGumpWidgetHooks { Node = t => t == 30 ? TextNode : null, Slider = t => Common.Nodes.GetValueOrDefault(t) as ScrollGumpNode, Base = t => Common.Nodes.GetValueOrDefault(t), Allocate = Allocate, Push = t => Common.TreeHooks.Push!(t), Attach = (t, x, y, p, m) => Common.TreeHooks.Attach!(t, x, y, p, m), Redraw = (t, _) => Common.Redraws.Add(t), Cursor = _ => { } }; Widgets = new(Common.Create, widgetState, Common.Factory, Common.Tree, Common.Lifetime, Common.Events, Common.Input, Fragments, Buffers, Text, Common.Widgets, widgetHooks);
        ScrollingTextGumpWidgets.BindScrollingTextGumpWidgets(Common.TreeHooks, Common.EventHooks, Common.InputHooks, Common.FrameHooks, Common.LifetimeHooks, t => t == 30 ? Widgets : null); TextGump.BindTextGumpDraw(Common.DrawHooks, _ => null, t => t == 30 ? Text : null); var pictures = new Markup(Fragments, new(), new()); Netstorm.Client.Gump.Markup.BindMarkup(styleHooks, pictures); var markup = new StyleTextMarkup(Fragments, pictures, new(), new()); markup.Hooks(styleHooks); Markup = new(Text, Style, pictures, markup, new() { Node = t => t == 30 ? TextNode : null, Base = t => Common.Nodes.GetValueOrDefault(t) });
        var bodyHooks = new HtmlBodyGumpHooks { Node = t => t == 40 ? Node : null, Base = t => Common.Nodes.GetValueOrDefault(t), Allocate = bytes => { if (bytes != 0xd8) throw new ArgumentException("HTML 표시 자식 할당 크기"); return 30; }, Push = t => Common.TreeHooks.Push!(t), Attach = (t, x, y, p, m) => Common.TreeHooks.Attach!(t, x, y, p, m) }; HtmlBodyGump.BindHtmlBodyGumpDevices(bodyHooks, Common.Factory, Common.Tree, Common.Lifetime, Widgets, Text, Markup); Body = new(new() { ScreenWidth = 64, ScreenHeight = 128, Color = 171 }, bodyHooks); HtmlBodyGump.BindHtmlBodyGump(Common.TreeHooks, t => t == 40 ? Body : null); var release = Common.LifetimeHooks.Release!; Common.LifetimeHooks.Release = token => { release(token); Common.Nodes.Remove(token); }; Body.Construct(40);
    }
    /// <summary>실제 현재 부모의 두 영역에 기본 HTML/링크 파서를 직접 연결한다.</summary>
    internal void SetHtml(string source) => Body.SetHtml(40, source, (text, body, links) => HtmlBodyTextParser.Parse(text, body, links, Links, Globals, Effects));
}
