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

    /// <summary>
    /// 글 사이 그림 표시(~[I타입.프레임])는 그림 구간이 되고, 그림 바로 뒤의 제목은 떼어 내지 않고 본문 흐름에 남는다
    /// (TEST01 브리핑: 풍선 그림 옆에 큰 제목, 아래 줄에 기울임 글).
    /// </summary>
    [Fact]
    public void PictureMark_BecomesPictureRunAndKeepsHeadingInline()
    {
        const string scriptText = """
            [A.]
            ~[IsunBalloon.a1]<h2>TEST01</h2>
            <i>caption</i> then ~[IwindWalker,9] icon
            $Button=Go!,DoNothing,0
            """;
        var dialog = new TutorialDialogScript(new MissionScript(scriptText), new ConfigStore());

        Assert.True(dialog.OpenStage("A."));
        TutorialDialogContent content = dialog.Current!;
        Assert.Equal("TEST01", content.Title);
        Assert.True(content.TitleInBody);
        Assert.Equal(TutorialTextStyle.Picture, content.Runs[0].Style);
        Assert.Equal("sunBalloon.a1", content.Runs[0].Text);
        // 그림 바로 뒤 제목은 같은 줄에 이어진다
        Assert.Equal(TutorialTextStyle.Heading, content.Runs[1].Style);
        Assert.Equal(TutorialTextBreak.None, content.Runs[1].BreakBefore);
        // 제목 뒤 글은 새 문단에서 시작한다
        Assert.Equal(TutorialTextStyle.Emphasis, content.Runs[2].Style);
        Assert.Equal(TutorialTextBreak.Paragraph, content.Runs[2].BreakBefore);
        // 쉼표 구분 표기도 "타입.프레임" 으로 맞춘다
        Assert.Contains(content.Runs, run => run.Style == TutorialTextStyle.Picture && run.Text == "windWalker.9");
        Assert.DoesNotContain(content.Runs, run => run.Text.Contains("~["));
    }

    /// <summary>제목이 맨 앞에 오는 보통 안내는 예전처럼 제목을 본문에서 떼어 낸다.</summary>
    [Fact]
    public void LeadingHeading_IsStillExtractedAsTitle()
    {
        const string scriptText = """
            [A.]
            <h2>Briefing</h2>
            Body with ~[Iicon.12] picture.
            $Button=OK,DoNothing,0
            """;
        var dialog = new TutorialDialogScript(new MissionScript(scriptText), new ConfigStore());

        Assert.True(dialog.OpenStage("A."));
        Assert.Equal("Briefing", dialog.Current!.Title);
        Assert.False(dialog.Current.TitleInBody);
        Assert.DoesNotContain(dialog.Current.Runs, run => run.Style == TutorialTextStyle.Heading);
        Assert.Contains(dialog.Current.Runs, run => run.Style == TutorialTextStyle.Picture && run.Text == "icon.12");
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

    /// <summary>
    /// 캠페인 미션의 초기 브리핑은 섹션 A. 이고 버튼은 Review Knowledge(ShowTechnology) / Play Mission(DoNothing)이다.
    /// Play Mission 으로 창이 닫혀야 세션 시간이 시작되고(docs/gameplay/dialog-pause.md 규칙 2), F8(Review)로 다시 열 수 있다.
    /// </summary>
    [Fact]
    public void OriginalCampaignBriefing_TheWarBeginsClosesWithPlayMission()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("thewarbegins"));
        var dialog = new TutorialDialogScript(mission.Script, resources.Settings);

        Assert.True(dialog.OpenStage("A."));
        Assert.Equal("The War Begins!", dialog.Current!.Title);
        Assert.Equal(["Review Knowledge", "Play Mission"], dialog.Current.Buttons.Select(button => button.Label));
        Assert.Equal("ShowTechnology", dialog.Current.Buttons[0].Action);
        Assert.Equal("DoNothing", dialog.Current.Buttons[1].Action);
        Assert.Equal(TutorialDialogActionKind.Close, dialog.Choose(1).Kind);
        Assert.Null(dialog.Current);
        // F8 (Review Mission Objectives) 는 브리핑을 다시 연다
        Assert.True(dialog.Review());
        Assert.Equal("A.", dialog.Current!.Section);
    }

    /// <summary>Review Knowledge(ShowTechnology,55)는 브리핑을 닫지 않고 지식 창 요청으로 분류된다 (인자 55는 원본에서도 쓰이지 않음).</summary>
    [Fact]
    public void ShowTechnologyButton_RequestsKnowledgeWindowAndKeepsBriefing()
    {
        var dialog = new TutorialDialogScript(
            new MissionScript("[A.]\n<h2>Title</h2>\nText.\n$Button=Review Knowledge,ShowTechnology,55\n$Button=Play Mission,DoNothing,0\n"),
            new ConfigStore());
        Assert.True(dialog.OpenBriefing());

        Assert.Equal(TutorialDialogActionKind.ShowKnowledge, dialog.Choose(0).Kind);
        Assert.NotNull(dialog.Current);
        Assert.Equal("A.", dialog.Current!.Section);
    }

    /// <summary>설정 명령만 있는 A. 는 브리핑 창으로 열지 않고, 보이는 본문이 있는 A. 는 연다.</summary>
    [Fact]
    public void OpenBriefing_SkipsSectionsWithoutVisibleText()
    {
        var silent = new TutorialDialogScript(new MissionScript("[A.]\n<$Config,atstop0=1>\n<$Config,atstop1=0>\n"), new ConfigStore());
        Assert.False(silent.OpenBriefing());
        Assert.Null(silent.Current);

        var briefing = new TutorialDialogScript(
            new MissionScript("[A.]\n<h2>Title</h2>\nText.\n$Button=Play Mission,DoNothing,0\n"), new ConfigStore());
        Assert.True(briefing.OpenBriefing());
        Assert.Equal("Title", briefing.Current!.Title);

        // A. 섹션이 아예 없는 스크립트는 열 것이 없다
        Assert.False(new TutorialDialogScript(new MissionScript("[B.]\nText.\n"), new ConfigStore()).OpenBriefing());
    }

    /// <summary>
    /// 원본의 모든 영어 미션 스크립트 중 섹션 A. 에 보이는 본문이 있는 것(튜토리얼·캠페인·훈련)은 제목과 버튼을 가진 안내 창으로 열려야 한다.
    /// 미션을 열 때 이 창이 열리는 동안 세션 시간이 멈추므로, 열리지 않는 스크립트가 있으면 그 미션은 브리핑 없이 바로 시작한다.
    /// </summary>
    [Fact]
    public void OriginalMissionScripts_EveryBriefingOpensWithTitleAndButtons()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        var opened = new List<string>();
        // d 폴더의 영어 스크립트를 모두 읽어 A. 가 있는 것만 검사한다
        foreach (string path in resources.Files.Find("d/*.english"))
        {
            string name = Path.GetFileNameWithoutExtension(path);
            LoadedMission? mission = resources.TryLoadMission(name);
            // 미션 이름으로 다시 찾지 못하는 파일(미션이 아닌 .english)은 건너뛴다
            if (mission == null)
            {
                continue;
            }
            var dialog = new TutorialDialogScript(mission.Script, resources.Settings);
            if (!dialog.OpenBriefing())
            {
                continue;
            }
            opened.Add(name);
            Assert.False(string.IsNullOrWhiteSpace(dialog.Current!.Title), name);
            Assert.True(dialog.Current.Buttons.Count > 0, $"{name}: 버튼 없음");
            Assert.True(dialog.Current.Runs.Count > 0, $"{name}: 본문 없음");
        }
        // 캠페인·튜토리얼·훈련 스크립트가 모두 들어 있다 (A. 가 있는 출하 스크립트는 34개)
        Assert.Contains("thewarbegins", opened);
        Assert.Contains("dissolvedalliance", opened);
        Assert.Contains("tutorial1", opened);
        Assert.True(opened.Count >= 30, $"A. 가 열리는 미션이 너무 적습니다: {opened.Count}");
    }

    /// <summary>
    /// 성공 창 버튼: MissionAbort 인자 1 은 곧바로 떠나기, 인자 0 은 확인 창, MissionRestart 는 재시작
    /// (exe FUN_00463e40, The War Begins! [Succeeded][BadTeamDead]).
    /// </summary>
    [Fact]
    public void MissionAbortAndRestart_ReturnActions()
    {
        const string scriptText = """
            [Succeeded][BadTeamDead]
            <h2>Success!</h2>
            Your island is now free!
            $Button=Leave Missions,MissionAbort,1
            $Button=Next Mission,MissionBegin,MasterOfWhirligigs
            [Failed]
            <h2>Failure!</h2>
            $Button=Leave Missions,MissionAbort,0
            $Button=Replay Mission,MissionRestart,0
            """;
        var dialog = new TutorialDialogScript(new MissionScript(scriptText), new ConfigStore());
        Assert.True(dialog.OpenSection("BadTeamDead"));
        Assert.Equal("Success!", dialog.Current!.Title);
        Assert.Equal(TutorialDialogActionKind.LeaveBattle, dialog.Choose(0).Kind);
        Assert.Null(dialog.Current);
        Assert.True(dialog.OpenSection("Succeeded"));
        Assert.Equal(new TutorialDialogAction(TutorialDialogActionKind.MissionBegin, "MasterOfWhirligigs"), dialog.Choose(1));
        Assert.True(dialog.OpenSection("Failed"));
        Assert.Equal(TutorialDialogActionKind.ConfirmLeave, dialog.Choose(0).Kind);
        Assert.True(dialog.OpenSection("Failed"));
        Assert.Equal(TutorialDialogActionKind.RestartMission, dialog.Choose(1).Kind);
    }

    /// <summary>
    /// 실패 창의 Continue(Tell,TryAgain)는 미션 스크립트가 아니라 공용 tell.english 의 섹션을 연다.
    /// 제목에는 현재 미션 이름이 들어가고 버튼은 Replay Mission(MissionRestart)·Leave Missions(MissionAbort,1)다.
    /// </summary>
    [Fact]
    public void FailureContinue_OpensTryAgainFromCommonScript()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("thewarbegins"));
        LoadedMission common = Assert.IsType<LoadedMission>(resources.TryLoadMission("tell"));
        var values = new Dictionary<string, string> { ["title"] = "The War Begins!", ["fileName"] = "thewarbegins" };
        var dialog = new TutorialDialogScript(mission.Script, resources.Settings, common.Script, values);
        int layersBefore = resources.Settings.LayerCount;

        Assert.True(dialog.OpenSection("Failed"));
        Assert.Equal("Failure!", dialog.Current!.Title);
        Assert.Equal(TutorialDialogActionKind.Navigate, dialog.Choose(0).Kind);
        Assert.Equal("TryAgain", dialog.Current!.Section);
        Assert.Contains("The War Begins!", string.Concat(dialog.Current.Runs.Select(run => run.Text)));
        Assert.DoesNotContain("Not Found", string.Concat(dialog.Current.Runs.Select(run => run.Text)));
        Assert.Equal(["Replay Mission", "Leave Missions"], dialog.Current.Buttons.Select(button => button.Label));
        Assert.Equal(TutorialDialogActionKind.RestartMission, dialog.Choose(0).Kind);
        // 미션 치환 층은 섹션을 준비한 뒤 남지 않는다
        Assert.Equal(layersBefore, resources.Settings.LayerCount);
    }
}
