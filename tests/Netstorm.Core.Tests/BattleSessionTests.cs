using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 게임 세션(고정 틱 루프 + 명령 + 규칙 코어) 테스트.
/// 원본 튜토리얼 1·2 의 시작 상태와 사용자 직접 조작 관찰(2026-09-30)을 기준으로 삼는다:
/// 튜토리얼 2 는 10,000 SP → 템플 완공 5,000 → 워크샵 완공 4,200 → Sun Disc Thrower 300 SP 씩, 회수하면 원가의 25%.
/// 미션 머리의 techAllowed·denySalvage 는 시작 값이고 튜토리얼 단계 처리가 실행 중에 바꾼다(docs/exe/mission-header-flags.md).
/// 아래 테스트는 규칙만 따로 확인하려고 단계 B(sunFactory 허용)·단계 H(denySalvage 해제)의 효과를 손으로 재현한다.
/// 단계 처리 자체(A→I 흐름)는 <see cref="TutorialStagesTests"/> 가 확인한다.
/// </summary>
public sealed class BattleSessionTests
{
    /// <summary>이벤트 목록에서 종류별 개수를 센다</summary>
    private static int Count(IEnumerable<SessionEvent> events, SessionEventKind kind) => events.Count(e => e.Kind == kind);

    /// <summary>템플을 짓고 완공될 때까지 진행한다. 지은 템플을 돌려준다.</summary>
    private static GameEntity BuildTemple(BattleSession session)
    {
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "windVortex", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "windVortex", x, y));
        session.RunTicks(1);
        GameEntity temple = session.Entities.Single(e => e.Kind == ObjectKind.Temple);
        session.RunTicks((int)(temple.CompleteTick - session.Tick));
        return temple;
    }

    /// <summary>튜토리얼 2 시작: 10,000 SP·Sun Disc Thrower 지식·전투 옵션 덮어쓰기, 사제·거주지만 있고 템플은 없다</summary>
    [Fact]
    public void Tutorial2_StartsFromMissionHeader()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        PlayerState player = session.Player(1);
        Assert.Equal(10000, player.StormPower);
        Assert.Contains("sunArcher", player.Deck.Knowledge);
        Assert.False(player.HasTemple);
        // 튜토리얼 2 는 공급 반지름이 Short(14칸)로 시작한다
        Assert.Equal(14, session.Map.Options.GeneratorRadius);
        Assert.Contains(session.Entities, e => e.Kind == ObjectKind.Priest && e.Owner == 1);
        // 템플이 없으면 다리 조각이 생기지 않는다
        session.RunTicks(session.TicksPerSecond * 3);
        Assert.Empty(player.Tray.Pieces);
    }

    /// <summary>템플 건설: 비용은 시작할 때 나가고, 건설 시간(16초) 동안은 효과가 없다가 완공되면 섬 소유·다리 공급이 시작된다</summary>
    [Fact]
    public void Temple_TakesConstructionTimeThenOwnsIslandAndSuppliesBridges()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "windVortex", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "windVortex", x, y));
        session.RunTicks(1);
        GameEntity temple = session.Entities.Single(e => e.Kind == ObjectKind.Temple);
        Assert.Equal(5000, session.Player(1).StormPower);
        Assert.False(temple.IsComplete);
        Assert.Equal(16 * session.TicksPerSecond, temple.CompleteTick - temple.StartTick);
        // 건설 중에는 섬이 빈 섬이고, 건설 중인 템플도 "이미 템플이 있음"으로 세어 두 번째 템플은 지을 수 없다
        Assert.Null(session.Map.Ownership.OwnerOf(temple.Territory!.Value));
        (int x2, int y2) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "windVortex", cx, cy).Site is
            { Problem: PlacementProblem.TempleAlreadyExists });
        Assert.Equal(CommandFailure.Placement, session.CheckBuilding(1, "windVortex", x2, y2).Failure);
        // 완공 직전 틱까지는 건설 중이다
        session.RunTicks((int)(temple.CompleteTick - session.Tick) - 1);
        Assert.False(temple.IsComplete);
        Assert.False(session.Player(1).HasTemple);
        session.RunTicks(1);
        Assert.True(temple.IsComplete);
        Assert.Equal(1, session.Map.Ownership.OwnerOf(temple.Territory!.Value));
        Assert.Equal(temple.Id, session.Player(1).Deck.TempleId);
        Assert.Contains(session.Map.Sources, s => s.Id == temple.Id && s.Element == Element.Wind && s.Owner == 1);
        // 템플이 있으면 다리 조각이 1초마다 칸 수(기본 6)까지 채워진다
        session.RunTicks(session.TicksPerSecond * 8);
        Assert.Equal(session.Map.Options.BridgeSlotCount, session.Player(1).Tray.Pieces.Count);
        Assert.Equal(1, Count(session.DrainEvents(), SessionEventKind.BuildingCompleted));
    }

    /// <summary>튜토리얼 2 전체 흐름: 템플 → 워크샵 → 지식 등록 → Sun Disc Thrower 배치(재충전) → 회수. SP 가 관찰 값과 같다</summary>
    [Fact]
    public void Tutorial2_FullProductionFlowMatchesObservedStormPower()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        PlayerState player = session.Player(1);
        BuildTemple(session);
        Assert.Equal(5000, player.StormPower);

        // 머리의 techAllowed 에 sunFactory 가 없어 Construct 메뉴에 나오지 않는다. 원본은 튜토리얼 단계 B 에서 허용한다 (FUN_004c3bb0)
        (int sx, int sy) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "sunFactory", cx, cy).Site is { Allowed: true });
        Assert.Equal(CommandFailure.TechDenied, session.CheckBuilding(1, "sunFactory", sx, sy).Failure);
        player.Tech.Set("sunFactory", true);

        // 워크샵 건설(약 10초)
        (int wx, int wy) = SessionData.FindCell((cx, cy) => session.CheckBuilding(1, "sunFactory", cx, cy).Allowed);
        session.Submit(new ConstructBuildingCommand(1, "sunFactory", wx, wy));
        session.RunTicks(1);
        GameEntity workshop = session.Entities.Single(e => e.Kind == ObjectKind.Workshop);
        Assert.Equal(10 * session.TicksPerSecond, workshop.CompleteTick - workshop.StartTick);
        session.RunTicks((int)(workshop.CompleteTick - session.Tick));
        Assert.Equal(4200, player.StormPower);

        // 등록 전에는 자리가 맞아도 덱에 없어 놓을 수 없다
        (int ax, int ay) = SessionData.FindCell((cx, cy) => session.CheckUnit(1, "sunArcher", cx, cy).Site is { Allowed: true });
        Assert.Equal(CommandFailure.NotInDeck, session.CheckUnit(1, "sunArcher", ax, ay).Failure);
        session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, "sunArcher"));
        session.RunTicks(1);
        Assert.Contains("sunArcher", player.Deck.RegisteredAt(workshop.Id));

        // 첫 유닛: 300 SP, 곧바로 다시 놓으면 재충전 중이다 (Unit Rate Fast = 1초)
        session.Submit(new PlaceUnitCommand(1, "sunArcher", ax, ay));
        session.RunTicks(1);
        Assert.Equal(3900, player.StormPower);
        Assert.Equal(1.0, session.SecondsUntilReady(1, "sunArcher"), 1);
        (int bx, int by) = SessionData.FindCell((cx, cy) => session.CheckUnit(1, "sunArcher", cx, cy).Site is { Allowed: true });
        Assert.Equal(CommandFailure.NotReady, session.CheckUnit(1, "sunArcher", bx, by).Failure);
        session.DrainEvents();
        session.Submit(new PlaceUnitCommand(1, "sunArcher", bx, by));
        session.RunTicks(1);
        Assert.Equal(3900, player.StormPower);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected && e.Failure == CommandFailure.NotReady);

        // 1초 뒤에는 다시 놓을 수 있다 → 둘째 유닛 300 SP
        session.RunTicks(session.TicksPerSecond);
        Assert.Equal(0, session.SecondsUntilReady(1, "sunArcher"));
        session.Submit(new PlaceUnitCommand(1, "sunArcher", bx, by));
        session.RunTicks(1);
        Assert.Equal(3600, player.StormPower);

        // 머리의 denySalvage = 1 이 남아 있는 동안은 회수되지 않는다. 원본은 튜토리얼 단계 H 에서 0 으로 바꾼다 (FUN_004c3bb0)
        GameEntity archer = session.EntityAt(bx, by)!;
        Assert.Equal(ObjectKind.Emplacement, archer.Kind);
        Assert.True(session.DenySalvage);
        session.DrainEvents();
        session.Submit(new SalvageCommand(1, archer.Id));
        session.RunTicks(1);
        Assert.Equal(3600, player.StormPower);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.SalvageDenied);
        session.DenySalvage = false;

        // 회수하면 원가(300)의 25% = 75 SP 를 돌려받고 자리가 비며 유닛이 사라진다
        session.Submit(new SalvageCommand(1, archer.Id));
        session.RunTicks(1);
        Assert.Equal(3675, player.StormPower);
        Assert.Null(session.Entity(archer.Id));
        Assert.False(session.Map.IsOccupied(archer.Footprint));
    }

    /// <summary>템플을 회수하면 섬이 빈 섬이 되고 다리 조각·골렘이 사라진다 (템플 5,000 → 1,250). 사제는 회수할 수 없다</summary>
    [Fact]
    public void Salvage_RemovesRuleEffectsOfBuildings()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        PlayerState player = session.Player(1);
        session.DenySalvage = false;
        GameEntity temple = BuildTemple(session);
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.NotEmpty(player.Tray.Pieces);
        Assert.Contains(player.Deck.Entries(), e => e.Kind == DeckEntryKind.Golem);
        int before = player.StormPower;
        session.Submit(new SalvageCommand(1, temple.Id));
        session.RunTicks(1);
        Assert.Equal(before + 1250, player.StormPower);
        Assert.False(player.HasTemple);
        Assert.Null(session.Map.Ownership.OwnerOf(temple.Territory!.Value));
        Assert.DoesNotContain(session.Map.Sources, s => s.Id == temple.Id);
        Assert.DoesNotContain(player.Deck.Entries(), e => e.Kind == DeckEntryKind.Golem);
        // 다음 갱신에서 칸의 조각이 비워지고 다시 채워지지 않는다
        session.RunTicks(session.TicksPerSecond * 2);
        Assert.Empty(player.Tray.Pieces);
        // 사제는 회수 대상이 아니다
        GameEntity priest = session.Entities.Single(e => e.Kind == ObjectKind.Priest);
        session.DrainEvents();
        session.Submit(new SalvageCommand(1, priest.Id));
        session.RunTicks(1);
        Assert.NotNull(session.Entity(priest.Id));
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.CannotSalvage);
    }

    /// <summary>튜토리얼 1: 처음부터 템플이 있어 다리 조각이 채워지고, 집어서 놓으면 다리가 생긴다. 놓을 수 없는 곳은 거부되고 조각은 그대로 든다</summary>
    [Fact]
    public void Tutorial1_PickAndPlaceBridgePiece()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        PlayerState player = session.Player(1);
        Assert.Equal(0, player.StormPower);
        Assert.True(player.HasTemple);
        session.RunTicks(session.TicksPerSecond * 8);
        Assert.Equal(6, player.Tray.Pieces.Count);
        // 조각을 집으면 칸에서 빠진다. 이미 집고 있으면 또 집을 수 없다
        session.Submit(new PickBridgePieceCommand(1, 0));
        session.Submit(new PickBridgePieceCommand(1, 0));
        session.DrainEvents();
        session.RunTicks(1);
        Assert.NotNull(player.HeldPiece);
        Assert.Equal(5, player.Tray.Pieces.Count);
        // 섬에 이어지지 않는 위치(월드 왼쪽 위 구석)는 거부되고 조각은 그대로 있다
        session.Submit(new PlaceBridgeCommand(1, 0, 0, 0));
        session.RunTicks(1);
        Assert.NotNull(player.HeldPiece);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.BridgeBlocked);
        (int rotation, int x, int y) = SessionData.FindBridgeSite(session, player.HeldPiece!.Pattern.Index);
        int cellsBefore = session.Bridges.Cells.Count;
        session.Submit(new PlaceBridgeCommand(1, rotation, x, y));
        session.RunTicks(1);
        Assert.Null(player.HeldPiece);
        Assert.True(session.Bridges.Cells.Count > cellsBefore);
        // 집은 조각을 되돌리면 칸으로 돌아간다
        session.Submit(new PickBridgePieceCommand(1, 0));
        session.Submit(new ReturnBridgePieceCommand(1));
        session.RunTicks(1);
        Assert.Null(player.HeldPiece);
    }

    /// <summary>한쪽만 섬에 붙은 다리 한 칸은 40초에 금이 가고 80초에 무너진다 (10초 주기·수명 7→0, 5 아래 금 감)</summary>
    [Fact]
    public void DanglingBridge_CracksThenCollapsesOnTick()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        (int rotation, int x, int y) = SessionData.FindBridgeSite(session, BridgePatternCatalog.SinglePiece);
        session.Bridges.Place(new BridgePiece(BridgePatternCatalog.SinglePiece, rotation), x, y, 1);
        session.DrainEvents();
        // 39초까지는 금이 가지 않는다
        session.RunTicks(39 * session.TicksPerSecond);
        Assert.Equal(BridgeCondition.Normal, session.Bridges.At(x, y)!.Condition);
        // 40초 갱신에서 금이 간다
        session.RunTicks(1 * session.TicksPerSecond);
        Assert.Equal(BridgeCondition.Cracked, session.Bridges.At(x, y)!.Condition);
        Assert.Equal(1, Count(session.DrainEvents(), SessionEventKind.BridgeCracked));
        // 80초 갱신에서 무너진다
        session.RunTicks(40 * session.TicksPerSecond);
        Assert.Null(session.Bridges.At(x, y));
        Assert.Equal(1, Count(session.DrainEvents(), SessionEventKind.BridgeCollapsed));
    }

    /// <summary>
    /// 기술 허용 표는 머리 techAllowed 로 시작해 실행 중에 바뀐다. 튜토리얼 2 는 windVortex·sunArcher 만 허용하고 시작하며
    /// (사용자가 관찰한 Sun Workshop 은 단계 B 가 허용한 뒤에 지었다), 튜토리얼 1 은 windVortex 만 허용한다.
    /// </summary>
    [Fact]
    public void TechTable_StartsFromHeaderAndChangesAtRuntime()
    {
        BattleSession two = SessionData.FromMission("tutorial2");
        Assert.True(two.Player(1).Tech.IsAllowed("windVortex"));
        Assert.True(two.Player(1).Tech.IsAllowed("sunArcher"));
        Assert.False(two.Player(1).Tech.IsAllowed("sunFactory"));
        (int x, int y) = SessionData.FindCell((cx, cy) => two.CheckBuilding(1, "windVortex", cx, cy).Allowed);
        Assert.Equal(CommandFailure.TechDenied, two.CheckBuilding(1, "sunFactory", x, y).Failure);
        two.Player(1).Tech.Set("sunFactory", true);
        Assert.True(two.CheckBuilding(1, "sunFactory", x, y).Allowed);
        // 표를 통째로 다시 채우면(원본 FUN_004c23c0) 개별 값은 사라진다
        two.Player(1).Tech.SetAll(false);
        Assert.False(two.Player(1).Tech.IsAllowed("sunFactory"));
        Assert.False(two.Player(1).Tech.IsAllowed("windVortex"));

        BattleSession one = SessionData.FromMission("tutorial1");
        Assert.True(one.Player(1).Tech.IsAllowed("windVortex"));
        Assert.False(one.Player(1).Tech.IsAllowed("sunFactory"));
    }

    /// <summary>
    /// 같은 <see cref="MissionStart"/> 로 만든 두 세션은 기술 허용 표를 따로 가진다: 한쪽을 바꿔도 다른 쪽과 미션 원본 표는 그대로다.
    /// (이전에는 미션의 표 객체를 그대로 플레이어에게 넘겨 한 세션의 튜토리얼 단계 처리가 다른 세션에 새었다.)
    /// </summary>
    [Fact]
    public void TechTable_IsIndependentPerSession()
    {
        GameResources resources = OriginalData.RequireResources();
        MissionStart start = MissionStart.FromScript(resources.TryLoadMission("tutorial2")!.Script);
        BattleSession a = SessionData.FromMap(start.LoadFort!, start);
        BattleSession b = SessionData.FromMap(start.LoadFort!, start);
        a.Player(1).Tech.Set("sunFactory", true);
        Assert.True(a.Player(1).Tech.IsAllowed("sunFactory"));
        Assert.False(b.Player(1).Tech.IsAllowed("sunFactory"));
        Assert.False(start.Tech.IsAllowed("sunFactory"));
    }

    /// <summary>검사합에는 기술 허용 표와 회수 금지가 들어간다: 이 상태가 다르면 같은 명령이 다른 결과를 내므로 동기화 검사가 잡아야 한다.</summary>
    [Fact]
    public void Checksum_CoversTechTableAndSalvageDenial()
    {
        BattleSession a = SessionData.FromMission("tutorial2");
        BattleSession b = SessionData.FromMission("tutorial2");
        Assert.Equal(a.Checksum(), b.Checksum());
        a.Player(1).Tech.Set("sunFactory", true);
        Assert.NotEqual(a.Checksum(), b.Checksum());
        b.Player(1).Tech.Set("SUNFACTORY", true);
        // 이름 대소문자가 달라도 같은 표면 같은 검사합이어야 한다
        Assert.Equal(a.Checksum(), b.Checksum());
        a.DenySalvage = !a.DenySalvage;
        Assert.NotEqual(a.Checksum(), b.Checksum());
    }

    /// <summary>회수 금지는 머리 denySalvage 로 시작하고, 켜져 있는 동안 회수 명령은 아무것도 바꾸지 않는다 (튜토리얼 1·2 모두 1 로 시작)</summary>
    [Fact]
    public void DenySalvage_StartsFromHeaderAndBlocksSalvage()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        Assert.True(session.DenySalvage);
        GameEntity temple = session.Entities.Single(e => e.Kind == ObjectKind.Temple);
        session.Submit(new SalvageCommand(1, temple.Id));
        session.RunTicks(1);
        Assert.NotNull(session.Entity(temple.Id));
        Assert.Equal(0, session.Player(1).StormPower);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.SalvageDenied);
        // 시험 모드(생산 규칙 끔)에서는 금지가 적용되지 않는다
        session.EnforceProductionRules = false;
        session.Submit(new SalvageCommand(1, temple.Id));
        session.RunTicks(1);
        Assert.Null(session.Entity(temple.Id));
        // 튜토리얼 2 도 denySalvage = 1 로 시작한다 (단계 H 가 0 으로 바꾸기 전까지)
        Assert.True(SessionData.FromMission("tutorial2").DenySalvage);
    }

    /// <summary>배치 시험 모드(EnforceProductionRules 끔)는 덱에 없는 유닛도 규칙 조건만 맞으면 놓을 수 있다</summary>
    [Fact]
    public void WithoutProductionRules_AnyUnitCanBePlacedWhenSiteIsValid()
    {
        BattleSession session = SessionData.FromMission("tutorial2");
        BuildTemple(session);
        (int x, int y) = SessionData.FindCell((cx, cy) => session.CheckUnit(1, "sunArcher", cx, cy).Site is { Allowed: true });
        Assert.Equal(CommandFailure.NotInDeck, session.CheckUnit(1, "sunArcher", x, y).Failure);
        session.EnforceProductionRules = false;
        Assert.True(session.CheckUnit(1, "sunArcher", x, y).Allowed);
        session.Submit(new PlaceUnitCommand(1, "sunArcher", x, y));
        session.RunTicks(1);
        Assert.NotNull(session.EntityAt(x, y));
    }

    /// <summary>같은 시작·같은 명령열은 같은 검사합, 다른 명령이 하나라도 있으면 다른 검사합 (락스텝·리플레이 기반)</summary>
    [Fact]
    public void SameCommands_ProduceSameChecksum()
    {
        BattleSession a = SessionData.FromMission("tutorial2");
        BattleSession b = SessionData.FromMission("tutorial2");
        uint start = a.Checksum();
        Assert.Equal(start, b.Checksum());
        (int x, int y) = SessionData.FindCell((cx, cy) => a.CheckBuilding(1, "windVortex", cx, cy).Allowed);
        // 두 세션에 같은 명령을 넣고 같은 틱만큼 진행한다
        foreach (BattleSession session in new[] { a, b })
        {
            session.Submit(new ConstructBuildingCommand(1, "windVortex", x, y));
            session.RunTicks(500);
        }
        Assert.Equal(a.Checksum(), b.Checksum());
        Assert.NotEqual(start, a.Checksum());
        // 한쪽만 다리 조각을 집으면 갈라진다
        a.Submit(new PickBridgePieceCommand(1, 0));
        a.RunTicks(1);
        b.RunTicks(1);
        Assert.NotEqual(a.Checksum(), b.Checksum());
    }

    /// <summary>실제 시간은 24Hz 틱으로 바뀌고, 한 번에 따라잡을 틱 수에는 한도(8)가 있다</summary>
    [Fact]
    public void Advance_ConvertsRealTimeToFixedTicks()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        Assert.Equal(1, session.Advance(1.0 / 24));
        Assert.Equal(1, session.Tick);
        Assert.Equal(0, session.Advance(0.01));
        // 오래 멈췄다 풀려도 한 번에 8틱까지만 진행한다
        Assert.Equal(8, session.Advance(5.0));
        Assert.Equal(9, session.Tick);
        Assert.Equal(9.0 / 24, session.Seconds, 9);
    }

    /// <summary>
    /// 저장 다리는 엔티티가 아니므로 시작할 때 "회수되어 사라짐"으로 판정되면 안 된다 (화면이 저장 다리를 숨기던 버그의 회귀 검사).
    /// 다리가 많은 Dissolved Alliance! 에서 시작 직후 모든 저장 오브젝트·저장 다리가 남아 있고, 회수한 저장 오브젝트만 사라진다.
    /// </summary>
    [Fact]
    public void StoredObjects_AreNotRemovedAtStart_OnlySalvagedOnesDisappear()
    {
        BattleSession session = SessionData.FromMap("dissolvedalliance");
        FortMapObject[] objects = [.. session.Map.InitialObjects.Select(o => o.Item)];
        Assert.All(objects, o => Assert.False(session.IsInitialObjectRemoved(o) || session.IsStoredBridgeGone(o)));
        Assert.NotEmpty(session.StoredBridgeCells);
        // 내 Generator 하나를 회수하면 그 저장 오브젝트만 사라진다
        session.EnforceProductionRules = false;
        GameEntity generator = session.Entities.First(e => e.Owner == 1 && e.Kind == ObjectKind.Generator && e.Source != null);
        session.Submit(new SalvageCommand(1, generator.Id));
        session.RunTicks(1);
        Assert.True(session.IsInitialObjectRemoved(generator.Source!));
        Assert.Equal(1, objects.Count(o => session.IsInitialObjectRemoved(o)));
    }

    /// <summary>맵의 오브젝트 번호는 BattleMap 이 매긴 번호와 같고, 새로 받는 번호는 겹치지 않는다</summary>
    [Fact]
    public void Entities_UseBattleMapObjectIds()
    {
        BattleSession session = SessionData.FromMission("tutorial1");
        Assert.All(session.Entities, e => Assert.Contains(session.Map.InitialObjects, o => o.Id == e.Id && ReferenceEquals(o.Item, e.Source)));
        Assert.Contains(session.Entities, e => e.Kind == ObjectKind.Temple && e.Owner == 1 && e.IsComplete);
        int max = session.Map.InitialObjects.Max(o => o.Id);
        Assert.True(session.Map.NextId() > max);
    }
}
