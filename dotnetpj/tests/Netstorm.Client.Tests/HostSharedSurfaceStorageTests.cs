using System.Reflection;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 Client의 준비된 상태가 새 Host/장치 설치/반납을 넘어 같은 물리로 남는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>장부·Graph·모드·로딩 깊이 및 Native 삭제 본체의 동일 소유자를 검증한다.</summary>
public sealed class HostSharedSurfaceStorageTests
{
    /// <summary>실제 Template 깊이를 Process 생성 전에 연결하고 선택되지 않은 전송은 명시 실패하도록 둔다.</summary>
    private static (ClientSurfaceGlobals Surface, CampaignTemplateStorage Templates) Owner(bool native = false)
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var templates = new CampaignTemplateStorage();
        var surface = new ClientSurfaceGlobals(game, new(local), new(), static () => 1000, installDevices: owner =>
        {
            owner.Storage.BindSerializationGlobals(templates.SerializationGlobals);
            var view = new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new(0, 0, 640, 480) };
            owner.Storage.BindDisplayDevices(new() { View = () => view, Suppressed = () => false, Invalidate = (_, _) => throw new IOException("상태 대여에서 dirty 선택 금지") });
            if (!native) return;
            owner.Storage.BindTransmissionDevices(new()
            {
                Broadcast = _ => throw new IOException("상태 대여에서 방송 선택 금지"),
                Connected = _ => throw new IOException("상태 대여에서 접속 조회 금지"),
                Send = (_, _) => throw new IOException("상태 대여에서 송신 선택 금지"),
            });
            owner.Storage.BindProcessDevices(new() { Selected = () => game.Selection.Selected.Value, ClearSelection = () => throw new IOException("상태 대여에서 선택 해제 금지") });
        });
        return (surface, templates);
    }

    /// <summary>준비된 장기 상태를 외곽 호출자가 소유하고 Host는 초기화 없이 빌린다.</summary>
    [Fact]
    public void PreparedBorrowPreservesAllSharedStateAndCurrentLoadingDepth()
    {
        var (surface, templates) = Owner(); RawSurfaceStorage storage = surface.Storage;
        storage.Graph.Records[21].Surfaces = 11; storage.Graph.Records[21].InUse = 7; storage.Graph.Stack[13] = 0xdeadbeefu;
        storage.TotalMade.LocalCounts[70] = 5; storage.TotalMade.LocalSecondaryCounts[71] = 6; storage.TotalMade.GlobalCounts[72] = 7;
        storage.PostPopState.Depth = 9; storage.PostPopState.ProductionDirty = 11; storage.PostPopState.CurrentTotalCost = 123;
        storage.OwnerMode.Fort = true; storage.OwnerMode.Battle = false; storage.OwnerMode.Challenge = true;
        storage.Links.BridgeType = 123; storage.Links.Mission = false; templates.SerializationGlobals.LoadingDepth = 19;
        using var use = storage.AcquirePrepared();
        var session = new HostSession(surface.Kernel, new(17), new(23), sharedSurface: surface, sharedTemplates: templates);
        Assert.Same(surface.Gameplay, session.Gameplay); Assert.Same(surface.Clock.LocalState, session.LocalState); Assert.Same(surface.Clock.GameTime, session.ProcessTime); Assert.Same(storage.Pool, session.Pool); Assert.Same(storage.SidLists, session.SidLists);
        Assert.Same(storage.PostPopState, session.TerrainBookkeeping); Assert.Same(storage.Graph, session.Graph); Assert.Same(storage.TotalMade, session.TotalMade);
        Assert.Same(storage.OwnerMode, session.OwnerMode); Assert.Same(storage.Links, session.BridgeState);
        Assert.Equal(11, session.Graph.Records[21].Surfaces); Assert.Equal(7, session.Graph.Records[21].InUse); Assert.Equal(0xdeadbeefu, session.Graph.Stack[13]);
        Assert.Equal(5u, session.TotalMade.LocalCounts[70]); Assert.Equal(6u, session.TotalMade.LocalSecondaryCounts[71]); Assert.Equal(7u, session.TotalMade.GlobalCounts[72]);
        Assert.Equal(9u, session.PostPopDepth); Assert.Equal(11u, session.ProductionDirty); Assert.Equal(123, session.TotalCost);
        Assert.True(session.OwnerMode.Fort); Assert.False(session.OwnerMode.Battle); Assert.True(session.OwnerMode.Challenge); Assert.Equal(123u, session.BridgeState.BridgeType); Assert.False(session.BridgeState.Mission);
        Assert.Equal(19u, storage.LoadingDepth); session.SetWorldLoadingDepth(23); Assert.Equal(23u, templates.SerializationGlobals.LoadingDepth);
        templates.SerializationGlobals.LoadingDepth = 29;
        Assert.Equal(29u, typeof(HostSession).GetProperty("_worldLoadingDepth", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
        session.TotalCost = 456; Assert.Equal(456, surface.Gameplay.GameAux.TotalCostBox.Value);
        session.ReleasePlayerRecordViews(); Assert.Equal(9u, storage.PostPopState.Depth); Assert.Equal(29u, storage.LoadingDepth); Assert.Equal(456, storage.PostPopState.CurrentTotalCost);
        var next = new HostSession(surface.Kernel, new(31), new(37), sharedSurface: surface, sharedTemplates: templates);
        Assert.Same(storage.PostPopState, next.TerrainBookkeeping); Assert.Equal(9u, next.PostPopDepth); Assert.Equal(0xdeadbeefu, next.Graph.Stack[13]); next.ReleasePlayerRecordViews();
    }

    /// <summary>다른 물리/깊이를 섞는 명시 인자는 Player 초기화보다 먼저 거부한다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)] [InlineData(5)] [InlineData(6)]
    public void RejectsMixedOwnersBeforePlayerInitialization(int mismatch)
    {
        var (surface, templates) = Owner();
        Assert.Throws<ArgumentException>(() => new HostSession(mismatch == 0 ? new Kernel() : surface.Kernel, new(17), new(23),
            sharedGameplay: mismatch == 1 ? new ClientGameplayGlobals() : null, sharedLists: mismatch == 2 ? new RawStateSidLists() : null,
            pool: mismatch == 3 ? new RawSidPool(HostSession.MissionPoolCapacity, true, static () => true) : null,
            sharedTemplates: mismatch == 4 ? new CampaignTemplateStorage() : templates, sharedSurface: surface,
            sharedProcessTime: mismatch == 5 ? new DoubleBox() : null, sharedLocal: mismatch == 6 ? new ClientLocalStateGlobals(surface.Gameplay, new()) : null));
        Assert.False(surface.Gameplay.PlayerRecords.Initialized); Assert.Equal(0u, surface.Gameplay.PlayerRecords.Base); Assert.True(surface.Gameplay.PlayerRecords.Records.IsEmpty); Assert.Equal(0, surface.Kernel.Size()); Assert.False(surface.Gameplay.Borrowed());
    }

    /// <summary>실제 자산으로 조립한 Graph는 같은 장기 실행기이고 Native 삭제만 같은 pre/post 깊이를 빌린다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void ActualWorldDevicesUseSameGraphAndSelectNativeDestroyOnly(bool native)
    {
        var (surface, templates) = Owner(native); RawSurfaceStorage storage = surface.Storage; RawGraphStorage.ResetTable(storage.Graph);
        var session = new HostSession(surface.Kernel, new(17), new(23), sharedSurface: surface, sharedTemplates: templates);
        var types = HostPostPopRuntimeTests.Build(session, storage, initializeAssetTables: true); storage.Graph.Records[21].Reserved = 0x1234; storage.Graph.Stack[13] = 0xdeadbeefu;
        session.BuildWorldGraphDevices(types); session.WireTerrainPostPopGraph(); _ = session.BuildPlayerWorldHooks();
        Assert.Same(storage.GraphExecutor(), typeof(HostSession).GetField("_worldGraph", BindingFlags.Instance | BindingFlags.NonPublic)!.GetValue(session));
        Assert.Equal(0x1234, storage.Graph.Records[21].Reserved); Assert.Equal(0xdeadbeefu, storage.Graph.Stack[13]);
        storage.Graph.Records[21].Surfaces = 13; Assert.Equal(13, session.DumpGraph()[21].Surfaces);
        RawSquidDestroy destroy = Assert.IsType<RawSquidDestroy>(session.PlayerProcesses.Runtime().Destroy);
        if (native) Assert.Same(storage.BaseDestroy(), destroy); else Assert.NotSame(storage.BaseDestroy(), destroy);
        Assert.Throws<ArgumentException>(() => session.BuildTerrainPopDevices(types.ToArray(), storage.Shapes));
        Assert.Throws<ArgumentException>(() => session.BuildFortObjectDevices(types, storage.Frames, new RawSquidFactory(storage.Pool, types)));
        session.ReleasePlayerRecordViews(); Assert.Equal(0xdeadbeefu, storage.Graph.Stack[13]);
    }
}
