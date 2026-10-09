using Netstorm.Assets;

namespace Netstorm.Core.Bridges;

/// <summary>
/// 표면 이웃 탐색이 읽는 오브젝트 하나의 정수 발자국 스냅샷. 원본 Squid/타입 구조체에서 탐색에 쓰이는 필드만 옮겼다
/// (docs/exe/cpp-surface-reconstruction.md, cpppj/src/o/SquidFinder.h 의 SurfaceObject).
/// </summary>
/// <param name="Id">오브젝트 번호 (1~32767)</param>
/// <param name="X">기준점 칸 x. 기준점은 발자국의 오른쪽 아래 칸이다</param>
/// <param name="Y">기준점 칸 y</param>
/// <param name="Width">발자국 가로 칸 수</param>
/// <param name="Height">발자국 세로 칸 수</param>
/// <param name="Flags1">타입 플래그 1 (<see cref="TypeFlagBits.Surface"/> 를 본다)</param>
/// <param name="Flags2">타입 플래그 2 (섬·다리·건물군 비트)</param>
/// <param name="Frame">현재 프레임의 코드 (방향·변형 글자)</param>
/// <param name="Dead">죽음 비트가 켜졌는지</param>
/// <param name="Buried">매장 비트가 켜졌는지 (이 탐색 경로는 읽지 않고 보존만 한다)</param>
public readonly record struct SurfaceObject(int Id, int X, int Y, int Width, int Height, uint Flags1, uint Flags2,
    FrameCode Frame, bool Dead = false, bool Buried = false);

/// <summary>
/// 원본 SquidFinder 의 표면 이웃 탐색(flag 8 경로)을 월드와 분리한 계산으로 옮긴 것.
/// 원본 004b23e0·004b1d70·004b1c20·004b1e80(10.78)의 규칙이며 기준 구현은 cpppj/src/o/SquidFinder.cpp 다.
/// 입력은 "칸마다 표면 오브젝트 번호를 담은 256×256 지도"와 같은 크기의 spot 바이트 지도다.
/// 발자국이 지도 안에 놓인 정수 스냅샷만 지원한다. 이동 중 소수 좌표·일반 공간 해시 경로는 다루지 않는다.
/// </summary>
public sealed class SurfaceFinder : ISurfaceLinks
{
    /// <summary>월드 한 변의 칸 수 (원본 BOARDDIM_IN_TILES)</summary>
    public const int WorldCells = 256;

    /// <summary>spot 비트 8: 건물군 발자국의 안쪽 칸 (docs/exe/cpp-hash-reconstruction.md)</summary>
    public const byte InteriorBit = 8;

    /// <summary>원본 이웃 캐시가 부호 있는 16비트로 비교하는 오브젝트 번호의 상한</summary>
    public const int MaximumSurfaceId = 32767;

    /// <summary>번호 → 오브젝트</summary>
    private readonly Dictionary<int, SurfaceObject> _objects = [];

    /// <summary>칸(y × 256 + x) → 표면 오브젝트 번호 (0 은 없음)</summary>
    private readonly ushort[] _map;

    /// <summary>칸(y × 256 + x) → spot 비트</summary>
    private readonly byte[] _spots;

    /// <summary>표면 스냅샷을 만든다. 지도와 spot 은 복사하므로 만든 뒤 입력이 바뀌어도 결과가 달라지지 않는다.</summary>
    /// <param name="objects">표면 오브젝트 목록 (번호 중복 불가)</param>
    /// <param name="map">256×256 표면 번호 지도</param>
    /// <param name="spots">256×256 spot 바이트 지도</param>
    public SurfaceFinder(IEnumerable<SurfaceObject> objects, ReadOnlySpan<ushort> map, ReadOnlySpan<byte> spots)
    {
        if (map.Length != WorldCells * WorldCells || spots.Length != map.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(map), "표면 지도와 spot 지도는 256×256 이어야 합니다");
        }
        _map = map.ToArray();
        _spots = spots.ToArray();
        // 번호 범위·발자국·중복 번호를 확인한 뒤 입력 필드를 그대로 보존한다
        foreach (SurfaceObject item in objects)
        {
            if (item.Id <= 0 || item.Id > MaximumSurfaceId || item.Width < 1 || item.Height < 1
                || item.X < item.Width - 1 || item.Y < item.Height - 1 || item.X >= WorldCells || item.Y >= WorldCells
                || !_objects.TryAdd(item.Id, item))
            {
                throw new ArgumentOutOfRangeException(nameof(objects), $"표면 오브젝트의 번호 또는 발자국이 잘못됐습니다: {item.Id}");
            }
        }
    }

    /// <summary>열린 방향 검사(원본 00421770)가 읽는 256×256 표면 번호 지도</summary>
    public ReadOnlySpan<ushort> Map => _map;

    /// <summary>번호로 오브젝트를 찾는다. 지도에 적힌 번호가 목록에 없으면 입력 오류다.</summary>
    /// <param name="id">오브젝트 번호</param>
    public SurfaceObject Object(int id) => _objects.TryGetValue(id, out SurfaceObject found)
        ? found : throw new KeyNotFoundException($"표면 오브젝트가 없습니다: {id}");

    /// <summary>기준점 한 칸의 spot 에 안쪽 비트가 있는지 (전역 그래프 재구성이 읽는 값)</summary>
    /// <param name="id">오브젝트 번호</param>
    public bool OriginInterior(int id)
    {
        SurfaceObject item = Object(id);
        return (_spots[item.Y * WorldCells + item.X] & InteriorBit) != 0;
    }

    /// <summary>
    /// 두 표면이 프레임 글자로 이어지는지. 방향은 from 중심에서 to 중심 쪽 네 방향 가운데 하나다
    /// (<see cref="DirectionBetween"/>).
    /// </summary>
    /// <param name="from">방향의 출발 쪽</param>
    /// <param name="to">방향의 도착 쪽</param>
    public static bool Connects(SurfaceObject from, SurfaceObject to) =>
        BridgeSurfaceRules.Connects(from.Frame, from.Flags2, to.Frame, to.Flags2, DirectionBetween(from, to));

    /// <summary>
    /// 두 발자국 중심의 차이로 고른 방향 (북 0, 동 2, 남 4, 서 6). 원본 004adea0·0041ce90:
    /// 중심은 (x − 폭 × 0.5, y − 높이 × 0.5)이고, 가로·세로 차이의 절댓값이 같으면 세로가 먼저다.
    /// </summary>
    /// <param name="from">출발 쪽</param>
    /// <param name="to">도착 쪽</param>
    public static int DirectionBetween(SurfaceObject from, SurfaceObject to)
    {
        float x = to.X - to.Width * 0.5f - (from.X - from.Width * 0.5f);
        float y = to.Y - to.Height * 0.5f - (from.Y - from.Height * 0.5f);
        if (MathF.Abs(x) <= MathF.Abs(y))
        {
            return y > 0 ? 4 : 0;
        }
        return x <= 0 ? 6 : 2;
    }

    /// <summary>
    /// 오브젝트의 표면 이웃 번호 목록 (원본 순서 그대로, 같은 오브젝트는 한 번만).
    /// 1. 자기 발자국의 spot 을 모두 AND 해 안쪽 비트가 남으면 이웃이 없다.
    /// 2. 발자국을 한 칸 넓힌 사각형을 위쪽 행부터, 행 안에서는 왼쪽부터 읽고 네 모서리는 읽지 않는다
    ///    (1×1 이면 북 → 서 → 자기 → 동 → 남).
    /// 3. 후보의 기준점이 자기 발자국 안이면 제외한다. 표면 플래그·죽음 비트·후보 기준점의 안쪽 비트를 본다.
    /// 4. 후보 → 자기 방향으로 프레임 글자가 이어져야 한다.
    /// </summary>
    /// <param name="id">오브젝트 번호</param>
    public IReadOnlyList<int> Neighbors(int id)
    {
        SurfaceObject source = Object(id);
        var result = new List<int>();
        if (Interior(source))
        {
            return result;
        }
        int left = source.X - source.Width + 1;
        int top = source.Y - source.Height + 1;
        // 한 칸 넓힌 사각형의 행을 위에서 아래로 훑는다
        for (int y = top - 1; y <= source.Y + 1; y++)
        {
            int inset = y == top - 1 || y == source.Y + 1 ? 1 : 0;
            // 맨 위·아래 행은 양 끝 모서리를 빼고 왼쪽에서 오른쪽으로 읽는다
            for (int x = left - 1 + inset; x <= source.X + 1 - inset; x++)
            {
                if (x < 0 || y < 0 || x >= WorldCells || y >= WorldCells)
                {
                    continue;
                }
                int candidate = _map[y * WorldCells + x];
                if (candidate == 0)
                {
                    continue;
                }
                SurfaceObject other = Object(candidate);
                if (left <= other.X && other.X <= source.X && top <= other.Y && other.Y <= source.Y)
                {
                    continue;
                }
                if ((other.Flags1 & TypeFlagBits.Surface) == 0 || other.Dead
                    || (_spots[other.Y * WorldCells + other.X] & InteriorBit) != 0)
                {
                    continue;
                }
                if (Connects(other, source) && !result.Contains(candidate))
                {
                    result.Add(candidate);
                }
            }
        }
        return result;
    }

    /// <summary>
    /// 발자국 전체가 안쪽인지 (원본 004ab880): 사각형의 각 좌표를 1~255 로 자른 뒤 모든 spot 바이트를 AND 한다.
    /// 지도 가장자리 0 번 줄이 1 로 잘리는 것도 원본 그대로다.
    /// </summary>
    private bool Interior(SurfaceObject item)
    {
        int left = Math.Clamp(item.X - item.Width + 1, 1, WorldCells - 1);
        int right = Math.Clamp(item.X, 1, WorldCells - 1);
        int top = Math.Clamp(item.Y - item.Height + 1, 1, WorldCells - 1);
        int bottom = Math.Clamp(item.Y, 1, WorldCells - 1);
        int bits = 0xffff;
        // 사각형의 행을 위에서 아래로 훑는다
        for (int y = top; y <= bottom; y++)
        {
            // 한 행의 모든 칸을 AND 한다
            for (int x = left; x <= right; x++)
            {
                bits &= _spots[y * WorldCells + x];
            }
        }
        return (bits & InteriorBit) != 0;
    }
}
