using Netstorm.Client.Display;

namespace Netstorm.Client.Host;

/// <summary>
/// 비율 유지 출력의 정수 최근접 사각형. 전체화면 borderless 의 검정 여백과 마우스 역변환이 같은 사각형을 쓴다.
/// <see cref="ScreenPresentation.FitScreenPresentation"/>이 비율 맞춤이라면, 이쪽은 정수 배율(1·2·3…)만 올려 화소를 뭉개지 않는다.
/// 근거: AGENTS.md 의 16:9·16:10·4:3 지원과 cpppj/src/client/ScreenPresentation.cpp. (R1-2a, 2026-10-28)
/// </summary>
public static class HostPresentation
{
    /// <summary>
    /// 논리 화면을 클라이언트 안에 정수 배율로 올린 출력 사각형을 돌려준다. 배율은 1 이상이며, 클라이언트가 논리보다 작아도 1은 유지한다.
    /// </summary>
    /// <param name="width">논리 폭.</param>
    /// <param name="height">논리 높이.</param>
    /// <param name="clientWidth">클라이언트 폭.</param>
    /// <param name="clientHeight">클라이언트 높이.</param>
    public static ScreenRect IntegerFitRect(int width, int height, int clientWidth, int clientHeight)
    {
        if (width <= 0 || height <= 0 || clientWidth <= 0 || clientHeight <= 0)
        {
            throw new ArgumentException("정수 맞춤 출력의 크기는 양수여야 합니다");
        }
        // 가로·세로 중 작은 쪽에 맞춘 가장 큰 정수 배율이다
        int scale = Math.Max(1, Math.Min(clientWidth / width, clientHeight / height));
        int fittedWidth = width * scale;
        int fittedHeight = height * scale;
        int left = (clientWidth - fittedWidth) / 2;
        int top = (clientHeight - fittedHeight) / 2;
        return new ScreenRect(left, top, left + fittedWidth, top + fittedHeight);
    }
}
