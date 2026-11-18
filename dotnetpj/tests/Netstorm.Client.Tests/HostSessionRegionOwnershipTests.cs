using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 지역 칠하기·소유권 세션 결선을 검사하는 클래스다. 2026-11-19 0.619에서 R4-2 세션 연결용으로 추가했다.
// 근거: cpppj RawCampaignSession.cpp 1266~1267행의 ownership 조립이다.
public sealed class HostSessionRegionOwnershipTests
{
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
        Assert.Empty(session.RegionTemples.Entries);
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
