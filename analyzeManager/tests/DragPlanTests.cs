namespace Netstorm.AnalyzeManager.Tests;

/// <summary>게임 없이 드래그 경로(경유점·대기)의 계산을 검사한다.</summary>
public sealed class DragPlanTests
{
    /// <summary>경유점이 없으면 시작점에서 도착점까지 10단계이고 마지막 단계가 정확히 도착점이다.</summary>
    [Fact]
    public void WithoutVia_IsOneLegOfTenSteps()
    {
        IReadOnlyList<DragStep> steps = DragPlan.Create(new InputRequest("drag", 100, 200, 300, 100, DurationMs: 1000));
        Assert.Equal(DragPlan.StepsPerLeg, steps.Count);
        Assert.Equal(new DragStep(120, 190, 100), steps[0]);
        Assert.Equal(new DragStep(300, 100, 100), steps[^1]);
        Assert.Equal(1000, DragPlan.TotalMs(steps));
    }

    /// <summary>경유점과 두 대기가 있으면 두 구간 사이에 경유점 대기, 끝에 도착점 대기 단계가 들어간다.</summary>
    [Fact]
    public void WithViaAndHolds_AddsWaitSteps()
    {
        var request = new InputRequest("drag", 100, 100, 100, 100, DurationMs: 500, ViaX: 100, ViaY: 200, HoldViaMs: 700, HoldMs: 300);
        IReadOnlyList<DragStep> steps = DragPlan.Create(request);
        // 첫 구간 10 + 경유점 대기 1 + 둘째 구간 10 + 도착점 대기 1
        Assert.Equal(22, steps.Count);
        Assert.Equal(new DragStep(100, 200, 50), steps[9]);
        Assert.Equal(new DragStep(100, 200, 700), steps[10]);
        Assert.Equal(new DragStep(100, 100, 50), steps[20]);
        Assert.Equal(new DragStep(100, 100, 300), steps[21]);
        Assert.Equal(500 + 700 + 500 + 300, DragPlan.TotalMs(steps));
    }

    /// <summary>대기가 0이면 대기 단계를 만들지 않고, 경유점만 있어도 두 구간이 된다.</summary>
    [Fact]
    public void ZeroHolds_AddNoWaitSteps()
    {
        IReadOnlyList<DragStep> steps = DragPlan.Create(new InputRequest("drag", 0, 0, 40, 0, DurationMs: 100, ViaX: 20, ViaY: 20));
        Assert.Equal(2 * DragPlan.StepsPerLeg, steps.Count);
        Assert.Equal(new DragStep(20, 20, 10), steps[9]);
        Assert.Equal(new DragStep(40, 0, 10), steps[^1]);
    }

    /// <summary>짧은 구간(durationMs 가 10 미만)도 단계마다 최소 1ms 는 기다린다.</summary>
    [Fact]
    public void VeryShortDuration_StillWaitsAtLeastOneMillisecond()
    {
        IReadOnlyList<DragStep> steps = DragPlan.Create(new InputRequest("drag", 0, 0, 9, 9, DurationMs: 5));
        Assert.All(steps, step => Assert.Equal(1, step.DelayMs));
    }

    /// <summary>한 구간의 위치는 시작점에서 도착점으로 단조롭게 가까워진다 (되돌아가지 않는다).</summary>
    [Fact]
    public void Leg_ApproachesTargetMonotonically()
    {
        IReadOnlyList<DragStep> steps = DragPlan.Create(new InputRequest("drag", 500, 400, 100, 90, DurationMs: 400));
        // 인접한 단계마다 x 는 줄고 y 는 줄어든다
        for (int i = 1; i < steps.Count; i++)
        {
            Assert.True(steps[i].X <= steps[i - 1].X);
            Assert.True(steps[i].Y <= steps[i - 1].Y);
        }
    }
}
