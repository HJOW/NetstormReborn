using Netstorm.Client.Display;

namespace Netstorm.Client.Host;

/// <summary>
/// 8비트 논리 화면의 변경 사각형을 RGBA 바이트로 바꾼다. MonoGame 호스트가 텍스처에 올리기 전의 순수 변환이다.
/// 원본 GDI BitBlt 자리에 인덱스를 현재 팔레트로 푼다. 알파는 항상 255(불투명)다.
/// 근거: cpppj/src/client/Screen.cpp 의 004a1800 창 복사. (R1-2a, 2026-10-28)
/// </summary>
public static class ScreenTextureConverter
{
    /// <summary>RGBA 한 픽셀의 바이트 수.</summary>
    public const int BytesPerPixel = 4;

    /// <summary>
    /// 변경 사각형 안의 인덱스를 현재 팔레트의 RGB와 불투명 알파로 푼다. 행 간격은 화면의 값을 쓴다.
    /// 사각형은 화면 안으로 잘라야 하며(빈 사각형은 거부), 결과는 위→아래·왼쪽→오른쪽 RGBA 행이다.
    /// </summary>
    /// <param name="screen">변환할 논리 화면.</param>
    /// <param name="rect">화면 안으로 잘린 변경 사각형.</param>
    public static byte[] ConvertToRgba(Screen screen, ScreenRect rect)
    {
        ArgumentNullException.ThrowIfNull(screen);
        ScreenRect bounds = new(0, 0, screen.Width, screen.Height);
        ScreenRect clipped = ScreenGeometry.ClipScreenRect(rect, bounds);
        if (clipped.Right <= clipped.Left || clipped.Bottom <= clipped.Top
            || clipped.Left != rect.Left || clipped.Top != rect.Top
            || clipped.Right != rect.Right || clipped.Bottom != rect.Bottom)
        {
            throw new ArgumentOutOfRangeException(nameof(rect), "호스트 텍스처 변환은 화면 안의 비어 있지 않은 사각형이어야 합니다");
        }
        int width = rect.Right - rect.Left;
        int height = rect.Bottom - rect.Top;
        var output = new byte[(long)width * height * BytesPerPixel];
        ReadOnlySpan<byte> pixels = screen.Pixels;
        ReadOnlySpan<ScreenColor> palette = screen.Palette;
        // 인덱스→현재 팔레트 RGB→RGBA 순서로 푼다
        for (int y = 0; y < height; y++)
        {
            int source = (rect.Top + y) * screen.Pitch + rect.Left;
            int target = y * width * BytesPerPixel;
            for (int x = 0; x < width; x++)
            {
                ScreenColor color = palette[pixels[source + x]];
                output[target] = color.Red;
                output[target + 1] = color.Green;
                output[target + 2] = color.Blue;
                output[target + 3] = 255;
                target += BytesPerPixel;
            }
        }
        return output;
    }
}
