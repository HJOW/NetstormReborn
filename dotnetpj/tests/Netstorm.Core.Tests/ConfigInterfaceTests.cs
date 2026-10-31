using System;
using System.Collections.Generic;
using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 수동 검사(ConfigInterfaceTests.cpp: 시작 순서·타입 접근·미션 조회·저장·인코딩·파일·섹션·정적 소유·한정 쓰기)를 C#으로 옮긴 계약 검사다.
/// 근거: cpppj/tests/ConfigInterfaceTests.cpp. 기대값은 cpp 원본 그대로이며 C# 결과로 바꾸지 않는다. (R3k-11, 2026-10-31)
/// ParseLong/ListItem 1종은 기존 ConfigNumbersTests가, MissionScript 1종은 미션 객체 단위에서 다룬다.
/// </summary>
public sealed class ConfigInterfaceTests
{
    /// <summary>검사 전용 가짜 파일 묶음. 경로는 원본 표기 그대로 비교한다.</summary>
    private static Dictionary<string, byte[]> MakeFiles() => new(StringComparer.Ordinal)
    {
        ["d\\options.cfg"] = Encoded("InstallDir = \"old\"\r\nSCREENW = \"1024\"\r\ncurrentLanguage = \"german\"\r\n"),
        ["d\\setup.cfg"] = Encoded("\r\n// comment\r\nDataDir = \"\\D\"\r\nSCREENW = \"640\"\r\ncurrentLanguage=\"english\"\r\n"
            + "missionSpec = \"{DataDir}\\{local.1}.{currentLanguage}\"\r\n"
            + "LanguageSpec = \"{DataDir}\\config.{local.1}\"\r\n"
            + "title = \"{@tutorial1.name|none}\"\r\nmissing = \"{@nofile.name|none}\"\r\n"
            + "zero = \"0\"\r\nseven = \" 7x\"\r\n"),
        ["\\D\\TUTORIAL1.german"] = Encoding.Latin1.GetBytes("name = \"Erste Schritte\"\r\n"),
        ["\\D\\config.german"] = Encoding.Latin1.GetBytes("Sun = \"Sonne\"\r\nSCREENW = \"1\"\r\n"),
    };

    /// <summary>가짜 파일에서 읽는 공급자. 없는 파일은 null.</summary>
    private static Func<string, byte[]?> Reader(Dictionary<string, byte[]> files) =>
        path => files.TryGetValue(path, out byte[]? bytes) ? bytes : null;

    /// <summary>원본 평문(서명+본문)을 XOR 인코딩한 바이트로 만든다.</summary>
    private static byte[] Encoded(string text) =>
        ConfigInterface.EncodeConfigFile(ConfigTextOps.Signature + text);

    /// <summary>시작 순서: 버전·인자·user·options·dev·guild·setup 순으로 섹션을 쌓고 InstallDir·CDDir을 쓴다.</summary>
    [Fact]
    public void Startup_BuildsSectionsInOriginalOrder()
    {
        var config = new ConfigInterface(Reader(MakeFiles()));
        var options = new ConfigInterface.StartupOptions
        {
            CommandLine = "cheat=1;SCREENW=\"800\"",
            MajorVersion = 10,
            MinorVersion = 78,
            InstallDir = "C:\\NS",
        };
        config.Startup(options);
        string text = config.Configuration.Text;
        Assert.StartsWith("[ARGS]\r\ncheat=1\r\nSCREENW=\"800\"\r\nMAJOR_VERSION=10\r\nMINOR_VERSION=78\r\nversion=\"v10.78\"\r\n"
            + "[user.cfg]\r\n[options.cfg]\r\nmQdsTInstallDir = \"old\"\r\n", text);
        Assert.EndsWith("[END]\r\nInstallDir = \"C:\\NS\"\r\nCDDir = \"C:\\NS\"\r\n", text);
        Assert.Contains("[dev.cfg]\r\n[guild.cfg]\r\n[setup.cfg]\r\nmQdsT\r\n// comment", text);
        // 먼저 읽은 것이 우선한다: 명령줄 > options > setup.
        Assert.Equal(800, config.GetInt("SCREENW"));
        Assert.Equal("german", config.Configuration.Get("currentLanguage"));
        Assert.Equal("v10.78", config.Configuration.Get("version"));
        // 서명이 붙은 options.cfg 첫 줄과 죽은 줄 사이로 시작 순서가 같은 값이 조회된다.
        Assert.Equal("C:\\NS", config.Configuration.Get("InstallDir"));
        Assert.Equal("\\D\\TEST01.german", config.PathSpec("missionSpec", "TEST01"));
        Assert.Equal("\\D\\{LOCAL.1}.german", config.PathSpec("missionSpec"));
        Assert.Equal("a-x-\\D", config.SubstituteLocal("{local.1}-{local.2|x}-{DataDir}", "a"));
        // 임시 local 객체는 조회가 끝나면 목록에서 빠진다.
        Assert.Equal(4, config.Registry.Objects.Count);
    }

    /// <summary>수·문자 읽기: "0에서 시작해 값이 바뀌었는가" 판정. 0과 같아도 ReadInt가 거짓이면 거짓.</summary>
    [Fact]
    public void TypedAccessors_KeepOriginalQuirks()
    {
        var config = new ConfigInterface(Reader(MakeFiles()));
        config.Startup(new ConfigInterface.StartupOptions());
        int width = 640;
        Assert.True(config.ReadInt("SCREENW", ref width) && width == 1024);
        Assert.False(config.ReadInt("SCREENW", ref width));
        Assert.False(config.ReadInt("nokey", ref width) && width == 1024);
        string language = "english";
        Assert.True(config.ReadString("currentLanguage", ref language) && language == "german");
        Assert.False(config.ReadString("currentLanguage", ref language));
        Assert.True(config.IntEquals("seven", 7));
        Assert.False(config.IntEquals("seven", 8));
        Assert.False(config.IntEquals("zero", 0));
        Assert.Equal(0, config.GetInt("nokey"));
    }

    /// <summary>`{@미션.이름}`은 missionSpec 경로의 파일로 임시 객체를 읽는다. '.'이 없으면 빈 값이다.</summary>
    [Fact]
    public void MissionLookup_ReadsMissionScriptThroughSpec()
    {
        var config = new ConfigInterface(Reader(MakeFiles()));
        config.Startup(new ConfigInterface.StartupOptions());
        Assert.Equal("Erste Schritte", config.Configuration.Get("title"));
        Assert.Equal("none", config.Configuration.Get("missing"));
        Assert.Equal("{@TUTORIAL1}", config.Configuration.Substitute("{@tutorial1.nokey}"));
        Assert.Equal("", config.Configuration.Substitute("{@nodot}"));
        Assert.Equal(4, config.Registry.Objects.Count);
        // 용어표는 전역 설정보다 먼저 등록되어 조회에서 앞선다.
        config.LoadLanguage(config.Configuration.Get("currentLanguage")!);
        Assert.Equal("Sonne", config.Configuration.Get("Sun"));
        Assert.Equal(1024, config.GetInt("SCREENW"));
    }

    /// <summary>쓰기·저장 왕복: 같은 키 삭제 후 END에 추가하고, 저장 본문+END 본문을 XOR한다. 다시 읽으면 같은 값이 나온다.</summary>
    [Fact]
    public void SetAndSave_RoundTripsThroughOriginalFormat()
    {
        var files = MakeFiles();
        var config = new ConfigInterface(Reader(files));
        var options = new ConfigInterface.StartupOptions { InstallDir = "C:\\NS" };
        config.Startup(options);
        int changed = 0;
        config.OnChanged = () => { changed++; };
        Assert.True(config.Set("SCREENW", "800"));
        Assert.False(config.SetInt("newKey", -3));
        config.Set("currentLanguage", "français");
        Assert.Equal(3, changed);
        Assert.True(config.Configuration.Changed);
        Assert.Equal(800, config.GetInt("SCREENW"));
        string saved = config.SaveText("d\\options.cfg");
        Assert.False(config.Configuration.Changed);
        Assert.Equal("mQdsTInstallDir = \"old\"\r\nInstallDir = \"C:\\NS\"\r\nCDDir = \"C:\\NS\"\r\n"
            + "SCREENW = \"800\"\r\nnewKey = \"-3\"\r\ncurrentLanguage = \"français\"\r\n", saved);
        // 저장한 바이트는 XOR 감쇠 조건을 만족한다.
        byte[] bytes = ConfigInterface.EncodeConfigFile(saved);
        Assert.True(bytes.Length >= 3 && bytes[0] == 0 && bytes[2] == 0);
        files["d\\options.cfg"] = bytes;
        var reloaded = new ConfigInterface(Reader(files));
        reloaded.Startup(options);
        Assert.Equal(800, reloaded.GetInt("SCREENW"));
        Assert.Equal(-3, reloaded.GetInt("newKey"));
        Assert.Equal("français", reloaded.Configuration.Get("currentLanguage"));
        Assert.Contains("français", reloaded.Configuration.Text, StringComparison.Ordinal);
    }

    /// <summary>256개 원본 바이트를 모두 왕복해도 깨지지 않는다. 서명이 있으면 UTF-8 BOM처럼 앞에 둔다.</summary>
    [Fact]
    public void Encoding_PreservesEveryWindows1252Byte()
    {
        var original = new byte[256];
        // 0x00~0xFF 전 범위를 그대로 복원한다.
        for (int i = 0; i < 256; i++)
        {
            original[i] = (byte)i;
        }
        Assert.True(OriginalText.EncodeWindows1252(OriginalText.DecodeWindows1252(original)).AsSpan().SequenceEqual(original));
        byte[] encoded = ConfigInterface.EncodeConfigFile("mQdsTName = \"café €\"\r\n");
        byte[] decoded = (byte[])encoded.Clone();
        XorCipher.Apply(decoded);
        Assert.Equal("mQdsTName = \"café \u0080\"\r\n", Encoding.Latin1.GetString(decoded));
    }

    /// <summary>파일 쓰기 실패·인코딩 실패는 변경 표시를 보존하고 예외를 그대로 던진다.</summary>
    [Fact]
    public void SaveFile_PreservesChangesOnFailure()
    {
        var config = new ConfigInterface(Reader(MakeFiles()));
        config.Startup(new ConfigInterface.StartupOptions());
        config.SetInt("SCREENW", 800);
        bool failed = false;
        try
        {
            // 쓰기가 터지면 예외가 그대로 나간다.
            config.SaveFile("options.cfg", (name, bytes) => throw new InvalidOperationException("write failure"));
        }
        catch (InvalidOperationException)
        {
            failed = true;
        }
        Assert.True(failed && config.Configuration.Changed);
        byte[] saved = [];
        config.SaveFile("options.cfg", (name, bytes) =>
        {
            Assert.Equal("options.cfg", name);
            saved = bytes;
        });
        Assert.NotEmpty(saved);
        Assert.False(config.Configuration.Changed);
        config.Set("name", "한");
        failed = false;
        try
        {
            // 인코딩이 터지면 파일 쓰기 자체가 시작되지 않는다.
            config.SaveFile("options.cfg", (name, bytes) => throw new InvalidOperationException("must not run"));
        }
        catch (InvalidDataException)
        {
            failed = true;
        }
        Assert.True(failed && config.Configuration.Changed);
    }

    /// <summary>인자 처리: 첫 `@` 인자 전까지는 글, 그 뒤는 전부 파일이다. 빈 줄과 없는 파일도 섹션 줄은 남는다.</summary>
    [Fact]
    public void Load_SwitchesFromTextToFilesAtFirstAtSign()
    {
        var files = new Dictionary<string, byte[]>(StringComparer.Ordinal)
        {
            ["@extra"] = Encoding.Latin1.GetBytes("E = \"from file\"\r\n"),
            ["a.cfg"] = Encoding.Latin1.GetBytes("A = \"1\"\r\n"),
            ["x\\b.cfg"] = Encoding.Latin1.GetBytes("B = \"2\"\r\n"),
        };
        var config = new ConfigInterface(Reader(files));
        config.Load(new List<string> { "K=1;@extra L=2", "@a.cfg", "x\\b.cfg", "", "@missing.cfg" });
        Assert.Equal("[ARGS]\r\nE = \"from file\"\r\nK=1\r\n L=2\r\n[a.cfg]\r\nA = \"1\"\r\n"
            + "[b.cfg]\r\nB = \"2\"\r\n[missing.cfg]\r\n[END]\r\n", config.Configuration.Text);
        Assert.True(config.Configuration.EnvironmentEnabled);
        Assert.Equal("ARGS;a.cfg;b.cfg;missing.cfg;", config.Configuration.SectionNames(""));
        Assert.Equal("A = \"1\"\r\n", config.Configuration.Section("a.cfg"));
        Assert.Null(config.Configuration.Section("END"));
    }

    /// <summary>생성 순서대로 language·configuration·global·fort 4개가 등록된다. 이름 조회는 대소문자를 무시한다.</summary>
    [Fact]
    public void StaticOwnersMatchPatchCrtOrder()
    {
        // 파일·게임 없이도 같은 Registry에 실제 Config 생성 순서로 둔다.
        var config = new ConfigInterface(null);
        IReadOnlyList<Config> objects = config.Registry.Objects;
        Assert.Equal(4, objects.Count);
        Config? global = config.Registry.FindByName("global");
        Config? fort = config.Registry.FindByName("fort");
        Assert.NotNull(global);
        Assert.NotNull(fort);
        Assert.Same(objects[0], config.Language);
        Assert.Same(objects[1], config.Configuration);
        Assert.Same(objects[2], global);
        Assert.Same(objects[3], fort);
        Assert.NotSame(global, fort);
        Assert.Equal("", objects[0].Prefix);
        Assert.Equal("", objects[1].Prefix);
        Assert.Equal("global.", objects[2].Prefix);
        Assert.Equal("fort.", objects[3].Prefix);
        Assert.Same(fort, config.Registry.FindByName("FORT"));
        Assert.Same(global, config.Registry.FindByName("GLOBAL"));
    }

    /// <summary>한정 쓰기(`fort.`·`global.` 접두): 맞는 객체에만 쓰고 조회는 전역·언어·루트와 분리된다.</summary>
    [Fact]
    public void QualifiedSetKeepsGlobalFortAndRootSeparate()
    {
        var config = new ConfigInterface(null);
        config.Configuration.Assign("mySubHandle = \"Root\"\r\n");
        config.Language.Assign("mySubHandle = \"Language\"\r\n");
        Config? global = config.Registry.FindByName("global");
        Config? fort = config.Registry.FindByName("fort");
        Assert.NotNull(global);
        Assert.NotNull(fort);
        int notifications = 0;
        // 실제 ConfigInterface.Set의 변경 알림만 센다.
        config.OnChanged = () => { notifications++; };
        Assert.False(config.Set("fort.mySubHandle", "Player"));
        Assert.False(config.Set("global.mySubHandle", "Global"));
        Assert.Equal("Player", config.Configuration.Get("fort.mySubHandle"));
        Assert.Equal("Global", config.Configuration.Get("global.mySubHandle"));
        Assert.Equal("Player", fort!.GetRaw("mySubHandle"));
        Assert.Equal("Global", global!.GetRaw("mySubHandle"));
        Assert.Equal("Root", config.Configuration.GetRaw("mySubHandle"));
        Assert.Equal("Language", config.Language.GetRaw("mySubHandle"));
        Assert.Null(config.Configuration.GetRaw("fort.mySubHandle"));
        Assert.Null(config.Configuration.GetRaw("global.mySubHandle"));
        Assert.Null(fort.GetRaw("fort.mySubHandle"));
        Assert.Null(fort.GetRaw("global.mySubHandle"));
        Assert.True(config.Set("fort.mySubHandle", "Player2"));
        Assert.Equal("Player2", config.Configuration.Get("fort.mySubHandle"));
        Assert.True(config.Set("mySubHandle", "Root2"));
        Assert.Equal("Root2", config.Configuration.Get("mySubHandle"));
        Assert.Equal("Player2", fort.GetRaw("mySubHandle"));
        Assert.Equal("Global", global.GetRaw("mySubHandle"));
        Assert.Equal(4, notifications);
    }

    /// <summary>임시 `local` 객체는 조회에 끼었다가 버려지면 목록에서 빠진다. 무접두어 조회는 기존 순서를 따른다.</summary>
    [Fact]
    public void StaticOwnersPreserveLegacyRootsAndTemporaryScope()
    {
        var config = new ConfigInterface(null);
        config.Language.Assign("shared = \"Language\"\r\nlangOnly = \"Translation\"\r\n");
        config.Configuration.Assign("shared = \"Root\"\r\nrootOnly = \"RootValue\"\r\n");
        Assert.Equal("Root", config.Configuration.Get("shared"));
        Assert.Equal("Translation", config.Configuration.Get("langOnly"));
        Assert.True(config.Set("shared", "ChangedRoot"));
        Assert.Equal("ChangedRoot", config.Configuration.GetRaw("shared"));
        Assert.Equal("Language", config.Language.GetRaw("shared"));
        Assert.False(config.Set("unknown.key", "FallbackRoot"));
        Assert.Equal("FallbackRoot", config.Configuration.GetRaw("key"));
        using (var local = new Config(config.Registry, "local"))
        {
            // 같은 실제 Config 생성·등록·파괴를 제자리에서 쓴다.
            Assert.Equal(5, config.Registry.Objects.Count);
            Assert.Same(local, config.Registry.FindByName("local"));
            Assert.False(config.Set("local.key", "Temporary"));
            Assert.Equal("Temporary", local.GetRaw("key"));
            Assert.Equal("Temporary", config.Configuration.Get("local.key"));
            Assert.Equal("FallbackRoot", config.Configuration.Get("key"));
        }
        Assert.Equal(4, config.Registry.Objects.Count);
        Assert.Null(config.Registry.FindByName("local"));
        Assert.Null(config.Configuration.Get("local.key"));
        Assert.Equal("FallbackRoot", config.Configuration.Get("key"));
        Assert.Equal("", config.Registry.FindByName("fort")!.Text);
        Assert.Equal("", config.Registry.FindByName("global")!.Text);
    }
}
