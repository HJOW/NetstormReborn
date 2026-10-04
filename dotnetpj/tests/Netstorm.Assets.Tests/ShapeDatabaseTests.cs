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
