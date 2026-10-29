using System.Text;
using System.Text.Json;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 전용 Help의 현재 효과/원문 기하·기존 실제 HTML/수명 연결을 검사한다.

/// <summary>독립 전수 외 콜백 재조회/부분 상태와 실제 장치 조립을 확인한다.</summary>
public sealed class QuickHelpWindowTests
{
    /// <summary>같은 원문 생성 입력을 실행하며 기대값을 만들지 않는다.</summary>
    private static void Construct(QuickHelpWindowFixtureScene scene, string input = "[0,1,0,1065353216]") { using var json = JsonDocument.Parse(input); scene.Run("C", json.RootElement, false); }
    /// <summary>두 질감 자식은 각각 현재 배경 TYPE/프레임을 읽고 단색 배경은 TYPE0이다.</summary>
    [Fact]
    public void ConstructReadsBackgroundGlobalsSeparatelyForEachChild()
    {
        var s = new QuickHelpWindowFixtureScene(); s.State.BackgroundType = 92; s.State.BackgroundFrame = 1; s.BackgroundCreated = token => { if (token == QuickHelpWindowFixtureScene.PictureBg) { s.State.BackgroundType = 93; s.State.BackgroundFrame = 2; } }; Construct(s); Assert.Contains($"Background,{QuickHelpWindowFixtureScene.Inner},0,0,{0x04000000U | 35},1,0,0", s.Trace); Assert.Contains($"Background,{QuickHelpWindowFixtureScene.PictureBg},0,0,{0x03e40000U},1,92,1", s.Trace); Assert.Contains($"Background,{QuickHelpWindowFixtureScene.BodyBg},0,0,{0x03e40000U},1,93,2", s.Trace);
    }
    /// <summary>라벨은 생성 인자,선택 Cancel은 현재 LearnType,캐시/등록 serial은 적재 뒤 현재 값을 쓴다.</summary>
    [Fact]
    public void ConstructPreservesOriginalLabelAndRequeriesLearnTypeAndSerial()
    {
        var s = new QuickHelpWindowFixtureScene(); s.State.Kind = -1; s.Hooks.Translate = label => { if (label == "OK") s.Node.LearnType = 71; return label; }; s.Hooks.Load = name => { Assert.Equal("d/help.english", name); s.Node.Serial = 909; s.State.Kind = -2; return ""; }; Construct(s, "[0,0,0,1065353216]"); Assert.Contains($"Stone,{QuickHelpWindowFixtureScene.Learn},0,0,4f4b,0,0", s.Trace); Assert.NotEqual(0U, s.Node.CancelButton); Assert.Equal(71U, s.Node.LearnType); Assert.Equal(909U, s.State.Id); Assert.Equal("\n", s.State.HelpData); Assert.Equal(uint.MaxValue, s.Node.Command); Assert.Equal(0x800U, s.Node.Flags & 0x800);
    }
    /// <summary>치환 전 빌린 같은 본문 배열을 유지하고 치환 뒤 현재 Body 토큰/프레임을 읽는다.</summary>
    [Fact]
    public void ShowKeepsCapturedOutputArrayAndReadsCurrentBodyAndHelpFrame()
    {
        var s = new QuickHelpWindowFixtureScene(); const uint alternateToken = 0x12060000; var alternate = new HtmlBodyGumpNode(); "keep\0"u8.CopyTo(alternate.Body); s.Hooks.Render = (body, type, topic, _, _) => { Assert.Equal((QuickHelpWindowFixtureScene.BodyToken, 1U, "unitType"), (body, type, topic)); "before\0"u8.CopyTo(s.BodyNode.Body); }; s.Hooks.Html = token => token == QuickHelpWindowFixtureScene.BodyToken ? s.BodyNode : token == alternateToken ? alternate : null; s.Hooks.Substitute = text => { Assert.Equal("before", text); s.Node.Body = alternateToken; return "after"; }; bool frameRead = false, displayed = false; s.Hooks.HelpFrame = type => { Assert.Equal(1U, type); Assert.Equal(alternateToken, s.Node.Body); frameRead = true; return 17; }; s.Hooks.BodyText = (token, text) => { Assert.True(frameRead); Assert.Equal((alternateToken, "after"), (token, text)); displayed = true; }; s.Help.Show(QuickHelpWindowFixtureScene.Window, 1, "unitType"); Assert.True(displayed); Assert.Equal("after\0", Encoding.Latin1.GetString(s.BodyNode.Body, 0, 6)); Assert.Equal("keep\0", Encoding.Latin1.GetString(alternate.Body, 0, 5)); Assert.Contains($"Picture,{QuickHelpWindowFixtureScene.Picture},1,17,8", s.Trace);
    }
    /// <summary>Reset 실패 때 이미 반납한 그림/본문과 아직 덮지 않은 이전 정보 토큰을 구별한다.</summary>
    [Fact]
    public void ResetKeepsPreviousInfoTokenUntilNewPaneConstructionSucceeds()
    {
        var s = new QuickHelpWindowFixtureScene(); s.Node.Picture = QuickHelpWindowFixtureScene.Picture; s.Node.Information = QuickHelpWindowFixtureScene.Info; s.BodyNode.Text = QuickHelpWindowFixtureScene.Scroll; const uint next = 0x12070000; s.Hooks.Allocate = bytes => { Assert.Equal(0xd8, bytes); return next; }; s.Hooks.ConstructInformation = (token, _, _, _, _) => { Assert.Equal(next, token); Assert.Equal(QuickHelpWindowFixtureScene.Info, s.Node.Information); throw new InvalidOperationException("명시 정보 pane 생성 실패"); }; Assert.Throws<InvalidOperationException>(() => s.Help.Reset(QuickHelpWindowFixtureScene.Window)); Assert.Equal(0U, s.Node.Picture); Assert.Equal(0U, s.BodyNode.Text); Assert.Equal(QuickHelpWindowFixtureScene.Info, s.Node.Information); Assert.Equal(new[] { "Kill," + QuickHelpWindowFixtureScene.Picture, "Kill," + QuickHelpWindowFixtureScene.Scroll, "Kill," + QuickHelpWindowFixtureScene.Info }, s.Trace);
    }
    /// <summary>strict>의 NaN 선택과 Push 뒤 현재 버튼/창 높이 재조회·초기 폭 목록을 구별한다.</summary>
    [Fact]
    public void AttachUsesOriginalNanChoiceAndCurrentButtonAfterPush()
    {
        var s = new QuickHelpWindowFixtureScene(); s.Node.Information = 0; s.Base(QuickHelpWindowFixtureScene.Back).BaseWidth = float.NaN; s.Base(QuickHelpWindowFixtureScene.Learn).BaseWidth = 60; s.Base(QuickHelpWindowFixtureScene.Cancel).BaseWidth = 50; s.Base(QuickHelpWindowFixtureScene.Learn).BaseHeight = 20; const uint replacement = 0x12060000; s.Base(replacement).BaseWidth = 999; s.Hooks.Push = token => { s.Push(token); if (token == QuickHelpWindowFixtureScene.Back) { s.Node.BackButton = replacement; s.Node.BaseHeight = 360; } }; s.Help.Attach(QuickHelpWindowFixtureScene.Window, 13, 21, 0, 0); Assert.Equal(60f, s.Base(QuickHelpWindowFixtureScene.Back).BaseWidth); Assert.Equal(60f, s.Base(QuickHelpWindowFixtureScene.Learn).BaseWidth); Assert.Equal(999f, s.Base(replacement).BaseWidth); Assert.Equal((120f, 330f), (s.Base(replacement).X, s.Base(replacement).Y)); Assert.Equal((190f, 330f), (s.Base(QuickHelpWindowFixtureScene.Learn).X, s.Base(QuickHelpWindowFixtureScene.Learn).Y));
    }
    /// <summary>현재 실제 전용 파서→같은 HTML 부모/스크롤/보호 글꼴·지연 수명을 연결한다.</summary>
    [Fact]
    public void ActualShowUsesExistingHtmlScrollGlyphAndCommonLifetimeDevices()
    {
        var s = new HtmlBodyWidgetScene(); var window = new QuickHelpContentNode { Body = 40 }; s.Common.Nodes.Add(50, window); s.Common.Factory.Construct(50, 19, 450, 350, 1); s.Common.Tree.Attach(50, 0, 0, 1, 1); var state = new QuickHelpWindowState { HelpData = "<a name=\"introHelp\">A</a>\nSTOP", Color = 171 }; var popup = new QuickHelpPopupState(); var hooks = new QuickHelpWindowHooks { Node = token => token == 50 ? window : null, Base = token => s.Common.Nodes.GetValueOrDefault(token), Html = token => token == 40 ? s.Node : null, Allocate = bytes => { Assert.Equal(0xd8, bytes); s.Common.Nodes.Add(51, new GumpCreateNode()); return 51; }, ConstructInformation = (token, width, height, font, _) => { Assert.Equal((51U, 0, 0, 5U), (token, width, height, font)); s.Common.Factory.Construct(token, 0, 0, 0, 1); }, Attach = (token, x, y, parent, mode) => s.Common.TreeHooks.Attach!(token, x, y, parent, mode), Substitute = text => text, Translate = text => text };
        QuickHelpWindow.BindQuickHelpWindowDevices(hooks, s.Common.Factory, s.Common.Tree, s.Common.Lifetime, s.Body); hooks.Render = (body, type, topic, information, stream) => { Assert.Equal((40U, 0U, "introHelp", 51U, 0U), (body, type, topic, information, stream)); var parserState = new HtmlTextParserState(); var basic = new HtmlTextParserHooks(); HtmlLinkText.BindHtmlLinkText(basic, s.Links); var parser = new QuickHelpHtml(parserState, s.Globals, state.HtmlMode, 0, topic, false, new(), new(), basic, s.Links); s.Body.Parse(body, new() { HasOutput = () => false, Bind = (_, _) => { }, Run = () => { string result = parser.Parse(state.HelpData); Encoding.Latin1.GetBytes(result).AsSpan().CopyTo(s.Node.Body); s.Node.Body[result.Length] = 0; s.Links.Bytes.ToArray().AsSpan().CopyTo(s.Node.Links); } }); }; var help = new QuickHelpWindow(state, popup, hooks); help.Show(50, 0, ""); Assert.Equal(30U, s.Node.Text); Assert.Equal(51U, window.Information); Assert.Equal(0U, state.InfoMode); Assert.StartsWith("~5~wA", Encoding.Latin1.GetString(s.Node.Body)); s.Common.TreeHooks.Attach!(40, 0, 0, 50, 1); s.Common.Draw.Draw(30); Assert.Contains((byte)77, s.Common.Pixels.ToArray()); s.Common.Lifetime.Kill(50); s.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Common.Create.Count); Assert.Equal(new uint[] { 500 }, s.FreedBuffers); Assert.Empty(s.Fragments.Fragments);
    }
    /// <summary>Render 실패 뒤 현재 새pane/본문 접두부와 정보 모드1을 임의로 되돌리지 않는다.</summary>
    [Fact]
    public void FailedRenderRetainsNewPanePrefixAndCurrentInfoMode()
    {
        var s = new QuickHelpWindowFixtureScene(); s.Hooks.Render = (_, _, _, _, _) => { "prefix\0"u8.CopyTo(s.BodyNode.Body); throw new InvalidOperationException("명시 전용 HTML 실패"); }; s.Hooks.Substitute = _ => throw new InvalidOperationException("실패 뒤 치환에 도달하면 안 됨"); Assert.Throws<InvalidOperationException>(() => s.Help.Show(QuickHelpWindowFixtureScene.Window, 1, "unitType")); Assert.Equal(1U, s.State.InfoMode); Assert.Equal(QuickHelpWindowFixtureScene.Info, s.Node.Information); Assert.Equal("prefix\0", Encoding.Latin1.GetString(s.BodyNode.Body, 0, 7)); Assert.DoesNotContain(s.Trace, text => text.StartsWith("Substitute,"));
    }
    /// <summary>파일명/본문 물리 한도는 실패 앞의 생성/표식 효과를 보존하고 표시를 추가하지 않는다.</summary>
    [Fact]
    public void FileAndBodyLimitsPreserveOnlyReachedEffects()
    {
        var s = new QuickHelpWindowFixtureScene(); s.Hooks.Config = _ => new string('x', 256); s.Hooks.Load = _ => throw new InvalidOperationException("파일명 거부 뒤 적재 금지"); Assert.Throws<ArgumentException>(() => Construct(s, "[0,0,1,1065353216]")); Assert.NotEqual(0U, s.Node.BackButton); Assert.Equal(0U, s.State.Id); Assert.Equal(0x2000095U, s.Popup.Command); Assert.Equal("mission", s.Popup.Argument()); var longBody = new QuickHelpWindowFixtureScene(); longBody.Hooks.Render = (_, _, _, _, _) => { longBody.BodyNode.Body[0] = 0; }; longBody.Hooks.Substitute = _ => new string('x', HtmlBodyGumpNode.Capacity); bool displayed = false; longBody.Hooks.BodyText = (_, _) => displayed = true; Assert.Throws<ArgumentException>(() => longBody.Help.Show(QuickHelpWindowFixtureScene.Window, 0, "introHelp")); Assert.False(displayed); Assert.Equal(0U, longBody.State.InfoMode);
    }
}
