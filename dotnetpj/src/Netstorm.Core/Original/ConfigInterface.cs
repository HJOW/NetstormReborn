using Netstorm.Assets;

namespace Netstorm.Core.Original;

/// <summary>
/// 언어·프로그램 설정 조회를 공통 저장소에 연결하는 인터페이스이다.
/// 근거: cpppj/src/o/ConfigInterface.h/.cpp
/// (원본 00440e0·00441790·00440f30·00441090·004410f0·00441270·00441300·00441470·004414d0·00441520·00441660·00441db0).
/// (R3k-11, 2026-10-31)
/// CRT atol·`;` 목록 함수는 기존 <see cref="ConfigNumbers"/>를 그대로 쓴다.
/// </summary>
public sealed class ConfigInterface
{
    /// <summary>시작 순서(원본 FUN_00441790의 설정 부분)에 필요한 값.</summary>
    public sealed class StartupOptions
    {
        /// <summary>실행 인자(원본 param_2). `키=값;키=값` 꼴.</summary>
        public string CommandLine { get; set; } = "";

        /// <summary>주 버전. MAJOR_VERSION.</summary>
        public int MajorVersion { get; set; }

        /// <summary>부 버전. MINOR_VERSION.</summary>
        public int MinorVersion { get; set; }

        /// <summary>설정 파일이 있는 폴더(원본은 setup.cfg를 찾은 위치로 정한다).</summary>
        public string DataPrefix { get; set; } = "d\\";

        /// <summary>환경 변수 IDENTITY 또는 USER. 비어 있으면 그 파일은 건너뛴다.</summary>
        public string Identity { get; set; } = "";

        /// <summary>InstallDir에 쓸 값(원본은 데이터 폴더의 상위 폴더).</summary>
        public string InstallDir { get; set; } = "";

        /// <summary>CDDir에 쓸 값(원본은 CD를 찾지 못하면 InstallDir과 같다).</summary>
        public string CdDir { get; set; } = "";
    }

    /// <summary>원본 00440e0의 `[ARGS]` 섹션 이름.</summary>
    private const string ArgumentsSection = "ARGS";

    /// <summary>원본 00440e0의 `[END]` 섹션 이름.</summary>
    private const string EndSection = "END";

    /// <summary>`local.1`~`local.3`용 임시 설정 객체의 이름(원본 "local").</summary>
    private const string LocalObjectName = "local";

    /// <summary>게임 경로 표기(`d\options.cfg`, `\D\config.english`)의 파일을 읽는다. 없으면 null.</summary>
    private readonly Func<string, byte[]?>? _reader;

    /// <summary>원본 CRT 순서대로 language→configuration→global→fort를 같은 Registry에 등록한다(004fc8c0~004fc920).</summary>
    /// <param name="reader">게임 경로 표기의 파일을 읽는 공급자. 없으면 파일 읽기를 건너뛴다.</param>
    public ConfigInterface(Func<string, byte[]?>? reader)
    {
        _reader = reader;
        _language = new Config(_registry);
        _configuration = new Config(_registry);
        _global = new Config(_registry, "global");
        _fort = new Config(_registry, "fort");
        // `{@미션.이름}`: 원본 00440760의 missionSpec 경로로 미션 스크립트를 임시 객체에 읽는다
        _registry.LoadMission = (temporary, mission) => LoadFile(temporary, PathSpec("missionSpec", mission));
    }

    /// <summary>원본 설정 객체(0x18바이트)의 언어 용어표.</summary>
    private readonly Config _language;

    /// <summary>현재 언어와 실행 설정을 조회하는 인터페이스.</summary>
    private readonly Config _configuration;

    /// <summary>004fc900이 만든 00558e40의 장기 "global." 객체이며 전역 루트와 다른 버퍼를 소유한다.</summary>
    private readonly Config _global;

    /// <summary>004fc920이 만든 00558e58의 장기 "fort." 객체이며 Session 교체보다 오래 살아 현재 Fort 설정을 보존한다.</summary>
    private readonly Config _fort;

    /// <summary>설정 객체를 조회하는 등록부.</summary>
    private readonly ConfigRegistry _registry = new();

    /// <summary>원본 DAT_00558dd0: 값을 쓸 때마다 부르는 알림.</summary>
    public Action? OnChanged { get; set; }

    /// <summary>전역 설정(theConfiguration).</summary>
    public Config Configuration => _configuration;

    /// <summary>언어 용어표.</summary>
    public Config Language => _language;

    /// <summary>설정 객체 목록.</summary>
    public ConfigRegistry Registry => _registry;

    /// <summary>파일 읽기 공급자. 미션 스크립트처럼 다른 설정 객체에 파일을 읽어 넣을 때 쓴다.</summary>
    public Func<string, byte[]?>? Reader => _reader;

    /// <summary>파일이 있으면 읽어 설정 객체에 더한다(원본 FUN_00440380).</summary>
    /// <param name="config">읽어 넣을 객체.</param>
    /// <param name="path">게임 경로.</param>
    private void LoadFile(Config config, string path)
    {
        if (_reader == null)
        {
            return;
        }
        byte[]? bytes = _reader(path);
        if (bytes != null)
        {
            config.LoadBytes(bytes);
        }
    }

    /// <summary>원본 00440e0. `[ARGS]`, 인자(첫 `@` 인자 전까지는 글, 그 뒤는 파일), `[END]`.</summary>
    /// <param name="arguments">실행 인자 목록.</param>
    public void Load(IReadOnlyList<string> arguments)
    {
        ArgumentNullException.ThrowIfNull(arguments);
        _configuration.Clear();
        _configuration.EnvironmentEnabled = true;
        _configuration.AppendSection(ArgumentsSection);
        bool inlineMode = true;
        // 인자를 받은 순서대로 처리한다
        foreach (string argument in arguments)
        {
            string path = argument;
            if (path.Length != 0 && path[0] == '@')
            {
                path = path.Substring(1);
            }
            else if (inlineMode)
            {
                _configuration.AppendArguments(path, _reader);
                continue;
            }
            inlineMode = false;
            if (path.Length == 0)
            {
                continue;
            }
            // 파일마다 `[파일이름]` 섹션 줄을 먼저 쓴다
            _configuration.AppendSection(FileNamePart(path));
            LoadFile(_configuration, path);
        }
        _configuration.AppendSection(EndSection);
    }

    /// <summary>원본 FUN_00459b20(경로, 1): 마지막 `\` 또는 `:` 앞의 파일 이름. `/`는 받는 것도 허용한다.</summary>
    /// <param name="path">게임 경로.</param>
    private static string FileNamePart(string path)
    {
        int separator = path.LastIndexOfAny(new[] { '\\', ':', '/' });
        return separator < 0 ? path : path.Substring(separator + 1);
    }

    /// <summary>원본 00441790과 00440e0의 인자 구성 그대로. demo.cfg 경로는 만들지 않는다. 원본 그대로.</summary>
    /// <param name="options">시작 순서에 필요한 값.</param>
    public void Startup(StartupOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        string major = options.MajorVersion.ToString(System.Globalization.CultureInfo.InvariantCulture);
        string minor = options.MinorVersion.ToString(System.Globalization.CultureInfo.InvariantCulture);
        var arguments = new List<string>
        {
            options.CommandLine,
            "MAJOR_VERSION=" + major + ";MINOR_VERSION=" + minor + ";version=\"v" + major + "." + minor + "\";",
            "@",
            options.Identity.Length == 0 ? "" : options.DataPrefix + options.Identity + ".cfg",
            options.DataPrefix + "user.cfg",
            options.DataPrefix + "options.cfg",
            options.DataPrefix + "dev.cfg",
            options.DataPrefix + "guild.cfg",
            options.DataPrefix + "setup.cfg",
        };
        Load(arguments);
        Set("InstallDir", options.InstallDir);
        Set("CDDir", options.CdDir.Length == 0 ? options.InstallDir : options.CdDir);
    }

    /// <summary>원본 00441090. 치환한 값을 정수로. 없으면 0.</summary>
    /// <param name="key">찾을 키.</param>
    public int GetInt(string key) => ConfigNumbers.ParseLong(_configuration.Get(key));

    /// <summary>원본 004410f0: `local.1`~`local.3`에 인자를 넣고 키를 조회한다(경로 지정값).</summary>
    /// <param name="key">찾을 키.</param>
    /// <param name="argument1">`local.1`에 넣을 값.</param>
    /// <param name="argument2">`local.2`에 넣을 값.</param>
    /// <param name="argument3">`local.3`에 넣을 값.</param>
    public string PathSpec(string key, string? argument1 = null, string? argument2 = null, string? argument3 = null)
    {
        using var local = new Config(_registry, LocalObjectName);
        AppendLocalArgument(local, 1, argument1);
        AppendLocalArgument(local, 2, argument2);
        AppendLocalArgument(local, 3, argument3);
        return _configuration.Get(key) ?? "";
    }

    /// <summary>임시 `local` 객체에 `번호="값"` 줄을 쓴다. 원본 서식 "1=\"%s\"" 꼴.</summary>
    /// <param name="local">쓸 임시 객체.</param>
    /// <param name="number">local 번호.</param>
    /// <param name="argument">적을 값. 없으면 줄을 쓰지 않는다.</param>
    private static void AppendLocalArgument(Config local, int number, string? argument)
    {
        if (argument != null)
        {
            local.Append(number.ToString(System.Globalization.CultureInfo.InvariantCulture) + "=\"" + argument + "\"");
        }
    }

    /// <summary>원본 00441270: 키가 있고 값이 지금 값과 다르면 바꾸고 참. 설정으로 기본값을 덮어쓸 때 쓴다.</summary>
    /// <param name="key">찾을 키.</param>
    /// <param name="value">비교·대입할 값. 바뀌었을 때만 갱신한다.</param>
    public bool ReadInt(string key, ref int value)
    {
        string? text = _configuration.Get(key);
        if (text == null)
        {
            return false;
        }
        int parsed = ConfigNumbers.ParseLong(text);
        if (parsed == value)
        {
            return false;
        }
        value = parsed;
        return true;
    }

    /// <summary>원본 00441300: 문자열판. 키가 있고 값이 다르면 바꾸고 참. 바이트 단위 비교다.</summary>
    /// <param name="key">찾을 키.</param>
    /// <param name="value">비교·대입할 값. 바뀌었을 때만 갱신한다.</param>
    public bool ReadString(string key, ref string value)
    {
        string? text = _configuration.Get(key);
        if (text == null || text == value)
        {
            return false;
        }
        value = text;
        return true;
    }

    /// <summary>원본 00441470: 0에서 시작해 ReadInt가 값을 바꿨고 그 값이 expected면 참. 그래서 expected가 0이면 항상 거짓이다(원본 그대로).</summary>
    /// <param name="key">찾을 키.</param>
    /// <param name="expected">기대값.</param>
    public bool IntEquals(string key, int expected)
    {
        int value = 0;
        return ReadInt(key, ref value) && value == expected;
    }

    /// <summary>원본 004414d0: 값을 쓰고 변경 알림을 부른다. 이전에 있었으면 참.</summary>
    /// <param name="key">쓸 키.</param>
    /// <param name="value">쓸 값.</param>
    public bool Set(string key, string value)
    {
        bool existed = _configuration.Set(key, value);
        OnChanged?.Invoke();
        return existed;
    }

    /// <summary>원본 00441520: 정수를 "%d"로 써서 Set.</summary>
    /// <param name="key">쓸 키.</param>
    /// <param name="value">쓸 값.</param>
    public bool SetInt(string key, int value) =>
        Set(key, value.ToString(System.Globalization.CultureInfo.InvariantCulture));

    /// <summary>원본 00441660/00441740: `local.1`~`local.3`을 넣고 글 전체를 치환한다.</summary>
    /// <param name="source">치환할 원본 문자열.</param>
    /// <param name="argument1">`local.1`에 넣을 값.</param>
    /// <param name="argument2">`local.2`에 넣을 값.</param>
    /// <param name="argument3">`local.3`에 넣을 값.</param>
    public string SubstituteLocal(string source, string? argument1 = null, string? argument2 = null, string? argument3 = null)
    {
        using var local = new Config(_registry, LocalObjectName);
        AppendLocalArgument(local, 1, argument1);
        AppendLocalArgument(local, 2, argument2);
        AppendLocalArgument(local, 3, argument3);
        return _configuration.Substitute(source);
    }

    /// <summary>원본 00441db0: 용어표를 비우고 `LanguageSpec` 경로의 파일을 읽는다.</summary>
    /// <param name="language">언어 이름.</param>
    public void LoadLanguage(string language)
    {
        _language.Clear();
        LoadFile(_language, PathSpec("LanguageSpec", language));
    }

    /// <summary>원본 00440f30: `[파일이름]` 섹션과 `[END]` 섹션을 이어 저장할 평문을 만든다. 파일 본문이 있고 서명이 없으면 붙인다. 변경 표시를 끈다. 파일 쓰기는 SaveFile에서 한다.</summary>
    /// <param name="fileName">저장할 파일 이름.</param>
    public string SaveText(string fileName)
    {
        string result = "";
        string? body = _configuration.Section(FileNamePart(fileName));
        if (body != null)
        {
            if (!body.StartsWith(ConfigTextOps.Signature, StringComparison.Ordinal))
            {
                result += ConfigTextOps.Signature;
            }
            result += body;
        }
        string? end = _configuration.Section(EndSection);
        if (end != null)
        {
            result += end;
        }
        _configuration.Changed = false;
        return result;
    }

    /// <summary>원본 00440f30: 저장 바이트를 쓰고 변경 표시를 끈다. 쓰기 실패 시 이전 변경 표시를 보존한다.</summary>
    /// <param name="fileName">저장할 파일 이름.</param>
    /// <param name="writer">경로와 완성된 바이트를 디스크에 쓰는 경계.</param>
    public void SaveFile(string fileName, Action<string, byte[]> writer)
    {
        bool changed = _configuration.Changed;
        try
        {
            byte[] bytes = EncodeConfigFile(SaveText(fileName));
            _configuration.Changed = changed;
            writer(fileName, bytes);
            _configuration.Changed = false;
        }
        catch
        {
            _configuration.Changed = changed;
            throw;
        }
    }

    /// <summary>SaveText가 정한 평문을 Windows-1252로 되돌려 XOR한다. 서명·BOM을 추가하지 않는다.</summary>
    /// <param name="text">저장할 평문.</param>
    public static byte[] EncodeConfigFile(string text)
    {
        byte[] bytes = OriginalText.EncodeWindows1252(text);
        XorCipher.Apply(bytes);
        return bytes;
    }
}
