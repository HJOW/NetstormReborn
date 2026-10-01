namespace Netstorm.Assets.Tests;

/// <summary>도움말의 실제 원문·내부 링크·그림·방문 기록을 검증한다.</summary>
public sealed class HelpDocumentTests
{
    /// <summary>앵커 별칭과 중복 정의에서 첫 본문·info 유무를 함께 유지한다.</summary>
    [Fact]
    public void AliasesAndDuplicates_PreserveFirstDefinition()
    {
        var topics = new HelpTopics("<a name=\"one\">\n<a name=\"alias\">\n<info>\nFirst\n</a>\n<a name=\"one\">\nLast\n</a>");
        Assert.Equal("First", topics.Find("one"));
        Assert.Equal(topics.Find("one"), topics.Find("alias"));
        Assert.True(topics.HasInformation("ONE"));
        Assert.True(topics.HasInformation("alias"));
    }

    /// <summary>내부·외부 링크를 보존하고 그림·제어 문자·미션 조건문을 본문으로 노출하지 않는다.</summary>
    [Fact]
    public void Parse_PreservesLinksPicturesAndMissionCondition()
    {
        // 색·틸드 이스케이프·앵커·그림·미션 조건문이 섞인 원본 형식의 예시.
        const string html = "<h2>Title</h2><p>~lblue~. ~~ <a href=\"#next\">Next</a><br><!\"priest.*\"><?{global.inMission}>F8</?><a href=\"https://example.com\">Site</a>";
        IReadOnlyList<HelpTextRun> runs = HelpDocument.Parse(html);
        Assert.Contains(runs, r => r.Text == "Title" && r.Style == TutorialTextStyle.Heading);
        Assert.Contains(runs, r => r.Text.Contains("blue ~"));
        Assert.Contains(runs, r => r.Link == "#next" && r.Text == "Next");
        Assert.Contains(runs, r => r.Picture == "priest.*");
        Assert.Contains(runs, r => r.Link == "https://example.com");
        Assert.DoesNotContain(runs, r => r.Text.Contains("F8"));
        Assert.Contains(HelpDocument.Parse(html, true), r => r.Text.Contains("F8"));
        IReadOnlyList<HelpTextRun> inline = HelpDocument.Parse("Pay 100<!\"fortgump.3\"> now.");
        Assert.Equal(TutorialTextBreak.None, inline[1].BreakBefore);
        Assert.Equal(" now.", inline[2].Text);
    }

    /// <summary>없는 링크·게임 명령은 방문 기록을 바꾸지 않고 Back은 스크롤 위치까지 복원한다.</summary>
    [Fact]
    public void Navigation_RestoresHistoryAndScroll()
    {
        var topics = new HelpTopics("<a name=\"one\">\nOne\n</a>\n<a name=\"two\">\nTwo\n</a>");
        var navigation = new HelpNavigation(topics);
        Assert.True(navigation.Open("one"));
        navigation.Scroll = 120;
        Assert.False(navigation.Follow("#missing"));
        Assert.False(navigation.Follow("cmd:Tell,TechSupport"));
        Assert.False(navigation.Follow("https://example.com"));
        Assert.False(navigation.CanGoBack);
        Assert.True(navigation.Follow("#TWO"));
        Assert.Equal(0, navigation.Scroll);
        Assert.True(navigation.Back());
        Assert.Equal(("one", 120), (navigation.Anchor, navigation.Scroll));
        navigation.Close();
        Assert.Null(navigation.Anchor);
        Assert.False(navigation.CanGoBack);
    }

    /// <summary>배포 자산의 원문을 사용하여 모든 주제와 녹화에서 방문한 목차 링크가 해석되는지 확인한다.</summary>
    [Fact]
    public void OriginalHelp_AllTopicsAndRecordedLinksAreAvailable()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), GameLanguage.English);
        HelpTopics topics = resources.TryLoadHelp()!;
        // 대소문자 별칭을 합치면 원본의 고유 목적지는 127개다.
        Assert.Equal(127, topics.Count);
        Assert.True(topics.HasInformation("priestType"));
        Assert.False(topics.HasInformation("F1Help"));
        IReadOnlyList<HelpTextRun> contents = HelpDocument.Parse(topics.Find("F1Help")!);
        string[] recorded = ["campaignHelp", "interfaceHelp", "sphereHelp", "themeHelp", "priestType", "unitHelp", "vesselpriestHelp", "sacrificeOutline"];
        // 녹화에서 방문한 목차 항목은 모두 실제 본문으로 연결되어야 한다.
        foreach (string topic in recorded)
        {
            Assert.NotEmpty(HelpDocument.Parse(topics.Find(topic)!));
            Assert.Contains(contents, run => string.Equals(run.Link, "#" + topic, StringComparison.OrdinalIgnoreCase));
        }
    }
}
