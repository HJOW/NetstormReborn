using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // Tell 파일 목록의 장기 Config와 현재 cur 수명을 소유한다.

/// <summary>원본 파일 목록의 항목. 구분선은 명령과 인자를 사용하지 않는다.</summary>
/// <param name="Label">원본 바이트 표현 라벨.</param><param name="Command">원본 명령.</param><param name="Parameter">현재 문자열 고리 토큰.</param><param name="Separator">구분선 여부.</param>
public readonly record struct TellFileMenuItem(string Label, uint Command, uint Parameter, bool Separator);

/// <summary>cpppj/src/client/TellFileMenus.cpp의 @·#·% 파일 목록과 Config 수명이다.</summary>
public sealed class TellFileMenus : IDisposable
{
    /// <summary>원본 지역 버퍼 오버런을 행별 치환 예외와 구별한다.</summary>
    private sealed class PhysicalLimitException(string message) : ArgumentException(message);
    /// <summary>현재 실제 설정과 같은 등록부.</summary>
    private readonly ConfigInterface _config;
    /// <summary>실제 FileFinder 순서의 현재 경로 열거.</summary>
    private readonly Func<string, IReadOnlyList<string>> _enumerate;
    /// <summary>목록 화면보다 오래 유지하는 신규 파일 설정.</summary>
    private readonly List<Config> _owned = [];

    /// <summary>현재 Client 설정과 경로 열거를 빌린다. 파일 적재는 목록 요청 때 수행한다.</summary>
    public TellFileMenus(ConfigInterface config, Func<string, IReadOnlyList<string>> enumerate)
    { _config = config ?? throw new ArgumentNullException(nameof(config)); _enumerate = enumerate ?? throw new ArgumentNullException(nameof(enumerate)); }

    /// <summary>원본 C 문자열의 첫 NUL 앞만 쓴다.</summary>
    private static string CString(string value) { int end = value.IndexOf('\0'); return end < 0 ? value : value[..end]; }
    /// <summary>원본 바이트 표현을 실제 Config 문자열로 해독한다.</summary>
    private static string Decode(string raw) => OriginalText.DecodeWindows1252(Encoding.Latin1.GetBytes(raw));
    /// <summary>설정 문자열을 원본 바이트 표현으로 변환한다. 한글 등 확장 대상은 4바이트 시퀀스다.</summary>
    /// <remarks>2026-11-11 0.357에서 cpppj HEAD의 표시 경계(`Narrow`)와 같게 고쳤다. ASCII·1252 범위는 기존과 같은 바이트다.</remarks>
    private static string Narrow(string text) => Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText(text));
    /// <summary>원본 지역 버퍼의 종료 NUL 공간을 검사한다. 표시 바이트 길이로 잰다.</summary>
    private static void Limit(string text, int capacity) { if (Narrow(text).Length >= capacity) throw new PhysicalLimitException("Tell 파일 목록 지역 버퍼 초과"); }
    /// <summary>표시 바이트(NUL 포함 capacity)를 넘지 않도록 마지막 문자부터 줄인다. 원문은 80바이트 지역 버퍼를 넘는 긴 캠페인 제목을 그대로 복사해 종료했다(10차 NB24).</summary>
    /// <param name="value">줄일 제목이다.</param><param name="capacity">NUL 포함 표시 한도다.</param>
    /// <returns>한도 안의 제목이다.</returns>
    /// <remarks>2026-11-11 0.348에서 cpppj HEAD의 NB24 교정(`FitDisplayLimit`)을 반영했다. 서로게이트 쌍이 갈라지지 않게 마지막 문자 단위로 줄인다.</remarks>
    private static string FitDisplayLimit(string value, int capacity)
    {
        // 한 번에 한 문자씩 줄여 표시 바이트가 중간에 잘리지 않게 한다
        while (value.Length != 0 && Narrow(value).Length >= capacity)
        {
            int last = value.Length - 1;
            if (last > 0 && char.IsLowSurrogate(value[last]) && char.IsHighSurrogate(value[last - 1]))
            {
                last--;
            }
            value = value[..last];
        }
        return value;
    }
    /// <summary>경로의 마지막 구분자 뒤에서 첫 점 앞까지 원본 이름을 얻는다.</summary>
    private static string FileName(string path)
    { int slash = path.LastIndexOfAny(['/', '\\', ':']); string name = path[(slash + 1)..]; int dot = name.IndexOf('.'); return dot < 0 ? name : name[..dot]; }
    /// <summary>원본 ASCII 대소문자만 접어 비교한다.</summary>
    private static string Lower(string text) => string.Concat(text.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
    /// <summary>등록 객체의 마지막 점을 제외한 이름을 반환한다.</summary>
    private static string ObjectName(Config config) => config.Prefix.Length > 0 ? config.Prefix[..^1] : throw new InvalidOperationException("Tell 파일 목록 접두부 없음");
    /// <summary>현재 local.1로 제목을 치환하고 선택한 경우 빈 제목을 파일명으로 대체한다. 긴 제목은 종료하지 않고 80바이트 안으로 줄여 표시한다(10차 NB24).</summary>
    private string Title(string name, bool fallback)
    { string title = FitDisplayLimit(_config.SubstituteLocal("{{local.1}.title}", name), 80); if (fallback && title.Length == 0) { title = FitDisplayLimit(name, 80); } return title; }

    /// <summary>같은 언어 전환 알림에서 소유한 목록 Config의 언어 내용만 재적재하고 등록 주소·index·열린 안내 링크는 유지한다.</summary>
    /// <remarks>2026-11-11 0.357에서 cpppj HEAD를 반영했다. mission·사용자 다른 객체에는 접근하지 않는다.</remarks>
    public void InvalidateLanguageFiles()
    {
        string language = _config.Configuration.Get("currentLanguage") ?? "";
        // 실제 Client가 확정한 현재 설정이며 빈 독립 입력은 다음 수집에서 재생성한다
        if (language.Length == 0) { foreach (Config item in _owned) item.Dispose(); _owned.Clear(); return; }
        // 언어 확장자만 교체하며 일반 설정·프로필 파일명은 바꾸지 않는다
        string[] languages = ["english", "french", "german", "spanish", "japanese", "portuguese", "korean"];
        if (Array.IndexOf(languages, language) < 0) return;
        // 소유한 목록 Config만 교체한다
        foreach (Config item in _owned)
        {
            string? path = item.GetRaw("path"); if (path is null) continue; int dot = path.LastIndexOf('.'); if (dot < 0) continue;
            string previous = Lower(path[(dot + 1)..]); if (Array.IndexOf(languages, previous) < 0) continue;
            string selected = path[..(dot + 1)] + language; if (selected == path) continue;
            // 접두부 주소와 원래 index·파일 이름은 보존한다
            string name = item.GetRaw("fileName") ?? ObjectName(item), index = item.GetRaw("index") ?? "0";
            item.Clear(); item.Append("path=" + selected); item.Append("fileName=" + name); item.Append("index=" + index);
            // 실제 Config 파일·본문 해독만 사용하며 인공지능·SP 실행을 다시 시작하지 않는다
            byte[]? bytes = _config.Reader?.Invoke(selected); if (bytes is not null) item.LoadBytes(bytes);
        }
    }

    /// <summary>열거 순서대로 신규 Config를 등록한 뒤 고유 prefix를 원본 비교·교환 순서로 정렬한다.</summary>
    private List<Config> Collect(string pattern, int capacity)
    {
        string spec = Decode(CString(pattern)); Limit(spec, 256); var result = new List<Config>(); uint index = 1;
        // 파일 경로별 첫 객체와 index를 정렬 전에 확정한다.
        foreach (string path in _enumerate(spec))
        {
            Limit(path, 256); string name = FileName(path); Limit(name, 256); Config? item = _config.Registry.FindByName(name);
            if (item is null)
            {
                item = new Config(_config.Registry, name); _owned.Add(item);
                item.Append("path=" + path); item.Append("fileName=" + name); item.Append("index=" + index.ToString(System.Globalization.CultureInfo.InvariantCulture));
                if (_config.Reader?.Invoke(path) is { } bytes) item.LoadBytes(bytes);
            }
            if (!result.Contains(item)) { if (result.Count >= capacity) throw new PhysicalLimitException("Tell 파일 목록 prefix 배열 초과"); result.Add(item); index++; }
        }
        // 제목이 아닌 원본 prefix 바이트를 두 겹 순회하여 비교·교환한다.
        for (int first = 0; first < result.Count; first++) for (int next = first; next < result.Count; next++)
            if (string.CompareOrdinal(Lower(Narrow(result[first].Prefix)), Lower(Narrow(result[next].Prefix))) > 0)
                (result[first], result[next]) = (result[next], result[first]);
        return result;
    }

    /// <summary>@/#의 실제 항목을 같은 cur.file 수명과 문자열 고리 안에서 순서대로 작성한다.</summary>
    public void Append(string pattern, uint command, string argument, bool fallback, ScriptStringRing strings, Action<TellFileMenuItem>? emit)
    {
        ArgumentNullException.ThrowIfNull(strings);
        // 각 파일의 cur 객체는 항목 콜백이 끝난 뒤 반납한다.
        foreach (Config item in Collect(pattern, 40))
        {
            string name = ObjectName(item); using var current = new Config(_config.Registry, "cur"); current.Append("file=" + name);
            string label = Narrow(Title(name, fallback)); bool separator = label.StartsWith("--", StringComparison.Ordinal); uint parameter = 0;
            if (!separator) { string value = _config.SubstituteLocal(Decode(CString(argument))); Limit(value, 1024); parameter = strings.Store(Narrow(value)); }
            (emit ?? throw new InvalidOperationException("Tell 파일 목록 항목 작성 미연결"))(new(label, separator ? 0 : command, parameter, separator));
        }
    }

    /// <summary>%의 두 번 치환과 원본 링크 철자, 행별 치환 실패의 ERROR 출력을 유지한다.</summary>
    public string Inline(string pattern, string command, string argument)
    {
        var result = new StringBuilder();
        // 제목·두 번의 치환·링크 생성 동안 같은 cur.file을 유지한다.
        foreach (Config item in Collect(pattern, 80))
        {
            string name = ObjectName(item); using var current = new Config(_config.Registry, "cur"); current.Append("file=" + name);
            string label = Narrow(Title(name, true)); if (label.StartsWith("--", StringComparison.Ordinal)) continue;
            try
            {
                string source = Decode(CString(argument)); string first = _config.SubstituteLocal(source); Limit(first, 1024);
                result.Append("\n<a herf=\"cmd:").Append(CString(command)).Append(',');
                string second = _config.SubstituteLocal(source); Limit(second, 1024); result.Append(Narrow(second)).Append("\">").Append(label).Append("</a>");
            }
            catch (PhysicalLimitException) { throw; }
            catch { result.Append("\n<a herf=\"cmd:").Append(CString(command)).Append(",\">~rERROR ").Append(label).Append("</a>"); }
        }
        return result.ToString();
    }

    /// <summary>이 장치가 만든 장기 Config만 등록부에서 반납한다.</summary>
    public void Dispose()
    {
        // 원본 소유 목록이 끝날 때 생성 역순으로 설정 객체를 반납한다.
        for (int i = _owned.Count - 1; i >= 0; i--) _owned[i].Dispose();
        _owned.Clear();
    }
}
