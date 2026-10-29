using System.Runtime.CompilerServices;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 스타일 문자열 조각의 소유권·측정·출력·짧은 명령 영역이다.

/// <summary>원본 가상 동작을 선택하는 문자열/파생/소멸 뒤 기본 분류.</summary>
public enum StyleTextFragmentKind
{
    /// <summary>소유 NUL 문자열.</summary>
    Text,
    /// <summary>그림 등 외부 파생 조각.</summary>
    External,
    /// <summary>소멸 뒤 기본 가상 표의 빈 동작.</summary>
    Base,
}

/// <summary>원본+4 next/+8 공통T태그/+c 문자열/+10 문맥이다.
/// 근거: cpppj/src/client/StyleText.h. Next/Metadata는 원본 주소 alias 검사를 위해 실제 저장 필드다.</summary>
public sealed class StyleTextFragment
{
    /// <summary>원본 가상 동작의 호스트 분류.</summary>
    public StyleTextFragmentKind Kind { get; set; }
    /// <summary>현재 다음 조각 토큰.</summary>
    public uint Next;
    /// <summary>공통T태그. CloneText는 원본처럼 이 값을 복사 길이로도 사용한다.</summary>
    public uint Metadata;
    /// <summary>소유 문자열. 정상 문자열은 마지막NUL을 포함한다.</summary>
    public byte[] Text { get; set; } = [];
    /// <summary>원본32바이트 저장 문맥.</summary>
    public uint[] Context { get; } = new uint[8];
}

/// <summary>원본 주소 대신 안정된 호스트 토큰으로 소유하는 공통 조각 표다.</summary>
public sealed class StyleTextState
{
    /// <summary>삽입으로 기존 객체 참조가 바뀌지 않는 번호 순서 표.</summary>
    public SortedDictionary<uint, StyleTextFragment> Fragments { get; } = [];
    /// <summary>다음 자동 토큰.0은 소진이다. 반납 토큰을 자동 재사용하지 않는다.</summary>
    public uint NextToken { get; set; } = 1;
}

/// <summary>NUL 문자열과 저장 문맥의 실제 폭/높이를 원본 출력 주소에 쓴다.</summary>
public delegate void StyleTextMeasure(ReadOnlySpan<byte> text, uint[] context, ref int width, ref int height);
/// <summary>문자열 조각을 지정 화면 위치에 실제 글리프로 출력한다.</summary>
public delegate void StyleTextDraw(ReadOnlySpan<byte> text, ScreenPoint position, uint[] context);
/// <summary>~[...] 내부 명령으로 현재 문맥/소유head/T태그를 같은 저장소에서 갱신한다.</summary>
public delegate void StyleTextBracket(ReadOnlySpan<byte> command, uint[] context, ref uint head, ref uint metadata);

/// <summary>안정된 문자열 입력과 현재 파생/팔레트/대괄호 경계다. 파생 콜백은 현재head/next를 바꿀 수 있다.</summary>
public sealed class StyleTextHooks
{
    /// <summary>실제 캐시 높이/상승/하강.</summary>
    public Func<uint[], (int Height, int Ascent, int Descent)>? TextMetrics { get; set; }
    /// <summary>실제 첫 폭 표/NUL 측정.</summary>
    public StyleTextMeasure? TextMeasure { get; set; }
    /// <summary>실제 저장 문맥/현재 표면 출력.</summary>
    public StyleTextDraw? TextDraw { get; set; }
    /// <summary>scalar 표 반납 직전 통지. 생략 가능하다.</summary>
    public Action<uint>? Release { get; set; }
    /// <summary>파생 가상+4의 높이/상승.</summary>
    public Func<uint, (int Height, int Ascent)>? ExternalMetrics { get; set; }
    /// <summary>파생 가상+8의 폭/높이.</summary>
    public Func<uint, (int Width, int Height)>? ExternalMeasure { get; set; }
    /// <summary>파생 가상+c의 토큰/위치/선택 문맥.</summary>
    public Action<uint, ScreenPoint, uint[]?>? ExternalDraw { get; set; }
    /// <summary>파생 자원의 가상 소멸. 공통 표 반납은 이후 같은 순서를 따른다.</summary>
    public Action<uint, byte>? ExternalDestroy { get; set; }
    /// <summary>원본 한 바이트 소문자 색 명령의 현재 팔레트 결과.</summary>
    public Func<byte, uint>? Color { get; set; }
    /// <summary>닫는]가 있는 대괄호 명령. 실제 StyleTextMarkup 연결은 별도다.</summary>
    public StyleTextBracket? Bracket { get; set; }
}

/// <summary>원본 문자열 조각 소유 체인과 두 번 순회하는 기준선·측정·출력을 옮긴다.
/// 근거: cpppj/src/client/StyleText.h/.cpp, 004755e0·004756b0·004756e0·00475720·00475760·004bb430.
/// 대응 C++의 styletext.json은 정적 복원 보고서이며 독립 실행 기대값이 아니다.</summary>
public sealed class StyleText
{
    /// <summary>두 판본 공통 소문자 색 명령.</summary>
    private const string CommonColors = "bgijklmnoqrtwxy";
    /// <summary>10.78에서만 추가된 소문자 색 명령.</summary>
    private const string PatchColors = "adpsu";
    /// <summary>직접 기본색 별칭을 사용하는 원본 명령/인덱스.</summary>
    private static readonly (byte Code, int Index)[] BasicColors = [((byte)'k', 0), ((byte)'r', 1), ((byte)'g', 2), ((byte)'b', 3), ((byte)'m', 4), ((byte)'y', 5), ((byte)'o', 6)];
    /// <summary>원본 추가색 전역의 목표RGB. 중복RGB의 별도 전역은 사용하지 않는다.</summary>
    private static readonly (byte Code, byte Red, byte Green, byte Blue)[] NamedColors = [((byte)'a', 255, 153, 0), ((byte)'d', 0, 255, 102), ((byte)'i', 249, 233, 145), ((byte)'j', 233, 214, 186), ((byte)'l', 151, 215, 210), ((byte)'p', 255, 102, 204), ((byte)'q', 200, 175, 144), ((byte)'s', 99, 66, 255), ((byte)'t', 219, 213, 203), ((byte)'u', 51, 255, 102), ((byte)'x', 47, 24, 17)];
    /// <summary>실제 공통 조각 표.</summary>
    private readonly StyleTextState _state;
    /// <summary>문자열/파생/팔레트 경계.</summary>
    private readonly StyleTextHooks _hooks;
    /// <summary>명령/출력 alias 조회 순서를 선택하는 판본.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>실제 도달한 가상 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("StyleText 가상/문자열/팔레트 경계 필요");
    /// <summary>같은 체인에서만 중복 토큰을 거부한다. 정상 첫/두 번째 순회는 독립이다.</summary>
    private static void Visit(HashSet<uint> seen, uint token) { if (!seen.Add(token)) throw new InvalidOperationException("StyleText 순환 체인"); }
    /// <summary>실제 공통 표의 살아 있는 조각만 읽는다.</summary>
    private static StyleTextFragment Fragment(StyleTextState state, uint token) => token != 0 && state.Fragments.TryGetValue(token, out StyleTextFragment? n) ? n : throw new ArgumentOutOfRangeException(nameof(token), "StyleText 조각 저장소 없음");
    /// <summary>현재next만 읽으며 소멸/순환을 거부한다.</summary>
    private static uint FragmentTail(StyleTextState state, uint head)
    {
        var seen = new HashSet<uint>();
        // 공통 next를 호출마다 다시 읽는다.
        while (true) { Visit(seen, head); uint next = Fragment(state, head).Next; if (next == 0) return head; head = next; }
    }
    /// <summary>원본 C++ 값 전달처럼 문자열/문맥을 복사해 같은 자동 토큰 표에 등록한다.</summary>
    public static uint RegisterStyleTextFragment(StyleTextState state, StyleTextFragment fragment)
    {
        uint token = state.NextToken;
        // 미리 등록된 번호는 덮어쓰지 않고 다음 빈 번호를 찾는다.
        while (token != 0 && state.Fragments.ContainsKey(token)) token = unchecked(token + 1);
        if (token == 0) throw new InvalidOperationException("StyleText 자동 토큰 발급 소진");
        var n = new StyleTextFragment { Kind = fragment.Kind, Next = fragment.Next, Metadata = fragment.Metadata, Text = (byte[])fragment.Text.Clone() }; fragment.Context.CopyTo(n.Context, 0);
        state.Fragments.Add(token, n); state.NextToken = unchecked(token + 1); return token;
    }
    /// <summary>현재 꼬리 뒤 단일 조각을 붙인 다음 새 조각next를0으로 쓴다. 체인 이전과 구별한다.</summary>
    public static void AppendStyleTextFragment(StyleTextState state, ref uint head, uint fragment)
    {
        Fragment(state, fragment); if (head == 0) head = fragment; else Fragment(state, FragmentTail(state, head)).Next = fragment; Fragment(state, fragment).Next = 0;
    }
    /// <summary>같은 상태와 경계를 빌린다. 파싱/연결만 할 때 글꼴/출력은 필요 없다.</summary>
    public StyleText(StyleTextState state, StyleTextHooks hooks, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); _state = state; _hooks = hooks; _edition = edition;
    }
    /// <summary>현재 공통 표의 실제 노드.</summary>
    private StyleTextFragment Node(uint token) => Fragment(_state, token);
    /// <summary>NUL 또는 명시 길이를 복사하고 반드시 마지막NUL을 덧붙인다. 중간NUL도 명시 복사에 포함한다.</summary>
    public uint NewText(ReadOnlySpan<byte> text, uint[] context, uint length = uint.MaxValue, uint metadata = 0)
    {
        if (context.Length != 8) throw new ArgumentException("StyleText 문맥 크기 오류"); int count;
        if (length == uint.MaxValue) { count = text.IndexOf((byte)0); if (count < 0) throw new ArgumentException("StyleText 문자열 범위에 NUL 필요"); }
        else { if (length > (uint)text.Length) throw new ArgumentOutOfRangeException(nameof(length), "StyleText 문자열 복사 범위 초과"); count = (int)length; }
        var fragment = new StyleTextFragment { Metadata = metadata, Text = new byte[checked(count + 1)] }; text[..count].CopyTo(fragment.Text); context.CopyTo(fragment.Context, 0); return RegisterStyleTextFragment(_state, fragment);
    }
    /// <summary>원본처럼+8을 복사 길이로 사용하고 새+8은0이다. 문자열 길이나 일반ID로 보정하지 않는다.</summary>
    public uint CloneText(uint token) { StyleTextFragment n = Node(token); if (n.Kind != StyleTextFragmentKind.Text) throw new ArgumentException("StyleText 문자열 조각 복제 필요"); return NewText(n.Text, n.Context, n.Metadata, 0); }
    /// <summary>단일 새 조각만 부착한다.</summary>
    public void Append(ref uint head, uint fragment) => AppendStyleTextFragment(_state, ref head, fragment);
    /// <summary>source 체인을 그대로 이전하고source를0으로 쓴다. 같은 소유 주소로 자기 이전은 거부한다.</summary>
    public void Transfer(ref uint head, ref uint source)
    {
        if (Unsafe.AreSame(ref head, ref source)) throw new ArgumentException("StyleText 소유 필드 자기 이전"); if (head == 0) head = source; else Node(FragmentTail(_state, head)).Next = source; source = 0;
    }
    /// <summary>문자열→next→기본 분류 순으로 소멸하고 scalar 반납 직전에release를 호출한다. 파생 자원은 실제 경계에 맡긴다.</summary>
    public void Destroy(uint token, byte flags)
    {
        StyleTextFragment n = Node(token);
        if (n.Kind == StyleTextFragmentKind.Text) { n.Text = []; n.Next = 0; n.Kind = StyleTextFragmentKind.Base; }
        else if (n.Kind == StyleTextFragmentKind.External) Require(_hooks.ExternalDestroy)(token, flags);
        if ((flags & 1) != 0) { _hooks.Release?.Invoke(token); _state.Fragments.Remove(token); }
    }
    /// <summary>다음 토큰을 삭제 전에 저장하고 마지막에 현재head를0으로 쓴다. 콜백이 바꾼 새head를 순회에 넣지 않는다.</summary>
    public void Clear(ref uint head)
    {
        uint token = head; var seen = new HashSet<uint>();
        // scalar 소멸자가 바꿀 수 있는next를 먼저 저장한다.
        while (token != 0) { Visit(seen, token); uint next = Node(token).Next; Destroy(token, 1); token = next; } head = 0;
    }
    /// <summary>문자열은 캐시 높이/상승, 파생은 가상+4, 소멸 뒤 기본은0이다.</summary>
    public (int Height, int Ascent) Baseline(uint token)
    {
        StyleTextFragment n = Node(token); if (n.Kind == StyleTextFragmentKind.Base) return default; if (n.Kind == StyleTextFragmentKind.External) return Require(_hooks.ExternalMetrics)(token); var m = Require(_hooks.TextMetrics)(n.Context); return (m.Height, m.Ascent);
    }
    /// <summary>저장 문맥/NUL 문자열로 측정한다. 출력override는 배치 측정에 쓰지 않는다.</summary>
    public (int Width, int Height) MeasureFragment(uint token)
    {
        StyleTextFragment n = Node(token); if (n.Kind == StyleTextFragmentKind.Base) return default; if (n.Kind == StyleTextFragmentKind.External) return Require(_hooks.ExternalMeasure)(token); int width = 0, height = 0; Require(_hooks.TextMeasure)(n.Text, n.Context, ref width, ref height); return (width, height);
    }
    /// <summary>선택override의 색/스타일/그림자를 쓰되 font는 조각의 저장 슬롯을 유지한다.</summary>
    public void DrawFragment(uint token, ScreenPoint position, uint[]? overrideContext = null)
    {
        StyleTextFragment n = Node(token); if (n.Kind == StyleTextFragmentKind.Base) return;
        if (n.Kind == StyleTextFragmentKind.External) { Require(_hooks.ExternalDraw)(token, position, overrideContext); return; }
        StyleTextDraw draw = Require(_hooks.TextDraw);
        if (overrideContext is not null) { if (overrideContext.Length != 8) throw new ArgumentException("StyleText 출력 문맥 크기 오류"); uint[] context = (uint[])overrideContext.Clone(); context[1] = n.Context[1]; draw(n.Text, position, context); }
        else draw(n.Text, position, n.Context);
    }
    /// <summary>두 번 기준선을 순회한다. in head와 출력/next 주소가 겹칠 때 판본별 조회·높이→폭 쓰기 순서도 유지한다.</summary>
    public void Measure(in uint head, ref int width, ref int height)
    {
        bool patch = _edition == OriginalEdition.Patch1078; uint token = patch ? 0 : head; height = 0; width = 0; if (patch) token = head; if (token == 0) return; int ascent = 0; var seen = new HashSet<uint>();
        // 첫 순회는 최대 상승과 콜백 뒤 현재next만 사용한다.
        while (token != 0) { Visit(seen, token); ascent = Math.Max(ascent, Baseline(token).Ascent); token = Node(token).Next; }
        token = head; seen.Clear();
        // 두 번째는 현재head부터 시작하고CD만 출력 쓰기 전에next를 저장한다.
        while (token != 0)
        {
            Visit(seen, token); var baseline = Baseline(token); var size = MeasureFragment(token); uint next = patch ? 0 : Node(token).Next;
            height = Math.Max(height, unchecked(size.Height - baseline.Ascent + ascent)); width = unchecked(width + size.Width); token = patch ? Node(token).Next : next;
        }
    }
    /// <summary>Draw의head는 값으로 저장한다. 첫 최대 캐시 높이를 반환하고 두 번째 순회는 기준선 정렬/현재next를 사용한다.</summary>
    public int Draw(uint head, ScreenPoint position, uint[]? overrideContext = null)
    {
        if (head == 0) return 0; int height = 0, ascent = 0; uint token = head; var seen = new HashSet<uint>();
        // 측정 첫 순회에서 높이와상승의 최대값을 각각 보관한다.
        while (token != 0) { Visit(seen, token); var baseline = Baseline(token); height = Math.Max(height, baseline.Height); ascent = Math.Max(ascent, baseline.Ascent); token = Node(token).Next; }
        token = head; seen.Clear();
        // 출력 콜백이 상태를 바꿔도 이번 측정 폭으로x를 증가시킨다.
        while (token != 0)
        {
            Visit(seen, token); var baseline = Baseline(token); var size = MeasureFragment(token); DrawFragment(token, new(position.X, unchecked(position.Y + ascent - baseline.Ascent)), overrideContext); position = new(unchecked(position.X + size.Width), position.Y); token = Node(token).Next;
        }
        return height;
    }
    /// <summary>현재 원본 팔레트 색 전역은 실제 색 명령에서만 읽는다.</summary>
    private uint Color(byte command) => Require(_hooks.Color)(command);
    /// <summary>한 줄/다중 줄이 공유하는 짧은 명령이다. restricted는 패치의0/1/3/4 슬롯 변경만 막고 소비는 유지한다.</summary>
    public bool ApplyShortCommand(byte command, uint[] context, uint[] currentDefault, bool restricted = false)
    {
        if (context.Length != 8 || currentDefault.Length != 8) throw new ArgumentException("StyleText 명령 문맥 크기 오류"); bool patch = _edition == OriginalEdition.Patch1078;
        switch ((char)command)
        {
            case '*': context[0] = 0; break; case '.': currentDefault.CopyTo(context, 0); break;
            case '0': case '1': case '3': case '4': if (!patch || !restricted) context[1] = (uint)(command - '0'); break;
            case '5': case '6': context[1] = (uint)(command - '0'); break;
            case 'B': context[3] = 2; break; case 'I': context[3] = 1; break;
            case 'N': context[0] &= ~2U; context[3] = 0; break;
            case 'E': context[0] |= 2; context[5] = Color((byte)'k'); context[6] = context[7] = 1; break;
            case 'S': if (!patch) return false; context[3] = 3; break; case 'U': if (!patch) return false; context[3] = 4; break;
            case 'n': context[2] = currentDefault[2]; break;
            default: if (!CommonColors.Contains((char)command) && (!patch || !PatchColors.Contains((char)command))) return false; context[2] = Color(command); break;
        }
        return true;
    }
    /// <summary>기존head 뒤에 짧은 명령으로 바뀐 구간을 붙인다. 각 Parse의T태그는0으로 시작하며 마지막 빈 조각도 보존한다.</summary>
    public void Parse(ref uint head, ReadOnlySpan<byte> text, uint[] initialContext)
    {
        if (initialContext.Length != 8) throw new ArgumentException("StyleText 초기 문맥 크기 오류"); int end = text.IndexOf((byte)0); if (end < 0) end = text.Length; byte[] source = new byte[checked(end + 1)]; text[..end].CopyTo(source);
        uint[] context = (uint[])initialContext.Clone(), original = (uint[])initialContext.Clone(); int position = 0; uint metadata = 0;
        // ~ 앞 문자열을 먼저 만들고 끝NUL은 빈 구간도 새 조각으로 추가한다.
        while (true)
        {
            int relative = source.AsSpan(position, end - position).IndexOf((byte)'~'); int mark = relative < 0 ? -1 : position + relative;
            if (mark < 0) { Append(ref head, NewText(source.AsSpan(position), context, uint.MaxValue, metadata)); return; }
            if (mark > position) Append(ref head, NewText(source.AsSpan(position), context, (uint)(mark - position), metadata));
            byte command = source[mark + 1]; int next = mark + 2;
            switch ((char)command)
            {
                case '~': Append(ref head, NewText("~\0"u8, context, uint.MaxValue, metadata)); break;
                case '[':
                    int closeRelative = source.AsSpan(next, end - next).IndexOf((byte)']');
                    if (closeRelative >= 0) { int close = next + closeRelative; Require(_hooks.Bracket)(source.AsSpan(next, close - next), context, ref head, ref metadata); next = close + 1; }
                    break;
                default: if (!ApplyShortCommand(command, context, original)) next = mark + 1; break;
            }
            position = Math.Min(next, end);
        }
    }
    /// <summary>같은 FontStore의 캐시 메트릭/첫 폭/NUL 측정과 실제 현재 글리프 출력을 연결한다.</summary>
    public static void BindStyleTextFonts(StyleTextHooks hooks, GumpText text, GumpTextDraw? draw = null)
    {
        hooks.TextMetrics = words => text.Metrics(GumpText.DecodeGumpTextContext(words));
        hooks.TextMeasure = (ReadOnlySpan<byte> bytes, uint[] words, ref int width, ref int height) => text.Measure(bytes, GumpText.DecodeGumpTextContext(words), ref width, ref height);
        if (draw is not null) hooks.TextDraw = (bytes, position, words) => draw.Draw(bytes, position, GumpText.DecodeGumpTextContext(words));
    }
    /// <summary>기본색은실제basic 별칭, 나머지는 원본RGB 검색 결과에 연결한다. 없는색을 가짜 번호로 대체하지 않는다.</summary>
    public static void BindStyleTextPalette(StyleTextHooks hooks, PaletteColorTable palette)
    {
        hooks.Color = command =>
        {
            if (command == 'w') return palette.Basic[7];
            // 원본 기본색 전역을 직접 읽는다.
            foreach (var basic in BasicColors) if (basic.Code == command) return palette.Basic[basic.Index];
            // 추가색의 실제RGB 항목만 현재표에서 찾는다.
            foreach (var color in NamedColors)
            {
                if (color.Code != command) continue;
                // 중복RGB가 있으면 원본표의 첫 항목이다.
                foreach (var named in palette.Named) if (named.Red == color.Red && named.Green == color.Green && named.Blue == color.Blue) return named.Index;
                throw new ArgumentException("StyleText 원본 색 표 항목 없음");
            }
            throw new ArgumentException("StyleText 팔레트 명령 범위 오류");
        };
    }
}
