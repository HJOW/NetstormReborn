namespace Netstorm.Client.Display;

/// <summary>
/// 원본 Renderer의 8비트 합성 도우미 묶음.
/// 근거: cpppj/src/client/Renderer.cpp 의 DrawIndexedImage (원본 004dae60 계열의 화면 합성 자리). (R2d-1, 2026-10-28)
/// VFXTransform·SpriteDraw가 같은 합성 규칙을 쓰므로 R5의 전체 Renderer보다 먼저 옮긴다.
/// </summary>
public static class RendererDraw
{
    /// <summary>
    /// 압축을 푼 그림의 불투명 픽셀만 8비트 화면에 합성한다. 그림자는 대상 화면 색을 색 표로 변환한다.
    /// 근거: cpppj/src/client/Renderer.h 의 DrawIndexedImage. (R2d-1, 2026-10-28)
    /// </summary>
    /// <param name="destination">그리기 화면(행 간격 포함 8비트 버퍼).</param>
    /// <param name="pitch">한 행의 바이트 간격.</param>
    /// <param name="height">화면 세로 크기.</param>
    /// <param name="image">합성할 그림(팔레트 번호와 불투명 마스크).</param>
    /// <param name="left">그림 왼쪽의 화면 x.</param>
    /// <param name="top">그림 위쪽의 화면 y.</param>
    /// <param name="clip">출력을 제한하는 클리핑 영역(half-open).</param>
    /// <param name="colors">색 변환표 256바이트. 없으면 원본 번호를 그대로 쓴다.</param>
    /// <param name="shadow">참이면 소스 색 대신 대상 화면 색을 표에서 변환한다. 표가 없으면 거부한다.</param>
    public static void DrawIndexedImage(Span<byte> destination, int pitch, int height, IndexedImage image,
        int left, int top, ScreenRect clip, byte[]? colors = null, bool shadow = false)
    {
        ArgumentNullException.ThrowIfNull(image);
        if (colors is not null && colors.Length != 256)
        {
            throw new ArgumentException("색 변환표는 256바이트여야 합니다", nameof(colors));
        }
        if (pitch <= 0 || height <= 0 || (long)pitch * height > destination.Length)
        {
            throw new ArgumentException("렌더러 표면이 작습니다", nameof(destination));
        }
        if (image.Width < 0 || image.Height < 0 ||
            (long)image.Width * image.Height != image.Indices.Length ||
            image.Opacity.Length != image.Indices.Length)
        {
            throw new ArgumentException("렌더러 그림 크기가 맞지 않습니다", nameof(image));
        }
        if (shadow && colors is null)
        {
            throw new ArgumentException("그림자는 대상 화면 색 변환표가 필요합니다", nameof(colors));
        }
        ScreenRect visible = ScreenGeometry.ClipScreenRect(clip, new ScreenRect(0, 0, pitch, height));
        // 잘린 실제 그림 영역만 순회한다
        for (long row = Math.Max(top, visible.Top);
            row < Math.Min((long)top + image.Height, visible.Bottom);
            row++)
        {
            // 한 행의 겹치는 구간만 처리하며 투명 픽셀은 기존 화면 값을 유지한다
            for (long column = Math.Max(left, visible.Left);
                column < Math.Min((long)left + image.Width, visible.Right);
                column++)
            {
                long input = (row - top) * image.Width + (column - left);
                if (image.Opacity[input] != 0)
                {
                    int output = (int)row * pitch + (int)column;
                    destination[output] = colors is null
                        ? image.Indices[input]
                        : colors[shadow ? destination[output] : image.Indices[input]];
                }
            }
        }
    }
}
