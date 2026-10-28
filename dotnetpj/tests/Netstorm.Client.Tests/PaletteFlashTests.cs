using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 번개 팔레트 프로세스의 보호 계약을 확인한다. 원본 기대값 재생(X86PaletteFlashTests)이 초기화·진행·중단을 대조하고,
/// 이 검사들은 정지·모드 변경·저장 복구·입력 거부의 계약을 옮긴다.
/// 근거: cpppj/tests/PaletteFlashTests.cpp. (R2a, 2026-10-28)
/// </summary>
public sealed class PaletteFlashTests
{
    /// <summary>화면 모드는 매 프레임 읽고 수명은 생성 시 정한다. 브리핑 정지 중에도 실시간의 64단계를 끝낸다.</summary>
    [Fact]
    public void PausedGameAndModeChangesUseWallClock()
    {
        var state = new PaletteFlashState { Duration = PaletteFlashRange.ThunderDuration(true) };
        double wall = 0;
        bool full = false;
        int writes = 0;
        int restores = 0;
        var flash = new PaletteFlash(state, new PaletteFlashHooks
        {
            WallSeconds = () => wall,
            FullScreen = () => full,
            ReadPalette = (_, _, colors) => Array.Fill(colors, new ScreenColor(10, 20, 30, 99)),
            ApplyPalette = (_, _, _) => writes++,
            RestorePalette = () => restores++,
        });
        flash.Init();
        Assert.True(flash.Frame() && writes == 0);
        full = true;
        wall = 0.125;
        Assert.True(flash.Frame() && writes == 1 && restores == 0);
        wall = 0.51;
        Assert.False(flash.Frame() && restores == 1);
        flash.Cancel();
        Assert.Equal(1, restores);
    }

    /// <summary>초기 RGB 부분 배열 대신 당시의 저장 팔레트를 복구한다.</summary>
    [Fact]
    public void CancelRestoresLatestSavedScreenPalette()
    {
        var screen = new Screen(640, 480);
        screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
        var bytes = new byte[1024];
        Array.Fill(bytes, (byte)20);
        screen.LoadPalette(new GamePalette(bytes));
        var state = new PaletteFlashState { Duration = 0.5 };
        double wall = 0;
        int restores = 0;
        var flash = new PaletteFlash(state, new PaletteFlashHooks
        {
            WallSeconds = () => wall,
            FullScreen = () => true,
            ReadPalette = (start, size, destination) => screen.Palette.Slice((int)start, (int)size).CopyTo(destination),
            ApplyPalette = (start, size, colors) => screen.SetPalette(start, size, colors, true),
            RestorePalette = () => { restores++; screen.SetPalette(0, 256, null, true); },
        });
        flash.Init();
        Assert.True(flash.Frame());
        Assert.Equal(84, screen.Palette[228].Red);
        Assert.Equal(20, screen.Palette[227].Red);
        Assert.Equal(20, screen.Palette[246].Red);
        // 효과 도중 새 저장 팔레트를 읽었으면 원본 복구는 새 색을 사용한다
        Array.Fill(bytes, (byte)30);
        screen.LoadPalette(new GamePalette(bytes));
        wall = 0.001;
        Assert.True(flash.Frame());
        flash.Cancel();
        flash.Cancel();
        Assert.Equal(1, restores);
        Assert.Equal(30, screen.Palette[228].Red);
        Assert.Equal(30, screen.Palette[227].Red);
    }

    /// <summary>빈 필수 경계와 부분 배열 밖의 색 범위를 OS 자원 없이 거부한다.</summary>
    [Fact]
    public void RejectsIncompleteHooksAndInvalidRange()
    {
        var state = new PaletteFlashState();
        Assert.Throws<ArgumentException>(() => new PaletteFlash(state, new PaletteFlashHooks()));
        state.Start = 255;
        state.Count = 2;
        Assert.Throws<ArgumentOutOfRangeException>(() => new PaletteFlash(state, new PaletteFlashHooks
        {
            WallSeconds = () => 0.0,
            FullScreen = () => false,
            ReadPalette = (_, _, _) => { },
            ApplyPalette = (_, _, _) => { },
            RestorePalette = () => { },
        }));
    }
}
