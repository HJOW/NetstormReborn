using Netstorm.Assets;

namespace Netstorm.Assets.Tests;

/// <summary>원본 미션 스크립트에서 캠페인 장·미션 목록과 진행 조건을 읽는 동작을 검사한다.</summary>
public sealed class CampaignMenuTests
{
    /// <summary>저장소의 캠페인 자료와 요청 언어·저장 진행값으로 자원 집합을 만든다.</summary>
    /// <param name="language">UI 언어</param>
    /// <param name="scriptValues">사용자별 스크립트 설정값</param>
    private static GameResources CreateResources(string language = "english",
        IEnumerable<KeyValuePair<string, string>>? scriptValues = null) =>
        new(GameFileSystem.Open(OriginalData.RequireDirectory()), language, scriptValues: scriptValues);

    /// <summary>첫 캠페인 창은 원본 장 파일 순서·구분선·Tell 대상을 사용한다.</summary>
    [Fact]
    public void Root_UsesOfficialCampaignFilesInOrder()
    {
        CampaignPage page = Assert.IsType<CampaignPage>(CampaignMenu.Open(CreateResources(), CampaignMenu.Root));

        Assert.Equal("Campaign", page.Content.Title);
        Assert.Equal(
            ["Early Missions", "Priest Training", "---", "Struggle For Freedom", "A Nation Rises", "Complete Victory", "---", "User Made Campaigns"],
            page.Items.Select(item => item.Label));
        Assert.Equal("offical3.overview", page.Items[3].Argument);
        Assert.True(page.Items[2].Separator);
        Assert.True(page.Items[6].Separator);
        Assert.Equal(1, page.TrailingBlankLines);
        Assert.DoesNotContain(page.Content.Runs, run => run.Text.Contains("~w", StringComparison.Ordinal));
    }

    /// <summary>Struggle For Freedom 의 여섯 미션은 완료값·선행 조건에 따라 완료와 잠금을 나눈다.</summary>
    [Fact]
    public void Chapter_UsesScriptCompletionAndAvailabilityConditions()
    {
        GameResources resources = CreateResources();
        CampaignPage unopened = Assert.IsType<CampaignPage>(CampaignMenu.Open(resources, "offical3.overview"));

        Assert.Equal(6, unopened.Items.Count);
        Assert.Equal("Struggle For Freedom", unopened.Content.Title);
        Assert.Equal(
            ["TheWarBegins", "MasterOfWhirligigs", "SaveTheIsland", "FragileFortune", "ThunderingPower", "DissolvedAlliance"],
            unopened.Items.Select(item => item.Argument));
        Assert.All(unopened.Items, item => Assert.False(item.Checked));
        Assert.True(unopened.Items[0].Enabled);
        Assert.False(unopened.Items[1].Enabled);

        resources.SetScriptValue("DoneTheWarBegins", "1");
        CampaignPage afterFirstWin = Assert.IsType<CampaignPage>(CampaignMenu.Open(resources, "offical3.overview"));
        Assert.True(afterFirstWin.Items[0].Checked);
        Assert.True(afterFirstWin.Items[1].Enabled);
        Assert.False(afterFirstWin.Items[1].Checked);
        Assert.False(afterFirstWin.Items[2].Enabled);
    }

    /// <summary>선택 언어의 공식 장 파일이 없으면 영어 파일을 찾아 장 목록을 채운다.</summary>
    [Fact]
    public void Root_FallsBackToEnglishCampaignFiles()
    {
        CampaignPage page = Assert.IsType<CampaignPage>(CampaignMenu.Open(CreateResources("korean"), CampaignMenu.Root));

        Assert.Equal(8, page.Items.Count);
        Assert.Equal("Struggle For Freedom", page.Items[3].Label);
    }
}
