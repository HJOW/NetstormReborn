using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Input;

namespace Netstorm.Tests;

/// <summary>
/// MonoGame 호스트의 순수 부분(인자·변환·입력 대응·정수 맞춤·프레임 골격)을 검사한다. 창·장치는 열지 않는다.
/// 근거: cpppj/src/app/main.cpp 의 --run 진입과 Screen.cpp·InputEvent.cpp 의 호스트 몫. (R1-2a, 2026-10-28)
/// </summary>
public sealed class ClientHostTests
{
    /// <summary>기본 인자는 창 모드 1024×768·40fps·무제한이다.</summary>
    [Fact]
    public void DefaultsAreWindowed1024x768()
    {
        HostOptions options = HostOptions.Parse([]);
        Assert.False(options.Fullscreen);
        Assert.Equal(1024, options.Width);
        Assert.Equal(768, options.Height);
        Assert.Equal(40, options.FramesPerSecond);
        Assert.Equal(0ul, options.Frames);
        Assert.Null(options.UiScriptPath);
        Assert.Null(options.UiReportPath);
        Assert.Empty(options.Settings);
    }

    /// <summary>전체 인자를 해석하고 --set 을 같은 이름에 반영한다.</summary>
    [Fact]
    public void ParsesAllFlagsAndSettings()
    {
        HostOptions options = HostOptions.Parse([
            "--fullscreen", "--width", "800", "--height", "600", "--fps", "30",
            "--frames", "34", "--ui-script", "main-menu.tsv", "--ui-report", "states.tsv",
            "--set", "SCREENW=640;SCREENH=480;maxFPS=20;tellTips=0",
        ]);
        Assert.True(options.Fullscreen);
        // 직접 인자보다 --set 이 뒤에 있으므로 --set 값이 이긴다
        Assert.Equal(640, options.Width);
        Assert.Equal(480, options.Height);
        Assert.Equal(20, options.FramesPerSecond);
        Assert.Equal(34ul, options.Frames);
        Assert.Equal("main-menu.tsv", options.UiScriptPath);
        Assert.Equal("states.tsv", options.UiReportPath);
        Assert.Equal("0", options.Settings["tellTips"]);
    }

    /// <summary>--set 조각은 뒤에 나온 값이 이기고 빈 조각은 건너뛴다.</summary>
    [Fact]
    public void SettingsLaterValueWins()
    {
        HostOptions options = HostOptions.Parse(["--set", ";SCREENW=800;;SCREENW=640;"]);
        Assert.Equal(640, options.Width);
    }

    /// <summary>모르는 인자·빠진 값·범위 밖 숫자는 거부한다.</summary>
    [Fact]
    public void RejectsBadArguments()
    {
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--mission", "x"]));
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--width"]));
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--width", "0"]));
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--fps", "-1"]));
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--set", "SCREENW"]));
        Assert.Throws<ArgumentException>(() => HostOptions.Parse(["--set", "maxFPS=0"]));
    }

    /// <summary>인덱스 사각형을 현재 팔레트의 RGBA로 푼다. 행 간격을 넘어 읽지 않는다.</summary>
    [Fact]
    public void ConvertsIndexRectToRgba()
    {
        var screen = new Screen(4, 2);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        // 행 간격 4: 두 번째 행 첫 칸만 5번 색으로 둔다
        byte[] pixels = screen.Lock();
        pixels[4] = 5;
        screen.Unlock();
        var colors = new ScreenColor[256];
        colors[5] = new ScreenColor(Blue: 11, Green: 22, Red: 33, Reserved: 0);
        screen.SetPalette(0, 256, colors, apply: true);
        byte[] rgba = ScreenTextureConverter.ConvertToRgba(screen, new ScreenRect(0, 0, 4, 2));
        // 0번은 검정·불투명, 5번 자리는 팔레트 색이다
        Assert.Equal(new byte[] { 0, 0, 0, 255 }, rgba[0..4]);
        Assert.Equal(new byte[] { 33, 22, 11, 255 }, rgba[16..20]);
        // 화면 밖·빈 사각형은 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => ScreenTextureConverter.ConvertToRgba(screen, new ScreenRect(0, 0, 5, 2)));
        Assert.Throws<ArgumentOutOfRangeException>(() => ScreenTextureConverter.ConvertToRgba(screen, new ScreenRect(1, 1, 1, 1)));
    }

    /// <summary>키·글자·버튼 코드를 UiScript key 단계와 같은 규칙으로 만든다.</summary>
    [Fact]
    public void MapsKeysAndButtons()
    {
        // A(0x41) 누름은 가상 키를 위로 민 값이다
        Assert.Equal(0x41u << 16, HostInputMapper.KeyCode(0x41, released: false, shift: false, control: false, alt: false));
        // 뗌·수식키는 비트를 더한다
        Assert.Equal((0x1Bu << 16) | InputCode.Release | InputCode.Control, HostInputMapper.KeyCode(0x1B, released: true, shift: false, control: true, alt: false));
        // 글자는 아래 코드에 그대로 둔다
        Assert.Equal((uint)'a', HostInputMapper.CharCode('a'));
        // 버튼 세 종류와 뗌 비트다
        Assert.Equal(InputCode.LeftButton, HostInputMapper.ButtonCode(HostMouseButton.Left, released: false));
        Assert.Equal(InputCode.RightButton | InputCode.Release, HostInputMapper.ButtonCode(HostMouseButton.Right, released: true));
        Assert.Equal(InputCode.MiddleButton, HostInputMapper.ButtonCode(HostMouseButton.Middle, released: false));
        // 전환 키와 범위 밖 번호는 거부한다
        Assert.Throws<ArgumentException>(() => HostInputMapper.KeyCode(0x7A, released: false, shift: false, control: false, alt: false));
        Assert.Throws<ArgumentOutOfRangeException>(() => HostInputMapper.KeyCode(256, released: false, shift: false, control: false, alt: false));
    }

    /// <summary>전체화면 커서 제한은 창 안에 있으면 두지 않고 밖이면 가장자리로 모은다.</summary>
    [Fact]
    public void ConfinesCursorOnlyOutside()
    {
        Assert.Null(HostInputMapper.ConfinePoint(10, 20, 100, 100));
        Assert.Equal(new ScreenPoint(99, 50), HostInputMapper.ConfinePoint(150, 50, 100, 100));
        Assert.Equal(new ScreenPoint(0, 0), HostInputMapper.ConfinePoint(-5, -5, 100, 100));
    }

    /// <summary>정수 맞춤은 비율을 지키는 가장 큰 정수 배율로 가운데 둔다.</summary>
    [Fact]
    public void IntegerFitKeepsAspect()
    {
        // 1024×768 을 1920×1080 에 올리면 배율 1(1080/768=1.4→1)로 가운데 둔다
        Assert.Equal(new ScreenRect(448, 156, 1472, 924), HostPresentation.IntegerFitRect(1024, 768, 1920, 1080));
        // 640×480 을 1920×1080 에 올리면 배율 2다
        Assert.Equal(new ScreenRect(320, 60, 1600, 1020), HostPresentation.IntegerFitRect(640, 480, 1920, 1080));
        // 1:1 이면 그대로다
        Assert.Equal(new ScreenRect(0, 0, 1024, 768), HostPresentation.IntegerFitRect(1024, 768, 1024, 768));
        Assert.Throws<ArgumentException>(() => HostPresentation.IntegerFitRect(0, 768, 1920, 1080));
    }

    /// <summary>프레임 골격은 번호를 올리고 스크립트 입력·관측을 같은 번호에 맞춘다.</summary>
    [Fact]
    public void FrameDriverStepsScriptInOrder()
    {
        var screen = new Screen(64, 48);
        var input = new InputQueue();
        var saved = new List<string>();
        var runner = new UiScriptRunner(
            UiScript.Parse(["1\tesc", "2\tsnapshot\tframe2.bmp"]), input, screen, null, saved.Add);
        var driver = new HostFrameDriver(screen, input, runner);
        Assert.False(driver.ShouldExit(2));
        driver.BeginFrame();
        Assert.Equal(1ul, driver.Frame);
        // esc 단계가 큐에 들어갔다
        Assert.False(input.Empty);
        driver.EndFrame();
        driver.BeginFrame();
        Assert.Equal(2ul, driver.Frame);
        driver.EndFrame();
        // snapshot 단계가 저장됐다
        Assert.Equal(["frame2.bmp"], saved);
        Assert.True(driver.ShouldExit(2));
        driver.Finish();
    }

    /// <summary>스크립트가 없으면 제한 없이 돌고 마무리는 비어 있다.</summary>
    [Fact]
    public void FrameDriverRunsWithoutScript()
    {
        var driver = new HostFrameDriver(new Screen(64, 48), new InputQueue());
        driver.BeginFrame();
        driver.EndFrame();
        Assert.False(driver.ShouldExit(0));
        driver.Finish();
        Assert.Empty(driver.SkippedObservations);
    }

    /// <summary>따라잡기 Update 뒤 Draw가 한 번만 돌아도 중간 프레임 관측을 건너뛰지 않는다.</summary>
    [Fact]
    public void FrameDriverDrainsSkippedFrames()
    {
        var screen = new Screen(64, 48);
        var input = new InputQueue();
        var saved = new List<string>();
        var runner = new UiScriptRunner(
            UiScript.Parse(["1\tsnapshot\tframe1.bmp", "3\tsnapshot\tframe3.bmp"]), input, screen, null, saved.Add);
        var driver = new HostFrameDriver(screen, input, runner);
        // Update 3번·Draw 1번을 흉내낸다
        driver.BeginFrame();
        driver.BeginFrame();
        driver.BeginFrame();
        Assert.Equal(3ul, driver.Frame);
        driver.EndFrame();
        Assert.Equal(["frame1.bmp", "frame3.bmp"], saved);
        Assert.True(runner.Finished);
        driver.Finish();
    }
}
