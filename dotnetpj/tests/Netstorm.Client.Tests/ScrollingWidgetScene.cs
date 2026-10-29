using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 다중 줄 위젯과 실제 공통 Slider/글꼴/버퍼/입력/픽셀/수명을 결합한다.

/// <summary>고정 기대 결과와 분리한 실제 조립 입력.원본 실행 fixture를 생성하지 않는다.</summary>
internal sealed class ScrollingWidgetScene
{
    /// <summary>같은 실제 기본/돌 버튼/Slider/픽셀 장면.</summary>
    public ScrollWidgetScene Scene { get; }
    /// <summary>실제 다중 줄 토큰30.</summary>
    public ScrollingTextGumpNode Node { get; } = new() { TextFlags = 0x80, Unwritten = 0x1234, DragY = 91 };
    /// <summary>같은 실제 Slider 토큰31.</summary>
    public ScrollGumpNode Slider => (ScrollGumpNode)Scene.Nodes[31];
    /// <summary>같은 공통 조각 표.</summary>
    public StyleTextState Fragments { get; } = new();
    /// <summary>같은 실제 글꼴 가상 분배.</summary>
    public StyleText Style { get; }
    /// <summary>메뉴 제목과 같은 실제 조각/글꼴/팔레트 분배를 공유한다.</summary>
    public StyleTextHooks StyleHooks { get; } = new();
    /// <summary>같은 실제 소유/외부 버퍼 표.</summary>
    public StyleTextBufferStore Buffers { get; }
    /// <summary>현재 실제 버퍼 상태/처리기.</summary>
    public TextGumpBufferBinding Buffer => Buffers.Binding(Node.Buffer);
    /// <summary>같은 실제 표시/스크롤/선택 처리기.</summary>
    public ScrollingTextGump Text { get; }
    /// <summary>현재 실제 종류/사건 전역.</summary>
    public ScrollingTextGumpWidgetState State { get; }
    /// <summary>현재 실제 생성/가상 경계.</summary>
    public ScrollingTextGumpWidgetHooks Hooks { get; }
    /// <summary>실제 다중 줄 위젯.</summary>
    public ScrollingTextGumpWidgets Widgets { get; }
    /// <summary>원본 버퍼/Slider 할당 크기 관측.</summary>
    public List<int> Allocations { get; } = [];
    /// <summary>버퍼 free와 기본 노드 free의 실제 순서 관측.</summary>
    public List<string> Released { get; } = [];
    /// <summary>현재 원본 커서 선택 관측.</summary>
    public List<uint> Cursors { get; } = [];
    /// <summary>소유 버퍼 free 중 현재 필드를 바꿀 수 있는 실제 경계.</summary>
    public Action<uint>? OnBufferFree { get; set; }
    /// <summary>원본 크기의 별도 버퍼/Slider 토큰만 제공한다.공통 자식은 기존 할당기로 생성한다.</summary>
    private uint Allocate(int size)
    { Allocations.Add(size); if (size == 0x38) return 500; if (size == 0xd8) { Scene.Nodes.Add(31, new ScrollGumpNode()); return 31; } throw new InvalidOperationException("예상하지 않은 다중 줄 할당 크기"); }
    /// <summary>같은 실제 트리/사건/수명 분배에 표시와 위젯을 추가한다.</summary>
    public ScrollingWidgetScene(OriginalEdition edition = OriginalEdition.Patch1078, bool titleFonts = false, ScrollWidgetScene? shared = null)
    {
        Scene = shared ?? new(edition); Scene.Nodes.Add(30, Node); var fonts = StyleTextTestSupport.PixelFonts(titleFonts); StyleText.BindStyleTextFonts(StyleHooks, new(fonts, edition), new(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(Scene.Pixels, 64, 128, Scene.Clip), new()), edition)); Style = new(Fragments, StyleHooks, edition); var bufferHooks = new StyleTextBufferHooks(); StyleTextBuffer.BindStyleTextBufferFonts(bufferHooks, new(fonts, edition)); Buffers = new(Fragments, Style, bufferHooks, t => { Released.Add("buffer:" + t); OnBufferFree?.Invoke(t); });
        State = new() { Alignment = -7, SliderType = 77, SliderFrame = 171, ChangeEvent = Scene.State.ChangeEvent, StepEvent = Scene.State.StepEvent, RefreshEvent = 0x12501, CursorEvent = 0x12502, LeftTagEvent = 0x12503, RightTagEvent = 0x12504 };
        var textHooks = new ScrollingTextGumpHooks { Node = t => t == 30 ? Node : null, BaseNode = t => Scene.Nodes.GetValueOrDefault(t), Redraw = (t, _) => Scene.Redraws.Add(t) }; StyleTextBufferStore.BindScrollingTextBuffers(textHooks, Buffers); ScrollGump.BindScrollingTextSlider(textHooks, Scene.Slider); Text = new(Scene.Create, Scene.Collection, Scene.Clip, Fragments, Style, textHooks);
        Hooks = new() { Node = t => t == 30 ? Node : null, Slider = t => Scene.Nodes.GetValueOrDefault(t) as ScrollGumpNode, Base = t => Scene.Nodes.GetValueOrDefault(t), Allocate = Allocate, Push = t => Scene.TreeHooks.Push!(t), Attach = (t, x, y, p, m) => Scene.TreeHooks.Attach!(t, x, y, p, m), Redraw = (t, _) => Scene.Redraws.Add(t), Cursor = c => Cursors.Add(c) }; Widgets = new(Scene.Create, State, Scene.Factory, Scene.Tree, Scene.Lifetime, Scene.Events, Scene.Input, Fragments, Buffers, Text, Scene.Widgets, Hooks);
        ScrollingTextGumpWidgets.BindScrollingTextGumpWidgets(Scene.TreeHooks, Scene.EventHooks, Scene.InputHooks, Scene.FrameHooks, Scene.LifetimeHooks, t => t == 30 ? Widgets : null); TextGump.BindTextGumpDraw(Scene.DrawHooks, _ => null, t => t == 30 ? Text : null); var release = Scene.LifetimeHooks.Release; Scene.LifetimeHooks.Release = t => { Released.Add("node:" + t); release!(t); };
    }
    /// <summary>실제 기본 생성과 루트 부착을 수행한다.</summary>
    public void Construct(int width = 30, int height = 40) { Widgets.Construct(30, width, height, 0, 171); Widgets.Attach(30, 2, 3, 1, 0); }
    /// <summary>실제 일반 입력의 명시 길이/태그를 추가하고 위에서부터 출력한다.</summary>
    public void AppendAndDraw(ReadOnlySpan<byte> text, uint metadata = 0) { Text.AppendText(30, text, text.Length, metadata); Text.RequestScroll(30, 0); Scene.Draw.Draw(30); }
}
