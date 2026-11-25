using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
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

    private static (Netstorm.Client.Display.ShapeDatabase Database, List<SquidDisplayShape> Shapes, IReadOnlyList<OriginalTypeRecord> Types, (uint Type, int Frame) Tile) RealTileAssets()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        byte[] shp = startup.Files.ReadAllBytes("d/_shapes.shp");
        var database = new Netstorm.Client.Display.ShapeDatabase(shp);
        IReadOnlyList<OriginalTypeRecord> types = MissionFortLoad.LoadTypeTable(startup.Files).Types;
        var shapes = TerrainDisplayShapes.Build(MissionFortLoad.LoadTypeDefinitions(startup.Files), new Netstorm.Assets.ShapeDatabase(shp));
        for (uint type = 70; type < (uint)types.Count; type++)
        {
            int block = (int)type - 70;
            if (block >= database.Blocks().Count)
            {
                break;
            }
            OriginalTypeRecord record = types[(int)type];
            if (record.FrameCount <= 0
                || record.MaxHitPoints == unchecked((int)0xDDDDDDDD)
                || record.MaxHitPoints == unchecked((int)0xCDCDCDCD))
            {
                continue;
            }
            var frames = database.Blocks()[block].Frames;
            for (int frame = 0; frame < frames.Count && frame < record.FrameCount; frame++)
            {
                if (frames[frame].IsSpecial())
                {
                    continue;
                }
                try
                {
                    if (database.Decode(block, frame).Opacity.Any(pixel => pixel != 0))
                    {
                        return (database, shapes, types, (type, frame));
                    }
                }
                catch (InvalidDataException)
                {
                }
            }
        }
        throw new InvalidOperationException("그릴 타일 프레임 없음");
    }

    /// <summary>미조립 제출은 실제 장치 요구로 거부된다.</summary>
    [Fact]
    public void UnassembledSubmitIsExplicit()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var (database, _, _, _) = RealTileAssets();
        var missing = Assert.Throws<InvalidOperationException>(() => session.SubmitWorldDrawing(new byte[64], 8, 8, new(0, 0, 8, 8), database, new RawTileDrawModes()));
        Assert.Contains("월드 수집 장치", missing.Message);
    }

    /// <summary>수집한 타일을 정렬 뒤 SHP 픽셀로 제출한다.</summary>
    [Fact]
    public void SubmitPaintsCollectedTiles()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var (database, shapes, types, tile) = RealTileAssets();
        var hash = new RawSquidHash();
        var pool = new RawSidPool(24000);
        hash.Select(0);
        hash.At(3, 4) = 7;
        Span<byte> raw = Raw(pool, 7);
        raw.Clear();
        raw[10] = (byte)tile.Type;
        BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], unchecked((uint)tile.Frame));
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 3.5f);
        BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 4.5f);
        BinaryPrimitives.WriteInt16LittleEndian(raw[22..], 56);
        BinaryPrimitives.WriteInt16LittleEndian(raw[24..], 50);
        raw[35] = 1;
        raw[40] = 0;
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 0);
        session.BuildWorldDrawingDevices(hash, pool, types, shapes, () => new ScreenRect(0, 0, 1024, 768), 1024, 768);
        var pixels = new byte[1024 * 768];
        var modes = new RawTileDrawModes { OwnerMaps = new byte[types.Count] };
        var clip = new ScreenRect(0, 0, 1024, 768);
        session.SubmitWorldDrawing(pixels, 1024, 768, clip, database, modes);
        var decoded = database.Decode((int)tile.Type - 70, tile.Frame);
        int expected = decoded.Opacity.Count(pixel => pixel != 0);
        Assert.True(expected > 0);
        var shape = database.Blocks()[(int)tile.Type - 70].Frames[tile.Frame];
        var record = types[(int)tile.Type];
        int left = 56 - record.HotspotX + shape.Rect.Left;
        int top = 50 - record.HotspotY + shape.Rect.Top;
        for (int row = 0; row < decoded.Height; row++)
        {
            for (int column = 0; column < decoded.Width; column++)
            {
                int at = row * decoded.Width + column;
                byte want = decoded.Opacity[at] != 0 ? decoded.Indices[at] : (byte)0;
                Assert.Equal(want, pixels[(top + row) * 1024 + left + column]);
            }
        }
    }

    /// <summary>타일 외 종류는 뒤 조각 명시 실패로 거부된다.</summary>
    [Fact]
    public void SubmitRejectsNonTileKinds()
    {
        var (session, _) = Scene();
        session.BuildPlayerWorldHooks();
        var (database, shapes, types, tile) = RealTileAssets();
        var hash = new RawSquidHash();
        var pool = new RawSidPool(24000);
        hash.Select(1);
        hash.At(5, 10) = 9;
        Span<byte> body = Raw(pool, 9);
        body.Clear();
        body[10] = (byte)tile.Type;
        BinaryPrimitives.WriteInt32LittleEndian(body[36..], tile.Frame);
        BinaryPrimitives.WriteSingleLittleEndian(body[14..], 10.5f);
        BinaryPrimitives.WriteSingleLittleEndian(body[18..], 20.5f);
        body[35] = 2;
        BinaryPrimitives.WriteUInt16LittleEndian(body[4..], 0);
        session.BuildWorldDrawingDevices(hash, pool, types, shapes, () => new ScreenRect(0, 0, 1024, 768), 1024, 768);
        var pixels = new byte[1024 * 768];
        var modes = new RawTileDrawModes { OwnerMaps = new byte[types.Count] };
        var missing = Assert.Throws<InvalidOperationException>(() =>
            session.SubmitWorldDrawing(pixels, 1024, 768, new(0, 0, 1024, 768), database, modes));
        Assert.Contains("R5-3c3", missing.Message);
    }
}
