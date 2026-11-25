using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 제품 Root 미션 흐름과 같은 Motion 전체 부모 설치를 검증한다.
// 2026-11-25 1.035에서 WP-2 제품 Root Path/Start/Motion 설치용으로 추가했다.
namespace Netstorm.Client.Tests;

/// <summary>제품 Root와 같은 효과 묶음으로 Motion 전체 부모를 설치한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>제품 흐름과 같은 순서로 지형·Dais·패킷 장치를 연결한 세션을 만든다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface, IReadOnlyList<RiftTypeDefinition> Definitions) MotionRootScene()
    {
        var (session, types, surface) = SquidPacketScene();
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        session.BuildRegionOwnershipDevices(types);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = _ => { } });
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        return (session, types, surface, MissionFortLoad.LoadTypeDefinitions(startup.Files));
    }

    /// <summary>제품 Root 설치가 시작 본체·부모·지면과 실제 예약·파괴·Metadata를 연결한다.</summary>
    [Fact]
    public void MotionRootInstallsFullAssemblyWithRealDevices()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, _, IReadOnlyList<RiftTypeDefinition> definitions) = MotionRootScene();
        HostMotionEffects effects = session.BuildMotionRootEffects(definitions);
        Assert.NotNull(effects.Cleanup.ReserveShared);
        Assert.NotNull(effects.Cleanup.Ritual);
        Assert.NotNull(effects.VirtualDestroy);
        Assert.True(effects.Cleanup.ReserveShared!());
        Assert.NotNull(effects.Path.Metadata);
        Assert.NotNull(effects.Path.SoundId);
        Assert.NotNull(effects.Path.Assertion);
        int priest = PlacePriest(session, types, 0, 9);
        RawPathStartType metadata = effects.Path.Metadata!(priest);
        Assert.True(metadata.Speed > 0);
        Assert.Equal(0u, effects.Path.SoundId!(priest));
        int contained = new RawSquidFactory(session.Pool, types).CreateContained(2u);
        effects.VirtualDestroy!(contained, 0);
        Assert.Throws<InvalidOperationException>(() => session.Pool.AllocatedBytes(contained));
        RawPathStart starter = session.BuildMotionRootDevices(definitions);
        Assert.Same(starter, session.PathStarter);
        Assert.NotNull(session.PathPriestHalt);
        Assert.NotNull(session.PathCarrierArrival);
        Assert.NotNull(session.PathPriestArrival);
        Assert.NotNull(session.MotionTerrain);
        Assert.Null(session.CurrentPathRuntime(priest));
        session.ReleasePlayerProcesses();
        Assert.Throws<InvalidOperationException>(() => session.PathStarter);
    }

    /// <summary>제품 Root 설치의 입력·음향 갈고리는 명시 거부한다.</summary>
    [Fact]
    public void MotionRootInputAndSoundHooksFailExplicitly()
    {
        (HostSession session, _, _, IReadOnlyList<RiftTypeDefinition> definitions) = MotionRootScene();
        HostMotionEffects effects = session.BuildMotionRootEffects(definitions);
        Assert.Throws<InvalidOperationException>(() => effects.Halt.CarrierPickup!(1));
        Assert.Throws<InvalidOperationException>(() => effects.Carrier.Pickup!(1));
        Assert.Throws<InvalidOperationException>(() => effects.Priest.NeutralReturn!(1));
        Assert.Throws<InvalidOperationException>(() => effects.Path.StepSound!(0, 0, 0, 0));
        Assert.Throws<InvalidOperationException>(() => effects.Path.Retarget!(null!, 1));
        Assert.Throws<InvalidOperationException>(() => effects.Carrier.PlayAt!(0, 0, null!, 0));
        session.ReleasePlayerProcesses();
    }

    /// <summary>제품 Root 설치는 한 번만 허용하고 장치 없이는 거부한다.</summary>
    [Fact]
    public void MotionRootInstallIsSingleAndRequiresDevices()
    {
        (HostSession session, _, _, IReadOnlyList<RiftTypeDefinition> definitions) = MotionRootScene();
        session.BuildMotionRootDevices(definitions);
        Assert.Throws<InvalidOperationException>(() => session.BuildMotionRootDevices(definitions));
        session.ReleasePlayerProcesses();
        var bare = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => bare.BuildMotionRootDevices(definitions));
        bare.ReleasePlayerRecordViews();
    }
}
