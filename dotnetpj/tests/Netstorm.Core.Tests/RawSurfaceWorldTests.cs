// 서피스 월드 껍데기의 생성·선택·저장소 공유를 검증한다.
using Netstorm.Core.Display;
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

    /// <summary>188개 TYPE면 표면 Graph를 갖는지 검증한다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void GraphAvailableForPatchTypes()
    {
        // 원본 전체 풀 재구성용 Graph 소유를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.NotNull(world.Graph());
    }

    /// <summary>188개가 아니면 Graph를 비워 두는지 검증한다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void GraphNullForNon188Types()
    {
        // CD 171개 TYPE의 그래프 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Null(world.Graph());
        world.BeginLoad();
        Assert.Throws<InvalidOperationException>(() => world.FinishLoad());
    }

    /// <summary>적재 종료가 그래프 재구성 뒤 상태를 닫는지 검증한다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void FinishLoadCompletes()
    {
        // 원본 FinishLoad의 재구성·종료 순서를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BeginLoad();
        Assert.True(world.TakeChanged());
        world.FinishLoad();
        Assert.False(world.Loading());
        Assert.True(world.Loaded());
        Assert.Equal(0u, world.LoadingDepth());
        Assert.True(world.TakeChanged());
    }

    /// <summary>순서 없는 적재 종료(그래프 포함)를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 그래프 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void FinishLoadRejectsOutOfOrder()
    {
        // 시작 전과 완료 뒤의 종료는 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.FinishLoad());
        world.BeginLoad();
        world.FinishLoad();
        Assert.Throws<InvalidOperationException>(() => world.FinishLoad());
    }

    /// <summary>같은 삭제 전역 참조를 빌리는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void DeletionStateReturnsSameReference()
    {
        // 후설치와 같은 현재 원본 삭제 전역임을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Same(world.DeletionState(), world.DeletionState());
    }

    /// <summary>삭제 전역 초기값이 원본과 같은지 검증한다. 2026-11-07 F2d 월드 본체 삭제 상태 이식에서 추가됐다.</summary>
    [Fact]
    public void DeletionStateStartsWithDefaults()
    {
        // 공유 좌표 연결은 BindDeletion 단위에서 잇고 기본값만 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Equal(0, world.DeletionState().Selected);
        Assert.Equal(0x50444200u, world.DeletionState().RegionMask);
        Assert.Equal(157, world.DeletionState().SpecialSurfaceType);
        Assert.False(world.DeletionState().RebuildGraph);
    }

    /// <summary>삭제 효과 콜백 묶음을 만든다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    private static SquidDeletionHooks DeletionHooks() =>
        new() { Emit = _ => { }, Forward = _ => { } };

    /// <summary>공통 삭제 소비자 설치가 현재 몸체를 바꾸는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindDeletionBindsOnce()
    {
        // 첫 객체 생성 전의 한 번 연결을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BindDeletion(() => 0, DeletionHooks());
        Assert.NotNull(world.CommonDeletion());
        Assert.Same(world.CommonDeletion(), world.CommonDeletion());
    }

    /// <summary>중복 설치를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindDeletionRejectsDuplicate()
    {
        // 한 번 연결 뒤의 설치는 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BindDeletion(() => 0, DeletionHooks());
        Assert.Throws<InvalidOperationException>(() => world.BindDeletion(() => 0, DeletionHooks()));
    }

    /// <summary>객체가 생긴 뒤의 늦은 설치를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindDeletionRejectsLateCall()
    {
        // 첫 객체 뒤의 늦은 설치는 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.Storage().Pool.Allocate();
        Assert.Throws<InvalidOperationException>(() => world.BindDeletion(() => 0, DeletionHooks()));
    }

    /// <summary>빠진 선택·효과 경계를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindDeletionRejectsMissingHooks()
    {
        // 선택·효과·파생 경계 누락은 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<ArgumentNullException>(() => world.BindDeletion(null, DeletionHooks()));
        Assert.Throws<ArgumentException>(() => world.BindDeletion(() => 0, new SquidDeletionHooks { Forward = _ => { } }));
        Assert.Throws<ArgumentException>(() => world.BindDeletion(() => 0, new SquidDeletionHooks { Emit = _ => { } }));
    }

    /// <summary>설치 전의 현재 몸체 조회를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void CommonDeletionBeforeBindThrows()
    {
        // 미연결 몸체 읽기는 원본처럼 명시 오류로 멈춘다(결정 6).
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.CommonDeletion());
    }

    /// <summary>188개가 아닌 TYPE의 설치를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindDeletionRejectsNon188Types()
    {
        // CD 171개 TYPE의 삭제 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.BindDeletion(() => 0, DeletionHooks()));
    }

    /// <summary>자산 소유자 지정이 슬롯에 적히는지 검증한다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetAssetOwnerWritesOwnerByte()
    {
        // 원본 패치 소유자 위치(0x22) 쓰기와 변경 알림을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        Assert.True(world.TakeChanged());
        world.SetAssetOwner(sid, 3u);
        Assert.Equal(3, world.Storage().Pool.Slot(sid)[34]);
        Assert.True(world.TakeChanged());
    }

    /// <summary>범위 밖 소유자 번호를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetAssetOwnerRejectsBadOwner()
    {
        // 목록이 없는 소유자 번호 지정은 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        Assert.Throws<ArgumentException>(() => world.SetAssetOwner(sid, 9u));
    }

    /// <summary>CD 소유자 위치에 적히는지 검증한다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetAssetOwnerWritesOwnerByteCd()
    {
        // 원본 CD 소유자 위치(0x20) 쓰기를 검증한다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        world.SetAssetOwner(sid, 5u);
        Assert.Equal(5, world.Storage().Pool.Slot(sid)[32]);
    }

    /// <summary>판본과 맞지 않는 TYPE 표의 지정을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 소유자 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetAssetOwnerRejectsMismatchedTypes()
    {
        // 어긋난 기존 껍데기 호출은 명시 오류로 멈춘다(결정 6).
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.SetAssetOwner(5, 3u));
    }

    /// <summary>188개 프레임 목록을 만든다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> Frames188() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new RiftTypeFrames([])).ToList();

    /// <summary>받은 프레임 표를 그대로 돌려준다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    [Fact]
    public void FramesReturnsConstructionValues()
    {
        // 생성 때 받은 같은 장기 프레임 표 정체성을 검증한다.
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, frames);
        Assert.Same(frames, world.Frames());
    }

    /// <summary>프레임 표를 주지 않으면 빈 목록을 두는지 검증한다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    [Fact]
    public void FramesEmptyWhenMissing()
    {
        // 기존 껍데기 호출과의 호환을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Empty(world.Frames());
    }

    /// <summary>수정 가능한 프레임 보기가 같은 목록 참조인지 검증한다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    [Fact]
    public void MutableFramesReturnsSameReference()
    {
        // 원본 span이 새 사본이 아닌 같은 장기 표임을 검증한다.
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, frames);
        Assert.Same(frames, world.MutableFrames());
        Assert.Same(world.Frames(), world.MutableFrames());
    }

    /// <summary>수정 가능 보기의 쓰기가 읽기 보기에도 보이는지 검증한다. 2026-11-07 F2d 월드 본체 프레임 소유 이식에서 추가됐다.</summary>
    [Fact]
    public void MutableFramesMutationVisibleThroughFrames()
    {
        // 표면·이웃 소비자의 쓰기가 같은 표에 반영되는 공유 의미를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        var replacement = new RiftTypeFrames([]);
        world.MutableFrames()[3] = replacement;
        Assert.Same(replacement, world.Frames()[3]);
    }

    /// <summary>같은 풀의 종속 해제 묶음을 만든다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    /// <param name="pool">묶을 풀이다.</param>
    /// <returns>같은 풀의 종속 해제다.</returns>
    private static RawSquidContainedUnpop ContainedBinding(RawSidPool pool) =>
        new(pool, new SquidContainedUnpopState
        {
            ContentFormType = new UIntBox { Value = 6u },
            GumpFormType = new UIntBox { Value = 3u },
            UiEvent = new UIntBox { Value = 0x02000039u },
        }, new SquidContainedUnpopHooks());

    /// <summary>빈 묶음을 받아들이는지 검증한다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void BindContainedUnpopAcceptsNull()
    {
        // null이면 기본 종단으로 되돌린다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BindContainedUnpop(null);
    }

    /// <summary>같은 풀의 Session 종단을 빌리는지 검증한다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void BindContainedUnpopAcceptsSamePool()
    {
        // 같은 실제 풀의 종단은 약한 참조로 빌린다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.BindContainedUnpop(ContainedBinding(world.Storage().Pool));
    }

    /// <summary>다른 풀의 종단을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 종속 묶음 이식에서 추가됐다.</summary>
    [Fact]
    public void BindContainedUnpopRejectsDifferentPool()
    {
        // 다른 실제 풀의 종단은 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        var other = new RawSurfaceStorage(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => world.BindContainedUnpop(ContainedBinding(other.Pool)));
    }

    /// <summary>같은 저장소 커널을 빌리는지 검증한다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    [Fact]
    public void KernelSharesStorageKernel()
    {
        // 새 커널을 만들지 않고 같은 실제 커널을 빌림을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Same(world.Storage().Processes().Kernel(), world.Kernel());
    }

    /// <summary>예약 객체 정리가 같은 커널을 비우는지 검증한다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    [Fact]
    public void ClearProcessesEmptiesKernel()
    {
        // 외부 연결 모듈 해제 전의 예약 정리를 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.Kernel().Add(new BaseProcess());
        world.Kernel().Add(new BaseProcess());
        Assert.Equal(2, world.Kernel().Size());
        world.ClearProcesses();
        Assert.Equal(0, world.Kernel().Size());
    }

    /// <summary>최종 소멸 정리가 같은 커널을 비우는지 검증한다. 2026-11-07 F2d 월드 본체 커널 차용 이식에서 추가됐다.</summary>
    [Fact]
    public void DrainProcessesEmptiesKernel()
    {
        // 명시 전체 커널의 최종 소멸을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        world.Kernel().Add(new BaseProcess());
        Assert.Equal(1, world.Kernel().Size());
        world.DrainProcesses();
        Assert.Equal(0, world.Kernel().Size());
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

    /// <summary>슬롯 바이트를 리틀 엔디언으로 읽는 검사 보조다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint ReadSlot(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
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

    /// <summary>188개가 아닌 TYPE 월드의 보고서를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportRejectsNon188World()
    {
        // 원본은 Graph·삭제 몸체를 항상 역참조하므로 CD 171개 연결은 명시 오류로 멈춘다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.Report());
    }

    /// <summary>프레임 표가 맞지 않는 월드의 보고서를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportRejectsMismatchedFrames()
    {
        // 원본 생성자는 자산 배열 일치를 강제하므로 프레임 없는 껍데기 호출은 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.Report());
    }

    /// <summary>빈 월드의 보고서 머리·표·주기를 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportEmptyWorld()
    {
        // 원본 CLI 관측의 줄 순서와 초기값을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        string report = world.Report();
        string[] lines = report.Split('\n');
        Assert.Equal(256, lines.Length);
        Assert.Equal("raw_world\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0", lines[0]);
        Assert.Equal("raw_graph_record\t0\t0\t1", lines[1]);
        Assert.Equal("raw_graph_record\t250\t0\t0", lines[251]);
        Assert.Equal("bridge_decay\t-1\t-1\t0\t0\t10", lines[252]);
        Assert.Equal("raw_terrain_type\t97\t-1\t0", lines[253]);
        Assert.Equal("raw_islands\t0", lines[254]);
        Assert.Equal(string.Empty, lines[255]);
    }

    /// <summary>살아 있는 자산 한 개의 보고서 줄을 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportLiveObject()
    {
        // 원본 객체·자산 필드 줄의 순서와 서식을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        world.PrepareFrame(new FrameTime { Game = 2.5 });
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        raw[34] = 7;
        WriteSlot(raw, 36, 0u, 4);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(1.5f), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(-2.25f), 4);
        raw[40] = 11;
        WriteSlot(raw, 26, 0x22u, 4);
        string report = world.Report();
        Assert.StartsWith("raw_world\t2.5\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\n", report);
        Assert.Contains($"raw_object\t{sid}\t70\t7\t0\t1.5\t-2.25\n", report);
        Assert.Contains($"raw_asset_fields\t{sid}\t0\t11\t34\n", report);
        Assert.DoesNotContain("raw_surface_graph", report);
        Assert.DoesNotContain("raw_bridge_fields", report);
    }

    /// <summary>표면 소속 줄이 같은 바이트를 관측하는지 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportSurfaceGraphLine()
    {
        // 원본 표면 플래그 분기의 소속 바이트 읽기를 검증한다.
        var types = Types188();
        types[OriginalConstructors.FirstAssetType].Flags1 = TypeFlag1.Surface;
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, null, Frames188());
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        raw[30] = 9;
        string report = world.Report();
        Assert.Contains($"raw_surface_graph\t{sid}\t9\n", report);
    }

    /// <summary>다리 수명 줄이 같은 프레임 코드를 관측하는지 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportBridgeFieldsLine()
    {
        // 원본 다리 플래그 분기의 수명·코드 플래그 읽기를 검증한다.
        var types = Types188();
        types[OriginalConstructors.FirstAssetType].Flags2 = 4u;
        var frames = Frames188();
        frames[OriginalConstructors.FirstAssetType] = new RiftTypeFrames([new RiftFrameCode((byte)'A', 0, 0, 7)]);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, null, frames);
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        WriteSlot(raw, 36, 0u, 4);
        WriteSlot(raw, 12, 5u, 2);
        string report = world.Report();
        Assert.Contains($"raw_bridge_fields\t{sid}\t5\t7\n", report);
    }

    /// <summary>범위 밖 다리 프레임의 보고서를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportBridgeFieldsRejectsOutOfRangeFrame()
    {
        // 원본 out_of_range 경계를 직역한다.
        var types = Types188();
        types[OriginalConstructors.FirstAssetType].Flags2 = 4u;
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, null, Frames188());
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        WriteSlot(raw, 36, 3u, 4);
        Assert.Throws<ArgumentOutOfRangeException>(() => world.Report());
    }

    /// <summary>종속 체인 진단용 전체 물리 저장소를 만든다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    /// <param name="types">같은 전역 TYPE 표다.</param>
    /// <param name="frames">같은 장기 프레임 표다.</param>
    /// <returns>호스트 조회까지 되는 저장소다.</returns>
    private static RawSurfaceStorage FullStorage(List<OriginalTypeRecord> types, List<RiftTypeFrames> frames)
    {
        // 장기 호스트는 저장소 TYPE으로 만들므로 껍데기 자동 생성분으로는 조회할 수 없다.
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        return new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>종속 체인의 Regular 예약 줄을 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportRegularChain()
    {
        // 원본 비석·주문 진단의 Form 체인 관측을 검증한다.
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullStorage(types, frames), frames);
        int parent = world.Storage().Pool.Allocate();
        Span<byte> parentRaw = world.Storage().Pool.AllocatedBytes(parent);
        parentRaw[RawSidPool.StateOffset] = 0;
        parentRaw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        int form = world.Storage().Pool.Allocate();
        Span<byte> formRaw = world.Storage().Pool.AllocatedBytes(form);
        formRaw[RawSidPool.TypeOffset] = 46;
        WriteSlot(parentRaw, 0, 0x50a748u, 4);
        WriteSlot(parentRaw, 6, (uint)form, 2);
        var regular = new RegularProcess(9u, 0.5f);
        uint pid = world.Kernel().Add(regular);
        regular.SetIdentity(form, parent, pid);
        WriteSlot(formRaw, 18, pid, 4);
        string report = world.Report();
        uint bits = BitConverter.SingleToUInt32Bits(0.5f);
        Assert.Contains($"raw_object_regular\t{parent}\t{form}\t46\t9\t0\t{bits}\t0\n", report);
        Assert.Contains($"raw_object_regular_count\t{parent}\t1\n", report);
    }

    /// <summary>등록과 맞지 않는 Regular 체인의 보고서를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportRegularChainRejectsMismatch()
    {
        // 실제 커널에 없는 Form 체인은 원본처럼 명시 오류로 멈춘다.
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullStorage(types, frames), frames);
        int parent = world.Storage().Pool.Allocate();
        Span<byte> parentRaw = world.Storage().Pool.AllocatedBytes(parent);
        parentRaw[RawSidPool.StateOffset] = 0;
        parentRaw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        int form = world.Storage().Pool.Allocate();
        world.Storage().Pool.AllocatedBytes(form)[RawSidPool.TypeOffset] = 46;
        WriteSlot(parentRaw, 0, 0x50a748u, 4);
        WriteSlot(parentRaw, 6, (uint)form, 2);
        Assert.Throws<InvalidOperationException>(() => world.Report());
    }

    /// <summary>Factory Form 체인 줄을 검증한다. 2026-11-07 F2d 월드 본체 Report 이식에서 추가됐다.</summary>
    [Fact]
    public void ReportFactoryChain()
    {
        // 원본 Factory 진단의 저장 종류·flags 관측을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        int parent = world.Storage().Pool.Allocate();
        Span<byte> parentRaw = world.Storage().Pool.AllocatedBytes(parent);
        parentRaw[RawSidPool.StateOffset] = 0;
        parentRaw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        int form = world.Storage().Pool.Allocate();
        Span<byte> formRaw = world.Storage().Pool.AllocatedBytes(form);
        formRaw[RawSidPool.TypeOffset] = 6;
        WriteSlot(formRaw, 18, 3u, 2);
        WriteSlot(formRaw, 20, 8u, 2);
        WriteSlot(parentRaw, 0, 0x509148u, 4);
        WriteSlot(parentRaw, 6, (uint)form, 2);
        string report = world.Report();
        Assert.Contains($"raw_factory_content\t{parent}\t{form}\t3\t8\n", report);
    }

    /// <summary>생성자 주소 표를 채운 타입 레코드 188개를 만든다. 2026-11-07 F2d 월드 본체 생성 연결 이식에서 추가됐다.</summary>
    /// <returns>공장 생성이 통하는 타입 표다.</returns>
    private static List<OriginalTypeRecord> TypesWithAddresses() =>
        Enumerable.Range(0, OriginalConstructors.TypeCount).Select(i => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) }).ToList();

    /// <summary>내장 Form 생성이 같은 풀에 슬롯을 쓰는지 검증한다. 2026-11-07 F2d 월드 본체 생성 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateFormWritesSlot()
    {
        // 원본 생성자·가상 소유자 경로의 공장 위임을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, TypesWithAddresses(), 97u, null, Frames188());
        Assert.True(world.TakeChanged());
        int sid = world.Create(6u, 0u);
        Assert.Equal(6, world.Storage().Pool.Slot(sid)[RawSidPool.TypeOffset]);
        Assert.True(world.TakeChanged());
    }

    /// <summary>자산 생성이 같은 풀에 슬롯을 쓰고 보고서에 보이는지 검증한다. 2026-11-07 F2d 월드 본체 생성 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateAssetWritesSlot()
    {
        // 원본 자산 생성자·후처리 경로의 공장 위임을 검증한다. Pop 전이라 void 상태로 남는다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, TypesWithAddresses(), 97u, null, Frames188());
        int sid = world.Create(70u, 0u);
        ReadOnlySpan<byte> slot = world.Storage().Pool.Slot(sid);
        Assert.Equal(70, slot[RawSidPool.TypeOffset]);
        Assert.Equal(RawSidPool.VoidBit, slot[RawSidPool.StateOffset]);
        Assert.Empty(world.Objects());
        Assert.True(world.TakeChanged());
    }

    /// <summary>188개가 아닌 TYPE 월드의 생성을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 생성 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void CreateRejectsNon188World()
    {
        // CD 171개 TYPE의 공장 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.Create(6u, 0u));
    }

    /// <summary>188개가 아닌 TYPE 월드의 받침 지정을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerRejectsNon188World()
    {
        // CD 171개 TYPE의 표면 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 3u));
    }

    /// <summary>프레임 표가 맞지 않는 월드의 받침 지정을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerRejectsMismatchedFrames()
    {
        // 껍데기 호출의 표면 연결은 전체 조립 단위에서 잇는다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 3u));
    }

    /// <summary>빈 칸의 받침 지정을 조용히 무시하는지 검증한다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerIgnoresEmptyCell()
    {
        // 지도에 머리가 없으면 갈고리 없이 돌아간다. 원본처럼 변경은 남긴다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        Assert.True(world.TakeChanged());
        world.SetSupportOwner(10f, 10f, 3u);
        Assert.True(world.TakeChanged());
    }

    /// <summary>무인도가 아닌 칸의 받침 지정을 조용히 무시하는지 검증한다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerIgnoresNonNoIsland()
    {
        // 머리 종류가 무인도가 아니면 소유자를 건드리지 않는다. 원본처럼 변경은 남긴다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        int sid = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(sid);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(10f), 4);
        world.Storage().Hash.MutableEntries(0)[10 * 256 + 10] = (ushort)sid;
        Assert.True(world.TakeChanged());
        world.SetSupportOwner(10f, 10f, 3u);
        Assert.Equal(0, world.Storage().Pool.Slot(sid)[34]);
        Assert.True(world.TakeChanged());
    }

    /// <summary>3×3 받침 소유자 전파가 같은 풀에 적히는지 검증한다. 2026-11-07 F2d 월드 본체 받침 소유 지정 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerPropagatesSupportOwners()
    {
        // 연결 조회 미연결에서 멈추기 전 3×3 소유자 쓰기와 변경 알림을 검증한다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u, null, Frames188());
        int target = world.Storage().Pool.Allocate();
        Span<byte> targetRaw = world.Storage().Pool.AllocatedBytes(target);
        targetRaw[RawSidPool.StateOffset] = 0;
        targetRaw[RawSidPool.TypeOffset] = 157;
        WriteSlot(targetRaw, 14, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(targetRaw, 18, BitConverter.SingleToUInt32Bits(10f), 4);
        int support = world.Storage().Pool.Allocate();
        Span<byte> supportRaw = world.Storage().Pool.AllocatedBytes(support);
        supportRaw[RawSidPool.StateOffset] = 0;
        supportRaw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        Span<ushort> heads = world.Storage().Hash.MutableEntries(0);
        for (int cx = 8; cx <= 10; cx++)
        {
            for (int cy = 8; cy <= 10; cy++)
            {
                if (cx != 10 || cy != 10)
                {
                    heads[cy * 256 + cx] = (ushort)support;
                }
            }
        }

        heads[10 * 256 + 10] = (ushort)target;

        Assert.True(world.TakeChanged());
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 5u));
        Assert.Equal(5, world.Storage().Pool.Slot(support)[34]);
        Assert.True(world.TakeChanged());
    }

    /// <summary>표시용 전체 저장소를 만든다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    /// <param name="types">같은 전역 TYPE 표다.</param>
    /// <param name="frames">같은 장기 프레임 표다.</param>
    /// <returns>호스트·표시 조회까지 되는 저장소다.</returns>
    private static RawSurfaceStorage FullDisplayStorage(List<OriginalTypeRecord> types, List<RiftTypeFrames> frames)
    {
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        shapes[70].FrameCount = 1;
        shapes[70].Loaded = true;
        shapes[70].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        return new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>표시 장면을 만든다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    /// <param name="table">받침 슬롯의 가상 표다.</param>
    /// <param name="invalidated">무효화 기록 목록이다.</param>
    /// <returns>받은 월드와 번호 쌍이다.</returns>
    private static (RawSurfaceWorld World, int Target, int Support) DisplayScene(uint table, List<(DisplayRect Rect, uint Flags)> invalidated)
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        int target = world.Storage().Pool.Allocate();
        Span<byte> targetRaw = world.Storage().Pool.AllocatedBytes(target);
        targetRaw[RawSidPool.StateOffset] = 0;
        targetRaw[RawSidPool.TypeOffset] = 157;
        WriteSlot(targetRaw, 14, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(targetRaw, 18, BitConverter.SingleToUInt32Bits(10f), 4);
        int support = world.Storage().Pool.Allocate();
        Span<byte> supportRaw = world.Storage().Pool.AllocatedBytes(support);
        supportRaw[RawSidPool.StateOffset] = 0;
        supportRaw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        WriteSlot(supportRaw, 0, table, 4);
        WriteSlot(supportRaw, 36, 0u, 4);
        WriteSlot(supportRaw, 14, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(supportRaw, 18, BitConverter.SingleToUInt32Bits(10f), 4);
        Span<ushort> heads = world.Storage().Hash.MutableEntries(0);
        for (int cx = 8; cx <= 10; cx++)
        {
            for (int cy = 8; cy <= 10; cy++)
            {
                if (cx != 10 || cy != 10)
                {
                    heads[cy * 256 + cx] = (ushort)support;
                }
            }
        }

        heads[10 * 256 + 10] = (ushort)target;
        world.SetDisplay(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 2000, 2000) }, new SurfaceDisplayHooks
        {
            Suppressed = () => false,
            Invalidate = (rect, flags) => invalidated.Add((rect, flags)),
        });
        return (world, target, support);
    }

    /// <summary>잘못된 카메라를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void SetDisplayRejectsBadView()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        var hooks = new SurfaceDisplayHooks();
        Assert.Throws<ArgumentException>(() => world.SetDisplay(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 0, Viewport = new DisplayRect(0, 0, 10, 10) }, hooks));
        Assert.Throws<ArgumentException>(() => world.SetDisplay(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(10, 0, 0, 10) }, hooks));
    }

    /// <summary>표시 수집이 없으면 설치를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void SetDisplayRejectsMissingDisplay()
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.SetDisplay(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 640, 480) }, new SurfaceDisplayHooks()));
    }

    /// <summary>받침 표시 갱신이 실제 무효화를 전달하는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateDisplayInvalidates()
    {
        var invalidated = new List<(DisplayRect Rect, uint Flags)>();
        (RawSurfaceWorld world, _, int support) = DisplayScene(0x00501DD0u, invalidated);
        Assert.True(world.TakeChanged());
        // 연결 조회가 없어 멈추기 전 8칸의 표시 갱신이 전달된다.
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 5u));
        Assert.Equal(5, world.Storage().Pool.Slot(support)[34]);
        var expected = new DisplayRect(158, 107, 169, 120);
        Assert.Equal(8, invalidated.Count);
        Assert.All(invalidated, entry => Assert.Equal((expected, 0u), entry));
        Assert.True(world.TakeChanged());
    }

    /// <summary>억제 중에는 등록하지 않는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void UpdateDisplaySkipsWhenSuppressed()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        int target = world.Storage().Pool.Allocate();
        Span<byte> targetRaw = world.Storage().Pool.AllocatedBytes(target);
        targetRaw[RawSidPool.StateOffset] = 0;
        targetRaw[RawSidPool.TypeOffset] = 157;
        int support = world.Storage().Pool.Allocate();
        Span<byte> supportRaw = world.Storage().Pool.AllocatedBytes(support);
        supportRaw[RawSidPool.StateOffset] = 0;
        supportRaw[RawSidPool.TypeOffset] = (byte)OriginalConstructors.FirstAssetType;
        WriteSlot(supportRaw, 0, 0x00501DD0u, 4);
        Span<ushort> heads = world.Storage().Hash.MutableEntries(0);
        heads[10 * 256 + 10] = (ushort)target;
        heads[9 * 256 + 10] = (ushort)support;
        var invalidated = new List<(DisplayRect Rect, uint Flags)>();
        world.SetDisplay(new SquidDisplayView { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 2000, 2000) }, new SurfaceDisplayHooks
        {
            Suppressed = () => true,
            Invalidate = (rect, flags) => invalidated.Add((rect, flags)),
        });
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 5u));
        Assert.Empty(invalidated);
    }

    /// <summary>메타 갱신이 장치에 전달되는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void MetaRefreshDelivered()
    {
        var invalidated = new List<(DisplayRect Rect, uint Flags)>();
        (RawSurfaceWorld world, _, int support) = DisplayScene(0x50c9f0u, invalidated);
        var refreshed = new List<byte[]>();
        world.BindMetaRefresh(raw => refreshed.Add(raw.ToArray()));
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 5u));
        Assert.Equal(8, refreshed.Count);
        byte[] expected = world.Storage().Pool.Slot(support).ToArray();
        Assert.All(refreshed, bytes => Assert.Equal(expected, bytes));
        Assert.Empty(invalidated);
    }

    /// <summary>빠진 메타 장치를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindMetaRefreshRejectsNull()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        Assert.Throws<ArgumentException>(() => world.BindMetaRefresh(null!));
    }

    /// <summary>연결되지 않은 메타 갱신을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 표시 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void MetaRefreshRejectsMissingBinding()
    {
        var invalidated = new List<(DisplayRect Rect, uint Flags)>();
        (RawSurfaceWorld world, _, _) = DisplayScene(0x50c9f0u, invalidated);
        Assert.Throws<InvalidOperationException>(() => world.SetSupportOwner(10f, 10f, 5u));
    }
    /// <summary>배치 가능한 타입 표를 만든다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    /// <returns>생성자 주소 표를 채운 타입 표다.</returns>
    private static List<OriginalTypeRecord> DisplayTypes()
    {
        var types = TypesWithAddresses();
        types[70].ConstructorAddress = 0u;
        return types;
    }

    /// <summary>배치 가능한 전체 저장소를 만든다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    /// <param name="types">같은 전역 TYPE 표다.</param>
    /// <param name="frames">같은 장기 프레임 표다.</param>
    /// <returns>호스트·표시·배치 조회까지 되는 저장소다.</returns>
    private static RawSurfaceStorage DisplayPopStorage(List<OriginalTypeRecord> types, List<RiftTypeFrames> frames)
    {
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        shapes[70].FrameCount = 1;
        shapes[70].Loaded = true;
        shapes[70].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        return new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
    }

    /// <summary>배치 가능한 월드를 만든다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    /// <returns>배치 가능한 월드다.</returns>
    private static RawSurfaceWorld DisplayPopWorld()
    {
        var types = DisplayTypes();
        var frames = Frames188();
        return new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, DisplayPopStorage(types, frames), frames);
    }

    /// <summary>188개가 아닌 TYPE 월드의 등록을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void PopRejectsNon188World()
    {
        // CD 171개 TYPE의 배치 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.Pop(5, 10f, 10f));
        Assert.Throws<InvalidOperationException>(() => world.Repop(5));
    }

    /// <summary>표시 수집이 없으면 등록을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void PopRejectsMissingDisplay()
    {
        // 셰이프 표 없는 껍데기 월드의 배치는 후속 단위에서 잇는다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.Pop(5, 10f, 10f));
    }

    /// <summary>살아 있는 자산이 같은 공간에 등록되는지 검증한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void PopPlacesLiveSlot()
    {
        // 생성→등록 파이프라인의 공간 고정을 검증한다.
        var world = DisplayPopWorld();
        int sid = world.Create(70u, 0u);
        Assert.True(world.TakeChanged());
        world.Pop(sid, 10f, 10f);
        List<SurfaceSnapshot> objects = world.Objects();
        Assert.Single(objects);
        Assert.Equal(sid, objects[0].Sid);
        Assert.Equal(10f, objects[0].Object.X);
        Assert.Equal(10f, objects[0].Object.Y);
        Assert.True(world.TakeChanged());
    }

    /// <summary>범위 밖 좌표가 10으로 고정되는지 검증한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void PopClampsOutOfRange()
    {
        // 원문처럼 지도 밖·0 이하 좌표는 10으로 둔다.
        var world = DisplayPopWorld();
        int sid = world.Create(70u, 0u);
        world.Pop(sid, 300f, 300f);
        List<SurfaceSnapshot> objects = world.Objects();
        Assert.Single(objects);
        Assert.Equal(10f, objects[0].Object.X);
        Assert.Equal(10f, objects[0].Object.Y);
    }

    /// <summary>다시 등록이 현재 좌표를 읽는지 검증한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void RepopReregisters()
    {
        // 해제 뒤 같은 공간 재등록 경로를 검증한다.
        var world = DisplayPopWorld();
        int sid = world.Create(70u, 0u);
        world.Pop(sid, 10f, 10f);
        world.Unpop(sid);
        Assert.Empty(world.Objects());
        world.Repop(sid);
        Assert.Single(world.Objects());
    }

    /// <summary>빈 번호의 등록을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 배치 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void PopRejectsFreeSlot()
    {
        // 슬롯 5번은 fresh 풀의 빈 머리라 SHP 조회 전에 멈춘다.
        var world = DisplayPopWorld();
        Assert.Throws<InvalidOperationException>(() => world.Pop(5, 10f, 10f));
    }

    /// <summary>연결 조회가 없으면 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void ConnectRejectsNon188World()
    {
        // CD 171개 TYPE의 연결 조회는 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.BridgeConnections());
        Assert.Throws<InvalidOperationException>(() => world.Find(10f, 10f, 70u));
    }

    /// <summary>같은 연결 생성기를 빌리는지 검증한다. 2026-11-07 F2d 월드 본체 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void BridgeConnectionsReturnsSame()
    {
        var world = DisplayPopWorld();
        Assert.Same(world.BridgeConnections(), world.BridgeConnections());
    }

    /// <summary>등록된 객체를 좌표·타입으로 찾는지 검증한다. 2026-11-07 F2d 월드 본체 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void FindReturnsPlacedObject()
    {
        var world = DisplayPopWorld();
        int sid = world.Create(70u, 0u);
        world.Pop(sid, 10f, 10f);
        Assert.Equal(sid, world.Find(10f, 10f, 70u));
        Assert.Equal(0, world.Find(10f, 10f, 71u));
    }

    /// <summary>표 밖 타입을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 연결 조회 이식에서 추가됐다.</summary>
    [Fact]
    public void FindRejectsBadType()
    {
        var world = DisplayPopWorld();
        Assert.Throws<ArgumentOutOfRangeException>(() => world.Find(10f, 10f, 999u));
    }

    /// <summary>초기 적재 장면을 만든다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    /// <returns>적재 가능한 월드와 타입·프레임 표다.</returns>
    private static (RawSurfaceWorld World, List<OriginalTypeRecord> Types, List<RiftTypeFrames> Frames) LoadScene()
    {
        var types = Types188();
        var frames = Frames188();
        foreach (int type in new int[] { 82, 97, 157 })
        {
            frames[type] = new RiftTypeFrames([new RiftFrameCode((byte)'A', (byte)'P', 0, 0)]);
        }

        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        foreach (int type in new int[] { 82, 97, 157 })
        {
            shapes[type].FrameCount = 1;
            shapes[type].Loaded = true;
            shapes[type].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        }

        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        return (new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, storage, frames), types, frames);
    }

    /// <summary>초기 위치 목록을 만든다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    private static List<SurfaceSeed> LoadSeeds() =>
    [
        new SurfaceSeed(82u, 5u, 0, 20f, 20f),
        new SurfaceSeed(157u, 3u, 0, 10f, 10f),
        new SurfaceSeed(97u, 7u, 0, 30f, 30f),
    ];

    /// <summary>세 종류를 순서대로 적재하는지 검증한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadPlacesSeeds()
    {
        (RawSurfaceWorld world, _, _) = LoadScene();
        world.Load(LoadSeeds());
        Assert.True(world.Loaded());
        Assert.True(world.TakeChanged());
        Assert.False(world.TakeChanged());
        List<SurfaceSnapshot> objects = world.Objects();
        Assert.Equal(3, objects.Count);
        SurfaceSnapshot bridge = Assert.Single(objects, snapshot => snapshot.Object.Type == 82u);
        Assert.Equal(5u, bridge.Object.Owner);
        Assert.Equal(0, bridge.Object.Frame);
        Assert.Equal(20f, bridge.Object.X);
        Assert.Equal(20f, bridge.Object.Y);
        SurfaceSnapshot noIsland = Assert.Single(objects, snapshot => snapshot.Object.Type == 157u);
        Assert.Equal(3u, noIsland.Object.Owner);
        SurfaceSnapshot terrain = Assert.Single(objects, snapshot => snapshot.Object.Type == 97u);
        Assert.Equal(7u, terrain.Object.Owner);
    }

    /// <summary>미지원 타입을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRejectsBadType()
    {
        (RawSurfaceWorld world, _, _) = LoadScene();
        Assert.Throws<ArgumentException>(() => world.Load([new SurfaceSeed(999u, 0u, 0, 10f, 10f)]));
    }

    /// <summary>범위 밖 필드를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRejectsBadField()
    {
        (RawSurfaceWorld world, _, _) = LoadScene();
        Assert.Throws<ArgumentOutOfRangeException>(() => world.Load([new SurfaceSeed(82u, 99u, 0, 10f, 10f)]));
        Assert.Throws<ArgumentOutOfRangeException>(() => world.Load([new SurfaceSeed(82u, 0u, 0, 0.5f, 10f)]));
        Assert.Throws<ArgumentOutOfRangeException>(() => world.Load([new SurfaceSeed(82u, 0u, 5, 10f, 10f)]));
    }

    /// <summary>중복 적재를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRejectsDuplicate()
    {
        (RawSurfaceWorld world, _, _) = LoadScene();
        world.Load(LoadSeeds());
        Assert.Throws<InvalidOperationException>(() => world.Load(LoadSeeds()));
    }

    /// <summary>188개가 아닌 TYPE 월드의 적재를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRejectsNon188World()
    {
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.Load([]));
    }

    /// <summary>프레임 표가 맞지 않으면 적재를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 초기 적재 이식에서 추가됐다.</summary>
    [Fact]
    public void LoadRejectsMismatchedFrames()
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<ArgumentOutOfRangeException>(() => world.Load(LoadSeeds()));
    }

    /// <summary>188개가 아닌 TYPE 월드의 삭제 분배를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 분배 이식에서 추가됐다.</summary>
    [Fact]
    public void DestroyRejectsNon188World()
    {
        // CD 171개 TYPE의 삭제 분배 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.Destroy(5));
    }

    /// <summary>배치된 자산 삭제가 장부와 변경을 갱신하는지 검증한다. 2026-11-07 F2d 월드 본체 삭제 분배 이식에서 추가됐다.</summary>
    [Fact]
    public void DestroyRemovesPlacedObject()
    {
        var world = DisplayPopWorld();
        int sid = world.Create(70u, 0u);
        world.Pop(sid, 10f, 10f);
        Assert.True(world.TakeChanged());
        Assert.True(world.Destroy(sid));
        Assert.Empty(world.Objects());
        Assert.True(world.TakeChanged());
        // PreDestroy·Transmit·PostDestroy가 삭제 요청으로 센다. 정확한 줄을 확인한다.
        // PreDestroy·Transmit·PostDestroy 3건이 삭제 요청으로 센다.
        Assert.Equal("raw_world\t0\t0\t0\t0\t0\t0\t0\t0\t0\t0\t3", world.Report().Split('\n')[0]);
    }

    /// <summary>실행 장면을 만든다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    /// <param name="life">다리 초기 수명이다.</param>
    /// <param name="carriers">대피 기록 목록이다.</param>
    /// <returns>묶인 월드와 다리 번호다.</returns>
    private static (RawSurfaceWorld World, int Root) RunScene(int life, List<int> carriers)
    {
        var types = Types188();
        types[82].Flags1 = TypeFlag1.Surface;
        types[82].Flags2 = TypeFlag2.Bridge;
        var frames = Frames188();
        frames[82] = new RiftTypeFrames([new RiftFrameCode((byte)'J', (byte)'P', 0, 0)]);
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        shapes[82].FrameCount = 1;
        shapes[82].Loaded = true;
        shapes[82].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 12, HotspotX = 2, HotspotY = 3, CellWidth = 8f, CellHeight = 6f });
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, storage, frames);
        storage.Graph.Records[0] = new Netstorm.Core.Bridges.GraphRecord { Surfaces = 9, InUse = 1 };
        int root = world.Storage().Pool.Allocate();
        Span<byte> raw = world.Storage().Pool.AllocatedBytes(root);
        raw[RawSidPool.StateOffset] = 0;
        raw[RawSidPool.TypeOffset] = 82;
        WriteSlot(raw, 36, 0u, 4);
        WriteSlot(raw, 14, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(raw, 18, BitConverter.SingleToUInt32Bits(10f), 4);
        WriteSlot(raw, 12, (uint)life * 8u, 2);
        world.BindBridgeDecay(() => 0u, sid => carriers.Add(sid), _ => { });
        return (world, root);
    }

    /// <summary>중복 연결을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindBridgeDecayRejectsDuplicate()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        world.BindBridgeDecay(() => 0u, _ => { }, _ => { });
        Assert.Throws<InvalidOperationException>(() => world.BindBridgeDecay(() => 0u, _ => { }, _ => { }));
    }

    /// <summary>빠진 장치를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindBridgeDecayRejectsMissingHooks()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        Assert.Throws<ArgumentNullException>(() => world.BindBridgeDecay(() => 0u, null!, _ => { }));
        Assert.Throws<ArgumentNullException>(() => world.BindBridgeDecay(() => 0u, _ => { }, null!));
    }

    /// <summary>조립이 덜 된 월드의 연결을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindBridgeDecayRejectsIncompleteWorld()
    {
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, Types188(), 97u);
        Assert.Throws<InvalidOperationException>(() => world.BindBridgeDecay(() => 0u, _ => { }, _ => { }));
    }

    /// <summary>한 칸 처리가 수명을 줄이는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void DecayBridgeCellDecaysOne()
    {
        var carriers = new List<int>();
        (RawSurfaceWorld world, int root) = RunScene(3, carriers);
        world.DecayBridgeCell(new FrameTime { Game = 100.0 }, root);
        Assert.Equal(2u * 8u, ReadSlot(world.Storage().Pool.Slot(root), 12, 2));
        Assert.Equal([root], carriers);
    }

    /// <summary>연결 전 한 칸 처리를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void DecayBridgeCellRejectsMissingDecay()
    {
        var types = Types188();
        var frames = Frames188();
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, FullDisplayStorage(types, frames), frames);
        Assert.Throws<InvalidOperationException>(() => world.DecayBridgeCell(new FrameTime { Game = 100.0 }, 5));
    }

    /// <summary>프레임 실행이 스캔·커널을 함께 도는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void RunFrameScansAndRunsKernel()
    {
        var carriers = new List<int>();
        (RawSurfaceWorld world, int root) = RunScene(3, carriers);
        world.Storage().BridgeDecayScan.First = root;
        world.Storage().BridgeDecayScan.Last = root + 1;
        world.Storage().BridgeDecayScan.Cursor = root;
        world.Storage().BridgeDecayScan.Next = 0.0;
        world.RunFrame(new FrameTime { Game = 100.0, Delta = 1.0 }, false);
        Assert.Equal(2u * 8u, ReadSlot(world.Storage().Pool.Slot(root), 12, 2));
        Assert.Equal(root, world.Storage().BridgeDecayScan.Cursor);
        Assert.Equal(110.0, world.Storage().BridgeDecayScan.Next);
        Assert.Equal(100.0, world.CurrentTime().Game);
    }

    /// <summary>정지 중에는 붕괴를 건드리지 않는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void RunFramePausedSkipsDecay()
    {
        var carriers = new List<int>();
        (RawSurfaceWorld world, int root) = RunScene(3, carriers);
        world.RunFrame(new FrameTime { Game = 100.0, Delta = 1.0 }, true);
        Assert.Equal(3u * 8u, ReadSlot(world.Storage().Pool.Slot(root), 12, 2));
        Assert.Empty(carriers);
        Assert.Equal(100.0, world.CurrentTime().Game);
    }

    /// <summary>완성된 런타임 훅 묶음을 만든다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    /// <param name="depths">로딩 깊이 전달 기록 목록이다.</param>
    /// <returns>연결용 훅 묶음이다.</returns>
    private static RawWorldRuntimeHooks RuntimeHooks(List<uint> depths) => new()
    {
        Assets = new SquidDestroyHooks { Emit = _ => { }, Selected = () => 0 },
        Destroy = (_, _, _) => false,
        Regular = (_, _, _, _) => 0f,
        Transmit = new ProcessFormTransmission { WriteData = (_, _) => 0, Transmit = _ => { } },
        WalkerFall = _ => true,
        LoadingDepth = depth => depths.Add(depth),
        BridgeEffect = _ => { },
    };

    /// <summary>런타임 경계가 한 번 연결되는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindRuntimeBindsOnce()
    {
        var world = DisplayPopWorld();
        world.BindRuntime(RuntimeHooks([]));
        Assert.Throws<InvalidOperationException>(() => world.BindRuntime(RuntimeHooks([])));
    }

    /// <summary>객체가 생긴 뒤의 늦은 연결을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindRuntimeRejectsLateCall()
    {
        var world = DisplayPopWorld();
        world.Storage().Pool.Allocate();
        Assert.Throws<InvalidOperationException>(() => world.BindRuntime(RuntimeHooks([])));
    }

    /// <summary>빠진 필수 경계를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindRuntimeRejectsMissingHooks()
    {
        var world = DisplayPopWorld();
        Assert.Throws<ArgumentNullException>(() => world.BindRuntime(null!));
        Assert.Throws<ArgumentException>(() => world.BindRuntime(new RawWorldRuntimeHooks()));
    }

    /// <summary>188개가 아닌 TYPE 월드의 연결을 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindRuntimeRejectsNon188World()
    {
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.BindRuntime(RuntimeHooks([])));
    }

    /// <summary>로딩 깊이가 양쪽에 전달되는지 검증한다. 2026-11-07 F2d 월드 본체 실행 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void BindRuntimeForwardsLoadingDepth()
    {
        var world = DisplayPopWorld();
        var depths = new List<uint>();
        world.BindRuntime(RuntimeHooks(depths));
        Assert.Equal([1u], depths);
        world.SetLoadingDepth(5u);
        Assert.Equal([1u, 5u], depths);
    }

    /// <summary>월드 생성 뒤 호스트 런타임 묶음이 연결되는지 검증한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void HostBoundAfterWorldCreation()
    {
        var types = Types188();
        var frames = Frames188();
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        Assert.Throws<InvalidOperationException>(() => storage.Processes().Host().Runtime());
        _ = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, storage, frames);
        Assert.NotNull(storage.Processes().Host().Runtime().Destroy);
    }

    /// <summary>같은 저장소의 두 번째 월드를 거부하는지 검증한다. 2026-11-07 F2d 호스트 대여 이식에서 추가됐다.</summary>
    [Fact]
    public void SecondWorldOnSameStorageThrows()
    {
        var types = Types188();
        var frames = Frames188();
        var shapes = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new SquidDisplayShape()).ToList();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes);
        _ = new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, storage, frames);
        Assert.Throws<InvalidOperationException>(() => new RawSurfaceWorld(OriginalEdition.Patch1078, types, 97u, storage, frames));
    }

    /// <summary>188개가 아닌 TYPE 월드의 해제를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 해제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopRejectsNon188World()
    {
        // CD 171개 TYPE의 해제 연결은 후속 단위에서 잇는다.
        var types171 = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToList();
        var world = new RawSurfaceWorld(OriginalEdition.Cd1072, types171, 97u);
        Assert.Throws<InvalidOperationException>(() => world.Unpop(5, 0u));
    }

    /// <summary>void 자산의 해제가 조용히 끝나는지 검증한다. 2026-11-07 F2d 월드 본체 해제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopVoidAssetIsNoOp()
    {
        // 배치 전 void 슬롯은 원본처럼 해제 없이 돌아간다. 변경은 남긴다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, TypesWithAddresses(), 97u, null, Frames188());
        int sid = world.Create(70u, 0u);
        Assert.True(world.TakeChanged());
        world.Unpop(sid, 0u);
        Assert.Equal(70, world.Storage().Pool.Slot(sid)[RawSidPool.TypeOffset]);
        Assert.True(world.TakeChanged());
    }

    /// <summary>Form 슬롯의 해제를 거부하는지 검증한다. 2026-11-07 F2d 월드 본체 해제 연결 이식에서 추가됐다.</summary>
    [Fact]
    public void UnpopRejectsFormSlot()
    {
        // 자산 밖 번호는 원본처럼 명시 오류로 멈춘다.
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, TypesWithAddresses(), 97u, null, Frames188());
        int sid = world.Create(6u, 0u);
        Assert.Throws<InvalidOperationException>(() => world.Unpop(sid, 0u));
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
