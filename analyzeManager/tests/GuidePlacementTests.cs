using System.Drawing;

namespace Netstorm.AnalyzeManager.Tests;

/// <summary>게임 창 크기 변경 후 수동 분석 안내 창의 배치 좌표를 검사한다.</summary>
public sealed class GuidePlacementTests
{
    /// <summary>게임 창이 작아진 뒤 확보된 오른쪽 공간으로 안내 창을 옮긴다.</summary>
    [Fact]
    public void MovesToRightAfterGameShrinks()
    {
        Rectangle[] screens = [new(0, 0, 1600, 900)];
        Size guide = new(328, 488);
        Assert.Null(GuidePlacement.Beside(new Rectangle(0, 0, 1600, 828), guide, screens));
        Assert.Equal(new Point(1032, 0), GuidePlacement.Beside(new Rectangle(0, 0, 1024, 768), guide, screens));
    }

    /// <summary>게임 창 오른쪽 공간이 부족하면 왼쪽으로 배치한다.</summary>
    [Fact]
    public void MovesToLeftWhenRightIsOccupied()
    {
        Point? location = GuidePlacement.Beside(new Rectangle(700, 100, 850, 700), new Size(320, 480), [new Rectangle(0, 0, 1600, 900)]);
        Assert.Equal(new Point(372, 100), location);
    }

    /// <summary>보조 모니터에 안내 창 전체가 들어가면 경계를 넘어 배치한다.</summary>
    [Fact]
    public void UsesSecondMonitor()
    {
        Rectangle[] screens = [new(0, 0, 1280, 800), new(1280, 0, 1280, 800)];
        Point? location = GuidePlacement.Beside(new Rectangle(0, 0, 1280, 768), new Size(320, 480), screens);
        Assert.Equal(new Point(1288, 0), location);
    }

    /// <summary>안내 창을 완전히 놓을 수 없는 작업 영역은 선택하지 않는다.</summary>
    [Fact]
    public void RejectsPartialPlacement()
    {
        Point? location = GuidePlacement.Beside(new Rectangle(0, 0, 1280, 768), new Size(320, 480), [new Rectangle(0, 0, 1500, 900)]);
        Assert.Null(location);
    }
}
