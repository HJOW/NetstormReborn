using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using System.Buffers.Binary;

namespace Netstorm.Client.Tests;

// 지역 칠하기·소유권 세션 결선을 검사하는 클래스다. 2026-11-19 0.619에서 R4-2 세션 연결용으로 추가했다.
// 근거: cpppj RawCampaignSession.cpp 1266~1267행의 ownership 조립이다.
public sealed class HostSessionRegionOwnershipTests
{
    /// <summary>지역 도장이 실제 에지 팜 가상 소유자를 사용해 중립을 보존한다.</summary>
    [Fact]
    public void RegionPaintKeepsEdgeFarmNeutral()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildRegionScene();
        session.BuildRegionOwnershipDevices(types, () => 1u);
        var hash = Assert.IsType<RawSquidHash>(session.TerrainPopHash);
        int ground = session.Pool.Allocate(SidAllocation.Client), edge = session.Pool.Allocate(SidAllocation.Client);
        int frame = Enumerable.Range(0, session.WorldFrames[97].Codes.Count)
            .First(index => session.WorldFrames[97].Codes[index].Side == (byte)'A' && session.WorldFrames[97].Codes[index].Variant == (byte)'A');
        Span<byte> terrain = session.Pool.AllocatedBytes(ground); terrain.Clear(); terrain[10] = 97;
        BinaryPrimitives.WriteSingleLittleEndian(terrain[14..], 10f); BinaryPrimitives.WriteSingleLittleEndian(terrain[18..], 10f);
        BinaryPrimitives.WriteInt32LittleEndian(terrain[36..], frame);
        Span<byte> farm = session.Pool.AllocatedBytes(edge); farm.Clear(); farm[10] = 121;
        BinaryPrimitives.WriteUInt32LittleEndian(farm, 0x50a8e8u);
        BinaryPrimitives.WriteSingleLittleEndian(farm[14..], 9f); BinaryPrimitives.WriteSingleLittleEndian(farm[18..], 10f);
        BinaryPrimitives.WriteInt32LittleEndian(farm[36..], frame);
        hash.Bucket(0, 10f, 10f) = (ushort)ground; hash.Bucket(1, 9f, 10f) = (ushort)edge;
        session.RegionPaint.Paint(new RegionOwnershipPaint(10, 10, 1, 0));
        Assert.Equal(1, session.Pool.Slot(ground)[34]);
        Assert.Equal(0, session.Pool.Slot(edge)[34]);
    }

    /// <summary>실제 자료로 지역 투표 장면을 만든다. 지형·요새 장치를 같은 인스턴스로 연결한다.</summary>
    /// <returns>세션과 같은 실제 TYPE 표다.</returns>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types) BuildRegionScene()
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
        // 가상 Owner의 섬 갈고리가 읽는 바인딩이다. 섬이 비어 있어 두 갈고리는 호출되지 않는다.
        // 2026-11-19 0.621에서 지역 가상 Owner 결선용으로 이었다.
        // 2026-11-19 0.622에서 사원 자산 후처리 결선용으로 섬 후처리·플레이어 월드를 이었다.
        session.BuildTerrainBinding(table.Types, session.TerrainDiagnostics, new IslandRuntimeHooks
        {
            SetResidenceOwner = (_, _) => { },
            Assertion = _ => { },
        });
        return (session, table.Types);
    }

    /// <summary>같은 장치에서는 지역 도장·투표·신전 목록이 연결되고 중복 연결을 거부한다.</summary>
    [Fact]
    public void BuildsPaintAndOwnership()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildRegionScene();
        session.BuildRegionOwnershipDevices(types, () => 1u);
        Assert.True(ReferenceEquals(session.Pool, session.RegionOwnership.Pool));
        Assert.NotNull(session.RegionPaint);
        // 신전·별도 목록은 세션 공유 목록(20칸·100칸)을 빌리며 시작 개수는 0이다.
        // 2026-11-19 0.622에서 공유 목록 결선용으로 바꿨다.
        Assert.Same(session.SidLists.Temples, session.RegionTemples);
        Assert.Same(session.SidLists.Outposts, session.RegionAdditional);
        Assert.Equal(0u, session.RegionTemples.Count);
        Assert.Equal(0u, session.RegionAdditional.Count);
        Assert.Throws<InvalidOperationException>(() => session.BuildRegionOwnershipDevices(types, () => 1u));
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

        Assert.Throws<InvalidOperationException>(() => session.BuildRegionOwnershipDevices(types, () => 1u));
        Assert.Throws<InvalidOperationException>(() => session.RegionOwnership);
        Assert.Throws<InvalidOperationException>(() => session.RegionPaint);
        Assert.Throws<InvalidOperationException>(() => session.RegionTemples);
        Assert.Throws<InvalidOperationException>(() => session.RegionAdditional);
        Assert.Throws<InvalidOperationException>(() => session.RegionVortexPostPop);
    }

    /// <summary>지형 Pop 장치와 다른 인스턴스를 넘기면 거부한다.</summary>
    [Fact]
    public void RejectsMismatchedStorage()
    {
        (HostSession session, IReadOnlyList<OriginalTypeRecord> types) = BuildRegionScene();
        var otherTypes = new List<OriginalTypeRecord>(types);
        Assert.Throws<ArgumentException>(() => session.BuildRegionOwnershipDevices(otherTypes, () => 1u));
    }
}
