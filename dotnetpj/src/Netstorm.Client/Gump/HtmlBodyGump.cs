using System.Text;

namespace Netstorm.Client.Gump; // 원본 HTML 부모의 두8192BYTE와 같은 실제 표시 자식을 소유한다.

/// <summary>원본0x40a4 부모의+a0 표시 자식/+a4 본문/+20a4 링크 영역.</summary>
public sealed class HtmlBodyGumpNode : GumpCreateNode
{
    /// <summary>두 원문 영역의 같은 물리 용량.</summary>
    public const int Capacity = 8192;
    /// <summary>현재 공통 수명이 소유하는 다중 줄 자식.</summary>
    public uint Text { get; set; }
    /// <summary>첫 NUL 뒤의 BYTE도 보존하는 실제 본문.</summary>
    public byte[] Body { get; } = new byte[Capacity];
    /// <summary>현재 링크의 같은8192BYTE 소유 영역.</summary>
    public byte[] Links { get; } = new byte[Capacity];
}
/// <summary>같은 원본 화면/종류/색 전역.</summary>
public sealed class HtmlBodyGumpState
{
    /// <summary>현재 signed 화면 폭.</summary>
    public int ScreenWidth { get; set; }
    /// <summary>현재 signed 화면 높이.</summary>
    public int ScreenHeight { get; set; }
    /// <summary>기본 command로 signed 확장할 원본54042f BYTE.</summary>
    public sbyte Kind { get; set; } = 20;
    /// <summary>실제 다중 줄 기본 색.</summary>
    public uint Color { get; set; }
}
/// <summary>같은 기본/파생·실제 다중 줄과 공통 트리/수명 장치.</summary>
public sealed class HtmlBodyGumpHooks
{
    /// <summary>현재 실제 부모.</summary>
    public Func<uint, HtmlBodyGumpNode?>? Node { get; set; }
    /// <summary>현재 같은 기본/자식 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>실제 기본 생성.</summary>
    public Action<uint, uint, float, float, uint>? ConstructBase { get; set; }
    /// <summary>원문 new0xd8의 실제 자식 할당.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>실제 다중 줄(0,0,5,color) 생성.</summary>
    public Action<uint, int, int, uint, uint>? ConstructText { get; set; }
    /// <summary>공통 자식 우선 지연 삭제.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>현재 파생 가상 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>자신의 기본 Gump 부착.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>실제 다중 줄 StyleText 입력.</summary>
    public Action<uint, string>? Styled { get; set; }
    /// <summary>실제 절대 스크롤 요청.</summary>
    public Action<uint, int>? Scroll { get; set; }
    /// <summary>실제 Slider 설정.</summary>
    public Action<uint, uint>? Slider { get; set; }
    /// <summary>실제 스크롤 허용 설정.</summary>
    public Action<uint, uint>? Scrollable { get; set; }
}
/// <summary>원본46ca70의 현재 파서 출력 포인터와 실제 순회를 공급한다.</summary>
public sealed class HtmlBodyParserHooks
{
    /// <summary>현재 파서+8이 비0인지 조회한다.</summary>
    public Func<bool>? HasOutput { get; set; }
    /// <summary>출력이 없을 때만 같은 두 소유 영역을 설치한다.</summary>
    public Action<Memory<byte>, Memory<byte>>? Bind { get; set; }
    /// <summary>실제 기본/전용 HTML 순회.</summary>
    public Action? Run { get; set; }
}
/// <summary>cpppj/src/client/HtmlBodyGump.h/.cpp의 현재 부모/자식 생성·갱신·배치를 옮긴다.</summary>
public sealed class HtmlBodyGump
{
    /// <summary>현재 화면/종류/색.</summary>
    private readonly HtmlBodyGumpState _state;
    /// <summary>같은 현재 저장소와 가상 장치.</summary>
    private readonly HtmlBodyGumpHooks _hooks;
    /// <summary>선택한 실제 효과만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("HTML 부모 실제 처리기 누락");
    /// <summary>현재 상태/장치를 빌리고 실제 분기 전에는 하위 장치를 요구하지 않는다.</summary>
    public HtmlBodyGump(HtmlBodyGumpState state, HtmlBodyGumpHooks hooks) { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); _state = state; _hooks = hooks; }
    /// <summary>같은 실제 부모 저장소가 없으면 원문 포인터 계약 오류다.</summary>
    private HtmlBodyGumpNode Node(uint token) => Require(_hooks.Node)(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "HTML 부모 노드 누락");
    /// <summary>현재 같은 기본/자식 필드를 읽는다.</summary>
    private GumpCreateNode Base(uint token) => Require(_hooks.Base)(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "HTML 부모/자식 기본 노드 누락");
    /// <summary>같은 x87 계산값에서 기본/배율 폭을 각각 저장한다.</summary>
    private static void Width(GumpCreateNode node, double value) { node.BaseWidth = (float)value; node.Width = (float)(value * node.Scale); }
    /// <summary>float32로 먼저 좁히지 않고 같은 계산값으로 두 높이를 저장한다.</summary>
    private static void Height(GumpCreateNode node, double value) { node.BaseHeight = (float)value; node.Height = (float)(value * node.Scale); }
    /// <summary>첫 두 NUL만 쓰고 나머지 BYTE/정수 나눗셈의 화면 절반 크기를 보존한다.</summary>
    public uint Construct(uint token) { Require(_hooks.ConstructBase)(token, 0, 0, 0, 1); var node = Node(token); node.Vtable = 0x50bd38; node.Command = unchecked((uint)_state.Kind); node.Text = 0; node.Body[0] = node.Links[0] = 0; Width(node, _state.ScreenWidth / 2); Height(node, _state.ScreenHeight / 2); return token; }
    /// <summary>현재 자식을 지연 Kill한 뒤 소유 필드0만 쓴다.</summary>
    public void Clear(uint token) { if (Node(token).Text != 0) { Require(_hooks.Kill)(Node(token).Text); Node(token).Text = 0; } }
    /// <summary>현재 외부 출력은 덮어쓰지 않고 두 첫 NUL→필요 시 Bind→실제 순회다.</summary>
    public void Parse(uint token, HtmlBodyParserHooks parser) { Node(token).Body[0] = Node(token).Links[0] = 0; if (!Require(parser.HasOutput)()) Require(parser.Bind)(Node(token).Body, Node(token).Links); Require(parser.Run)(); }
    /// <summary>이전 자식은 새 생성 성공 전까지 남기며 실제 표시/scroll/Slider/부모 재부착을 순서대로 실행한다.</summary>
    public void SetText(uint token, string text)
    {
        int end = text.IndexOf('\0'); if (end >= 0) text = text[..end]; if (Node(token).Text != 0) Require(_hooks.Kill)(Node(token).Text); uint child = Require(_hooks.Allocate)(0xd8); if (child == 0) throw new OutOfMemoryException("HTML 표시 자식 할당 실패"); Require(_hooks.ConstructText)(child, 0, 0, 5, _state.Color); Node(token).Text = child; Require(_hooks.Attach)(Node(token).Text, 0, 0, token, 0); Require(_hooks.Styled)(Node(token).Text, text); Require(_hooks.Scroll)(Node(token).Text, 0); Require(_hooks.Slider)(Node(token).Text, 1); Require(_hooks.Scrollable)(Node(token).Text, 1); if ((Node(token).Flags & 8) == 0) { Require(_hooks.Push)(token); Require(_hooks.Attach)(token, Node(token).X, Node(token).Y, 0, 0); }
    }
    /// <summary>같은8192BYTE 본문/링크를 실제 파서에 맡기고 그 뒤 현재 본문의 첫 NUL 앞을 표시한다.</summary>
    public void SetHtml(uint token, string source, Action<string, Memory<byte>, Memory<byte>> parse) { Node(token).Body[0] = Node(token).Links[0] = 0; Require(parse)(source, Node(token).Body, Node(token).Links); byte[] bytes = Node(token).Body; int end = Array.IndexOf(bytes, (byte)0); if (end < 0) throw new ArgumentException("내장 TELL 본문 실제8192BYTE NUL 범위 초과"); SetText(token, Encoding.Latin1.GetString(bytes, 0, end)); }
    /// <summary>가상 Push 뒤 현재 부모/자식을 다시 읽고 가로 여백을 뺀 원문 치수로 맞춘다.</summary>
    public void Fit(uint token, float inset) { if (Node(token).Text == 0) return; Require(_hooks.Push)(Node(token).Text); Width(Base(Node(token).Text), (double)Node(token).BaseWidth - inset); Height(Base(Node(token).Text), Node(token).BaseHeight); Require(_hooks.Attach)(Node(token).Text, inset, 0, token, 0); }
    /// <summary>기본 부착 효과 뒤 현재 표시 자식을 inset0으로 다시 맞춘다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode) { Require(_hooks.BaseAttach)(token, x, y, parent, mode); Fit(token, 0); }
    /// <summary>같은 실제 기본 생성/자식/본문/스크롤/수명만 연결한다.할당/현재 가상 Attach·Push는 상위가 공급한다.</summary>
    public static void BindHtmlBodyGumpDevices(HtmlBodyGumpHooks hooks, GumpCreate factory, GumpTree tree, GumpLifetime lifetime, ScrollingTextGumpWidgets widgets, ScrollingTextGump text, ScrollingTextGumpMarkup markup) { hooks.ConstructBase = (t, c, w, h, f) => factory.Construct(t, c, w, h, f); hooks.BaseAttach = tree.Attach; hooks.ConstructText = (t, w, h, font, color) => widgets.Construct(t, w, h, font, color); hooks.Kill = lifetime.Kill; hooks.Styled = (t, source) => markup.Parse(t, Encoding.Latin1.GetBytes(source)); hooks.Scroll = text.RequestScroll; hooks.Slider = widgets.SetSlider; hooks.Scrollable = widgets.SetScrollable; }
    /// <summary>현재 HTML 부모의 가상 Attach만 분배하고 다른 실제 파생은 보존한다.</summary>
    public static void BindHtmlBodyGump(GumpTreeHooks hooks, Func<uint, HtmlBodyGump?> resolve) { var previous = hooks.Attach; hooks.Attach = (t, x, y, p, m) => { if (resolve(t) is { } body) body.Attach(t, x, y, p, m); else Require(previous)(t, x, y, p, m); }; }
}
