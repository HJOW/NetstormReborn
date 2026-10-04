using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 사제의 건물 건설 규칙: 설치 클릭에서 비용을 차감하고 공사장을 놓은 뒤 사제가 현장까지 걸어가 도착해야 건설 시간이 시작된다.
/// 템플·알타는 플레이어당 1기이며(건설 중 포함), 도착 전에 사제가 다른 명령을 받으면 공사장은 비용 전액 환불과 함께 사라진다.
/// 근거는 2026-10-03 원본 자동 분석(docs/videos/auto-war-begins-20261003.md 4.5절)과 웹 팬게임의 같은 흐름이다.
/// 사제가 (2,2)에 있는 합성 섬에서 거리와 도달 여부를 정확히 통제해 검사한다.
/// </summary>
public sealed class ConstructionTests
{
    /// <summary>기본 섬 한 변의 칸 수 (x·y 0~59가 모두 지면)</summary>
    private const int IslandSize = 60;

    /// <summary>사제가 서 있는 칸 (x, y 모두)</summary>
    private const int PriestStart = 2;

    /// <summary>사제에게서 멀리 떨어진 템플 기준점 x (발자국 8×8 이라 x 43~50, y 33~40을 차지한다)</summary>
    private const int FarX = 50;

    /// <summary>사제에게서 멀리 떨어진 템플 기준점 y</summary>
    private const int FarY = 40;

    /// <summary>사제가 서 있는 칸 바로 옆에 놓이는 템플 기준점 x (발자국 x 3~10, y 0~7 — 사제 (2,2)가 왼쪽 변 바로 바깥이다)</summary>
    private const int NearX = 10;

    /// <summary>사제 바로 옆 템플 기준점 y</summary>
    private const int NearY = 7;

    /// <summary>건설 시간(10초)의 틱 수 (초당 24틱)</summary>
    private static int BuildTicks(BattleSession session) => (int)(ConstructionTimes.BuildingSeconds * session.TicksPerSecond);

    /// <summary>
    /// 사제 한 명(번호 1, 플레이어 1)이 (2,2)에 서 있는 합성 세션을 만든다. 모든 기술을 허용하고 Storm Power 20,000으로 시작한다.
    /// </summary>
    /// <param name="island">지면 판정 (없으면 한 변 60칸 정사각형 섬)</param>
    private static BattleSession Create(Func<int, int, bool>? island = null)
    {
        island ??= (x, y) => x is >= 0 and < IslandSize && y is >= 0 and < IslandSize;
        BattleSession? session = null;
        var grid = new BridgeGrid(island, (x, y) => session?.Entities.Any(e => e.OccupiesGround && e.Footprint.Contains(x, y)) ?? false);
        TypeCatalog types = OriginalData.RequireTypes();
        var priest = new FortMapObject(PriestStart, PriestStart, 0, new FortObject(0, 0, types.Find("priest")!, null, null, null, null, null, 1, []));
        session = new BattleSession(new BattleMap([priest], (x, y) => island(x, y) ? 0 : null), grid, types, startStormPower: 20000);
        session.CombatEnabled = false;
        session.Player(1).Tech.SetAll(true);
        return session;
    }

    /// <summary>세션의 내 사제</summary>
    private static GameEntity PriestOf(BattleSession session) =>
        session.Entities.Single(e => e.Kind == ObjectKind.Priest && e.Owner == 1);

    /// <summary>발자국 a 의 기준점이 b 를 한 칸 넓힌 사각형 안에 있는지 (건설 현장 바로 옆에 서 있음)</summary>
    private static bool IsBeside(Footprint a, Footprint b) =>
        a.AnchorX >= b.Left - 1 && a.AnchorX <= b.AnchorX + 1 && a.AnchorY >= b.Top - 1 && a.AnchorY <= b.AnchorY + 1;

    /// <summary>건물 건설 명령을 넣고 한 틱 진행한 뒤 새로 생긴 공사장을 돌려준다 (이미 있던 오브젝트는 제외)</summary>
    private static GameEntity Order(BattleSession session, string type, int x, int y, int builderId = 0)
    {
        HashSet<int> before = [.. session.Entities.Select(e => e.Id)];
        session.Submit(new ConstructBuildingCommand(1, type, x, y, builderId));
        session.RunTicks(1);
        GameEntity[] created = [.. session.Entities.Where(e => !before.Contains(e.Id))];
        if (created.Length != 1)
        {
            // 거부됐다면 이유를 알려 준다 (이벤트를 비우므로 호출한 시험은 이 뒤에 이벤트를 다시 확인하지 않는다)
            string reasons = string.Join(" / ", session.DrainEvents().Where(e => e.Kind == SessionEventKind.CommandRejected).Select(e => e.Text));
            throw new InvalidOperationException($"{type} 건설 명령 뒤 새 오브젝트가 {created.Length}개입니다 (거부: {reasons})");
        }
        return created[0];
    }

    /// <summary>기준점이 지면이어도 7×8 발자국 위쪽이 섬 밖이면 건설을 거부한다. 두 건물은 정확히 맞닿아도 허용한다.</summary>
    [Fact]
    public void Workshop_RequiresAllEightRowsAndAllowsTouchingNeighbors()
    {
        BattleSession session = Create();
        Assert.Equal(PlacementProblem.BuildingNeedsIsland, session.CheckBuilding(1, "sunFactory", 20, 6).Site!.Problem);
        // 원본처럼 사제 (2,2)가 발자국 안에 있어도 설치를 막지 않는다.
        Assert.True(session.CheckBuilding(1, "sunFactory", 7, 7).Allowed);
        GameEntity workshop = Order(session, "sunFactory", 20, 7);
        Assert.Equal((7, 8), (workshop.Footprint.Width, workshop.Footprint.Height));
        Assert.False(session.CheckBuilding(1, "sunFactory", 26, 7).Allowed);
        Assert.True(session.CheckBuilding(1, "sunFactory", 27, 7).Allowed);
        Assert.False(session.CheckBuilding(1, "sunFactory", 20, 14).Allowed);
        Assert.True(session.CheckBuilding(1, "sunFactory", 20, 15).Allowed);
    }

    /// <summary>워크샵은 템플과 달리 건물 수로 제한하지 않는다. 원본 TEST02의 14채 설치를 합성 섬에서도 확인한다.</summary>
    [Fact]
    public void Workshop_FourteenSitesHaveNoCountRestriction()
    {
        BattleSession session = Create();
        // 각 공사장에 사제를 도착시켜 이전 건설을 취소하지 않고 차례로 건설을 시작한다.
        for (int index = 0; index < 14; index++)
        {
            int x = 13 + index % 6 * 7;
            int y = 15 + index / 6 * 8;
            GameEntity site = Order(session, "sunFactory", x, y);
            // 이동 거리에 충분한 시간을 주어 사제 도착·건설 완료를 모두 진행한다.
            session.RunTicks(60 * session.TicksPerSecond);
            Assert.True(site.IsComplete);
        }
        Assert.Equal(14, session.Entities.Count(e => e.Kind == ObjectKind.Workshop));
        Assert.Equal(20000 - 14 * 800, session.Player(1).StormPower);
    }

    /// <summary>
    /// 멀리 있는 템플: 비용은 설치 때 한 번에 나가고 공사장이 생기지만, 사제가 현장 옆에 도착하기 전에는 건설 시간이 시작되지 않는다.
    /// 도착한 틱에 건설이 시작되어 정확히 10초 뒤 완공된다.
    /// </summary>
    [Fact]
    public void FarSite_PaysAtOnceButStartsOnlyWhenPriestArrives()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        Assert.True(session.CheckBuilding(1, "windVortex", FarX, FarY, priest.Id).Allowed);
        GameEntity temple = Order(session, "windVortex", FarX, FarY);

        // 설치 즉시: 비용 차감 + 공사장. 건설은 시작되지 않았다
        Assert.Equal(20000 - 5000, session.Player(1).StormPower);
        Assert.True(temple.AwaitingBuilder);
        Assert.False(temple.IsComplete);
        Assert.Equal(priest.Id, temple.BuilderId);
        Assert.Equal(0, temple.StartTick);
        Assert.Equal(0, temple.CompleteTick);
        Assert.Equal(0, session.ConstructionProgress(temple));
        IReadOnlyList<SessionEvent> ordered = session.DrainEvents();
        Assert.Contains(ordered, e => e.Kind == SessionEventKind.ConstructionOrdered && e.EntityId == temple.Id);
        Assert.DoesNotContain(ordered, e => e.Kind == SessionEventKind.BuildingStarted);

        // 사제가 걸어가는 동안 (사제 속도 1.8칸/초, 현장까지 약 53칸 = 약 30초) 건설은 시작되지 않고 사제는 현장에서 멀다
        session.RunTicks(10 * session.TicksPerSecond);
        Assert.True(temple.AwaitingBuilder);
        Assert.Equal(0, temple.CompleteTick);
        Assert.NotEqual((PriestStart, PriestStart), (priest.Footprint.AnchorX, priest.Footprint.AnchorY));
        Assert.False(IsBeside(priest.Footprint, temple.Footprint));
        Assert.True(session.IsMoving(priest.Id));
        Assert.Equal(UnitMovePurpose.ConstructBuilding, session.MovePurposeOf(priest.Id));

        // 도착한 틱에 건설이 시작된다: 사제가 현장 옆에 서 있고, 10초 뒤 완공
        SessionData.RunUntilStarted(session, temple);
        Assert.True(IsBeside(priest.Footprint, temple.Footprint));
        Assert.True(session.Tick > 20 * session.TicksPerSecond, "멀리 떨어진 자리라 걷는 데 20초 넘게 걸려야 한다");
        Assert.Equal(session.Tick, temple.StartTick);
        Assert.Equal(BuildTicks(session), temple.CompleteTick - temple.StartTick);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.BuildingStarted && e.EntityId == temple.Id);
        session.RunTicks(BuildTicks(session) - 1);
        Assert.False(temple.IsComplete);
        Assert.False(session.Player(1).HasTemple);
        session.RunTicks(1);
        Assert.True(temple.IsComplete);
        Assert.True(session.Player(1).HasTemple);
    }

    /// <summary>이미 사제가 현장 바로 옆에 서 있으면 걷지 않고 설치한 틱에 곧바로 건설이 시작된다.</summary>
    [Fact]
    public void SiteBesidePriest_StartsImmediately()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        GameEntity temple = Order(session, "windVortex", NearX, NearY);
        Assert.True(IsBeside(priest.Footprint, temple.Footprint));
        Assert.False(temple.AwaitingBuilder);
        Assert.Equal(session.Tick, temple.StartTick);
        Assert.Equal(BuildTicks(session), temple.CompleteTick - temple.StartTick);
        Assert.Null(session.MovePurposeOf(priest.Id));
        IReadOnlyList<SessionEvent> events = session.DrainEvents();
        Assert.Contains(events, e => e.Kind == SessionEventKind.ConstructionOrdered);
        Assert.Contains(events, e => e.Kind == SessionEventKind.BuildingStarted);
    }

    /// <summary>
    /// 도착 전에 사제가 다른 이동 명령을 받으면 공사장은 사라지고 비용은 전액 돌아온다. 자리는 다시 쓸 수 있고, 다리는 약해지지 않는다.
    /// </summary>
    [Fact]
    public void PriestOrderedElsewhereBeforeArrival_CancelsAndRefunds()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        GameEntity temple = Order(session, "windVortex", FarX, FarY);
        Footprint site = temple.Footprint;
        Assert.Equal(15000, session.Player(1).StormPower);
        session.RunTicks(2 * session.TicksPerSecond);
        session.DrainEvents();

        // 다른 곳으로 가라는 새 명령
        session.Submit(new MoveEntityCommand(1, priest.Id, 20, 3));
        session.RunTicks(1);
        Assert.Null(session.Entity(temple.Id));
        Assert.Equal(20000, session.Player(1).StormPower);
        Assert.False(session.Map.IsOccupied(site));
        Assert.DoesNotContain(session.Entities, e => e.Kind == ObjectKind.Temple);
        IReadOnlyList<SessionEvent> events = session.DrainEvents();
        Assert.Contains(events, e => e.Kind == SessionEventKind.ConstructionCancelled && e.EntityId == temple.Id);
        Assert.DoesNotContain(events, e => e.Kind is SessionEventKind.BridgeCracked or SessionEventKind.BridgeCollapsed);
        // 취소했으므로 템플을 다시 지을 수 있다
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "windVortex"));
        Assert.True(session.CheckBuilding(1, "windVortex", FarX, FarY, priest.Id).Allowed);
    }

    /// <summary>정지·수확 같은 사제의 어떤 새 명령도 도착 전 공사장을 취소한다 (웹 팬게임: 모든 사제 명령이 도착 전 공사장을 환불 취소).</summary>
    [Fact]
    public void StopCommandBeforeArrival_CancelsAndRefunds()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        GameEntity temple = Order(session, "windVortex", FarX, FarY);
        session.RunTicks(session.TicksPerSecond);
        session.Submit(new StopEntityCommand(1, priest.Id));
        session.RunTicks(1);
        Assert.Null(session.Entity(temple.Id));
        Assert.Equal(20000, session.Player(1).StormPower);
    }

    /// <summary>건설이 시작된 뒤에는 사제가 떠나도 건설은 이어져 완공된다 (취소는 도착 전 공사장만 해당).</summary>
    [Fact]
    public void ConstructionInProgress_ContinuesWhenPriestLeaves()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        GameEntity temple = Order(session, "windVortex", NearX, NearY);
        Assert.False(temple.AwaitingBuilder);
        session.Submit(new MoveEntityCommand(1, priest.Id, 30, 30));
        session.RunTicks(1);
        Assert.NotNull(session.Entity(temple.Id));
        Assert.Equal(15000, session.Player(1).StormPower);
        SessionData.RunUntilComplete(session, temple);
        Assert.True(temple.IsComplete);
    }

    /// <summary>
    /// 같은 사제의 새 건설 지시는 앞서 맡은 도착 전 공사장을 취소(환불)하고 새 공사장으로 간다. (템플·알타는 1기 제한 때문에 워크샵으로 확인한다.
    /// 워크샵은 에너지 공급원이 필요하므로 먼저 템플을 지어 둔다.)
    /// </summary>
    [Fact]
    public void NewOrderForSamePriest_ReplacesPendingSite()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        SessionData.RunUntilComplete(session, Order(session, "windVortex", NearX, NearY));
        int money = session.Player(1).StormPower;
        Assert.Equal(15000, money);

        GameEntity first = Order(session, "sunFactory", 20, 12);
        Assert.True(first.AwaitingBuilder);
        Assert.Equal(money - 800, session.Player(1).StormPower);
        GameEntity second = Order(session, "sunFactory", 12, 16);
        // 첫 공사장은 사라지고 환불된 뒤 두 번째 비용만 나가 있다
        Assert.Null(session.Entity(first.Id));
        Assert.True(second.AwaitingBuilder);
        Assert.Equal(money - 800, session.Player(1).StormPower);
        Assert.Equal(UnitMovePurpose.ConstructBuilding, session.MovePurposeOf(priest.Id));
        Assert.Single(session.Entities, e => e.Kind == ObjectKind.Workshop);
        // 사제는 두 번째 공사장으로 가서 건설을 시작한다
        SessionData.RunUntilStarted(session, second);
        Assert.True(IsBeside(priest.Footprint, second.Footprint));
    }

    /// <summary>
    /// 건설 중이거나 사제를 기다리는 템플도 "이미 템플이 있음"이다: Construct 메뉴가 Temple 줄을 어둡게 하는 조건과 판정이 같다.
    /// </summary>
    [Fact]
    public void PendingTemple_BlocksSecondTempleAndMenuRestriction()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "windVortex"));
        Assert.False(session.HasBuilding(1, ObjectKind.Temple));
        GameEntity temple = Order(session, "windVortex", FarX, FarY);
        Assert.True(temple.AwaitingBuilder);

        Assert.True(session.HasBuilding(1, ObjectKind.Temple));
        Assert.Equal(BuildingRestriction.TempleExists, session.GetBuildingRestriction(1, "windVortex"));
        Assert.Equal(BuildingRestriction.TempleExists, session.GetBuildingRestriction(1, "rainVortex"));
        SessionPlacementCheck second = session.CheckBuilding(1, "rainVortex", 20, 20, priest.Id);
        Assert.Equal(CommandFailure.Placement, second.Failure);
        Assert.Equal(PlacementProblem.TempleAlreadyExists, second.Site!.Problem);
        // 명령으로도 거부되고 비용은 나가지 않는다
        session.DrainEvents();
        session.Submit(new ConstructBuildingCommand(1, "rainVortex", 20, 20, priest.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected && e.Failure == CommandFailure.Placement);
        Assert.Equal(15000, session.Player(1).StormPower);
        Assert.Single(session.Entities, e => e.Kind == ObjectKind.Temple);
        // 템플이 아닌 건물은 템플이 있어도 제한이 없다
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "outpost"));
    }

    /// <summary>알타도 플레이어당 1기다 (웹 팬게임 규칙). 도착 전 공사장도 센다.</summary>
    [Fact]
    public void Altar_OnlyOnePerPlayer()
    {
        BattleSession session = Create();
        GameEntity priest = PriestOf(session);
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "altar"));
        GameEntity altar = Order(session, "altar", FarX, FarY);
        Assert.Equal(ObjectKind.Altar, altar.Kind);
        Assert.Equal(BuildingRestriction.AltarExists, session.GetBuildingRestriction(1, "altar"));
        SessionPlacementCheck second = session.CheckBuilding(1, "altar", 30, 20, priest.Id);
        Assert.Equal(CommandFailure.Placement, second.Failure);
        Assert.Equal(PlacementProblem.AltarAlreadyExists, second.Site!.Problem);
    }

    /// <summary>기술 허용 표가 막은 건물은 메뉴에서 어둡게(제한) 되고 판정도 막는다.</summary>
    [Fact]
    public void TechDenied_IsReportedAsRestriction()
    {
        BattleSession session = Create();
        session.Player(1).Tech.SetAll(false);
        session.Player(1).Tech.Set("sunFactory", false);
        session.Player(1).Tech.Set("windVortex", true);
        Assert.Equal(BuildingRestriction.TechDenied, session.GetBuildingRestriction(1, "sunFactory"));
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "windVortex"));
    }

    /// <summary>
    /// 워크샵 줄은 그 원소의 지식(발전기 제외)이 있어야 쓸 수 있다: 원본 1-1 관찰에서 Sun Workshop 만 활성이고 Rain·Wind·Thunder 는 어두웠다
    /// (1-1 은 Rain 발전기 지식을 가졌지만 Rain Workshop 이 어두웠다). 웹 팬게임 fo 와 같다.
    /// </summary>
    [Fact]
    public void Workshop_NeedsNonGeneratorKnowledgeOfItsElement()
    {
        BattleSession session = Create();
        PlayerState player = session.Player(1);
        Assert.Equal(BuildingRestriction.NoKnowledge, session.GetBuildingRestriction(1, "sunFactory"));
        // 발전기 지식은 세지 않는다
        player.Deck.LearnKnowledge("rainBattery");
        Assert.Equal(BuildingRestriction.NoKnowledge, session.GetBuildingRestriction(1, "rainFactory"));
        Assert.Equal(BuildingRestriction.NoKnowledge, session.GetBuildingRestriction(1, "sunFactory"));
        // 같은 원소의 일반 유닛 지식이 생기면 그 원소의 워크샵만 쓸 수 있다
        player.Deck.LearnKnowledge("sunCannon");
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "sunFactory"));
        Assert.Equal(BuildingRestriction.NoKnowledge, session.GetBuildingRestriction(1, "rainFactory"));
        player.Deck.LearnKnowledge("rainCannon");
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "rainFactory"));
        Assert.Equal(BuildingRestriction.NoKnowledge, session.GetBuildingRestriction(1, "windFactory"));
        // 생산 규칙을 끈 개발용 세션은 지식을 따지지 않는다
        session.EnforceProductionRules = false;
        Assert.Equal(BuildingRestriction.None, session.GetBuildingRestriction(1, "windFactory"));
    }

    /// <summary>사제가 걸어갈 길이 없는 섬의 자리는 미리보기 판정과 명령 모두 막고, 비용은 나가지 않는다 (웹 팬게임 builderCannotReach).</summary>
    [Fact]
    public void UnreachableIsland_IsRejectedWithoutCharging()
    {
        // 사제가 있는 왼쪽 섬과 길이 없는 오른쪽 섬
        BattleSession session = Create((x, y) => y is >= 0 and < 30 && (x is >= 0 and < 30 || x is >= 40 and < 70));
        GameEntity priest = PriestOf(session);
        // 사제를 따지지 않는 판정은 자리 자체가 가능하다고 본다
        Assert.True(session.CheckBuilding(1, "windVortex", 60, 20).Allowed);
        SessionPlacementCheck check = session.CheckBuilding(1, "windVortex", 60, 20, priest.Id);
        Assert.Equal(CommandFailure.NoRoute, check.Failure);
        // 같은 섬의 자리는 갈 수 있다
        Assert.True(session.CheckBuilding(1, "windVortex", 25, 25, priest.Id).Allowed);

        session.Submit(new ConstructBuildingCommand(1, "windVortex", 60, 20, priest.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected && e.Failure == CommandFailure.NoRoute);
        Assert.Equal(20000, session.Player(1).StormPower);
        Assert.DoesNotContain(session.Entities, e => e.Kind == ObjectKind.Temple);
    }

    /// <summary>맡길 사제가 없거나(없는 번호) 내 사제가 아니면 거부되고 비용은 나가지 않는다.</summary>
    [Fact]
    public void WithoutBuilder_IsRejected()
    {
        BattleSession session = Create();
        session.Submit(new ConstructBuildingCommand(1, "windVortex", FarX, FarY, 999));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected && e.Failure == CommandFailure.NoPriest);
        Assert.Equal(20000, session.Player(1).StormPower);
        // 사제가 아닌 오브젝트(공사장)를 맡길 수도 없다
        GameEntity site = Order(session, "altar", FarX, FarY);
        session.Submit(new ConstructBuildingCommand(1, "windVortex", 30, 20, site.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Failure == CommandFailure.NoPriest);
    }

    /// <summary>생산 규칙을 끈 개발용 세션은 사제 없이 곧바로 짓는다 (맵 뷰어의 배치 시험 모드).</summary>
    [Fact]
    public void WithoutProductionRules_BuildsWithoutPriest()
    {
        BattleSession session = Create();
        session.EnforceProductionRules = false;
        GameEntity temple = Order(session, "windVortex", FarX, FarY);
        Assert.False(temple.AwaitingBuilder);
        Assert.Equal(0, temple.BuilderId);
        Assert.Equal(BuildTicks(session), temple.CompleteTick - temple.StartTick);
        SessionData.RunUntilComplete(session, temple);
    }

    /// <summary>사제를 기다리는 공사장과 건설 사제는 검사합에 들어간다: 같은 명령은 같은 값, 취소하면 달라진다 (락스텝·리플레이).</summary>
    [Fact]
    public void Checksum_CoversPendingConstruction()
    {
        BattleSession a = Create();
        BattleSession b = Create();
        BattleSession none = Create();
        Order(a, "windVortex", FarX, FarY);
        Order(b, "windVortex", FarX, FarY);
        // 명령 없이 같은 틱만큼만 진행한 세션과도 비교한다 (틱 수 차이가 값을 가르지 않게 같은 틱을 맞춘다)
        none.RunTicks(1);
        // 세 세션을 같은 시간만큼 더 진행한다 (사제가 현장으로 걷는 도중)
        foreach (BattleSession session in new[] { a, b, none })
        {
            session.RunTicks(3 * session.TicksPerSecond);
        }
        Assert.Equal(a.Checksum(), b.Checksum());
        Assert.NotEqual(a.Checksum(), none.Checksum());
        // 한쪽만 사제를 불러들이면 취소되어 갈라진다
        a.Submit(new StopEntityCommand(1, PriestOf(a).Id));
        a.RunTicks(1);
        b.RunTicks(1);
        Assert.NotEqual(a.Checksum(), b.Checksum());
    }
}
