namespace Netstorm.Client.Display;

/// <summary>
/// 원본과 다른 Windows 10/11 전체화면 출력의 계산 경계. 원본 8비트 논리 화면의 크기와 인덱스를 바꾸지 않고, 현재 모니터에 비율대로 가운데 맞춰
/// 정수 픽셀 중심 최근접 확대로 내보내며 남는 영역을 검정으로 둔다. 장치별 확대 반올림에 맡기지 않아 같은 입력이 항상 같은 픽셀을 만든다.
/// 근거: cpppj/src/client/ScreenPresentation.cpp, 단위495 docs/exe/cpp-modern-fullscreen-20261027.md. (2026-10-28 R1에서 이식)
/// </summary>
public static class ScreenPresentation
{
    /// <summary>
    /// 원본 4a1ad0 DIB 경로의 겹친 행 이동. 양수 이동은 오른쪽/아래 픽셀을 왼쪽/위로 복사한다. 이동량이 영역을 넘거나 0이면 아무것도 하지 않는다.
    /// 화면 밖 읽기는 쓰기 전에 <see cref="ArgumentOutOfRangeException"/>으로 거부한다. 아래로 복사할 때는 뒤쪽 행부터 옮겨 겹친 원본을 보존한다.
    /// </summary>
    /// <param name="pixels">행 간격을 포함한 인덱스 버퍼.</param>
    /// <param name="pitch">행 간격(바이트).</param>
    /// <param name="width">논리 폭.</param>
    /// <param name="height">논리 높이.</param>
    /// <param name="x">이동 영역의 왼쪽.</param>
    /// <param name="y">이동 영역의 위쪽.</param>
    /// <param name="regionWidth">이동 영역의 폭.</param>
    /// <param name="regionHeight">이동 영역의 높이.</param>
    /// <param name="dx">가로 이동량.</param>
    /// <param name="dy">세로 이동량.</param>
    public static void ScrollScreenPixels(Span<byte> pixels, int pitch, int width, int height, int x, int y,
        int regionWidth, int regionHeight, int dx, int dy)
    {
        if (regionWidth <= 0 || regionHeight <= 0 || Math.Abs((long)dx) >= regionWidth || Math.Abs((long)dy) >= regionHeight || (dx == 0 && dy == 0)) return;
        if (width <= 0 || height <= 0 || pitch < width || x < 0 || y < 0 || (long)x + regionWidth > width || (long)y + regionHeight > height
            || pixels.Length < (long)pitch * height)
            throw new ArgumentOutOfRangeException(nameof(pixels), "DIB 화면 이동의 실제 픽셀 범위 오류");
        int sourceX = x + Math.Max(dx, 0), destinationX = x + Math.Max(-dx, 0);
        int rows = regionHeight - (int)Math.Abs((long)dy), columns = regionWidth - (int)Math.Abs((long)dx);
        // 이동할 행마다 한 줄씩 옮긴다. 아래로 복사할 때는 뒤쪽 행부터 옮긴다.
        for (int index = 0; index < rows; index++)
        {
            int row = dy > 0 ? index : rows - 1 - index;
            int sourceY = y + Math.Max(dy, 0) + row, destinationY = y + Math.Max(-dy, 0) + row;
            // Span.CopyTo는 겹친 구간도 memmove처럼 안전하게 복사한다.
            pixels.Slice(sourceY * pitch + sourceX, columns).CopyTo(pixels.Slice(destinationY * pitch + destinationX, columns));
        }
    }

    /// <summary>
    /// 원본과 다른 Windows 10/11 경로의 실제 DIB/비flip 모드로 정규화한다. 전체화면은 DIB를 켜고 flip을 끈다. 창 모드는 원본 00435ba0 의 정리(소프트웨어 마우스·flip·시차 끔)를
    /// 유지하고 DIB를 켠다. 모드 설정 비트는 항상 켠다.
    /// </summary>
    /// <param name="requested">요청한 <see cref="ScreenMode"/> 비트.</param>
    public static uint ModernScreenMode(uint requested)
    {
        requested |= ScreenMode.ModeSet;
        if ((requested & ScreenMode.FullScreen) != 0) requested = (requested | ScreenMode.BackBufferDib) & ~ScreenMode.Flipping;
        else requested = (requested & ~(ScreenMode.SoftwareMouse | ScreenMode.Flipping | ScreenMode.Parallax)) | ScreenMode.BackBufferDib;
        return requested;
    }

    /// <summary>
    /// 논리 크기를 바꾸지 않고 현재 클라이언트 영역에 비율대로 가운데 표시할 출력 사각형을 구한다. 4:3·16:9·16:10 등의 비율을 유지하며 늘어나지 않는다.
    /// 양수가 아닌 크기는 <see cref="ArgumentException"/>이다.
    /// </summary>
    /// <param name="width">논리 폭.</param>
    /// <param name="height">논리 높이.</param>
    /// <param name="clientWidth">클라이언트 폭.</param>
    /// <param name="clientHeight">클라이언트 높이.</param>
    public static ScreenRect FitScreenPresentation(int width, int height, int clientWidth, int clientHeight)
    {
        if (width <= 0 || height <= 0 || clientWidth <= 0 || clientHeight <= 0) throw new ArgumentException("전체화면 출력의 양수 크기 필요");
        int fittedWidth = clientWidth, fittedHeight = clientHeight;
        if ((long)clientWidth * height <= (long)clientHeight * width) fittedHeight = Math.Max(1, (int)((long)clientWidth * height / width));
        else fittedWidth = Math.Max(1, (int)((long)clientHeight * width / height));
        int left = (clientWidth - fittedWidth) / 2, top = (clientHeight - fittedHeight) / 2;
        return new ScreenRect(left, top, left + fittedWidth, top + fittedHeight);
    }

    /// <summary>
    /// 호환 출력의 실제 마우스 위치를 같은 논리 화면에 되돌리고 원본 Poll의 끝 포함 범위 [0,width]×[0,height]로 제한한다.
    /// 레터박스나 음수 위치도 가장자리로 모인다.
    /// </summary>
    /// <param name="point">클라이언트 좌표의 마우스 위치.</param>
    /// <param name="output"><see cref="FitScreenPresentation"/>이 돌려준 출력 사각형.</param>
    /// <param name="width">논리 폭.</param>
    /// <param name="height">논리 높이.</param>
    public static ScreenPoint MapScreenPresentationPoint(ScreenPoint point, ScreenRect output, int width, int height)
    {
        if (width <= 0 || height <= 0 || output.Right <= output.Left || output.Bottom <= output.Top)
            throw new ArgumentException("전체화면 입력 변환의 실제 출력 범위 필요");
        long x = ((long)point.X - output.Left) * width / ((long)output.Right - output.Left);
        long y = ((long)point.Y - output.Top) * height / ((long)output.Bottom - output.Top);
        return new ScreenPoint((int)Math.Clamp(x, 0, width), (int)Math.Clamp(y, 0, height));
    }

    /// <summary>
    /// 논리 화면의 변경 사각형을 출력 좌표로 옮긴다. 전체 표면과 같은 비율이지만 시작은 내림, 끝은 올림으로 바깥쪽 반올림해 확대 경계의 잔상을 막는다.
    /// </summary>
    /// <param name="rect">논리 화면의 변경 사각형.</param>
    /// <param name="output">출력 사각형.</param>
    /// <param name="width">논리 폭.</param>
    /// <param name="height">논리 높이.</param>
    public static ScreenRect MapScreenPresentationRect(ScreenRect rect, ScreenRect output, int width, int height)
    {
        if (width <= 0 || height <= 0 || output.Right <= output.Left || output.Bottom <= output.Top)
            throw new ArgumentException("전체화면 dirty 변환의 실제 출력 범위 필요");
        rect = ScreenGeometry.ClipScreenRect(rect, new ScreenRect(0, 0, width, height));
        long x = (long)output.Right - output.Left, y = (long)output.Bottom - output.Top;
        return new ScreenRect(
            output.Left + (int)((long)rect.Left * x / width),
            output.Top + (int)((long)rect.Top * y / height),
            output.Left + (int)(((long)rect.Right * x + width - 1) / width),
            output.Top + (int)(((long)rect.Bottom * y + height - 1) / height));
    }

    /// <summary>
    /// 8비트 원본 번호만 복사하는 정수 픽셀 중심 최근접 확대·축소. 양수 크기와 실제 행 범위를 검사하며 목적지 padding과 화면비는 바꾸지 않는다.
    /// 크기·행 간격이 틀리면 <see cref="ArgumentException"/>, 버퍼가 모자라면 <see cref="ArgumentOutOfRangeException"/>이다.
    /// </summary>
    /// <param name="source">원본 인덱스 버퍼.</param>
    /// <param name="sourcePitch">원본 행 간격.</param>
    /// <param name="sourceWidth">원본 폭.</param>
    /// <param name="sourceHeight">원본 높이.</param>
    /// <param name="destination">출력 인덱스 버퍼.</param>
    /// <param name="destinationPitch">출력 행 간격.</param>
    /// <param name="destinationWidth">출력 폭.</param>
    /// <param name="destinationHeight">출력 높이.</param>
    public static void ScaleScreenPresentation(ReadOnlySpan<byte> source, int sourcePitch, int sourceWidth, int sourceHeight,
        Span<byte> destination, int destinationPitch, int destinationWidth, int destinationHeight)
    {
        if (sourceWidth <= 0 || sourceHeight <= 0 || destinationWidth <= 0 || destinationHeight <= 0 || sourcePitch < sourceWidth || destinationPitch < destinationWidth)
            throw new ArgumentException("전체화면 확대의 양수 크기/행 간격 필요");
        if (source.Length < (long)(sourceHeight - 1) * sourcePitch + sourceWidth || destination.Length < (long)(destinationHeight - 1) * destinationPitch + destinationWidth)
            throw new ArgumentOutOfRangeException(nameof(source), "전체화면 확대의 실제 픽셀 범위 오류");
        var columns = new int[destinationWidth];
        // 각 열의 최근접 선택을 한 번만 계산해 둔다.
        for (int x = 0; x < destinationWidth; x++) columns[x] = (int)((2L * x + 1) * sourceWidth / (2L * destinationWidth));
        // 출력 행마다 원본의 한 행을 골라 열 표로 복사한다.
        for (int y = 0; y < destinationHeight; y++)
        {
            int row = (int)((2L * y + 1) * sourceHeight / (2L * destinationHeight)) * sourcePitch;
            int offset = y * destinationPitch;
            // padding과 화면비를 바꾸지 않고 8비트 원본 번호만 복사한다.
            for (int x = 0; x < destinationWidth; x++) destination[offset + x] = source[row + columns[x]];
        }
    }
}
