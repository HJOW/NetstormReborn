using Netstorm.Core.Display;

namespace Netstorm.Core.Tests;

/// <summary>가장자리 스크롤 규칙 테스트 (원본 004d65de~004d67ad 분석, docs/exe/edge-scroll.md)</summary>
public sealed class EdgeScrollTests
{
    /// <summary>전체화면 1920×1080 에서 커서 위치만 바꾼 기본 입력</summary>
    private static EdgeScrollInput Input(int x, int y, bool borderless = true, bool enabled = true,
        bool leftDown = false, bool shift = false, bool popup = false, bool topBlocked = false) =>
        new(x, y, 1920, 1080, enabled, borderless, leftDown, shift, popup, topBlocked);

    /// <summary>화면 한가운데는 스크롤하지 않고, 맨 바깥 1픽셀 이내에서만 방향이 생긴다</summary>
    [Theory]
    [InlineData(960, 540, 0, 0)]
    [InlineData(2, 540, 0, 0)]
    [InlineData(1, 540, -1, 0)]
    [InlineData(0, 540, -1, 0)]
    [InlineData(1917, 540, 0, 0)]
    [InlineData(1919, 540, 1, 0)]
    [InlineData(960, 1, 0, -1)]
    [InlineData(960, 1079, 0, 1)]
    [InlineData(0, 0, -1, -1)]
    [InlineData(1919, 1079, 1, 1)]
    [InlineData(0, 1079, -1, 1)]
    public void Direction_UsesOnePixelEdge(int x, int y, int expectedX, int expectedY)
    {
        Assert.Equal((expectedX, expectedY), EdgeScrollController.Direction(Input(x, y)));
    }

    /// <summary>창 테두리가 있는 일반 창 모드·설정 꺼짐·팝업·왼쪽 버튼 누름에서는 스크롤하지 않는다</summary>
    [Fact]
    public void Direction_DisabledConditions()
    {
        Assert.Equal((0, 0), EdgeScrollController.Direction(Input(0, 540, borderless: false)));
        Assert.Equal((0, 0), EdgeScrollController.Direction(Input(0, 540, enabled: false)));
        Assert.Equal((0, 0), EdgeScrollController.Direction(Input(0, 540, popup: true)));
        Assert.Equal((0, 0), EdgeScrollController.Direction(Input(0, 540, leftDown: true)));
    }

    /// <summary>게임 메뉴 막대가 열려 있으면 위쪽만 막고 다른 가장자리는 그대로 동작한다</summary>
    [Fact]
    public void Direction_TopEdgeBlockedByMenuBar()
    {
        Assert.Equal((0, 0), EdgeScrollController.Direction(Input(960, 0, topBlocked: true)));
        Assert.Equal((-1, 0), EdgeScrollController.Direction(Input(0, 0, topBlocked: true)));
        Assert.Equal((0, 1), EdgeScrollController.Direction(Input(960, 1079, topBlocked: true)));
    }

    /// <summary>Shift 를 누르면 모서리에서 한 축만 남는다 (같은 크기면 y 가 남음, 원본 004d6762)</summary>
    [Fact]
    public void Direction_ShiftKeepsSingleAxis()
    {
        Assert.Equal((0, -1), EdgeScrollController.Direction(Input(0, 0, shift: true)));
        Assert.Equal((0, 1), EdgeScrollController.Direction(Input(1919, 1079, shift: true)));
        // 한 축만 가장자리이면 Shift 와 상관없이 그대로
        Assert.Equal((-1, 0), EdgeScrollController.Direction(Input(0, 540, shift: true)));
    }

    /// <summary>속도 곡선: 시작 2, 초당 +30, 상한 35 (원본 [00506590]=2.0, [00501708]=30.0, edgeScrollSpeed=35)</summary>
    [Theory]
    [InlineData(0.0, 35, 2)]
    [InlineData(0.03, 35, 2)]
    [InlineData(0.034, 35, 3)]
    [InlineData(0.5, 35, 17)]
    [InlineData(1.0, 35, 32)]
    [InlineData(1.1, 35, 35)]
    [InlineData(10.0, 35, 35)]
    [InlineData(10.0, 10, 10)]
    [InlineData(0.0, 1, 1)]
    [InlineData(0.0, 0, 0)]
    public void SpeedPerFrame_Ramps(double seconds, int maxSpeed, int expected)
    {
        Assert.Equal(expected, EdgeScrollController.SpeedPerFrame(seconds, maxSpeed));
    }

    /// <summary>처음 닿은 프레임은 시작 속도(2 px/프레임)를 초당 픽셀로 환산한다 (2 × 75 × dt)</summary>
    [Fact]
    public void Update_FirstFrame_UsesStartSpeed()
    {
        var controller = new EdgeScrollController();
        (double x, double y) = controller.Update(Input(0, 540), 1.0 / 60);
        Assert.Equal(-2 * 75.0 / 60, x, 9);
        Assert.Equal(0.0, y);
    }

    /// <summary>머무는 시간이 길어지면 상한 35 px/프레임(= 초당 2625px)에 닿고, 벗어났다 돌아오면 다시 시작 속도</summary>
    [Fact]
    public void Update_AcceleratesThenResets()
    {
        var controller = new EdgeScrollController();
        (double x, _) = (0.0, 0.0);
        // 2초 동안(120프레임) 왼쪽 가장자리에 머문다
        for (int frame = 0; frame < 120; frame++)
        {
            (x, _) = controller.Update(Input(0, 540), 1.0 / 60);
        }
        Assert.Equal(-35 * 75.0 / 60, x, 9);
        // 한 프레임 벗어나면 이동이 없고 가속이 초기화된다
        Assert.Equal((0.0, 0.0), controller.Update(Input(500, 540), 1.0 / 60));
        (x, _) = controller.Update(Input(0, 540), 1.0 / 60);
        Assert.Equal(-2 * 75.0 / 60, x, 9);
    }

    /// <summary>속도 상한을 설정으로 바꿀 수 있다 (원본 edgeScrollSpeed)</summary>
    [Fact]
    public void Update_RespectsConfiguredMaxSpeed()
    {
        var controller = new EdgeScrollController { MaxSpeed = 10 };
        (double x, double y) = (0.0, 0.0);
        // 충분히 오래 오른쪽 아래 모서리에 머문다 (대각선)
        for (int frame = 0; frame < 120; frame++)
        {
            (x, y) = controller.Update(Input(1919, 1079), 1.0 / 60);
        }
        Assert.Equal(10 * 75.0 / 60, x, 9);
        Assert.Equal(10 * 75.0 / 60, y, 9);
    }
}
