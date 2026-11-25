using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 같은 세션 저장소·화면 크기로 타일·객체 수집기를 묶는다.
// 2026-11-26 1.068에서 R5-2d3용으로 추가했다.
public sealed partial class HostPriestWorldLiveTests
{
    private static (RawSquidHash Hash, RawSidPool Pool, List<OriginalTypeRecord> Types, List<SquidDisplayShape> Shapes) DrawingStorage()
    {
        var hash = new RawSquidHash();
        var pool = new RawSidPool(24000);
        var types = new List<OriginalTypeRecord> { new(), new() { Flags1 = 0, Flags2 = 0, FrameCount = 1 } };
        var shapes = new List<SquidDisplayShape> { new(), new() { Loaded = true, FrameCount = 1 } };
        shapes[1].Frames.Add(new SquidDisplayFrame { CellWidth = 4, CellHeight = 4, CellHotspotX = 0, CellHotspotY = 0 });
        return (hash, pool, types, shapes);
    }

    /// <summary>읽기 전용 슬롯을 같은 실제 바이트의 쓰기 span으로 본다.</summary>
    private static Span<byte> Raw(RawSidPool pool, int sid) =>
        MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(pool.Slot(sid)), pool.SlotStride);

    /// <summary>미조립 수집은 실제 장치 요구로 거부된다.</summary>
    [Fact]
    public void UnassembledWorldDrawingIsExplicit()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var missing = Assert.Throws<InvalidOperationException>(() => session.CollectWorldDrawing(new(0, 0, 64, 64)));
        Assert.Contains("월드 수집 장치", missing.Message);
    }

    /// <summary>같은 저장소의 타일 묶음·일반 자산을 순서대로 수집한다.</summary>
    [Fact]
    public void WorldDrawingCollectsTilesThenObjects()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var (hash, pool, types, shapes) = DrawingStorage();
        hash.Select(0);
        hash.At(3, 4) = 7;
        Span<byte> tile = Raw(pool, 7);
        tile.Clear();
        BinaryPrimitives.WriteSingleLittleEndian(tile[14..], 3.5f);
        BinaryPrimitives.WriteSingleLittleEndian(tile[18..], 4.5f);
        tile[35] = 1;
        BinaryPrimitives.WriteUInt16LittleEndian(tile[4..], 0);
        hash.Select(1);
        hash.At(5, 10) = 9;
        Span<byte> body = Raw(pool, 9);
        body.Clear();
        body[10] = 1;
        BinaryPrimitives.WriteInt32LittleEndian(body[36..], 0);
        BinaryPrimitives.WriteSingleLittleEndian(body[14..], 10.5f);
        BinaryPrimitives.WriteSingleLittleEndian(body[18..], 20.5f);
        body[35] = 2;
        BinaryPrimitives.WriteUInt16LittleEndian(body[4..], 0);
        session.BuildWorldDrawingDevices(hash, pool, types, shapes, () => new ScreenRect(0, 0, 1024, 768), 1024, 768);
        RawDrawCollectionResult result = session.CollectWorldDrawing(new(0, 0, 1024, 768));
        Assert.Equal(2, result.Count);
        Assert.False(result.BackgroundCovered);
        Assert.Equal(1, result.Entries[0].Kind);
        Assert.Equal(0x80000000U, result.Entries[0].Token);
        Assert.Equal(0, result.Entries[1].Kind);
        Assert.Equal(9u * 50u, result.Entries[1].Token);
    }

    /// <summary>빈 월드는 0건을 돌리고 배경을 다시 그린다.</summary>
    [Fact]
    public void EmptyWorldCollectsNothing()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var (hash, pool, types, shapes) = DrawingStorage();
        session.BuildWorldDrawingDevices(hash, pool, types, shapes, () => new ScreenRect(0, 0, 1024, 768), 1024, 768);
        RawDrawCollectionResult result = session.CollectWorldDrawing(new(0, 0, 64, 64));
        Assert.Equal(0, result.Count);
        Assert.False(result.BackgroundCovered);
    }

    /// <summary>높이가 바뀌면 수집기를 다시 만든다.</summary>
    [Fact]
    public void HeightChangeRebuildsCollectors()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var (hash, pool, types, shapes) = DrawingStorage();
        hash.Select(0);
        hash.At(3, 4) = 7;
        Span<byte> tile = Raw(pool, 7);
        tile.Clear();
        BinaryPrimitives.WriteSingleLittleEndian(tile[14..], 3.5f);
        BinaryPrimitives.WriteSingleLittleEndian(tile[18..], 4.5f);
        tile[35] = 1;
        session.BuildWorldDrawingDevices(hash, pool, types, shapes, () => new ScreenRect(0, 0, 1024, 768), 1024, 768);
        Assert.Equal(1, session.CollectWorldDrawing(new(0, 0, 1024, 768)).Count);
        session.BuildWorldDrawingDevices(hash, pool, types, shapes, () => new ScreenRect(0, 0, 1024, 768), 1024, 600);
        Assert.Equal(1, session.CollectWorldDrawing(new(0, 0, 1024, 768)).Count);
    }
}
