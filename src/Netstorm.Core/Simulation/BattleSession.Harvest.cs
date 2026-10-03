using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>사제·수송 유닛이 가이저에서 결정을 가져와 신전에 전달하는 수집 경제.</summary>
public sealed partial class BattleSession
{
    /// <summary>수집자 번호순으로 처리하는 수확 작업. 내부 작업 타입의 기존 이름은 유지한다.</summary>
    private readonly SortedDictionary<int, PriestHarvestTask> _harvestTasks = [];

    /// <summary>
    /// 가이저에 도착한 수집자가 결정 하나를 얻을 때까지 머무는 시간(초). 원본 값은 미측정인 임시값이다.
    /// 가이저 둘레와 신전 둘레가 맞닿아 이동 거리가 0인 경우에도 틱마다 결정을 얻지 않게 한다.
    /// </summary>
    public const double HarvestMineSeconds = 1.0;

    /// <summary>
    /// 걸음 후보 8방향. 대각선(북동·남동·남서·북서)을 앞에 두어 같은 걸음 수의 경로에서 대각선 걸음이 먼저 선택된다.
    /// 그 뒤에 북·동·남·서 순서의 직선 걸음이 온다.
    /// </summary>
    private static readonly (int Dx, int Dy)[] WalkDirections =
    [
        (1, -1), (1, 1), (-1, 1), (-1, -1),
        (0, -1), (1, 0), (0, 1), (-1, 0),
    ];

    /// <summary>경로 탐색에서 직선 한 걸음의 정수 길이.</summary>
    private const int StraightStepCost = 10;

    /// <summary>경로 탐색에서 대각선 한 걸음의 정수 길이 (10 × √2 = 14.14 를 정수로 근사).</summary>
    private const int DiagonalStepCost = 14;

    /// <summary>한 걸음의 칸 길이. 대각선은 정사각 칸의 대각선 길이(√2)이고 직선은 1이다.</summary>
    internal const double DiagonalStepLength = 1.4142135623730951;

    /// <summary>두 이웃 칸 사이 한 걸음의 길이(칸). 가로·세로가 모두 바뀌면 대각선이다.</summary>
    internal static double StepLength((int X, int Y) from, (int X, int Y) to) =>
        from.X != to.X && from.Y != to.Y ? DiagonalStepLength : 1.0;

    /// <summary>명령 대상 가이저와 수집자·신전을 검사한 뒤 갈 수 있는 길을 예약한다.</summary>
    private CommandResult ExecuteHarvestGeyser(HarvestGeyserCommand command)
    {
        if (!_players.ContainsKey(command.Player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        GameEntity? geyser = Entity(command.GeyserId);
        if (geyser == null)
        {
            return new CommandResult(CommandFailure.NoSuchEntity);
        }
        if (geyser.Kind != ObjectKind.Geyser)
        {
            return new CommandResult(CommandFailure.WrongKind);
        }
        if (geyser.IsDepletedGeyser)
        {
            return new CommandResult(CommandFailure.GeyserEmpty);
        }
        GameEntity? priest = command.CollectorId == 0
            ? _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Priest && e.Owner == command.Player)
            : Entity(command.CollectorId);
        if (priest != null && priest.Owner != command.Player) return new CommandResult(CommandFailure.NotOwner);
        if (priest != null && priest.Kind is not (ObjectKind.Priest or ObjectKind.Transport)) return new CommandResult(CommandFailure.WrongKind);
        if (priest is { CarriedPriestId: not 0 }) return new CommandResult(CommandFailure.AlreadyCarrying);
        if (priest == null || priest.IsStunned || priest.Captivity != PriestCaptivity.Free)
        {
            return new CommandResult(CommandFailure.NoPriest);
        }
        GameEntity? temple = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Temple && e.Owner == command.Player && e.IsComplete);
        if (temple == null)
        {
            return new CommandResult(CommandFailure.NoTemple);
        }
        bool carrying = priest.CarriedCrystals > 0;
        GameEntity target = carrying ? temple : geyser;
        List<(int X, int Y)>? path = FindMovePath(priest, target.Footprint);
        if (path == null)
        {
            return new CommandResult(CommandFailure.NoRoute);
        }
        _moveTasks.Remove(priest.Id);
        _harvestTasks[priest.Id] = new PriestHarvestTask(priest.Id, geyser.Id, temple.Id, path,
            carrying ? HarvestPhase.ToTemple : HarvestPhase.ToGeyser, Bridges.Version);
        return CommandResult.Ok();
    }

    /// <summary>튜토리얼 1의 화면 복귀 신호를 틱 이벤트로 기록한다.</summary>
    private CommandResult ExecuteReturnHome(ReturnHomeCommand command)
    {
        if (!_players.ContainsKey(command.Player))
        {
            return new CommandResult(CommandFailure.UnknownPlayer);
        }
        Emit(SessionEventKind.ReturnedHome, command.Player, 0, "자기 섬 화면으로 복귀");
        return CommandResult.Ok();
    }

    /// <summary>현재 다리·섬에서 가이저까지 사제가 갈 수 있는지. 튜토리얼 1의 연결 단계에 쓴다.</summary>
    internal bool CanReachAnyGeyser(int owner)
    {
        GameEntity? priest = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Priest && e.Owner == owner);
        return priest != null && _entities.Values.Any(e => e.Kind == ObjectKind.Geyser
            && FindHarvestPath(priest.Footprint, e.Footprint, owner) != null);
    }

    /// <summary>현재 다리 연결이 바뀌면 길을 다시 찾고, 한 틱만큼 사제를 움직여 수확·전달한다.</summary>
    private void UpdateHarvests()
    {
        // 사제 번호순으로 같은 틱의 작업을 처리한다.
        foreach (PriestHarvestTask task in _harvestTasks.Values.ToArray())
        {
            GameEntity? priest = Entity(task.PriestId);
            GameEntity? geyser = Entity(task.GeyserId);
            GameEntity? temple = Entity(task.TempleId);
            if (priest == null || priest.IsStunned || priest.Captivity != PriestCaptivity.Free || geyser == null || temple is not { IsComplete: true })
            {
                _harvestTasks.Remove(task.PriestId);
                continue;
            }
            GameEntity target = task.Phase == HarvestPhase.ToGeyser ? geyser : temple;
            if (!PrepareMovement(priest, task, target.Footprint)) continue;
            AdvanceMovement(priest, task);
            if (task.NextIndex < task.Path.Count)
            {
                continue;
            }
            if (task.Phase == HarvestPhase.ToGeyser)
            {
                // 이미 다른 수집자가 비워 버렸으면 반복을 끝낸다
                if (geyser.IsDepletedGeyser)
                {
                    _harvestTasks.Remove(task.PriestId);
                    continue;
                }
                // 가이저 옆에서 정해진 시간 동안 머문 뒤 결정을 얻는다
                if (task.MiningUntilTick == 0) task.MiningUntilTick = Tick + TicksFor(HarvestMineSeconds);
                if (Tick < task.MiningUntilTick) continue;
                task.MiningUntilTick = 0;
                geyser.StoredStormPower = Math.Max(0, geyser.StoredStormPower - StormPower.CrystalValue);
                priest.CarriedCrystals = 1;
                Emit(SessionEventKind.CrystalCollected, priest.Owner, priest.Id, $"{priest.DisplayName} 결정 수확");
                if (geyser.IsDepletedGeyser) Emit(SessionEventKind.GeyserDepleted, 0, geyser.Id, "가이저 고갈");
                List<(int X, int Y)>? home = FindMovePath(priest, temple.Footprint);
                task.Phase = HarvestPhase.ToTemple;
                ReplaceMovementRoute(priest, task, home);
            }
            else
            {
                int crystals = priest.CarriedCrystals;
                priest.CarriedCrystals = 0;
                _players[priest.Owner].StormPower += crystals * StormPower.CrystalValue;
                Emit(SessionEventKind.CrystalDelivered, priest.Owner, priest.Id,
                    $"{priest.DisplayName} 결정 전달 (+{crystals * StormPower.CrystalValue})");
                // 가이저가 비었으면 원본처럼 반복을 멈춘다
                if (geyser.IsDepletedGeyser)
                {
                    _harvestTasks.Remove(task.PriestId);
                    continue;
                }
                List<(int X, int Y)>? outbound = FindMovePath(priest, geyser.Footprint);
                task.Phase = HarvestPhase.ToGeyser;
                ReplaceMovementRoute(priest, task, outbound);
            }
        }
    }

    /// <summary>
    /// 지면과 소유한 다리의 연결 방향을 따라 목표 발자국 둘레까지 최단 걸음 경로를 찾는다.
    /// 섬 위에서는 대각선 걸음도 허용하며(8방향) 같은 걸음 수의 경로 가운데 **대각선을 먼저** 걷는 경로를 고른다.
    /// 2026-10-03 원본 자동 분석에서 사제가 대각선으로 먼저 간 뒤 직선으로 이어 걷는 것을 확인했다.
    /// 다리 칸은 상하좌우 연결 방향으로만 지나므로 다리에 걸친 걸음은 대각선이 될 수 없다.
    /// </summary>
    private List<(int X, int Y)>? FindHarvestPath(Footprint start, Footprint target, int owner, bool exact = false)
    {
        int size = BridgeGrid.WorldSize;
        int first = start.AnchorY * size + start.AnchorX;
        if (start.AnchorX < 0 || start.AnchorY < 0 || start.AnchorX >= size || start.AnchorY >= size ||
            !IsHarvestPassable(start.AnchorX, start.AnchorY, owner))
        {
            return null;
        }
        HashSet<int> goals = (exact ? target.Cells() : target.BorderCells())
            .Where(cell => cell.X >= 0 && cell.Y >= 0 && cell.X < size && cell.Y < size)
            .Where(cell => IsHarvestPassable(cell.X, cell.Y, owner))
            .Select(cell => cell.Y * size + cell.X).ToHashSet();
        if (goals.Count == 0)
        {
            return null;
        }
        // 목표 칸들에서 한꺼번에 퍼지는 다익스트라 탐색으로 각 칸의 "목표까지 걸음 길이"를 구한다.
        // 길이는 정수(직선 10·대각선 14 ≈ √2배)로 세어 결과가 결정적이다. 걸음 수만 세면 세로 이동에서도 지그재그가 최단이 된다.
        int[] distance = new int[size * size];
        Array.Fill(distance, int.MaxValue);
        var queue = new PriorityQueue<int, int>();
        // 목표는 칸 번호순으로 넣어 같은 길이의 동률도 언제나 같은 순서로 처리한다.
        foreach (int goal in goals.Order())
        {
            distance[goal] = 0;
            queue.Enqueue(goal, 0);
        }
        // 시작 칸이 확정될 때까지 가까운 칸부터 확정한다.
        while (queue.TryDequeue(out int at, out int atDistance))
        {
            if (atDistance > distance[at]) continue;
            if (at == first) break;
            int x = at % size;
            int y = at / size;
            // 걸음 가능 여부는 양방향이 같으므로 이 칸에서 이웃으로 가는 방향으로 검사한다.
            foreach ((int dx, int dy) in WalkDirections)
            {
                int nx = x + dx;
                int ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= size || ny >= size || !CanWalkStep(x, y, nx, ny, owner)) continue;
                int through = atDistance + (dx != 0 && dy != 0 ? DiagonalStepCost : StraightStepCost);
                if (through >= distance[ny * size + nx]) continue;
                distance[ny * size + nx] = through;
                queue.Enqueue(ny * size + nx, through);
            }
        }
        if (distance[first] == int.MaxValue)
        {
            return null;
        }
        // 시작 칸에서 "남은 길이 = 이웃의 남은 길이 + 걸음 길이"가 되는 이웃을 따라 걷는다.
        // WalkDirections 는 대각선이 앞서므로 같은 길이의 경로 가운데 대각선 걸음이 먼저 나온다.
        var path = new List<(int X, int Y)> { (first % size, first / size) };
        int current = first;
        // 목표 칸(길이 0)에 닿을 때까지 한 걸음씩 고른다.
        while (distance[current] > 0)
        {
            int x = current % size;
            int y = current / size;
            int chosen = -1;
            // 최단 길이를 이루는 첫 이웃이 이번 걸음이다.
            foreach ((int dx, int dy) in WalkDirections)
            {
                int nx = x + dx;
                int ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= size || ny >= size || !CanWalkStep(x, y, nx, ny, owner)) continue;
                int step = dx != 0 && dy != 0 ? DiagonalStepCost : StraightStepCost;
                if (distance[ny * size + nx] == int.MaxValue || distance[ny * size + nx] + step != distance[current]) continue;
                chosen = ny * size + nx;
                break;
            }
            current = chosen;
            path.Add((current % size, current / size));
        }
        return path;
    }

    /// <summary>섬 칸 또는 플레이어 소유의 다리 칸인지 확인한다.</summary>
    private bool IsHarvestPassable(int x, int y, int owner) =>
        Bridges.IsIsland(x, y) || Bridges.At(x, y) is { } bridge && bridge.Owner == owner;

    /// <summary>섬끼리는 인접 이동하고, 다리가 끼면 양쪽 연결 방향을 확인한다.</summary>
    private bool CanHarvestStep(int x, int y, int nx, int ny, BridgeLinks direction, int owner)
    {
        if (!IsHarvestPassable(nx, ny, owner))
        {
            return false;
        }
        BridgeCellState? from = Bridges.At(x, y);
        BridgeCellState? to = Bridges.At(nx, ny);
        if (from != null && (from.Owner != owner || !from.Cell.Links.HasFlag(direction)))
        {
            return false;
        }
        if (to != null && (to.Owner != owner || !to.Cell.Links.HasFlag(BridgeDirections.Opposite(direction))))
        {
            return false;
        }
        return true;
    }

    /// <summary>
    /// 이웃 칸으로 한 걸음 걸을 수 있는지. 직선은 섬끼리 인접 이동하거나 다리 연결 방향을 확인하고(<see cref="CanHarvestStep"/>),
    /// 대각선은 걸음이 닿는 네 칸이 모두 다리 없는 섬 칸일 때만 허용해 허공이나 다리 모서리를 가로지르지 않는다.
    /// 걸음 가능 여부는 어느 쪽에서 보아도 같다.
    /// </summary>
    private bool CanWalkStep(int x, int y, int nx, int ny, int owner)
    {
        int dx = nx - x;
        int dy = ny - y;
        if (dx == 0 || dy == 0)
        {
            BridgeLinks link = (dx, dy) switch
            {
                (0, -1) => BridgeLinks.North, (1, 0) => BridgeLinks.East, (0, 1) => BridgeLinks.South, _ => BridgeLinks.West,
            };
            return CanHarvestStep(x, y, nx, ny, link, owner);
        }
        return IsPlainIsland(x, y) && IsPlainIsland(nx, ny) && IsPlainIsland(nx, y) && IsPlainIsland(x, ny);
    }

    /// <summary>다리가 놓이지 않은 섬 칸인지 (대각선 걸음이 지날 수 있는 칸).</summary>
    private bool IsPlainIsland(int x, int y) => Bridges.IsIsland(x, y) && Bridges.At(x, y) == null;

    /// <summary>사제가 가이저로 가는지 신전으로 결정을 되돌리는지.</summary>
    private enum HarvestPhase { ToGeyser, ToTemple }

    /// <summary>사제 한 명의 반복 수확 경로와 다음 칸까지의 이동량.</summary>
    private sealed class PriestHarvestTask(int priestId, int geyserId, int templeId,
        List<(int X, int Y)> path, HarvestPhase phase, int routeVersion) : MovementRoute(path, routeVersion)
    {
        /// <summary>움직이는 사제 번호.</summary>
        public int PriestId { get; } = priestId;
        /// <summary>수확 대상 가이저 번호.</summary>
        public int GeyserId { get; } = geyserId;
        /// <summary>결정을 받을 신전 번호.</summary>
        public int TempleId { get; } = templeId;
        /// <summary>현재 왕복 방향.</summary>
        public HarvestPhase Phase { get; set; } = phase;
        /// <summary>가이저 옆에서 결정을 얻는 틱 (머무는 중이 아니면 0).</summary>
        public long MiningUntilTick { get; set; }
    }
}
