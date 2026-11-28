using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 요청 기하 쓰기와 실제 장치 선택/부분 실패 순서를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>Screen 재생성과 별개인 원문 옵션 창 이동의 결과다.</summary>
public sealed class ClientWindowOptionsTests
{
    /// <summary>같은 크기는 설정 정리만 수행하고 실제 HWND getter를 포함한 모든 창 장치를 생략한다.</summary>
    [Fact]
    public void SameSizeDoesNotSelectAnyWindowDevice()
    {
        var config = new ConfigInterface(null); var state = new ClientWindowOptions { LogicalWidth = 640, LogicalHeight = 480, PresentWidth = 640, PresentHeight = 480 };
        int notices = 0; config.OnChanged += () => notices++;
        state.Interpret(config, false, WindowsClientWindowOptions.CreateHooks(() => throw new IOException("변경 없는 HWND 선택")));
        Assert.Equal(640, config.GetInt("SCREENW")); Assert.Equal(480, config.GetInt("SCREENH")); Assert.Equal(0, notices);
        Assert.Equal(-1, state.WindowWidth); Assert.Equal(-1, state.WindowHeight);
    }
    /// <summary>요청값을 먼저 쓰고 두 사각형 뒤 계산하며 Move의 높이에 폭을 쓰는 원문 효과도 유지한다.</summary>
    [Fact]
    public void ChangedSizeStoresBeforeEffectsAndUsesWidthForMoveHeight()
    {
        var config = new ConfigInterface(null); config.SetInt("SCREENW", 800); config.SetInt("SCREENH", 600);
        var state = new ClientWindowOptions(); var events = new List<string>();
        state.Interpret(config, false, new()
        {
            WindowRectangle = () => { Assert.Equal(800, state.LogicalWidth); Assert.Equal(600, state.PresentHeight); events.Add("window"); return new(-10, 20, 810, 660); },
            ClientRectangle = () => { events.Add("client"); return new(0, 0, 640, 480); },
            Move = (x, y, width, height, repaint) => { events.Add("move"); Assert.Equal((0, 0, 980, 980, true), (x, y, width, height, repaint)); },
        });
        Assert.Equal(new[] { "window", "client", "move" }, events); Assert.Equal(980, state.WindowWidth); Assert.Equal(760, state.WindowHeight);
    }
    /// <summary>실제 사각형 조회 실패는 앞선 요청/설정 쓰기만 남기고 계산/이동을 생략한다.</summary>
    [Fact]
    public void SelectedFailureKeepsEarlierRequestedDimensions()
    {
        var config = new ConfigInterface(null); config.SetInt("SCREENW", 800); config.SetInt("SCREENH", 600); var state = new ClientWindowOptions();
        Assert.Throws<IOException>(() => state.Interpret(config, false, new() { WindowRectangle = () => throw new IOException("실제 조회 실패") }));
        Assert.Equal(800, state.LogicalWidth); Assert.Equal(600, state.LogicalHeight); Assert.Equal(800, state.PresentWidth); Assert.Equal(600, state.PresentHeight);
        Assert.Equal(-1, state.WindowWidth); Assert.Equal(-1, state.WindowHeight);
    }
}
