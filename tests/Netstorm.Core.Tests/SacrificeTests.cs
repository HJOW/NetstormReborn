using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>수송 사제 포획·제단 의식·구출 이벤트의 시간과 결정론을 검사한다.</summary>
public sealed class SacrificeTests
{
    /// <summary>기절하지 않은 적 사제는 일반 캠페인에서 집을 수 없다.</summary>
    [Fact]
    public void Capture_RejectsPriestBeforeStun()
    {
        BattleSession session = Create();
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.CommandRejected
            && item.Failure == CommandFailure.NotCapturable);
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
    }

    /// <summary>포획 후 제단까지 운반하고 내 사제가 옆에 있으면 의식이 시작된다.</summary>
    [Fact]
    public void Capture_CarriesCaptiveToAltarAndStartsRitual()
    {
        BattleSession session = Create(includeCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.RunTicks(session.TicksPerSecond * 10);
        Assert.True(captive.IsStunned);
        session.CombatEnabled = false;

        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        session.RunTicks(session.TicksPerSecond * 60);

        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
        Assert.Equal(altar.Id, captive.CaptorId);
        Assert.Equal(0, carrier.CarriedPriestId);
        Assert.Single(session.Rituals, ritual => ritual.AltarId == altar.Id);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.SacrificeStarted);
    }

    /// <summary>다섯 룬·희생 음성·제단 소멸·포로 제거 시각에 맞춰 승패 안내가 한 번 발생한다.</summary>
    [Fact]
    public void Ritual_UsesMeasuredTimelineAndDelaysVictoryUntilPriestRemoval()
    {
        MissionStart mission = CampaignMission();
        BattleSession session = Create(mission, includeCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.RunTicks(session.TicksPerSecond * 10);
        Assert.True(captive.IsStunned);
        session.CombatEnabled = false;
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        // 포획·운반 경로가 끝나 의식이 시작될 때까지 한 틱씩 진행한다.
        for (int tick = 0; tick < session.TicksPerSecond * 90 && session.Rituals.Count == 0; tick++)
        {
            session.RunTicks(1);
        }
        long started = Assert.Single(session.DrainEvents(), item => item.Kind == SessionEventKind.SacrificeStarted).Tick;

        session.RunTicks(Ticks(session, BattleSession.CompletionSeconds));
        SessionEvent[] completed = [.. session.DrainEvents()];
        Assert.Equal(BattleSession.SacrificeRuneCount, completed.Count(item => item.Kind == SessionEventKind.SacrificeRune));
        Assert.Contains(completed, item => item.Kind == SessionEventKind.SacrificeCompleted);
        Assert.True(session.Tick >= started + Ticks(session, BattleSession.CompletionSeconds));

        session.RunTicks(Ticks(session, BattleSession.SacrificeKillDelaySeconds));
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.PriestSacrificed);
        session.RunTicks(Ticks(session, BattleSession.AltarConsumeDelaySeconds - BattleSession.SacrificeKillDelaySeconds));
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.AltarConsumed);
        Assert.Null(session.Entity(altar.Id));
        Assert.Same(captive, session.Entity(captive.Id));
        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
        Assert.Equal(0, captive.CaptorId);
        Assert.DoesNotContain(session.DrainEvents(), item => item.Text == "BadTeamDead");

        int stormPowerBefore = session.Player(1).StormPower;
        session.RunTicks(Ticks(session, BattleSession.SacrificedPriestRemovalDelaySeconds));
        Assert.Null(session.Entity(captive.Id));
        // 적 사제를 완전히 처리하면 제단 주인이 스톰 파워 5,000을 받는다 (사용자 확인, 녹화 8,450 → 13,450).
        Assert.Equal(stormPowerBefore + BattleSession.SacrificeRewardStormPower, session.Player(1).StormPower);
        SessionEvent[] removed = [.. session.DrainEvents()];
        Assert.Single(removed, item => item.Kind == SessionEventKind.SacrificeRewarded);
        Assert.Single(removed, item => item.Text == "BadTeamDead");
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.DoesNotContain(session.DrainEvents(), item => item.Text == "BadTeamDead");
    }

    /// <summary>allowAnyCapture 미션은 기절·적대 여부와 관계없이 사제를 집고, 미션 섹션은 한 번만 알린다.</summary>
    [Fact]
    public void RescueMission_AllowsCaptureAndTellsCapturedOnce()
    {
        MissionStart mission = MissionStart.FromHeader(key => key switch
        {
            "allowAnyCapture" => "1",
            "ai2AllyList" => "1",
            _ => null,
        });
        BattleSession session = Create(mission);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(session.TicksPerSecond * 12);

        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);
        Assert.Equal(captive.Id, carrier.CarriedPriestId);
        Assert.Single(session.DrainEvents(), item => item.Text == "ai2PriestCaptured");
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.DoesNotContain(session.DrainEvents(), item => item.Text == "ai2PriestCaptured");
    }

    /// <summary>동맹 사제를 사람 섬에 내려놓으면 완전히 회복하고 PriestSaved 안내를 한 번 낸다.</summary>
    [Fact]
    public void RescueMission_DropOnHumanTerritoryTellsPriestSaved()
    {
        MissionStart mission = MissionStart.FromHeader(key => key switch
        {
            "allowAnyCapture" => "1",
            "ai2AllyList" => "1",
            _ => null,
        });
        BattleSession session = Create(mission, includeTemples: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(session.TicksPerSecond * 12);
        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);

        session.Submit(new DropPriestCommand(1, carrier.Id, 90, 90));
        session.RunTicks(session.TicksPerSecond * 90);
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.False(captive.IsStunned);
        Assert.Equal(captive.MaxHitPoints, captive.HitPoints);
        Assert.InRange(Math.Abs(captive.Footprint.AnchorX - 90) + Math.Abs(captive.Footprint.AnchorY - 90), 0, 2);
        Assert.Single(session.DrainEvents(), item => item.Text == "ai2PriestSaved");
    }

    /// <summary>운반 유닛이 파괴되면 포획된 사제가 그 자리에 완전히 회복해 풀려난다.</summary>
    [Fact]
    public void DestroyedCarrier_ReleasesAndHealsCaptive()
    {
        MissionStart mission = MissionStart.FromHeader(key => key == "allowAnyCapture" ? "1" : null);
        BattleSession session = Create(mission, includeEnemyCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.CombatEnabled = false;
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(session.TicksPerSecond * 12);
        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);
        session.CombatEnabled = true;
        session.RunTicks(session.TicksPerSecond * 3);

        Assert.Null(session.Entity(carrier.Id));
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.False(captive.IsStunned);
        Assert.Equal(captive.MaxHitPoints, captive.HitPoints);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.PriestReleased);
    }

    /// <summary>
    /// 내려놓기 이동도 수확·포획·제단 운반처럼 다리가 바뀌면 같은 목표로 길을 다시 찾는다.
    /// 앞쪽 구간이 끊기면 현재 발판에서 대기하고, 길을 다시 이으면 같은 목적지로 재개한다.
    /// </summary>
    [Fact]
    public void DropPriest_RecomputesRouteWhenBridgeIsCutMidTransit()
    {
        MissionStart mission = MissionStart.FromHeader(key => key == "allowAnyCapture" ? "1" : null);
        TypeCatalog types = OriginalData.RequireTypes();
        var objects = new List<FortMapObject>
        {
            Object(types, "priest", 2, 9, 50),
            Object(types, "sunwalker", 1, 10, 50),
        };
        // 가운데 물길(칸 11~19)로 나뉜 두 섬: x<=10 또는 x>=20 만 섬이다.
        bool IsIsland(int x, int _) => x <= 10 || x >= 20;
        var map = new BattleMap(objects, (x, _) => x < 50 ? 2 : 1, allied: (a, b) => a == b);
        var bridges = new BridgeGrid(IsIsland);
        var session = new BattleSession(map, bridges, types, mission);

        GameEntity captive = Assert.Single(session.Entities, e => e.Kind == ObjectKind.Priest);
        GameEntity carrier = Assert.Single(session.Entities, e => e.Kind == ObjectKind.Transport);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id));
        session.RunTicks(2);
        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);
        session.DrainEvents();

        // 물길을 한 칸짜리 수평 조각으로 잇는다 (ConnectPracticeGeyser 와 같은 방식).
        for (int x = 11; x <= 19; x++)
        {
            var piece = new BridgePiece(BridgePatternCatalog.SinglePiece, 1);
            BridgePlacementCheck check = session.Bridges.Check(piece, x, 50, 1);
            Assert.True(check.Allowed, $"다리 ({x}, 50): {check.Problem}");
            session.Bridges.Place(piece, x, 50, 1);
        }

        session.Submit(new DropPriestCommand(1, carrier.Id, 25, 50));
        session.RunTicks(1);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected);

        // 다리를 반쯤 건넌(물길 안) 상태까지 진행시킨다.
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.InRange(carrier.Footprint.AnchorX, 11, 19);
        Assert.NotNull(session.MovePurposeOf(carrier.Id));

        // 건너는 중인 구간의 다리를 날려 버린다 (금 감 → 소멸, 두 번 불러 완전히 없앤다).
        int versionBeforeCut = session.Bridges.Version;
        session.Bridges.WeakenAround(18, 50);
        session.Bridges.WeakenAround(18, 50);
        Assert.True(session.Bridges.Version > versionBeforeCut);

        session.RunTicks(session.TicksPerSecond * 3);

        // 발밑 칸은 남았으므로 낙하하지 않고 작업을 보존한 채 앞쪽 길 복구를 기다린다.
        Assert.Equal(UnitMovePurpose.DropPriest, session.MovePurposeOf(carrier.Id));
        Assert.True(session.IsMoveBlocked(carrier.Id));
        Assert.Equal(PriestCaptivity.Carried, captive.Captivity);
        Assert.True(carrier.Footprint.AnchorX < 20, "끊긴 다리 너머로 건너가면 안 됨");
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.MoveBlocked);

        // 원래 목표를 다시 명령하지 않고 끊긴 네 칸만 복구한다.
        for (int x = 16; x <= 19; x++)
            session.Bridges.Place(new BridgePiece(BridgePatternCatalog.SinglePiece, 1), x, 50, 1);
        session.RunTicks(session.TicksPerSecond * 7);
        Assert.False(session.IsMoveBlocked(carrier.Id));
        Assert.Null(session.MovePurposeOf(carrier.Id));
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.True(carrier.Footprint.AnchorX >= 20);
        Assert.Single(session.DrainEvents(), e => e.Kind == SessionEventKind.MoveResumed);
    }

    /// <summary>
    /// 희생된 사제가 제거될 때도 전투 파괴·회수와 같은 RemoveEntity 를 거쳐, 그 사제를 보고 있던 선택이 함께 풀린다
    /// (전에는 _entities 에서만 지워 선택 상태가 죽은 오브젝트 번호를 계속 가리킬 수 있었다).
    /// </summary>
    [Fact]
    public void Ritual_RemovingVictimClearsSelectionLikeOtherDestruction()
    {
        MissionStart mission = CampaignMission();
        BattleSession session = Create(mission, includeCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.RunTicks(session.TicksPerSecond * 10);
        Assert.True(captive.IsStunned);
        session.CombatEnabled = false;
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        for (int tick = 0; tick < session.TicksPerSecond * 90 && session.Rituals.Count == 0; tick++)
        {
            session.RunTicks(1);
        }
        Assert.Single(session.Rituals);

        // 묶인 사제를 선택해 둔 상태를 흉내 낸다 (상태 창으로 포로를 계속 보는 경우).
        session.Submit(new SelectEntityCommand(1, captive.Id));
        session.RunTicks(1);
        Assert.Equal(captive.Id, session.Player(1).SelectedEntityId);

        session.RunTicks(Ticks(session, BattleSession.CompletionSeconds + BattleSession.SacrificeKillDelaySeconds
            + BattleSession.AltarConsumeDelaySeconds + BattleSession.SacrificedPriestRemovalDelaySeconds));

        Assert.Null(session.Entity(captive.Id));
        Assert.Equal(0, session.Player(1).SelectedEntityId);
    }

    /// <summary>
    /// 룬 마크가 나타난 뒤 의식 사제가 떠나면 의식은 깨지지 않고 멈춘다: 그 룬은 제시간에 타고, 포로는 묶인 채 남으며,
    /// 다음 룬은 사제가 돌아와 첫 룬 지연이 지난 뒤 시작한다(2026-10-01 캠페인 1-2 녹화 Rain 룬).
    /// </summary>
    [Fact]
    public void Ritual_PerformerLeavingAfterMarkFinishesRuneThenWaitsForReturn()
    {
        BattleSession session = Create(CampaignMission(), includeCannon: true);
        (GameEntity captive, GameEntity altar, GameEntity performer) = StartRitual(session);
        long voice = RunUntil(session, SessionEventKind.SacrificeRune).Tick;
        session.RunTicks(Ticks(session, BattleSession.SacrificeRuneMarkSeconds + 0.5));
        session.Submit(new MoveEntityCommand(1, performer.Id, 80, 30));
        session.RunTicks(session.TicksPerSecond * 25);

        SessionEvent[] away = [.. session.DrainEvents()];
        Assert.Contains(away, item => item.Kind == SessionEventKind.SacrificePaused);
        Assert.DoesNotContain(away, item => item.Kind is SessionEventKind.SacrificeBroken or SessionEventKind.SacrificeRuneCancelled);
        // 확정된 Wind 룬은 사제가 없어도 음성 12.1초 뒤에 탄다.
        long burned = Assert.Single(away, item => item.Kind == SessionEventKind.SacrificeRuneBurned).Tick;
        Assert.InRange(burned - voice, Ticks(session, BattleSession.SacrificeRuneBurnSeconds) - 1, Ticks(session, BattleSession.SacrificeRuneBurnSeconds) + 1);
        Assert.DoesNotContain(away, item => item.Kind == SessionEventKind.SacrificeRune);
        AltarRitual ritual = Assert.Single(session.Rituals);
        Assert.True(ritual.PerformerAway);
        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
        Assert.Equal(altar.Id, captive.CaptorId);

        session.Submit(new MovePriestToAltarCommand(1, altar.Id, performer.Id));
        long resumed = RunUntil(session, SessionEventKind.SacrificeResumed).Tick;
        SessionEvent next = RunUntil(session, SessionEventKind.SacrificeRune);
        Assert.Equal("Sun", next.Text);
        Assert.InRange(next.Tick - resumed, Ticks(session, BattleSession.SacrificeRuneLeadSeconds) - 1, Ticks(session, BattleSession.SacrificeRuneLeadSeconds) + 1);
        RunUntil(session, SessionEventKind.SacrificeCompleted);
        Assert.Equal(BattleSession.SacrificeRuneCount, ritual.RunesBurned);
    }

    /// <summary>
    /// 음성만 나오고 마크가 나타나기 전에 사제가 떠나면 그 룬은 취소되고, 돌아오면 같은 룬을 음성부터 다시 한다
    /// (2026-10-01 캠페인 1-2 녹화 Thunder 룬: 18:34.6 음성 → 이탈 → 18:42.2 같은 음성 재생).
    /// </summary>
    [Fact]
    public void Ritual_PerformerLeavingBeforeMarkCancelsAndRepeatsRune()
    {
        BattleSession session = Create(CampaignMission(), includeCannon: true);
        (GameEntity captive, GameEntity altar, GameEntity performer) = StartRitual(session);
        RunUntil(session, SessionEventKind.SacrificeRune);
        session.RunTicks(Ticks(session, 0.5));
        session.Submit(new MoveEntityCommand(1, performer.Id, 80, 30));
        session.RunTicks(session.TicksPerSecond * 20);

        SessionEvent[] away = [.. session.DrainEvents()];
        Assert.Equal("Wind", Assert.Single(away, item => item.Kind == SessionEventKind.SacrificeRuneCancelled).Text);
        Assert.DoesNotContain(away, item => item.Kind is SessionEventKind.SacrificeRuneBurned or SessionEventKind.SacrificeRune);
        AltarRitual ritual = Assert.Single(session.Rituals);
        Assert.Equal(0, ritual.RunesStarted);
        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);

        session.Submit(new MovePriestToAltarCommand(1, altar.Id, performer.Id));
        SessionEvent again = RunUntil(session, SessionEventKind.SacrificeRune);
        Assert.Equal("Wind", again.Text);
        SessionEvent burned = RunUntil(session, SessionEventKind.SacrificeRuneBurned);
        Assert.InRange(burned.Tick - again.Tick, Ticks(session, BattleSession.SacrificeRuneBurnSeconds) - 1, Ticks(session, BattleSession.SacrificeRuneBurnSeconds) + 1);
    }

    /// <summary>
    /// 제단이 크게 다쳐도 포로는 풀리지 않고, 제단이 파괴돼 사라질 때 비로소 의식이 깨지고 포로가 회복해 풀려난다
    /// (사용자 확인 2026-10-01: 포로는 제단 파괴·판매로만 풀린다).
    /// </summary>
    [Fact]
    public void DamagedAltar_KeepsCaptiveBoundUntilAltarIsDestroyed()
    {
        MissionStart mission = MissionStart.FromHeader(key => key == "allowAnyCapture" ? "1" : null);
        BattleSession session = Create(mission, lowHpAltar: true, includeAltarCannon: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.CombatEnabled = false;
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        RunUntil(session, SessionEventKind.SacrificeStarted);
        session.CombatEnabled = true;

        bool sawHeavyDamage = false;
        // 제단이 사라질 때까지 한 틱씩 진행하며, 크게 다친 동안에도 포로가 묶여 있는지 본다
        for (int tick = 0; tick < session.TicksPerSecond * 60 && session.Entity(altar.Id) != null; tick++)
        {
            session.RunTicks(1);
            if (session.Entity(altar.Id) != null && altar.HitPoints < altar.MaxHitPoints / 2)
            {
                sawHeavyDamage = true;
                Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
                Assert.Single(session.Rituals);
            }
        }
        Assert.True(sawHeavyDamage);
        Assert.Null(session.Entity(altar.Id));
        Assert.Empty(session.Rituals);
        Assert.Equal(PriestCaptivity.Free, captive.Captivity);
        Assert.False(captive.IsStunned);
        Assert.Contains(session.DrainEvents(), item => item.Kind == SessionEventKind.SacrificeBroken);
    }

    /// <summary>
    /// 의식 사제가 적 공격으로 기절해도 포로는 풀리지 않고 의식만 멈춘다 (사용자 확인 2026-10-01).
    /// </summary>
    [Fact]
    public void StunnedPerformer_PausesRitualWithoutReleasingCaptive()
    {
        MissionStart mission = MissionStart.FromHeader(key => key == "allowAnyCapture" ? "1" : null);
        BattleSession session = Create(mission, includePerformerArcher: true);
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        session.CombatEnabled = false;
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        RunUntil(session, SessionEventKind.SacrificeStarted);
        GameEntity performer = Entity(session, ObjectKind.Priest, 1);
        session.CombatEnabled = true;
        // 적 포대가 의식 사제를 기절시킬 때까지 진행한다
        for (int tick = 0; tick < session.TicksPerSecond * 60 && !performer.IsStunned; tick++)
        {
            session.RunTicks(1);
        }
        Assert.True(performer.IsStunned);
        session.CombatEnabled = false;
        session.RunTicks(session.TicksPerSecond * 30);

        Assert.NotNull(session.Entity(altar.Id));
        AltarRitual ritual = Assert.Single(session.Rituals);
        Assert.True(ritual.PerformerAway);
        Assert.False(ritual.Completed);
        Assert.Equal(PriestCaptivity.Bound, captive.Captivity);
        SessionEvent[] events = [.. session.DrainEvents()];
        Assert.Contains(events, item => item.Kind == SessionEventKind.SacrificePaused);
        Assert.DoesNotContain(events, item => item.Kind is SessionEventKind.SacrificeBroken or SessionEventKind.PriestReleased);
    }

    /// <summary>같은 시드와 명령·틱열은 수송 경로·의식 상태·알림 집합까지 같은 검사합을 만든다.</summary>
    [Fact]
    public void RitualState_IsDeterministic()
    {
        BattleSession first = Create(includeCannon: true);
        BattleSession second = Create(includeCannon: true);
        first.RunTicks(first.TicksPerSecond * 2);
        second.RunTicks(second.TicksPerSecond * 2);
        first.CombatEnabled = false;
        second.CombatEnabled = false;
        GameEntity firstVictim = Entity(first, ObjectKind.Priest, 2);
        GameEntity secondVictim = Entity(second, ObjectKind.Priest, 2);
        GameEntity firstCarrier = Entity(first, ObjectKind.Transport, 1);
        GameEntity secondCarrier = Entity(second, ObjectKind.Transport, 1);
        int firstAltar = Entity(first, ObjectKind.Altar, 1).Id;
        int secondAltar = Entity(second, ObjectKind.Altar, 1).Id;
        first.Submit(new CapturePriestCommand(1, firstCarrier.Id, firstVictim.Id, firstAltar));
        second.Submit(new CapturePriestCommand(1, secondCarrier.Id, secondVictim.Id, secondAltar));
        first.RunTicks(first.TicksPerSecond * 90);
        second.RunTicks(second.TicksPerSecond * 90);
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>가짜 미션 헤더로 포획·승패 테스트 조건을 만든다.</summary>
    private static MissionStart CampaignMission() => MissionStart.FromHeader(key => key switch
    {
        "title" => "Sacrifice test",
        "loadFort" => "SacrificeTest",
        _ => null,
    });

    /// <summary>주어진 플레이어·분류의 고유 엔티티를 조회한다.</summary>
    private static GameEntity Entity(BattleSession session, ObjectKind kind, int owner) =>
        Assert.Single(session.Entities, entity => entity.Kind == kind && entity.Owner == owner);

    /// <summary>적 사제를 기절시켜 제단으로 운반하고, 내 사제가 옆에 있어 의식이 시작될 때까지 진행한다.</summary>
    private static (GameEntity Captive, GameEntity Altar, GameEntity Performer) StartRitual(BattleSession session)
    {
        GameEntity captive = Entity(session, ObjectKind.Priest, 2);
        session.RunTicks(session.TicksPerSecond * 10);
        Assert.True(captive.IsStunned);
        session.CombatEnabled = false;
        GameEntity carrier = Entity(session, ObjectKind.Transport, 1);
        GameEntity altar = Entity(session, ObjectKind.Altar, 1);
        session.Submit(new CapturePriestCommand(1, carrier.Id, captive.Id, altar.Id));
        RunUntil(session, SessionEventKind.SacrificeStarted);
        return (captive, altar, Entity(session, ObjectKind.Priest, 1));
    }

    /// <summary>해당 종류의 이벤트가 날 때까지 한 틱씩 진행하고 그 이벤트를 돌려준다 (다른 이벤트는 버린다, 최대 3분).</summary>
    private static SessionEvent RunUntil(BattleSession session, SessionEventKind kind)
    {
        // 이벤트가 날 때까지 한 틱씩 진행한다
        for (int tick = 0; tick < session.TicksPerSecond * 180; tick++)
        {
            session.RunTicks(1);
            SessionEvent? found = session.DrainEvents().FirstOrDefault(item => item.Kind == kind);
            if (found != null)
            {
                return found;
            }
        }
        throw new Xunit.Sdk.XunitException($"{kind} 이벤트가 나지 않았습니다.");
    }

    /// <summary>측정 초를 틱으로 바꿀 때 분수 틱이 생기면 다음 틱 경계로 올림한다.</summary>
    private static int Ticks(BattleSession session, double seconds) =>
        (int)Math.Ceiling(seconds * session.TicksPerSecond);

    /// <summary>원본 타입과 간단한 양측 지도로 포획·제단 규칙을 실행한다.</summary>
    private static BattleSession Create(MissionStart? mission = null, bool includeCannon = false, bool includeTemples = false,
        bool includeEnemyCannon = false, bool includeAltarCannon = false, bool lowHpAltar = false, bool includePerformerArcher = false)
    {
        TypeCatalog types = OriginalData.RequireTypes();
        var objects = new List<FortMapObject>
        {
            Object(types, "sunwalker", 1, 30, 15),
            Object(types, "priest", 2, 20, 9),
            Object(types, "altar", 1, 65, 30, lowHpAltar ? 100 : null),
            Object(types, "priest", 1, 66, 30),
        };
        if (includeCannon)
        {
            objects.Add(Object(types, "suncannon", 1, 10, 10));
        }
        if (includeTemples)
        {
            objects.Add(Object(types, "windvortex", 1, 100, 100));
            objects.Add(Object(types, "rainvortex", 2, 5, 80));
        }
        if (includeEnemyCannon)
        {
            objects.Add(Object(types, "suncannon", 2, 40, 10));
        }
        if (includeAltarCannon)
        {
            objects.Add(Object(types, "suncannon", 2, 55, 31));
        }
        if (includePerformerArcher)
        {
            // 축 제한 없는 적 궁수를 의식 사제(66,30) 바로 동쪽에 둔다 (의식 사제 기절 검사용)
            objects.Add(Object(types, "sunarcher", 2, 70, 30));
        }
        int? TerritoryAt(int x, int _) => x < 50 ? 2 : 1;
        bool Allied(int first, int second) => mission?.AreAllied(first, second) ?? first == second;
        var map = new BattleMap(objects, TerritoryAt, allied: Allied);
        var bridges = new BridgeGrid((_, _) => true);
        return new BattleSession(map, bridges, types, mission);
    }

    /// <summary>원본 타입을 사용해 맵 오브젝트를 만든다.</summary>
    private static FortMapObject Object(TypeCatalog types, string typeName, int owner, int x, int y, int? hitPoints = null)
    {
        TypeInfo type = types.Find(typeName) ?? throw new InvalidDataException(typeName);
        if (hitPoints is int value)
        {
            string properties = string.Join('\n', type.Definition.Properties.Select(property => $"{property.Key} = \"{property.Value}\";"));
            string flags = string.Join(' ', type.Definition.Flags);
            TypeDefinition definition = TypeDefinition.Parse($"typename {type.Name}\ntypeflags {flags};\n{{\n{properties}\nmaxHitPoints = {value};\n}}");
            type = type with { Definition = definition };
        }
        int territory = x < 50 ? 2 : 1;
        var item = new FortObject(0, 0, type, null, null, null, null, null, owner, []);
        return new FortMapObject(x, y, territory, item);
    }
}
