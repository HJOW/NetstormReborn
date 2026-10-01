using Netstorm.Assets;
using Netstorm.Core.Display;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>원본 1-1 맵의 시작 조건·수입·워크샵·일반 이동을 검사한다.</summary>
public sealed class CampaignOneTests
{
    /// <summary>시작 조건이 같으면 임시 AI도 결정적이며, 지식의 차이는 검사합에 반영된다.</summary>
    [Fact]
    public void AiAndProductionState_StayDeterministicAndAffectChecksum()
    {
        BattleSession first = Create(); BattleSession second = Create();
        first.RunTicks(first.TicksPerSecond * 30); second.RunTicks(second.TicksPerSecond * 30);
        Assert.Equal(first.Checksum(), second.Checksum());
        second.Player(1).Deck.LearnKnowledge("sunArcher");
        Assert.NotEqual(first.Checksum(), second.Checksum());
    }

    /// <summary>적 섬에 들어가 기절한 내 사제는 방어 AI의 운반·의식으로 제거되고 실패가 한 번 발생한다.</summary>
    [Fact]
    public void DefensiveAi_CapturesAndSacrificesInvadingPriest()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        MissionStart start = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission(CampaignAccess.FirstMission)!.Script) with { AiCollectors = 0 };
        // 실제 타입을 쓰되 경로·방어 행동을 분리해서 검사할 작은 두 섬 배치를 만든다.
        FortMapObject Object(string name, int owner, int x, int y) => new(x, y, x < 100 ? 2 : 1,
            new FortObject(0, 0, types.Find(name)!, null, null, null, null, null, owner, []));
        var map = new BattleMap(new[] { Object("priest", 1, 30, 20), Object("rainVortex", 1, 140, 100),
            Object("priest", 2, 70, 45), Object("windVortex", 2, 60, 65), Object("altar", 2, 65, 45),
            Object("sunWalker", 2, 30, 25), Object("sunCannon", 2, 25, 21) }, (x, _) => x < 100 ? 2 : 1);
        var session = new BattleSession(map, new BridgeGrid((_, _) => true), types, start) { CombatEnabled = true };
        int victim = session.Entities.First(e => e.Owner == 1 && e.Kind == ObjectKind.Priest).Id;
        session.RunTicks(session.TicksPerSecond * 300);
        IReadOnlyList<SessionEvent> events = session.DrainEvents();
        Assert.True(session.Entity(victim) == null, $"남은 사제: HP={session.Entity(victim)?.HitPoints}, 기절={session.Entity(victim)?.IsStunned}, 포획={session.Entity(victim)?.Captivity}; " + string.Join("; ", events.Where(e => e.Kind is not SessionEventKind.BridgePieceAdded).TakeLast(30).Select(e => $"{e.Kind}:{e.Text}")));
        Assert.Contains("Failed", session.ToldSections);
        Assert.Contains(events, e => e.Kind == SessionEventKind.SacrificeStarted && e.Player == 2);
        Assert.Single(events, e => e.Kind == SessionEventKind.MissionTell && e.Text == "Failed");
    }

    /// <summary>원본 머리 값을 적용하고 적은 0 SP·골렘 한 기에서 시작하며 사제 대신 골렘으로 수입을 얻는다.</summary>
    [Fact]
    public void OriginalMission_StartsWithConfiguredAiEconomy()
    {
        BattleSession session = Create();
        Assert.Equal(3000, session.Player(1).StormPower);
        Assert.Equal(0, session.Player(2).StormPower);
        Assert.Contains("sunAviary", session.Player(1).Deck.Knowledge);
        Assert.Contains("sunArcher", session.Player(2).Deck.Knowledge);
        GameEntity collector = Assert.Single(session.Entities, e => e.Owner == 2 && e.Kind == ObjectKind.Transport);
        GameEntity priest = Assert.Single(session.Entities, e => e.Owner == 2 && e.Kind == ObjectKind.Priest);
        Footprint initial = priest.Footprint;
        session.CombatEnabled = false;
        session.RunTicks(session.TicksPerSecond * 120);
        Assert.True(session.Player(2).StormPower > 0);
        Assert.Equal(initial, priest.Footprint);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CrystalDelivered && e.EntityId == collector.Id);
        Assert.DoesNotContain("BadTeamDead", session.ToldSections);
        Assert.DoesNotContain("Failed", session.ToldSections);
    }

    /// <summary>실제 생산 골렘도 가이저를 왕복하고 내 SP를 늘린다.</summary>
    [Fact]
    public void ProducedGolem_HarvestsAndCanStopWithoutLosingCrystals()
    {
        BattleSession session = Create();
        GameEntity golem = ProduceGolem(session);
        GameEntity temple = Assert.Single(session.Entities, e => e.Owner == 1 && e.Kind == ObjectKind.Temple);
        GameEntity geyser = session.Entities.Where(e => e.Kind == ObjectKind.Geyser)
            .OrderBy(e => Math.Abs(e.Footprint.AnchorX - golem.Footprint.AnchorX) + Math.Abs(e.Footprint.AnchorY - golem.Footprint.AnchorY)).First();
        int money = session.Player(1).StormPower;
        session.Submit(new HarvestGeyserCommand(1, geyser.Id, golem.Id));
        session.RunTicks(session.TicksPerSecond * 90);
        Assert.True(session.Player(1).StormPower > money, $"골렘 {golem.Footprint}, 신전 {temple.Footprint}, 가이저 {geyser.Footprint}: " + string.Join("; ", session.DrainEvents().Where(e => e.Kind == SessionEventKind.CommandRejected).Select(e => e.Text)));
        int carried = golem.CarriedCrystals;
        session.Submit(new StopEntityCommand(1, golem.Id)); session.RunTicks(1);
        Footprint stopped = golem.Footprint;
        int stoppedMoney = session.Player(1).StormPower;
        session.RunTicks(session.TicksPerSecond * 5);
        Assert.Equal(stopped, golem.Footprint);
        Assert.Equal(carried, golem.CarriedCrystals);
        Assert.Equal(stoppedMoney, session.Player(1).StormPower);
        session.Submit(new HarvestGeyserCommand(1, geyser.Id, session.Entities.First(e => e.Owner == 2 && e.Kind == ObjectKind.Transport).Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotOwner);
    }

    /// <summary>일반 이동은 지정 칸에 도착하며 정지 명령은 현재 위치를 유지한다.</summary>
    [Fact]
    public void MoveToCell_ReachesExactSupportedCell()
    {
        BattleSession session = Create();
        GameEntity golem = ProduceGolem(session);
        (int x, int y) = SessionData.FindCell((x, y) => session.Bridges.IsIsland(x, y) && !session.Map.IsOccupied(new Footprint(x, y, 1, 1))
            && Math.Abs(x - golem.Footprint.AnchorX) + Math.Abs(y - golem.Footprint.AnchorY) is >= 3 and <= 7);
        session.Submit(new MoveEntityCommand(1, golem.Id, x, y));
        session.RunTicks(session.TicksPerSecond * 15);
        Assert.Equal((x, y), (golem.Footprint.AnchorX, golem.Footprint.AnchorY));
        session.Submit(new MoveEntityCommand(2, golem.Id, x, y)); session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotOwner);
    }

    /// <summary>워크샵 두 생산 칸을 실제 지식으로 채우고 업그레이드 후 세 번째 타입을 등록한다.</summary>
    [Fact]
    public void Workshop_UpgradeUnlocksThirdStartingUnitAndChargesOnce()
    {
        BattleSession session = Create();
        (int x, int y) = SessionData.FindCell((x, y) => session.CheckBuilding(1, "sunFactory", x, y).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "sunFactory", x, y));
        session.RunTicks(session.TicksPerSecond * 25);
        GameEntity workshop = Assert.Single(session.Entities, e => e.Owner == 1 && e.Kind == ObjectKind.Workshop);
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "rainBattery"));
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "sunCannon"));
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "sunAviary"));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.RegisterFailed);
        int money = session.Player(1).StormPower;
        session.Submit(new UpgradeWorkshopCommand(1, workshop.Id));
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "sunAviary")); session.RunTicks(1);
        Assert.Equal(money - BattleSession.WorkshopUpgradeCost, session.Player(1).StormPower);
        Assert.Equal(2, session.Player(1).Deck.WorkshopLevel(workshop.Id));
        Assert.Contains(session.Player(1).Deck.Entries(), e => e.TypeName == "sunaviary" || e.TypeName == "sunAviary");
        session.Submit(new UpgradeWorkshopCommand(2, workshop.Id)); session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotOwner);
        Assert.Equal(money - BattleSession.WorkshopUpgradeCost, session.Player(1).StormPower);
    }

    /// <summary>와이드 해상도의 렌더링 높이가 정규화 과정에서 원본 높이로 되돌아가지 않는다.</summary>
    [Theory]
    [InlineData(1280, 720)] [InlineData(1280, 800)] [InlineData(1920, 1080)] [InlineData(1920, 1200)]
    public void Options_KeepWideResolution(int width, int height)
    {
        var settings = new DisplaySettings { WindowWidth = width, WindowHeight = height, ViewHeight = height };
        settings.Normalize();
        ScreenLayout layout = ScreenLayoutCalculator.Compute(width, height, settings.ViewHeight, settings.WideScreen);
        Assert.Equal((width, height), (layout.LogicalWidth, layout.LogicalHeight));
    }

    /// <summary>원본 성공 스크립트의 다음 미션(1-2)은 공개됐고, 그다음 1-3 은 구현 범위 밖이다.</summary>
    [Fact]
    public void SuccessScript_UnlocksOnlyImplementedNextMission()
    {
        MissionScript script = OriginalData.RequireResources().TryLoadMission(CampaignAccess.FirstMission)!.Script;
        Assert.True(CampaignAccess.IsAvailable(CampaignAccess.FirstMission));
        Assert.Contains("MasterOfWhirligigs", script.Text, StringComparison.OrdinalIgnoreCase);
        Assert.True(CampaignAccess.IsAvailable("MasterOfWhirligigs"));
        Assert.False(CampaignAccess.IsAvailable("SaveTheIsland"));
    }

    /// <summary>원본 1-1을 미션 머리 값과 실제 지면으로 시작한다.</summary>
    private static BattleSession Create()
    {
        MissionStart start = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission(CampaignAccess.FirstMission)!.Script);
        BattleSession session = SessionData.FromMap(CampaignAccess.FirstMission, start);
        session.CombatEnabled = false;
        return session;
    }

    /// <summary>돈·덱·소유 섬 판정을 통과한 칸에서 원본 골렘을 생산한다.</summary>
    private static GameEntity ProduceGolem(BattleSession session)
    {
        (int x, int y) = SessionData.FindCell((x, y) => session.CheckUnit(1, "sunWalker", x, y).Allowed);
        session.Submit(new PlaceUnitCommand(1, "sunWalker", x, y)); session.RunTicks(1);
        return Assert.Single(session.Entities, e => e.Owner == 1 && e.Kind == ObjectKind.Transport);
    }
}
