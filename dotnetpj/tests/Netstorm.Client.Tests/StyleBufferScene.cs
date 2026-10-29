using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 같은 글꼴/조각/버퍼/노드/픽셀을 연결하는 검사 입력이다.

/// <summary>원본 실행 기대값을 생성하지 않는 헤드리스 입력 장면. 고정 기대값은 각 검사에 둔다.</summary>
internal sealed class StyleBufferScene
{
    /// <summary>공통 소유 조각 표.</summary>
    public StyleTextState Fragments { get; } = new();
    /// <summary>현재 실제 글자/파생 경계.</summary>
    public StyleTextHooks StyleHooks { get; } = new();
    /// <summary>같은 실제 조각 처리기.</summary>
    public StyleText Style { get; }
    /// <summary>버퍼 상태.</summary>
    public StyleTextBufferState State { get; } = new();
    /// <summary>실제 글꼴/파생 복제 경계.</summary>
    public StyleTextBufferHooks BufferHooks { get; } = new();
    /// <summary>같은 실제 버퍼.</summary>
    public StyleTextBuffer Buffer { get; }
    /// <summary>기본 생성/판본 전역.</summary>
    public GumpCreateState Create { get; } = new();
    /// <summary>같은 다중 줄 파생/기본 노드.</summary>
    public ScrollingTextGumpNode Node { get; } = new();
    /// <summary>슬라이더 기본 폭을 공급하는 안정된 노드.</summary>
    public GumpCreateNode Slider { get; } = new() { BaseWidth = 2 };
    /// <summary>현재 원본 슬라이더/재그리기 경계.</summary>
    public ScrollingTextGumpHooks ScrollHooks { get; } = new();
    /// <summary>같은 표시 처리기.</summary>
    public ScrollingTextGump Scroll { get; }
    /// <summary>공유 논리/VFX 클립.</summary>
    public GumpDrawState Clip { get; } = new();
    /// <summary>앞뒤 guard가 있는 픽셀 할당.</summary>
    public byte[] Guarded { get; } = Enumerable.Repeat((byte)7, 1282).ToArray();
    /// <summary>32×40 실제 표면.</summary>
    public Memory<byte> Pixels => Guarded.AsMemory(1, 1280);
    /// <summary>글자와 그림이 공유하는 실제 색표.</summary>
    public GumpGlyphColorState Colors { get; } = new();
    /// <summary>공통 가상 Draw.</summary>
    public GumpDraw Draw { get; }
    /// <summary>가상 통지 관측 목록.</summary>
    public List<string> Events { get; } = [];
    /// <summary>독립 입력의 기본 문맥.</summary>
    public uint[] Context { get; } = [0, 0, 171, 0, 0, 0, 1, 1];
    /// <summary>현재 같은 실제 노드만 조회한다.</summary>
    private GumpCreateNode? Find(uint token) => token == 2 ? Node : token == 3 ? Slider : null;
    /// <summary>창 없이 실제 글꼴/픽셀/버퍼/기본 생성/표시를 연결한다.</summary>
    public StyleBufferScene(OriginalEdition edition = OriginalEdition.Patch1078, int maxLines = 8)
    {
        Create.Collection.Edition = edition; var factory = new GumpCreate(Create, new() { Node = Find }); factory.Construct(2, 0, 12, 30, 1); Node.X = 2; Node.Y = 1; Node.Buffer = 10; Node.Slider = 3; Context.CopyTo(Node.TextContext, 0); var collection = new GumpCollection(Create.Collection, factory.CollectionHooks());
        GumpDraw.SetGumpDrawClip(Clip, new(0, 0, 32, 40)); var fonts = StyleTextTestSupport.PixelFonts(); StyleText.BindStyleTextFonts(StyleHooks, new(fonts, edition), new(fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(new(Pixels, 32, 40, Clip), Colors), edition)); Style = new(Fragments, StyleHooks, edition);
        StyleTextBuffer.BindStyleTextBufferFonts(BufferHooks, new(fonts, edition)); Buffer = new(State, Fragments, Style, maxLines, BufferHooks);
        ScrollHooks.Node = t => t == 2 ? Node : null; ScrollHooks.BaseNode = Find; ScrollHooks.Buffer = t => t == 10 ? new(State, Buffer) : default; ScrollHooks.Redraw = (t, erase) => Events.Add($"redraw:{t},{erase}");
        ScrollHooks.SliderRange = (t, min, max) => Events.Add($"range:{t},{min},{max}"); ScrollHooks.SliderPage = (t, page) => Events.Add($"page:{t},{page}"); ScrollHooks.SliderSetPosition = (t, p) => Events.Add($"set:{t},{p}"); ScrollHooks.SliderPosition = t => { Events.Add($"get:{t}"); return Node.Scroll; };
        Scroll = new(Create, collection, Clip, Fragments, Style, ScrollHooks); var draw = new GumpDrawHooks { Draw = t => Events.Add($"fallback:{t}") }; TextGump.BindTextGumpDraw(draw, _ => null, t => t == 2 ? Scroll : null); Draw = new(collection, Clip, draw);
    }
    /// <summary>실제 링크를 관측해 배열로 반환한다. 예상 배치는 계산하지 않는다.</summary>
    public StyleTextBufferLine[] Rows()
    {
        var rows = new List<StyleTextBufferLine>(); var seen = new HashSet<uint>(); uint t = State.Sentinel.Next;
        // 현재 양방향 목록의 순방향 링크만 관측한다.
        while (t != 0) { Assert.True(seen.Add(t)); rows.Add(State.SoftLines[t]); t = State.SoftLines[t].Next; } return rows.ToArray();
    }
    /// <summary>같은 조각 표에서 해당 줄의 현재 문자열만 관측한다.</summary>
    public string[] Texts(uint head) => StyleTextTestSupport.Chain(Fragments, head).Select(t => StyleTextTestSupport.Text(Fragments.Fragments[t])).ToArray();
}
