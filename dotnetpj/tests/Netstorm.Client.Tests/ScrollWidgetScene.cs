using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 Slider/화살표/배경/입력/수명을 같은 안정된 객체에 결합한다.

/// <summary>원본 결과를 생성하지 않는 헤드리스 조립 입력.일반 Renderer 정렬은 후속이며 각 Draw 대상은 검사에서 명시한다.</summary>
internal sealed class ScrollWidgetScene
{
    /// <summary>실제 기본/파생 노드 저장소.</summary>
    public Dictionary<uint, GumpCreateNode> Nodes { get; } = new() { [1] = new(), [2] = new ScrollGumpNode { ReservedBC = 0x1234 } };
    /// <summary>다음 원본 자식 할당 토큰.</summary>
    private uint _next = 10;
    /// <summary>실제 등록/판본 전역.</summary>
    public GumpCreateState Create { get; } = new() { ScreenWidth = 64, ScreenHeight = 128 };
    /// <summary>실제 idle/overlay/캡처/삭제 표.</summary>
    public GumpLifetimeState LifeState { get; } = new();
    /// <summary>실제 Slider 토큰2.</summary>
    public ScrollGumpNode Node => (ScrollGumpNode)Nodes[2];
    /// <summary>실제 생성한 손잡이.</summary>
    public ScrollGumpThumbNode Thumb => (ScrollGumpThumbNode)Nodes[Node.Thumb];
    /// <summary>실제 문자열 사건 초기화 입력.</summary>
    public ScrollGumpState State { get; } = new() { ClickEvent = 0x12346, RepeatEvent = 0x12348, StepEvent = 0x12349, ChangeEvent = 0x12350 };
    /// <summary>실제 현재 종류/기본 배경.</summary>
    public ScrollGumpWidgetState WidgetState { get; } = new() { DefaultType = 83 };
    /// <summary>실제 기본 생성기.</summary>
    public GumpCreate Factory { get; }
    /// <summary>실제 트리.</summary>
    public GumpTree Tree { get; }
    /// <summary>실제 수집/위치.</summary>
    public GumpCollection Collection { get; }
    /// <summary>실제 수명.</summary>
    public GumpLifetime Lifetime { get; }
    /// <summary>실제 입력 캡처.</summary>
    public GumpInput Input { get; }
    /// <summary>실제 Slider 동작.</summary>
    public ScrollGump Slider { get; }
    /// <summary>실제 Slider 자식 생성/출력.</summary>
    public ScrollGumpWidgets Widgets { get; }
    /// <summary>실제 돌 버튼.</summary>
    public StoneButtonGump Stones { get; }
    /// <summary>현재 폴링/재그리기/부착 경계.</summary>
    public ScrollGumpHooks SliderHooks { get; }
    /// <summary>현재 원본 할당 경계.</summary>
    public ScrollGumpWidgetHooks WidgetHooks { get; }
    /// <summary>직접 캡처 입력 분배.</summary>
    public GumpInputHooks InputHooks { get; }
    /// <summary>프레임 통지 분배.</summary>
    public GumpFrameHooks FrameHooks { get; }
    /// <summary>자식 할당 요청 크기의 실제 관측.</summary>
    public List<int> Allocations { get; } = [];
    /// <summary>실제 기본 free 순서.</summary>
    public List<uint> Released { get; } = [];
    /// <summary>실제 부모 체인 최종 도달 관측.</summary>
    public List<(uint Code, uint Serial, uint Value)> Notices { get; } = [];
    /// <summary>현재 부모 소비/변경 경계.</summary>
    public Func<uint, uint, uint, uint>? ParentHandle { get; set; }
    /// <summary>현재 폴링 입력.</summary>
    public InputEvent Poll { get; set; } = new(InputCode.LeftButton, 999, 999);
    /// <summary>실제 가상 재그리기 관측.</summary>
    public List<uint> Redraws { get; } = [];
    /// <summary>실제 출력 클립.</summary>
    public GumpDrawState Clip { get; } = new();
    /// <summary>실제 질감 pass 클립 관측.</summary>
    public List<ScreenRect> Clips { get; } = [];
    /// <summary>실제 자산 출력 요청 관측.</summary>
    public List<GumpBackgroundShapeRequest> Shapes { get; } = [];
    /// <summary>앞뒤 guard 포함 할당.</summary>
    public byte[] Guarded { get; } = Enumerable.Repeat((byte)7, 8194).ToArray();
    /// <summary>64×128 실제 표면.</summary>
    public Memory<byte> Pixels => Guarded.AsMemory(1, 8192);
    /// <summary>실제 공통 Draw 분배.</summary>
    public GumpDraw Draw { get; }
    /// <summary>다중 줄 조립에서도 같은 실제 트리 가상 분배를 확장한다.</summary>
    public GumpTreeHooks TreeHooks { get; }
    /// <summary>같은 자식 우선 scalar 분배.</summary>
    public GumpLifetimeHooks LifetimeHooks { get; }
    /// <summary>같은 부모/재귀 사건 분배.</summary>
    public GumpEventHooks EventHooks { get; }
    /// <summary>같은 실제 부모 체인 처리기.</summary>
    public GumpEvents Events { get; }
    /// <summary>같은 실제 픽셀 Draw 분배.</summary>
    public GumpDrawHooks DrawHooks { get; }
    /// <summary>같은 기본/파생 저장소를 조회한다.</summary>
    private GumpCreateNode? Find(uint token) => Nodes.GetValueOrDefault(token);
    /// <summary>실제 배경+b4의 ref 저장 필드.</summary>
    private ref uint BackgroundFlags(uint token) => ref ((GumpBackgroundNode)Nodes[token]).BackgroundFlags;
    /// <summary>지정 원본 크기의 배경/화살표/손잡이를 안정된 토큰에 할당한다.</summary>
    private uint Allocate(int size)
    {
        Allocations.Add(size); uint token = _next++; Nodes.Add(token, size switch { 0xb8 => new GumpBackgroundNode(), 0x178 => new ScrollGumpArrowNode(), 0xd8 or 0xd4 => new ScrollGumpThumbNode { PreviousY = 73, PreviousX = 91 }, _ => throw new InvalidOperationException("예상하지 않은 Slider 할당 크기") }); return token;
    }
    /// <summary>실제 기본 생성/부모 통지/캡처/트리/돌 버튼/질감/글꼴/수명을 연결한다.</summary>
    public ScrollWidgetScene(OriginalEdition edition = OriginalEdition.Patch1078)
    {
        Create.Collection.Edition = edition; Factory = new(Create, new() { Node = Find, Allocate = _ => 1 }); Factory.InitializeRoot(); Collection = new(Create.Collection, Factory.CollectionHooks());
        var branch = TreeHooks = new GumpTreeHooks { Node = Find, Redraw = (t, _) => Redraws.Add(t), Push = t => Tree!.Push(t), Attach = (t, x, y, p, m) => Tree!.Attach(t, x, y, p, m), Kill = t => Lifetime!.Kill(t) }; var life = LifetimeHooks = new GumpLifetimeHooks { Node = Find, Push = branch.Push, DeletingDestructor = (t, f) => Lifetime!.DeleteScalar(t, f), Release = t => Released.Add(t) }; StoneButtonGump.BindStoneButtonGump(branch, life, t => Find(t) is ScrollGumpArrowNode ? Stones : null); ScrollGumpWidgets.BindScrollGumpWidgetLifetime(life, t => Find(t) is ScrollGumpNode or ScrollGumpArrowNode or ScrollGumpThumbNode ? Widgets : null);
        Tree = new(Create, new() { MapView = new(0, 0, 64, 128) }, branch); Lifetime = new(Create, LifeState, life); var eventsHooks = EventHooks = new GumpEventHooks { Node = Find, Handle = (t, c, a, b) => { if (t == 1) { Notices.Add((c, a, b)); return ParentHandle?.Invoke(c, a, b) ?? 0; } return 0; } }; InputHooks = new() { Node = Find, Handle = eventsHooks.Handle }; FrameHooks = new() { Node = Find, Broadcast = (_, _, _, _) => { } }; ButtonGump.BindButtonGump(eventsHooks, InputHooks, FrameHooks, t => Find(t) is ScrollGumpArrowNode ? Buttons : null); ScrollGump.BindScrollGump(branch, eventsHooks, InputHooks, FrameHooks, t => Find(t) is ScrollGumpNode ? Slider : null, t => Find(t) is ScrollGumpThumbNode ? Slider : null);
        var events = Events = new GumpEvents(Create, LifeState, new(), Collection, eventsHooks); Input = new(Create, LifeState, new(), Collection, events, InputHooks); Buttons = new(Create, new() { ClickEvent = State.ClickEvent, RepeatEvent = State.RepeatEvent }, Factory, Collection, events, Input, Lifetime, ButtonHooks = new() { Node = t => Find(t) as ButtonGumpNode, BaseNode = Find, BackgroundFlags = BackgroundFlags, Redraw = (t, _) => Redraws.Add(t), Now = () => 1, Sound = () => { } });
        var assets = GumpBackgroundTestAssets.Assets(); var bgCreate = new GumpBackgroundCreateHooks { Node = t => Find(t) as GumpBackgroundNode }; var bgDraw = new GumpBackgroundDrawHooks { Node = t => Find(t) as GumpBackgroundNode }; GumpBackgroundAssets.BindGumpBackgroundAssets(bgCreate, bgDraw, assets); var backgrounds = new GumpBackgroundCreate(Create, Factory, () => 0, bgCreate); var fonts = StyleTextTestSupport.PixelFonts(); var stoneHooks = new StoneButtonGumpHooks { Node = t => Find(t) as StoneButtonGumpNode, Allocate = Allocate, Push = branch.Push, Attach = branch.Attach }; GumpText.BindStoneButtonText(stoneHooks, new(fonts, edition)); Stones = new(Create, new() { BackgroundType = 82, Palette = new(9, 243, 57) }, Factory, Buttons, backgrounds, Tree, Lifetime, stoneHooks);
        SliderHooks = new() { Node = t => Find(t) as ScrollGumpNode, ThumbNode = t => Find(t) as ScrollGumpThumbNode, BaseNode = Find, Push = branch.Push, Attach = branch.Attach, Redraw = (t, _) => Redraws.Add(t), Poll = _ => Poll }; Slider = new(Create, State, Tree, Collection, events, Input, SliderHooks); WidgetHooks = new() { Node = t => Find(t) as ScrollGumpNode, Arrow = t => Find(t) as ScrollGumpArrowNode, Thumb = t => Find(t) as ScrollGumpThumbNode, Base = Find, Allocate = Allocate, Attach = branch.Attach }; Widgets = new(Create, WidgetState, Factory, Stones, backgrounds, Slider, Lifetime, WidgetHooks);
        GumpDraw.SetGumpDrawClip(Clip, new(0, 0, 64, 128)); var target = new GumpPixelTarget(Pixels, 64, 128, Clip); byte[] colors = new byte[256]; var raster = new GumpBackgroundPixelTarget(target, assets, GumpBackgroundTestAssets.Maps(), colors); raster.Hooks(Clip, bgDraw); var setClip = bgDraw.Clip; bgDraw.Clip = r => { Clips.Add(r); setClip!(r); }; var shape = bgDraw.Shape; bgDraw.Shape = r => { Shapes.Add(r); shape!(r); }; var backgroundDraw = new GumpBackgroundDraw(Collection, Clip, new(), bgDraw); var text = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, new()), edition); var draw = DrawHooks = new GumpDrawHooks { Draw = _ => { } }; ScrollGumpWidgets.BindScrollGumpWidgetDraw(draw, Collection, text, target, backgroundDraw, t => Find(t) is ScrollGumpNode ? Widgets : null, t => Find(t) is ScrollGumpArrowNode ? Widgets : null, t => Find(t) is ScrollGumpThumbNode ? Widgets : null); Draw = new(Collection, Clip, draw);
    }
    /// <summary>원본 버튼 상태 처리기.</summary>
    public ButtonGump Buttons { get; }
    /// <summary>같은 실제 기본 버튼의 도움말/장치 경계를 상위 UI 조립에 연결한다.</summary>
    public ButtonGumpHooks ButtonHooks { get; }
    /// <summary>일반 세로 Slider를 생성하고 루트에 실제 부착한다.</summary>
    public void ConstructAndAttach() { Widgets.Construct(2, 10, 100, 77, 171); Slider.Attach(2, 2, 3, 1, 0); Notices.Clear(); Redraws.Clear(); }
}
