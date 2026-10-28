namespace Netstorm.Client.Display;

/// <summary>
/// 화면 모드 검사와 사각형 자르기, 창 위치 계산처럼 상태가 없는 원본 Screen.cpp 의 자유 함수 모음.
/// 근거: 원본 004a13d0(모드 검사), 00445290(사각형 자르기), 004a4fe0(창 위치). cpppj/src/client/Screen.cpp. (2026-10-28 R1에서 이식)
/// </summary>
public static class ScreenGeometry
{
    /// <summary>
    /// 화면 모드 조합이 허용되는지 검사한다. 원본은 맞지 않는 조건마다 "Illegal screen mode: …" 로그를 남기고 개수를 세며, 하나라도 있으면 거부한다.
    /// 사용: 모드를 정하기 전에 호출한다. 이 클론의 현대 경로는 <c>ModernScreenMode</c>로 정규화한 값을 directDrawInstalled=true로 검사한다.
    /// </summary>
    /// <param name="flags"><see cref="ScreenMode"/> 비트의 조합.</param>
    /// <param name="directDrawInstalled">DirectDraw가 설치돼 있는지.</param>
    public static bool ScreenModeIsLegal(uint flags, bool directDrawInstalled)
    {
        int problems = 0;
        if (!directDrawInstalled && (flags & ScreenMode.FullScreen) != 0) problems++;                       // DirectDraw without being installed
        if ((flags & ScreenMode.Flipping) != 0 && (flags & ScreenMode.FullScreen) == 0) problems++;       // flipping without DirectDraw
        if ((flags & ScreenMode.Parallax) != 0 && (flags & ScreenMode.Clouds) == 0) problems++;           // parallax without clouds
        if ((flags & ScreenMode.Animating) != 0 && (flags & ScreenMode.Clouds) == 0) problems++;          // animating without clouds
        if ((flags & (ScreenMode.FullScreen | ScreenMode.BackBufferDib)) == 0) problems++;                // no DirectDraw and no DIB
        return problems == 0;
    }

    /// <summary>원본 FUN_00445290 의 좌표 하나: 값을 [low, high] 안으로 넣는다. low가 high보다 크면 low가 이긴다(뒤집힌 경계를 바로잡지 않는다).</summary>
    private static int Clamp(int value, int low, int high)
    {
        int limited = Math.Min(value, high);
        return low > limited ? low : limited;
    }

    /// <summary>사각형의 네 좌표를 각각 경계 안으로 넣는다. 뒤집힌 사각형을 바로잡지는 않으며, 완전히 밖이면 폭·높이가 0인 사각형이 된다.</summary>
    /// <param name="rect">자를 사각형.</param>
    /// <param name="bounds">경계 사각형.</param>
    public static ScreenRect ClipScreenRect(ScreenRect rect, ScreenRect bounds) =>
        new(Clamp(rect.Left, bounds.Left, bounds.Right), Clamp(rect.Top, bounds.Top, bounds.Bottom),
            Clamp(rect.Right, bounds.Left, bounds.Right), Clamp(rect.Bottom, bounds.Top, bounds.Bottom));

    /// <summary>
    /// 원본 004a4fe0 의 창 위치 계산(설정 initWindowPos): 1 왼쪽 위, 2 오른쪽 위, 3 오른쪽 아래, 4 왼쪽 아래, 5 가운데. 그 밖의 값이면 지금 위치를 그대로 둔다.
    /// 창이 바탕 화면보다 크면 음수가 될 수 있으며 원본처럼 고치지 않는다.
    /// </summary>
    /// <param name="initWindowPos">설정 값.</param>
    /// <param name="desktopWidth">바탕 화면 폭.</param>
    /// <param name="desktopHeight">바탕 화면 높이.</param>
    /// <param name="windowWidth">창 폭.</param>
    /// <param name="windowHeight">창 높이.</param>
    /// <param name="current">현재 창 위치.</param>
    public static ScreenPoint InitialWindowPosition(int initWindowPos, int desktopWidth, int desktopHeight,
        int windowWidth, int windowHeight, ScreenPoint current) => initWindowPos switch
        {
            1 => new ScreenPoint(0, 0),
            2 => new ScreenPoint(desktopWidth - windowWidth, 0),
            3 => new ScreenPoint(desktopWidth - windowWidth, desktopHeight - windowHeight),
            4 => new ScreenPoint(0, desktopHeight - windowHeight),
            5 => new ScreenPoint((desktopWidth - windowWidth) / 2, (desktopHeight - windowHeight) / 2),
            _ => current,
        };
}
