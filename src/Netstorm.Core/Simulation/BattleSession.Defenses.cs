using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>동맹인 썬 바리케이트 기둥 사이의 직선 방어선. 좌표는 논리 칸 중심이다.</summary>
public sealed record SunForceField(int FirstId, int SecondId, int Owner, double StartX, double StartY, double EndX, double EndY);

/// <summary>아이스 타워 재성장과 썬 바리케이트의 연결·탄 흡수 규칙.</summary>
public sealed partial class BattleSession
{
    /// <summary>원본 재성장 프레임 간격의 분자(FUN_0048ead0, 0x5069bc). 도움말 프레임도 분모에 들어간다.</summary>
    public const double IceGrowthPeriodSeconds = 40;

    /// <summary>현재 기둥 배치에서 만들어지는 방어선. 적 기둥도 가장 가까운 후보에 포함해 동맹 선을 끊는다.</summary>
    public IReadOnlyList<SunForceField> SunForceFields => BuildSunForceFields();

    /// <summary>도움말을 제외한 재성장 프레임 수. 중복 이름 P00을 포함해 저장 순서를 유지한다.</summary>
    private int IceGrowthFrameCount => Math.Max(1, (_types.Find("growingRainBlocker")?.Definition.Clusters.Count ?? 15) - 1);

    /// <summary>현재 성장 그림의 저장 순서. 새로 자라는 얼음은 체력이 없어 일반 탄에 다시 파괴되지 않는다.</summary>
    public int IceGrowthFrame(GameEntity entity) => Math.Clamp(
        (Tick == entity.RegenerationStartTick ? 0 : 1) +
        (int)((Tick - entity.RegenerationStartTick) * (IceGrowthFrameCount + 1) / (TicksPerSecond * IceGrowthPeriodSeconds)),
        0, IceGrowthFrameCount - 1);

    /// <summary>마지막 성장 그림 다음에 최대 체력 타워로 돌아가는 틱.</summary>
    public long IceRegrowthTick(GameEntity entity) => entity.RegenerationStartTick +
        // 원본 Periodic.cpp의 첫 실행 시각은 현재 시각이다(00496ee7). 따라서 첫 성장 간격은 기다리지 않는다.
        TicksFor(IceGrowthPeriodSeconds * (IceGrowthFrameCount - 1) / (IceGrowthFrameCount + 1));

    /// <summary>받침·점유·소유자를 남긴 채 얼음만 재성장 상태로 바꾼다. 처치 보상과 주변 다리 약화는 없다.</summary>
    private void ShatterIceTower(GameEntity entity)
    {
        entity.IsRegenerating = true;
        entity.RegenerationStartTick = Tick;
        entity.AttackTargetId = 0;
        Emit(SessionEventKind.IceTowerShattered, entity.Owner, entity.Id, $"{entity.DisplayName} 파괴·재성장 시작");
    }

    /// <summary>고정 틱으로 성장 완료를 처리해 화면 갱신 속도와 무관하게 같은 시점에 복원한다.</summary>
    private void UpdateRegrowingTowers()
    {
        // 재성장 타워의 정체성을 유지하므로 선택과 지면 점유가 끊기지 않는다.
        foreach (GameEntity entity in _entities.Values.Where(e => e.IsRegenerating && Tick >= IceRegrowthTick(e)))
        {
            entity.IsRegenerating = false;
            entity.HitPoints = entity.MaxHitPoints;
            Emit(SessionEventKind.IceTowerRegrown, entity.Owner, entity.Id, $"{entity.DisplayName} 재성장 완료");
        }
    }

    /// <summary>행·열마다 가장 가까운 기둥만 연결한다(원본 FUN_00455fb0). 동·남 방향만 순회해 중복을 없앤다.</summary>
    private List<SunForceField> BuildSunForceFields()
    {
        GameEntity[] posts = _entities.Values.Where(e => e.IsComplete && e.HitPoints > 0 && e.Owner > 0
            && e.Type.Name.Equals("sunFence", StringComparison.OrdinalIgnoreCase)).ToArray();
        var fields = new List<SunForceField>();
        // 원본은 가장 가까운 같은 타입을 찾은 뒤 동맹을 검사한다. 적을 처음부터 걸러서는 안 된다.
        foreach (GameEntity first in posts)
        {
            // 동쪽과 남쪽 각각 가장 가까운 기둥을 찾는다.
            for (int axis = 0; axis < 2; axis++)
            {
                GameEntity? second = posts.Where(e => axis == 0
                    ? e.Footprint.AnchorY == first.Footprint.AnchorY && e.Footprint.AnchorX > first.Footprint.AnchorX
                    : e.Footprint.AnchorX == first.Footprint.AnchorX && e.Footprint.AnchorY > first.Footprint.AnchorY)
                    .OrderBy(e => DistanceSquared(first, e)).ThenBy(e => e.Id).FirstOrDefault();
                if (second == null || !Map.AreAllied(first.Owner, second.Owner)) continue;
                double range = Math.Min(first.Type.Definition.GetDouble("range") ?? 0, second.Type.Definition.GetDouble("range") ?? 0);
                if (DistanceSquared(first, second) >= range * range) continue;
                fields.Add(new SunForceField(first.Id, second.Id, first.Owner, first.WorldX, first.WorldY, second.WorldX, second.WorldY));
            }
        }
        return fields;
    }

    /// <summary>탄 이동 구간과 적 방어선의 첫 교점을 반환한다. 선을 따라가는 탄과 끝점 직격은 흡수하지 않는다.</summary>
    private (SunForceField Field, double Fraction)? FirstForceFieldHit(int owner, double x, double y, double endX, double endY,
        IReadOnlyList<SunForceField> fields)
    {
        (SunForceField Field, double Fraction)? hit = null;
        double dx = endX - x, dy = endY - y;
        // 여러 선을 가로지를 때도 발사점에서 가장 가까운 선 하나에만 맞힌다.
        foreach (SunForceField field in fields)
        {
            if (Map.AreAllied(owner, field.Owner)) continue;
            double fx = field.EndX - field.StartX, fy = field.EndY - field.StartY;
            double cross = dx * fy - dy * fx;
            if (Math.Abs(cross) < 1e-9) continue;
            double ax = field.StartX - x, ay = field.StartY - y;
            double fraction = (ax * fy - ay * fx) / cross;
            double along = (ax * dy - ay * dx) / cross;
            if (fraction < 0 || fraction > 1 || along <= 0 || along >= 1) continue;
            if (hit == null || fraction < hit.Value.Fraction) hit = (field, fraction);
        }
        return hit;
    }

    /// <summary>탄이 전진한 구간만 검사해 도중에 만들어진 적 방어선에도 흡수되게 한다.</summary>
    private bool AbsorbMovingShot(CombatShot shot, IReadOnlyList<SunForceField> fields)
    {
        if (shot.IsBeam || shot.BlockedByFenceId != 0) return false;
        double previous = Math.Clamp((Tick - 1 - shot.FiredTick) / (double)(shot.ImpactTick - shot.FiredTick), 0, 1);
        double current = Math.Clamp((Tick - shot.FiredTick) / (double)(shot.ImpactTick - shot.FiredTick), 0, 1);
        if (FirstForceFieldHit(shot.Owner, shot.StartX + (shot.EndX - shot.StartX) * previous,
            shot.StartY + (shot.EndY - shot.StartY) * previous, shot.StartX + (shot.EndX - shot.StartX) * current,
            shot.StartY + (shot.EndY - shot.StartY) * current, fields) is not { } hit) return false;
        _shots.Remove(shot);
        Emit(SessionEventKind.ShotBlocked, hit.Field.Owner, hit.Field.FirstId, "썬 바리케이트 탄 흡수");
        return true;
    }
}
