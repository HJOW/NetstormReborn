using System.Collections.Generic;
using System.Text;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 미션 개체(스크립트·fortSpec·missionType·tellAtList)를 실제 Config와 함께 검사한다.
/// 근거: cpppj/tests/ConfigInterfaceTests.cpp 의 MissionScript_ResolvesScriptFortAndType.
/// 대화창·미션 실행이 쓰는 Fort 이미지 읽기·Guide 생성은 미션 실행 조립 때 잇는다. (R3k-12, 2026-10-31)
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
}
