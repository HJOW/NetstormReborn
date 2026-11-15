using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 장기 공통 상태/실행기가 같은 실제 전역과 물리를 재사용하는지 확인한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj RawSurfaceStorage/World의 공통 상태 대여와 현재 장기 실행기 계약을 검증한다.</summary>
public sealed class SurfaceStorageStateCurrentTests
{
    /// <summary>원본 게임 실행 없이 같은 크기의 실제 타입/프레임/셰이프 물리를 준비한다.</summary>
    private static RawSurfaceStorage Storage()
    {
        var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([new((byte)'A', (byte)'0', 0, 0)])).ToArray();
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        return new(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>장부/삭제와 GameAux의 실제 스칼라·정적 목록·세 통계표를 사본 없이 읽고 쓴다.</summary>
    [Fact]
    public void SharedStateUsesActualScalarListsAndTotalMadeOwners()
    {
        var storage = Storage(); var state = storage.PostPopState; var game = storage.GameAux();
        Assert.Same(storage.TotalMade.LocalCounts, state.LocalCounts); Assert.Same(storage.TotalMade.GlobalCounts, state.GlobalCounts);
        Assert.Same(storage.SidLists.Providers, state.Providers); Assert.Same(storage.SidLists.Factories, state.Factories);
        game.TotalCost = 17; state.TotalCost = 99; Assert.Equal(17, state.CurrentTotalCost); state.CurrentTotalCost = -3; Assert.Equal(-3, game.TotalCost); Assert.Equal(99, state.TotalCost);
        game.LostX = BitConverter.UInt32BitsToSingle(0x7fc01234); Assert.Equal(0x7fc01234u, BitConverter.SingleToUInt32Bits(storage.DeletionState.CurrentLostX));
        storage.DeletionState.CurrentLostY = BitConverter.UInt32BitsToSingle(0x80000000); Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(game.LostY));
        UIntBox depth = state.DepthBox; state.Depth = uint.MaxValue; ref uint current = ref state.ActivationDepth;
        current = unchecked(current + 1); Assert.Equal(0u, depth.Value); depth.Value = 0xfedcba98; Assert.Equal(0xfedcba98u, current); Assert.Equal(0xfedcba98u, state.Depth);
    }

    /// <summary>일반 fresh는 현재 장부/깊이/list 객체를 유지하며 이전 짧은 목록 장치를 호출하지 않는다.</summary>
    [Fact]
    public void GeneralFreshKeepsStateIdentityAndDropsShortOwnedListBinding()
    {
        var storage = Storage(); var state = storage.PostPopState; var deletion = storage.DeletionState; var loading = storage.IslandPostPopState; var depth = state.DepthBox;
        var ownerList = state.OwnerFactories[1]; uint used = 1; ownerList.BindCurrent(new()
        {
            Storage = () => [55u], Capacity = () => 1, ReadCount = () => used,
            WriteCount = _ => throw new InvalidOperationException("반납한 이전 Player 장치 호출"),
        });
        state.Depth = 7; state.ProductionDirty = 9; state.AiAttached[1] = true; state.GraphsEnabled = true; state.PendingPlacement = true;
        state.CurrentTotalCost = 25; deletion.RebuildGraph = true; deletion.CurrentLostX = 12;
        using (storage.Acquire())
        {
            Assert.Same(state, storage.PostPopState); Assert.Same(deletion, storage.DeletionState); Assert.Same(loading, storage.IslandPostPopState); Assert.Same(depth, state.DepthBox); Assert.Same(ownerList, state.OwnerFactories[1]);
            Assert.Equal(0u, state.Depth); Assert.Equal(0u, state.ProductionDirty); Assert.False(state.AiAttached[1]); Assert.False(state.GraphsEnabled); Assert.False(state.PendingPlacement);
            Assert.Equal(0, state.CurrentTotalCost); Assert.Equal(0f, deletion.CurrentLostX); Assert.False(deletion.RebuildGraph);
            Assert.Equal(0u, ownerList.Count); Assert.Equal(0u, ownerList.Capacity); Assert.Equal(1u, used);
            Assert.Same(storage.SidLists.Providers, state.Providers); Assert.Same(storage.SidLists.Factories, state.Factories);
        }
    }

    /// <summary>두 실제 World가 장기 상태와 세 실행기를 공유하며 준비 재진입은 현재 장부를 초기화하지 않는다.</summary>
    [Fact]
    public void WorldsReuseStateGraphNeighborsAndOwnerExecutors()
    {
        var storage = Storage(); RawGraph graph; RawSquidNeighbors neighbors; SquidOwner owner;
        using (var first = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        {
            RawWorldCore core = first.Core(); graph = core.Graph; neighbors = core.Neighbors; owner = core.Owner;
            Assert.Same(storage.PostPopState, first.Bookkeeping()); Assert.Same(storage.DeletionState, first.DeletionState());
            Assert.Same(storage.TotalMade.LocalCounts, first.Bookkeeping().LocalCounts); Assert.Same(storage.GraphExecutor(), graph);
            first.Bookkeeping().Depth = 7; first.Bookkeeping().ProductionDirty = 9; first.Bookkeeping().GlobalCounts[74] = 11;
            first.Bookkeeping().CurrentTotalCost = 25; first.DeletionState().RebuildGraph = true;
        }
        using (var next = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        {
            RawWorldCore core = next.Core(); Assert.Same(graph, core.Graph); Assert.Same(neighbors, core.Neighbors); Assert.Same(owner, core.Owner);
            Assert.Equal(7u, core.Bookkeeping.Depth); Assert.Equal(9u, core.Bookkeeping.ProductionDirty); Assert.Equal(11u, core.Bookkeeping.GlobalCounts[74]); Assert.Equal(25, core.Bookkeeping.CurrentTotalCost); Assert.True(next.DeletionState().RebuildGraph);
        }
        using (var fresh = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage))
        {
            Assert.Same(graph, fresh.Core().Graph); Assert.Same(neighbors, fresh.Core().Neighbors); Assert.Same(owner, fresh.Core().Owner);
            Assert.Equal(0u, fresh.Bookkeeping().Depth); Assert.Equal(0u, fresh.Bookkeeping().GlobalCounts[74]); Assert.Equal(0, fresh.Bookkeeping().CurrentTotalCost); Assert.False(fresh.DeletionState().RebuildGraph);
        }
    }

    /// <summary>Template의 현재 DWORD를 World/표면도 직접 읽으며 다른 owner/대여 중 최초 결합은 거부한다.</summary>
    [Fact]
    public void LoadingDepthUsesTheSameTemplateOwnerWithoutResettingIt()
    {
        var storage = Storage(); var globals = new FortSerializationGlobals { Version = 2, LoadingDepth = 9 };
        storage.BindSerializationGlobals(globals); storage.BindSerializationGlobals(globals); Assert.Equal(9u, storage.LoadingDepth);
        Assert.Throws<ArgumentException>(() => storage.BindSerializationGlobals(new())); Assert.Equal(9u, globals.LoadingDepth);
        using (var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, prepared: true))
        {
            Assert.Equal(9u, world.LoadingDepth()); world.SetLoadingDepth(4); Assert.Equal(4u, globals.LoadingDepth);
            globals.LoadingDepth = 7; Assert.Equal(7u, world.LoadingDepth()); Assert.Equal(7u, storage.IslandPostPopState.LoadingDepth);
            Assert.Equal(2u, globals.Version);
        }
        Assert.Equal(7u, globals.LoadingDepth);
        var other = Storage(); using (other.AcquirePrepared()) Assert.Throws<ArgumentException>(() => other.BindSerializationGlobals(new()));
        Assert.Equal(1u, other.LoadingDepth);
    }

    /// <summary>장기 실행기 조회는 대여/초기화/게임 스칼라 변경을 만들지 않고 같은 실제 객체를 돌려준다.</summary>
    [Fact]
    public void ExecutorGettersDoNotAcquireOrResetCurrentMemory()
    {
        var storage = Storage(); storage.Graph.Records[252].Reserved = 0x2345; storage.Graph.Stack[13] = 0x89abcdef; storage.Spots[7] = 0x88; storage.GameAux().TotalCost = 17;
        uint free = storage.Pool.FreeCount; RawGraph graph = storage.GraphExecutor(); var neighbors = storage.NeighborsExecutor(); var owner = storage.OwnerExecutor();
        Assert.Same(graph, storage.GraphExecutor()); Assert.Same(neighbors, storage.NeighborsExecutor()); Assert.Same(owner, storage.OwnerExecutor());
        Assert.False(storage.Borrowed()); Assert.Equal(0ul, storage.Generation()); Assert.Equal(free, storage.Pool.FreeCount);
        Assert.Equal(0x2345, storage.Graph.Records[252].Reserved); Assert.Equal(0x89abcdefu, storage.Graph.Stack[13]); Assert.Equal(0x88, storage.Spots[7]); Assert.Equal(17, storage.GameAux().TotalCost);
        storage.PostPopState.GraphsEnabled = false; graph.Detach(65000, true);
        storage.PostPopState.GraphsEnabled = true; Assert.ThrowsAny<ArgumentException>(() => graph.Detach(65000, false));
    }

    /// <summary>Graph는 최초 Storage 코드 사본을 쓰고 생성 뒤 코드 변경도 기존 실행기의 사본을 바꾸지 않는다.</summary>
    [Fact]
    public void GraphUsesInitialStorageCodesAndCurrentTypeBinding()
    {
        var storage = Storage(); storage.Frames[74] = new([new((byte)'B', (byte)'0', 0, 0)]);
        RawGraph graph = storage.GraphExecutor(); int sid = storage.Pool.Allocate(SidAllocation.Client); storage.Pool.RawSlot(sid).Clear(); storage.Pool.RawSlot(sid)[10] = 74;
        Assert.Equal('A', graph.Frame(sid).Side); storage.Codes[74][0] = new((byte)'C', (byte)'0', 0, 0);
        Assert.Equal('A', graph.Frame(sid).Side);
    }
}
