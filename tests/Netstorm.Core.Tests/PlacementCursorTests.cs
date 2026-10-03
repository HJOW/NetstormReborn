using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>
/// 커서가 가리키는 칸과 들고 있는 유닛의 발자국 기준 칸 규칙을 검사한다.
/// 기대값은 2026-10-03 TEST01 녹화의 커서 좌표와 흰 발자국 사각형을 맞춰 본 결과다.
/// </summary>
public sealed class PlacementCursorTests
{
    /// <summary>height 속성이 없는 3×3 유닛 (Crossbow·Ice Cannon 과 같은 조건).</summary>
    private static readonly TypeDefinition Flat = TypeDefinition.Parse("typename flat\n{\n\tfoot_x = 3;\n\tfoot_y = 3;\n}\nA00 : default : \"a\" #0 ;\n");

    /// <summary>height = 2 인 3×3 유닛 (Thunder Cannon 과 같은 조건).</summary>
    private static readonly TypeDefinition Tall = TypeDefinition.Parse("typename tall\n{\n\tfoot_x = 3;\n\tfoot_y = 3;\n\theight = 2;\n}\nA00 : default : \"a\" #0 ;\n");

    /// <summary>칸 (cx, cy) 는 월드 픽셀 (16(cx−1), 16cx] × (11(cy−1), 11cy] 에 보인다.</summary>
    [Theory]
    [InlineData(1.0, 1.0, 1, 1)]
    [InlineData(16.0, 11.0, 1, 1)]
    [InlineData(16.5, 11.5, 2, 2)]
    [InlineData(2735.9, 2221.9, 171, 202)]
    [InlineData(2736.0, 2222.0, 171, 202)]
    [InlineData(2736.1, 2222.1, 172, 203)]
    public void CellUnder_UsesVisibleCell(double worldX, double worldY, int expectedX, int expectedY)
    {
        Assert.Equal((expectedX, expectedY), PlacementCursor.CellUnder(worldX, worldY));
    }

    /// <summary>height 가 없으면 기준 칸은 커서 칸의 바로 아래 행이다 — 커서가 3×3 발자국의 오른쪽 열·가운데 행에 놓인다.</summary>
    [Fact]
    public void AnchorCell_WithoutHeight_IsOneRowBelowCursor()
    {
        (int x, int y) = PlacementCursor.AnchorCell(100 * 16 - 4, 50 * 11 - 3, Flat);
        Assert.Equal((100, 51), (x, y));
        Footprint footprint = Footprint.ForType(Flat, x, y);
        // 커서 칸 (100, 50) 은 발자국의 오른쪽 끝 열, 가운데 행이다
        Assert.Equal(100, footprint.AnchorX);
        Assert.Equal(50, footprint.Top + 1);
    }

    /// <summary>height = 2 이면 세 행 아래다 — 커서가 발자국 위쪽 한 칸 밖(오른쪽 열)에 있다.</summary>
    [Fact]
    public void AnchorCell_WithHeight_MovesFootprintDown()
    {
        (int x, int y) = PlacementCursor.AnchorCell(100 * 16 - 4, 50 * 11 - 3, Tall);
        Assert.Equal((100, 53), (x, y));
        Assert.Equal(51, Footprint.ForType(Tall, x, y).Top);
    }

    /// <summary>원본 타입의 height: 사제·골렘 1, Thunder Cannon 2, Ice Cannon 없음.</summary>
    [Theory]
    [InlineData("priest", 2)]
    [InlineData("sunWalker", 2)]
    [InlineData("thunderCannon", 3)]
    [InlineData("rainCannon", 1)]
    [InlineData("windArcher", 1)]
    public void AnchorCell_OriginalTypes(string typeName, int expectedRows)
    {
        TypeDefinition type = OriginalData.RequireTypes().Find(typeName)!.Definition;
        (int x, int y) = PlacementCursor.AnchorCell(160, 110, type);
        Assert.Equal((10, 10 + expectedRows), (x, y));
    }
}
