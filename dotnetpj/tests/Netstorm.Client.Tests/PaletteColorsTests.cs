using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 색 표의 보호 계약을 확인한다. 원본 기대값 재생(X86PaletteColorsTests)이 파일 로더·색 표를 대조하고,
/// 이 검사들은 일시 효과의 표 보존·예약 바이트·판본별 개수 계약을 옮긴다.
/// 근거: cpppj/tests/PaletteColorsTests.cpp. (R2a, 2026-10-28)
/// </summary>
public sealed class PaletteColorsTests
{
    /// <summary>두 색 표의 기본·날씨·이름 목록이 모두 같은지 비교한다.</summary>
    private static bool SameTable(PaletteColorTable a, PaletteColorTable b)
    {
        if (!a.Basic.SequenceEqual(b.Basic) || !a.Weather.SequenceEqual(b.Weather) || a.Named.Count != b.Named.Count)
        {
            return false;
        }
        // 별도 원본 전역에 저장한 중복 RGB 항목도 개별 비교한다
        return a.Named.SequenceEqual(b.Named);
    }

    /// <summary>번개·일시 적용은 파일 로더의 색 표를 갱신하지 않는다. 새 파일을 읽을 때만 새 RGB로 계산한다.</summary>
    [Fact]
    public void TemporaryFlashKeepsFileTable()
    {
        var screen = new Screen(640, 480);
        screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
        var bytes = new byte[1024];
        // 초기 팔레트의 각 번호를 서로 다른 회색으로 채운다
        for (int i = 0; i < 256; i++)
        {
            bytes[i * 4] = bytes[i * 4 + 1] = bytes[i * 4 + 2] = (byte)i;
            bytes[i * 4 + 3] = 91;
        }
        screen.LoadPalette(new GamePalette(bytes));
        PaletteColorTable original = screen.Colors();
        var flash = new ScreenColor(255, 255, 255, 47);
        screen.SetPalette(9, 1, [flash], true);
        Assert.True(SameTable(screen.Colors(), original));
        screen.SetPalette(0, 256, null, true);
        Assert.True(SameTable(screen.Colors(), original));
        Array.Fill(bytes, (byte)19);
        screen.LoadPalette(new GamePalette(bytes));
        Assert.False(SameTable(screen.Colors(), original));
        Assert.Equal(screen.Colors().Weather, screen.WeatherTints());
    }

    /// <summary>RGB COL은 이전 BGRX의 예약 바이트를 보존하고 두 끝점은 SetPalette에서 항상 0으로 바뀐다.</summary>
    [Fact]
    public void ColPreservesPreviousReservedBytes()
    {
        var screen = new Screen(640, 480);
        screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
        var previous = new byte[1024];
        var col = new byte[0x308];
        // 앞 파일의 예약 값과 뒤 COL의 채널을 서로 구별되는 값으로 만든다
        for (int i = 0; i < 256; i++)
        {
            previous[i * 4 + 3] = (byte)i;
            col[8 + i * 3] = 30;
            col[9 + i * 3] = 50;
            col[10 + i * 3] = 70;
        }
        var bgrx = new GamePalette(previous);
        var rgb = new GamePalette(col);
        Assert.Equal((byte?)43, bgrx.Reserved(43));
        Assert.Null(rgb.Reserved(43));
        screen.LoadPalette(bgrx);
        screen.LoadPalette(rgb);
        // 전체 내부 색의 RGB가 바뀌어도 예약값은 앞 파일에 남아 있는 값이다
        for (int i = 1; i < 255; i++)
        {
            ScreenColor color = screen.Palette[i];
            Assert.Equal(30, color.Red);
            Assert.Equal(50, color.Green);
            Assert.Equal(70, color.Blue);
            Assert.Equal((byte)i, color.Reserved);
        }
        Assert.Equal(0, screen.Palette[0].Reserved);
        Assert.Equal(0, screen.Palette[255].Reserved);
    }

    /// <summary>해상도 변경의 현재 BGRX 복사는 예약 바이트와 선택 판본의 전체 색 표를 보존한다.</summary>
    [Fact]
    public void ResolutionCopyRetainsTableAndEdition()
    {
        var first = new Screen(640, 480);
        first.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
        var bytes = new byte[1024];
        // 각 색의 네 바이트를 전부 다르게 채워 RGB만 복사하는 오류를 드러낸다
        for (int i = 0; i < bytes.Length; i++) bytes[i] = (byte)((i * 17 + i / 4) % 256);
        first.LoadPalette(new GamePalette(bytes), patch1078: false);
        PaletteColorTable table = first.Colors();
        for (int i = 0; i < bytes.Length; i += 4)
        {
            ScreenColor color = first.Palette[i / 4];
            bytes[i] = color.Blue;
            bytes[i + 1] = color.Green;
            bytes[i + 2] = color.Red;
            bytes[i + 3] = color.Reserved;
        }
        var second = new Screen(800, 600);
        second.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
        second.LoadPalette(new GamePalette(bytes), patch1078: false);
        Assert.Equal(bytes, PaletteBytes(second));
        Assert.True(SameTable(second.Colors(), table));
        Assert.Equal(PaletteColors.CdColorCount, second.Colors().Named.Count);
    }

    /// <summary>표시 팔레트 256색을 B·G·R·예약 바이트 순서로 펼친다.</summary>
    private static byte[] PaletteBytes(Screen screen)
    {
        var output = new byte[1024];
        for (int i = 0; i < 256; i++)
        {
            ScreenColor color = screen.Palette[i];
            output[i * 4] = color.Blue;
            output[i * 4 + 1] = color.Green;
            output[i * 4 + 2] = color.Red;
            output[i * 4 + 3] = color.Reserved;
        }
        return output;
    }
}
