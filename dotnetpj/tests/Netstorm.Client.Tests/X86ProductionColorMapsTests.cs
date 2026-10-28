using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(production-colors-x86.tsv: 3행)로 생산 아이콘 비활성·선행 표 L1 일치를 검사한다.
/// 근거: cpppj/tests/ProductionColorMapsTests.cpp. 기대값은 C# 결과로 바꾸지 않는다. (R2c, 2026-10-28)
/// </summary>
public sealed class X86ProductionColorMapsTests
{
    /// <summary>fixture 행 수(세 원본 팔레트).</summary>
    private const int Rows = 3;

    /// <summary>세 원본 팔레트의 두 출력 표를 재계산한 기대값 없이 읽는다.</summary>
    [Fact]
    public void NativeAllEntriesMatch()
    {
        string[][] rows = X86Fixture.Read("production-colors");
        Assert.Equal(Rows, rows.Length);
        // 실제 관측의 모든 입력/출력을 순서대로 읽는다
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length);
            uint[] logical = ColorMapFixture.Dwords(ColorMapFixture.Bytes(row[2]));
            ProductionColorMaps maps = ProductionColorMapBuilder.BuildProductionColorMaps(logical);
            Assert.Equal(ColorMapFixture.Bytes(row[3]), maps.Disabled);
            Assert.Equal(ColorMapFixture.Bytes(row[4]), maps.Underlay);
        }
    }
}
