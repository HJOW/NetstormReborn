// 벌프41의 회전/보행 프레임과 실제 난수/현재 좌표 발소리를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>벌프 프레임이 빌리는 실제 정지/Step/공간/소리 장치다.</summary>
public sealed class BulfFrameHooks
{
    /// <summary>첫 Fall 사건의 현재 이동 불가다.</summary>
    public Func<bool>? Immobile { get; set; }
    /// <summary>같은 경로의 실제 Halt다.</summary>
    public Func<bool>? Halt { get; set; }
    /// <summary>같은 Runtime/각도를 갱신하는 실제 Step이다.</summary>
    public Func<RawPathSample>? Step { get; set; }
    /// <summary>완료 뒤 실제 공통 도착이다.</summary>
    public Action? Arrive { get; set; }
    /// <summary>현재 부모의 실제 Unpop이다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>현재 부모의 실제 Pop이다.</summary>
    public Action<int, RawPathPoint, uint>? Pop { get; set; }
    /// <summary>같은 공용 난수의 반열린 정수 범위다.</summary>
    public Func<int, int, int>? RandomInt { get; set; }
    /// <summary>현재 부모 좌표/파일명의 실제 위치 소리다.</summary>
    public Action<float, float, string>? Sound { get; set; }
}

/// <summary>cpppj/src/o/RawBulfFrame.cpp의48ba30/48b760이다.</summary>
public static class RawBulfFrame
{
    /// <summary>원문 float1/360 배율이다.</summary>
    private const float AngleScale = 0.0027777778450399637f;
    /// <summary>원문 signed64 절삭 경계다.</summary>
    private const double FtolLimit = 9223372036854775808.0;
    /// <summary>E/F에서 A~D로 돌아올 때 사용하는 원문 중간 프레임이다.</summary>
    private static readonly int[] AfterTurning = [3, 9, 17, 30];
    /// <summary>선택한 실제 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("벌프 경로의 선택된 실제 경계 미연결");
    /// <summary>E/F 구간 선택과 float 저장→+0.5→CRT 절삭의 DWORD 감김을 유지한다.</summary>
    public static int TurningFrame(RiftTypeFrames frames, float angle, uint counterClockwise)
    {
        if (counterClockwise > 1) throw new ArgumentException("벌프 회전 방향은0/1"); RiftTypeFrames.LetterRun run = frames.Run((byte)(counterClockwise != 0 ? 'F' : 'E'));
        float scaled = (float)((double)angle * AngleScale * (double)(run.Count - 1)); double rounded = (double)scaled + 0.5;
        uint integer = double.IsFinite(rounded) && rounded >= -FtolLimit && rounded < FtolLimit ? unchecked((uint)(long)rounded) : 0;
        return unchecked((int)(integer + (uint)run.First));
    }
    /// <summary>원문 이동 불가/완료/회전/보행과 프레임 변경 뒤 현재 난수·발소리를 같은 부모에 적용한다.</summary>
    public static void Run(RawSidPool pool, int parent, RawPathRuntime runtime, IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<SquidDisplayShape> shapes, BulfFrameHooks hooks, PathLifeWatch watch)
    {
        if (pool.Edition != OriginalEdition.Patch1078 || hooks.Immobile is null || hooks.Halt is null || hooks.Step is null || hooks.Arrive is null || hooks.Unpop is null || hooks.Pop is null || watch is null) throw new ArgumentException("벌프41 판본/필수 연결 누락");
        if (hooks.Immobile()) return; float previous = runtime.Angle; if (runtime.Finished || hooks.Halt() || !watch.Alive) return;
        RawPathSample sample = hooks.Step(); if (!watch.Alive) return; if (sample.Finished) { hooks.Arrive(); return; }
        ReadOnlySpan<byte> raw = pool.Slot(parent); int type = raw[10], current = unchecked((int)RawPathSlot.Read(raw, 36));
        if (type >= frames.Count || type >= shapes.Count || current < 0 || current >= shapes[type].Frames.Count) throw new ArgumentOutOfRangeException(nameof(parent), "벌프41 현재TYPE/SHP 프레임 범위");
        RawPathPoint position = RawPathSlot.Position(pool, parent); SquidDisplayFrame shape = shapes[type].Frames[current];
        float footX = (float)((double)position.X + shape.WalkStepX), footY = (float)((double)position.Y + shape.WalkStepY); int direction = runtime.Direction(runtime.Current) / 2, next = current;
        if (previous != sample.Angle)
        {
            bool counterClockwise = sample.Angle < previous || (double)sample.Angle - previous > 180.0;
            next = TurningFrame(frames[type], sample.Angle, counterClockwise ? 1u : 0u); if (next >= frames[type].Codes.Count) throw new ArgumentOutOfRangeException(nameof(parent), "벌프 회전 프레임 원문 진단5bc");
        }
        else if ((double)sample.Angle % 90.0 == 0)
        {
            double dx = (double)sample.Position.X - position.X, dy = (double)sample.Position.Y - position.Y;
            double fx = (double)footX - position.X, fy = (double)footY - position.Y;
            if (fx * fx + fy * fy <= dx * dx + dy * dy)
            {
                if (direction < 0 || direction >= 4) throw new ArgumentOutOfRangeException(nameof(parent), "벌프41 A~D 방향 범위");
                int side = RawPathAnimation.Direction(frames[type], current); if (side == 4 || side == 5) next = AfterTurning[direction];
                RiftTypeFrames.LetterRun run = frames[type].Run((byte)('A' + direction)); if (run.Count == 0) throw new ArgumentOutOfRangeException(nameof(parent), "벌프41 보행 구간0");
                next = unchecked(next + 1) % run.Count + run.First; if (next >= frames[type].Codes.Count) throw new ArgumentOutOfRangeException(nameof(parent), "벌프 보행 프레임 원문 진단5cd");
            }
        }
        if (next == current) return; hooks.Unpop(parent, 0); if (!watch.Alive) return; RawPathSlot.SetFrame(pool, parent, next); hooks.Pop(parent, sample.Position, 0x40); if (!watch.Alive) return;
        if (unchecked((uint)next + 1) % 4 == 0)
        {
            int variant = Need(hooks.RandomInt)(1000, 6000) / 1000; RawPathPoint point = RawPathSlot.Position(pool, parent); Need(hooks.Sound)(point.X, point.Y, FormattableString.Invariant($"bulfWalk{variant}.wav"));
        }
    }
}
