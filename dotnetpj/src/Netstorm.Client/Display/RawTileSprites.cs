// 수집한 타일 항목의 일반 그림을 실제 SHP 픽셀로 출력한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>타일 묶음 멤버의 일반 타일 그림을 같은 풀·타입·SHP에서 직접 합성한다.</summary>
/// <remarks>cpppj RawCampaignFactoryDisplay.cpp의 DrawEntry가 타일에 쓰는 SpriteDraw::Draw 일반 분기(498220 제외) 대응이다. 소유자·그림자·죽음·추상·가상 분기와 선택선·막대는 R5-3c에서 잇는다. 2026-11-26 1.070에서 R5-3b용으로 추가했다.</remarks>
public static class RawTileSprites
{
    /// <summary>타일 토큰의 기본이다. 수집기와 같은 기준을 쓴다.</summary>
    private const uint TokenBase = 0;

    /// <summary>원문 CRT signed64 절삭의 하위 DWORD를 반환한다. 유한 범위 밖은 거부한다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>하위 DWORD다.</returns>
    private static int Truncate(double value) => SpriteDraw.Truncate(value);

    /// <summary>수집한 타일 항목 하나를 8비트 화면에 그린다.</summary>
    /// <param name="entry">타일 묶음 멤버 항목이다.</param>
    /// <param name="pool">미션 SID 풀이다.</param>
    /// <param name="types">미션 타입 표다.</param>
    /// <param name="database">타입 로딩 순서의 SHP 저장소다.</param>
    /// <param name="cameraX">카메라의 화면 가로 원점이다.</param>
    /// <param name="cameraY">카메라의 화면 세로 원점이다.</param>
    /// <param name="pixels">그리기 화면(행 간격 포함 8비트 버퍼).</param>
    /// <param name="width">화면 폭이다.</param>
    /// <param name="height">화면 세로 크기다.</param>
    /// <param name="clip">출력을 제한하는 클리핑 영역(half-open).</param>
    public static void DrawTile(RenderCollectionEntry entry, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ShapeDatabase database,
        int cameraX, int cameraY, Span<byte> pixels, int width, int height, ScreenRect clip)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(database);
        if (entry.Token < TokenBase)
        {
            throw new ArgumentOutOfRangeException(nameof(entry), "타일 토큰 오류");
        }
        uint offset = entry.Token - TokenBase;
        if (offset == 0 || offset % RawSidPool.Stride != 0 || offset / RawSidPool.Stride >= (uint)pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(entry), "타일 SID 오류");
        }
        int sid = unchecked((ushort)(offset / RawSidPool.Stride));
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        uint type = raw[10];
        if (type >= types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(entry), "타일 타입 오류");
        }
        OriginalTypeRecord record = types[(int)type];
        bool patch = pool.Edition == OriginalEdition.Patch1078;
        uint frame = patch ? BinaryPrimitives.ReadUInt32LittleEndian(raw[36..]) : raw[34];
        byte extra = raw[patch ? 40 : 35];
        // 일반 타일이 아니면 R5-3c에서 잇는다
        if ((entry.Tail & 4) != 0 || (extra & 0x10) != 0 || (extra & 1) != 0 || (extra & 2) != 0)
        {
            throw new InvalidOperationException("타일 일반 외 그리기 미연결(R5-3c)");
        }
        int block = checked((int)type - 70);
        if (block < 0 || block >= database.Blocks().Count)
        {
            throw new ArgumentOutOfRangeException(nameof(entry), "타일 셰이프 블록 오류");
        }
        if (frame >= (uint)database.Blocks()[block].Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(entry), "타일 셰이프 프레임 오류");
        }
        int x = unchecked(BinaryPrimitives.ReadInt16LittleEndian(raw[22..]) - cameraX);
        int y = unchecked(BinaryPrimitives.ReadInt16LittleEndian(raw[24..]) - cameraY);
        float wx = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
        float wy = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
        if (Truncate((double)wx * 16 + 0.5) - cameraX != x || Truncate((double)wy * 11 + 0.5) - cameraY != y)
        {
            throw new ArgumentException("타일 월드/화면 좌표 불일치");
        }
        var point = new ScreenPoint(unchecked(x - record.HotspotX), unchecked(y - record.HotspotY));
        var pane = new ShapeRect(clip.Left, clip.Top, clip.Right - 1, clip.Bottom - 1);
        int drawn = VfxDraw.DrawShape(pixels, width, height, database, block, checked((int)frame), point.X, point.Y, pane);
        // 화면 밖(-3)은 가장자리 타일의 정상이며 나머지는 호출 계약 오류다
        if (drawn is -1 or -2 or -4)
        {
            throw new InvalidOperationException("타일 그리기 화면/프레임 오류: " + drawn);
        }
    }
}
