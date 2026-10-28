namespace Netstorm.Client.Display;

/// <summary>
/// 실제 VFX mapped RLE의 소스 색 치환을 기존 보호 셰이프 저장소에 연결한다.
/// 근거: cpppj/src/client/VFXMappedDraw.cpp 의 DrawMappedShape (원본 0040229f). (R2b, 2026-10-28)
/// </summary>
public static class VfxMappedDraw
{
    /// <summary>
    /// 불투명 소스 색만 256바이트 표에서 치환한다. 색 0·255도 불투명 픽셀이면 치환한다. 배경은 치환하지 않는다.
    /// 반환값은 <see cref="VfxDraw.DrawShape"/>와 같은 0/-1/-2/-3/-4다.
    /// </summary>
    /// <param name="destination">그리기 화면.</param>
    /// <param name="width">화면 폭.</param>
    /// <param name="height">화면 높이.</param>
    /// <param name="database">셰이프 저장소.</param>
    /// <param name="block">블록 번호.</param>
    /// <param name="frame">프레임 번호.</param>
    /// <param name="x">그리기 기준 x.</param>
    /// <param name="y">그리기 기준 y.</param>
    /// <param name="pane">원본 pane(양 끝 포함).</param>
    /// <param name="colors">소스 색 치환표 256바이트.</param>
    public static int DrawMappedShape(Span<byte> destination, int width, int height, ShapeDatabase database,
        int block, int frame, int x, int y, ShapeRect pane, byte[] colors)
    {
        ArgumentNullException.ThrowIfNull(database);
        ArgumentNullException.ThrowIfNull(colors);
        if (colors.Length != 256)
        {
            throw new ArgumentException("mapped 색 치환표는 256바이트여야 합니다", nameof(colors));
        }
        if (width <= 0 || height <= 0)
        {
            return -1;
        }
        if ((long)width * height > destination.Length)
        {
            throw new ArgumentException("mapped RLE 출력 버퍼가 작습니다", nameof(destination));
        }
        int left = Math.Max(pane.Left, 0);
        int top = Math.Max(pane.Top, 0);
        int right = Math.Min(pane.Right, width - 1);
        int bottom = Math.Min(pane.Bottom, height - 1);
        if (right < left || bottom < top)
        {
            return -2;
        }
        if (block < 0 || block >= database.Blocks().Count)
        {
            throw new ArgumentOutOfRangeException(nameof(block), block, "mapped RLE 블록 범위 오류");
        }
        ShapeFrame record = database.Blocks()[block].Frames[frame];
        if (record.Rect.Right < record.Rect.Left || record.Rect.Bottom < record.Rect.Top)
        {
            return -4;
        }
        long originX = (long)x + pane.Left;
        long originY = (long)y + pane.Top;
        long frameLeft = originX + record.Rect.Left;
        long frameTop = originY + record.Rect.Top;
        long frameRight = originX + record.Rect.Right;
        long frameBottom = originY + record.Rect.Bottom;
        if (frameRight < left || frameBottom < top || frameLeft > right || frameTop > bottom)
        {
            return -3;
        }
        IndexedImage image = database.Decode(block, frame);
        // 실제로 겹치는 행만 처리하며 다른 배경 픽셀을 유지한다
        for (long row = Math.Max(frameTop, top); row <= Math.Min(frameBottom, bottom); row++)
        {
            // 투명 건너뛰기는 색 번호 0/255와 분리한다
            for (long column = Math.Max(frameLeft, left); column <= Math.Min(frameRight, right); column++)
            {
                int input = (int)(row - frameTop) * image.Width + (int)(column - frameLeft);
                if (image.Opacity[input] != 0)
                {
                    destination[(int)row * width + (int)column] = colors[image.Indices[input]];
                }
            }
        }
        return 0;
    }
}
