using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 PATCH 공통 삭제의 현재 저장소·선택·보상 경계를 검사하는 네임스페이스다.
namespace Netstorm.Core.Tests;

/// <summary>외부 콜백 뒤의 현재 물리 읽기와 원문 효과 순서를 검사한다.</summary>
public sealed class RawDestroyLifecyclePatchCurrentTests
{
    /// <summary>실제 SID 풀·현재 TYPE·장부와 공통 삭제를 소유하는 독립 입력 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 객체 풀이다.</summary>
        public RawSidPool Pool { get; } = new(32768, true);
        /// <summary>현재 TYPE 물리 표다.</summary>
        public OriginalTypeRecord[] Types { get; } = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord { Group = 10 }).ToArray();
        /// <summary>독립/공유 장부 입력이다.</summary>
        public RawPostPopState Book { get; } = new();
        /// <summary>선택과 손실 위치의 독립/공유 입력이다.</summary>
        public SquidDeletionState State { get; } = new();
        /// <summary>효과 사이 현재 입력을 바꾸는 외부 경계다.</summary>
        public SquidDeletionHooks Hooks { get; } = new();
        /// <summary>실제 공통 삭제 몸체다.</summary>
        public RawSquidDestroy Destroy { get; }
        /// <summary>이번에 실제 확보한 객체 번호다.</summary>
        public int Sid { get; }
        /// <summary>선택/소리/보상 효과 순서다.</summary>
        public List<SquidDeletionEvent> Events { get; } = [];
        /// <summary>원문 진단이 반환할 때 실행하는 선택 입력이다.</summary>
        public Action<string, string, uint>? Assertion { get; set; }
        /// <summary>공간 밖 자산을 실제 할당하고 Native 진단 경계를 연결한다.</summary>
        public Scene()
        {
            Sid = Pool.Allocate(SidAllocation.Client);
            Span<byte> raw = Pool.RawSlot(Sid); raw.Clear(); raw[10] = 74; raw[11] = 4; raw[34] = 1;
            BinaryPrimitives.WriteUInt32LittleEndian(raw, OriginalConstructors.BaseVtable);
            Destroy = new(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Destroy.BindPatchDiagnostics(new() { Assertion = (expression, file, line) => Assertion?.Invoke(expression, file, line), Trace = (_, _) => { } });
            Hooks.Emit = Events.Add;
        }
        /// <summary>현재 입력으로 수명을 만들며 실제 보상도 선택적으로 연결한다.</summary>
        public RawSquidDestroyLifecycle Body(RawSquidReward? reward = null) => new(Pool, Types, Book, State, Destroy, Hooks, reward: reward);
        /// <summary>좌표 비트를 산술 없이 현재 슬롯에 쓴다.</summary>
        public void Coordinates(uint x, uint y)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(Sid)[14..], x);
            BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(Sid)[18..], y);
        }
    }

    /// <summary>사전 검사 진입은 raw/목록/비용을 읽지 않으며 실제 Pre 단언 복귀 뒤 현재 TYPE를 사용한다.</summary>
    [Fact]
    public void NativeValidationIsLazyAndReturningAssetAssertionUsesCurrentType()
    {
        var s = new Scene(); var body = s.Body();
        body.ValidatePre(-1, 0); body.ValidatePost(-1, 0);
        s.Pool.RawSlot(s.Sid)[10] = 6;
        s.Assertion = (expression, file, line) =>
        {
            Assert.Equal("!isForm()", expression); Assert.Equal(@"\Ns\O\Squid.cpp", file); Assert.Equal(0x771u, line);
            s.Pool.RawSlot(s.Sid)[10] = 75; s.Types[75].Flags1 = 0x10000000;
            s.Book.Providers.Entries = [(uint)s.Sid, 88, (uint)s.Sid, 99]; s.Book.Providers.Count = 3;
        };
        body.PreDestroy(s.Sid, 0x200000);
        Assert.Equal(new uint[] { 88, 88, (uint)s.Sid, 99 }, s.Book.Providers.Entries);
        Assert.Equal(1u, s.Book.Providers.Count); Assert.Equal(1u, s.Book.ProductionDirty);
        Assert.Equal(uint.MaxValue, s.Book.GlobalCounts[75]); Assert.Equal(0u, s.Book.GlobalCounts[74]);
    }

    /// <summary>Native 전체 선택이 일치할 때 원시 NaN/-0 기억점이 선택 해제보다 먼저 전달된다.</summary>
    [Fact]
    public void PlacementKeepsRawCoordinateBitsAndIndependentSelectionShadow()
    {
        var s = new Scene(); s.State.Selected = 123; s.Book.SharedSuppressed = new() { Value = true };
        s.Pool.RawSlot(s.Sid)[40] = 1; s.Coordinates(0x7fc01234, 0x80000000);
        var order = new List<string>(); s.Hooks.SelectedRaw = () => (uint)s.Sid;
        s.Hooks.RememberPlacement = (x, y, type) => { Assert.Equal(0x7fc01234u, x); Assert.Equal(0x80000000u, y); Assert.Equal(74u, type); order.Add("remember"); };
        s.Hooks.Emit = effect => { Assert.Equal(SquidDeletionEffect.ClearSelection, effect.Effect); order.Add("clear"); };
        var body = s.Body(); body.PreDestroy(s.Sid, 0);
        Assert.Equal(new[] { "remember", "clear" }, order); Assert.Equal(123, s.State.Selected);
        Assert.False(s.Book.PendingPlacement); Assert.Equal(0u, s.State.PlacementType);
        Assert.Equal((uint)s.Sid, body.Hooks().SelectedRaw!());
        s.Hooks.SelectedRaw = () => 0x10000u | (uint)s.Sid; body.PreDestroy(s.Sid, 0); Assert.Equal(2, order.Count);
        Assert.Equal(0x10000u | (uint)s.Sid, body.Hooks().SelectedRaw!());
    }

    /// <summary>선택 해제 뒤 현재 억제 bool을 읽으며 독립 사본을 덮어쓰지 않는다.</summary>
    [Fact]
    public void SelectionEffectCanChangeCurrentSuppressionWithoutChangingShadow()
    {
        var s = new Scene(); s.Book.Suppressed = true; s.Book.SharedSuppressed = new() { Value = true };
        s.Hooks.SelectedRaw = () => (uint)s.Sid;
        s.Hooks.Emit = _ => s.Book.SharedSuppressed.Value = false;
        s.Body().PreDestroy(s.Sid, 0x200000);
        Assert.Equal(uint.MaxValue, s.Book.GlobalCounts[74]); Assert.True(s.Book.Suppressed); Assert.False(s.Book.CurrentSuppressed);
    }

    /// <summary>음수 count는 포인터/용량을 읽지 않고 원문 감긴 DWORD를 그대로 보존한다.</summary>
    [Theory]
    [InlineData(0u)]
    [InlineData(0x80000000u)]
    [InlineData(0xffffffffu)]
    public void NativeRemoveUsesSignedCountAndDoesNotReadUnusedStorage(uint count)
    {
        var s = new Scene(); s.Types[74].Flags1 = 0x10000000;
        uint current = count; s.Book.Providers.BindCurrent(new()
        {
            Storage = () => throw new InvalidOperationException("도달하면 안 되는 포인터"),
            Capacity = () => throw new InvalidOperationException("도달하면 안 되는 용량"),
            ReadCount = () => current, WriteCount = value => current = value,
        });
        s.Body().PreDestroy(s.Sid, 0x200000);
        Assert.Equal(count, current); Assert.Equal(1u, s.Book.ProductionDirty);
    }

    /// <summary>양수 압축은 각 반복에서 한 번 읽은 포인터에만 쓰고 현재 count를 다시 읽는다.</summary>
    [Fact]
    public void NativeRemoveKeepsOnePointerPerIterationAndStaleTail()
    {
        var s = new Scene(); s.Types[74].Flags1 = 0x10000000;
        uint[][] rows = [[(uint)s.Sid, 44, 55], [66, (uint)s.Sid, 77], [88, 99, 111, 222]];
        uint count = 3; int reads = 0, counts = 0;
        s.Book.Providers.BindCurrent(new()
        {
            Storage = () => rows[reads++], Capacity = () => throw new InvalidOperationException("선검사 금지"),
            ReadCount = () => { counts++; return count; }, WriteCount = value => count = value,
        });
        s.Body().PreDestroy(s.Sid, 0x200000);
        Assert.Equal(3, reads); Assert.Equal(5, counts); Assert.Equal(1u, count);
        Assert.Equal(new uint[] { 111, 99, 111, 222 }, rows[2]); Assert.Equal(66u, rows[1][0]);
    }

    /// <summary>provider 압축 뒤 AI가 현재 타입을 바꾸면 후행 factory 제거도 새 타입을 사용한다.</summary>
    [Fact]
    public void AiNoticeRunsBetweenProviderAndCurrentFactoryRemoval()
    {
        var s = new Scene(); s.Types[74].Flags1 = 0x10000000; var body = s.Body();
        s.Book.Providers.Entries = [(uint)s.Sid, 80]; s.Book.Providers.Count = 2;
        s.Book.Factories.Entries = [90, (uint)s.Sid]; s.Book.Factories.Count = 2;
        body.BindPatchAiNotice((owner, sid, created) =>
        {
            Assert.Equal(1u, owner); Assert.Equal(s.Sid, sid); Assert.False(created);
            Assert.Equal(1u, s.Book.Providers.Count); Assert.Equal(2u, s.Book.Factories.Count); Assert.Equal(1u, s.Book.ProductionDirty);
            s.Pool.RawSlot(s.Sid)[10] = 75; s.Types[75].Flags2 = 0x4200; body.BindPatchAiNotice(null);
        });
        body.PreDestroy(s.Sid, 0x200000);
        Assert.Equal(1u, s.Book.Factories.Count); Assert.Equal(2u, s.Book.ProductionDirty);
        Assert.Equal(uint.MaxValue, s.Book.GlobalCounts[74]); Assert.Equal(0u, s.Book.GlobalCounts[75]);
    }

    /// <summary>현재 로컬 owner는 전체 DWORD로 비교하고 손실 억제도 현재 저장소를 읽는다.</summary>
    [Fact]
    public void LocalOwnerAndLostGatesAreReadAfterRefundEffects()
    {
        var s = new Scene(); s.Types[74].Group = 1; s.State.UnitLostSuppressed1 = true; s.State.UnitLostSuppressed2 = true;
        s.State.SharedLoadingDepth = new() { Value = 8 }; s.State.SharedFortMode = new() { Value = true };
        s.State.SharedLostX = new() { Value = 12 }; s.State.SharedLostY = new() { Value = 13 }; s.State.LostX = 90; s.State.LostY = 91;
        s.Hooks.LocalOwner = () => 0x10001;
        s.Hooks.Emit = effect =>
        {
            s.Events.Add(effect);
            if (effect.Effect == SquidDeletionEffect.Refund)
            {
                s.Hooks.LocalOwner = () => 1; s.State.SharedLoadingDepth.Value = 0; s.State.SharedFortMode.Value = false;
            }
            if (effect.Effect == SquidDeletionEffect.UnitLostSound) s.Coordinates(0xffc01234, 0x80000000);
        };
        s.Body().PreDestroy(s.Sid, 0);
        Assert.Equal(new[] { SquidDeletionEffect.Refund, SquidDeletionEffect.UnitLostSound }, s.Events.Select(e => e.Effect));
        Assert.Equal(uint.MaxValue, s.Book.LocalCounts[74]); Assert.Equal(0u, s.Book.LocalSecondaryCounts[74]);
        Assert.Equal(0xffc01234u, BitConverter.SingleToUInt32Bits(s.State.CurrentLostX)); Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(s.State.CurrentLostY));
        Assert.Equal(90f, s.State.LostX); Assert.Equal(91f, s.State.LostY);
    }

    /// <summary>상위 비트가 다른 로컬 owner는 통계와 손실 알림 어느 곳에서도 BYTE로 좁히지 않는다.</summary>
    [Fact]
    public void HighLocalOwnerBitsPreventLocalCountsAndLostSound()
    {
        var s = new Scene(); s.Book.LocalOwner = 1; s.Types[74].Group = 1; s.Hooks.LocalOwner = () => 0x10001;
        s.Body().PreDestroy(s.Sid, 0);
        Assert.Equal(0u, s.Book.LocalCounts[74]); Assert.Single(s.Events); Assert.Equal(SquidDeletionEffect.Refund, s.Events[0].Effect);
    }

    /// <summary>현재 silent TYPE는 소리만 억제하고 손실 좌표 기록은 유지한다.</summary>
    [Fact]
    public void CurrentSilentTypeStillRecordsLostCoordinates()
    {
        var s = new Scene(); s.Types[74].Group = 1; s.Hooks.LocalOwner = () => 1;
        s.State.SharedSilentType = new() { Value = 74 }; s.Coordinates(0x80000000, 0x7fc05678);
        s.Body().PreDestroy(s.Sid, 0);
        Assert.Single(s.Events); Assert.Equal(0x80000000u, BitConverter.SingleToUInt32Bits(s.State.LostX)); Assert.Equal(0x7fc05678u, BitConverter.SingleToUInt32Bits(s.State.LostY));
    }

    /// <summary>현재 TYPE 비용 대신 최초 로딩 표와 같은 장기 비용 주소를 사용하며 캐시 교체도 다음 호출에 반영한다.</summary>
    [Fact]
    public void PostUsesLoadedCostCacheAndCurrentTotalWithoutOverwritingShadow()
    {
        var s = new Scene(); s.Types[74].Cost = 9; var body = s.Body(); s.Types[74].Cost = 999;
        s.Book.TotalCost = 777; s.Book.SharedTotalCost = new() { Value = 1000 };
        OriginalTypeRecord[] cache = s.Types.Select(t => t.Clone()).ToArray(); cache[74].Cost = 17.75f;
        s.Destroy.BindPatchDiagnostics(new() { LoadedCostTypes = cache });
        body.PostDestroy(s.Sid, 0); Assert.Equal(983, s.Book.CurrentTotalCost); Assert.Equal(777, s.Book.TotalCost);
        cache[74].Cost = 25; body.PostDestroy(s.Sid, 0); Assert.Equal(958, s.Book.CurrentTotalCost);
        s.Destroy.BindPatchDiagnostics(new()); body.PostDestroy(s.Sid, 0); Assert.Equal(949, s.Book.CurrentTotalCost);
    }

    /// <summary>실제 보상은 SP·샘 풀에 한 번만 반영하고 외부 Refund 대체 사건은 내보내지 않는다.</summary>
    [Fact]
    public void ActualRewardRunsOnceAndRequiresSameBookkeeping()
    {
        var s = new Scene(); s.Types[74].Cost = 100;
        var sp = new RawSpStore(new GameRandom(1), () => 1); sp.Set(1, BitConverter.SingleToUInt32Bits(50));
        var state = new SquidRewardState(); var reward = new RawSquidReward(s.Pool, s.Types, s.Book, state, sp);
        s.Body(reward).PreDestroy(s.Sid, 0x10000);
        Assert.Equal(75f, BitConverter.UInt32BitsToSingle(sp.Get(1))); Assert.Equal(75, state.GeyserPool); Assert.Equal(1u, reward.VirtualCountCalls); Assert.Empty(s.Events);
        var other = new RawSquidReward(s.Pool, s.Types, new(), state, sp);
        Assert.Throws<ArgumentException>(() => s.Body(other));
    }

    /// <summary>Raw-only Form 부모를 허용하고 이미 만든 훅도 현재 부모의 Raw/typed getter를 다시 읽는다.</summary>
    [Fact]
    public void FormDispatchPreservesCurrentWholeSelectionAndOriginalNativeDestroyParent()
    {
        var s = new Scene(); int original = 0, replacement = 0;
        var assets = new SquidDestroyHooks { Emit = _ => { }, SelectedRaw = () => 0x10005 };
        var dispatch = new RawSquidFormDispatch(s.Pool, s.Destroy, assets, (_, _, _) => { original++; return false; });
        SquidDestroyHooks hooks = dispatch.Hooks(); Assert.Null(hooks.Selected); Assert.Equal(0x10005u, hooks.SelectedRaw!());
        dispatch.SetAssets(new() { Emit = _ => { }, SelectedRaw = () => 0x20005 }, (_, _, _) => { replacement++; return true; });
        Assert.Equal(0x20005u, hooks.SelectedRaw()); Assert.False(dispatch.Destroy(s.Sid)); Assert.Equal(1, original); Assert.Equal(0, replacement);
        dispatch.SetAssets(new() { Emit = _ => { }, Selected = () => 7 }, (_, _, _) => true); Assert.Equal(7u, hooks.SelectedRaw());
        s.Pool.RawSlot(s.Sid)[0] = 0x99; Assert.Throws<InvalidOperationException>(() => dispatch.Destroy(s.Sid));
    }

    /// <summary>실제 Form VT의 미등록 선택은 TYPE 별칭이 자산이어도 자산 경계로 보내지 않는다.</summary>
    [Fact]
    public void FormEffectRoutingUsesVtableInsteadOfMutableTypeAlias()
    {
        var s = new Scene(); int assets = 0;
        var dispatch = new RawSquidFormDispatch(s.Pool, s.Destroy, new() { Emit = _ => assets++, SelectedRaw = () => 0 });
        BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(s.Sid), 0x512110); s.Pool.RawSlot(s.Sid)[10] = 74;
        Assert.Throws<InvalidOperationException>(() => dispatch.Hooks().Emit!(new(SquidDestroyEffect.PreDestroy, s.Sid, 0, 0)));
        Assert.Equal(0, assets);
        BinaryPrimitives.WriteUInt32LittleEndian(s.Pool.RawSlot(s.Sid), OriginalConstructors.BaseVtable); s.Pool.RawSlot(s.Sid)[10] = 1;
        dispatch.Hooks().Emit!(new(SquidDestroyEffect.PostDestroy, s.Sid, 0, 0)); Assert.Equal(1, assets);
    }
}
