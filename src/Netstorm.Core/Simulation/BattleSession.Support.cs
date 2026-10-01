using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>다리·받침 소멸 후 지상 이동체의 낙하와 허공 사제의 기절·복귀를 처리한다.</summary>
public sealed partial class BattleSession
{
    /// <summary>소유와 무관하게 섬·받침 또는 다리가 있으면 기준점 칸을 지지한다.</summary>
    private bool HasGroundSupport(int x, int y) => Bridges.IsIsland(x, y) || Bridges.At(x, y) != null;

    /// <summary>이번 틱의 지형 변화가 끝난 뒤 지상 이동체를 번호순으로 검사한다.</summary>
    private void UpdateGroundSupport()
    {
        // 포획 사제는 운반체/제단이 지지하므로 자유 상태인 지상 이동체만 검사한다.
        foreach (GameEntity entity in _entities.Values.Where(e => e.Captivity == PriestCaptivity.Free &&
            (e.Kind == ObjectKind.Priest || e.Type.Definition.HasFlag("walker")) &&
            e.Kind != ObjectKind.Flyer && !IsAirborneTransport(e)).ToArray())
        {
            if (HasGroundSupport(entity.Footprint.AnchorX, entity.Footprint.AnchorY))
            {
                if (entity.Kind == ObjectKind.Priest && entity.IsSuspended)
                {
                    entity.IsSuspended = false;
                    Map.AddOccupant(entity.Footprint);
                    // 허공 기절의 복귀 경계는 사용자 설명의 '절반 이상'을 포함한다. 수확은 자동 재개하지 않는다.
                    if (entity.HitPoints >= entity.MaxHitPoints * 0.5)
                    {
                        entity.IsStunned = false;
                        Emit(SessionEventKind.PriestRecovered, entity.Owner, entity.Id, $"{entity.DisplayName} 발판 복구로 기절 회복");
                    }
                }
                continue;
            }
            if (entity.Kind == ObjectKind.Priest)
            {
                if (!entity.IsSuspended) SuspendPriest(entity, removeOccupancy: true);
            }
            else
            {
                // 낙하 시간·효과는 미확정이라 즉시 제거한다. 적 처치 보상은 지급하지 않는다.
                RemoveEntity(entity);
                Emit(SessionEventKind.UnitFell, entity.Owner, entity.Id, $"{entity.DisplayName} 발판 소멸로 낙하");
            }
        }
    }

    /// <summary>그 자리에서 사제를 허공 기절시킨다. 이미 점유에서 빠진 포로 해방은 중복 해제하지 않는다.</summary>
    private void SuspendPriest(GameEntity priest, bool removeOccupancy)
    {
        if (removeOccupancy) Map.RemoveOccupant(priest.Footprint);
        priest.IsSuspended = true;
        priest.IsStunned = true;
        _harvestTasks.Remove(priest.Id);
        _moveTasks.Remove(priest.Id);
        Emit(SessionEventKind.PriestSuspended, priest.Owner, priest.Id, $"{priest.DisplayName} 허공에서 기절");
    }
}
