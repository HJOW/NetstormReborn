using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 일반 피해와 burning 판정의 보호 조건을 검사한다.
/// 원본 fixture 가 담지 않는 거부 경로(미연결 효과·죽음/abstract 표적·잘못된 번호)를 직접 구성해 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 여덟째 조각, 2026-10-21).
/// </summary>
public sealed class RawDamageableHitTests
{
    /// <summary>보호 검사용 장면. 풀·타입 표·전역을 함께 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>피격 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool = new(24000, false);

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>현재 레벨·권한 전역.</summary>
        public readonly PriestDamageState Levels = new();

        /// <summary>현재 일반 피해 전역.</summary>
        public readonly DamageableHitState State = new();

        /// <summary>현재 burning 전역.</summary>
        public readonly DamageableBurningState BurningState = new();

        /// <summary>나간 외부 경계 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>장면을 만든다. 클라이언트 번호 5~70 을 확보한다.</summary>
        public Scene()
        {
            // 각 독립 입력은 같은 원본 번호를 사용한다
            for (int id = 5; id <= 70; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            State.Levels = Levels;
            // 피해 대상을 유효한 자산으로 만든다
            Span<byte> raw = Pool.RawSlot(50);
            raw.Clear();
            raw[RawSidPool.TypeOffset] = 71;
            raw[RawSidPool.StateOffset] = 4;
            raw[34] = 1;
            Types[71].MaxHitPoints = 300;
            Types[71].Flags1 = 0x10;
            Types[71].DamageEffect = 1;
        }

        /// <summary>기록을 모두 지운다.</summary>
        public void ClearEvents() => Events.Clear();

        /// <summary>일반 피해 훅을 만든다. 모든 하위 효과가 기록만 한다.</summary>
        /// <returns>일반 피해 훅.</returns>
        public DamageableHitHooks Hooks()
        {
            return new DamageableHitHooks
            {
                RejectDamage = (sid, attacker) => false,
                Destroy = (sid, flags) => Events.Add($"D:{sid}:{flags}"),
                OwnerDamageNotice = (owner, victim, attacker) => Events.Add($"N:{owner}:{victim}:{attacker}"),
                Refresh = sid => Events.Add($"H:{sid}"),
                Unrealize = sid => Events.Add($"U:{sid}"),
                IslandAt = (x, y) => 0,
                RealTime = () => 0,
                WarningEvent = () => Events.Add("V:12"),
                WarningSound = () => Events.Add("S:islandUnderAttack.wav"),
                WarningText = () => Events.Add("F:islandUnderAttack"),
            };
        }

        /// <summary>burning 훅을 만든다. 모든 하위 효과가 기록만 한다.</summary>
        /// <returns>burning 훅.</returns>
        public DamageableBurningHooks BurningHooks()
        {
            return new DamageableBurningHooks
            {
                Find = (sid, type, index) => 0,
                Create = sid => 90,
                Destroy = (sid, flags) => Events.Add($"X:{sid}:{flags}"),
            };
        }
    }

    /// <summary>가상 방어 판정과 서버 삭제 경계가 없으면 정상 완료로 바꾸지 않는다.</summary>
    [Fact]
    public void MissingDefenseAndDeleteAreRejected()
    {
        var scene = new Scene();
        var hit = new RawDamageableHit(scene.Pool, scene.Types, scene.State, new DamageableHitHooks());
        // 방어 판정이 없으면 효과를 쓰기 전에 거부한다
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => hit.BaseHit(50, 200, 2, 60));
        Assert.Empty(scene.Events);
        // 방어 판정은 있지만 삭제 경계가 없으면 사망 처리를 거부한다
        DamageableHitHooks hooks = scene.Hooks();
        hooks.RejectDamage = (sid, attacker) => false;
        hooks.Destroy = null;
        var noDelete = new RawDamageableHit(scene.Pool, scene.Types, scene.State, hooks);
        scene.Pool.RawSlot(50)[26] = 1;
        scene.Pool.RawSlot(50)[27] = 0;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => noDelete.BaseHit(50, 200, 2, 60));
        Assert.Equal(0, scene.State.KilledVictim);
    }

    /// <summary>죽은 표적과 abstract 표적은 효과 전에 거부하거나 자연 반환한다.</summary>
    [Fact]
    public void DeadAndAbstractVictimsAreRejectedOrIgnored()
    {
        var scene = new Scene();
        var hit = new RawDamageableHit(scene.Pool, scene.Types, scene.State, scene.Hooks());
        // 죽음 비트는 패치판에서 assert 경로다
        scene.Pool.RawSlot(50)[RawSidPool.StateOffset] = 6;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => hit.BaseHit(50, 10, 2, 60));
        Assert.Empty(scene.Events);
        // abstract 표적은 자연 반환한다
        scene.Pool.RawSlot(50)[RawSidPool.StateOffset] = 4;
        scene.Pool.RawSlot(50)[40] = 1;
        scene.ClearEvents();
        Assert.True(hit.BaseHit(50, 10, 2, 60));
        Assert.Empty(scene.Events);
        // held 진단이 켜져 있으면 abstract 표적도 거부한다
        scene.State.Checking = true;
        scene.ClearEvents();
        Assert.Throws<InvalidOperationException>(() => hit.BaseHit(50, 10, 2, 60));
        Assert.Empty(scene.Events);
    }

    /// <summary>할당되지 않은 번호와 타입 표 밖 번호는 거부한다.</summary>
    [Fact]
    public void InvalidSidAndTypeAreRejected()
    {
        var scene = new Scene();
        var hit = new RawDamageableHit(scene.Pool, scene.Types, scene.State, scene.Hooks());
        Assert.Throws<ArgumentOutOfRangeException>(() => hit.BaseHit(0, 10, 2, 60));
        Assert.Throws<ArgumentOutOfRangeException>(() => hit.BaseHit(scene.Pool.Capacity, 10, 2, 60));
        scene.Pool.RawSlot(50)[RawSidPool.TypeOffset] = 200;
        Assert.Throws<ArgumentOutOfRangeException>(() => hit.BaseHit(50, 10, 2, 60));
    }

    /// <summary>burning 판정은 조회·생성·삭제 경계가 없으면 거부한다.</summary>
    [Fact]
    public void BurningMissingHooksAreRejected()
    {
        var scene = new Scene();
        var hp = new RawHitPoints(scene.Pool, scene.Types, new RawHitPointState());
        // 조회 콜백이 없으면 거부한다
        var noFind = new RawDamageableBurning(scene.Pool, scene.Types, hp, scene.BurningState, new DamageableBurningHooks());
        scene.Pool.RawSlot(50)[26] = 1;
        scene.Pool.RawSlot(50)[27] = 0;
        Assert.Throws<InvalidOperationException>(() => noFind.Refresh(50));
        // 저 HP 면 생성 경계가 필요하다
        DamageableBurningHooks hooks = scene.BurningHooks();
        hooks.Create = null;
        var noCreate = new RawDamageableBurning(scene.Pool, scene.Types, hp, scene.BurningState, hooks);
        Assert.Throws<InvalidOperationException>(() => noCreate.Refresh(50));
    }

    /// <summary>HP 조회는 번호·타입 범위를 지키고 사제 1/4 규칙을 적용한다.</summary>
    [Fact]
    public void HitPointWriteKeepsRawWidth()
    {
        var scene = new Scene();
        // 패치판은 26 번 자리에 네 바이트로 쓴다
        RawSquidHitPoints.Set(scene.Pool, 50, 123456);
        Assert.Equal(123456u, BitConverter.ToUInt32(scene.Pool.Slot(50)[26..30]));
        // HP 조회 몸체는 타입 표 밖 번호를 거부한다
        var hp = new RawHitPoints(scene.Pool, scene.Types, new RawHitPointState());
        Assert.Throws<ArgumentOutOfRangeException>(() => hp.TypeHitPoints(200));
        Assert.Throws<ArgumentOutOfRangeException>(() => hp.HasHitPoints(0));
    }
}
