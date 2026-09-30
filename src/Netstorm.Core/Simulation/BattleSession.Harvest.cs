using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>사제가 가이저에서 결정을 가져와 신전에 전달하는 수집 경제.</summary>
public sealed partial class BattleSession
{
    /// <summary>사제 번호순으로 처리하는 수확 작업.</summary>
    private readonly SortedDictionary<int, PriestHarvestTask> _harvestTasks = [];

    /// <summary>네 방향 탐색 순서. 같은 지형에서는 언제나 같은 경로를 고른다.</summary>
    private static readonly (int Dx, int Dy, BridgeLinks Link)[] HarvestDirections =
    [
        (0, -1, BridgeLinks.North), (1, 0, BridgeLinks.East),
        (0, 1, BridgeLinks.South), (-1, 0, BridgeLinks.West),
    ];

    /// <summary>명령 대상 가이저와 사제·신전을 검사한 뒤 갈 수 있는 길을 예약한다.</summary>
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
        GameEntity? priest = _entities.Values.FirstOrDefault(e => e.Kind == ObjectKind.Priest && e.Owner == command.Player);
        if (priest == null)
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
        List<(int X, int Y)>? path = FindHarvestPath(priest.Footprint, target.Footprint, command.Player);
        if (path == null)
        {
            return new CommandResult(CommandFailure.NoRoute);
        }
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
            if (priest == null || geyser == null || temple is not { IsComplete: true })
            {
                _harvestTasks.Remove(task.PriestId);
                continue;
            }
            GameEntity target = task.Phase == HarvestPhase.ToGeyser ? geyser : temple;
            if (task.RouteVersion != Bridges.Version)
            {
                List<(int X, int Y)>? replacement = FindHarvestPath(priest.Footprint, target.Footprint, priest.Owner);
                if (replacement == null)
                {
                    _harvestTasks.Remove(task.PriestId);
                    continue;
                }
                task.ChangeRoute(replacement, Bridges.Version);
            }
            double speed = MovementRate.CellsPerSecond(priest.Type);
            task.Progress += speed / TicksPerSecond;
            // 한 틱에 두 칸 이상 움직일 수 있는 타입도 남은 이동량만큼 진행한다.
            while (task.Progress >= 1.0 && task.NextIndex < task.Path.Count)
            {
                (int x, int y) = task.Path[task.NextIndex++];
                Map.RemoveOccupant(priest.Footprint);
                priest.Footprint = Footprint.ForType(priest.Type.Definition, x, y);
                Map.AddOccupant(priest.Footprint);
                task.Progress -= 1.0;
            }
            if (task.NextIndex < task.Path.Count)
            {
                continue;
            }
            if (task.Phase == HarvestPhase.ToGeyser)
            {
                priest.CarriedCrystals = 1;
                Emit(SessionEventKind.CrystalCollected, priest.Owner, priest.Id, $"{priest.DisplayName} 결정 수확");
                List<(int X, int Y)>? home = FindHarvestPath(priest.Footprint, temple.Footprint, priest.Owner);
                if (home == null)
                {
                    _harvestTasks.Remove(task.PriestId);
                    continue;
                }
                task.Phase = HarvestPhase.ToTemple;
                task.ChangeRoute(home, Bridges.Version);
            }
            else
            {
                int crystals = priest.CarriedCrystals;
                priest.CarriedCrystals = 0;
                _players[priest.Owner].StormPower += crystals * StormPower.CrystalValue;
                Emit(SessionEventKind.CrystalDelivered, priest.Owner, priest.Id,
                    $"{priest.DisplayName} 결정 전달 (+{crystals * StormPower.CrystalValue})");
                List<(int X, int Y)>? outbound = FindHarvestPath(priest.Footprint, geyser.Footprint, priest.Owner);
                if (outbound == null)
                {
                    _harvestTasks.Remove(task.PriestId);
                    continue;
                }
                task.Phase = HarvestPhase.ToGeyser;
                task.ChangeRoute(outbound, Bridges.Version);
            }
        }
    }

    /// <summary>지면과 소유한 다리의 연결 방향을 따라 목표 발자국 둘레까지 최단 칸 경로를 찾는다.</summary>
    private List<(int X, int Y)>? FindHarvestPath(Footprint start, Footprint target, int owner)
    {
        int size = BridgeGrid.WorldSize;
        int first = start.AnchorY * size + start.AnchorX;
        if (start.AnchorX < 0 || start.AnchorY < 0 || start.AnchorX >= size || start.AnchorY >= size)
        {
            return null;
        }
        HashSet<int> goals = target.BorderCells()
            .Where(cell => cell.X >= 0 && cell.Y >= 0 && cell.X < size && cell.Y < size)
            .Where(cell => IsHarvestPassable(cell.X, cell.Y, owner))
            .Select(cell => cell.Y * size + cell.X).ToHashSet();
        if (goals.Count == 0)
        {
            return null;
        }
        int[] previous = new int[size * size];
        Array.Fill(previous, -1);
        previous[first] = first;
        var queue = new Queue<int>();
        queue.Enqueue(first);
        // 폭 우선 탐색으로 짧은 경로를 찾는다. 방향 순서가 고정이라 결과도 결정적이다.
        while (queue.Count > 0)
        {
            int at = queue.Dequeue();
            if (goals.Contains(at))
            {
                var path = new List<(int X, int Y)>();
                // 부모 칸을 거슬러 올라가 시작점부터의 순서로 뒤집는다.
                for (int point = at; point != first; point = previous[point])
                {
                    path.Add((point % size, point / size));
                }
                path.Add((first % size, first / size));
                path.Reverse();
                return path;
            }
            int x = at % size;
            int y = at / size;
            // 북·동·남·서 이웃 중 지나갈 수 있는 칸을 큐에 넣는다.
            foreach ((int dx, int dy, BridgeLinks link) in HarvestDirections)
            {
                int nx = x + dx;
                int ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= size || ny >= size || previous[ny * size + nx] >= 0
                    || !CanHarvestStep(x, y, nx, ny, link, owner))
                {
                    continue;
                }
                int next = ny * size + nx;
                previous[next] = at;
                queue.Enqueue(next);
            }
        }
        return null;
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

    /// <summary>사제가 가이저로 가는지 신전으로 결정을 되돌리는지.</summary>
    private enum HarvestPhase { ToGeyser, ToTemple }

    /// <summary>사제 한 명의 반복 수확 경로와 다음 칸까지의 이동량.</summary>
    private sealed class PriestHarvestTask(int priestId, int geyserId, int templeId,
        List<(int X, int Y)> path, HarvestPhase phase, int routeVersion)
    {
        /// <summary>움직이는 사제 번호.</summary>
        public int PriestId { get; } = priestId;
        /// <summary>수확 대상 가이저 번호.</summary>
        public int GeyserId { get; } = geyserId;
        /// <summary>결정을 받을 신전 번호.</summary>
        public int TempleId { get; } = templeId;
        /// <summary>현재 왕복 방향.</summary>
        public HarvestPhase Phase { get; set; } = phase;
        /// <summary>지나갈 칸 목록.</summary>
        public List<(int X, int Y)> Path { get; private set; } = path;
        /// <summary>다음에 지나갈 칸의 목록 인덱스.</summary>
        public int NextIndex { get; set; } = 1;
        /// <summary>다음 칸까지 누적한 칸 단위 이동량.</summary>
        public double Progress { get; set; }
        /// <summary>경로가 계산된 다리 격자 버전.</summary>
        public int RouteVersion { get; private set; } = routeVersion;

        /// <summary>연결망이 바뀌거나 왕복 방향이 바뀌면 다음 경로를 현재 칸에서 시작한다.</summary>
        public void ChangeRoute(List<(int X, int Y)> replacement, int version)
        {
            Path = replacement;
            NextIndex = 1;
            Progress = 0;
            RouteVersion = version;
        }
    }
}
