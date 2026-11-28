using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 실제 이동/표면/표시와 같은 세션의 붕괴 칸 처리를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 자산/지형/사제/이동 조립을 빌려 붕괴 연결의 현재 상태와 반납을 확인한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>풀만 빌리고 Graph·장부는 따로 가진 조합을 같은 실제 Surface로 허용하지 않는다.</summary>
    [Fact]
    public void BridgeDecayRejectsPoolOnlySurface()
    {
        var (session, _, surface, definitions) = MotionRootScene();
        session.BuildMotionRootDevices(definitions);
        Assert.Throws<ArgumentException>(() => session.BuildBridgeDecayDevices(surface.Storage, _ => { }, _ => { }));
        session.ReleasePlayerProcesses();
    }

    /// <summary>고립된 비단단 다리의 실제 수명·통지 뒤에도 같은 장치와 시각을 사용한다.</summary>
    [Fact]
    public void BridgeDecayUsesSameSurfaceMotionAndCurrentClock()
    {
        var (session, types, surface) = WorldDeletionRootScene(sharedNative: true);
        var startup = new HostStartupScene(GameDataLocator.FindDataDirectory() ?? throw new DirectoryNotFoundException("실제 자산 부재"));
        var definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new IslandRuntimeHooks { SetResidenceOwner = (_, _) => { }, Assertion = _ => { } });
        session.BuildRegionOwnershipDevices(types);
        session.BuildMotionRootDevices(definitions);
        session.BuildWorldDeletionRootDevices();
        Assert.Same(session.Graph.Records, surface.Storage.Graph.Records);
        FrameTime observed = default; int clocks = 0;
        session.BuildBridgeDecayDevices(surface.Storage, _ => throw new InvalidOperationException("고립 칸은 균열 소리를 요청하지 않는다"),
            time => { observed = time; clocks++; });
        Assert.Throws<InvalidOperationException>(() => session.BuildBridgeDecayDevices(surface.Storage, _ => { }, _ => { }));
        int sid = session.BuildPlayerWorldHooks().Create!(82, 1);
        Span<byte> raw = session.Pool.AllocatedBytes(sid);
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 20); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 21);
        raw[30] = 0; raw[12] = 7 << 3;
        int frame = Enumerable.Range(0, session.WorldFrames[82].Codes.Count)
            .First(index => (session.WorldFrames[82].Codes[index].Flags & 0x40) == 0);
        BinaryPrimitives.WriteInt32LittleEndian(raw[36..], frame);
        session.Graph.Records[0].Surfaces = 5;
        session.DecayBridgeCell(new FrameTime { Game = 13.25, Number = 47 }, sid);
        Assert.Equal(1, clocks); Assert.Equal(13.25, observed.Game); Assert.Equal(47ul, observed.Number);
        Assert.Equal(6, (session.Pool.Slot(sid)[12] & 0x78) >> 3);
        session.ReleasePlayerProcesses();
        Assert.Throws<InvalidOperationException>(() => session.DecayBridgeCell(default, sid));
    }
}
