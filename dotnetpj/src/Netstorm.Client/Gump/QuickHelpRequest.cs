namespace Netstorm.Client.Gump; // 원본 도움말 요청의 주제/타입 조회·이력·현재 창 재부착 영역이다.

/// <summary>실제 타입 조회/순수 서식·같은 전용 창/이력/가상 동작을 공급한다.</summary>
public sealed class QuickHelpRequestHooks
{
    /// <summary>원본49a860의 signed 타입 조회.0도 유효한 결과다.</summary>
    public Func<string, int>? Lookup { get; set; }
    /// <summary>원본%s/sun%s/wind%s 순수 서식 장치.</summary>
    public Func<string, string, string>? Print { get; set; }
    /// <summary>정규화 전 원래 주제를 같은 실제 이력에 보낸다.</summary>
    public Action<string>? History { get; set; }
    /// <summary>전용 본문/그림을 같은 현재 창에서 갱신한다.</summary>
    public Action<uint, string>? Show { get; set; }
    /// <summary>현재 flags/좌표를 바꿀 수 있는 가상 Push.</summary>
    public Action? Push { get; set; }
    /// <summary>Push 뒤의 현재 x/y와 실제 가상 부착.</summary>
    public Action<float, float, uint, uint>? Attach { get; set; }
}
/// <summary>cpppj/src/client/QuickHelpRequest.h/.cpp의 PATCH004c5c00.</summary>
public sealed class QuickHelpRequest
{
    /// <summary>종료 NUL까지 포함하는 원본 지역 주제 버퍼.</summary>
    private const int TopicBytes = 80;
    /// <summary>창 갱신/Push 뒤 다시 읽는 같은 실제 기본 노드.</summary>
    private readonly GumpCreateNode _window;
    /// <summary>선택된 실제 조회/창/가상 효과.</summary>
    private readonly QuickHelpRequestHooks _hooks;
    /// <summary>실제 선택 효과만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("Quick Help 요청 실제 경계 누락");
    /// <summary>같은 창/외부 장치 수명을 빌리며 요청을 미리 실행하지 않는다.</summary>
    public QuickHelpRequest(GumpCreateNode window, QuickHelpRequestHooks hooks) { ArgumentNullException.ThrowIfNull(window); ArgumentNullException.ThrowIfNull(hooks); _window = window; _hooks = hooks; }
    /// <summary>ASCII 대소문자만 무시하고 짧은 원문 버퍼 앞 읽기는 명시 거부한다.</summary>
    private static string RemoveSuffix(string topic, string suffix) { if (topic.Length < suffix.Length) throw new ArgumentOutOfRangeException(nameof(topic), "Quick Help 접미 비교의 원본 버퍼 앞 읽기"); return Lower(topic[^suffix.Length..]) == suffix ? topic[..^suffix.Length] : topic; }
    /// <summary>원문 C locale의 ASCII 소문자만 만든다.</summary>
    private static string Lower(string text) => string.Concat(text.Select(item => item is >= 'A' and <= 'Z' ? (char)(item + 'a' - 'A') : item));
    /// <summary>type0만 두 접미/3단계 signed 조회를 수행하고 원래 주제로 이력/표시·현재 재부착을 실행한다.</summary>
    public void Open(uint type, string topic)
    {
        int end = topic.IndexOf('\0'); if (end >= 0) topic = topic[..end]; if (type == 0)
        {
            if (topic.Length >= TopicBytes) throw new ArgumentException("Quick Help 원본 주제80BYTE 범위 초과"); string stem = topic; if (stem.Length != 0) { stem = RemoveSuffix(stem, "type"); stem = RemoveSuffix(stem, "help"); }
            // 같은 원문 접두 서식과 현재 signed 조회를 원래 순서로 수행한다.
            int Query(string pattern) => Require(_hooks.Lookup)(Require(_hooks.Print)(pattern, stem));
            int found = Query("%s"); if (found < 0) found = Query("sun%s"); if (found < 0) found = Query("wind%s"); type = found < 0 ? 0U : (uint)found;
        }
        Require(_hooks.History)(topic); Require(_hooks.Show)(type, topic); if ((_window.Flags & 8) == 0) { Require(_hooks.Push)(); Require(_hooks.Attach)(_window.X, _window.Y, 0, 0); }
    }
}
