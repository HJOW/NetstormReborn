using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 세션 지형 공용 후처리의 운반체·피해 분배 결선을 검사한다.
namespace Netstorm.Client.Tests;

// 지형 분배 Pop의 Damageable 가상표 라우팅을 검사하는 클래스다. 2026-11-18 0.608에서 R4-1 장기 장치 결선용으로 추가했다.
// 근거: cpppj RawCampaignSession.cpp 1856행의 MakeNormalTerrainFarmWorldHooks와 RawNormalTerrainWorldBinding.cpp 40행의 SetCarrierPostPop 결선이다.
public sealed class HostSessionTerrainCarrierTests
{
    /// <summary>실제 자료로 지형 분배 장면을 만든다. 농지 Repop이 타는 Damageable 경로까지 연결한다.</summary>
    /// <returns>세션과 타입 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildCarrierScene()
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
        session.BuildWorldGraphDevices(table.Types);
        session.WireTerrainPostPopGraph();
        return (session, table.Types);
    }

    /// <summary>Damageable 가상표(농지 121)의 Repop이 전용 몸체로 분배되어 미복원으로 멈추지 않는지 확인한다.</summary>
    [Fact]
    public void DamageableRepopRoutesThroughCarrierPostPop()
    {
        var (session, types) = BuildCarrierScene();
        int totalsBefore = session.DumpTotals().TotalCost;
        var madeBefore = session.DumpTotalMade();
        uint depthBefore = session.DumpTotals().PostPopDepth;
        // 농지 타입을 공장에서 만든다. 생성자가 쓰는 가상표는 원본과 같은 Damageable 표다.
        int sid = session.BuildTerrainStampHooks(types).Create(121, 1);
        uint vtable = BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(sid));
        Assert.Equal(0x0050A8E8u, vtable);
        // 농지 교체와 같은 호출 플래그(0x40)로 분배 Pop을 실행한다.
        // 최초 등록이므로 공통 장부에 121이 한 번 집계된다(비용 0이라 총계는 그대로다).
        session.BuildTerrainStampHooks(types).Pop(sid, 50f, 60f, 0x40u);
        // 공통 몸체로 끝나므로 총계·깊이는 그대로이며 121 개수만 1씩 는다.
        Assert.Equal(totalsBefore, session.DumpTotals().TotalCost);
        Assert.Equal(depthBefore, session.DumpTotals().PostPopDepth);
        uint[] localAfter = session.DumpTotalMade().Local;
        uint[] builtAfter = session.DumpTotalMade().Built;
        uint[] globalAfter = session.DumpTotalMade().Global;
        Assert.Equal(madeBefore.Local.Length, localAfter.Length);
        Assert.Equal(madeBefore.Built.Length, builtAfter.Length);
        Assert.Equal(madeBefore.Global.Length, globalAfter.Length);
        for (int type = 0; type < localAfter.Length; ++type)
        {
            uint expectedLocal = madeBefore.Local[type] + (type == 121 ? 1u : 0u);
            uint expectedBuilt = madeBefore.Built[type] + (type == 121 ? 1u : 0u);
            uint expectedGlobal = madeBefore.Global[type] + (type == 121 ? 1u : 0u);
            Assert.Equal(expectedLocal, localAfter[type]);
            Assert.Equal(expectedBuilt, builtAfter[type]);
            Assert.Equal(expectedGlobal, globalAfter[type]);
        }
        Assert.Equal(0, session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.VoidBit);
    }

    /// <summary>운반체 몸체 없이 Damageable 가상표를 분배하면 명시 예외로 멈추는지 확인한다.</summary>
    [Fact]
    public void DamageableRepopWithoutCarrierPostPopThrows()
    {
        var (session, types) = BuildCarrierScene();
        int sid = session.BuildTerrainStampHooks(types).Create(121, 1);
        var bare = new RawPostPop(session.Pool, types, new RawPostPopState());
        Assert.Throws<System.InvalidOperationException>(() => bare.Activate(sid, 0x40u));
    }
}
