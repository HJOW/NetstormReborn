// 표면 순회와 연결 판정의 계약을 확인한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 표면 순회 모듈을 검증하는 테스트 클래스다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.
public sealed class RawCanonSurfaceWalkTests
{
    /// <summary>방향 문자 코드를 만든다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static RiftFrameCode Code(byte side) => new(side, (byte)'A', 0, 0);

    /// <summary>방향이 맞으면 연결되는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectsAcceptsOpenDirection()
    {
        // 양쪽 'A'는 모든 방향 비트를 가져 0번(북)이 통한다.
        Assert.True(RawBridgeConnects.Connects(Code((byte)'A'), 0u, Code((byte)'A'), 0u, 0));
    }

    /// <summary>방향이 막히면 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectsRejectsClosedDirection()
    {
        // 'M'은 북 비트만 가져 남쪽 확인에서 떨어진다.
        Assert.False(RawBridgeConnects.Connects(Code((byte)'A'), 0u, Code((byte)'M'), 0u, 0));
    }

    /// <summary>포대 타입이 'P'로 바뀌는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectsTreatsEmplacementAsP()
    {
        // 포대는 연결 비트가 없는 'P'라서 첫 마스크에서 떨어진다.
        Assert.False(RawBridgeConnects.Connects(Code((byte)'A'), TypeFlag2.Emplacement, Code((byte)'A'), 0u, 0));
    }

    /// <summary>범위 밖 방향을 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectsRejectsBadDirection()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBridgeConnects.Connects(Code((byte)'A'), 0u, Code((byte)'A'), 0u, 8));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBridgeConnects.Connects(Code((byte)'A'), 0u, Code((byte)'A'), 0u, -1));
    }

    /// <summary>범위 밖 코드를 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectsRejectsBadCode()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBridgeConnects.Connects(Code(0), 0u, Code((byte)'A'), 0u, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBridgeConnects.Connects(Code((byte)'A'), 0u, Code(0), 0u, 0));
    }

    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types()
    {
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToList();
        types[80].Flags1 = TypeFlag1.Surface;
        types[81].Flags1 = TypeFlag1.Surface;
        return types;
    }

    /// <summary>한 코드짜리 프레임 표를 만든다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> Frames() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([new RiftFrameCode((byte)'A', (byte)'A', 0, 0)])).ToList();

    /// <summary>빈 지도를 만든다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static ushort[] Islands() => new ushort[65536];

    /// <summary>빈 spot 지도를 만든다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static byte[] Spots() => new byte[65536];

    /// <summary>원천 슬롯을 만든다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static int Source(RawSidPool pool, uint type, float x, float y)
    {
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)type;
        WriteSlot(raw, 36, 0u, 4);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
        return sid;
    }

    /// <summary>이웃 후보 슬롯을 지도에 올린다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static int Candidate(RawSidPool pool, ushort[] islands, uint type, float x, float y)
    {
        int sid = Source(pool, type, x, y);
        islands[(int)y * 256 + (int)x] = (ushort)sid;
        return sid;
    }

    /// <summary>이웃이 없으면 끝나는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkEndsWhenMapIsEmpty()
    {
        var pool = new RawSidPool(32768);
        int source = Source(pool, 80u, 10f, 10f);
        var walk = new RawCanonSurfaceWalk(pool, Types(), Frames(), Islands(), Spots(), source);
        Assert.Equal(0, walk.Current());
        Assert.Equal(0, walk.Next());
    }

    /// <summary>연결된 이웃을 순서대로 내보내는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkYieldsConnectedNeighbor()
    {
        var pool = new RawSidPool(32768);
        int source = Source(pool, 80u, 10f, 10f);
        ushort[] islands = Islands();
        int first = Candidate(pool, islands, 81u, 10f, 9f);
        var walk = new RawCanonSurfaceWalk(pool, Types(), Frames(), islands, Spots(), source);
        Assert.Equal(first, walk.Current());
        Assert.Equal(0, walk.Next());
        Assert.Equal(0, walk.Current());
        Assert.Equal(first, Assert.Single(walk.State().Returned));
    }

    /// <summary>죽은 후보를 건너뛰는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkSkipsDeadCandidate()
    {
        var pool = new RawSidPool(32768);
        int source = Source(pool, 80u, 10f, 10f);
        ushort[] islands = Islands();
        int dead = Candidate(pool, islands, 81u, 10f, 9f);
        pool.AllocatedBytes(dead)[RawSidPool.StateOffset] = 2;
        var walk = new RawCanonSurfaceWalk(pool, Types(), Frames(), islands, Spots(), source);
        Assert.Equal(0, walk.Current());
    }

    /// <summary>표면이 아닌 후보를 건너뛰는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkSkipsNonSurfaceCandidate()
    {
        var pool = new RawSidPool(32768);
        var types = Types();
        types[81].Flags1 = 0u;
        int source = Source(pool, 80u, 10f, 10f);
        ushort[] islands = Islands();
        Candidate(pool, islands, 81u, 10f, 9f);
        var walk = new RawCanonSurfaceWalk(pool, types, Frames(), islands, Spots(), source);
        Assert.Equal(0, walk.Current());
    }

    /// <summary>자료 크기가 맞지 않으면 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkRejectsMismatchedData()
    {
        var pool = new RawSidPool(32768);
        int source = Source(pool, 80u, 10f, 10f);
        Assert.Throws<ArgumentException>(() => new RawCanonSurfaceWalk(pool, Types(), Frames().GetRange(0, 10), Islands(), Spots(), source));
    }

    /// <summary>원천 타입이 표를 벗어나면 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkRejectsBadSourceType()
    {
        var pool = new RawSidPool(32768);
        int source = Source(pool, 200u, 10f, 10f);
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawCanonSurfaceWalk(pool, Types(), Frames(), Islands(), Spots(), source));
    }

    /// <summary>발자국이 비어 있으면 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkRejectsEmptyFootprint()
    {
        var pool = new RawSidPool(32768);
        var types = Types();
        types[80].FootX = 0;
        int source = Source(pool, 80u, 10f, 10f);
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawCanonSurfaceWalk(pool, types, Frames(), Islands(), Spots(), source));
    }

    /// <summary>배치 모양으로도 순회하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkAcceptsPlacementShape()
    {
        var pool = new RawSidPool(32768);
        var frames = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new PriestPlainCanonType { Frames = new RiftTypeFrames([new RiftFrameCode((byte)'A', (byte)'A', 0, 0)]), DefaultFrame = 0 })
            .ToList();
        var query = new CanonPlacementQuery(80u, 0u, 10f, 10f, 0u, 0u, 0u);
        var cell = new CanonPlacementCell(0, 0, 0, 10f, 10f, 10f, 10f, default);
        var walk = new RawCanonSurfaceWalk(pool, Types(), frames, Islands(), Spots(), query, cell);
        Assert.Equal(0, walk.Current());
    }

    /// <summary>배치 좌표가 지도 밖이면 거부하는지 검증한다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkRejectsOutOfRangeCell()
    {
        var pool = new RawSidPool(32768);
        var frames = Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => new PriestPlainCanonType { Frames = new RiftTypeFrames([new RiftFrameCode((byte)'A', (byte)'A', 0, 0)]), DefaultFrame = 0 })
            .ToList();
        var query = new CanonPlacementQuery(80u, 0u, 300f, 10f, 0u, 0u, 0u);
        var cell = new CanonPlacementCell(0, 0, 0, 300f, 10f, 300f, 10f, default);
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawCanonSurfaceWalk(pool, Types(), frames, Islands(), Spots(), query, cell));
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 쓰는 검사 보조다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private static void WriteSlot(Span<byte> raw, int offset, uint value, int width)
    {
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
