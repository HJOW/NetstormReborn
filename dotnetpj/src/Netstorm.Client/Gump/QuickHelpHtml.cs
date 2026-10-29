using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 기본 HTML/링크와 현재 도움말 주제/동적 정보 pane을 결합한다.

/// <summary>RawPriestMenu.h의 문자열/int32 variant 서식 인자.사제 메뉴 본체의 이식은 후속이다.</summary>
public readonly record struct PriestMenuArgument
{
    /// <summary>문자열 인자일 때 원시 본문.정수 인자는null이다.</summary>
    public string? Text { get; }
    /// <summary>정수 인자의 전체 signed DWORD.</summary>
    public int Number { get; }
    /// <summary>실제 variant의 문자열/정수 분류.</summary>
    public bool IsText => Text is not null;
    /// <summary>실제 두 인자 종류의 저장 값만 설치한다.</summary>
    private PriestMenuArgument(string? text, int number) { Text = text; Number = number; }
    /// <summary>고정%s 슬롯에 문자열 인자를 공급한다.</summary>
    public static implicit operator PriestMenuArgument(string text) => new(text, 0);
    /// <summary>고정%d 슬롯에 원시 signed 정수를 공급한다.</summary>
    public static implicit operator PriestMenuArgument(int number) => new(null, number);
}
/// <summary>실제472e60의 마나 그림 TYPE와 ASCII 프레임 표.</summary>
public sealed class ManaPictureState
{
    /// <summary>현재005411f4의 마나 그림 TYPE.</summary>
    public uint Type { get; set; } = 103;
    /// <summary>알 수 없는 ASCII의 초기0을 보존하는128칸 프레임.</summary>
    public int[] Frames { get; } = new int[128];
    /// <summary>원문11문자의 위치를 프레임0..10으로 설치한다.</summary>
    public ManaPictureState()
    {
        const string letters = "wrtsWRTS.?>"; // 원본00540ad8의 실제 문자 순서다.
        // 문자 위치를 같은 실제 전역 칸에 쓴다.
        for (int i = 0; i < letters.Length; i++) Frames[letters[i]] = i;
    }
}
/// <summary>현재 TYPE 정보.공급자가 같은 실제 자산/월드 저장소의 수명을 유지해야 한다.</summary>
public sealed class QuickHelpTypeInfo
{
    /// <summary>+98 원소 번호.5이상은 None 문구다.</summary>
    public int Theme { get; set; }
    /// <summary>+e8 HP 가능 비트.</summary>
    public uint Flags1 { get; set; }
    /// <summary>+ec 특수 피해/마나 표시 비트.</summary>
    public uint Flags2 { get; set; }
    /// <summary>+44 현재 사거리.</summary>
    public float Range { get; set; }
    /// <summary>+48 현재 hpPerSec 값.</summary>
    public float Damage { get; set; }
    /// <summary>+c4 현재 SP 비용.</summary>
    public float Cost { get; set; }
    /// <summary>+aa 원문 클래스.</summary>
    public string Classification { get; set; } = "";
    /// <summary>+a0 후처리된 요구 에너지 문자열.</summary>
    public string Mana { get; set; } = "";
}
/// <summary>다중 줄 해석에서 공유하는 원본00565ec4의 실제 현재 전역.</summary>
public sealed class QuickHelpHtmlState
{
    /// <summary>중첩 정보 해석 전0/정상 복귀 뒤1로 쓰는 모드.</summary>
    public uint InfoMode { get; set; }
}
/// <summary>현재 TYPE/번역/Config와 같은 실제 pane·파일 효과를 공급한다.</summary>
public sealed class QuickHelpHtmlHooks
{
    /// <summary>각 실제 읽기 지점에서 다시 조회하는 같은 현재 TYPE.</summary>
    public Func<QuickHelpTypeInfo>? Type { get; set; }
    /// <summary>현재 타입 설명의 실제49af80.</summary>
    public Func<string>? Description { get; set; }
    /// <summary>현재 사제1/4 모드도 포함하는 실제49fce0.</summary>
    public Func<int>? HitPoints { get; set; }
    /// <summary>실제 번역 조회.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>현재 Config의1024BYTE 치환.</summary>
    public Func<string, string>? Substitute { get; set; }
    /// <summary>현재 influence 등 용어 조회.</summary>
    public Func<string, string>? Config { get; set; }
    /// <summary>추가 번역 없이 실제 고정%s/%d 서식을 처리한다.</summary>
    public Func<string, IReadOnlyList<PriestMenuArgument>, string>? Print { get; set; }
    /// <summary>같은 현재 정보 pane의 실제 기본 필드.</summary>
    public Func<GumpCreateNode?>? Pane { get; set; }
    /// <summary>같은 실제 정보 pane의 styled 본문 입력.</summary>
    public Action<string>? PaneText { get; set; }
    /// <summary>실제 파일 스트림의 원문 BYTE 기록.</summary>
    public Action<string>? Stream { get; set; }
    /// <summary>같은 pane의 절대 스크롤 요청.</summary>
    public Action<int>? PaneScroll { get; set; }
    /// <summary>같은 pane의 가상 Push.</summary>
    public Action? PanePush { get; set; }
    /// <summary>같은 pane의419df0 현재 위치 재부착.</summary>
    public Action? PaneAttach { get; set; }
}
/// <summary>cpppj/src/client/QuickHelpHtml.h/.cpp의 마나/전용 주제/동적 정보/중첩 HTML 순서.</summary>
public sealed class QuickHelpHtml
{
    /// <summary>실제542444의 첫5개 원소 이름.</summary>
    private static readonly string[] Themes = ["Wind", "Rain", "Thunder", "Sun", "Energy"];
    /// <summary>같은 기본 파서 객체.</summary>
    private readonly HtmlTextParserState _state;
    /// <summary>현재 단락 억제/배경 전역.</summary>
    private readonly HtmlTextParserGlobals _globals;
    /// <summary>실제 다중 줄 해석 모드 전역.</summary>
    private readonly QuickHelpHtmlState _mode;
    /// <summary>생성 때 조회한 원래 TYPE 번호.</summary>
    private readonly uint _type;
    /// <summary>첫 NUL 앞의 원래 주제.비교 길이도 이 값이다.</summary>
    private readonly string _topic;
    /// <summary>실제 파일 스트림 유무.</summary>
    private readonly bool _stream;
    /// <summary>생성 인자로 복사한 실제 마나 그림 표.</summary>
    private readonly ManaPictureState _mana;
    /// <summary>현재 정보/번역/파일 장치.</summary>
    private readonly QuickHelpHtmlHooks _hooks;
    /// <summary>같은 기본 명령/조건/그림/링크/배경 슬롯.</summary>
    private readonly HtmlTextParserHooks _base;
    /// <summary>중첩과 부모가 공유하는 실제 링크 BYTE.</summary>
    private readonly HtmlLinkTextState? _links;
    /// <summary>선택된 실제 장치만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("전용 Help HTML 실제 효과 누락");
    /// <summary>원문 C문자열의 첫 NUL 뒤는 제외한다.</summary>
    private static string CText(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>원본 고정 지역 버퍼의 종료 NUL 공간까지 확인한다.</summary>
    private static string Text(string text, int capacity) { text = CText(text); if (text.Length >= capacity) throw new ArgumentException("전용 Help HTML 원본 버퍼 초과"); return text; }
    /// <summary>원시 ASCII 소문자만 바꾸며 문화권/비ASCII는 정규화하지 않는다.</summary>
    private static string Lower(string text) => string.Concat(text.Select(item => item is >= 'A' and <= 'Z' ? (char)(item + 'a' - 'A') : item));
    /// <summary>호스트 string_view의 짧은 접두부를 동일하게 빌린다.</summary>
    private static string Part(string source, int offset, int length) { if ((uint)offset > (uint)source.Length) throw new ArgumentOutOfRangeException(nameof(offset)); return source.Substring(offset, Math.Min(length, source.Length - offset)); }
    /// <summary>기본 생성 상태를 설치한 뒤 현재 TYPE 조회→shown0 순서를 유지한다.</summary>
    public QuickHelpHtml(HtmlTextParserState state, HtmlTextParserGlobals globals, QuickHelpHtmlState mode, uint type, string topic, bool stream, ManaPictureState mana, QuickHelpHtmlHooks hooks, HtmlTextParserHooks? basic = null, HtmlLinkTextState? links = null)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(mode); ArgumentNullException.ThrowIfNull(topic); ArgumentNullException.ThrowIfNull(mana); ArgumentNullException.ThrowIfNull(hooks); basic ??= new(); if (basic.Link is not null && links is null) throw new ArgumentException("Help 중첩 HTML의 같은 링크 저장소 누락"); _state = state; _globals = globals; _mode = mode; _type = type; _topic = CText(topic); _stream = stream; _mana = new() { Type = mana.Type }; mana.Frames.CopyTo(_mana.Frames, 0); _hooks = hooks; _base = new() { Command = basic.Command, Condition = basic.Condition, Picture = basic.Picture, Link = basic.Link, Background = basic.Background }; _links = links;
        state.InputOffset = 0; state.Output = ""; state.Shown = 1; state.HiddenDepth = state.InTag = state.TextAllowed = state.AfterHeading = 0; if (type != 0) _ = Require(hooks.Type)(); state.Shown = 0;
    }
    /// <summary>점은 건너뛰고 출력 아이콘 인덱스가 강조 범위 안이면 프레임에11을 더한다.</summary>
    public static string ManaPictureText(string mana, ManaPictureState state, int highlighted = -1)
    {
        mana = CText(mana); if (mana.Length >= 256) throw new ArgumentException("원본 마나 임시 버퍼256BYTE 초과"); string result = ""; int cursor = 0, index = 0;
        // 점은 출력 개수에 포함하지 않고 실제 비점 문자마다 아이콘 하나를 만든다.
        while (cursor < mana.Length)
        {
            // 연속 점을 다음 비점 문자 앞까지 소비한다.
            while (cursor < mana.Length && mana[cursor] == '.') cursor++; if (cursor == mana.Length) break; char letter = mana[cursor++]; if (letter >= state.Frames.Length) throw new ArgumentOutOfRangeException(nameof(mana), "마나 비ASCII 전역 조회 미복원"); int frame = unchecked(state.Frames[letter] + (index < highlighted ? 11 : 0)); result += "~[I" + state.Type + '.' + frame + ']'; index++;
        }
        if (result.Length >= 250) throw new ArgumentException("원본 마나 그림 문자열250BYTE 초과"); return result;
    }
    /// <summary>현재 입력 바로 앞 LF/CR만 textAllowed0으로 만들고 shown은 유지한다.</summary>
    public static void QuickHelpBeforeClose(HtmlTextParserState state, string source) { if (state.InputOffset <= 0 || state.InputOffset > source.Length) throw new ArgumentOutOfRangeException(nameof(source), "Help 닫힘 슬롯 현재 입력 범위"); if (source[state.InputOffset - 1] is '\n' or '\r') state.TextAllowed = 0; }
    /// <summary>번역은 호출 위치에서 수행하며 서식기는 추가 번역하지 않는다.</summary>
    private string Print(string pattern, PriestMenuArgument[] arguments, int capacity = 256) => Text(Require(_hooks.Print)(pattern, arguments), capacity);
    /// <summary>제목/원소/클래스/HP/거리/초당 피해/SP/마나를 각 실제 시점의 현재 TYPE로 만든다.</summary>
    private void Info()
    {
        // 각 호출 때 같은 현재 TYPE와 번역 결과를 다시 읽는다.
        QuickHelpTypeInfo Current() => Require(_hooks.Type)();
        // 원문 번역 장치 호출 시점을 그대로 유지한다.
        string Translate(string value) => Require(_hooks.Translate)(value);
        string heading = Print("<h2>%s</h2>", [Require(_hooks.Description)()]), alignment;
        if (Current().Theme < 5) { string pattern = Translate("Alignment: <c>%s</c>"); int theme = Current().Theme; if (theme < 0 || theme >= 5) throw new ArgumentOutOfRangeException(nameof(theme), "Help 원소 이름표 범위"); alignment = Print(pattern, [Translate(Themes[theme])]); } else alignment = Print(Translate("Alignment: <c>None</c>"), []);
        string classification = ""; if (CText(Current().Classification).Length != 0) { string pattern = Translate("Class: <c>%s</c>"), label = Translate(CText(Current().Classification)); classification = Print(pattern, [Text(Require(_hooks.Substitute)(label), 1024)]); }
        string hits; if ((Current().Flags1 & 0x10) != 0 && Require(_hooks.HitPoints)() != 0) { string pattern = Translate("Hits: <c>%d</c>"); hits = Print(pattern, [Require(_hooks.HitPoints)()]); } else hits = Print(Translate("Hits: <c>Invulnerable</c>"), []);
        // 번역 뒤 현재 float을 다시 읽고 원문 signed64 절삭의 하위 DWORD를 사용한다.
        string Number(string numeric, string empty, Func<float> value) { if (value() == 0) return Print(Translate(empty), []); string pattern = Translate(numeric); return Print(pattern, [GumpCollection.Ftol(value())]); }
        string range = Number("Range: <c>%d</c>", "Range: <c>n/a</c>", () => Current().Range), damage;
        if (Current().Damage == 0) damage = Print(Translate((Current().Flags2 & 0x100) != 0 ? "Damage: <c>special</c>" : "Damage: <c>n/a</c>"), []); else { string pattern = Translate("Damage: <c>%d</c>"); damage = Print(pattern, [GumpCollection.Ftol(Current().Damage)]); }
        string cost = Number("Cost in Storm Power: <c>%d</c>~[IfortGump.3]", "Cost in Storm Power: <c>n/a</c>", () => Current().Cost), mana;
        if (CText(Current().Mana).Length == 0) { string pattern = Translate((Current().Flags2 & 0x100) != 0 ? "%s to Build: <c>n/a</c>" : "%s to Build: <c>None</c>"); mana = Print(pattern, [Require(_hooks.Config)("influence")]); }
        else { string letters = Text(Current().Mana, 256); letters = string.Concat(letters.Select(letter => letter is >= 'a' and <= 'z' ? (char)(letter - 'a' + 'A') : letter)); string pictures = ManaPictureText(letters, _mana), pattern = Translate("%s to Build: <c>%s</c>"); mana = Print(pattern, [Require(_hooks.Config)("influence"), pictures]); }
        string html = Print("%s%s<br>%s<br>%s<br>%s<br>%s<br>%s<br>%s<br>", [heading, alignment, classification, hits, range, damage, cost, mana], 1024); if (_stream) Require(_hooks.Stream)(html); int cursor = _links?.Cursor ?? 0; _mode.InfoMode = 0; string styled = new HtmlTextParser(new(), _globals, _base).Parse(html, 1024); _mode.InfoMode = 1; if (_links is not null) _links.Cursor = cursor; Require(_hooks.PaneText)(styled); Require(_hooks.PaneScroll)(0); var pane = Require(_hooks.Pane)() ?? throw new ArgumentOutOfRangeException(nameof(_hooks.Pane), "Help 현재 정보 pane 누락"); if ((pane.Flags & 8) == 0) { Require(_hooks.PanePush)(); Require(_hooks.PaneAttach)(); }
    }
    /// <summary>소문자n에서 name을 검사하며 주제 접두 일치/현재info 처리 후 진입 커서를 복구한다.</summary>
    private void Inside(HtmlTextParserState state, string source, int capacity)
    {
        int entry = state.InputOffset, position = entry; if (position < source.Length && source[position] == 'n' && Lower(Part(source, position, 4)) == "name")
        {
            // 속성의=나 공백 해석 없이 첫 따옴표/NUL까지 찾는다.
            while (position < source.Length && source[position] != '"') position++; if (position < source.Length) position++; if (Lower(Part(source, position, _topic.Length)) == Lower(_topic)) { state.Shown = state.TextAllowed = 1; if ((long)state.Output.Length + 4 >= capacity) throw new ArgumentException("Help name 스타일 출력 범위 초과"); state.Output += "~5~w"; position += _topic.Length; }
        }
        if (Part(source, position, 4) == "info" && _type != 0 && state.Shown != 0) Info(); state.InputOffset = entry;
    }
    /// <summary>본문 파서 커서는 바꾸지 않고 다음 평문/태그를 원문 BYTE 단위로 파일에 쓴다.</summary>
    private void After(HtmlTextParserState state, string source)
    {
        if (!_stream) return; int position = state.InputOffset + 1;
        // 다음 태그 앞의 CR/LF와 일반 BYTE도 그대로 기록한다.
        while (position < source.Length && source[position] != '<') { Require(_hooks.Stream)(source.Substring(position, 1)); position++; }
        // 다음 태그의 첫 닫힘 꺾쇠까지 기록하고 NUL은 기록하지 않는다.
        while (position < source.Length && (position == 0 || source[position - 1] != '>')) { Require(_hooks.Stream)(source.Substring(position, 1)); position++; }
    }
    /// <summary>기본 효과/같은 링크 소유자를 보존하고 세 파생 가상 슬롯만 전용 도움말로 연결한다.</summary>
    public string Parse(string source, int capacity = 8192) { var hooks = new HtmlTextParserHooks { Command = _base.Command, Condition = _base.Condition, Picture = _base.Picture, Link = _base.Link, Background = _base.Background, InsideTag = (state, text) => Inside(state, text, capacity), BeforeClose = QuickHelpBeforeClose, AfterTag = After }; return new HtmlTextParser(_state, _globals, hooks).Parse(source, capacity); }
}
