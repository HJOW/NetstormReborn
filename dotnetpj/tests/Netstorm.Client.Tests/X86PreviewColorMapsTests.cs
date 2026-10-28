using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(preview-colors-x86.tsv: 3행)로 커서 파괴·불법·글자 그림자 표 L1 일치를 검사한다.
/// 근거: cpppj/tests/PreviewColorMapsTests.cpp. 기대값은 C# 결과로 바꾸지 않는다. (R2c, 2026-10-28)
/// </summary>
public sealed class X86PreviewColorMapsTests
{
    /// <summary>fixture 행 수(파일·혼합 RGB·중복 회색).</summary>
    private const int Rows = 3;

    /// <summary>실제 파일/혼합 RGB/중복 회색의 모든 출력 번호를 원본 fixture 그대로 비교한다.</summary>
    [Fact]
    public void ActualWholeNativeOutputsMatch()
    {
        string[][] rows = X86Fixture.Read("preview-colors");
        Assert.Equal(Rows, rows.Length);
        // 세 입력을 각각 독립 논리 팔레트로 재생한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            uint[] logical = ColorMapFixture.Dwords(ColorMapFixture.Bytes(row[2]));
            PreviewColorMaps maps = PreviewColorMapBuilder.BuildPreviewColorMaps(logical, X86Fixture.UInt(row[1]));
            Assert.Equal(ColorMapFixture.Bytes(row[3]), maps.Destroy);
            Assert.Equal(ColorMapFixture.Bytes(row[4]), maps.Illegal);
            Assert.Equal(ColorMapFixture.Bytes(row[5]), maps.LabelShadow);
        }
    }
}
