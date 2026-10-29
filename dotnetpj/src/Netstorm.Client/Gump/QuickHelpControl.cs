using System.Collections.ObjectModel;
using System.Text;
using Netstorm.Client.Input;

namespace Netstorm.Client.Gump; // 전용 도움말의 공용 팝업/이력/열기/현재 입력을 같은 저장소에 연결한다.

/// <summary>원본005ca938/940의 공용 마지막 명령/80BYTE 인자.</summary>
public sealed class QuickHelpPopupState
{
    /// <summary>마지막 이력이 없어질 때 다시 예약할 명령.</summary>
    public uint Command { get; set; }
    /// <summary>첫 NUL 뒤도 보존하는 실제80BYTE 표식.</summary>
    public byte[] ArgumentBytes { get; } = new byte[80];
    /// <summary>null은 전체0,비null 빈 문자열은 첫 NUL만 쓴다.범위 거부 전 명령은 바꾸지 않는다.</summary>
    public void Mark(uint code, string? text = null) { if (text is not null) { int end = text.IndexOf('\0'); if (end >= 0) text = text[..end]; if (text.Length >= ArgumentBytes.Length) throw new ArgumentException("Quick Help 팝업 인자80BYTE 초과"); } Command = code; if (text is null) { Array.Clear(ArgumentBytes); return; }
        // 원문 접두부와 종료 NUL만 덮어쓰고 남은 BYTE는 유지한다.
        for (int i = 0; i < text.Length; i++) ArgumentBytes[i] = checked((byte)text[i]); ArgumentBytes[text.Length] = 0; }
    /// <summary>이전 길이/사본을 캐시하지 않고 현재 첫 NUL 앞 문자열을 읽는다.</summary>
    public string Argument() => Encoding.Latin1.GetString(ArgumentView().Span);
    /// <summary>원문 string_view처럼 현재 같은 실제 BYTE를 빌린다.큐의 두 인자는 이 동일 영역을 사용한다.</summary>
    public ReadOnlyMemory<byte> ArgumentView() { int end = Array.IndexOf(ArgumentBytes, (byte)0); if (end < 0) throw new ArgumentException("Quick Help 팝업 인자 NUL 누락"); return ArgumentBytes.AsMemory(0, end); }
}
/// <summary>원본4c52c0/4c6d00의 창 수명과 별개인 주제 이력.</summary>
public sealed class QuickHelpHistory
{
    /// <summary>가장 오래된 항목부터 보관하는 실제 소유 문자열.</summary>
    private readonly List<string> _topics = [];
    /// <summary>같은 실제 목록의 읽기 전용 현재 보기.</summary>
    private readonly ReadOnlyCollection<string> _view;
    /// <summary>창 생성/반납과 별개인 빈 이력을 만든다.</summary>
    public QuickHelpHistory() { _view = _topics.AsReadOnly(); }
    /// <summary>같은 현재 저장 목록을 오래된 항목부터 읽는다.</summary>
    public IReadOnlyList<string> Topics => _view;
    /// <summary>중간 중복은 남기고 머리의 연속 중복만 ASCII 대소문자를 무시해 생략한다.</summary>
    public void Push(string topic) { int end = topic.IndexOf('\0'); if (end >= 0) topic = topic[..end]; if (_topics.Count != 0 && Lower(_topics[^1]) == Lower(topic)) return; _topics.Add(topic); }
    /// <summary>원문 C locale의 ASCII만 정규화한다.</summary>
    private static string Lower(string text) => string.Concat(text.Select(item => item is >= 'A' and <= 'Z' ? (char)(item + 'a' - 'A') : item));
    /// <summary>현재 항목 하나를 제거하고 새 머리를 반환한다.빈 주제와 이력 없음은 다르다.</summary>
    public string? Back() { if (_topics.Count == 0) return null; _topics.RemoveAt(_topics.Count - 1); return _topics.Count == 0 ? null : _topics[^1]; }
}
/// <summary>열기마다 다시 읽는 등록 ID/현재 루트/signed 화면 영역.</summary>
public sealed class QuickHelpOpenState
{
    /// <summary>현재 Help 등록 serial.</summary>
    public uint HelpId { get; set; }
    /// <summary>허용할 대체 모달의 등록 serial.</summary>
    public uint AlternateId { get; set; }
    /// <summary>현재 실제 UI 루트.</summary>
    public uint Root { get; set; }
    /// <summary>signed 화면 영역 왼쪽.</summary>
    public int AreaLeft { get; set; }
    /// <summary>signed 화면 영역 폭.</summary>
    public int AreaWidth { get; set; }
}
/// <summary>실제 전용 생성자·현재 등록/모달/가상 트리만 공급한다.</summary>
public sealed class QuickHelpOpenHooks
{
    /// <summary>현재 sticky 팝업 Kill.</summary>
    public Action? ClosePopups { get; set; }
    /// <summary>현재 등록 ID 조회.</summary>
    public Func<uint, uint>? Lookup { get; set; }
    /// <summary>현재 모달이 없는지 판정한다.</summary>
    public Func<bool>? NoModal { get; set; }
    /// <summary>각 비교 시점의 현재 모달.</summary>
    public Func<uint>? CurrentModal { get; set; }
    /// <summary>new0xd0/전용 생성자.</summary>
    public Func<uint, uint>? Construct { get; set; }
    /// <summary>같은 실제 기본 노드.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>전용 창의 현재 가상 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>자식까지 같은 depth delta를 더한다.</summary>
    public Action<uint, uint>? AddDepth { get; set; }
    /// <summary>같은 실제 공통 모달 진입.</summary>
    public Action<uint, uint>? Modal { get; set; }
    /// <summary>같은 현재 전용 가상 입력.</summary>
    public Func<uint, uint, uint, uint, uint>? Handle { get; set; }
}
/// <summary>원본0xd0의 입력 필드.전체 전용 생성/본문은 같은 파생에서 확장한다.</summary>
public class QuickHelpWindowNode : GumpCreateNode
{
    /// <summary>serial을 비교할 실제 Cancel 자식 토큰.</summary>
    public uint CancelButton { get; set; }
    /// <summary>serial을 비교할 실제 Learn 자식 토큰.</summary>
    public uint LearnButton { get; set; }
    /// <summary>serial을 비교할 실제 Back 자식 토큰.</summary>
    public uint BackButton { get; set; }
    /// <summary>0이면 학습 없이 닫는 현재 TYPE.</summary>
    public uint LearnType { get; set; }
}
/// <summary>실제 본문/링크/스크롤과 현재 공통 Gump·명령 소비자의 경계.</summary>
public sealed class QuickHelpEventHooks
{
    /// <summary>모든 사건에서 가장 먼저 실행하는 현재 좌표 변환.</summary>
    public Func<InputEvent, (float X, float Y)>? Local { get; set; }
    /// <summary>현재 버튼의 실제 serial 조회.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; set; }
    /// <summary>음수 비트도 보존하는 현재 TYPE 이름.</summary>
    public Func<uint, string>? TypeName { get; set; }
    /// <summary>원문 Type 접미 번역.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>현재 유효 호스트 문자열 토큰.</summary>
    public Func<uint, string>? Text { get; set; }
    /// <summary>같은 창의 실제 요청 처리.</summary>
    public Action<uint, string>? Request { get; set; }
    /// <summary>현재 본문 페이지 높이 float.</summary>
    public Func<float>? PageHeight { get; set; }
    /// <summary>같은 실제 본문의 상대 스크롤 요청.</summary>
    public Action<int>? Scroll { get; set; }
    /// <summary>실제 내부 주제/프로토콜 링크 분배.</summary>
    public Action<string>? Link { get; set; }
    /// <summary>창을 지연 삭제하며 공용 popup도 바꿀 수 있다.</summary>
    public Action? Kill { get; set; }
    /// <summary>실제 기술 학습 요청.</summary>
    public Action<uint>? Learn { get; set; }
    /// <summary>같은 현재 문자열을 두 번 전달하는 원시 큐.</summary>
    public Action<uint, ReadOnlyMemory<byte>, ReadOnlyMemory<byte>>? Queue { get; set; }
    /// <summary>실제 기본 GumpHelp의 현재 입력.</summary>
    public Func<InputEvent, uint>? Base { get; set; }
}
/// <summary>cpppj/src/client/QuickHelpControl.h/.cpp의 열기와 전체 입력 분배.</summary>
public static class QuickHelpControl
{
    /// <summary>실제 Help/Request/Link/Button 명령 DWORD.</summary>
    private const uint Help = 0x02000017, Request = 0x02000057, Link = 0x02000025, Button = 0x0200002c;
    /// <summary>원본 사건 종류/키 해제/좌우 버튼 비트.</summary>
    private const uint CommandMask = 0x03ffffff, Release = 0x80000000, Left = 0x10000, Right = 0x20000;
    /// <summary>실제 선택 경계만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("Quick Help 제어 실제 경계 누락");
    /// <summary>원문 첫 NUL 뒤를 요청/이력으로 읽지 않는다.</summary>
    private static string CText(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>같은 실제 저장소의 유효 기본/자식만 읽는다.</summary>
    private static GumpCreateNode Node(Func<uint, GumpCreateNode?>? lookup, uint token) => Require(lookup)(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "Quick Help 현재 Gump 토큰 누락");
    /// <summary>팝업 닫기/두 등록 조회 뒤 현재 모달 관문을 검사하고 같은 Help를 재사용한다.</summary>
    public static uint OpenQuickHelpWindow(QuickHelpOpenState state, QuickHelpOpenHooks hooks, uint topic, int mode, uint learnType)
    {
        Require(hooks.ClosePopups)(); uint help = Require(hooks.Lookup)(state.HelpId), alternate = Require(hooks.Lookup)(state.AlternateId); if (Require(hooks.NoModal)() || Require(hooks.CurrentModal)() == help || Require(hooks.CurrentModal)() == alternate)
        {
            if (help == 0) { help = Require(hooks.Construct)(learnType); if (help == 0) throw new OutOfMemoryException("Quick Help 전용 창 할당 실패"); var node = Node(hooks.Node, help); float x = (float)(((double)state.AreaWidth - node.BaseWidth) * .5 + state.AreaLeft); Require(hooks.Attach)(help, x, 40, state.Root, 0); Require(hooks.AddDepth)(help, unchecked(Node(hooks.Node, help).Depth - 76)); }
            Require(hooks.Modal)(help, 1); Require(hooks.Handle)(help, mode == 0 ? Request : Help, 0, topic);
        }
        return help;
    }
    /// <summary>현재 좌표 변환→바깥 클릭→일반 키→요청/링크/현재 버튼/Back 순서의 전체 입력.</summary>
    public static uint DispatchQuickHelpWindow(QuickHelpWindowNode window, InputEvent input, QuickHelpHistory history, QuickHelpPopupState popup, QuickHelpEventHooks hooks)
    {
        var point = Require(hooks.Local)(input); uint code = input.Code & CommandMask, first = unchecked((uint)input.X), second = unchecked((uint)input.Y); bool down = (input.Code & Release) == 0;
        // 지연 삭제 뒤 현재 창 필드를 추가로 읽지 않는다.
        uint Close() { Require(hooks.Kill)(); return 1; }
        if ((!(point.X >= 0) || point.X >= window.Width || !(point.Y >= 0) || point.Y >= window.Height) && down && code == Left) return Close();
        if ((input.Code & 0x02000000) == 0 && code != Left && code != Right)
        {
            if (down) { if (code is 0x210000 or 0x220000) { float height = Require(hooks.PageHeight)(); Require(hooks.Scroll)(GumpCollection.Ftol(code == 0x210000 ? -height : height)); return 1; } if (code is 0x260000 or 0x280000) { Require(hooks.Scroll)(code == 0x260000 ? -10 : 10); return 1; } } return Require(hooks.Base)(input);
        }
        if (code is Help or Request) { if (input.Y < 256 && second != 0) { string topic = CText(Require(hooks.TypeName)(second)) + CText(Require(hooks.Translate)("Type")); if (topic.Length >= 80) throw new ArgumentException("Quick Help TYPE 주제80BYTE 초과"); Require(hooks.Request)(second, topic); } else { string topic = Require(hooks.Text)(second != 0 ? second : first); Require(hooks.Request)(0, CText(topic)); } return 1; }
        if (code == Link) { if (second != 0) { string topic = Require(hooks.Text)(second); Require(hooks.Link)(CText(topic)); } return 1; }
        if (code == Button)
        {
            // 등록 ID가 아닌 같은 현재 버튼+2c serial을 비교한다.
            bool Matches(uint token) => token != 0 && Node(hooks.Node, token).Serial == first;
            if (Matches(window.CancelButton)) return Close(); if (Matches(window.LearnButton)) { if (window.LearnType != 0) Require(hooks.Learn)(window.LearnType); return Close(); }
            if (Matches(window.BackButton)) { if (history.Topics.Count != 0) { string? topic = history.Back(); uint previous = popup.Command; if (topic is not null) { Require(hooks.Link)(topic); return 1; } Require(hooks.Kill)(); if (previous != 0) { var argument = popup.ArgumentView(); Require(hooks.Queue)(previous, argument, argument); } return 1; } return Close(); }
        }
        return Require(hooks.Base)(input);
    }
    /// <summary>같은 실제 좌표/지연 수명만 공급한다.본문/기본 Help/명령 큐는 현재 실제 소비자를 연결해야 한다.</summary>
    public static void BindQuickHelpDevices(QuickHelpEventHooks hooks, uint token, GumpCollection collection, GumpLifetime lifetime) { hooks.Local = input => collection.ScreenToLocal(token, new(input.X, input.Y)); hooks.Kill = () => lifetime.Kill(token); }
    /// <summary>현재 전용 노드만 같은 부모 사건/직접 입력으로 분배하고 다른 파생은 보존한다.</summary>
    public static void BindQuickHelpWindow(GumpEventHooks events, GumpInputHooks input, QuickHelpHistory history, QuickHelpPopupState popup, Func<uint, QuickHelpWindowNode?> resolve, Func<uint, QuickHelpEventHooks> hooks)
    {
        // 원시 인자 DWORD를 signed 좌표 칸에 그대로 보존해 같은 본체에 전달한다.
        uint Route(Func<uint, uint, uint, uint, uint>? previous, uint t, uint c, uint a, uint b) => resolve(t) is { } node ? DispatchQuickHelpWindow(node, new(c, unchecked((int)a), unchecked((int)b)), history, popup, hooks(t)) : Require(previous)(t, c, a, b);
        var e = events.Handle; var i = input.Handle; events.Handle = (t, c, a, b) => Route(e, t, c, a, b); input.Handle = (t, c, a, b) => Route(i, t, c, a, b);
    }
}
