using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 실제 ShapeToBuffer 메모리 캡처(<see cref="RawShapeToBuffer"/>) 계약을 고정한다.
/// 근거: cpppj/src/client/RawShapeToBuffer.cpp. 합성 SHP·TYPE·raw 슬롯으로 분기를 검증하며,
/// 실자료(세션 자산) L1 대조는 R4에서 잇는다. (R2d-2, 2026-10-29)
/// </summary>
public sealed class RawShapeToBufferTests
{
    /// <summary>합성 2픽셀 SHP를 만드는 원본 VFX 바이트. 0번 블록·0번 프레임에 [5, 6]이 들어 있다.</summary>
    private static ShapeDatabase TwoPixelDatabase()
    {
        var image = new RawImage();
        image.Convert(new TerrainBitmapInput(2, 1, [5, 6]), crop: false);
        return new ShapeDatabase(image.Shape().ToArray());
    }

    /// <summary>합성 TYPE 표. 70번만 기준점·HP를 채우고 나머지는 기본값이다.</summary>
    private static List<OriginalTypeRecord> TypeTable(int count)
    {
        var types = new List<OriginalTypeRecord>(count);
        // 판본별 개수만큼 기본 레코드를 채운다
        for (int i = 0; i < count; i++)
        {
            types.Add(new OriginalTypeRecord());
        }
        types[70].MaxHitPoints = 100;
        return types;
    }

    /// <summary>합성 SHP 표시 자료. 70번만 2×1 프레임(기준점 1, 0)을 갖는다.</summary>
    private static List<SquidDisplayShape> ShapeTable(int count)
    {
        var shapes = new List<SquidDisplayShape>(count);
        // 판본별 개수만큼 빈 표시를 채운다
        for (int i = 0; i < count; i++)
        {
            shapes.Add(new SquidDisplayShape());
        }
        shapes[70].FrameCount = 1;
        shapes[70].Loaded = true;
        shapes[70].Frames.Add(new SquidDisplayFrame { Width = 2, Height = 1, HotspotX = 1, HotspotY = 0 });
        return shapes;
    }

    /// <summary>풀에 슬롯을 확보하고 raw를 0으로 둔 뒤 기본 patch 입력을 쓴다.</summary>
    private static int PreparePatchSlot(RawSidPool pool)
    {
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        raw[RawSidPool.TypeOffset] = 70;
        WriteVtable(raw);
        return sid;
    }

    /// <summary>원본 진단에 걸리지 않는 0 아닌 가상 표식을 쓴다(기대 5278920과 같은 자리).</summary>
    private static void WriteVtable(Span<byte> raw)
    {
        // 슬롯 0번 DWORD의 해제 표식(0·0xdddddddd·0xcdcdcdcd)을 피한다
        raw[0] = 0x20;
        raw[1] = 0x74;
        raw[2] = 0x50;
        raw[3] = 0x00;
    }

    /// <summary>plain 경로가 합성 RLE를 버퍼에 그대로 옮긴다.</summary>
    [Fact]
    public void PlainCopiesDecodedPixels()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState());
        ConstructionVisualImage image = capture.Capture(sid);
        Assert.Equal(2, image.Width);
        Assert.Equal(1, image.Height);
        Assert.Equal(1, image.HotspotX);
        Assert.Equal(new byte[] { 5, 6 }, image.Pixels);
    }

    /// <summary>CD 판본도 같은 합성 그림을 1바이트 오프셋으로 읽는다.</summary>
    [Fact]
    public void CdReadsOneByteOffsets()
    {
        var pool = new RawSidPool(32768, edition: OriginalEdition.Cd1072);
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        raw[RawSidPool.TypeOffset] = 70;
        WriteVtable(raw);
        var capture = new RawShapeToBuffer(pool, TypeTable(171), ShapeTable(171), TwoPixelDatabase(), new ShapeToBufferState());
        ConstructionVisualImage image = capture.Capture(sid);
        Assert.Equal(new byte[] { 5, 6 }, image.Pixels);
    }

    /// <summary>CD의 invalid 현재 객체는 8×8·4×4 투명 그림으로 돌려준다.</summary>
    [Fact]
    public void CdInvalidReturnsTransparentFallback()
    {
        var pool = new RawSidPool(32768, edition: OriginalEdition.Cd1072);
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        raw[RawSidPool.TypeOffset] = 70;
        WriteVtable(raw);
        raw[RawSidPool.StateOffset] = RawSidPool.FreeBit;
        var capture = new RawShapeToBuffer(pool, TypeTable(171), ShapeTable(171), TwoPixelDatabase(), new ShapeToBufferState());
        ConstructionVisualImage image = capture.Capture(sid);
        Assert.Equal(8, image.Width);
        Assert.Equal(4, image.HotspotX);
        Assert.Equal(new byte[64], image.Pixels);
    }

    /// <summary>패치의 invalid 현재 객체·frame은 원본 진단으로 거부한다.</summary>
    [Fact]
    public void PatchInvalidThrowsDiagnostic()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        pool.AllocatedBytes(sid)[RawSidPool.StateOffset] = RawSidPool.FreeBit;
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState());
        Assert.Throws<InvalidOperationException>(() => capture.Capture(sid));
    }

    /// <summary>자산 번호 밖 타입은 거부한다.</summary>
    [Fact]
    public void NonAssetTypeRejected()
    {
        var pool = new RawSidPool(32768);
        int sid = pool.Allocate();
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        raw[RawSidPool.TypeOffset] = 69;
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState());
        Assert.Throws<ArgumentOutOfRangeException>(() => capture.Capture(sid));
    }

    /// <summary>해제 가상 표식은 원본 진단으로 거부한다.</summary>
    [Fact]
    public void FreedVtableRejected()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw[0] = 0xDD;
        raw[1] = 0xDD;
        raw[2] = 0xDD;
        raw[3] = 0xDD;
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState());
        Assert.Throws<InvalidOperationException>(() => capture.Capture(sid));
    }

    /// <summary>추가 가상 그림은 소비 후크 없이 거부한다.</summary>
    [Fact]
    public void AdditionalWithoutHooksRejected()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        pool.AllocatedBytes(sid)[40] = 2;
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState());
        Assert.Throws<InvalidOperationException>(() => capture.Capture(sid));
    }

    /// <summary>가상 main -1은 기본 그림을 생략하고 빈 투명 버퍼를 남긴다.</summary>
    [Fact]
    public void VirtualMinusOneSkipsMainDraw()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        pool.AllocatedBytes(sid)[40] = 2;
        var state = new ShapeToBufferState { MainFrame = _ => -1, Additional = (_, _, _, _) => { } };
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), state);
        ConstructionVisualImage image = capture.Capture(sid);
        Assert.Equal(new byte[] { 255, 255 }, image.Pixels);
    }

    /// <summary>소유자 색표가 있으면 mapped 경로로 치환한다.</summary>
    [Fact]
    public void MappedTranslatesOwnerColors()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        pool.AllocatedBytes(sid)[34] = 3;
        var maps = new uint[188];
        maps[70] = 1;
        var table = new byte[256];
        // 표 항목마다 번호를 그대로 둔다
        for (int i = 0; i < 256; i++)
        {
            table[i] = (byte)i;
        }
        table[5] = 9;
        table[6] = 8;
        var state = new ShapeToBufferState { OwnerTypeMaps = maps, OwnerColors = (_, _) => table };
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), state);
        ConstructionVisualImage image = capture.Capture(sid);
        Assert.Equal(new byte[] { 9, 8 }, image.Pixels);
    }

    /// <summary>TYPE 추가 기준점은 마지막에 기준점을 보정한다.</summary>
    [Fact]
    public void AddTypeHotspotAdjustsLast()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        var types = TypeTable(188);
        types[70].HotspotX = 2;
        types[70].HotspotY = 3;
        var state = new ShapeToBufferState { AddTypeHotspot = true };
        var capture = new RawShapeToBuffer(pool, types, ShapeTable(188), TwoPixelDatabase(), state);
        ConstructionVisualImage image = capture.Capture(sid);
        Assert.Equal(3, image.HotspotX);
        Assert.Equal(3, image.HotspotY);
    }

    /// <summary>음수 헤더 치수는 버퍼 크기 진단으로 거부한다(Scale 음수 분기).</summary>
    [Fact]
    public void NegativeHeaderRejected()
    {
        var pool = new RawSidPool(32768);
        int sid = PreparePatchSlot(pool);
        var shapes = ShapeTable(188);
        shapes[70].Frames[0].Width = -1;
        var capture = new RawShapeToBuffer(pool, TypeTable(188), shapes, TwoPixelDatabase(), new ShapeToBufferState());
        Assert.Throws<ArgumentOutOfRangeException>(() => capture.Capture(sid));
    }

    /// <summary>TYPE/SHP 개수 불일치는 거부한다.</summary>
    [Fact]
    public void CountMismatchRejected()
    {
        var pool = new RawSidPool(32768);
        var shortTypes = new List<OriginalTypeRecord>();
        // 판본 개수보다 짧은 표를 그대로 둔다
        for (int i = 0; i < 10; i++)
        {
            shortTypes.Add(new OriginalTypeRecord());
        }
        Assert.Throws<ArgumentException>(() =>
            new RawShapeToBuffer(pool, shortTypes, ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState()));
    }

    /// <summary>같은 월드의 풀을 돌려준다.</summary>
    [Fact]
    public void PoolReturnsSameWorld()
    {
        var pool = new RawSidPool(32768);
        var capture = new RawShapeToBuffer(pool, TypeTable(188), ShapeTable(188), TwoPixelDatabase(), new ShapeToBufferState());
        Assert.Same(pool, capture.Pool());
    }
}
