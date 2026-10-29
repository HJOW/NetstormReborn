using System.Text;

namespace Netstorm.Client.Gump; // 실제 전용 도움말의 모든 자식 생성/배치와 본문/그림을 같은 공통 UI로 조립한다.

/// <summary>원본0xd0 창의 같은 기본/입력 필드 뒤 남은12DWORD.</summary>
public sealed class QuickHelpContentNode : QuickHelpWindowNode
{
    /// <summary>현재 ImageGump 자식.</summary>
    public uint Picture { get; set; }
    /// <summary>현재 장식 자식.</summary>
    public uint Decoration { get; set; }
    /// <summary>현재 정보 다중 줄 자식.</summary>
    public uint Information { get; set; }
    /// <summary>실제 두8192BYTE를 소유하는 HTML 부모.</summary>
    public uint Body { get; set; }
    /// <summary>현재 파일 기록 스트림.</summary>
    public uint Stream { get; set; }
    /// <summary>실제 안쪽 단색 배경.</summary>
    public uint InnerBackground { get; set; }
    /// <summary>현재 그림 질감 배경.</summary>
    public uint PictureBackground { get; set; }
    /// <summary>현재 본문 질감 배경.</summary>
    public uint BodyBackground { get; set; }
}
/// <summary>창과 별개 수명의 원본 캐시/마지막 표식·현재 생성/표시 전역.</summary>
public sealed class QuickHelpWindowState
{
    /// <summary>원본005ca990 현재 등록 serial.</summary>
    public uint Id { get; set; }
    /// <summary>null과 빈 성공 파일을 구별하는 실제 도움말 캐시.성공 뒤 LF가 붙는다.</summary>
    public string? HelpData { get; set; }
    /// <summary>기본 command에 signed 확장하는 원본54042e BYTE.</summary>
    public sbyte Kind { get; set; } = 19;
    /// <summary>현재 다중 줄 색.</summary>
    public uint Color { get; set; }
    /// <summary>돌 버튼의 실제 unused 인자.</summary>
    public uint ButtonUnused { get; set; }
    /// <summary>현재 스크립트 복귀 명령.</summary>
    public uint ScriptCommand { get; set; } = 0x02000095;
    /// <summary>첫 NUL 앞만 쓰는 현재 스크립트 이름.</summary>
    public string ScriptName { get; set; } = "";
    /// <summary>기본 주제 선택의 우선 전투 모드.</summary>
    public bool Battle { get; set; }
    /// <summary>전투 모드가 아닐 때의 도전 모드.</summary>
    public bool Challenge { get; set; }
    /// <summary>전용 HTML과 같은00565ec4 저장소를 공유한다.</summary>
    public QuickHelpHtmlState HtmlMode { get; } = new();
    /// <summary>본문 렌더링/정보 해석이 읽고 쓰는 동일 현재 모드.</summary>
    public uint InfoMode { get => HtmlMode.InfoMode; set => HtmlMode.InfoMode = value; }
    /// <summary>그림을 제외하는 첫 현재 TYPE.</summary>
    public uint NoPictureA { get; set; }
    /// <summary>그림을 제외하는 둘째 현재 TYPE.</summary>
    public uint NoPictureB { get; set; }
    /// <summary>각 질감 자식 생성 때 다시 읽는 현재 배경 TYPE.</summary>
    public uint BackgroundType { get; set; }
    /// <summary>각 질감 자식 생성 때 다시 읽는 현재 배경 프레임.</summary>
    public uint BackgroundFrame { get; set; }
}
/// <summary>기존 실제 자식/UI/Config/VFS와 같은 현재 가상 장치.</summary>
public sealed class QuickHelpWindowHooks
{
    /// <summary>현재 동일 전용 파생.</summary>
    public Func<uint, QuickHelpContentNode?>? Node { get; set; }
    /// <summary>같은 기본/모든 자식의 현재 필드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>같은 HTML 부모/두8192BYTE.</summary>
    public Func<uint, HtmlBodyGumpNode?>? Html { get; set; }
    /// <summary>실제 기본 Gump 생성.</summary>
    public Action<uint, uint, float, float, uint>? ConstructBase { get; set; }
    /// <summary>각 원문 크기에 맞는 파생 저장소 할당.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>실제 장식(0,0) 생성.</summary>
    public Action<uint, float, float>? ConstructDecoration { get; set; }
    /// <summary>실제 배경의 원래 여섯 인자.</summary>
    public Action<uint, float, float, uint, uint, uint, uint>? ConstructBackground { get; set; }
    /// <summary>실제 HTML 부모 생성.</summary>
    public Action<uint>? ConstructHtml { get; set; }
    /// <summary>실제 돌 버튼의 원래 다섯 인자.</summary>
    public Action<uint, int, int, string, uint, uint>? ConstructButton { get; set; }
    /// <summary>실제 돌 버튼 글자 맞춤.</summary>
    public Action<uint>? FitButton { get; set; }
    /// <summary>실제 정보 다중 줄 생성.</summary>
    public Action<uint, int, int, uint, uint>? ConstructInformation { get; set; }
    /// <summary>실제 ImageGump(type,frame,8) 생성.</summary>
    public Action<uint, uint, uint, uint>? ConstructPicture { get; set; }
    /// <summary>현재 실제 번역.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>현재 파일 키 등 설정 조회.</summary>
    public Func<string, string>? Config { get; set; }
    /// <summary>현재8192BYTE 본문 치환.</summary>
    public Func<string, string>? Substitute { get; set; }
    /// <summary>실제 파일의 raw BYTE 문자열.실패null/빈 성공을 구별한다.</summary>
    public Func<string, string?>? Load { get; set; }
    /// <summary>원본 등급9 파일 실패 진단.</summary>
    public Action<string>? LoadError { get; set; }
    /// <summary>현재 팔레트에서 RGB(55,51,54)를 조회한다.</summary>
    public Func<int, int, int, uint>? NearestColor { get; set; }
    /// <summary>현재 스크립트의 양의 깊이 여부.</summary>
    public Func<bool>? Scripted { get; set; }
    /// <summary>실제 자식 우선 지연 삭제.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>같은 현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>실제 HTML 표시 자식 정리.</summary>
    public Action<uint>? ClearBody { get; set; }
    /// <summary>현재 자식의 가상 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>자신 기본 부착.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>현재 정보 버퍼의 완료 줄 수.</summary>
    public Func<uint, int>? Lines { get; set; }
    /// <summary>같은 본문/링크와 정보 pane/stream의 실제 전용 HTML 해석.</summary>
    public Action<uint, uint, string, uint, uint>? Render { get; set; }
    /// <summary>치환 뒤 읽는 현재 TYPE+120.타입 없음은null이다.</summary>
    public Func<uint, int?>? HelpFrame { get; set; }
    /// <summary>같은 HTML 부모의 실제 다중 줄 본문 교체.</summary>
    public Action<uint, string>? BodyText { get; set; }
}
/// <summary>cpppj/src/client/QuickHelpWindow.h/.cpp의4c5380/58a0/5970/5d60 전체 몸체.</summary>
public sealed class QuickHelpWindow
{
    /// <summary>현재 캐시/생성/표시 전역.</summary>
    private readonly QuickHelpWindowState _state;
    /// <summary>지식 카드/Back과 동일한 공용 마지막 popup.</summary>
    private readonly QuickHelpPopupState _popup;
    /// <summary>같은 실제 GUI/Config/VFS 효과.</summary>
    private readonly QuickHelpWindowHooks _hooks;
    /// <summary>실제로 선택한 하위 장치만 요구한다.</summary>
    private static T Require<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Quick Help 전용 창 실제 효과 누락");
    /// <summary>같은 상태/저장소 수명을 빌리고 별도 근사 창을 만들지 않는다.</summary>
    public QuickHelpWindow(QuickHelpWindowState state, QuickHelpPopupState popup, QuickHelpWindowHooks hooks) { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(popup); ArgumentNullException.ThrowIfNull(hooks); _state = state; _popup = popup; _hooks = hooks; }
    /// <summary>첫 NUL 뒤를 파일명/주제로 해석하지 않는다.</summary>
    private static string CText(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>현재 전용 파생을 같은 저장소에서 읽는다.</summary>
    private QuickHelpContentNode Node(uint token) => Require(_hooks.Node)(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "전용 Help 창 노드 누락");
    /// <summary>현재 모든 실제 기본/자식을 읽는다.</summary>
    private GumpCreateNode Base(uint token) => Require(_hooks.Base)(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "전용 Help 현재 자식 누락");
    /// <summary>현재 body 토큰의 같은 두 소유 배열을 읽는다.</summary>
    private HtmlBodyGumpNode Body(uint token) => Require(_hooks.Html)(Node(token).Body) ?? throw new ArgumentOutOfRangeException(nameof(token), "전용 Help HTML 부모 누락");
    /// <summary>원문 new 실패 앞의 부분 생성 효과를 되돌리지 않는다.</summary>
    private uint Allocate(int bytes) { uint token = Require(_hooks.Allocate)(bytes); return token != 0 ? token : throw new OutOfMemoryException("전용 Help 파생 할당 실패"); }
    /// <summary>동일 x87 계산값에서 기본/배율 폭을 각각 저장한다.</summary>
    private void Width(uint token, double value) { var node = Base(token); node.BaseWidth = (float)value; node.Width = (float)(value * node.Scale); }
    /// <summary>각 원문 FST 시점에만 높이를 단정도로 좁힌다.</summary>
    private void Height(uint token, double value) { var node = Base(token); node.BaseHeight = (float)value; node.Height = (float)(value * node.Scale); }
    /// <summary>원문 strict>이며 동률/NaN은 둘째 값을 선택한다.</summary>
    private static double Larger(double first, double second) => first > second ? first : second;
    /// <summary>세 배경/HTML 부모·OK 또는Sacrifice/Back/선택Cancel·캐시/등록 serial을 원문 순서로 만든다.</summary>
    public uint Construct(uint token, uint learnType)
    {
        Require(_hooks.ConstructBase)(token, 0, 0, 0, 1); Node(token).Vtable = 0x514ec4; Node(token).LearnType = learnType; Node(token).Stream = 0; if (Require(_hooks.Scripted)()) _popup.Mark(_state.ScriptCommand, CText(_state.ScriptName));
        uint decoration = Allocate(0xc4); Require(_hooks.ConstructDecoration)(decoration, 0, 0); Node(token).Decoration = decoration; Base(Node(token).Decoration).AlignFlags |= 3; Base(Node(token).Decoration).SecondaryDepth = 1; Require(_hooks.Attach)(Node(token).Decoration, 0, 0, token, 0);
        uint color = Require(_hooks.NearestColor)(55, 51, 54), inner = Allocate(0xb8); Require(_hooks.ConstructBackground)(inner, 0, 0, 0x04000000 | color, 1, 0, 0); Node(token).InnerBackground = inner; Require(_hooks.Attach)(Node(token).InnerBackground, 0, 0, token, 0);
        uint pictureBackground = Allocate(0xb8); Require(_hooks.ConstructBackground)(pictureBackground, 0, 0, 0x03e40000, 1, _state.BackgroundType, _state.BackgroundFrame); Node(token).PictureBackground = pictureBackground; Require(_hooks.Attach)(Node(token).PictureBackground, 0, 0, token, 0); uint bodyBackground = Allocate(0xb8); Require(_hooks.ConstructBackground)(bodyBackground, 0, 0, 0x03e40000, 1, _state.BackgroundType, _state.BackgroundFrame); Node(token).BodyBackground = bodyBackground; Require(_hooks.Attach)(Node(token).BodyBackground, 0, 0, token, 0); Node(token).Information = 0;
        uint body = Allocate(0x40a4); Require(_hooks.ConstructHtml)(body); Node(token).Body = body; Require(_hooks.Attach)(Node(token).Body, 0, 0, token, 0);
        uint learn = Allocate(0x170); string label = Require(_hooks.Translate)(learnType != 0 ? "Sacrifice" : "OK"); Require(_hooks.ConstructButton)(learn, 0, 0, CText(label), _state.ButtonUnused, 0); Node(token).LearnButton = learn; Require(_hooks.FitButton)(Node(token).LearnButton); Width(Node(token).LearnButton, (double)Base(Node(token).LearnButton).BaseWidth * 1.2); Require(_hooks.Attach)(Node(token).LearnButton, 0, 0, token, 0);
        uint back = Allocate(0x170); string backLabel = Require(_hooks.Translate)("Back"); Require(_hooks.ConstructButton)(back, 0, 0, CText(backLabel), _state.ButtonUnused, 0); Node(token).BackButton = back; Require(_hooks.FitButton)(Node(token).BackButton); Require(_hooks.Attach)(Node(token).BackButton, 0, 0, token, 0); Node(token).CancelButton = 0;
        if (Node(token).LearnType != 0) { uint cancel = Allocate(0x170); string cancelLabel = Require(_hooks.Translate)("Cancel"); Require(_hooks.ConstructButton)(cancel, 0, 0, CText(cancelLabel), _state.ButtonUnused, 0); Node(token).CancelButton = cancel; Require(_hooks.FitButton)(Node(token).CancelButton); Require(_hooks.Attach)(Node(token).CancelButton, 0, 0, token, 0); }
        Node(token).Command = unchecked((uint)_state.Kind); if (_state.HelpData is null) { string name = CText(Require(_hooks.Config)("Quick Help Spec")); if (name.Length >= 256) throw new ArgumentException("Quick Help 원본 파일명 버퍼 초과"); string? data = Require(_hooks.Load)(name); if (data is not null) _state.HelpData = data + '\n'; else Require(_hooks.LoadError)(name); } _state.Id = Node(token).Serial; Width(token, 450); Height(token, 350); Node(token).Flags |= 0x800; Node(token).Picture = 0; return token;
    }
    /// <summary>그림/본문 표시/이전 정보 pane을 지연 반납하고 실제 새 정보 pane을 만든다.</summary>
    public void Reset(uint token) { if (Node(token).Picture != 0) { Require(_hooks.Kill)(Node(token).Picture); Node(token).Picture = 0; } Require(_hooks.ClearBody)(Node(token).Body); if (Node(token).Information != 0) Require(_hooks.Kill)(Node(token).Information); uint pane = Allocate(0xd8); Require(_hooks.ConstructInformation)(pane, 0, 0, 5, _state.Color); Node(token).Information = pane; Require(_hooks.Attach)(Node(token).Information, 0, 0, token, 0); }
    /// <summary>그림→버튼→본문 질감/안쪽 배경→HTML 부모→현재 정보 pane 순서로 재배치한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        Require(_hooks.BaseAttach)(token, x, y, parent, mode); if (Node(token).Picture != 0) { Require(_hooks.Push)(Node(token).PictureBackground); Width(Node(token).PictureBackground, 157); Height(Node(token).PictureBackground, 126); Require(_hooks.Attach)(Node(token).PictureBackground, 15, 17, token, 0); Require(_hooks.Push)(Node(token).Picture); Width(Node(token).Picture, 153); Height(Node(token).Picture, 122); Require(_hooks.Attach)(Node(token).Picture, 2, 2, Node(token).PictureBackground, 0); }
        int height = GumpCollection.Ftol(Base(Node(token).LearnButton).BaseHeight);
        // 선택 버튼0의 폭만0이며 다른 값은 현재 같은 기본 필드에서 읽는다.
        double ButtonWidth(uint child) => child == 0 ? 0 : Base(child).BaseWidth;
        int widest = GumpCollection.Ftol(Larger(ButtonWidth(Node(token).BackButton), Larger(ButtonWidth(Node(token).LearnButton), ButtonWidth(Node(token).CancelButton)))), count = 0;
        // 원문 초기 목록의 Back/Learn/Cancel 순서로 폭을 통일한다.
        foreach (uint child in new[] { Node(token).BackButton, Node(token).LearnButton, Node(token).CancelButton }) if (child != 0) { Width(child, widest); count++; }
        int offset = GumpCollection.Ftol(((double)Node(token).BaseWidth - (double)count * widest - (double)count * 10) * .5);
        // 각 Push 뒤 현재 창 높이/현재 버튼 토큰을 다시 읽는다.
        for (int index = 0; index < 3; index++)
        {
            // 가상 효과가 바꾼 같은 현재 자식 필드를 읽는다.
            uint Current() => index == 0 ? Node(token).BackButton : index == 1 ? Node(token).LearnButton : Node(token).CancelButton;
            if (Current() == 0) continue; Require(_hooks.Push)(Current()); int bottom = unchecked(GumpCollection.Ftol(Node(token).BaseHeight) - height - 10); Require(_hooks.Attach)(Current(), offset, bottom, token, 0); if (index != 2) offset = unchecked(offset + 10 + widest);
        }
        Require(_hooks.Push)(Node(token).BodyBackground); Width(Node(token).BodyBackground, ((double)Node(token).BaseWidth - 15) - 15); double top = Node(token).Picture != 0 ? 153 : 17; Height(Node(token).BodyBackground, ((((double)Node(token).BaseHeight - top) - 10) - 10) - height); Require(_hooks.Attach)(Node(token).BodyBackground, 17, (float)top, token, 0);
        Require(_hooks.Push)(Node(token).InnerBackground); Width(Node(token).InnerBackground, ((double)Base(Node(token).BodyBackground).BaseWidth - 2) - 2); Height(Node(token).InnerBackground, ((double)Base(Node(token).BodyBackground).BaseHeight - 2) - 2); Require(_hooks.Attach)(Node(token).InnerBackground, 2, 2, Node(token).BodyBackground, 0);
        Require(_hooks.Push)(Node(token).Body); Width(Node(token).Body, Base(Node(token).InnerBackground).BaseWidth); Height(Node(token).Body, Base(Node(token).InnerBackground).BaseHeight); Require(_hooks.Attach)(Node(token).Body, 0, 0, Node(token).InnerBackground, 0);
        if (Node(token).Information != 0) { Require(_hooks.Push)(Node(token).Information); if (Require(_hooks.Lines)(Node(token).Information) == 0) Base(Node(token).Information).Flags &= ~1U; Width(Node(token).Information, (double)Node(token).BaseWidth - 197); Height(Node(token).Information, 126); Require(_hooks.Attach)(Node(token).Information, 182, 15, token, 0); }
    }
    /// <summary>기본 주제→현재 Reset/전용 HTML→같은 본문 치환→현재 TYPE그림→실제 표시 자식을 갱신한다.</summary>
    public void Show(uint token, uint type, string topic)
    {
        topic = CText(topic); if (Node(token).Picture != 0) _ = Base(Node(token).Picture); string selected = topic; if (type == 0) { if (topic.Length == 0) selected = _state.Battle ? "pyrosphereHelp" : _state.Challenge ? "challengeHelp" : "introHelp"; else if (topic.Length >= 80) throw new ArgumentException("Quick Help 원본 주제80BYTE 초과"); }
        Reset(token); if (type != 0) _state.InfoMode = 1; Require(_hooks.Render)(Node(token).Body, type, selected, Node(token).Information, Node(token).Stream); _state.InfoMode = 0; byte[] output = Body(token).Body; int end = Array.IndexOf(output, (byte)0); if (end < 0) throw new ArgumentException("Quick Help 본문 NUL 누락"); string text = CText(Require(_hooks.Substitute)(Encoding.Latin1.GetString(output, 0, end))); if (Node(token).Picture != 0) _ = Base(Node(token).Picture); if (text.Length == 0) text = CText(Require(_hooks.Translate)("No help information available")); if (text.Length >= HtmlBodyGumpNode.Capacity) throw new ArgumentException("Quick Help 본문8192BYTE 초과");
        // 치환 전에 빌린 동일 배열의 접두부만 쓰고 뒤쪽은 보존한다.
        for (int i = 0; i < text.Length; i++) output[i] = checked((byte)text[i]); output[text.Length] = 0;
        int? frame = type != 0 ? Require(_hooks.HelpFrame)(type) : null; if (frame is null or -1) { if (Node(token).Picture != 0) { _ = Base(Node(token).Picture); Require(_hooks.Kill)(Node(token).Picture); Node(token).Picture = 0; } } else if (type != _state.NoPictureA && type != _state.NoPictureB) { uint picture = Allocate(0xb4); Require(_hooks.ConstructPicture)(picture, type, unchecked((uint)frame.Value), 8); Node(token).Picture = picture; Require(_hooks.Attach)(Node(token).Picture, 0, 0, token, 0); } Require(_hooks.BodyText)(Node(token).Body, Encoding.Latin1.GetString(output, 0, text.Length));
    }
    /// <summary>실제 기본 생성/기본 부착/지연 삭제·같은 HTML 부모 수명과 표시만 연결한다.</summary>
    public static void BindQuickHelpWindowDevices(QuickHelpWindowHooks hooks, GumpCreate factory, GumpTree tree, GumpLifetime lifetime, HtmlBodyGump html) { hooks.ConstructBase = (t, c, w, h, f) => factory.Construct(t, c, w, h, f); hooks.BaseAttach = tree.Attach; hooks.Kill = lifetime.Kill; hooks.ConstructHtml = t => html.Construct(t); hooks.ClearBody = html.Clear; hooks.BodyText = html.SetText; }
    /// <summary>실제 장식/배경/돌 버튼/정보/이미지 생성기만 연결한다.현재 할당/자산·파일/전용 Render는 상위가 공급한다.</summary>
    public static void BindQuickHelpWindowWidgets(QuickHelpWindowHooks hooks, GumpDecoration decoration, GumpBackgroundCreate background, StoneButtonGump buttons, ScrollingTextGumpWidgets text, ImageGump images, Func<uint, uint[]>? context = null) { hooks.ConstructDecoration = (t, w, h) => decoration.Construct(t, w, h); hooks.ConstructBackground = (t, w, h, bg, f, type, frame) => background.Construct(t, w, h, bg, f, type, frame); hooks.ConstructButton = (t, w, h, label, unused, pointer) => buttons.Construct(t, w, h, Encoding.Latin1.GetBytes(label + '\0'), unused, pointer == 0 ? null : Require(context)(pointer)); hooks.FitButton = buttons.FitToText; hooks.ConstructInformation = (t, w, h, font, color) => text.Construct(t, w, h, font, color); hooks.ConstructPicture = (t, type, frame, mode) => images.Construct(t, type, frame, mode); }
    /// <summary>현재 전용 창만 가상 부착으로 분배하고 나머지 파생은 보존한다.</summary>
    public static void BindQuickHelpWindow(GumpTreeHooks hooks, Func<uint, QuickHelpWindow?> resolve) { var previous = hooks.Attach; hooks.Attach = (t, x, y, parent, mode) => { if (resolve(t) is { } help) help.Attach(t, x, y, parent, mode); else Require(previous)(t, x, y, parent, mode); }; }
}
