using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>생산 기점·원소 공급원·늦은 도착·기능 활성화·길 소멸·환불·결정성을 합성 지도에서 검증한다.</summary>
public sealed class UnitProductionTests
{
    /// <summary>합성 지도의 한 변 길이.</summary>
    private const int IslandSize = 70;
    /// <summary>각 생산 시험의 시작 스톰 파워.</summary>
    private const int StartingPower = 20000;

    /// <summary>작은 실제 타입 지도를 만든다. 기본은 템플 한 기와 두 Sun 워크샵이다.</summary>
    private static BattleSession Create(Func<int, int, bool>? ground = null, params FortMapObject[] objects)
    {
        ground ??= (x, y) => x is >= 0 and < IslandSize && y is >= 0 and < IslandSize;
        if (objects.Length == 0) objects = [Object("windVortex", 8, 8), Object("sunFactory", 12, 18), Object("sunFactory", 32, 18)];
        BattleSession? session = null;
        var grid = new BridgeGrid(ground, (x, y) => session?.Entities.Any(e => e.OccupiesGround && e.Footprint.Contains(x, y)) ?? false);
        session = new BattleSession(new BattleMap(objects, (_, _) => 0), grid, OriginalData.RequireTypes(), startStormPower: StartingPower);
        session.CombatEnabled = false;
        return session;
    }

    /// <summary>플레이어 1의 실제 타입 저장 오브젝트.</summary>
    private static FortMapObject Object(string type, int x, int y) =>
        new(x, y, 0, new FortObject(0, 0, OriginalData.RequireTypes().Find(type)!, null, null, null, null, null, 1, []));

    /// <summary>지식을 지정 워크샵에 등록한다.</summary>
    private static void Register(BattleSession session, string type, int workshopId = 2)
    {
        session.Player(1).Deck.LearnKnowledge(type);
        session.Submit(new RegisterKnowledgeCommand(1, workshopId, type));
        session.RunTicks(1);
        Assert.DoesNotContain(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected);
    }

    /// <summary>한 틱만 진행해 즉시 완성되지 않은 생산 예약을 돌려준다.</summary>
    private static GameEntity Order(BattleSession session, string type, int x, int y, int rotation = 0)
    {
        Assert.True(session.CheckUnit(1, type, x, y).Allowed, session.CheckUnit(1, type, x, y).Describe());
        HashSet<int> before = [.. session.Entities.Select(e => e.Id)];
        session.Submit(new PlaceUnitCommand(1, type, x, y, rotation));
        session.RunTicks(1);
        return Assert.Single(session.Entities, e => !before.Contains(e.Id));
    }

    /// <summary>골렘은 템플에서 출발하고 배치 때 한 번만 비용을 낸다. 자원 도착·실체화 뒤 이동할 수 있다.</summary>
    [Fact]
    public void Golem_UsesTempleAndWaitsForDeliveriesThenMaterializes()
    {
        BattleSession session = Create();
        GameEntity golem = Order(session, "sunwalker", 20, 8);
        UnitProduction production = Assert.IsType<UnitProduction>(golem.Production);
        Assert.Equal(1, production.SourceId);
        Assert.Equal(1, Assert.Single(production.Deliveries, d => d.IsStormPower).SourceId);
        Assert.False(golem.IsComplete);
        Assert.Equal(0, golem.HitPoints);
        Assert.Equal(StartingPower - golem.Cost, session.Player(1).StormPower);
        Assert.Equal(0, session.Player(1).Made("sunwalker"));
        session.Submit(new MoveEntityCommand(1, golem.Id, 25, 8));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotComplete);
        // 템플 경계에서 12칸 떨어진 골렘은 원소보다 느린 스톰 파워의 도착을 기다린다.
        session.RunTicks(session.TicksPerSecond / 2);
        Assert.True(production.AwaitingDeliveries);
        Assert.Equal(0, golem.StartTick);
        SessionData.RunUntilComplete(session, golem);
        Assert.Null(golem.Production);
        Assert.Equal(golem.MaxHitPoints, golem.HitPoints);
        Assert.Equal(1, session.Player(1).Made("sunwalker"));
        Assert.Equal(StartingPower - golem.Cost, session.Player(1).StormPower);
        session.Submit(new MoveEntityCommand(1, golem.Id, 25, 8));
        session.RunTicks(1);
        Assert.True(session.IsMoving(golem.Id));
    }

    /// <summary>가까운 다른 워크샵이 있어도 덱 등록에 사용한 워크샵에서 스톰 파워를 보낸다.</summary>
    [Fact]
    public void RegisteredUnit_UsesItsWorkshopInsteadOfNearestWorkshop()
    {
        BattleSession session = Create(null, Object("windVortex", 38, 30), Object("sunFactory", 12, 18), Object("sunFactory", 32, 18));
        Register(session, "sunCannon");
        GameEntity cannon = Order(session, "sunCannon", 40, 18);
        Assert.Equal(2, cannon.Production!.SourceId);
        ProductionDelivery storm = Assert.Single(cannon.Production.Deliveries, d => d.IsStormPower);
        Assert.InRange(storm.PreviousX, session.Entity(2)!.Footprint.Left, session.Entity(2)!.Footprint.AnchorX);
        Assert.True(storm.TargetX > session.Entity(3)!.Footprint.AnchorX);
        SessionData.RunUntilComplete(session, cannon);
        Assert.True(cannon.IsComplete);
    }

    /// <summary>요구 원소를 먼저 채우고 아무 원소 요구는 남은 가장 가까운 공급원으로 보낸다.</summary>
    [Fact]
    public void ElementResources_UseNearestDistinctCompatibleSources()
    {
        BattleSession session = Create(null, Object("windVortex", 38, 20), Object("thunderFactory", 12, 18),
            Object("thunderBattery", 42, 32), Object("thunderBattery", 47, 32), Object("windBattery", 53, 32));
        Register(session, "thunderCannon");
        GameEntity cannon = Order(session, "thunderCannon", 50, 40);
        ProductionDelivery[] energy = [.. cannon.Production!.Deliveries.Where(d => !d.IsStormPower)];
        Assert.Equal(2, energy.Length);
        Assert.Equal(Element.Thunder, energy[0].Element);
        Assert.Equal(4, energy[0].SourceId);
        Assert.Equal(5, energy[1].SourceId);
        Assert.All(energy, d => Assert.True(d.IsAirborne));
        Assert.Equal(2, energy.Select(d => d.SourceId).Distinct().Count());
    }

    /// <summary>스톰 파워가 먼저 도착했더라도 먼 원소 에너지가 도착할 때까지 실체화가 시작되지 않는다.</summary>
    [Fact]
    public void Materialization_WaitsForLastResource()
    {
        BattleSession session = Create(null, Object("windVortex", 14, 18), Object("sunFactory", 32, 18));
        Register(session, "sunCannon");
        GameEntity cannon = Order(session, "sunCannon", 38, 18);
        UnitProduction production = cannon.Production!;
        session.RunTicks(session.TicksPerSecond / 2);
        Assert.True(Assert.Single(production.Deliveries, d => d.IsStormPower).HasArrived);
        Assert.False(Assert.Single(production.Deliveries, d => !d.IsStormPower).HasArrived);
        Assert.Equal(0, cannon.StartTick);
        // 모든 운송이 도착한 바로 그 틱에 반투명 실체화가 시작된다.
        while (production.AwaitingDeliveries) session.RunTicks(1);
        Assert.Equal(session.Tick, cannon.StartTick);
        Assert.False(cannon.IsComplete);
        long completeTick = cannon.CompleteTick;
        session.RunTicks((int)(completeTick - session.Tick - 1));
        Assert.False(cannon.IsComplete);
        session.RunTicks(1);
        Assert.True(cannon.IsComplete);
        IReadOnlyList<SessionEvent> events = session.DrainEvents();
        Assert.Single(events, e => e.Kind == SessionEventKind.UnitMaterializing && e.EntityId == cannon.Id);
        Assert.Single(events, e => e.Kind == SessionEventKind.UnitCompleted && e.EntityId == cannon.Id);
        Assert.DoesNotContain(events, e => e.Kind == SessionEventKind.BuildingCompleted);
    }

    /// <summary>제네레이터는 배치·운송·실체화 동안 공급원으로 사용할 수 없고 완료 때 정확히 한 번 등록된다.</summary>
    [Fact]
    public void Generator_DoesNotSupplyEnergyBeforeCompletion()
    {
        BattleSession session = Create();
        Register(session, "rainBattery");
        GameEntity generator = Order(session, "rainBattery", 22, 18);
        Assert.DoesNotContain(session.Map.Sources, s => s.Id == generator.Id);
        Assert.Equal(0, generator.HitPoints);
        SessionData.RunUntilComplete(session, generator);
        Assert.Single(session.Map.Sources, s => s.Id == generator.Id);
        session.RunTicks(session.TicksPerSecond);
        Assert.Single(session.Map.Sources, s => s.Id == generator.Id);
    }

    /// <summary>예약 위치는 실체가 없어도 중복 배치를 막고 아직 이동·수집·회수가 불가능하다.</summary>
    [Fact]
    public void Reservation_BlocksDuplicatePlacementAndEarlyCommands()
    {
        BattleSession session = Create();
        GameEntity golem = Order(session, "sunwalker", 20, 8);
        Assert.True(session.Map.IsOccupied(golem.Footprint));
        session.EnforceProductionRules = false;
        Assert.Equal(PlacementProblem.Occupied, session.CheckUnit(1, "sunwalker", 20, 8).Site!.Problem);
        session.EnforceProductionRules = true;
        session.Submit(new SalvageCommand(1, golem.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotComplete);
        Assert.Null(session.EntityAt(20, 8));
    }

    /// <summary>생산 기점이 판매돼 덱에서 항목이 없어져도 이미 지불하고 출발한 자원은 도착한다.</summary>
    [Fact]
    public void DestroyedProductionSource_DoesNotCancelInFlightResources()
    {
        BattleSession session = Create();
        Register(session, "rainBattery");
        GameEntity generator = Order(session, "rainBattery", 22, 18);
        session.Submit(new SalvageCommand(1, 2));
        session.RunTicks(1);
        Assert.Null(session.Entity(2));
        Assert.DoesNotContain(session.Player(1).Deck.Entries(), e => e.TypeName.Equals("rainBattery", StringComparison.OrdinalIgnoreCase));
        SessionData.RunUntilComplete(session, generator);
        Assert.True(generator.IsComplete);
    }

    /// <summary>원소 공급원이 판매되어도 이미 출발한 보석은 계속 도착하고 요구값을 다시 검사하지 않는다.</summary>
    [Fact]
    public void DestroyedEnergySource_DoesNotCancelInFlightResources()
    {
        BattleSession session = Create(null, Object("windVortex", 8, 8), Object("sunFactory", 12, 18), Object("rainBattery", 24, 24));
        Register(session, "sunCannon");
        GameEntity cannon = Order(session, "sunCannon", 28, 26);
        Assert.Equal(3, Assert.Single(cannon.Production!.Deliveries, d => !d.IsStormPower).SourceId);
        session.Submit(new SalvageCommand(1, 3));
        session.RunTicks(1);
        Assert.DoesNotContain(session.Map.Sources, s => s.Id == 3);
        SessionData.RunUntilComplete(session, cannon);
        Assert.True(cannon.IsComplete);
    }

    /// <summary>지상 경로가 끊기면 현재 위치에서 공중 직선으로 전환하여 도착한다.</summary>
    [Fact]
    public void CutBridge_SwitchesStormToAirAndStillCompletes()
    {
        BattleSession session = Create((x, _) => x <= 9 || x >= 13);
        // 두 섬 사이의 가로 연결을 저장 다리 칸으로 만든다.
        for (int x = 10; x <= 12; x++) AddBridge(session, x, 8);
        GameEntity golem = Order(session, "sunwalker", 20, 8);
        ProductionDelivery storm = Assert.Single(golem.Production!.Deliveries, d => d.IsStormPower);
        Assert.False(storm.IsAirborne);
        double beforeX = storm.X;
        Cut(session, 11, 8);
        session.RunTicks(1);
        Assert.True(storm.IsAirborne);
        Assert.InRange(storm.X - beforeX, 0, storm.Speed / session.TicksPerSecond);
        SessionData.RunUntilComplete(session, golem);
        Assert.Equal((20, 8), (golem.Footprint.AnchorX, golem.Footprint.AnchorY));
    }

    /// <summary>배치가 합법이지만 지상 경로가 처음부터 없을 때도 공중 경로로 도착한다.</summary>
    [Fact]
    public void MissingGroundPath_StartsAirborneAndCompletes()
    {
        BattleSession session = Create((x, _) => x <= 9 || x >= 13);
        GameEntity golem = Order(session, "sunwalker", 20, 8);
        Assert.True(Assert.Single(golem.Production!.Deliveries, d => d.IsStormPower).IsAirborne);
        SessionData.RunUntilComplete(session, golem);
        Assert.True(golem.IsComplete);
    }

    /// <summary>지상 유닛의 목적지 발판까지 사라지면 도착 시 전액 환불하고 점유·선택을 정리한다.</summary>
    [Fact]
    public void LostDestination_RefundsOnceWithoutCreatingOrFallingUnit()
    {
        BattleSession session = Create((x, _) => x <= 9);
        AddBridge(session, 10, 8);
        AddBridge(session, 11, 8);
        GameEntity golem = Order(session, "sunwalker", 11, 8);
        session.Submit(new SelectEntityCommand(1, golem.Id));
        session.RunTicks(1);
        Cut(session, 11, 8);
        session.RunTicks(4 * session.TicksPerSecond);
        Assert.Null(session.Entity(golem.Id));
        Assert.Equal(StartingPower, session.Player(1).StormPower);
        Assert.False(session.Map.IsOccupied(golem.Footprint));
        Assert.Equal(0, session.Player(1).SelectedEntityId);
        IReadOnlyList<SessionEvent> events = session.DrainEvents();
        Assert.Single(events, e => e.Kind == SessionEventKind.UnitProductionCancelled);
        Assert.DoesNotContain(events, e => e.Kind is SessionEventKind.UnitCompleted or SessionEventKind.UnitFell);
        session.RunTicks(4 * session.TicksPerSecond);
        Assert.Equal(StartingPower, session.Player(1).StormPower);
    }

    /// <summary>없는 생산 기점이 덱에만 남아 있을 경우 비용·점유 없이 배치를 거부한다.</summary>
    [Fact]
    public void StaleDeckSource_RejectsPlacementBeforePayment()
    {
        BattleSession session = Create();
        session.Player(1).Deck.SetTemple(999);
        session.Submit(new PlaceUnitCommand(1, "sunwalker", 20, 8));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotInDeck);
        Assert.Equal(StartingPower, session.Player(1).StormPower);
        Assert.False(session.Map.IsOccupied(new Footprint(20, 8, 1, 1)));
    }

    /// <summary>동일 명령열은 자원 운송 중부터 실체화·완료까지 매 틱 같은 검사합을 만든다.</summary>
    [Fact]
    public void DeliveryAndMaterialization_AreDeterministicAtEveryTick()
    {
        BattleSession first = Create(), second = Create();
        GameEntity a = Order(first, "sunwalker", 20, 8), b = Order(second, "sunwalker", 20, 8);
        // 운송과 실체화를 모두 포함한 3초 동안 매 틱 비교한다.
        for (int tick = 0; tick < 3 * first.TicksPerSecond; tick++)
        {
            Assert.Equal(first.Checksum(), second.Checksum());
            first.RunTicks(1); second.RunTicks(1);
        }
        Assert.True(a.IsComplete && b.IsComplete);
    }

    /// <summary>무관한 지형 버전 변화는 운송 중인 안전한 걸음을 되돌리거나 도착을 지연시키지 않는다.</summary>
    [Fact]
    public void UnrelatedTerrainChange_PreservesDeliveryInProgress()
    {
        BattleSession changed = Create(), reference = Create();
        GameEntity a = Order(changed, "sunwalker", 20, 20), b = Order(reference, "sunwalker", 20, 20);
        ProductionDelivery first = Assert.Single(a.Production!.Deliveries, d => d.IsStormPower);
        ProductionDelivery second = Assert.Single(b.Production!.Deliveries, d => d.IsStormPower);
        changed.RunTicks(1); reference.RunTicks(1);
        changed.Bridges.InvalidateTerrain();
        // 지면 변화 이후에도 같은 속도로 움직이고 같은 틱에 도착한다.
        for (int tick = 0; tick < 4 * changed.TicksPerSecond; tick++)
        {
            changed.RunTicks(1); reference.RunTicks(1);
            Assert.Equal(second.X, first.X, precision: 8);
            Assert.Equal(second.Y, first.Y, precision: 8);
            Assert.Equal(b.IsComplete, a.IsComplete);
        }
        Assert.True(a.IsComplete);
    }

    /// <summary>스톰 파워 운송·실체화를 거쳐도 고정 포대의 설치 방위는 보존된다.</summary>
    [Theory]
    [InlineData("thunderCannon", "thunderFactory", "thunderVortex", "thunderBattery")]
    [InlineData("windArcher", "windFactory", "windVortex", "windBattery")]
    public void Production_PreservesFixedPlacementDirection(string unit, string workshop, string temple, string generator)
    {
        BattleSession session = Create(null, Object(temple, 8, 8), Object(workshop, 12, 18), Object(generator, 30, 25));
        Register(session, unit);
        GameEntity entity = Order(session, unit, 26, 20, rotation: 3);
        Assert.Equal(3, entity.CannonDirection);
        SessionData.RunUntilComplete(session, entity);
        Assert.Equal(3, entity.CannonDirection);
    }

    /// <summary>아직 생성 중인 골렘은 가이저 수집 명령도 받지 않는다.</summary>
    [Fact]
    public void PendingGolem_CannotHarvest()
    {
        BattleSession session = Create(null, Object("windVortex", 8, 8), Object("sunFactory", 12, 18), Object("geyser", 28, 25));
        GameEntity golem = Order(session, "sunwalker", 20, 8);
        session.Submit(new HarvestGeyserCommand(1, 3, golem.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NotComplete);
        Assert.Equal(0, golem.CarriedCrystals);
        Assert.False(session.IsMoving(golem.Id));
    }

    /// <summary>실제 타입의 가로 연결 다리 칸을 추가한다.</summary>
    private static void AddBridge(BattleSession session, int x, int y)
    {
        TypeFrameTable frames = OriginalData.RequireTypes().Find("bridge")!.Definition.Frames;
        session.Bridges.AddStored(frames, frames.Find('K', TypeFrameTable.DefaultVariant, 1), x, y, 1);
    }

    /// <summary>정해진 다리 칸을 약화·제거해 경로와 발판 소멸을 재현한다.</summary>
    private static void Cut(BattleSession session, int x, int y)
    {
        session.Bridges.WeakenAround(x, y); session.Bridges.WeakenAround(x, y);
        Assert.Null(session.Bridges.At(x, y));
    }
}
