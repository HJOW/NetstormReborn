using System.Buffers.Binary;

namespace Netstorm.Assets.Tests;

/// <summary>셰이프 데이터베이스·팔레트 테스트 (docs/formats/shp.md 의 확인 결과와 일치해야 한다)</summary>
public sealed class ShapeDatabaseTests
{
    /// <summary>원본 블록 수 (= 타입 로딩 목록 길이)</summary>
    private const int ExpectedBlocks = 116;

    /// <summary>원본 전체 프레임 수</summary>
    private const int ExpectedFrames = 3783;

    /// <summary>원본 특수 레코드(이미지 아님) 수</summary>
    private const int ExpectedSpecialRecords = 91;

    /// <summary>타입 로딩 목록은 116개이고 대소문자 무시로 찾을 수 있다</summary>
    [Fact]
    public void TypeLoadOrder_HasExpectedEntries()
    {
        Assert.Equal(ExpectedBlocks, TypeLoadOrder.Names.Count);
        Assert.Equal(0, TypeLoadOrder.IndexOf("DUDE"));
        Assert.Equal(58, TypeLoadOrder.IndexOf("suncannon"));
        Assert.Equal(-1, TypeLoadOrder.IndexOf("없는타입"));
    }

    /// <summary>원본 셰이프: 블록·프레임·특수 레코드 수</summary>
    [Fact]
    public void OriginalShapes_Counts()
    {
        ShapeDatabase shapes = ShapeDatabase.Load(OriginalData.RequireFile("d/_shapes.shp"));
        Assert.Equal(ExpectedBlocks, shapes.Blocks.Count);
        Assert.Equal(ExpectedFrames, shapes.Blocks.Sum(b => b.Frames.Count));
        Assert.Equal(ExpectedSpecialRecords, shapes.Blocks.Sum(b => b.Frames.Count(f => f.IsSpecial)));
        // sunCannon: 클러스터 29개 × 레이어 2 (본체 + 그림자)
        Assert.Equal(58, shapes.FindBlock("sunCannon")!.Frames.Count);
    }

    /// <summary>패치판의 3,783개 추가 헤더를 전수 판독하고 VFX 내부 치수와의 실제 차이를 회귀로 고정한다.</summary>
    [Fact]
    public void OriginalShapes_SquidMetricsMatchStoredHeaders()
    {
        byte[] raw = File.ReadAllBytes(OriginalData.RequireFile("d/_shapes.shp"));
        var shapes = new ShapeDatabase(raw);
        int total = 0;
        var differences = new int[4];
        // 모든 블록과 공유 프레임의 헤더를 원본 파일 오프셋에서 직접 대조한다.
        foreach (ShapeBlock block in shapes.Blocks)
        {
            // 프레임 번호를 유지하여 중복 참조도 별도로 검사한다.
            for (int i = 0; i < block.Frames.Count; i++)
            {
                ShapeFrame frame = block.Frames[i];
                SquidFrameMetrics metrics = shapes.SquidMetrics(block.Index, i);
                ReadOnlySpan<byte> prefix = raw.AsSpan(frame.Offset - 36, 36);
                Assert.Equal(BinaryPrimitives.ReadInt16LittleEndian(prefix[24..]), metrics.DisplayWidth);
                Assert.Equal(BinaryPrimitives.ReadInt16LittleEndian(prefix[26..]), metrics.DisplayHeight);
                Assert.Equal(BinaryPrimitives.ReadInt16LittleEndian(prefix[28..]), metrics.HotspotX);
                Assert.Equal(BinaryPrimitives.ReadInt16LittleEndian(prefix[30..]), metrics.HotspotY);
                Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(prefix), BitConverter.SingleToUInt32Bits(metrics.CellWidth));
                Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(prefix[4..]), BitConverter.SingleToUInt32Bits(metrics.CellHeight));
                if (frame.Bounds0 != metrics.DisplayWidth) differences[0]++;
                if (frame.Bounds1 != metrics.DisplayHeight) differences[1]++;
                if (frame.Origin0 != metrics.HotspotX) differences[2]++;
                if (frame.Origin1 != metrics.HotspotY) differences[3]++;
                total++;
            }
        }
        Assert.Equal(ExpectedFrames, total);
        // 독립 Python 정적 판독 결과. VFX 내부 값으로 실제 표시 경계를 대신할 수 없음을 고정한다.
        Assert.Equal([3711, 3697, 3388, 3434], differences);
    }

    /// <summary>追加 헤더 없는 순수 VFX는 기존 이미지 해독이 가능하고 Squid 헤더 조회는 거부한다.</summary>
    [Fact]
    public void PureVfx_HasNoSquidMetrics()
    {
        byte[] bytes = new byte[42];
        "1.10"u8.CopyTo(bytes);
        BinaryPrimitives.WriteInt32LittleEndian(bytes.AsSpan(4), 1);
        BinaryPrimitives.WriteInt32LittleEndian(bytes.AsSpan(8), 16);
        // 1×1 프레임의 포함 경계는 모두 0이며 반복 토큰 2가 색 번호 7을 한 번 쓴다.
        Array.Resize(ref bytes, 43);
        bytes[40] = 2;
        bytes[41] = 7;
        var shapes = new ShapeDatabase(bytes);
        Assert.Equal((byte)7, shapes.Decode(shapes.Blocks[0].Frames[0]).Indices[0]);
        Assert.Throws<InvalidDataException>(() => shapes.SquidMetrics(0, 0));
    }

    /// <summary>원본 셰이프: 이미지 프레임 전부 예외 없이 디코딩되고, 불투명 픽셀이 하나 이상 있다</summary>
    [Fact]
    public void OriginalShapes_AllImageFramesDecode()
    {
        ShapeDatabase shapes = ShapeDatabase.Load(OriginalData.RequireFile("d/_shapes.shp"));
        int decoded = 0;
        // 모든 블록의 모든 이미지 프레임을 디코딩
        foreach (ShapeBlock block in shapes.Blocks)
        {
            // 블록 안의 프레임을 차례로 검사
            foreach (ShapeFrame frame in block.Frames.Where(f => !f.IsSpecial))
            {
                IndexedImage image = shapes.Decode(frame);
                Assert.Equal(frame.Width * frame.Height, image.Indices.Length);
                Assert.True(Array.IndexOf(image.Opaque, true) >= 0, $"{block.TypeName} 블록 {block.Index} 프레임 오프셋 {frame.Offset}: 불투명 픽셀 없음");
                decoded++;
            }
        }
        Assert.Equal(ExpectedFrames - ExpectedSpecialRecords, decoded);
    }

    /// <summary>원본 게임 팔레트: 256색, 인덱스 0xFE 는 빨강 (분석 시 확인한 값)</summary>
    [Fact]
    public void OriginalGamePalette_Loads()
    {
        Palette palette = Palette.Load(OriginalData.RequireFile("d/" + Palette.GameCol));
        Assert.Equal(new Rgb(255, 0, 0), palette[0xFE]);
    }

    /// <summary>지원하지 않는 크기의 팔레트 파일은 거부한다</summary>
    [Fact]
    public void Palette_RejectsUnknownSize()
    {
        Assert.Throws<InvalidDataException>(() => Palette.Parse(new byte[100]));
    }
}
