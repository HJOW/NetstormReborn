using System.Globalization;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 Config 등록부와 HTML 창 사이의 Tell 경계다.

/// <summary>Tell의 마지막 원본 이름을 NUL 포함 256바이트에 보관한다.</summary>
public sealed class TellSectionState
{
    /// <summary>현재 이름과 NUL 뒤 잔여 바이트를 같은 수명에 보관한다.</summary>
    public byte[] Current { get; } = new byte[256];
}

/// <summary>Tell 조회 시 실제 선택된 설정과 이전 이름 경계다.</summary>
public sealed class TellSectionHooks
{
    /// <summary>현재 이름이 달라지기 직전 이전 이름을 기록한다.</summary>
    public Action<string>? Prior { get; init; }
    /// <summary>첫 점 앞의 등록 객체에서 뒷부분 섹션을 찾는다.</summary>
    public Func<string, string, string?>? Named { get; init; }
    /// <summary>현재 미션의 전체 이름을 조회한다.</summary>
    public Func<string, string?>? Mission { get; init; }
    /// <summary>매번 새 Tell 설정을 적재해 전체 이름을 조회한다.</summary>
    public Func<string, string?>? Fallback { get; init; }
}

/// <summary>Tell 대상의 매개변수를 순차 치환하고 같은 param 객체에 붙인다.</summary>
public static class TellParameters
{
    /// <summary>첫 NUL 앞까지의 원본 BYTE 표현을 반환한다.</summary>
    private static string CString(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }

    /// <summary>원본 1024BYTE 지역 복사와 80BYTE 토큰 helper로 이름 및 인자를 읽는다.</summary>
    /// <param name="source">Latin1으로 보존한 원본 BYTE 문자열.</param>
    /// <param name="substitute">현재 param을 볼 수 있는 치환기.</param>
    /// <param name="append">같은 param에 서식이 완성된 항목을 붙이는 함수.</param>
    public static string Parse(string source, Func<string, string>? substitute, Action<string>? append)
    {
        ArgumentNullException.ThrowIfNull(source);
        source = CString(source);
        if (source.Length >= 1024) throw new ArgumentException("Tell 원본 1024BYTE 지역 입력 범위 초과");
        RawTextTokenResult name = RawTextToken.Read(source, 0, "("); int offset = name.Next; uint index = 1;
        // 마지막 빈 토큰에서 끝내며 앞 인자의 설정 변경을 다음 치환이 관측한다.
        while (true)
        {
            RawTextTokenResult token = RawTextToken.Read(source, offset, ",)"); offset = token.Next;
            if (token.Text.Length == 0) break;
            if (substitute is null || append is null) throw new InvalidOperationException("Tell 매개변수 치환/Append 미연결");
            string value = CString(substitute(token.Text));
            append(index.ToString(CultureInfo.InvariantCulture) + "=\"" + value + "\""); index++;
        }
        return name.Text;
    }
}

/// <summary>Tell 이름 이력과 등록 객체·미션·새 tell 파일의 조회 순서를 구현한다.</summary>
public static class TellSection
{
    /// <summary>선택된 경계만 요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("Tell 선택 경계 미연결");
    /// <summary>원본 문자열 첫 NUL 이전만 읽는다.</summary>
    private static string CString(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>ASCII 문자만 소문자로 바꿔 원본 이력 이름을 비교한다.</summary>
    private static string Lower(string text) => string.Concat(text.Select(c => c is >= 'A' and <= 'Z' ? (char)(c + 32) : c));
    /// <summary>원본 BYTE 표현을 설정의 유니코드 경계로 변환한다.</summary>
    private static string Decode(string raw) => OriginalText.DecodeWindows1252(Encoding.Latin1.GetBytes(raw));
    /// <summary>설정의 치환 결과를 원본 BYTE 표현으로 변환한다.</summary>
    private static string Encode(string value) => Encoding.Latin1.GetString(OriginalText.EncodeWindows1252(value));

    /// <summary>이름을 먼저 저장하고 등록된 접두·미션·fallback 순으로 본문을 선택한다.</summary>
    public static string? Find(TellSectionState state, string name, TellSectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(name); ArgumentNullException.ThrowIfNull(hooks);
        string local = CString(name);
        if (local.Length >= state.Current.Length) throw new ArgumentException("Tell 이름 256BYTE 범위 초과");
        int end = Array.IndexOf(state.Current, (byte)0);
        if (end < 0) throw new InvalidOperationException("Tell 현재 이름 NUL 없음");
        string previous = Encoding.Latin1.GetString(state.Current, 0, end);
        if (Lower(previous) != Lower(local)) Need(hooks.Prior)(previous);
        Encoding.Latin1.GetBytes(local, state.Current); state.Current[local.Length] = 0;
        int dot = local.IndexOf('.');
        if (dot >= 0 && Need(hooks.Named)(local[..dot], local[(dot + 1)..]) is { } named) return named;
        if (Need(hooks.Mission)(local) is { } mission) return mission;
        return Need(hooks.Fallback)(local);
    }

    /// <summary>동일 Config 등록부와 파일 reader로 섹션을 찾는다. 빈 본문과 누락은 구분한다.</summary>
    public static string? FindInConfig(TellSectionState state, string name, ConfigInterface config, Func<string, string?>? mission)
    {
        ArgumentNullException.ThrowIfNull(config);
        return Find(state, name, new TellSectionHooks
        {
            Prior = previous => config.Set("global.priorTell", Decode(previous)),
            Named = (prefix, section) => config.Registry.FindByName(Decode(prefix))?.Section(Decode(section)),
            Mission = target => mission?.Invoke(Decode(target)),
            Fallback = target =>
            {
                using var temporary = new Config(config.Registry, "tell");
                string path = config.PathSpec("missionSpec", "tell");
                if (config.Reader?.Invoke(path) is { } bytes) temporary.LoadBytes(bytes);
                return temporary.Section(Decode(target));
            }
        });
    }

    /// <summary>실제 param을 창 생성 전체 동안 등록하고 본문 발견 여부와 생성 serial을 반환한다.</summary>
    public static (bool Found, uint Serial) ShowInConfig(TellSectionState state, string target, uint suppress,
        ConfigInterface config, Func<string, string?>? mission, Func<string, uint, uint, uint, uint>? show)
    {
        ArgumentNullException.ThrowIfNull(config);
        using var parameters = new Config(config.Registry, "param");
        string name = TellParameters.Parse(target, value => Encode(config.SubstituteLocal(Decode(value))), line => parameters.Append(Decode(line)));
        string? body = FindInConfig(state, name, config, mission);
        return body is null ? (false, 0) : (true, Need(show)(body, suppress, 0, 0));
    }
}
