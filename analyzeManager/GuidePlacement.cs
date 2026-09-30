using System.Drawing;

namespace Netstorm.AnalyzeManager;

/// <summary>주 게임 창과 겹치지 않는 안내 창 위치를 모니터 작업 영역에서 찾는다.</summary>
public static class GuidePlacement
{
    /// <summary>게임 창과 안내 창 사이에 남길 픽셀 간격.</summary>
    private const int Gap = 8;

    /// <summary>오른쪽을 먼저 시도하고, 공간이 없으면 왼쪽의 첫 사용 가능한 위치를 반환한다.</summary>
    public static Point? Beside(Rectangle game, Size guide, IEnumerable<Rectangle> workAreas)
    {
        if (game.Width <= 0 || game.Height <= 0 || guide.Width <= 0 || guide.Height <= 0) return null;
        int right = game.Right + Gap;
        int left = game.Left - guide.Width - Gap;
        // 오른쪽과 왼쪽을 차례로 시도해 게임과의 거리를 최소화한다.
        foreach (int x in new[] { right, left })
        {
            // 각 모니터에서 안내 창 전체가 작업 영역 안에 드는지 검사한다.
            foreach (Rectangle area in workAreas)
            {
                if (area.Height < guide.Height) continue;
                int y = Math.Clamp(game.Top, area.Top, area.Bottom - guide.Height);
                if (area.Contains(new Rectangle(x, y, guide.Width, guide.Height))) return new Point(x, y);
            }
        }
        return null;
    }
}
