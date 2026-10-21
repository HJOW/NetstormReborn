using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 피해가능객체(Damageable) 사전 파괴와 캐리어 경계의 보호 조건을 검사한다.
/// 원본 fixture 가 담지 않는 거부 경로(미연결 효과·잘못된 자산·비유한 좌표·손상된 종속 체인)를 직접 구성해 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 다섯째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageablePreDestroyTests
{
    /// <summary>장면이 확보하는 슬롯 번호(5~63).</summary>
    private const int LastSlot = 63;

    /// <summary>삭제 대상 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>NaN 비트 패턴(float).</summary>
    private const uint NotANumber = 0x7FC00000;

    /// <summary>보호 검사용 장면. 풀·spot 지도·전역·기록을 함께 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool = new(24000, false);

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>내용물 검색기의 권한·타입 전역.</summary>
        public readonly ContainedFinderState FinderState = new() { CheckingDead = true };

        /// <summary>사제 해방 판정 전역.</summary>
        public readonly DamageablePreDestroyState State = new();

        /// <summary>나간 외부 호출 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>장면을 만든다. 클라이언트 번호 5~63 을 먼저 확보한다.</summary>
        public Scene()
        {
            // 부모 실제 Create와 같은 클라이언트 할당만 준비한다
            for (int id = 5; id <= LastSlot; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
        }

        /// <summary>슬롯 바이트를 직접 쓴다(장면 준비용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        public Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>유효한 자산 슬롯을 만든다(타입 158·void 상태·좌표 20,21).</summary>
        public void PrepareValid()
        {
            Span<byte> raw = Slot(Source);
            raw.Clear();
            raw[RawSidPool.TypeOffset] = 158;
            raw[RawSidPool.StateOffset] = 4;
            raw[ExtraOffset] = 0;
            // 좌표 20,21 로 둔다
            BitConverter.TryWriteBytes(raw[14..], 20.0f);
            BitConverter.TryWriteBytes(raw[18..], 21.0f);
        }

        /// <summary>기록을 모두 지운다.</summary>
        public void ClearEvents() => Events.Clear();

        /// <summary>피해가능객체 사전 파괴 훅을 만든다. 모든 하위 효과가 기록만 한다.</summary>
        /// <returns>피해가능객체 사전 파괴 훅.</returns>
        public DamageablePreDestroyHooks Hooks()
        {
            return new DamageablePreDestroyHooks
            {
                Collapse = sid => Events.Add($"C:{sid}"),
                Explosion = sid => Events.Add($"X:{sid}"),
                Sound = effect => Events.Add($"S:{effect.Sound}"),
                ReleaseContained = sid => Events.Add($"R:{sid}"),
                ValidateBase = (sid, flags) => Events.Add($"V:{sid}:{flags}"),
                Base = (sid, flags) => Events.Add($"B:{sid}:{flags}"),
            };
        }
    }

    /// <summary>미연결 하위 효과와 잘못된 spot 지도 크기는 효과를 부르기 전에 거부한다.</summary>
    [Fact]
    public void MissingHooksAndBadMapSizeAreRejected()
    {
        var scene = new Scene();
        scene.PrepareValid();
        // 여섯 하위 효과 중 하나라도 없으면 만들 수 없다
        DamageablePreDestroyHooks hooks = scene.Hooks();
        hooks.Sound = null;
        Assert.Throws<ArgumentException>(() => new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, hooks));
        hooks = scene.Hooks();
        hooks.ReleaseContained = null;
        Assert.Throws<ArgumentException>(() => new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, hooks));
        // spot 지도 크기가 다르면 연결하지 않는다
        Assert.Throws<ArgumentException>(() => new RawDamageablePreDestroy(scene.Pool, new byte[10], scene.FinderState, scene.State, scene.Hooks()));
        // 캐리어 경계도 하위 효과 세 개가 모두 있어야 한다
        var carrierHooks = new CarrierPreDestroyHooks
        {
            ValidateDamageable = (sid, flags) => { },
            DamageablePre = (sid, flags) => { },
            DependentFound = null,
        };
        Assert.Throws<ArgumentException>(() => new RawCarrierPreDestroy(scene.Pool, scene.FinderState, carrierHooks));
    }

    /// <summary>빈·내용물 상태와 자산 밖 타입은 효과를 내기 전에 거부하고 슬롯을 보존한다.</summary>
    [Fact]
    public void InvalidAssetStateAndTypeAreRejectedBeforeAnyEffect()
    {
        var scene = new Scene();
        var damageable = new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, scene.Hooks());

        // 빈 슬롯은 풀 접근 자체가 거부한다
        scene.PrepareValid();
        scene.Slot(Source)[RawSidPool.StateOffset] = 1;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => damageable.PreDestroy(Source, 0));
        Assert.Empty(scene.Events);

        // 들어 있는 내용물 상태도 거부한다
        scene.PrepareValid();
        scene.Slot(Source)[RawSidPool.StateOffset] = 8;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => damageable.PreDestroy(Source, 0));
        Assert.Empty(scene.Events);

        // 자산 밖 타입 번호도 거부한다
        scene.PrepareValid();
        scene.Slot(Source)[RawSidPool.TypeOffset] = 69;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => damageable.PreDestroy(Source, 0));
        Assert.Empty(scene.Events);

        // 타입 표 밖 번호도 거부한다
        scene.PrepareValid();
        scene.Slot(Source)[RawSidPool.TypeOffset] = 188;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => damageable.PreDestroy(Source, 0));
        Assert.Empty(scene.Events);
    }

    /// <summary>유한하지 않거나 지도 밖인 좌표는 spot 을 읽기 전에 거부한다.</summary>
    [Fact]
    public void NonFiniteOrOutOfMapCoordinateIsRejected()
    {
        var scene = new Scene();
        var damageable = new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, scene.Hooks());

        // NaN 좌표는 spot 좌표 계산에서 거부한다
        scene.PrepareValid();
        BitConverter.TryWriteBytes(scene.Slot(Source)[14..], BitConverter.UInt32BitsToSingle(NotANumber));
        scene.ClearEvents();
        Assert.Throws<ArgumentOutOfRangeException>(() => damageable.PreDestroy(Source, 0));
        Assert.Empty(scene.Events);

        // 지도 밖 좌표도 거부한다
        scene.PrepareValid();
        BitConverter.TryWriteBytes(scene.Slot(Source)[14..], 256.0f);
        scene.ClearEvents();
        Assert.Throws<ArgumentOutOfRangeException>(() => damageable.PreDestroy(Source, 0));
        Assert.Empty(scene.Events);
    }

    /// <summary>abstract·매림이면 좌표를 읽지 않고 공통 본체까지 전달한다.</summary>
    [Fact]
    public void AbstractOrBuriedSkipsCoordinatesButKeepsBaseCall()
    {
        var scene = new Scene();
        var damageable = new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, scene.Hooks());
        scene.PrepareValid();
        // 좌표를 읽지 않으므로 NaN 도 통과한다
        BitConverter.TryWriteBytes(scene.Slot(Source)[14..], BitConverter.UInt32BitsToSingle(NotANumber));
        scene.Slot(Source)[ExtraOffset] = 9;
        scene.ClearEvents();
        damageable.PreDestroy(Source, 0x300000);
        // 좌표 효과 없이 사전 검사와 공통 본체만 남는다
        Assert.Equal(["V:50:3145728", "B:50:3145728"], scene.Events);
    }

    /// <summary>공통 사전 검사가 거부하면 아무 효과도 내지 않는다.</summary>
    [Fact]
    public void BaseValidationFailureRejectsEverything()
    {
        var scene = new Scene();
        scene.PrepareValid();
        DamageablePreDestroyHooks hooks = scene.Hooks();
        hooks.ValidateBase = (sid, flags) => throw new InvalidOperationException("공통 효과 거부");
        var damageable = new RawDamageablePreDestroy(scene.Pool, scene.Spots, scene.FinderState, scene.State, hooks);
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => damageable.PreDestroy(Source, 1));
        Assert.Empty(scene.Events);
    }

    /// <summary>종속 체인이 순환하거나 풀 밖 번호를 가리키면 거부한다.</summary>
    [Fact]
    public void ContainedFinderRejectsBrokenChain()
    {
        var scene = new Scene();
        scene.PrepareValid();
        // 50 번이 60 번을 가리키고 60·61 번이 서로를 가리키게 만든다
        scene.Slot(Source)[6] = 60;
        scene.Slot(Source)[7] = 0;
        scene.Slot(60)[4] = 61;
        scene.Slot(61)[4] = 60;
        // 둘 다 내용물 타입이 아니어야 후보로-return 되지 않고 순환까지 간다
        scene.Slot(60)[RawSidPool.TypeOffset] = 7;
        scene.Slot(61)[RawSidPool.TypeOffset] = 7;
        var finder = new RawContainedFinder(scene.Pool, scene.FinderState);
        Assert.Throws<InvalidOperationException>(() => finder.Begin(Source));
    }

    /// <summary>권한 없는 죽은 부모 조회는 허용 플래그가 없을 때 거부한다.</summary>
    [Fact]
    public void DeadParentLookupNeedsPermissionOrAllowFlag()
    {
        var scene = new Scene();
        scene.PrepareValid();
        scene.Slot(Source)[RawSidPool.StateOffset] = 2;
        scene.FinderState.Boss = false;
        var finder = new RawContainedFinder(scene.Pool, scene.FinderState);
        Assert.Throws<InvalidOperationException>(() => finder.Begin(Source));
        // 죽은 부모를 허용하면 조회는 된다
        Assert.Equal(0, finder.Begin(Source, true));
    }
}
