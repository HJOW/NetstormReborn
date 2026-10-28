using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 화면 모드·사각형·전체화면 출력 계산(L1 계약)과 8비트 논리 화면 <see cref="Screen"/>의 동작을 검사한다.
/// 앞쪽 검사는 cpppj/tests/ScreenTests.cpp·ScreenPresentationTests.cpp를 같은 입력과 기대값으로 옮긴 것이다. <see cref="Screen"/> 자체의 검사는
/// 원본 fixture가 없는 C# 쪽 계약 확인이며 원본 일치의 근거로 세지 않는다. (2026-10-28 R1에서 추가)
/// </summary>
public sealed class ScreenTests
{
    /// <summary>원본 004a13d0의 다섯 조건. setup.cfg의 기본값(창 10, 전체화면 11)이 허용되는지도 본다.</summary>
    [Fact]
    public void LegalCombinationsFollowOriginalRules()
    {
        Assert.True(ScreenGeometry.ScreenModeIsLegal(ScreenMode.FallbackWindowed, false));
        Assert.Equal(10u, ScreenMode.FallbackWindowed);
        Assert.False(ScreenGeometry.ScreenModeIsLegal(ScreenMode.FullScreen, false));
        Assert.True(ScreenGeometry.ScreenModeIsLegal(ScreenMode.FullScreen, true));
        Assert.True(ScreenGeometry.ScreenModeIsLegal(ScreenMode.FullScreen | ScreenMode.Flipping, true));
        Assert.True(ScreenGeometry.ScreenModeIsLegal(11, true) && !ScreenGeometry.ScreenModeIsLegal(11, false));
        Assert.False(ScreenGeometry.ScreenModeIsLegal(ScreenMode.BackBufferDib | ScreenMode.Flipping, true));
        Assert.False(ScreenGeometry.ScreenModeIsLegal(ScreenMode.BackBufferDib | ScreenMode.Parallax, true));
        Assert.False(ScreenGeometry.ScreenModeIsLegal(ScreenMode.BackBufferDib | ScreenMode.Animating, true));
        Assert.True(ScreenGeometry.ScreenModeIsLegal(ScreenMode.BackBufferDib | ScreenMode.Clouds | ScreenMode.Parallax | ScreenMode.Animating, false));
        Assert.False(ScreenGeometry.ScreenModeIsLegal(0, true));
        Assert.False(ScreenGeometry.ScreenModeIsLegal(ScreenMode.Clouds | ScreenMode.ModeSet, true));
    }

    /// <summary>원본 00445290: 네 좌표를 각각 경계 안으로 넣는다. 뒤집힌 사각형을 바로잡지는 않는다.</summary>
    [Fact]
    public void ClipClampsEachCoordinate()
    {
        var bounds = new ScreenRect(0, 0, 640, 480);
        Assert.Equal(new ScreenRect(10, 20, 30, 40), ScreenGeometry.ClipScreenRect(new ScreenRect(10, 20, 30, 40), bounds));
        Assert.Equal(bounds, ScreenGeometry.ClipScreenRect(new ScreenRect(-5, -7, 700, 500), bounds));
        // 완전히 밖이면 폭·높이가 0인 사각형이 된다.
        Assert.Equal(new ScreenRect(640, 480, 640, 480), ScreenGeometry.ClipScreenRect(new ScreenRect(650, 490, 700, 500), bounds));
        ScreenRect reversed = ScreenGeometry.ClipScreenRect(new ScreenRect(30, 40, 10, 20), bounds);
        Assert.True(reversed.Left == 30 && reversed.Right == 10);
    }

    /// <summary>원본 004a4fe0의 switch(initWindowPos): 1~5는 바탕 화면의 모서리·가운데, 그 밖은 지금 위치.</summary>
    [Fact]
    public void InitialWindowPositionFollowsSetting()
    {
        var current = new ScreenPoint(33, 44);
        ScreenPoint At(int setting) => ScreenGeometry.InitialWindowPosition(setting, 1920, 1080, 1040, 807, current);
        Assert.Equal(new ScreenPoint(0, 0), At(1));
        Assert.Equal(new ScreenPoint(880, 0), At(2));
        Assert.Equal(new ScreenPoint(880, 273), At(3));
        Assert.Equal(new ScreenPoint(0, 273), At(4));
        Assert.Equal(new ScreenPoint(440, 136), At(5));
        Assert.Equal(current, At(0));
        Assert.Equal(current, At(6));
        // 창이 바탕 화면보다 크면 음수가 된다(원본은 고치지 않는다).
        Assert.Equal(-120, ScreenGeometry.InitialWindowPosition(5, 800, 600, 1040, 807, current).X);
    }

    /// <summary>원본 4:3과 와이드 논리 화면이 4:3·16:9·16:10 출력에서 늘어나지 않는다.</summary>
    [Fact]
    public void AspectFitAndLetterbox()
    {
        Assert.Equal(new ScreenRect(240, 0, 1680, 1080), ScreenPresentation.FitScreenPresentation(1024, 768, 1920, 1080));
        Assert.Equal(new ScreenRect(0, 150, 1600, 1050), ScreenPresentation.FitScreenPresentation(1280, 720, 1600, 1200));
        Assert.Equal(new ScreenRect(96, 0, 1824, 1080), ScreenPresentation.FitScreenPresentation(1280, 800, 1920, 1080));
        Assert.Equal(new ScreenRect(0, 0, 640, 480), ScreenPresentation.FitScreenPresentation(1024, 768, 640, 480));
        Assert.Equal(new ScreenRect(0, 0, 1920, 1079), ScreenPresentation.FitScreenPresentation(1366, 768, 1920, 1080));
        Assert.Throws<ArgumentException>(() => ScreenPresentation.FitScreenPresentation(0, 768, 1920, 1080));
    }

    /// <summary>모니터 좌표의 음수/레터박스/끝점과 확대된 실제 중앙 클릭을 논리 입력으로 되돌린다. 변경 사각형은 바깥쪽으로 반올림한다.</summary>
    [Fact]
    public void InversePointAndDirtyPhase()
    {
        var output = new ScreenRect(240, 0, 1680, 1080);
        Assert.Equal(new ScreenPoint(512, 384), ScreenPresentation.MapScreenPresentationPoint(new ScreenPoint(960, 540), output, 1024, 768));
        Assert.Equal(new ScreenPoint(0, 0), ScreenPresentation.MapScreenPresentationPoint(new ScreenPoint(-200, -40), output, 1024, 768));
        Assert.Equal(new ScreenPoint(1024, 768), ScreenPresentation.MapScreenPresentationPoint(new ScreenPoint(1920, 1080), output, 1024, 768));
        Assert.Equal(new ScreenRect(241, 1, 243, 3), ScreenPresentation.MapScreenPresentationRect(new ScreenRect(1, 1, 2, 2), output, 1024, 768));
        Assert.Equal(new ScreenRect(240, 0, 1680, 1080), ScreenPresentation.MapScreenPresentationRect(new ScreenRect(-10, -10, 2000, 1000), output, 1024, 768));
        ScreenRect tiny = ScreenPresentation.FitScreenPresentation(1024, 768, 1, 1);
        Assert.True(tiny.Right == 1 && tiny.Bottom == 1);
    }

    /// <summary>호환 모드는 실제 DIB/비flip을 공개하며 원본 합법성 함수의 DirectDraw 판정은 바꾸지 않는다.</summary>
    [Fact]
    public void CompatibilityFlags()
    {
        Assert.Equal(235u, ScreenPresentation.ModernScreenMode(111));
        Assert.Equal(139u, ScreenPresentation.ModernScreenMode(139));
        Assert.Equal(130u, ScreenPresentation.ModernScreenMode(0));
        Assert.Equal(138u, ScreenPresentation.ModernScreenMode(94));
        Assert.False(ScreenGeometry.ScreenModeIsLegal(111, false));
        Assert.True(ScreenGeometry.ScreenModeIsLegal(ScreenPresentation.ModernScreenMode(111), true));
    }

    /// <summary>확대/축소의 중앙 표본·실제 행 간격·padding 보존을 작은 독립 입력으로 확인한다. 버퍼가 모자라면 쓰기 전에 거부한다.</summary>
    [Fact]
    public void IntegerNearestPixelsAndBounds()
    {
        byte[] source = [1, 2, 77, 77, 3, 4, 77, 77];
        var output = new byte[12];
        Array.Fill(output, (byte)99);
        ScreenPresentation.ScaleScreenPresentation(source, 4, 2, 2, output, 4, 3, 3);
        Assert.Equal(new byte[] { 1, 2, 2, 99, 3, 4, 4, 99, 3, 4, 4, 99 }, output);
        byte[] large = [0, 1, 2, 3, 77, 4, 5, 6, 7, 77, 8, 9, 10, 11, 77];
        var small = new byte[8];
        Array.Fill(small, (byte)99);
        ScreenPresentation.ScaleScreenPresentation(large, 5, 4, 3, small, 4, 3, 2);
        Assert.Equal(new byte[] { 0, 2, 3, 99, 8, 10, 11, 99 }, small);
        Assert.Throws<ArgumentOutOfRangeException>(() => ScreenPresentation.ScaleScreenPresentation(source.AsSpan(0, 5), 4, 2, 2, new byte[12], 4, 3, 3));
    }

    /// <summary>목적지 각 픽셀을 변경 전 버퍼의 독립 좌표로 대조해 행 순서·겹침·부분 영역·padding을 확인한다(모든 방향 이동).</summary>
    [Fact]
    public void ScrollAllDirectionsPreserveSourceAndPadding()
    {
        const int pitch = 16, width = 13, height = 9, x = 2, y = 1, w = 9, h = 7; // 홀수 논리 폭과 실제 padding을 포함한 입력이다.
        var original = new byte[pitch * height];
        for (int i = 0; i < original.Length; i++) original[i] = (byte)i;
        // 위/아래·범위 밖 이동과 0, 좌/우·범위 밖과 대각선의 겹침을 교차한다.
        for (int dy = -8; dy <= 8; dy++)
        {
            for (int dx = -10; dx <= 10; dx++)
            {
                byte[] actual = (byte[])original.Clone();
                ScreenPresentation.ScrollScreenPixels(actual, pitch, width, height, x, y, w, h, dx, dy);
                // 이동 범위 밖과 padding도 변경되지 않아야 한다. 각 기대 픽셀은 변경 전 원본에서만 읽는다.
                for (int row = 0; row < height; row++)
                {
                    for (int column = 0; column < pitch; column++)
                    {
                        bool copied = Math.Abs(dx) < w && Math.Abs(dy) < h && column >= x && column < x + w && row >= y && row < y + h
                            && column + dx >= x && column + dx < x + w && row + dy >= y && row + dy < y + h;
                        Assert.Equal(original[copied ? (row + dy) * pitch + column + dx : row * pitch + column], actual[row * pitch + column]);
                    }
                }
            }
        }
    }

    /// <summary>선택된 복사의 화면 밖 메모리는 쓰기 전에 거부하고 0·겹침 없는 이동은 원문처럼 반환한다.</summary>
    [Fact]
    public void ScrollSelectedSurfaceBounds()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => ScreenPresentation.ScrollScreenPixels(new byte[64], 8, 8, 8, -1, 0, 5, 5, 1, 1));
        ScreenPresentation.ScrollScreenPixels([], 8, 8, 8, -1, 0, 5, 5, 0, 0);
        ScreenPresentation.ScrollScreenPixels([], 8, 8, 8, 0, 0, 5, 5, 5, 0);
    }

    /// <summary>해상도 목록은 원본 4:3 세 항목 뒤에 호환 세 항목이 이어진다.</summary>
    [Fact]
    public void ResolutionChoicesKeepOriginalOrder()
    {
        Assert.Equal(6, ScreenResolutions.Choices.Length);
        Assert.Equal([(640u, 480u), (800u, 600u), (1024u, 768u)], ScreenResolutions.Original.ToArray());
        Assert.Equal([(1280u, 720u), (1280u, 800u), (1366u, 768u)], ScreenResolutions.Compatibility.ToArray());
    }

    /// <summary>실제 Win32 DIB처럼 행 간격이 4바이트로 정렬된다(폭 1366 → 1368). 끝 행 쓰기가 옆 행과 섞이지 않는다.</summary>
    [Fact]
    public void WideScreenUsesAlignedPitch()
    {
        var screen = new Screen(1366, 768);
        Assert.Equal(1368, screen.Pitch);
        Assert.Equal(1368 * 768, screen.Pixels.Length);
        Assert.Equal(640, new Screen(640, 480).Pitch);
        Assert.Equal(1024, new Screen(1024, 768).Pitch);
        Assert.Throws<ArgumentOutOfRangeException>(() => new Screen(0, 480));
    }

    /// <summary>모드를 정하면 현대 DIB 모드로 정규화되고, 같은 모드는 그대로 성공하며, 허용되지 않는 조합은 거부한다. 모드 전 그리기는 아무것도 쓰지 않는다.</summary>
    [Fact]
    public void SetModeNormalizesAndGatesDrawing()
    {
        var screen = new Screen(16, 8);
        screen.Clear(new ScreenRect(0, 0, 16, 8), 9);
        Assert.All(screen.Pixels.ToArray(), p => Assert.Equal(0, p)); // flags 0: Clear는 아무것도 하지 않는다.
        Assert.True(screen.SetMode(111));
        Assert.Equal(235u, screen.Flags);
        Assert.True(screen.SetMode(111)); // 같은 모드(정규화 뒤)는 성공한다.
    }

    /// <summary>창 모드에서는 시차가 정규화로 꺼지므로 시차만 요청해도 거부되지 않는다. 합법 조합만 true다.</summary>
    [Fact]
    public void SetModeWindowedDropsParallax()
    {
        var screen = new Screen(16, 8);
        Assert.True(screen.SetMode(ScreenMode.Parallax));
        Assert.Equal(ScreenMode.BackBufferDib | ScreenMode.ModeSet, screen.Flags);
        Assert.True(screen.SetMode(ScreenMode.Animating | ScreenMode.Clouds | ScreenMode.BackBufferDib));
        Assert.False(screen.SetMode(ScreenMode.Animating)); // clouds 없는 animating은 창 모드 정규화로도 남아 거부된다.
    }

    /// <summary>잠금은 중첩 횟수를 세며 처음 잠글 때만 버퍼를 얻고 마지막에 푼다. 잠그지 않고 풀면 예외다.</summary>
    [Fact]
    public void LockDepthIsCounted()
    {
        var screen = new Screen(8, 4);
        Assert.Throws<InvalidOperationException>(screen.Unlock);
        byte[] first = screen.Lock();
        byte[] second = screen.Lock();
        Assert.Same(first, second);
        screen.Unlock();
        screen.Unlock();
        Assert.Throws<InvalidOperationException>(screen.Unlock);
    }

    /// <summary>FillRect는 자르기 영역으로 먼저 자르고, 잠그지 않았거나 모드 전이면 쓰지 않는다. Clear는 잠그고 채우고 푼다.</summary>
    [Fact]
    public void FillRectClipsAndNeedsLockAndMode()
    {
        var screen = new Screen(8, 4);
        screen.SetMode(ScreenMode.BackBufferDib);
        screen.SetClip(1, 1, 5, 3);
        screen.FillRect(new ScreenRect(0, 0, 8, 4), 7); // 잠그지 않았으므로 쓰지 않는다.
        Assert.All(screen.Pixels.ToArray(), p => Assert.Equal(0, p));
        screen.Lock();
        screen.FillRect(new ScreenRect(0, 0, 8, 4), 7);
        screen.Unlock();
        int filled = screen.Pixels.ToArray().Count(p => p == 7);
        Assert.Equal(4 * 2, filled); // 폭 4, 높이 2만 채워진다.
        Assert.Equal(7, screen.Pixels[1 * screen.Pitch + 1]);
        Assert.Equal(0, screen.Pixels[1 * screen.Pitch + 0]);
        screen.Clear(new ScreenRect(1, 1, 5, 3), 0);
        Assert.All(screen.Pixels.ToArray(), p => Assert.Equal(0, p));
    }

    /// <summary>Update는 화면 안으로 자른 사각형을 호스트에 알리고 자르기 영역을 화면 전체로 되돌린다. 빈 사각형은 알리지 않는다.</summary>
    [Fact]
    public void UpdateClipsNotifiesAndResetsClip()
    {
        var screen = new Screen(8, 4);
        var output = new RecordingOutput();
        screen.Output = output;
        screen.SetClip(2, 2, 3, 3);
        screen.Update(new ScreenRect(-5, 1, 20, 3));
        Assert.Equal([new ScreenRect(0, 1, 8, 3)], output.Presented);
        Assert.Equal(new ScreenRect(0, 0, 8, 4), screen.Clip);
        screen.Update(new ScreenRect(3, 3, 3, 3)); // 비어 있다.
        Assert.Single(output.Presented);
    }

    /// <summary>팔레트 설정은 0번 검정·255번 흰색을 고정하고, apply=false 준비도 색 검색에 보이며 표시 팔레트는 바꾸지 않는다.</summary>
    [Fact]
    public void PreparedLogicalPaletteAndWeatherTable()
    {
        var screen = new Screen(640, 480);
        var colors = new ScreenColor[256];
        colors[17] = new ScreenColor(22, 255, 255, 193);
        colors[38] = new ScreenColor(255, 22, 22, 147);
        colors[69] = new ScreenColor(22, 22, 255, 91);
        colors[127] = new ScreenColor(111, 140, 181, 255);
        screen.SetPalette(0, 256, colors, false);
        Assert.Equal([17u, 38u, 69u, 127u], screen.WeatherTints());
        Assert.True(screen.FindColor(0, 0, 0) == 0 && screen.FindColor(255, 255, 255) == 255);
        // 표시 버퍼는 적용하지 않았으므로 여전히 검정이다.
        Assert.True(screen.Palette[17].Red == 0 && screen.Palette[17].Green == 0);
        screen.SetPalette(0, 256, colors, true);
        Assert.Equal(255, screen.Palette[17].Green);
        Assert.Equal(new ScreenColor(0, 0, 0, 0), screen.Palette[0]);
        Assert.Equal(new ScreenColor(255, 255, 255, 0), screen.Palette[255]);
    }

    /// <summary>파일 팔레트는 COL(0x308)과 BGRX(0x400) 두 크기만 받는다. BGRX는 예약 바이트를 돌려주고 COL은 null이다.</summary>
    [Fact]
    public void GamePaletteReadsBothFileSizes()
    {
        var col = new byte[0x308];
        col[8 + 3 * 5 + 0] = 10; col[8 + 3 * 5 + 1] = 20; col[8 + 3 * 5 + 2] = 30; // 5번 색 RGB
        var fromCol = new GamePalette(col);
        Assert.Equal(new PaletteColor(10, 20, 30), fromCol.Color(5));
        Assert.Null(fromCol.Reserved(5));
        var bgrx = new byte[0x400];
        bgrx[4 * 7 + 0] = 3; bgrx[4 * 7 + 1] = 2; bgrx[4 * 7 + 2] = 1; bgrx[4 * 7 + 3] = 9; // 7번 색 BGRX
        var fromBgrx = new GamePalette(bgrx);
        Assert.Equal(new PaletteColor(1, 2, 3), fromBgrx.Color(7));
        Assert.Equal((byte?)9, fromBgrx.Reserved(7));
        Assert.Throws<InvalidDataException>(() => new GamePalette(new byte[100]));
    }

    /// <summary>
    /// 파일 팔레트 적용은 저장 팔레트를 채우고 현재 팔레트에 반영한다. clear를 요청하면 모드가 정해진 화면을 검정으로 지운 뒤 호스트에 알리고 수직 동기를 기다린다.
    /// COL은 예약 바이트를 읽지 않으므로 기존 저장 예약 값을 유지한다.
    /// </summary>
    [Fact]
    public void LoadPaletteAppliesAndClears()
    {
        var screen = new Screen(8, 4);
        var output = new RecordingOutput();
        screen.Output = output;
        screen.SetMode(ScreenMode.BackBufferDib);
        screen.Lock();
        screen.SetClip(0, 0, 8, 4);
        screen.FillRect(new ScreenRect(0, 0, 8, 4), 5);
        screen.Unlock();
        var col = new byte[0x308];
        col[8 + 3 * 5] = 100; col[8 + 3 * 5 + 1] = 110; col[8 + 3 * 5 + 2] = 120;
        screen.LoadPalette(new GamePalette(col), clear: 1);
        Assert.Equal(1u, screen.PaletteLoaded);
        Assert.Equal(new ScreenColor(120, 110, 100, 0), screen.Palette[5]);
        Assert.All(screen.Pixels.ToArray(), p => Assert.Equal(0, p)); // 지우기
        Assert.Equal([new ScreenRect(0, 0, 8, 4)], output.Presented);
        Assert.Equal(1, output.VerticalBlanks);
    }

    /// <summary>Scroll은 DIB 모드에서만 가능하고 잠금 수를 남기지 않는다.</summary>
    [Fact]
    public void ScrollNeedsDibModeAndReleasesLock()
    {
        var screen = new Screen(16, 8);
        Assert.Throws<InvalidOperationException>(() => screen.Scroll(0, 0, 8, 4, 1, 0));
        screen.SetMode(ScreenMode.BackBufferDib);
        screen.Lock();
        screen.FillRect(new ScreenRect(0, 0, 0, 0), 0);
        screen.Unlock();
        screen.Scroll(0, 0, 8, 4, 1, 0);
        screen.Scroll(0, 0, 8, 4, 0, 0);
        Assert.Throws<InvalidOperationException>(screen.Unlock); // 남은 잠금이 없다.
    }

    /// <summary>BlitBuffer는 잠근 버퍼에 원본 사각형을 복사하고 자르기 영역을 넘는 부분은 자른다. 잠그지 않으면 예외다.</summary>
    [Fact]
    public void BlitBufferCopiesAndClips()
    {
        var screen = new Screen(8, 4);
        screen.SetMode(ScreenMode.BackBufferDib);
        byte[] source = new byte[8 * 4];
        for (int i = 0; i < source.Length; i++) source[i] = (byte)(i + 1);
        Assert.Throws<InvalidOperationException>(() => screen.BlitBuffer(0, 0, source, 8, new ScreenRect(0, 0, 4, 2)));
        screen.SetClip(0, 0, 8, 4);
        screen.Lock();
        screen.BlitBuffer(6, 2, source, 8, new ScreenRect(0, 0, 4, 2)); // 오른쪽·아래가 화면 밖으로 잘린다.
        screen.Unlock();
        Assert.Equal(1, screen.Pixels[2 * screen.Pitch + 6]);
        Assert.Equal(2, screen.Pixels[2 * screen.Pitch + 7]);
        Assert.Equal(9, screen.Pixels[3 * screen.Pitch + 6]);
        Assert.Equal(0, screen.Pixels[2 * screen.Pitch + 5]);
    }

    /// <summary>전체화면에서만 실제 클라이언트 좌표를 논리 좌표로 되돌린다. 창 모드는 1:1이다.</summary>
    [Fact]
    public void ToLogicalPointOnlyScalesInFullScreen()
    {
        var screen = new Screen(1024, 768);
        screen.SetMode(ScreenMode.BackBufferDib);
        Assert.Equal(new ScreenPoint(960, 540), screen.ToLogicalPoint(new ScreenPoint(960, 540), 1920, 1080));
        screen.SetMode(ScreenMode.FullScreen | ScreenMode.BackBufferDib);
        Assert.Equal(new ScreenPoint(512, 384), screen.ToLogicalPoint(new ScreenPoint(960, 540), 1920, 1080));
        Assert.Equal(new ScreenRect(240, 0, 1680, 1080), screen.PresentationRect(1920, 1080));
    }

    /// <summary>호스트 호출을 기록하는 검사용 출력.</summary>
    private sealed class RecordingOutput : IScreenOutput
    {
        /// <summary>알림을 받은 갱신 사각형들.</summary>
        public List<ScreenRect> Presented { get; } = [];

        /// <summary>수직 동기 대기 횟수.</summary>
        public int VerticalBlanks { get; private set; }

        /// <inheritdoc />
        public void Present(Screen screen, ScreenRect rect) => Presented.Add(rect);

        /// <inheritdoc />
        public void WaitVerticalBlank(Screen screen) => VerticalBlanks++;
    }
}
