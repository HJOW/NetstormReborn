using System.Globalization;
using System.Text;

namespace Netstorm.Assets.Tests;

/// <summary>실행 자산 설정의 우선순위·언어 대체·공통 파일 조회를 검사한다.</summary>
public sealed class GameResourcesTests
{
    /// <summary>같은 설정 객체 안에서는 options가 setup보다 우선하고 언어 용어표는 별도 층으로 적용된다.</summary>
    [Fact]
    public void Startup_OptionsWinAndLocalizedTermsOverlayEnglish()
    {
        using var data = new ResourceDirectory();
        data.Write("d/options.cfg", new ConfigFile("currentLanguage = german\nfortPal = selected\n").Encode());
        data.WriteText("d/setup.cfg", "currentLanguage = english\nfortPal = default\nbattlePal = cloudy\n");
        data.WriteText("d/config.english", "vortex = Temple\npriest = High Priest\n");
        data.WriteText("d/config.german", "vortex = Tempel\n");
        var resources = new GameResources(data.Files());

        Assert.Equal("german", resources.Language);
        Assert.Equal("selected", resources.Settings.Get("fortPal"));
        Assert.Equal("cloudy", resources.Settings.Get("battlePal"));
        Assert.Equal("Tempel / High Priest", resources.Settings.Expand("{vortex} / {priest}"));
        Assert.Equal(@"\D\config.german", resources.LanguageConfigPath);
    }

    /// <summary>명시 언어가 설정보다 우선하며 UTF-8 한국어 용어표·번역표·미션을 실제로 조회한다.</summary>
    [Fact]
    public void ExplicitKorean_LoadsUtf8ResourcesAndMissionHeaderReferences()
    {
        using var data = new ResourceDirectory();
        data.WriteText("d/options.cfg", "currentLanguage = german\n");
        data.WriteText("d/config.english", "vortex = Temple\npriest = High Priest\n");
        data.WriteText("d/config.korean", "vortex = 신전\ncurrentLanguage = french\n");
        data.WriteText("d/xlat.korean", "***\nCancel\n---\n취소\n===\n");
        data.WriteText("d/tutorial1.korean", "[Header]\nname = 다리 놓기\n[A.]\n<h2>환영합니다</h2>\n");
        var resources = new GameResources(data.Files(), "KOREAN");

        Assert.Equal("korean", resources.Language);
        Assert.Equal("korean", resources.Settings.Get("currentLanguage"));
        Assert.Equal("신전 / High Priest", resources.Settings.Expand("{vortex} / {priest}"));
        Assert.Equal("d/xlat.korean", resources.TranslationPath);
        Assert.Equal("취소", resources.Translations.Translate("Cancel"));
        Assert.Equal("다리 놓기", resources.Settings.Expand("{@tutorial1.name}"));
        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("tutorial1"));
        Assert.Equal("korean", mission.Language);
        Assert.Contains("환영합니다", mission.Script.GetSection("A."));
    }

    /// <summary>지정식의 폴더·파일 접두어를 보존하여 영어로 대체하고 원래 선택 언어와 층 수를 복구한다.</summary>
    [Fact]
    public void MissingLanguage_FallbackPreservesCustomSpecsAndSettings()
    {
        using var data = new ResourceDirectory();
        data.WriteText("d/setup.cfg", "languageSpec = words/config.{local.1}\n"
            + "missionSpec = scripts/mission-{local.1}.{currentLanguage}\n");
        data.WriteText("words/config.english", "vortex = Temple\n");
        data.WriteText("d/xlat.english", "placeholder");
        data.WriteText("scripts/mission-intro.english", "[Header]\nname = Welcome\n");
        var resources = new GameResources(data.Files(), "korean");
        int layers = resources.Settings.LayerCount;

        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("intro"));
        Assert.Equal("scripts/mission-intro.english", mission.Path);
        Assert.Equal("english", mission.Language);
        Assert.Equal("words/config.english", resources.LanguageConfigPath);
        Assert.Equal("d/xlat.english", resources.TranslationPath);
        Assert.Equal("Welcome", resources.Settings.Expand("{@intro.name}"));
        Assert.Null(resources.TryLoadMission("absent"));
        Assert.Equal("{Not Found:@ABSENT.NAME}", resources.Settings.Expand("{@absent.name}"));
        Assert.Equal("korean", resources.Settings.Get("currentLanguage"));
        Assert.Equal(layers, resources.Settings.LayerCount);
    }

    /// <summary>설정이 없으면 OS 언어를 선택하고 지원하지 않는 언어는 영어를 사용한다.</summary>
    [Theory]
    [InlineData("ko-KR", "korean")]
    [InlineData("de-DE", "german")]
    [InlineData("en-US", "english")]
    [InlineData("it-IT", "english")]
    public void MissingSettings_UsesCulture(string culture, string expected)
    {
        using var data = new ResourceDirectory();
        var resources = new GameResources(data.Files(), culture: new CultureInfo(culture));
        Assert.Equal(expected, resources.Language);
        Assert.Null(resources.LanguageConfigPath);
        Assert.Equal("Cancel", resources.Translations.Translate("Cancel"));
    }

    /// <summary>설정에서 선택한 다른 팔레트를 VFS에서 읽어 GIFCLOUD 고정 경로 의존성을 검사한다.</summary>
    [Fact]
    public void Palette_UsesConfiguredNameAndPath()
    {
        using var data = new ResourceDirectory();
        data.WriteText("d/options.cfg", "battlePal = alternate\nGamePalSpec = colors/{local.1}.COL\n");
        byte[] palette = new byte[Palette.ColorCount * 4];
        palette[4] = 17;
        palette[5] = 42;
        palette[6] = 99;
        data.Write("colors/alternate.COL", palette);
        var resources = new GameResources(data.Files());
        Assert.Equal(new Rgb(17, 42, 99), resources.LoadPalette("battlePal")[1]);
    }

    /// <summary>원본 셰이프·팔레트·타입·맵과 번역 없는 한국어 미션의 영어 대체를 통합 검사한다.</summary>
    [Fact]
    public void Original_LoadsGraphicsMapAndEnglishMissionFallback()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "korean");
        Assert.Equal(@"\D\config.english", resources.LanguageConfigPath);
        Assert.Equal("Temple", resources.Settings.Get("vortex"));
        Assert.Equal("korean", resources.Settings.Get("currentLanguage"));
        Assert.Equal(116, resources.LoadShapes().Blocks.Count);
        Assert.Equal(Palette.Load(OriginalData.RequireFile("d/GIFCLOUD.COL"))[42], resources.LoadPalette("battlePal")[42]);
        TypeCatalog catalog = resources.LoadTypes();
        FortFile map = resources.LoadFort("savetheisland", catalog);
        Assert.NotEmpty(map.Chaff);
        Assert.Equal(map.Section("Chaff").ToArray(), resources.LoadFort("savetheisland.fort", catalog).Section("Chaff").ToArray());
        Assert.Equal(map.Section("Chaff").ToArray(), resources.LoadFort(@"\D\savetheisland.fort", catalog).Section("Chaff").ToArray());
        Assert.NotEmpty(resources.LoadFort(OriginalData.RequireFile("d/b0.fort"), catalog).Chaff);
        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("tutorial1"));
        Assert.Equal("english", mission.Language);
        Assert.Equal("Tutorial", mission.Script.GetHeader("missionType"));
        Assert.Equal("Tutorial", resources.Settings.Expand("{@tutorial1.missionType}"));
    }

    /// <summary>느슨한 UTF-8 타입 정의가 아카이브보다 우선하면서 타입 번호와 나머지 정의는 보존된다.</summary>
    [Fact]
    public void Original_TypeCatalogHonorsDiskOverride()
    {
        GameFileSystem original = GameFileSystem.Open(OriginalData.RequireDirectory());
        using var data = new ResourceDirectory();
        data.WriteText("d/priest.type", "typename priest\ntypeflags priest;\n{helpText = \"한글 사제\";}\n");
        var resources = new GameResources(new GameFileSystem(data.Path, original.Archives));
        TypeCatalog catalog = resources.LoadTypes();
        Assert.Equal(116, catalog.Types.Count);
        Assert.Equal("한글 사제", catalog.Find("priest")!.Definition.GetString("helpText"));
        Assert.Equal(Array.IndexOf(TypeLoadOrder.Names.ToArray(), "priest"), catalog.Find("priest")!.LoadIndex);
        Assert.NotEmpty(catalog.Find("isle")!.Definition.Clusters);
    }

    /// <summary>검사마다 고유한 임시 자산 폴더를 만들고 해당 폴더만 정리한다.</summary>
    private sealed class ResourceDirectory : IDisposable
    {
        /// <summary>이 검사에서 만든 임시 폴더의 절대 경로.</summary>
        public string Path { get; } = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "ns-resources-" + Guid.NewGuid().ToString("N"));

        /// <summary>검사용 빈 데이터 폴더를 만든다.</summary>
        public ResourceDirectory() => Directory.CreateDirectory(Path);

        /// <summary>검사용 파일 시스템을 연다.</summary>
        public GameFileSystem Files() => GameFileSystem.Open(Path);

        /// <summary>새 텍스트 자산을 UTF-8로 쓴다.</summary>
        public void WriteText(string relative, string text) => Write(relative, Encoding.UTF8.GetBytes(text));

        /// <summary>하위 폴더를 만들고 검사용 파일을 쓴다.</summary>
        public void Write(string relative, byte[] bytes)
        {
            string target = System.IO.Path.Combine(Path, relative);
            Directory.CreateDirectory(System.IO.Path.GetDirectoryName(target)!);
            File.WriteAllBytes(target, bytes);
        }

        /// <summary>이 객체가 만든 고유 임시 폴더를 정리한다.</summary>
        public void Dispose() => Directory.Delete(Path, recursive: true);
    }
}
