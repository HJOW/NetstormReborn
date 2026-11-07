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

    /// <summary>붕괴 실행 상태 초기값이 싱글 플레이인지 검증한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    [Fact]
    public void DecayModeStartsAsSinglePlayer()
    {
        // 원본 Impl.mode 기본값(로컬이 서버 전투 화면)을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.True(world.DecayMode().Server);
        Assert.True(world.DecayMode().Authority);
        Assert.False(world.DecayMode().Editor);
    }

    /// <summary>설치 모드 지정이 세션 전역에 함께 적히는지 검증한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    [Fact]
    public void ConfigureInstallModesSetsModes()
    {
        // 원본 ownerMode·links.battle·mode.editor 대입 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.ConfigureInstallModes(true, true, false, true);
        Assert.True(world.Storage().OwnerMode.Fort);
        Assert.True(world.Storage().OwnerMode.Battle);
        Assert.False(world.Storage().OwnerMode.Challenge);
        Assert.True(world.Storage().Links.Battle);
        Assert.True(world.DecayMode().Editor);
        Assert.True(world.DecayMode().Server);
        Assert.True(world.DecayMode().Authority);
    }

    /// <summary>객체가 생긴 뒤의 늦은 설치 모드 지정을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    [Fact]
    public void ConfigureInstallModesRejectsLateCall()
    {
        // 첫 객체 뒤의 늦은 설치는 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.Storage().Pool.Allocate();
        Assert.Throws<InvalidOperationException>(() => world.ConfigureInstallModes(true, true, false, false));
    }

    /// <summary>프레임 시각 공급이 그대로 저장되는지 검증한다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    [Fact]
    public void PrepareFrameAdvancesTime()
    {
        // 같은 시각만 공급하는 원본 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.PrepareFrame(new FrameTime { Game = 1.0, Wall = 2.0, Delta = 0.5, Number = 7u });
        Assert.Equal(1.0, world.CurrentTime().Game);
        Assert.Equal(2.0, world.CurrentTime().Wall);
        Assert.Equal(0.5, world.CurrentTime().Delta);
        Assert.Equal(7u, world.CurrentTime().Number);
        Assert.Equal(1.0, world.Storage().Processes().State().Now);
    }

    /// <summary>역행 게임 시각을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    [Fact]
    public void PrepareFrameRejectsBackwardTime()
    {
        // 역행 시각은 저장 전에 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.PrepareFrame(new FrameTime { Game = 2.0 });
        Assert.Throws<ArgumentException>(() => world.PrepareFrame(new FrameTime { Game = 1.0 }));
        Assert.Equal(2.0, world.CurrentTime().Game);
    }

    /// <summary>비유한 게임 시각을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    [Fact]
    public void PrepareFrameRejectsNonFiniteTime()
    {
        // 비유한 시각은 역행 여부와 무관하게 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<ArgumentException>(() => world.PrepareFrame(new FrameTime { Game = double.NaN }));
        Assert.Throws<ArgumentException>(() => world.PrepareFrame(new FrameTime { Game = double.PositiveInfinity }));
        Assert.Equal(0.0, world.CurrentTime().Game);
    }

    /// <summary>공유 시각이 있으면 스냅샷 게임 값을 교정하는지 검증한다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    [Fact]
    public void PrepareFrameUsesSharedNow()
    {
        // bound Client 시각은 스냅샷으로 역쓰기하지 않고 현재 값으로 교정한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.Storage().Processes().State().SharedNow = new DoubleBox { Value = 7.0 };
        world.PrepareFrame(new FrameTime { Game = 1.0 });
        Assert.Equal(7.0, world.CurrentTime().Game);
    }

    /// <summary>제어 장부 초기값이 원본과 같은지 검증한다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    [Fact]
    public void BookkeepingStartsWithLocalOwnerOne()
    {
        // 원본 Impl 생성자의 bookkeeping.localOwner=1을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Equal(1, world.Bookkeeping().LocalOwner);
        Assert.False(world.Loading());
        Assert.False(world.Loaded());
    }

    /// <summary>적재 시작이 장부·깊이·진행 상태를 함께 여는지 검증한다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    [Fact]
    public void BeginLoadStartsLoading()
    {
        // 원본 BeginLoad의 장부·진행·깊이 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BeginLoad();
        Assert.True(world.Loading());
        Assert.False(world.Loaded());
        Assert.Equal(1u, world.LoadingDepth());
        Assert.True(world.Bookkeeping().GraphsEnabled);
    }

    /// <summary>중복 적재 시작을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    [Fact]
    public void BeginLoadRejectsDuplicate()
    {
        // 적재 중·완료 뒤의 시작은 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BeginLoad();
        Assert.Throws<InvalidOperationException>(() => world.BeginLoad());
        world.FinishLoadAfterGraph();
        Assert.Throws<InvalidOperationException>(() => world.BeginLoad());
    }

    /// <summary>그래프 뒤 적재 종료가 상태를 닫는지 검증한다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    [Fact]
    public void FinishLoadAfterGraphCompletes()
    {
        // 원본 FinishLoadAfterGraph의 장부·깊이·완료·변경 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BeginLoad();
        Assert.True(world.TakeChanged());
        world.FinishLoadAfterGraph();
        Assert.False(world.Loading());
        Assert.True(world.Loaded());
        Assert.Equal(0u, world.LoadingDepth());
        Assert.True(world.Bookkeeping().GraphsEnabled);
        Assert.True(world.TakeChanged());
    }

    /// <summary>순서 없는 적재 종료를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 적재 구간 이식에서 추가됐다.</summary>
    [Fact]
    public void FinishLoadAfterGraphRejectsOutOfOrder()
    {
        // 시작 전과 완료 뒤의 종료는 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.FinishLoadAfterGraph());
        world.BeginLoad();
        world.FinishLoadAfterGraph();
        Assert.Throws<InvalidOperationException>(() => world.FinishLoadAfterGraph());
    }

    /// <summary>살아 있는 자산이 없으면 빈 목록을 돌려주는지 검증한다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    [Fact]
    public void ObjectsEmptyWhenNoLiveAssets()
    {
        // 빈 풀·void 할당만으로는 표시 대상이 없음을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Empty(world.Objects());
        world.Storage().Pool.Allocate();
        Assert.Empty(world.Objects());
    }

    /// <summary>패치 슬롯 바이트가 스냅샷으로 그대로 읽히는지 검증한다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    [Fact]
    public void ObjectsDecodesLiveSlot()
    {
        // 원본 패치 오프셋(소유자 34·프레임 36/4바이트) 읽기 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        raw[34] = 3;
        WriteSlot(raw, 36, 0x01020304u, 4);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(1.5f), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(-2.25f), 4);
        List<SurfaceSnapshot> objects = world.Objects();
        Assert.Single(objects);
        Assert.Equal(sid, objects[0].Sid);
        Assert.Equal((uint)OriginalConstructors.FirstAssetType, objects[0].Object.Type);
        Assert.Equal(3u, objects[0].Object.Owner);
        Assert.Equal(0x01020304, objects[0].Object.Frame);
        Assert.Equal(1.5f, objects[0].Object.X);
        Assert.Equal(-2.25f, objects[0].Object.Y);
    }

    /// <summary>CD 슬롯 바이트가 스냅샷으로 그대로 읽히는지 검증한다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    [Fact]
    public void ObjectsDecodesLiveSlotCd()
    {
        // 원본 CD 오프셋(소유자 32·프레임 34/1바이트) 읽기 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, Types188(), 97u);
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        raw[32] = 5;
        WriteSlot(raw, 34, 0x7Fu, 1);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(2.5f), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(4.5f), 4);
        List<SurfaceSnapshot> objects = world.Objects();
        Assert.Single(objects);
        Assert.Equal(sid, objects[0].Sid);
        Assert.Equal((uint)OriginalConstructors.FirstAssetType, objects[0].Object.Type);
        Assert.Equal(5u, objects[0].Object.Owner);
        Assert.Equal(0x7F, objects[0].Object.Frame);
        Assert.Equal(2.5f, objects[0].Object.X);
        Assert.Equal(4.5f, objects[0].Object.Y);
    }

    /// <summary>슬롯 바이트를 리틀 엔디언으로 쓰는 검사 보조다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 폭이다.</param>
    private static void WriteSlot(Span<byte> raw, int offset, uint value, int width)
    {
        // 원본 Write의 필드 밖 보존 의미를 검사 범위에서 재현한다.
        for (int i = 0; i < width; i++)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
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
