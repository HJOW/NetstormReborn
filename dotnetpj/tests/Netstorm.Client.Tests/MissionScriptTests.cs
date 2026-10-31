using System.Collections.Generic;
using System.Text;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 미션 개체(스크립트·fortSpec·missionType·tellAtList)를 실제 Config와 함께 검사한다.
/// 근거: cpppj/tests/ConfigInterfaceTests.cpp 의 MissionScript_ResolvesScriptFortAndType.
/// Fort reader의 경로·소유 수명도 검사한다. Guide 생성은 미션 실행 조립 때 잇는다. (2026-10-31)
/// </summary>
public sealed class MissionScriptTests
{
    /// <summary>Latin1 바이트로 고정된 가짜 파일 묶음. 경로는 원본 표기 그대로 비교한다.</summary>
    private static Dictionary<string, byte[]> MakeFiles() => new(System.StringComparer.Ordinal)
    {
        ["d\\setup.cfg"] = Latin1("DataDir = \"\\D\"\r\ncurrentLanguage=\"english\"\r\n"
            + "missionSpec = \"{DataDir}\\{local.1}.{currentLanguage}\"\r\nfortSpec = \"{DataDir}\\{local.1}.fort\"\r\n"
            + "banner = \"{mission.title|none}\"\r\n"),
        ["\\D\\first.english"] = Latin1("[Header]\r\nmissionType=\"Tutorial\"\r\ntitle=\"First {DataDir}\"\r\nmyStartMoney=3000 // comment\r\n[@10]\r\nFirst\r\n[@25]\r\nSecond\r\n"),
        ["\\D\\second.english"] = Latin1("missionType=\"Battle\"\r\nloadFort=\"shared\"\r\n"),
    };

    /// <summary>가짜 파일에서 읽는 공급자. 없는 경로는 null.</summary>
    private static Func<string, byte[]?> Reader(Dictionary<string, byte[]> files) =>
        path => files.TryGetValue(path, out byte[]? bytes) ? bytes : null;

    /// <summary>원본 바이트 그대로 두는 Latin1 인코딩.</summary>
    private static byte[] Latin1(string text) => Encoding.Latin1.GetBytes(text);

    /// <summary>미션 개체·추가 설정·loadFort·missionType·fortSpec 경로가 원본과 같이 동작한다.</summary>
    [Fact]
    public void MissionScript_ResolvesScriptFortAndType()
    {
        var files = MakeFiles();
        var config = new ConfigInterface(Reader(files));
        config.Startup(new ConfigInterface.StartupOptions());
        Assert.Equal("none", config.Configuration.Get("banner"));
        using (var first = new MissionScript(config, "first"))
        {
            Assert.True(first.Loaded);
            Assert.Equal("\\D\\first.english", first.ScriptPath);
            Assert.Equal("Tutorial", first.MissionType());
            Assert.Equal("first", first.FortName());
            Assert.Equal("\\D\\first.fort", first.FortPath());
            Assert.Equal("First \\D", first.Get("title"));
            Assert.Null(first.Get("nokey"));
            // 미션 객체가 살아 있는 동안 전역 설정에서 `{mission.??}`가 스크립트 값을 읽는다.
            Assert.Equal("First \\D", config.Configuration.Get("banner"));
            Assert.Equal(5, config.Registry.Objects.Count);
            Assert.Equal("@10;@25;", first.SectionNames("@"));
            Assert.False(first.Set("tellAtList", "@10;@25;"));
            Assert.Equal("@10;@25;", first.Get("tellAtList"));
            Assert.Equal("@10;@25;", config.PathSpec("mission.tellAtList"));
            // 현재 미션 범위가 전역 설정에 직접 키를 만들지는 않는다.
            Assert.Null(config.Configuration.GetRaw("tellAtList"));
            Assert.True(first.Set("title", "Changed {DataDir}"));
            Assert.Equal("Changed \\D", config.Configuration.Get("banner"));
        }
        Assert.Equal(4, config.Registry.Objects.Count);
        Assert.Equal("none", config.Configuration.Get("banner"));
        // loadFort가 없으면 extra가 먼저 들어가고 추가 설정 글이 스크립트보다 먼저 읽힌다.
        using (var second = new MissionScript(config, "second", "missionType=\"Override\";extra=1"))
        {
            Assert.Equal("shared", second.FortName());
            Assert.Equal("\\D\\shared.fort", second.FortPath());
            Assert.Equal("Override", second.MissionType());
            Assert.Equal("1", second.Get("extra"));
        }
        // 스크립트가 없으면 미션 이름이 새 파일 이름이 된다.
        using (var missing = new MissionScript(config, "nofile"))
        {
            Assert.False(missing.Loaded);
            Assert.Equal("", missing.MissionType());
            Assert.Equal("\\D\\nofile.fort", missing.FortPath());
        }
    }
    /// <summary>선택된 파일 경계와 현재 경로 설정이 미션 반납 뒤에도 유지되고 부재를 다른 reader로 우회하지 않는다.</summary>
    [Fact]
    public void FortReaders_PreserveSourceAndResolveCurrentPathAfterDisposal()
    {
        var files = MakeFiles();
        files["\\D\\shared.fort"] = [1, 2];
        var config = new ConfigInterface(Reader(files));
        config.Startup(new ConfigInterface.StartupOptions());
        var selected = new Dictionary<string, byte[]>(StringComparer.Ordinal)
        {
            ["\\D\\shared.fort"] = [3, 4],
            ["\\Other\\shared.fort"] = [5, 6],
        };
        Func<string, byte[]?> source = Reader(selected);
        Func<string, byte[]> reader;
        MissionFortFileReader fileReader;
        using (var mission = new MissionScript(config, "second", source: source))
        {
            Assert.Same(source, mission.FileSource());
            Assert.Equal(new byte[] { 3, 4 }, mission.ReadFortImage("shared"));
            reader = mission.FortReader();
            fileReader = mission.FortFileReader(path => "mapped:" + path);
        }
        config.Set("DataDir", "\\Other");
        Assert.Equal(new byte[] { 5, 6 }, reader("shared"));
        MissionFortSourceImage result = fileReader("shared");
        Assert.Equal(new byte[] { 5, 6 }, result.Image);
        Assert.Equal("mapped:\\Other\\shared.fort", result.FileName);
        config.Set("DataDir", "\\D");
        selected.Remove("\\D\\shared.fort");
        Assert.Throws<InvalidOperationException>(() => reader("shared"));
        using var defaultMission = new MissionScript(config, "second");
        Assert.Null(defaultMission.FileSource());
        Assert.Equal(new byte[] { 1, 2 }, defaultMission.ReadFortImage("shared"));
    }

}
