namespace Netstorm.Core.Original;

/// <summary>
/// 원본 Begin 의 탐색 사각형. 오른쪽·아래가 −1이면 보드 끝까지다.
/// </summary>
/// <param name="Left">사각형의 왼쪽 경계.</param>
/// <param name="Top">사각형의 위쪽 경계.</param>
/// <param name="Right">사각형의 오른쪽 경계(−1은 끝).</param>
/// <param name="Bottom">사각형의 아래쪽 경계(−1은 끝).</param>
public readonly record struct SquidSearchArea(int Left, int Top, int Right, int Bottom);

/// <summary>
/// 원본 finder +4..+60 의 상태를 그대로 두는 탐색 커서(원본 SquidSearchState).
/// </summary>
public struct SquidSearchState
{
    /// <summary>후보를 찾거나 검사할 사각형 영역.</summary>
    public SquidSearchArea Area;

    /// <summary>검사 사각형의 첫 X 칸.</summary>
    public int FirstX;

    /// <summary>검사 사각형의 첫 Y 칸.</summary>
    public int FirstY;

    /// <summary>검사 사각형의 마지막 X 칸.</summary>
    public int LastX;

    /// <summary>검사 사각형의 마지막 Y 칸.</summary>
    public int LastY;

    /// <summary>공간 해시에서 조회하는 현재 단계.</summary>
    public int Level;

    /// <summary>X 버킷 번호.</summary>
    public int X;

    /// <summary>Y 버킷 번호.</summary>
    public int Y;

    /// <summary>다음 후보 번호. 0이면 다음 버킷·단계로 넘어간다.</summary>
    public int Next;

    /// <summary>현재 처리 위치. 0이면 끝이다.</summary>
    public int Current;

    /// <summary>표면 생략 여부(호출 플래그 1).</summary>
    public bool SkipSurface;

    /// <summary>확장 높이 여부(호출 플래그 4).</summary>
    public bool ExtendedHeight;
}

/// <summary>
/// 원본 사각형 탐색기의 4단계 해시·적 raw 체인(원본 RawSquidFinder, 10.78).
/// Begin 으로 사각형·플래그·필터를 정하고 Next 로 후보를 하나씩 받는다.
/// 반환된 후보의 다음 체인은 먼저 확보하고, 필터·호출 중 변경은 다음 후보의 변경된 값으로 읽는다.
/// 매립은 제외하고 free·dead·void 는 호출자 필터의 몫으로 남긴다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 좌표 assert·추가 바이트 위치가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawSquidFinder.cpp (W02 탐색 조각, 2026-10-20).
/// </summary>
public sealed class RawSquidFinder
{
    /// <summary>슬롯 안 다음 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 x 좌표(float)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>추가 비트의 매립(buried) 비트. 켜진 후보는 건너뛴다.</summary>
    private const byte BuriedBit = 8;

    /// <summary>호출 플래그의 허용 마스크. 1(표면 생략)·4(확장 높이)만 받는다.</summary>
    private const uint AllowedFlags = 5;

    /// <summary>호출 플래그의 표면 생략 비트.</summary>
    private const uint SkipSurfaceFlag = 1;

    /// <summary>호출 플래그의 확장 높이 비트.</summary>
    private const uint ExtendedHeightFlag = 4;

    /// <summary>슬롯을 읽는 풀(읽기 전용으로 쓴다).</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표로 후보를 찾는 공간 해시.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>타입 번호 → 타입 레코드. 발자국을 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입별 높이 확장. 확장 높이 호출 때 발자국 세로에 더한다.</summary>
    private readonly IReadOnlyList<int> _heightExtensions;

    /// <summary>탐색 커서 상태.</summary>
    private SquidSearchState _state;

    /// <summary>현재 후보가 검사 조건을 만족하는지 정하는 콜백.</summary>
    private Func<int, bool>? _filter;

    /// <summary>같은 버킷의 재방문을 거른다. 다른 버킷의 중복은 원본처럼 허용한다.</summary>
    private readonly HashSet<ushort> _chain = [];

    /// <summary>Begin 으로 회귀를 시작했는지.</summary>
    private bool _started;

    /// <summary>
    /// 풀·해시·타입·높이 확장으로 탐색기를 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드.</param>
    /// <param name="heightExtensions">타입별 높이 확장. 비우면 확장 높이 호출을 거부한다.</param>
    public RawSquidFinder(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<int>? heightExtensions = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        _pool = pool;
        _hash = hash;
        _types = types;
        _heightExtensions = heightExtensions ?? Array.Empty<int>();
    }

    /// <summary>탐색 커서를 읽기 전용으로 본다. 독립 실행 기계어 대조에 현재 상태를 공개한다.</summary>
    public SquidSearchState State => _state;

    /// <summary>
    /// 탐색 사각형으로 회귀를 시작한다. 해시 0단계(표면 생략이면 1단계)부터 다음 후보를 찾는다.
    /// </summary>
    /// <param name="area">탐색 사각형.</param>
    /// <param name="flags">호출 플래그(1·4 조합).</param>
    /// <param name="filter">후보 검사 콜백. 비우면 모두 받는다.</param>
    public int Begin(SquidSearchArea area, uint flags = 0, Func<int, bool>? filter = null)
    {
        // 1·4 밖의 플래그는 거부한다
        if ((flags & ~AllowedFlags) != 0)
        {
            throw new ArgumentException("원시 찾기 플래그 오류", nameof(flags));
        }
        // 확장 높이인데 높이 표가 타입 표보다 짧으면 거부한다
        if ((flags & ExtendedHeightFlag) != 0 && _heightExtensions.Count < _types.Count)
        {
            throw new ArgumentException("원시 찾기 확장 높이 표 오류", nameof(flags));
        }
        area = new SquidSearchArea(
            Math.Max(area.Left, 0),
            Math.Max(area.Top, 0),
            Math.Min(area.Right == -1 ? RawSquidHash.WorldCells : area.Right, RawSquidHash.WorldCells - 1),
            Math.Min(area.Bottom == -1 ? RawSquidHash.WorldCells : area.Bottom, RawSquidHash.WorldCells - 1));
        _state = new SquidSearchState
        {
            Area = area,
            SkipSurface = (flags & SkipSurfaceFlag) != 0,
            ExtendedHeight = (flags & ExtendedHeightFlag) != 0,
        };
        _filter = filter;
        _started = true;
        SetLevel(_state.SkipSurface ? 1 : 0);
        _state.Next = _hash.Cell(_state.Level, _state.X, _state.Y);
        return Next();
    }

    /// <summary>
    /// y·x·단계 순서로 탐색하며 이전 반환의 next chain 을 먼저 쓴다.
    /// 후보 extra·좌표·타입과 적 raw 체인·버킷 머리를 그때 값으로 읽는다.
    /// </summary>
    public int Next()
    {
        if (!_started)
        {
            throw new InvalidOperationException("원시 찾기가 시작되지 않았습니다");
        }
        if (_state.Level > 3)
        {
            return 0;
        }
        // 단계가 끝날 때까지 버킷 범위와 원본 체인 순서대로 방문한다
        for (; ; )
        {
            if (_state.Next == 0)
            {
                if (!AdvanceBucket())
                {
                    return 0;
                }
                continue;
            }
            _state.Current = _state.Next;
            if (_state.Current >= _pool.Capacity)
            {
                throw new ArgumentOutOfRangeException(nameof(_state.Current), _state.Current, "원시 찾기 번호 범위 오류");
            }
            // 같은 버킷의 재방문은 원본처럼 멈춘다
            if (!_chain.Add((ushort)_state.Current))
            {
                throw new InvalidOperationException("원시 찾기 체인 순환");
            }
            ReadOnlySpan<byte> raw = _pool.Slot(_state.Current);
            _state.Next = (int)Read(raw, NextOffset, 2);
            // 매립 후보는 건너뛴다
            if ((raw[ExtraOffset] & BuriedBit) != 0)
            {
                continue;
            }
            if (Intersects(_state.Current) && (_filter is null || _filter(_state.Current)))
            {
                return _state.Current;
            }
        }
    }

    /// <summary>한 단계에서 첫·마지막 버킷을 정하고 체인 재방문 기록을 지운다.</summary>
    /// <param name="level">공간 해시 단계 0~3.</param>
    private void SetLevel(int level)
    {
        _state.Level = level;
        int scale = RawSquidHash.WorldCells / RawSquidHash.Side(level);
        int last = RawSquidHash.Side(level) - 1;
        _state.FirstX = Math.Min(_state.Area.Left / scale, last);
        _state.FirstY = Math.Min(_state.Area.Top / scale, last);
        _state.LastX = Math.Clamp(Math.Min(_state.Area.Right / scale, last) + 1, 0, last);
        _state.LastY = Math.Clamp(Math.Min(_state.Area.Bottom / scale, last) + 1, 0, last);
        _state.X = _state.FirstX;
        _state.Y = _state.FirstY;
        _state.Current = 0;
        _chain.Clear();
    }

    /// <summary>체인 다음 번호를 먼저 확보하고 다음 버킷·단계로 전진한다. 끝이면 거짓이다.</summary>
    private bool AdvanceBucket()
    {
        _state.X++;
        if (_state.X > _state.LastX)
        {
            _state.X = _state.FirstX;
            _state.Y++;
            if (_state.Y > _state.LastY)
            {
                _state.Level++;
                if (_state.Level > 3)
                {
                    _state.Current = 0;
                    return false;
                }
                SetLevel(_state.Level);
            }
        }
        _chain.Clear();
        _state.Next = _hash.Cell(_state.Level, _state.X, _state.Y);
        return true;
    }

    /// <summary>
    /// 현재 후보의 발자국이 탐색 사각형과 겹치는지 본다. 보드 밖·0 이하 좌표는 건너뛰고, 타입·발자국이 깨지면 거부한다.
    /// </summary>
    /// <param name="sid">후보 번호.</param>
    private bool Intersects(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        float x = BitConverter.UInt32BitsToSingle(Read(raw, XOffset));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, YOffset));
        // 보드 밖·0 이하 좌표는 건너뛴다
        if (!float.IsFinite(x) || !float.IsFinite(y) || x <= 0 || y <= 0 || x >= RawSquidHash.WorldCells || y >= RawSquidHash.WorldCells)
        {
            return false;
        }
        uint number = raw[TypeOffset];
        if (number >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), number, "원시 찾기 타입 범위 오류");
        }
        OriginalTypeRecord type = _types[(int)number];
        long height = (long)type.FootY + (_state.ExtendedHeight ? _heightExtensions[(int)number] : 0);
        if (type.FootX < 1 || height < 1 || height > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "원시 찾기 발자국 범위 오류");
        }
        int right = (int)x;
        int bottom = (int)y;
        long left = (long)right - type.FootX + 1;
        long top = (long)bottom - height + 1;
        return (_state.Area.Left <= right || _state.Area.Left <= left) && (right <= _state.Area.Right || left <= _state.Area.Right)
            && (_state.Area.Top <= bottom || _state.Area.Top <= top) && (bottom <= _state.Area.Bottom || top <= _state.Area.Bottom);
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
