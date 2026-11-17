using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 요새 일반 객체 Pop이 지형 분배 Pop으로 등록되고 전용 파생은 기록만 남기는지 검사한다.
// cpppj/src/client/RawCampaignWorldRuntime.cpp 46행의 world.Pop 결선에 대응한다.
// R4-2 적재 중 postPop의 여섯 번째 단계(요새 일반 Pop)다. 0.607에서 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>요새 일반 Pop의 지원 타입 등록과 전용 파생 건너뜀을 검사한다. 0.607에서 추가됐다.</summary>
public sealed class HostSessionFortPopTests
{
    /// <summary>실제 자료로 적재 장면을 만든다. 지형·요새·섬·그래프 장치를 잇고 그래프 관문을 연다.</summary>
    /// <param name="startup">실제 자료 시작 화면이다.</param>
    /// <param name="fort">적재할 미션 요새다.</param>
    /// <returns>지형 생성이 끝난 세션이다.</returns>
    private static HostSession BuildFortScene(HostStartupScene startup, MissionFortLoadResult fort)
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.BuildIslandPostPopDevices(table.Types, session.WorldFrames);
        session.BuildWorldGraphDevices(table.Types);
        session.WireTerrainPostPopGraph();
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        return session;
    }

    /// <summary>사용한 슬롯 수를 센다. 빈 칸은 건너뛴다.</summary>
    /// <param name="session">셀 세션이다.</param>
    private static int UsedSlots(HostSession session)
    {
        int count = 0;
        // 일반 번호부터 빈 칸을 건너뛰고 실제 슬롯을 세는 반복문이다.
        for (int sid = RawSidPool.FirstOrdinary; sid < session.Pool.Capacity; ++sid)
        {
            if ((session.Pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) != 0) continue;
            ++count;
        }

        return count;
    }

    /// <summary>섬 장치 없이 요새를 적재하면 일반 Pop을 모두 기록만 남긴다.</summary>
    [Fact]
    public void FortObjectsSkipWithoutIslandDevices()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        session.LoadFortObjects(fort.Fort, 1u);
        // 분배 장치가 없으므로 일반 Pop은 모두 기록된다.
        Assert.NotEmpty(session.FortPopSkips);
        Assert.All(session.FortPopSkips, entry => Assert.Contains("missing-devices", entry));
    }

    /// <summary>요새 일반 Pop이 지형 분배로 등록되고 전용 파생은 기록된다.</summary>
    [Fact]
    public void FortGeneralPopRegistersSupportedTypes()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        HostSession first = BuildFortScene(startup, fort);
        int callsBeforeFort = first.TerrainPopCalls.Count;
        int totalsBeforeFort = first.DumpTotals().TotalCost;
        first.LoadFortObjects(fort.Fort, 1u);
        // 요새 객체 중 지원 타입은 분배 Pop으로 등록된다.
        Assert.True(first.TerrainPopCalls.Count > callsBeforeFort);
        Assert.Contains(first.TerrainPopCalls, entry => entry.Contains("island:ready") && entry.Contains("Registered"));
        // 전용 파생이 필요한 종류는 기록만 남는다.
        Assert.Contains(first.FortPopSkips, entry => entry.Contains("dedicated-r6"));
        // 요새 건물 비용이 장부에 누적된다.
        Assert.True(first.DumpTotals().TotalCost != totalsBeforeFort);
        // 같은 입력의 다른 세션과 완전히 같다.
        HostSession second = BuildFortScene(startup, fort);
        second.LoadFortObjects(fort.Fort, 1u);
        Assert.Equal(first.TerrainPopCalls.Count, second.TerrainPopCalls.Count);
        for (int index = 0; index < first.TerrainPopCalls.Count; ++index)
        {
            Assert.Equal(first.TerrainPopCalls[index], second.TerrainPopCalls[index]);
        }

        Assert.Equal(first.FortPopSkips.Count, second.FortPopSkips.Count);
        Assert.Equal(first.Pool.Bytes.ToArray(), second.Pool.Bytes.ToArray());
    }
}
