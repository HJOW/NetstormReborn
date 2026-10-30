using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using ConfigText = Netstorm.Assets.ConfigText;

namespace Netstorm.Tests; // HTML을 현재 Config/배경/실제 StyleText 소유/출력에 연결한다.

/// <summary>독립 전수 외 현재 효과/파생 커서·물리 한도와 실제 조립을 확인한다.</summary>
public sealed class HtmlTextParserTests
{
    /// <summary>장기80BYTE의 짧은 덮어쓰기/숨김/빈 이름과 실제 GIF 적재를 연결한다.</summary>
    [Fact]
    public void SharedBackgroundLifetimeTailPhysicalLimitAndActualGifStorage()
    {
        var globals = new HtmlTextParserGlobals(); var a = new HtmlTextParserState(); var b = new HtmlTextParserState(); var first = new HtmlTextParserHooks(); var second = new HtmlTextParserHooks(); HtmlTextParser.BindHtmlBackground(first, globals); HtmlTextParser.BindHtmlBackground(second, globals); Assert.Equal("", HtmlTextParser.HtmlBackgroundName(globals)); var parser = new HtmlTextParser(a, globals, first); Assert.Equal("A\n", parser.Parse("<background=\"titleMenu\">A"));
        var files = GameFileSystem.Open(Path.Combine(BackgroundGifAssetsNativeTests.Root(), "originals")); var storage = new BackgroundGifStorage(files.ReadAllBytes); int width = 0, height = 0; uint bitmap = storage.Load("d/" + HtmlTextParser.HtmlBackgroundName(globals) + ".gif", ref width, ref height); Assert.Equal((639, 480), (width, height)); storage.Release(bitmap); byte[] saved = globals.Background.ToArray(); var next = new HtmlTextParser(b, globals, second); next.Parse("Next page"); Assert.Equal(saved, globals.Background); b.Shown = 0; next.Parse("<background=\"hidden\">"); Assert.Equal(saved, globals.Background); b.Shown = 1; second.Background!(b, "ab\0ignored"); Assert.Equal("ab", HtmlTextParser.HtmlBackgroundName(globals)); Assert.Equal((byte)'l', globals.Background[3]); saved = globals.Background.ToArray(); Assert.Throws<ArgumentException>(() => next.Parse("<background=\"" + new string('x', 80) + "\">")); Assert.Equal(saved, globals.Background); next.Parse("<background=\"\">"); Assert.Equal("", HtmlTextParser.HtmlBackgroundName(globals)); Assert.Equal((byte)'b', globals.Background[1]); Array.Fill(globals.Background, (byte)'x'); Assert.Throws<ArgumentException>(() => HtmlTextParser.HtmlBackgroundName(globals)); Assert.Equal(0, storage.ImageCount);
    }
    /// <summary>미연결 선택 효과/숨김 무효과와 현재 조건/명령 콜백의 원문/출력을 확인한다.</summary>
    [Fact]
    public void SelectedEffectsAreExplicitAndCallbacksChangeCurrentParserState()
    {
        var state = new HtmlTextParserState(); var globals = new HtmlTextParserGlobals(); var missing = new HtmlTextParser(state, globals);
        // 선택된 실제 효과마다 미연결을 명시적으로 거부한다.
        foreach (string source in new[] { "<$cmd 1>", "<?condition>", "<!\"picture\">", "<a href=\"target\">", "</a>", "<background=\"cloud\">" }) Assert.Throws<InvalidOperationException>(() => missing.Parse(source));
        state.Shown = 0; Assert.Equal("\n", missing.Parse("<$cmd><!\"picture\"><background=\"cloud\">")); state.Shown = 1; Assert.Equal("text\n", missing.Parse("<unknown>text")); Assert.Equal("text\n", missing.Parse("<background>text")); var calls = new List<string>(); var hooks = new HtmlTextParserHooks { Condition = (current, raw, close) => { calls.Add(raw); current.Shown = close ? 1U : 0; current.HiddenDepth = close ? 0U : 1; }, Command = (current, raw) => { calls.Add(raw); current.Output += "Q"; } }; var parser = new HtmlTextParser(state, globals, hooks); Assert.Equal("abQc\n", parser.Parse("a<?off>hidden</?off>b<$raw 7>c")); Assert.Equal(new[] { "?off>hidden</?off>b<$raw 7>c", "?off>b<$raw 7>c", "raw 7>c" }, calls); Assert.Equal((1U, 0U, 1U), (state.Shown, state.HiddenDepth, state.TextAllowed)); Assert.Equal("next\n", parser.Parse("next"));
    }
    /// <summary>태그 안/닫힘/꺾쇠 뒤의 정확한 가상 순서와 콜백 뒤 현재 커서를 확인한다.</summary>
    [Fact]
    public void DerivedVirtualSlotsKeepOrderAndCanMoveCurrentCursor()
    {
        var state = new HtmlTextParserState(); var globals = new HtmlTextParserGlobals(); var calls = new List<string>(); var hooks = new HtmlTextParserHooks { InsideTag = (current, source) => { calls.Add("in:" + current.InputOffset); Assert.NotEqual('<', source[current.InputOffset]); }, BeforeClose = (current, source) => { calls.Add("close:" + current.InputOffset); Assert.Equal('/', source[current.InputOffset]); }, AfterTag = (current, source) => { calls.Add("after:" + current.InputOffset); Assert.Equal('>', source[current.InputOffset]); Assert.Equal(0U, current.InTag); } }; var parser = new HtmlTextParser(state, globals, hooks); Assert.Equal("~N~jx~.\n", parser.Parse("<i>x</i>")); Assert.Equal(new[] { "in:1", "after:2", "close:5", "in:6", "after:7" }, calls); hooks.AfterTag = (current, _) => current.InputOffset += 2; Assert.Equal("~N~BX\n", parser.Parse("<b>12X"));
    }
    /// <summary>오른쪽 조회 후 바뀐 설정을 왼쪽이 다시 읽고 숨김/닫힘은 치환하지 않는다.</summary>
    [Fact]
    public void ConditionReadsRightThenCurrentLeftAndHiddenCloseDoesNotReadConfig()
    {
        var config = new ConfigStore(); config.Push(new ConfigText("X=1\r\n")); var state = new HtmlTextParserState(); var calls = new List<string>(); var effect = new HtmlConditionEffect(new() { Substitute = token => { calls.Add(token); string result = config.Expand(token); config.Push(new ConfigText("X=2\r\n")); return result; } }); effect.Apply(state, "?g{X}={X}>", false); Assert.Equal((1U, 0U), (state.Shown, state.HiddenDepth)); Assert.Equal(new[] { "{X}", "{X}" }, calls); state.Shown = 0; state.HiddenDepth = 2; effect.Apply(state, "?{X}>", false); Assert.Equal(2, calls.Count); effect.Apply(state, "?", true); Assert.Equal((0U, 1U), (state.Shown, state.HiddenDepth)); effect.Apply(state, "?", true); Assert.Equal((1U, 0U), (state.Shown, state.HiddenDepth)); Assert.Equal(2, calls.Count);
        var missing = new HtmlConditionEffect(new()); Assert.Throws<InvalidOperationException>(() => missing.Apply(state, "?1>", false)); state.Shown = 0; state.HiddenDepth = 1; missing.Apply(state, "?1>", false); missing.Apply(state, "?", true); Assert.Equal((1U, 0U), (state.Shown, state.HiddenDepth)); Assert.Throws<ArgumentException>(() => new HtmlConditionEffect(new() { Substitute = _ => new string('1', 100) }).Apply(state, "?1>", false)); Assert.Throws<ArgumentException>(() => effect.Apply(state, "1>", false));
    }
    /// <summary>같은 실제 ConfigStore의 이름 범위/현재 값/기본값을 파서에 연결한다.</summary>
    [Fact]
    public void ActualConfigBindingUsesNamespacesDefaultsAndCurrentValues()
    {
        var config = new ConfigStore(); config.Push(new ConfigText("ShardtoStone=100\r\n")); config.Push(new ConfigText("3=99\r\n"), "param"); var state = new HtmlTextParserState(); var hooks = new HtmlTextParserHooks(); HtmlConditionEffect.BindHtmlConditionConfig(hooks, config); var parser = new HtmlTextParser(state, new(), hooks); Assert.Equal("lessend\n", parser.Parse("<?l {param.3}={ShardtoStone}>less</?>end")); config.Push(new ConfigText("3=100\r\n"), "param"); Assert.Equal("enoughend\n", parser.Parse("<?l! {param.3}={ShardtoStone}>enough</?>end")); Assert.Equal("end\n", parser.Parse("<?{missing|0}>hidden</?>end")); Assert.Equal("visibleend\n", parser.Parse("<?!{missing|0}>visible</?>end")); Assert.Equal(3, config.LayerCount);
    }
    /// <summary>HTML 제어문자를 실제 StyleText 조각/색/굵기·그림 소유와 글리프 픽셀에 전달한다.</summary>
    [Fact]
    public void ActualStyleTextInlineContextsPictureFragmentsAndGlyphPixels()
    {
        var scene = new StyleMarkupScene(); var html = new HtmlTextParser(new(), new()); string converted = html.Parse("walk <i>edge</i> and <b>open</b>."); Assert.Equal("walk ~N~jedge~. and ~N~Bopen~N.\n", converted); scene.Scene.StyleHooks.Color = code => { Assert.Equal((byte)'j', code); return 77; }; scene.Scene.Style.Parse(ref scene.Head, Encoding.Latin1.GetBytes(converted), scene.Scene.Context); uint[] chain = StyleTextTestSupport.Chain(scene.Scene.Fragments, scene.Head); Assert.Equal("walk edge and open.\n", string.Concat(chain.Select(token => StyleTextTestSupport.Text(scene.Scene.Fragments.Fragments[token])))); var edge = Assert.Single(chain.Select(token => scene.Scene.Fragments.Fragments[token]), node => StyleTextTestSupport.Text(node) == "edge"); Assert.Equal((77U, 0U), (edge.Context[2], edge.Context[3])); var open = Assert.Single(chain.Select(token => scene.Scene.Fragments.Fragments[token]), node => StyleTextTestSupport.Text(node) == "open"); Assert.Equal(2U, open.Context[3]); scene.Scene.Style.Clear(ref scene.Head);
        var hooks = new HtmlTextParserHooks(); HtmlTextParser.BindHtmlPictureText(hooks); converted = new HtmlTextParser(new(), new(), hooks).Parse("A<!\"Unit.1\">A"); Assert.Equal("A~[IUnit.1]A\n", converted); scene.Scene.Style.Parse(ref scene.Head, Encoding.Latin1.GetBytes(converted), scene.Scene.Context); Assert.Equal(1U, Assert.Single(scene.Pictures.Pictures).Value.Frame); scene.Scene.Style.Clear(ref scene.Head); Assert.Empty(scene.Pictures.Pictures); Assert.Empty(scene.Scene.Fragments.Fragments);
        scene.Scene.Style.Parse(ref scene.Head, Encoding.Latin1.GetBytes(html.Parse("<i>A</i>")), scene.Scene.Context); scene.Scene.Pixels.Span.Fill(7); scene.Scene.Style.Draw(scene.Head, new(2, 2)); Assert.Contains((byte)77, scene.Scene.Pixels.ToArray()); Assert.Equal(7, scene.Scene.Guarded[0]); Assert.Equal(7, scene.Scene.Guarded[^1]); scene.Scene.Style.Clear(ref scene.Head); Assert.Empty(scene.Scene.Fragments.Fragments);
    }
    /// <summary>공통 토큰의 한도/인용 입력 소비·구분자·첫 공백·NUL 경계를 고정 입력으로 확인한다.</summary>
    [Fact]
    public void RawTokenPreservesQuotedInputLimitDelimitersFirstSpaceAndNul()
    {
        Assert.Equal(new RawTextTokenResult("A=B", 11), RawTextToken.Read(" \t\"A=B\"===>x", 0, "=>")); Assert.Equal(new RawTextTokenResult(" ", 3), RawTextToken.Read("\" \"")); Assert.Equal(new RawTextTokenResult(new string('x', 77), 78), RawTextToken.Read("\"" + new string('x', 90), 0, "=")); Assert.Equal(new RawTextTokenResult(new string('x', 78), 81), RawTextToken.Read(new string('x', 78) + "===y", 0, "=")); Assert.Equal(new RawTextTokenResult("abc", 3), RawTextToken.Read("abc\0def")); Assert.Throws<ArgumentOutOfRangeException>(() => RawTextToken.Read("a\0b", 2)); Assert.Throws<ArgumentOutOfRangeException>(() => RawTextToken.Read("a", -1));
    }
    /// <summary>논리 여유의 strict 비교와 콜백의 현재 물리 포인터 오류를 구분한다.</summary>
    [Fact]
    public void StrictLogicalReserveAndCurrentCallbackBufferBounds()
    {
        var state = new HtmlTextParserState(); var hooks = new HtmlTextParserHooks(); var parser = new HtmlTextParser(state, new(), hooks); Assert.Equal("ABC\n", parser.Parse("ABCDE", 258)); Assert.Equal(3, state.InputOffset); Assert.Equal("A\n", parser.Parse("ABC", 256)); Assert.Equal("\n", parser.Parse("ABC", 255)); Assert.Throws<ArgumentException>(() => parser.Parse("", 1)); Assert.Throws<InvalidOperationException>(() => parser.Parse("<b><b>", 2)); hooks.AfterTag = (current, source) => current.InputOffset = source.Length + 1; Assert.Throws<ArgumentOutOfRangeException>(() => parser.Parse("<x>")); hooks.AfterTag = (current, _) => current.Output = new string('x', 8192); Assert.Throws<ArgumentOutOfRangeException>(() => parser.Parse("<x>"));
    }
}
