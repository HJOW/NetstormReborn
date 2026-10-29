using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 두 실제 PE의 HTML 출력/현재 상태/Config 호출 순서를 대조한다.

/// <summary>원본 기대값은 비교에만 쓰고 입력/현재 설정 계획을 독립 실행한다.</summary>
public sealed class HtmlTextParserNativeTests
{
    /// <summary>현재 입력/가림/태그/평문/제목/P전역 전체를 같은 순서로 관측한다.</summary>
    private static uint[] Fields(HtmlTextParserState state, HtmlTextParserGlobals globals) => [unchecked((uint)state.InputOffset), state.Shown, state.HiddenDepth, state.InTag, state.TextAllowed, state.AfterHeading, globals.SuppressParagraph];
    /// <summary>Latin1 원시 BYTE 문자열을 원본 trace와 같은16진수로 기록한다.</summary>
    private static string Hex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();
    /// <summary>기본816행/그림276행의 출력 바이트와 전체 현재 상태를 대조한다.</summary>
    [Theory]
    [InlineData("htmltext", 816)]
    [InlineData("htmlpicture", 276)]
    public void AllBaseAndPictureNativeBytesAndStateMatch(string fixture, int expectedCount)
    {
        int count = 0;
        // 앞6열만 초기 입력으로 쓰고 뒤8열의 원문 기대값은 비교에만 사용한다.
        foreach (string[] row in X86Fixture.Read(fixture))
        {
            Assert.Equal(14, row.Length); var state = new HtmlTextParserState { Shown = X86Fixture.UInt(row[3]), HiddenDepth = X86Fixture.UInt(row[4]) }; var globals = new HtmlTextParserGlobals { SuppressParagraph = X86Fixture.UInt(row[5]) }; var hooks = new HtmlTextParserHooks { Link = (_, _, _) => "" }; if (fixture == "htmlpicture") HtmlTextParser.BindHtmlPictureText(hooks); var edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; var parser = new HtmlTextParser(state, globals, hooks, edition); Assert.Equal(X86Fixture.Text(row[6]), parser.Parse(X86Fixture.Text(row[1]), X86Fixture.Int(row[2]))); Assert.Equal(row.Skip(7).Select(X86Fixture.UInt), Fields(state, globals)); count++;
        }
        Assert.Equal(expectedCount, count);
    }
    /// <summary>배경648행의 짧은 덮어쓰기/숨긴 태그·현재80BYTE 전체를 대조한다.</summary>
    [Fact]
    public void All648NativeBackgroundBytesAndParserStateMatch()
    {
        int count = 0;
        // 실제 배경80BYTE seed와 앞5열의 초기 입력만 장치에 공급한다.
        foreach (string[] row in X86Fixture.Read("html-background"))
        {
            Assert.Equal(14, row.Length); var globals = new HtmlTextParserGlobals { SuppressParagraph = X86Fixture.UInt(row[4]) }; byte[] seed = X86Fixture.Bytes(row[0]); Assert.Equal(80, seed.Length); seed.CopyTo(globals.Background, 0); var state = new HtmlTextParserState { Shown = X86Fixture.UInt(row[2]), HiddenDepth = X86Fixture.UInt(row[3]) }; var hooks = new HtmlTextParserHooks(); HtmlTextParser.BindHtmlBackground(hooks, globals); Assert.Equal(X86Fixture.Text(row[5]), new HtmlTextParser(state, globals, hooks).Parse(X86Fixture.Text(row[1]))); Assert.Equal(row.Skip(6).Take(7).Select(X86Fixture.UInt), Fields(state, globals)); Assert.Equal(X86Fixture.Bytes(row[13]), globals.Background); count++;
        }
        Assert.Equal(648, count);
    }
    /// <summary>조건754행의 실제 토큰/숫자/현재 설정 치환·원문/결과 trace와 전체 상태를 대조한다.</summary>
    [Fact]
    public void All754NativeConditionsTokensLiveConfigTraceAndStateMatch()
    {
        int count = 0;
        // 앞7열만 실행 입력으로 쓰며 매 Config 호출의 독립 설정 계획을 원문처럼 교체한다.
        foreach (string[] row in X86Fixture.Read("htmlcondition"))
        {
            Assert.Equal(16, row.Length); var edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; string[] plans = row[6].Split(','); int call = 0; var trace = new List<string>(); var effect = new HtmlConditionEffect(new() { Substitute = token => { var config = new ConfigStore(); config.Push(new ConfigText(X86Fixture.Text(plans[Math.Min(call++, plans.Length - 1)]))); string result = config.Expand(token); trace.Add(Hex(token) + '=' + Hex(result)); return result; } }, edition); var state = new HtmlTextParserState { Shown = X86Fixture.UInt(row[3]), HiddenDepth = X86Fixture.UInt(row[4]) }; var globals = new HtmlTextParserGlobals(); string source = X86Fixture.Text(row[2]); if (row[1] == "Tag") effect.Apply(state, source, X86Fixture.UInt(row[5]) != 0); else { Assert.Equal("Html", row[1]); var hooks = new HtmlTextParserHooks { Condition = effect.Apply, Link = (_, _, _) => "" }; new HtmlTextParser(state, globals, hooks, edition).Parse(source); } Assert.True(state.Output == X86Fixture.Text(row[7]), $"조건행{count}: {row[0]}/{row[1]} {row[2]}"); Assert.Equal(row.Skip(8).Take(7).Select(X86Fixture.UInt), Fields(state, globals)); Assert.Equal(row[15], trace.Count == 0 ? "-" : string.Join(';', trace)); count++;
        }
        Assert.Equal(754, count);
    }
}
