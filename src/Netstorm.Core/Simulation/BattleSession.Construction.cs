using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 사제 Construct 메뉴가 건물 줄을 어둡게 하는, 자리와 무관한 이유.
/// 원본 1-1 관찰: 이미 템플이 있으면 Temple 줄이, 지식이 없으면 다른 원소 워크샵 줄이 어둡게 표시된다
/// (docs/videos/auto-war-begins-20261003.md 4.5절). 웹 팬게임은 알타도 플레이어당 1기로 막는다.
/// </summary>
public enum BuildingRestriction
{
    /// <summary>제한 없음</summary>
    None,

    /// <summary>기술 허용 표가 막은 타입</summary>
    TechDenied,

    /// <summary>이미 템플이 있다 (건설 중인 템플 포함)</summary>
    TempleExists,

    /// <summary>이미 알타가 있다 (건설 중인 알타 포함)</summary>
    AltarExists,

    /// <summary>
    /// 워크샵 원소의 지식이 없다 (발전기는 세지 않는다). 원본 1-1 관찰: Rain 발전기 지식이 있어도 Rain Workshop 줄은 어둡다.
    /// 이 조건은 원본처럼 Construct 메뉴에만 적용하고 건설 명령 판정(<see cref="BattleSession.CheckBuilding"/>)은 보지 않는다.
    /// </summary>
    NoKnowledge,
}

/// <summary>
/// 사제의 건물 건설: 설치 클릭에서 비용을 차감하고 공사장을 놓은 뒤, 맡은 사제가 현장까지 걸어가 도착해야 건설 시간이 시작된다.
/// 도착 전에 사제가 다른 명령을 받거나 기절·포획·사망하면 공사장은 비용 전액 환불과 함께 사라진다.
/// 근거: 2026-10-03 원본 자동 분석(docs/videos/auto-war-begins-20261003.md 4.5절)과 웹 팬게임의 같은 흐름
/// (placeBuilding → 사제 이동 → 도착 시 건설 시작, 사제의 새 명령은 도착 전 공사장을 환불 취소).
/// </summary>
public sealed partial class BattleSession
{
    /// <summary>한 번 계산한 사제 도달 판정의 열쇠 (같은 자리에서 미리보기가 매 프레임 길 탐색을 되풀이하지 않게 한다)</summary>
    private readonly record struct RouteProbe(int BuilderId, int FromX, int FromY, int Left, int Top, int Right, int Bottom, int BridgeVersion);

    /// <summary>가장 최근 사제 도달 판정 (규칙·검사합과 무관한 캐시)</summary>
    private (RouteProbe Key, bool Reachable)? _routeProbe;

    /// <summary>플레이어가 그 분류의 오브젝트를 이미 가지고 있는지 (건설 중·사제 대기 중인 것 포함)</summary>
    /// <param name="player">플레이어</param>
    /// <param name="kind">분류</param>
    public bool HasBuilding(int player, ObjectKind kind) => _entities.Values.Any(e => e.Owner == player && e.Kind == kind);

    /// <summary>
    /// 건물 타입을 지금 지을 수 있는 종류인지 (자리와 무관한 조건): 템플·알타는 플레이어당 1기, 기술 허용 표, 워크샵은 그 원소의 지식.
    /// 우클릭 Construct 메뉴가 줄을 어둡게 할지 정하는 데 쓴다. 앞의 세 조건은 실제 판정(<see cref="CheckBuilding"/>)과 같고,
    /// 지식 조건은 원본처럼 메뉴에만 있다.
    /// </summary>
    /// <param name="player">플레이어</param>
    /// <param name="typeName">건물 타입 이름</param>
    public BuildingRestriction GetBuildingRestriction(int player, string typeName)
    {
        if (!_players.TryGetValue(player, out PlayerState? state) || _types.Find(typeName) is not { } type) return BuildingRestriction.None;
        ObjectKind kind = ObjectKinds.Of(type);
        if (kind == ObjectKind.Temple && HasBuilding(player, ObjectKind.Temple)) return BuildingRestriction.TempleExists;
        if (kind == ObjectKind.Altar && HasBuilding(player, ObjectKind.Altar)) return BuildingRestriction.AltarExists;
        if (EnforceProductionRules && !state.Tech.IsAllowed(type.Name)) return BuildingRestriction.TechDenied;
        if (EnforceProductionRules && kind == ObjectKind.Workshop && Elements.FromTheme(type.Definition.GetString("theme")) is { } element
            && !KnowsElement(state, element))
        {
            return BuildingRestriction.NoKnowledge;
        }
        return BuildingRestriction.None;
    }

    /// <summary>
    /// 플레이어가 그 원소의 지식(생산 유닛, 발전기 제외)을 하나라도 알고 있는지. 원본 Construct 메뉴가 지식 없는 원소의 워크샵 줄을
    /// 어둡게 하는 조건이다 (웹 팬게임 fo: 같은 원소 · 발전기 아님).
    /// </summary>
    /// <param name="state">플레이어 상태</param>
    /// <param name="element">워크샵 원소</param>
    private bool KnowsElement(PlayerState state, Element element) =>
        state.Deck.Knowledge.Any(name => _types.Find(name) is { } type
            && ProducibleUnit.FromType(type) is { IsGenerator: false } unit && unit.Element == element);

    /// <summary>
    /// 건설을 맡길 사제를 찾는다: 번호가 있으면 그 사제, 0 이면 첫 자유 사제. 내 사제이고 자유로우며 기절하지 않아야 한다.
    /// </summary>
    /// <param name="player">플레이어</param>
    /// <param name="builderId">사제 번호 (0 이면 자동)</param>
    private GameEntity? ResolveBuilder(int player, int builderId)
    {
        if (builderId == 0) return OwnFreePriest(player);
        return Entity(builderId) is { Kind: ObjectKind.Priest, IsStunned: false, IsSuspended: false, Captivity: PriestCaptivity.Free } priest
            && priest.Owner == player ? priest : null;
    }

    /// <summary>사제가 건물 자리 둘레까지 걸어갈 수 있는지 (섬과 내 다리를 따라). 같은 자리·같은 다리 상태의 결과는 기억한다.</summary>
    /// <param name="builder">사제</param>
    /// <param name="site">건물 발자국</param>
    private bool CanBuilderReach(GameEntity builder, Footprint site)
    {
        var key = new RouteProbe(builder.Id, builder.Footprint.AnchorX, builder.Footprint.AnchorY,
            site.Left, site.Top, site.AnchorX, site.AnchorY, Bridges.Version);
        if (_routeProbe is { } cached && cached.Key == key) return cached.Reachable;
        bool reachable = FindMovePath(builder, site) != null;
        _routeProbe = (key, reachable);
        return reachable;
    }

    /// <summary>
    /// 건물 건설: 판정 → Storm Power 차감 → 자리 점유(공사장) → 사제가 현장으로 이동. 사제가 이미 현장 옆이면 곧바로 건설을 시작하고,
    /// 아니면 사제가 도착할 때 시작한다. 생산 규칙이 꺼진 개발용 세션은 사제 없이 곧바로 짓는다.
    /// 같은 사제가 앞서 맡은, 아직 도착하지 않은 공사장은 이 새 지시로 취소(환불)된다.
    /// </summary>
    private CommandResult ExecuteConstruct(ConstructBuildingCommand command)
    {
        GameEntity? builder = null;
        if (EnforceProductionRules)
        {
            builder = ResolveBuilder(command.Player, command.BuilderId);
            if (builder == null) return new CommandResult(CommandFailure.NoPriest);
        }
        SessionPlacementCheck check = CheckBuilding(command.Player, command.TypeName, command.X, command.Y, builder?.Id ?? 0);
        if (!check.Allowed)
        {
            return Reject(check);
        }
        PlayerState player = _players[command.Player];
        TypeInfo type = _types.Find(command.TypeName)!;
        PlacementCheck site = check.Site!;
        ObjectKind kind = ObjectKinds.Of(type);
        // 길은 사제가 지금 하던 이동을 이어받아 정한다 (아래에서 이전 작업을 지우기 전에 계산한다)
        double carried = 0;
        List<(int X, int Y)>? path = builder == null ? null : PlanPath(builder, site.Footprint, exact: false, out carried);
        if (builder != null && path == null) return new CommandResult(CommandFailure.NoRoute);
        // 사제의 새 지시는 도착 전 공사장을 취소한다 (웹 팬게임의 Io). 판정은 취소 전 상태로 이미 끝났다.
        if (builder != null) CancelPendingConstructionOf(builder.Id);
        int id = Map.NextId();
        Map.AddOccupant(site.Footprint);
        player.StormPower -= site.Cost;
        var entity = new GameEntity(id, type, kind, command.Player, site.Footprint, Map.TerritoryAt(command.X, command.Y), null)
        {
            IsComplete = false,
            BuilderId = builder?.Id ?? 0,
        };
        _entities.Add(id, entity);
        Emit(SessionEventKind.ConstructionOrdered, command.Player, id, $"{entity.DisplayName} 건설 지시 (−{site.Cost})");
        if (builder == null)
        {
            BeginConstruction(entity);
            return CommandResult.Ok();
        }
        // 사제는 하던 일(수확·이동)을 멈추고 건설에 나선다
        _harvestTasks.Remove(builder.Id);
        _moveTasks.Remove(builder.Id);
        if (path!.Count <= 1)
        {
            // 이미 현장 옆에 서 있다
            BeginConstruction(entity);
            return CommandResult.Ok();
        }
        entity.AwaitingBuilder = true;
        _moveTasks[builder.Id] = new UnitMoveTask(builder.Id, UnitMovePurpose.ConstructBuilding, id, 0, path, Bridges.Version) { Progress = carried };
        return CommandResult.Ok();
    }

    /// <summary>건설 시간을 시작한다: 사제가 도착했거나 처음부터 현장 옆이었다.</summary>
    /// <param name="entity">공사장</param>
    private void BeginConstruction(GameEntity entity)
    {
        entity.AwaitingBuilder = false;
        entity.StartTick = Tick;
        entity.CompleteTick = Tick + TicksFor(ConstructionTimes.BuildingSeconds);
        Emit(SessionEventKind.BuildingStarted, entity.Owner, entity.Id, $"{entity.DisplayName} 건설 시작");
    }

    /// <summary>
    /// 틱마다 사제를 기다리는 공사장을 점검한다: 맡은 사제가 더 이상 그 공사장으로 걸어가는 중이 아니면(다른 이동·수확·희생 명령,
    /// 기절·포획·사망) 취소한다. 도착해서 시작된 건설은 대상이 아니다.
    /// </summary>
    private void UpdatePendingConstructions()
    {
        // 번호순으로 점검한다
        foreach (GameEntity site in _entities.Values.Where(e => e.AwaitingBuilder).ToArray())
        {
            bool onTheWay = Entity(site.BuilderId) is { IsStunned: false, Captivity: PriestCaptivity.Free } builder
                && _moveTasks.TryGetValue(builder.Id, out UnitMoveTask? task)
                && task.Purpose == UnitMovePurpose.ConstructBuilding && task.TargetId == site.Id;
            if (!onTheWay) CancelPendingConstruction(site);
        }
    }

    /// <summary>한 사제가 맡은, 아직 도착하지 않은 공사장이 있으면 취소한다.</summary>
    /// <param name="builderId">사제 번호</param>
    private void CancelPendingConstructionOf(int builderId)
    {
        // 사제 한 명이 맡은 도착 전 공사장은 하나뿐이지만 번호순으로 모두 처리해 규칙을 단순하게 둔다
        foreach (GameEntity site in _entities.Values.Where(e => e.AwaitingBuilder && e.BuilderId == builderId).ToArray())
        {
            CancelPendingConstruction(site);
        }
    }

    /// <summary>
    /// 도착 전 공사장을 취소한다: 비용 전액을 돌려주고 자리를 비우며 오브젝트를 없앤다. 파괴·회수가 아니므로 주변 다리는 약해지지 않는다.
    /// </summary>
    /// <param name="site">공사장</param>
    private void CancelPendingConstruction(GameEntity site)
    {
        if (_players.TryGetValue(site.Owner, out PlayerState? owner)) owner.StormPower += site.Cost;
        Map.RemoveOccupant(site.Footprint);
        _entities.Remove(site.Id);
        // 이 공사장을 가리키던 선택과 이동 작업을 정리한다
        foreach (PlayerState viewer in _players.Values.Where(p => p.SelectedEntityId == site.Id)) ClearSelection(viewer);
        foreach (UnitMoveTask task in _moveTasks.Values.Where(t => t.TargetId == site.Id && t.Purpose == UnitMovePurpose.ConstructBuilding).ToArray())
        {
            _moveTasks.Remove(task.MoverId);
        }
        Emit(SessionEventKind.ConstructionCancelled, site.Owner, site.Id, $"{site.DisplayName} 건설 취소 (+{site.Cost})");
    }
}
