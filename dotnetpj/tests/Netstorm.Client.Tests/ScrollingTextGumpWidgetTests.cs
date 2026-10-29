using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 같은 실제 다중 줄 생성·Slider·태그·커서·드래그·소유 수명의 고정 계약이다.

/// <summary>cpppj ScrollingTextGumpWidgets의 정적 계약과 실제 조립.독립 원본 실행 대조/L1은 아니다.</summary>
public sealed class ScrollingTextGumpWidgetTests
{
    /// <summary>두 판본의 실제128칸 버퍼/숨긴Slider/overlay/부분 필드/문맥/현재 폭과 맨 아래 요청.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0x51454cU)]
    [InlineData(OriginalEdition.Cd1072, 0x505d80U)]
    public void ActualConstructorAndAttach(OriginalEdition edition, uint vtable)
    {
        var s = new ScrollingWidgetScene(edition); s.Construct(); Assert.Equal(new[] { 0x38, 0xd8 }, s.Allocations); Assert.Equal(128, s.Buffer.State.MaxLines); Assert.Equal(1U, s.Node.OwnsBuffer); Assert.Equal((0x1234U, 91), (s.Node.Unwritten, s.Node.DragY)); Assert.Equal(vtable, s.Node.Vtable); Assert.Equal(0xfffffff9U, s.Node.Command); Assert.Equal(new uint[] { 0, 0, 171, 0, 0, 0, 1, 1 }, s.Node.TextContext); Assert.Equal(8U, s.Scene.Create.Count); Assert.Contains(30U, s.Scene.LifeState.Overlay); Assert.Equal(0x99U, s.Node.TextFlags); Assert.Equal(-2, s.Node.ScrollRequest); Assert.Equal((21f, 0f, 40f, 30U), (s.Slider.X, s.Slider.Y, s.Slider.BaseHeight, s.Slider.Parent)); Assert.Equal(0U, s.Slider.Flags & 1); Assert.Equal(30f, s.Node.LayoutWidth);
    }
    /// <summary>최하위 비트/전체 비0 설정 차이와 실제 현재 Slider 재부착,줄바꿈의 dirty 누락을 보존한다.</summary>
    [Fact]
    public void SettersUseOriginalBitsAndActualReattach()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.Widgets.SetSelectable(30, 2); Assert.Equal(0U, s.Node.Flags & 0x200); s.Widgets.SetSelectable(30, 0); Assert.Equal(0x200U, s.Node.Flags & 0x200); s.Widgets.SetSlider(30, 3); Assert.Equal(6U, s.Node.TextFlags & 6); Assert.Equal(1U, s.Slider.Flags & 1); s.Node.TextFlags &= ~1U; s.Buffer.State.Dirty = 0; s.Scene.Redraws.Clear(); s.Widgets.SetWrap(30, 0); Assert.Equal(1U, s.Buffer.State.NoWrap); Assert.Equal(0U, s.Node.TextFlags & 1); Assert.Equal(0U, s.Buffer.State.Dirty); Assert.Empty(s.Scene.Redraws); s.Widgets.SetWrap(30, -2); Assert.Equal(0U, s.Buffer.State.NoWrap); s.Widgets.SetScrollable(30, 2); Assert.Equal(0U, s.Node.TextFlags & 6); Assert.Equal(0U, s.Slider.Flags & 1);
    }
    /// <summary>실제 본문/슬라이더/표시가 연결되고 화살표가 부모의5픽셀 요청을 소비한다.</summary>
    [Fact]
    public void ActualTextPixelsAndArrowUsesFivePixelParentStep()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.AppendAndDraw("A\nA\nA\nA\n"u8); Assert.Equal(171, s.Scene.Pixels.Span[3 * 64 + 2]); Assert.Equal(171, s.Scene.Pixels.Span[17 * 64 + 2]); Assert.Equal((56, 40, 0, 0), (s.Slider.Maximum, s.Slider.Page, s.Slider.Position, s.Node.Scroll)); s.Widgets.SetSlider(30, 1); s.Scene.Slider.Handle(31, s.Scene.State.ClickEvent, s.Scene.Nodes[s.Slider.Increment].Serial, 0); Assert.Equal(5, s.Node.ScrollRequest); Assert.Equal(0, s.Slider.Position); s.Scene.Draw.Draw(30); Assert.Equal((5, 5, -1), (s.Node.Scroll, s.Slider.Position, s.Node.ScrollRequest)); Assert.Equal(new ScreenRect(0, 0, 64, 128), s.Scene.Clip.Clip);
    }
    /// <summary>현재 적용scroll에 step/page를 더하며 잘못된 change serial은 현재 경계에서 거부한다.</summary>
    [Theory]
    [InlineData(1U, 15)]
    [InlineData(2U, 5)]
    [InlineData(3U, 50)]
    [InlineData(4U, 0)]
    public void StepRequestsUseAppliedScroll(uint step, int expected)
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.Node.Scroll = 10; s.Node.ScrollRequest = 99; Assert.Equal(1U, s.Widgets.Handle(30, s.State.StepEvent, s.Slider.Serial, step)); Assert.Equal(expected, s.Node.ScrollRequest); Assert.Throws<ArgumentException>(() => s.Widgets.Handle(30, s.State.ChangeEvent, s.Slider.Serial + 1, 0)); s.Slider.Position = 7; Assert.Equal(0U, s.Widgets.Handle(30, s.State.ChangeEvent, s.Slider.Serial, 0)); Assert.Equal(7, s.Node.ScrollRequest);
    }
    /// <summary>태그가 있는 왼쪽/오른쪽 누름은 현재 부모에 통지하며 release는 통지하지 않는다.</summary>
    [Fact]
    public void TaggedClicksAndCursorReachActualParentChain()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.AppendAndDraw("A\n"u8, 42); s.Scene.Notices.Clear(); Assert.Equal(1U, s.Scene.InputHooks.Handle!(30, InputCode.LeftButton, 2, 4)); Assert.Equal((s.State.LeftTagEvent, s.Node.Serial, 42U), Assert.Single(s.Scene.Notices)); Assert.Equal(0U, s.Scene.LifeState.Focus); s.Scene.InputHooks.Handle(30, s.State.CursorEvent, 2, 4); Assert.Equal(16U, Assert.Single(s.Cursors)); Assert.Equal(1U, s.Widgets.Handle(30, InputCode.RightButton, 2, 4)); Assert.Equal((s.State.RightTagEvent, s.Node.Serial, 42U), s.Scene.Notices[^1]); int count = s.Scene.Notices.Count; Assert.Equal(0U, s.Widgets.Handle(30, InputCode.RightButton | InputCode.Release, 2, 4)); s.Widgets.Handle(30, InputCode.LeftButton | InputCode.Release, 2, 4); Assert.Equal(count, s.Scene.Notices.Count);
    }
    /// <summary>태그0 왼쪽은 캡처/프레임 드래그이며 오른쪽은 태그0도 조각이 있으면 통지한다.</summary>
    [Fact]
    public void UntaggedCaptureDragAndRightZeroTag()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.AppendAndDraw("A\n"u8); s.Widgets.SetScrollable(30, 1); s.Widgets.Handle(30, InputCode.LeftButton, 2, 4); Assert.Equal(30U, s.Scene.LifeState.Focus); Assert.Equal(0x100U, s.Node.Flags & 0x100); s.Node.Scroll = 10; s.Scene.FrameHooks.Broadcast!(30, uint.MaxValue, uint.MaxValue, 7); Assert.Equal((7, 7), (s.Node.DragY, s.Node.ScrollRequest)); Assert.Equal(1U, s.Widgets.Handle(30, InputCode.RightButton, 2, 4)); Assert.Equal((s.State.RightTagEvent, s.Node.Serial, 0U), s.Scene.Notices[^1]); s.Widgets.Handle(30, InputCode.LeftButton | InputCode.Release, 2, 4); Assert.Equal(0U, s.Node.Flags & 0x100); s.Widgets.SetScrollable(30, 0); s.Widgets.Broadcast(30, 0, 0, 999); Assert.Equal(7, s.Node.DragY);
    }
    /// <summary>커서는 태그→실제 페이지보다 큰 범위→일반 순서이며 refresh는 dirty와 Redraw만 쓴다.</summary>
    [Fact]
    public void CursorAndRefreshRules()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.AppendAndDraw("A\nA\nA\nA\n"u8); s.Widgets.SetScrollable(30, 1); s.Widgets.Handle(30, s.State.CursorEvent, 20, 4); Assert.Equal(15U, s.Cursors[^1]); s.Slider.Page = s.Slider.Maximum - s.Slider.Minimum; s.Widgets.Handle(30, s.State.CursorEvent, 20, 4); Assert.Equal(1U, s.Cursors[^1]); s.Node.TextFlags &= ~1U; s.Scene.Redraws.Clear(); Assert.Equal(0U, s.Widgets.Handle(30, s.State.RefreshEvent, 0, 0)); Assert.Equal(1U, s.Node.TextFlags & 1); Assert.Equal(new uint[] { 30 }, s.Scene.Redraws);
    }
    /// <summary>버퍼 교체는 소유free 뒤 현재 포인터를 덮고 나머지 표시/태그/스크롤을 바꾸지 않는다.</summary>
    [Fact]
    public void ReplaceOwnedWithBorrowedPreservesOtherState()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.Buffers.Construct(600, 8); s.Node.TextFlags = 0x98; s.Node.Metadata = 42; s.Node.Scroll = 7; s.Node.ScrollRequest = 2; s.OnBufferFree = _ => s.Node.Buffer = 999; s.Scene.Redraws.Clear(); s.Widgets.ReplaceBuffer(30, 600, 0); Assert.Equal((600U, 0U, 0x98U, 42U, 7, 2), (s.Node.Buffer, s.Node.OwnsBuffer, s.Node.TextFlags, s.Node.Metadata, s.Node.Scroll, s.Node.ScrollRequest)); Assert.Equal(new[] { "buffer:500" }, s.Released); Assert.Empty(s.Scene.Redraws); s.Scene.Lifetime.Kill(30); s.Scene.Lifetime.Cleanup(); Assert.True(s.Buffers.Binding(600).State.Initialized); Assert.Equal(600U, s.Node.Buffer); Assert.Equal(1U, s.Scene.Create.Count);
    }
    /// <summary>자식Slider와손잡이/배경이 먼저 반납되고 소유 버퍼free 뒤 실제 본문 노드가 반납된다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualOwnedLifetimeOrder(OriginalEdition edition)
    {
        var s = new ScrollingWidgetScene(edition); s.Construct(); s.AppendAndDraw("A\n"u8); s.Scene.Input.Capture(30); s.Scene.Lifetime.Kill(30); s.Scene.Lifetime.Cleanup(); Assert.Equal("node:30", s.Released[^1]); Assert.Equal("buffer:500", s.Released[^2]); Assert.True(s.Released.IndexOf("node:31") < s.Released.IndexOf("buffer:500")); Assert.Equal(0U, s.Node.Buffer); Assert.Empty(s.Fragments.Fragments); Assert.Equal(0U, s.Scene.LifeState.Focus); Assert.DoesNotContain(30U, s.Scene.LifeState.Overlay); Assert.Equal(1U, s.Scene.Create.Count);
    }
    /// <summary>같은 소유 버퍼 교체도 원본처럼 소멸 후 같은 토큰을 설치하며 후속 조회에서 끊어진 참조를 거부한다.</summary>
    [Fact]
    public void SameOwnedBufferAndFailedAllocationsPreserveNativePartialState()
    {
        var s = new ScrollingWidgetScene(); s.Construct(); s.Widgets.ReplaceBuffer(30, 500, 7); Assert.Equal((500U, 7U), (s.Node.Buffer, s.Node.OwnsBuffer)); Assert.Throws<InvalidOperationException>(() => s.Buffers.Binding(500)); var noBuffer = new ScrollingWidgetScene(); noBuffer.Hooks.Allocate = _ => 0; Assert.Throws<InvalidOperationException>(() => noBuffer.Widgets.Construct(30, 30, 40, 0, 171)); Assert.Equal((0U, 1U), (noBuffer.Node.Buffer, noBuffer.Node.OwnsBuffer)); Assert.Equal(2U, noBuffer.Scene.Create.Count); var noSlider = new ScrollingWidgetScene(); var allocate = noSlider.Hooks.Allocate; noSlider.Hooks.Allocate = size => size == 0xd8 ? 0 : allocate!(size); Assert.Throws<InvalidOperationException>(() => noSlider.Widgets.Construct(30, 30, 40, 0, 171)); Assert.True(noSlider.Buffer.State.Initialized); Assert.Equal(0U, noSlider.Node.Slider); Assert.Contains(30U, noSlider.Scene.LifeState.Overlay);
    }
    /// <summary>이전 배치 폭NaN은 패치에서 변경으로 저장하고 CD에서는 unordered라 그대로 남긴다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, true)]
    [InlineData(OriginalEdition.Cd1072, false)]
    public void EditionUnorderedLayoutWidth(OriginalEdition edition, bool changed)
    {
        var s = new ScrollingWidgetScene(edition); s.Construct(); s.Node.LayoutWidth = float.NaN; s.Node.TextFlags &= ~1U; s.Scene.Tree.Push(30); s.Widgets.Attach(30, 2, 3, 1, 0); Assert.Equal(changed ? 1U : 0U, s.Node.TextFlags & 1); Assert.Equal(!changed, float.IsNaN(s.Node.LayoutWidth));
    }
}
