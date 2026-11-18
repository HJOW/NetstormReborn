using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 공장 정기 트리거 세션 결선을 검사하는 클래스다. 2026-11-19 0.625에서 R4-2 공장 정기 트리거용으로 추가했다.
// 근거: cpppj RawCampaignAssetBindings.cpp 73·81~88행의 Factory 조립과 182행의 SetFactoryPostPop 등록이다.
public sealed class HostSessionFactoryTests
{
    /// <summary>실제 자료로 공장 후처리 장면을 만든다. 지형·요새·바인딩·지역·자산을 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildFactoryScene()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        session.BuildPlayerWorldHooks();
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        session.BuildRegionOwnershipDevices(table.Types, () => 1u);
        return (session, table.Types);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.625에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓰는 반복문이다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>같은 장치에서는 공장 본체·경고·정기 예약이 연결된다.</summary>
    [Fact]
    public void BuildsFactoryDevices()
    {
        (HostSession session, _) = BuildFactoryScene();
        Assert.NotNull(session.FactoryPostPop);
        Assert.Empty(session.FactoryWarnings);
        // 공장 가상표는 공장 몸체가 맡고 일반 슬롯은 맡지 않는다.
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x00509148u);
        Assert.True(session.FactoryPostPop.Handles(sid));
        Assert.True(session.TerrainPostPop.HandlesFactory(sid));
        int plain = session.Pool.Allocate(SidAllocation.Client);
        session.Pool.AllocatedBytes(plain).Fill(0);
        Assert.False(session.FactoryPostPop.Handles(plain));
        Assert.False(session.TerrainPostPop.HandlesFactory(plain));
    }

    /// <summary>공장 정기 예약은 실제 프로세스에 부착된다.</summary>
    [Fact]
    public void FactoryRegularReservationAttachesProcess()
    {
        (HostSession session, _) = BuildFactoryScene();
        Assert.NotNull(session.PlayerProcesses);
        Assert.NotNull(session.FactoryPostPop);
        Assert.Empty(session.FactoryWarnings);
        // 공장 가상표는 세션 분배에서도 처리된다.
        int sid = session.Pool.Allocate(SidAllocation.Client);
        Put(session.Pool.AllocatedBytes(sid), 0, 0x00509148u);
        Assert.True(session.TerrainPostPop.HandlesFactory(sid));
    }

    /// <summary>장치가 없으면 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RejectsUnbuiltFactory()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        Assert.Throws<InvalidOperationException>(() => session.FactoryPostPop);
        Assert.Empty(session.FactoryWarnings);
    }
}
