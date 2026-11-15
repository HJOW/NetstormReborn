using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 장기 물리의 주소와 실제 World 대여의 정상/실패 반납을 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj RawSurfaceStorageTests의 같은 Graph 배열·준비 보존·일반 fresh·생성 실패 경계를 옮긴다.</summary>
public sealed class SurfaceStorageLifetimeCurrentTests
{
    /// <summary>소스 조회/게임 실행 없이 동일 크기의 실제 자산 배열을 준비한다.</summary>
    private static RawSurfaceStorage Storage(int count = 188) => new(OriginalEdition.Patch1078,
        Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToArray(),
        Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray(),
        Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToArray());

    /// <summary>일반 fresh는 모든 지정 값을 초기화하되 장기 실행기가 빌린 물리 객체/배열을 바꾸지 않는다.</summary>
    [Fact]
    public void GeneralAcquireKeepsAllPhysicalOwnersAndFreshCompatibility()
    {
        var storage = Storage(); var graph = storage.Graph; var records = graph.Records; var stack = graph.Stack;
        var totals = storage.TotalMade; var islands = storage.Islands; var chunks = storage.Chunks; var target = storage.BoardTargetState;
        using (storage.Acquire()) { }
        records[0].Reserved = 0x7b23; records[252].Surfaces = 3; stack[11] = 0xa1b2c3d4;
        totals.LocalCounts[255] = 5; totals.LocalSecondaryCounts[255] = 6; totals.GlobalCounts[255] = 7;
        islands.Records[0] = 2; chunks.Mode = 5; chunks.At(0, 0).Island = 3; target.Invalid = new() { X = 1, Y = 2 };
        using (storage.Acquire())
        {
            Assert.Same(graph, storage.Graph); Assert.Same(records, storage.Graph.Records); Assert.Same(stack, storage.Graph.Stack);
            Assert.Same(totals, storage.TotalMade); Assert.Same(islands, storage.Islands); Assert.Same(chunks, storage.Chunks); Assert.Same(target, storage.BoardTargetState);
            Assert.Equal(1, records[0].Reserved); Assert.Equal(0, records[252].Surfaces); Assert.Equal(0u, stack[11]);
            Assert.Equal(0u, totals.LocalCounts[255]); Assert.Equal(0u, totals.LocalSecondaryCounts[255]); Assert.Equal(0u, totals.GlobalCounts[255]);
            Assert.Equal(0, islands.Records[0]); Assert.Equal(1u, chunks.Mode); Assert.Equal(new ChunkMap().At(0, 0).Island, chunks.At(0, 0).Island);
            Assert.Equal(-10000f, target.Invalid.X); Assert.Equal(-10000f, target.Invalid.Y);
        }
        Assert.False(storage.Borrowed()); Assert.Equal(2ul, storage.Generation());
    }

    /// <summary>준비 대여/반납과 중복 거부는 현재 물리·모드·실제 장부 값을 보존한다.</summary>
    [Fact]
    public void PreparedLeaseAndRejectionPreserveCurrentStorage()
    {
        var storage = Storage(); storage.Graph.Records[252].Reserved = 0x2345; storage.Graph.Stack[^1] = 0xa1b2c3d4;
        storage.TotalMade.GlobalCounts[255] = 9; storage.OwnerMode.Challenge = true; storage.Authority().Boss = false; storage.Spots[7] = 0x88;
        using (storage.AcquirePrepared())
        {
            Assert.Throws<InvalidOperationException>(() => storage.Acquire()); Assert.Throws<InvalidOperationException>(() => storage.AcquirePrepared());
            Assert.Equal(0x2345, storage.Graph.Records[252].Reserved); Assert.Equal(0xa1b2c3d4u, storage.Graph.Stack[^1]);
            Assert.Equal(9u, storage.TotalMade.GlobalCounts[255]); Assert.True(storage.OwnerMode.Challenge); Assert.False(storage.Authority().Boss); Assert.Equal(0x88, storage.Spots[7]);
        }
        Assert.Equal(0x2345, storage.Graph.Records[252].Reserved); Assert.Equal(1ul, storage.Generation());
    }

    /// <summary>두 실제 World 사이에 준비 Graph의 기록/stack 주소와 현재 바이트를 유지한다.</summary>
    [Fact]
    public void PreparedWorldReturnsLeaseAndSharesTheSameGraphArrays()
    {
        var storage = Storage(); var records = storage.Graph.Records; var stack = storage.Graph.Stack;
        records[252].Reserved = 0x2345; stack[^1] = 0xa1b2c3d4;
        using (var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        {
            Assert.Same(storage.Types, world.Types()); Assert.Same(storage.Frames, world.Frames());
            Assert.Same(records, world.Graph()!.Records); Assert.Same(stack, world.Graph()!.FloodStack);
            Assert.True(storage.Borrowed());
        }
        Assert.False(storage.Borrowed()); Assert.Equal(0x2345, records[252].Reserved); Assert.Equal(0xa1b2c3d4u, stack[^1]);
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        { Assert.Same(records, next.Graph()!.Records); Assert.Same(stack, next.Graph()!.FloodStack); }
        Assert.False(storage.Borrowed()); Assert.Equal(2ul, storage.Generation());
    }

    /// <summary>대여 뒤 Graph 생성이 실패하면 같은 물리/세대는 유지하고 lease만 반납한다.</summary>
    [Fact]
    public void PreparedConstructionFailureKeepsBytesAndReturnsLease()
    {
        var storage = Storage(187); storage.Graph.Records[252].Reserved = 0x2345; storage.Graph.Stack[13] = 0x89abcdef;
        Assert.ThrowsAny<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true));
        Assert.False(storage.Borrowed()); Assert.Equal(1ul, storage.Generation()); Assert.Equal(0x2345, storage.Graph.Records[252].Reserved); Assert.Equal(0x89abcdefu, storage.Graph.Stack[13]);
        using (storage.AcquirePrepared()) { Assert.Equal(2ul, storage.Generation()); }
    }

    /// <summary>다른 크기/없는 실제 저장소는 준비 대여 전에 거부한다.</summary>
    [Fact]
    public void PreparedAssetMismatchFailsBeforeBorrowing()
    {
        var storage = Storage(); var bad = storage.Types.Take(187).ToArray();
        Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, bad, 97, storage, prepared: true));
        Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, storage.Frames.Take(187).ToArray(), prepared: true));
        Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, prepared: true));
        Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation());
    }
}
