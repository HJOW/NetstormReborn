using System.Text;

namespace Netstorm.Client.Gump; // 일반 HTML 창의 실제 장식/버튼/본문과 원시 명령 수명을 보존한다.

/// <summary>원본+20c4부터20개 행에 있는 실제 세 DWORD. 콜백 중에도 행 주소를 유지한다.</summary>
public sealed class HtmlWindowControl
{
    /// <summary>현재 실제 돌 버튼 토큰.</summary>
    public uint Child { get; set; }
    /// <summary>선택 시 큐에 쓰는 원시 명령.</summary>
    public uint Command { get; set; }
    /// <summary>선택 시 두 큐 인자에 똑같이 쓰는 값.</summary>
    public uint Parameter { get; set; }
}
/// <summary>원본0x21e8 HTML 창. 장식과 같은 기본 주소 뒤에8192BYTE/20행/기한을 둔다.</summary>
public class HtmlWindowGumpNode : GumpDecorationNode
{
    /// <summary>원본+c4의 링크 저장소. 생성자는 첫 BYTE만 초기화한다.</summary>
    public byte[] Html { get; } = new byte[8192];
    /// <summary>같은 물리20행. 현재 count와 무관하게 입력이 전체 행을 검색한다.</summary>
    public HtmlWindowControl[] Controls { get; } = [.. Enumerable.Range(0, 20).Select(_ => new HtmlWindowControl())];
    /// <summary>현재 signed 비교에 쓰는 버튼 개수.</summary>
    public uint ControlCount { get; set; }
    /// <summary>현재 ScrollingTextGump 본문.</summary>
    public uint Body { get; set; }
    /// <summary>원본+21bc의 현재 갱신 깊이.</summary>
    public uint UpdateDepth { get; set; }
    /// <summary>사용자가 요청한 원시 폭 DWORD.</summary>
    public uint RequestedWidth { get; set; }
    /// <summary>사용자가 요청한 원시 높이 DWORD.</summary>
    public uint RequestedHeight { get; set; }
    /// <summary>배치 때 현재 메뉴 스택에서 꺼낼 열 수.맵 메타가 원문 주소처럼 ref로 같은 필드를 빌린다.</summary>
    public uint MenuColumns;
    /// <summary>본문 갱신/소멸 때 현재 등록표로 조회하는 보조창 serial.</summary>
    public uint AuxiliarySerial { get; set; }
    /// <summary>0은 비활성인 원본 벽 시각 기한.</summary>
    public double TimeoutSeconds { get; set; }
    /// <summary>기한 만료 시 쓰는 명령. 0도 큐에 쓴다.</summary>
    public uint TimeoutCommand { get; set; }
    /// <summary>생성자가 덮지 않는 기한 명령 인자.</summary>
    public uint TimeoutParameter { get; set; }
    /// <summary>실제 소멸 시 count 감소 뒤 큐에 쓰는 명령.</summary>
    public uint OnExitCommand { get; set; }
    /// <summary>생성자가 덮지 않는 소멸 명령 인자.</summary>
    public uint OnExitParameter { get; set; }
}
/// <summary>Client 수명의 같은 HTML 창 개수/표식/벽 시각과 사건 전역.</summary>
public sealed class HtmlWindowGumpState
{
    /// <summary>생성/실제 소멸 때 감기는 원본005cab2c 개수.</summary>
    public uint Instances { get; set; }
    /// <summary>전체 생성기에서 쓰는 현재 창 serial.</summary>
    public uint CreatedSerial { get; set; }
    /// <summary>원본005cab34의 임시 serial.</summary>
    public uint TransientSerial { get; set; }
    /// <summary>원본005cad30의 마지막 닫힘 벽 시각.</summary>
    public double LastCloseTime { get; set; }
    /// <summary>기본 command에 signed 확장할 실제 BYTE.</summary>
    public sbyte Kind { get; set; } = 27;
    /// <summary>첫 버튼을 선택하는 원본 Enter 사건.</summary>
    public uint EnterEvent { get; set; } = 0x02000038;
    /// <summary>버튼 serial로 찾는 선택 사건.</summary>
    public uint ClickEvent { get; set; } = 0x0200002c;
    /// <summary>닫은 뒤 링크를 실행하는 태그 사건.</summary>
    public uint TagEvent { get; set; } = 0x02000025;
    /// <summary>현재 창만 닫는 원본 사건.</summary>
    public uint ClosedEvent { get; set; } = 0x020000be;
}
/// <summary>같은 현재 노드/하위 위젯과 공통 등록·트리·수명·원시 큐 장치.</summary>
public sealed class HtmlWindowGumpHooks
{
    /// <summary>현재 실제 HTML 파생.</summary>
    public Func<uint, HtmlWindowGumpNode?>? Node { get; set; }
    /// <summary>현재 같은 기본/자식 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 돌 버튼의 같은 기본 버튼 저장소.</summary>
    public Func<uint, ButtonGumpNode?>? Button { get; set; }
    /// <summary>실제 장식(0,0) 생성.</summary>
    public Action<uint>? ConstructDecoration { get; set; }
    /// <summary>실제 기본 소멸. 객체 free와 구별한다.</summary>
    public Action<uint>? DestructBase { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>실제 자식 우선 지연 삭제.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>원본0x170 돌 버튼/0xd8 본문 할당.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>현재 문맥/색으로 실제 돌 버튼 생성.</summary>
    public Action<uint, string>? ConstructButton { get; set; }
    /// <summary>실제 다중 줄(0,0,0,color) 생성.</summary>
    public Action<uint>? ConstructBody { get; set; }
    /// <summary>실제 StyleText 다중 줄 파서.</summary>
    public Action<uint, string>? ParseBody { get; set; }
    /// <summary>실제 본문의 스크롤 허용 설정.</summary>
    public Action<uint, int>? Scrollable { get; set; }
    /// <summary>실제 파생 가상 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 공통 등록표에서 serial 조회.</summary>
    public Func<uint, uint>? Reference { get; set; }
    /// <summary>현재 벽 시각.</summary>
    public Func<double>? WallSeconds { get; set; }
    /// <summary>세 DWORD를 그대로 쓰는 원시 입력 큐.</summary>
    public Action<uint, uint, uint>? Queue { get; set; }
    /// <summary>죽음 표시 뒤 실제 링크 주소를 소비한다.</summary>
    public Action<uint>? Link { get; set; }
    /// <summary>선택하지 않은 사건의 기본 처리.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>scalar 소멸 뒤 객체 토큰 반납.</summary>
    public Action<uint>? Release { get; set; }
}
/// <summary>cpppj/src/client/HtmlWindowGump.h/.cpp의 생성·버튼·본문·입력·소멸을 같은 현재 저장소에서 수행한다.</summary>
public sealed class HtmlWindowGump
{
    /// <summary>원본10.78의 HTML 창 가상 표.</summary>
    private const uint Vtable = 0x515e7c;
    /// <summary>Client와 같은 장기 전역.</summary>
    private readonly HtmlWindowGumpState _state;
    /// <summary>같은 실제 저장소/장치를 빌린다.</summary>
    private readonly HtmlWindowGumpHooks _hooks;
    /// <summary>실제 도달한 효과만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("HTML 창 실제 장치 미연결");
    /// <summary>기본/파생 조회만 미리 요구하고 선택 장치는 해당 분기에서 검증한다.</summary>
    public HtmlWindowGump(HtmlWindowGumpState state, HtmlWindowGumpHooks hooks) { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _state = state; _hooks = hooks; }
    /// <summary>기본과 파생이 같은 현재 객체여야 한다.</summary>
    private HtmlWindowGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("HTML 창 같은 실제 노드 없음"); return n; }
    /// <summary>현재 실제 본문/버튼 기본 필드를 읽는다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("HTML 창 실제 자식 없음");
    /// <summary>원문 유효20행 밖 물리 접근을 명시 거부한다.</summary>
    private static int Slot(uint index) => index < 20 ? (int)index : throw new IndexOutOfRangeException("HTML 창 실제20행 밖 접근");
    /// <summary>장식 뒤20행/선택 필드만 초기화한다. 두 명령 인자와 첫 NUL 뒤는 보존한다.</summary>
    public uint Construct(uint token, uint width, uint height)
    {
        Node(token); Require(_hooks.ConstructDecoration)(token); var n = Node(token); n.Vtable = Vtable;
        // 원본20행의 세 DWORD만 초기화하고 행 객체의 주소는 유지한다.
        foreach (var e in n.Controls) { e.Child = e.Command = e.Parameter = 0; }
        n.TimeoutSeconds = 0; n.Command = unchecked((uint)(int)_state.Kind); n.ControlCount = 0; n.Body = 0; n.AuxiliarySerial = 0; n.Html[0] = 0; n.UpdateDepth = 0; n.MenuColumns = 0; n.TimeoutCommand = 0; n.OnExitCommand = 0; n.RequestedWidth = width; n.RequestedHeight = height; _state.Instances = unchecked(_state.Instances + 1); return token;
    }
    /// <summary>count 감소→현재 OnExit 큐→현재 보조창 Kill→벽 시각→기본 소멸 순서다.</summary>
    public void Destruct(uint token) { Node(token).Vtable = Vtable; _state.Instances = unchecked(_state.Instances - 1); if (Node(token).OnExitCommand != 0) { uint command = Node(token).OnExitCommand, parameter = Node(token).OnExitParameter; Require(_hooks.Queue)(command, parameter, parameter); } uint auxiliary = Require(_hooks.Reference)(Node(token).AuxiliarySerial); if (auxiliary != 0) Require(_hooks.Kill)(auxiliary); _state.LastCloseTime = Require(_hooks.WallSeconds)(); Require(_hooks.DestructBase)(token); }
    /// <summary>매 반복 현재 signed count를 다시 읽으며 Kill 뒤 같은 행을0으로 쓴다.</summary>
    public void ClearButtons(uint token)
    {
        // 콜백이 변경한 현재 count를 다음 조건에서 다시 읽는다.
        for (uint i = 0; unchecked((int)i) < unchecked((int)Node(token).ControlCount); i++) { var e = Node(token).Controls[Slot(i)]; if (e.Child != 0) { Require(_hooks.Kill)(e.Child); e.Child = 0; } e.Command = e.Parameter = 0; }
        Node(token).ControlCount = 0;
    }
    /// <summary>현재20행 한도→할당/생성→현재 행 저장/가상 부착→인자/명령→Instant→count 증가다.</summary>
    public void AddButton(uint token, string label, uint command, uint parameter, uint instant)
    {
        if (unchecked((int)Node(token).ControlCount) >= 20) return; Slot(Node(token).ControlCount); uint child = Require(_hooks.Allocate)(0x170); if (child != 0) Require(_hooks.ConstructButton)(child, label); var e = Node(token).Controls[Slot(Node(token).ControlCount)]; e.Child = child; Require(_hooks.Attach)(child, 0, 0, token, 0); e.Parameter = parameter; e.Command = command; if (instant != 0) { var button = Require(_hooks.Button)(child) ?? throw new InvalidOperationException("HTML 실제 돌 버튼 없음"); button.ButtonFlags |= 4; } Node(token).ControlCount = unchecked(Node(token).ControlCount + 1);
    }
    /// <summary>끝 LF가 없는 입력만1008BYTE 지역 버퍼를 쓰고 실제 본문/보조창을 바꾼 뒤 필요 시 전체 Fit을 부른다.</summary>
    public void SetBody(uint token, string text)
    {
        int end = text.IndexOf('\0'); if (end >= 0) text = text[..end]; if (text.Length == 0 || text[^1] != '\n') { if (text.Length > 1006) throw new ArgumentException("HTML 원문 지역 LF 버퍼 범위 초과"); text += '\n'; }
        if (Node(token).Body != 0) { Require(_hooks.Kill)(Node(token).Body); Node(token).Body = 0; } uint child = Require(_hooks.Allocate)(0xd8); if (child != 0) Require(_hooks.ConstructBody)(child); Node(token).Body = child; Base(child).SecondaryDepth = uint.MaxValue; Require(_hooks.Scrollable)(Node(token).Body, 0); Require(_hooks.ParseBody)(Node(token).Body, text); Require(_hooks.Attach)(Node(token).Body, 0, 0, token, 0); uint auxiliary = Require(_hooks.Reference)(Node(token).AuxiliarySerial); if (auxiliary != 0) Require(_hooks.Kill)(auxiliary); if ((Node(token).Flags & 8) == 0) { Require(_hooks.Push)(token); Require(_hooks.Attach)(token, Node(token).X, Node(token).Y, 0, 0); }
    }
    /// <summary>명령/인자를 큐 전에 캡처하고 부모 Kill 뒤에는 필드를 읽지 않는다.</summary>
    private uint Fire(uint token, HtmlWindowControl entry) { uint command = entry.Command, parameter = entry.Parameter; Require(_hooks.Queue)(command, parameter, parameter); Require(_hooks.Kill)(token); return 1; }
    /// <summary>release 비트도 허용하며 첫 버튼/전체20serial/태그/닫힘 외 사건은 기본에 위임한다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second)
    {
        uint masked = code & 0x03ffffff; if (masked == _state.EnterEvent && Node(token).Controls[0].Child != 0) return Fire(token, Node(token).Controls[0]);
        if (masked == _state.ClickEvent) {
            // count와 무관한 원문20개 물리 행을 순서대로 검색한다.
            foreach (var e in Node(token).Controls) if (e.Child != 0 && first == Base(e.Child).Serial) return Fire(token, e);
        } else if (masked == _state.TagEvent) { if (second != 0) { Require(_hooks.Kill)(token); Require(_hooks.Link)(second); } return 1; } else if (masked == _state.ClosedEvent) { Require(_hooks.Kill)(token); return 1; } return Require(_hooks.BaseHandle)(token, code, first, second);
    }
    /// <summary>기한0만 즉시 거부하고 시계 효과 뒤 현재 기한/명령을 다시 읽는다. NaN은 만료되지 않는다.</summary>
    public void Idle(uint token) { if (Node(token).TimeoutSeconds == 0) return; double now = Require(_hooks.WallSeconds)(); if (now >= Node(token).TimeoutSeconds) { uint command = Node(token).TimeoutCommand, parameter = Node(token).TimeoutParameter; Require(_hooks.Queue)(command, parameter, parameter); Require(_hooks.Kill)(token); } }
    /// <summary>실제 파생/기본 소멸 뒤 하위 BYTE bit0이 켜지면 객체를 반납한다.</summary>
    public uint DeleteScalar(uint token, uint flags) { Destruct(token); if ((flags & 1) != 0) Require(_hooks.Release)(token); return token; }
    /// <summary>기존 실제 장식/본문/공통 수명 장치를 연결한다. 현재 색/할당/가상 부착은 상위가 공급한다.</summary>
    public static void BindHtmlWindowDevices(HtmlWindowGumpHooks hooks, GumpDecoration decoration, GumpTree tree, GumpLifetime lifetime, GumpLifetimeHooks life, ScrollingTextGumpWidgets widgets, ScrollingTextGumpMarkup markup, Func<uint> bodyColor) { hooks.ConstructDecoration = t => decoration.Construct(t, 0, 0); hooks.DestructBase = lifetime.DestructBase; hooks.Kill = lifetime.Kill; hooks.Push = tree.Push; hooks.Release = t => Require(life.Release)(t); hooks.ConstructBody = t => widgets.Construct(t, 0, 0, 0, bodyColor()); hooks.Scrollable = (t, v) => widgets.SetScrollable(t, unchecked((uint)v)); hooks.ParseBody = (t, text) => markup.Parse(t, Encoding.Latin1.GetBytes(text)); }
    /// <summary>현재 팔레트 색과 실제 돌 버튼(0,0,label,color)의 생성 경계.</summary>
    public static void BindHtmlWindowButtons(HtmlWindowGumpHooks hooks, StoneButtonGump buttons, Func<uint> color) { hooks.ConstructButton = (t, label) => buttons.Construct(t, 0, 0, Encoding.Latin1.GetBytes(label + '\0'), color()); }
    /// <summary>현재 HTML 창의 사건/입력/Idle/scalar를 분배하고 기존 다른 파생 처리기를 보존한다.</summary>
    public static void BindHtmlWindow(GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, GumpLifetimeHooks lifetime, Func<uint, HtmlWindowGump?> resolve)
    { var oldEvent = events.Handle; var oldInput = input.Handle; var oldIdle = frame.Idle; var oldDelete = lifetime.DeletingDestructor; events.Handle = (t, e, a, b) => resolve(t) is { } h ? h.Handle(t, e, a, b) : Require(oldEvent)(t, e, a, b); input.Handle = (t, e, a, b) => resolve(t) is { } h ? h.Handle(t, e, a, b) : Require(oldInput)(t, e, a, b); frame.Idle = t => { if (resolve(t) is { } h) h.Idle(t); else Require(oldIdle)(t); }; lifetime.DeletingDestructor = (t, f) => { if (resolve(t) is { } h) h.DeleteScalar(t, f); else Require(oldDelete)(t, f); }; }
}
