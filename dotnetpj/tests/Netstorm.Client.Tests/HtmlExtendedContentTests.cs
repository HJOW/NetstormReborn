using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// 확장 토큰의 파서 보존과 전체 생성의 내용 인정을 검사한다
/// (cpppj <c>client/HtmlTextParser.cpp</c>·<c>client/HtmlWindowFactory.cpp</c>의 HEAD 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.353에서 K1 확장 본문 재대조와 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class HtmlExtendedContentTests
{
    /// <summary>UTF-16 본문을 Latin1 원시 입력으로만 준비한다.</summary>
    /// <param name="value">본문이다.</param>
    /// <returns>원시 입력이다.</returns>
    private static string Raw(string value) => Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText(value));

    /// <summary>확장 토큰을 끊지 않고 통째로 보존한다.</summary>
    [Fact]
    public void ParserPreservesExtendedTokenWhole()
    {
        string token = Raw("한");
        var globals = new HtmlTextParserGlobals { ExtendedText = true };
        var parser = new HtmlTextParser(new HtmlTextParserState(), globals);
        // 제어 바이트 네 개를 통째로 보존하고 하위 숫자를 일반 공백·태그로 재해석하지 않는다
        Assert.Equal("A" + token + "B\n", parser.Parse("A" + token + "B"));
    }

    /// <summary>관문이 꺼지면 제어 바이트는 일반 바이트처럼 버려진다.</summary>
    [Fact]
    public void ParserDropsControlBytesWithoutGate()
    {
        string token = Raw("한");
        var parser = new HtmlTextParser(new HtmlTextParserState(), new HtmlTextParserGlobals());
        Assert.Equal("AB\n", parser.Parse("A" + token + "B"));
    }

    /// <summary>내용 없음 판정은 ASCII 관문을 유지하고 실제 클론 언어에서만 확장 문자를 인정한다.</summary>
    [Fact]
    public void FactoryContentGateKeepsAsciiAndAdmitsExtended()
    {
        var node = new HtmlWindowGumpNode { Serial = 7, BaseWidth = 100, BaseHeight = 100 };
        var trace = new List<string>();
        var hooks = new HtmlWindowFactoryHooks
        {
            Node = _ => node,
            Create = (_, _) => 1,
            Substitute = (source, _) => source,
            Parse = (_, _, _, _) => "",
            WallSeconds = () => 0,
            Body = (token, text) => trace.Add("body:" + token + ":" + text),
            Attach = (token, x, y, parent, mode) => trace.Add($"attach:{token}:{parent}:{mode}"),
            Modal = (token, enabled) => trace.Add("modal:" + token + ":" + enabled),
            Read = address => address switch { 0x5c78f0 => 640u, 0x5c78f4 => 480u, 0x563e38 => 1u, _ => throw new ArgumentOutOfRangeException(nameof(address)) },
            Kill = token => trace.Add("kill:" + token),
            Metadata = (_, _) => 1,
        };
        var factory = new HtmlWindowFactory(new HtmlWindowGumpState(), hooks);
        // 빈 장치는 ASCII 관문 그대로 빈 창을 버린다
        Assert.Equal(0u, factory.Show("", 0, 100, 100));
        Assert.Contains("kill:1", trace);
        // 확장 문자를 내용으로 인정하면 빈 ASCII도 창을 만든다
        string seen = "?";
        hooks.LocalizedContent = text => { seen = text; return true; };
        trace.Clear();
        Assert.Equal(7u, factory.Show("", 0, 100, 100));
        Assert.Equal("", seen);
        Assert.DoesNotContain("kill:1", trace);
        Assert.Contains("modal:1:True", trace);
    }
}
