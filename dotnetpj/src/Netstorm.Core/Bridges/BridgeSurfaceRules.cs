using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>
/// 붕괴 방문(원본 004218b0)이 읽는 표면 연결 정보. 번호로 이웃 목록과 오브젝트의 종류·프레임 글자를 내준다.
/// <see cref="SurfaceFinder"/> 가 원본 규칙의 구현이고, <see cref="BridgeGrid"/> 는 섬을 칸 단위로 보는 근사 구현을 준다.
/// </summary>
public interface ISurfaceLinks
{
    /// <summary>번호의 오브젝트 (플래그 2 와 프레임 글자를 읽는다)</summary>
    /// <param name="id">오브젝트 번호</param>
    SurfaceObject Object(int id);

    /// <summary>번호의 표면 이웃을 원본 탐색 순서로 돌려준다</summary>
    /// <param name="id">오브젝트 번호</param>
    IReadOnlyList<int> Neighbors(int id);
}

/// <summary>붕괴 방문 목록과 두 진행 플래그 (원본 정적 목록 005453c0, 전역 005453a0·005453a4)</summary>
/// <param name="Visited">수명을 맞출 다리 번호 목록 (시작 칸 포함, 중복 가능, 최대 용량까지)</param>
/// <param name="ShortJunction">시작 칸 바로 옆 접합 칸 가운데 이웃이 3개 미만인 것이 있는지</param>
/// <param name="CanDecay">붕괴가 진행되는지</param>
/// <param name="Complete">
/// 방문을 끝까지 했는지. 원본은 접합 칸 고리에서 끝없이 재귀한다. 새 코드는 고리나 안전 깊이에서 멈추고 거짓을 돌려주며,
/// 이때는 수명을 바꾸지 않는다 (cpppj 와 같은 안전 처리이고 원본 동작이 아니다).
/// </param>
public sealed record BridgeDecayWalk(IReadOnlyList<int> Visited, bool ShortJunction, bool CanDecay, bool Complete);

/// <summary>
/// 다리·표면의 연결 판정, 열린 방향, 붕괴 방문 목록. 원본 10.78 의 00441e40·00421770·004217f0·004218b0 을 옮겼다
/// (docs/exe/cpp-bridge-reconstruction.md, cpp-surface-reconstruction.md. 기준 구현은 cpppj/src/o/Bridge.cpp).
/// 표면 지도·오브젝트 번호를 입력으로 받는 계산이며 월드 상태를 바꾸지 않는다.
/// 검증은 원본 기계어 기대값 surface-x86.tsv(Connect·Neighbor·Collect)와 bridge-x86.tsv(Open)로 한다.
/// </summary>
public static class BridgeSurfaceRules
{
    /// <summary>방향 수: 북 0 부터 시계 방향으로 북동 1, 동 2, … 북서 7</summary>
    public const int DirectionCount = 8;

    /// <summary>원본 정적 방문 목록의 용량 (생성 인자 0x64)</summary>
    public const int VisitedCapacity = 100;

    /// <summary>
    /// 원본의 고리 재귀에는 깊이 제한이 없다. 스택 고갈을 막으려고 새 코드가 둔 한계이며 원본 상수가 아니다 (cpppj 와 같은 값).
    /// </summary>
    public const int SafeRecursionDepth = 256;

    /// <summary>
    /// 표면 조회의 칸 변환에 더하는 값. 원본 00500edc 의 실제 float 비트는 3f7ff972 다 (0.9999 에 가장 가까운 단정밀도).
    /// </summary>
    public const float SurfaceCoordinateBias = 0.9999f;

    /// <summary>원본 00441e40 의 타입 플래그 2 마스크: 다리·폭탄</summary>
    public const uint BridgeOrBombMask = TypeFlagBits.Bridge | TypeFlagBits.Bomb;

    /// <summary>원본 00441e40 의 타입 플래그 2 마스크: 섬·신전·작업장·포대류·3×3 섬 받침</summary>
    public const uint LandObjectMask = 0x1044202;

    /// <summary>접합 칸의 마지막 방향 글자: 'A'~'I' 는 갈래·모퉁이, 'J' 부터는 판자·끝 칸·빈 칸 (원본 비교 글자 &lt; 0x4a)</summary>
    private const char LastJunctionLetter = 'I';

    /// <summary>원본 0052f8dc: 여덟 방향이 요구하는 연결 비트 (북 1, 동 2, 남 4, 서 8). 대각 방향은 시계 방향 다음 비트를 쓴다</summary>
    private static readonly BridgeLinks[] DirectionBits =
    [
        BridgeLinks.North, BridgeLinks.East, BridgeLinks.East, BridgeLinks.South,
        BridgeLinks.South, BridgeLinks.West, BridgeLinks.West, BridgeLinks.North,
    ];

    /// <summary>여덟 방향의 이웃 칸 좌표 차이 (북부터 시계 방향)</summary>
    private static readonly (int Dx, int Dy)[] NeighborCells =
        [(0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1)];

    /// <summary>
    /// 두 표면이 한 방향으로 이어지는지 (원본 00441e40). 소유자·동맹·배치 가능 여부와는 무관한 타입·프레임 계산이다.
    /// 짝수 방향은 프레임의 측면 글자, 홀수(대각) 방향은 변형 글자를 본다. 포대류(<see cref="TypeFlagBits.Emplacement"/>)는
    /// 글자를 'P' 로 만들고, 다리·폭탄과 섬·건물군이 만나면 섬·건물 쪽을 'A'(사방)로 본다. 이 보정의 순서와
    /// 먼저 맞은 쪽만 적용하는 우선순위도 원본 그대로다.
    /// </summary>
    /// <param name="first">방향의 출발 쪽 프레임</param>
    /// <param name="firstFlags2">출발 쪽 타입 플래그 2</param>
    /// <param name="second">도착 쪽 프레임</param>
    /// <param name="secondFlags2">도착 쪽 타입 플래그 2</param>
    /// <param name="direction">출발 → 도착 방향 0~7</param>
    public static bool Connects(FrameCode first, uint firstFlags2, FrameCode second, uint secondFlags2, int direction)
    {
        if (direction < 0 || direction >= DirectionCount)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), direction, "방향은 0~7 이어야 합니다");
        }
        char a = direction % 2 == 0 ? first.Side : first.Variant;
        char b = direction % 2 == 0 ? second.Side : second.Variant;
        if ((firstFlags2 & TypeFlagBits.Emplacement) != 0)
        {
            a = BridgeDirections.LastLetter;
        }
        if ((secondFlags2 & TypeFlagBits.Emplacement) != 0)
        {
            b = BridgeDirections.LastLetter;
        }
        if ((firstFlags2 & BridgeOrBombMask) != 0 && (secondFlags2 & LandObjectMask) != 0)
        {
            b = BridgeDirections.FirstLetter;
        }
        else if ((secondFlags2 & BridgeOrBombMask) != 0 && (firstFlags2 & LandObjectMask) != 0)
        {
            a = BridgeDirections.FirstLetter;
        }
        if (!BridgeDirections.IsLetter(a) || !BridgeDirections.IsLetter(b))
        {
            // 원본은 표 범위 밖을 읽는다. 새 코드는 그 읽기를 흉내 내지 않고 입력 오류로 알린다
            throw new ArgumentOutOfRangeException(nameof(first), $"{a}/{b}", "연결 글자는 'A'~'P' 여야 합니다");
        }
        return (BridgeDirections.ToLinks(a) & DirectionBits[direction]) != 0
            && (BridgeDirections.ToLinks(b) & DirectionBits[(direction + 4) % DirectionCount]) != 0;
    }

    /// <summary>
    /// 한 방향이 열려 있는지 (원본 00421770): 그 방향으로 연결이 있고, 그쪽 칸의 표면 번호가 이웃 목록에 없으면 열린 것이다.
    /// 지도 밖이나 번호 0 은 열린 것으로 본다. 칸은 좌표에 방향 차이를 더한 뒤 0.9999 를 더하고 0 쪽으로 잘라 고른다.
    /// </summary>
    /// <param name="side">다리 칸의 방향 글자 'A'~'P'</param>
    /// <param name="direction">방향 0~7</param>
    /// <param name="x">칸의 x 좌표</param>
    /// <param name="y">칸의 y 좌표</param>
    /// <param name="surface">256×256 표면 번호 지도</param>
    /// <param name="neighbors">이 칸의 이웃 번호 목록</param>
    public static bool IsOpen(char side, int direction, float x, float y, ReadOnlySpan<ushort> surface, IReadOnlyCollection<int> neighbors)
    {
        if (direction < 0 || direction >= DirectionCount || surface.Length != SurfaceFinder.WorldCells * SurfaceFinder.WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), direction, "방향 또는 표면 지도 크기가 잘못됐습니다");
        }
        if ((BridgeDirections.ToLinks(side) & DirectionBits[direction]) == 0)
        {
            return false;
        }
        // 방향 차이는 단정밀도로 더한다 (원본 fadd). 그 뒤의 0.9999 덧셈만 넓은 정밀도다
        int cellX = CellCoordinate((float)(x + NeighborCells[direction].Dx));
        int cellY = CellCoordinate((float)(y + NeighborCells[direction].Dy));
        if (cellX < 0 || cellY < 0 || cellX >= SurfaceFinder.WorldCells || cellY >= SurfaceFinder.WorldCells)
        {
            return true;
        }
        int id = surface[cellY * SurfaceFinder.WorldCells + cellX];
        return id == 0 || !neighbors.Contains(id);
    }

    /// <summary>
    /// 붕괴를 이끄는 열린 방향 (원본 004217f0). 북 0 → 동 2 → 남 4 → 서 6 순서로 검사한다.
    /// 세 갈래 B~E 는 두 번째, 모퉁이·판자 F~K 는 첫 번째 열린 방향을 만난 즉시 돌려준다.
    /// 끝 칸 L·M·N·O 는 고정 방향 0·2·4·6 이고, 사방 A 와 빈 칸 P 는 −1(열린 방향 없음)이다.
    /// </summary>
    /// <param name="side">다리 칸의 방향 글자 'A'~'P'</param>
    /// <param name="isOpen">짝수 방향 하나가 열려 있는지 알려 주는 판정</param>
    public static int OpenDirection(char side, Func<int, bool> isOpen)
    {
        int required = RequiredOpenSides(side);
        int found = 0;
        // 네 방향을 북부터 시계 방향으로 보며 필요한 개수째 열린 방향에서 멈춘다
        for (int direction = 0; required != 0 && direction < DirectionCount; direction += 2)
        {
            if (isOpen(direction) && ++found == required)
            {
                return direction;
            }
        }
        return FixedOpenDirection(side);
    }

    /// <summary>표면 지도와 이웃 목록으로 열린 방향을 구한다 (<see cref="OpenDirection(char, Func{int, bool})"/> 와 같은 규칙)</summary>
    /// <param name="side">다리 칸의 방향 글자 'A'~'P'</param>
    /// <param name="x">칸의 x 좌표</param>
    /// <param name="y">칸의 y 좌표</param>
    /// <param name="surface">256×256 표면 번호 지도</param>
    /// <param name="neighbors">이 칸의 이웃 번호 목록</param>
    public static int OpenDirection(char side, float x, float y, ReadOnlySpan<ushort> surface, IReadOnlyCollection<int> neighbors)
    {
        if (surface.Length != SurfaceFinder.WorldCells * SurfaceFinder.WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(surface), "표면 지도는 256×256 이어야 합니다");
        }
        int required = RequiredOpenSides(side);
        int found = 0;
        // 네 방향을 북부터 시계 방향으로 보며 필요한 개수째 열린 방향에서 멈춘다 (Span 은 람다로 넘길 수 없어 같은 반복을 둔다)
        for (int direction = 0; required != 0 && direction < DirectionCount; direction += 2)
        {
            if (IsOpen(side, direction, x, y, surface, neighbors) && ++found == required)
            {
                return direction;
            }
        }
        return FixedOpenDirection(side);
    }

    /// <summary>열린 방향이 몇 개째일 때 붕괴를 이끄는지: 세 갈래 B~E 는 2, 모퉁이·판자 F~K 는 1, 그 밖은 0(검사하지 않음)</summary>
    private static int RequiredOpenSides(char side)
    {
        if (!BridgeDirections.IsLetter(side))
        {
            throw new ArgumentOutOfRangeException(nameof(side), side, "방향 글자는 'A'~'P' 여야 합니다");
        }
        return side switch
        {
            >= 'B' and <= 'E' => 2,
            >= 'F' and <= 'K' => 1,
            _ => 0,
        };
    }

    /// <summary>열린 방향을 찾지 못했을 때의 값: 끝 칸 L·M·N·O 는 0·2·4·6, 그 밖은 −1</summary>
    private static int FixedOpenDirection(char side) => side switch
    {
        'L' => 0,
        'M' => 2,
        'N' => 4,
        'O' => 6,
        _ => -1,
    };

    /// <summary>
    /// 붕괴 방문 목록을 만든다 (원본 004218b0). 시작 다리에서 접합 칸(A~I)으로만 재귀하고 판자·끝 칸·빈 칸·섬은 경계다.
    /// - 다리·섬인 이웃만 보고, 바로 앞에서 들어온 칸(부모)만 건너뛴다. 다리 이웃은 용량이 남으면 목록에 넣는다.
    ///   목록이 차도 재귀와 경계 처리는 계속한다.
    /// - 접합 칸이면 재귀한다. 깊이 0 에서 그 칸의 이웃이 3개 미만이면 짧은 접합 플래그를 켠다.
    /// - 경계 이웃: 깊이 0, 또는 깊이 1 이고 짧은 접합이면 붕괴 진행 + 그 번호의 모든 항목을 목록에서 뺀다.
    ///   그 밖에는 다리 이웃 수가 2 미만이면 붕괴 진행(목록에 남는다), 2 이상이면 목록에서만 뺀다. 섬은 이웃 수 0 으로 본다.
    /// 목록은 방문 집합이 아니다. 같은 번호가 여러 번 들어갈 수 있다.
    /// </summary>
    /// <param name="surfaces">표면 연결 정보</param>
    /// <param name="root">시작 다리 번호</param>
    /// <param name="capacity">방문 목록 용량 (1~100)</param>
    public static BridgeDecayWalk CollectDecay(ISurfaceLinks surfaces, int root, int capacity = VisitedCapacity)
    {
        SurfaceObject start = surfaces.Object(root);
        if (start.Dead || (start.Flags2 & TypeFlagBits.Bridge) == 0 || capacity <= 0 || capacity > VisitedCapacity)
        {
            throw new ArgumentOutOfRangeException(nameof(root), root, "붕괴 방문의 시작 칸 또는 용량이 잘못됐습니다");
        }
        var walk = new Walk(surfaces, capacity);
        walk.Visited.Add(root);
        walk.Visit(root, 0, 0);
        return new BridgeDecayWalk(walk.Visited, walk.ShortJunction, walk.CanDecay, walk.Complete);
    }

    /// <summary>
    /// 좌표를 칸 번호로 바꾼다 (원본 0040eaf0 의 어셈블리: 0.9999 를 더하고 _ftol). x87 의 중간값처럼 단정밀도로 다시 좁히지 않는다.
    /// 정수 경계 바로 아래 값이 다음 칸으로 넘어가면 안 되기 때문이다.
    /// </summary>
    private static int CellCoordinate(float value)
    {
        double shifted = (double)value + (double)SurfaceCoordinateBias;
        if (!double.IsFinite(shifted) || shifted < int.MinValue || shifted > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "다리 좌표가 범위를 벗어났습니다");
        }
        return (int)shifted;
    }

    /// <summary>붕괴 방문 한 번의 진행 상태</summary>
    private sealed class Walk(ISurfaceLinks surfaces, int capacity)
    {
        /// <summary>지금 재귀 경로에 있는 번호 (고리 확인용)</summary>
        private readonly List<int> _active = [];

        /// <summary>방문 목록</summary>
        public List<int> Visited { get; } = [];

        /// <summary>짧은 접합 플래그 (원본 005453a0)</summary>
        public bool ShortJunction { get; private set; }

        /// <summary>붕괴 진행 플래그 (원본 005453a4)</summary>
        public bool CanDecay { get; private set; }

        /// <summary>고리·깊이 한계 없이 끝까지 방문했는지</summary>
        public bool Complete { get; private set; } = true;

        /// <summary>한 칸의 이웃을 원본 순서로 처리한다</summary>
        /// <param name="id">방문 중인 번호</param>
        /// <param name="depth">재귀 깊이 (시작 칸 = 0)</param>
        /// <param name="parent">바로 앞에서 들어온 번호 (시작 칸은 0)</param>
        public void Visit(int id, int depth, int parent)
        {
            if (depth >= SafeRecursionDepth || _active.Contains(id))
            {
                Complete = false;
                CanDecay = false;
                return;
            }
            _active.Add(id);
            // 탐색기 순서와 중복 추가·전체 삭제를 그대로 따른다
            foreach (int neighbor in surfaces.Neighbors(id))
            {
                SurfaceObject item = surfaces.Object(neighbor);
                bool bridge = (item.Flags2 & TypeFlagBits.Bridge) != 0;
                bool island = (item.Flags2 & TypeFlagBits.Island) != 0;
                if ((!bridge && !island) || neighbor == parent)
                {
                    continue;
                }
                if (bridge && Visited.Count < capacity)
                {
                    Visited.Add(neighbor);
                }
                if (item.Frame.Side <= LastJunctionLetter && !island)
                {
                    if (depth == 0 && surfaces.Neighbors(neighbor).Count < 3)
                    {
                        ShortJunction = true;
                    }
                    Visit(neighbor, depth + 1, id);
                    if (!Complete)
                    {
                        break;
                    }
                }
                else if (depth == 0 || (depth == 1 && ShortJunction))
                {
                    CanDecay = true;
                    Visited.RemoveAll(entry => entry == neighbor);
                }
                else
                {
                    int count = bridge ? surfaces.Neighbors(neighbor).Count : 0;
                    if (count < 2)
                    {
                        CanDecay = true;
                    }
                    else
                    {
                        Visited.RemoveAll(entry => entry == neighbor);
                    }
                }
            }
            _active.RemoveAt(_active.Count - 1);
        }
    }
}
