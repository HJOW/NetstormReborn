using System.Buffers.Binary;

// 건설 발자국의 유닛 이동과 실패한 섬 배치의 정리를 원본 탐색 순서대로 실행한다.
namespace Netstorm.Core.Original;

/// <summary>관계 표·현재 표면TYPE·억제·전파 역할을 같은 월드에서 빌린다.</summary>
public sealed class ConstructionEffectsState
{
    /// <summary>요청 소유자*9+후보 소유자 순서의 독립 동맹 표다.</summary>
    public uint[] Alliances { get; set; } = new uint[81];
    /// <summary>실제 참여자 장치의 현재 관계 표다.</summary>
    public Func<IReadOnlyList<uint>>? CurrentAlliances { get; set; }
    /// <summary>현재 섬TYPE의 독립값이다.</summary>
    public uint IslandType { get; set; } = 94;
    /// <summary>현재 표면 조각TYPE의 독립값이다.</summary>
    public uint NoIslandType { get; set; } = 157;
    /// <summary>현재 종유석TYPE의 독립값이다.</summary>
    public uint StalagType { get; set; } = 95;
    /// <summary>현재 그루터기TYPE의 독립값이다.</summary>
    public uint StumpType { get; set; } = 96;
    /// <summary>진입 때 섬 정리를 생략하는 독립 억제값이다.</summary>
    public bool Blocked { get; set; }
    /// <summary>로딩 깊이와 별개인 실제 억제DWORD다.</summary>
    public Func<uint>? SharedBlocked { get; set; }
    /// <summary>섬 정리를 방송하는 독립 서버 역할이다.</summary>
    public bool Server { get; set; }
    /// <summary>boss와 별개인 실제 서버 역할이다.</summary>
    public Func<bool>? SharedServer { get; set; }
    /// <summary>각 원문 비교에서 읽는 현재 섬TYPE다.</summary>
    public Func<uint>? SharedIslandType { get; set; }
    /// <summary>삭제 뒤에도 다시 읽는 현재 표면TYPE다.</summary>
    public Func<uint>? SharedNoIslandType { get; set; }
    /// <summary>각 후보 비교의 현재 종유석TYPE다.</summary>
    public Func<uint>? SharedStalagType { get; set; }
    /// <summary>앞선 삭제 뒤 현재 그루터기TYPE다.</summary>
    public Func<uint>? SharedStumpType { get; set; }
    /// <summary>장기 억제DWORD가 있으면 그 현재값을 사용한다.</summary>
    public bool IsBlocked() => SharedBlocked is null ? Blocked : SharedBlocked() != 0;
    /// <summary>방송 직전 현재 서버 역할을 읽는다.</summary>
    public bool IsServer() => SharedServer?.Invoke() ?? Server;
    /// <summary>현재 섬TYPE다.</summary>
    public uint Island() => SharedIslandType?.Invoke() ?? IslandType;
    /// <summary>현재 표면TYPE다.</summary>
    public uint NoIsland() => SharedNoIslandType?.Invoke() ?? NoIslandType;
    /// <summary>현재 종유석TYPE다.</summary>
    public uint Stalag() => SharedStalagType?.Invoke() ?? StalagType;
    /// <summary>현재 그루터기TYPE다.</summary>
    public uint Stump() => SharedStumpType?.Invoke() ?? StumpType;
    /// <summary>실제 요청의 현재 동맹 관계를 읽는다.</summary>
    public bool Allied(uint player, uint owner) => (CurrentAlliances?.Invoke() ?? Alliances)[checked((int)(player * 9 + owner))] != 0;
}

/// <summary>실제 이동·삭제·낙하·음향·전파 효과이며 원본 인자 폭을 보존한다.</summary>
public sealed class ConstructionEffectsHooks
{
    /// <summary>현재 유닛TYPE에 대응하는 경로contained TYPE다.</summary>
    public Func<uint, uint>? PathType { get; set; }
    /// <summary>첫 살아 있는 경로contained를 원문0/1 인자로 삭제한다.</summary>
    public Action<int, uint>? ClearPath { get; set; }
    /// <summary>실제 공간에서flags0으로 뺀다.</summary>
    public Action<int, uint>? Unpop { get; set; }
    /// <summary>선택한 정수 좌표에flags0으로 다시 놓는다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>현재 섬 조각의 낙하 표시다.</summary>
    public Action<int>? Fall { get; set; }
    /// <summary>낙하 효과 뒤 현재 좌표에서islandFall.wav를 한 번 낸다.</summary>
    public Action<float, float>? Sound { get; set; }
    /// <summary>현재 후보를 원본flags로 삭제한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>삭제 뒤 현재genus0x10000 후보에 가상 통지한다.</summary>
    public Action<int>? Notify { get; set; }
    /// <summary>초기화와 원격 전파를 묶으며 좌표는16.16 signed DWORD 비트다.</summary>
    public Action<uint, uint, uint, uint>? Broadcast { get; set; }
    /// <summary>선택된 조각 개수 진단이며 비어 있으면 보고만 생략한다.</summary>
    public Action<string, int>? Assertion { get; set; }
    /// <summary>실제 탐색기의Begin 인자를 관찰하며 탐색을 대체하지 않는다.</summary>
    public Action<SquidSearchArea>? Scan { get; set; }
    /// <summary>C++ 함수 값 묶음처럼 대리자를 복사한다.</summary>
    public ConstructionEffectsHooks Copy() => (ConstructionEffectsHooks)MemberwiseClone();
}

/// <summary>00443800/CD004d2450 밀어내기와0046e6c0/CD0048f960 섬 정리 본체다.</summary>
/// <remarks>근거: cpppj/src/o/RawConstructionEffects.cpp·h. 2026-11-20 0.671 이식.</remarks>
public sealed class RawConstructionEffects
{
    /// <summary>원본 큰 모서리의 거의 올림 보정이다.</summary>
    private const double RoundUpBias = 0.9999899864196777;
    /// <summary>좌표를 전송하는16.16 배율이다.</summary>
    private const double WireScale = 65536.0;
    /// <summary>현재 물리 슬롯과후보를 공유하는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>실제 공간 탐색 순서를 제공하는 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>효과 뒤 현재genus/발자국을 재조회한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>실제256×256 점유·통행 지도다.</summary>
    private readonly byte[] _spots;
    /// <summary>현재 전역의비소유 장치다.</summary>
    private readonly ConstructionEffectsState _state;
    /// <summary>원문 효과 대리자 사본이다.</summary>
    private readonly ConstructionEffectsHooks _hooks;

    /// <summary>첫 효과 전에 지도 폭·필수 경계를 확인한다.</summary>
    public RawConstructionEffects(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types,
        byte[] spots, ConstructionEffectsState state, ConstructionEffectsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(spots); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (spots.Length < 65536 || hooks.PathType is null || hooks.ClearPath is null || hooks.Unpop is null || hooks.Pop is null ||
            hooks.Fall is null || hooks.Sound is null || hooks.Destroy is null || hooks.Notify is null || hooks.Broadcast is null)
            throw new ArgumentException("건설 효과 자료/경계 연결 오류");
        _pool = pool; _hash = hash; _types = types; _spots = spots; _state = state; _hooks = hooks.Copy();
    }

    /// <summary>원본 finder 순서로 유닛을 아래/오른쪽에 옮기며 PATCH의 적대 거부 앞 효과도 유지한다.</summary>
    public bool Displace(float x, float y, uint type, uint player)
    {
        if (type >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(type), "건설 밀어내기 타입 오류");
        OriginalTypeRecord record = _types[(int)type]; Validate(x, y, record);
        float[] rectangle = Footprint(x, y, record.FootX, record.FootY);
        if ((double)rectangle[2] - rectangle[0] >= 3f)
        {
            rectangle[0] += 1f; rectangle[1] += 1f; rectangle[2] -= 1f; rectangle[3] -= 1f;
        }
        SquidSearchArea area = Area(rectangle); _hooks.Scan?.Invoke(area);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 실제 탐색기 자체의 저장next를 사용해 앞선 이동·삭제의 순서를 유지한다.
        for (int current = finder.Begin(area); current != 0; current = finder.Next())
        {
            if ((Genus(current) & (Patch ? 0x218000u : 0x210000u)) == 0) continue;
            bool found = false; int targetX = 0, targetY = 0;
            if ((Genus(current) & 0x200000) != 0)
            {
                int cx = Ftol((double)x - (record.FootX - 1) / 2), cy = Ftol((double)y + 1);
                if (Open(cx, cy)) { found = true; targetX = cx; targetY = cy; }
            }
            long baseY = Ftol(y), baseX = Ftol(x); int distance = Patch ? 3 : 2;
            // 아래 줄을 가까운 순서로, 각 줄은 오른쪽에서 왼쪽으로 폭+1칸 조회한다.
            for (long row = baseY + 1; !found && row <= baseY + distance; row++)
            {
                // 왼쪽 바로 바깥 칸도 원문 후보에 포함한다.
                for (long column = baseX; column >= baseX - record.FootX; column--)
                {
                    if (Open(column, row)) { found = true; targetX = (int)column; targetY = (int)row; break; }
                }
            }
            // 아래에서 못 찾으면 오른쪽 열을 가까운 순서로 조회한다.
            for (long column = baseX + 1; !found && column <= baseX + distance; column++)
            {
                // 각 열은 아래에서 위로 높이+1칸 조회한다.
                for (long row = baseY; row >= baseY - record.FootY; row--)
                {
                    if (Open(column, row)) { found = true; targetX = (int)column; targetY = (int)row; break; }
                }
            }
            if (!found) continue;
            uint owner = _pool.Slot(current)[Owner];
            if (Patch && owner != player)
            {
                if (owner >= 9 || player >= 9) throw new ArgumentOutOfRangeException(nameof(player), "건설 밀어내기 관계 표 범위 오류");
                if (!_state.Allied(player, owner) && (Genus(current) & 0x8000) == 0) return false;
            }
            uint path = _hooks.PathType!(_pool.Slot(current)[10]);
            _hooks.ClearPath!(current, path); _hooks.Unpop!(current, 0);
            // 원문이 Unpop 뒤 다시 읽는 현재genus의 유효성도 보존한다.
            _ = Genus(current); _hooks.Pop!(current, targetX, targetY, 0);
        }
        return true;
    }

    /// <summary>현재 억제·전파 뒤 실제 섬 발자국의 같은 소유자 후보를 원문 순서로 정리한다.</summary>
    public void DropIsland(float x, float y, uint flags, uint player)
    {
        if (_state.IsBlocked()) return;
        if (_state.IsServer() && (flags & 0x50) == 0)
            _hooks.Broadcast!(unchecked((uint)Ftol((double)x * WireScale)), unchecked((uint)Ftol((double)y * WireScale)), flags, player);
        if (_state.Island() >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(x), "전파 뒤 섬 정리 타입 오류");
        Validate(x, y, _types[(int)_state.Island()]);
        OriginalTypeRecord island = _types[(int)_state.Island()];
        SquidSearchArea area = Area([(float)((double)x - (island.FootX - 1)), (float)((double)y - (island.FootY - 1)), x, y]);
        _hooks.Scan?.Invoke(area); var finder = new RawSquidFinder(_pool, _hash, _types);
        int noIslands = 0, islands = 0, stalags = 0;
        // finder가 저장한next로 삭제 중에도 같은 후보 방문 순서를 잇는다.
        for (int current = finder.Begin(area); current != 0; current = finder.Next())
        {
            if (_pool.Slot(current)[Owner] != player) continue;
            uint type = _pool.Slot(current)[10];
            if (type == _state.NoIsland() || type == _state.Stalag())
            {
                if ((flags & 0x1000) == 0) { _hooks.Fall!(current); _hooks.Sound!(Float(current, 14), Float(current, 18)); }
                _hooks.Destroy!(current, flags | 0x48);
                if (type == _state.NoIsland()) noIslands++; else stalags++;
            }
            if (type == _state.Stump()) _hooks.Destroy!(current, flags | 0x48);
            if (type == _state.Island()) { _hooks.Destroy!(current, flags); islands++; }
            if ((Genus(current) & 0x10000) != 0) _hooks.Notify!(current);
        }
        if (noIslands != 0 || islands != 0 || stalags != 0)
        {
            if (noIslands != 9) _hooks.Assertion?.Invoke("noIslandTypeCount == 9", 0x7b);
            if (islands != 1) _hooks.Assertion?.Invoke("islandTypeCount == 1", 0x7c);
            if (stalags != 1) _hooks.Assertion?.Invoke("islandStalagTypeCount == 1", 0x7d);
        }
    }

    /// <summary>세션·요청 어댑터의 같은 풀 확인용이다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>현재 실제 판본이다.</summary>
    private bool Patch => _pool.Edition == OriginalEdition.Patch1078;
    /// <summary>현재 판본의 소유자BYTE 위치다.</summary>
    private int Owner => Patch ? 34 : 32;
    /// <summary>각 효과 뒤 후보의 현재TYPE와genus를 다시 읽는다.</summary>
    private uint Genus(int sid)
    {
        uint type = _pool.Slot(sid)[10];
        if (type >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "건설 효과 후보 타입 오류");
        return _types[(int)type].Flags2;
    }
    /// <summary>spot0x10이 없고2/4가 있는 칸만 허용하며 지도 밖은 명시 거부한다.</summary>
    private bool Open(long x, long y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256) throw new ArgumentOutOfRangeException(nameof(x), "건설 밀어내기 지도 조회 오류");
        byte spot = _spots[(int)(y * 256 + x)]; return (spot & 0x10) == 0 && (spot & 6) != 0;
    }
    /// <summary>원본float 발자국의 두 모서리를 구하며 시작이 지도 밖이면 반대 한 점으로 축소한다.</summary>
    private static float[] Footprint(float x, float y, int width, int height)
    {
        float px = Math.Clamp((float)((double)x + 1 - width), 1f, 255f), py = Math.Clamp((float)((double)y + 1 - height), 1f, 255f);
        return !(x > 0 && y > 0 && x < 256 && y < 256) ? [px, py, px, py] : [Math.Min(x, px), Math.Min(y, py), Math.Max(x, px), Math.Max(y, py)];
    }
    /// <summary>작은모서리는절삭·큰모서리는거의올림하며지도보정은실제finder에맡긴다.</summary>
    private static SquidSearchArea Area(float[] rectangle) => new(Ftol(rectangle[0]), Ftol(rectangle[1]),
        Ftol((double)rectangle[2] + RoundUpBias), Ftol((double)rectangle[3] + RoundUpBias));
    /// <summary>유한좌표·0..256 발자국을 확인하며Dude의foot0도허용한다.</summary>
    private static void Validate(float x, float y, OriginalTypeRecord type)
    {
        if (!float.IsFinite(x) || !float.IsFinite(y) || type.FootX < 0 || type.FootY < 0 || type.FootX > 256 || type.FootY > 256)
            throw new ArgumentOutOfRangeException(nameof(x), "건설 효과 좌표/발자국 오류");
    }
    /// <summary>원본signed64절삭의하위DWORD이며비유한·범위밖은0이다.</summary>
    private static int Ftol(double value) => !double.IsFinite(value) || Math.Abs(value) >= 9223372036854775808.0 ? 0 : unchecked((int)(uint)(long)value);
    /// <summary>현재슬롯의비정렬float좌표를그대로읽는다.</summary>
    private float Float(int sid, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(sid).Slice(offset, 4)));
}
