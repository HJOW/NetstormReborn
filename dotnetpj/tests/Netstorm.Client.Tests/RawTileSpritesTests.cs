using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 수집한 타일 항목의 일반 그림을 실제 SHP 픽셀로 출력한다.
// 2026-11-26 1.070에서 R5-3b용으로 추가했다.
public sealed class RawTileSpritesTests
{
    private sealed class TileScene
    {
        public TileScene()
        {
            string? data = GameDataLocator.FindDataDirectory();
            Assert.NotNull(data);
            var startup = new HostStartupScene(data);
            Database = new Netstorm.Client.Display.ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp"));
            Types = MissionFortLoad.LoadTypeTable(startup.Files).Types;
            for (uint type = 70; type < (uint)Types.Count; type++)
            {
                int block = (int)type - 70;
                if (block >= Database.Blocks().Count)
                {
                    break;
                }
                var frames = Database.Blocks()[block].Frames;
                for (int frame = 0; frame < frames.Count; frame++)
                {
                    if (frames[frame].IsSpecial())
                    {
                        continue;
                    }
                    try
                    {
                        if (Database.Decode(block, frame).Opacity.Any(pixel => pixel != 0))
                        {
                            Tile = (type, frame);
                            return;
                        }
                    }
                    catch (InvalidDataException)
                    {
                    }
                }
            }
            throw new InvalidOperationException("그릴 타일 프레임 없음");
        }

        public RawSidPool Pool { get; } = new(24000);
        public IReadOnlyList<OriginalTypeRecord> Types { get; }
        public Netstorm.Client.Display.ShapeDatabase Database { get; }
        public (uint Type, int Frame) Tile { get; }
    }

    private static Span<byte> Raw(RawSidPool pool, int sid) =>
        MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(pool.Slot(sid)), pool.SlotStride);

    private static RenderCollectionEntry Place(TileScene scene, float wx, float wy, byte tail = 0)
    {
        const int sid = 100;
        Span<byte> raw = Raw(scene.Pool, sid);
        raw.Clear();
        raw[10] = (byte)scene.Tile.Type;
        BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], unchecked((uint)scene.Tile.Frame));
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], wx);
        BinaryPrimitives.WriteSingleLittleEndian(raw[18..], wy);
        BinaryPrimitives.WriteInt16LittleEndian(raw[22..], unchecked((short)(SpriteDraw.Truncate((double)wx * 16 + 0.5))));
        BinaryPrimitives.WriteInt16LittleEndian(raw[24..], unchecked((short)(SpriteDraw.Truncate((double)wy * 11 + 0.5))));
        raw[40] = 0;
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], 0);
        return new(wx, wy, unchecked((uint)sid * (uint)RawSidPool.Stride), 0, 1, tail);
    }

    /// <summary>일반 타일을 카메라 상대 위치에 칠한다.</summary>
    [Fact]
    public void PlainTilePaintsPixels()
    {
        var scene = new TileScene();
        var pixels = new byte[1024 * 768];
        RenderCollectionEntry entry = Place(scene, 20, 20);
        RawTileSprites.DrawTile(entry, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, new(0, 0, 1024, 768), PlainModes(scene));
        int block = (int)scene.Tile.Type - 70;
        var image = scene.Database.Decode(block, scene.Tile.Frame);
        int expected = image.Opacity.Count(pixel => pixel != 0);
        Assert.True(expected > 0);
        var frame = scene.Database.Blocks()[block].Frames[scene.Tile.Frame];
        var record = scene.Types[(int)scene.Tile.Type];
        int left = 320 - record.HotspotX + frame.Rect.Left;
        int top = 220 - record.HotspotY + frame.Rect.Top;
        for (int row = 0; row < image.Height; row++)
        {
            for (int column = 0; column < image.Width; column++)
            {
                int at = row * image.Width + column;
                byte want = image.Opacity[at] != 0 ? image.Indices[at] : (byte)0;
                Assert.Equal(want, pixels[(top + row) * 1024 + left + column]);
            }
        }
    }

    private static RawTileDrawModes PlainModes(TileScene scene) =>
        new() { OwnerMaps = new byte[scene.Types.Count] };

    /// <summary>클립 밖은 건드리지 않는다.</summary>
    [Fact]
    public void ClipRestrictsPaint()
    {
        var scene = new TileScene();
        var pixels = new byte[1024 * 768];
        RenderCollectionEntry entry = Place(scene, 20, 20);
        RawTileSprites.DrawTile(entry, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, new(0, 0, 8, 8), PlainModes(scene));
        Assert.All(pixels, pixel => Assert.Equal(0, pixel));
    }

    /// <summary>일반 외 분기와 잘못된 토큰·프레임을 거부한다.</summary>
    [Fact]
    public void NonPlainAndBadEntriesThrow()
    {
        var scene = new TileScene();
        var pixels = new byte[1024 * 768];
        var clip = new ScreenRect(0, 0, 1024, 768);
        RenderCollectionEntry shadow = Place(scene, 20, 20, tail: 4);
        var missing = Assert.Throws<InvalidOperationException>(() =>
            RawTileSprites.DrawTile(shadow, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, clip, new RawTileDrawModes()));
        Assert.Contains("색표 미연결", missing.Message);
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            RawTileSprites.DrawTile(new(0, 0, 3, 0, 1, 0), scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, clip, new RawTileDrawModes()));
        var badFrame = Place(scene, 20, 20);
        Span<byte> raw = Raw(scene.Pool, 100);
        BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], 0xFFFFFFU);
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            RawTileSprites.DrawTile(badFrame, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, clip, new RawTileDrawModes()));
        var virtualEntry = Place(scene, 20, 20);
        Raw(scene.Pool, 100)[40] = 2;
        var virtualMissing = Assert.Throws<InvalidOperationException>(() =>
            RawTileSprites.DrawTile(virtualEntry, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, clip, new RawTileDrawModes()));
        Assert.Contains("R5-3c2", virtualMissing.Message);
    }

    private static byte[] ConstantTable(byte value)
    {
        var table = new byte[256];
        Array.Fill(table, value);
        return table;
    }

    /// <summary>그림자는 대상 화면 색을 표에서 변환한다.</summary>
    [Fact]
    public void ShadowTransformsDestination()
    {
        var scene = new TileScene();
        var pixels = new byte[1024 * 768];
        Array.Fill(pixels, (byte)7);
        RenderCollectionEntry entry = Place(scene, 20, 20, tail: 4);
        var modes = new RawTileDrawModes { ShadowTable = ConstantTable(99) };
        RawTileSprites.DrawTile(entry, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, new(0, 0, 1024, 768), modes);
        Assert.Contains(pixels, pixel => pixel == 99);
    }

    /// <summary>죽음·추상은 각 색표로 칠하고 로컬이 아니면 추상을 건너뛴다.</summary>
    [Fact]
    public void DeadAndAbstractUseTables()
    {
        var scene = new TileScene();
        var clip = new ScreenRect(0, 0, 1024, 768);
        var deadPixels = new byte[1024 * 768];
        RenderCollectionEntry dead = Place(scene, 20, 20);
        Raw(scene.Pool, 100)[40] = 1;
        var deadModes = new RawTileDrawModes { DeadTable = ConstantTable(77) };
        RawTileSprites.DrawTile(dead, scene.Pool, scene.Types, scene.Database, 0, 0, deadPixels, 1024, 768, clip, deadModes);
        Assert.Contains(deadPixels, pixel => pixel == 77);
        var abstractPixels = new byte[1024 * 768];
        RenderCollectionEntry proximity = Place(scene, 20, 20);
        Raw(scene.Pool, 100)[40] = 0x10;
        Raw(scene.Pool, 100)[34] = 5;
        var abstractModes = new RawTileDrawModes { LocalPlayer = 5, AbstractTable = ConstantTable(66) };
        RawTileSprites.DrawTile(proximity, scene.Pool, scene.Types, scene.Database, 0, 0, abstractPixels, 1024, 768, clip, abstractModes);
        Assert.Contains(abstractPixels, pixel => pixel == 66);
        var foreignPixels = new byte[1024 * 768];
        Raw(scene.Pool, 100)[34] = 6;
        RawTileSprites.DrawTile(proximity, scene.Pool, scene.Types, scene.Database, 0, 0, foreignPixels, 1024, 768, clip, abstractModes);
        Assert.All(foreignPixels, pixel => Assert.Equal(0, pixel));
    }

    /// <summary>소유자 표가 있고 소유자가 있으면 소유자 색표로 칠한다.</summary>
    [Fact]
    public void OwnerUsesOwnerTable()
    {
        var scene = new TileScene();
        var pixels = new byte[1024 * 768];
        RenderCollectionEntry entry = Place(scene, 20, 20);
        Raw(scene.Pool, 100)[34] = 3;
        var ownerMaps = new byte[scene.Types.Count];
        ownerMaps[(int)scene.Tile.Type] = 1;
        var modes = new RawTileDrawModes { OwnerMaps = ownerMaps, OwnerColors = _ => ConstantTable(55) };
        RawTileSprites.DrawTile(entry, scene.Pool, scene.Types, scene.Database, 0, 0, pixels, 1024, 768, new(0, 0, 1024, 768), modes);
        Assert.Contains(pixels, pixel => pixel == 55);
        // 특수 모드·거름이면 추상으로 간다
        var specialPixels = new byte[1024 * 768];
        var specialModes = new RawTileDrawModes
        {
            SpecialMode = 1,
            WideOwnerType = scene.Tile.Type,
            AbstractTable = ConstantTable(66),
        };
        Raw(scene.Pool, 100)[34] = 0;
        RawTileSprites.DrawTile(entry, scene.Pool, scene.Types, scene.Database, 0, 0, specialPixels, 1024, 768, new(0, 0, 1024, 768), specialModes);
        Assert.Contains(specialPixels, pixel => pixel == 66);
    }
}
