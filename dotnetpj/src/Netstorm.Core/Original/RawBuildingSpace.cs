namespace Netstorm.Core.Original;

/// <summary>
/// 건물 Pop/Unpop 이 쓰는 표면 영역과 판본 진단 영역(원본 BuildingSpaceState).
/// 섬·다리 자산의 타입 번호와 진단 출력 여부를 담는다.
/// </summary>
public sealed class BuildingSpaceState
{
    /// <summary>섬 자산을 타입 표에서 찾는 번호.</summary>
    public uint IslandType { get; set; }

    /// <summary>다리 자산을 타입 표에서 찾는 번호.</summary>
    public uint BridgeType { get; set; }

    /// <summary>판본 assert 경계의 진단 출력을 검사할지.</summary>
    public bool Debug { get; set; }
}

/// <summary>
/// 건물 발자국 옆면의 점유 바이트를 쓰는 표면 공간 모듈(원본 RawBuildingSpace, 10.78).
/// 해시 0단계에서 칸의 표면 번호를 찾고, 건물 좌표의 지면이 섬이면 옆면 WORD 의 bit1을 지우고 bit2를 켠다.
/// Unpop 은 옆면 WORD 의 bit1·bit2를 지운다. 다리 수명 nibble 이 7을 넘기면 진단 모드에서 멈춘다.
/// Pop·Unpop 의 건물 옆면 연결 지점(<see cref="IRawPopBuildingSide"/>·<see cref="IRawBuildingSide"/>)을 구현한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 표면 شرط·진단 방식이 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/RawBuildingSpace.cpp (W02 건물 공간 조각, 2026-10-20).
/// </summary>
public sealed class RawBuildingSpace : IRawPopBuildingSide, IRawBuildingSide
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 표면 WORD(2바이트)의 위치.</summary>
    private const int WordOffset = 12;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>추가 비트의 매립(buried) 비트. UnpopSide 경로의 표면 갱신 조건에 쓴다.</summary>
    private const byte BuriedBit = 8;

    /// <summary>표면 타입 플래그1(원본 TypeFlag1::kSurface).</summary>
    private const uint SurfaceFlag = 0x800;

    /// <summary>다리 수명 nibble 이ALLOWED 최대값. 넘기면 진단 모드에서 멈춘다.</summary>
    private const int BridgeLifeMax = 7;

    /// <summary>슬롯을 읽고 쓸 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표로 표면 번호를 찾는 공간 해시.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>타입 번호 → 타입 레코드. 표면 플래그를 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>판본 표면 영역과 진단 영역.</summary>
    private readonly BuildingSpaceState _state;

    /// <summary>
    /// 같은 풀·같은 해시·타입 표·상태로 건물 공간을 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시. Pop·Unpop 과 같은 해시여야 한다.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드. 10.78 은 188개여야 한다.</param>
    /// <param name="state">섬·다리 타입 번호와 진단 여부.</param>
    public RawBuildingSpace(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, BuildingSpaceState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        // CD 10.72(171개)는 표면 판정·진단 방식이 다르므로 받지 않는다
        if (types.Count != OriginalConstructors.TypeCount
            || state.IslandType >= (uint)types.Count || state.BridgeType >= (uint)types.Count)
        {
            throw new ArgumentException("건물 공간 타입 표·섬·다리 번호 오류", nameof(types));
        }
        _pool = pool;
        _hash = hash;
        _types = types;
        _state = state;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. Pop·Unpop 과 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>이 몸체가 쓰는 공간 해시. Pop·Unpop 과 같은 해시인지 확인하는 데 쓴다.</summary>
    public RawSquidHash Hash => _hash;

    /// <summary>
    /// 칸의 표면 번호를 돌려준다. 지도 밖이면 0이며 조회할 때 WORD 상태는 검사하지 않는다.
    /// </summary>
    /// <param name="x">칸 x(0~255).</param>
    /// <param name="y">칸 y(0~255).</param>
    public int Surface(int x, int y)
    {
        // 지도 밖은 원본처럼 0이다
        if (x < 0 || y < 0 || x >= RawSquidHash.WorldCells || y >= RawSquidHash.WorldCells)
        {
            return 0;
        }
        return _hash.Bucket(0, x, y);
    }

    /// <summary>
    /// 지면이 현재 섬 타입인지 돌려준다. 건물 Pop 은 건물 객체의 좌표를 전달한다.
    /// float 좌표는 0 방향으로 절삭하며 범위를 벗어나면 거부한다.
    /// </summary>
    /// <param name="x">월드 x 좌표.</param>
    /// <param name="y">월드 y 좌표.</param>
    public bool HasIsland(float x, float y)
    {
        int sid = Surface(Truncate(x), Truncate(y));
        return sid != 0 && _pool.Slot(sid)[TypeOffset] == _state.IslandType;
    }

    /// <summary>
    /// Pop 이 건물 발자국의 왼쪽·오른쪽 가장자리 열(맨 위·맨 아래 행 제외)에 놓는 건물 점유 바이트를 쓴다.
    /// 표면이 아니면 두고, 섬 지면이면 WORD bit2를 켜고 bit1을 지우며, 아니면 bit1·bit2를 지운다.
    /// </summary>
    /// <param name="cellX">칸 x.</param>
    /// <param name="cellY">칸 y.</param>
    /// <param name="buildingX">건물 오브젝트의 월드 x 좌표.</param>
    /// <param name="buildingY">건물 오브젝트의 월드 y 좌표.</param>
    public void PopSide(int cellX, int cellY, float buildingX, float buildingY)
    {
        int sid = Surface(cellX, cellY);
        if (sid == 0 || sid >= _pool.Capacity)
        {
            return;
        }
        ReadOnlySpan<byte> old = _pool.Slot(sid);
        if (old[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(cellX), old[TypeOffset], "건물 표면 타입 번호 범위 오류");
        }
        // 표면 타입이 아니면 그대로 둔다
        if ((_types[old[TypeOffset]].Flags1 & SurfaceFlag) == 0)
        {
            return;
        }
        bool island = HasIsland(buildingX, buildingY);
        Span<byte> raw = _pool.RawSlot(sid);
        // 섬이면 WORD bit2를 켜고 bit1을 지운다. 아니면 두 비트를 지운다
        raw[WordOffset] = island ? (byte)((raw[WordOffset] & 0xfb) | 2) : (byte)(raw[WordOffset] & 0xf9);
        NotifySurface(sid);
    }

    /// <summary>
    /// 발자국 옆면의 한 칸에 놓인 건물 점유 바이트를 푼다. 표면 타입 검사는 Pop 때 이미 했으므로 다시 하지 않는다.
    /// </summary>
    /// <param name="cellX">칸 x.</param>
    /// <param name="cellY">칸 y.</param>
    public void UnpopSide(int cellX, int cellY)
    {
        int sid = Surface(cellX, cellY);
        if (sid == 0 || sid >= _pool.Capacity)
        {
            return;
        }
        _pool.RawSlot(sid)[WordOffset] &= 0xf9;
        NotifySurface(sid);
    }

    /// <summary>
    /// 실제 무상 다리 수명 진단(원본 004214a0). 진단이 켜졌는데 다리 수명 nibble 이 7을 넘으면 멈춘다.
    /// 꺼져 있으면 수명이 잘못돼도 쓰기·추가 효과 없이 그대로 둔다.
    /// </summary>
    /// <param name="sid">표면 번호.</param>
    public void NotifySurface(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        // 다리 수명이 7을 넘기면 진단 모드에서 원본처럼 멈춘다
        if (raw[TypeOffset] == _state.BridgeType && (raw[ExtraOffset] & BuriedBit) == 0
            && _state.Debug && ((raw[WordOffset] >> 3) & 15) > BridgeLifeMax)
        {
            throw new InvalidOperationException("다리 수명 진단: num >= 0 && num <= 7");
        }
    }

    /// <summary>
    /// 같은 풀·같은 해시를 쓰는 구성이 맞는지 확인한다. 어긋나면 Pop·Unpop 의 연결에서 거부된다.
    /// </summary>
    /// <param name="pool">Pop·Unpop 이 쓰는 풀.</param>
    /// <param name="hash">Pop·Unpop 이 쓰는 해시.</param>
    public void ValidateSpace(RawSidPool pool, RawSquidHash hash)
    {
        // 표면 지도 identity 가 다른 풀·해시와 섞이면 거부한다
        if (!ReferenceEquals(pool, _pool) || !ReferenceEquals(hash, _hash))
        {
            throw new ArgumentException("건물 공간 풀·해시 불일치", nameof(pool));
        }
    }

    /// <summary>원본 CRT 절삭의 유한 int 범위에서 절삭값을 돌려준다.</summary>
    /// <param name="value">자를 실수 좌표.</param>
    private static int Truncate(float value)
    {
        double truncated = Math.Truncate(value);
        // 무한·비수·int 범위를 벗어나면 원본처럼 거부한다
        if (!double.IsFinite(truncated) || truncated < int.MinValue || truncated > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "건물 좌표 절삭 범위 오류");
        }
        return (int)truncated;
    }
}
