// Terrainbuilder.cpp의 저장 영역 지면 생성과 월드 칸 격자(cpppj o/TerrainBuilder.h/.cpp)를 복원한다.
// 2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 이식했다.
namespace Netstorm.Core.Original;

/// <summary>
/// 지상 이동에 쓰는 논리 칸이다. 표시와 경로 탐색이 같은 지지 정보를 공유한다
/// (cpppj <c>o/TerrainBuilder.h</c>의 <c>GroundCell</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class GroundCell
{
    /// <summary>칸별 지면 여부다.</summary>
    public bool Land { get; set; }

    /// <summary>다리 여부다.</summary>
    public bool Bridge { get; set; }

    /// <summary>객체나 지역을 소유한 플레이어 번호다.</summary>
    public int Owner { get; set; }

    /// <summary>현재 칸을 점유한 객체의 런타임 번호다.</summary>
    public uint Occupant { get; set; }
}

/// <summary>
/// 월드 격자의 정수 X·Y 칸 좌표다(cpppj <c>o/TerrainBuilder.h</c>의 <c>CellPoint</c>).
/// </summary>
/// <param name="X">X 좌표다.</param>
/// <param name="Y">Y 좌표다.</param>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public readonly record struct CellPoint(int X, int Y);

/// <summary>
/// 영역과 청크 패턴을 월드 지면 타일로 펼치는 처리기다(cpppj <c>o/TerrainBuilder</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainBuilder
{
    /// <summary>원본 월드의 한 변은 16청크 × 16칸이다.</summary>
    public const int WorldCells = ChunkMapShape.Side * ChunkMapShape.CellsPerChunk;

    /// <summary>본섬 영역 마스크에서 허공을 나타내는 검사 스트림의 값이다.</summary>
    public const byte EmptyTerrain = 255;

    /// <summary>0052f83c/0052f85c의 북부터 시계 방향인 여덟 이웃이다.</summary>
    public static readonly (int Dx, int Dy)[] NeighborCells =
    [
        (0, -1),
        (1, -1),
        (1, 0),
        (1, 1),
        (0, 1),
        (-1, 1),
        (-1, 0),
        (-1, -1),
    ];

    /// <summary>원본 방향 A~P의 연결 비트 표다.</summary>
    private static readonly int[] Connections = [15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0];

    /// <summary>청크별 난수 시드 증가량이다.</summary>
    private const uint ChunkSeedStep = 0x10e3u;

    /// <summary>성장 실패가 연속되는 최대 횟수(004c0800)다.</summary>
    private const int GrowthRetries = 1000;

    /// <summary>칸별 지면 번호 또는 지면 타일 자료다.</summary>
    private readonly byte[] _land = new byte[WorldCells * WorldCells];

    /// <summary>
    /// 지형 생성 중 현재 위치·섬·진행량·연결 정보를 보관하는 작업 항목이다.
    /// </summary>
    private readonly record struct WorkItem(
        int X,
        int Y,
        int Island,
        int Index,
        int Count,
        int Target,
        byte Territory,
        byte Seed,
        int ConnectionsMask);

    /// <summary>
    /// 004c10a0: 전체 연결 통로 → 청크별 성장 → 두 차례 빈틈 보정 순서를 보존해 본섬 마스크를 생성한다.
    /// </summary>
    /// <param name="map">청크 지도다.</param>
    /// <param name="islands">섬 목록이다.</param>
    public TerrainBuilder(ChunkMap map, IslandList islands)
    {
        ArgumentNullException.ThrowIfNull(map);
        ArgumentNullException.ThrowIfNull(islands);
        Array.Fill(_land, EmptyTerrain);
        List<WorkItem> work = [];
        int[] counts = new int[128];
        int[] indices = new int[128];

        // 통로와 성장에 쓸 영역별 청크 수를 먼저 계산한다.
        for (int y = 0; y < map.YLength(); y++)
        {
            // 월드 x 순서로 유효한 섬 소속을 센다.
            for (int x = 0; x < map.XLength(); x++)
            {
                byte island = map.At(x, y).Island;
                if (islands.Exists(island))
                {
                    counts[island]++;
                }
            }
        }

        // 통로 생성 순서는 월드 y·x다.
        for (int y = 0; y < map.YLength(); y++)
        {
            // 영역 번호 순으로 묶지 않는다.
            for (int x = 0; x < map.XLength(); x++)
            {
                ref ChunkEntry chunk = ref map.At(x, y);
                if (!islands.Exists(chunk.Island))
                {
                    continue;
                }
                byte region = (byte)islands.Islands()[chunk.Island].Territory;
                int index = indices[chunk.Island]++;
                int mask = Connection((char)(byte)chunk.Side);
                int left = x * 16;
                int top = y * 16;
                byte seed = chunk.Record[3];
                int cx = left + 4 + (seed & 7);
                int cy = top + 4 + ((9999 - seed) & 7);
                int added = Fill(cx - 1, cy - 1, cx + 3, cy + 3, region);
                added += Fill(
                    (mask & 8) != 0 ? left : cx,
                    (mask & 1) != 0 ? top : cy,
                    (mask & 2) != 0 ? left + 16 : cx + 2,
                    (mask & 4) != 0 ? top + 16 : cy + 2,
                    region);
                if (index == 1)
                {
                    added += Fill(left + 3, top + 3, left + 13, top + 13, region);
                }
                uint state = unchecked((uint)index * ChunkSeedStep + seed);
                int target = Math.Max(20, Next(ref state, 30) + 50 - added * 100 / 256) * 256 / 100;
                work.Add(new WorkItem(x, y, chunk.Island, index, counts[chunk.Island], target, region, seed, mask));
            }
        }

        // 모든 통로가 있는 상태에서 3×3 덩어리를 성장시킨다.
        foreach (WorkItem chunk in work)
        {
            uint state = unchecked((uint)(chunk.Count + chunk.Index) * ChunkSeedStep + chunk.Seed);
            int remaining = chunk.Target;
            int retries = GrowthRetries;
            // 연속 실패 한도 또는 목표량에서 끝난다.
            while (remaining > 0 && retries > 0)
            {
                retries--;
                int x = chunk.X * 16 + Next(ref state, 15);
                int y = chunk.Y * 16 + Next(ref state, 15);
                Next(ref state, 1);
                if (At(x, y) != chunk.Territory)
                {
                    continue;
                }
                int added = 0;
                // 성장 덩어리의 행을 먼저 순회한다.
                for (int dy = -1; dy <= 1; dy++)
                {
                    // 다른 영역 청크로는 성장시키지 않는다.
                    for (int dx = -1; dx <= 1; dx++)
                    {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && ny >= 0 && nx < WorldCells && ny < WorldCells
                            && map.At(nx / 16, ny / 16).Island == chunk.Island)
                        {
                            added += Put(nx, ny, chunk.Territory);
                        }
                    }
                }
                if (added != 0)
                {
                    remaining -= added;
                    retries = GrowthRetries;
                }
            }
        }

        // 두 번의 순회 중 추가 칸을 즉시 반영한다.
        for (int pass = 0; pass < 2; pass++)
        {
            // 월드 바깥 한 줄은 조사하지 않는다.
            for (int y = 1; y < WorldCells - 1; y++)
            {
                // 직선 이웃이 있는 빈 칸만 조사한다.
                for (int x = 1; x < WorldCells - 1; x++)
                {
                    if (At(x, y) != EmptyTerrain
                        || (At(x, y - 1) == EmptyTerrain && At(x + 1, y) == EmptyTerrain && At(x, y + 1) == EmptyTerrain && At(x - 1, y) == EmptyTerrain))
                    {
                        continue;
                    }
                    int run = 0;
                    byte owner = EmptyTerrain;
                    // 북서·북의 경계에 걸친 연속 이웃도 확인한다.
                    for (int direction = 0; direction < 13; direction++)
                    {
                        (int dx, int dy) = NeighborCells[direction & 7];
                        byte value = At(x + dx, y + dy);
                        if (value == EmptyTerrain)
                        {
                            run = 0;
                            continue;
                        }
                        if (run == 0)
                        {
                            owner = value;
                        }
                        if (value != owner)
                        {
                            run = 0;
                            continue;
                        }
                        if (++run > 4 && (direction & 1) != 0)
                        {
                            Put(x, y, owner);
                            break;
                        }
                    }
                }
            }
        }
    }

    /// <summary>
    /// 월드 밖과 허공은 255이며 지면은 원본 저장 영역 번호다.
    /// </summary>
    /// <param name="x">X 좌표다.</param>
    /// <param name="y">Y 좌표다.</param>
    /// <returns>해당 칸의 영역 번호 또는 허공(255)이다.</returns>
    public byte At(int x, int y)
    {
        return x >= 0 && y >= 0 && x < WorldCells && y < WorldCells
            ? _land[y * WorldCells + x]
            : EmptyTerrain;
    }

    /// <summary>
    /// 독립 마스크 대조용 y·x 순서의 65,536바이트다.
    /// </summary>
    /// <returns>생성된 마스크의 읽기 전용 스팬이다.</returns>
    public ReadOnlySpan<byte> Mask() => _land;

    /// <summary>
    /// 004bca20: 32비트 오버플로를 사용하는 정수 난수다. limit은 양수여야 한다.
    /// </summary>
    /// <param name="state">난수 상태 참조다.</param>
    /// <param name="limit">양수 상한이다.</param>
    /// <returns>0 이상 limit 미만의 난수 값이다.</returns>
    public static int Next(ref uint state, int limit)
    {
        if (limit <= 0)
        {
            throw new ArgumentException("Terrain random limit must be positive", nameof(limit));
        }
        if (state == 0)
        {
            state = 0x0bad0badu;
        }
        state = unchecked(state * 0x10003u + 3u);
        return (int)((state >> 16) % (uint)limit);
    }

    /// <summary>
    /// 0052f910의 방향 문자 A~P를 연결 비트로 바꾼다.
    /// </summary>
    /// <param name="side">방향 문자('A'~'P')다.</param>
    /// <returns>4비트 연결 마스크다.</returns>
    public static int Connection(char side)
    {
        if (side < 'A' || side > 'P')
        {
            throw new ArgumentException("Invalid terrain side", nameof(side));
        }
        return Connections[side - 'A'];
    }

    /// <summary>
    /// 0052f8fc의 연결 비트 → 문자 변환이다.
    /// </summary>
    /// <param name="mask">연결 비트 마스크다.</param>
    /// <returns>방향 문자('A'~'P')다.</returns>
    public static char Orientation(int mask) => "PNOILJFBMHKEGDCA"[mask & 15];

    /// <summary>
    /// 0041cd20의 직선/대각선 방향 정규화다.
    /// </summary>
    /// <param name="cardinal">직선 방향 문자다.</param>
    /// <param name="diagonal">대각선 방향 문자다.</param>
    /// <returns>정규화된 (직선, 대각선) 문자 쌍이다.</returns>
    public static (char Cardinal, char Diagonal) Normalize(char cardinal, char diagonal)
    {
        int a = Connection(cardinal);
        int b = Connection(diagonal);
        int supported = 0;
        int result = 0;
        // 북서·북동·남동·남서의 지지 쌍을 조사한다.
        for (int i = 0; i < 4; i++)
        {
            int pair = (1 << i) | (1 << ((i + 3) & 3));
            if ((a & pair) == pair)
            {
                supported |= 1 << i;
            }
        }
        b &= supported;
        // 남은 대각선의 직선 지지 비트를 합한다.
        for (int i = 0; i < 4; i++)
        {
            if ((b & (1 << i)) != 0)
            {
                result |= (1 << i) | (1 << ((i + 3) & 3));
            }
        }
        return (Orientation(result), Orientation(b));
    }

    /// <summary>빈 칸에 지면을 넣고 실제 추가 여부를 돌려준다.</summary>
    private int Put(int x, int y, byte territory)
    {
        if (x < 0 || y < 0 || x >= WorldCells || y >= WorldCells || At(x, y) != EmptyTerrain)
        {
            return 0;
        }
        _land[y * WorldCells + x] = territory;
        return 1;
    }

    /// <summary>004c0930: x 바깥·y 안쪽 순서로 반열린 사각형을 채운다.</summary>
    private int Fill(int left, int top, int right, int bottom, byte territory)
    {
        int count = 0;
        // 사각형의 열을 먼저 순회한다.
        for (int x = left; x < right; x++)
        {
            // 그 열의 행을 채운다.
            for (int y = top; y < bottom; y++)
            {
                count += Put(x, y, territory);
            }
        }
        return count;
    }
}

/// <summary>
/// 이동·점유·소유권 조회에 사용하는 월드 칸 격자다(cpppj <c>o/TerrainBuilder.h</c>의 <c>GroundGrid</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class GroundGrid
{
    /// <summary>방향 동률은 대각선을 먼저 선택하는 관찰 기반 순서다.</summary>
    private static readonly int[] PathDirections = [1, 3, 5, 7, 0, 2, 4, 6];

    /// <summary>좌표별로 조회·갱신할 격자 칸 배열이다.</summary>
    private readonly GroundCell[] _cells = new GroundCell[TerrainBuilder.WorldCells * TerrainBuilder.WorldCells];

    /// <summary>기본 격자 칸 배열을 초기화한다.</summary>
    public GroundGrid()
    {
        // 모든 칸 인스턴스를 미리 할당한다.
        for (int i = 0; i < _cells.Length; i++)
        {
            _cells[i] = new GroundCell();
        }
    }

    /// <summary>
    /// 월드 밖은 null이며 가장자리로 끌어당기지 않는다.
    /// </summary>
    /// <param name="x">X 좌표다.</param>
    /// <param name="y">Y 좌표다.</param>
    /// <returns>해당 좌표의 칸 객체 또는 null이다.</returns>
    public GroundCell? At(int x, int y)
    {
        return x >= 0 && y >= 0 && x < TerrainBuilder.WorldCells && y < TerrainBuilder.WorldCells
            ? _cells[y * TerrainBuilder.WorldCells + x]
            : null;
    }

    /// <summary>
    /// 8방향 최단 경로 어댑터다. 대각선은 네 칸이 모두 지면이며 통과 가능할 때만 허용하고 시작 칸은 반환 목록에서 뺀다.
    /// </summary>
    /// <param name="from">출발 칸이다.</param>
    /// <param name="to">도착 칸이다.</param>
    /// <param name="mover">이동 객체 번호다.</param>
    /// <param name="allies">동맹 비트 마스크다.</param>
    /// <returns>경로 칸 목록이다.</returns>
    public List<CellPoint> Path(CellPoint from, CellPoint to, uint mover, ushort allies)
    {
        if (!Walkable(from, mover, allies) || !Walkable(to, mover, allies) || from == to)
        {
            return [];
        }
        bool Step(CellPoint a, CellPoint b)
        {
            if (!Walkable(b, mover, allies))
            {
                return false;
            }
            if (a.X == b.X || a.Y == b.Y)
            {
                return true;
            }
            CellPoint c = new(a.X, b.Y);
            CellPoint d = new(b.X, a.Y);
            return At(a.X, a.Y)!.Land
                && At(b.X, b.Y)!.Land
                && Walkable(c, mover, allies)
                && Walkable(d, mover, allies)
                && At(c.X, c.Y)!.Land
                && At(d.X, d.Y)!.Land;
        }
        static int Index(CellPoint p) => p.Y * TerrainBuilder.WorldCells + p.X;
        int[] distance = new int[TerrainBuilder.WorldCells * TerrainBuilder.WorldCells];
        Array.Fill(distance, int.MaxValue);
        PriorityQueue<int, (int Cost, int Index)> queue = new();
        int targetIndex = Index(to);
        distance[targetIndex] = 0;
        queue.Enqueue(targetIndex, (0, targetIndex));

        // 도달 가능한 칸의 최소 비용을 계산한다.
        while (queue.TryDequeue(out int i, out (int Cost, int Index) priority))
        {
            if (priority.Cost != distance[i])
            {
                continue;
            }
            CellPoint p = new(i % TerrainBuilder.WorldCells, i / TerrainBuilder.WorldCells);
            if (p == from)
            {
                break;
            }
            // 원본의 여덟 방향 좌표를 사용한다.
            foreach (int direction in PathDirections)
            {
                (int dx, int dy) = TerrainBuilder.NeighborCells[direction];
                CellPoint next = new(p.X + dx, p.Y + dy);
                if (!Step(p, next))
                {
                    continue;
                }
                int value = priority.Cost + ((direction & 1) != 0 ? 14 : 10);
                int n = Index(next);
                if (value < distance[n])
                {
                    distance[n] = value;
                    queue.Enqueue(n, (value, n));
                }
            }
        }

        List<CellPoint> route = [];
        if (distance[Index(from)] == int.MaxValue)
        {
            return route;
        }

        // 거리가 감소하는 한 칸씩 선택한다.
        for (CellPoint p = from; p != to;)
        {
            bool found = false;
            // 동률의 대각선 걸음을 먼저 선택한다.
            foreach (int direction in PathDirections)
            {
                (int dx, int dy) = TerrainBuilder.NeighborCells[direction];
                CellPoint next = new(p.X + dx, p.Y + dy);
                if (!Step(p, next))
                {
                    continue;
                }
                int remaining = distance[Index(next)];
                if (remaining != int.MaxValue && remaining + ((direction & 1) != 0 ? 14 : 10) == distance[Index(p)])
                {
                    route.Add(next);
                    p = next;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return [];
            }
        }
        return route;
    }

    /// <summary>지면/동맹 다리와 점유 상태로 보행 가능 여부를 판정한다.</summary>
    private bool Walkable(CellPoint p, uint mover, ushort allies)
    {
        GroundCell? cell = At(p.X, p.Y);
        return cell is not null
            && (cell.Occupant == 0 || cell.Occupant == mover)
            && (cell.Land || (cell.Bridge && cell.Owner > 0 && cell.Owner <= 8 && (allies & (1u << cell.Owner)) != 0));
    }
}
