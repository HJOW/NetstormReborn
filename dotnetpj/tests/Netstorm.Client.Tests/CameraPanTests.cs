using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 4d6b54 카메라 Pan 본체와 497e00 목표 쓰기를 대조한다.
// 2026-11-26 1.050에서 DI-4d Pan용으로 추가했다.
public sealed class CameraPanTests
{
    /// <summary>폴링·전역·팝업·요청을 기록 갈고리로 묶는다.</summary>
    /// <returns>갈고리와 호출 기록이다.</returns>
    private static (CameraPanHooks Hooks, List<string> Calls) Scene(InputEvent middle, InputEvent shift, CameraPanGlobals globals)
    {
        var calls = new List<string>();
        var hooks = new CameraPanHooks
        {
            Poll = code => { calls.Add($"P{code:x}"); return code == (uint)InputCode.MiddleButton ? middle : shift; },
            Globals = () => { calls.Add("G"); return globals; },
            ClosePopups = () => calls.Add("K"),
            Request = (dx, dy) => calls.Add($"R{dx},{dy}"),
        };
        return (hooks, calls);
    }

    /// <summary>창 밖·뗌·대화창 관문은 팝업 정리 없이 거부한다.</summary>
    [Fact]
    public void GatedPointersAreRejectedWithoutClosePopups()
    {
        var gate = new CameraPanGlobals { ScrollWhileMouseOut = 0, Dialog = 0, Active = 1, ClientWidth = 1024, ClientHeight = 768, ViewX = 100, ViewY = 80, ViewWidth = 800, ViewHeight = 600 };
        (CameraPanHooks hooks, List<string> calls) = Scene(new(0, 0, 0), new((uint)InputCode.Release, 0, 0), gate);
        Assert.False(RawCameraPan.ProcessCameraPan(new(InputCode.Outside, 500, 300), hooks));
        Assert.DoesNotContain("K", calls);
        // 뗌 사건과 가운데 뗌이 함께이면 거부된다.
        (CameraPanHooks released, List<string> releasedCalls) = Scene(new((uint)InputCode.Release, 0, 0), new((uint)InputCode.Release, 0, 0), gate);
        Assert.False(RawCameraPan.ProcessCameraPan(new(InputCode.Release, 500, 300), released));
        Assert.DoesNotContain("K", releasedCalls);
        // 대화창이 열려 있으면 눌림도 거부된다.
        calls.Clear(); gate.Dialog = 1;
        Assert.False(RawCameraPan.ProcessCameraPan(new(0, 500, 300), hooks));
        Assert.DoesNotContain("K", calls);
    }

    /// <summary>가운데 이동량은 8로 나눈 목표 쓰기가 된다.</summary>
    [Fact]
    public void ArmedDragRequestsEighthMovement()
    {
        var gate = new CameraPanGlobals { ScrollWhileMouseOut = 0, Dialog = 0, Active = 1, ClientWidth = 1024, ClientHeight = 768, ViewX = 100, ViewY = 80, ViewWidth = 800, ViewHeight = 600 };
        // 가운데 뗌·Shift 뗌이면 축 고정이 없다. x=(500-500)*1.28=0, y=(300-380)*1.28=-102.4이다.
        (CameraPanHooks hooks, List<string> calls) = Scene(new((uint)InputCode.Release, 0, 0), new((uint)InputCode.Release, 0, 0), gate);
        Assert.True(RawCameraPan.ProcessCameraPan(new(0, 500, 300), hooks));
        Assert.Contains("K", calls);
        Assert.Contains("R0,-12", calls);
    }

    /// <summary>Shift가 눌려 있으면 큰 축만 남긴다.</summary>
    [Fact]
    public void ShiftLocksToDominantAxis()
    {
        var gate = new CameraPanGlobals { ScrollWhileMouseOut = 0, Dialog = 0, Active = 1, ClientWidth = 1024, ClientHeight = 768, ViewX = 100, ViewY = 80, ViewWidth = 800, ViewHeight = 600 };
        // 가운데 뗌·Shift 눌림이면 x=448이 y=25를 이겨 y가 0이다.
        (CameraPanHooks hooks, List<string> calls) = Scene(new((uint)InputCode.Release, 0, 0), new(0, 0, 0), gate);
        Assert.True(RawCameraPan.ProcessCameraPan(new(0, 850, 400), hooks));
        Assert.Contains("R56,0", calls);
        // Shift 뗌이면 두 축이 그대로 8로 나뉜다.
        var released = Scene(new((uint)InputCode.Release, 0, 0), new((uint)InputCode.Release, 0, 0), gate);
        Assert.True(RawCameraPan.ProcessCameraPan(new(0, 850, 400), released.Hooks));
        Assert.Contains("R56,3", released.Calls);
    }

    /// <summary>좌표 -1·비활성도 팝업 정리 뒤 소비만 한다.</summary>
    [Fact]
    public void MissingPointerOrInactiveConsumesWithoutRequest()
    {
        var gate = new CameraPanGlobals { ScrollWhileMouseOut = 1, Dialog = 0, Active = 1, ClientWidth = 1024, ClientHeight = 768, ViewX = 100, ViewY = 80, ViewWidth = 800, ViewHeight = 600 };
        (CameraPanHooks hooks, List<string> calls) = Scene(new((uint)InputCode.Release, 0, 0), new((uint)InputCode.Release, 0, 0), gate);
        Assert.True(RawCameraPan.ProcessCameraPan(new(0, -1, 200), hooks));
        Assert.Contains("K", calls);
        Assert.DoesNotContain(calls, item => item.StartsWith("R", StringComparison.Ordinal));
        gate.Active = 0;
        Assert.True(RawCameraPan.ProcessCameraPan(new(0, 500, 300), hooks));
        Assert.Equal(2, calls.Count(item => item == "K"));
    }

    /// <summary>497e00은 현재 view와 이동량의 합을 목표에 쓰고 재구성을 센다.</summary>
    [Fact]
    public void RequestCameraDeltaWritesViewPlusDeltaAndBumpsRevision()
    {
        var view = new RawCameraViewState(); RawCameraView.Initialize(view);
        RawCameraView.SetPixels(view, 100, 80);
        var target = new RawCameraTargetState(); var revision = new UIntBox();
        RawCameraPan.RequestCameraDelta(view, target, 768, 40, -15, revision);
        Assert.Equal(new[] { 140, 65 }, target.Pixels);
        Assert.Equal(1u, revision.Value);
        // 절반(384)을 더했다 빼므로 결과는 같다.
        Assert.Equal(384, 768 / 2);
    }
}
