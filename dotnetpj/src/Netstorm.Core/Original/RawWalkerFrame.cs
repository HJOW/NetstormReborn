// 두 워커의 공유 카운터와 개별 예약 시각을 같은 부모 프레임에 적용한다.
namespace Netstorm.Core.Original;

/// <summary>워커38의 실제 정지/Step/지면/도착/공간 장치다.</summary>
public sealed class WalkerFrameHooks
{
    /// <summary>현재 Fall 사건이 만든 이동 불가다.</summary>
    public Func<bool>? Immobile { get; set; }
    /// <summary>실제 부모 Halt다.</summary>
    public Func<bool>? Halt { get; set; }
    /// <summary>같은 Runtime의 실제 Step이다.</summary>
    public Func<RawPathSample>? Step { get; set; }
    /// <summary>현재 지면 검사이며 자신을 삭제할 수 있다.</summary>
    public Func<RawPathPoint, bool>? GroundStop { get; set; }
    /// <summary>실제 공통 도착이다.</summary>
    public Action? Arrive { get; set; }
    /// <summary>현재 부모의 실제 Unpop이다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>현재 부모의 실제 Pop이다.</summary>
    public Action<int, RawPathPoint, uint>? Pop { get; set; }
}

/// <summary>cpppj/src/o/RawWalkerFrame.cpp의48b080이다.</summary>
public static class RawWalkerFrame
{
    /// <summary>원문 float 각도 배율이다.</summary>
    private const float AngleScale = 0.0027777778450399637f;
    /// <summary>원문 회전 프레임 배율이다.</summary>
    private const float TurningFrames = 19.0f;
    /// <summary>레인 보행 예약의 실제 double 간격이다.</summary>
    private const double FrameDelay = 0.07999999821186066;
    /// <summary>CRT signed64 절삭 경계다.</summary>
    private const double FtolLimit = 9223372036854775808.0;
    /// <summary>선택한 실제 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("워커 경로의 선택된 실제 경계 미연결");
    /// <summary>원문 float 저장 뒤 +0.5/CRT 절삭의 낮은DWORD로 회전 프레임을 계산한다.</summary>
    private static uint TurningFrame(float angle)
    {
        float scaled = (float)((1.0 - (double)angle * AngleScale) * TurningFrames); double rounded = (double)scaled + 0.5;
        uint result = double.IsFinite(rounded) && rounded >= -FtolLimit && rounded < FtolLimit ? unchecked((uint)(long)rounded) : 0;
        return unchecked(result + 16);
    }
    /// <summary>현재 권한/수명을 확인하며 같은 프레임에서도 좌표가 달라지면 실제 Unpop/Pop한다.</summary>
    public static void Run(RawSidPool pool, int parent, RawPathRuntime runtime, double now, IReadOnlyList<RiftTypeFrames> frames, uint windType, uint rainType, ref uint windCounter, WalkerFrameHooks hooks, PathLifeWatch watch)
    {
        if (pool.Edition != OriginalEdition.Patch1078 || hooks.Immobile is null || hooks.Halt is null || hooks.Step is null || hooks.GroundStop is null || hooks.Arrive is null || hooks.Unpop is null || hooks.Pop is null || watch is null) throw new ArgumentException("워커38 판본/필수 효과 누락");
        if (hooks.Immobile() || !watch.Alive || runtime.Finished || hooks.Halt() || !watch.Alive) return;
        RawPathSample sample = hooks.Step(); if (!watch.Alive) return; if (sample.Finished) { hooks.Arrive(); return; }
        if (hooks.GroundStop(sample.Position) || !watch.Alive) return;
        ReadOnlySpan<byte> raw = pool.Slot(parent); int type = raw[10]; uint current = RawPathSlot.Read(raw, 36), next = current;
        if (type == windType)
        {
            windCounter = unchecked(windCounter + 1) & 3; next = windCounter;
            if (sample.Angle == 90.0f) next += 12; else if (sample.Angle == 180.0f) next += 8; else if (sample.Angle == 270.0f) next += 4; else if (sample.Angle != 0.0f) next = TurningFrame(sample.Angle);
        }
        else if (type == rainType)
        {
            if (runtime.NextFrame <= now)
            {
                byte side;
                if (sample.Angle == 0.0f || sample.Angle == 180.0f) side = (byte)'B'; else if (sample.Angle == 90.0f || sample.Angle == 270.0f) side = (byte)'A'; else return;
                if (type >= frames.Count) throw new ArgumentOutOfRangeException(nameof(parent), "워커38 레인TYPE 범위");
                RiftTypeFrames.LetterRun run = frames[type].Run(side); int frame = unchecked((int)current); if (run.Count == 0) throw new ArgumentOutOfRangeException(nameof(parent), "워커38 레인 보행 구간0");
                next = unchecked((uint)(frame >= run.First && frame < run.First + run.Count ? (frame - run.First + 1) % run.Count + run.First : run.First)); runtime.NextFrame = now + FrameDelay;
            }
        }
        else throw new ArgumentException("워커38 원문 부모 진단553");
        if (sample.Position.X == BitConverter.UInt32BitsToSingle(RawPathSlot.Read(raw, 14)) && sample.Position.Y == BitConverter.UInt32BitsToSingle(RawPathSlot.Read(raw, 18)) && next == current) return;
        hooks.Unpop(parent, 0); if (!watch.Alive) return; RawPathSlot.SetFrame(pool, parent, unchecked((int)next)); hooks.Pop(parent, sample.Position, 0x40);
    }
}
