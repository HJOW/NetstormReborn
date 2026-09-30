namespace Netstorm.Assets.Tests;

/// <summary>튜토리얼 안내의 본문 표시, 버튼 이동, 단계 다시 보기를 검사한다.</summary>
public sealed class TutorialDialogScriptTests
{
    /// <summary>MORE/BACK으로 옮겨도 F8은 단계 시작으로 돌아가고 보정 안내는 복귀 지점을 바꾸지 않는다.</summary>
    [Fact]
    public void StageNavigation_ReviewReturnsToLatestStage()
    {
        const string scriptText = """
            [A.]
            <h2>Buildings, Part 1</h2>
            Start <c>Temple</c> and <i>build</i>.
            <p>Next line.<br>More <drop>text</drop>.
            <!"mana.8">
            $Button=MORE,Tell,A1.
            [A1.]
            <h2>Buildings, Part 2</h2>
            Details.
            $Button=BACK,Tell,A.
            $Button=OK,DoNothing,0
            [B.]
            <h2>Temple Complete</h2>
            Continue.
            $Button=OK,DoNothing,0
            [NotVortex]
            <h2>Correction</h2>
            Wrong building.
            """;
        var dialog = new TutorialDialogScript(new MissionScript(scriptText), new ConfigStore());

        Assert.True(dialog.OpenStage("A."));
        Assert.Equal("Buildings, Part 1", dialog.Current!.Title);
        Assert.Contains(dialog.Current.Runs, run => run.Text.Contains("Temple") && run.Style == TutorialTextStyle.Highlight);
        Assert.Contains(dialog.Current.Runs, run => run.Text.Contains("build") && run.Style == TutorialTextStyle.Emphasis);
        Assert.Contains(dialog.Current.Runs, run => run.BreakBefore == TutorialTextBreak.Paragraph);
        Assert.Contains(dialog.Current.Runs, run => run.Text.Contains("[그림: mana]"));
        Assert.DoesNotContain(dialog.Current.Runs, run => run.Text.Contains("$Button"));
        Assert.Equal(TutorialDialogActionKind.Navigate, dialog.Choose(0).Kind);
        Assert.Equal("A1.", dialog.Current!.Section);
        Assert.True(dialog.OpenSection("NotVortex"));
        Assert.True(dialog.Review());
        Assert.Equal("A.", dialog.Current!.Section);
        Assert.True(dialog.OpenStage("B."));
        Assert.Equal(TutorialDialogActionKind.Close, dialog.Choose(0).Kind);
        Assert.Null(dialog.Current);
        Assert.True(dialog.Review());
        Assert.Equal("B.", dialog.Current!.Section);
        Assert.False(dialog.OpenSection("Missing"));
        Assert.Equal("B.", dialog.Current.Section);
    }

    /// <summary>버튼 없는 보정 안내의 닫기와 미션 종료·이동 명령을 구별한다.</summary>
    [Fact]
    public void FinalButtons_ReturnActionsAndButtonlessPageCanClose()
    {
        const string scriptText = """
            [F1.]
            <h2>Energy</h2>
            Select the Temple.
            [I.]
            <h2>Finished</h2>
            <$Config,Done=1>
            $Button=Leave Tutorials,LeaveBattle,1
            $Button=Next Tutorial,MissionBegin,Tutorial3
            """;
        var dialog = new TutorialDialogScript(new MissionScript(scriptText), new ConfigStore());

        Assert.True(dialog.OpenSection("F1."));
        Assert.Single(dialog.Current!.Buttons);
        Assert.Equal(TutorialDialogActionKind.Close, dialog.Choose(0).Kind);
        Assert.True(dialog.OpenStage("I."));
        Assert.DoesNotContain(dialog.Current!.Runs, run => run.Text.Contains("Config"));
        Assert.Equal(TutorialDialogActionKind.LeaveBattle, dialog.Choose(0).Kind);
        Assert.True(dialog.OpenStage("I."));
        TutorialDialogAction next = dialog.Choose(1);
        Assert.Equal(TutorialDialogActionKind.MissionBegin, next.Kind);
        Assert.Equal("Tutorial3", next.Argument);
    }

    /// <summary>원본 튜토리얼 2의 실제 섹션과 버튼 형식을 함께 읽는다.</summary>
    [Fact]
    public void OriginalTutorialTwo_OpensAThroughFinalActions()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("tutorial2"));
        var dialog = new TutorialDialogScript(mission.Script, resources.Settings);

        Assert.True(dialog.OpenStage("A."));
        Assert.Equal("Buildings, Part 1", dialog.Current!.Title);
        Assert.Equal("MORE", dialog.Current.Buttons[0].Label);
        Assert.Equal(TutorialDialogActionKind.Navigate, dialog.Choose(0).Kind);
        Assert.Equal("A1.", dialog.Current!.Section);
        Assert.True(dialog.OpenStage("I."));
        Assert.Equal("Mission Accomplished!", dialog.Current!.Title);
        Assert.Equal("Tutorial3", dialog.Choose(1).Argument);
    }
}
