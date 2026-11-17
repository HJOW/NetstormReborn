using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Tests;

// 개발 호스트 미션 흐름의 저장소 풀 전환이 덤프를 바꾸지 않는지 검사한다.
// 실제 자료로 thewarbegins live 장면(지형+요새 객체)을 조립해 풀 바이트를 대조한다.
// 0.595에서 추가됐다. (R4-1 잔여 중 HostSession 별도 풀 제거)
public sealed class HostSessionPoolTransitionTests
{
    /// <summary>실제 자료로 live 장면을 조립한다. 빌린 풀·공장·공간·건물을 쓴다.</summary>
    /// <param name="pool">빌릴 외부 풀. null이면 세션 전용 풀을 만든다.</param>
    /// <param name="factory">빌릴 외부 공장. null이면 세션 전용으로 만든다.</param>
    /// <param name="hash">빌릴 외부 공간 해시. null이면 세션 전용으로 만든다.</param>
    /// <param name="spots">빌릴 외부 점유 지도. null이면 세션 전용으로 만든다.</param>
    /// <param name="building">빌릴 외부 건물 공간. null이면 세션 전용으로 만든다.</param>
    private static HostSession LoadLiveScene(RawSidPool? pool, RawSquidFactory? factory = null, RawSquidHash? hash = null, byte[]? spots = null, RawBuildingSpace? building = null)
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom(), pool: pool);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        session.BuildTerrainPopDevices(table.Types, shapes, factory, hash, spots);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames, factory, building);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        session.LoadFortObjects(fort.Fort, 1u);
        return session;
    }

    /// <summary>다른 풀의 외부 공장은 거부한다.</summary>
    [Fact]
    public void BuildersRejectForeignPoolFactory()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        var types = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
        var shapes = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new SquidDisplayShape()).ToList();
        var foreign = new RawSquidFactory(new RawSidPool(HostSession.MissionPoolCapacity, true), types);
        Assert.Throws<System.ArgumentException>(() => session.BuildTerrainPopDevices(types, shapes, foreign));
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

    /// <summary>저장소 풀 전환 뒤에도 live 장면의 풀·난수가 완전히 같다.</summary>
    [Fact]
    public void StoragePoolKeepsLiveSceneIdentical()
    {
        var own = LoadLiveScene(null);
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        var borrowed = LoadLiveScene(storage.Pool);
        // 같은 별도 풀 전환 전후로 슬롯 사용 수·풀 바이트·난수가 같다.
        Assert.Equal(UsedSlots(own), UsedSlots(borrowed));
        Assert.Equal(own.Pool.Bytes.ToArray(), borrowed.Pool.Bytes.ToArray());
        Assert.Equal(own.Game.State(), borrowed.Game.State());
    }

    /// <summary>저장소 풀 장면은 같은 시드에 결정적이다.</summary>
    [Fact]
    public void StorageBackedScenesAreDeterministic()
    {
        var first = LoadLiveScene(new RawSurfaceStorage(OriginalEdition.Patch1078).Pool);
        var second = LoadLiveScene(new RawSurfaceStorage(OriginalEdition.Patch1078).Pool);
        Assert.Equal(first.Pool.Bytes.ToArray(), second.Pool.Bytes.ToArray());
        Assert.Equal(first.Game.State(), second.Game.State());
    }

    /// <summary>저장소 공장으로 바꿔도 live 장면이 완전히 같다.</summary>
    [Fact]
    public void StorageFactoryKeepsLiveSceneIdentical()
    {
        var own = LoadLiveScene(null);
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        var mission = MissionTable();
        ClientHostGame.InstallMissionStorageTables(storage, mission.Types, mission.WorldFrames, mission.Shapes);
        var borrowed = LoadLiveScene(storage.Pool, storage.FactoryExecutor());
        Assert.Equal(UsedSlots(own), UsedSlots(borrowed));
        Assert.Equal(own.Pool.Bytes.ToArray(), borrowed.Pool.Bytes.ToArray());
        Assert.Equal(own.Game.State(), borrowed.Game.State());
    }

    /// <summary>빌린 공장이 실제로 생성을 주도한다. 표를 바꾸면 바이트가 달라진다.</summary>
    [Fact]
    public void SuppliedFactoryDrivesCreation()
    {
        var own = LoadLiveScene(null);
        var mission = MissionTable();
        // 저장소 표의 ZOrder를 전부 올려 생성 바이트가 달라지게 한다.
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        ClientHostGame.InstallMissionStorageTables(storage, mission.Types, mission.WorldFrames, mission.Shapes);
        foreach (OriginalTypeRecord record in storage.Types)
        {
            record.ZOrder += 1;
        }

        var changed = LoadLiveScene(storage.Pool, storage.FactoryExecutor());
        Assert.NotEqual(own.Pool.Bytes.ToArray(), changed.Pool.Bytes.ToArray());
    }

    /// <summary>풀·공장·공간·건물을 모두 저장소에서 빌려도 live 장면이 완전히 같다.</summary>
    [Fact]
    public void StorageExecutorsKeepLiveSceneIdentical()
    {
        var own = LoadLiveScene(null);
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        var mission = MissionTable();
        ClientHostGame.InstallMissionStorageTables(storage, mission.Types, mission.WorldFrames, mission.Shapes);
        storage.BuildingSpaceState.IslandType = 97u;
        var borrowed = LoadLiveScene(storage.Pool, storage.FactoryExecutor(), storage.Hash, storage.Spots, storage.BuildingExecutor());
        Assert.Equal(UsedSlots(own), UsedSlots(borrowed));
        Assert.Equal(own.Pool.Bytes.ToArray(), borrowed.Pool.Bytes.ToArray());
        Assert.Equal(own.Game.State(), borrowed.Game.State());
    }

    /// <summary>건물 공간의 섬 타입이 HasIsland 판정을 가른다. 동기화가 필수다.</summary>
    [Fact]
    public void BuildingIslandTypeSelectsHasIsland()
    {
        var pool = new RawSidPool(24000, false);
        int island = pool.Allocate(Netstorm.Core.Simulation.SidAllocation.Client);
        System.Span<byte> raw = pool.AllocatedBytes(island);
        raw.Clear();
        raw[Netstorm.Core.Original.RawSidPool.TypeOffset] = 97;
        WriteUint(raw, 14, BitConverter.SingleToUInt32Bits(10.0f));
        WriteUint(raw, 18, BitConverter.SingleToUInt32Bits(10.0f));
        var hash = new Netstorm.Core.Original.RawSquidHash();
        hash.Bucket(0, 10.0f, 10.0f) = unchecked((ushort)island);
        var types = System.Linq.Enumerable.Range(0, 188)
            .Select(_ => new Netstorm.Core.Original.OriginalTypeRecord { FootX = 1, FootY = 1 }).ToArray();
        var space97 = new Netstorm.Core.Original.RawBuildingSpace(pool, hash, types, new Netstorm.Core.Original.BuildingSpaceState { IslandType = 97u, BridgeType = 82u });
        var space0 = new Netstorm.Core.Original.RawBuildingSpace(pool, hash, types, new Netstorm.Core.Original.BuildingSpaceState { IslandType = 0u, BridgeType = 82u });
        Assert.True(space97.HasIsland(10.0f, 10.0f));
        Assert.False(space0.HasIsland(10.0f, 10.0f));
    }

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    private static void WriteUint(System.Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 4바이트를 쓴다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>다른 풀·해시의 건물 공간은 거부한다.</summary>
    [Fact]
    public void BuildersRejectForeignBuilding()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        var mission = MissionTable();
        session.BuildTerrainPopDevices(mission.Types, mission.Shapes);
        var foreignPool = new RawSidPool(HostSession.MissionPoolCapacity, true);
        var foreignBuilding = new RawBuildingSpace(foreignPool, new RawSquidHash(), mission.Types, new BuildingSpaceState { IslandType = 97u, BridgeType = 82u });
        Assert.Throws<System.ArgumentException>(() => session.BuildFortObjectDevices(mission.Types, mission.WorldFrames, null, foreignBuilding));
        var foreignHash = new RawBuildingSpace(session.Pool, new RawSquidHash(), mission.Types, new BuildingSpaceState { IslandType = 97u, BridgeType = 82u });
        Assert.Throws<System.ArgumentException>(() => session.BuildFortObjectDevices(mission.Types, mission.WorldFrames, null, foreignHash));
    }

    /// <summary>해시·점유지 한쪽만 빌리거나 크기가 다르면 거부한다.</summary>
    [Fact]
    public void BuildersRejectMismatchedSpace()
    {
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        var mission = MissionTable();
        List<SquidDisplayShape> shapes = mission.Shapes;
        var hash = new RawSquidHash();
        // 한쪽만 넘기면 거부한다.
        Assert.Throws<System.ArgumentException>(() => session.BuildTerrainPopDevices(mission.Types, shapes, null, hash, null));
        Assert.Throws<System.ArgumentException>(() => session.BuildTerrainPopDevices(mission.Types, shapes, null, null, new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]));
        Assert.Throws<System.ArgumentException>(() => session.BuildTerrainPopDevices(mission.Types, shapes, null, hash, new byte[8]));
    }

    /// <summary>미션 표 3종을 실제 자료에서 읽는다.</summary>
    private static (IReadOnlyList<OriginalTypeRecord> Types, RiftTypeFrames[] WorldFrames, List<SquidDisplayShape> Shapes) MissionTable()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
        return (table.Types, session.WorldFrames, shapes);
    }
}
