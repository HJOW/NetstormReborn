// 수집한 타일 항목의 일반·소유자·그림자·죽음·추상 그림을 실제 SHP 픽셀로 출력한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Client.Display;

/// <summary>타일 그리기 모드에 쓰는 소유자 표·색표와 특수 모드 값이다.</summary>
public sealed class RawTileDrawModes
{
    /// <summary>특수 추상 표시 모드다.</summary>
    public uint SpecialMode { get; set; }
    /// <summary>현재 로컬 플레이어다.</summary>
    public uint LocalPlayer { get; set; }
    /// <summary>특수 표시 대상 flags2 마스크다.</summary>
    public uint FilteredFlags { get; set; }
    /// <summary>소유자를 +12 WORD에서 읽는 타입이다.</summary>
    public uint WideOwnerType { get; set; }
    /// <summary>특수 모드에서 항상 추상 출력하는 타입이다.</summary>
    public uint UnfilteredType { get; set; }
    /// <summary>타입별 소유자 변환표 존재 여부다. 비어 있으면 소유자 표가 없다.</summary>
    public byte[] OwnerMaps { get; set; } = [];
    /// <summary>소유자 번호의 256색표다. 없으면 소유자 그림을 그리지 않는다.</summary>
    public Func<uint, byte[]?>? OwnerColors { get; set; }
    /// <summary>그림자 256색표다. 없으면 그림자를 그리지 않는다.</summary>
    public byte[]? ShadowTable { get; set; }
    /// <summary>죽음 256색표다. 없으면 죽음을 그리지 않는다.</summary>
    public byte[]? DeadTable { get; set; }
    /// <summary>추상 256색표다. 없으면 추상을 그리지 않는다.</summary>
    public byte[]? AbstractTable { get; set; }
}

/// <summary>타일 묶음 멤버의 타일 그림을 같은 풀·타입·SHP에서 직접 합성한다.</summary>
/// <remarks>cpppj RawCampaignFactoryDisplay.cpp의 DrawEntry가 타일에 쓰는 SpriteDraw::Draw 분기(498220 제외) 대응이다. 가상 분기는 공장 MainFrame이 필요해 R5-3c2에서 잇는다. 2026-11-26 1.070에서 R5-3b용으로 추가하고 1.071에서 모드 분기를 이었다.</remarks>
public static class RawTileSprites
{
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
    /// <param name="modes">소유자 표·색표와 특수 모드 값이다.</param>
    public static void DrawTile(RenderCollectionEntry entry, RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ShapeDatabase database,
        int cameraX, int cameraY, Span<byte> pixels, int width, int height, ScreenRect clip, RawTileDrawModes modes)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(database);
        ArgumentNullException.ThrowIfNull(modes);
        uint offset = entry.Token;
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
        int x = unchecked(BinaryPrimitives.ReadInt16LittleEndian(raw[22..]) - cameraX);
        int y = unchecked(BinaryPrimitives.ReadInt16LittleEndian(raw[24..]) - cameraY);
        float wx = BinaryPrimitives.ReadSingleLittleEndian(raw[14..]);
        float wy = BinaryPrimitives.ReadSingleLittleEndian(raw[18..]);
        if (Truncate((double)wx * 16 + 0.5) - cameraX != x || Truncate((double)wy * 11 + 0.5) - cameraY != y)
        {
            throw new ArgumentException("타일 월드/화면 좌표 불일치");
        }
        uint owner = type == modes.WideOwnerType ? BinaryPrimitives.ReadUInt16LittleEndian(raw[12..]) : raw[patch ? 34 : 32];
        var point = new ScreenPoint(unchecked(x - record.HotspotX), unchecked(y - record.HotspotY));
        if ((entry.Tail & 4) != 0)
        {
            if ((extra & 0x10) != 0)
            {
                return;
            }
            if (modes.SpecialMode != 0 || (extra & 1) != 0)
            {
                return;
            }
            uint index = frame;
            if (record.FrameCount <= unchecked((int)index))
            {
                index = unchecked(index - (uint)record.FrameCount);
            }
            Blit(database, type, index, point, pixels, width, height, clip,
                modes.ShadowTable ?? throw new InvalidOperationException("타일 그림자 색표 미연결"), shadow: true);
            return;
        }
        if ((extra & 2) != 0)
        {
            throw new InvalidOperationException($"타일 가상 그리기 미연결(R5-3c2): SID={sid}, TYPE={type}, frame={frame}, flags=0x{extra:x2}");
        }
        if ((extra & 1) != 0)
        {
            Blit(database, type, frame, point, pixels, width, height, clip,
                modes.DeadTable ?? throw new InvalidOperationException("타일 죽음 색표 미연결"));
            return;
        }
        if ((extra & 0x10) != 0)
        {
            if (owner == modes.LocalPlayer)
            {
                Blit(database, type, frame, point, pixels, width, height, clip,
                    modes.AbstractTable ?? throw new InvalidOperationException("타일 추상 색표 미연결"));
            }
            return;
        }
        bool filtered = (record.Flags2 & modes.FilteredFlags) != 0 || type == modes.WideOwnerType || type == modes.UnfilteredType;
        if (modes.SpecialMode != 0 && filtered)
        {
            Blit(database, type, frame, point, pixels, width, height, clip,
                modes.AbstractTable ?? throw new InvalidOperationException("타일 추상 색표 미연결"));
            return;
        }
        if (type >= modes.OwnerMaps.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(entry), "타일 소유자 표 오류");
        }
        if (modes.OwnerMaps[type] != 0 && owner != 0)
        {
            byte[] table = (modes.OwnerColors ?? throw new InvalidOperationException("타일 소유자 색표 미연결"))(owner)
                ?? throw new InvalidOperationException("타일 소유자 색표 미연결");
            Blit(database, type, frame, point, pixels, width, height, clip, table);
            return;
        }
        Blit(database, type, frame, point, pixels, width, height, clip, null);
    }

    /// <summary>원문 CRT signed64 절삭의 하위 DWORD를 반환한다. 유한 범위 밖은 거부한다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>하위 DWORD다.</returns>
    private static int Truncate(double value) => SpriteDraw.Truncate(value);

    /// <summary>SHP 프레임을 기준점에 색표·그림자 지정으로 합성한다.</summary>
    /// <param name="database">타입 로딩 순서의 SHP 저장소다.</param>
    /// <param name="type">그림 타입 번호다.</param>
    /// <param name="frame">물리 프레임 번호다.</param>
    /// <param name="point">화면 기준점이다.</param>
    /// <param name="pixels">그리기 화면이다.</param>
    /// <param name="width">화면 폭이다.</param>
    /// <param name="height">화면 세로 크기다.</param>
    /// <param name="clip">출력을 제한하는 클리핑 영역이다.</param>
    /// <param name="colors">색 변환표 256바이트다. 없으면 원본 번호를 그대로 쓴다.</param>
    /// <param name="shadow">참이면 소스 색 대신 대상 화면 색을 표에서 변환한다.</param>
    private static void Blit(ShapeDatabase database, uint type, uint frame, ScreenPoint point,
        Span<byte> pixels, int width, int height, ScreenRect clip, byte[]? colors, bool shadow = false)
    {
        int block = checked((int)type - 70);
        if (block < 0 || block >= database.Blocks().Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "타일 셰이프 블록 오류");
        }
        if (frame >= (uint)database.Blocks()[block].Frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), "타일 셰이프 프레임 오류");
        }
        ShapeFrame shape = database.Blocks()[block].Frames[checked((int)frame)];
        // 특수 셰이프 레코드는 그림이 없으며 원문처럼 조용히 건너뛴다.
        if (shape.IsSpecial())
        {
            return;
        }
        RendererDraw.DrawIndexedImage(pixels, width, height, database.Decode(block, checked((int)frame)),
            unchecked(point.X + shape.Rect.Left), unchecked(point.Y + shape.Rect.Top), clip, colors, shadow);
    }
}
