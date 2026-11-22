using System.Reflection;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제Host가만든후처리부모의공개시점/교체/예외수명을검사한다.
namespace Netstorm.Client.Tests;

/// <summary>기존실제부모를관찰해제품진단API없이외곽대여전달을검증한다.</summary>
public sealed class HostPostPopRuntimeTests
{
    /// <summary>외부장치접근자를제품에추가하지않고현재실제후처리인스턴스만빌린다.</summary>
    private static RawPostPop Parent(HostSession session, string field) => Assert.IsType<RawPostPop>(typeof(HostSession).GetField(field, BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
    /// <summary>반납시에실제Core부모의현재pin이분리됐는지확인한다.</summary>
    private static void AssertUnpinned(RawPostPop parent) => Assert.Null(typeof(RawPostPop).GetField("_runtimePin", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(parent));
    /// <summary>제품과같은실자산표/프레임/형상을실제Host에공급한다.</summary>
    internal static IReadOnlyList<OriginalTypeRecord> Build(HostSession session, RawSurfaceStorage? storage = null, bool initializeAssetTables = false)
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data); var startup = new HostStartupScene(data);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files); session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files); var shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp")));
        if (storage is not null)
        {
            // 실제 장기 Graph 검사는 최초 코드 캐시도 포함하는 제품 자산 초기화 경로를 사용한다.
            if (storage.Types.Count == 0)
            {
                if (initializeAssetTables) storage.InitializeAssetTables(table.Types, session.WorldFrames, shapes);
                else ClientHostGame.InstallMissionStorageTables(storage, table.Types, session.WorldFrames, shapes);
            }
            session.BuildTerrainPopDevices(storage.Types, storage.Shapes, storage.FactoryExecutor(), storage.Hash, storage.Spots);
            session.BuildFortObjectDevices(storage.Types, storage.Frames, storage.FactoryExecutor(), storage.BuildingExecutor());
            session.BuildIslandPostPopDevices(storage.Types, storage.Frames); return storage.Types;
        }
        session.BuildTerrainPopDevices(table.Types, shapes); session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames); return table.Types;
    }
    /// <summary>같은실제공통장부에명시슬롯입력을준비하고상위파생은호출하지않는다.</summary>
    private static int Slot(HostSession session)
    {
        int sid = session.Pool.Allocate(SidAllocation.Client); Span<byte> raw = session.Pool.AllocatedBytes(sid); raw.Clear(); raw[10] = 0;
        session.TerrainBookkeeping.GraphsEnabled = false; session.TerrainBookkeeping.Suppressed = false; return sid;
    }
    /// <summary>공통부모의실제깊이한번감소와장부효과를실행한다.</summary>
    private static void Run(HostSession session, RawPostPop parent, int sid)
    {
        session.TerrainBookkeeping.Depth = 1; parent.PostPop(sid, 1); Assert.Equal(0u, session.TerrainBookkeeping.Depth);
    }

    /// <summary>공개전생성/공개후생성모두현재실제후처리안에서만전체Runtime을보유한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void ActualParentsReceivePinBeforeOrAfterDeviceCreation(bool publishFirst)
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool);
        HostMissionRuntime? runtime = publishFirst ? HostMissionRuntime.Create(session, storage, []) : null;
        _ = Build(session); RawPostPop parent = Parent(session, "_terrainPostPop"); int sid = Slot(session); int calls = 0;
        if (runtime is null)
        {
            parent.BindPatchSelectionRecheck(() => { calls++; Assert.False(session.Gameplay.RuntimeCallActive(session)); }); Run(session, parent, sid);
            runtime = HostMissionRuntime.Create(session, storage, []);
        }
        parent.BindPatchSelectionRecheck(() => { calls++; Assert.True(session.Gameplay.RuntimeCallActive(session)); Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerRecordViews()); });
        Run(session, parent, sid); Assert.Equal(publishFirst ? 1 : 2, calls); Assert.False(session.Gameplay.RuntimeCallActive(session));
        runtime.Dispose(); session.ReleasePlayerRecordViews(); AssertUnpinned(parent);
    }

    /// <summary>그래프교체전부모와늦게태어난live부모도다음실제owner공개/반납을함께따른다.</summary>
    [Fact]
    public void ReplacedGraphAndLateLiveParentFollowCurrentOwner()
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool); IReadOnlyList<OriginalTypeRecord> types = Build(session);
        RawPostPop previous = Parent(session, "_terrainPostPop"); using (var first = HostMissionRuntime.Create(session, storage, []))
        {
            session.BuildWorldGraphDevices(types); session.WireTerrainPostPopGraph(); _ = session.BuildPlayerWorldHooks();
        }
        RawPostPop current = Parent(session, "_terrainPostPop"), live = Parent(session, "_livePostPop"); Assert.NotSame(previous, current); int sid = Slot(session); int calls = 0;
        var second = HostMissionRuntime.Create(session, storage, []);
        // 이전호출자가아직보유한부모도같은현재Runtime pin의함수사본을받아야한다.
        foreach (RawPostPop parent in new[] { previous, current, live })
        {
            parent.BindPatchSelectionRecheck(() => { calls++; Assert.True(session.Gameplay.RuntimeCallActive(session)); }); Run(session, parent, sid);
        }
        Assert.Equal(3, calls); Assert.False(session.Gameplay.RuntimeCallActive(session)); second.Dispose(); session.ReleasePlayerRecordViews();
        AssertUnpinned(previous); AssertUnpinned(current); AssertUnpinned(live);
    }

    /// <summary>후처리실제효과실패는앞선장부쓰기를유지하고외곽종료를마지막복귀까지미룬다.</summary>
    [Fact]
    public void ActualPostPopExceptionReturnsLeaseAfterPreservingPrefix()
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool); _ = Build(session);
        RawPostPop parent = Parent(session, "_terrainPostPop"); int sid = Slot(session); var runtime = HostMissionRuntime.Create(session, storage, []); var failure = new IOException("실제선택재검사실패");
        parent.BindPatchSelectionRecheck(() => { Assert.True(session.Gameplay.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        session.TerrainBookkeeping.Depth = 1; uint before = session.TerrainBookkeeping.GlobalCounts[0]; Assert.Same(failure, Assert.Throws<IOException>(() => parent.PostPop(sid, 1)));
        Assert.Equal(before + 1, session.TerrainBookkeeping.GlobalCounts[0]); Assert.Equal(1u, session.TerrainBookkeeping.Depth); Assert.True(runtime.Released); Assert.False(session.Gameplay.Borrowed()); Assert.False(session.Gameplay.RuntimeCallActive(session));
        session.ReleasePlayerRecordViews(); AssertUnpinned(parent);
    }
}
