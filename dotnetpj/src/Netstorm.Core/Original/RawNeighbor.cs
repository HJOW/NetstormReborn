using Netstorm.Assets;
using Netstorm.Core.Bridges;

// 현재 원본 이웃의 원천 snapshot과 후보별 현재 지도·타입 조회를 구분한다.
namespace Netstorm.Core.Original;

/// <summary>
/// 탐색 플래그 비트(원본 finder +0x48). 일반 해시와 표면 지도 순회가 같은 후보 필터를 사용한다.
/// </summary>
public static class NeighborFlags
{
    /// <summary>래프터 결합 검사를 건너뛴다.</summary>
    public const uint NoJoint = 1;

    /// <summary>표면 타입이 아닌 후보도 받는다.</summary>
    public const uint AnyType = 2;

    /// <summary>abstract(extra 비트 1) 후보를 제외한다.</summary>
    public const uint SkipAbstract = 4;

    /// <summary>현재0단계 표면 지도의 원문 y/x 순회를 선택한다.</summary>
    public const uint SurfaceMap = 8;

    /// <summary>원문 네 비트의 지원 조합(0~15)이다.</summary>
    public const uint Supported = NoJoint | AnyType | SkipAbstract | SurfaceMap;
}

/// <summary>
/// 원본 float 발자국 사각형(원본 004ade70).
/// </summary>
/// <param name="Left">사각형의 왼쪽 경계.</param>
/// <param name="Top">사각형의 위쪽 경계.</param>
/// <param name="Right">사각형의 오른쪽 경계.</param>
/// <param name="Bottom">사각형의 아래쪽 경계.</param>
public readonly record struct SquidFootprint(float Left, float Top, float Right, float Bottom);

/// <summary>삭제 부모가 값으로 보관한 위치·타입·프레임이다. 후보의 실제 raw 타입과 별개다.</summary>
/// <param name="X">포착한 가로 단정도 좌표다.</param>
/// <param name="Y">포착한 세로 단정도 좌표다.</param>
/// <param name="Type">전체 DWORD 타입이다.</param>
/// <param name="Frame">현재 프레임 인덱스다.</param>
public readonly record struct SquidNeighborSource(float X, float Y, uint Type, uint Frame);

/// <summary>
/// 탐색기로 찾은 이웃을 원본 raw 값·발자국·표면에서 고르는 처리기(원본 RawSquidNeighbors, 10.78).
/// 해시 0단계·일반 탐색의 머리·다음 순서대로 원본 생성 순서의 후보를 받는다.
/// 기준 판본은 10.78이다. CD 10.72의 추가 비트·프레임 위치(35·34)도 판본 분기로 읽는다.
/// 근거: cpppj/src/o/RawSquidNeighbors.cpp (W02 이웃 조각, 2026-10-20; CD 분기 0.162, 2026-11-07).
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

    /// <summary>슬롯 안 추가 비트 바이트의 위치(CD 10.72).</summary>
    private const int CdExtraOffset = 35;

    /// <summary>슬롯 안 프레임 DWORD의 위치(10.78).</summary>
    private const int FrameOffset = 36;

    /// <summary>슬롯 안 프레임 바이트의 위치(CD 10.72).</summary>
    private const int CdFrameOffset = 34;

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

    /// <summary>C++ vector처럼 값으로 보관하는 독립 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>명시 결합한 실제 PATCH 장기 TYPE다. null이면 독립 사본을 쓴다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _currentTypes;

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
        _types = types.Select(type => type.Clone()).ToArray();
        _frames = frames.Select(codes => (IReadOnlyList<FrameCode>)codes.ToArray()).ToArray();
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

    /// <summary>실제 PATCH의 현재188개 TYPE만 명시 대여하며 null/빈 표는 독립 사본으로 돌린다. 물리·코드를 초기화하지 않는다.</summary>
    public void BindCurrentTypes(IReadOnlyList<OriginalTypeRecord>? types)
    {
        if (types is { Count: > 0 } && (_pool.Edition != OriginalEdition.Patch1078 || types.Count != 188))
            throw new ArgumentException("이웃 현재TYPE 대여의 판본/길이 오류", nameof(types));
        _currentTypes = types is { Count: > 0 } ? types : null;
    }

    /// <summary>프레임 판독 전에 실제 슬롯의 원문 인자만 값으로 포착한다.</summary>
    public SquidNeighborSource Source(int sid)
    {
        if (sid <= 0 || sid >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid), sid, "원본 이웃 번호 오류");
        ReadOnlySpan<byte> raw = _pool.Slot(sid); bool patch = _pool.Edition == OriginalEdition.Patch1078;
        return new(BitConverter.UInt32BitsToSingle(Read(raw, XOffset)), BitConverter.UInt32BitsToSingle(Read(raw, YOffset)),
            raw[TypeOffset], Read(raw, patch ? FrameOffset : CdFrameOffset, patch ? 4 : 1));
    }

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
    /// 세로 절댓값이 가로보다 작지 않으면(동률·NaN 포함) 세로 방향이다.
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
        return ReadObject(Source(sid), readFrame, false);
    }

    /// <summary>값 원천의 보정 발자국·중심과 필요한 코드만 읽는다. lazyFrame은 실제 접합 분기까지 범위 밖 코드를 기본값으로 둔다.</summary>
    internal NeighborObject ReadObject(SquidNeighborSource source, bool readFrame, bool lazyFrame)
    {
        uint number = source.Type;
        if (number < (uint)OriginalConstructors.FirstAssetType || number >= (uint)Types.Count)
        {
            throw new ArgumentOutOfRangeException("sid", number, "원본 이웃 자산 타입 범위 오류");
        }
        OriginalTypeRecord type = Types[(int)number];
        float x = source.X, y = source.Y;
        if (!float.IsFinite(x) || !float.IsFinite(y)
            || type.FootX < 1 || type.FootY < 1 || type.FootX > RawSquidHash.WorldCells || type.FootY > RawSquidHash.WorldCells)
        {
            throw new ArgumentOutOfRangeException("sid", "원본 이웃 좌표·발자국 범위 오류");
        }
        // 두 원래 좌표가 모두(0,256) 안일 때만 이전 점을 보존하고 그 밖에는 보정된 한 점을 사용한다.
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
            uint frame = source.Frame;
            IReadOnlyList<FrameCode> codes = _frames[(int)number];
            if (frame >= (uint)codes.Count)
            {
                if (!lazyFrame) throw new ArgumentOutOfRangeException("sid", frame, "원본 이웃 프레임 범위 오류");
            }
            else body.Frame = codes[(int)frame];
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
        if ((Types[(int)number].Flags1 & SurfaceFlag) == 0 && (flags & NeighborFlags.AnyType) == 0)
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
        if ((flags & NeighborFlags.SurfaceMap) == 0 && Intersects(body.Left - 1.0f, body.Top, body.Right + 1.0f, body.Bottom)
            == Intersects(body.Left, body.Top - 1.0f, body.Right, body.Bottom + 1.0f))
        {
            return false;
        }
        if ((flags & NeighborFlags.NoJoint) == 0)
        {
            NeighborObject framed = ReadObject(candidate, true);
            // 후보 중심에서 source 중심을 보는 방향으로 결합을 검사한다
            int direction = Direction(framed.CenterX, framed.CenterY, source.CenterX, source.CenterY);
            if (!BridgeSurfaceRules.Connects(framed.Frame, Types[(int)number].Flags2, source.Frame, Types[(int)source.Type].Flags2, direction))
            {
                return false;
            }
        }
        if ((raw[StateOffset] & DeadBit) != 0)
        {
            return false;
        }
        // abstract 후보는 제외한다. 판본별 추가 비트 위치가 다르다
        if ((flags & NeighborFlags.SkipAbstract) != 0 && (raw[_pool.Edition == OriginalEdition.Patch1078 ? ExtraOffset : CdExtraOffset] & AbstractBit) != 0)
        {
            return false;
        }
        // 후보 기준점의 spot 칸이 다음 칸이면 그 칸을 보고, 배열 밖만 거부한다
        int index = BiasedCell(BitConverter.UInt32BitsToSingle(Read(raw, YOffset))) * RawSquidHash.WorldCells + BiasedCell(BitConverter.UInt32BitsToSingle(Read(raw, XOffset)));
        if (index < 0 || index >= _spots.Count)
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
    public IReadOnlyList<OriginalTypeRecord> Types => _currentTypes ?? _types;
}

/// <summary>
/// 결합 이웃 탐색기의 회귀(원본 RawSquidNeighborWalk).
/// 생성 직후 Current()가 첫 이웃(원본 +0x34)이며, Next()로 다음 후보를 받는다.
/// </summary>
public sealed class RawSquidNeighborWalk
{
    /// <summary>탐색기. 안쪽이면 만들지 않는다.</summary>
    private readonly RawSquidFinder? _finder;

    /// <summary>표면 지도용 원천 snapshot·커서·signed short 캐시를 보관하는 현재 Next다.</summary>
    private readonly Func<int>? _surfaceNext;

    /// <summary>현재 처리 위치. 0이면 끝이다.</summary>
    private int _current;

    /// <summary>
    /// 이웃 탐색기·기준 번호·플래그로 회귀를 만든다. 플래그는 0~15 조합이다.
    /// </summary>
    /// <param name="neighbors">이웃 탐색기. 이 객체보다 오래 살아야 한다.</param>
    /// <param name="source">기준 번호.</param>
    /// <param name="flags">호출 플래그.</param>
    public RawSquidNeighborWalk(RawSquidNeighbors neighbors, int source, uint flags)
        : this(neighbors, (neighbors ?? throw new ArgumentNullException(nameof(neighbors))).Source(source), flags) { }

    /// <summary>삭제 정보의 값 원천만 치환하며 실제 후보·지도·현재 타입은 Next마다 읽는다.</summary>
    public RawSquidNeighborWalk(RawSquidNeighbors neighbors, SquidNeighborSource source, uint flags, bool lazyFrame = false)
    {
        ArgumentNullException.ThrowIfNull(neighbors);
        if ((flags & ~NeighborFlags.Supported) != 0)
        {
            throw new ArgumentException("원본 이웃 플래그 오류", nameof(flags));
        }
        RawSquidNeighbors.NeighborObject body = neighbors.ReadObject(source, true, lazyFrame);
        if (neighbors.Interior(body))
        {
            return;
        }
        if ((flags & NeighborFlags.SurfaceMap) != 0)
        {
            int left = (int)(body.Left - 1f), top = (int)(body.Top - 1f), right = (int)(body.Right + 1f), bottom = (int)(body.Bottom + 1f);
            int x = left, y = top; var seen = new List<short>();
            _surfaceNext = () =>
            {
                // 원문 선증가·모서리 생략 뒤 현재 지도/후보를 읽는다. 각 Walk의 커서와 캐시는 독립적이다.
                while (true)
                {
                    x++;
                    if (y == top || y == bottom) { if (x >= right) { y++; if (y > bottom) return 0; x = left; } }
                    else if (x > right) { y++; x = left + (y == bottom ? 1 : 0); }
                    if (x < 0 || y < 0 || x >= 256 || y >= 256) continue;
                    int candidate = neighbors.Hash.Cell(0, x, y); if (candidate == 0) continue;
                    SquidNeighborSource current = neighbors.Source(candidate);
                    if (body.Left <= current.X && current.X <= body.Right && body.Top <= current.Y && current.Y <= body.Bottom) continue;
                    if (!neighbors.Accept(candidate, body, flags)) continue;
                    if (seen.Any(prior => (int)prior == candidate)) continue;
                    seen.Add(unchecked((short)candidate)); return candidate;
                }
            };
            _current = _surfaceNext(); return;
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
    /// 다음 후보를 받는다. flag8이면 현재 단계0 지도, 나머지는 일반 Next로 다음 버킷·단계를 전진한다.
    /// </summary>
    public int Next()
    {
        if (_current != 0)
        {
            _current = _surfaceNext is not null ? _surfaceNext() : _finder!.Next();
        }
        return _current;
    }
}
