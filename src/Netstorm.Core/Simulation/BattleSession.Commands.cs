using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>게임 세션의 판정(조회)과 명령 실행.</summary>
public sealed partial class BattleSession
{
    /// <summary>회수할 수 있는 오브젝트 분류 (건물과 생산 유닛. 사제·가이저·결정·지형·다리는 제외)</summary>
    private static readonly IReadOnlySet<ObjectKind> SalvageableKinds = new HashSet<ObjectKind>
    {
        ObjectKind.Temple, ObjectKind.Workshop, ObjectKind.Outpost, ObjectKind.Altar,
        ObjectKind.Generator, ObjectKind.Emplacement, ObjectKind.Transport,
    };

    /// <summary>이동체 분류 비트 (타입 플래그2 walker 0x10000 · balloon 0x20000 · flyer 0x100000, 원본 FUN_0044b9e0 의 0x130000)</summary>
    private const uint MobileGenusFlags = 0x130000;

    // ───────────────────────── 판정 (상태를 바꾸지 않는다) ─────────────────────────

    /// <summary>
    /// 빈 섬이 플레이어의 섬과 다리로 연결되어 있는지 (docs/gameplay/island-ownership.md 규칙 4).
    /// 다리 격자가 바뀔 때만 연결망을 다시 계산한다. 근사 규칙은 <see cref="BridgeReach"/> 참고.
    /// </summary>
    /// <param name="player">플레이어</param>
    /// <param name="x">확인할 칸 x</param>
    /// <param name="y">확인할 칸 y</param>
    public bool IsConnectedToHome(int player, int x, int y)
    {
        if (Map.TerritoryAt(x, y) is not int territory)
        {
            return false;
        }
        if (_reachVersion != Bridges.Version)
        {
            _reaches = BridgeReach.Compute(Bridges, Map.TerritoryAt);
            _reachVersion = Bridges.Version;
        }
        return BridgeReach.IsConnectedToHome(_reaches, player, territory, Map.Ownership.OwnerOf);
    }

    /// <summary>발자국 둘레에 플레이어의 다리 칸이 있는지 (다리 끝 위치의 근사 판정)</summary>
    private bool IsAtOwnBridgeEnd(Footprint footprint, int player) =>
        footprint.BorderCells().Any(c => Bridges.At(c.X, c.Y) is { } cell && cell.Owner == player);

    /// <summary>
    /// 유닛을 놓을 수 있는지 판정한다. 생산 규칙(덱 등록 → 재충전)이 먼저이고, 그다음 위치·자리·Storm Power·에너지다.
    /// 생산 규칙 검사는 <see cref="EnforceProductionRules"/> 가 꺼져 있으면 건너뛴다.
    /// </summary>
    /// <param name="player">플레이어</param>
    /// <param name="typeName">유닛 타입 이름</param>
    /// <param name="x">기준점 칸 x</param>
    /// <param name="y">기준점 칸 y</param>
    public SessionPlacementCheck CheckUnit(int player, string typeName, int x, int y)
    {
        if (!_players.TryGetValue(player, out PlayerState? state))
        {
            return new SessionPlacementCheck(CommandFailure.UnknownPlayer, null);
        }
        TypeInfo? type = _types.Find(typeName);
        if (type == null)
        {
            return new SessionPlacementCheck(CommandFailure.UnknownType, null);
        }
        if (ProducibleUnit.FromType(type) == null)
        {
            return new SessionPlacementCheck(CommandFailure.WrongKind, null);
        }
        Footprint footprint = Footprint.ForType(type.Definition, x, y);
        bool connected = AssumeConnected || IsConnectedToHome(player, x, y);
        PlacementCheck site = Map.CheckUnit(type, x, y, player, state.StormPower, connected, IsAtOwnBridgeEnd(footprint, player));
        // 생산 규칙 실패가 있으면 위치 판정보다 먼저 알린다
        CommandFailure rule = EnforceProductionRules ? CheckProductionRules(state, type) : CommandFailure.None;
        if (rule != CommandFailure.None)
        {
            return new SessionPlacementCheck(rule, site);
        }
        return new SessionPlacementCheck(site.Allowed ? CommandFailure.None : CommandFailure.Placement, site);
    }

    /// <summary>
    /// 기술 허용 표 → 덱 등록 → 재충전을 차례로 검사한다. 원본도 덱 갱신에서 표를 확인한다
    /// (Combatgump 생산 창 처리, 템플의 골렘 항목은 sunWalker 를 조회 — docs/exe/mission-header-flags.md).
    /// </summary>
    private CommandFailure CheckProductionRules(PlayerState state, TypeInfo type)
    {
        if (!state.Tech.IsAllowed(type.Name))
        {
            return CommandFailure.TechDenied;
        }
        // 덱에 등록된 유닛(워크샵 등록 또는 템플의 골렘)이어야 한다
        bool inDeck = state.Deck.Entries().Any(e => e.Kind != DeckEntryKind.Bridge
            && e.TypeName.Equals(type.Name, StringComparison.OrdinalIgnoreCase));
        if (!inDeck)
        {
            return CommandFailure.NotInDeck;
        }
        return state.UnitReadyTick.TryGetValue(type.Name, out long ready) && Tick < ready ? CommandFailure.NotReady : CommandFailure.None;
    }

    /// <summary>
    /// 건물(템플·워크샵·알타·아웃포스트)을 지을 수 있는지 판정한다: 기술 허용 표 → 위치 → 자리 → Storm Power → 에너지 순서다.
    /// 원본은 허용되지 않은 타입을 사제의 Construct 메뉴에서 아예 빼 버린다 (FUN_00461cf0).
    /// 사제의 이동·도달 거리와 건설 자리에 사제가 서 있어야 하는지는 확인하지 못해 판정하지 않는다.
    /// </summary>
    /// <param name="player">플레이어</param>
    /// <param name="typeName">건물 타입 이름</param>
    /// <param name="x">기준점 칸 x</param>
    /// <param name="y">기준점 칸 y</param>
    public SessionPlacementCheck CheckBuilding(int player, string typeName, int x, int y)
    {
        if (!_players.TryGetValue(player, out PlayerState? state))
        {
            return new SessionPlacementCheck(CommandFailure.UnknownPlayer, null);
        }
        TypeInfo? type = _types.Find(typeName);
        if (type == null)
        {
            return new SessionPlacementCheck(CommandFailure.UnknownType, null);
        }
        if (!ObjectKinds.IsBuilding(ObjectKinds.Of(type)))
        {
            return new SessionPlacementCheck(CommandFailure.WrongKind, null);
        }
        // 건설 중인 템플도 "이미 템플이 있음"으로 센다 (플레이어당 동시에 1기)
        bool hasTemple = _entities.Values.Any(e => e.Owner == player && e.Kind == ObjectKind.Temple);
        PlacementCheck site = Map.CheckBuilding(type, x, y, player, state.StormPower, hasTemple);
        if (EnforceProductionRules && !state.Tech.IsAllowed(type.Name))
        {
            return new SessionPlacementCheck(CommandFailure.TechDenied, site);
        }
        return new SessionPlacementCheck(site.Allowed ? CommandFailure.None : CommandFailure.Placement, site);
    }

    /// <summary>다리 조각을 (originX, originY) 왼쪽 위 칸에 놓을 수 있는지 판정한다.</summary>
    /// <param name="player">플레이어</param>
    /// <param name="pattern">조각 모양 번호</param>
    /// <param name="rotation">회전 번호</param>
    /// <param name="originX">조각 왼쪽 위 칸 x</param>
    /// <param name="originY">조각 왼쪽 위 칸 y</param>
    public BridgePlacementCheck CheckBridge(int player, int pattern, int rotation, int originX, int originY) =>
        Bridges.Check(new BridgePiece(pattern, rotation), originX, originY, player);

    /// <summary>플레이어가 유닛을 다시 놓을 수 있게 될 때까지 남은 시간(초). 준비되어 있으면 0.</summary>
    /// <param name="player">플레이어</param>
    /// <param name="typeName">유닛 타입 이름</param>
    public double SecondsUntilReady(int player, string typeName)
    {
        if (!_players.TryGetValue(player, out PlayerState? state) || !state.UnitReadyTick.TryGetValue(typeName, out long ready) || Tick >= ready)
        {
            return 0;
        }
        return (ready - Tick) / (double)TicksPerSecond;
    }

    /// <summary>건설 진행률 (0~1). 완성된 오브젝트는 1.</summary>
    /// <param name="entity">오브젝트</param>
    public double ConstructionProgress(GameEntity entity)
    {
        if (entity.IsComplete || entity.CompleteTick <= entity.StartTick)
        {
            return 1;
        }
        return Math.Clamp((Tick - entity.StartTick) / (double)(entity.CompleteTick - entity.StartTick), 0, 1);
    }

    // ───────────────────────── 명령 실행 ─────────────────────────

    /// <summary>명령 종류에 맞는 실행 함수를 고른다.</summary>
    private CommandResult Execute(GameCommand command) => command switch
    {
        PlaceUnitCommand c => ExecutePlaceUnit(c),
        ConstructBuildingCommand c => ExecuteConstruct(c),
        RegisterKnowledgeCommand c => ExecuteRegister(c),
        SalvageCommand c => ExecuteSalvage(c),
        SelectEntityCommand c => ExecuteSelect(c),
        HarvestGeyserCommand c => ExecuteHarvestGeyser(c),
        MoveEntityCommand c => ExecuteMoveEntity(c),
        StopEntityCommand c => ExecuteStopEntity(c),
        UpgradeWorkshopCommand c => ExecuteUpgradeWorkshop(c),
        ReturnHomeCommand c => ExecuteReturnHome(c),
        CapturePriestCommand c => ExecuteCapturePriest(c),
        DeliverPriestCommand c => ExecuteDeliverPriest(c),
        DropPriestCommand c => ExecuteDropPriest(c),
        MovePriestToAltarCommand c => ExecuteMovePriestToAltar(c),
        PickBridgePieceCommand c => ExecutePickBridge(c),
        ReturnBridgePieceCommand c => ExecuteReturnBridge(c),
        PlaceBridgeCommand c => ExecutePlaceBridge(c),
        _ => new CommandResult(CommandFailure.UnknownCommand, command.GetType().Name),
    };

    /// <summary>판정 실패를 명령 결과로 바꾼다.</summary>
    private static CommandResult Reject(SessionPlacementCheck check) => new(check.Failure, check.Describe());

    /// <summary>유닛 배치: 판정 → Storm Power 차감 → 점유·공급원 등록 → 재충전 예약.</summary>
    private CommandResult ExecutePlaceUnit(PlaceUnitCommand command)
    {
        SessionPlacementCheck check = CheckUnit(command.Player, command.TypeName, command.X, command.Y);
        if (!check.Allowed)
        {
            return Reject(check);
        }
        PlayerState player = _players[command.Player];
        TypeInfo type = _types.Find(command.TypeName)!;
        PlacementCheck site = check.Site!;
        int id = Map.PlaceUnit(type, site, command.Player);
        player.StormPower -= site.Cost;
        var entity = new GameEntity(id, type, ObjectKinds.Of(type), command.Player, site.Footprint, Map.TerritoryAt(command.X, command.Y), null);
        _entities.Add(id, entity);
        if (type.Definition.HasFlag("createsisland")) Bridges.InvalidateTerrain();
        // 유닛은 놓을 때 지은 수로 센다 (튜토리얼 단계 처리가 읽는다)
        player.RecordMade(type.Name, type.Flags2);
        if (EnforceProductionRules)
        {
            // 배치한 유닛은 Unit Rate 에 따른 시간이 지나야 덱에서 다시 쓸 수 있다 (docs/exe/production-refresh.md)
            double interval = ProductionTimers.RefreshInterval(Map.Options.Get(BattleOptions.UnitRate), inFortMode: false);
            player.UnitReadyTick[type.Name] = Tick + TicksFor(interval);
        }
        Emit(SessionEventKind.UnitPlaced, command.Player, id, $"{entity.DisplayName} 배치 (−{site.Cost})");
        return CommandResult.Ok();
    }

    /// <summary>건물 건설 시작: 판정 → Storm Power 차감 → 자리 점유 → 건설 시간 뒤 완성 (완성 때 규칙 효과).</summary>
    private CommandResult ExecuteConstruct(ConstructBuildingCommand command)
    {
        if (EnforceProductionRules && OwnFreePriest(command.Player) == null) return new CommandResult(CommandFailure.NoPriest);
        SessionPlacementCheck check = CheckBuilding(command.Player, command.TypeName, command.X, command.Y);
        if (!check.Allowed)
        {
            return Reject(check);
        }
        PlayerState player = _players[command.Player];
        TypeInfo type = _types.Find(command.TypeName)!;
        PlacementCheck site = check.Site!;
        ObjectKind kind = ObjectKinds.Of(type);
        int id = Map.NextId();
        Map.AddOccupant(site.Footprint);
        player.StormPower -= site.Cost;
        var entity = new GameEntity(id, type, kind, command.Player, site.Footprint, Map.TerritoryAt(command.X, command.Y), null)
        {
            IsComplete = false,
            StartTick = Tick,
            CompleteTick = Tick + TicksFor(ConstructionTimes.Seconds(kind)),
        };
        _entities.Add(id, entity);
        Emit(SessionEventKind.BuildingStarted, command.Player, id, $"{entity.DisplayName} 건설 시작 (−{site.Cost})");
        return CommandResult.Ok();
    }

    /// <summary>지식 등록: 워크샵의 원소·빈 칸·중복 규칙은 ProductionDeck 이 판정한다.</summary>
    private CommandResult ExecuteRegister(RegisterKnowledgeCommand command)
    {
        if (!_players.TryGetValue(command.Player, out PlayerState? player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        TypeInfo? type = _types.Find(command.TypeName);
        ProducibleUnit? unit = type == null ? null : ProducibleUnit.FromType(type);
        if (type == null || unit == null)
        {
            return new CommandResult(type == null ? CommandFailure.UnknownType : CommandFailure.WrongKind);
        }
        if (EnforceProductionRules && !player.Tech.IsAllowed(type.Name))
        {
            return new CommandResult(CommandFailure.TechDenied);
        }
        if (!EnforceProductionRules)
        {
            // 배치 시험 모드는 지식 획득 절차를 건너뛰고 곧바로 등록할 수 있게 한다
            player.Deck.LearnKnowledge(type.Name);
        }
        RegisterResult result = player.Deck.Register(command.WorkshopId, unit);
        if (result != RegisterResult.Registered)
        {
            return new CommandResult(CommandFailure.RegisterFailed, $"등록 실패: {result}");
        }
        Emit(SessionEventKind.Registered, command.Player, command.WorkshopId, $"{type.Definition.GetString("description") ?? type.Name} 등록");
        return CommandResult.Ok();
    }

    /// <summary>워크샵 소유권·완공·최대 단계·비용을 확인하고 생산 칸을 늘린다. 시간은 추후 원본 대조 대상이다.</summary>
    private CommandResult ExecuteUpgradeWorkshop(UpgradeWorkshopCommand command)
    {
        GameEntity? workshop = Entity(command.WorkshopId);
        if (workshop == null) return new CommandResult(CommandFailure.NoSuchEntity);
        if (workshop.Owner != command.Player) return new CommandResult(CommandFailure.NotOwner);
        if (workshop.Kind != ObjectKind.Workshop) return new CommandResult(CommandFailure.WrongKind);
        if (!workshop.IsComplete) return new CommandResult(CommandFailure.NotComplete);
        PlayerState player = Player(command.Player);
        if (player.Deck.WorkshopLevel(workshop.Id) >= ProductionDeck.MaxWorkshopLevel) return new CommandResult(CommandFailure.NotReady, "최대 단계 워크샵");
        int cost = WorkshopUpgradeCostFor(workshop.Type.Definition);
        if (player.StormPower < cost) return new CommandResult(CommandFailure.Placement, $"워크샵 업그레이드에 {cost:N0} SP 필요");
        player.StormPower -= cost;
        player.Deck.UpgradeWorkshop(workshop.Id);
        Emit(SessionEventKind.Registered, player.Number, workshop.Id, $"워크샵 단계 {player.Deck.WorkshopLevel(workshop.Id)} (−{cost})");
        return CommandResult.Ok();
    }

    /// <summary>
    /// 회수: 비용의 25% 를 돌려받고, 오브젝트가 준 규칙 효과(점유·공급원·섬 소유·덱)를 되돌린다.
    /// <see cref="DenySalvage"/> 가 켜져 있으면 회수하지 않는다 (원본: 미션 스크립트 "DenySalvage" 섹션을 알리고 끝).
    /// </summary>
    private CommandResult ExecuteSalvage(SalvageCommand command)
    {
        if (!_players.TryGetValue(command.Player, out PlayerState? player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        GameEntity? entity = Entity(command.EntityId);
        if (entity == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        if (entity.Owner != command.Player)
        {
            return new CommandResult(CommandFailure.NotOwner);
        }
        if (EnforceProductionRules && DenySalvage)
        {
            return new CommandResult(CommandFailure.SalvageDenied);
        }
        if (!SalvageableKinds.Contains(entity.Kind))
        {
            return new CommandResult(CommandFailure.CannotSalvage);
        }
        if (!entity.IsComplete)
        {
            return new CommandResult(CommandFailure.NotComplete);
        }
        int refund = StormPower.SalvageValue(entity.Cost);
        player.StormPower += refund;
        RemoveEntity(entity);
        Emit(SessionEventKind.Salvaged, command.Player, entity.Id, $"{entity.DisplayName} 회수 (+{refund})");
        return CommandResult.Ok();
    }

    /// <summary>선택: 있는 오브젝트를 고르거나(번호) 선택을 푼다(0). 다른 플레이어의 오브젝트도 선택해 살펴볼 수 있다.</summary>
    private CommandResult ExecuteSelect(SelectEntityCommand command)
    {
        if (!_players.TryGetValue(command.Player, out PlayerState? player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        if (command.EntityId != 0 && Entity(command.EntityId) == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        player.SelectedEntityId = command.EntityId;
        return CommandResult.Ok();
    }

    /// <summary>다리 조각 집기: 칸의 조각을 커서 조각으로 둔다 (칸에는 놓을 때까지 어둡게 남는다).</summary>
    private CommandResult ExecutePickBridge(PickBridgePieceCommand command)
    {
        if (!_players.TryGetValue(command.Player, out PlayerState? player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        if (player.HeldPiece != null)
        {
            return new CommandResult(CommandFailure.AlreadyHolding);
        }
        if (!player.Tray.CanTake(command.TrayIndex))
        {
            return new CommandResult(CommandFailure.NoBridgePiece);
        }
        player.HeldPiece = player.Tray.Take(command.TrayIndex);
        Emit(SessionEventKind.BridgePicked, command.Player, 0, $"{player.HeldPiece} 집음");
        return CommandResult.Ok();
    }

    /// <summary>다리 조각 되돌리기: 집은 조각을 칸에 다시 둔다 (칸이 비워졌는데 빈 칸이 없으면 계속 들고 있는다).</summary>
    private CommandResult ExecuteReturnBridge(ReturnBridgePieceCommand command)
    {
        if (!_players.TryGetValue(command.Player, out PlayerState? player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        if (player.HeldPiece == null)
        {
            return new CommandResult(CommandFailure.NotHolding);
        }
        if (!player.Tray.Return(player.HeldPiece))
        {
            return new CommandResult(CommandFailure.TrayFull);
        }
        player.HeldPiece = null;
        Emit(SessionEventKind.BridgeReturned, command.Player, 0, "다리 조각을 칸으로 되돌림");
        return CommandResult.Ok();
    }

    /// <summary>다리 조각 놓기: 집은 조각을 명령의 회전으로 돌려 판정하고, 통과하면 격자에 놓는다.</summary>
    private CommandResult ExecutePlaceBridge(PlaceBridgeCommand command)
    {
        if (!_players.TryGetValue(command.Player, out PlayerState? player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        if (player.HeldPiece is not { } held)
        {
            return new CommandResult(CommandFailure.NotHolding);
        }
        var piece = new BridgePiece(held.Pattern.Index, command.Rotation);
        BridgePlacementCheck check = Bridges.Check(piece, command.X, command.Y, command.Player);
        if (!check.Allowed)
        {
            return new CommandResult(CommandFailure.BridgeBlocked, SessionText.Describe(check.Problem));
        }
        // 생산 창에 들어온 지 6초가 안 된 조각은 금 간 채로 놓인다
        BridgeCondition quality = BridgeTray.QualityAt(held, Seconds);
        Bridges.Place(piece, command.X, command.Y, command.Player, quality);
        // 놓은 조각의 칸을 비운다 (원본도 다음 채우기에서 그 자리를 채운다)
        player.Tray.ConsumeHeld();
        player.HeldPiece = null;
        string qualityText = quality == BridgeCondition.Cracked ? ", 금 간 품질" : "";
        Emit(SessionEventKind.BridgePlaced, command.Player, 0,
            $"{piece} 을(를) ({command.X}, {command.Y})에 놓음 (연결 {check.Attachments}곳{qualityText})");
        return CommandResult.Ok();
    }

    /// <summary>
    /// 없어질 때 주변 다리를 약화하는 오브젝트인지: 타입에 maxHitPoints 가 있고(플래그1 0x10) 이동체(walker·balloon·flyer,
    /// 플래그2 0x130000)가 아니다 — 즉 건물형 유닛·건물. 원본 FUN_0044b9e0 의 조건(FUN_004adc60, 플래그2 & 0x130000 == 0)이다.
    /// </summary>
    /// <param name="type">오브젝트 타입</param>
    private static bool WeakensBridgesOnRemoval(TypeInfo type) =>
        type.Definition.GetString("maxHitPoints") != null && (type.Flags2 & MobileGenusFlags) == 0;

    /// <summary>
    /// 건물형 유닛이 없어진 뒤 주변 다리를 한 단계 약화하고 결과를 이벤트로 알린다 (<see cref="BridgeGrid.WeakenAround"/>).
    /// 원본은 오브젝트가 월드에서 빠지는 공통 처리에서 제거 이유를 보지 않고 약화하므로 파괴·회수 모두 해당한다고 본다.
    /// </summary>
    /// <param name="entity">없어진 오브젝트</param>
    private void WeakenBridgesAround(GameEntity entity)
    {
        if (!WeakensBridgesOnRemoval(entity.Type))
        {
            return;
        }
        // 원본은 발자국 중심(실수)을 소수점 버림한 칸을 중심으로 삼는다
        BridgeDecayResult result = Bridges.WeakenAround((int)Math.Truncate(entity.Footprint.CenterX), (int)Math.Truncate(entity.Footprint.CenterY));
        // 금 간 칸을 이벤트로 알린다
        foreach (BridgeCellState cell in result.Cracked)
        {
            Emit(SessionEventKind.BridgeCracked, cell.Owner, entity.Id, $"다리 ({cell.X}, {cell.Y}) 금 감 ({entity.DisplayName} 없어짐)");
        }
        // 무너진 칸을 이벤트로 알린다
        foreach (BridgeCellState cell in result.Removed)
        {
            Emit(SessionEventKind.BridgeCollapsed, cell.Owner, entity.Id, $"다리 ({cell.X}, {cell.Y}) 무너짐 ({entity.DisplayName} 없어짐)");
        }
    }
}
