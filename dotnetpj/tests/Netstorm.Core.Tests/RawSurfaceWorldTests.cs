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

    /// <summary>변경 요청을 한 번만 소비한다. 2026-11-07 F2d 월드 본체 TakeChanged 이식에서 추가됐다.</summary>
    [Fact]
    public void TakeChangedConsumesOnce()
    {
        // 생성 직후 초기값 true를 한 번만 소비하는 원본 std::exchange 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.True(world.TakeChanged());
        Assert.False(world.TakeChanged());
        Assert.False(world.TakeChanged());
    }

    /// <summary>수정 가능한 TYPE 보기가 같은 목록 참조인지 검증한다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.</summary>
    [Fact]
    public void MutableTypesReturnsSameReference()
    {
        // 원본 span이 새 사본이 아닌 같은 전역 TYPE임을 검증한다.
        var types = Types188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u);
        Assert.Same(types, world.MutableTypes());
        Assert.Same(world.Types(), world.MutableTypes());
    }

    /// <summary>수정 가능 보기의 쓰기가 읽기 보기에도 보이는지 검증한다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.</summary>
    [Fact]
    public void MutableTypesMutationVisibleThroughTypes()
    {
        // 원본 옵션 소비자의 쓰기가 같은 TYPE에 반영되는 공유 의미를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.MutableTypes()[3].Flags1 = 0x1234u;
        Assert.Equal(0x1234u, world.Types()[3].Flags1);
        var replacement = new OriginalTypeRecord { Flags1 = 0x5678u };
        world.MutableTypes()[0] = replacement;
        Assert.Same(replacement, world.Types()[0]);
    }

    /// <summary>수정 불가능한 목록의 수정 가능 보기를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.</summary>
    [Fact]
    public void MutableTypesRejectsNonList()
    {
        // IList가 아닌 읽기 전용 목록은 명시 오류로 멈춘다(결정 6).
        var readOnly = new NonListTypes(Types188());
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, readOnly, 97u);
        Assert.Throws<InvalidOperationException>(() => world.MutableTypes());
    }

    /// <summary>로딩 깊이 초기값이 원본과 같은지 검증한다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadingDepthStartsAtOne()
    {
        // 원본 Impl.loadState{1,false}의 초기 깊이를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Equal(1u, world.LoadingDepth());
    }

    /// <summary>로딩 깊이 쓰기가 그대로 읽히는지 검증한다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    [Fact]
    public void SetLoadingDepthStoresValue()
    {
        // 원본 loadState.loadingDepth 대입 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.SetLoadingDepth(5u);
        Assert.Equal(5u, world.LoadingDepth());
        world.SetLoadingDepth(0u);
        Assert.Equal(0u, world.LoadingDepth());
    }

    // IList를 구현하지 않는 읽기 전용 TYPE 목록이다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.
    private sealed class NonListTypes : IReadOnlyList<OriginalTypeRecord>
    {
        // 감싼 실제 목록이다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.
        private readonly IReadOnlyList<OriginalTypeRecord> _inner;

        /// <summary>읽기 전용 목록을 감싼다.</summary>
        /// <param name="inner">감쌀 실제 목록이다.</param>
        public NonListTypes(IReadOnlyList<OriginalTypeRecord> inner)
        {
            _inner = inner;
        }

        /// <summary>기록 개수다.</summary>
        public int Count => _inner.Count;

        /// <summary>번호로 기록을 읽는다.</summary>
        /// <param name="index">읽을 번호다.</param>
        /// <returns>해당 번호의 기록이다.</returns>
        public OriginalTypeRecord this[int index] => _inner[index];

        /// <summary>기록을 순서대로 열거한다.</summary>
        /// <returns>기록 열거자다.</returns>
        public IEnumerator<OriginalTypeRecord> GetEnumerator() => _inner.GetEnumerator();

        /// <summary>기록을 순서대로 열거한다.</summary>
        /// <returns>기록 열거자다.</returns>
        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator() => _inner.GetEnumerator();
    }
}
