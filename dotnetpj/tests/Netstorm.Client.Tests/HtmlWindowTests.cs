using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // 실제 일반 HTML 창과 공통 UI의 소유/현재 필드/입력/배치를 확인한다.

/// <summary>독립 원본 대조 외 실제 위젯 연결과 원문 유효 범위/부분 효과를 검증한다.</summary>
public sealed class HtmlWindowTests
{
    /// <summary>장식 기반/20개 돌 버튼과 배경을 생성하고 실제 지연 삭제 뒤 같은 창에서 재생성한다.</summary>
    [Fact]
    public void ActualDecorationTwentyStoneButtonsDeferredClearAndConcurrentWindows()
    {
        var s = new HtmlWindowWidgetScene(); uint before = s.Common.Create.Count, first = s.Create(), second = s.Create(uint.MaxValue, 0x80000000); var root = s.Root(first); Assert.Equal((0x515e7cU, 27U, 9U, 2U, 0U, 0U), (root.Vtable, root.Command, root.Flags, s.State.Instances, root.Body, root.ControlCount)); Assert.NotEqual(0U, root.CornerType); Assert.NotEqual(0U, root.Type); Assert.Equal((uint.MaxValue, 0x80000000U), (s.Root(second).RequestedWidth, s.Root(second).RequestedHeight));
        // 같은20행/실제 돌 버튼/실제 배경과 Instant 비트를 확인한다.
        for (uint i = 0; i < 20; i++) { s.Window.AddButton(first, "Action", unchecked(0xfedcba98 + i), unchecked(0x87654321 + i), i & 1); var e = root.Controls[i]; var b = Assert.IsType<StoneButtonGumpNode>(s.Common.Nodes[e.Child]); Assert.Equal(first, b.Parent); Assert.Equal(b.Background, b.Child); Assert.Equal(i & 1, (b.ButtonFlags & 4) >> 2); }
        Assert.Equal(before + 42, s.Common.Create.Count); s.Window.AddButton(first, "Ignored", 1, 2, 1); Assert.Equal(20U, root.ControlCount); s.Window.ClearButtons(first); Assert.Equal(before + 42, s.Common.Create.Count); Assert.All(root.Controls, e => Assert.Equal((0U, 0U, 0U), (e.Child, e.Command, e.Parameter))); s.Common.Lifetime.Cleanup(); Assert.Equal(before + 2, s.Common.Create.Count); s.Window.AddButton(first, "Again", uint.MaxValue, 0xabcdef01, 1); Assert.Equal(1U, root.ControlCount); s.Common.Lifetime.Kill(first); s.Common.Lifetime.Cleanup(); Assert.Equal((1U, before + 1, 123.25), (s.State.Instances, s.Common.Create.Count, s.State.LastCloseTime)); Assert.True(s.Common.Nodes.ContainsKey(second)); s.Common.Lifetime.Kill(second); s.Common.Lifetime.Cleanup(); Assert.Equal(before, s.Common.Create.Count); Assert.Equal(0U, s.State.Instances);
    }
    /// <summary>실제 소유 버퍼/Slider/보호 글꼴·전체 Fit/픽셀과 이전 본문 반납을 연결한다.</summary>
    [Fact]
    public void ActualDynamicBodyReplacementFullFitAndProtectedFontPixels()
    {
        var s = new HtmlWindowWidgetScene(); uint root = s.Create(100); s.Window.AddButton(root, "OK", 1, 2, 0); s.Window.SetBody(root, "~rFirst body"); var first = s.Body(root); uint firstToken = s.Root(root).Body, firstBuffer = first.Buffer; Assert.Equal((root, uint.MaxValue, 1U), (first.Parent, first.SecondaryDepth, first.OwnsBuffer)); Assert.NotEqual(0U, first.Slider); Assert.Equal(firstToken, s.Common.Nodes[first.Slider].Parent); Assert.Equal(0U, first.TextFlags & 4); s.Attach(root); Assert.Equal((1U, 0xffffffb0U), (s.Root(root).Parent, s.Root(root).Depth)); Assert.True(s.Root(root).BaseWidth >= 100); Assert.True(s.Root(root).BaseHeight >= 65); Assert.Equal(20f, first.Y);
        s.Window.SetBody(root, "~rSecond\nThird\n"); var second = s.Body(root); Assert.NotSame(first, second); Assert.NotEqual(firstBuffer, second.Buffer); Assert.NotEqual(0U, first.Flags & 0x10000); s.Common.Lifetime.Cleanup(); Assert.False(s.Common.Nodes.ContainsKey(firstToken)); Assert.Contains(firstBuffer, s.FreedBuffers); Assert.True(s.Common.Nodes.ContainsKey(s.Root(root).Body)); Assert.Equal(2, s.Buffers.Binding(second.Buffer).State.LineCount);
        s.Globals[0x5ca9d4] = 64; s.Globals[0x5ca9d8] = 128; s.Common.Tree.Move(root, 0, 0, 1); s.Common.Draw.Draw(s.Root(root).Body); Assert.Contains((byte)77, s.Common.Pixels.ToArray()); Assert.Equal((byte)7, s.Common.Guarded[0]); Assert.Equal((byte)7, s.Common.Guarded[^1]); s.Common.Lifetime.Kill(root); s.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Common.Create.Count); Assert.Empty(s.Fragments.Fragments); Assert.Equal(0U, s.State.Instances);
    }
    /// <summary>실제 버튼 통지/전체DWORD 큐→Kill→count감소→OnExit와 현재 보조창 삭제 순서.</summary>
    [Fact]
    public void ActualButtonBubbleQueueOnExitAndCurrentAuxiliaryLifetime()
    {
        var s = new HtmlWindowWidgetScene(); uint root = s.Create(), auxiliary = s.Create(); s.Window.AddButton(root, "Select", uint.MaxValue, 0x87654321, 1); var n = s.Root(root); n.OnExitCommand = 0x01001234; n.OnExitParameter = 0xabcdef01; uint button = n.Controls[0].Child; Assert.Equal(1U, s.Common.Events.Bubble(button, s.State.ClickEvent | InputCode.Release, s.Common.Nodes[button].Serial, 0)); var selected = s.Queue.Pop(false); Assert.Equal((uint.MaxValue, unchecked((int)0x87654321), unchecked((int)0x87654321)), (selected.Code, selected.X, selected.Y)); Assert.Equal(2U, s.State.Instances); Assert.NotEqual(0U, n.Flags & 0x10000); s.OnQueue = () => { Assert.Equal((0x01001234U, 0xabcdef01U), (n.OnExitCommand, n.OnExitParameter)); n.AuxiliarySerial = s.Root(auxiliary).Serial; }; s.Common.Lifetime.Cleanup(); var exit = s.Queue.Pop(false); Assert.Equal((0x01001234U, unchecked((int)0xabcdef01), unchecked((int)0xabcdef01)), (exit.Code, exit.X, exit.Y)); Assert.Equal(new uint[] { 2, 1 }, s.QueueCounts); Assert.Equal(0U, s.State.Instances); Assert.Equal(1U, s.Common.Create.Count); Assert.False(s.Common.Nodes.ContainsKey(auxiliary)); Assert.True(s.Queue.Empty);
    }
    /// <summary>프레임 Idle이 시계 효과 뒤 현재 기한/명령을 읽으며0명령도 큐에 쓴다.</summary>
    [Fact]
    public void ActualIdleCurrentDeadlineAndZeroCommandThroughCommonFrame()
    {
        var s = new HtmlWindowWidgetScene(); uint root = s.Create(); var n = s.Root(root); n.TimeoutSeconds = 1000; n.TimeoutCommand = 1; n.TimeoutParameter = 2; s.Common.Lifetime.SetIdle(root, true); var frame = new GumpFrame(s.Common.Create, s.Common.LifeState, s.Common.FrameHooks); s.OnClock = () => { n.TimeoutSeconds = 123.25; n.TimeoutCommand = 0; n.TimeoutParameter = uint.MaxValue; }; frame.Idle(); var item = s.Queue.Pop(false); Assert.Equal((0U, -1, -1), (item.Code, item.X, item.Y)); Assert.NotEqual(0U, n.Flags & 0x10000); s.OnClock = null; s.Common.Lifetime.Cleanup(); Assert.DoesNotContain(root, s.Common.LifeState.Idle); Assert.Equal(0U, s.State.Instances);
    }
    /// <summary>이미 측정된 실제 두 메뉴를 controlled 스택에서 꺼내 부착하며 첫 메뉴 높이와 최대 높이를 구별한다.</summary>
    [Fact]
    public void ActualTwoColumnMenuStackAttachPopAndDifferentHeights()
    {
        var s = new HtmlWindowWidgetScene(); uint root = s.Create(100); s.Window.SetBody(root, "Body"); MenuGumpNode left = new(), right = new(); s.Common.Nodes.Add(800, left); s.Common.Nodes.Add(801, right); s.Common.Factory.Construct(800, 0, 70, 22, 1); s.Common.Factory.Construct(801, 0, 70, 55, 1); var state = new MenuGumpState { Index = 2 }; state.Entries[1] = 801; state.Entries[2] = 800; var menu = new MenuGump(s.Common.Create, state, s.Common.Lifetime, new() { Node = t => s.Common.Nodes.GetValueOrDefault(t) as MenuGumpNode, BaseNode = t => s.Common.Nodes.GetValueOrDefault(t), Attach = s.Common.Tree.Attach }); HtmlWindowFit.BindHtmlWindowFitMenus(s.FitHooks, menu); s.Root(root).MenuColumns = 2; s.Attach(root); Assert.Equal((0U, 0U), (state.Index, menu.Current())); Assert.Equal((root, root, 0U), (left.Parent, right.Parent, s.Root(root).MenuColumns)); Assert.Equal(left.Y, right.Y); Assert.True(right.X > left.X); Assert.True(s.Root(root).BaseWidth >= 180); Assert.True(s.Root(root).BaseHeight >= s.Body(root).BaseHeight + 65 + 55 + 10); s.Common.Lifetime.Kill(root); s.Common.Lifetime.Cleanup(); Assert.Equal(1U, s.Common.Create.Count);
    }
    /// <summary>입력/태그는 현재 링크 주소를 Kill 뒤 반납 전에 소비하며 count와 무관한20행을 찾는다.</summary>
    [Fact]
    public void ActualTagLinkReadsDeadWindowBeforeDeferredReleaseAndPhysicalRowSearch()
    {
        var s = new HtmlWindowWidgetScene(); uint root = s.Create(); var n = s.Root(root); n.Html[17] = 91; s.Window.AddButton(root, "Select", 0x87654321, 0xfedcba98, 0); var first = n.Controls[0]; n.Controls[19].Child = first.Child; n.Controls[19].Command = first.Command; n.Controls[19].Parameter = first.Parameter; first.Child = 0; n.ControlCount = 0; Assert.Equal(1U, s.Common.InputHooks.Handle!(root, s.State.ClickEvent, s.Common.Nodes[n.Controls[19].Child].Serial, 0)); Assert.Equal(0x87654321U, s.Queue.Pop(false).Code); s.Common.Lifetime.Cleanup();
        root = s.Create(); n = s.Root(root); n.Html[17] = 91; Assert.Equal(1U, s.Common.EventHooks.Handle!(root, s.State.TagEvent, 0, 0x63000011)); Assert.NotEqual(0U, n.Flags & 0x10000); Assert.Equal((byte)91, n.Html[17]); Assert.Equal(0x63000011U, Assert.Single(s.Links)); Assert.True(s.Common.Nodes.ContainsKey(root)); s.Common.Lifetime.Cleanup(); Assert.False(s.Common.Nodes.ContainsKey(root));
    }
    /// <summary>원문 LF 지역 버퍼 한도/할당 실패와 생성 미작성 인자를 보존한다.</summary>
    [Fact]
    public void ExplicitBoundsPreserveReachedPrefixAndUnwrittenConstructorArguments()
    {
        var n = new HtmlWindowGumpNode { TimeoutParameter = 0xabcdef01, OnExitParameter = 0x98765432, Body = 2 }; Array.Fill(n.Html, (byte)0xa5); var child = new GumpCreateNode(); int killed = 0; var h = new HtmlWindowGumpHooks { Node = _ => n, Base = t => t == 1 ? n : child, ConstructDecoration = _ => { }, Kill = _ => killed++, Allocate = _ => 0 }; var state = new HtmlWindowGumpState { Kind = -1 }; var body = new HtmlWindowGump(state, h); body.Construct(1, uint.MaxValue, 0x80000000); Assert.Equal((0xabcdef01U, 0x98765432U, uint.MaxValue), (n.TimeoutParameter, n.OnExitParameter, n.Command)); Assert.Equal(0, n.Html[0]); Assert.All(n.Html.Skip(1), b => Assert.Equal(0xa5, b)); n.Body = 2; Assert.Throws<ArgumentException>(() => body.SetBody(1, new('A', 1007))); Assert.Equal((2U, 0), (n.Body, killed)); Assert.Throws<InvalidOperationException>(() => body.SetBody(1, "A")); Assert.Equal((0U, 1), (n.Body, killed)); n.ControlCount = uint.MaxValue; Assert.Throws<IndexOutOfRangeException>(() => body.AddButton(1, "A", 1, 2, 0));
    }
}
