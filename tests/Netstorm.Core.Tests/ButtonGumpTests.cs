using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 돌 버튼의 누름·떼기 규칙 테스트. 기대값은 2026-10-03 원본 자동 분석(메인 메뉴 Credits 버튼, 대화상자 Cancel,
/// 팁 창 버튼)에서 관찰한 결과다 — docs/videos/menu-buttons-20261003.md.
/// </summary>
public sealed class ButtonGumpTests
{
    /// <summary>메인 메뉴 Credits 버튼 (가로 435~509, 세로 334~353) 과 이웃 Options 버튼.</summary>
    private static readonly GumpButton[] Menu =
    [
        new(1, 435, 334, 75, 20), // Credits
        new(2, 514, 334, 75, 20), // Options
    ];

    /// <summary>한 프레임을 처리하는 도우미.</summary>
    private static GumpResult Step(ButtonGump gump, int x, int y, bool down, GumpButton[]? buttons = null) =>
        gump.Update(buttons ?? Menu, x, y, down);

    /// <summary>버튼 위에서 누르면 그 프레임에 눌림 이벤트가 나오고 눌린 모양이 되며, 안쪽에서 떼면 실행된다.</summary>
    [Fact]
    public void PressAndReleaseInside_Activates()
    {
        var gump = new ButtonGump();
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, false));
        Assert.Equal(new GumpResult(1, null), Step(gump, 472, 343, true));
        Assert.True(gump.IsPressed(1));
        Assert.False(gump.IsPressed(2));
        Assert.Equal(new GumpResult(null, 1), Step(gump, 472, 343, false));
        Assert.False(gump.IsPressed(1));
        Assert.Null(gump.Held);
    }

    /// <summary>오래 눌러도(프레임이 많이 지나도) 눌림·실행 이벤트는 반복되지 않는다.</summary>
    [Fact]
    public void LongHold_DoesNotRepeat()
    {
        var gump = new ButtonGump();
        Assert.Equal(1, Step(gump, 472, 343, true).Pressed);
        // 2초(60프레임) 동안 계속 누른다
        for (int frame = 0; frame < 60; frame++)
        {
            Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, true));
            Assert.True(gump.IsPressed(1));
        }
        Assert.Equal(1, Step(gump, 472, 343, false).Activated);
    }

    /// <summary>누른 채 바깥으로 나가면 평소 모양이 되고, 바깥에서 떼면 실행하지 않는다 (O1).</summary>
    [Fact]
    public void ReleaseOutside_DoesNotActivate()
    {
        var gump = new ButtonGump();
        Step(gump, 472, 343, true);
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 430, true));
        Assert.False(gump.IsPressed(1));
        Assert.Equal(1, gump.Held);
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 430, false));
        Assert.Null(gump.Held);
    }

    /// <summary>나갔다가 돌아와서 떼면 눌린 모양이 돌아오고 실행한다. 다시 들어올 때 눌림 이벤트(소리)는 없다 (O2·O3).</summary>
    [Fact]
    public void LeaveAndReturn_ActivatesWithoutSecondPress()
    {
        var gump = new ButtonGump();
        Assert.Equal(1, Step(gump, 472, 343, true).Pressed);
        Step(gump, 472, 430, true);
        Assert.False(gump.IsPressed(1));
        GumpResult back = Step(gump, 472, 345, true);
        Assert.Null(back.Pressed);
        Assert.True(gump.IsPressed(1));
        Assert.Equal(new GumpResult(null, 1), Step(gump, 472, 345, false));
    }

    /// <summary>버튼 밖에서 눌러 버튼 안으로 끌고 와 떼면 불발이고, 그 사이 버튼은 눌린 모양이 되지 않는다 (O4).</summary>
    [Fact]
    public void PressOutsideThenReleaseInside_DoesNothing()
    {
        var gump = new ButtonGump();
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 430, true));
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, true));
        Assert.False(gump.IsPressed(1));
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, false));
    }

    /// <summary>한 버튼에서 눌러 이웃 버튼 위에서 떼면 어느 쪽도 실행하지 않는다 (O5).</summary>
    [Fact]
    public void ReleaseOnNeighbor_DoesNothing()
    {
        var gump = new ButtonGump();
        Step(gump, 472, 343, true);
        Step(gump, 551, 343, true);
        Assert.False(gump.IsPressed(1));
        Assert.False(gump.IsPressed(2));
        Assert.Equal(new GumpResult(null, null), Step(gump, 551, 343, false));
    }

    /// <summary>판정 영역은 왼쪽·위 끝을 포함하고 오른쪽·아래 끝을 포함하지 않는다 (경계 스캔: x 435~509, y 334~353).</summary>
    [Theory]
    [InlineData(434, 343, false)]
    [InlineData(435, 343, true)]
    [InlineData(509, 343, true)]
    [InlineData(510, 343, false)]
    [InlineData(472, 333, false)]
    [InlineData(472, 334, true)]
    [InlineData(472, 353, true)]
    [InlineData(472, 354, false)]
    public void HitArea_MatchesOriginalEdges(int x, int y, bool expectedPress)
    {
        var gump = new ButtonGump();
        Assert.Equal(expectedPress, Step(gump, x, y, true).Pressed == 1);
    }

    /// <summary>비활성 버튼은 눌러도 붙잡지 않고 이벤트도 없다. 아래에 있는 활성 버튼으로 넘어가지도 않는다.</summary>
    [Fact]
    public void DisabledButton_IsIgnored()
    {
        GumpButton[] buttons = [new(1, 0, 0, 100, 20), new(2, 0, 0, 100, 20, Enabled: false)];
        var gump = new ButtonGump();
        Assert.Equal(new GumpResult(null, null), gump.Update(buttons, 10, 10, true));
        Assert.Null(gump.Held);
        Assert.Equal(new GumpResult(null, null), gump.Update(buttons, 10, 10, false));
    }

    /// <summary>겹친 버튼은 뒤쪽(위에 그려진) 것이 받는다.</summary>
    [Fact]
    public void OverlappingButtons_TopMostWins()
    {
        GumpButton[] buttons = [new(1, 0, 0, 100, 40), new(2, 0, 0, 100, 20)];
        var gump = new ButtonGump();
        Assert.Equal(2, gump.Update(buttons, 10, 10, true).Pressed);
    }

    /// <summary>다른 곳(펼침 메뉴)이 누름을 가져간 프레임에는 버튼을 붙잡지 않는다.</summary>
    [Fact]
    public void CanPressFalse_IgnoresPress()
    {
        var gump = new ButtonGump();
        Assert.Equal(new GumpResult(null, null), gump.Update(Menu, 472, 343, true, canPress: false));
        Assert.Null(gump.Held);
        Assert.Equal(new GumpResult(null, null), gump.Update(Menu, 472, 343, false));
    }

    /// <summary>붙잡은 버튼이 화면에서 사라지면(페이지 전환) 취소하고 실행하지 않는다.</summary>
    [Fact]
    public void ButtonRemovedWhileHeld_Cancels()
    {
        var gump = new ButtonGump();
        Step(gump, 472, 343, true);
        GumpButton[] other = [new(9, 0, 0, 10, 10)];
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, true, other));
        Assert.Null(gump.Held);
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, false, other));
    }

    /// <summary>Cancel 은 붙잡은 버튼을 놓고 이후 떼기도 실행하지 않는다.</summary>
    [Fact]
    public void Cancel_ReleasesWithoutActivating()
    {
        var gump = new ButtonGump();
        Step(gump, 472, 343, true);
        gump.Cancel();
        Assert.False(gump.IsPressed(1));
        Assert.Equal(new GumpResult(null, null), Step(gump, 472, 343, false));
    }
}
