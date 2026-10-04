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

    /// <summary>
    /// 칸이 격자에 들어온 순서(1부터). 원본은 붕괴 스캔이 오브젝트 번호 순으로 칸을 훑는데, 번호 재사용 규칙을 몰라서
    /// 만든 순서로 대신한다(근사). 격자가 붙이므로 직접 바꾸지 않는다.
    /// </summary>
    public int Sequence { get; internal set; }
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
/// exe 로 확인한 부분: 10초 주기 스캔(Bridge.cpp FUN_00422bc0, 상수 0x52f968 = 10.0)이 칸마다 한 번씩 붕괴 처리(FUN_004227e0)를 하고,
/// 그 처리를 칸 단위로 그대로 옮겼다(2026-09-30): 표면 연결 그래프가 5칸 미만이면 제거(Graph.cpp), 단단한 칸 제외,
/// 이웃이 없으면 수명 −1, 열린 쪽이 없는 칸은 구동자가 아님(FUN_004217f0), 구동자에서 접합 칸(A~I)만 따라 방문 목록을 만들어
/// 0이 아닌 최소 수명 − 1(없으면 7)로 맞춤(FUN_004218b0), 수명 0~7·금 감 기준 5(0x52f960)·0 이면 제거(FUN_00421c30).
/// 겹치는 칸이 있으면 배치 불가(Rifttype.cpp FUN_0049b510).
/// 근사한 부분: "섬 가장자리 또는 내 다리의 열린 끝에 이어짐"(원본은 영역 소유 판정 FUN_0048fdb0), 칸 처리 순서(만든 순서),
/// 섬 오브젝트를 칸 단위로 봄(원본은 여러 칸 오브젝트가 하나), 초목 가장자리 제외는 호출자 판정.
/// </summary>
public sealed class BridgeGrid
{
    /// <summary>붕괴 갱신 간격(초) (원본 0x52f968)</summary>
    public const double DecaySeconds = 10.0;

    /// <summary>수명 최대값 (원본 brMAX_TIME_LEFT)</summary>
    public const int MaxTimeLeft = 7;

    /// <summary>이 값보다 작아지면 금 간 상태가 된다 (원본 DAT_0052f960 = 5)</summary>
    public const int CrackBelow = 5;

    /// <summary>금 간 채로 놓이거나 약화된 칸의 수명 (원본 DAT_0052f960 − 1 = 4, FUN_00442c80·FUN_00421db0)</summary>
    public const int WeakenedTimeLeft = CrackBelow - 1;

    /// <summary>건물형 유닛이 없어질 때 약화하는 범위: 중심 칸에서 가로·세로 이 칸 수까지 (원본 FUN_00422230(…, 2, 2, 0))</summary>
    public const int WeakenRadius = 2;

    /// <summary>월드 칸 크기 (원본 BOARDDIM_IN_TILES = 256)</summary>
    public const int WorldSize = 256;

    /// <summary>
    /// 붕괴 스캔에서 칸이 속한 표면 연결 그래프가 이 수보다 작으면 그 칸을 곧바로 제거한다
    /// (원본 FUN_004227e0: 그래프 기록 numSurface &lt; 5, Graph.cpp). 섬에서 떨어져 나온 작은 다리 조각이 한 번에 무너지는 이유다.
    /// </summary>
    public const int MinSurfaceGraphSize = 5;

    /// <summary>붕괴 스캔 방문 목록의 최대 길이 (원본 정적 객체 0x5453c0 를 인자 0x64 로 만든다)</summary>
    public const int VisitedCapacity = 100;

    /// <summary>접합 칸의 마지막 방향 글자: 'A'~'I' 는 갈래·모퉁이(접합), 'J'~'P' 는 판자·끝 칸·빈 칸 (원본 글자 코드 &lt; 0x4a)</summary>
    private const char LastJunctionLetter = 'I';

    /// <summary>네 방향 (북·동·남·서)</summary>
    private static readonly BridgeLinks[] Directions = [BridgeLinks.North, BridgeLinks.East, BridgeLinks.South, BridgeLinks.West];

    /// <summary>이웃을 찾는 순서: 북 → 서 → 동 → 남 (원본 FUN_004b1c20 이 칸 둘레를 훑는 순서. 붕괴 판정의 일부 조건이 이 순서에 의존한다)</summary>
    private static readonly BridgeLinks[] ScanOrder = [BridgeLinks.North, BridgeLinks.West, BridgeLinks.East, BridgeLinks.South];

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

    /// <summary>마지막으로 붙인 칸 순번 (<see cref="BridgeCellState.Sequence"/>)</summary>
    private int _lastSequence;

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

    /// <summary>섬 판정 콜백의 동적 받침이 생기거나 사라졌음을 알려 이동 경로를 재탐색하게 한다.</summary>
    public void InvalidateTerrain() => Version++;

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
            state.Sequence = ++_lastSequence;
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

    /// <summary>
    /// 조각을 놓는다 (판정은 호출자가 먼저 한다). 칸의 시작 상태는 조각 품질로 정한다
    /// (원본 Construction.cpp FUN_00442c80, 품질 인자 param_10):
    /// 금 감(품질 1) = 금 간 프레임·수명 4(<see cref="CrackBelow"/> − 1), 단단함(품질 3) = 단단한 프레임,
    /// 그 밖(보통) = 보통 프레임·수명 0. 원본은 끝 칸 글자(L~O)면 품질과 관계없이 수명 4 로 두지만 생산 창 조각에는 끝 칸이 없다.
    /// </summary>
    /// <param name="piece">회전된 조각</param>
    /// <param name="originX">조각 왼쪽 위 칸 x</param>
    /// <param name="originY">조각 왼쪽 위 칸 y</param>
    /// <param name="player">소유 플레이어</param>
    /// <param name="quality">조각 품질 (<see cref="BridgeTray.QualityAt"/>, 기본 보통)</param>
    public IReadOnlyList<BridgeCellState> Place(BridgePiece piece, int originX, int originY, int player,
        BridgeCondition quality = BridgeCondition.Normal)
    {
        var placed = new List<BridgeCellState>();
        // 회전된 칸마다 상태를 만들어 격자에 넣는다
        foreach (PlacedBridgeCell cell in piece.Cells())
        {
            var state = new BridgeCellState(originX + cell.Dx, originY + cell.Dy, cell.Cell, player)
            {
                Condition = quality,
                TimeLeft = quality == BridgeCondition.Cracked ? WeakenedTimeLeft : 0,
                Sequence = ++_lastSequence,
            };
            _cells[(state.X, state.Y)] = state;
            placed.Add(state);
        }
        Version++;
        return placed;
    }

    /// <summary>
    /// 건물형 유닛이 없어질 때 주변 다리를 한 단계 약화한다 (원본 전투 오브젝트 공통 처리 FUN_0044b9e0, vtable 슬롯 0x18).
    /// 중심 칸 ±<see cref="WeakenRadius"/> 칸 사각형(FUN_00422230(중심, 2, 2))의 다리 칸마다:
    /// 금 간 칸은 무너지고, 보통 칸은 금 간 상태·수명 4 가 되며(FUN_00421db0), 단단한 칸은 그대로다(FUN_00422560).
    /// 원본은 이미 늦춰진 낙하 예약(이벤트 0x2692, FUN_00421fe0)이 있는 금 간 칸은 무너뜨리지 않는데, 클론에는 그 예약이 없다.
    /// </summary>
    /// <param name="centerX">없어진 오브젝트의 중심 칸 x (원본은 실수 중심을 소수점 버림)</param>
    /// <param name="centerY">중심 칸 y</param>
    public BridgeDecayResult WeakenAround(int centerX, int centerY)
    {
        var cracked = new List<BridgeCellState>();
        var removed = new List<BridgeCellState>();
        // 사각형 안의 칸을 위쪽 행·왼쪽 칸 순서로 처리해 결과 순서를 일정하게 한다
        for (int y = centerY - WeakenRadius; y <= centerY + WeakenRadius; y++)
        {
            // 한 행의 칸을 왼쪽부터 처리한다
            for (int x = centerX - WeakenRadius; x <= centerX + WeakenRadius; x++)
            {
                if (!_cells.TryGetValue((x, y), out BridgeCellState? cell) || cell.Condition == BridgeCondition.Hard)
                {
                    continue;
                }
                if (cell.Condition == BridgeCondition.Cracked)
                {
                    _cells.Remove((x, y));
                    removed.Add(cell);
                    Version++;
                }
                else
                {
                    cell.Condition = BridgeCondition.Cracked;
                    cell.TimeLeft = WeakenedTimeLeft;
                    cracked.Add(cell);
                }
            }
        }
        return new BridgeDecayResult(cracked, removed);
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
    /// 게임 시각을 진행한다. 10초마다 모든 다리 칸을 만든 순서대로 한 번씩 붕괴 처리한다(<see cref="DecayOnce"/>).
    /// 원본은 오브젝트 번호 범위를 10초에 걸쳐 나누어 훑으며(Bridge.cpp FUN_00422bc0), 클론은 10초 경계에서 한 번에 처리한다.
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

    /// <summary>붕괴 스캔 한 번: 스캔이 시작될 때 있던 칸을 만든 순서대로 처리한다. 스캔 중 사라진 칸은 건너뛴다.</summary>
    private void DecayOnce(List<BridgeCellState> cracked, List<BridgeCellState> removed)
    {
        // 칸마다 원본 FUN_004227e0 의 처리를 한 번씩 한다
        foreach (BridgeCellState cell in _cells.Values.OrderBy(c => c.Sequence).ToArray())
        {
            if (IsAlive(cell))
            {
                ScanCell(cell, cracked, removed);
            }
        }
    }

    /// <summary>칸이 아직 격자에 있는지 (같은 좌표의 다른 칸이 아닌 그 칸 자신)</summary>
    private bool IsAlive(BridgeCellState cell) =>
        _cells.TryGetValue((cell.X, cell.Y), out BridgeCellState? current) && ReferenceEquals(current, cell);

    /// <summary>칸을 격자에서 없앤다 (원본 오브젝트 제거)</summary>
    private void Kill(BridgeCellState cell, List<BridgeCellState> removed)
    {
        _cells.Remove((cell.X, cell.Y));
        removed.Add(cell);
        Version++;
    }

    /// <summary>
    /// 칸 하나의 붕괴 처리 (원본 Bridge.cpp FUN_004227e0). 이 칸이 "구동자"가 되어 자기와 이어진 접합 칸들의 수명을 줄인다.
    /// 1. 표면 연결 그래프가 <see cref="MinSurfaceGraphSize"/> 칸 미만이면 곧바로 제거한다.
    /// 2. 단단한 칸은 아무것도 하지 않는다.
    /// 3. 이웃이 하나도 없으면 수명을 1 줄인다 (그래프 조건 때문에 실제로는 거의 닿지 않는다).
    /// 4. 열린 쪽이 없는 칸은 구동자가 아니다 (<see cref="HasOpenSide"/>) — 양쪽이 이어진 판자는 수명이 줄지 않는다.
    /// 5. 방문 목록을 만들고(<see cref="Walk"/>) 붕괴 조건이 서면 목록 칸들의 수명을 "0 이 아닌 최소값 − 1"(없으면 7)로 맞춘다.
    /// </summary>
    private void ScanCell(BridgeCellState cell, List<BridgeCellState> cracked, List<BridgeCellState> removed)
    {
        if (!SurfaceGraphReaches(cell, MinSurfaceGraphSize))
        {
            Kill(cell, removed);
            return;
        }
        if (cell.Condition == BridgeCondition.Hard)
        {
            return;
        }
        List<Neighbor> neighbors = NeighborsOf(cell);
        if (neighbors.Count == 0)
        {
            LowerTimeLeft(cell, 1, cracked, removed);
            return;
        }
        if (!HasOpenSide(cell, neighbors))
        {
            return;
        }
        var walk = new CollapseWalk();
        walk.Visited.Add(cell);
        Walk(cell, 0, null, walk);
        if (!walk.CanCollapse)
        {
            return;
        }
        // 방문 목록에서 0 이 아닌 수명의 최소값을 찾는다 (목록의 칸은 만든 뒤 처음 스캔되면 0 이다)
        int lowest = int.MaxValue;
        foreach (BridgeCellState visited in walk.Visited)
        {
            if (visited.TimeLeft != 0 && visited.TimeLeft < lowest)
            {
                lowest = visited.TimeLeft;
            }
        }
        int next = lowest == int.MaxValue ? MaxTimeLeft : Math.Min(lowest, MaxTimeLeft) - 1;
        // 목록의 모든 칸을 같은 수명으로 맞춘다 (단단한 접합 칸도 수명은 바뀌며 프레임만 그대로다, 중복 항목은 두 번째부터 변화가 없다)
        foreach (BridgeCellState visited in walk.Visited.ToArray())
        {
            if (IsAlive(visited))
            {
                LowerTimeLeft(visited, visited.TimeLeft - next, cracked, removed);
            }
        }
    }

    /// <summary>
    /// 수명을 줄인다 (원본 FUN_00421c30). 0 이 되면 칸을 없애고, 5 이상에서 5 미만으로 내려가는 순간 보통 칸이 금 간 상태가 된다.
    /// 수명이 0 이던 칸이 곧바로 5 미만으로 맞춰질 때는 금이 가지 않는다 (원본이 "이전 수명 ≥ 5" 를 요구한다).
    /// 이 수명 이하(5)에서 칸 위 이동체가 떨어지는 처리는 아직 없다.
    /// </summary>
    /// <param name="cell">칸</param>
    /// <param name="delta">줄일 양 (음수면 늘어난다)</param>
    /// <param name="cracked">금이 간 칸 목록</param>
    /// <param name="removed">사라진 칸 목록</param>
    private void LowerTimeLeft(BridgeCellState cell, int delta, List<BridgeCellState> cracked, List<BridgeCellState> removed)
    {
        int before = cell.TimeLeft;
        int after = Math.Max(0, before - delta);
        if (after == 0)
        {
            Kill(cell, removed);
            return;
        }
        cell.TimeLeft = after;
        if (after < CrackBelow && before >= CrackBelow && cell.Condition == BridgeCondition.Normal)
        {
            cell.Condition = BridgeCondition.Cracked;
            cracked.Add(cell);
        }
    }

    /// <summary>
    /// 칸의 이웃 (원본 FUN_004b2660 → 탐색기 FUN_004b1e80/FUN_00441e40): 북·서·동·남 칸의 표면 오브젝트 가운데
    /// 이 칸이 그쪽으로 연결되어 있고, 다리라면 마주 보는 연결도 있는 것. 섬 칸은 늘 이어진다(원본은 섬 쪽 글자를 'A' 로 본다).
    /// </summary>
    /// <param name="cell">다리 칸</param>
    private List<Neighbor> NeighborsOf(BridgeCellState cell)
    {
        var result = new List<Neighbor>(4);
        // 이 칸이 이어지는 방향만 순서대로 살핀다
        foreach (BridgeLinks direction in ScanOrder)
        {
            if (!cell.Cell.Links.HasFlag(direction))
            {
                continue;
            }
            (int dx, int dy) = BridgeDirections.Offset(direction);
            (int nx, int ny) = (cell.X + dx, cell.Y + dy);
            if (_cells.TryGetValue((nx, ny), out BridgeCellState? other))
            {
                if (other.Cell.Links.HasFlag(BridgeDirections.Opposite(direction)))
                {
                    result.Add(new Neighbor(nx, ny, other));
                }
            }
            else if (_isIsland(nx, ny))
            {
                result.Add(new Neighbor(nx, ny, null));
            }
        }
        return result;
    }

    /// <summary>
    /// 구동자가 될 수 있는지: 이어지는 방향 가운데 이웃 목록에 없는 "열린 쪽" (원본 FUN_004217f0·FUN_00421770).
    /// 갈래 B~E 는 열린 쪽이 2개 이상, 모퉁이·판자 F~K 는 1개 이상 있어야 하고, 끝 칸 L~O 는 늘 열려 있으며, A·P 는 열린 쪽이 없다.
    /// </summary>
    /// <param name="cell">다리 칸</param>
    /// <param name="neighbors">이 칸의 이웃</param>
    private bool HasOpenSide(BridgeCellState cell, List<Neighbor> neighbors)
    {
        char letter = cell.Cell.Letter;
        if (letter is >= 'L' and <= 'O')
        {
            return true;
        }
        int needed = letter switch
        {
            >= 'B' and <= 'E' => 2,
            >= 'F' and <= 'K' => 1,
            _ => int.MaxValue,
        };
        int open = 0;
        // 이어지는 방향마다 그쪽 칸의 오브젝트가 이웃이 아니면 열린 쪽으로 센다
        foreach (BridgeLinks direction in Directions)
        {
            if (!cell.Cell.Links.HasFlag(direction))
            {
                continue;
            }
            (int dx, int dy) = BridgeDirections.Offset(direction);
            (int nx, int ny) = (cell.X + dx, cell.Y + dy);
            bool joined = neighbors.Any(n => n.X == nx && n.Y == ny);
            if (!joined)
            {
                open++;
            }
        }
        return open >= needed;
    }

    /// <summary>
    /// 붕괴 방문 (원본 FUN_004218b0): <paramref name="cell"/> 의 이웃을 훑으며 접합 칸(A~I)으로만 재귀해 방문 목록을 만든다.
    /// 판자(J·K)·끝 칸·섬은 경계다. 경계 이웃이 어떤 조건이면 붕괴가 진행되고(<see cref="CollapseWalk.CanCollapse"/>), 목록에서 빠진다:
    /// 깊이 0, 또는 깊이 1 이고 "이웃이 3개 미만인 접합 칸"이 이미 있을 때는 항상 붕괴 진행 + 목록에서 제외.
    /// 그 밖에는 이웃이 2개 미만인 다리(자기 쪽에서만 이어진 끝)면 붕괴 진행 + 목록에 남고, 이웃이 2개 이상이면 목록에서만 빠진다.
    /// 섬은 다리가 아니므로 이웃 수가 0 으로 계산되어 늘 붕괴 진행이다.
    /// </summary>
    /// <param name="cell">방문 중인 칸</param>
    /// <param name="depth">재귀 깊이 (구동자 = 0)</param>
    /// <param name="parent">바로 앞에서 들어온 칸 (구동자는 null)</param>
    /// <param name="walk">방문 상태</param>
    private void Walk(BridgeCellState cell, int depth, BridgeCellState? parent, CollapseWalk walk)
    {
        walk.OnPath.Add(cell);
        // 이웃마다 앞에서 들어온 칸은 건너뛴다
        foreach (Neighbor neighbor in NeighborsOf(cell))
        {
            if (neighbor.Bridge is { } bridge && (ReferenceEquals(bridge, parent) || walk.OnPath.Contains(bridge)))
            {
                // 원본은 접합 칸이 고리를 이루면 끝없이 재귀한다. 지금 재귀 경로에 있는 칸은 건너뛰어 막는다.
                continue;
            }
            if (neighbor.Bridge is { } added && walk.Visited.Count < VisitedCapacity)
            {
                walk.Visited.Add(added);
            }
            bool junction = neighbor.Bridge is { } candidate && candidate.Cell.Letter <= LastJunctionLetter;
            if (junction)
            {
                BridgeCellState next = neighbor.Bridge!;
                if (depth == 0 && NeighborsOf(next).Count < 3)
                {
                    walk.ShortJunction = true;
                }
                Walk(next, depth + 1, cell, walk);
            }
            else if (depth == 0 || (depth == 1 && walk.ShortJunction))
            {
                walk.CanCollapse = true;
                RemoveFromVisited(walk, neighbor.Bridge);
            }
            else
            {
                int count = neighbor.Bridge is { } other ? NeighborsOf(other).Count : 0;
                if (count < 2)
                {
                    walk.CanCollapse = true;
                }
                else
                {
                    RemoveFromVisited(walk, neighbor.Bridge);
                }
            }
        }
        walk.OnPath.Remove(cell);
    }

    /// <summary>방문 목록에서 그 칸의 모든 항목을 뺀다 (섬이면 목록에 없으므로 아무 일도 없다)</summary>
    private static void RemoveFromVisited(CollapseWalk walk, BridgeCellState? bridge)
    {
        if (bridge != null)
        {
            walk.Visited.RemoveAll(v => ReferenceEquals(v, bridge));
        }
    }

    /// <summary>
    /// 칸이 속한 표면 연결 그래프의 크기가 <paramref name="size"/> 이상인지 (원본 Graph.cpp 의 numSurface 검사).
    /// 그래프는 다리 칸과 섬 칸을 이웃 규칙(<see cref="SurfacesConnect"/>)으로 이은 무리이고, 크기는 그 오브젝트 수다.
    /// 필요한 수를 채우면 곧바로 멈춘다.
    /// </summary>
    /// <param name="start">시작 다리 칸</param>
    /// <param name="size">필요한 최소 크기</param>
    private bool SurfaceGraphReaches(BridgeCellState start, int size)
    {
        var seen = new HashSet<(int X, int Y)> { (start.X, start.Y) };
        var queue = new Queue<(int X, int Y)>([(start.X, start.Y)]);
        // 필요한 크기에 이르거나 이을 칸이 없을 때까지 너비 우선으로 넓힌다
        while (queue.Count > 0 && seen.Count < size)
        {
            (int x, int y) = queue.Dequeue();
            foreach (BridgeLinks direction in Directions)
            {
                (int dx, int dy) = BridgeDirections.Offset(direction);
                (int nx, int ny) = (x + dx, y + dy);
                if (!seen.Contains((nx, ny)) && SurfacesConnect(x, y, direction))
                {
                    seen.Add((nx, ny));
                    queue.Enqueue((nx, ny));
                }
            }
        }
        return seen.Count >= size;
    }

    /// <summary>
    /// 표면 칸 (x, y) 와 그 <paramref name="direction"/> 쪽 이웃 칸이 서로 이어지는지: 다리끼리는 마주 연결,
    /// 다리와 섬은 다리 쪽 연결, 섬끼리는 인접이면 이어진다. 표면이 아닌 칸은 이어지지 않는다.
    /// </summary>
    private bool SurfacesConnect(int x, int y, BridgeLinks direction)
    {
        (int dx, int dy) = BridgeDirections.Offset(direction);
        (int nx, int ny) = (x + dx, y + dy);
        bool hereBridge = _cells.TryGetValue((x, y), out BridgeCellState? here);
        bool thereBridge = _cells.TryGetValue((nx, ny), out BridgeCellState? there);
        if (hereBridge && thereBridge)
        {
            return here!.Cell.Links.HasFlag(direction) && there!.Cell.Links.HasFlag(BridgeDirections.Opposite(direction));
        }
        if (hereBridge)
        {
            return _isIsland(nx, ny) && here!.Cell.Links.HasFlag(direction);
        }
        if (thereBridge)
        {
            return _isIsland(x, y) && there!.Cell.Links.HasFlag(BridgeDirections.Opposite(direction));
        }
        return _isIsland(x, y) && _isIsland(nx, ny);
    }

    /// <summary>다리 칸의 이웃 하나: 이웃 칸 좌표와 다리 칸 (섬 칸이면 null)</summary>
    private readonly record struct Neighbor(int X, int Y, BridgeCellState? Bridge);

    /// <summary>붕괴 방문 상태: 방문 목록, 재귀 경로, 붕괴 진행 여부, 짧은 접합 칸 여부 (원본 전역 0x5453c0·0x5453a4·0x5453a0)</summary>
    private sealed class CollapseWalk
    {
        /// <summary>수명을 맞출 칸 목록 (구동자 포함, 중복 가능, 최대 <see cref="VisitedCapacity"/>)</summary>
        public List<BridgeCellState> Visited { get; } = [];

        /// <summary>지금 재귀 경로에 있는 칸 (고리 방지, 참조 비교)</summary>
        public HashSet<BridgeCellState> OnPath { get; } = new(ReferenceEqualityComparer.Instance);

        /// <summary>붕괴가 진행되는지 (원본 DAT_005453a4)</summary>
        public bool CanCollapse { get; set; }

        /// <summary>구동자 바로 옆 접합 칸 가운데 이웃이 3개 미만인 것이 있는지 (원본 DAT_005453a0)</summary>
        public bool ShortJunction { get; set; }
    }
}
