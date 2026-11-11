using System.Text;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 표시 제목·브리핑만 현재 언어를 읽고 실행 키는 기존 대본을 유지하는 분리를 검사한다
/// (cpppj <c>client/Mission.cpp</c>의 HEAD 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.349에서 K1 표시 분리 재대조와 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class MissionScriptDisplayTests
{
    /// <summary>제목·종류·브리핑이 다른 대본 바이트를 만든다.</summary>
    /// <param name="title">표시 제목이다.</param><param name="type">실행 종류다.</param><param name="body">브리핑 본문이다.</param>
    /// <returns>대본 바이트다.</returns>
    private static byte[] Script(string title, string type, string body) =>
        Encoding.Latin1.GetBytes("title=" + title + "\nmissionType=" + type + "\n[Briefing]\n" + body + "\n[End]\n");

    /// <summary>두 언어 경로를 읽는 설정 경계를 만든다.</summary>
    /// <returns>scriptA를 가리키는 설정이다.</returns>
    private static ConfigInterface Config()
    {
        var config = new ConfigInterface(path =>
            path == "scriptA" ? Script("TitleA", "TypeA", "BodyA.") :
            path == "scriptB" ? Script("TitleB", "TypeB", "BodyB.") : null);
        config.Configuration.Assign("missionSpec=scriptA\n");
        return config;
    }

    /// <summary>표시 제목·브리핑만 현재 언어를 읽고 실행 종류는 기존 대본을 유지한다.</summary>
    [Fact]
    public void TitleAndBriefingReadCurrentLanguageWhileExecutionKeysStay()
    {
        using var mission = new MissionScript(Config(), "M");
        Assert.Equal("TitleA", mission.Get("title"));
        Assert.Equal("TypeA", mission.Get("missionType"));
        Assert.Equal("BodyA.\n", mission.Section("Briefing"));
    }

    /// <summary>언어 경로가 바뀌면 제목·브리핑만 새 파일을 읽는다.</summary>
    [Fact]
    public void ChangedLanguagePathReadsDisplayOnly()
    {
        ConfigInterface config = Config();
        using var mission = new MissionScript(config, "M");
        config.Configuration.Assign("missionSpec=scriptB\n");
        // 표시만 현재 언어를 읽는다
        Assert.Equal("TitleB", mission.Get("title"));
        Assert.Equal("BodyB.\n", mission.Section("Briefing"));
        // 인공지능·SP 등의 실행 키는 기존 객체를 유지한다
        Assert.Equal("TypeA", mission.Get("missionType"));
    }

    /// <summary>새 경로에 대본이 없으면 기존 대본으로 돌아간다.</summary>
    [Fact]
    public void MissingDisplayFallsBackToCachedScript()
    {
        ConfigInterface config = Config();
        using var mission = new MissionScript(config, "M");
        config.Configuration.Assign("missionSpec=scriptC\n");
        Assert.Equal("TitleA", mission.Get("title"));
        Assert.Equal("BodyA.\n", mission.Section("Briefing"));
    }
}
