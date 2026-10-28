using Netstorm.Client.Automation;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Tests;

/// <summary>
/// 논리 화면 캡처(cpppj와 byte 동일한 BMP)와 UI 스크립트 파서·실행기를 검사한다. 캡처 형식의 기대값은 구현을 거치지 않고 BITMAPV4HEADER 필드 명세에서
/// 직접 쓴 바이트이며, cpppj가 이 PC에서 만든 실제 1024×768 캡처의 머리(122바이트)와도 같음을 확인했다(docs/dotnet-client-screen-20261028.md 5절).
/// 스크립트 검사는 cpppj/src/app/main.cpp 의 동작을 문서화한 C# 쪽 계약이다. (2026-10-28 R1-2에서 추가)
/// </summary>
public sealed class CaptureAndScriptTests
{
    /// <summary>명세에서 직접 만든 BMP 머리 122바이트: 파일 머리 14 + BITMAPV4HEADER 108.</summary>
    private static byte[] ExpectedHeader(int width, int height)
    {
        var h = new byte[122];
        void U32(int offset, uint value) { for (int i = 0; i < 4; i++) h[offset + i] = (byte)(value >> (8 * i)); }
        h[0] = 0x42; h[1] = 0x4d;                       // 'B' 'M'
        U32(2, (uint)(122 + width * height * 4));        // 파일 크기
        U32(10, 122);                                    // 화소 시작 위치
        U32(14, 108);                                    // BITMAPV4HEADER 크기
        U32(18, (uint)width);                            // 폭
        U32(22, unchecked((uint)-height));               // 음수 높이: 위에서 아래 행
        h[26] = 1;                                       // 평면 수(16비트)
        h[28] = 32;                                      // 비트 수(16비트)
        U32(30, 3);                                      // BI_BITFIELDS
        U32(34, (uint)(width * height * 4));             // 화소 크기
        U32(54, 0x00ff0000); U32(58, 0x0000ff00); U32(62, 0x000000ff); U32(66, 0xff000000); // R·G·B·A 마스크
        U32(70, 0x73524742);                             // 색 공간 'sRGB'
        return h;
    }

    /// <summary>머리가 명세와 byte 단위로 같고 화소는 RGBA가 아니라 BGRA 순서로 나온다.</summary>
    [Fact]
    public void EncodeBitmapMatchesSpecAndUsesBgra()
    {
        byte[] rgba = [10, 20, 30, 255, 40, 50, 60, 255];
        byte[] bmp = ScreenCapture.EncodeBitmap(2, 1, rgba);
        Assert.Equal(ExpectedHeader(2, 1), bmp[..122]);
        Assert.Equal(new byte[] { 30, 20, 10, 255, 60, 50, 40, 255 }, bmp[122..]);
        Assert.Equal(122 + 8, bmp.Length);
    }

    /// <summary>1024×768 머리도 명세와 같고 파일 크기는 3,145,850바이트다(cpppj 실제 캡처와 같은 값).</summary>
    [Fact]
    public void EncodeBitmapHeaderForStandardScreen()
    {
        byte[] bmp = ScreenCapture.EncodeBitmap(1024, 768, new byte[1024 * 768 * 4]);
        Assert.Equal(3145850, bmp.Length);
        Assert.Equal(ExpectedHeader(1024, 768), bmp[..122]);
    }

    /// <summary>크기가 틀린 버퍼나 양수가 아닌 크기는 거부한다.</summary>
    [Fact]
    public void EncodeBitmapRejectsInvalidSizes()
    {
        Assert.Throws<ArgumentException>(() => ScreenCapture.EncodeBitmap(2, 2, new byte[8]));
        Assert.Throws<ArgumentException>(() => ScreenCapture.EncodeBitmap(0, 1, new byte[0]));
        Assert.Throws<ArgumentException>(() => ScreenCapture.EncodeBitmap(1, -1, new byte[0]));
    }

    /// <summary>화면 캡처는 padding을 빼고 표시 팔레트로 색을 바꾸며 잠금 수를 남기지 않는다.</summary>
    [Fact]
    public void ToRgbaUsesPaletteAndSkipsPadding()
    {
        var screen = new Screen(3, 2); // pitch 4: 행마다 padding 1바이트
        screen.SetMode(ScreenMode.BackBufferDib);
        var colors = new ScreenColor[256];
        colors[5] = new ScreenColor(30, 20, 10, 0); // 파랑 30, 초록 20, 빨강 10
        colors[6] = new ScreenColor(60, 50, 40, 0);
        screen.SetPalette(0, 256, colors, true);
        screen.Lock();
        screen.SetClip(0, 0, 3, 2);
        screen.FillRect(new ScreenRect(0, 0, 3, 2), 5);
        screen.FillRect(new ScreenRect(2, 1, 3, 2), 6);
        screen.Unlock();
        byte[] rgba = ScreenCapture.ToRgba(screen);
        Assert.Equal(3 * 2 * 4, rgba.Length);
        Assert.Equal(new byte[] { 10, 20, 30, 255 }, rgba[..4]);
        Assert.Equal(new byte[] { 40, 50, 60, 255 }, rgba[^4..]); // (2,1) 픽셀
        Assert.Throws<InvalidOperationException>(screen.Unlock); // 캡처가 잠금을 남기지 않았다.
    }

    /// <summary>화면을 파일로 저장하면 머리와 화소가 <see cref="ScreenCapture.EncodeBitmap"/>과 같다.</summary>
    [Fact]
    public void WriteBitmapWritesFile()
    {
        var screen = new Screen(2, 2);
        string path = Path.Combine(Path.GetTempPath(), "netstorm-capture-" + Guid.NewGuid().ToString("N") + ".bmp");
        try
        {
            ScreenCapture.WriteBitmap(path, screen);
            byte[] written = File.ReadAllBytes(path);
            Assert.Equal(122 + 16, written.Length);
            Assert.Equal(ExpectedHeader(2, 2), written[..122]);
        }
        finally { File.Delete(path); }
    }

    /// <summary>파서는 빈 줄·주석·CR을 건너뛰고 인자에 탭이 있어도 보존하며, 프레임 규칙을 어기면 거부한다.</summary>
    [Fact]
    public void ParseReadsStepsAndRejectsBadFrames()
    {
        List<UiStep> steps = UiScript.Parse(["# 주석", "", "3\tesc\r", "3\tsnapshot\tC:\\a b\\c.bmp", "7\tclick\tscreen:1,2", "9\tkey"]);
        Assert.Equal([new UiStep(3, "esc", ""), new UiStep(3, "snapshot", "C:\\a b\\c.bmp"), new UiStep(7, "click", "screen:1,2"), new UiStep(9, "key", "")], steps);
        Assert.Throws<ArgumentException>(() => UiScript.Parse(["no tab"]));
        Assert.Throws<ArgumentException>(() => UiScript.Parse(["0\tesc"]));
        Assert.Throws<ArgumentException>(() => UiScript.Parse(["5\tesc", "4\tesc"]));
        Assert.Throws<ArgumentException>(() => UiScript.Parse(["x\tesc"]));
    }

    /// <summary>입력 큐에 쌓인 사건을 모두 꺼낸다.</summary>
    private static List<InputEvent> Drain(InputQueue queue)
    {
        var events = new List<InputEvent>();
        // 비어 있지 않은 동안 하나씩 꺼낸다.
        while (!queue.Empty) events.Add(queue.Pop(false));
        return events;
    }

    /// <summary>esc·key·keyup은 원래 사건 코드(글자는 아래 비트, 가상 키는 위 비트, shift+ 접두)를 큐에 넣는다. 알 수 없는 키는 거부한다.</summary>
    [Fact]
    public void RunnerPushesKeyEvents()
    {
        var input = new InputQueue();
        var runner = new UiScriptRunner(UiScript.Parse(["1\tesc", "1\tkey\ta", "1\tkeyup\ta", "1\tkey\tshift+left", "1\tkey\thome"]), input, new Screen(8, 8));
        runner.BeforeInput(1);
        Assert.Equal(
            [new InputEvent(0x1b, 0, 0), new InputEvent('a', 0, 0), new InputEvent('a' | InputCode.Release, 0, 0),
             new InputEvent((0x25u << 16) | InputCode.Shift, 0, 0), new InputEvent(0x73u << 16, 0, 0)],
            Drain(input));
        var bad = new UiScriptRunner(UiScript.Parse(["1\tkey\tnope"]), new InputQueue(), new Screen(8, 8));
        Assert.Throws<ArgumentException>(() => bad.BeforeInput(1));
    }

    /// <summary>좌표 클릭은 누름·뗌 두 사건을, right는 오른쪽 버튼 두 사건을, down은 누름만, up은 뗌만 넣고 폴링 포인터를 같은 위치로 맞춘다.</summary>
    [Fact]
    public void RunnerPushesPointerEvents()
    {
        var input = new InputQueue();
        var screen = new Screen(100, 50);
        var runner = new UiScriptRunner(UiScript.Parse(["1\tclick\tscreen:10,20", "2\tright\tscreen:3,4", "3\tdown\tscreen:5,6", "4\tup\tscreen:5,6", "5\toutside"]), input, screen);
        runner.BeforeInput(1);
        Assert.Equal([new InputEvent(InputCode.LeftButton, 10, 20), new InputEvent(InputCode.LeftButton | InputCode.Release, 10, 20)], Drain(input));
        Assert.Equal(new InputEvent(InputCode.LeftButton | InputCode.Release, 10, 20), runner.ScriptPointer);
        runner.AfterFrame(1);
        runner.BeforeInput(2);
        Assert.Equal([new InputEvent(InputCode.RightButton, 3, 4), new InputEvent(InputCode.RightButton | InputCode.Release, 3, 4)], Drain(input));
        runner.AfterFrame(2);
        runner.BeforeInput(3);
        Assert.Equal([new InputEvent(InputCode.LeftButton, 5, 6)], Drain(input));
        Assert.Equal(new InputEvent(InputCode.LeftButton, 5, 6), runner.ScriptPointer); // down 뒤에는 누른 상태로 폴링한다.
        runner.AfterFrame(3);
        runner.BeforeInput(4);
        Assert.Equal([new InputEvent(InputCode.LeftButton | InputCode.Release, 5, 6)], Drain(input));
        runner.AfterFrame(4);
        runner.BeforeInput(5);
        Assert.Equal([new InputEvent(InputCode.LeftButton, 0, 0)], Drain(input));
    }

    /// <summary>화면 밖 좌표와 잘못된 형식은 거부하고, 라벨 클릭은 조회기가 있어야 하며 click-if는 없는 컨트롤을 조용히 건너뛴다.</summary>
    [Fact]
    public void RunnerValidatesCoordinatesAndLabels()
    {
        var screen = new Screen(100, 50);
        Assert.Throws<ArgumentException>(() => new UiScriptRunner(UiScript.Parse(["1\tclick\tscreen:100,0"]), new InputQueue(), screen).BeforeInput(1));
        Assert.Throws<ArgumentException>(() => new UiScriptRunner(UiScript.Parse(["1\tclick\tscreen:-1,0"]), new InputQueue(), screen).BeforeInput(1));
        Assert.Throws<ArgumentException>(() => new UiScriptRunner(UiScript.Parse(["1\tclick\tscreen:1"]), new InputQueue(), screen).BeforeInput(1));
        Assert.Throws<InvalidOperationException>(() => new UiScriptRunner(UiScript.Parse(["1\tclick\tCampaign"]), new InputQueue(), screen).BeforeInput(1));
        var optional = new InputQueue();
        new UiScriptRunner(UiScript.Parse(["1\tclick-if\tCampaign"]), optional, screen).BeforeInput(1);
        Assert.True(optional.Empty);
        var located = new InputQueue();
        new UiScriptRunner(UiScript.Parse(["1\tclick\tCampaign"]), located, screen, new FixedLocator(new ScreenPoint(7, 9))).BeforeInput(1);
        Assert.Equal([new InputEvent(InputCode.LeftButton, 7, 9), new InputEvent(InputCode.LeftButton | InputCode.Release, 7, 9)], Drain(located));
    }

    /// <summary>pointer 단계는 큐에 넣지 않고 폴링 상태만 바꾼다. 코드는 10진수·16진수·8진수를 받는다.</summary>
    [Fact]
    public void RunnerPointerSetsPollingOnly()
    {
        var input = new InputQueue();
        var runner = new UiScriptRunner(UiScript.Parse(["1\tpointer\t0x10000,3,4", "2\tpointer\t65536,5,6", "3\tpointer\t010,1,2"]), input, new Screen(8, 8));
        // 실제 프레임 루프처럼 프레임마다 입력과 그리기 뒤 처리를 짝으로 부른다.
        runner.BeforeInput(1);
        runner.AfterFrame(1);
        Assert.Equal(new InputEvent(0x10000, 3, 4), runner.ScriptPointer);
        runner.BeforeInput(2);
        runner.AfterFrame(2);
        Assert.Equal(new InputEvent(65536, 5, 6), runner.ScriptPointer);
        runner.BeforeInput(3);
        runner.AfterFrame(3);
        Assert.Equal(new InputEvent(8, 1, 2), runner.ScriptPointer);
        Assert.True(input.Empty);
        Assert.Throws<ArgumentException>(() => new UiScriptRunner(UiScript.Parse(["1\tpointer\t1,2"]), input, new Screen(8, 8)).BeforeInput(1));
    }

    /// <summary>snapshot은 그리기 뒤에 저장하고, report·refresh는 건너뛰며 기록하고, 위치는 AfterFrame에서만 전진하며, 남은 단계는 마무리 검사가 알린다.</summary>
    [Fact]
    public void RunnerSnapshotsAfterFrameAndTracksCompletion()
    {
        var saved = new List<string>();
        var runner = new UiScriptRunner(UiScript.Parse(["2\treport\tmain", "2\tsnapshot\tout.bmp", "4\tsnapshot\tlate.bmp"]), new InputQueue(), new Screen(8, 8), snapshot: saved.Add);
        runner.BeforeInput(2);
        Assert.Empty(saved);
        runner.AfterFrame(2);
        Assert.Equal(["out.bmp"], saved);
        Assert.Equal(["2:report"], runner.SkippedObservations);
        Assert.False(runner.Finished);
        Assert.Throws<InvalidOperationException>(runner.RequireFinished);
        runner.AfterFrame(3); // 다른 프레임에서는 아무것도 하지 않는다.
        runner.AfterFrame(4);
        Assert.Equal(["out.bmp", "late.bmp"], saved);
        runner.RequireFinished();
    }

    /// <summary>아직 옮기지 않은 동작은 빈 성공이 아니라 명시 예외다. 앞 프레임에서 못 한 단계가 있으면 뒤 단계도 막힌다.</summary>
    [Fact]
    public void RunnerRejectsUnsupportedAndBlockedSteps()
    {
        Assert.Throws<NotSupportedException>(() => new UiScriptRunner(UiScript.Parse(["1\tkernel-tell\tCredits"]), new InputQueue(), new Screen(8, 8)).BeforeInput(1));
        Assert.Throws<NotSupportedException>(() => new UiScriptRunner(UiScript.Parse(["1\tsnapshot-output\tx.bmp"]), new InputQueue(), new Screen(8, 8)).BeforeInput(1));
        var input = new InputQueue();
        var runner = new UiScriptRunner(UiScript.Parse(["1\tesc", "2\tesc"]), input, new Screen(8, 8));
        runner.BeforeInput(2); // 1프레임 단계를 놓친 채 2프레임에 온다.
        Assert.True(input.Empty);
        runner.AfterFrame(2);
        Assert.False(runner.Finished);
        Assert.Throws<InvalidOperationException>(runner.RequireFinished);
    }

    /// <summary>항상 같은 좌표를 돌려주는 검사용 컨트롤 조회기.</summary>
    private sealed class FixedLocator(ScreenPoint point) : IUiControlLocator
    {
        /// <inheritdoc />
        public ScreenPoint? ControlPoint(string label) => point;
    }
}
