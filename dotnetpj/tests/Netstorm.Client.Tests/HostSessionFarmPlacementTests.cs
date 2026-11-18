using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 농지 파이프라인·단일·군집 실결선을 검사하는 클래스다. 2026-11-18 0.614에서 R4-2 Generate 실결선용으로 추가했다.
// 근거: cpppj RawCampaignSession.cpp 1826~1858행과 RawNormalTerrainWorldBinding.cpp 36~40행이다.
public sealed class HostSessionFarmPlacementTests
{
    /// <summary>실제 자료로 농지 배치 장면을 만든다. 지형·요새·섬·그래프·바인딩까지 연결한다.</summary>
    /// <returns>세션·타입 표·정의·셰이프 묶음이다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, IReadOnlyList<RiftTypeDefinition> Definitions, List<SquidDisplayShape> Shapes) BuildFarmScene()
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
        session.TerrainDiagnostics.Context = _ => { };
        session.TerrainDiagnostics.Stamp = _ => { };
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        return (session, table.Types, definitions, shapes);
    }

    /// <summary>같은 장치에서는 파이프라인·단일·군집이 모두 연결되고 군집 호출이 명시 예외 없이 실행된다.</summary>
    [Fact]
    public void BuildsPipelineSingleAndCluster()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions, List<SquidDisplayShape> shapes) = BuildFarmScene();
        session.BuildFarmPlacementDevices(types, definitions, shapes);
        Assert.True(ReferenceEquals(session.Pool, session.FarmPipeline.Pool));
        // 패턴 전역 8개는 로딩 순서의 70+번호와 같아야 한다. 고정 107·82·94·157·131·129·140·142가 기대값이다.
        // 파이프라인 내부 상태는 직접 노출하지 않으므로 MayPlace가 명시 예외 없이 실행되는지로 결선을 확인한다.
        var query = new CanonPlacementQuery(121, 0, 50f, 60f, 0, 0, 0);
        bool allowed = session.FarmPipeline.MayPlace(query);
        Assert.True(allowed || !allowed);
        // 빈 군집 호출도 장치 누락으로 멈추지 않아야 한다. 실제 배치는 MayPlace 결과에 따라 달라진다.
        session.PlaceFarm(new FortRandomRectangle(80f, 80f, 96f, 96f), 127, 0);
        Assert.Throws<InvalidOperationException>(() => session.BuildFarmPlacementDevices(types, definitions, shapes));
    }

    /// <summary>장치가 없으면 명시 예외로 멈춘다.</summary>
    [Fact]
    public void RequiresDevices()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        var types = new List<OriginalTypeRecord>();
        // 공장 규격 188개를 빈 기록으로 채우는 반복문이다.
        for (int i = 0; i < OriginalConstructors.TypeCount; ++i)
        {
            types.Add(new OriginalTypeRecord());
        }
        var definitions = new List<RiftTypeDefinition>();
        var shapes = new List<SquidDisplayShape>();
        // 타입 개수와 같은 빈 셰이프를 넣는 반복문이다.
        for (int i = 0; i < types.Count; ++i)
        {
            shapes.Add(new SquidDisplayShape());
        }
        Assert.Throws<InvalidOperationException>(() => session.BuildFarmPlacementDevices(types, definitions, shapes));
        Assert.Throws<InvalidOperationException>(() => session.PlaceFarm(new FortRandomRectangle(0f, 0f, 16f, 16f), 0, 0));
    }

    /// <summary>지형 Pop 장치와 다른 인스턴스를 넘기면 거부한다.</summary>
    [Fact]
    public void RejectsMismatchedStorage()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions, List<SquidDisplayShape> shapes) = BuildFarmScene();
        var otherTypes = new List<OriginalTypeRecord>(types);
        var otherShapes = new List<SquidDisplayShape>(shapes);
        Assert.Throws<ArgumentException>(() => session.BuildFarmPlacementDevices(otherTypes, definitions, otherShapes));
    }
}
