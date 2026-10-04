namespace Netstorm.Core.Display;

/// <summary>
/// 게임 화면(논리 해상도)이 창·전체화면 안 어디에 얼마나 크게 그려지는지 나타낸다.
/// 게임은 항상 논리 해상도로 그린 뒤 뷰포트 사각형에 늘려 표시한다.
/// </summary>
/// <param name="LogicalWidth">게임이 그리는 화면 폭(원본 픽셀 단위)</param>
/// <param name="LogicalHeight">게임이 그리는 화면 높이(원본 픽셀 단위)</param>
/// <param name="ViewportX">뷰포트 왼쪽 위 x (창 픽셀)</param>
/// <param name="ViewportY">뷰포트 왼쪽 위 y (창 픽셀)</param>
/// <param name="ViewportWidth">뷰포트 폭 (창 픽셀)</param>
/// <param name="ViewportHeight">뷰포트 높이 (창 픽셀)</param>
public readonly record struct ScreenLayout(
    int LogicalWidth, int LogicalHeight, int ViewportX, int ViewportY, int ViewportWidth, int ViewportHeight)
{
    /// <summary>논리 픽셀 하나가 창에서 차지하는 세로 배율 (1 = 원본과 같은 크기)</summary>
    public double Scale => (double)ViewportHeight / LogicalHeight;

    /// <summary>배율이 정수이면 점 샘플링으로 또렷하게, 아니면 선형 보간으로 늘려도 된다</summary>
    public bool IsIntegerScale => ViewportWidth % LogicalWidth == 0 && ViewportHeight % LogicalHeight == 0;

    /// <summary>창 픽셀 좌표를 논리 화면 좌표로 옮긴다 (뷰포트 밖이면 논리 화면 밖의 값이 나온다).</summary>
    /// <param name="windowX">창 클라이언트 x</param>
    /// <param name="windowY">창 클라이언트 y</param>
    public (double X, double Y) ToLogical(double windowX, double windowY) =>
        ((windowX - ViewportX) * LogicalWidth / ViewportWidth, (windowY - ViewportY) * LogicalHeight / ViewportHeight);

    /// <summary>논리 화면 좌표를 창 픽셀 좌표로 옮긴다.</summary>
    /// <param name="logicalX">논리 화면 x</param>
    /// <param name="logicalY">논리 화면 y</param>
    public (double X, double Y) ToWindow(double logicalX, double logicalY) =>
        (ViewportX + logicalX * ViewportWidth / LogicalWidth, ViewportY + logicalY * ViewportHeight / LogicalHeight);

    /// <summary>창 픽셀 좌표가 게임 화면(뷰포트) 안인지 알려 준다 (검은 여백이면 false)</summary>
    /// <param name="windowX">창 클라이언트 x</param>
    /// <param name="windowY">창 클라이언트 y</param>
    public bool ContainsWindowPoint(int windowX, int windowY) =>
        windowX >= ViewportX && windowX < ViewportX + ViewportWidth &&
        windowY >= ViewportY && windowY < ViewportY + ViewportHeight;
}

/// <summary>창 크기·해상도 설정·와이드 모드에서 <see cref="ScreenLayout"/> 을 계산한다 (MonoGame 비의존).</summary>
public static class ScreenLayoutCalculator
{
    /// <summary>지원하는 가장 좁은 화면비 = 4:3 (원본 해상도 전부)</summary>
    public const double NarrowestAspect = 4.0 / 3.0;

    /// <summary>지원하는 가장 넓은 화면비 = 16:9. 더 넓은 모니터는 좌우에 검은 여백을 둔다.</summary>
    public const double WidestAspect = 16.0 / 9.0;

    /// <summary>기본 논리 높이 = 원본 최대 해상도 1024×768 의 높이</summary>
    public const int DefaultViewHeight = 768;

    /// <summary>원본 Options → Resolution 메뉴의 세 해상도(640×480, 800×600, 1024×768)의 논리 높이</summary>
    public static IReadOnlyList<int> ViewHeights { get; } = [480, 600, 768];

    /// <summary>옵션의 원본·와이드 렌더링 높이. F9의 원본 세 단계와 분리한다.</summary>
    public static IReadOnlyList<int> RenderHeights { get; } = [480, 600, 720, 768, 800, 900, 1080, 1200];

    /// <summary>화면비를 판정할 때 허용하는 오차 (1600×1200 처럼 딱 떨어지지 않는 해상도 대비)</summary>
    private const double AspectTolerance = 0.02;

    /// <summary>
    /// 창 크기에서 화면 배치를 계산한다.
    /// <list type="bullet">
    /// <item><description><see cref="WideScreenMode.Letterbox"/>: 논리 화면은 항상 4:3 이고 창 안에 가운데로 맞춘다.</description></item>
    /// <item><description><see cref="WideScreenMode.Extend"/>: 4:3~16:9 창은 창 전체를 쓰고 논리 폭을 화면비에 맞춰 늘린다
    /// (같은 배율에서 맵을 옆으로 더 보여 준다). 범위 밖 화면비는 4:3 또는 16:9 로 제한하고 여백을 둔다.</description></item>
    /// </list>
    /// </summary>
    /// <param name="windowWidth">창(또는 전체화면) 클라이언트 폭, 픽셀</param>
    /// <param name="windowHeight">창 클라이언트 높이, 픽셀</param>
    /// <param name="viewHeight">논리 높이(원본 해상도의 높이 480·600·768 중 하나)</param>
    /// <param name="mode">와이드 화면 처리 방식</param>
    public static ScreenLayout Compute(int windowWidth, int windowHeight, int viewHeight, WideScreenMode mode)
    {
        // 최소화 등으로 크기가 0 이 되어도 0 으로 나누지 않도록 1 이상으로 맞춘다.
        int width = Math.Max(1, windowWidth);
        int height = Math.Max(1, windowHeight);
        int logicalHeight = Math.Max(1, viewHeight);
        if (mode == WideScreenMode.Letterbox)
        {
            // 4:3 논리 화면(640×480 등)을 창 안에 가운데로 맞춘다.
            var (letterX, letterY, letterW, letterH) = Fit(width, height, NarrowestAspect);
            int classicWidth = (int)Math.Round(logicalHeight * NarrowestAspect);
            return new ScreenLayout(classicWidth, logicalHeight, letterX, letterY, letterW, letterH);
        }
        double windowAspect = (double)width / height;
        // 지원 범위 안이면 창 전체, 밖이면 가까운 한계 화면비로 맞춘 사각형을 쓴다.
        var (x, y, w, h) = windowAspect >= NarrowestAspect && windowAspect <= WidestAspect
            ? (0, 0, width, height)
            : Fit(width, height, Math.Clamp(windowAspect, NarrowestAspect, WidestAspect));
        // 논리 높이를 고정하고 폭을 뷰포트 화면비에 맞춘다 (배율 = 뷰포트 높이 / 논리 높이).
        int logicalWidth = Math.Max(1, (int)Math.Round((double)w * logicalHeight / h));
        return new ScreenLayout(logicalWidth, logicalHeight, x, y, w, h);
    }

    /// <summary>창 안에 주어진 화면비의 가장 큰 사각형을 가운데로 맞춰 돌려준다.</summary>
    /// <param name="width">창 폭</param>
    /// <param name="height">창 높이</param>
    /// <param name="aspect">폭/높이 화면비</param>
    private static (int X, int Y, int Width, int Height) Fit(int width, int height, double aspect)
    {
        // 창이 목표보다 넓으면 높이를 기준으로, 아니면 폭을 기준으로 맞춘다.
        int fitWidth = (double)width / height > aspect ? (int)Math.Round(height * aspect) : width;
        int fitHeight = (double)width / height > aspect ? height : (int)Math.Round(width / aspect);
        return ((width - fitWidth) / 2, (height - fitHeight) / 2, fitWidth, fitHeight);
    }

    /// <summary>화면비를 사람이 읽는 이름("4:3", "16:10", "16:9")으로 바꾼다. 지원 목록에 없으면 소수 화면비를 쓴다.</summary>
    /// <param name="width">폭</param>
    /// <param name="height">높이</param>
    public static string DescribeAspect(int width, int height)
    {
        double aspect = (double)Math.Max(1, width) / Math.Max(1, height);
        // 지원하는 세 화면비 중 오차 안에 드는 것을 찾는다.
        foreach ((string name, double value) in new[] { ("4:3", 4.0 / 3.0), ("16:10", 16.0 / 10.0), ("16:9", 16.0 / 9.0) })
        {
            if (Math.Abs(aspect - value) <= AspectTolerance)
            {
                return name;
            }
        }
        return $"{aspect:0.00}:1";
    }
}
