using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 사제 피격과 HP 변경의 보호 조건을 검사한다.
/// 원본 fixture 가 담지 않는 거부 경로(권한 없음·회복 금지·가상 표 오류·공간 효과 누락·감시 쌍)를 직접 구성해 확인한다.
/// 기준 판본은 10.78 이다 (W03 삭제·수명 아홉째 조각, 2026-10-21).
/// </summary>
public sealed class RawPriestDamageTests
{
    /// <summary>사제 가상 표 값(10.78).</summary>
    private const uint PatchPriestVtable = 0x0050F210;

    /// <summary>피격 대상 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>보호 검사용 장면. 풀·타입/프레임 표·전역을 함께 만든다.</summary>
    private sealed class Scene
    {
        /// <summary>피격 대상 슬롯 풀(24,000칸, 서버 아님).</summary>
        public readonly RawSidPool Pool = new(24000, false);

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>타입별 프레임 코드 표(10.78 188개).</summary>
        public readonly FrameCode[][] Frames = new FrameCode[OriginalConstructors.TypeCount][];

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>사제 HP 변경의 권한·회복 허용 상태.</summary>
        public readonly PriestHitPointMode Mode = new();

        /// <summary>현재 레벨·권한 전역.</summary>
        public readonly PriestDamageState DamageState = new();

        /// <summary>나간 외부 경계 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>장면을 만든다. 클라이언트 번호 5~50 을 확보한다.</summary>
        public Scene()
        {
            for (int id = 5; id <= 50; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            for (int i = 0; i < Frames.Length; i++)
            {
                Frames[i] = [new FrameCode('A', default, 0, FrameCodeFlags.None)];
            }
            // 사제 타입(158)을 준비한다
            Types[158].MaxHitPoints = 200;
            Types[158].Flags1 = 0x69012;
            Types[158].Flags2 = 0x210000;
            Span<byte> raw = Pool.RawSlot(Source);
            raw.Clear();
            // 사제 가상 표 값을 낮은 바이트부터 쓴다
            for (int i = 0; i < 4; i++)
            {
                raw[i] = (byte)(PatchPriestVtable >> (8 * i));
            }
            raw[RawSidPool.TypeOffset] = 158;
            raw[RawSidPool.StateOffset] = 4;
            raw[34] = 1;
            // 저 HP(50) 에서 시작해 회복할 때 절반 경계를 넘게 만든다
            raw[26] = 50;
            Mode.Authority = true;
            Array.Fill(Mode.RecoveryAllowed, true);
        }

        /// <summary>기록을 모두 지운다.</summary>
        public void ClearEvents() => Events.Clear();

        /// <summary>사제 HP setter 몸체.</summary>
        /// <returns>HP setter.</returns>
        public RawPriestState PriestState()
        {
            var hp = new RawHitPoints(Pool, Types, new RawHitPointState());
            return new RawPriestState(Pool, hp, Types, Spots);
        }

        /// <summary>사제 피격 훅을 만든다. 모든 하위 효과가 기록만 한다.</summary>
        /// <returns>사제 피격 훅.</returns>
        public PriestDamageHooks Hooks(RawPriestState hpState)
        {
            return new PriestDamageHooks
            {
                HasRegenEvent = sid => false,
                CancelEvent = (sid, @event) => Events.Add($"C:{sid}:{@event}"),
                DefaultFrame = sid => 0,
                SetFrame = (sid, frame, flags) => Events.Add($"F:{sid}:{frame}:{flags}"),
                SetHitPoints = (sid, value) => hpState.SetHitPoints(sid, value, Mode, new PriestHitPointHooks
                {
                    Unpop = (target, flags) => Events.Add($"U:{target}"),
                    Repop = (target, flags) => Events.Add($"R:{target}"),
                }),
                OwnerDamageNotice = (owner, victim, attacker) => Events.Add($"N:{owner}:{victim}:{attacker}"),
                DamageableRefresh = sid => Events.Add($"H:{sid}"),
            };
        }
    }

    /// <summary>회복이 막힌 저 HP 회복은 원래 HP 를 되살리고 공간 효과를 내지 않는다.</summary>
    [Fact]
    public void BlockedRecoveryKeepsOldHitPoints()
    {
        var scene = new Scene();
        RawPriestState hpState = scene.PriestState();
        // 최대 HP 200 의 절반이 100 이므로 HP 100 에서 150 으로 올리면 저 HP 경계를 넘는다
        scene.Mode.RecoveryAllowed[1] = false;
        scene.ClearEvents();
        hpState.SetHitPoints(Source, 150, scene.Mode, new PriestHitPointHooks
        {
            Unpop = (target, flags) => scene.Events.Add($"U:{target}"),
            Repop = (target, flags) => scene.Events.Add($"R:{target}"),
        });
        // 회복이 막혔으므로 원래 HP 50 이 남고 공간 효과도 없다
        Assert.Equal(50, hpState.CurrentHitPoints(Source));
        Assert.Empty(scene.Events);
    }

    /// <summary>권한이 없으면 절반 경계값으로 되돌리고 공간 효과를 내지 않는다.</summary>
    [Fact]
    public void MissingAuthorityKeepsBoundaryValue()
    {
        var scene = new Scene();
        RawPriestState hpState = scene.PriestState();
        scene.Mode.Authority = false;
        scene.ClearEvents();
        hpState.SetHitPoints(Source, 150, scene.Mode, new PriestHitPointHooks
        {
            Unpop = (target, flags) => scene.Events.Add($"U:{target}"),
            Repop = (target, flags) => scene.Events.Add($"R:{target}"),
        });
        // 권한이 없으면 저 HP 경계값(99) 으로 되돌린다
        Assert.Equal(99, hpState.CurrentHitPoints(Source));        Assert.Empty(scene.Events);
    }

    /// <summary>사제 타입·가상 표가 아니거나 공간 효과가 없으면 거부한다.</summary>
    [Fact]
    public void InvalidPriestAndMissingSpaceHooksAreRejected()
    {
        var scene = new Scene();
        RawPriestState hpState = scene.PriestState();
        // 가상 표가 다르면 거부한다
        scene.Pool.RawSlot(Source)[0] = 0x11;
        Assert.Throws<InvalidOperationException>(() => hpState.SetHitPoints(Source, 150, scene.Mode, new PriestHitPointHooks()));
        for (int i = 0; i < 4; i++)
        {
            scene.Pool.RawSlot(Source)[i] = (byte)(PatchPriestVtable >> (8 * i));
        }
        // 타입이 사제가 아니어도 거부한다
        scene.Pool.RawSlot(Source)[RawSidPool.TypeOffset] = 71;
        Assert.Throws<InvalidOperationException>(() => hpState.SetHitPoints(Source, 150, scene.Mode, new PriestHitPointHooks()));
        scene.Pool.RawSlot(Source)[RawSidPool.TypeOffset] = 158;
        // 공간 효과가 없으면 거부한다
        Assert.Throws<ArgumentException>(() => hpState.SetHitPoints(Source, 150, scene.Mode, new PriestHitPointHooks()));
    }

    /// <summary>사제 피격은 죽음 assert·free 슬롯·필수 하위 효과 누락을 거부한다.</summary>
    [Fact]
    public void PriestHitRejectsBrokenState()
    {
        var scene = new Scene();
        RawPriestState hpState = scene.PriestState();
        var hit = new RawPriestDamage(scene.Pool, scene.Types, scene.Frames, scene.DamageState, scene.Hooks(hpState));
        // 죽음 비트는 패치판에서 assert 경로다
        scene.Pool.RawSlot(Source)[RawSidPool.StateOffset] = 6;
        Assert.Throws<InvalidOperationException>(() => hit.Hit(Source, 10, 2, 60));
        // free 슬롯은 거부한다
        scene.Pool.RawSlot(Source)[RawSidPool.StateOffset] = 1;
        Assert.Throws<InvalidOperationException>(() => hit.Hit(Source, 10, 2, 60));
        // 필수 하위 효과가 없으면 거부한다
        scene.Pool.RawSlot(Source)[RawSidPool.StateOffset] = 4;
        var noHooks = new RawPriestDamage(scene.Pool, scene.Types, scene.Frames, scene.DamageState, new PriestDamageHooks());
        Assert.Throws<InvalidOperationException>(() => noHooks.Hit(Source, 10, 2, 60));
    }

    /// <summary>HP 1 로 떨어져 감시 쌍을 끊어야 하는데 연결이 없으면 거부한다.</summary>
    [Fact]
    public void MissingDependentHostIsRejected()
    {
        var scene = new Scene();
        RawPriestState hpState = scene.PriestState();
        var hit = new RawPriestDamage(scene.Pool, scene.Types, scene.Frames, scene.DamageState, scene.Hooks(hpState));
        // 감시 Form(타입 1)을 종속 체인에 건다
        scene.Pool.RawSlot(Source)[6] = 51;
        scene.Pool.RawSlot(Source)[7] = 0;
        scene.Pool.RawSlot(51)[RawSidPool.TypeOffset] = 1;
        scene.Pool.RawSlot(51)[RawSidPool.StateOffset] = 4;
        // HP 를 1 로 만들고 권한이 있으면 감시 쌍을 끊어야 한다
        scene.Pool.RawSlot(Source)[26] = 1;
        scene.DamageState.Boss = true;
        Assert.Throws<InvalidOperationException>(() => hit.Hit(Source, 0, 2, 60));
    }
}
