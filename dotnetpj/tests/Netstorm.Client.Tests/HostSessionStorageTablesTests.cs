using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 미션 저장소 표 설치(공장 일원화 바탕)가 세션 표와 같은 독립 사본을 두는지 검사한다.
// 0.596에서 추가됐다. (R4-1 잔여 중 공장·실행기 일원화)
public sealed class HostSessionStorageTablesTests
{
    /// <summary>실제 자료로 미션 표 3종을 읽는다.</summary>
    private static (IReadOnlyList<OriginalTypeRecord> Types, IReadOnlyList<RiftTypeFrames> Frames, List<SquidDisplayShape> Shapes) LoadMissionTables()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        var session = new HostSession(new Kernel(), new GameRandom(1u), new CrtRandom());
        session.FillWorldFrames(definitions);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        byte[] shpRaw = startup.Files.ReadAllBytes("d/_shapes.shp");
        var shpDb = new ShapeDatabase(shpRaw);
        List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, shpDb);
        return (table.Types, session.WorldFrames, shapes);
    }

    /// <summary>표 3종을 독립 사본으로 설치하고 풀을 바꾸지 않는다.</summary>
    [Fact]
    public void InstallsIndependentCopiesWithoutTouchingPool()
    {
        var (types, frames, shapes) = LoadMissionTables();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        byte[] before = storage.Pool.Bytes.ToArray();
        ClientHostGame.InstallMissionStorageTables(storage, types, frames, shapes);
        Assert.Equal(types.Count, storage.Types.Count);
        Assert.Equal(frames.Count, storage.Frames.Count);
        Assert.Equal(shapes.Count, storage.Shapes.Count);
        // 풀 바이트는 설치 전후로 같다.
        Assert.Equal(before, storage.Pool.Bytes.ToArray());
        // 독립 사본이므로 원본을 바꿔도 저장소 표는 그대로다.
        int probe = 12;
        int savedFootX = storage.Types[probe].FootX;
        types[probe].FootX = savedFootX + 1;
        Assert.Equal(savedFootX, storage.Types[probe].FootX);
        types[probe].FootX = savedFootX;
    }

    /// <summary>크기 불일치와 중복 설치를 거부한다.</summary>
    [Fact]
    public void RejectsMismatchAndDuplicate()
    {
        var (types, frames, shapes) = LoadMissionTables();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        Assert.Throws<System.ArgumentException>(() => ClientHostGame.InstallMissionStorageTables(storage, types, [.. frames.SkipLast(1)], shapes));
        ClientHostGame.InstallMissionStorageTables(storage, types, frames, shapes);
        Assert.Throws<System.InvalidOperationException>(() => ClientHostGame.InstallMissionStorageTables(storage, types, frames, shapes));
    }
}
