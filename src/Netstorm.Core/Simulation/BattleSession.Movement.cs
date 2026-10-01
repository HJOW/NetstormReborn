using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>수확·수송·사제 이동이 공유하는 재탐색·대기·진행 상태.</summary>
public sealed partial class BattleSession
{
    /// <summary>오브젝트의 작업이 길 막힘으로 대기 중인지. 상태 표시와 헤드리스 검사에 쓴다.</summary>
    public bool IsMoveBlocked(int entityId) => _moveTasks.TryGetValue(entityId, out UnitMoveTask? move) && move.IsBlocked
        || _harvestTasks.TryGetValue(entityId, out PriestHarvestTask? harvest) && harvest.IsBlocked;

    /// <summary>지상 이동 직전 지지를 확인하고, 지형이 바뀐 때만 같은 목표로 재탐색한다.</summary>
    private bool PrepareMovement(GameEntity mover, MovementRoute route, Footprint goal)
    {
        if (IsAirborneTransport(mover)) return !route.IsBlocked;
        if (!HasGroundSupport(mover.Footprint.AnchorX, mover.Footprint.AnchorY)) return false;
        if (route.RouteVersion != Bridges.Version)
            ReplaceMovementRoute(mover, route, FindMovePath(mover, goal, route is UnitMoveTask { Purpose: UnitMovePurpose.MoveToCell }));
        return !route.IsBlocked;
    }

    /// <summary>새 길을 저장하거나 작업을 유지한 채 대기한다. 대기·재개 이벤트는 전환 때 한 번만 낸다.</summary>
    private void ReplaceMovementRoute(GameEntity mover, MovementRoute route, List<(int X, int Y)>? replacement)
    {
        bool wasBlocked = route.IsBlocked;
        if (replacement == null)
        {
            route.Block(Bridges.Version);
            if (!wasBlocked) Emit(SessionEventKind.MoveBlocked, mover.Owner, mover.Id, $"{mover.DisplayName} 길 막힘·대기");
        }
        else
        {
            route.ChangeRoute(replacement, Bridges.Version);
            if (wasBlocked) Emit(SessionEventKind.MoveResumed, mover.Owner, mover.Id, $"{mover.DisplayName} 길 복구·이동 재개");
        }
    }

    /// <summary>타입 속도만큼 공통 경로를 진행하고 운반 중인 사제도 함께 옮긴다.</summary>
    private void AdvanceMovement(GameEntity mover, MovementRoute route)
    {
        route.Progress += MovementRate.CellsPerSecond(mover.Type) / TicksPerSecond;
        // 빠른 타입도 누적 이동량을 모두 소모하며 결정적인 칸 순서로 진행한다.
        while (route.Progress >= 1.0 && route.NextIndex < route.Path.Count)
        {
            (int x, int y) = route.Path[route.NextIndex++];
            MoveEntityTo(mover, x, y, occupies: mover.OccupiesGround);
            route.Progress -= 1.0;
        }
    }

    /// <summary>대기·재개와 다음 경로 계산을 바꾸는 상태까지 검사합에 넣는다.</summary>
    private static void AddMovementChecksum(Fnv1a hash, MovementRoute route)
    {
        hash.Add(route.NextIndex);
        hash.Add(BitConverter.DoubleToInt64Bits(route.Progress));
        hash.Add(route.RouteVersion);
        hash.Add(route.IsBlocked ? 1 : 0);
        // 경로의 순서도 이후 이동 결과에 영향을 준다.
        foreach ((int x, int y) in route.Path)
        {
            hash.Add(x);
            hash.Add(y);
        }
    }

    /// <summary>작업 종류와 독립적인 공통 이동 경로. 목표는 각 작업이 보관한다.</summary>
    private abstract class MovementRoute(List<(int X, int Y)> path, int routeVersion)
    {
        /// <summary>출발 칸부터의 이동 경로.</summary>
        public List<(int X, int Y)> Path { get; private set; } = path;
        /// <summary>다음에 이동할 경로 인덱스.</summary>
        public int NextIndex { get; set; } = 1;
        /// <summary>다음 칸까지 누적한 이동량.</summary>
        public double Progress { get; set; }
        /// <summary>마지막 탐색을 시도한 지형 버전. 실패한 경우도 기록한다.</summary>
        public int RouteVersion { get; private set; } = routeVersion;
        /// <summary>다음 지형 변화까지 현재 칸에서 기다리는지.</summary>
        public bool IsBlocked { get; private set; }

        /// <summary>경로를 바꾸고 이동 진행량·대기 상태를 초기화한다.</summary>
        public void ChangeRoute(List<(int X, int Y)> replacement, int version)
        {
            Path = replacement;
            NextIndex = 1;
            Progress = 0;
            RouteVersion = version;
            IsBlocked = false;
        }

        /// <summary>목표 작업을 남기고 옛 경로 이동만 멈춘다. 같은 버전에서는 다시 탐색하지 않는다.</summary>
        public void Block(int version)
        {
            Progress = 0;
            RouteVersion = version;
            IsBlocked = true;
        }
    }
}
