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
    /// <summary>실제 자료로 live 장면을 조립한다. 빌린 풀이 있으면 그 풀을 쓴다.</summary>
    /// <param name="pool">빌릴 외부 풀. null이면 세션 전용 풀을 만든다.</param>
    private static HostSession LoadLiveScene(RawSidPool? pool)
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
        session.BuildTerrainPopDevices(table.Types, shapes);
        session.BuildFortObjectDevices(table.Types, session.WorldFrames);
        MissionFortLoadResult fort = MissionFortLoad.Load(startup, "thewarbegins");
        HostFortObjectLiveTests.BuildLiveTerrainFirst(session, fort.Fort, table.Types);
        session.LoadFortObjects(fort.Fort, 1u);
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
}
