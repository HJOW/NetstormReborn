using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using ShapeDatabase = Netstorm.Client.Display.ShapeDatabase;

// 독립 원본 실행의 전체 픽셀과 같은 보호 SHP를 대조한다.
namespace Netstorm.Tests;

/// <summary>Factory 가상 그림과 실제 ShapeToBuffer/VFX의200개 독립 원문 입력을 검사한다.</summary>
public sealed class FactoryCaptureNativeTests
{
    /// <summary>부호 없는 원문 입력 숫자만 해석한다. 기대 픽셀은 입력 조립에 사용하지 않는다.</summary>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>기본/추가 그림·배율·기준점의 전체200버퍼를 원본과 바이트 대조한다.</summary>
    [Fact]
    public void FactoryNative200Pixels()
    {
        GameFileSystem files = GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals"));
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(files);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(files);
        byte[] shp = files.ReadAllBytes("d/_shapes.shp");
        var shapes = TerrainDisplayShapes.Build(definitions, new Netstorm.Assets.ShapeDatabase(shp));
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        // 프레임 코드의 실제 로딩 순서를 TYPE 번호70부터 그대로 둔다.
        for (int index = 0; index < definitions.Count; index++) frames[index + 70] = definitions[index].FrameTable();
        table.Types[85].HotspotX = -2; table.Types[85].HotspotY = 3;
        var database = new ShapeDatabase(shp); int count = 0, overlays = 0;
        // 원본 관찰 행의 입력 여섯 칸만 소비하고 결과 네 칸은 실행 뒤에만 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "factory-capture-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#') || line.StartsWith("vtable\t", StringComparison.Ordinal)) continue;
            string[] row = line.Split('\t'); Assert.Equal(10, row.Length);
            var pool = new RawSidPool(32768);
            // 독립 원본 입력과 같은 client50 슬롯을 확보한다.
            for (int sid = 5; sid <= 50; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
            Span<byte> raw = pool.AllocatedBytes(50); raw.Clear();
            BinaryPrimitives.WriteUInt32LittleEndian(raw, Number(row[0])); raw[10] = 85;
            raw[11] = checked((byte)(Number(row[1]) << 5)); raw[34] = 1; raw[40] = checked((byte)Number(row[2]));
            BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(36, 4), Number(row[3]));
            var factory = new RawFactoryDraw(pool, frames);
            var state = new ShapeToBufferState { Zoom = unchecked((int)Number(row[5])), AddTypeHotspot = Number(row[4]) != 0, OwnerTypeMaps = new uint[188], MainFrame = factory.MainFrame };
            state.Additional = (sid, image, x, y) => factory.Additional(sid, x, y, (px, py, type, frame) =>
            {
                Assert.Equal(65536, state.Zoom); Assert.Equal(85u, type); overlays++;
                _ = VfxDraw.DrawShape(image.Pixels, image.Width, image.Height, database, checked((int)type) - 70, frame, px, py,
                    new(0, 0, image.Width - 1, image.Height - 1));
            });
            ConstructionVisualImage image = new RawShapeToBuffer(pool, table.Types, shapes, database, state).Capture(50);
            Assert.Equal(row[6], $"{image.Width},{image.Height},{image.HotspotX},{image.HotspotY}");
            Assert.Equal(Convert.FromHexString(row[7]), image.Pixels); Assert.Equal(Number(row[8]), (uint)image.Pixels.Length);
            Assert.Equal("1", row[9]); count++;
        }
        Assert.Equal(200, count); Assert.Equal(60, overlays);
    }
}
