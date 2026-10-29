using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 실제 공통 등록/Slider/글꼴/픽셀/수명에 문자 입력창을 조립한다.

/// <summary>독립 입력 캐시를 실제 글꼴 판독기에 공급한다.원본 기대상태를 생성하지 않는다.</summary>
internal sealed class TextEntryWidgetScene
{
    /// <summary>같은 실제 공통 트리/자식/사건/표면.</summary>
    public ScrollWidgetScene Scene { get; } = new();
    /// <summary>같은 기본/배경/Console 객체.</summary>
    public ConsoleGumpNode Node { get; } = new() { ReservedC4 = 0x1234, Reserved10C = 0x5678, Reserved124 = 0xabcd };
    /// <summary>현재 실제 편집 물리/객체 저장소.</summary>
    public TextEntryStorage Storage { get; }
    /// <summary>현재 실제 편집 처리기.</summary>
    public TextEntryGump Entry { get; }
    /// <summary>실제 부모 사건과 같은 현재 사건 번호.</summary>
    public TextEntryGumpState State { get; }
    /// <summary>현재 실제 장치 경계.콜백 재진입/상태 변경을 허용한다.</summary>
    public TextEntryGumpHooks Hooks { get; }
    /// <summary>같은 실제 프레임idle 분배.</summary>
    public GumpFrame Frame { get; }
    /// <summary>현재 살아 있는 같은 편집 버퍼.</summary>
    public TextEntryBuffer Buffer => Storage.Find(Node.Buffer) ?? throw new InvalidOperationException("검사 실제 버퍼 없음");
    /// <summary>명시 외부 클립보드/시계 입력.</summary>
    public byte[] Clipboard { get; set; } = [];
    /// <summary>명시 실제 시각.</summary>
    public double Seconds { get; set; }
    /// <summary>클립보드 읽기 관측 수.</summary>
    public int Reads { get; private set; }
    /// <summary>클립보드 쓰기 관측 수.</summary>
    public int Writes { get; private set; }
    /// <summary>현재 제출 본문 관측.</summary>
    public byte[] Submitted { get; private set; } = [];
    /// <summary>자동완성 요청의 현재 단어/null 관측.</summary>
    public byte[]? CompletionWord { get; private set; }
    /// <summary>현재 후보 길이 관측.</summary>
    public int CompletionLength { get; private set; }
    /// <summary>현재Shift 후보 방향 관측.</summary>
    public bool CompletionShift { get; private set; }
    /// <summary>명시 자동완성 반환 입력.</summary>
    public byte[]? Completion { get; set; } = "AAAA"u8.ToArray();
    /// <summary>다음 실제 두 Slider 객체 토큰.</summary>
    private uint _nextSlider = 31;
    /// <summary>현재 할당 요청 관측.</summary>
    public List<int> Allocations { get; } = [];
    /// <summary>새 버퍼와 실제 Slider 객체만 할당한다.그 자식은 공통Widgets가 할당한다.</summary>
    private uint Allocate(int size) { Allocations.Add(size); if (size == 0x28) { Storage.Create(500); return 500; } if (size == 0xd8) { uint token = _nextSlider++; Scene.Nodes.Add(token, new ScrollGumpNode()); return token; } throw new InvalidOperationException("문자 입력창 할당 크기"); }
    /// <summary>실제 파서/글리프가 읽을 독립ASCII 한 픽셀 글꼴 입력이다.</summary>
    private static FontStore Fonts()
    {
        byte[] widths = new byte[1024], mask = new byte[256];
        // 출력 가능한 원문BYTE에 독립 폭/실제 글리프를 공급한다.
        for (int code = 32; code < 127; code++) { GumpFontFixture.Put(widths, code * 4, code == 32 ? 3U : 2U); mask[code] = 1; }
        byte[] cache = GumpFontFixture.GlyphCache(widths, mask); widths.CopyTo(cache, 36); return new(path => path == "d/!Arial.normal.14.700.chfnt" ? cache : null, "Arial", (_, _, _, _) => throw new InvalidOperationException("문자 입력창 검사 글꼴 없음"));
    }
    /// <summary>원문 상태 계산을 대신하지 않고 실제 공통 처리기를 연결한다.</summary>
    public TextEntryWidgetScene()
    {
        Scene.Nodes.Add(30, Node); Storage = new(target => { Reads++; target.Clear(); Clipboard.AsSpan(0, Math.Min(Clipboard.Length, target.Length - 1)).CopyTo(target); }, source => { Writes++; Clipboard = source.ToArray(); }); State = new() { Brown = 9, Black = 0, Yellow = 5, SliderChange = Scene.State.ChangeEvent, SliderStep = Scene.State.StepEvent };
        Hooks = new() { Node = t => t == 30 ? Node : null, Console = t => t == 30 ? Node : null, Base = t => Scene.Nodes.GetValueOrDefault(t), Slider = t => Scene.Nodes.GetValueOrDefault(t) as ScrollGumpNode, Allocate = Allocate, Attach = (t, x, y, p, m) => Scene.TreeHooks.Attach!(t, x, y, p, m), Redraw = (t, _) => Scene.Redraws.Add(t), Now = () => Seconds, Recipient = value => value.SequenceEqual("A"u8), Complete = (callback, word, length, shift) => { CompletionWord = word?.ToArray(); CompletionLength = length; CompletionShift = shift; return Completion; }, Submit = (callback, text) => Submitted = text.ToArray(), Assertion = line => throw new InvalidOperationException("문자 입력창 원본 진단 " + line) };
        var assets = GumpBackgroundTestAssets.Assets(); var create = new GumpBackgroundCreateHooks { Node = t => Scene.Nodes.GetValueOrDefault(t) as GumpBackgroundNode }; var draw = new GumpBackgroundDrawHooks { Node = t => Scene.Nodes.GetValueOrDefault(t) as GumpBackgroundNode }; GumpBackgroundAssets.BindGumpBackgroundAssets(create, draw, assets); var backgrounds = new GumpBackgroundCreate(Scene.Create, Scene.Factory, () => 0, create); var pixels = new GumpPixelTarget(Scene.Pixels, 64, 128, Scene.Clip); var raster = new GumpBackgroundPixelTarget(pixels, assets, GumpBackgroundTestAssets.Maps(), new byte[256]); raster.Hooks(Scene.Clip, draw); var backgroundDraw = new GumpBackgroundDraw(Scene.Collection, Scene.Clip, new(), draw); var fonts = Fonts(); var text = new GumpTextDraw(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(pixels, new()));
        TextEntryGump.BindTextEntryGumpDevices(Hooks, Storage, backgrounds, backgroundDraw, Scene.Widgets, Scene.Slider, Scene.Tree, Scene.Lifetime, Scene.Events, Scene.Collection, new(fonts), text, pixels); Entry = new(State, Hooks); TextEntryGump.BindTextEntryGump(Scene.TreeHooks, Scene.EventHooks, Scene.InputHooks, Scene.FrameHooks, Scene.DrawHooks, Scene.LifetimeHooks, t => t == 30 ? Entry : null, Scene.LifetimeHooks.Release!); Frame = new(Scene.Create, Scene.LifeState, Scene.FrameHooks);
    }
    /// <summary>실제 Console 생성/두 Slider 자식 생성과 공통 가상Attach다.</summary>
    public void Construct(ReadOnlyMemory<byte>? initial = null) { Entry.ConstructConsole(30, 40, 28, initial, 0, 171, 31); Scene.TreeHooks.Attach!(30, 2, 3, 1, 0); Scene.Notices.Clear(); Scene.Redraws.Clear(); }
    /// <summary>원본Console/기본 분류를 포함한 공통 직접 입력 경로다.</summary>
    public uint Handle(uint code, uint first = 0, uint second = 0) => Scene.InputHooks.Handle!(30, code, first, second);
    /// <summary>같은 실제 표면을 초기화한 뒤 공통 가상Draw에 요청한다.</summary>
    public byte[] Draw() { Scene.Pixels.Span.Fill(0x5a); Scene.Draw.Draw(30); return Scene.Pixels.ToArray(); }
}
