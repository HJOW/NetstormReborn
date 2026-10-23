using Netstorm.Assets;
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

/// <summary>
/// 탐색 플래그 비트(원본 finder +0x48). 8(표면 지도 조회)은 이웃 탐색에서 지원하지 않는다.
/// </summary>
public static class NeighborFlags
{
    /// <summary>래프터 결합 검사를 건너뛴다.</summary>
    public const uint NoJoint = 1;

    /// <summary>표면 타입이 아닌 후보도 받는다.</summary>
    public const uint AnyType = 2;

    /// <summary>abstract(extra 비트 1) 후보를 제외한다.</summary>
    public const uint SkipAbstract = 4;

    /// <summary>이웃 탐색이 지원하는 조합(0~7). 8은 표면 지도 조회라 거부한다.</summary>
    public const uint Supported = NoJoint | AnyType | SkipAbstract;
}

/// <summary>
/// 원본 float 발자국 사각형(원본 004ade70).
/// </summary>
/// <param name="Left">사각형의 왼쪽 경계.</param>
/// <param name="Top">사각형의 위쪽 경계.</param>
/// <param name="Right">사각형의 오른쪽 경계.</param>
/// <param name="Bottom">사각형의 아래쪽 경계.</param>
public readonly record struct SquidFootprint(float Left, float Top, float Right, float Bottom);

/// <summary>
/// 탐색기로 찾은 이웃을 원본 raw 값·발자국·표면에서 고르는 처리기(원본 RawSquidNeighbors, 10.78).
/// 해시 0단계·일반 탐색의 머리·다음 순서대로 원본 생성 순서의 후보를 받는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 추가 바이트·프레임 위치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawSquidNeighbors.cpp (W02 이웃 조각, 2026-10-20).
/// </summary>
public sealed class RawSquidNeighbors
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 현재 x 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 프레임 DWORD의 위치(10.78).</summary>
    private const int FrameOffset = 36;

    /// <summary>상태 바이트의 죽음(dead) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>추가 비트의 추상(abstract) 비트.</summary>
    private const byte AbstractBit = 1;

    /// <summary>점유 바이트의 안쪽(interior) 비트.</summary>
    private const byte InteriorBit = 8;

    /// <summary>원본 발자국 보정 한계. 좌표·칸 모두 이 범위로 보정한다.</summary>
    private const float FirstCell = 1.0f;

    /// <summary>원본 발자국 보정 상한.</summary>
    private const float LastCell = 255.0f;

    /// <summary>후보 기준점의 spot 칸을 고를 때 더하는 값. 같은 값이면 다음 칸을 본다.</summary>
    private const double SpotBias = 0.9999;

    /// <summary>표면 타입 플래그1(원본 TypeFlag1::kSurface).</summary>
    private const uint SurfaceFlag = 0x800;

    /// <summary>섬·다리 부류 마스크(원본 TypeFlag2::kIsland|kBridge). 프레임 크기를 세지 않는다.</summary>
    private const uint IslandBridgeMask = 0x6;

    /// <summary>슬롯을 읽는 풀(읽기 전용으로 쓴다).</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표로 후보를 찾는 공간 해시.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>칸별 점유·비행 비트를 읽는 지도.</summary>
    private readonly IReadOnlyList<byte> _spots;

    /// <summary>타입 번호 → 타입 레코드.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입 번호 → SHP 프레임 목록.</summary>
    private readonly IReadOnlyList<IReadOnlyList<FrameCode>> _frames;

    /// <summary>
    /// 풀·해시·점유 지도·타입 표·프레임 표로 이웃 탐색기를 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="spots">칸별 점유 지도 65,536바이트.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드.</param>
    /// <param name="frames">타입 번호 순서의 프레임 목록. 타입 표와 개수가 같아야 한다.</param>
    public RawSquidNeighbors(RawSidPool pool, RawSquidHash hash, IReadOnlyList<byte> spots, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<IReadOnlyList<FrameCode>> frames)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        if (spots.Count != RawSquidHash.WorldCells * RawSquidHash.WorldCells || types.Count == 0 || types.Count != frames.Count)
        {
            throw new ArgumentException("원본 이웃 지도·타입·프레임 표 오류", nameof(types));
        }
        _pool = pool;
        _hash = hash;
        _spots = spots;
        _types = types;
        _frames = frames;
    }

    /// <summary>이 탐색기가 쓰는 슬롯 풀.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>이 탐색기가 쓰는 공간 해시.</summary>
    public RawSquidHash Hash => _hash;

    /// <summary>
    /// 타입 번호의 SHP 프레임 목록을 돌려준다.
    /// </summary>
    /// <param name="type">타입 번호.</param>
    public IReadOnlyList<FrameCode> Frames(int type) => _frames[type];

    /// <summary>
    /// 생성 직후 +0x34 값이다. 결합 이웃이 없으면 0이다.
    /// </summary>
    /// <param name="source">기준 번호.</param>
    public int First(int source) => new RawSquidNeighborWalk(this, source, 0).Current;

    /// <summary>
    /// 객체의 float 발자국 사각형(원본 004ade70)이다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    public SquidFootprint Footprint(int sid)
    {
        NeighborObject body = ReadObject(sid, false);
        return new SquidFootprint(body.Left, body.Top, body.Right, body.Bottom);
    }

    /// <summary>
    /// 패치에서 발자국 크기를 뺀 중심이다. x87의 double 뺄셈 값을 float 으로 좁힌 값과 같다.
    /// </summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    public float[] Center(int sid)
    {
        NeighborObject body = ReadObject(sid, false);
        return [body.CenterX, body.CenterY];
    }

    /// <summary>
    /// from 에서 to 를 보는 방향(0 위·2 오른·4 아래·6 왼)이다.
    /// 가로 차이가 세로 차이보다 크거나 같으면(비유한 포함) 가로 방향이다.
    /// </summary>
    /// <param name="fromX">출발 x.</param>
    /// <param name="fromY">출발 y.</param>
    /// <param name="toX">도착 x.</param>
    /// <param name="toY">도착 y.</param>
    public static int Direction(float fromX, float fromY, float toX, float toY)
    {
        double dx = (double)toX - (double)fromX;
        float dy = (float)((double)toY - (double)fromY);
        // 세로 절댓값이 가로보다 작지 않으면(같음·비유한 포함) 세로 방향이다
        if (!(Math.Abs((double)dy) < Math.Abs(dx)))
        {
            return dy > 0 ? 4 : 0;
        }
        return dx > 0 ? 2 : 6;
    }

    /// <summary>원본 float 발자국 사각형과 중심 좌표. 정수 좌표·정수 발자국과 달리 부호 바이트를 쓰지 않는다.</summary>
    internal struct NeighborObject
    {
        /// <summary>사각형의 왼쪽·위쪽·오른쪽·아래쪽 경계.</summary>
        public float Left, Top, Right, Bottom;

        /// <summary>중심 x·y.</summary>
        public float CenterX, CenterY;

        /// <summary>타입 표에서 객체 종류를 찾는 번호.</summary>
        public uint Type;

        /// <summary>쓰는 그림 프레임.</summary>
        public FrameCode Frame;
    }

    /// <summary>원본 float 발자국 사각형과 중심 좌표를 읽는다. 이웃 회귀가 공유한다.</summary>
    /// <param name="sid">오브젝트 슬롯 번호.</param>
    /// <param name="readFrame">참이면 현재 프레임도 읽는다.</param>
    internal NeighborObject ReadObject(int sid, bool readFrame)
    {
        if (sid == 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "원본 이웃 번호 오류");
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint number = raw[TypeOffset];
        if (number < (uint)OriginalConstructors.FirstAssetType || number >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), number, "원본 이웃 자산 타입 범위 오류");
        }
        OriginalTypeRecord type = _types[(int)number];
        float x = BitConverter.UInt32BitsToSingle(Read(raw, XOffset));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, YOffset));
        if (!float.IsFinite(x) || !float.IsFinite(y)
            || type.FootX < 1 || type.FootY < 1 || type.FootX > RawSquidHash.WorldCells || type.FootY > RawSquidHash.WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "원본 이웃 좌표·발자국 범위 오류");
        }
        // 모서리 확장은 낮은 좌표가 모두 (0, 256) 밖일 때만 이전 값을 보존한다(원본 49ae80)
        float left = Math.Clamp(x - (type.FootX - 1), FirstCell, LastCell);
        float top = Math.Clamp(y - (type.FootY - 1), FirstCell, LastCell);
        bool valid = x > 0 && x < RawSquidHash.WorldCells && y > 0 && y < RawSquidHash.WorldCells;
        var body = new NeighborObject
        {
            Left = valid ? Math.Min(x, left) : left,
            Top = valid ? Math.Min(y, top) : top,
            Right = valid ? Math.Max(x, left) : left,
            Bottom = valid ? Math.Max(y, top) : top,
            CenterX = x - type.FootX * 0.5f,
            CenterY = y - type.FootY * 0.5f,
            Type = number,
        };
        if (readFrame)
        {
            uint frame = Read(raw, FrameOffset);
            IReadOnlyList<FrameCode> codes = _frames[(int)number];
            if (frame >= (uint)codes.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), frame, "원본 이웃 프레임 범위 오류");
            }
            body.Frame = codes[(int)frame];
        }
        return body;
    }

    /// <summary>원본 발자국 spot 을 AND 한다. 이웃 회귀가 공유한다.</summary>
    /// <param name="source">기준 객체.</param>
    internal bool Interior(in NeighborObject source)
    {
        byte bits = 0xff;
        int left = Math.Clamp((int)source.Left, 1, 255);
        int right = Math.Clamp((int)source.Right, 1, 255);
        int top = Math.Clamp((int)source.Top, 1, 255);
        int bottom = Math.Clamp((int)source.Bottom, 1, 255);
        // 원본처럼 y·x 순서로 발자국을 훑는다
        for (int y = top; y <= bottom; y++)
        {
            // 같은 행의 발자국 칸 spot 값을 AND 로 모은다(모든 칸에 내부 비트가 있어야 마지막까지 남는다)
            for (int x = left; x <= right; x++)
            {
                bits &= _spots[y * RawSquidHash.WorldCells + x];
            }
        }
        return (bits & InteriorBit) != 0;
    }

    /// <summary>표면·교차·결합·죽음·abstract·후보 기준점 spot 으로 후보를 받는다. 이웃 회귀가 공유한다.</summary>
    /// <param name="candidate">후보 번호.</param>
    /// <param name="source">기준 객체.</param>
    /// <param name="flags">호출 플래그.</param>
    internal bool Accept(int candidate, in NeighborObject source, uint flags)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(candidate);
        uint number = raw[TypeOffset];
        // 표면 타입이 아니고 아무 부류나 받지 않으면 거부한다
        if ((_types[(int)number].Flags1 & SurfaceFlag) == 0 && (flags & NeighborFlags.AnyType) == 0)
        {
            return false;
        }
        NeighborObject body = ReadObject(candidate, false);
        NeighborObject src = source;
        // 후보 사각형을 축별로 1칸씩 넓힌 것이 source 와 교차하며, 정확 겹침만 겹쳐야 한다
        bool Intersects(float left, float top, float right, float bottom)
        {
            float x1 = Math.Max(src.Left, left);
            float y1 = Math.Max(src.Top, top);
            float x2 = Math.Min(src.Right, right);
            float y2 = Math.Min(src.Bottom, bottom);
            return x1 >= FirstCell && y1 >= FirstCell && x2 < RawSquidHash.WorldCells && y2 < RawSquidHash.WorldCells && x1 <= x2 && y1 <= y2;
        }
        if (Intersects(body.Left - 1.0f, body.Top, body.Right + 1.0f, body.Bottom)
            == Intersects(body.Left, body.Top - 1.0f, body.Right, body.Bottom + 1.0f))
        {
            return false;
        }
        if ((flags & NeighborFlags.NoJoint) == 0)
        {
            NeighborObject framed = ReadObject(candidate, true);
            // 후보 중심에서 source 중심을 보는 방향으로 결합을 검사한다
            int direction = Direction(framed.CenterX, framed.CenterY, source.CenterX, source.CenterY);
            if (!BridgeSurfaceRules.Connects(framed.Frame, _types[(int)number].Flags2, source.Frame, _types[(int)source.Type].Flags2, direction))
            {
                return false;
            }
        }
        if ((raw[StateOffset] & DeadBit) != 0)
        {
            return false;
        }
        // abstract 후보는 제외한다
        if ((flags & NeighborFlags.SkipAbstract) != 0 && (raw[ExtraOffset] & AbstractBit) != 0)
        {
            return false;
        }
        // 후보 기준점의 spot 칸이 다음 칸이면 그 칸을 보고, 배열 밖만 거부한다
        int index = BiasedCell(body.Bottom) * RawSquidHash.WorldCells + BiasedCell(body.Right);
        if (index >= _spots.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(candidate), "원본 이웃 spot 범위 오류");
        }
        return (_spots[index] & InteriorBit) == 0;
    }

    /// <summary>원본 CRT 절삭과 같이 0 방향으로 버린 칸 번호를 돌려준다.</summary>
    /// <param name="value">자를 실수 좌표.</param>
    private static int BiasedCell(float value) => (int)((double)value + SpotBias);

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset) => Read(bytes, offset, 4);

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }

    /// <summary>이웃 탐색기가 쓰는 타입 표.</summary>
    public IReadOnlyList<OriginalTypeRecord> Types => _types;
}

/// <summary>
/// 결합 이웃 탐색기의 회귀(원본 RawSquidNeighborWalk).
/// 생성 직후 Current()가 첫 이웃(원본 +0x34)이며, Next()로 다음 후보를 받는다.
/// </summary>
public sealed class RawSquidNeighborWalk
{
    /// <summary>탐색기. 안쪽이면 만들지 않는다.</summary>
    private readonly RawSquidFinder? _finder;

    /// <summary>현재 처리 위치. 0이면 끝이다.</summary>
    private int _current;

    /// <summary>
    /// 이웃 탐색기·기준 번호·플래그로 회귀를 만든다. 플래그는 0~7 조합이다.
    /// </summary>
    /// <param name="neighbors">이웃 탐색기. 이 객체보다 오래 살아야 한다.</param>
    /// <param name="source">기준 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public RawSquidNeighborWalk(RawSquidNeighbors neighbors, int source, uint flags)
    {
        ArgumentNullException.ThrowIfNull(neighbors);
        // 8(표면 지도 조회)은 이웃 탐색에서 지원하지 않는다
        if ((flags & ~NeighborFlags.Supported) != 0)
        {
            throw new ArgumentException("원본 이웃 플래그 오류", nameof(flags));
        }
        RawSquidNeighbors.NeighborObject body = neighbors.ReadObject(source, true);
        if (neighbors.Interior(body))
        {
            return;
        }
        _finder = new RawSquidFinder(neighbors.Pool, neighbors.Hash, neighbors.Types);
        _current = _finder.Begin(
            new SquidSearchArea((int)(body.Left - 1.0f), (int)(body.Top - 1.0f), (int)(body.Right + 1.0f), (int)(body.Bottom + 1.0f)),
            0,
            candidate => neighbors.Accept(candidate, body, flags));
    }

    /// <summary>현재 이웃 번호. 0이면 끝이다.</summary>
    public int Current => _current;

    /// <summary>
    /// 다음 후보를 받는다. flag 8이 없으면 일반 Next(004b1810)로 다음 버킷·단계를 전진한다.
    /// </summary>
    public int Next()
    {
        if (_current != 0)
        {
            _current = _finder!.Next();
        }
        return _current;
    }
}
