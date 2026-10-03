namespace Netstorm.AnalyzeManager;

/// <summary>드래그 중 커서가 가야 할 한 지점과, 그 지점으로 가기 전에 기다릴 시간.</summary>
/// <param name="X">클라이언트 x</param>
/// <param name="Y">클라이언트 y</param>
/// <param name="DelayMs">이 지점으로 옮기기 전에 기다릴 밀리초 (대기 지점은 같은 좌표로 한 번 더 옮기며 대기만 한다)</param>
public readonly record struct DragStep(int X, int Y, int DelayMs);

/// <summary>
/// 눌러서 끄는 입력의 경로 계산. 순수 계산이라 게임 없이 검사할 수 있다.
/// 경로는 시작점 → (선택) 경유점 → 도착점이고, 각 구간은 <see cref="InputRequest.DurationMs"/> 를 10등분해 나누어 옮긴다.
/// 원본이 이동 중 상태(누른 버튼 위인지 밖인지)를 갱신할 수 있도록 구간마다 여러 단계로 나눈다.
/// 경유점에서는 <see cref="InputRequest.HoldViaMs"/>, 도착점에서는 <see cref="InputRequest.HoldMs"/> 만큼 더 기다린 뒤 버튼을 뗀다.
/// 누른 채 바깥으로 나갔다가 기다리고 돌아와서 떼는 것 같은 버튼 동작 시험에 쓴다.
/// </summary>
public static class DragPlan
{
    /// <summary>한 구간을 나누는 단계 수.</summary>
    public const int StepsPerLeg = 10;

    /// <summary>드래그 경로의 단계 목록을 만든다. 첫 지점(시작점)은 버튼을 누른 직후의 위치이므로 목록에 넣지 않는다.</summary>
    /// <param name="request">드래그 요청 (Kind 는 검사하지 않는다)</param>
    public static IReadOnlyList<DragStep> Create(InputRequest request)
    {
        var steps = new List<DragStep>();
        int delay = Math.Max(1, request.DurationMs / StepsPerLeg);
        (int x, int y) = (request.X, request.Y);
        // 경유점이 있으면 시작점 → 경유점 구간을 먼저 걷고 그 자리에서 기다린다
        if (request.ViaX is int viaX && request.ViaY is int viaY)
        {
            AddLeg(steps, (x, y), (viaX, viaY), delay);
            if (request.HoldViaMs > 0) steps.Add(new DragStep(viaX, viaY, request.HoldViaMs));
            (x, y) = (viaX, viaY);
        }
        AddLeg(steps, (x, y), (request.ToX, request.ToY), delay);
        if (request.HoldMs > 0) steps.Add(new DragStep(request.ToX, request.ToY, request.HoldMs));
        return steps;
    }

    /// <summary>두 지점 사이를 <see cref="StepsPerLeg"/> 단계로 직선 이동하는 단계를 목록에 더한다.</summary>
    private static void AddLeg(List<DragStep> steps, (int X, int Y) from, (int X, int Y) to, int delay)
    {
        // 각 단계의 위치는 정수 나눗셈으로 구해 마지막 단계가 정확히 도착점이 되게 한다
        for (int step = 1; step <= StepsPerLeg; step++)
            steps.Add(new DragStep(from.X + (to.X - from.X) * step / StepsPerLeg, from.Y + (to.Y - from.Y) * step / StepsPerLeg, delay));
    }

    /// <summary>경로를 끝까지 걷는 데 걸리는 총 대기 시간(밀리초). 시험의 누름 지속 시간을 계산할 때 쓴다.</summary>
    public static int TotalMs(IReadOnlyList<DragStep> steps) => steps.Sum(step => step.DelayMs);
}
