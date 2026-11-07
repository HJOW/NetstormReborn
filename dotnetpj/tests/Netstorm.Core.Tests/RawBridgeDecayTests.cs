// 다리 붕괴 몸체와 금간 프레임 판정의 계약을 확인한다.
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 붕괴 몸체 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.
public sealed class RawBridgeDecayTests
{
    /// <summary>효과 호출을 기록한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private sealed class Calls
    {
        public List<(int Sid, uint Flags)> Destroy { get; } = [];
        public List<int> NotifySurface { get; } = [];
        public List<int> Refresh { get; } = [];
        public List<int> Carriers { get; } = [];
        public List<int> CrackSound { get; } = [];
    }

    /// <summary>호출 기록 묶음을 만든다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static (RawBridgeDecayHooks Hooks, Calls Calls) HookSet()
    {
        var calls = new Calls();
        var hooks = new RawBridgeDecayHooks
        {
            Destroy = (sid, flags) => calls.Destroy.Add((sid, flags)),
            NotifySurface = sid => calls.NotifySurface.Add(sid),
            Refresh = sid => calls.Refresh.Add(sid),
            Carriers = sid => calls.Carriers.Add(sid),
            CrackSound = sid => calls.CrackSound.Add(sid),
            Assertion = (_, _, _) => { },
        };
        return (hooks, calls);
    }

    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types()
    {
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToList();
        types[82].Flags1 = TypeFlag1.Surface;
        types[82].Flags2 = TypeFlag2.Bridge;
        types[83].Flags1 = TypeFlag1.Surface;
        types[83].Flags2 = TypeFlag2.Bridge;
        types[84].Flags1 = TypeFlag1.Surface;
        types[84].Flags2 = TypeFlag2.Bridge;
        return types;
    }

    /// <summary>프레임 표를 만든다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> Frames()
    {
        var frames = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([])).ToList();
        frames[82] = new RiftTypeFrames([new RiftFrameCode((byte)'J', (byte)'P', 0, 0)]);
        frames[83] = new RiftTypeFrames([new RiftFrameCode((byte)'J', (byte)'P', 0, 0), new RiftFrameCode((byte)'J', (byte)'P', 10, 0x20)]);
        frames[84] = new RiftTypeFrames([new RiftFrameCode((byte)'J', (byte)'P', 0, 0x40)]);
        return frames;
    }

    /// <summary>다리 슬롯을 만든다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static int Slot(RawSidPool pool, uint type, float x, float y, int life, int graph = 0)
    {
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)type;
        WriteSlot(raw, 36, 0u, 4);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
        WriteSlot(raw, 12, (uint)life * 8u, 2);
        raw[30] = (byte)graph;
        return sid;
    }

    /// <summary>붕괴 몸체를 만든다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static RawBridgeDecay Decay(RawSidPool pool, List<OriginalTypeRecord> types, List<RiftTypeFrames> frames, ushort[] islands, byte[] spots, GraphRecord[] graphs, RawBridgeDecayHooks hooks, BridgeDecayMode? mode = null) =>
        new(pool, types, frames, islands, spots, graphs, () => mode ?? new BridgeDecayMode(), hooks);

    /// <summary>판본·표 크기가 맞지 않으면 거부하는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CtorRejectsBadInputs()
    {
        var (hooks, _) = HookSet();
        var cdPool = new RawSidPool(32768, true, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => Decay(cdPool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks));
        var pool = new RawSidPool(32768);
        Assert.Throws<ArgumentException>(() => Decay(pool, Types(), Frames().GetRange(0, 10), new ushort[65536], new byte[65536], new GraphRecord[255], hooks));
    }

    /// <summary>수명이 1 줄고 이동체 확인이 도는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyLifeReducesLife()
    {
        var pool = new RawSidPool(32768);
        int sid = Slot(pool, 82u, 10f, 10f, 5);
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks).ApplyLife(sid, 1);
        Assert.Equal(4u * 8u, Read(pool.Slot(sid), 12, 2));
        Assert.Contains(sid, calls.NotifySurface);
        Assert.Contains(sid, calls.Carriers);
        Assert.Empty(calls.Destroy);
        Assert.Empty(calls.Refresh);
    }

    /// <summary>금이 가는 경계에서 프레임이 바뀌는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyLifeCracksFrame()
    {
        var pool = new RawSidPool(32768);
        int sid = Slot(pool, 83u, 10f, 10f, 5);
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks).ApplyLife(sid, 1);
        Assert.Equal(1, BitConverter.ToInt32(pool.Slot(sid).Slice(36, 4).ToArray(), 0));
        Assert.Contains(sid, calls.Refresh);
    }

    /// <summary>수명이 0이면 서버에서 판자를 없애는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyLifeZeroDestroysPlank()
    {
        var pool = new RawSidPool(32768);
        int sid = Slot(pool, 82u, 10f, 10f, 1);
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks).ApplyLife(sid, 1);
        Assert.Contains((sid, 0x02000000u), calls.Destroy);
        Assert.Empty(calls.NotifySurface);
    }

    /// <summary>서버가 아니면 0 비트만 쓰고 넘어가는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyLifeZeroKeepsBitsWithoutServer()
    {
        var pool = new RawSidPool(32768);
        int sid = Slot(pool, 82u, 10f, 10f, 1);
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks, new BridgeDecayMode { Server = false }).ApplyLife(sid, 1);
        Assert.Equal(0u, Read(pool.Slot(sid), 12, 2));
        Assert.Contains(sid, calls.NotifySurface);
        Assert.Empty(calls.Destroy);
    }

    /// <summary>빠진 효과를 명시 오류로 알리는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyLifeRejectsMissingHook()
    {
        var pool = new RawSidPool(32768);
        int sid = Slot(pool, 82u, 10f, 10f, 5);
        var hooks = new RawBridgeDecayHooks { Destroy = (_, _) => { }, Refresh = _ => { }, Carriers = _ => { } };
        Assert.Throws<InvalidOperationException>(() => Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks).ApplyLife(sid, 1));
    }

    /// <summary>무효 그래프를 거부하는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CellRejectsInvalidGraph()
    {
        var pool = new RawSidPool(32768);
        int root = Slot(pool, 82u, 10f, 10f, 3, 254);
        var (hooks, _) = HookSet();
        Assert.Throws<ArgumentOutOfRangeException>(() => Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], new GraphRecord[255], hooks).Cell(root));
    }

    /// <summary>작은 그래프를 곧바로 없애는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CellDestroysSmallGraph()
    {
        var pool = new RawSidPool(32768);
        int root = Slot(pool, 82u, 10f, 10f, 3);
        var graphs = new GraphRecord[255];
        graphs[0] = new GraphRecord { Surfaces = 2, InUse = 1 };
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], graphs, hooks).Cell(root);
        Assert.Contains((root, 0u), calls.Destroy);
    }

    /// <summary>단단한 프레임은 건드리지 않는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CellSkipsHardFrame()
    {
        var pool = new RawSidPool(32768);
        int root = Slot(pool, 84u, 10f, 10f, 3);
        var graphs = new GraphRecord[255];
        graphs[0] = new GraphRecord { Surfaces = 9, InUse = 1 };
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], graphs, hooks).Cell(root);
        Assert.Empty(calls.Destroy);
        Assert.Empty(calls.NotifySurface);
        Assert.Equal(3u * 8u, Read(pool.Slot(root), 12, 2));
    }

    /// <summary>이웃이 없으면 수명 1을 줄이는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CellIsolatedDecaysOne()
    {
        var pool = new RawSidPool(32768);
        int root = Slot(pool, 82u, 10f, 10f, 3);
        var graphs = new GraphRecord[255];
        graphs[0] = new GraphRecord { Surfaces = 9, InUse = 1 };
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), new ushort[65536], new byte[65536], graphs, hooks).Cell(root);
        Assert.Equal(2u * 8u, Read(pool.Slot(root), 12, 2));
        Assert.Contains(root, calls.NotifySurface);
    }

    /// <summary>방문 목록의 수명을 목표값에 맞추는지 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CellDecaysVisited()
    {
        var pool = new RawSidPool(32768);
        int root = Slot(pool, 82u, 10f, 10f, 3);
        int neighbor = Slot(pool, 82u, 10f, 9f, 3);
        ushort[] islands = new ushort[65536];
        islands[9 * 256 + 10] = (ushort)neighbor;
        var graphs = new GraphRecord[255];
        graphs[0] = new GraphRecord { Surfaces = 9, InUse = 1 };
        var (hooks, calls) = HookSet();
        Decay(pool, Types(), Frames(), islands, new byte[65536], graphs, hooks).Cell(root);
        Assert.Equal(2u * 8u, Read(pool.Slot(root), 12, 2));
        Assert.Equal(3u * 8u, Read(pool.Slot(neighbor), 12, 2));
        Assert.Empty(calls.Destroy);
        Assert.Contains(root, calls.NotifySurface);
        Assert.Contains(root, calls.Carriers);
    }

    /// <summary>금간 프레임 직접 조회를 검증한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    [Fact]
    public void CrackedFrameFindsCracked()
    {
        var frames = Frames();
        Assert.Equal(1, RawBridgeFrames.CrackedFrame(frames[83], 0, false));
        Assert.Equal(-1, RawBridgeFrames.CrackedFrame(frames[82], 0, false));
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 읽는 검사 보조다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 쓰는 검사 보조다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private static void WriteSlot(Span<byte> raw, int offset, uint value, int width)
    {
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
