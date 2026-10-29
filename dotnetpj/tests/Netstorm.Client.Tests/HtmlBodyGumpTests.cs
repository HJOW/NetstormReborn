using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // HTML 부모의 현재 버퍼/자식·배치와 실제 내장 설명 장치를 검사한다.

/// <summary>원본 전수 외 미작성 영역/실패 부분 상태·실제 공통 UI/Config 연결을 확인한다.</summary>
public sealed class HtmlBodyGumpTests
{
    /// <summary>첫 NUL 앞 원시 BYTE를 읽는다.</summary>
    private static string CText(byte[] bytes) { int end = Array.IndexOf(bytes, (byte)0); return Encoding.Latin1.GetString(bytes, 0, end < 0 ? bytes.Length : end); }
    /// <summary>원문 두 첫 NUL/정수 화면 절반·signed 종류와 배열 뒤쪽 보존을 확인한다.</summary>
    [Fact]
    public void ConstructOnlyClearsFirstBytesAndUsesSignedIntegerHalfScreen()
    {
        var n = new HtmlBodyGumpNode(); Array.Fill(n.Body, (byte)0xa5); Array.Fill(n.Links, (byte)0x5a); var body = new HtmlBodyGump(new() { ScreenWidth = -5, ScreenHeight = -7, Kind = -1 }, new() { Node = _ => n, ConstructBase = (_, c, w, h, f) => { Assert.Equal((0U, 0f, 0f, 1U), (c, w, h, f)); n.Scale = .5f; } }); Assert.Equal(40U, body.Construct(40)); Assert.Equal((0x50bd38U, uint.MaxValue, 0U), (n.Vtable, n.Command, n.Text)); Assert.Equal((-2f, -3f, -1f, -1.5f), (n.BaseWidth, n.BaseHeight, n.Width, n.Height)); Assert.Equal(0, n.Body[0]); Assert.Equal(0, n.Links[0]); Assert.All(n.Body.Skip(1), item => Assert.Equal(0xa5, item)); Assert.All(n.Links.Skip(1), item => Assert.Equal(0x5a, item)); body.Clear(40);
    }
    /// <summary>기존 출력 유무와 무관하게 첫 NUL부터 쓰고 필요할 때만 같은 배열을 Bind한다.</summary>
    [Fact]
    public void ParseBindsOnlyMissingOutputAndAlwaysClearsFirstBytesBeforeEffects()
    {
        var n = new HtmlBodyGumpNode(); var body = new HtmlBodyGump(new(), new() { Node = _ => n }); int bound = 0, ran = 0; var hooks = new HtmlBodyParserHooks { HasOutput = () => { Assert.Equal((byte)0, n.Body[0]); Assert.Equal((byte)0, n.Links[0]); return false; }, Bind = (text, links) => { bound++; Assert.True(text.Equals(n.Body.AsMemory())); Assert.True(links.Equals(n.Links.AsMemory())); text.Span[1] = 17; links.Span[1] = 19; }, Run = () => ran++ }; n.Body[0] = n.Links[0] = 99; body.Parse(40, hooks); hooks.HasOutput = () => true; hooks.Bind = (_, _) => throw new InvalidOperationException("외부 출력 덮어쓰면 안 됨"); n.Body[0] = n.Links[0] = 99; body.Parse(40, hooks); Assert.Equal((1, 2, (byte)17, (byte)19), (bound, ran, n.Body[1], n.Links[1]));
    }
    /// <summary>새 생성 실패 전까지 이전 토큰을 유지하고 각 효과 뒤 현재 자식을 다시 읽는다.</summary>
    [Fact]
    public void FailedSetTextKeepsOldTokenAndLaterEffectsUseCurrentChild()
    {
        var n = new HtmlBodyGumpNode { Text = 3, Flags = 8 }; var trace = new List<string>(); var h = new HtmlBodyGumpHooks { Node = _ => n, Kill = token => { Assert.Equal(3U, token); trace.Add("kill"); }, Allocate = bytes => { Assert.Equal(0xd8, bytes); Assert.Equal(3U, n.Text); return 2; }, ConstructText = (_, _, _, _, _) => throw new InvalidOperationException("명시 자식 생성 실패") }; var body = new HtmlBodyGump(new() { Color = 7 }, h); Assert.Throws<InvalidOperationException>(() => body.SetText(40, "A")); Assert.Equal(3U, n.Text); h.ConstructText = (t, w, height, font, color) => { Assert.Equal((2U, 0, 0, 5U, 7U), (t, w, height, font, color)); Assert.Equal(3U, n.Text); }; h.Attach = (t, _, _, parent, _) => { Assert.Equal((2U, 40U), (t, parent)); n.Text = 4; }; h.Styled = (t, text) => { Assert.Equal(4U, t); Assert.Equal("A", text); n.Text = 5; }; h.Scroll = (t, value) => { Assert.Equal((5U, 0), (t, value)); n.Text = 6; }; h.Slider = (t, value) => { Assert.Equal((6U, 1U), (t, value)); n.Text = 7; }; h.Scrollable = (t, value) => Assert.Equal((7U, 1U), (t, value)); body.SetText(40, "A\0ignored"); Assert.Equal(7U, n.Text); Assert.Equal(new[] { "kill", "kill" }, trace); h.Allocate = _ => 0; h.Kill = _ => { }; Assert.Throws<OutOfMemoryException>(() => body.SetText(40, "A")); Assert.Equal(7U, n.Text);
    }
    /// <summary>Push가 바꾼 현재 자식/치수와 float32 저장 전 x87 계산값을 구별한다.</summary>
    [Fact]
    public void FitReadsCurrentChildAfterPushAndPreservesDoubleIntermediate()
    {
        var n = new HtmlBodyGumpNode { Text = 2, BaseWidth = 5, BaseHeight = 6 }; var old = new GumpCreateNode { Width = 123 }; var child = new GumpCreateNode { Scale = BitConverter.UInt32BitsToSingle(0x3f800001) }; var h = new HtmlBodyGumpHooks { Node = _ => n, Base = t => t == 2 ? old : child, Push = t => { Assert.Equal(2U, t); n.Text = 3; n.BaseWidth = 16777216; n.BaseHeight = 7.25f; }, Attach = (t, x, y, parent, mode) => Assert.Equal((3U, -1f, 0f, 40U, 0U), (t, x, y, parent, mode)) }; var body = new HtmlBodyGump(new(), h); body.Fit(40, -1); Assert.Equal(16777216f, child.BaseWidth); Assert.Equal(16777220f, child.Width); Assert.Equal(7.25f, child.BaseHeight); Assert.Equal(123f, old.Width); n.Text = 0; body.Fit(40, 1); bool attached = false; h.BaseAttach = (_, _, _, _, _) => attached = true; body.Attach(40, 2, 3, 1, 0); Assert.True(attached);
    }
    /// <summary>같은 실제8192BYTE/스크롤 자식/보호 글꼴·가상 부착/입력/지연 수명을 연결한다.</summary>
    [Fact]
    public void ActualParentScrollingChildOwnedBuffersLinkInputPixelsAndLifetime()
    {
        var s = new HtmlBodyWidgetScene(); s.SetHtml("<a href=\"#Topic\">A</a>A"); Assert.Equal(30U, s.Node.Text); Assert.Equal((40U, 5U), (s.TextNode.Parent, s.TextNode.TextContext[1])); Assert.Equal("Topic", CText(s.Node.Links)); s.Common.TreeHooks.Attach!(40, 2, 3, 1, 0); Assert.Equal((32f, 64f, 30U), (s.TextNode.BaseWidth, s.TextNode.BaseHeight, s.Node.Text)); s.Common.Draw.Draw(30); var position = s.Common.Collection.Position(30); var point = new Netstorm.Client.Display.ScreenPoint(position.X + 1, position.Y + 5); uint hit = s.Text.HitTest(30, point); Assert.NotEqual(0U, hit); Assert.Equal(s.Links.Base, s.Fragments.Fragments[hit].Metadata); s.Common.ParentHandle = (_, _, _) => 1; Assert.Equal(1U, s.Widgets.Handle(30, InputCode.LeftButton, (uint)point.X, (uint)point.Y)); Assert.Contains(s.Common.Notices, item => item == (0x12503U, s.TextNode.Serial, s.Links.Base)); Assert.Contains((byte)77, s.Common.Pixels.ToArray()); Assert.Equal(7, s.Common.Guarded[0]); Assert.Equal(7, s.Common.Guarded[^1]); s.Body.Clear(40); Assert.Equal(0U, s.Node.Text); s.Common.Lifetime.Cleanup(); Assert.Equal(new uint[] { 500 }, s.FreedBuffers); Assert.Empty(s.Fragments.Fragments); Assert.Equal(2U, s.Common.Create.Count); s.Common.Lifetime.Kill(40); s.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Common.Create.Count); Assert.False(s.Common.Nodes.ContainsKey(40));
    }
    /// <summary>미션 우선/빈 섹션 성공·매번 새 파일/캐시 미사용과 외부 설정 층 생존을 확인한다.</summary>
    [Fact]
    public void ActualConfigPriorityFreshFallbackAndExistingScopesRemainIntact()
    {
        var config = new ConfigStore(); config.Push(new ConfigText("missionSpec=d/{local.1}.english\n")); config.Push(new ConfigText("1=outer\n"), "param"); int reads = 0, made = 0; string version = "First", installed = ""; bool missing = false; var hooks = new EmbeddedTellHooks { Construct = () => { Assert.Equal(2, config.LayerCount); Assert.Equal("outer", config.Expand("{param.1}")); return (uint)++made; }, SetHtml = (_, text) => installed = text, Serial = t => t + 700, Missing = key => { Assert.Equal("file.part", key); missing = true; } };
        // 같은 현재 경로에서 매번 바뀐 실제 파일 입력을 공급한다.
        byte[]? Read(string path) { Assert.Equal("d/tell.english", path); reads++; return Encoding.ASCII.GetBytes("[PlacingIslands]\n" + version + "\n[BLANK]\n[END]\n"); }
        Assert.Equal(701U, EmbeddedTell.ShowEmbeddedTellInConfig("PlacingIslands", config, Read, null, hooks)); Assert.Equal("First\n", installed); version = "Second"; Assert.Equal(702U, EmbeddedTell.ShowEmbeddedTellInConfig("PlacingIslands", config, Read, null, hooks)); Assert.Equal("Second\n", installed); Assert.Equal(703U, EmbeddedTell.ShowEmbeddedTellInConfig("BLANK", config, Read, null, hooks)); Assert.Equal("", installed); Assert.Equal(704U, EmbeddedTell.ShowEmbeddedTellInConfig("file.part", config, Read, _ => "Mission", hooks)); Assert.Equal("Mission", installed); Assert.Equal(3, reads); Assert.Equal(0U, EmbeddedTell.ShowEmbeddedTellInConfig("file.part", config, Read, null, hooks)); Assert.True(missing); Assert.Equal((4, 4, 2), (reads, made, config.LayerCount));
    }
    /// <summary>원본 좁은 키의CP1252/Config UTF-8 본문 BYTE 경계를 현재 문자열 표현에서 보존한다.</summary>
    [Fact]
    public void ConfigBoundaryDecodesNarrowKeyAndPreservesUtf8BodyBytes()
    {
        var config = new ConfigStore(); config.Push(new ConfigText("missionSpec=d/{local.1}.english\n")); string installed = ""; var hooks = new EmbeddedTellHooks { Construct = () => 1, SetHtml = (_, text) => installed = text, Serial = _ => 7 }; byte[] text = [0xef, 0xbb, 0xbf, .. Encoding.UTF8.GetBytes("[€]\né\n[END]\n")]; Assert.Equal(7U, EmbeddedTell.ShowEmbeddedTellInConfig("\x80", config, _ => text, null, hooks)); Assert.Equal(new byte[] { 0xc3, 0xa9, 10 }, Encoding.Latin1.GetBytes(installed)); Assert.Equal(7U, EmbeddedTell.ShowEmbeddedTellInConfig("\x80", config, _ => throw new InvalidOperationException("미션 성공 뒤 파일 조회 금지"), key => { Assert.Equal("€", key); return "é"; }, hooks)); Assert.Equal(new byte[] { 0xc3, 0xa9 }, Encoding.Latin1.GetBytes(installed));
    }
    /// <summary>효과 진입 전 현재 본문과 오류 앞 접두부/링크·전역은 실제 부모 영역에 남는다.</summary>
    [Fact]
    public void ActualParserPublishesCurrentPrefixAndKeepsPartialStateOnFailure()
    {
        var s = new HtmlBodyWidgetScene(); s.Effects.Command = (state, raw) => { Assert.Equal("stop>", raw); Assert.Equal("prefix ", CText(s.Node.Body)); s.Node.Links[0] = (byte)'Z'; s.Node.Links[1] = 0; s.Globals.SuppressParagraph = 9; state.Output += "Q"; throw new InvalidOperationException("명시 원문 명령 중단"); }; Assert.Throws<InvalidOperationException>(() => s.SetHtml("prefix <$stop>")); Assert.Equal("prefix Q", CText(s.Node.Body)); Assert.Equal("Z", CText(s.Node.Links)); Assert.Equal(9U, s.Globals.SuppressParagraph); Assert.Equal(0U, s.Node.Text); Assert.Equal(2U, s.Common.Create.Count); s.Effects.Command = null; s.SetHtml("<a href=\"#After\">A</a>"); Assert.Equal("After", CText(s.Node.Links)); Assert.Equal("After", HtmlLinkText.ResolveHtmlLink(s.Links, s.Links.Base)); Assert.Equal(30U, s.Node.Text); s.Common.Lifetime.Kill(40); s.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Common.Create.Count);
    }
}
