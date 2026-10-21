using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 피해가능객체(Damageable) 종속 해방의 보호 조건을 검사한다.
/// 원본 fixture 가 담지 않는 거부 경로(미연결 효과·자료 크기·잘못된 부모·비유한 좌표·다른 풀 연결)를 직접 구성해 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 여섯째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageableReleaseTests
{
    /// <summary>장면이 확보하는 슬롯의 마지막 번호.</summary>
    private const int LastSlot = 103;

    /// <summary>해방 부모 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>NaN 비트 패턴(float).</summary>
    private const uint NotANumber = 0x7FC00000;

    /// <summary>보호 검사용 장면. 풀·spot 지도·타입 표·기록을 함께 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>해방 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool = new(24000, false);

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>내용물 검색기의 권한·타입 전역.</summary>
        public readonly ContainedFinderState FinderState = new() { CheckingDead = true };

        /// <summary>전투·허용 genus·다리 타입 전역.</summary>
        public readonly DamageableReleaseState State = new();

        /// <summary>나간 외부 호출 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>장면을 만든다. 클라이언트 번호 5~103 을 먼저 확보한다.</summary>
        public Scene()
        {
            // 실행기와 같은 번호 범위만 확보한다
            for (int id = 5; id <= LastSlot; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            // 해방할 부모를 유효한 자산으로 만든다
            Span<byte> raw = Pool.RawSlot(Source);
            raw.Clear();
            raw[RawSidPool.TypeOffset] = 160;
            raw[RawSidPool.StateOffset] = 4;
            BitConverter.TryWriteBytes(raw[14..], 20.0f);
            BitConverter.TryWriteBytes(raw[18..], 21.0f);
        }

        /// <summary>기록을 모두 지운다.</summary>
        public void ClearEvents() => Events.Clear();

        /// <summary>해방 훅을 만든다. 모든 하위 효과가 기록만 한다.</summary>
        /// <returns>해방 훅.</returns>
        public DamageableReleaseHooks Hooks()
        {
            return new DamageableReleaseHooks
            {
                SpawnPriest = (x, y, kind, word) => Events.Add($"P:{kind}:{word}"),
                Create = (kind, flags) =>
                {
                    Events.Add($"N:{kind}:{flags}");
                    return 100;
                },
                SetOwner = (sid, owner) => Events.Add($"O:{sid}:{owner}"),
                Pop = (sid, x, y, flags) => Events.Add($"L:{sid}"),
                NotifySurface = sid => Events.Add($"S:{sid}"),
            };
        }
    }

    /// <summary>미연결 하위 효과와 잘못된 자료 크기는 해방을 부르기 전에 거부한다.</summary>
    [Fact]
    public void MissingHooksAndBadDataSizesAreRejected()
    {
        var scene = new Scene();
        // 다섯 하위 효과 중 하나라도 없으면 만들 수 없다
        DamageableReleaseHooks hooks = scene.Hooks();
        hooks.Create = null;
        Assert.Throws<ArgumentException>(() => new RawDamageableRelease(scene.Pool, scene.Types, scene.Spots, scene.FinderState, scene.State, hooks));
        hooks = scene.Hooks();
        hooks.Pop = null;
        Assert.Throws<ArgumentException>(() => new RawDamageableRelease(scene.Pool, scene.Types, scene.Spots, scene.FinderState, scene.State, hooks));
        // spot 지도 크기가 다르면 연결하지 않는다
        Assert.Throws<ArgumentException>(() => new RawDamageableRelease(scene.Pool, scene.Types, new byte[10], scene.FinderState, scene.State, scene.Hooks()));
        // 타입 표 크기가 다르면 연결하지 않는다
        Assert.Throws<ArgumentException>(() => new RawDamageableRelease(scene.Pool, new OriginalTypeRecord[1], scene.Spots, scene.FinderState, scene.State, scene.Hooks()));
    }

    /// <summary>빈·내용물 상태의 부모와 자산 밖 타입은 효과를 내기 전에 거부한다.</summary>
    [Fact]
    public void InvalidParentStateAndTypeAreRejectedBeforeAnyEffect()
    {
        var scene = new Scene();
        var release = new RawDamageableRelease(scene.Pool, scene.Types, scene.Spots, scene.FinderState, scene.State, scene.Hooks());

        // 할당되지 않은 번호는 풀 접근 자체가 거부한다
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => release.Release(0));

        // 빈 슬롯도 풀 접근이 거부한다
        scene.Pool.RawSlot(Source)[RawSidPool.StateOffset] = 1;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => release.Release(Source));
        Assert.Empty(scene.Events);

        // 들어 있는 내용물 상태도 거부한다
        scene.Pool.RawSlot(Source)[RawSidPool.StateOffset] = 8;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => release.Release(Source));
        Assert.Empty(scene.Events);

        // 자산 밖 타입 번호도 거부한다
        scene.Pool.RawSlot(Source)[RawSidPool.StateOffset] = 4;
        scene.Pool.RawSlot(Source)[RawSidPool.TypeOffset] = 69;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => release.Release(Source));
        Assert.Empty(scene.Events);
    }

    /// <summary>유한하지 않은 좌표는 spot 을 읽기 전에 거부한다.</summary>
    [Fact]
    public void NonFiniteCoordinateIsRejected()
    {
        var scene = new Scene();
        var release = new RawDamageableRelease(scene.Pool, scene.Types, scene.Spots, scene.FinderState, scene.State, scene.Hooks());
        // NaN 좌표는 스냅 계산에서 거부한다
        BitConverter.TryWriteBytes(scene.Pool.RawSlot(Source)[14..], BitConverter.UInt32BitsToSingle(NotANumber));
        scene.ClearEvents();
        Assert.Throws<ArgumentOutOfRangeException>(() => release.Release(Source));
        Assert.Empty(scene.Events);
    }

    /// <summary>다른 풀의 해방 몸체는 연결할 수 없다.</summary>
    [Fact]
    public void DifferentPoolBindingIsRejected()
    {
        var scene = new Scene();
        var release = new RawDamageableRelease(scene.Pool, scene.Types, scene.Spots, scene.FinderState, scene.State, scene.Hooks());
        var other = new RawSidPool(24000, false);
        Assert.Throws<ArgumentException>(() => RawDamageableRelease.MakeDamageableReleaseHooks(other, release, new DamageablePreDestroyHooks()));
    }
}
