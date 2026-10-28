namespace Netstorm.Client.Display;

/// <summary>
/// 논리 화면 캡처. 8비트 인덱스 버퍼를 현재 팔레트로 RGBA로 바꾸고 cpppj와 byte 단위로 같은 32비트 BMP(122바이트 BITMAPV4 머리, 위→아래 행, BGRA, sRGB)로 쓴다.
/// 같은 입력의 같은 화면이면 두 구현의 캡처 파일이 완전히 같으므로 파일 해시만으로 L4a(픽셀 차이 0) 판정을 할 수 있다.
/// 이 캡처는 논리 화면이며 OS 클라이언트에 실제 출력된 확대·레터박스 화면(cpppj의 snapshot-output)과 구별한다.
/// 근거: cpppj/src/client/ClientMain.cpp 의 Client::Capture, cpppj/src/platform/Bitmap.cpp 의 WriteBitmap. (2026-10-28 R1-2에서 이식)
/// </summary>
public static class ScreenCapture
{
    /// <summary>BMP 머리 전체 크기: 파일 머리 14바이트 + BITMAPV4HEADER 108바이트.</summary>
    public const int BitmapHeaderSize = 122;

    /// <summary>
    /// 화면의 인덱스를 현재 표시 팔레트로 바꿔 RGBA(행마다 R,G,B,A=255) 바이트 열을 만든다. 행 간격 padding은 제외한 폭×높이만 담는다.
    /// 원본 Client::Capture와 같이 잠그고 읽은 뒤 푼다.
    /// </summary>
    /// <param name="screen">캡처할 논리 화면.</param>
    public static byte[] ToRgba(Screen screen)
    {
        var rgba = new byte[(long)screen.Width * screen.Height * 4];
        byte[] buffer = screen.Lock();
        try
        {
            ReadOnlySpan<ScreenColor> palette = screen.Palette;
            // 줄마다 팔레트 번호를 RGBA로 바꾼다.
            for (int y = 0; y < screen.Height; y++)
            {
                // 한 줄의 픽셀.
                for (int x = 0; x < screen.Width; x++)
                {
                    ScreenColor color = palette[buffer[y * screen.Pitch + x]];
                    int output = (y * screen.Width + x) * 4;
                    rgba[output] = color.Red;
                    rgba[output + 1] = color.Green;
                    rgba[output + 2] = color.Blue;
                    rgba[output + 3] = 255;
                }
            }
        }
        finally { screen.Unlock(); }
        return rgba;
    }

    /// <summary>리틀 엔디언 32비트 값을 쓴다.</summary>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        bytes[offset] = (byte)value;
        bytes[offset + 1] = (byte)(value >> 8);
        bytes[offset + 2] = (byte)(value >> 16);
        bytes[offset + 3] = (byte)(value >> 24);
    }

    /// <summary>리틀 엔디언 16비트 값을 쓴다.</summary>
    private static void Put16(byte[] bytes, int offset, ushort value)
    {
        bytes[offset] = (byte)value;
        bytes[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>
    /// RGBA 바이트 열을 cpppj WriteBitmap과 같은 BMP 바이트로 만든다: "BM", 크기, 데이터 시작 122, V4 머리 108, 폭, 음수 높이(위→아래), 평면 1, 32비트,
    /// BI_BITFIELDS(3), 마스크 R=00ff0000 G=0000ff00 B=000000ff A=ff000000, 색 공간 sRGB('sRGB'). 크기가 맞지 않으면 <see cref="ArgumentException"/>이다.
    /// </summary>
    /// <param name="width">폭(양수).</param>
    /// <param name="height">높이(양수).</param>
    /// <param name="rgba">폭×높이×4바이트의 RGBA.</param>
    public static byte[] EncodeBitmap(int width, int height, ReadOnlySpan<byte> rgba)
    {
        long pixelSize = (long)width * height * 4;
        if (width <= 0 || height <= 0 || pixelSize > uint.MaxValue - BitmapHeaderSize || pixelSize != rgba.Length)
            throw new ArgumentException("Invalid bitmap dimensions or pixel buffer");
        var bytes = new byte[BitmapHeaderSize + rgba.Length];
        bytes[0] = (byte)'B';
        bytes[1] = (byte)'M';
        Put(bytes, 2, (uint)bytes.Length);
        Put(bytes, 10, BitmapHeaderSize);
        Put(bytes, 14, 108);
        Put(bytes, 18, (uint)width);
        Put(bytes, 22, unchecked((uint)-height));
        Put16(bytes, 26, 1);
        Put16(bytes, 28, 32);
        Put(bytes, 30, 3);
        Put(bytes, 34, (uint)pixelSize);
        Put(bytes, 54, 0x00ff0000);
        Put(bytes, 58, 0x0000ff00);
        Put(bytes, 62, 0x000000ff);
        Put(bytes, 66, 0xff000000);
        Put(bytes, 70, 0x73524742); // 색 공간을 sRGB로 지정하여 외부 도구에서 색을 유지한다.
        // 팔레트를 적용한 RGBA를 BMP의 BGRA 순서로 변환한다.
        for (int i = 0; i < rgba.Length; i += 4)
        {
            bytes[BitmapHeaderSize + i] = rgba[i + 2];
            bytes[BitmapHeaderSize + i + 1] = rgba[i + 1];
            bytes[BitmapHeaderSize + i + 2] = rgba[i];
            bytes[BitmapHeaderSize + i + 3] = rgba[i + 3];
        }
        return bytes;
    }

    /// <summary>화면을 위 형식의 BMP 파일로 저장한다. 폴더가 없으면 만들지 않고 파일 쓰기 예외를 그대로 전달한다.</summary>
    /// <param name="path">저장할 경로.</param>
    /// <param name="screen">캡처할 논리 화면.</param>
    public static void WriteBitmap(string path, Screen screen) =>
        File.WriteAllBytes(path, EncodeBitmap(screen.Width, screen.Height, ToRgba(screen)));
}
