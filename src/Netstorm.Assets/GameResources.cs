using System.Globalization;

namespace Netstorm.Assets;

/// <summary>언어 대체 결과를 포함하는 미션 스크립트. Path는 가상 파일 시스템 경로다.</summary>
public sealed record LoadedMission(string Path, string Language, MissionScript Script);

/// <summary>실행 시 사용할 파일 시스템·설정·번역·미션 조회를 한 곳에 구성한다.</summary>
public sealed class GameResources
{
    /// <summary>설정 파일이 없는 개발용 데이터에서도 사용하는 최소 경로 기본값.</summary>
    private const string DefaultSettings = """
        DataDir = "\D"
        fortSpec = "{DataDir}\{local.1}.fort"
        GamePalSpec = "{DataDir}\{local.1}.COL"
        languageSpec = "{DataDir}\config.{local.1}"
        missionSpec = "{DataDir}\{local.1}.{currentLanguage}"
        fortPal = "gifcloud"
        battlePal = "gifcloud"
        """;

    /// <summary>자산을 읽는 공통 가상 파일 시스템.</summary>
    public GameFileSystem Files { get; }

    /// <summary>원본 설정 조회·치환과 미션 머리 값 조회에 사용하는 설정 층.</summary>
    public ConfigStore Settings { get; } = new();

    /// <summary>요청한 언어. 개별 파일이 영어로 대체되어도 이 값은 유지한다.</summary>
    public string Language { get; }

    /// <summary>선택한 용어표 경로. 영어 대체를 포함하며 파일이 없으면 null.</summary>
    public string? LanguageConfigPath { get; }

    /// <summary>선택한 UI 번역표 경로. 영어 대체를 포함하며 파일이 없으면 null.</summary>
    public string? TranslationPath { get; }

    /// <summary>선택한 언어의 UI 번역표. 없으면 원문을 반환하는 빈 표.</summary>
    public XlatTable Translations { get; }

    /// <summary>options → setup의 첫 일치 우선 설정과 언어별 용어표를 구성한다.</summary>
    /// <param name="files">자산 파일 시스템.</param>
    /// <param name="language">명시적 언어 선택. 생략하면 설정, 그 값도 없으면 OS UI 언어를 사용한다.</param>
    /// <param name="culture">OS 언어 대체값. 테스트에서는 고정된 문화권을 전달할 수 있다.</param>
    public GameResources(GameFileSystem files, string? language = null, CultureInfo? culture = null)
    {
        Files = files;
        Settings.Push(new ConfigText(DefaultSettings));
        // 원본은 같은 설정 객체에 파일을 이어 붙인다. 먼저 읽은 options의 값이 우선한다.
        var texts = new List<string>();
        foreach (string path in new[] { "d/options.cfg", "d/setup.cfg" })
        {
            byte[]? data = Files.TryReadAllBytes(path);
            if (data != null)
            {
                texts.Add(ConfigText.FromFileBytes(data).Text);
            }
        }
        Settings.Push(new ConfigText(string.Join('\n', texts)));
        Language = GameLanguage.Normalize(language ?? Settings.Get("currentLanguage")
            ?? GameLanguage.FromCulture(culture ?? CultureInfo.CurrentUICulture));

        // 일부만 번역된 용어표도 누락 키는 영어 표에서 찾을 수 있도록 바닥에 영어를 둔다.
        string englishPath = LanguageSpec("languageSpec", GameLanguage.English, GameLanguage.English);
        byte[]? english = Files.TryReadAllBytes(englishPath);
        if (english != null)
        {
            Settings.Push(ConfigText.FromFileBytes(english));
            LanguageConfigPath = englishPath;
        }
        if (Language != GameLanguage.English)
        {
            string wanted = LanguageSpec("languageSpec", Language, Language);
            byte[]? translated = Files.TryReadAllBytes(wanted);
            if (translated != null)
            {
                Settings.Push(ConfigText.FromFileBytes(translated));
                LanguageConfigPath = wanted;
            }
        }
        // 용어표에 같은 키가 있더라도 실제 선택 언어를 유지한다. 원본 파일은 수정하지 않는다.
        Settings.Push(LanguageOverride(Language));
        TranslationPath = GameLanguage.ResolveFile(Files, "d/xlat", Language);
        Translations = TranslationPath == null ? XlatTable.Empty
            : XlatTable.FromFileBytes(Files.ReadAllBytes(TranslationPath));
        Settings.MissionLoader = name => TryLoadMission(name)?.Script.Header;
    }

    /// <summary>GamePalSpec과 모드별 팔레트 이름으로 팔레트를 읽는다.</summary>
    public Palette LoadPalette(string paletteKey = "fortPal") => LoadNamedPalette(PaletteName(paletteKey));

    /// <summary>설정 키가 가리키는 팔레트 이름. 설정이 없으면 원본 기본값 gifcloud.</summary>
    public string PaletteName(string paletteKey = "fortPal") => Settings.Get(paletteKey) ?? "gifcloud";

    /// <summary>GamePalSpec 위치(디스크·아카이브)에 있는 모든 팔레트 이름을 이름순으로 찾는다.</summary>
    public IReadOnlyList<string> FindPaletteNames() =>
        [.. Files.Find(Settings.ExpandSpec("GamePalSpec", "*")).Select(Path.GetFileNameWithoutExtension).OfType<string>()];

    /// <summary>GamePalSpec에 이름을 넣어 팔레트 하나를 읽는다.</summary>
    public Palette LoadNamedPalette(string name) =>
        Palette.Parse(Files.ReadAllBytes(Settings.ExpandSpec("GamePalSpec", name)));

    /// <summary>설정의 DataDir에서 셰이프 데이터베이스를 읽는다.</summary>
    public ShapeDatabase LoadShapes() => new(Files.ReadAllBytes($"{Settings.Get("DataDir")}/{ShapeDatabase.FileName}"));

    /// <summary>모든 타입을 디스크·아카이브의 공통 조회 순서로 읽는다.</summary>
    public TypeCatalog LoadTypes() => new(Files);

    /// <summary>명시한 실제 파일, 가상 경로, fortSpec으로 만든 맵 이름 순서로 .fort를 읽는다.</summary>
    public FortFile LoadFort(string name, TypeCatalog catalog)
    {
        if (File.Exists(name))
        {
            return new FortFile(File.ReadAllBytes(name), catalog);
        }
        string path = name.Contains('/') || name.Contains('\\')
            ? name : Settings.ExpandSpec("fortSpec", name.EndsWith(".fort", StringComparison.OrdinalIgnoreCase)
                ? name[..^5] : name);
        return new FortFile(Files.ReadAllBytes(path), catalog);
    }

    /// <summary>missionSpec에서 미션을 찾고 해당 언어 파일이 없으면 같은 지정식의 영어 파일을 찾는다.</summary>
    public LoadedMission? TryLoadMission(string name)
    {
        string path = LanguageSpec("missionSpec", name, Language);
        byte[]? data = Files.TryReadAllBytes(path);
        string actualLanguage = Language;
        if (data == null && Language != GameLanguage.English)
        {
            path = LanguageSpec("missionSpec", name, GameLanguage.English);
            data = Files.TryReadAllBytes(path);
            actualLanguage = GameLanguage.English;
        }
        return data == null ? null : new LoadedMission(path, actualLanguage, MissionScript.FromFileBytes(data));
    }

    /// <summary>임시 언어 층으로 지정식을 치환하고 호출 뒤 원래 선택 언어로 되돌린다.</summary>
    private string LanguageSpec(string key, string local, string language)
    {
        Settings.Push(LanguageOverride(language));
        try
        {
            return Settings.ExpandSpec(key, local);
        }
        finally
        {
            Settings.Pop();
        }
    }

    /// <summary>선택 언어를 설정식에 반영하는 임시 설정 텍스트를 만든다.</summary>
    private static ConfigText LanguageOverride(string language) =>
        ConfigStore.FromPairs(new Dictionary<string, string> { ["currentLanguage"] = language });
}
