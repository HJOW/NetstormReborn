using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 HTML BYTE를 같은 StyleText 입력/현재 상태로 바꾸는 영역이다.

/// <summary>원본+1c..2c와 입력/출력 포인터가 가리키는 실제 현재 상태.</summary>
public sealed class HtmlTextParserState
{
    /// <summary>현재 평문/태그 효과를 표시하는 DWORD.</summary>
    public uint Shown { get; set; } = 1;
    /// <summary>닫힘 조건이 되돌리는 숨김 깊이.</summary>
    public uint HiddenDepth { get; set; }
    /// <summary>꺾쇠 사이 태그 본문 상태.</summary>
    public uint InTag { get; set; }
    /// <summary>소문자 a 열림/닫힘이 증감하는 signed 판정 카운터.</summary>
    public uint TextAllowed { get; set; }
    /// <summary>제목 닫힘 뒤 공백만 출력한 상태.</summary>
    public uint AfterHeading { get; set; }
    /// <summary>콜백도 변경할 수 있는 현재 원시 입력 BYTE 위치.</summary>
    public int InputOffset { get; set; }
    /// <summary>실제 StyleText 제어문자와 평문을 Latin1으로 보존한 출력.종료 NUL은 길이에서 제외한다.</summary>
    public string Output { get; set; } = "";
}
/// <summary>같은 미션/Tell/페이지가 공유하는 전역이다.</summary>
public sealed class HtmlTextParserGlobals
{
    /// <summary>다음 P 출력을 억제한 뒤0으로 돌아오는 DWORD.</summary>
    public uint SuppressParagraph { get; set; }
    /// <summary>짧은 덮어쓰기 뒤 나머지 BYTE도 유지하는 실제80BYTE 배경 이름.</summary>
    public byte[] Background { get; } = new byte[80];
}
/// <summary>선택된 실제 명령/조건/그림/링크/배경과 파생 가상 슬롯의 외부 효과.</summary>
public sealed class HtmlTextParserHooks
{
    /// <summary>$ 다음부터 입력 NUL까지의 원시 명령.</summary>
    public Action<HtmlTextParserState, string>? Command { get; set; }
    /// <summary>?부터 입력 NUL까지의 조건 원문/닫힘과 현재 가림 상태.</summary>
    public Action<HtmlTextParserState, string, bool>? Condition { get; set; }
    /// <summary>!부터의 원문을 실제 그림 제어문자로 바꾸는 helper.</summary>
    public Func<HtmlTextParserState, string, string>? Picture { get; set; }
    /// <summary>실제 T태그/링크 저장소 효과.닫힘은 빈 원문이다.</summary>
    public Func<HtmlTextParserState, string, bool, string>? Link { get; set; }
    /// <summary>이미 추출한 원문 배경 이름을 같은 전역에 쓴다.</summary>
    public Action<HtmlTextParserState, string>? Background { get; set; }
    /// <summary>태그 내부 일반 BYTE의 virtual0.기본 VT는 noop다.</summary>
    public Action<HtmlTextParserState, string>? InsideTag { get; set; }
    /// <summary>닫힘에서 slash로 옮긴 뒤의 virtual1.기본 VT는 noop다.</summary>
    public Action<HtmlTextParserState, string>? BeforeClose { get; set; }
    /// <summary>꺾쇠 닫힘/inTag0 뒤의 virtual2.기본 VT는 상태를 바꾸지 않는다.</summary>
    public Action<HtmlTextParserState, string>? AfterTag { get; set; }
}
/// <summary>cpppj/src/client/HtmlTextParser.h/.cpp의 기본 변환과 현재 콜백 순서.</summary>
public sealed class HtmlTextParser
{
    /// <summary>평문 반복이 태그/마지막 LF를 위해 남기는 논리 여유.</summary>
    private const int OutputReserve = 256;
    /// <summary>같은 현재 파서 상태.</summary>
    private readonly HtmlTextParserState _state;
    /// <summary>같은 현재 P 억제/배경 전역.</summary>
    private readonly HtmlTextParserGlobals _globals;
    /// <summary>실제 선택 효과/파생 슬롯.</summary>
    private readonly HtmlTextParserHooks _hooks;
    /// <summary>원본 H 글꼴 등 실제 판본 차이.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>현재 상태/장기 전역만 빌리고 외부 효과는 실제 분기에서 요구한다.</summary>
    public HtmlTextParser(HtmlTextParserState state, HtmlTextParserGlobals globals, HtmlTextParserHooks? hooks = null, OriginalEdition edition = OriginalEdition.Patch1078) { ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(globals); _state = state; _globals = globals; _hooks = hooks ?? new(); _edition = edition; }
    /// <summary>첫 NUL 앞의 원시 문자열만 남긴다.</summary>
    private static string BeforeNul(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>호스트 범위 밖은 원본 NUL 문자열 끝의 가상0이다.</summary>
    private static char Byte(string source, int offset) => (uint)offset < (uint)source.Length ? source[offset] : '\0';
    /// <summary>선택된 실제 효과만 요구한다.</summary>
    private static T Require<T>(T? hook, string name) where T : Delegate => hook ?? throw new InvalidOperationException("HTML 실제 효과 미연결: " + name);
    /// <summary>물리 버퍼의 마지막 NUL 공간도 포함하여 출력을 추가한다.</summary>
    private void Append(string text, int capacity) { if ((long)_state.Output.Length + text.Length >= capacity) throw new InvalidOperationException("HTML 원본 물리 출력 버퍼 초과"); _state.Output += text; }
    /// <summary>콜백 뒤 현재 입력/출력 포인터가 같은 물리 버퍼 범위인지 확인한다.</summary>
    private void Validate(string source, int capacity) { if ((uint)_state.InputOffset > (uint)source.Length || _state.Output.Length >= capacity) throw new ArgumentOutOfRangeException(nameof(source), "HTML callback 현재 버퍼 위치 오류"); }
    /// <summary>인용 전/안의 꺾쇠와 첫 NUL에서 끝내며 이름/프레임 BYTE를 정규화하지 않는다.</summary>
    public static string HtmlPictureText(string raw)
    {
        raw = BeforeNul(raw); int position = 0;
        // 속성 해석 없이 첫 따옴표/꺾쇠만 찾는다.
        while (position < raw.Length && raw[position] is not '>' and not '"') position++;
        string result = "~[I"; if (position < raw.Length && raw[position] == '"')
        {
            int start = ++position;
            // 인용 내부에서도 첫 따옴표/꺾쇠에서 멈춘다.
            while (position < raw.Length && raw[position] is not '>' and not '"') position++;
            result += raw[start..position];
        }
        return result + ']';
    }
    /// <summary>기본 그림 helper를 연결하고 자산 해석/출력은 기존 StyleTextMarkup에 맡긴다.</summary>
    public static void BindHtmlPictureText(HtmlTextParserHooks hooks) { ArgumentNullException.ThrowIfNull(hooks); hooks.Picture = (_, raw) => HtmlPictureText(raw); }
    /// <summary>이름 뒤 NUL만 쓰고80BYTE의 나머지를 지우지 않는 실제 전역 저장을 연결한다.</summary>
    public static void BindHtmlBackground(HtmlTextParserHooks hooks, HtmlTextParserGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(globals); hooks.Background = (_, name) => { name = BeforeNul(name); if (name.Length >= globals.Background.Length) throw new ArgumentException("HTML 배경80BYTE NUL 범위 초과");
            // 원시 Latin1 BYTE를 같은 장기 전역의 접두부에만 덮어쓴다.
            for (int i = 0; i < name.Length; i++) globals.Background[i] = checked((byte)name[i]); globals.Background[name.Length] = 0; };
    }
    /// <summary>같은80BYTE 저장소의 현재 첫 NUL 앞 이름을 읽는다.</summary>
    public static string HtmlBackgroundName(HtmlTextParserGlobals globals) { int end = Array.IndexOf(globals.Background, (byte)0); if (end < 0) throw new ArgumentException("HTML 배경80BYTE NUL 없음"); return System.Text.Encoding.Latin1.GetString(globals.Background, 0, end); }
    /// <summary>첫 BYTE만 ASCII 소문자로 분기하며 후속 문자는 원문 literal 비교를 유지한다.</summary>
    private void Tag(string source, int offset, bool closing, int capacity)
    {
        int raw = unchecked((sbyte)Byte(source, offset)), first = raw is >= 'A' and <= 'Z' ? raw + 'a' - 'A' : raw; char second = Byte(source, offset + 1); string remaining = source[Math.Min(offset, source.Length)..], result = "";
        switch (first)
        {
            case '!': Append(Require(_hooks.Picture, "picture")(_state, remaining), capacity); break;
            case '$': Require(_hooks.Command, "command")(_state, remaining[1..]); break;
            case '?': Require(_hooks.Condition, "condition")(_state, remaining, closing); break;
            case 'a': if (closing) { Append(Require(_hooks.Link, "link-close")(_state, "", true), capacity); result = "~N~."; } else if (Byte(source, offset + 2) == 'h') { Append(Require(_hooks.Link, "link-open")(_state, remaining, false), capacity); result = "~l~E"; } break;
            case 'b':
                if (second == 'r') { result = "\n"; _state.AfterHeading = 0; }
                else if (second == 'a')
                {
                    int end = remaining.IndexOf('>'), equal = remaining.IndexOf('='); if (equal >= 0 && (end < 0 || equal < end))
                    {
                        int start = equal + 1; if (Byte(remaining, start) == '"') start++; int finish = start;
                        // 배경 이름은 공백/대소문자를 유지하고 따옴표/꺾쇠에서만 끝난다.
                        while (finish < remaining.Length && remaining[finish] is not '>' and not '"') finish++;
                        Require(_hooks.Background, "background")(_state, remaining[start..finish]);
                    }
                }
                else result = closing ? "~N" : "~N~B"; break;
            case 'c': result = closing ? "~." : "~N~i"; break;
            case 'h': if (closing) { result = _edition == OriginalEdition.Cd1072 && second == '1' ? "~*~N~.\n \n" : "~*~N~.\n"; _state.AfterHeading = 1; } else if (second == '3') result = "~N~5~B~E"; else if (_edition == OriginalEdition.Cd1072) result = "~N~3~E"; else if (second == '1') result = "~N~4~E"; else if (second == '2') result = "~N~3~E"; else result = "~N~1~E"; break;
            case 'i': result = closing ? "~." : "~N~j"; break;
            case 'p': if (_globals.SuppressParagraph != 0) _globals.SuppressParagraph = 0; else { result = _state.AfterHeading != 0 ? "~*\n" : "~*\n \n"; _state.AfterHeading = 0; } break;
            case 'q': result = closing ? "~." : "~N~q"; break;
            case '~': Append("~", capacity); if (second != 0) Append(source.Substring(offset + 1, 1), capacity); break;
        }
        Validate(source, capacity); Append(result, capacity);
    }
    /// <summary>Latin1 원시 BYTE/NUL 입력을 변환한다.기본 VT noop만 제공하고 파생 동적 본문은 현재 콜백에 맡긴다.</summary>
    public string Parse(string source, int capacity = 8192)
    {
        ArgumentNullException.ThrowIfNull(source); if (capacity < 2) throw new ArgumentException("HTML 마지막 LF/NUL의 물리 버퍼 필요"); source = BeforeNul(source); _state.InputOffset = 0; _state.Output = ""; _state.AfterHeading = 0; _state.TextAllowed = _state.Shown; _state.InTag = 0;
        // 태그 전체를 건너뛰지 않고 매 BYTE에서 콜백 뒤 현재 커서를 다시 읽는다.
        while (Byte(source, _state.InputOffset) != 0)
        {
            char item = Byte(source, _state.InputOffset);
            if (item == '<')
            {
                _state.InTag = 1; if (_state.HiddenDepth == 0 && Byte(source, _state.InputOffset + 1) == 'a') _state.TextAllowed = unchecked(_state.TextAllowed + 1); bool closing = Byte(source, _state.InputOffset + 1) == '/';
                if (_state.Shown != 0 || (_state.HiddenDepth != 0 && closing && Byte(source, _state.InputOffset + 2) == '?'))
                {
                    if (closing) { _state.InputOffset++; if (Byte(source, _state.InputOffset + 1) == 'a') _state.TextAllowed = unchecked(_state.TextAllowed - 1); _hooks.BeforeClose?.Invoke(_state, source); Validate(source, capacity); }
                    Tag(source, _state.InputOffset + 1, closing, capacity);
                }
            }
            else if (item == '>') { _state.InTag = 0; if (_state.Shown != 0) _hooks.AfterTag?.Invoke(_state, source); }
            else if (_state.InTag == 0)
            {
                if (_state.Shown != 0)
                {
                    if (unchecked((int)_state.TextAllowed) < 1 || (long)_state.Output.Length > (long)capacity - OutputReserve) break;
                    if (item > 0x1f) { Append(source.Substring(_state.InputOffset, 1), capacity); if (_state.AfterHeading != 0) _state.AfterHeading = Byte(source, _state.InputOffset) == ' ' ? 1U : 0; }
                    if (Byte(source, _state.InputOffset) == '\n' && _state.Output.Length != 0 && _state.Output[^1] is not ' ' and not '\n') Append(" ", capacity);
                }
            }
            else _hooks.InsideTag?.Invoke(_state, source);
            Validate(source, capacity); _state.InputOffset++;
        }
        Append("\n", capacity); return _state.Output;
    }
}
