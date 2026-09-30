using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>놓인 다리 칸 하나의 상태</summary>
/// <param name="X">월드 칸 x</param>
/// <param name="Y">월드 칸 y</param>
/// <param name="Cell">방향 글자·변형</param>
/// <param name="Owner">소유 플레이어</param>
public sealed record BridgeCellState(int X, int Y, BridgeCell Cell, int Owner)
{
    /// <summary>칸 상태 (보통·금 감·단단함)</summary>
    public BridgeCondition Condition { get; set; }

    /// <summary>
    /// 남은 수명 0~7 (원본 오브젝트 +0xc 의 비트 3~6, brMAX_TIME_LEFT = 7).
    /// 0 은 "수명 계산 전"이며, 붕괴가 시작되면 7 부터 한 단계씩 줄어 다시 0 이 되는 순간 사라진다.
    /// </summary>
    public int TimeLeft { get; set; }
}

/// <summary>다리 조각을 놓을 수 없는 이유</summary>
public enum BridgePlacementProblem
{
    /// <summary>놓을 수 있다</summary>
    None,
    /// <summary>월드(256×256 칸) 밖으로 나간다</summary>
    OutOfWorld,
    /// <summary>섬·다른 다리·오브젝트가 있는 칸과 겹친다 (원본 FUN_0049b510 의 겹침 검사)</summary>
    Blocked,
    /// <summary>섬 가장자리나 내 다리의 열린 끝에 이어지지 않는다</summary>
    NotAttached,
}

/// <summary>다리 조각 배치 판정 결과</summary>
/// <param name="Problem">불가 이유 (None 이면 가능)</param>
/// <param name="BlockedCells">겹쳐서 막힌 칸 (빨강 표시용)</param>
/// <param name="Attachments">이어지는 연결 수 (섬 가장자리 + 다리 끝)</param>
public sealed record BridgePlacementCheck(BridgePlacementProblem Problem, IReadOnlyList<(int X, int Y)> BlockedCells, int Attachments)
{
    /// <summary>놓을 수 있는지</summary>
    public bool Allowed => Problem == BridgePlacementProblem.None;
}

/// <summary>붕괴 갱신 한 번의 결과</summary>
/// <param name="Cracked">이번에 금 간 상태로 바뀐 칸 (원본은 bridgeCrack.wav 재생)</param>
/// <param name="Removed">이번에 무너져 사라진 칸</param>
public sealed record BridgeDecayResult(IReadOnlyList<BridgeCellState> Cracked, IReadOnlyList<BridgeCellState> Removed);

/// <summary>
/// 놓인 다리 칸들의 연결·배치 판정·붕괴 (docs/exe/bridge-pieces.md 8절).
/// exe 로 확인한 부분: 10초 주기 갱신(Bridge.cpp FUN_00422bc0, 상수 0x52f968 = 10.0),
/// 수명 0~7·금 감 기준 5(0x52f960)·0 이면 제거(FUN_00421c30), 단단한 칸 제외, 연결망 최소값 − 1 동기화(FUN_004227e0),
/// 겹치는 칸이 있으면 배치 불가(Rifttype.cpp FUN_0049b510).
/// 근사한 부분: "섬 가장자리 또는 내 다리의 열린 끝에 이어짐"(원본은 영역 소유 판정 FUN_0048fdb0),
/// "열린 끝이 있는 연결망만 붕괴"(원본 FUN_004217f0·FUN_004218b0 의 조건을 단순화), 초목 가장자리 제외는 호출자 판정.
/// </summary>
public sealed class BridgeGrid
{
    /// <summary>붕괴 갱신 간격(초) (원본 0x52f968)</summary>
    public const double DecaySeconds = 10.0;

    /// <summary>수명 최대값 (원본 brMAX_TIME_LEFT)</summary>
    public const int MaxTimeLeft = 7;

    /// <summary>이 값보다 작아지면 금 간 상태가 된다 (원본 DAT_0052f960 = 5)</summary>
    public const int CrackBelow = 5;

    /// <summary>월드 칸 크기 (원본 BOARDDIM_IN_TILES = 256)</summary>
    public const int WorldSize = 256;

    /// <summary>네 방향 (북·동·남·서)</summary>
    private static readonly BridgeLinks[] Directions = [BridgeLinks.North, BridgeLinks.East, BridgeLinks.South, BridgeLinks.West];

    /// <summary>칸 → 다리 칸 상태</summary>
    private readonly Dictionary<(int X, int Y), BridgeCellState> _cells = [];

    /// <summary>섬 칸인지 (본섬·받침·가이저 바위)</summary>
    private readonly Func<int, int, bool> _isIsland;

    /// <summary>다리 외의 오브젝트가 칸을 차지하는지</summary>
    private readonly Func<int, int, bool> _isOccupied;

    /// <summary>섬 칸의 그 방향 가장자리에 다리를 붙일 수 있는지 (섬 x, 섬 y, 섬에서 다리 쪽 방향, 플레이어)</summary>
    private readonly Func<int, int, BridgeLinks, int, bool> _canAttachToIsland;

    /// <summary>다음 붕괴 갱신 시각(초)</summary>
    private double _nextDecay = DecaySeconds;

    /// <summary>다리 칸이 추가·제거될 때마다 커지는 번호 (연결 결과 캐시의 유효 여부 확인용)</summary>
    public int Version { get; private set; }

    /// <summary>다리 칸 모음을 만든다</summary>
    /// <param name="isIsland">섬 칸 판정</param>
    /// <param name="isOccupied">다른 오브젝트 점유 판정 (없으면 항상 비어 있음)</param>
    /// <param name="canAttachToIsland">섬 가장자리 부착 가능 판정 (없으면 모든 섬 가장자리 허용)</param>
    public BridgeGrid(Func<int, int, bool> isIsland, Func<int, int, bool>? isOccupied = null,
        Func<int, int, BridgeLinks, int, bool>? canAttachToIsland = null)
    {
        _isIsland = isIsland;
        _isOccupied = isOccupied ?? ((_, _) => false);
        _canAttachToIsland = canAttachToIsland ?? ((_, _, _, _) => true);
    }

    /// <summary>놓인 다리 칸 전체</summary>
    public IReadOnlyCollection<BridgeCellState> Cells => _cells.Values;

    /// <summary>칸의 다리 (없으면 null)</summary>
    /// <param name="x">월드 칸 x</param>
    /// <param name="y">월드 칸 y</param>
    public BridgeCellState? At(int x, int y) => _cells.GetValueOrDefault((x, y));

    /// <summary>맵에서 만든 본섬·받침·가이저 바위 칸인지. 사제 이동 경로의 지면 판정에 쓴다.</summary>
    public bool IsIsland(int x, int y) => _isIsland(x, y);

    /// <summary>
    /// 저장된 다리 칸(.fort bridge 값 = bridge.type 클러스터 번호)을 추가한다.
    /// 클러스터 이름의 번호로 상태를 정한다: 20 = 단단함, 11~19 = 금 감(변형 = 번호 − 10), 그 밖 = 보통.
    /// </summary>
    /// <param name="frames">bridge 타입 프레임 표</param>
    /// <param name="cluster">저장된 클러스터 번호</param>
    /// <param name="x">월드 칸 x</param>
    /// <param name="y">월드 칸 y</param>
    /// <param name="owner">소유 플레이어</param>
    public BridgeCellState AddStored(TypeFrameTable frames, int cluster, int x, int y, int owner)
    {
        FrameCode code = frames.Codes[cluster];
        (BridgeCondition condition, int variation) = code.Number switch
        {
            >= BridgeFrames.HardNumber => (BridgeCondition.Hard, 1),
            > BridgeFrames.CrackedOffset => (BridgeCondition.Cracked, code.Number - BridgeFrames.CrackedOffset),
            _ => (BridgeCondition.Normal, Math.Max(1, code.Number)),
        };
        var state = new BridgeCellState(x, y, new BridgeCell(code.Side, variation), owner) { Condition = condition };
        // 원본 다리 방향 글자(A~P)만 다리 칸으로 본다 (그 밖의 특수 프레임은 연결 계산에서 제외)
        if (BridgeDirections.IsLetter(code.Side))
        {
            _cells[(x, y)] = state;
            Version++;
        }
        return state;
    }

    /// <summary>조각을 (originX, originY) 왼쪽 위 칸에 놓을 수 있는지 판정한다.</summary>
    /// <param name="piece">회전된 조각</param>
    /// <param name="originX">조각 왼쪽 위 칸 x</param>
    /// <param name="originY">조각 왼쪽 위 칸 y</param>
    /// <param name="player">놓는 플레이어</param>
    public BridgePlacementCheck Check(BridgePiece piece, int originX, int originY, int player)
    {
        IReadOnlyList<PlacedBridgeCell> cells = piece.Cells();
        var own = cells.Select(c => (originX + c.Dx, originY + c.Dy)).ToHashSet();
        var blocked = new List<(int X, int Y)>();
        bool outside = false;
        // 조각 칸마다 월드 범위와 겹침을 검사한다
        foreach ((int x, int y) in own)
        {
            if (x < 0 || y < 0 || x >= WorldSize || y >= WorldSize)
            {
                outside = true;
            }
            else if (_isIsland(x, y) || _cells.ContainsKey((x, y)) || _isOccupied(x, y))
            {
                blocked.Add((x, y));
            }
        }
        if (outside)
        {
            return new BridgePlacementCheck(BridgePlacementProblem.OutOfWorld, blocked, 0);
        }
        if (blocked.Count > 0)
        {
            return new BridgePlacementCheck(BridgePlacementProblem.Blocked, blocked, 0);
        }
        int attachments = 0;
        // 조각 칸의 연결 방향이 조각 밖으로 나가는 곳에서 섬 가장자리 또는 내 다리의 열린 끝을 찾는다
        foreach (PlacedBridgeCell cell in cells)
        {
            int x = originX + cell.Dx;
            int y = originY + cell.Dy;
            foreach (BridgeLinks direction in Directions)
            {
                if (!cell.Cell.Links.HasFlag(direction))
                {
                    continue;
                }
                (int dx, int dy) = BridgeDirections.Offset(direction);
                (int nx, int ny) = (x + dx, y + dy);
                if (own.Contains((nx, ny)))
                {
                    continue;
                }
                if (_isIsland(nx, ny) && _canAttachToIsland(nx, ny, BridgeDirections.Opposite(direction), player))
                {
                    attachments++;
                }
                else if (_cells.TryGetValue((nx, ny), out BridgeCellState? neighbor) && neighbor.Owner == player
                    && neighbor.Cell.Links.HasFlag(BridgeDirections.Opposite(direction)))
                {
                    attachments++;
                }
            }
        }
        return new BridgePlacementCheck(attachments > 0 ? BridgePlacementProblem.None : BridgePlacementProblem.NotAttached, blocked, attachments);
    }

    /// <summary>조각을 놓는다 (판정은 호출자가 먼저 한다). 새 칸은 보통 상태·수명 0 으로 시작한다.</summary>
    /// <param name="piece">회전된 조각</param>
    /// <param name="originX">조각 왼쪽 위 칸 x</param>
    /// <param name="originY">조각 왼쪽 위 칸 y</param>
    /// <param name="player">소유 플레이어</param>
    public IReadOnlyList<BridgeCellState> Place(BridgePiece piece, int originX, int originY, int player)
    {
        var placed = new List<BridgeCellState>();
        // 회전된 칸마다 상태를 만들어 격자에 넣는다
        foreach (PlacedBridgeCell cell in piece.Cells())
        {
            var state = new BridgeCellState(originX + cell.Dx, originY + cell.Dy, cell.Cell, player);
            _cells[(state.X, state.Y)] = state;
            placed.Add(state);
        }
        Version++;
        return placed;
    }

    /// <summary>칸의 한 방향이 열려 있는지: 연결 글자에 그 방향이 있는데 그쪽에 섬도, 마주 연결된 다리도 없다</summary>
    /// <param name="state">다리 칸</param>
    /// <param name="direction">방향</param>
    public bool IsOpen(BridgeCellState state, BridgeLinks direction)
    {
        if (!state.Cell.Links.HasFlag(direction))
        {
            return false;
        }
        (int dx, int dy) = BridgeDirections.Offset(direction);
        (int nx, int ny) = (state.X + dx, state.Y + dy);
        if (_isIsland(nx, ny))
        {
            return false;
        }
        return !(_cells.TryGetValue((nx, ny), out BridgeCellState? neighbor) && neighbor.Cell.Links.HasFlag(BridgeDirections.Opposite(direction)));
    }

    /// <summary>서로 마주 연결된 다리 칸 무리(연결망)를 나눈다</summary>
    public IReadOnlyList<IReadOnlyList<BridgeCellState>> Networks()
    {
        var seen = new HashSet<(int, int)>();
        var networks = new List<IReadOnlyList<BridgeCellState>>();
        // 아직 방문하지 않은 칸에서 너비 우선으로 연결망을 모은다 (좌표 순서로 시작해 결과를 일정하게 한다)
        foreach (BridgeCellState start in _cells.Values.OrderBy(c => c.Y).ThenBy(c => c.X))
        {
            if (!seen.Add((start.X, start.Y)))
            {
                continue;
            }
            var network = new List<BridgeCellState>();
            var queue = new Queue<BridgeCellState>([start]);
            // 마주 연결된 이웃을 따라 넓힌다
            while (queue.Count > 0)
            {
                BridgeCellState cell = queue.Dequeue();
                network.Add(cell);
                foreach (BridgeLinks direction in Directions)
                {
                    if (!cell.Cell.Links.HasFlag(direction))
                    {
                        continue;
                    }
                    (int dx, int dy) = BridgeDirections.Offset(direction);
                    if (_cells.TryGetValue((cell.X + dx, cell.Y + dy), out BridgeCellState? next)
                        && next.Cell.Links.HasFlag(BridgeDirections.Opposite(direction)) && seen.Add((next.X, next.Y)))
                    {
                        queue.Enqueue(next);
                    }
                }
            }
            networks.Add(network);
        }
        return networks;
    }

    /// <summary>
    /// 게임 시각을 진행한다. 10초마다 한 번씩 연결망 붕괴를 갱신한다.
    /// 열린 끝이 있는 연결망은 단단하지 않은 칸의 수명을 "0 이 아닌 최소값 − 1"(모두 0 이면 7)로 맞추고,
    /// 5 아래로 내려가면 금 간 상태로 바꾸며, 0 이 되면 제거한다.
    /// </summary>
    /// <param name="now">게임 시각(초)</param>
    public BridgeDecayResult Update(double now)
    {
        var cracked = new List<BridgeCellState>();
        var removed = new List<BridgeCellState>();
        // 밀린 갱신이 있으면 모두 처리한다 (한 번에 여러 주기가 지난 경우)
        while (now >= _nextDecay)
        {
            _nextDecay += DecaySeconds;
            DecayOnce(cracked, removed);
        }
        return new BridgeDecayResult(cracked, removed);
    }

    /// <summary>붕괴 갱신 한 번 (원본 FUN_004227e0 의 수명 동기화)</summary>
    private void DecayOnce(List<BridgeCellState> cracked, List<BridgeCellState> removed)
    {
        // 연결망마다 열린 끝이 있는지 보고 수명을 줄인다
        foreach (IReadOnlyList<BridgeCellState> network in Networks())
        {
            var mortal = network.Where(c => c.Condition != BridgeCondition.Hard).ToList();
            if (mortal.Count == 0 || !network.Any(c => Directions.Any(d => IsOpen(c, d))))
            {
                continue;
            }
            var running = mortal.Where(c => c.TimeLeft > 0).ToList();
            int next = running.Count == 0 ? MaxTimeLeft : running.Min(c => c.TimeLeft) - 1;
            // 연결망의 모든 칸을 같은 수명으로 맞춘다
            foreach (BridgeCellState cell in mortal)
            {
                int before = cell.TimeLeft == 0 ? MaxTimeLeft + 1 : cell.TimeLeft;
                cell.TimeLeft = next;
                if (next <= 0)
                {
                    _cells.Remove((cell.X, cell.Y));
                    removed.Add(cell);
                    Version++;
                }
                else if (next < CrackBelow && before >= CrackBelow && cell.Condition == BridgeCondition.Normal)
                {
                    cell.Condition = BridgeCondition.Cracked;
                    cracked.Add(cell);
                }
            }
        }
    }
}
