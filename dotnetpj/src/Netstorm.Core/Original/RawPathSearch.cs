// 현재 표면·Graph·접합과 원문 작업 저장소를 동일한 경로 탐색기에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawPathSearch.cpp의 실제 BFS/Graph/역추적 처리기다.</summary>
public sealed partial class RawPathSearch
{
    /// <summary>원문 북쪽부터 시계 방향으로 이동하는 가로 단위다.</summary>
    private static readonly int[] DirectionX = [0, 1, 1, 1, 0, -1, -1, -1];
    /// <summary>원문 북쪽부터 시계 방향으로 이동하는 세로 단위다.</summary>
    private static readonly int[] DirectionY = [-1, -1, 0, 1, 1, 1, 0, -1];
    /// <summary>원문 두 BYTE 큐의 마지막 항목이다.</summary>
    private const int QueueMask = 0x3fff;
    /// <summary>표면과 부모가 함께 사용하는 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 hash0를 복사 없이 읽는 실제 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>같은65536칸 점유 바이트다.</summary>
    private readonly byte[] _spots;
    /// <summary>현재 원문 TYPE 속성을 빌린다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 물리 프레임의 side/variant를 읽는다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>방향 전제/방문 순서/현재 정책의 같은 owner다.</summary>
    private readonly RawPathSearchState _state;
    /// <summary>실제 장기 배열 소유자이며 독립 검사에서는 없다.</summary>
    private readonly RawPathSearchStorage? _sharedWork;
    /// <summary>이 생성자가 확보하거나 기존 owner에서 빌린 같은 작업 배열이다.</summary>
    public RawPathSearchWork Work { get; }
    /// <summary>월드 참조를 빌리고 장기 작업 배열을 재사용한다. 초기화 표식은 실제 검색에서만 쓴다.</summary>
    public RawPathSearch(RawSidPool pool, RawSquidHash hash, byte[] spots, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, RawPathSearchState state, RawPathSearchStorage? sharedWork = null)
    {
        Pool = pool; _hash = hash; _spots = spots; _types = types; _frames = frames; _state = state; _sharedWork = sharedWork;
        Work = sharedWork?.Work ?? new(); if (sharedWork is not null) sharedWork.Work = Work;
        if (spots.Length != 65536 || types.Count != frames.Count) throw new ArgumentException("BFS 실제 월드 연결");
    }
    /// <summary>큰 작업 배열을 추가 생성하지 않는 기존 자유 표면 getter다.</summary>
    public int SurfaceAt(RawPathPoint point) => RawPathSurface.SurfaceAt(_hash, point);
    /// <summary>현재 표면/예외TYPE의 실제 Graph BYTE를 읽고254를 없음으로 돌려준다.</summary>
    public byte? GraphAt(RawPathPoint point)
    {
        if (!_state.GraphsEnabled) return null; int sid = SurfaceAt(point); if (sid == 0) return null;
        ReadOnlySpan<byte> raw = Pool.Slot(sid); int type = raw[10];
        if (type >= _types.Count || Pool.Edition == OriginalEdition.Patch1078 && (_types[type].Flags1 & OriginalTypeRecord.Surface) == 0 && type != _state.FenceMarkType && type != _state.WindArcherType) throw new InvalidOperationException("BFS Graph getter 표면 타입");
        byte graph = raw[Pool.Edition == OriginalEdition.Patch1078 ? 30 : 28]; return graph == 254 ? null : graph;
    }
    /// <summary>같은SID는 즉시 연결하고 다른SID만 현재 프레임/부류로 기존 접합을 호출한다.</summary>
    public bool Connects(int first, int second, int direction)
    {
        if (first == 0 || second == 0) return false; if (first == second) return true;
        return RawBridgeConnects.Connects(Frame(first), _types[Pool.Slot(first)[10]].Flags2, Frame(second), _types[Pool.Slot(second)[10]].Flags2, direction);
    }
    /// <summary>PATCH signedDWORD/CD BYTE의 물리 프레임만 조회한다.</summary>
    private RiftFrameCode Frame(int sid)
    {
        int type = Pool.Slot(sid)[10], frame = RawPathSlot.Frame(Pool, sid);
        if (type >= _frames.Count || frame < 0 || frame >= _frames[type].Codes.Count) throw new ArgumentOutOfRangeException(nameof(sid), "BFS 표면 프레임"); return _frames[type].Codes[frame];
    }
    /// <summary>원문 좌표0..255의 보드 범위다.</summary>
    private static bool Inside(int x, int y) => RawPathSurface.Inside(x, y);
    /// <summary>현재 방향 전역을 실제 소비 위치에서만 검증한다.</summary>
    private int Direction(int index)
    {
        int value = _state.DirectionOrder[index]; if (value < 0 || value > 7) throw new ArgumentOutOfRangeException(nameof(index), "BFS 방향 전역"); return value;
    }
    /// <summary>기존 prefix 뒤에 좌표/방향을 더하고 원문 최대 길이 진단을 수행한다.</summary>
    private void Append(List<RawPathPoint> points, List<int> directions, RawPathPoint point, int direction, RawPathRuntime output)
    {
        points.Add(point); directions.Add(direction); RawPathLayout layout = RawPathRuntime.LayoutFor(Pool.Edition);
        if (points.Count >= layout.Capacity || output.Bytes.Length != layout.Size) throw new ArgumentOutOfRangeException(nameof(output), "BFS 최대 경로 길이");
    }
    /// <summary>목적지/시작의 현재 Graph를 비교한 뒤 목적→시작 BFS로 실제 start→dest 배열을 만든다.</summary>
    public bool Search(RawPathRuntime output, int parent, RawPathPoint start, RawPathPoint destination, uint flags)
    {
        if (Pool.Slot(parent)[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(parent), "BFS 부모 타입");
        byte? destinationGraph = GraphAt(destination); if (!destinationGraph.HasValue || destinationGraph != GraphAt(start)) return false;
        int sx = RawPathSurface.Cell(destination.X), sy = RawPathSurface.Cell(destination.Y), dx = RawPathSurface.Cell(start.X), dy = RawPathSurface.Cell(start.Y);
        if (!Inside(sx, sy) || !Inside(dx, dy) || (_spots[dy * 256 + dx] & 6) == 0) return false;
        if (!_state.SearchEnabled) throw new InvalidOperationException("원본 kenGo 비활성 진단"); return SearchCells(output, sx, sy, dx, dy, (flags & 1) != 0);
    }
    /// <summary>같은Pool의 실제 탐색만 Search 경계에 붙이고 나머지 실제 공간/수명 함수 값은 유지한다.</summary>
    public static RawPathProcessHooks MakeHooks(RawSidPool pool, RawPathSearch search, RawPathProcessHooks hooks)
    {
        if (!ReferenceEquals(pool, search.Pool)) throw new ArgumentException("Path 검색/프로세스 SID 풀 불일치");
        return hooks with { Search = (output, parent, start, destination, flags) => _ = search.Search(output, parent, start, destination, flags) };
    }
}
