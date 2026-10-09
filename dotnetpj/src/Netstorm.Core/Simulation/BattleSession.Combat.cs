using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>발사 시 정한 탄 경로와 피해. 캐논은 착탄점에서 충돌을 검사하며 방어선에 흡수된 탄은 목표 번호가 0이다.</summary>
public sealed record CombatShot(int AttackerId, int Owner, int TargetId, double Damage,
    double StartX, double StartY, double EndX, double EndY, long FiredTick, long ImpactTick, bool IsBeam = false,
    string? AttackerType = null, int BlockedByFenceId = 0);

/// <summary>포대의 자동 목표 선택·발사·피해·파괴를 고정 틱으로 처리한다. 근사값은 docs/gameplay/combat.md 참고.</summary>
public sealed partial class BattleSession
{
    /// <summary>명시적 발사 간격이 없는 포대의 임시 간격(초). 원본 애니메이션 구동 간격은 후속 분석한다.</summary>
    private const double FallbackShotSeconds = 1;

    /// <summary>원반·석궁의 원본 탄속(FUN_004680e0, VA 0x50b168)은 초당 35칸이다.</summary>
    private const double ArcherProjectileSpeed = 35;

    /// <summary>세 캐논의 원본 탄속(FUN_00468640, 0x501a54)은 초당 20칸이다.</summary>
    private const double CannonProjectileSpeed = 20;

    /// <summary>신전이 있는 기절 사제의 임시 초당 회복량. 원본 회복식은 미확정이다.</summary>
    private const double PriestRecoveryPerSecond = 5;

    /// <summary>충돌 경로를 검사할 칸 단위 간격. 반 칸 간격으로 발자국을 검사한다.</summary>
    private const double ShotTraceStep = 0.5;

    /// <summary>
    /// 템플(타입 플래그 vortex)이 파괴될 때 주위 칸에 주는 고정 피해. exe FUN_0044b9e0: 타입 플래그2 0x200(vortex, Rifttype.cpp)이면 400.
    /// 골렘(50 HP)은 한 번에 파괴되고 사제(100 HP)는 바로 기절한다(사용자 설명 2026-10-01과 일치).
    /// </summary>
    public const double TempleExplosionDamage = 400;

    /// <summary>파괴 폭발이 미치는 범위: 발자국 바깥 칸 수. exe 는 발자국 크기/2+1 을 중심에서 잰다(= 발자국 + 1칸).</summary>
    public const int ExplosionRadiusCells = 1;

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
        UpdateRegrowingTowers();
        if (!CombatEnabled) return;
        _impacts.RemoveAll(impact => Tick >= impact.Tick + TicksFor(ImpactLifetimeSeconds));
        IReadOnlyList<SunForceField> fields = SunForceFields;
        _lightning.RemoveAll(shot => Tick >= shot.FiredTick + TicksFor(LightningDisplaySeconds));
        // 기절한 사제는 자기 신전이 완공되어 있을 때만 회복한다. 포획된 사제(운반·묶임)는 풀려날 때 회복하므로 여기서 제외한다.
        foreach (GameEntity priest in _entities.Values.Where(e => e.IsStunned && !e.IsSuspended && e.Captivity == PriestCaptivity.Free &&
            HasGroundSupport(e.Footprint.AnchorX, e.Footprint.AnchorY)))
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
        foreach (CombatShot shot in _shots.ToArray())
        {
            if (AbsorbMovingShot(shot, fields) || shot.ImpactTick > Tick) continue;
            _shots.Remove(shot);
            if (shot.BlockedByFenceId != 0)
            {
                RecordShotImpact(shot);
                Emit(SessionEventKind.ShotBlocked, Entity(shot.BlockedByFenceId)?.Owner ?? 0, shot.BlockedByFenceId, "썬 바리케이트 탄 흡수");
                continue;
            }
            if (Entity(shot.TargetId) is { IsStunned: false, IsRegenerating: false } target &&
                (shot.IsBeam || ShotHitsFootprint(shot, target)))
            {
                RecordShotImpact(shot);
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
            if (attacker.Type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase))
            {
                int? direction = target == null ? null : AimDirection(attacker, target);
                if (direction.HasValue) attacker.CannonDirection = direction.Value;
                int previousFrame = attacker.SunCannonFrame;
                bool ready = SunCannonAnimation.Advance(attacker, direction, Tick, TicksPerSecond);
                // 효과음 선택은 그림 번호에 고정해 다리 추첨 등에 쓰는 세션 난수를 소비하지 않는다.
                if (attacker.SunCannonFrame != previousFrame && previousFrame < 24 && direction.HasValue)
                    Emit(SessionEventKind.CombatSound, attacker.Owner, attacker.Id,
                        $"sunCannonRaiseLower{attacker.SunCannonFrame % 9 + 1}.wav");
                if (!ready) continue;
            }
            if (attacker.Type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase))
            {
                int previousOrientation = attacker.CrossbowFrame / 5;
                int? orientation = target == null ? null : CrossbowAnimation.Orientation(
                    target.WorldX - attacker.WorldX, target.WorldY - attacker.WorldY);
                bool release = CrossbowAnimation.Advance(attacker, orientation, Tick >= attacker.NextAttackTick, Tick, TicksPerSecond);
                if (attacker.CrossbowFrame / 5 != previousOrientation)
                    Emit(SessionEventKind.CombatSound, attacker.Owner, attacker.Id,
                        $"sunDiscThrowerRatchet{attacker.CrossbowFrame / 5 % 9 + 1:00}.wav");
                if (!release) continue;
            }
            if (target == null)
            {
                attacker.AttackStartedTick = -1;
                continue;
            }
            if (Tick < attacker.NextAttackTick) continue;
            double interval = type.GetDouble("delayBetweenShots") ?? FallbackShotSeconds;
            if (!double.IsFinite(interval) || interval <= 0) interval = FallbackShotSeconds;
            if (attacker.Type.Name.Equals("thunderCannon", StringComparison.OrdinalIgnoreCase))
            {
                if (attacker.AttackStartedTick < 0) attacker.AttackStartedTick = Tick;
                if (Tick < attacker.AttackStartedTick + TicksFor(CannonAnimation.ThunderChargeSeconds)) continue;
                interval = CannonAnimation.ThunderChargeSeconds + CannonAnimation.ThunderRecoverySeconds;
            }
            else if (attacker.Type.Name.Equals("rainCannon", StringComparison.OrdinalIgnoreCase))
            {
                if (attacker.AttackStartedTick < 0) attacker.AttackStartedTick = Tick;
                double elapsed = (Tick - attacker.AttackStartedTick) / (double)TicksPerSecond;
                if (elapsed < CannonAnimation.IceFrameSeconds || elapsed % (CannonAnimation.IceBurstSeconds + CannonAnimation.IceRestSeconds)
                    >= CannonAnimation.IceBurstSeconds) continue;
                interval = CannonAnimation.IceShotSeconds;
            }
            bool airborne = target.Type.Definition.HasFlag("balloon") || target.Kind == ObjectKind.Flyer;
            double damageRate = airborne && type.GetInt("useairdamage") == 1
                ? type.GetDouble("airdamage") ?? rate : rate;
            if (damageRate <= 0) continue;
            bool beam = attacker.Type.Name.Equals("thunderArcher", StringComparison.OrdinalIgnoreCase);
            // Vander Tower는 영상에서 목표까지 한 번에 이어지는 번개다. 피해의 원본 프레임은 미확정이라 다음 틱에 적용한다.
            (double endX, double endY) = ShotAim(attacker, target);
            int fenceId = 0;
            if (FirstForceFieldHit(attacker.Owner, attacker.WorldX, attacker.WorldY, endX, endY, fields) is { } intercepted)
            {
                endX = attacker.WorldX + (endX - attacker.WorldX) * intercepted.Fraction;
                endY = attacker.WorldY + (endY - attacker.WorldY) * intercepted.Fraction;
                fenceId = intercepted.Field.FirstId;
            }
            double dx = endX - attacker.WorldX, dy = endY - attacker.WorldY;
            double speed = CannonAnimation.IsCannon(attacker.Type) ? CannonProjectileSpeed : ArcherProjectileSpeed;
            long travel = beam ? 1 : TicksFor(Math.Sqrt(dx * dx + dy * dy) / speed);
            var shot = new CombatShot(attacker.Id, attacker.Owner, target.Id, damageRate * interval,
                attacker.WorldX, attacker.WorldY, endX, endY,
                Tick, Tick + travel, beam, attacker.Type.Name, fenceId);
            if (fenceId != 0) shot = shot with { TargetId = 0 };
            _shots.Add(shot);
            if (beam) _lightning.Add(shot);
            attacker.LastShotTick = Tick;
            if (attacker.Type.Name.Equals("sunCannon", StringComparison.OrdinalIgnoreCase))
                SunCannonAnimation.Fire(attacker, Tick, TicksPerSecond);
            attacker.NextAttackTick = Tick + TicksFor(attacker.Type.Name.Equals("thunderCannon", StringComparison.OrdinalIgnoreCase)
                ? CannonAnimation.ThunderRecoverySeconds : interval);
            // 석궁은 타입 주석처럼 장전 그림 자체가 발사 간격을 정한다. 별도의 1초 대기를 더하지 않는다.
            if (attacker.Type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase)) attacker.NextAttackTick = Tick + 1;
            if (attacker.Type.Name.Equals("thunderCannon", StringComparison.OrdinalIgnoreCase)) attacker.AttackStartedTick = -1;
            Emit(SessionEventKind.ShotFired, attacker.Owner, attacker.Id, $"{attacker.DisplayName} → {target.DisplayName} 발사");
        }
    }

    /// <summary>적·사거리·기절·캐논 직선·Crossbow 60도 사격각과 차폐를 검사한다.</summary>
    private bool CanShoot(GameEntity attacker, GameEntity target, double range)
    {
        if (target.Owner <= 0 || Map.AreAllied(attacker.Owner, target.Owner) || target.HitPoints <= 0 || target.IsStunned ||
            DistanceSquared(attacker, target) > range * range) return false;
        if (CombatImmunity.Blocks(target, attacker.Type, attacker.WorldX, attacker.WorldY)) return false;
        bool axisOnly = CannonAnimation.IsCannon(attacker.Type);
        if (axisOnly && !(attacker.Footprint.CenterX >= target.Footprint.Left && attacker.Footprint.CenterX <= target.Footprint.AnchorX) &&
            !(attacker.Footprint.CenterY >= target.Footprint.Top && attacker.Footprint.CenterY <= target.Footprint.AnchorY)) return false;
        if (CannonAnimation.IsFixed(attacker.Type) && AimDirection(attacker, target) != attacker.CannonDirection) return false;
        if (attacker.Type.Name.Equals("windArcher", StringComparison.OrdinalIgnoreCase)
            && !EmplacementDirection.InsideCrossbowSector(attacker.CannonDirection,
                target.WorldX - attacker.WorldX, target.WorldY - attacker.WorldY)) return false;
        bool airborne = target.Type.Definition.HasFlag("balloon") || target.Kind == ObjectKind.Flyer;
        if (airborne && attacker.Type.Definition.GetInt("useairdamage") == 1 &&
            attacker.Type.Definition.GetDouble("airdamage") == 0) return false;
        double distance = Math.Sqrt(DistanceSquared(attacker, target));
        // shotblocking 플래그를 가진 건물은 아군·적군 모두 사선을 막는다. 발사자와 목표는 제외한다.
        (double endX, double endY) = ShotAim(attacker, target);
        foreach (GameEntity obstacle in _entities.Values.Where(e => e.Id != attacker.Id && e.Id != target.Id && !e.IsRegenerating && e.Production == null &&
            (e.Type.Definition.HasFlag("shotblocking") || !airborne && e.Type.Definition.GetString("group")?.Equals("blocker", StringComparison.OrdinalIgnoreCase) == true)))
        {
            // 발사점에서 목표까지 반 칸 간격으로 검사한다. 원본 픽셀 단위 충돌은 후속 구현이다.
            for (double travelled = ShotTraceStep; travelled < distance; travelled += ShotTraceStep)
            {
                double fraction = travelled / distance;
                double x = attacker.WorldX + (endX - attacker.WorldX) * fraction;
                double y = attacker.WorldY + (endY - attacker.WorldY) * fraction;
                if (x >= obstacle.Footprint.Left - 0.5 && x <= obstacle.Footprint.AnchorX + 0.5 &&
                    y >= obstacle.Footprint.Top - 0.5 && y <= obstacle.Footprint.AnchorY + 0.5) return false;
            }
        }
        return true;
    }

    /// <summary>발자국에 닿는 직선 캐논의 방위를 고른다. 넓은 목표의 중심을 향해 비스듬히 쏘지 않는다.</summary>
    private static int AimDirection(GameEntity attacker, GameEntity target) =>
        attacker.WorldX >= target.Footprint.Left && attacker.WorldX <= target.Footprint.AnchorX
            ? target.WorldY < attacker.WorldY ? 0 : 2 : target.WorldX < attacker.WorldX ? 3 : 1;

    /// <summary>캐논은 정해진 행·열로 조준하고 다른 포대는 목표 중심으로 조준한다.</summary>
    private static (double X, double Y) ShotAim(GameEntity attacker, GameEntity target) =>
        !CannonAnimation.IsCannon(attacker.Type) ? (target.WorldX, target.WorldY)
        : AimDirection(attacker, target) is 0 or 2 ? (attacker.WorldX, target.WorldY) : (target.WorldX, attacker.WorldY);

    /// <summary>현재 위치의 목표가 고정 착탄점에 남아 있는지 검사해 이동으로 피한 탄이 강제로 명중하지 않게 한다.</summary>
    private bool ShotHitsFootprint(CombatShot shot, GameEntity target)
    {
        (double x, double y) = MovementCell(target, 0);
        double offsetX = target.Flight?.X - target.Footprint.CenterX ?? x - target.Footprint.AnchorX;
        double offsetY = target.Flight?.Y - target.Footprint.CenterY ?? y - target.Footprint.AnchorY;
        return shot.EndX >= target.Footprint.Left + offsetX - 0.5 && shot.EndX <= target.Footprint.AnchorX + offsetX + 0.5 &&
            shot.EndY >= target.Footprint.Top + offsetY - 0.5 && shot.EndY <= target.Footprint.AnchorY + offsetY + 0.5;
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
        // 아직 실체가 없는 생산 예약은 폭발·이미 날아오는 탄에도 피해를 받지 않는다.
        if (target.Production != null) return;
        // 발사자가 사라져도 탄의 타입·출발점으로 면역을 유지한다. 타입 없는 폭발은 방위 면역을 우회한다.
        TypeInfo? attackerType = shot.AttackerType == null ? null : _types.Find(shot.AttackerType);
        GameEntity? attacker = Entity(shot.AttackerId);
        if (CombatImmunity.Blocks(target, attackerType, attacker?.WorldX ?? shot.StartX, attacker?.WorldY ?? shot.StartY)) return;
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
            _impacts.Add(new CombatImpact(target.WorldX, target.WorldY, Tick, target.Kind == ObjectKind.Temple ? 'A' : 'B'));
            if (target.Type.Name.Equals("rainBlocker", StringComparison.OrdinalIgnoreCase))
            {
                ShatterIceTower(target);
                return;
            }
            RemoveEntity(target);
            // 자기·동맹 오브젝트가 폭발에 휘말려 파괴된 경우에는 보상을 주지 않는다 (원본 보상 분기의 소유자 조건은 미확인, 임시).
            bool enemy = shot.Owner > 0 && !Map.AreAllied(shot.Owner, target.Owner);
            int reward = enemy ? StormPower.KillReward(target.Cost, Map.Options.KillRewardPercent) : 0;
            if (_players.TryGetValue(shot.Owner, out PlayerState? player)) player.StormPower += reward;
            Emit(SessionEventKind.EntityDestroyed, shot.Owner, target.Id, $"{target.DisplayName} 파괴 (+{reward})");
            ExplodeDestroyed(target, shot.Owner);
        }
    }

    /// <summary>
    /// 파괴 시 폭발 피해량 (없으면 null). exe FUN_0044b9e0: group 이 archer(0)·cannon(1)이면 최대 체력/2+1, 템플(vortex)이면 400.
    /// group 번호는 exe 이름 표 VA 0x542468(archer, cannon, blocker, aviary, flyer, battery, fence, walker, balloon) 순서다.
    /// </summary>
    private static double? ExplosionDamageOf(GameEntity destroyed)
    {
        TypeDefinition type = destroyed.Type.Definition;
        if (type.HasFlag("vortex")) return TempleExplosionDamage;
        string? group = type.GetString("group");
        bool gun = string.Equals(group, "archer", StringComparison.OrdinalIgnoreCase) ||
            string.Equals(group, "cannon", StringComparison.OrdinalIgnoreCase);
        return gun ? Math.Floor(destroyed.MaxHitPoints / 2) + 1 : null;
    }

    /// <summary>
    /// 전투로 파괴된 템플·포대가 발자국 주위 1칸의 지상 오브젝트에 피해를 준다(사용자 설명 2026-10-01, exe FUN_0044b9e0).
    /// 피해는 처치한 플레이어 이름으로 들어가 연쇄 파괴의 보상도 그 플레이어가 받는다. 소유자 구분 없이 범위 안 모두에 적용하며,
    /// 연쇄 폭발은 오브젝트 번호순으로 같은 틱에 처리한다. 판매·낙하는 이 경로를 거치지 않아 폭발하지 않는다.
    /// </summary>
    /// <param name="destroyed">방금 파괴된 오브젝트 (이미 제거됨)</param>
    /// <param name="killer">처치한 플레이어 (보상 수령자)</param>
    private void ExplodeDestroyed(GameEntity destroyed, int killer)
    {
        if (ExplosionDamageOf(destroyed) is not double damage) return;
        Footprint f = destroyed.Footprint;
        var area = new Footprint(f.AnchorX + ExplosionRadiusCells, f.AnchorY + ExplosionRadiusCells,
            f.Width + 2 * ExplosionRadiusCells, f.Height + 2 * ExplosionRadiusCells);
        Emit(SessionEventKind.EntityExploded, killer, destroyed.Id, $"{destroyed.DisplayName} 폭발 (주위 피해 {damage:0})");
        // 범위 안의 피해 가능한 지상 오브젝트를 번호순으로 고정해 둔 뒤 차례로 피해를 준다 (연쇄 폭발로 사라진 것은 건너뛴다)
        int[] victims = [.. _entities.Values
            .Where(e => e.MaxHitPoints > 0 && e.HitPoints > 0 && e.OccupiesGround && e.Captivity == PriestCaptivity.Free
                && e.Footprint.Overlaps(area))
            .Select(e => e.Id)];
        foreach (int id in victims)
        {
            if (Entity(id) is not { IsStunned: false } victim) continue;
            ApplyCombatDamage(victim, new CombatShot(destroyed.Id, killer, id, damage,
                destroyed.WorldX, destroyed.WorldY, victim.WorldX, victim.WorldY, Tick, Tick));
        }
    }

    /// <summary>회수·파괴 공통 정리: 점유·공급·소유권·생산·수집·선택을 함께 제거한다.</summary>
    private void RemoveEntity(GameEntity entity)
    {
        // 운반 중이거나 제단에 묶인 사제를 먼저 풀어 준다 (도움말: 운반 유닛·제단이 파괴되면 사제가 풀려난다)
        ReleaseCaptivesOf(entity);
        // 공중 공격체는 지상 점유를 등록하지 않으므로 아래 건물의 점유를 해제해서는 안 된다.
        if (entity.OccupiesGround) Map.RemoveOccupant(entity.Footprint);
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
        Map.Sids.ReleaseWorld(entity.Id);
        if (entity.Type.Definition.HasFlag("createsisland")) Bridges.InvalidateTerrain();
        _harvestTasks.Remove(entity.Id);
        if (entity.Kind != ObjectKind.Flyer && entity.Production == null) WeakenBridgesAround(entity);
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
            hash.Add(shot.AttackerType ?? "");
            hash.Add(shot.BlockedByFenceId);
            hash.Add(BitConverter.DoubleToInt64Bits(shot.StartX));
            hash.Add(BitConverter.DoubleToInt64Bits(shot.StartY));
            hash.Add(BitConverter.DoubleToInt64Bits(shot.EndX));
            hash.Add(BitConverter.DoubleToInt64Bits(shot.EndY));
        }
    }
}
