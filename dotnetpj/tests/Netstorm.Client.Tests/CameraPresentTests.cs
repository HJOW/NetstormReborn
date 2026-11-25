using Netstorm.Client.Display;

namespace Netstorm.Client.Tests;

// 49a081 카메라 Present 본체의 진단 관문·두 목표 제한 순서를 대조한다.
// 2026-11-26 1.048에서 DI-4b 카메라 Present용으로 추가했다.
public sealed class CameraPresentTests
{
    /// <summary>목표·전역·기록 갈고리를 묶는다.</summary>
    /// <returns>목표와 호출 기록이다.</returns>
    private static (RawCameraTargetState Target, List<string> Calls) Scene(int[] limits, int[] pixels, uint diagnostic, int width, int height, int worldCells, RawCameraPresentHooks hooks, List<string> calls)
    {
        var target = new RawCameraTargetState();
        target.Limits[0] = limits[0]; target.Limits[1] = limits[1]; target.Pixels[0] = pixels[0]; target.Pixels[1] = pixels[1];
        RawCameraPresent.PresentCameraTarget(target, new(diagnostic, width, height, worldCells), hooks);
        return (target, calls);
    }

    /// <summary>갈고리가 기록만 남기는 실제 장치다.</summary>
    private static RawCameraPresentHooks RecordingHooks(List<string> calls) => new()
    {
        Flip = () => calls.Add("F"),
        Commit = () => calls.Add("C"),
    };

    /// <summary>진단 1은 뒤집기·제한·적용을 모두 건너뛰고 목표를 그대로 둔다.</summary>
    [Fact]
    public void DiagnosticOneSkipsFlipLimitsAndCommit()
    {
        var calls = new List<string>();
        (RawCameraTargetState target, _) = Scene([1000, 800], [5000, -50], 1, 1024, 768, 256, RecordingHooks(calls), calls);
        Assert.Empty(calls);
        Assert.Equal(new[] { 5000, -50 }, target.Pixels);
    }

    /// <summary>Renderer 한계와 세계 한계를 순서대로 적용한 뒤 전체를 호출한다.</summary>
    [Fact]
    public void LimitsAndWorldMaximumsClampThenCommit()
    {
        var calls = new List<string>();
        // 세계 한계는 256*16-1024=3072와 256*11-768=2048이다.
        (RawCameraTargetState target, _) = Scene([1000, 800], [5000, -50], 0, 1024, 768, 256, RecordingHooks(calls), calls);
        Assert.Equal(new[] { "F", "C" }, calls);
        Assert.Equal(new[] { 1000, 0 }, target.Pixels);
    }

    /// <summary>범위 안 목표는 그대로 두고 뒤집기와 전체를 순서대로 호출한다.</summary>
    [Fact]
    public void WithinRangePassesThrough()
    {
        var calls = new List<string>();
        (RawCameraTargetState target, _) = Scene([1000, 800], [100, 200], 0, 1024, 768, 256, RecordingHooks(calls), calls);
        Assert.Equal(new[] { "F", "C" }, calls);
        Assert.Equal(new[] { 100, 200 }, target.Pixels);
    }

    /// <summary>화면이 세계보다 크면 감긴 상한이 음수가 되어 목표를 0에 둔다.</summary>
    [Fact]
    public void OversizedScreenClampsToOrigin()
    {
        var calls = new List<string>();
        // 세계 한계는 10*16-1024=-864와 10*11-768=-658이며 원문 감김 순서를 보존한다.
        (RawCameraTargetState target, _) = Scene([1000, 800], [100, 200], 0, 1024, 768, 10, RecordingHooks(calls), calls);
        Assert.Equal(new[] { "F", "C" }, calls);
        Assert.Equal(new[] { 0, 0 }, target.Pixels);
    }
}
