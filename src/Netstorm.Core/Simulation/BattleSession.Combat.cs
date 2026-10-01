using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>발사 시 확정한 탄의 경로와 명중 예약. 이동하는 목표에도 예약된 피해를 적용하는 임시 모델이다.</summary>
public sealed record CombatShot(int AttackerId, int Owner, int TargetId, double Damage,
    double StartX, double StartY, double EndX, double EndY, long FiredTick, long ImpactTick, bool IsBeam = false);

/// <summary>포대의 자동 목표 선택·발사·피해·파괴를 고정 틱으로 처리한다. 근사값은 docs/gameplay/combat.md 참고.</summary>
public sealed partial class BattleSession
{
    /// <summary>명시적 발사 간격이 없는 포대의 임시 간격(초). 원본 애니메이션 구동 간격은 후속 분석한다.</summary>
    private const double FallbackShotSeconds = 1;

    /// <summary>원본 탄속 분석 전 공통으로 사용하는 임시 탄속(칸/초).</summary>
    private const double ProjectileSpeed = 24;

    /// <summary>신전이 있는 기절 사제의 임시 초당 회복량. 원본 회복식은 미확정이다.</summary>
    private const double PriestRecoveryPerSecond = 5;

    /// <summary>충돌 경로를 검사할 칸 단위 간격. 반 칸 간격으로 발자국을 검사한다.</summary>
    private const double ShotTraceStep = 0.5;

    /// <summary>Vander Tower 번개 표시 시간. 새 영상 01:20:00 부근의 약 0.2초 표시를 임시로 사용한다.</summary>
    public const double LightningDisplaySeconds = 0.2;

    /// <summary>발사 순서로 보관하는 비행 중 탄.</summary>
    private readonly List<CombatShot> _shots = [];

    /// <summary>피해 처리와 별도로 짧게 남기는 번개 표시 목록. 게임 규칙에는 영향을 주지 않는다.</summary>
    private readonly List<CombatShot> _lightning = [];

    /// <summary>화면의 탄 표시와 헤드리스 검사에 사용하는 읽기 전용 목록.</summary>
    public IReadOnlyList<CombatShot> Shots => _shots.AsReadOnly();

    /// <summary>최근 발사된 번개의 표시 경로.</summary>
    public IReadOnlyList<CombatShot> Lightning => _lightning.AsReadOnly();

    /// <summary>자동 전투를 켜거나 끈다. 미션은 기본 켜짐이며 정적 맵 뷰어는 끈다.</summary>
    public bool CombatEnabled { get; set; } = true;

    /// <summary>회복 → 도착한 탄 → 목표 탐색·발사 순서로 갱신한다.</summary>
    private void UpdateCombat()
    {
        if (!CombatEnabled) return;
        _lightning.RemoveAll(shot => Tick >= shot.FiredTick + TicksFor(LightningDisplaySeconds));
        // 기절한 사제는 자기 신전이 완공되어 있을 때만 회복한다.
        foreach (GameEntity priest in _entities.Values.Where(e => e.IsStunned))
        {
            if (!_entities.Values.Any(e => e.Owner == priest.Owner && e.Kind == ObjectKind.Temple && e.IsComplete)) continue;
            priest.HitPoints = Math.Min(priest.MaxHitPoints, priest.HitPoints + PriestRecoveryPerSecond / TicksPerSecond);
            if (priest.HitPoints >= priest.MaxHitPoints)
            {
                priest.IsStunned = false;
                Emit(SessionEventKind.PriestRecovered, priest.Owner, priest.Id, $"{priest.DisplayName} 회복");
            }
        }
        // 같은 틱에 맞는 탄도 발사 순서로 적용한다. 먼저 파괴된 목표에 보상을 중복 지급하지 않는다.
        foreach (CombatShot shot in _shots.Where(s => s.ImpactTick <= Tick).ToArray())
        {
            _shots.Remove(shot);
            if (Entity(shot.TargetId) is { IsStunned: false } target)
            {
                ApplyCombatDamage(target, shot);
            }
        }
        UpdateFlyers();
        // 오브젝트 번호순으로 목표를 선택해 동일한 초기 상태에서 같은 결과를 얻는다.
        foreach (GameEntity attacker in _entities.Values)
        {
            TypeDefinition type = attacker.Type.Definition;
            string? group = type.GetString("group");
            if (!attacker.IsComplete || attacker.Owner <= 0 || attacker.HitPoints <= 0 ||
                !(string.Equals(group, "cannon", StringComparison.OrdinalIgnoreCase) ||
                  string.Equals(group, "archer", StringComparison.OrdinalIgnoreCase))) continue;
            double range = type.GetDouble("range") ?? 0;
            double rate = type.GetDouble("hpPerSec") ?? 0;
            if (range <= 0 || rate <= 0) continue;
            GameEntity? target = Entity(attacker.AttackTargetId);
            if (target == null || !CanShoot(attacker, target, range))
            {
                target = _entities.Values.Where(e => CanShoot(attacker, e, range))
                    .OrderBy(e => DistanceSquared(attacker, e)).ThenBy(e => e.Id).FirstOrDefault();
                attacker.AttackTargetId = target?.Id ?? 0;
            }
            if (target == null || Tick < attacker.NextAttackTick) continue;
            double interval = type.GetDouble("delayBetweenShots") ?? FallbackShotSeconds;
            if (!double.IsFinite(interval) || interval <= 0) interval = FallbackShotSeconds;
            bool airborne = target.Type.Definition.HasFlag("balloon") || target.Kind == ObjectKind.Flyer;
            double damageRate = airborne && type.GetInt("useairdamage") == 1
                ? type.GetDouble("airdamage") ?? rate : rate;
            if (damageRate <= 0) continue;
            bool beam = attacker.Type.Name.Equals("thunderArcher", StringComparison.OrdinalIgnoreCase);
            // Vander Tower는 영상에서 목표까지 한 번에 이어지는 번개다. 피해의 원본 프레임은 미확정이라 다음 틱에 적용한다.
            long travel = beam ? 1 : TicksFor(Math.Sqrt(DistanceSquared(attacker, target)) / ProjectileSpeed);
            var shot = new CombatShot(attacker.Id, attacker.Owner, target.Id, damageRate * interval,
                attacker.WorldX, attacker.WorldY, target.WorldX, target.WorldY,
                Tick, Tick + travel, beam);
            _shots.Add(shot);
            if (beam) _lightning.Add(shot);
            attacker.NextAttackTick = Tick + TicksFor(interval);
            Emit(SessionEventKind.ShotFired, attacker.Owner, attacker.Id, $"{attacker.DisplayName} → {target.DisplayName} 발사");
        }
    }

    /// <summary>적·사거리·기절·직선 포대와 차폐를 검사한다. 다른 포대의 방향각은 후속 분석 대상이다.</summary>
    private bool CanShoot(GameEntity attacker, GameEntity target, double range)
    {
        if (target.Owner <= 0 || Map.AreAllied(attacker.Owner, target.Owner) || target.HitPoints <= 0 || target.IsStunned ||
            DistanceSquared(attacker, target) > range * range) return false;
        bool axisOnly = attacker.Type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase) ||
            attacker.Type.Name.Equals("thunderCannon", StringComparison.OrdinalIgnoreCase);
        if (axisOnly && !(attacker.Footprint.CenterX >= target.Footprint.Left && attacker.Footprint.CenterX <= target.Footprint.AnchorX) &&
            !(attacker.Footprint.CenterY >= target.Footprint.Top && attacker.Footprint.CenterY <= target.Footprint.AnchorY)) return false;
        bool airborne = target.Type.Definition.HasFlag("balloon") || target.Kind == ObjectKind.Flyer;
        if (airborne && attacker.Type.Definition.GetInt("useairdamage") == 1 &&
            attacker.Type.Definition.GetDouble("airdamage") == 0) return false;
        double distance = Math.Sqrt(DistanceSquared(attacker, target));
        // shotblocking 플래그를 가진 건물은 아군·적군 모두 사선을 막는다. 발사자와 목표는 제외한다.
        foreach (GameEntity obstacle in _entities.Values.Where(e => e.Id != attacker.Id && e.Id != target.Id && e.Type.Definition.HasFlag("shotblocking")))
        {
            // 발사점에서 목표까지 반 칸 간격으로 검사한다. 원본 픽셀 단위 충돌은 후속 구현이다.
            for (double travelled = ShotTraceStep; travelled < distance; travelled += ShotTraceStep)
            {
                double fraction = travelled / distance;
                double x = attacker.WorldX + (target.WorldX - attacker.WorldX) * fraction;
                double y = attacker.WorldY + (target.WorldY - attacker.WorldY) * fraction;
                if (x >= obstacle.Footprint.Left - 0.5 && x <= obstacle.Footprint.AnchorX + 0.5 &&
                    y >= obstacle.Footprint.Top - 0.5 && y <= obstacle.Footprint.AnchorY + 0.5) return false;
            }
        }
        return true;
    }

    /// <summary>논리 칸 좌표의 발자국 중심 거리 제곱. 화면의 세로 압축 비율과 무관하다.</summary>
    private static double DistanceSquared(GameEntity first, GameEntity second)
    {
        double dx = first.WorldX - second.WorldX;
        double dy = first.WorldY - second.WorldY;
        return dx * dx + dy * dy;
    }

    /// <summary>피해를 적용하며 사제는 죽이는 대신 절반 체력에서 기절시킨다.</summary>
    private void ApplyCombatDamage(GameEntity target, CombatShot shot)
    {
        target.HitPoints = Math.Max(0, target.HitPoints - shot.Damage);
        Emit(SessionEventKind.EntityDamaged, target.Owner, target.Id, $"{target.DisplayName} 피해 {shot.Damage:0.#}");
        if (target.Kind == ObjectKind.Priest && target.HitPoints <= target.MaxHitPoints / 2)
        {
            target.HitPoints = target.MaxHitPoints / 2;
            target.IsStunned = true;
            _harvestTasks.Remove(target.Id);
            Emit(SessionEventKind.PriestStunned, target.Owner, target.Id, $"{target.DisplayName} 기절 (보호막)");
        }
        else if (target.HitPoints <= 0)
        {
            RemoveEntity(target);
            int reward = StormPower.KillReward(target.Cost, Map.Options.KillRewardPercent);
            if (_players.TryGetValue(shot.Owner, out PlayerState? player)) player.StormPower += reward;
            Emit(SessionEventKind.EntityDestroyed, shot.Owner, target.Id, $"{target.DisplayName} 파괴 (+{reward})");
        }
    }

    /// <summary>회수·파괴 공통 정리: 점유·공급·소유권·생산·수집·선택을 함께 제거한다.</summary>
    private void RemoveEntity(GameEntity entity)
    {
        // 공중 공격체는 지상 점유를 등록하지 않으므로 아래 건물의 점유를 해제해서는 안 된다.
        if (entity.Kind != ObjectKind.Flyer) Map.RemoveOccupant(entity.Footprint);
        Map.RemoveSource(entity.Id);
        _players.TryGetValue(entity.Owner, out PlayerState? player);
        if (entity.Kind == ObjectKind.Temple)
        {
            if (entity.Territory is int territory && Map.Ownership.OwnerOf(territory) == entity.Owner)
                Map.Ownership.RemoveTemple(territory);
            player?.Deck.RemoveTemple();
        }
        else if (entity.Kind == ObjectKind.Workshop) player?.Deck.RemoveWorkshop(entity.Id);
        _entities.Remove(entity.Id);
        _harvestTasks.Remove(entity.Id);
        if (entity.Kind != ObjectKind.Flyer) WeakenBridgesAround(entity);
        if (entity.Flight is { } flight && Entity(flight.BaseId) is { } home)
            home.NextAttackTick = Tick + TicksFor(WhirligigBuildSeconds);
        // 사라진 오브젝트를 가리키는 선택과 수집 예약을 해제한다.
        foreach (PlayerState viewer in _players.Values.Where(p => p.SelectedEntityId == entity.Id)) ClearSelection(viewer);
        // 신전·가이저가 제거되면 그 대상을 사용하던 작업도 즉시 해제한다.
        foreach (PriestHarvestTask task in _harvestTasks.Values.Where(t => t.TempleId == entity.Id || t.GeyserId == entity.Id).ToArray())
            _harvestTasks.Remove(task.PriestId);
    }

    /// <summary>앞으로의 피해를 결정하는 비행 중 탄과 전투 활성 상태를 검사합에 포함한다.</summary>
    private void AddCombatChecksum(Fnv1a hash)
    {
        hash.Add(CombatEnabled ? 1 : 0);
        // 발사 순서까지 상태이므로 목록 순서를 그대로 사용한다.
        foreach (CombatShot shot in _shots)
        {
            hash.Add(shot.AttackerId);
            hash.Add(shot.Owner);
            hash.Add(shot.TargetId);
            hash.Add(BitConverter.DoubleToInt64Bits(shot.Damage));
            hash.Add(shot.FiredTick);
            hash.Add(shot.ImpactTick);
            hash.Add(shot.IsBeam ? 1 : 0);
        }
    }
}
