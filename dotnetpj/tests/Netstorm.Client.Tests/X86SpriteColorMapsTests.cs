using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(sprite-colors-x86.tsv: 3행)로 일반 자산의 밝음·건설·그림자 표 L1 일치를 검사한다.
/// 근거: cpppj/tests/SpriteColorMapsTests.cpp. 기대값은 C# 결과로 바꾸지 않는다. (R2c, 2026-10-28)
/// </summary>
public sealed class X86SpriteColorMapsTests
{
    /// <summary>fixture 행 수(실제 GIFCLOUD·혼합 RGB·중복 회색).</summary>
    private const int Rows = 3;

    /// <summary>실제 GIFCLOUD/혼합 RGB/중복 회색에서 밝음·건설·그림자 모두 원본과 같다.</summary>
    [Fact]
    public void WholeNativeOutputsMatch()
    {
        string[][] rows = X86Fixture.Read("sprite-colors");
        Assert.Equal(Rows, rows.Length);
        // 세 입력의 색 256개 × 세 표를 동결한 원문 그대로 대조한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            uint[] logical = ColorMapFixture.Dwords(ColorMapFixture.Bytes(row[2]));
            SpriteColorMaps maps = SpriteColorMapBuilder.BuildSpriteColorMaps(logical);
            Assert.Equal(ColorMapFixture.Bytes(row[3]), maps.Bright);
            Assert.Equal(ColorMapFixture.Bytes(row[4]), maps.Dead);
            Assert.Equal(ColorMapFixture.Bytes(row[5]), maps.Shadow);
        }
    }
}
