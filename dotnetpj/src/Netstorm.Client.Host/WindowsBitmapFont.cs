using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Client.Display;

namespace Netstorm.Client.Host; // Windows 전용 메모리 GDI 글꼴 생성 장치 영역이다.

/// <summary>원본 캐시가 없는 경우에만 같은 CreateFontA·ABC·8비트DIB로 글리프를 생성한다.
/// 근거: cpppj/src/client/BitmapFont.cpp 의 BitmapFont::Generate. Win32를 플랫폼 독립 Client와 분리하며 파일이나 창을 만들지 않는다.</summary>
public static class WindowsBitmapFont
{
    /// <summary>원본 안전 캔버스의 행 간격.</summary>
    private const int CanvasWidth = 512;
    /// <summary>원본 안전 캔버스 높이.</summary>
    private const int CanvasHeight = 256;
    /// <summary>원본 투명 바탕 색 번호.</summary>
    private const byte Background = 32;
    /// <summary>원본 글자 색 번호.</summary>
    private const byte Ink = 100;

    /// <summary>원본 GDI 클립 사각형. 네 signed LONG의 순서/크기를 유지한다.</summary>
    [StructLayout(LayoutKind.Sequential)]
    private struct Rect
    {
        /// <summary>왼쪽 포함 좌표.</summary>
        public int Left;
        /// <summary>위쪽 포함 좌표.</summary>
        public int Top;
        /// <summary>오른쪽 제외 좌표.</summary>
        public int Right;
        /// <summary>아래쪽 제외 좌표.</summary>
        public int Bottom;
    }

    /// <summary>메모리 참조 DC. 실제 화면/창은 생성하지 않는다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern nint CreateCompatibleDC(nint dc);
    /// <summary>원본 ANSI 코드 페이지의 실제 글꼴 생성.</summary>
    [DllImport("gdi32.dll", CharSet = CharSet.Ansi, ExactSpelling = true)] private static extern nint CreateFontA(int height, int width, int escapement, int orientation, int weight, uint italic, uint underline, uint strikeout, uint charset, uint outputPrecision, uint clipPrecision, uint quality, uint pitchAndFamily, string face);
    /// <summary>256색 top-down DIB와 픽셀 포인터를 만든다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern nint CreateDIBSection(nint dc, byte[] info, uint usage, out nint bits, nint section, uint offset);
    /// <summary>DC에 글꼴/비트맵을 선택하고 이전 객체를 반환한다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern nint SelectObject(nint dc, nint value);
    /// <summary>선택을 복구한 생성 객체를 해제한다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool DeleteObject(nint value);
    /// <summary>소유한 메모리 DC를 해제한다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool DeleteDC(nint dc);
    /// <summary>TEXTMETRICA 전체56바이트를 읽는다. 필요한 높이/상승/하강은 명시 DWORD로 해석한다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool GetTextMetricsA(nint dc, [Out] byte[] metrics);
    /// <summary>모든 ANSI 문자의 기본 폭.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool GetCharWidthA(nint dc, uint first, uint last, [Out] int[] widths);
    /// <summary>각 문자의 A/signed C·unsigned B를 DWORD 비트 그대로 읽는다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool GetCharABCWidthsA(nint dc, uint first, uint last, [Out] int[] abc);
    /// <summary>원본 왼쪽/위쪽 기준 정렬을 설치한다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern uint SetTextAlign(nint dc, uint align);
    /// <summary>불투명 바탕 모드.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern int SetBkMode(nint dc, int mode);
    /// <summary>회색조 팔레트의 바탕 RGB.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern uint SetBkColor(nint dc, uint color);
    /// <summary>회색조 팔레트의 글자 RGB.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern uint SetTextColor(nint dc, uint color);
    /// <summary>한 ANSI 바이트를 실제 원본 클립/불투명 옵션으로 그린다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool ExtTextOutA(nint dc, int x, int y, uint options, in Rect rect, byte[] text, uint count, nint spacing);
    /// <summary>GDI 쓰기를 픽셀 읽기 전에 완료한다.</summary>
    [DllImport("gdi32.dll", ExactSpelling = true)] private static extern bool GdiFlush();

    /// <summary>캐시/셰이프의 WORD/DWORD를 명시 리틀 엔디언으로 쓴다.</summary>
    private static void Put(List<byte> bytes, int offset, uint value, int size = 4)
    {
        // 정렬과 무관하게 원본 하위 바이트 순서로 쓴다.
        for (int i = 0; i < size; i++) bytes[offset + i] = unchecked((byte)(value >> (i * 8)));
    }

    /// <summary>바탕32가 아닌 최소 영역을 원본100색 투명 skip/반복 run으로 저장한다. 빈 글자도 특수 프레임 포인터를 가진다.</summary>
    private static byte[] EncodeGlyph(byte[] pixels, int width, int height)
    {
        int left = width, top = height, right = -1, bottom = -1;
        // 실제 글자의 최소 사각형을 행 순서로 찾는다.
        for (int y = 0; y < height; y++)
        {
            // 원본 DIB 행 간격 안의 요청 폭만 읽는다.
            for (int x = 0; x < width; x++) if (pixels[y * CanvasWidth + x] != Background) { left = Math.Min(left, x); top = Math.Min(top, y); right = Math.Max(right, x); bottom = Math.Max(bottom, y); }
        }
        var shape = new List<byte>(new byte[40]); Put(shape, 0, 0x30312e31); Put(shape, 4, 1); Put(shape, 8, 16); Put(shape, 16, (uint)(height - 1), 2); Put(shape, 18, (uint)(width - 1), 2);
        if (right < 0) { Put(shape, 24, 0x7fffffff); Put(shape, 28, 0x7fffffff); Put(shape, 32, 0x80000001); Put(shape, 36, 0x80000001); return shape.ToArray(); }
        Put(shape, 24, (uint)left); Put(shape, 28, (uint)top); Put(shape, 32, (uint)right); Put(shape, 36, (uint)bottom);
        // 압축 영역의 각 행을 원본 RLE 형식으로 기록한다.
        for (int y = top; y <= bottom; y++)
        {
            int x = left;
            // 같은 종류의 구간을127픽셀까지 묶는다.
            while (x <= right)
            {
                bool ink = pixels[y * CanvasWidth + x] != Background; int count = 1;
                // 다음 픽셀도 같은 투명/불투명 종류인지 검사한다.
                while (count < 127 && x + count <= right && (pixels[y * CanvasWidth + x + count] != Background) == ink) count++;
                shape.Add((byte)(ink ? count * 2 : 1)); shape.Add((byte)(ink ? Ink : count)); x += count;
            }
            shape.Add(0);
        }
        return shape.ToArray();
    }

    /// <summary>같은 Windows ANSI GDI로 원본 네 표/실제 VFX를 메모리에 생성한다. 성공/예외 모두 선택 객체와 소유 핸들을 정리한다.</summary>
    public static BitmapFont Generate(string face, int height, int weight, FontStyle style)
    {
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("원본 GDI 글꼴 생성은 Windows 호스트 경계다");
        if (height <= 0 || height > 128) throw new ArgumentException("생성 글꼴 높이 오류"); nint dc = 0, font = 0, bitmap = 0, oldFont = 0, oldBitmap = 0;
        try
        {
            dc = CreateCompatibleDC(0); font = CreateFontA(height, 0, 0, 0, weight, style == FontStyle.Italic ? 1U : 0, style == FontStyle.Underline ? 1U : 0, style == FontStyle.Strikeout ? 1U : 0, 0, 7, 0, 0, 32, face);
            if (dc == 0 || font == 0) throw new InvalidOperationException("글꼴 GDI 객체 생성 실패"); byte[] info = new byte[40 + 256 * 4];
            BinaryPrimitives.WriteInt32LittleEndian(info, 40); BinaryPrimitives.WriteInt32LittleEndian(info.AsSpan(4), CanvasWidth); BinaryPrimitives.WriteInt32LittleEndian(info.AsSpan(8), -CanvasHeight);
            BinaryPrimitives.WriteUInt16LittleEndian(info.AsSpan(12), 1); BinaryPrimitives.WriteUInt16LittleEndian(info.AsSpan(14), 8);
            // 색 번호와 PALETTEINDEX가 같도록 실제 회색조 표를 만든다.
            for (int i = 0; i < 256; i++) info[40 + i * 4] = info[41 + i * 4] = info[42 + i * 4] = (byte)i;
            bitmap = CreateDIBSection(dc, info, 0, out nint pointer, 0, 0); if (bitmap == 0 || pointer == 0) throw new InvalidOperationException("글꼴 DIB 생성 실패"); oldBitmap = SelectObject(dc, bitmap); oldFont = SelectObject(dc, font);
            byte[] metrics = new byte[56]; int[] widths = new int[256], abc = new int[256 * 3];
            if (!GetTextMetricsA(dc, metrics) || !GetCharWidthA(dc, 0, 255, widths)) throw new InvalidOperationException("글꼴 메트릭 읽기 실패");
            if (!GetCharABCWidthsA(dc, 0, 255, abc))
            {
                // 비트맵/비트루타입 글꼴은 원본처럼 A=C=0과 기본 폭B를 사용한다.
                for (int code = 0; code < 256; code++) { abc[code * 3] = abc[code * 3 + 2] = 0; abc[code * 3 + 1] = widths[code]; }
            }
            SetTextAlign(dc, 0); SetBkMode(dc, 2); SetBkColor(dc, Background * 0x010101U); SetTextColor(dc, Ink * 0x010101U);
            var cache = new List<byte>(new byte[BitmapFont.PayloadStart]);
            // 마지막 NUL까지 원본 서명을 복사한다.
            for (int i = 0; i < BitmapFont.Signature.Length; i++) cache[i] = BitmapFont.Signature[i];
            int actualHeight = BinaryPrimitives.ReadInt32LittleEndian(metrics); Put(cache, 16, 1); Put(cache, 32, 256); Put(cache, 20, unchecked((uint)actualHeight)); Put(cache, 24, BinaryPrimitives.ReadUInt32LittleEndian(metrics.AsSpan(4))); Put(cache, 28, BinaryPrimitives.ReadUInt32LittleEndian(metrics.AsSpan(8)));
            byte[] pixels = new byte[CanvasWidth * CanvasHeight], blank = Enumerable.Repeat(Background, pixels.Length).ToArray();
            // 한 바이트씩 실제 ANSI GDI로 출력하고 네 표와 독립 블록을 작성한다.
            for (int code = 0; code < 256; code++)
            {
                int width = unchecked(abc[code * 3 + 1] + 2); if (width <= 0 || width > CanvasWidth || actualHeight >= CanvasHeight) throw new InvalidOperationException("생성 글리프 DIB 범위 초과");
                Marshal.Copy(blank, 0, pointer, blank.Length); var rect = new Rect { Left = 0, Top = 0, Right = width - 1, Bottom = actualHeight + 1 };
                if (!ExtTextOutA(dc, unchecked(-abc[code * 3]), 0, 6, in rect, [(byte)code], 1, 0)) throw new InvalidOperationException("GDI 글리프 출력 실패"); GdiFlush(); Marshal.Copy(pointer, pixels, 0, pixels.Length);
                byte[] shape = EncodeGlyph(pixels, width, actualHeight + 1); uint advance = unchecked((uint)(abc[code * 3 + 1] + abc[code * 3 + 2]));
                Put(cache, BitmapFont.TableStart + code * 4, advance); Put(cache, BitmapFont.TableStart + BitmapFont.TableBytes + code * 4, unchecked((uint)abc[code * 3])); Put(cache, BitmapFont.TableStart + BitmapFont.TableBytes * 2 + code * 4, advance); Put(cache, BitmapFont.TableStart + BitmapFont.TableBytes * 3 + code * 4, (uint)shape.Length); cache.AddRange(shape);
            }
            return new(cache.ToArray());
        }
        finally
        {
            if (oldFont != 0) SelectObject(dc, oldFont); if (oldBitmap != 0) SelectObject(dc, oldBitmap); if (font != 0) DeleteObject(font); if (bitmap != 0) DeleteObject(bitmap); if (dc != 0) DeleteDC(dc);
        }
    }
}
