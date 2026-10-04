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
        {
            bool exact = route is UnitMoveTask { Purpose: UnitMovePurpose.MoveToCell };
            double progress = route.Progress;
            // 안전한 다음 걸음은 끝까지 이어 간다. 무관한 다리 배치가 진행량을 초기화하면 화면에서 뒤로 튄다.
            if (!route.IsBlocked && progress > 0 && route.NextIndex < route.Path.Count &&
                route.Path[route.NextIndex - 1] == (mover.Footprint.AnchorX, mover.Footprint.AnchorY))
            {
                (int x, int y) = route.Path[route.NextIndex];
                if (CanWalkStep(mover.Footprint.AnchorX, mover.Footprint.AnchorY, x, y, mover.Owner) &&
                    FindMovePath(mover, goal, exact, new Footprint(x, y, 1, 1)) is { } remaining)
                {
                    remaining.Insert(0, (mover.Footprint.AnchorX, mover.Footprint.AnchorY));
                    ReplaceMovementRoute(mover, route, remaining);
                    route.Progress = progress;
                    return true;
                }
            }
            ReplaceMovementRoute(mover, route, FindMovePath(mover, goal, exact));
        }
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

    /// <summary>
    /// 타입 속도만큼 공통 경로를 진행하고 운반 중인 사제도 함께 옮긴다. 속도는 칸 길이 기준이라
    /// 대각선 한 걸음(√2칸)은 직선 한 걸음보다 오래 걸린다 — 원본 녹화에서 사제의 8칸 직선 이동 4.3초와
    /// 대각선 3.75칸 + 직선 7칸(약 12.3칸 길이) 이동 약 7초가 같은 속도 1.8칸/초로 맞는다.
    /// </summary>
    private void AdvanceMovement(GameEntity mover, MovementRoute route)
    {
        FaceNextStep(mover, route);
        route.Progress += MovementRate.CellsPerSecond(mover.Type) / TicksPerSecond;
        // 빠른 타입도 누적 이동량을 모두 소모하며 결정적인 칸 순서로 진행한다.
        while (route.NextIndex < route.Path.Count)
        {
            double length = StepLength(route.Path[route.NextIndex - 1], route.Path[route.NextIndex]);
            if (route.Progress < length) break;
            (int x, int y) = route.Path[route.NextIndex++];
            MoveEntityTo(mover, x, y, occupies: mover.OccupiesGround);
            route.Progress -= length;
            FaceNextStep(mover, route);
        }
    }

    /// <summary>다음 걸음의 방향을 오브젝트가 바라보는 방향으로 기록한다 (화면 연출 전용).</summary>
    private static void FaceNextStep(GameEntity mover, MovementRoute route)
    {
        if (route.NextIndex >= route.Path.Count) return;
        (int fromX, int fromY) = route.Path[route.NextIndex - 1];
        (int toX, int toY) = route.Path[route.NextIndex];
        int heading = UnitHeading.FromStep(toX - fromX, toY - fromY);
        if (heading != UnitHeading.None) mover.Heading = heading;
    }

    /// <summary>오브젝트가 지금 쓰는 이동 작업 (이동 명령·수송·수확 중 하나, 없으면 null).</summary>
    private MovementRoute? RouteOf(int entityId) =>
        _moveTasks.TryGetValue(entityId, out UnitMoveTask? move) ? move
        : _harvestTasks.TryGetValue(entityId, out PriestHarvestTask? harvest) ? harvest : null;

    /// <summary>걸어서 이동 중인지 (길 막힘 대기나 도착한 뒤·가이저에서 머무는 동안은 false). 걷는 그림을 고르는 데 쓴다.</summary>
    /// <param name="entityId">오브젝트 번호</param>
    public bool IsMoving(int entityId) => RouteOf(entityId) is { IsBlocked: false } route && route.NextIndex < route.Path.Count;

    /// <summary>
    /// 화면에 그릴 칸 좌표(소수). 규칙상 위치는 칸 단위지만 걷는 유닛은 현재 칸에서 다음 칸 쪽으로
    /// 이동량(<c>Progress</c>)과 다음 틱까지 흐른 시간(<c>Alpha</c>)만큼 앞서 그려 부드럽게 걷는 것처럼 보인다.
    /// 규칙·검사합·충돌 판정에는 쓰이지 않는다. 걷지 않으면 기준 칸 그대로다.
    /// </summary>
    /// <param name="entity">그릴 오브젝트</param>
    public (double X, double Y) VisualCell(GameEntity entity) => MovementCell(entity, _timestep.Alpha);

    /// <summary>틱 진행량과 선택한 보간 계수로 이동 위치를 구한다. 전투 판정은 계수 0을 써 그리기 시간과 독립시킨다.</summary>
    private (double X, double Y) MovementCell(GameEntity entity, double alpha)
    {
        (double x, double y) = (entity.Footprint.AnchorX, entity.Footprint.AnchorY);
        if (RouteOf(entity.Id) is not { IsBlocked: false } route || route.NextIndex >= route.Path.Count
            || route.Path[route.NextIndex - 1] != (entity.Footprint.AnchorX, entity.Footprint.AnchorY))
        {
            return (x, y);
        }
        (int fromX, int fromY) = route.Path[route.NextIndex - 1];
        (int toX, int toY) = route.Path[route.NextIndex];
        double perTick = MovementRate.CellsPerSecond(entity.Type) / TicksPerSecond;
        double fraction = Math.Clamp((route.Progress + alpha * perTick) / StepLength((fromX, fromY), (toX, toY)), 0, 1);
        return (fromX + (toX - fromX) * fraction, fromY + (toY - fromY) * fraction);
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
