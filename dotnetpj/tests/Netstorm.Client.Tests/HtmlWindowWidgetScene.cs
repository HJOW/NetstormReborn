using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // 일반 HTML 창의 실제 장식/돌 버튼/동적 본문/공통 수명을 함께 조립한다.

/// <summary>보호 캐시 글꼴과 명시 RLE 배경 입력으로 구성한 헤드리스 장면. 전체 게임 자산 소유자는 후속이다.</summary>
internal sealed class HtmlWindowWidgetScene
{
    /// <summary>같은 실제 등록/트리/사건/수명/픽셀.</summary>
    internal ScrollWidgetScene Common { get; } = new();
    /// <summary>같은 실제 장식 생성/출력. 검사 배경/모서리는 독립 RLE 입력이다.</summary>
    internal DecorationScene Decoration { get; }
    /// <summary>같은 현재 창 전역.</summary>
    internal HtmlWindowGumpState State { get; } = new();
    /// <summary>같은 실제 소유 조각 표.</summary>
    internal StyleTextState Fragments { get; } = new();
    /// <summary>같은 실제 본문 버퍼 소유자.</summary>
    internal StyleTextBufferStore Buffers { get; }
    /// <summary>현재 실제 본문 측정/표시/스크롤.</summary>
    internal ScrollingTextGump Text { get; }
    /// <summary>현재 실제 동적 본문/Slider 생성·부착·수명.</summary>
    internal ScrollingTextGumpWidgets Widgets { get; }
    /// <summary>현재 실제 일반 HTML 창 본체.</summary>
    internal HtmlWindowGump Window { get; }
    /// <summary>현재 실제 전체 배치.</summary>
    internal HtmlWindowFit Fit { get; }
    /// <summary>현재 실제 메뉴 스택 연결을 바꿀 수 있는 배치 경계.</summary>
    internal HtmlWindowFitHooks FitHooks { get; }
    /// <summary>원시 입력 큐.</summary>
    internal InputQueue Queue { get; } = new();
    /// <summary>큐 실행 중 실제 count의 관측.</summary>
    internal List<uint> QueueCounts { get; } = [];
    /// <summary>반납된 실제 버퍼 목록.</summary>
    internal List<uint> FreedBuffers { get; } = [];
    /// <summary>실제 Kill 뒤 링크 주소 소비 관측.</summary>
    internal List<uint> Links { get; } = [];
    /// <summary>현재 벽 시각 입력.</summary>
    internal double Now { get; set; } = 123.25;
    /// <summary>시계 경계에서 현재 필드를 바꾸는 명시 입력.</summary>
    internal Action? OnClock { get; set; }
    /// <summary>큐 경계에서 현재 필드를 바꾸는 명시 입력.</summary>
    internal Action? OnQueue { get; set; }
    /// <summary>원본 전역 주소별 현재 화면/뷰 입력.</summary>
    internal Dictionary<uint, uint> Globals { get; } = new() { [0x5ca9b4] = 640, [0x531860] = 640, [0x531864] = 480, [0x5c78f0] = 640, [0x5c78f4] = 480, [0x5ca9cc] = 0, [0x5ca9d4] = 640, [0x5ca9d8] = 480, [0x5ca9d0] = 0 };
    /// <summary>공통 Slider의 자체 토큰과 겹치지 않는 동적 할당기.</summary>
    private uint _next = 1000;
    /// <summary>현재 같은 실제 기본/파생 조회.</summary>
    private GumpCreateNode? Find(uint token) => Common.Nodes.GetValueOrDefault(token);
    /// <summary>원문 크기별 실제 기본/파생 저장소를 설치한다. 버퍼는 공통 표에 넣지 않는다.</summary>
    private uint Allocate(int bytes) { uint token = _next++; if (bytes != 0x38) Common.Nodes.Add(token, bytes switch { 0x170 => new StoneButtonGumpNode(), 0xd8 => new ScrollingTextGumpNode(), 0x21e8 => new HtmlWindowGumpNode(), 0xb8 => new GumpBackgroundNode(), _ => throw new ArgumentOutOfRangeException(nameof(bytes)) }); return token; }
    /// <summary>본문 생성 중 같은0xd8 크기는 실제 Slider다.</summary>
    private uint AllocateText(int bytes) { if (bytes != 0xd8) return Allocate(bytes); uint token = _next++; Common.Nodes.Add(token, new ScrollGumpNode()); return token; }
    /// <summary>공통 등록표에서 현재 serial을 읽는다. dead 표시와 실제 반납을 구별한다.</summary>
    internal uint Reference(uint serial) { if (serial == 0) return 0;
        // 등록된 실제 노드의 serial만 검사한다.
        foreach (uint token in Common.Create.Entries) if (token != 0 && Find(token)?.Serial == serial) return token; return 0;
    }
    /// <summary>동적 실제 본문/보호 글꼴·버퍼·돌 버튼·장식/가상 분배를 같은 공통 저장소에 연결한다.</summary>
    internal HtmlWindowWidgetScene()
    {
        Common.Create.ScreenWidth = 640; Common.Create.ScreenHeight = 480; Decoration = new(shared: Common); var files = GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals")); var fonts = new FontStore(files.TryReadAllBytes, "Arial", (_, _, _, _) => throw new InvalidOperationException("보호 글꼴 캐시 외 생성 안 함")); var styleHooks = new StyleTextHooks { Color = _ => 77 }; StyleText.BindStyleTextFonts(styleHooks, new(fonts), new(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(Common.Pixels, 64, 128, Common.Clip), new()))); var style = new StyleText(Fragments, styleHooks); var bufferHooks = new StyleTextBufferHooks(); StyleTextBuffer.BindStyleTextBufferFonts(bufferHooks, new(fonts)); Buffers = new(Fragments, style, bufferHooks, t => FreedBuffers.Add(t));
        var textHooks = new ScrollingTextGumpHooks { Node = t => Find(t) as ScrollingTextGumpNode, BaseNode = Find, Redraw = (t, _) => Common.Redraws.Add(t) }; StyleTextBufferStore.BindScrollingTextBuffers(textHooks, Buffers); ScrollGump.BindScrollingTextSlider(textHooks, Common.Slider); Text = new(Common.Create, Common.Collection, Common.Clip, Fragments, style, textHooks); var widgetState = new ScrollingTextGumpWidgetState { SliderType = 77, SliderFrame = 171, ChangeEvent = Common.State.ChangeEvent, StepEvent = Common.State.StepEvent, LeftTagEvent = State.TagEvent, RightTagEvent = 0x12504, RefreshEvent = 0x12501, CursorEvent = 0x12502 }; var widgetHooks = new ScrollingTextGumpWidgetHooks { Node = t => Find(t) as ScrollingTextGumpNode, Slider = t => Find(t) as ScrollGumpNode, Base = Find, Allocate = AllocateText, Push = t => Common.TreeHooks.Push!(t), Attach = (t, x, y, p, m) => Common.TreeHooks.Attach!(t, x, y, p, m), Redraw = (t, _) => Common.Redraws.Add(t), Cursor = _ => { } }; Widgets = new(Common.Create, widgetState, Common.Factory, Common.Tree, Common.Lifetime, Common.Events, Common.Input, Fragments, Buffers, Text, Common.Widgets, widgetHooks); ScrollingTextGumpWidgets.BindScrollingTextGumpWidgets(Common.TreeHooks, Common.EventHooks, Common.InputHooks, Common.FrameHooks, Common.LifetimeHooks, t => Find(t) is ScrollingTextGumpNode ? Widgets : null); TextGump.BindTextGumpDraw(Common.DrawHooks, _ => null, t => Find(t) is ScrollingTextGumpNode ? Text : null);
        var pictures = new Markup(Fragments, new(), new()); Markup.BindMarkup(styleHooks, pictures); var commands = new StyleTextMarkup(Fragments, pictures, new(), new()); commands.Hooks(styleHooks); var parser = new ScrollingTextGumpMarkup(Text, style, pictures, commands, new() { Node = t => Find(t) as ScrollingTextGumpNode, Base = Find });
        var stoneHooks = new StoneButtonGumpHooks { Node = t => Find(t) as StoneButtonGumpNode, Allocate = Allocate, Push = t => Common.TreeHooks.Push!(t), Attach = (t, x, y, p, m) => Common.TreeHooks.Attach!(t, x, y, p, m) }; GumpText.BindStoneButtonText(stoneHooks, new(fonts)); var stones = new StoneButtonGump(Common.Create, new() { BackgroundType = 82, Palette = new(9, 243, 57) }, Common.Factory, Common.Buttons, Decoration.Background, Common.Tree, Common.Lifetime, stoneHooks); StoneButtonGump.BindStoneButtonGump(Common.TreeHooks, Common.LifetimeHooks, t => Find(t) is StoneButtonGumpNode and not ScrollGumpArrowNode ? stones : null); ButtonGump.BindButtonGump(Common.EventHooks, Common.InputHooks, Common.FrameHooks, t => Find(t) is StoneButtonGumpNode and not ScrollGumpArrowNode ? Common.Buttons : null); State.ClickEvent = Common.State.ClickEvent;
        var hooks = new HtmlWindowGumpHooks { Node = t => Find(t) as HtmlWindowGumpNode, Base = Find, Button = t => Find(t) as ButtonGumpNode, Allocate = Allocate, Attach = (t, x, y, p, m) => Common.TreeHooks.Attach!(t, x, y, p, m), Reference = Reference, WallSeconds = () => { OnClock?.Invoke(); return Now; }, Queue = (c, a, b) => { QueueCounts.Add(State.Instances); Queue.Push(c, unchecked((int)a), unchecked((int)b)); OnQueue?.Invoke(); }, Link = t => Links.Add(t), BaseHandle = Common.EventHooks.Handle }; HtmlWindowGump.BindHtmlWindowDevices(hooks, Decoration.Decoration, Common.Tree, Common.Lifetime, Common.LifetimeHooks, Widgets, parser, () => 171); HtmlWindowGump.BindHtmlWindowButtons(hooks, stones, () => 9); Window = new(State, hooks);
        var fitHooks = FitHooks = new HtmlWindowFitHooks { Node = hooks.Node, Base = Find, Attach = hooks.Attach, Reference = Reference, Read = address => Globals[address] }; HtmlWindowFit.BindHtmlWindowFitDevices(fitHooks, stones, Common.Tree, Text, Widgets); Fit = new(fitHooks); HtmlWindowFit.BindHtmlWindowFit(Common.TreeHooks, t => Find(t) is HtmlWindowGumpNode ? Fit : null); HtmlWindowGump.BindHtmlWindow(Common.EventHooks, Common.InputHooks, Common.FrameHooks, Common.LifetimeHooks, t => Find(t) is HtmlWindowGumpNode ? Window : null); var release = Common.LifetimeHooks.Release!; Common.LifetimeHooks.Release = t => { release(t); Common.Nodes.Remove(t); };
    }
    /// <summary>실제 전체 저장소 할당/장식 생성/공통 등록.</summary>
    internal uint Create(uint width = 400, uint height = 30) { uint t = Allocate(0x21e8); return Window.Construct(t, width, height); }
    /// <summary>현재 같은 파생 객체를 관측한다.</summary>
    internal HtmlWindowGumpNode Root(uint token) => (HtmlWindowGumpNode)Common.Nodes[token];
    /// <summary>현재 같은 다중 줄 객체를 관측한다.</summary>
    internal ScrollingTextGumpNode Body(uint token) => (ScrollingTextGumpNode)Common.Nodes[Root(token).Body];
    /// <summary>공통 실제 가상 부착을 사용한다.</summary>
    internal void Attach(uint token) => Common.TreeHooks.Attach!(token, 0, 0, 1, 0);
}
