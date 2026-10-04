using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>유닛 생산 예약 → 자원 운송 → 실체화 → 활성화를 고정 틱에서 처리한다.</summary>
public sealed partial class BattleSession
{
    /// <summary>팬게임·manabolt.type의 원소 에너지 속도. 타입 값이 없을 때만 쓴다.</summary>
    private const double ElementDeliverySpeed = 18;
    /// <summary>스톰 파워 이동 속도의 대체값(teleportEffect.type).</summary>
    private const double StormDeliverySpeed = 12;
    /// <summary>팬게임의 포대 실체화 시간(초).</summary>
    private const double EmplacementMaterializationSeconds = 0.6;
    /// <summary>팬게임의 나머지 유닛 실체화 시간(초).</summary>
    private const double UnitMaterializationSeconds = 1;
    /// <summary>도착 거리 오차 한계.</summary>
    private const double DeliveryEpsilon = 0.0000001;

    /// <summary>덱에 저장한 실제 생산 기점을 찾는다. 가까운 다른 워크샵으로 바꾸지 않는다.</summary>
    private GameEntity? ProductionSource(PlayerState player, string typeName)
    {
        DeckEntry? entry = player.Deck.Entries().FirstOrDefault(e => e.Kind != DeckEntryKind.Bridge
            && e.TypeName.Equals(typeName, StringComparison.OrdinalIgnoreCase));
        return entry == null ? null : Entity(entry.SourceId);
    }

    /// <summary>유닛은 예약 자리만 점유하고 활성화에 필요한 자원 운송을 시작한다.</summary>
    private void StartUnitProduction(GameEntity entity, GameEntity source, PlacementCheck site)
    {
        entity.IsComplete = false;
        entity.HitPoints = 0;
        var deliveries = new List<ProductionDelivery>();
        // 기점 발자국에서 목표와 가장 가까운 칸을 스톰 파워 출구로 삼는다(팬게임 Hd).
        int startX = Math.Clamp((int)Math.Floor(site.Footprint.CenterX + 0.5), source.Footprint.Left, source.Footprint.AnchorX);
        int startY = Math.Clamp((int)Math.Floor(site.Footprint.CenterY + 0.5), source.Footprint.Top, source.Footprint.AnchorY);
        ProductionDelivery storm = NewDelivery(source.Id, null, startX, startY, site.Footprint,
            _types.Find("teleportEffect")?.Definition.GetDouble("speed") ?? StormDeliverySpeed);
        ReplanProductionDelivery(storm, entity.Owner);
        deliveries.Add(storm);
        // 같은 원소 안에서는 가까운 공급원을 먼저 쓰고 동일 공급원은 한 번만 배정한다.
        EnergyCheck energy = EnergySupply.Match(site.Requirement, site.Energy.Covering
            .OrderBy(s => Math.Pow(s.CenterX - site.Footprint.CenterX, 2) + Math.Pow(s.CenterY - site.Footprint.CenterY, 2))
            .ThenBy(s => s.Id).ToArray());
        // 요구 글자마다 별도의 원소 운송을 만들어 가장 늦은 도착을 기다린다.
        foreach (EnergySource assigned in energy.Assigned.OfType<EnergySource>())
        {
            ProductionDelivery delivery = NewDelivery(assigned.Id, assigned.Element, assigned.CenterX, assigned.CenterY,
                site.Footprint, _types.Find("manabolt")?.Definition.GetDouble("speed") ?? ElementDeliverySpeed);
            delivery.IsAirborne = true;
            deliveries.Add(delivery);
        }
        entity.Production = new UnitProduction(source.Id, deliveries);
    }

    /// <summary>공급원 위치와 목적지를 고정한다. 공급원이 나중에 사라져도 출발한 자원은 계속 간다.</summary>
    private ProductionDelivery NewDelivery(int sourceId, Element? element, double x, double y, Footprint target, double speed) => new()
    {
        SourceId = sourceId, Element = element, X = x, Y = y, PreviousX = x, PreviousY = y,
        TargetX = target.CenterX, TargetY = target.CenterY, Speed = speed > 0 ? speed : StormDeliverySpeed,
        RouteVersion = Bridges.Version,
    };

    /// <summary>현재 위치에서 지상 경로를 다시 찾는다. 실패하면 그 자리에서 공중 직선 이동으로 전환한다.</summary>
    private void ReplanProductionDelivery(ProductionDelivery delivery, int owner)
    {
        var start = new Footprint((int)Math.Floor(delivery.X + 0.5), (int)Math.Floor(delivery.Y + 0.5), 1, 1);
        // 안전한 걸음 도중에는 다음 칸에서 길을 잇는다. 무관한 지형 변화로 이전 칸까지 되돌아가지 않는다.
        if (delivery.NextIndex > 0 && delivery.NextIndex < delivery.Path.Count)
        {
            var from = delivery.Path[delivery.NextIndex - 1];
            var next = delivery.Path[delivery.NextIndex];
            if (IsHarvestPassable(from.X, from.Y, owner) && CanWalkStep(from.X, from.Y, next.X, next.Y, owner))
                start = new Footprint(next.X, next.Y, 1, 1);
        }
        var goal = new Footprint((int)Math.Floor(delivery.TargetX + 0.5), (int)Math.Floor(delivery.TargetY + 0.5), 1, 1);
        List<(int X, int Y)>? path = FindHarvestPath(start, goal, owner, exact: true);
        delivery.RouteVersion = Bridges.Version;
        delivery.Path = path ?? [];
        // 재탐색 때에도 현재 소수 좌표에서 새 첫 칸으로 이어 이동해 순간이동하지 않는다.
        delivery.NextIndex = 0;
        if (path == null) delivery.IsAirborne = true;
    }

    /// <summary>운송과 실체화 시간을 진행한다. 자원이 모두 도착하기 전에는 그림·체력·공급 효과가 없다.</summary>
    private void UpdateUnitProductions()
    {
        // 생산 예약을 오브젝트 번호순으로 갱신해 같은 명령열의 활성화 순서를 고정한다.
        foreach (GameEntity entity in _entities.Values.Where(e => e.Production != null).ToArray())
        {
            UnitProduction production = entity.Production!;
            // 스톰 파워와 각 원소는 독립적으로 이동한다.
            foreach (ProductionDelivery delivery in production.Deliveries)
            {
                if (delivery.HasArrived) continue;
                delivery.PreviousX = delivery.X;
                delivery.PreviousY = delivery.Y;
                if (!delivery.IsAirborne && delivery.RouteVersion != Bridges.Version)
                    ReplanProductionDelivery(delivery, entity.Owner);
                AdvanceProductionDelivery(delivery);
            }
            if (production.AwaitingDeliveries) continue;
            if (!ProductionSiteSupported(entity))
            {
                CancelUnitProduction(entity);
                continue;
            }
            if (entity.CompleteTick == 0)
            {
                entity.StartTick = Tick;
                entity.CompleteTick = Tick + TicksFor(entity.Kind == ObjectKind.Emplacement
                    ? EmplacementMaterializationSeconds : UnitMaterializationSeconds);
                Emit(SessionEventKind.UnitMaterializing, entity.Owner, entity.Id, $"{entity.DisplayName} 자원 도착·실체화");
            }
            if (Tick < entity.CompleteTick) continue;
            entity.IsComplete = true;
            entity.Production = null;
            entity.HitPoints = entity.MaxHitPoints;
            if (entity.Kind == ObjectKind.Generator && Elements.FromTheme(entity.Type.Definition.GetString("theme")) is { } element)
                Map.AddSource(new EnergySource(entity.Id, element, entity.WorldX, entity.WorldY, entity.Owner));
            if (entity.Type.Definition.HasFlag("createsisland")) Bridges.InvalidateTerrain();
            Player(entity.Owner).RecordMade(entity.Type.Name, entity.Type.Flags2);
            Emit(SessionEventKind.UnitCompleted, entity.Owner, entity.Id, $"{entity.DisplayName} 생산 완료");
        }
    }

    /// <summary>한 틱 이동량을 경로 칸과 최종 소수 중심 좌표까지 모두 사용한다.</summary>
    private void AdvanceProductionDelivery(ProductionDelivery delivery)
    {
        double remaining = delivery.Speed / TicksPerSecond;
        // 여러 칸을 지나는 빠른 운송도 한 틱의 남은 이동량을 버리지 않는다.
        while (!delivery.HasArrived && remaining > DeliveryEpsilon)
        {
            bool waypoint = !delivery.IsAirborne && delivery.NextIndex < delivery.Path.Count;
            double x = waypoint ? delivery.Path[delivery.NextIndex].X : delivery.TargetX;
            double y = waypoint ? delivery.Path[delivery.NextIndex].Y : delivery.TargetY;
            double dx = x - delivery.X, dy = y - delivery.Y;
            double distance = Math.Sqrt(dx * dx + dy * dy);
            if (distance <= remaining + DeliveryEpsilon)
            {
                delivery.X = x;
                delivery.Y = y;
                remaining = Math.Max(0, remaining - distance);
                if (waypoint) delivery.NextIndex++;
                else delivery.HasArrived = true;
            }
            else
            {
                delivery.X += dx / distance * remaining;
                delivery.Y += dy / distance * remaining;
                remaining = 0;
            }
        }
    }

    /// <summary>목적지 발판이 남았는지. 받침을 만드는 유닛은 섬 밖에서도 둘레의 다리 끝이 남으면 완성한다.</summary>
    private bool ProductionSiteSupported(GameEntity entity) => entity.Type.Definition.HasFlag("balloon")
        || HasGroundSupport(entity.Footprint.AnchorX, entity.Footprint.AnchorY)
        || entity.Type.Definition.HasFlag("createsisland") && entity.Footprint.BorderCells().Any(c => HasGroundSupport(c.X, c.Y));

    /// <summary>목적지까지 사라진 예약은 전액 환불한다. 실제 유닛 제거가 아니므로 다리 약화·폭발이 없다.</summary>
    private void CancelUnitProduction(GameEntity entity)
    {
        Player(entity.Owner).StormPower += entity.Cost;
        if (entity.OccupiesGround) Map.RemoveOccupant(entity.Footprint);
        _entities.Remove(entity.Id);
        // 없어진 예약을 선택한 모든 플레이어의 선택을 해제한다.
        foreach (PlayerState viewer in _players.Values.Where(p => p.SelectedEntityId == entity.Id)) ClearSelection(viewer);
        Emit(SessionEventKind.UnitProductionCancelled, entity.Owner, entity.Id, $"{entity.DisplayName} 발판 소멸·생산 환불 (+{entity.Cost})");
    }

    /// <summary>예약·운송 경로·현재 좌표를 검사합에 넣는다. 화면 보간용 직전 좌표는 제외한다.</summary>
    private static void AddProductionChecksum(Fnv1a hash, UnitProduction? production)
    {
        hash.Add(production == null ? 0 : 1);
        if (production == null) return;
        hash.Add(production.SourceId);
        hash.Add(production.Deliveries.Count);
        // 운송 생성 순서와 경로 칸 순서가 이후 도착 시각을 결정한다.
        foreach (ProductionDelivery delivery in production.Deliveries)
        {
            hash.Add(delivery.SourceId); hash.Add(delivery.Element is { } element ? (int)element : -1);
            hash.Add(BitConverter.DoubleToInt64Bits(delivery.X)); hash.Add(BitConverter.DoubleToInt64Bits(delivery.Y));
            hash.Add(BitConverter.DoubleToInt64Bits(delivery.TargetX)); hash.Add(BitConverter.DoubleToInt64Bits(delivery.TargetY));
            hash.Add(BitConverter.DoubleToInt64Bits(delivery.Speed));
            hash.Add(delivery.IsAirborne ? 1 : 0); hash.Add(delivery.HasArrived ? 1 : 0);
            hash.Add(delivery.NextIndex); hash.Add(delivery.RouteVersion); hash.Add(delivery.Path.Count);
            // 각 경로 칸을 이동 순서대로 섞는다.
            foreach ((int x, int y) in delivery.Path) { hash.Add(x); hash.Add(y); }
        }
    }
}
