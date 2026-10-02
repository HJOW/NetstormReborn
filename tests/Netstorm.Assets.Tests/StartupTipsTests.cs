namespace Netstorm.Assets.Tests;

/// <summary>시작 팁 창의 원본 [TipList] 읽기·문자열 정리·한국어 번역을 검사한다.</summary>
public sealed class StartupTipsTests
{
    /// <summary>
    /// 원본 tell.english 의 tip0~tip39 를 모두 읽고, 2026-10-01 녹화 시작 화면의 tip15 문장과 같은 본문을 돌려준다.
    /// 색 코드·링크 태그·스크립트 명령은 빠지고, 마지막 팁만 팁 번호를 0 으로 되돌린다.
    /// </summary>
    [Fact]
    public void OriginalTipList_ReadsAllTipsAndRecordedTip()
    {
        var resources = new GameResources(GameFileSystem.Open(OriginalData.RequireDirectory()), "english");
        LoadedMission tell = Assert.IsType<LoadedMission>(resources.TryLoadMission("tell"));
        var tips = new StartupTips(tell.Script.GetSection(StartupTips.SectionName));

        Assert.Equal(40, tips.Count);
        Assert.Equal("A mix of weapons is often better than focusing on just one type.", tips.Text(15, korean: false));
        Assert.Equal("You can use F2 to make bridge connecting easier.", tips.Text(1, korean: false));
        Assert.DoesNotContain(tips.Text(0, korean: false), "<");
        Assert.Contains("NetstormHQ", tips.Text(0, korean: false));
        Assert.True(tips.ResetsNumber(39));
        Assert.False(tips.ResetsNumber(15));
        Assert.Equal(StartupTips.OutOfTipsEnglish, tips.Text(40, korean: false));
        // 한국어는 번호마다 번역 문장을 쓰고, 없는 번호는 한국어 안내다
        Assert.Contains("무기", tips.Text(15, korean: true));
        Assert.Equal(StartupTips.OutOfTipsKorean, tips.Text(40, korean: true));
        // 모든 번호에 번역이 있다
        for (int number = 0; number < tips.Count; number++)
        {
            Assert.NotEqual(tips.Text(number, korean: false), tips.Text(number, korean: true));
        }
    }

    /// <summary>원본 문자열 정리: 역따옴표로 감싼 따옴표·링크·색 코드·명령을 빼고 공백을 하나로 줄인다.</summary>
    [Fact]
    public void Clean_RemovesMarkupButKeepsVisibleText()
    {
        Assert.Equal("Visit NetstormHQ now.", StartupTips.Clean("Visit <a href=\"http://x\">NetstormHQ</a>  now.<$Config,tipNumber=0>"));
        Assert.Equal("Press Q W to start.", StartupTips.Clean("Press {CText}Q W{Text} to start."));
        var tips = new StartupTips("tip2=\"Say `\"hi`\" {CText}now{Text}\"\r\nnot a tip\r\n");
        Assert.Equal(1, tips.Count);
        Assert.Equal("Say \"hi\" now", tips.Text(2, korean: false));
    }
}
