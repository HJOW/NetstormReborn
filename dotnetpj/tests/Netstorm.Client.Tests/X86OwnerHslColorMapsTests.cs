using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(owner-hsl-colors-x86.tsv: 15행)로 전용 소유자 색표 L1 일치를 검사한다.
/// 논리 팔레트 adapter와 실제 primary 두 경로가 같은 기대값과 일치해야 한다.
/// 근거: cpppj/tests/PreviewColorMapsTests.cpp. 기대값은 C# 결과로 바꾸지 않는다. (R2c, 2026-10-28)
/// </summary>
public sealed class X86OwnerHslColorMapsTests
{
    /// <summary>fixture 데이터 행 수.</summary>
    private const int Rows = 15;

    /// <summary>실제 CRT가 선택한 53비트 기본 출력에 다섯 전용 표·모든 소유자·모든 소스 번호를 대조한다.</summary>
    [Fact]
    public void SpecialOwnerMatchesActualDefaultPrecision()
    {
        string[][] rows = X86Fixture.Read("owner-hsl-colors");
        Assert.Equal(Rows, rows.Length);
        int different = 0;
        // 64비트 별도 관측은 보존하되 기본 출력의 기대값으로 혼합하지 않는다
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length);
            byte[] raw = ColorMapFixture.Bytes(row[2]);
            uint[] logical = ColorMapFixture.Dwords(raw);
            uint type = X86Fixture.UInt(row[1]);
            OwnerColorMaps maps = OwnerColorMapBuilder.BuildSpecialOwnerColorMaps(logical, type);
            byte[] expected = ColorMapFixture.Bytes(row[3]);
            var allocation = new byte[RawColorSortPrimary.TotalBytes];
            Array.Fill(allocation, (byte)0xa7);
            var primary = new RawColorSortPrimary(allocation);
            primary.Construct(raw);
            OwnerColorMaps storedMaps = OwnerColorMapBuilder.BuildSpecialOwnerColorMaps(primary, type);
            // 중립 0·소유자 1~8의 전체 2304바이트를 한 번씩 검사한다
            for (int owner = 0; owner < 9; owner++)
            {
                Assert.Equal(expected.AsSpan(owner * 256, 256).ToArray(), maps.Tables[owner]);
                Assert.Equal(expected.AsSpan(owner * 256, 256).ToArray(), storedMaps.Tables[owner]);
            }
            if (row[3] != row[4])
            {
                different++;
            }
        }
        Assert.True(different > 0);
    }
}
