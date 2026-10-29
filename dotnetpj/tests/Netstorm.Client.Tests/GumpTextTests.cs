using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 측정 폭/높이의 alias와 판본별 문맥 선택을 검사한다.

/// <summary>cpppj/tests/GumpTextTests.cpp의 기본 측정/메트릭 계약이다. 돌 버튼 조립 검사는 버튼 이식 단위에서 잇는다.</summary>
public sealed class GumpTextTests
{
    /// <summary>판본별305행의 정수 감김·명시NUL·불법 스타일·출력 alias를 전수 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void NativeMeasurements(string edition)
    {
        var fixture = new GumpFontFixture("gumptext"); string[][] rows = fixture.Cases.Where(r => r[1] == edition).ToArray(); Assert.Equal(305, rows.Length); var text = new GumpText(fixture.Fonts, edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        // 폭 표와 PE 출력은 독립 저장 입력 그대로 읽는다.
        foreach (string[] r in rows)
        {
            Assert.Equal(13, r.Length); var context = new GumpTextContext(X86Fixture.UInt(r[5]), X86Fixture.UInt(r[3]), 0, X86Fixture.UInt(r[4]), 0, 0, X86Fixture.Int(r[6]), X86Fixture.Int(r[7]));
            byte[] bytes = X86Fixture.Bytes(r[10]); int width = 17, height = 19; bool alias = X86Fixture.UInt(r[9]) != 0; int length = X86Fixture.Int(r[8]);
            if (alias) text.Measure(bytes, context, ref width, ref width, length); else text.Measure(bytes, context, ref width, ref height, length);
            Assert.Equal(X86Fixture.Int(r[11]), width); Assert.Equal(X86Fixture.Int(r[12]), alias ? width : height);
        }
        Assert.Equal(31, fixture.Fonts.Get().Height()); Assert.Equal(14, FontStore.LogicalHeight(0));
    }

    /// <summary>잘못된 문자열/슬롯은 출력 일부를 쓰지 않고 거부한다. -2 측정과 명시NUL은 원본처럼 허용한다.</summary>
    [Fact]
    public void BoundsSignedLengthAndPreservedOutputs()
    {
        int[] widths = new int[256]; widths[0] = 23; byte[] bytes = [65, 66]; var context = new GumpTextContext(2, 0, 0, 0, 0, 0, -3, 5); int width = 17, height = 19;
        Assert.Throws<ArgumentException>(() => GumpText.MeasureGumpText(bytes, widths, 14, context, ref width, ref height)); Assert.Equal((17, 19), (width, height));
        Assert.Throws<ArgumentException>(() => GumpText.MeasureGumpText(bytes, widths, 14, context, ref width, ref height, 3)); Assert.Equal((17, 19), (width, height));
        GumpText.MeasureGumpText([], widths, 14, context, ref width, ref height, -2); Assert.Equal((-3, 19), (width, height)); bytes[0] = 0; GumpText.MeasureGumpText(bytes, widths, 14, context, ref width, ref height, 1); Assert.Equal((20, 19), (width, height));
        var fixture = new GumpFontFixture("gumptext"); var text = new GumpText(fixture.Fonts); context.Font = uint.MaxValue; width = 17;
        Assert.Throws<ArgumentException>(() => text.Measure([], context, ref width, ref height, 0)); Assert.Equal((17, 19), (width, height));
    }

    /// <summary>캐시 높이와 논리 슬롯 높이를 구별하고 불법 스타일/빈 슬롯/CD bold의 실제 메트릭을 확인한다.</summary>
    [Fact]
    public void CacheMetricsAndEditionSelection()
    {
        var fixture = new GumpFontFixture("gumpglyph"); var patch = new GumpText(fixture.Fonts); var cd = new GumpText(fixture.Fonts, OriginalEdition.Cd1072);
        Assert.Equal((31, 26, 5), patch.Metrics(new())); Assert.Equal((0, 0, 0), patch.Metrics(new() { Font = 2 })); Assert.Equal((0, 0, 0), patch.Metrics(new() { Font = 3, Style = 1 }));
        Assert.Equal((31, 26, 5), patch.Metrics(new() { Font = uint.MaxValue, Style = uint.MaxValue })); Assert.Equal((0, 0, 0), cd.Metrics(new() { Font = 6, Style = 2 })); Assert.Equal((31, 26, 5), patch.Metrics(new() { Font = 6, Style = 2 }));
        Assert.Throws<ArgumentException>(() => patch.Metrics(new() { Font = uint.MaxValue })); uint[] words = [2, 3, 4, 1, 8, 9, uint.MaxValue, 0x80000000]; Assert.Equal(new GumpTextContext(2, 3, 4, 1, 8, 9, -1, int.MinValue), GumpText.DecodeGumpTextContext(words));
    }
}
