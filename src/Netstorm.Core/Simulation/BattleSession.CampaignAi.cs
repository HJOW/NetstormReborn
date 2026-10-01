using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>캠페인 1-1의 임시 방어 AI. 원본 전략의 디컴파일 결과가 아니며 docs/gameplay/campaign-one.md에 추정을 기록한다.</summary>
public sealed partial class BattleSession
{
    /// <summary>초기 AI 수집자는 미션 설정의 배치로 지급한다. 돈을 만들거나 사제를 수확에 쓰지 않는다.</summary>
    private void InitializeCampaignAi()
    {
        if (Mission is not { IsFirstCampaign: true, AiOff: false }) return;
        TypeInfo? walker = _types.Find("sunWalker");
        if (walker == null) return;
        // 적 번호 순으로 aiCollectors만큼 초기 골렘을 소유 섬에 놓는다.
        foreach (PlayerState player in _players.Values.Where(p => p.Number != HumanPlayer))
        {
            GameEntity? temple = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Temple && e.Owner == player.Number);
            if (temple == null) continue;
            // 원본의 초기 수집자 생성 위치는 미확정이며 신전 주변의 첫 합법 위치로 대체한다.
            for (int count = 0; count < Mission.AiCollectors; count++)
            {
                (int X, int Y)? position = FindAiSite(temple, (x, y) => Map.CheckUnit(walker, x, y, player.Number,
                    int.MaxValue, connectedToHome: false, atOwnBridgeEnd: false).Allowed);
                if (position == null) break;
                PlacementCheck site = Map.CheckUnit(walker, position.Value.X, position.Value.Y, player.Number, int.MaxValue, false, false);
                int id = Map.PlaceUnit(walker, site, player.Number);
                _entities.Add(id, new GameEntity(id, walker, ObjectKind.Transport, player.Number, site.Footprint,
                    Map.TerritoryAt(position.Value.X, position.Value.Y), null));
            }
        }
    }

    /// <summary>현재 수입·생산·포획 규칙으로 방어한다. 판단 시각은 틱 기반이라 재현 가능하다.</summary>
    private void UpdateCampaignAi()
    {
        if (Mission is not { IsFirstCampaign: true, AiOff: false } || Tick % TicksFor(Mission.AiMoveInterval) != 0) return;
        // 적 플레이어마다 동일한 우선순위로 판단한다.
        foreach (PlayerState player in _players.Values.Where(p => p.Number != HumanPlayer && !Map.AreAllied(p.Number, HumanPlayer)))
        {
            GameEntity? priest = OwnFreePriest(player.Number);
            GameEntity? temple = _entities.Values.FirstOrDefault(e => e.Owner == player.Number && e.Kind == ObjectKind.Temple && e.IsComplete);
            if (priest == null || temple == null) continue;
            GameEntity? carrier = _entities.Values.FirstOrDefault(e => e.Owner == player.Number && e.Kind == ObjectKind.Transport);
            GameEntity? altar = _entities.Values.FirstOrDefault(e => e.Owner == player.Number && e.Kind == ObjectKind.Altar);
            GameEntity? target = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Priest && e.Owner == HumanPlayer && CanCapture(player.Number, e));
            if (altar is { IsComplete: true } && carrier != null)
            {
                if (carrier.CarriedPriestId != 0 && !_moveTasks.ContainsKey(carrier.Id)) ExecuteDeliverPriest(new DeliverPriestCommand(player.Number, carrier.Id, altar.Id));
                else if (target != null && carrier.CarriedPriestId == 0 && !_moveTasks.ContainsKey(carrier.Id)) ExecuteCapturePriest(new CapturePriestCommand(player.Number, carrier.Id, target.Id, altar.Id));
                if ((carrier.CarriedPriestId != 0 || BoundPriestOf(altar.Id) != null) && !_moveTasks.ContainsKey(priest.Id))
                    ExecuteMovePriestToAltar(new MovePriestToAltarCommand(player.Number, altar.Id, priest.Id));
            }
            if (target != null && altar == null && TryAiConstruct(player, temple, "altar")) continue;
            // 운반 중이 아닌 골렘만 수확하며 원본의 !USE_PRIEST_TO_COLLECT 설정을 따른다.
            foreach (GameEntity collector in _entities.Values.Where(e => e.Owner == player.Number && e.Kind == ObjectKind.Transport && e.CarriedPriestId == 0).ToArray())
            {
                if (_moveTasks.ContainsKey(collector.Id) || _harvestTasks.ContainsKey(collector.Id)) continue;
                GameEntity? geyser = _entities.Values.Where(e => e.Kind == ObjectKind.Geyser && FindMovePath(collector, e.Footprint) != null)
                    .OrderBy(e => DistanceSquared(collector, e)).ThenBy(e => e.Id).FirstOrDefault();
                if (geyser != null) ExecuteHarvestGeyser(new HarvestGeyserCommand(player.Number, geyser.Id, collector.Id));
            }
            // 포위·공격 다리 확장은 하지 않고 기존 방어 탑만 보충한다.
            if (_entities.Values.Count(e => e.Owner == player.Number && e.Type.Name.Equals("sunArcher", StringComparison.OrdinalIgnoreCase)) >= 9) continue;
            GameEntity? workshop = _entities.Values.FirstOrDefault(e => e.Owner == player.Number && e.Kind == ObjectKind.Workshop && e.IsComplete);
            if (workshop == null)
            { if (!_entities.Values.Any(e => e.Owner == player.Number && e.Kind == ObjectKind.Workshop)) TryAiConstruct(player, temple, "sunFactory"); continue; }
            if (!player.Deck.Entries().Any(e => e.TypeName.Equals("sunArcher", StringComparison.OrdinalIgnoreCase)))
                ExecuteRegister(new RegisterKnowledgeCommand(player.Number, workshop.Id, "sunArcher"));
            (int X, int Y)? site = FindAiSite(temple, (x, y) => CheckUnit(player.Number, "sunArcher", x, y).Allowed);
            if (site != null) ExecutePlaceUnit(new PlaceUnitCommand(player.Number, "sunArcher", site.Value.X, site.Value.Y));
        }
    }

    /// <summary>돈·자리 등 일반 건설 규칙을 통과한 곳에서만 적 건물을 짓는다.</summary>
    private bool TryAiConstruct(PlayerState player, GameEntity temple, string type)
    {
        TypeInfo? info = _types.Find(type);
        if (info == null || player.StormPower < (info.Definition.GetInt("cost") ?? 0)) return false;
        (int X, int Y)? site = FindAiSite(temple, (x, y) => CheckBuilding(player.Number, type, x, y).Allowed);
        return site != null && ExecuteConstruct(new ConstructBuildingCommand(player.Number, type, site.Value.X, site.Value.Y)).Accepted;
    }

    /// <summary>신전 주변의 가까운 칸부터 정해진 순서로 합법 위치를 찾는다.</summary>
    private static (int X, int Y)? FindAiSite(GameEntity temple, Func<int, int, bool> valid)
    {
        // 가까운 정사각형 경계를 한 겹씩 확장한다.
        for (int radius = 1; radius <= 28; radius++)
        {
            // 각 겹의 행을 위에서 아래로 검사한다.
            for (int dy = -radius; dy <= radius; dy++)
            {
                // 내부 칸은 이미 검사했으므로 현재 경계만 검사한다.
                for (int dx = -radius; dx <= radius; dx++)
                {
                    if (Math.Abs(dx) != radius && Math.Abs(dy) != radius) continue;
                    int x = temple.Footprint.AnchorX + dx; int y = temple.Footprint.AnchorY + dy;
                    if (valid(x, y)) return (x, y);
                }
            }
        }
        return null;
    }
}
