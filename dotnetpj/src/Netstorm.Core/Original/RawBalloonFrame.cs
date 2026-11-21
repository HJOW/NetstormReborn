// 같은 실제 경로/부모의 고도와 현재 시각을 원문 저장 폭으로 갱신한다.
namespace Netstorm.Core.Original;

/// <summary>벌룬42의 실제 Halt/Step/도착/공간/화염 경계다.</summary>
public sealed class BalloonFrameHooks
{
    /// <summary>같은 경로의 실제 Halt다.</summary>
    public Func<bool>? Halt { get; set; }
    /// <summary>같은 Runtime을 갱신하는 실제 Step이다.</summary>
    public Func<RawPathSample>? Step { get; set; }
    /// <summary>완료 뒤 실제 가상 도착이다.</summary>
    public Action? Arrive { get; set; }
    /// <summary>현재 부모의 실제 Unpop이다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>고도 저장 뒤 실제 Pop이다.</summary>
    public Action<int, RawPathPoint, uint>? Pop { get; set; }
    /// <summary>현재 좌표의 실제 화염 소리 요청이다.</summary>
    public Action<float, float>? Flame { get; set; }
}

/// <summary>cpppj/src/o/RawBalloonFrame.cpp의48ad60 전체 분기다.</summary>
public static class RawBalloonFrame
{
    /// <summary>원문 순항 고도다.</summary>
    private const float CruiseHeight = 44.0f;
    /// <summary>원문 최소 구간 길이의 float 값이다.</summary>
    private const float MinimumDuration = 0.0010000000474974513f;
    /// <summary>원문 CRT signed64의 배타적 양수 상한이다.</summary>
    private const double FtolLimit = 9223372036854775808.0;
    /// <summary>선택된 실제 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("벌룬 경로의 선택된 실제 경계 미연결");
    /// <summary>CRT signed64 절삭의 낮은DWORD와 비정상0을 보존한다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value >= FtolLimit || value < -FtolLimit ? 0 : unchecked((int)(long)value);
    /// <summary>now는 원문 const double 참조처럼 Step 뒤 현재 게임 시각을 조회한다.</summary>
    public static void Run(RawSidPool pool, int parent, RawPathRuntime runtime, Func<double> now, uint balloonType, BalloonFrameHooks hooks, PathLifeWatch watch)
    {
        if (runtime.Finished || Need(hooks.Halt)()) return; RawPathSample sample = Need(hooks.Step)(); if (!watch.Alive) return;
        int current = runtime.Current; float remaining = (float)(runtime.PhysicalTime(current) - now());
        double difference = runtime.PhysicalTime(current) - runtime.PhysicalTime(current - 1); float duration = difference < MinimumDuration ? MinimumDuration : (float)difference;
        int oldHeight = pool.Slot(parent)[30], height = -1;
        if (current == 1)
        {
            height = Math.Max(oldHeight, Truncate((double)CruiseHeight - (double)remaining * CruiseHeight / duration));
            if (height == 0 && pool.Slot(parent)[10] == balloonType) Need(hooks.Flame)(RawPathSlot.Position(pool, parent).X, RawPathSlot.Position(pool, parent).Y);
            if (!watch.Alive) return;
        }
        if (current == runtime.Count - 1) height = Math.Min(oldHeight, Truncate((double)remaining * CruiseHeight / duration));
        if (sample.Finished) { Need(hooks.Arrive)(); return; }
        RawPathPoint position = RawPathSlot.Position(pool, parent);
        if (sample.Position.X != position.X || sample.Position.Y != position.Y)
        {
            Need(hooks.Unpop)(parent, 0); if (!watch.Alive) return;
            if (height != -1) pool.AllocatedBytes(parent)[30] = unchecked((byte)height);
            Need(hooks.Pop)(parent, sample.Position, 0x40);
        }
    }
}
