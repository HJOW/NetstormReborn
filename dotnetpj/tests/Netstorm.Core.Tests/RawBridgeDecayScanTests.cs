// 붕괴 스캔 실행기의 계약을 확인한다.
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using ScanState = Netstorm.Core.Original.BridgeDecayScanState;

namespace Netstorm.Core.Tests;

// 스캔 실행기 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.
public sealed class RawBridgeDecayScanTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types()
    {
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToList();
        types[82].Flags1 = TypeFlag1.Surface;
        types[82].Flags2 = TypeFlag2.Bridge;
        return types;
    }

    /// <summary>번호 슬롯을 만든다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private static int Slot(RawSidPool pool, uint type, byte state = 0, byte extra = 0)
    {
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = state;
        raw[RawSidPool.TypeOffset] = (byte)type;
        raw[40] = extra;
        return sid;
    }

    /// <summary>스캔 실행기를 만든다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    private static RawBridgeDecayScan Scan(RawSidPool pool, List<OriginalTypeRecord> types, ScanState state, List<int> visited, Func<float>? period = null, Func<BridgeDecayMode>? mode = null, Action<int>? cell = null) =>
        new(pool, types, state, period ?? (() => 10f), mode ?? (() => new BridgeDecayMode()), cell ?? (sid => visited.Add(sid)));

    /// <summary>판본·표 크기가 맞지 않으면 거부하는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void CtorRejectsBadInputs()
    {
        var cdPool = new RawSidPool(32768, true, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => Scan(cdPool, Types(), new ScanState(), []));
        var pool = new RawSidPool(32768);
        Assert.Throws<ArgumentException>(() => Scan(pool, Types().GetRange(0, 10), new ScanState(), []));
    }

    /// <summary>적격 칸만 방문하고 기한을 재설정하는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameVisitsEligibleOnly()
    {
        var pool = new RawSidPool(32768);
        int bridge = Slot(pool, 82u);
        Slot(pool, 83u);
        Slot(pool, 82u, 0, 1);
        Slot(pool, 82u, 2);
        var visited = new List<int>();
        var state = new ScanState { First = 15000, Last = 15003, Cursor = 15000, Next = 0.0 };
        Scan(pool, Types(), state, visited).Frame(new FrameTime { Game = 100.0, Delta = 1.0 }, false);
        Assert.Equal([bridge], visited);
        Assert.Equal(15000, state.Cursor);
        Assert.Equal(110.0, state.Next);
    }

    /// <summary>정지 중에는 손대지 않는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FramePausedSkips()
    {
        var pool = new RawSidPool(32768);
        Slot(pool, 82u);
        var visited = new List<int>();
        var state = new ScanState { First = 15000, Last = 15000, Cursor = 15000, Next = 0.0 };
        Scan(pool, Types(), state, visited).Frame(new FrameTime { Game = 100.0, Delta = 1.0 }, true);
        Assert.Empty(visited);
        Assert.Equal(15000, state.Cursor);
        Assert.Equal(0.0, state.Next);
    }

    /// <summary>편집기·비권한에서는 손대지 않는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameModeGateSkips()
    {
        var pool = new RawSidPool(32768);
        Slot(pool, 82u);
        Slot(pool, 82u);
        var visited = new List<int>();
        var editor = new ScanState { First = 15000, Last = 15001, Cursor = 15000, Next = 0.0 };
        Scan(pool, Types(), editor, visited, null, () => new BridgeDecayMode { Editor = true }).Frame(new FrameTime { Game = 100.0, Delta = 1.0 }, false);
        Assert.Empty(visited);
        Assert.Equal(0.0, editor.Next);
        var noAuthority = new ScanState { First = 15000, Last = 15001, Cursor = 15000, Next = 0.0 };
        Scan(pool, Types(), noAuthority, visited, null, () => new BridgeDecayMode { Authority = false }).Frame(new FrameTime { Game = 100.0, Delta = 1.0 }, false);
        Assert.Empty(visited);
        Assert.Equal(0.0, noAuthority.Next);
    }

    /// <summary>예산만큼만 부분 방문하는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameBudgetsPartialRange()
    {
        var pool = new RawSidPool(32768);
        int first = Slot(pool, 82u);
        Slot(pool, 83u);
        Slot(pool, 82u);
        Slot(pool, 83u);
        var visited = new List<int>();
        var state = new ScanState { First = 15000, Last = 15003, Cursor = 15000, Next = 200.0 };
        Scan(pool, Types(), state, visited).Frame(new FrameTime { Game = 100.0, Delta = 5.0 }, false);
        Assert.Equal([first], visited);
        Assert.Equal(15001, state.Cursor);
        Assert.Equal(200.0, state.Next);
    }

    /// <summary>비유한 델타는 방문하지 않는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameNonFiniteDeltaSkips()
    {
        var pool = new RawSidPool(32768);
        Slot(pool, 82u);
        var visited = new List<int>();
        var state = new ScanState { First = 15000, Last = 15000, Cursor = 15000, Next = 200.0 };
        Scan(pool, Types(), state, visited).Frame(new FrameTime { Game = 100.0, Delta = double.NaN }, false);
        Assert.Empty(visited);
        Assert.Equal(15000, state.Cursor);
    }

    /// <summary>칸 처리 예외를 삼키고 진행하는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameSwallowsCellError()
    {
        var pool = new RawSidPool(32768);
        int failing = Slot(pool, 82u);
        int passing = Slot(pool, 82u);
        var visited = new List<int>();
        var state = new ScanState { First = 15000, Last = 15002, Cursor = 15000, Next = 0.0 };
        Scan(pool, Types(), state, visited, null, null, sid => { visited.Add(sid); if (sid == failing) { throw new InvalidOperationException("칸 처리 실패"); } }).Frame(new FrameTime { Game = 100.0, Delta = 1.0 }, false);
        Assert.Equal([failing, passing], visited);
        Assert.Equal(15000, state.Cursor);
    }

    /// <summary>범위 밖 커서를 건너뛰는지 검증한다. 2026-11-07 F2d 붕괴 스캔 이식에서 추가됐다.</summary>
    [Fact]
    public void FrameSkipsOutOfRangeCursor()
    {
        var pool = new RawSidPool(32768);
        var visited = new List<int>();
        var state = new ScanState { First = -1, Last = 1, Cursor = -1, Next = 0.0 };
        Scan(pool, Types(), state, visited).Frame(new FrameTime { Game = 100.0, Delta = 1.0 }, false);
        Assert.Empty(visited);
        Assert.Equal(-1, state.Cursor);
        Assert.Equal(110.0, state.Next);
    }
}
