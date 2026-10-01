using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>Whirlibase와 Whirligig의 첫 비행 모델. 미확정 시간·거리 값은 flyers.md에 기록한다.</summary>
public sealed partial class BattleSession
{
    /// <summary>원본 도움말의 1분 출격 뒤 보급 규칙.</summary>
    public const double WhirligigSortieSeconds = 60;
    /// <summary>원본 생성 애니메이션 시간 미측정으로 사용하는 임시 재생성 지연.</summary>
    public const double WhirligigBuildSeconds = 5;
    /// <summary>원본 보급 애니메이션 시간 미측정으로 사용하는 임시 보급 지연.</summary>
    public const double WhirligigRefuelSeconds = 3;
    /// <summary>적 중심에서 공격을 시작하는 임시 접근 거리(논리 칸).</summary>
    private const double FlyerAttackReach = 1.5;
    /// <summary>도움말이 정한 한 목표의 최대 공중 공격체 수.</summary>
    private const int MaximumFlyersPerTarget = 3;

    /// <summary>완공된 기지의 무료 생성과 비행체 이동을 번호순으로 처리한다.</summary>
    private void UpdateFlyers()
    {
        TypeInfo? flyerType = _types.Find("sunFlyer");
        if (flyerType == null) return;
        // 기지별 한 기만 유지한다. 사망 후에는 사망 시점부터 생성 지연을 다시 센다.
        foreach (GameEntity home in _entities.Values.Where(e => e.Type.Name.Equals("sunaviary", StringComparison.OrdinalIgnoreCase)).ToArray())
        {
            if (!home.IsComplete || home.Owner <= 0 || home.HitPoints <= 0 ||
                _entities.Values.Any(e => e.Flight?.BaseId == home.Id)) continue;
            if (home.NextAttackTick == 0) home.NextAttackTick = Tick + TicksFor(WhirligigBuildSeconds);
            if (Tick < home.NextAttackTick) continue;
            var flight = new FlyerFlight { BaseId = home.Id, OriginX = home.WorldX, OriginY = home.WorldY,
                X = home.WorldX, Y = home.WorldY };
            var flyer = new GameEntity(Map.NextId(), flyerType, ObjectKind.Flyer, home.Owner,
                new Footprint((int)flight.X, (int)flight.Y, 1, 1), null, null) { Flight = flight };
            _entities.Add(flyer.Id, flyer);
            Emit(SessionEventKind.FlyerLaunched, flyer.Owner, flyer.Id, $"{home.DisplayName}: Whirligig 생성");
        }
        // 이동·소멸이 목록을 바꿀 수 있으므로 이번 틱의 비행체를 먼저 고정한다.
        foreach (GameEntity flyer in _entities.Values.Where(e => e.Flight != null).ToArray())
        {
            FlyerFlight flight = flyer.Flight!;
            GameEntity? home = Entity(flight.BaseId);
            if (flight.Phase == FlyerPhase.Docked && home == null) { ExpireFlyer(flyer); continue; }
            if (flight.Phase == FlyerPhase.Refuelling)
            {
                if (home == null) { ExpireFlyer(flyer); continue; }
                if (Tick < flight.ReadyTick) continue;
                flight.Phase = FlyerPhase.Docked;
            }
            if (flight.Phase == FlyerPhase.Hunting && Tick >= flight.ReturnTick)
            {
                flight.Phase = FlyerPhase.Returning;
                flyer.AttackTargetId = 0;
            }
            if (flight.Phase == FlyerPhase.Returning)
            {
                // 기지 파괴는 출격 중인 비행체를 즉시 없애지 않는다. 귀환할 곳이 없을 때 소멸한다.
                if (home == null) { ExpireFlyer(flyer); continue; }
                if (MoveFlyer(flyer, home.WorldX, home.WorldY, 0))
                {
                    flight.Phase = FlyerPhase.Refuelling;
                    flight.ReadyTick = Tick + TicksFor(WhirligigRefuelSeconds);
                    Emit(SessionEventKind.FlyerRefuelling, flyer.Owner, flyer.Id, "Whirligig 귀환·보급");
                }
                continue;
            }
            GameEntity? target = Entity(flyer.AttackTargetId);
            if (target == null || !CanFlyerTarget(flyer, target))
            {
                flyer.AttackTargetId = 0;
                target = _entities.Values.Where(e => CanFlyerTarget(flyer, e))
                    .OrderBy(e => DistanceSquared(flyer, e)).ThenBy(e => e.Id).FirstOrDefault();
                flyer.AttackTargetId = target?.Id ?? 0;
            }
            if (target == null)
            {
                if (flight.Phase == FlyerPhase.Hunting) flight.Phase = FlyerPhase.Returning;
                else if (home == null) ExpireFlyer(flyer);
                continue;
            }
            if (flight.Phase == FlyerPhase.Docked)
            {
                flight.Phase = FlyerPhase.Hunting;
                flight.ReturnTick = Tick + TicksFor(WhirligigSortieSeconds);
            }
            if (!MoveFlyer(flyer, target.WorldX, target.WorldY, FlyerAttackReach) || Tick < flyer.NextAttackTick) continue;
            // 비행체는 지상 차폐 위에서 공격한다. 정확한 타격 프레임 대신 1초마다 다음 틱 피해를 예약한다.
            _shots.Add(new CombatShot(flyer.Id, flyer.Owner, target.Id, flyer.Type.Definition.GetDouble("hpPerSec") ?? 0,
                flyer.WorldX, flyer.WorldY, target.WorldX, target.WorldY, Tick, Tick + 1));
            flyer.NextAttackTick = Tick + TicksFor(FallbackShotSeconds);
            Emit(SessionEventKind.ShotFired, flyer.Owner, flyer.Id, $"Whirligig → {target.DisplayName} 공격");
        }
    }

    /// <summary>원본 도움말의 수송 제외·최대 3대와 패치 기록의 출발점 사거리를 적용한다.</summary>
    private bool CanFlyerTarget(GameEntity flyer, GameEntity target)
    {
        if (target.Owner <= 0 || target.HitPoints <= 0 || target.IsStunned || target.Kind == ObjectKind.Transport ||
            Map.AreAllied(flyer.Owner, target.Owner)) return false;
        FlyerFlight flight = flyer.Flight!;
        double dx = target.WorldX - flight.OriginX;
        double dy = target.WorldY - flight.OriginY;
        double range = flyer.Type.Definition.GetDouble("range") ?? 0;
        if (dx * dx + dy * dy > range * range) return false;
        return _entities.Values.Count(e => e.Kind == ObjectKind.Flyer && e.Id != flyer.Id && e.AttackTargetId == target.Id)
            < MaximumFlyersPerTarget;
    }

    /// <summary>장애물·다리와 무관하게 직선 이동한다. speed를 칸/초로 해석한 임시 모델이다.</summary>
    private bool MoveFlyer(GameEntity flyer, double x, double y, double stopDistance)
    {
        FlyerFlight flight = flyer.Flight!;
        double dx = x - flight.X;
        double dy = y - flight.Y;
        double distance = Math.Sqrt(dx * dx + dy * dy);
        if (distance <= stopDistance) return true;
        double step = Math.Min(distance - stopDistance, (flyer.Type.Definition.GetDouble("speed") ?? 0) / TicksPerSecond);
        flight.X += dx / distance * step;
        flight.Y += dy / distance * step;
        flyer.Footprint = flyer.Footprint with { AnchorX = (int)Math.Round(flight.X), AnchorY = (int)Math.Round(flight.Y) };
        return step >= distance - stopDistance;
    }

    /// <summary>기지 없는 비행체의 연료 종료는 공격자의 파괴 보상을 만들지 않는다.</summary>
    private void ExpireFlyer(GameEntity flyer)
    {
        RemoveEntity(flyer);
        Emit(SessionEventKind.FlyerExpired, flyer.Owner, flyer.Id, "Whirligig 귀환 기지 없음");
    }

    /// <summary>부모·출발점·소수 좌표·단계·보급 시각을 결정론 검사합에 넣는다.</summary>
    private static void AddFlightChecksum(Fnv1a hash, FlyerFlight? flight)
    {
        hash.Add(flight == null ? 0 : 1);
        if (flight == null) return;
        hash.Add(flight.BaseId);
        hash.Add(BitConverter.DoubleToInt64Bits(flight.OriginX));
        hash.Add(BitConverter.DoubleToInt64Bits(flight.OriginY));
        hash.Add(BitConverter.DoubleToInt64Bits(flight.X));
        hash.Add(BitConverter.DoubleToInt64Bits(flight.Y));
        hash.Add((int)flight.Phase);
        hash.Add(flight.ReturnTick);
        hash.Add(flight.ReadyTick);
    }
}
