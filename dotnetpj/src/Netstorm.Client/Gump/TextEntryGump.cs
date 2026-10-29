using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 같은 기본 Gump/문자 버퍼를 쓰는 원본 Editgump와 Console 영역이다.

/// <summary>VT00508e10의 +b8..108 필드.기본/배경 객체를 복제하지 않는다.</summary>
public class TextEntryGumpNode : GumpBackgroundNode
{
    /// <summary>세로 Slider 토큰.</summary>
    public uint VerticalSlider { get; set; }
    /// <summary>가로 Slider 토큰.</summary>
    public uint HorizontalSlider { get; set; }
    /// <summary>비0이면 편집 버퍼 객체도 소유한다.</summary>
    public uint OwnsBuffer { get; set; }
    /// <summary>생성자가 쓰지 않는+c4.</summary>
    public uint ReservedC4 { get; set; }
    /// <summary>다음 캐럿 점멸의 실제 벽 시계 초.</summary>
    public double BlinkAt { get; set; }
    /// <summary>캐럿 표시 위상.하위bit만 표시에서 읽는다.</summary>
    public uint BlinkPhase { get; set; }
    /// <summary>문자 입력과 캐럿 점멸 금지 관문.</summary>
    public uint ReadOnly { get; set; }
    /// <summary>Shift 선택의 고정 기준.</summary>
    public int ShiftAnchor { get; set; }
    /// <summary>Shift 선택의 이동 끝.</summary>
    public int ShiftEnd { get; set; }
    /// <summary>현재 표시 시작 줄.</summary>
    public int ScrollLine { get; set; }
    /// <summary>현재 표시 시작 열.</summary>
    public int ScrollColumn { get; set; }
    /// <summary>세로 Slider 표시 원시DWORD.</summary>
    public uint ShowVertical { get; set; }
    /// <summary>가로 Slider 표시 원시DWORD.</summary>
    public uint ShowHorizontal { get; set; }
    /// <summary>원시 입력을 부모에 전달하는 관문.</summary>
    public uint ForwardInput { get; set; }
    /// <summary>Enter에서 LF를 삽입하는 관문.</summary>
    public uint Multiline { get; set; }
    /// <summary>편집 뒤 변경 사건을 보내는 관문.</summary>
    public uint ChangeNotice { get; set; }
    /// <summary>콜론 앞 수신 대상의 강조 관문.</summary>
    public uint RecipientHighlight { get; set; }
    /// <summary>양수이면 현재length+1과 엄격 비교하는 입력 제한.</summary>
    public int MaxLength { get; set; }
    /// <summary>별도 소유 NUL 제외문자 주소.</summary>
    public uint ExcludedChars { get; set; }
    /// <summary>같은 실제0x28 편집 버퍼 토큰.</summary>
    public uint Buffer { get; set; }
}
/// <summary>VT00506a6c의 동일 TextEntry와 자동완성/제출 확장.</summary>
public sealed class ConsoleGumpNode : TextEntryGumpNode
{
    /// <summary>생성자가 쓰지 않는+10c.</summary>
    public uint Reserved10C { get; set; }
    /// <summary>현재 자동완성 callback 식별자.</summary>
    public uint Autocomplete { get; set; }
    /// <summary>현재 제출 callback 식별자.</summary>
    public uint Submit { get; set; }
    /// <summary>다음Tab에서 새 단어를 찾는 관문.</summary>
    public uint ResetWord { get; set; }
    /// <summary>현재 후보 단어 시작의 원시DWORD.</summary>
    public uint WordStart { get; set; }
    /// <summary>현재 후보 길이의 원시DWORD.</summary>
    public uint WordLength { get; set; }
    /// <summary>생성자가 쓰지 않는+124.</summary>
    public uint Reserved124 { get; set; }
}
/// <summary>실제 Client 초기화가 공급하는 종류/팔레트/사건 전역.</summary>
public sealed class TextEntryGumpState
{
    /// <summary>signed 확장하는 편집창 종류BYTE.</summary>
    public sbyte Kind { get; set; } = 15;
    /// <summary>signed 확장하는 Console 종류BYTE.</summary>
    public sbyte ConsoleKind { get; set; } = 10;
    /// <summary>Slider 갈색.</summary>
    public uint Brown { get; set; }
    /// <summary>Slider 검정.</summary>
    public uint Black { get; set; }
    /// <summary>수신 대상 노랑.</summary>
    public uint Yellow { get; set; }
    /// <summary>Enter 부모 사건.</summary>
    public uint EnterEvent { get; set; } = 0x02000038;
    /// <summary>변경 부모 사건.</summary>
    public uint ChangeEvent { get; set; } = 0x020000bf;
    /// <summary>Slider 값 변경 사건.</summary>
    public uint SliderChange { get; set; } = 0x02000036;
    /// <summary>Slider 화살표 사건.</summary>
    public uint SliderStep { get; set; } = 0x0200002e;
}
/// <summary>현재 기본 문맥으로 NUL/명시 길이를 측정한다.</summary>
public delegate (int Width, int Height) TextEntryMeasure(uint token, ReadOnlySpan<byte> text, int length);
/// <summary>현재 기본 또는 명시 문맥으로 같은 상대 좌표에 출력한다.</summary>
public delegate void TextEntryDraw(uint token, ReadOnlySpan<byte> text, ScreenPoint point, uint[]? context);
/// <summary>원시 수신 대상 문자열을 검사한다.</summary>
public delegate bool TextEntryRecipient(ReadOnlySpan<byte> text);
/// <summary>새 단어는 같은 현재 본문 뷰, 반복Tab은null이다.반환null/빈 후보를 구별한다.</summary>
public delegate byte[]? TextEntryComplete(uint callback, ReadOnlyMemory<byte>? word, int length, bool shift);
/// <summary>현재 실제 본문 바이트로 제출 callback을 호출한다.</summary>
public delegate void TextEntrySubmit(uint callback, ReadOnlySpan<byte> text);
/// <summary>같은 노드/문자 버퍼/Slider와 실제 장치 경계.미연결 경계는 도달한 때만 거부한다.</summary>
public sealed class TextEntryGumpHooks
{
    /// <summary>같은 실제 파생 노드.</summary>
    public Func<uint, TextEntryGumpNode?>? Node { get; set; }
    /// <summary>같은 실제 Console 파생.</summary>
    public Func<uint, ConsoleGumpNode?>? Console { get; set; }
    /// <summary>같은 기본 노드 저장소.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 +108의 같은 실제 편집 버퍼.</summary>
    public Func<uint, TextEntryBuffer?>? Buffer { get; set; }
    /// <summary>같은 실제 Slider의 현재position.</summary>
    public Func<uint, ScrollGumpNode?>? Slider { get; set; }
    /// <summary>0x28 버퍼/0xd8 Slider 객체 할당.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>Destroy를 마친 버퍼 객체 반납.</summary>
    public Action<uint>? ReleaseBuffer { get; set; }
    /// <summary>배경(token,flags)의 실제 생성.나머지 원본 인자는0,0,1,0,0이다.</summary>
    public Action<uint, uint>? ConstructBackground { get; set; }
    /// <summary>현재 실제 Slider/세 자식 생성.</summary>
    public Action<uint, int, int, uint, uint>? ConstructSlider { get; set; }
    /// <summary>공통 기본 Attach.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>현재 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>실제 idle 등록.</summary>
    public Action<uint, bool>? Idle { get; set; }
    /// <summary>현재 가상 Redraw.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>현재 부모 체인의 공통 Bubble.</summary>
    public Action<uint, uint, uint, uint>? Bubble { get; set; }
    /// <summary>실제 Slider 최소/최대.</summary>
    public Action<uint, int, int>? SliderRange { get; set; }
    /// <summary>실제 Slider 페이지.</summary>
    public Action<uint, int>? SliderPage { get; set; }
    /// <summary>실제 Slider 위치.</summary>
    public Action<uint, int>? SliderPosition { get; set; }
    /// <summary>현재 실제 벽 시계 초.</summary>
    public Func<double>? Now { get; set; }
    /// <summary>현재 기본 글꼴 문맥의 측정.</summary>
    public TextEntryMeasure? Measure { get; set; }
    /// <summary>실제 화면 기준 위치.</summary>
    public Func<uint, ScreenPoint>? Position { get; set; }
    /// <summary>같은 실제 배경 픽셀 출력.</summary>
    public Action<uint>? DrawBackground { get; set; }
    /// <summary>같은 실제 글리프 출력.</summary>
    public TextEntryDraw? DrawText { get; set; }
    /// <summary>현재 클립 안의 인덱스 픽셀 채우기.</summary>
    public Action<ScreenRect, uint>? Fill { get; set; }
    /// <summary>현재 실제 수신 대상 검사.</summary>
    public TextEntryRecipient? Recipient { get; set; }
    /// <summary>별도 NUL 제외문자 할당.</summary>
    public Func<uint, uint>? AllocateBytes { get; set; }
    /// <summary>제외문자 교체 시 실제 반납.</summary>
    public Action<uint>? ReleaseBytes { get; set; }
    /// <summary>현재 내부 문자 주소의 순수 범위 조회.</summary>
    public Func<uint, Memory<byte>>? Bytes { get; set; }
    /// <summary>자동완성 callback.</summary>
    public TextEntryComplete? Complete { get; set; }
    /// <summary>입력 제출 callback.</summary>
    public TextEntrySubmit? Submit { get; set; }
    /// <summary>원본 줄 번호 진단.돌아오면 원문 흐름을 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>실제 텍스트/idle/기본 수명 반납.</summary>
    public Action<uint>? DestructBase { get; set; }
}
/// <summary>cpppj/src/client/TextEntryGump.cpp의 원문 생성/선택·키 입력/스크롤/캐럿·수신 대상/소유 수명.</summary>
public sealed class TextEntryGump
{
    /// <summary>원본 편집창 가상 표.</summary>
    private const uint EntryVtable = 0x508e10;
    /// <summary>원본 Console 가상 표.</summary>
    private const uint ConsoleVtable = 0x506a6c;
    /// <summary>기본 표시 비트.</summary>
    private const uint Visible = 1;
    /// <summary>미부착 비트.</summary>
    private const uint Void = 8;
    /// <summary>삭제 대기 비트.</summary>
    private const uint Dead = 0x10000;
    /// <summary>원본 입력 코드 마스크.</summary>
    private const uint CodeMask = 0x03ffffff;
    /// <summary>명령 사건 비트.</summary>
    private const uint Command = 0x02000000;
    /// <summary>해제 사건 비트.</summary>
    private const uint Release = 0x80000000;
    /// <summary>Shift 입력 비트.</summary>
    private const uint Shift = 0x20000000;
    /// <summary>Control 입력 비트.</summary>
    private const uint Control = 0x10000000;
    /// <summary>원본0.5초 캐럿 점멸 주기.</summary>
    private const double BlinkPeriod = .5;
    /// <summary>현재 종류/팔레트/사건 전역.</summary>
    private readonly TextEntryGumpState _state;
    /// <summary>현재 공유 저장소와 실제 장치.</summary>
    private readonly TextEntryGumpHooks _hooks;
    /// <summary>도달한 장치만 필수로 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("문자 입력창 경계 필요");
    /// <summary>같은 파생/기본/편집 버퍼 조회만 생성 때 요구한다.</summary>
    public TextEntryGump(TextEntryGumpState state, TextEntryGumpHooks hooks)
    { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); Require(hooks.Buffer); _state = state; _hooks = hooks; }
    /// <summary>같은 실제 파생/기본 주소를 확인한다.</summary>
    private TextEntryGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("문자 입력창 공유 노드 없음"); return n; }
    /// <summary>같은 실제 Console/TextEntry 객체를 확인한다.</summary>
    private ConsoleGumpNode Console(uint token) { var n = token != 0 ? _hooks.Console?.Invoke(token) : null; if (n is null || !ReferenceEquals(n, Node(token))) throw new InvalidOperationException("Console 공유 파생 없음"); return n; }
    /// <summary>도달한 현재 자식만 조회한다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("문자 입력창 실제 자식 없음");
    /// <summary>현재+108을 매번 재조회한다.</summary>
    private TextEntryBuffer Buffer(uint token) { uint current = Node(token).Buffer; return (current != 0 ? _hooks.Buffer!(current) : null) ?? throw new InvalidOperationException("문자 입력창 현재 버퍼 없음"); }
    /// <summary>가상 호출 뒤 같은 실제 Slider를 읽는다.</summary>
    private ScrollGumpNode Slider(uint token) { var n = token != 0 ? _hooks.Slider?.Invoke(token) : null; if (n is null || !ReferenceEquals(n, Base(token))) throw new InvalidOperationException("문자 입력창 실제 Slider 없음"); return n; }
    /// <summary>현재 가상+8(false) 요청.</summary>
    private void Redraw(uint token) => Require(_hooks.Redraw)(token, false);
    /// <summary>현재 부모 체인의 공통 사건 전달.</summary>
    private void Bubble(uint token, uint code, uint first, uint second) => Require(_hooks.Bubble)(token, code, first, second);
    /// <summary>현재 기본 문맥의 실제 글꼴 측정.</summary>
    private (int Width, int Height) Measure(uint token, ReadOnlySpan<byte> text, int length = -1) => Require(_hooks.Measure)(token, text, length);
    /// <summary>현재 NUL까지 포함한 뷰.미종료는 명시 호스트 오류다.</summary>
    private static ReadOnlySpan<byte> CString(ReadOnlySpan<byte> text) { int end = text.IndexOf((byte)0); return end >= 0 ? text[..(end + 1)] : throw new ArgumentException("문자 입력창 NUL 범위 없음"); }
    /// <summary>외부 optional 문자열에 한 번 안정된NUL을 붙인다.</summary>
    private static byte[] Terminated(ReadOnlySpan<byte> text) { int end = text.IndexOf((byte)0); return [.. (end < 0 ? text : text[..end]), 0]; }
    /// <summary>현재 별도 제외문자.0과 빈 문자열은 구별한다.</summary>
    private ReadOnlySpan<byte> Excluded(uint token) { uint address = Node(token).ExcludedChars; return address != 0 ? CString(Require(_hooks.Bytes)(address).Span) : []; }
    /// <summary>배경→버퍼→idle→초기 본문→두 실제 Slider 순서의 부분 초기화다.</summary>
    public uint Construct(uint token, int width, int height, ReadOnlyMemory<byte>? initial, uint font, uint background, uint color)
    {
        var n = Node(token); Require(_hooks.ConstructBackground); Require(_hooks.Allocate); Require(_hooks.ConstructSlider); Require(_hooks.Idle); Require(_hooks.Attach); Require(_hooks.Now);
        _hooks.ConstructBackground!(token, background); n.Vtable = EntryVtable; n.BlinkAt = 0; n.ShiftAnchor = 0; n.ShiftEnd = 0; n.Command = unchecked((uint)_state.Kind);
        uint allocated = _hooks.Allocate!(0x28); if (allocated != 0) (_hooks.Buffer!(allocated) ?? throw new InvalidOperationException("새 편집 버퍼 저장소 없음")).Construct(); n.Buffer = allocated; n.OwnsBuffer = 1; if (allocated == 0) _hooks.Assertion?.Invoke(0x31);
        n.MaxLength = 0; n.BlinkPhase = 0; n.BlinkAt = _hooks.Now!() + BlinkPeriod; n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); uint[] context = [0, font, color, 0, 0, 0, 1, 1]; context.CopyTo(n.TextContext, 0); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale); n.ScrollLine = 0; n.ScrollColumn = 0; n.Flags |= 0x400400; _hooks.Idle!(token, true);
        byte[] bytes = initial.HasValue ? Terminated(initial.Value.Span) : []; Buffer(token).Reset(bytes); Redraw(token); n.ReadOnly = 0; n.ForwardInput = 0; n.ChangeNotice = 0; n.Multiline = 1;
        uint slider = _hooks.Allocate(0xd8); if (slider != 0) _hooks.ConstructSlider!(slider, 10, 100, _state.Brown, _state.Black); n.VerticalSlider = slider; slider = _hooks.Allocate(0xd8); if (slider != 0) _hooks.ConstructSlider!(slider, 100, 10, _state.Brown, _state.Black); n.HorizontalSlider = slider;
        Base(n.VerticalSlider).Flags &= ~Visible; Base(n.HorizontalSlider).Flags &= ~Visible; _hooks.Attach!(n.VerticalSlider, (float)((double)(float)width - Base(n.VerticalSlider).BaseWidth), 0, token, 1); _hooks.Attach(n.HorizontalSlider, 0, (float)((double)(float)height - Base(n.HorizontalSlider).BaseHeight), token, 1);
        SetScroll(token, 0, 0); n.ShowVertical = 0; n.ShowHorizontal = 0; n.RecipientHighlight = 0; if (initial.HasValue) { n.ShiftAnchor = 0; n.ShiftEnd = bytes.Length - 1; Buffer(token).Select(0, n.ShiftEnd); } n.ExcludedChars = 0; return token;
    }
    /// <summary>같은 TextEntry를 생성한 뒤 Console 확장만 초기화한다.</summary>
    public uint ConstructConsole(uint token, int width, int height, ReadOnlyMemory<byte>? initial, uint font, uint background, uint color)
    { Console(token); Construct(token, width, height, initial, font, background, color); var n = Console(token); n.Vtable = ConsoleVtable; n.Command = unchecked((uint)_state.ConsoleKind); n.Autocomplete = 0; n.Submit = 0; n.WordLength = 0; n.WordStart = 0; n.ResetWord = 1; n.RecipientHighlight = 1; return token; }
    /// <summary>기본 부착→자식Push→현재 크기/표시→현재scroll 순서다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        var n = Node(token); Require(_hooks.BaseAttach); Require(_hooks.Push); Require(_hooks.Attach); _hooks.BaseAttach!(token, x, y, parent, mode); _hooks.Push!(n.VerticalSlider); _hooks.Push(n.HorizontalSlider);
        var vertical = Base(n.VerticalSlider); vertical.BaseHeight = n.BaseHeight; vertical.Height = (float)((double)n.BaseHeight * vertical.Scale); var horizontal = Base(n.HorizontalSlider); horizontal.BaseWidth = n.BaseWidth; horizontal.Width = (float)((double)n.BaseWidth * horizontal.Scale);
        Base(n.VerticalSlider).Flags = n.ShowVertical != 0 ? Base(n.VerticalSlider).Flags | Visible : Base(n.VerticalSlider).Flags & ~Visible; Base(n.HorizontalSlider).Flags = n.ShowHorizontal != 0 ? Base(n.HorizontalSlider).Flags | Visible : Base(n.HorizontalSlider).Flags & ~Visible;
        _hooks.Attach!(n.VerticalSlider, (float)((double)n.BaseWidth - Base(n.VerticalSlider).BaseWidth), 0, token, 1); _hooks.Attach(n.HorizontalSlider, 0, (float)((double)n.BaseHeight - Base(n.HorizontalSlider).BaseHeight), token, 1); SetScroll(token, n.ScrollLine, n.ScrollColumn);
    }
    /// <summary>열 제한은 새line이 아닌 이전scrollLine을 조회한다.두 Slider callback 뒤 현재position을 읽는다.</summary>
    public void SetScroll(uint token, int line, int column)
    {
        var n = Node(token); if (line < 0) line = 0; else if (Buffer(token).LineCount() <= line) line = unchecked(Buffer(token).LineCount() - 1); if (column < 0) column = 0; else if (Buffer(token).LineLength(n.ScrollLine) <= column) column = unchecked(Buffer(token).LineLength(n.ScrollLine) - 1);
        n.ScrollLine = line; n.ScrollColumn = column; var metric = Measure(token, "M\0"u8); int rows = GumpCollection.Ftol((double)n.BaseHeight / metric.Height), columns = GumpCollection.Ftol((double)n.BaseWidth / metric.Width); Require(_hooks.SliderRange); Require(_hooks.SliderPage); Require(_hooks.SliderPosition);
        Buffer(token).LineLength(n.ScrollLine); int count = Buffer(token).LineLength(n.ScrollLine); _hooks.SliderRange!(n.HorizontalSlider, 0, unchecked(count - 1)); _hooks.SliderPage!(n.HorizontalSlider, columns); _hooks.SliderPosition!(n.HorizontalSlider, column); n.ScrollColumn = Slider(n.HorizontalSlider).Position;
        int lines = Buffer(token).LineCount(); _hooks.SliderRange(n.VerticalSlider, 0, unchecked(lines - 1)); _hooks.SliderPage(n.VerticalSlider, rows); _hooks.SliderPosition(n.VerticalSlider, line); n.ScrollLine = Slider(n.VerticalSlider).Position; Redraw(token);
    }
    /// <summary>현재signed DWORD에 감김delta를 더한다.</summary>
    public void RelativeScroll(uint token, int line, int column) => SetScroll(token, unchecked(Node(token).ScrollLine + line), unchecked(Node(token).ScrollColumn + column));
    /// <summary>삭제/읽기전용 관문 뒤 현재 시각을 두 번 읽는다.Redraw 뒤 시각으로 다음 점멸을 예약한다.</summary>
    public void Idle(uint token) { var n = Node(token); if ((n.Flags & Dead) != 0 || n.ReadOnly != 0) return; Require(_hooks.Now); if (n.BlinkAt <= _hooks.Now!()) { n.BlinkPhase = unchecked(n.BlinkPhase + 1); Redraw(token); n.BlinkAt = _hooks.Now() + BlinkPeriod; } }
    /// <summary>같은 현재 본문Reset 뒤 가상Redraw를 요청한다.</summary>
    public void SetText(uint token, ReadOnlySpan<byte> text = default) { Buffer(token).Reset(text); Redraw(token); }
    /// <summary>끝 삽입 뒤 요청한 경우에만 이전 선택을 복구한다.</summary>
    public void AppendText(uint token, ReadOnlySpan<byte> text, bool restoreSelection = false) { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd, at = unchecked(Buffer(token).State.Length + 1); Buffer(token).Select(at, at); Buffer(token).Insert(text); if (restoreSelection) Buffer(token).Select(begin, end); Redraw(token); }
    /// <summary>실제 편집 버퍼의 전체 복사/용량 거부다.</summary>
    public uint CopyText(uint token, Span<byte> output) => Buffer(token).CopyAll(output);
    /// <summary>이전 제외문자부터 반납한 뒤 새 문자열에서LF만 제거한다.기본 소멸자의 누락free는 추가하지 않는다.</summary>
    public void SetExcluded(uint token, ReadOnlyMemory<byte>? text)
    {
        var n = Node(token); if (n.ExcludedChars != 0) { Require(_hooks.ReleaseBytes)(n.ExcludedChars); n.ExcludedChars = 0; } if (!text.HasValue) return; Require(_hooks.AllocateBytes); Require(_hooks.Bytes); byte[] source = Terminated(text.Value.Span); n.ExcludedChars = _hooks.AllocateBytes!((uint)source.Length); if (n.ExcludedChars == 0) _hooks.Assertion?.Invoke(0x85); var target = _hooks.Bytes!(n.ExcludedChars).Span; if (target.Length < source.Length) throw new InvalidOperationException("제외문자 할당 범위 실패"); source.CopyTo(target); int index = 0;
        // LF를 지우면 같은 현재 위치를 다시 검사하며 최초 할당폭의memmove를 유지한다.
        while (target[index] != 0) { if (target[index] == 10) target.Slice(index + 1, source.Length - index - 1).CopyTo(target[index..]); else index++; }
    }
    /// <summary>현재 가상Push 뒤 현재 위치로 재부착한다.이미void면 필드만 쓴다.</summary>
    public void SetHorizontal(uint token, uint value) { var n = Node(token); n.ShowHorizontal = value; if ((n.Flags & Void) == 0) { Require(_hooks.Push); Require(_hooks.Attach); _hooks.Push!(token); _hooks.Attach!(token, n.X, n.Y, 0, 0); } }
    /// <summary>소유한 버퍼만 반납한다.자식Slider는 공통 자식 우선Destroy가 맡는다.</summary>
    public void Destruct(uint token, bool console = false) { var n = Node(token); if (console) { Console(token); n.Vtable = ConsoleVtable; } n.Vtable = EntryVtable; if (n.Buffer != 0 && n.OwnsBuffer != 0) { uint saved = n.Buffer; Buffer(token).Destroy(); Require(_hooks.ReleaseBuffer)(saved); n.Buffer = 0; } Require(_hooks.DestructBase)(token); }
    /// <summary>선택/편집→변경 통지→Redraw→원시 전달과 별도 Slider 분기의 원문 순서다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second)
    {
        var n = Node(token); uint value = code, result = 0, key = code & CodeMask;
        if ((code & Command) == 0 && key != 0x10000 && key != 0x20000 && n.ReadOnly == 0 && (code & Release) == 0)
        {
            result = 1; bool notify = true;
            switch (key)
            {
                case 0x250000: { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd; if ((code & Shift) == 0) Buffer(token).Select(unchecked(begin - 1), unchecked(begin - 1)); else { if (begin == end) { n.ShiftAnchor = begin; n.ShiftEnd = end; } n.ShiftEnd = unchecked(n.ShiftEnd - 1); Buffer(token).Select(n.ShiftAnchor, n.ShiftEnd); } break; }
                case 0x270000: { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd; if ((code & Shift) == 0) Buffer(token).Select(unchecked(end + 1), unchecked(end + 1)); else { if (begin == end) { n.ShiftAnchor = begin; n.ShiftEnd = end; } n.ShiftEnd = unchecked(n.ShiftEnd + 1); Buffer(token).Select(n.ShiftAnchor, n.ShiftEnd); } break; }
                case 0x230000: { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd; if ((code & Shift) == 0) { int at = unchecked(Buffer(token).State.Length + 1); Buffer(token).Select(at, at); RelativeScroll(token, 0, n.ShiftEnd); } else { if (begin == end) { n.ShiftAnchor = begin; n.ShiftEnd = end; } n.ShiftEnd = unchecked(Buffer(token).State.Length + 1); Buffer(token).Select(n.ShiftAnchor, n.ShiftEnd); RelativeScroll(token, 0, n.ShiftEnd); } break; }
                case 0x240000: { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd; if ((code & Shift) == 0) Buffer(token).Select(0, 0); else { if (begin == end) { n.ShiftAnchor = begin; n.ShiftEnd = end; } n.ShiftEnd = 0; Buffer(token).Select(n.ShiftAnchor, n.ShiftEnd); } RelativeScroll(token, 0, unchecked(-n.ScrollColumn)); break; }
                case 8: if ((code & Control) != 0) Buffer(token).Reset(); else { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd; if (begin == end) begin = unchecked(begin - 1); Buffer(token).Select(begin, end); Buffer(token).Insert(); } break;
                case 13: if (n.Multiline != 0) { value = 10; Buffer(token).Insert("\n\0"u8); } Bubble(token, _state.EnterEvent, n.Serial, 0); break;
                case 9: case 27: case 0x260000: case 0x280000: result = 0; notify = false; break;
                case 0x2d0000: if ((code & Control) != 0) Buffer(token).Copy(); else if ((code & Shift) != 0) Buffer(token).Paste(Excluded(token)); break;
                case 0x2e0000: if ((code & Shift) != 0) Buffer(token).Cut(); else { int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd; if (begin == end) end = unchecked(end + 1); Buffer(token).Select(begin, end); Buffer(token).Insert(); } break;
                default:
                    if (key < 0x20 || key >= 0x100) { result = 0; notify = false; break; }
                    if (n.MaxLength < 1 || unchecked(Buffer(token).State.Length + 1) < n.MaxLength) { var excluded = Excluded(token); if (!excluded.IsEmpty && excluded.Contains(unchecked((byte)value))) { result = 0; notify = false; break; } Span<byte> character = [(byte)value, 0]; Buffer(token).Insert(character); } break;
            }
            if (notify && n.ChangeNotice != 0) Bubble(token, _state.ChangeEvent, 0, 0); Redraw(token); if (n.ForwardInput != 0) Bubble(token, value, first, second);
        }
        if ((value & CodeMask) == _state.SliderChange) { if (first == Base(n.VerticalSlider).Serial) SetScroll(token, Slider(n.VerticalSlider).Position, n.ScrollColumn); else if (first == Base(n.HorizontalSlider).Serial) SetScroll(token, n.ScrollLine, Slider(n.HorizontalSlider).Position); else _hooks.Assertion?.Invoke(0x246); }
        if ((value & CodeMask) == _state.SliderStep) { var metric = Measure(token, "M\0"u8); if (first == Base(n.HorizontalSlider).Serial && second is 1 or 2 or 3 or 4) { RelativeScroll(token, 0, second is 2 or 4 ? unchecked(-metric.Width) : metric.Width); result = 1; } } return result;
    }
    /// <summary>새 단어의 실제 본문 뷰/null 순환 요청·제출 뒤 기본 입력 위임을 보존한다.</summary>
    public uint HandleConsole(uint token, uint code, uint first, uint second)
    {
        var n = Console(token); uint key = code & CodeMask;
        if ((code & Command) == 0 && key != 0x10000 && key != 0x20000 && (code & Release) == 0)
        {
            if (key == 9)
            {
                int begin = Buffer(token).State.SelectionBegin, end = Buffer(token).State.SelectionEnd;
                if (begin == end && n.Autocomplete != 0)
                {
                    ReadOnlyMemory<byte>? word = null;
                    if (n.ResetWord != 0) { n.WordStart = unchecked((uint)begin); var data = Buffer(token).Data();
                        // 원본 ASCII 공백/콜론 바로 뒤까지 단어 시작을 역방향으로 찾는다.
                        while (n.WordStart > 0) { uint at = n.WordStart - 1; if (at >= data.Length) throw new ArgumentOutOfRangeException(nameof(begin)); byte b = data.Span[(int)at]; if (b == 32 || b is >= 9 and <= 13 || b == 58) break; n.WordStart--; }
                        n.WordLength = unchecked((uint)begin - n.WordStart); if (n.WordStart >= data.Length) throw new ArgumentOutOfRangeException(nameof(begin)); var raw = data[(int)n.WordStart..]; int count = CString(raw.Span).Length - 1; word = raw[..count];
                    }
                    byte[]? found = Require(_hooks.Complete)(n.Autocomplete, word, unchecked((int)n.WordLength), (code & Shift) != 0); if (found is not null) { Buffer(token).Select(unchecked((int)n.WordStart), unchecked((int)(n.WordStart + n.WordLength))); byte[] bytes = Terminated(found); Buffer(token).Insert(bytes); n.WordLength = (uint)bytes.Length - 1; n.ResetWord = 0; }
                }
                return 1;
            }
            if (key == 13) { if (n.Submit != 0) Require(_hooks.Submit)(n.Submit, Buffer(token).Text()); } else if (key == 0x100000) return Handle(token, code, first, second); n.ResetWord = 1;
        }
        return Handle(token, code, first, second);
    }
    /// <summary>같은 줄 저장소에서 선택 조각을NUL 종료로 복사한다.</summary>
    private static byte[] Piece(byte[] line, int at, int count) { if (at < 0 || at > line.Length || count < 0 || count >= line.Length || count > line.Length - at) throw new ArgumentOutOfRangeException(nameof(count)); byte[] bytes = new byte[count + 1]; line.AsSpan(at, count).CopyTo(bytes); return bytes; }
    /// <summary>배경→각 현재 줄→일반/선택·수신 대상→캐럿의 실제 상대 글자/화면 채우기 순서다.</summary>
    public void Draw(uint token)
    {
        var n = Node(token); Require(_hooks.DrawBackground); Require(_hooks.DrawText); Require(_hooks.Position); Require(_hooks.Fill); _hooks.DrawBackground!(token); var metric = Measure(token, "M\0"u8); int truncated = GumpCollection.Ftol(n.BaseHeight); if (metric.Height == 0 || (truncated == int.MinValue && metric.Height == -1)) throw new InvalidOperationException("문자 입력창 줄높이 나눗셈 범위"); int partial = truncated % metric.Height != 0 ? 1 : 0, limit = GumpCollection.Ftol((double)n.BaseHeight / metric.Height + partial); byte[] line = new byte[1024]; int[] selection = new int[2]; int y = 0;
        // 각 반복에서 현재 줄 수를 재조회한다.수평 제거 뒤에도 선택/길이의 원시값을 보정하지 않는다.
        for (int index = n.ScrollLine; index < Buffer(token).LineCount(); index = unchecked(index + 1))
        {
            if (limit <= unchecked(index - n.ScrollLine)) return; uint length = Buffer(token).CopyLine(index, line, selection); if (length == uint.MaxValue) { _hooks.Assertion?.Invoke(0x2ae); throw new InvalidOperationException("문자 입력창 줄 복사 범위 거부"); }
            if (n.ScrollColumn > 0 && n.ScrollColumn < 1024) { int visible = CString(line).Length - 1, removed = n.ScrollColumn; if (removed <= visible) line.AsSpan(removed, visible - removed + 1).CopyTo(line); }
            if (length != 0) { if (length > line.Length) throw new ArgumentOutOfRangeException(nameof(length)); if (line[length - 1] == 10) line[length - 1] = 32; } else { line[0] = 32; line[1] = 0; }
            var dimensions = Measure(token, line);
            if (selection[0] == -1 || selection[1] <= selection[0])
            {
                int colon = CString(line).IndexOf((byte)':'); if (n.RecipientHighlight == 0 || colon < 0) _hooks.DrawText!(token, line, new(0, y), null);
                else { byte saved = line[colon + 1]; line[colon + 1] = 0; line[colon] = 0; bool valid = Require(_hooks.Recipient)(line.AsSpan(0, colon)); line[colon] = 58; if (valid) { uint[] context = (uint[])n.TextContext.Clone(); context[2] = _state.Yellow; _hooks.DrawText!(token, line, new(0, y), context); } else _hooks.DrawText!(token, line, new(0, y), null); var prefix = Measure(token, line); line[colon + 1] = saved; _hooks.DrawText!(token, line.AsSpan(colon + 1), new(prefix.Width, y), null); }
            }
            else
            {
                if (selection[0] < 0 || selection[1] < 0) throw new InvalidOperationException("문자 입력창 선택 인덱스 범위"); int x = 0; if (selection[0] > 0) { byte[] before = Piece(line, 0, selection[0]); var size = Measure(token, before); _hooks.DrawText!(token, before, new(0, y), null); x = size.Width; }
                uint[] context = (uint[])n.TextContext.Clone(); context[2] = n.BackgroundFlags & 0xff; byte[] selected = Piece(line, selection[0], unchecked(selection[1] - selection[0])); var selectedSize = Measure(token, selected); var position = _hooks.Position!(token); int left = unchecked(position.X + x), top = unchecked(position.Y + y); _hooks.Fill!(new(left, top, unchecked(left + selectedSize.Width), unchecked(top + selectedSize.Height)), n.TextContext[2]); _hooks.DrawText!(token, selected, new(x, y), context);
                if (selection[1] < unchecked((int)length)) { int at = selection[1]; if (at >= line.Length) throw new ArgumentOutOfRangeException(nameof(selection)); _hooks.DrawText!(token, line.AsSpan(at), new(unchecked(x + selectedSize.Width), y), null); }
            }
            if (selection[0] == selection[1] && selection[0] != -1 && (n.BlinkPhase & 1) != 0 && n.ReadOnly == 0) { var position = _hooks.Position!(token); var caret = Measure(token, line, selection[0]); int left = unchecked(position.X + caret.Width), top = unchecked(position.Y + y); _hooks.Fill!(new(left, top, unchecked(left + 1), unchecked(top + caret.Height)), n.TextContext[2]); } y = unchecked(y + dimensions.Height);
        }
    }
    /// <summary>RawTextEntryControls.cpp의 공통 장치 조립을 같은 실제 처리기에 연결한다.객체 할당/종류/클립보드·시계는 소유 호스트가 공급한다.</summary>
    public static void BindTextEntryGumpDevices(TextEntryGumpHooks hooks, TextEntryStorage storage, GumpBackgroundCreate background, GumpBackgroundDraw backgroundDraw, ScrollGumpWidgets widgets, ScrollGump slider, GumpTree tree, GumpLifetime lifetime, GumpEvents events, GumpCollection collection, GumpText metrics, GumpTextDraw text, GumpPixelTarget pixels)
    {
        ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(background); ArgumentNullException.ThrowIfNull(backgroundDraw); ArgumentNullException.ThrowIfNull(widgets); ArgumentNullException.ThrowIfNull(slider); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(metrics); ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(pixels); Require(hooks.Node);
        // 글꼴/출력에서도 별도 상태를 만들지 않고 현재 같은 파생 문맥을 읽는다.
        TextEntryGumpNode Current(uint token) => hooks.Node!(token) ?? throw new InvalidOperationException("문자 장치 현재 노드 없음");
        hooks.Buffer = storage.Find; hooks.ReleaseBuffer = storage.ReleaseBuffer; hooks.ConstructBackground = (token, flags) => background.Construct(token, 0, 0, flags, 1, 0, 0); hooks.ConstructSlider = (token, width, height, color, edge) => widgets.Construct(token, width, height, color, edge); hooks.BaseAttach = tree.Attach; hooks.Push = tree.Push; hooks.Idle = lifetime.SetIdle; hooks.Bubble = (token, code, first, second) => events.Bubble(token, code, first, second); hooks.SliderRange = slider.SetRange; hooks.SliderPage = slider.SetPage; hooks.SliderPosition = slider.SetPosition; hooks.Position = collection.Position; hooks.DrawBackground = backgroundDraw.Draw; hooks.Fill = (rect, color) => pixels.Fill(rect, color);
        hooks.Measure = (token, bytes, length) => { int width = 0, height = 0; metrics.Measure(bytes, GumpText.DecodeGumpTextContext(Current(token).TextContext), ref width, ref height, length); return (width, height); };
        hooks.DrawText = (token, bytes, point, explicitContext) => { if (explicitContext is null) text.DrawGump(collection, token, bytes, point, Current(token).TextContext); else { var decoded = GumpText.DecodeGumpTextContext(explicitContext); text.DrawGump(collection, token, bytes, point, Current(token).TextContext, in decoded); } }; hooks.AllocateBytes = storage.Allocate; hooks.ReleaseBytes = storage.Release; hooks.Bytes = storage.Bytes; hooks.DestructBase = lifetime.DestructBase;
    }
    /// <summary>현재 파생만 실제 Attach/사건/캡처/idle/출력/scalar로 연결한다.다른 종류는 기존 분배를 유지한다.</summary>
    public static void BindTextEntryGump(GumpTreeHooks tree, GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, GumpDrawHooks draw, GumpLifetimeHooks lifetime, Func<uint, TextEntryGump?> resolve, Action<uint> release)
    {
        ArgumentNullException.ThrowIfNull(resolve); ArgumentNullException.ThrowIfNull(release); var attach = tree.Attach; tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } entry) entry.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); };
        // 같은 현재Console 분류로 부모 사건과 직접 캡처를 처리한다.기본 재귀 통지는 기존 분배를 유지한다.
        uint Route(Func<uint, uint, uint, uint, uint>? previous, uint t, uint c, uint a, uint b) { if (resolve(t) is not { } entry) return Require(previous)(t, c, a, b); return entry._hooks.Console?.Invoke(t) is not null ? entry.HandleConsole(t, c, a, b) : entry.Handle(t, c, a, b); }
        var e = events.Handle; var i = input.Handle; events.Handle = (t, c, a, b) => Route(e, t, c, a, b); input.Handle = (t, c, a, b) => Route(i, t, c, a, b); var idle = frame.Idle; frame.Idle = t => { if (resolve(t) is { } entry) entry.Idle(t); else Require(idle)(t); }; var render = draw.Draw; draw.Draw = t => { if (resolve(t) is { } entry) entry.Draw(t); else Require(render)(t); }; var deleting = lifetime.DeletingDestructor; lifetime.DeletingDestructor = (t, flags) => { if (resolve(t) is { } entry) { entry.Destruct(t, entry._hooks.Console?.Invoke(t) is not null); if ((flags & 1) != 0) release(t); } else Require(deleting)(t, flags); };
    }
}
