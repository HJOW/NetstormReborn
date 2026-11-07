// 서피스 월드 껍데기의 생성·선택·저장소 공유를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 서피스 월드 껍데기를 검증하는 테스트 클래스다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.
public sealed class RawSurfaceWorldTests
{
    /// <summary>188개 타입 목록을 만든다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private static List<OriginalTypeRecord> Types188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();

    /// <summary>비어 있는 TYPE 표를 거부한다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CtorGuardsTypes()
    {
        var types = Types188();
        Assert.Throws<ArgumentNullException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, null!, 97u));
        Assert.Throws<ArgumentException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, [], 97u));
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u);
        Assert.Same(types, world.Types());
    }

    /// <summary>받은 판본·지형·표본을 그대로 돌려준다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void SelectorsReturnConstructionValues()
    {
        var types = Types188();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types, 121u);
        Assert.Equal(OriginalEdition.Cd1072, world.Edition());
        Assert.Equal(121u, world.TerrainType());
        Assert.Same(types, world.Types());
    }

    /// <summary>저장소가 없으면 같은 판본으로 새로 만든다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void StorageAutoCreatedWhenMissing()
    {
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, Types188(), 97u);
        Assert.NotNull(world.Storage());
    }

    /// <summary>준 저장소는 그대로 빌린다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void StorageSharedWhenGiven()
    {
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, storage);
        Assert.Same(storage, world.Storage());
        Assert.Throws<ArgumentNullException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, null!, 97u, storage));
    }
}
