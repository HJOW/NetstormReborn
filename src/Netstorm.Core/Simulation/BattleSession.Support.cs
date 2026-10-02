using Netstorm.Core.Bridges;
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
                // 규칙상으로는 즉시 제거한다(적 처치 보상 없음). 원본의 약 1.8초 낙하 연출은 화면이 낙하 기록으로 그린다.
                RecordFallen(new FallenObject(Tick, entity, null));
                RemoveEntity(entity);
                Emit(SessionEventKind.UnitFell, entity.Owner, entity.Id, $"{entity.DisplayName} 발판 소멸로 낙하");
            }
        }
    }

    /// <summary>낙하 기록의 최대 보관 수 (화면이 꺼내 가지 않는 헤드리스 실행에서 무한히 쌓이지 않게 한다)</summary>
    private const int FallenRecordLimit = 256;

    /// <summary>화면 낙하 연출용 기록 (규칙·검사합과 무관)</summary>
    private readonly List<FallenObject> _fallen = [];

    /// <summary>
    /// 마지막으로 꺼낸 뒤 발판을 잃고 떨어진 지상 이동체와 무너진 다리 칸을 꺼내고 비운다.
    /// 원본은 떨어지는 유닛·다리 조각을 아래로 미끄러지듯 떨어뜨린 뒤 지운다(2026-10-01 캠페인 1-2 녹화). 화면 연출 전용이다.
    /// </summary>
    public IReadOnlyList<FallenObject> DrainFallen()
    {
        FallenObject[] fallen = [.. _fallen];
        _fallen.Clear();
        return fallen;
    }

    /// <summary>낙하 기록 하나를 남긴다. 보관 수를 넘으면 가장 오래된 기록을 버린다.</summary>
    private void RecordFallen(FallenObject fallen)
    {
        if (_fallen.Count >= FallenRecordLimit) _fallen.RemoveAt(0);
        _fallen.Add(fallen);
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

/// <summary>화면 낙하 연출용 기록 하나 (떨어진 이동체 또는 무너진 다리 칸 중 하나)</summary>
/// <param name="Tick">떨어진 틱</param>
/// <param name="Entity">발판을 잃고 제거된 지상 이동체 (다리 칸이면 null). 제거 뒤에도 마지막 위치·타입을 가진다.</param>
/// <param name="Cell">무너진 다리 칸 (이동체면 null)</param>
public sealed record FallenObject(long Tick, GameEntity? Entity, BridgeCellState? Cell);
