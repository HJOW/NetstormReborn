using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

// 원문 callback 자료의 두 필드만 독립 입력 AST에서 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>두 PE의 방향/물리 프레임44관측과 실제 숫자 AST/범위 검사다.</summary>
public sealed class RawArcherTypeDataTests
{
    /// <summary>기대값에서 방향/클러스터 두 필드만 읽으며 나머지 SHP/후처리 범위와 구별한다.</summary>
    [Theory]
    [InlineData("originals")] [InlineData("originalCD")]
    public void MatchesAllNativeDirectionAndClusterFields(string edition)
    {
        int count = 0;
        // 결과5~7열은 정의의 입력으로 쓰지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/archertype-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue; string[] row = line.Split('\t'); if (row[0] != edition) continue; Assert.Equal(8, row.Length); RiftTypeDefinition definition = row[1] == "prefix" ? new() : RiftTypeDefinition.Parse(Encoding.ASCII.GetString(Convert.FromHexString(row[4]))); ArcherTypeData actual = RawArcherTypeData.Read(definition); string[] expected = row[5].Split(','); Assert.Equal(int.Parse(expected[0], CultureInfo.InvariantCulture), actual.Directions); Assert.Equal(int.Parse(expected[1], CultureInfo.InvariantCulture), actual.FrameCount); count++;
        }
        Assert.Equal(22, count);
    }
    /// <summary>현재 정의 변경과 float 먼저 저장하는 절삭, 누락/문자열/비유한/정수 범위를 구별한다.</summary>
    [Fact]
    public void UsesCurrentDefinitionAndRejectsInvalidNumericAst()
    {
        var definition = new RiftTypeDefinition(); Assert.Equal(0, RawArcherTypeData.Read(definition).Directions); definition.Properties.Add(new("orientations", 15.9999999)); definition.Clusters.Add(new()); ArcherTypeData value = RawArcherTypeData.Read(definition); Assert.Equal(16, value.Directions); Assert.Equal(1, value.FrameCount); definition.Properties.Clear(); definition.Properties.Add(new("orientations", -3.75)); Assert.Equal(-3, RawArcherTypeData.Read(definition).Directions); definition.Properties.Clear(); definition.Properties.Add(new("orientations", "16")); Assert.Throws<ArgumentException>(() => RawArcherTypeData.Read(definition));
        // 원문 정상 float CRT 변환 범위 밖만 거부한다.
        foreach (double number in new[] { double.NaN, double.PositiveInfinity, 2147483647.0, -2147484000.0 }) { definition.Properties.Clear(); definition.Properties.Add(new("orientations", number)); Assert.Throws<ArgumentOutOfRangeException>(() => RawArcherTypeData.Read(definition)); }
    }
}
