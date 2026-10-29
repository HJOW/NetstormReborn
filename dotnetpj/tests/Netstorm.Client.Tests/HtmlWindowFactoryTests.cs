using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;

namespace Netstorm.Tests; // 실제 일반 HTML 생성/메타의 전체 하위 UI와 현재 상태/부분 오류를 검사한다.

/// <summary>실제 Config/파서/돌버튼/본문/메뉴/모달/고리/원시큐와 현재 재진입/반납을 확인한다.</summary>
public sealed class HtmlWindowFactoryTests
{
    /// <summary>현재 치환/기본Okay·전체 본문/Fit/모달입력/큐/자식 우선 반납을 확인한다.</summary>
    [Fact]
    public void ActualConfigFallbackBodyModalInputQueueAndCleanup()
    {
        var s = new HtmlWindowFactoryScene(); uint before = s.Ui.Common.Create.Count, root = s.Show("{caption}<br>Choose a route."); var n = s.Ui.Root(root); Assert.NotEqual(0U, root); Assert.Equal(n.Serial, s.Ui.State.CreatedSerial); Assert.Equal((1, 1U, 1U, root, 1U), (s.FallbackCalls, s.Ui.State.Instances, s.Ui.Common.LifeState.DialogCount, s.Ui.Common.LifeState.CurrentDialog, n.ControlCount)); Assert.Contains("Storm Islands", s.BodyText(root)); Assert.Equal((0x0200006dU, 0U), (n.Controls[0].Command, n.Controls[0].Parameter)); s.Ui.Common.Input.Route(s.Ui.State.EnterEvent, 0, 0); var selected = s.Ui.Queue.Pop(false); Assert.Equal((0x0200006dU, 0, 0), (selected.Code, selected.X, selected.Y)); Assert.Equal(0U, s.Ui.Common.LifeState.DialogCount); Assert.Equal(1U, s.Ui.State.Instances); s.Ui.Common.Lifetime.Cleanup(); Assert.Equal((0U, before), (s.Ui.State.Instances, s.Ui.Common.Create.Count)); Assert.Empty(s.Ui.Fragments.Fragments);
    }
    /// <summary>메타 종류 우선순위/원문 공백과 처리수24 대 실제 버튼20개 한도를 검증한다.</summary>
    [Fact]
    public void ActualMetadataPriorityRawCommandAndTwentyControlLimit()
    {
        var s = new HtmlWindowFactoryScene(); uint root = s.Ui.Create(); string source = "$OnExit=unused,Warning,9 \n$ButtonInstant=Fast,Config,2 \n";
        // 일반 Button24개가 앞 종류로 먼저20칸을 소비한다.
        for (int i = 0; i < 24; i++) source += $"$Button=Action{i},Config,{i + 10} \n"; source += "$Button=Raw, Config,7 \n"; Assert.Equal(1U, s.Metadata.Process(root, HtmlWindowFactoryScene.Buffer(source))); var n = s.Ui.Root(root); Assert.Equal(20U, n.ControlCount);
        // 실제20개 돌 버튼의 현재 값과 Instant 미적용을 확인한다.
        for (uint i = 0; i < 20; i++) { var row = n.Controls[i]; Assert.Equal((0x0200006dU, i + 10), (row.Command, row.Parameter)); Assert.Equal(0U, ((StoneButtonGumpNode)s.Ui.Common.Nodes[row.Child]).ButtonFlags & 4); }
        Assert.Contains(" Config", s.Lookups); Assert.Empty(s.Diagnostics); Assert.Equal(0, s.Strings.Index); Assert.Equal((0x02000102U, 9U), (n.OnExitCommand, n.OnExitParameter)); s.Close(); Assert.Equal(0x02000102U, s.Ui.Queue.Pop(false).Code); Assert.Equal(1U, s.Ui.Common.Create.Count);
    }
    /// <summary>실제 일반/체크 메뉴를 작성하고 현재 enabled 전역 단락 평가·고리·전체Fit 스택반납을 확인한다.</summary>
    [Theory]
    [InlineData(0U, 0U, 0, false)]
    [InlineData(1U, 0U, 0, true)]
    [InlineData(0U, 1U, 0, true)]
    [InlineData(0U, 0U, 1, true)]
    public void ActualMenuCheckedGlobalsAndFit(uint force, uint editor, int explicitEnable, bool enabled)
    {
        var s = new HtmlWindowFactoryScene(); s.Ui.Globals[0x5cab30] = force; s.Ui.Globals[0x55a480] = editor; var reads = new List<uint>(); var read = s.MetaHooks.Read!; s.MetaHooks.Read = a => { reads.Add(a); return read(a); }; uint root = s.Show($"Choose<br>$Checked=Remember,Warning,8,1,{explicitEnable} \n$Menu=First,Config,7 \n"); var n = s.Ui.Root(root); Assert.Equal((0U, 0U, 1U, 0), (n.MenuColumns, s.Ui.Menu.Current(), s.Ui.MenuState.Instances, s.FallbackCalls)); var menu = Assert.Single(s.Ui.Common.Nodes.Values.OfType<MenuGumpNode>()); Assert.Equal((2U, root), (menu.ItemCount, menu.Parent)); Assert.Equal((0x0200006dU, 7U), (menu.Items[0].Command, menu.Items[0].Parameter)); Assert.Equal(4U | (enabled ? 0U : 0x200U), menu.Items[1].Flags & 0x204); Assert.Equal(explicitEnable != 0 ? Array.Empty<uint>() : force != 0 ? [0x5cab30U] : [0x5cab30U, 0x55a480U], reads); s.Close(); Assert.Equal((0U, 1U), (s.Ui.MenuState.Instances, s.Ui.Common.Create.Count)); Assert.Empty(s.Ui.Fragments.Fragments);
    }
    /// <summary>실제 기한/공통Idle→원시큐→Kill→OnExit와80칸 고리의 선해석 수명을 확인한다.</summary>
    [Fact]
    public void ActualTimeoutOnExitAndStringRingResolveBeforeDisposal()
    {
        var s = new HtmlWindowFactoryScene(); uint root = s.Show("Wait\n$Timeout=3,Config,12 \n$OnExit=unused,Tell,Blank \n"); var n = s.Ui.Root(root); uint argument = n.OnExitParameter; Assert.Equal("Blank", s.Strings.Resolve(argument)); Assert.Equal(1, s.Strings.Index); Assert.Equal(103.5, n.TimeoutSeconds); var frame = new GumpFrame(s.Ui.Common.Create, s.Ui.Common.LifeState, s.Ui.Common.FrameHooks); frame.Idle(); Assert.True(s.Ui.Queue.Empty); s.Ui.Now = 103.5; frame.Idle(); Assert.Equal(0x0200006dU, s.Ui.Queue.Pop(false).Code); s.Ui.Common.Lifetime.Cleanup(); var exit = s.Ui.Queue.Pop(false); Assert.Equal((0x02000095U, (int)argument, (int)argument), (exit.Code, exit.X, exit.Y)); Assert.Equal("Blank", s.Strings.Resolve(argument)); Assert.Equal(1U, s.Ui.Common.Create.Count);
    }
    /// <summary>첫 링크 직후 명령에서 같은 실제 부모 바이트를 읽고 두 번째 변환/죽음표시/반납까지 확인한다.</summary>
    [Fact]
    public void ActualImmediateLinkBytesTwoPassAndLifetime()
    {
        var s = new HtmlWindowFactoryScene(); uint address = 0; int probes = 0; s.Effects.Command = (parser, _) => { probes++; int start = parser.Output.LastIndexOf("~[T", StringComparison.Ordinal) + 3; int end = parser.Output.IndexOf(']', start); address = uint.Parse(parser.Output[start..end]); Assert.Equal("route", s.Parser.Resolve(address)); }; uint root = s.Show("<a href=\"#route\"><$probe>Travel</a><br>"); Assert.NotEqual(0U, root); Assert.Equal(1, probes); Assert.Equal("route", s.Parser.Resolve(address)); Assert.Equal("route", Encoding.Latin1.GetString(s.Ui.Root(root).Html, 0, 5)); Assert.Contains("Travel", s.BodyText(root)); Assert.Equal(1U, s.Ui.Common.EventHooks.Handle!(root, s.Ui.State.TagEvent, 0, address)); Assert.Equal(address, Assert.Single(s.Ui.Links)); Assert.Equal("route", s.Parser.Resolve(address)); s.Ui.Common.Lifetime.Cleanup(); Assert.Throws<ArgumentOutOfRangeException>(() => s.Parser.Resolve(address)); Assert.Equal(1U, s.Ui.Common.Create.Count);
    }
    /// <summary>첫 파서의 닫히지 않은 조건 상태를 두 번째 변환까지 공유하며 배경 전역은 창 수명과 독립적이다.</summary>
    [Fact]
    public void ActualHiddenStateAndBackgroundAcrossBothPassesAndSuppression()
    {
        var s = new HtmlWindowFactoryScene(); uint root = s.Show("<background=\"nsstart\">visible<?0>hidden"); Assert.NotEqual(0U, root); Assert.Equal("", s.BodyText(root)); Assert.Equal("nsstart", HtmlTextParser.HtmlBackgroundName(s.Globals)); s.Close(); Assert.Equal(0U, s.Show("<background=\"b\">!?")); Assert.Equal("b", HtmlTextParser.HtmlBackgroundName(s.Globals)); Assert.Equal((byte)'s', s.Globals.Background[2]); s.Ui.Common.Lifetime.Cleanup(); uint before = s.Ui.Common.Create.Count; Assert.Equal(0U, s.Show("<background=\"nsisles\">No window", 1)); Assert.Equal("nsisles", HtmlTextParser.HtmlBackgroundName(s.Globals)); Assert.Equal(before, s.Ui.Common.Create.Count); Assert.Equal(0U, s.Ui.State.CreatedSerial);
    }
    /// <summary>빈 출력은xxx 본문을 실제 부착/Kill하고 창 생략은 첫 파서 효과만 실행한다.</summary>
    [Theory]
    [InlineData(0U)]
    [InlineData(1U)]
    public void ActualEmptyDummyAndSuppressedCommandEffects(uint suppress)
    {
        var s = new HtmlWindowFactoryScene(); uint root = 0; int probes = 0; s.Effects.Command = (_, _) => { probes++; root = s.Ui.Reference(s.Ui.State.CreatedSerial); }; Assert.Equal(0U, s.Show("<$probe>!?", suppress)); Assert.Equal((1, 0, 0U), (probes, s.FallbackCalls, s.Ui.State.CreatedSerial)); if (suppress != 0) Assert.Equal(0U, root); else { Assert.NotEqual(0U, root); Assert.NotEqual(0U, s.Ui.Root(root).Flags & 0x10000); Assert.Equal("xxx", s.BodyText(root)); s.Ui.Common.Lifetime.Cleanup(); } Assert.Equal((0U, 1U), (s.Ui.State.Instances, s.Ui.Common.Create.Count));
    }
    /// <summary>실제 중첩 모달은 이전 표시 버튼까지 내려간 최소 깊이보다3만큼 앞서며 닫으면 이전 입력을 복구한다.</summary>
    [Fact]
    public void ActualNestedModalLowestChildDepthAndInputRestoration()
    {
        var s = new HtmlWindowFactoryScene(); uint first = s.Show("First window"), firstDepth = s.Ui.Root(first).Depth, buttonDepth = s.Ui.Common.Nodes[s.Ui.Root(first).Controls[0].Child].Depth, second = s.Show("Second window"); Assert.Equal((2U, second, firstDepth, unchecked(buttonDepth - 3)), (s.Ui.Common.LifeState.DialogCount, s.Ui.Common.LifeState.CurrentDialog, s.Ui.Root(first).Depth, s.Ui.Root(second).Depth)); s.Modal.SetModal(second, true); Assert.Equal(2U, s.Ui.Common.LifeState.DialogCount); s.Ui.Common.Input.Route(s.Ui.State.EnterEvent, 0, 0); Assert.Equal(first, s.Ui.Common.LifeState.CurrentDialog); Assert.NotEqual(0U, s.Ui.Root(second).Flags & 0x10000); s.Ui.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Ui.State.Instances); s.Ui.Common.Input.Route(s.Ui.State.EnterEvent, 0, 0); s.Ui.Common.Lifetime.Cleanup(); Assert.Equal(0U, s.Ui.State.Instances); Assert.Equal(1U, s.Ui.Common.Create.Count);
    }
    /// <summary>고리80칸 덮어쓰기와 단일반납 재진입·누락장치/캡처 범위의 기존 상태 보존을 확인한다.</summary>
    [Fact]
    public void RingWrapSingleDisposalReentrancyAndCapturedBounds()
    {
        var strings = new ScriptStringRing(); uint first = strings.Store("Same\0ignored"), second = strings.Store("Same"); Assert.NotEqual(first, second); Assert.Equal(("Same", "Same", 2), (strings.Resolve(first), strings.Resolve(second), strings.Index));
        // 첫 토큰까지 덮어쓰되 두 번째 토큰의 기존 수명은 남긴다.
        for (int i = 0; i < 79; i++) strings.Store("Next"); Assert.Throws<ArgumentOutOfRangeException>(() => strings.Resolve(first)); Assert.Equal("Same", strings.Resolve(second)); var state = new DialogExitCommandState(); var hooks = DialogExitCommand.MakeHooks(strings, new() { Lookup = _ => 0x02000095 }); DialogExitCommand.Resolve(state, "Tell", "Blank", hooks); int calls = 0; DialogExitCommand.Dispose(state, (c, a, b) => { calls++; Assert.Equal((0U, 0U), (state.Command, state.Parameter)); DialogExitCommand.Dispose(state, (Action<uint, uint, uint>?)null); Assert.Equal("Blank", strings.Resolve(a)); Assert.Equal(a, b); }); Assert.Equal(1, calls); DialogExitCommand.Resolve(state, "Tell", "7", hooks); Assert.Throws<InvalidOperationException>(() => DialogExitCommand.Dispose(state, (Action<uint, uint, uint>?)null)); Assert.Equal((0x02000095U, 7U), (state.Command, state.Parameter)); Assert.Throws<ArgumentException>(() => DialogExitCommand.Resolve(state, new('C', 80), "0", hooks)); Assert.Throws<ArgumentException>(() => DialogExitCommand.Resolve(state, "Tell", new('A', 256), hooks)); Assert.Equal((0x02000095U, 7U), (state.Command, state.Parameter));
    }
    /// <summary>맵 목록이 같은 실제 열 필드를 ref로 바꾸고 효과 뒤 현재 버퍼를 지운다.</summary>
    [Fact]
    public void SameColumnFieldAliasAndCurrentBufferMutationAfterInput()
    {
        var s = new HtmlWindowFactoryScene(); uint root = s.Ui.Create(); var n = s.Ui.Root(root); s.MetaHooks.Maps = (string _, ref uint columns) => { Assert.Equal(1U, columns); n.MenuColumns = 2; Assert.Equal(2U, columns); columns = 3; }; Assert.Equal(1U, s.Metadata.Process(root, HtmlWindowFactoryScene.Buffer("$Menu=*sololist*,Config,0 \n"))); Assert.Equal(3U, n.MenuColumns); byte[] buffer = HtmlWindowFactoryScene.Buffer("prefix $Input=Name,15,value \nremain"); s.MetaHooks.Input = (_, _, _) => { string changed = "prefix altered$Key=K,Config,0 \n"; Encoding.Latin1.GetBytes(changed, buffer); buffer[changed.Length] = 0; s.Ui.Globals[0x5429ec] = 0xfedcba98; }; n.MenuColumns = 0; Assert.Equal(0U, s.Metadata.Process(root, buffer)); Assert.Equal(0xfedcba98U, s.Ui.Queue.Pop(false).Code); s.Close(); Assert.Equal(1U, s.Ui.Common.Create.Count);
    }
    /// <summary>긴 실제 본문이 화면 제한/Slider를 선택하고 보호 글꼴을 작은 명시 뷰에 그릴 때 guard를 보존한다.</summary>
    [Fact]
    public void ActualLongBodyScreenLimitScrollingAndIndexedPixels()
    {
        var s = new HtmlWindowFactoryScene(); s.Ui.Globals[0x5ca9b4] = 320; s.Ui.Globals[0x531860] = 320; s.Ui.Globals[0x531864] = 240; s.Ui.Globals[0x5ca9d4] = 64; s.Ui.Globals[0x5ca9d8] = 128; string source = string.Concat(Enumerable.Repeat("Storm islands and bridges require careful planning. ", 120)); uint root = s.Show(source); var body = s.Ui.Body(root); Assert.Equal(6U, body.TextFlags & 6); Assert.True(s.Ui.Root(root).BaseWidth <= 320); Assert.True(s.Ui.Root(root).BaseHeight <= 240); Assert.True(body.BaseHeight < 240); uint token = s.Ui.Root(root).Body; s.Ui.Text.RequestScroll(token, 0); s.Ui.Common.Draw.Draw(token); Assert.Contains((byte)171, s.Ui.Common.Pixels.ToArray()); Assert.Equal(((byte)7, (byte)7), (s.Ui.Common.Guarded[0], s.Ui.Common.Guarded[^1])); var slider = (ScrollGumpNode)s.Ui.Common.Nodes[body.Slider]; Assert.True(slider.Maximum - slider.Minimum > slider.Page); s.Ui.Text.RequestRelativeScroll(token, 30); s.Ui.Common.Draw.Draw(token); Assert.True(body.Scroll > 0); s.Close(); Assert.Equal(1U, s.Ui.Common.Create.Count); Assert.Empty(s.Ui.Fragments.Fragments);
    }
    /// <summary>선택 장치 누락/8192BYTE 한도 실패에서도 이미 생성한 부모와 현재 표식을 실제 수명으로 정리한다.</summary>
    [Fact]
    public void MissingServicesAndCapacityFailuresKeepReachedOwnerState()
    {
        var s = new HtmlWindowFactoryScene(); s.Hooks.Substitute = (_, _) => new('A', 8192); Assert.Throws<ArgumentException>(() => s.Show("Body")); Assert.Equal(1U, s.Ui.State.Instances); Assert.NotEqual(0U, s.Ui.State.CreatedSerial); s.Close(); s.Hooks.Substitute = (_, _) => "Body"; s.Hooks.Fallback = null; Assert.Throws<InvalidOperationException>(() => s.Show("Body")); Assert.Equal(1U, s.Ui.State.Instances); s.Close(); Assert.Equal(1U, s.Ui.Common.Create.Count);
    }
}
