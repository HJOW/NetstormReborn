using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// 실제 네 자식/공유 버퍼/controlled 메뉴를 같은 원문 부모에 연결한다.
namespace Netstorm.Client.Gump;

/// <summary>ChatWindow.h의 같은 현재 노드와 실제 선택 장치 경계다.</summary>
public sealed class ChatWindowHooks
{
    /// <summary>같은 실제 root 파생 노드다.</summary>
    public Func<uint, HudChatNode?>? Chat { get; set; }
    /// <summary>root와 네 자식의 같은 실제 기본 주소다.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>같은 실제 Console 자식이다.</summary>
    public Func<uint, ConsoleGumpNode?>? Console { get; set; }
    /// <summary>같은 실제 로그 자식이다.</summary>
    public Func<uint, ScrollingTextGumpNode?>? Text { get; set; }
    /// <summary>현재 Console+108의 같은 편집 버퍼다.</summary>
    public Func<uint, TextEntryBuffer?>? Buffer { get; set; }
    /// <summary>동일 노드/버퍼에 설치한 실제 편집 본체다.</summary>
    public TextEntryGump? Entry { get; set; }
    /// <summary>같은 네 자식의 실제 부착 본체다.</summary>
    public HudChatGeometry? Geometry { get; set; }
    /// <summary>현재 사건/모드/색/포인터를 사용 위치에서 조회한다.</summary>
    public Func<uint, uint>? Global { get; set; }
    /// <summary>요청 크기에 맞는 실제 소유 노드를 할당한다.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>실제 공통 기본 생성이다.</summary>
    public Action<uint, float, float>? ConstructBase { get; set; }
    /// <summary>실제 돌 버튼 helper 생성이다.</summary>
    public Action<uint, string, uint>? ConstructHelper { get; set; }
    /// <summary>현재 helper 글자 크기 맞춤이다.</summary>
    public Action<uint>? FitHelper { get; set; }
    /// <summary>현재 소유 제목 문자열을 설치한다.</summary>
    public Action<uint, string>? Title { get; set; }
    /// <summary>현재 소유 도움말 키를 설치한다.</summary>
    public Action<uint, string>? Help { get; set; }
    /// <summary>현재 원문 표시 BYTE 번역이다.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>현재 root 문맥의 실제 글자 측정이다.</summary>
    public Func<uint, string, (int Width, int Height)>? Measure { get; set; }
    /// <summary>같은 실제 배경 생성이다.</summary>
    public Action<uint, float, float, uint>? ConstructBackground { get; set; }
    /// <summary>실제 로그와 자기 버퍼/Slider를 생성한다.</summary>
    public Action<uint, int, int, uint, uint>? ConstructScrolling { get; set; }
    /// <summary>이전 소유 버퍼를 반납하고 공유 버퍼를 빌린다.</summary>
    public Action<uint, uint, uint>? ReplaceBuffer { get; set; }
    /// <summary>같은 Client 수명의 실제 공유 로그 토큰이다.</summary>
    public Func<uint>? SharedBuffer { get; set; }
    /// <summary>현재 로그의 실제 Slider 재부착이다.</summary>
    public Action<uint, uint>? SetSlider { get; set; }
    /// <summary>현재 로그의 실제 스크롤 비트다.</summary>
    public Action<uint, uint>? SetScrollable { get; set; }
    /// <summary>현재 실제 가상 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 실제 가상 Push다.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>같은 공통 자식 우선 기본 수명이다.</summary>
    public Action<uint>? DestructBase { get; set; }
    /// <summary>같은 실제 인접 명령표에 등록한다.</summary>
    public Action<string, uint, uint>? RegisterCommand { get; set; }
    /// <summary>같은 공유 로그에 원문 callback을 설치한다.</summary>
    public Action<uint, uint>? BufferAppendCallback { get; set; }
    /// <summary>현재 TYPE의 프레임 코드/클러스터 수다.</summary>
    public Func<uint, int>? FrameCount { get; set; }
    /// <summary>현재 프레임 헤더의 실제 signed 높이다.</summary>
    public Func<uint, uint, short>? FrameHeight { get; set; }
    /// <summary>현재 로그의 실제 절대 스크롤 요청이다.</summary>
    public Action<uint, int>? RequestScroll { get; set; }
    /// <summary>현재 로그의 실제 상대 스크롤 요청이다.</summary>
    public Action<uint, int>? RelativeScroll { get; set; }
    /// <summary>현재 로그의 실제 맨 아래 요청이다.</summary>
    public Action<uint>? RequestBottom { get; set; }
    /// <summary>로그 추가 전 현재 맨 아래 여부다.</summary>
    public Func<uint, bool>? AtBottom { get; set; }
    /// <summary>같은 실제 공유 버퍼/스타일 파서에 추가한다.</summary>
    public Action<uint, string>? AppendLog { get; set; }
    /// <summary>같은 현재 기본 Help 입력에 위임한다.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>같은 현재 부모 사건이다.</summary>
    public Action<uint, uint, uint, uint>? Bubble { get; set; }
    /// <summary>실제 다중 수신 대상 검사다.</summary>
    public Func<string, bool>? Recipient { get; set; }
    /// <summary>같은 실제 manager와 입력 BYTE를 제출한다.</summary>
    public Action<uint, string>? SendChat { get; set; }
    /// <summary>같은 현재60BYTE Identity를 조회한다.</summary>
    public Func<byte[], bool>? Resolve { get; set; }
    /// <summary>이름 조회 실패의 실제 원문 진단이다.</summary>
    public Action<uint, string, uint>? Log { get; set; }
    /// <summary>helper 체크의 현재 실제 설정이다.</summary>
    public Func<string, int>? Config { get; set; }
    /// <summary>실제 controlled 메뉴 시작이다.</summary>
    public Action? MenuBegin { get; set; }
    /// <summary>실제 현재 controlled 메뉴를 확정한다.</summary>
    public Action? MenuFinalize { get; set; }
    /// <summary>같은 메뉴의 실제 제목이다.</summary>
    public Action<string>? MenuTitle { get; set; }
    /// <summary>실제 일반 항목이며 null과 빈 payload를 구별한다.</summary>
    public Action<string, uint, string?>? MenuItem { get; set; }
    /// <summary>현재 체크값/사건/payload를 실제 메뉴에 추가한다.</summary>
    public Action<uint, string, uint, string>? MenuChecked { get; set; }
    /// <summary>같은 현재 메뉴를 pop한다.</summary>
    public Func<uint>? MenuTake { get; set; }
    /// <summary>완성한 실제 자식 메뉴를 연결한다.</summary>
    public Action<string, uint>? MenuSubmenu { get; set; }
    /// <summary>현재 x/y/참조 객체로 실제 메뉴를 연다.</summary>
    public Action<uint, uint, uint>? MenuOpen { get; set; }
    /// <summary>돌아오면 계속하는 원문 진단 줄 번호다.</summary>
    public Action<uint>? Assertion { get; set; }
}

/// <summary>ChatWindow.cpp의 생성/편집/메뉴/현재 포인터/공통 수명 본체다.</summary>
public sealed class ChatWindow
{
    /// <summary>10.78 원문 ChatWindow 가상 표 식별자다.</summary>
    private const uint Vtable = 0x504a94;
    /// <summary>실제 Console 표시 비트다.</summary>
    private const uint Visible = 1;
    /// <summary>일반/명령 사건의 실제 코드 마스크다.</summary>
    private const uint CodeMask = 0x03ffffff;
    /// <summary>원문 명령 사건 비트다.</summary>
    private const uint Command = 0x02000000;
    /// <summary>일반 입력에서만 선행 거부하는 뗌 비트다.</summary>
    private const uint Release = 0x80000000;
    /// <summary>같은 Client 수명의 현재 포인터/등록/후보 상태다.</summary>
    private readonly ChatWindowState _state;
    /// <summary>같은 실제 자식/편집기/공유 로그와 선택 장치다.</summary>
    private readonly ChatWindowHooks _hooks;
    /// <summary>현재 root/기본 조회만 먼저 검증하고 나머지는 실제 도달 위치에서 요구한다.</summary>
    public ChatWindow(ChatWindowState state, ChatWindowHooks hooks)
    {
        _state = state ?? throw new ArgumentNullException(nameof(state)); _hooks = hooks ?? throw new ArgumentNullException(nameof(hooks)); Need(hooks.Chat); Need(hooks.Base);
    }
    /// <summary>미선택 하위는 먼저 요구하지 않는다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("ChatWindow의 실제 하위 연결 누락");
    /// <summary>원문 C문자열의 첫NUL 전까지만 사용한다.</summary>
    private static string Z(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>실제 편집기가 빌리는 안정된 NUL 포함 BYTE 입력이다.</summary>
    private static byte[] Terminated(string text) => Encoding.Latin1.GetBytes(Z(text) + '\0');
    /// <summary>원문DWORD 감김의 signed 차다.</summary>
    private static int Sub(int first, int second) => unchecked(first - second);
    /// <summary>원문 CRT의 signed64 절삭 후 하위DWORD다.</summary>
    private static int Ftol(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>같은 root 파생/기본 주소를 현재 공통 저장소와 확인한다.</summary>
    private HudChatNode Node(uint token)
    {
        var node = token != 0 ? Need(_hooks.Chat)(token) : null; if (node is null || !ReferenceEquals(node, Need(_hooks.Base)(token))) throw new InvalidOperationException("ChatWindow 공유 root 없음"); return node;
    }
    /// <summary>실제 사용 시점의 현재 자식 주소를 조회한다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? Need(_hooks.Base)(token) : null) ?? throw new InvalidOperationException("ChatWindow 실제 자식 없음");
    /// <summary>현재 Console이 같은 실제 기본 객체인지 확인한다.</summary>
    private ConsoleGumpNode Console(uint token)
    {
        uint child = Node(token).Console; var node = child != 0 ? Need(_hooks.Console)(child) : null; if (node is null || !ReferenceEquals(node, Base(child))) throw new InvalidOperationException("ChatWindow 실제 Console 없음"); return node;
    }
    /// <summary>현재 로그가 같은 실제 기본 객체인지 확인한다.</summary>
    private ScrollingTextGumpNode Text(uint token)
    {
        uint child = Node(token).Text; var node = child != 0 ? Need(_hooks.Text)(child) : null; if (node is null || !ReferenceEquals(node, Base(child))) throw new InvalidOperationException("ChatWindow 실제 ScrollingText 없음"); return node;
    }
    /// <summary>각 호출마다 현재 Console+108의 같은 실제 편집 버퍼를 읽는다.</summary>
    private TextEntryBuffer Buffer(uint token)
    {
        uint address = Console(token).Buffer; return (address != 0 ? Need(_hooks.Buffer)(address) : null) ?? throw new InvalidOperationException("ChatWindow 현재 편집 버퍼 없음");
    }
    /// <summary>실제 같은 편집 본체를 요구한다.</summary>
    private TextEntryGump Entry() => _hooks.Entry ?? throw new InvalidOperationException("ChatWindow 실제 TextEntry 본체 없음");
    /// <summary>현재 원문 전역을 사용하는 위치에서 조회한다.</summary>
    private uint Global(uint address) => Need(_hooks.Global)(address);
    /// <summary>실제 CopyText128의 실패/NUL 물리 끝을 확인한다.</summary>
    private string CopyInput(uint token)
    {
        byte[] bytes = new byte[128]; if (Entry().CopyText(Node(token).Console, bytes) == uint.MaxValue) throw new InvalidOperationException("ChatWindow 원본 입력 복사128 범위"); int end = bytes.AsSpan().IndexOf((byte)0); if (end < 0) throw new InvalidOperationException("ChatWindow 원본 입력 NUL 없음"); return Encoding.Latin1.GetString(bytes, 0, end);
    }
    /// <summary>접두의 NUL까지 실제 물리에 쓰고 미작성 꼬리는 보존한다.</summary>
    private void Prefix(uint token, string text)
    {
        text = Z(text); byte[] prefix = Node(token).InputPrefix; if (text.Length >= prefix.Length) throw new InvalidOperationException("ChatWindow 원본 접두128 범위"); Encoding.Latin1.GetBytes(text).CopyTo(prefix, 0); prefix[text.Length] = 0;
    }
    /// <summary>실제 네 자식 생성과 소유 버퍼→같은 공유 버퍼 교체를 원문 순서로 실행한다.</summary>
    public uint Construct(uint token, int width, int height)
    {
        var node = Node(token); Need(_hooks.ConstructBase)(token, width, height); node.Vtable = Vtable; node.Command = unchecked((uint)(int)(sbyte)(byte)Global(0x540424));
        uint child = Need(_hooks.Allocate)(0x170); if (child != 0) Need(_hooks.ConstructHelper)(child, ">", Global(0x5acd24)); node.Helper = child; Need(_hooks.FitHelper)(node.Helper);
        string helperTitle = Need(_hooks.Translate)("Chat Helper"); Need(_hooks.Title)(Node(token).Helper, helperTitle); Need(_hooks.Help)(Node(token).Helper, "chathelper");
        node.BaseWidth = width; node.Width = (float)((double)width * node.Scale); node.BaseHeight = height; node.Height = (float)((double)height * node.Scale); node.HelperVisible = 1;
        var metric = Need(_hooks.Measure)(token, "M"); node.Capacity = 128; node.LeftPadding = 0; child = Need(_hooks.Allocate)(0xb8);
        if (child != 0) Need(_hooks.ConstructBackground)(child, Sub(width, 20), Sub(height, metric.Height), Global(0x5b5dc0) | 0x04000000); node.Background = child;
        child = Need(_hooks.Allocate)(0x128); if (child != 0) Entry().ConstructConsole(child, Sub(width, 20), metric.Height, null, 5, Global(0x5b5e60) | 0x04000000, Global(0x5b5dd4)); node.Console = child; Console(token).ChangeNotice = 1; Console(token).MaxLength = 80;
        child = Need(_hooks.Allocate)(0xd8); if (child != 0) Need(_hooks.ConstructScrolling)(child, Sub(width, 20), Sub(height, metric.Height), 5, Global(0x5b5dd8)); node.Text = child;
        Need(_hooks.ReplaceBuffer)(Node(token).Text, Need(_hooks.SharedBuffer)(), 0); Text(token).Callback = 1;
        Need(_hooks.Attach)(Node(token).Background, Node(token).LeftPadding, 0, token, 0); Need(_hooks.Attach)(Node(token).Console, Node(token).LeftPadding, Sub(height, metric.Height), token, 0); Need(_hooks.Attach)(Node(token).Helper, Node(token).LeftPadding, Sub(height, metric.Height), token, 0); Need(_hooks.Attach)(Node(token).Text, 0, 0, Node(token).Background, 0);
        Need(_hooks.SetSlider)(Node(token).Text, 1); Need(_hooks.SetScrollable)(Node(token).Text, 1); Text(token).TextFlags &= ~0x10u; Console(token).Multiline = 0; Console(token).Flags &= ~Visible; node.Flags |= 0xe00; node.InputPrefix[0] = 0; Entry().SetText(Node(token).Console, Terminated("")); NormalizePrefix(token); Console(token).Autocomplete = 0x42e540;
        if (_state.CompletionReady == 0) Completions(); _state.CompletionReady = 1; return token;
    }
    /// <summary>최초 clear/quit 등록 뒤 실제 root를 만들고 같은 공유 로그 callback을 설치한다.</summary>
    public uint Create()
    {
        if (_state.Registered == 0) { Need(_hooks.RegisterCommand)("clear", 0x42f680, 1); Need(_hooks.RegisterCommand)("quit", 0x42e520, 1); _state.Registered = 1; }
        uint token = Need(_hooks.Allocate)(0x13c); _state.Current.Value = token != 0 ? Construct(token, 200, 100) : 0; if (_state.Current.Value == 0) throw new InvalidOperationException("ChatWindow 원본 new 실패 뒤 null 자식 접근"); Need(_hooks.BufferAppendCallback)(Text(_state.Current.Value).Buffer, 0x42f590); return _state.Current.Value;
    }
    /// <summary>같은 실제 기하 본체에 전체 자식 재부착을 맡긴다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode) => (_hooks.Geometry ?? throw new InvalidOperationException("ChatWindow 실제 Attach 본체 없음")).Attach(token, x, y, parent, mode);
    /// <summary>자식 문맥을 저장하지만 지정/복원은 부모에 쓰며 하위 예외에서 부분 효과를 복구하지 않는다.</summary>
    public void AppendLog(uint token, string text, ReadOnlySpan<uint> context = default)
    {
        uint child = Node(token).Text; uint[] saved = Base(child).TextContext.ToArray(); if (!context.IsEmpty) { if (context.Length != 8) throw new ArgumentException("Chat 로그 문맥8DWORD 계약", nameof(context)); uint[] selected = context.ToArray(); selected.CopyTo(Base(token).TextContext, 0); }
        bool bottom = Need(_hooks.AtBottom)(child); Need(_hooks.AppendLog)(Node(token).Text, text); if (bottom) Need(_hooks.RequestBottom)(Node(token).Text); saved.CopyTo(Base(token).TextContext, 0);
    }
    /// <summary>같은 두256색 목록과 높이8..16의 현재 TYPE 아이콘 문자열을 만든다.</summary>
    private void Completions()
    {
        // 두 실제256개 문자열을 원문 색번호 그대로 순서대로 추가한다.
        for (uint color = 0; color < 256; color++) { _state.Colors.Add("~[C." + color.ToString(CultureInfo.InvariantCulture) + ']'); _state.Backgrounds.Add("~[B." + color.ToString(CultureInfo.InvariantCulture) + ']'); }
        if (_state.CompletionReady != 0) return;
        // 원문은 각 반복에서 현재 signed TYPE 수를 다시 읽는다.
        for (int type = 0; type < unchecked((int)Global(0x541348)); type++)
        {
            int count = Need(_hooks.FrameCount)(unchecked((uint)type));
            // 이 TYPE의 처음 읽은 프레임 수로 현재 높이만 조회한다.
            for (int frame = 0; frame < count; frame++) { short height = Need(_hooks.FrameHeight)(unchecked((uint)type), unchecked((uint)frame)); if (height > 7 && height < 17) _state.Icons.Add("~[I" + type.ToString(CultureInfo.InvariantCulture) + '.' + frame.ToString(CultureInfo.InvariantCulture) + ']'); }
        }
    }
    /// <summary>실제 입력을 두 번 복사한 뒤 현재 접두와 본문을 다시 합친다.</summary>
    public void NormalizePrefix(uint token)
    {
        string prefix = CopyInput(token); int colon = prefix.IndexOf(':'); prefix = colon < 0 ? "" : prefix[..colon]; if (prefix.Length != 0) prefix += ':'; string body = CopyInput(token); int second = body.IndexOf(':'); if (second >= 0) body = body[(second + 1)..]; Entry().SetText(Node(token).Console, Terminated(prefix)); Entry().AppendText(Node(token).Console, Terminated(body), false); Prefix(token, prefix);
    }
    /// <summary>대소문자 구분 substring/공백으로 실제 현재 수신 접두를 추가 또는 제거한다.</summary>
    public void ChangeRecipient(uint token, string? name, uint operation)
    {
        if (operation > 1) _hooks.Assertion?.Invoke(0x561); if (name is null) return; string target = Z(name), prefix = CopyInput(token); int colon = prefix.IndexOf(':'); bool valid = true;
        if (colon >= 0) { prefix = prefix[..colon]; valid = Need(_hooks.Recipient)(prefix); } else prefix = ""; if (!valid) prefix = ""; int found = prefix.IndexOf(target, StringComparison.Ordinal);
        if (operation == 0 && found < 0) { if (prefix.Length != 0 && prefix[^1] != ' ') prefix += ' '; prefix += target + ' '; }
        else if (operation == 1 && found >= 0)
        {
            int end = found;
            // 먼저 찾은 substring부터 다음 공백 직전까지 이동한다.
            while (end < prefix.Length && prefix[end] != ' ') end++;
            // 이어진 원문 공백도 함께 제거한다.
            while (end < prefix.Length && prefix[end] == ' ') end++;
            prefix = prefix.Remove(found, end - found);
        }
        if (prefix.Length != 0) prefix += ':'; string body = CopyInput(token); int bodyColon = body.IndexOf(':'); if (bodyColon >= 0 && valid) body = body[(bodyColon + 1)..]; Entry().SetText(Node(token).Console, Terminated(prefix)); Entry().AppendText(Node(token).Console, Terminated(body), false); Prefix(token, prefix);
    }
    /// <summary>실제 입력128/유효 접두에서 대소문자 구분 substring을 찾으며 빈 이름도 허용한다.</summary>
    public bool ContainsRecipient(uint token, string name)
    {
        string input = CopyInput(token); int colon = input.IndexOf(':'); if (colon < 0) input = ""; else { input = input[..colon]; if (!Need(_hooks.Recipient)(input)) input = ""; } return input.Contains(Z(name), StringComparison.Ordinal);
    }
    /// <summary>명령에 추가 release 관문 없이 실제 기본 Help/Console/메뉴/대상 효과를 원문 순서로 선택한다.</summary>
    public uint Handle(uint token, uint input, uint first, uint second, string? text = null)
    {
        var node = Node(token); uint code = input & CodeMask;
        // 나머지 사건은 같은 현재 기본 Help 입력으로 위임한다.
        uint HandleBase() => Need(_hooks.BaseHandle)(token, input, first, second);
        if ((input & Command) == 0)
        {
            if (code != 0x10000 && code != 0x20000 && (input & Release) == 0)
            {
                if (code == 0x240000) { Need(_hooks.RequestScroll)(Node(token).Text, 0); return 1; } if (code == 0x230000) { Need(_hooks.RequestBottom)(Node(token).Text); return 1; }
                if (code is 0x210000 or 0x220000 or 0x260000 or 0x280000) { int delta = code == 0x210000 ? Ftol(-(double)node.BaseHeight) : code == 0x220000 ? Ftol(node.BaseHeight) : code == 0x260000 ? -5 : 5; Need(_hooks.RelativeScroll)(Node(token).Text, delta); return 1; }
            }
            return HandleBase();
        }
        if (code == Global(0x542b24)) { if (text is null) throw new InvalidOperationException("ChatWindow 삽입 사건의 실제 문자열 없음"); string value = Z(text); if (value == "copy") Buffer(token).Copy(); else if (value == "paste") Buffer(token).Paste(); else Entry().AppendText(Node(token).Console, Terminated(value), false); }
        if (code == Global(0x5427a8) && first == Base(Node(token).Helper).Serial) HelperMenu(token);
        if (code == Global(0x5429f0)) { if (_state.Current.Value != 0) Need(_hooks.Bubble)(_state.Current.Value, Global(0x542914), 0, 1); return 1; }
        if (code == Global(0x5427d8) && first == Base(Node(token).Console).Serial)
        {
            Entry().AppendText(Node(token).Console, Terminated("\n"), false); if (Buffer(token).State.Length != -2) Need(_hooks.SendChat)(Global(0x548714), Encoding.Latin1.GetString(Buffer(token).Text())); string prefix = CopyInput(token); int colon = prefix.IndexOf(':'); if (colon >= 0) { prefix = prefix[..colon]; prefix = Need(_hooks.Recipient)(prefix) ? prefix + ':' : ""; } else prefix = ""; Prefix(token, prefix); Entry().SetText(Node(token).Console, Terminated(prefix)); NormalizePrefix(token); return 1;
        }
        bool add = code == Global(0x542794), remove = !add && code == Global(0x542af8);
        if (add || remove)
        {
            byte[] record = new byte[60]; RawPlayerIdentity.Initialize(record, 0); BinaryPrimitives.WriteUInt32LittleEndian(record.AsSpan(4, 4), second);
            if (Need(_hooks.Resolve)(record)) { int end = record.AsSpan(8, 40).IndexOf((byte)0); ChangeRecipient(token, Encoding.Latin1.GetString(record, 8, end < 0 ? 40 : end), remove ? 1u : 0); return 1; } Need(_hooks.Log)(0x13, "Could not lookup a name for subId == %d\n", first); return 1;
        }
        if (code == Global(0x54298c)) { Need(_hooks.Push)(token); Need(_hooks.Attach)(token, Node(token).X, Node(token).Y, 0, 0); } return HandleBase();
    }
    /// <summary>다른 창이 current여도 원문처럼 무조건0으로 쓰고 공통 자식 우선 수명에 맡긴다.</summary>
    public void Destruct(uint token) { Node(token).Vtable = Vtable; _state.Current.Value = 0; Need(_hooks.DestructBase)(token); }
    /// <summary>실제 여덟 스타일 항목과 삽입값의 고정 순서다.</summary>
    private static readonly (string Label, string Value)[] Styles =
    [
        ("~BBold", "~B"),
        ("~EEmboss", "~E"),
        ("~IItalic", "~I"),
        ("~SStrikeout", "~S"),
        ("~UUnderline", "~U"),
        ("~.Reset Style", "~."),
        ("~NReset Color & Emboss", "~N"),
        ("~*Reset to Default", "~*"),
    ];
    /// <summary>실제 스무 기본 색 문자열과 삽입값의 고정 순서다.</summary>
    private static readonly (string Label, string Value)[] Colors =
    [
        ("~kBlack", "~k"),
        ("~xBrown", "~x"),
        ("~qDark Tan", "~q"),
        ("~jLight Tan", "~j"),
        ("~rRed", "~r"),
        ("~gGreen", "~g"),
        ("~uLight Green", "~u"),
        ("~dTeal", "~d"),
        ("~bBlue", "~b"),
        ("~lLight Blue", "~l"),
        ("~mMagenta", "~m"),
        ("~sDark Magenta", "~s"),
        ("~pPink", "~p"),
        ("~yYellow", "~y"),
        ("~iLight Yellow", "~i"),
        ("~oOrange", "~o"),
        ("~aLight Orange", "~a"),
        ("~wWhite", "~w"),
        ("~tSilver", "~t"),
        ("~nReset Color", "~n"),
    ];
    /// <summary>원문 controlled 하위 메뉴를 완성한 뒤 현재 스택에 연결한다.</summary>
    private void HelperMenu(uint token)
    {
        // 실제 현재 메뉴 builder를 시작한다.
        void Begin() => Need(_hooks.MenuBegin)();
        // 현재 표시 BYTE 번역을 매사용 조회한다.
        string Translate(string value) => Need(_hooks.Translate)(value);
        // 실제 일반 항목에 null과 빈 payload를 구별해 전달한다.
        void Item(string label, uint command, string? payload) => Need(_hooks.MenuItem)(label, command, payload);
        // 번역 callback 뒤 현재 사건 전역을 조회한다.
        void TranslatedItem(string label, uint address, string? payload) { string translated = Translate(label); uint command = Global(address); Item(translated, command, payload); }
        // 번역 뒤 실제 현재 메뉴를 pop하고 하위 메뉴에 연결한다.
        void Submenu(string label) { string translated = Translate(label); uint child = Need(_hooks.MenuTake)(); Need(_hooks.MenuSubmenu)(translated, child); }
        // 원문 두Config조회 사이에 번역/현재사건을 캡처한다.
        void Toggle(string key, string label) { int first = Need(_hooks.Config)(key); string assignment = key + (first != 0 ? "=0" : "=1"); string translated = Translate(label); uint command = Global(0x5428ac); int check = Need(_hooks.Config)(key); Need(_hooks.MenuChecked)(unchecked((uint)check), translated, command, assignment); }
        Begin(); Need(_hooks.MenuTitle)(Translate("Chat Helper:"));
        if (Global(0x594fc8) != 0 || Global(0x594fbc) != 0)
        {
            Begin(); TranslatedItem("Edit Back Format", 0x542b08, null); TranslatedItem("Edit Front Format", 0x542b0c, null); Submenu("Format"); Begin(); Toggle("allowzonesounds", "Allow Chat Sound"); Toggle("zonebgcolor", "Allow Background Colors"); Toggle("zonefgcolor", "Allow Foreground Colors"); Toggle("zoneoffset", "Allow Embossed Offset"); Submenu("Chat Effects ");
            if (Global(0x594fbc) != 0) { Begin(); Begin(); Toggle("ShowEveryone", "Show General Channel"); Toggle("ShowAllies", "Show Allies Channel"); Toggle("ShowObserver", "Show Observer Channel"); Toggle("ShowWisper", "Show Whisper Channel"); Submenu("Channel Controls"); Begin(); Toggle("ShowMyAllies", "Show Allies Chat"); Toggle("ShowMyEmenys", "Show Enemies Chat"); Toggle("ShowMyNeutrals", "Show Observers Chat"); Submenu("Player Controls"); Toggle("ShowChatWindow", "Open Window on Message"); Toggle("PlaySoundMessage", "Play sound on Message"); Submenu("Chat Controls"); }
        }
        Begin(); Begin();
        // 실제 목록의 현재 size/마지막 제외와 원문21/23 그룹 수식을 유지한다.
        void Grouped(List<string> list, uint step, string prefix, string title)
        {
            if (list.Count == 0) throw new InvalidOperationException("ChatWindow 원본 helper의 빈 자동완성 벡터 범위"); bool secondGroup = false; uint index = 0;
            // 원문은 매반복 현재 개수를 다시 읽고 마지막 원소를 제외한다.
            for (; index < unchecked((uint)list.Count - 1); index++)
            {
                if (index == 0) Begin(); string value = list[checked((int)index)], label = value + '~' + value; Item(label, Global(0x542b24), value);
                if (index != 0 && index % step == 0) { uint first = unchecked(index - (step == 23 || secondGroup ? step - 1 : step)); secondGroup = true; Submenu(prefix + first.ToString(CultureInfo.InvariantCulture) + " to " + index.ToString(CultureInfo.InvariantCulture)); Begin(); }
            }
            uint size = (uint)list.Count; Submenu(prefix + unchecked(size - (size - 1) % step).ToString(CultureInfo.InvariantCulture) + " to " + unchecked(size - 2).ToString(CultureInfo.InvariantCulture)); Submenu(title); Begin();
        }
        Grouped(_state.Colors, 21, "Colors ", "Text Colors"); Grouped(_state.Backgrounds, 21, "BGColors ", "Background Colors"); Grouped(_state.Icons, 23, "Icons ", "Icons"); Item("Mimic", Global(0x542b24), "~[]"); Item("Sound", Global(0x542b24), "~[S.name]"); Item("Icon", Global(0x542b24), "~[I.name]"); Item("Emboss Offset", Global(0x542b24), "~[E.X#xY#]"); Submenu("KeyWords"); Begin();
        // 원문 여덟 스타일의 고정 삽입 순서다.
        foreach (var (label, value) in Styles) TranslatedItem(label, 0x542b24, value);
        Submenu("Styles"); Begin();
        // 원문 스무 기본 색 항목을 현재 번역/사건으로 추가한다.
        foreach (var (label, value) in Colors) TranslatedItem(label, 0x542b24, value);
        Submenu("Prefabricated Colors"); Begin();
        // 세 원문 명령을 같은 실제 입력 삽입 메뉴에 넣는다.
        foreach (string value in new[] { ".front", ".back", ".format" }) TranslatedItem(value, 0x542b24, value);
        Submenu("Commands"); Submenu("Insert"); TranslatedItem("Copy", 0x542b24, "copy"); TranslatedItem("Paste", 0x542b24, "paste"); Need(_hooks.MenuFinalize)(); Need(_hooks.MenuOpen)(Global(0x5cae94), Global(0x5cae98), Global(0x563e38));
    }
}
