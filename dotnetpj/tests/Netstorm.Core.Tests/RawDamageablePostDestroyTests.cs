using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 피해가능객체 삭제 후 처리와 HP 조회의 보호 조건을 검사한다.
/// 원본 fixture 가 담지 않는 거부 경로(미연결 효과·자료 크기·잘못된 번호·비유한 좌표)를 직접 구성해 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 일곱째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageablePostDestroyTests
{
    /// <summary>보호 검사용 장면. 풀·해시·타입 표·프레임 표를 함께 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버 아님).</summary>
        public readonly RawSidPool Pool = new(32768, false);

        /// <summary>4단계 공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>타입별 프레임 코드 표(10.78 188개).</summary>
        public readonly FrameCode[][] Frames = new FrameCode[OriginalConstructors.TypeCount][];

        /// <summary>HP 조회 전역.</summary>
        public readonly RawHitPointState HitPointState = new();

        /// <summary>삭제 후 처리 전역.</summary>
        public readonly DamageablePostDestroyState State = new();

        /// <summary>나간 외부 경계 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>장면을 만든다. 실제 할당 영역 5~127 을 확보한다.</summary>
        public Scene()
        {
            // 원본 분석 장면의 대상을 실제 할당 영역으로 준비한다
            for (int id = 5; id <= 127; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            FrameCode[] codes =
            [
                new FrameCode('A', 'P', 1, FrameCodeFlags.None),
                new FrameCode('A', 'P', 11, (FrameCodeFlags)0x20),
                new FrameCode('A', 'P', 21, (FrameCodeFlags)0x40),
                new FrameCode('A', 'P', 11, (FrameCodeFlags)0x60),
            ];
            for (int i = 0; i < Frames.Length; i++)
            {
                Frames[i] = codes;
            }
        }

        /// <summary>기록을 모두 지운다.</summary>
        public void ClearEvents() => Events.Clear();

        /// <summary>삭제 후 처리 훅을 만든다. 모든 하위 효과가 기록만 한다.</summary>
        /// <returns>삭제 후 처리 훅.</returns>
        public DamageablePostDestroyHooks Hooks()
        {
            return new DamageablePostDestroyHooks
            {
                DropDude = (sid, index, randomPoint) => Events.Add($"D:{sid}:{index}"),
                DropIsland = (x, y, flags, owner) => Events.Add($"I:{owner}"),
                Regular = (sid, @event, payload, count) => Events.Add($"R:{sid}:{count}"),
                HasBridgeFall = sid => false,
                NotifySurface = sid => Events.Add($"N:{sid}"),
                DisplayUpdate = sid => Events.Add($"U:{sid}"),
                Destroy = (sid, flags) => Events.Add($"X:{sid}"),
                Base = (sid, flags) => Events.Add($"B:{sid}:{flags}"),
            };
        }
    }

    /// <summary>미연결 하위 효과와 잘못된 자료 크기는 실행 전에 거부한다.</summary>
    [Fact]
    public void MissingHooksAndBadDataSizesAreRejected()
    {
        var scene = new Scene();
        var hp = new RawHitPoints(scene.Pool, scene.Types, scene.HitPointState);
        var footprint = new RawSquidNeighbors(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames);
        // 여덟 하위 효과 중 하나라도 없으면 만들 수 없다
        DamageablePostDestroyHooks hooks = scene.Hooks();
        hooks.DropDude = null;
        Assert.Throws<ArgumentException>(() => new RawDamageablePostDestroy(scene.Pool, scene.Hash, scene.Types, scene.Frames, hp, footprint, scene.State, hooks));
        hooks = scene.Hooks();
        hooks.Base = null;
        Assert.Throws<ArgumentException>(() => new RawDamageablePostDestroy(scene.Pool, scene.Hash, scene.Types, scene.Frames, hp, footprint, scene.State, hooks));
        // 타입 표·프레임 표 크기가 다르면 연결하지 않는다
        Assert.Throws<ArgumentException>(() => new RawDamageablePostDestroy(scene.Pool, scene.Hash, new OriginalTypeRecord[1], scene.Frames, hp, footprint, scene.State, scene.Hooks()));
        // 다른 풀의 HP 조회·이웃 몸체는 연결하지 않는다
        var other = new RawSidPool(32768, false);
        var otherHp = new RawHitPoints(other, scene.Types, scene.HitPointState);
        Assert.Throws<ArgumentException>(() => new RawDamageablePostDestroy(scene.Pool, scene.Hash, scene.Types, scene.Frames, otherHp, footprint, scene.State, scene.Hooks()));
    }

    /// <summary>할당되지 않은 번호와 타입 표 밖 번호는 효과를 내기 전에 거부한다.</summary>
    [Fact]
    public void InvalidSidAndTypeAreRejectedBeforeAnyEffect()
    {
        var scene = new Scene();
        var hp = new RawHitPoints(scene.Pool, scene.Types, scene.HitPointState);
        var footprint = new RawSquidNeighbors(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames);
        var post = new RawDamageablePostDestroy(scene.Pool, scene.Hash, scene.Types, scene.Frames, hp, footprint, scene.State, scene.Hooks());
        // 0 번과 풀 밖 번호는 거부한다
        scene.ClearEvents();
        Assert.Throws<ArgumentOutOfRangeException>(() => post.PostDestroy(0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => post.PostDestroy(scene.Pool.Capacity, 0));
        Assert.Empty(scene.Events);
        // 타입 표 밖 번호도 거부한다
        scene.Pool.RawSlot(50)[RawSidPool.TypeOffset] = 200;
        scene.ClearEvents();
        Assert.Throws<ArgumentOutOfRangeException>(() => post.PostDestroy(50, 0));
        Assert.Empty(scene.Events);
    }

    /// <summary>유한하지 않은 좌표는 탐색 영역 계산에서 거부한다.</summary>
    [Fact]
    public void NonFiniteCoordinateIsRejected()
    {
        var scene = new Scene();
        var hp = new RawHitPoints(scene.Pool, scene.Types, scene.HitPointState);
        var footprint = new RawSquidNeighbors(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames);
        var post = new RawDamageablePostDestroy(scene.Pool, scene.Hash, scene.Types, scene.Frames, hp, footprint, scene.State, scene.Hooks());
        // 부모 타입을 만들고 섬 정리 경로가 좌표를 읽게 한 뒤 좌표를 NaN 으로 둔다
        scene.Types[160].FootX = 1;
        scene.Types[160].FootY = 1;
        scene.Types[160].Flags1 = 0x400;
        BitConverter.TryWriteBytes(scene.Pool.RawSlot(50)[14..], BitConverter.UInt32BitsToSingle(0x7FC00000));
        scene.Pool.RawSlot(50)[RawSidPool.TypeOffset] = 160;
        scene.ClearEvents();
        Assert.Throws<ArgumentOutOfRangeException>(() => post.PostDestroy(50, 0));
        Assert.Empty(scene.Events);
    }

    /// <summary>HP 조회는 번호·타입 범위와 debug assert 를 지킨다.</summary>
    [Fact]
    public void HitPointLookupKeepsRangeAndAssertRules()
    {
        var scene = new Scene();
        var hp = new RawHitPoints(scene.Pool, scene.Types, scene.HitPointState);
        // 0 번과 풀 밖 번호는 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => hp.HasHitPoints(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => hp.MaxHitPoints(scene.Pool.Capacity));
        // 타입 표 밖 번호는 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => hp.TypeHitPoints(200));
        // debug assert 를 켜면 HP 없는 타입은 거부한다
        scene.HitPointState.DebugAsserts = true;
        scene.Types[160].Flags1 = 0;
        Assert.Throws<InvalidOperationException>(() => hp.TypeHitPoints(160));
        // 사제 전용 HP 1/4 규칙을 켜면 사제 타입만 나누어진다
        scene.HitPointState.DebugAsserts = false;
        scene.Types[158].MaxHitPoints = 100;
        scene.Types[160].MaxHitPoints = 100;
        scene.HitPointState.PriestQuarterHp = true;
        Assert.Equal(25, hp.TypeHitPoints(158));
        Assert.Equal(100, hp.TypeHitPoints(160));
    }
}
