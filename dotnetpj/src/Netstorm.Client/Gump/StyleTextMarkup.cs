using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 대괄호 명령의 부분 검색·그림 선택·옵션 처리 영역이다.

/// <summary>원본 zone 허용 전역과 그림 맞춤 크기.0/비0 값을 그대로 보관한다.</summary>
public sealed class StyleTextMarkupState
{
    /// <summary>일반 S 명령 소리 허용.</summary>
    public int AllowSounds { get; set; } = 1;
    /// <summary>배경색 명령 허용.</summary>
    public int AllowBackgroundColor { get; set; } = 1;
    /// <summary>전경색 명령 허용.</summary>
    public int AllowForegroundColor { get; set; } = 1;
    /// <summary>그림자 오프셋 허용.</summary>
    public int AllowOffset { get; set; } = 1;
    /// <summary>그림 맞춤 폭.</summary>
    public int FitWidth { get; set; } = 65;
    /// <summary>그림 맞춤 높이.</summary>
    public int FitHeight { get; set; } = 60;
}
/// <summary>원본 타입의 논리 수/특수 프레임/글자 첫 인덱스.각 Type 조회는 값 복사한다.</summary>
public sealed class StyleTextMarkupType
{
    /// <summary>논리 프레임 수.</summary>
    public int FrameCount { get; set; }
    /// <summary>기본 프레임.</summary>
    public int DefaultFrame { get; set; }
    /// <summary>Gump 프레임.-1일 때 기본 프레임을 사용한다.</summary>
    public int GumpFrame { get; set; }
    /// <summary>허용되는 A부터의 글자 종류 수.</summary>
    public int LetterKinds { get; set; }
    /// <summary>A~P 첫 프레임.없는 글자는-1이다.</summary>
    public int[] First { get; } = Enumerable.Repeat(-1, 16).ToArray();
    /// <summary>C++의 배열을 포함한 값 반환과 같은 독립 사본.</summary>
    internal StyleTextMarkupType Copy()
    { var result = new StyleTextMarkupType { FrameCount = FrameCount, DefaultFrame = DefaultFrame, GumpFrame = GumpFrame, LetterKinds = LetterKinds }; First.CopyTo(result.First, 0); return result; }
}
/// <summary>현재 타입 이름/선택 필드/실제 SHP 높이와 검정색/이름 소리의 경계.</summary>
public sealed class StyleTextMarkupHooks
{
    /// <summary>원본 이름 바이트의 일대일 문자열 검색.-1은 실패다.</summary>
    public Func<string, int>? FindType { get; set; }
    /// <summary>현재 타입 선택 필드.처리기는 각 반환을 값 복사한다.</summary>
    public Func<int, StyleTextMarkupType>? Type { get; set; }
    /// <summary>현재 팔레트 검정 인덱스.</summary>
    public Func<uint>? Black { get; set; }
    /// <summary>현재 이름 소리 요청.</summary>
    public Action<string>? Sound { get; set; }
    /// <summary>내장 슬롯 포함 현재 signed 타입 수.</summary>
    public Func<int>? TypeCount { get; set; }
    /// <summary>현재 논리 프레임의 signed SHP 높이.무자산이면 null.</summary>
    public Func<int, int, short?>? FrameHeight { get; set; }
}
/// <summary>cpppj/src/client/StyleTextMarkup.cpp의 제한 정규식 검색과 판본별 명령·그림 선택을 같은 조각 저장소에 연결한다.</summary>
public sealed class StyleTextMarkup
{
    /// <summary>원본 Regexp Get의 최대 캡처 바이트.</summary>
    private const int CaptureBytes = 80;
    /// <summary>패치 명령 우선순위.한 줄 CD는 I/T만 사용한다.</summary>
    private const string CommandOrder = "ISTCBE";
    /// <summary>CD 다중 줄은 W의 두 캡처를 요구한다.</summary>
    private const string CdScrollingOrder = "IWT";
    /// <summary>같은 실제 공통 조각 표.</summary>
    private readonly StyleTextState _fragments;
    /// <summary>같은 실제 그림 처리기.</summary>
    private readonly Markup _pictures;
    /// <summary>현재 허용/맞춤 전역.</summary>
    private readonly StyleTextMarkupState _state;
    /// <summary>현재 타입/팔레트/소리 경계.</summary>
    private readonly StyleTextMarkupHooks _hooks;
    /// <summary>명령/그림 재조회 분기를 고르는 판본.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>실제 도달 시에만 필수 경계를 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("스타일 대괄호 현재 자산/효과 경계 필요");
    /// <summary>첫NUL 앞 원본 BYTE를 UTF-8 재해석 없이 일대일 문자열로 복사한다.</summary>
    private static string Source(ReadOnlySpan<byte> bytes) { int end = bytes.IndexOf((byte)0); return Encoding.Latin1.GetString(end < 0 ? bytes : bytes[..end]); }
    /// <summary>캡처 앞80바이트만 실제 이름/숫자 해석에 쓴다.</summary>
    private static string Capture(string text) => text[..Math.Min(text.Length, CaptureBytes)];
    /// <summary>기존 CRT 공백/부호/십진수/DWORD 감김 복원을 공유한다.</summary>
    private static int Number(string text) => ConfigNumbers.ParseLong(Capture(text));
    /// <summary>원본 제외 클래스는 점/CR/LF/space만 제외하며 탭은 허용한다.</summary>
    private static bool IsToken(char value) => value is not ('.' or '\r' or '\n' or ' ');
    /// <summary>ASCII space의 가장 긴 끝을 구한다.</summary>
    private static int Spaces(string text, int position)
    {
        // 일반 공백 문자 분류를 쓰지 않는다.
        while (position < text.Length && text[position] == ' ') position++; return position;
    }
    /// <summary>원본 캡처의 가장 긴 끝을 구한다.</summary>
    private static int TokenEnd(string text, int position)
    {
        // 탭/비ASCII를 포함한 제외 클래스의 연속 구간이다.
        while (position < text.Length && IsToken(text[position])) position++; return position;
    }
    /// <summary>원본의 이스케이프되지 않은 점은 임의 한BYTE다.앞 greedy space를 되돌리며 캡처한다.</summary>
    private static string? Separated(string text, int position)
    {
        int separator = Spaces(text, position);
        // 가장 긴 공백부터 역순으로 시도한다.점은 CR/LF/space도 소비한다.
        while (true) { if (separator < text.Length) { int begin = Spaces(text, separator + 1), end = TokenEnd(text, begin); if (end > begin) return text[begin..end]; } if (separator == position) return null; separator--; }
    }
    /// <summary>전체 일치 대신 첫 부분 일치를 찾으며 두 캡처는 첫 캡처 길이를 역순으로 줄인다.</summary>
    private static (string First, string Second)? Match(string text, char command)
    {
        // 앵커 없는 원본 정규식처럼 모든 시작 위치를 순서대로 검사한다.
        for (int mark = 0; mark < text.Length; mark++)
        {
            if (text[mark] != command) continue; int begin = Spaces(text, mark + 1);
            if (command == 'T')
            {
                int end = begin;
                // T 캡처는 부호 없는 ASCII 숫자만 허용한다.
                while (end < text.Length && text[end] is >= '0' and <= '9') end++; if (end > begin) return (text[begin..end], "");
            }
            else if (command is 'I' or 'E' or 'W')
            {
                int end = TokenEnd(text, begin);
                // greedy 첫 캡처를 줄여 처음 성립하는 둘째 캡처를 고른다.
                while (end > begin) { if (Separated(text, end) is { } second) return (text[begin..end], second); end--; }
            }
            else if (Separated(text, mark + 1) is { } first) return (first, "");
        }
        return null;
    }
    /// <summary>같은 조각/그림/현재 옵션 경계를 빌린다.</summary>
    public StyleTextMarkup(StyleTextState fragments, Markup pictures, StyleTextMarkupState state, StyleTextMarkupHooks hooks, OriginalEdition edition = OriginalEdition.Patch1078)
    { ArgumentNullException.ThrowIfNull(fragments); ArgumentNullException.ThrowIfNull(pictures); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); _fragments = fragments; _pictures = pictures; _state = state; _hooks = hooks; _edition = edition; }
    /// <summary>각 가상 조회 결과의 배열까지 값 복사한다.타입 번호 범위는 실제 공급 경계가 검사한다.</summary>
    private StyleTextMarkupType Type(int type) => Require(_hooks.Type)(type).Copy();
    /// <summary>I 명령의 현재 타입/프레임 선택과 count까지의 원본 클램프를 적용하고 공통 체인에 붙인다.</summary>
    private void Picture(string typeExpression, string frameExpression, ref uint head, uint metadata)
    {
        string name = Capture(typeExpression), expression = Capture(frameExpression); if (expression.Length >= CaptureBytes) throw new ArgumentException("스타일 그림 프레임 지역 배열 범위 초과"); int type = Number(name); if (type == 0) type = Require(_hooks.FindType)(name); if (type == -1) return;
        char first = expression[0]; int frame = 0; bool fit = false, mapped = false, patch = _edition == OriginalEdition.Patch1078;
        if (first is '*' or '#' or '&') { fit = first is '#' or '&'; mapped = first == '&'; int gump = Type(type).GumpFrame; frame = patch ? (gump == -1 ? Type(type).DefaultFrame : Type(type).GumpFrame) : (gump == -1 ? Type(type).DefaultFrame : gump); }
        else
        {
            string suffix = expression; int letter = unchecked((sbyte)(byte)first);
            if (letter >= 'A') { var view = Type(type); if (view.LetterKinds is < 0 or > 16) throw new ArgumentOutOfRangeException("LetterKinds"); if (letter < 'A' + view.LetterKinds) { frame = patch ? Type(type).First[letter - 'A'] : view.First[letter - 'A']; suffix = suffix[1..]; } } frame = unchecked(frame + Number(suffix));
        }
        int count = Type(type).FrameCount; if (Math.Min(frame, count) < 0) frame = 0; else if (patch) { if (frame >= Type(type).FrameCount) frame = Type(type).FrameCount; } else if (frame >= count) frame = count;
        uint picture = _pictures.NewPicture(unchecked((uint)type), unchecked((uint)frame), metadata); _pictures.SetMapped(picture, mapped ? 1U : 0U); if (fit) _pictures.Fit(picture, _state.FitWidth, _state.FitHeight); StyleText.AppendStyleTextFragment(_fragments, ref head, picture);
    }
    /// <summary>일치한 옵션은 비활성이어도 소비한다.E는 두 축 중 하나만 허용 범위여도 두 값을 같이 쓴다.</summary>
    private void Option(char code, string first, string second, uint[] context, ref uint metadata, bool unconditionalSound = false)
    {
        switch (code)
        {
            case 'S': if (unconditionalSound || _state.AllowSounds != 0) Require(_hooks.Sound)(Capture(first)); break;
            case 'T': metadata = unchecked((uint)Number(first)); break;
            case 'C': if (_state.AllowForegroundColor != 0) context[2] = unchecked((uint)Number(first)); break;
            case 'B': if (_state.AllowBackgroundColor != 0) context[5] = unchecked((uint)Number(first)); break;
            case 'E': if (_state.AllowOffset != 0) { int x = Number(first), y = Number(second); if (x is >= -4 and <= 4 || y is >= -4 and <= 4) { context[0] |= 2; context[5] = Require(_hooks.Black)(); context[6] = unchecked((uint)x); context[7] = unchecked((uint)y); } } break;
        }
    }
    /// <summary>첫NUL까지만 읽고 I→S→T→C→B→E 중 첫 일치를 처리한다.CD 한 줄은 I/T만 검사한다.</summary>
    public void Apply(ReadOnlySpan<byte> command, uint[] context, ref uint head, ref uint metadata)
    {
        string text = Source(command); bool patch = _edition == OriginalEdition.Patch1078;
        // 우선순위의 첫 일치만 소비하며 무효 그림/비활성 옵션에서도 다음 명령을 시도하지 않는다.
        foreach (char code in CommandOrder) { if (!patch && code is not ('I' or 'T')) continue; if (Match(text, code) is not { } m) continue; if (code == 'I') Picture(m.First, m.Second, ref head, metadata); else Option(code, m.First, m.Second, context, ref metadata); return; }
    }
    /// <summary>다중 줄은 */# 접두와 남은 suffix의# 맞춤,숫자 타입 상한,현재 제한/높이 문턱을 별도로 처리한다.</summary>
    private void ScrollingPicture(string typeExpression, string frameExpression, Func<bool> restricted, Action<uint, uint, int, int> appendPicture)
    {
        bool patch = _edition == OriginalEdition.Patch1078; string name = Capture(typeExpression), expression = Capture(frameExpression); if (expression.Length >= CaptureBytes) throw new ArgumentException("다중 줄 그림 프레임 지역 배열 범위 초과"); int type = Number(name);
        if (type == 0) type = Require(_hooks.FindType)(name); else if (patch && type >= Require(_hooks.TypeCount)()) return; if (type == -1) return;
        int frame = 0; string suffix = expression;
        if (expression[0] is '*' or '#') { int gump = Type(type).GumpFrame; frame = patch ? (gump == -1 ? Type(type).DefaultFrame : Type(type).GumpFrame) : (gump == -1 ? Type(type).DefaultFrame : gump); suffix = suffix[1..]; }
        else
        {
            int letter = unchecked((sbyte)(byte)expression[0]); if (letter >= 'A') { var view = Type(type); if (view.LetterKinds is < 0 or > 16) throw new ArgumentOutOfRangeException("LetterKinds"); if (letter < 'A' + view.LetterKinds) { frame = patch ? Type(type).First[letter - 'A'] : view.First[letter - 'A']; suffix = suffix[1..]; } } frame = unchecked(frame + Number(suffix));
        }
        int width = 0, height = 0; if (suffix.Length != 0 && suffix[0] == '#') { width = _state.FitWidth; height = _state.FitHeight; }
        int count = Type(type).FrameCount; if (Math.Min(frame, count) < 0) frame = 0; else if (patch) { if (frame >= Type(type).FrameCount) frame = Type(type).FrameCount; } else if (frame >= count) frame = count;
        if (Require(restricted)())
        {
            if (type <= 0 || type >= Require(_hooks.TypeCount)() || frame < 0 || frame >= Type(type).FrameCount) return; short? h = Require(_hooks.FrameHeight)(type, frame); if (h is null || h > (patch ? 16 : 12)) return; width = 99999; height = 12;
        }
        Require(appendPicture)(unchecked((uint)type), unchecked((uint)frame), width, height);
    }
    /// <summary>CD 다중 줄은 I→W→T이며 W는 zone 소리 허용과 무관하게 첫 캡처를 재생한다.</summary>
    public void ApplyScrolling(ReadOnlySpan<byte> command, uint[] context, ref uint metadata, Func<bool> restricted, Action<uint, uint, int, int> appendPicture)
    {
        string text = Source(command); bool patch = _edition == OriginalEdition.Patch1078;
        // 같은 부분 검색을 쓰되 판본별 명령 목록은 구별한다.
        foreach (char code in patch ? CommandOrder : CdScrollingOrder) { if (Match(text, code) is not { } m) continue; if (code == 'I') ScrollingPicture(m.First, m.Second, restricted, appendPicture); else Option(code == 'W' ? 'S' : code, m.First, m.Second, context, ref metadata, !patch); return; }
    }
    /// <summary>글꼴/팔레트/외부 가상 훅을 유지하고 실제 대괄호 처리만 연결한다.</summary>
    public StyleTextHooks Hooks(StyleTextHooks hooks)
    { hooks.Bracket = (ReadOnlySpan<byte> source, uint[] context, ref uint head, ref uint metadata) => Apply(source, context, ref head, ref metadata); return hooks; }
    /// <summary>현재 팔레트 기본 검정 별칭을 E 명령에 공급한다.</summary>
    public static void BindStyleTextMarkupPalette(StyleTextMarkupHooks hooks, PaletteColorTable palette) => hooks.Black = () => palette.Basic[0];
    /// <summary>원본 ReadInt처럼 키가 없으면 현재 값을 유지하고 기존 CRT 숫자 파서를 쓴다.전체 ConfigInterface 조립은 후속이다.</summary>
    public static void LoadStyleTextMarkupOptions(StyleTextMarkupState state, Func<string, string?> read)
    {
        ArgumentNullException.ThrowIfNull(read); if (read("allowzonesounds") is { } sounds) state.AllowSounds = ConfigNumbers.ParseLong(sounds); if (read("zonebgcolor") is { } bg) state.AllowBackgroundColor = ConfigNumbers.ParseLong(bg); if (read("zonefgcolor") is { } fg) state.AllowForegroundColor = ConfigNumbers.ParseLong(fg); if (read("zoneoffset") is { } offset) state.AllowOffset = ConfigNumbers.ParseLong(offset);
    }
    /// <summary>원본 이름 재생의 loop/volume/pan/priority/limit 인자를 모두0으로 전달한다.실제 SoundPlayer 장치 조립은 후속이다.</summary>
    public static void BindStyleTextMarkupSound(StyleTextMarkupHooks hooks, Action<string, int, int, int, int, int> playByName)
    { ArgumentNullException.ThrowIfNull(playByName); hooks.Sound = name => playByName(name, 0, 0, 0, 0, 0); }
}
