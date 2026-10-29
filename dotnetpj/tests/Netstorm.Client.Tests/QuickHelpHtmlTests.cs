using System.Text;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 실제 링크 소유/표시 적중과 전용 Help의 현재 효과 경계를 검사한다.

/// <summary>독립 원문 대조 외 실제 저장소/StyleText/픽셀과 콜백 뒤 현재 상태를 확인한다.</summary>
public sealed class QuickHelpHtmlTests
{
    /// <summary>비활성 상태도 실제 링크 helper를 연결하여 원문 null 저장소 분기를 실행한다.</summary>
    private static HtmlTextParserHooks Basic(HtmlLinkTextState links) { var hooks = new HtmlTextParserHooks(); HtmlLinkText.BindHtmlLinkText(hooks, links); return hooks; }
    /// <summary>첫#만 생략하고 원문 BYTE·연속 NUL·닫힘/인용 없음의 무소비를 유지한다.</summary>
    [Fact]
    public void LinkStoragePreservesRawTargetsNulTokensAndRewoundTail()
    {
        var links = new HtmlLinkTextState { Base = 100 }; Assert.Equal("~[T100]", HtmlLinkText.Convert(links, "a href=\"# Topic\">rest", false)); Assert.Equal(" Topic", HtmlLinkText.ResolveHtmlLink(links, 100)); Assert.Equal(7, links.Cursor); Assert.Equal("~[T107]", HtmlLinkText.Convert(links, "a href=\"cmd:Go\"", false)); Assert.Equal("cmd:Go", HtmlLinkText.ResolveHtmlLink(links, 107)); byte[] prior = links.Bytes.ToArray(); int cursor = links.Cursor; Assert.Equal("~[T0]", HtmlLinkText.Convert(links, "ignored", true)); Assert.Equal("~[T0]", HtmlLinkText.Convert(links, "a>\"ignored\"", false)); Assert.Equal(cursor, links.Cursor); Assert.Equal(prior, links.Bytes); links.Cursor = 0; Assert.Equal("~[T100]", HtmlLinkText.Convert(links, "\"##X\"\0ignored", false)); Assert.Equal("#X", HtmlLinkText.ResolveHtmlLink(links, 100)); Assert.Equal(3, links.Cursor); Assert.Equal(prior.Length, links.Bytes.Count); Assert.Equal(prior.AsSpan(3).ToArray(), links.Bytes.Skip(3)); Assert.Equal("~[T103]", HtmlLinkText.Convert(links, "\"\"", false)); Assert.Equal("", HtmlLinkText.ResolveHtmlLink(links, 103));
    }
    /// <summary>비활성 저장소는 완전 무효과이며 현재 물리 용량/포인터/NUL의 오류만 명시 거부한다.</summary>
    [Fact]
    public void LinkDisabledCapacityTokenAndPhysicalNulBoundaries()
    {
        var links = new HtmlLinkTextState { Base = 0, Enabled = false, Capacity = 0, Cursor = 99 }; Assert.Equal("", HtmlLinkText.Convert(links, "\"X\"", false)); Assert.Empty(links.Bytes); Assert.Throws<ArgumentOutOfRangeException>(() => HtmlLinkText.ResolveHtmlLink(links, 0)); links.Enabled = true; links.Cursor = 0; Assert.Throws<ArgumentException>(() => HtmlLinkText.Convert(links, "\"X\"", false)); links.Base = uint.MaxValue; links.Capacity = 10; Assert.Throws<ArgumentException>(() => HtmlLinkText.Convert(links, "\"\"", false)); links.Base = 1; links.Capacity = 2; Assert.Equal("~[T1]", HtmlLinkText.Convert(links, "\"X\"", false)); Assert.Throws<ArgumentException>(() => HtmlLinkText.Convert(links, "\"Y\"", false)); Assert.Equal(new byte[] { (byte)'X', 0 }, links.Bytes); links.Cursor = 3; Assert.Throws<ArgumentException>(() => HtmlLinkText.Convert(links, "\"\"", false)); Assert.Throws<ArgumentOutOfRangeException>(() => HtmlLinkText.ResolveHtmlLink(links, 0)); Assert.Throws<ArgumentOutOfRangeException>(() => HtmlLinkText.ResolveHtmlLink(links, 3)); links.Bytes[1] = (byte)'Y'; Assert.Throws<ArgumentException>(() => HtmlLinkText.ResolveHtmlLink(links, 1));
    }
    /// <summary>실제 HTML T태그를 다중 줄 조각/복제/적중/글리프 출력과 소유 수명에 연결한다.</summary>
    [Fact]
    public void ActualHtmlLinksFlowThroughScrollingFragmentsHitAndGlyphPixels()
    {
        var links = new HtmlLinkTextState(); var hooks = new HtmlTextParserHooks(); HtmlLinkText.BindHtmlLinkText(hooks, links); string styled = new HtmlTextParser(new(), new(), hooks).Parse("<a href=\"#Topic\">A</a>A"); var scene = new StyleMarkupScene(); scene.Scene.StyleHooks.Color = _ => 77; scene.Parser.Parse(2, Encoding.Latin1.GetBytes(styled)); scene.Scene.Draw.Draw(2); uint selected = scene.Scene.Scroll.HitTest(2, new(2, 2)); Assert.NotEqual(0U, selected); uint metadata = scene.Scene.Fragments.Fragments[selected].Metadata; Assert.Equal(links.Base, metadata); Assert.Equal("Topic", HtmlLinkText.ResolveHtmlLink(links, metadata)); Assert.Equal(0U, scene.Scene.Scroll.HitTest(2, new(5, 2))); uint second = scene.Scene.Scroll.HitTest(2, new(5, 3)); Assert.NotEqual(0U, second); Assert.Equal(0U, scene.Scene.Fragments.Fragments[second].Metadata); Assert.Contains((byte)77, scene.Scene.Pixels.ToArray()); Assert.Equal(7, scene.Scene.Guarded[0]); Assert.Equal(7, scene.Scene.Guarded[^1]); scene.Scene.Buffer.Destroy(); Assert.Empty(scene.Scene.Fragments.Fragments); Assert.Equal("Topic", HtmlLinkText.ResolveHtmlLink(links, links.Base));
    }
    /// <summary>기본 생성 초기화→현재 TYPE→shown0과 대소문자/접두 주제 선택을 확인한다.</summary>
    [Fact]
    public void ConstructorResetsCurrentFieldsBeforeTypeLookupAndTopicPrefixSelection()
    {
        var state = new HtmlTextParserState { Shown = 99, HiddenDepth = 99, Output = "prior" }; int calls = 0; var hooks = new QuickHelpHtmlHooks { Type = () => { calls++; Assert.Equal((1U, 0U, ""), (state.Shown, state.HiddenDepth, state.Output)); state.HiddenDepth = 7; return new(); } }; var links = new HtmlLinkTextState { Enabled = false }; var html = new QuickHelpHtml(state, new(), new(), 1, "Top", false, new(), hooks, Basic(links), links); Assert.Equal(1, calls); Assert.Equal((0U, 7U), (state.Shown, state.HiddenDepth)); state.HiddenDepth = 0; Assert.Equal("~5~wA~N~.\n", html.Parse("<a name=\"TOPIC\">A</a>\nSTOP")); Assert.Equal(0U, state.TextAllowed); var missed = new QuickHelpHtml(new(), new(), new(), 0, "Top", false, new(), new()); Assert.Equal("\n", missed.Parse("<a NAME=\"Top\">A</a>")); Assert.Throws<InvalidOperationException>(() => new QuickHelpHtml(new(), new(), new(), 1, "Top", false, new(), new())); var prior = new HtmlTextParserState { Shown = 99 }; Assert.Throws<ArgumentException>(() => new QuickHelpHtml(prior, new(), new(), 0, "", false, new(), new(), new() { Link = (_, _, _) => "" })); Assert.Equal(99U, prior.Shown);
    }
    /// <summary>번역/HP/본문 효과 뒤 TYPE/현재 pane 플래그를 다시 읽으며 중첩 모드를 보존한다.</summary>
    [Fact]
    public void DynamicInfoRequeriesTypeAfterTranslationAndCurrentPaneAfterText()
    {
        var scene = new QuickHelpHtmlScene(1, false, 8); scene.Mode.InfoMode = 17; var hooks = scene.Hooks(); int hits = 0; hooks.HitPoints = () => ++hits == 1 ? 5 : 9; hooks.Translate = pattern => { if (pattern == "Alignment: <c>%s</c>") scene.Type.Theme = 4; if (pattern == "Class: <c>%s</c>") scene.Type.Classification = "Next Class"; if (pattern == "Range: <c>%d</c>") scene.Type.Range = 41.9f; if (pattern == "Damage: <c>%d</c>") scene.Type.Damage = -7.9f; if (pattern.StartsWith("Cost in Storm Power:")) scene.Type.Cost = 23.8f; return pattern; }; string output = ""; hooks.PaneText = styled => { Assert.Equal(1U, scene.Mode.InfoMode); output = styled; scene.Pane.Flags = 0; }; var html = new QuickHelpHtml(scene.State, scene.Globals, scene.Mode, 1, "Unit", false, new(), hooks, Basic(scene.Links), scene.Links); html.Parse("<a name=\"Unit\"><info></a>\nSTOP"); Assert.Contains("Energy", output); Assert.Contains("Next Class", output); Assert.Contains("9~.", output); Assert.Contains("41~.", output); Assert.Contains("-7~.", output); Assert.Contains("23~.", output); Assert.Equal(2, hits); Assert.Equal(new[] { "Scroll,0", "Push", "PaneAttach" }, scene.Trace); Assert.Equal(1U, scene.Mode.InfoMode);
    }
    /// <summary>중첩 해석 중의 현재 모드와 링크 위치 복구·부분 예외의 미복구 상태를 구별한다.</summary>
    [Fact]
    public void NestedParserSharesBytesRestoresCursorOnlyAfterSuccessfulParse()
    {
        var scene = new QuickHelpHtmlScene(1, true, 8) { Description = "<a href=\"#Inner\">A</a>" }; var basic = new HtmlTextParserHooks(); HtmlLinkText.BindHtmlLinkText(basic, scene.Links); var link = basic.Link!; int nested = 0; basic.Link = (state, raw, close) => { if (raw.Contains("#Inner")) { Assert.Equal(0U, scene.Mode.InfoMode); nested++; } return link(state, raw, close); }; var hooks = scene.Hooks(); var paneText = hooks.PaneText!; hooks.PaneText = text => { Assert.Equal(1U, scene.Mode.InfoMode); Assert.Equal(0, scene.Links.Cursor); paneText(text); }; var html = new QuickHelpHtml(scene.State, scene.Globals, scene.Mode, 1, "Unit", false, new(), hooks, basic, scene.Links); html.Parse("<a name=\"Unit\"><info><a href=\"#Outer\">A</a></a>\nSTOP"); Assert.Equal(1, nested); Assert.Equal("Outer", HtmlLinkText.ResolveHtmlLink(scene.Links, scene.Links.Base)); Assert.Equal(6, scene.Links.Cursor);
        var failed = new QuickHelpHtmlScene(1, true, 8) { Description = "<a href=\"#Inner\">A</a><$bad>" }; var effects = new HtmlTextParserHooks { Command = (_, _) => throw new InvalidOperationException("명시 원문 외부 효과 중단") }; HtmlLinkText.BindHtmlLinkText(effects, failed.Links); failed.Mode.InfoMode = 99; var stopped = new QuickHelpHtml(failed.State, failed.Globals, failed.Mode, 1, "Unit", false, new(), failed.Hooks(), effects, failed.Links); Assert.Throws<InvalidOperationException>(() => stopped.Parse("<a name=\"Unit\"><info></a>")); Assert.Equal(0U, failed.Mode.InfoMode); Assert.Equal(6, failed.Links.Cursor); Assert.Equal("Inner", HtmlLinkText.ResolveHtmlLink(failed.Links, failed.Links.Base)); Assert.Empty(failed.Trace);
    }
    /// <summary>마나 문자표/점/강조 감김과 원문250/256BYTE·비ASCII 경계를 확인한다.</summary>
    [Fact]
    public void ManaFramesDotsHighlightWrapAndOriginalPhysicalLimits()
    {
        var mana = new ManaPictureState(); Assert.Equal("~[I103.11]~[I103.1]~[I103.0]", QuickHelpHtml.ManaPictureText(".w..r.x", mana, 1)); mana.Frames['w'] = int.MaxValue; Assert.Equal("~[I103.-2147483638]", QuickHelpHtml.ManaPictureText("w", mana, 1)); Assert.Equal("", QuickHelpHtml.ManaPictureText("...\0w", mana)); Assert.Throws<ArgumentException>(() => QuickHelpHtml.ManaPictureText(new string('.', 256), mana)); Assert.Throws<ArgumentOutOfRangeException>(() => QuickHelpHtml.ManaPictureText("é", mana)); Assert.Throws<ArgumentException>(() => QuickHelpHtml.ManaPictureText(new string('r', 28), mana)); Assert.Equal(243, QuickHelpHtml.ManaPictureText(new string('r', 27), mana).Length); var state = new HtmlTextParserState { TextAllowed = 9 }; Assert.Throws<ArgumentOutOfRangeException>(() => QuickHelpHtml.QuickHelpBeforeClose(state, "/")); state.InputOffset = 1; QuickHelpHtml.QuickHelpBeforeClose(state, "\r/"); Assert.Equal(0U, state.TextAllowed);
    }
    /// <summary>필수 선택 장치와 지역 버퍼/주제 스타일 출력 한도의 명시 경계를 확인한다.</summary>
    [Fact]
    public void RequiredInfoDevicesAndFixedLocalOutputLimitsRemainExplicit()
    {
        var scene = new QuickHelpHtmlScene(1, false, 8); var hooks = scene.Hooks(); hooks.Print = (_, _) => new string('x', 256); var html = new QuickHelpHtml(scene.State, scene.Globals, scene.Mode, 1, "Unit", false, new(), hooks); Assert.Throws<ArgumentException>(() => html.Parse("<a name=\"Unit\"><info>")); var missing = new QuickHelpHtml(new(), new(), new(), 1, "Unit", false, new(), new() { Type = () => new() }); Assert.Throws<InvalidOperationException>(() => missing.Parse("<a name=\"Unit\"><info>")); var shortOutput = new QuickHelpHtml(new(), new(), new(), 0, "T", false, new(), new()); Assert.Throws<ArgumentException>(() => shortOutput.Parse("<a name=\"T\">", 4));
    }
}
