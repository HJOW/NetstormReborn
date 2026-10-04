namespace Netstorm.Assets.Tests;

/// <summary>원본 조건 문법·32비트 숫자 변환·표시 상태와 미션 본문 준비를 검증한다.</summary>
public sealed class MissionConditionTests
{
    /// <summary>원본은 10진 정수 접두어가 0인지 검사하며 문자열 불리언이나 16진 표기를 특별 취급하지 않는다.</summary>
    [Theory]
    [InlineData("0", false)]
    [InlineData("1", true)]
    [InlineData("-2", true)]
    [InlineData("+12 trailing", true)]
    [InlineData("true", false)]
    [InlineData("0x10", false)]
    [InlineData("0.5", false)]
    [InlineData("1=0", true)]
    [InlineData("", false)]
    [InlineData("!0", true)]
    [InlineData("!!0", true)]
    [InlineData("!!1", false)]
    [InlineData(" !0", false)]
    [InlineData("4294967296", false)]
    [InlineData("4294967297", true)]
    public void NumericTruth_UsesOriginalDecimalPrefix(string expression, bool expected)
    {
        Assert.Equal(expected, MissionConditions.Evaluate(expression, new ConfigStore()));
    }

    /// <summary>문자열 일치·부정·숫자 비교의 방향과 같은 값의 경계를 검사한다.</summary>
    [Theory]
    [InlineData("?same=same", true)]
    [InlineData("?Same=same", false)]
    [InlineData("?!same=same", false)]
    [InlineData("?\"a=b\"=\"a=b\"", true)]
    [InlineData("? a b = a b ", true)]
    [InlineData("?a=b=c", false)]
    [InlineData("g3=2", true)]
    [InlineData("g2=3", false)]
    [InlineData("g3=3", false)]
    [InlineData("GE3=3", true)]
    [InlineData("ge2=3", false)]
    [InlineData("l2=3", true)]
    [InlineData("l3=2", false)]
    [InlineData("l3=3", false)]
    [InlineData("LE3=3", true)]
    [InlineData("le3=2", false)]
    [InlineData("l!2=3", false)]
    [InlineData("!l2=3", false)]
    [InlineData("g2147483648=0", false)]
    [InlineData("l2147483648=0", true)]
    public void Comparisons_MatchOriginalPrefixes(string expression, bool expected)
    {
        Assert.Equal(expected, MissionConditions.Evaluate(expression, new ConfigStore()));
    }

    /// <summary>숨겨진 본문의 줄·인라인 명령을 제거하고 표시되는 HTML과 명령만 그대로 남긴다.</summary>
    [Fact]
    public void Filter_PreservesVisibleMarkupAndRemovesHiddenCommands()
    {
        var settings = new ConfigStore();
        settings.Push(new ConfigText("enabled = 1\ndisabled = 0\nlabel = 확인\n"));
        string body = "<h2>제목</h2>\n"
            + "<?{disabled}>$Button=숨김,QuitApp,0\n<$Config,changed=1></?>"
            + "<?{enabled}>$Button={label},Tell,@1</?>\n<$PlaySound,ready.wav>";
        string result = MissionConditions.Filter(body, settings);

        Assert.Equal("<h2>제목</h2>\n$Button=확인,Tell,@1\n<$PlaySound,ready.wav>", result);
        Assert.Null(settings.Get("changed"));
        Assert.DoesNotContain("QuitApp", result);
    }

    /// <summary>첫 치환에서 생성한 비교식과 태그를 평가하며 미지정 변수의 숫자 값은 0으로 취급된다.</summary>
    [Fact]
    public void Filter_ExpandsBodyBeforeParsingConditions()
    {
        var settings = new ConfigStore();
        settings.Push(new ConfigText("expression = g3=2\nname = Temple\n"));
        string result = MissionConditions.Filter(
            "<?{expression}>compare</?><??{name}=Temple>equal</?>"
            + "<?{absent}>hidden</?><?!{absent}>missing</?>", settings);
        Assert.Equal("compareequalmissing", result);
    }

    /// <summary>원본은 참인 여는 조건을 쌓지 않고, 거짓 상태의 첫 닫는 조건에서 표시를 재개한다.</summary>
    [Fact]
    public void Filter_HiddenOpeningConditionsDoNotCreateNestedScopes()
    {
        Assert.Equal("outertail", MissionConditions.Filter("<?1>outer<?0>hidden</?>tail</?>", new ConfigStore()));
        Assert.Equal("tail", MissionConditions.Filter("<?0><?1>hidden</?>tail</?>", new ConfigStore()));
        Assert.Equal("tail", MissionConditions.Filter("<?0><?0>hidden</?>tail</?>", new ConfigStore()));
    }

    /// <summary>불완전한 조건 태그와 여분 닫는 태그에도 멈추지 않고 숨김 상태의 끝부분을 표시하지 않는다.</summary>
    [Theory]
    [InlineData("before<?0>after", "before")]
    [InlineData("before<?1>after", "beforeafter")]
    [InlineData("before</?>after", "beforeafter")]
    [InlineData("before<?0", "before")]
    [InlineData("before<h2", "before<h2")]
    public void Filter_IncompleteTagsDoNotThrow(string body, string expected)
    {
        Assert.Equal(expected, MissionConditions.Filter(body, new ConfigStore()));
    }

    /// <summary>따옴표까지 포함한 원본 피연산자 입력 한도 78문자를 적용한다.</summary>
    [Fact]
    public void Operand_ReadLimitIncludesQuotes()
    {
        string prefix = new('a', 76);
        Assert.True(MissionConditions.Evaluate($"?\"{prefix}\"={prefix}", new ConfigStore()));
        Assert.False(MissionConditions.Evaluate($"?\"{prefix}b\"={prefix}b", new ConfigStore()));
    }

    /// <summary>동일 섹션을 설정 상태에 따라 다시 준비하면 버튼과 체크 필드가 함께 갱신된다.</summary>
    [Fact]
    public void PrepareSection_ExtractsOnlyEnabledCommandsAfterSubstitution()
    {
        var script = new MissionScript("[Menu]\n<?{ready}>$Button=진행,Tell,@1</?>\n"
            + "$Checked=다음,MissionBegin,next,{done},<?!{alive}>1</?>\n[End]\n");
        var settings = new ConfigStore();
        settings.Push(new ConfigText("ready = 0\ndone = 1\nalive = 1\n"));
        PreparedMissionSection first = Assert.IsType<PreparedMissionSection>(script.PrepareSection("Menu", settings));
        MissionCommand check = Assert.Single(first.Commands);
        Assert.Equal(new[] { "다음", "MissionBegin", "next", "1", "" }, check.Fields);

        settings.Push(new ConfigText("ready = 1\nalive = 0\n"));
        PreparedMissionSection second = Assert.IsType<PreparedMissionSection>(script.PrepareSection("Menu", settings));
        Assert.Equal(2, second.Commands.Count);
        Assert.Equal("Button", second.Commands[0].Name);
        Assert.Equal("1", second.Commands[1].Fields[4]);
        Assert.Null(script.PrepareSection("missing", settings));
    }

    /// <summary>원본 CraftWarning의 경계값을 검증하여 숫자 비교 방향을 확인한다.</summary>
    [Theory]
    [InlineData(3, true)]
    [InlineData(4, false)]
    [InlineData(5, false)]
    public void Original_CraftWarningUsesShardThreshold(int shards, bool insufficient)
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        resources.Settings.Push(ConfigStore.FromPairs(new Dictionary<string, string> { ["3"] = shards.ToString() }), "param");
        LoadedMission mission = Assert.IsType<LoadedMission>(resources.TryLoadMission("tell"));
        PreparedMissionSection section = Assert.IsType<PreparedMissionSection>(mission.Script.PrepareSection("CraftWarning", resources.Settings));
        Assert.Equal(insufficient, section.Body.Contains("You can not Craft at This Time", StringComparison.Ordinal));
        Assert.Equal(!insufficient, section.Body.Contains("you can craft this !", StringComparison.Ordinal));
        Assert.Equal(2, section.Commands.Count);
        Assert.DoesNotContain("<?", section.Body);
    }

    /// <summary>전체 원본 미션·메뉴의 섹션을 준비하여 조건 평가가 멈추거나 명령을 실행하지 않는지 검사한다.</summary>
    [Fact]
    public void Original_AllScriptSectionsCanBePrepared()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        var files = GameLanguage.OriginalLanguages.SelectMany(language => resources.Files.Find($"d/*.{language}"))
            .Where(path => !System.IO.Path.GetFileName(path).StartsWith("xlat.", StringComparison.Ordinal)
                && !System.IO.Path.GetFileName(path).StartsWith("config.", StringComparison.Ordinal)).ToArray();
        int sections = 0;
        int conditions = 0;
        // 모든 언어의 원본 스크립트를 읽는다. 게임 동작이나 설정 저장은 호출하지 않는다.
        foreach (string path in files)
        {
            MissionScript script = MissionScript.FromFileBytes(resources.Files.ReadAllBytes(path));
            // 각 섹션의 본문을 원본 조건 규칙으로 필터링한다.
            foreach (MissionSection section in script.Sections)
            {
                string prepared = MissionConditions.Filter(section.Body, resources.Settings);
                Assert.NotNull(MissionScript.FindCommands(prepared));
                sections++;
                conditions += section.Body.Split("<?", StringSplitOptions.None).Length - 1;
            }
        }
        Assert.True(files.Length > 600);
        Assert.True(sections > 1000);
        Assert.True(conditions > 2500);
    }
}
