using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Assets.Tests;

/// <summary>
/// 가상 파일 시스템, 설정 조회·치환, 번역표, 미션 스크립트 로더 검사
/// </summary>
public sealed class TextResourceTests
{
    // ───────────────────────── 텍스트 인코딩 ─────────────────────────

    /// <summary>Windows-1252 특수 구간(0x80~0x9F)과 Latin-1 구간을 올바른 유니코드로 바꾼다</summary>
    [Fact]
    public void OriginalText_DecodesWindows1252()
    {
        byte[] data = [0x93, (byte)'H', (byte)'i', 0x94, (byte)' ', 0xFC, 0x80];
        Assert.Equal("\u201CHi\u201D \u00FC\u20AC", OriginalText.Decode(data));
    }

    /// <summary>UTF-8(BOM 유무 모두) 한국어 파일은 UTF-8 로 읽는다</summary>
    [Fact]
    public void OriginalText_DecodesUtf8Korean()
    {
        byte[] plain = Encoding.UTF8.GetBytes("신전 = \"Temple\"");
        byte[] withBom = [0xEF, 0xBB, 0xBF, .. plain];
        Assert.Equal("신전 = \"Temple\"", OriginalText.Decode(plain));
        Assert.Equal("신전 = \"Temple\"", OriginalText.Decode(withBom));
    }

    // ───────────────────────── 설정 조회 ─────────────────────────

    /// <summary>키는 대소문자 무시·공백 포함 가능, 같은 키는 처음 것이 이기고 주석 줄은 무시한다</summary>
    [Fact]
    public void ConfigText_FirstMatchWins()
    {
        var text = new ConfigText(
            "// fortPal = \"old\"\r\n" +
            "\tQuick Help Spec = \"{DataDir}\\{Quick Help Filename}\"\r\n" +
            "fortPal = \"gifcloud\" // 주석\r\n" +
            "fortPal = \"second\"\r\n" +
            "maxFPS = 75\r\n" +
            "fortPalX = \"no\"\r\n");
        Assert.Equal("gifcloud ", text.GetRaw("FORTPAL"));
        Assert.Equal(@"{DataDir}\{Quick Help Filename}", text.GetRaw("quick help spec"));
        Assert.Equal("75", text.GetRaw("maxFPS"));
        Assert.Null(text.GetRaw("fort"));
        Assert.Equal(["Quick Help Spec", "fortPal", "maxFPS", "fortPalX"], text.Keys());
    }

    /// <summary>백틱이 붙은 따옴표는 값 안에 남고, 따옴표 안의 // 는 주석이 아니다</summary>
    [Fact]
    public void ConfigText_EscapedQuotesAndSlashes()
    {
        var text = new ConfigText("WindowName = \"Game `\"{local.1}`\" http://x\"\n");
        Assert.Equal("Game `\"{local.1}`\" http://x", text.GetRaw("WindowName"));
    }

    /// <summary>ConfigFile.Set 은 조회에 쓰이는 첫 줄을 바꾸고, 없는 키는 끝에 추가한다</summary>
    [Fact]
    public void ConfigFile_SetReplacesFirstLine()
    {
        var config = new ConfigFile("a=1\r\na = \"2\"\r\nb = \"x\"");
        config.Set("A", "9");
        config.Set("c", "new");
        Assert.Equal("9", config.Get("a"));
        Assert.Equal("new", config.Get("c"));
        Assert.Equal("a = \"9\"\r\na = \"2\"\r\nb = \"x\"\r\nc = \"new\"\r\n", config.Text);
    }

    // ───────────────────────── 치환 ─────────────────────────

    /// <summary>{키}, {키|기본값}, 중첩, 이스케이프, 찾지 못한 키 표시</summary>
    [Fact]
    public void ConfigStore_ExpandsLikeOriginal()
    {
        var store = new ConfigStore();
        store.Push(new ConfigText(
            "DataDir = \"\\D\"\n" +
            "vortex = \"Temple\"\n" +
            "a factory = \"a Workshop\"\n" +
            "currentLanguage = \"english\"\n" +
            "missionSpec = \"{DataDir}\\{local.1}.{currentLanguage}\"\n" +
            "OkButton = \"$Button=OK,DoNothing,0\"\n" +
            "loop = \"{loop}\"\n" +
            "which = \"vortex\"\n"));

        Assert.Equal("The Temple", store.Expand("The {vortex}"));
        Assert.Equal("Build a Workshop.", store.Expand("Build {a factory}."));
        Assert.Equal("Owner: Not Registered", store.Expand("Owner: {fort.mySubHandle|Not Registered}"));
        Assert.Equal("x {Not Found:NOSUCHKEY} y", store.Expand("x {NoSuchKey} y"));
        Assert.Equal("Temple", store.Expand("{{which}}"));
        Assert.Equal("say \"hi\"\n{literal}", store.Expand("say `\"hi`\"`n`{literal`}"));
        Assert.Equal("trail", store.Expand("trail  \t"));
        Assert.Equal(@"\D\tutorial1.english", store.ExpandSpec("missionSpec", "tutorial1"));
        Assert.Equal("$Button=OK,DoNothing,0", store.Get("okbutton"));
        Assert.Null(store.Get("absent"));
        // 자기 참조는 무한 반복하지 않고 멈춘다
        Assert.Contains("Not Found", store.Get("loop"));
    }

    /// <summary>위에 쌓은 층이 먼저 조회되고, 이름 있는 층은 접두어가 맞는 키만 받는다</summary>
    [Fact]
    public void ConfigStore_LayersAndScopes()
    {
        var store = new ConfigStore();
        store.Push(new ConfigText("title = \"base\"\nname = \"base name\"\n"));
        store.Push(new ConfigText("title = \"top\"\n"));
        store.Push(ConfigStore.FromPairs([new("fileName", "Tutorial1"), new("title", "scoped")]), "mission");
        Assert.Equal("top", store.Get("title"));
        Assert.Equal("base name", store.Get("name"));
        Assert.Equal("scoped", store.Get("mission.title"));
        Assert.Equal("Done Tutorial1", store.Expand("Done {mission.fileName}"));
        store.Pop();
        Assert.Equal("{Not Found:MISSION.FILENAME}", store.Expand("{mission.fileName}"));
    }

    /// <summary>{@미션.키} 는 미션 공급자에서 머리 값을 읽는다</summary>
    [Fact]
    public void ConfigStore_MissionReference()
    {
        var store = new ConfigStore
        {
            MissionLoader = name => name.Equals("tutorial1", StringComparison.OrdinalIgnoreCase)
                ? new MissionScript("[Header]\ntitle = \"Bridge the Gap!\"\n").Header
                : null,
        };
        Assert.Equal("Bridge the Gap!", store.Expand("{@Tutorial1.title}"));
        Assert.Equal("?", store.Expand("{@nothing.title|?}"));
    }

    // ───────────────────────── 번역표 ─────────────────────────

    /// <summary>원본 xlat 형식: 여러 줄 원문·번역문, 구분선·주석 무시, 대소문자 구분 완전 일치</summary>
    [Fact]
    public void XlatTable_ParsesBlocks()
    {
        const string text =
            "oCompXlat.pl output: 2 entries\r\n" +
            "+++++++++++ NEW TRANSLATIONS +++++++++++++\r\n" +
            "// comment\r\n" +
            "***\r\nCancel\r\n---\r\nAbbrechen\r\n===\r\n" +
            "***\r\nLine one\r\nLine %s\r\n---\r\nZeile eins\r\nZeile %s\r\n===\r\n" +
            "***\r\nCancel\r\n---\r\nAbbruch\r\n===\r\n" +
            "***\r\nBroken\r\n---\r\nno end";
        XlatTable table = XlatTable.Parse(text);
        Assert.Equal(2, table.Count);
        Assert.Equal("Abbruch", table.Translate("Cancel"));
        Assert.Equal("Zeile eins\nZeile %s", table.Translate("Line one\nLine %s"));
        Assert.Equal("cancel", table.Translate("cancel"));
        Assert.False(table.TryTranslate("Broken", out _));
    }

    /// <summary>언어 이름 정규화·운영체제 언어 대응</summary>
    [Fact]
    public void GameLanguage_Normalize()
    {
        Assert.Equal("german", GameLanguage.Normalize("German"));
        Assert.Equal("english", GameLanguage.Normalize("klingon"));
        Assert.Equal("korean", GameLanguage.FromCulture(new System.Globalization.CultureInfo("ko-KR")));
        Assert.Equal("english", GameLanguage.FromCulture(System.Globalization.CultureInfo.InvariantCulture));
    }

    // ───────────────────────── 미션 스크립트 ─────────────────────────

    /// <summary>섹션 찾기: 복수 이름 머리, 공백 뒤 두 번째 이름, 들여쓴 머리, 0열 '[' 에서 본문 종료</summary>
    [Fact]
    public void MissionScript_Sections()
    {
        var script = new MissionScript(
            "[Header]\r\n" +
            "missionType = \"Tutorial\"\r\n" +
            "ai2Name = \"Juggler of Thunder\"\r\n" +
            "\r\n" +
            "[A.]\r\n" +
            "<h2>Welcome</h2>\r\n" +
            "$Button=OK,Tell,@1\r\n" +
            "[Succeeded] [BadTeamDead]\r\n" +
            "You won!\r\n" +
            "  [indented]\r\n" +
            "still in succeeded\r\n" +
            "[END]\r\n");
        Assert.Equal("Tutorial", script.GetHeader("missiontype"));
        Assert.Equal("Juggler of Thunder", script.GetHeader("ai2Name"));
        Assert.Equal("<h2>Welcome</h2>\n$Button=OK,Tell,@1", script.GetSection("a."));
        Assert.Equal("You won!\n  [indented]\nstill in succeeded", script.GetSection("BadTeamDead"));
        Assert.Equal("still in succeeded", script.GetSection("indented"));
        Assert.Equal("", script.GetSection("END"));
        Assert.Null(script.GetSection("Failed"));
        Assert.Equal(
            ["Header", "A.", "Succeeded", "BadTeamDead", "indented", "END"],
            script.Sections.SelectMany(s => s.Names));
    }

    /// <summary>$명령 어휘 분석: 한 줄에 여러 명령, 조건 태그 뒤 명령</summary>
    [Fact]
    public void MissionScript_FindCommands()
    {
        IReadOnlyList<MissionCommand> commands = MissionScript.FindCommands(
            "Text $Button=Yes,Tell,a $Button=No,DoNothing,0\n" +
            "<?{outpost}>$Checked=5 The Execution,MissionBegin,execute,{Doneexecute},1</?>\n" +
            "$Timeout=20,DoNothing,0");
        Assert.Equal(4, commands.Count);
        Assert.Equal(("Button", "Yes,Tell,a"), (commands[0].Name, commands[0].Arguments));
        Assert.Equal(["No", "DoNothing", "0"], commands[1].Fields);
        Assert.Equal("Checked", commands[2].Name);
        Assert.Equal("5 The Execution", commands[2].Fields[0]);
        Assert.Equal(2, commands[3].Line);
    }

    // ───────────────────────── 가상 파일 시스템 ─────────────────────────

    /// <summary>원본 순서: 데이터 폴더 디스크 → 아카이브 → 보조 폴더. 대소문자·구분자·".\" 접두어 무시, 와일드카드 검색</summary>
    [Fact]
    public void GameFileSystem_PriorityAndFind()
    {
        string root = Path.Combine(Path.GetTempPath(), "ns-vfs-" + Guid.NewGuid().ToString("N"));
        string secondary = Path.Combine(root, "cd");
        try
        {
            Directory.CreateDirectory(Path.Combine(root, "D"));
            Directory.CreateDirectory(Path.Combine(secondary, "d"));
            File.WriteAllText(Path.Combine(root, "D", "Both.english"), "disk");
            File.WriteAllText(Path.Combine(root, "D", "offical9.english"), "disk only");
            File.WriteAllText(Path.Combine(secondary, "d", "both.english"), "secondary");
            File.WriteAllText(Path.Combine(secondary, "d", "cdonly.english"), "secondary only");
            File.WriteAllBytes(Path.Combine(root, "netstorm.tarc"), BuildArchive(
                (@"\d\both.english", "archive"),
                (@"\d\offical1.english", "archive only"),
                (@"\d\cdonly.english", "archive wins over cd")));

            GameFileSystem fs = GameFileSystem.Open(root, secondary);
            Assert.Single(fs.Archives);
            Assert.Equal("disk", Text(fs, @".\D\BOTH.english"));
            Assert.Equal(GameFileSource.Disk, fs.Locate("d/both.english"));
            Assert.Equal("archive only", Text(fs, @"\d\offical1.english"));
            Assert.Equal(GameFileSource.Archive, fs.Locate("d/offical1.english"));
            Assert.Equal("archive wins over cd", Text(fs, "d/cdonly.english"));
            Assert.Equal(GameFileSource.None, fs.Locate("d/missing.english"));
            Assert.Null(fs.TryReadAllBytes("d/missing.english"));
            Assert.Throws<FileNotFoundException>(() => fs.ReadAllBytes("d/missing.english"));
            Assert.Equal(["d/offical1.english", "d/offical9.english"], fs.Find(@"\D\offical*.english"));
            Assert.Equal("d/both.english", GameLanguage.ResolveFile(fs, "d/both", "korean"));
            Assert.Null(GameLanguage.ResolveFile(fs, "d/none", "korean"));
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    /// <summary>원본: 공식 미션은 아카이브, 팬 미션은 느슨한 d/ 폴더에서 읽힌다</summary>
    [Fact]
    public void Original_FileSystemFindsBothSources()
    {
        GameFileSystem fs = GameFileSystem.Open(OriginalData.RequireDirectory());
        Assert.Equal(GameFileSource.Archive, fs.Locate(@"\D\tutorial1.english"));
        Assert.Equal(GameFileSource.Disk, fs.Locate("d/GIFCLOUD.COL"));
        Assert.Contains("d/offical1.english", fs.Find("d/offical*.english"));
    }

    // ───────────────────────── 원본 전수 검사 ─────────────────────────

    /// <summary>원본 번역표: 블록 수(중복 포함)와 고유 원문 수, 기본 문구 번역을 확인한다</summary>
    [Theory]
    [InlineData("german", 871, 774)]
    [InlineData("french", 746, 664)]
    [InlineData("spanish", 746, 664)]
    [InlineData("portuguese", 746, 664)]
    public void Original_XlatTables(string language, int expectedBlocks, int expectedUnique)
    {
        GameFileSystem fs = GameFileSystem.Open(OriginalData.RequireDirectory());
        XlatTable table = XlatTable.FromFileBytes(fs.ReadAllBytes($"d/xlat.{language}"));
        Assert.Equal(expectedBlocks, table.BlockCount);
        Assert.Equal(expectedUnique, table.Count);
        Assert.NotEqual("Cancel", table.Translate("Cancel"));
    }

    /// <summary>원본 설정 체계: setup.cfg + config.english 로 용어 치환과 missionSpec 경로를 만든다</summary>
    [Fact]
    public void Original_ConfigExpansion()
    {
        GameFileSystem fs = GameFileSystem.Open(OriginalData.RequireDirectory());
        var store = new ConfigStore();
        store.Push(ConfigText.FromFileBytes(fs.ReadAllBytes("d/setup.cfg")));
        store.Push(ConfigText.FromFileBytes(fs.ReadAllBytes("d/options.cfg")));
        string language = GameLanguage.Normalize(store.Get("currentLanguage"));
        store.Push(ConfigText.FromFileBytes(fs.ReadAllBytes(store.ExpandSpec("languageSpec", language))));

        Assert.Equal("Temple", store.Expand("{vortex}"));
        Assert.Equal("High Priest", store.Expand("{priest}"));
        Assert.Equal("gifcloud", store.Get("fortPal"));
        string missionPath = store.ExpandSpec("missionSpec", "tutorial1");
        Assert.Equal(@"\D\tutorial1." + language, missionPath);
        Assert.True(fs.Exists(missionPath));
        Assert.Equal("NetStorm Mission \"Bridge\"", store.ExpandSpec("MissionWindowName", "Bridge"));
    }

    /// <summary>원본의 모든 미션·메뉴 스크립트를 읽어 섹션을 찾고, 공식 튜토리얼 머리 값을 확인한다</summary>
    [Fact]
    public void Original_AllMissionScripts()
    {
        GameFileSystem fs = GameFileSystem.Open(OriginalData.RequireDirectory());
        var files = GameLanguage.OriginalLanguages
            .SelectMany(lang => fs.Find($"d/*.{lang}"))
            .Where(f => !Path.GetFileName(f).StartsWith("xlat.", StringComparison.Ordinal)
                        && !Path.GetFileName(f).StartsWith("config.", StringComparison.Ordinal))
            .ToList();
        Assert.True(files.Count > 600, $"미션 파일 수: {files.Count}");
        int withSections = 0;
        // 모든 파일이 예외 없이 해석되고 대부분 섹션을 가진다
        foreach (string file in files)
        {
            MissionScript script = MissionScript.FromFileBytes(fs.ReadAllBytes(file));
            if (script.Sections.Count > 0)
            {
                withSections++;
            }
        }
        Assert.True(withSections > files.Count * 9 / 10, $"섹션이 있는 파일: {withSections}/{files.Count}");

        MissionScript tutorial = MissionScript.FromFileBytes(fs.ReadAllBytes("d/tutorial1.english"));
        Assert.Equal("Tutorial", tutorial.GetHeader("missionType"));
        Assert.Equal("0", tutorial.GetHeader("moreGeysers"));
        Assert.NotNull(tutorial.GetSection("A."));
        Assert.Contains("NoBridgeYet", tutorial.Sections.SelectMany(sec => sec.Names));

        MissionScript menu = MissionScript.FromFileBytes(fs.ReadAllBytes("d/offical1.english"));
        IEnumerable<MissionCommand> commands = menu.Sections.SelectMany(s => MissionScript.FindCommands(s.Body));
        Assert.Contains(commands, c => c.Name == "Checked" && c.Fields.Count >= 3
                                       && c.Fields[1] == "MissionBegin" && c.Fields[2].Equals("Tutorial1", StringComparison.OrdinalIgnoreCase));
    }

    /// <summary>파일 시스템에서 텍스트를 읽는 도우미</summary>
    /// <param name="fs">파일 시스템</param>
    /// <param name="path">경로</param>
    private static string Text(GameFileSystem fs, string path) => fs.TryReadAllText(path) ?? "(없음)";

    /// <summary>
    /// 검사용 TAFF 아카이브를 만든다 (docs/formats/taff.md 의 헤더·디렉터리 배치, 데이터는 XOR 인코딩)
    /// </summary>
    /// <param name="files">(원본 경로, 내용) 목록</param>
    private static byte[] BuildArchive(params (string Name, string Content)[] files)
    {
        const int headerSize = 0x30;
        var directory = new MemoryStream();
        var data = new MemoryStream();
        // 파일마다 디렉터리 레코드와 인코딩된 데이터를 쓴다
        foreach ((string name, string content) in files)
        {
            byte[] body = Encoding.ASCII.GetBytes(content);
            XorCipher.Apply(body);
            Span<byte> fixedPart = stackalloc byte[8];
            BinaryPrimitives.WriteInt32LittleEndian(fixedPart, (int)data.Length);
            BinaryPrimitives.WriteInt32LittleEndian(fixedPart[4..], body.Length);
            directory.Write(fixedPart);
            directory.Write(Encoding.ASCII.GetBytes(name + "\0"));
            data.Write(body);
        }
        var archive = new byte[headerSize + directory.Length + data.Length];
        Encoding.ASCII.GetBytes("TAFF v0.2\x1A").CopyTo(archive, 0);
        BinaryPrimitives.WriteInt32LittleEndian(archive.AsSpan(0x14), files.Length);
        BinaryPrimitives.WriteInt32LittleEndian(archive.AsSpan(0x20), headerSize);
        BinaryPrimitives.WriteInt32LittleEndian(archive.AsSpan(0x28), headerSize + (int)directory.Length);
        directory.ToArray().CopyTo(archive, headerSize);
        data.ToArray().CopyTo(archive, headerSize + (int)directory.Length);
        return archive;
    }
}
