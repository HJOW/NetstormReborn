using System.Buffers.Binary;

namespace Netstorm.Core.Original; // 실제 공간 해시와50BYTE 슬롯에서 미니맵 셀을 그린다.

/// <summary>원본 셀 그림이 매 호출에서 읽는 미니맵 전용 전역이다.</summary>
public sealed class MinimapRasterState
{
    /// <summary>지면 색을 쓰는 Isle TYPE다.</summary>
    public uint IsleType { get; set; } = 97;
    /// <summary>두 번째 지면 색을 쓰는 NoIsland TYPE다.</summary>
    public uint NoIslandType { get; set; } = 157;
    /// <summary>Challenge 초록 표식 TYPE다.</summary>
    public uint BattleIslandType { get; set; } = 109;
    /// <summary>Challenge 자홍 표식 TYPE다.</summary>
    public uint PlayerIslandType { get; set; } = 108;
    /// <summary>현재 Challenge 모드다.</summary>
    public uint Challenge { get; set; }
    /// <summary>비영이면 현재 프레임 하위BYTE로 색을 대체한다.</summary>
    public uint SuppressColors { get; set; }
    /// <summary>색 억제에서 사용하는 현재DWORD다.</summary>
    public uint Frame { get; set; }
    /// <summary>2×2 초기화 색이다.</summary>
    public uint Black { get; set; }
    /// <summary>중립 Isle의 전용 색이다.</summary>
    public uint NeutralIsle { get; set; }
    /// <summary>중립 NoIsland의 전용 색이다.</summary>
    public uint NeutralNoIsland { get; set; }
    /// <summary>Challenge BattleIsland의 전용 색이다.</summary>
    public uint ChallengeGreen { get; set; }
    /// <summary>owner1..8의 실제 미니맵 전용 색이며 일반 Player 색표와 다르다.</summary>
    public uint[] Owners { get; } = new uint[8];
}

/// <summary>cpppj/src/o/RawMinimapRaster.cpp의 실제 지면/해시 순서와 BYTE 쓰기 본체다.</summary>
public sealed class RawMinimapRaster
{
    /// <summary>bitmap 한 변이다.</summary>
    private const int Width = 128;
    /// <summary>셀 묶음 한 변이다.</summary>
    private const int Cells = 64;
    /// <summary>현재 슬롯의type/next/owner/좌표를 읽는 같은 물리 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>지면 WORD와3단계 버킷이 같은 공간 저장소다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 flags2를 읽는 실제 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>복사하지 않는 현재 전용 색/모드 전역이다.</summary>
    private readonly MinimapRasterState _state;
    /// <summary>같은10.78 Pool/Hash/TYPE/전역을 빌린다. bitmap은 각 DrawCell에서 별도로 빌린다.</summary>
    public RawMinimapRaster(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, MinimapRasterState state)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count == 0) throw new ArgumentException("미니맵 셀 그림의 실제10.78 Pool/TYPE가 필요합니다");
        _pool = pool; _hash = hash; _types = types; _state = state;
    }
    /// <summary>owner1..8 switch와 fallback을 매번 읽으며 색 억제는 현재 프레임 하위BYTE다.</summary>
    private byte OwnerColor(byte owner, uint fallback) => unchecked((byte)(_state.SuppressColors != 0 ? _state.Frame : owner is >= 1 and <= 8 ? _state.Owners[owner - 1] : fallback));
    /// <summary>현재 정렬되지 않은 좌표를 x87 signed64 ftol의 하위DWORD로 절삭한다.</summary>
    private static int Coordinate(ReadOnlySpan<byte> raw, int at)
    {
        double value = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[at..]));
        return !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    }
    /// <summary>실제128폭 bitmap의 한BYTE만 쓴다. 손상 좌표는 명시적으로 거부한다.</summary>
    private static void Pixel(Span<byte> bitmap, int x, int y, byte color)
    {
        if (x < 0 || x >= Width || y < 0 || y >= Width) throw new ArgumentOutOfRangeException(nameof(x), "미니맵 픽셀 좌표 범위 오류");
        bitmap[y * Width + x] = color;
    }
    /// <summary>현재 슬롯의+4 next를 효과 뒤에 읽으며 원문의 일반점/Challenge 네 점을 덮어쓴다.</summary>
    private void Chain(Span<byte> bitmap, ushort head, bool challengeMarks)
    {
        int visited = 0;
        // 정상 체인의 방문 순서를 유지하고 손상된 순환만 거부한다.
        for (ushort sid = head; sid != 0;)
        {
            if (++visited > _pool.Capacity) throw new InvalidOperationException("미니맵 해시 체인의 순환");
            var raw = _pool.Slot(sid);
            if (_state.Challenge == 0)
            {
                if (raw[10] >= _types.Count) throw new ArgumentOutOfRangeException(nameof(head), "미니맵 해시의 TYPE 범위 오류");
                uint flags = _types[raw[10]].Flags2;
                if ((flags & 6) != 0)
                {
                    int x = Coordinate(raw, 14), y = Coordinate(raw, 18);
                    if ((flags & 4) != 0) Pixel(bitmap, x / 2, y / 2, OwnerColor(raw[34], 246));
                }
            }
            else if (challengeMarks && (raw[10] == _state.BattleIslandType || raw[10] == _state.PlayerIslandType))
            {
                int x = (Coordinate(raw, 14) / 2) & ~1, y = (Coordinate(raw, 18) / 2) & ~1;
                byte color = unchecked((byte)(_state.SuppressColors != 0 ? _state.Frame : raw[10] == _state.BattleIslandType ? _state.ChallengeGreen : _state.Owners[4]));
                Pixel(bitmap, x, y, color); Pixel(bitmap, x + 1, y, color); Pixel(bitmap, x, y + 1, color); Pixel(bitmap, x + 1, y + 1, color);
            }
            sid = BinaryPrimitives.ReadUInt16LittleEndian(raw[4..]);
        }
    }
    /// <summary>004808d0. 2×2 초기화→지면4×4→해시2/1/0의 원래 X/Y 순서로 덮는다.</summary>
    public void DrawCell(Span<byte> bitmap, int cellX, int cellY)
    {
        if (bitmap.Length != Width * Width || cellX < 0 || cellX >= Cells || cellY < 0 || cellY >= Cells) throw new ArgumentOutOfRangeException(nameof(bitmap), "미니맵 bitmap/셀 범위 오류");
        int px = cellX * 2, py = cellY * 2; byte black = unchecked((byte)_state.Black);
        Pixel(bitmap, px, py, black); Pixel(bitmap, px + 1, py, black); Pixel(bitmap, px, py + 1, black); Pixel(bitmap, px + 1, py + 1, black);
        // 지면은 Y 바깥/X 안쪽으로 읽어 같은 픽셀을 마지막 지면 색으로 덮는다.
        for (int y = cellY * 4; y < cellY * 4 + 4; y++)
        {
            // Hash0의 현재 머리만 지면으로 읽으며 next는 따라가지 않는다.
            for (int x = cellX * 4; x < cellX * 4 + 4; x++)
            {
                ushort sid = _hash.Cell(0, x, y); if (sid == 0) continue; var raw = _pool.Slot(sid);
                if (raw[10] == _state.IsleType || raw[10] == _state.NoIslandType) Pixel(bitmap, x / 2, y / 2, OwnerColor(raw[34], raw[10] == _state.IsleType ? _state.NeutralIsle : _state.NeutralNoIsland));
            }
        }
        Chain(bitmap, _hash.Cell(2, cellX, cellY), false);
        // 해시1은 지면과 달리 X 바깥/Y 안쪽의2×2 순서다.
        for (int x = px; x < px + 2; x++)
        {
            // 현재 체인을 마친 뒤 다음 Y 버킷으로 이동한다.
            for (int y = py; y < py + 2; y++) Chain(bitmap, _hash.Cell(1, x, y), true);
        }
        // 해시0도 X 바깥/Y 안쪽이며 이미 그린 지면도 일반 flags 검사를 거친다.
        for (int x = cellX * 4; x < cellX * 4 + 4; x++)
        {
            // 같은 실제 머리의 현재 next 순서를 사용한다.
            for (int y = cellY * 4; y < cellY * 4 + 4; y++) Chain(bitmap, _hash.Cell(0, x, y), true);
        }
    }
}
