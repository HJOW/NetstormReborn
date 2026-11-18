// 지역 투표 뒤의 실제 지면 flood와 패치판 경계 프레임 도장을 같은 SID 풀에 적용한다.
namespace Netstorm.Core.Original;

// 지역 도장을 담는 네임스페이스다. 2026-11-18 0.618에서 R4-2 지역 칠하기 결선용으로 추가했다.

/// <summary>
/// Region drawer의 네 float·현재 요청·두 독립 변형 캐시와 좌표 후보 목록이다.
/// 근거: cpppj <c>o/RawRegionPaint.h</c>의 <c>RegionPaintState</c>.
/// </summary>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 칠하기 결선용으로 추가했다.</remarks>
public sealed class RegionPaintState
{
    /// <summary>bounds: 생성자는 -10000을 저장하며 각 Paint 호출은 257,257,-1,-1로 다시 쓴다.</summary>
    public float[] Bounds { get; } = [-10000.0F, -10000.0F, -10000.0F, -10000.0F];

    /// <summary>owner: 마지막 유효 지면에서 시작한 요청의 전체 DWORD 소유자 값이다.</summary>
    public uint Owner { get; set; }

    /// <summary>theme: 패치 요청의 원소 quartile이다. 0/-1은 기본, 1/2/그 밖은 다음 세 원소다.</summary>
    public int Theme { get; set; }

    /// <summary>revision: 독립 입력의 월드 다시 그리기 DWORD다. 패치 0059a8b0은 flood 호출마다 한 번 증가한다.</summary>
    public uint Revision { get; set; }

    /// <summary>boundary: 패치의 중복 없는 8바이트 정수 좌표를 삽입 순서로 보관한다.</summary>
    public List<CellPoint> Boundary { get; } = [];

    /// <summary>coreVariants: 본섬 내부 AA 프레임에 사용하는 독립 99개 캐시다.</summary>
    public int[]? CoreVariants { get; set; }

    /// <summary>boundaryVariants: 경계 도장의 AA 프레임에 사용하는 별도 99개 캐시다.</summary>
    public int[]? BoundaryVariants { get; set; }

    /// <summary>sharedWorldRedraw: 패치 0059a8b0의 같은 현재 DWORD다. 없으면 독립 revision을 쓴다.</summary>
    public UIntBox? SharedWorldRedraw { get; set; }

    /// <summary>같은 실제 DWORD를 읽는다.</summary>
    /// <returns>공유 주인이 있으면 그 값, 없으면 독립 값이다.</returns>
    public uint WorldRedraw => SharedWorldRedraw?.Value ?? Revision;

    /// <summary>같은 실제 DWORD에 쓴다.</summary>
    /// <param name="value">쓸 값이다.</param>
    public void SetWorldRedraw(uint value)
    {
        // 공유 주인이 있으면 그곳에 쓰고 독립 값은 그대로 둔다.
        if (SharedWorldRedraw is not null)
        {
            SharedWorldRedraw.Value = value;
        }
        else
        {
            Revision = value;
        }
    }
}

/// <summary>
/// 소유자 가상 호출·빈 표면의 edgeFarm finder·현재 지역 getter를 연결한다.
/// 근거: cpppj <c>o/RawRegionPaint.h</c>의 <c>RegionPaintHooks</c>.
/// </summary>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 칠하기 결선용으로 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class RegionPaintHooks
{
    /// <summary>setOwner: 가상 +0x74다. 같은 raw 슬롯을 갱신하며 표시/장부 효과도 유지해야 한다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>boundaryAt: 표면 0 또는 월드 밖 좌표에서 edgeFarm 타입을 조회하는 실제 일반 finder다.</summary>
    public Func<float, float, uint, int>? BoundaryAt { get; set; }

    /// <summary>regionAt: 패치의 영향을 받는 지역 기록에 사용할 0046f530 getter다.</summary>
    public Func<float, float, uint>? RegionAt { get; set; }
}

/// <summary>
/// 00470900/0046fe40/004703b0 및 CD 004bd2d0/004bd0b0의 도장 몸체다.
/// 근거: cpppj <c>o/RawRegionPaint.h/.cpp</c>.
/// </summary>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 칠하기 결선용으로 추가했다. 실제 농지 호출·사용자 건설 통합은 R4-6·R6에서 검증한다.</remarks>
public sealed class RawRegionPaint
{
    /// <summary>원본 네 방향은 북·동·남·서이며 비트는 1·2·4·8이다.</summary>
    private static readonly CellPoint[] Directions = [new(0, -1), new(1, 0), new(0, 1), new(-1, 0)];

    /// <summary>비정렬 little endian 필드를 읽는 폭이다.</summary>
    private const int DwordWidth = 4;

    /// <summary>같은 풀의 슬롯을 읽는 SID 풀이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 타입별 코드·J 시작 번호의 조회 표다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>섬 지도는 hash0의 실제 WORD 머리다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<ushort> _islands;

    /// <summary>terrainType: flood가 처리하는 실제 isle 타입 번호다.</summary>
    private readonly uint _terrainType;

    /// <summary>edgeFarmType: 빈 표면의 일반 finder에서 찾는 지형 경계 타입 번호다.</summary>
    private readonly uint _edgeFarmType;

    /// <summary>random: 비AA 후보 선택이 소비하는 게임 전역 난수다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly GameRandom _random;

    /// <summary>state: drawer 요청과 원본 전역의 도장 캐시/후보/revision 상태다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RegionPaintState _state;

    /// <summary>ownership: 투표 wrapper와 공유하는 중복 없는 영향 지역 기록이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RegionOwnershipState _ownership;

    /// <summary>hooks: 실제 소유자/일반 finder/현재 지역 경계다.</summary>
    private readonly RegionPaintHooks _hooks;

    /// <summary>
    /// 섬 지도와 프레임·난수·영향 목록을 같은 월드에 연결한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="frames">타입별 프레임 자료다.</param>
    /// <param name="islands">섬 지도는 hash0의 실제 WORD 머리다.</param>
    /// <param name="terrainType">flood가 처리하는 실제 isle 타입 번호다.</param>
    /// <param name="edgeFarmType">빈 표면의 일반 finder에서 찾는 지형 경계 타입 번호다.</param>
    /// <param name="random">비AA 후보 선택이 소비하는 게임 전역 난수다.</param>
    /// <param name="state">drawer 요청과 원본 전역의 도장 캐시/후보/revision 상태다.</param>
    /// <param name="ownership">투표 wrapper와 공유하는 중복 없는 영향 지역 기록이다.</param>
    /// <param name="hooks">실제 소유자/일반 finder/현재 지역 경계다.</param>
    /// <exception cref="ArgumentException">지역 도장 지도/프레임/필수 효과 연결 오류다.</exception>
    public RawRegionPaint(
        RawSidPool pool,
        IReadOnlyList<RiftTypeFrames> frames,
        IReadOnlyList<ushort> islands,
        uint terrainType,
        uint edgeFarmType,
        GameRandom random,
        RegionPaintState state,
        RegionOwnershipState ownership,
        RegionPaintHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(ownership);
        ArgumentNullException.ThrowIfNull(hooks);
        if (islands.Count != 65536 || terrainType >= (uint)frames.Count || hooks.SetOwner is null
            || (pool.Edition == OriginalEdition.Patch1078 && (edgeFarmType >= (uint)frames.Count || hooks.BoundaryAt is null || hooks.RegionAt is null)))
        {
            throw new ArgumentException("지역 도장 지도/프레임/필수 효과 연결 오류");
        }

        _pool = pool;
        _frames = frames;
        _islands = islands;
        _terrainType = terrainType;
        _edgeFarmType = edgeFarmType;
        _random = random;
        _state = state;
        _ownership = ownership;
        _hooks = hooks;
    }

    /// <summary>외부 연결의 SID 저장소 동일성에 사용한다.</summary>
    /// <returns>이 모듈이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>월드 밖은 SID 0이다. 별도의 snap이나 좌표 clamp를 넣지 않는다.</summary>
    /// <param name="x">가로 칸 좌표다.</param>
    /// <param name="y">세로 칸 좌표다.</param>
    /// <returns>해당 칸 머리 번호다. 지도 밖이면 0이다.</returns>
    private int Surface(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256)
        {
            return 0;
        }

        return _islands[y * 256 + x];
    }

    /// <summary>비정렬 little endian 필드를 지정 폭으로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    /// <returns>리틀 엔디언으로 조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = DwordWidth)
    {
        uint result = 0;
        // 인접 CD 필드를 섞지 않고 실제 폭만 조립한다.
        for (int index = 0; index < width; ++index)
        {
            result |= (uint)raw[offset + index] << (index * 8);
        }

        return result;
    }

    /// <summary>raw float 좌표에 원본 _ftol의 0 방향 절삭을 적용한다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>절삭한 정수 좌표다.</returns>
    private static int Coordinate(ReadOnlySpan<byte> raw, int offset)
    {
        double value = Math.Truncate(BitConverter.UInt32BitsToSingle(Read(raw, offset)));
        if (!double.IsFinite(value) || value < int.MinValue || value > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "지형 도장 좌표 절삭 범위 오류");
        }

        return (int)value;
    }

    /// <summary>소유자 가상 호출 뒤 현재 프레임을 지면 코드 배열에서 읽는다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>현재 프레임의 코드다.</returns>
    private RiftFrameCode Code(int sid)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        uint index = Read(_pool.Slot(sid), patch ? 36 : 34, patch ? 4 : 1);
        IReadOnlyList<RiftFrameCode> codes = _frames[(int)_terrainType].Codes;
        if (index >= (uint)codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "지역 도장 현재 프레임 코드 범위 오류");
        }

        return codes[(int)index];
    }

    /// <summary>캐시 배열을 만들고 원본 로컬 seed0 동작으로 채운다.</summary>
    /// <returns>99개 변형 캐시다.</returns>
    private static int[] CreateCache()
    {
        int[] values = new int[99];
        // 매 항목마다 seed0를 새로 만드는 반복문이다. 시드를 누적하면 변형 캐시 전체가 어긋난다.
        for (int index = 0; index < values.Length; ++index)
        {
            uint seed = 0;
            values[index] = TerrainBuilder.Next(ref seed, 65536);
            if (index != 0 && values[index - 1] % 4 == values[index] % 4)
            {
                ++values[index];
            }
        }

        return values;
    }

    /// <summary>기계어에서 두 캐시 좌표 항은 요청 x/y이며 픽셀 residue는 현재 raw 좌표를 절삭한다.</summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <param name="x">요청 가로 좌표다.</param>
    /// <param name="y">요청 세로 좌표다.</param>
    /// <param name="boundary">경계 도장이면 참이다.</param>
    /// <returns>선택된 프레임 번호다.</returns>
    private int Select(int sid, int x, int y, bool boundary)
    {
        RiftFrameCode code = Code(sid);
        int[] cache = boundary
            ? _state.BoundaryVariants ??= CreateCache()
            : _state.CoreVariants ??= CreateCache();
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        int px = Coordinate(raw, 14);
        int py = Coordinate(raw, 18);
        int bx = (x / 3) % 99;
        int by = (y / 3) % 99;
        if (bx < 0 || by < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "지역 도장 변형 캐시 좌표 오류");
        }

        // AA가 아니면 실제 후보 검색을 쓰고 게임 난수를 소비한다.
        if (code.Side != (byte)'A' || code.Variant != (byte)'A')
        {
            return RawFrameChoose.ChooseTerrainFrame(_frames[(int)_terrainType], code, _state.Theme, _random);
        }

        int sum = (cache[bx] + cache[by]) % 99;
        int variant = (py / 3 + cache[sum]) % 4;
        int theme = _state.Theme == 0 || _state.Theme == -1 ? 0 : _state.Theme == 1 ? 1 : _state.Theme == 2 ? 2 : 3;
        return _frames[(int)_terrainType].Run((byte)'J').First + 1 + theme * 36 + (variant * 3 + py % 3) * 3 + px % 3;
    }

    /// <summary>좌표별 중복은 값이 완전히 같을 때만 적용한다.</summary>
    /// <param name="x">추가할 가로 좌표다.</param>
    /// <param name="y">추가할 세로 좌표다.</param>
    private void AddBoundary(int x, int y)
    {
        var point = new CellPoint(x, y);
        // 삽입 순서대로 두며 이미 있으면 추가하지 않는다.
        if (!_state.Boundary.Contains(point))
        {
            _state.Boundary.Add(point);
        }
    }

    /// <summary>원본 코드의 네 직선 이웃을 원본 순서로 좌표 목록에 추가한다.</summary>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="connections">북·동·남·서 비트다.</param>
    private void AddNeighbors(int x, int y, int connections)
    {
        // 북·동·남·서 순서로 후보를 넣는다.
        for (int i = 0; i < Directions.Length; ++i)
        {
            if ((connections & (1 << i)) != 0)
            {
                AddBoundary(x + Directions[i].X, y + Directions[i].Y);
            }
        }
    }

    /// <summary>재귀의 가상 호출 순서를 유지하며 큰 실제 섬에서도 호스트 C++ 스택을 넘지 않게 한다.</summary>
    /// <param name="x">시작 가로 좌표다.</param>
    /// <param name="y">시작 세로 좌표다.</param>
    private void Flood(int x, int y)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        var work = new List<CellPoint> { new(x, y) };
        // 원본의 북·동·남 재귀와 서쪽 tail loop를 역순 push로 재현한다. 스택 끝에서 꺼내 북·동·남·서 순으로 방문한다.
        while (work.Count != 0)
        {
            CellPoint point = work[^1];
            work.RemoveAt(work.Count - 1);
            int sid = Surface(point.X, point.Y);
            if (sid == 0)
            {
                if (patch)
                {
                    AddBoundary(point.X, point.Y);
                }

                continue;
            }

            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            if (raw[10] != _terrainType || raw[patch ? 34 : 32] == _state.Owner)
            {
                continue;
            }

            Need(_hooks.SetOwner, nameof(RegionPaintHooks.SetOwner))(sid, _state.Owner);
            RiftFrameCode code = Code(sid);
            int connections = TerrainBuilder.Connection((char)code.Side);
            if (patch)
            {
                int selected = Select(sid, point.X, point.Y, false);
                Span<byte> target = _pool.AllocatedBytes(sid);
                // 원본은 SetFrame 가상 함수를 부르지 않고 DWORD만 직접 저장한다.
                for (int i = 0; i < 4; ++i)
                {
                    target[36 + i] = (byte)(selected >> (i * 8));
                }
            }

            // 먼저 호출할 이웃이 마지막 push가 되도록 서·남·동·북 순서로 준비한다.
            for (int i = Directions.Length; i > 0; --i)
            {
                if ((connections & (1 << (i - 1))) != 0)
                {
                    work.Add(new CellPoint(point.X + Directions[i - 1].X, point.Y + Directions[i - 1].Y));
                }
            }
        }
    }

    /// <summary>patch만 빈 표면의 edgeFarm fallback·영향 지역·추가 후보를 수행한다.</summary>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    private void Boundary(int x, int y)
    {
        int sid = Surface(x, y);
        if (sid == 0)
        {
            sid = Need(_hooks.BoundaryAt, nameof(RegionPaintHooks.BoundaryAt))((float)x, (float)y, _edgeFarmType);
            if (sid == 0)
            {
                return;
            }
        }

        uint region = Need(_hooks.RegionAt, nameof(RegionPaintHooks.RegionAt))((float)x, (float)y);
        bool found = false;
        // 같은 지역이 이미 영향 목록에 있는지 확인한다.
        foreach (OwnershipAffectedRegion value in _ownership.Affected)
        {
            if (value.Region == region)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            // 원본처럼 정수 비트를 float 비트로 그대로 둔다(숫자 변환이 아니다).
            _ownership.Affected.Add(new OwnershipAffectedRegion(BitConverter.UInt32BitsToSingle(unchecked((uint)x)), BitConverter.UInt32BitsToSingle(unchecked((uint)y)), region));
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint type = raw[10];
        if (type == _terrainType && raw[34] == _state.Owner)
        {
            return;
        }

        if (type != _terrainType && type != _edgeFarmType)
        {
            return;
        }

        Need(_hooks.SetOwner, nameof(RegionPaintHooks.SetOwner))(sid, _state.Owner);
        RiftFrameCode code = Code(sid);
        int connections = TerrainBuilder.Connection((char)code.Side);
        int selected = Select(sid, x, y, true);
        Span<byte> target = _pool.AllocatedBytes(sid);
        // 경계도 직접 프레임 쓰기 뒤에 이웃 후보를 추가한다.
        for (int i = 0; i < 4; ++i)
        {
            target[36 + i] = (byte)(selected >> (i * 8));
        }

        AddNeighbors(x, y, connections);
    }

    /// <summary>실패한 시작 좌표에서도 원본 전역 revision 증가를 생략하지 않는다.</summary>
    /// <param name="request">도장 요청이다.</param>
    public void Paint(RegionOwnershipPaint request)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (patch)
        {
            _state.Boundary.Clear();
        }

        _state.Bounds[0] = 257.0F;
        _state.Bounds[1] = 257.0F;
        _state.Bounds[2] = -1.0F;
        _state.Bounds[3] = -1.0F;
        int sid = Surface(request.X, request.Y);
        if (sid != 0 && _pool.Slot(sid)[10] == _terrainType)
        {
            _state.Owner = request.Owner;
            if (patch)
            {
                _state.Theme = unchecked((int)request.Theme);
            }

            Flood(request.X, request.Y);
        }

        if (patch)
        {
            // stamp 중 추가된 후보도 같은 호출에서 뒤이어 처리한다. 순회 중 목록이 늘어난다.
            for (int i = 0; i < _state.Boundary.Count; ++i)
            {
                CellPoint point = _state.Boundary[i];
                Boundary(point.X, point.Y);
            }
        }

        _state.SetWorldRedraw(unchecked(_state.WorldRedraw + 1u));
    }

    /// <summary>기존 투표 wrapper를 다시 구현하지 않고 실제 도장 몸체만 공급한다.</summary>
    /// <param name="pool">같은 풀의 실제 지역 조회다. 다른 풀의 몸체와 섞으면 거부한다.</param>
    /// <param name="paint">같은 풀의 실제 도장 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다. 도장 효과만 실제 구현으로 바꾼다.</param>
    /// <returns>도장이 연결된 훅 묶음이다.</returns>
    /// <exception cref="ArgumentException">지역 투표/도장 SID 풀이 다르다.</exception>
    /// <remarks>2026-11-18 0.618에서 R4-2 지역 칠하기 결선용으로 추가했다. 근거: cpppj `RawRegionPaint.cpp` 139행의 `MakeRegionPaintOwnershipHooks`다.</remarks>
    public static RegionOwnershipHooks MakePaintOwnershipHooks(RawSidPool pool, RawRegionPaint paint, RegionOwnershipHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(paint);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, paint.Pool))
        {
            throw new ArgumentException("지역 투표/도장 SID 풀이 다릅니다", nameof(pool));
        }

        RegionOwnershipHooks result = new()
        {
            RegionAt = hooks.RegionAt,
            Paint = hooks.Paint,
            ThemeMode = hooks.ThemeMode,
            TypeTheme = hooks.TypeTheme,
        };
        result.Paint = request => paint.Paint(request);
        return result;
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <param name="name">장치 이름이다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">지역 도장의 실제 장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? function, string name) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("지역 도장 실제 장치 누락: " + name);
        }

        return function;
    }
}
