// 배치 decoder의 현재 모양에서 flag 8 표면 지도를 동적으로 순회한다.
using System.Globalization;

namespace Netstorm.Core.Original;

// 표면 순회 모듈을 담는 네임스페이스다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.

// 일반 타입·패턴 표면 순회 중 현재 위치와 대상이다. 원본 CanonSurfaceCursor에 대응하며 지도·캐시를 호출자와 공유하지 않는다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.
public sealed class CanonSurfaceCursor
{
    /// <summary>현재 읽는 격자 가로·세로와 확장 모서리다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    public int X, Y, Left, Top, Right, Bottom;

    /// <summary>현재 처리 위치 또는 현재 선택 상태다. 없으면 0이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    public int Current;

    /// <summary>원본 signed WORD 중복 캐시의 반환 순서다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    public List<ushort> Returned = [];
}

// 일반 타입·패턴 표면 순회 동작을 실행하는 처리기이다. 배치 decoder의 현재 모양과 실제 Squid 원천을 같은 flag 8 커서·필터로 읽는다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.
public sealed class RawCanonSurfaceWalk
{
    /// <summary>원본 모서리 clamp 범위와 기준점 spot 절삭 보정이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const float First = 1.0f;
    /// <summary>지도 오른쪽·아래쪽 경계다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const float Last = 255.0f;
    /// <summary>기준점 spot 조회에 더하는 실제 값이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const float Bias = 0.9999f;

    /// <summary>원본 signed WORD 중복 캐시의 실제 용량이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const int CacheSize = 64;

    /// <summary>중복 캐시가 정상 비교하는 최대 SID다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private const int LargestSid = 32767;

    /// <summary>객체 슬롯의 생성·조회·반납에 사용하는 SID 풀이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>배치 표만 빌릴 때 쓰는 프레임 표다. 실제 Squid 원천일 때는 비워 둔다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<PriestPlainCanonType>? _frames;

    /// <summary>실제 Squid 원천만 빌리는 코드 표다. 배치 표일 때는 비워 둔다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames>? _rawFrames;

    /// <summary>현재 섬의 배치·소유권 자료다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<ushort> _islands;

    /// <summary>칸별 점유·통행 비트를 읽는 spot 지도다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<byte> _spots;

    /// <summary>원천 타입이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly uint _sourceType;

    /// <summary>원본 +44에 저장한 프레임 번호다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private int _sourceFrame;

    /// <summary>사각형의 왼쪽·위쪽·오른쪽·아래쪽 경계와 중심이다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private float _left, _top, _right, _bottom, _centerX, _centerY;

    /// <summary>처리 중 조회·갱신하는 상태다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private readonly CanonSurfaceCursor _state = new();

    /// <summary>종료된 여부다. 2026-11-07 F2d 표면 순회 이식에서 추가됐다.</summary>
    private bool _ended;

    /// <summary>배치 decoder의 현재 모양으로 순회한다.</summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="frames">타입별 배치 프레임 표다.</param>
    /// <param name="islands">섬 지도 65,536칸이다.</param>
    /// <param name="spots">spot 지도 65,536칸이다.</param>
    /// <param name="query">현재 모양의 배치 입력이다.</param>
    /// <param name="cell">현재 모양의 격자 정보다.</param>
    public RawCanonSurfaceWalk(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<PriestPlainCanonType> frames, IReadOnlyList<ushort> islands, IReadOnlyList<byte> spots, CanonPlacementQuery query, CanonPlacementCell cell)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(spots);
        _pool = pool;
        _types = types;
        _frames = frames;
        _islands = islands;
        _spots = spots;
        _sourceType = query.Type;
        Initialize(cell);
    }

    /// <summary>실제 Squid 원천으로 순회한다. 원천의 float 발자국·기준점을 고정하고 후보·코드·지도는 매번 현재값으로 읽는다.</summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="frames">타입별 실제 프레임 표다.</param>
    /// <param name="islands">섬 지도 65,536칸이다.</param>
    /// <param name="spots">spot 지도 65,536칸이다.</param>
    /// <param name="source">원천 객체 번호다.</param>
    public RawCanonSurfaceWalk(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<ushort> islands, IReadOnlyList<byte> spots, int source)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(spots);
        _pool = pool;
        _types = types;
        _rawFrames = frames;
        _islands = islands;
        _spots = spots;
        ReadOnlySpan<byte> raw = pool.Slot(source);
        _sourceType = raw[RawSidPool.TypeOffset];
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        int frame = patch ? BitConverter.ToInt32(raw.Slice(36, 4)) : raw[34];
        float x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
        Initialize(new CanonPlacementCell(frame, 0, 0, x, y, 0f, 0f, default));
    }

    /// <summary>두 원천의 같은 발자국· 중심·최초 방문을 구성한다.</summary>
    /// <param name="cell">현재 모양의 격자 정보다.</param>
    private void Initialize(CanonPlacementCell cell)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int plainCount = _frames?.Count ?? 0;
        int rawCount = _rawFrames?.Count ?? 0;
        if (_types.Count != (patch ? OriginalConstructors.TypeCount : 171) || plainCount + rawCount != _types.Count || _islands.Count != 65536 || _spots.Count != 65536)
        {
            throw new ArgumentException("주변 표면 자료 크기 오류");
        }

        if (_sourceType >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(_sourceType), _sourceType.ToString(CultureInfo.InvariantCulture), "주변 표면 배치 타입 오류");
        }

        OriginalTypeRecord type = _types[(int)_sourceType];
        IReadOnlyList<RiftFrameCode> codes = Codes(_sourceType);
        if (type.FootX < 1 || type.FootY < 1 || type.FootX > 12 || type.FootY > 12 || cell.Frame < 0 || cell.Frame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(cell), "주변 표면 발자국·프레임 오류");
        }

        Cell(cell.X);
        Cell(cell.Y);
        _sourceFrame = cell.Frame;
        _left = Math.Clamp(cell.X - (type.FootX - 1), First, Last);
        _top = Math.Clamp(cell.Y - (type.FootY - 1), First, Last);
        _right = Math.Max(cell.X, _left);
        _bottom = Math.Max(cell.Y, _top);
        _centerX = (float)((double)cell.X - type.FootX * 0.5);
        _centerY = (float)((double)cell.Y - type.FootY * 0.5);
        byte bits = 0xff;
        // 원본은 발자국을 1..255로 자른 뒤 y/x 순서로 spot을 AND한다. 발자국 안 모든 칸의 공통 내부 비트만 사용한다.
        for (int y = Cell(_top); y <= Math.Min(255, Cell(_bottom)); ++y)
        {
            for (int x = Cell(_left); x <= Math.Min(255, Cell(_right)); ++x)
            {
                bits = (byte)(bits & _spots[y * 256 + x]);
            }
        }

        if ((bits & 8) != 0)
        {
            _ended = true;
            return;
        }

        _state.Left = (int)(_left - 1.0f);
        _state.Top = (int)(_top - 1.0f);
        _state.Right = (int)(_right + 1.0f);
        _state.Bottom = (int)(_bottom + 1.0f);
        _state.X = _state.Left;
        _state.Y = _state.Top;
        Next();
    }

    /// <summary>실제 Squid 표와 배치 표는 복사·변환하지 않고 같은 타입 번호로 조회한다.</summary>
    /// <param name="type">조회할 타입 번호다.</param>
    /// <returns>해당 타입의 코드 표다.</returns>
    private IReadOnlyList<RiftFrameCode> Codes(uint type)
    {
        return _rawFrames is not null ? _rawFrames[(int)type].Codes : _frames![(int)type].Frames.Codes;
    }

    /// <summary>필터는 원본처럼 후보 spot부터 읽고, 접합 계산 뒤 죽음·내부 비트를 검사한다.</summary>
    /// <param name="candidate">검사할 후보 번호다.</param>
    /// <returns>순회에 내보내면 참이다.</returns>
    private bool Accept(int candidate)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(candidate);
        float x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
        Cell(x);
        Cell(y);
        int spot = (int)((double)y + Bias) * 256 + (int)((double)x + Bias);
        if (spot >= _spots.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(candidate), "주변 표면 기준점 spot 오류");
        }

        byte bits = _spots[spot];
        if (raw[RawSidPool.TypeOffset] >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(candidate), "주변 표면 후보 타입 오류");
        }

        OriginalTypeRecord type = _types[raw[RawSidPool.TypeOffset]];
        if ((type.Flags1 & TypeFlag1.Surface) == 0)
        {
            return false;
        }

        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        uint frame = Read(raw, patch ? 36 : 34, patch ? 4 : 1);
        IReadOnlyList<RiftFrameCode> codes = Codes(raw[RawSidPool.TypeOffset]);
        if (frame >= (uint)codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(candidate), "주변 표면 후보 프레임 오류");
        }

        float cx = (float)((double)x - type.FootX * 0.5);
        float cy = (float)((double)y - type.FootY * 0.5);
        int direction = RawSquidNeighbors.Direction(cx, cy, _centerX, _centerY);
        IReadOnlyList<RiftFrameCode> sourceCodes = Codes(_sourceType);
        if ((uint)_sourceFrame >= (uint)sourceCodes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(_sourceFrame), "주변 표면 현재 배치 프레임 오류");
        }

        if (!RawBridgeConnects.Connects(codes[(int)frame], type.Flags2, sourceCodes[_sourceFrame], _types[(int)_sourceType].Flags2, direction))
        {
            return false;
        }

        return (raw[RawSidPool.StateOffset] & 2) == 0 && (bits & 8) == 0;
    }

    /// <summary>확장 사각형의 네 모서리를 제외하고 다음 지도 WORD를 그 시점에 읽는다.</summary>
    /// <returns>다음 후보 번호다. 없으면 0이다.</returns>
    public int Next()
    {
        _state.Current = 0;
        if (_ended)
        {
            return 0;
        }

        // 반환 가능한 새 후보가 나오거나 마지막 행을 소진할 때까지 원본 커서를 진행한다.
        for (;;)
        {
            ++_state.X;
            if (_state.Y == _state.Top || _state.Y == _state.Bottom)
            {
                if (_state.X >= _state.Right)
                {
                    if (++_state.Y > _state.Bottom)
                    {
                        _ended = true;
                        return 0;
                    }

                    _state.X = _state.Left;
                }
            }
            else if (_state.X > _state.Right)
            {
                ++_state.Y;
                _state.X = _state.Left + (_state.Y == _state.Bottom ? 1 : 0);
            }

            if (_state.X < 0 || _state.Y < 0 || _state.X >= 256 || _state.Y >= 256)
            {
                continue;
            }

            int candidate = _islands[_state.Y * 256 + _state.X];
            if (candidate == 0)
            {
                continue;
            }

            if (candidate > LargestSid)
            {
                throw new ArgumentOutOfRangeException(nameof(candidate), "주변 표면 signed SID 범위 오류");
            }

            ReadOnlySpan<byte> raw = _pool.Slot(candidate);
            float x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
            float y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
            if (_left <= x && x <= _right && _top <= y && y <= _bottom)
            {
                continue;
            }

            if (!Accept(candidate) || _state.Returned.Contains((ushort)candidate))
            {
                continue;
            }

            if (_state.Returned.Count == CacheSize)
            {
                throw new ArgumentOutOfRangeException(nameof(candidate), "주변 표면 중복 캐시 용량 오류");
            }

            _state.Returned.Add((ushort)candidate);
            _state.Current = candidate;
            return candidate;
        }
    }

    /// <summary>현재 후보만 반환하고 지도를 다시 읽지 않는다.</summary>
    /// <returns>현재 후보 번호다.</returns>
    public int Current() => _state.Current;

    /// <summary>물리 포인터를 제외한 원본 커서·캐시다.</summary>
    /// <returns>현재 커서다.</returns>
    public CanonSurfaceCursor State() => _state;

    /// <summary>비정렬 raw 필드를 리틀 엔디언 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        // 지정한 필드 폭의 낮은 바이트부터 조립한다.
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>지도 안의 유한 float 계약을 확인하고 원본 CRT처럼 0 방향으로 절삭한다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>칸 번호다.</returns>
    private static int Cell(float value)
    {
        if (!float.IsFinite(value) || value < 0 || value >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "주변 표면 좌표 오류");
        }

        return (int)value;
    }
}
