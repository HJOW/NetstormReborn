using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // Slider의 고정 원본 계약과 실제 트리/사건/픽셀/수명을 검사한다.

/// <summary>cpppj ScrollGump/ScrollGumpWidgets의 정적 계약과 헤드리스 통합.독립 원본 실행 fixture/L1은 아니다.</summary>
public sealed class ScrollGumpTests
{
    /// <summary>두 판본의 실제 자식 생성/할당 크기/부분 필드/기본값과 position 분자의 손잡이 사각형.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 10, 100)]
    [InlineData(OriginalEdition.Cd1072, 10, 100)]
    [InlineData(OriginalEdition.Patch1078, 100, 10)]
    [InlineData(OriginalEdition.Cd1072, 100, 10)]
    [InlineData(OriginalEdition.Patch1078, 0, 0)]
    [InlineData(OriginalEdition.Cd1072, 0, 0)]
    public void ActualConstructionAndAttach(OriginalEdition edition, int width, int height)
    {
        var s = new ScrollWidgetScene(edition); s.WidgetState.SliderAlignment = -7; s.Widgets.Construct(2, width, height, 77, 171); Assert.Equal(new[] { 0x178, 0xb8, 0x178, 0xb8, edition == OriginalEdition.Patch1078 ? 0xd8 : 0xd4 }, s.Allocations); Assert.Equal((0, 100, 50, 10, 1), (s.Node.Minimum, s.Node.Maximum, s.Node.Position, s.Node.Page, s.Node.Step)); Assert.Equal(7U, s.Create.Count); Assert.Equal(0x1234U, s.Node.ReservedBC); Assert.Equal(0xfffffff9U, s.Node.Command); Assert.Equal((77U, 171U, 83U, 1U, 73, 91), (s.Thumb.DetailType, s.Thumb.DetailFrame, s.Thumb.Type, s.Thumb.Frame, s.Thumb.PreviousY, s.Thumb.PreviousX)); Assert.Equal(edition == OriginalEdition.Patch1078 ? 0x5116f8U : 0x500c78U, s.Node.Vtable);
        if (width == 0) { Assert.Equal((40f, 40f), (s.Node.BaseWidth, s.Node.BaseHeight)); return; } s.Slider.Attach(2, 2, 3, 1, 0); bool horizontal = width > height; var expected = horizontal ? new ScreenRect(41, 1, 47, 9) : new(1, 41, 9, 47); Assert.Equal(expected, s.Thumb.Handle); Assert.Equal(2U, s.Thumb.Parent); Assert.Equal(1U, s.Node.Parent); Assert.Equal(horizontal ? 3U : 0U, ((ScrollGumpArrowNode)s.Nodes[s.Node.Decrement]).Direction); Assert.Equal(horizontal ? 1U : 2U, ((ScrollGumpArrowNode)s.Nodes[s.Node.Increment]).Direction); s.Slider.SetRange(2, 10, 110); Assert.Equal(expected, s.Thumb.Handle);
    }
    /// <summary>동률 range는+1/두 번 위치 재적용이며 음수 페이지/정수 감김에 임의 보정을 넣지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void RangePageAndWrappedPosition(OriginalEdition edition)
    {
        var s = new ScrollWidgetScene(edition); s.Widgets.Construct(2, 10, 100, 77, 171); s.Redraws.Clear(); s.Slider.SetRange(2, 10, 10); Assert.Equal((10, 11, 1, 10), (s.Node.Minimum, s.Node.Maximum, s.Node.Page, s.Node.Position)); Assert.Equal(new uint[] { 2, 2 }, s.Redraws); s.Slider.SetPage(2, -5); s.Slider.SetPosition(2, 100); Assert.Equal(16, s.Node.Position); s.Node.Page = 2; Assert.Throws<ArgumentException>(() => s.Slider.SetPosition(2, 0)); s.Node.Page = 0; s.Slider.SetRange(2, int.MaxValue, int.MaxValue);
        // max+1이 MIN으로 감긴 뒤 max < page+position 비교가 참이므로 두 번 모두 position=MIN이다.
        Assert.Equal((int.MaxValue, int.MinValue, int.MinValue), (s.Node.Minimum, s.Node.Maximum, s.Node.Position));
    }
    /// <summary>부모가 이동을 소비하지 않았을 때만 현재 step/position으로 기본 화살표 이동을 한다.</summary>
    [Fact]
    public void ParentBubbleCanConsumeOrModifyArrowFallback()
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); s.Slider.Handle(2, s.State.ClickEvent | InputCode.Release, s.Nodes[s.Node.Decrement].Serial, 0); Assert.Equal(49, s.Node.Position); Assert.Equal(new[] { (s.State.StepEvent, s.Node.Serial, 2U), (s.State.ChangeEvent, s.Node.Serial, 0U) }, s.Notices); s.Notices.Clear(); s.ParentHandle = (c, _, _) => { if (c == s.State.StepEvent) { s.Node.Position = 61; s.Node.Step = 3; } return 0; }; s.Slider.Handle(2, s.State.RepeatEvent, s.Nodes[s.Node.Decrement].Serial, 0); Assert.Equal(58, s.Node.Position); s.ParentHandle = (c, _, _) => { if (c == s.State.StepEvent) s.Node.Position = 71; return 1; }; Assert.Equal(0U, s.Slider.Handle(2, s.State.ClickEvent, s.Nodes[s.Node.Increment].Serial, 0)); Assert.Equal(71, s.Node.Position); Assert.Equal((s.State.ChangeEvent, s.Node.Serial, 0U), s.Notices[^1]);
    }
    /// <summary>포함 손잡이 양끝은 캡처,바깥은 실제 Slider를 거쳐 현재 serial의 page4/3을 부모로 보낸다.</summary>
    [Fact]
    public void VerticalThumbInclusiveCaptureAndPageRequests()
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); ScreenRect b = s.Collection.Bounds(s.Node.Thumb); uint low = (uint)(b.Top + 41), high = (uint)(b.Top + 47); s.InputHooks.Handle!(s.Node.Thumb, InputCode.LeftButton, 0, low - 1); s.InputHooks.Handle(s.Node.Thumb, InputCode.LeftButton, 0, high + 1);
        // Slider.Handle도0을 반환하므로 안쪽 Bubble의 Slider serial 뒤 바깥 Bubble의 원래 Thumb serial도 루트까지 전달된다.
        Assert.Equal(new[] { (s.State.StepEvent, s.Node.Serial, 4U), (s.State.StepEvent, s.Thumb.Serial, 4U), (s.State.StepEvent, s.Node.Serial, 3U), (s.State.StepEvent, s.Thumb.Serial, 3U) }, s.Notices); Assert.Equal(0U, s.LifeState.Focus); s.InputHooks.Handle(s.Node.Thumb, InputCode.LeftButton, 0, low); Assert.Equal(s.Node.Thumb, s.LifeState.Focus); Assert.Equal((int)low, s.Thumb.PreviousY); Assert.Equal(0x100U, s.Thumb.Flags & 0x100); s.InputHooks.Handle(s.Node.Thumb, InputCode.LeftButton | InputCode.Release, 0, high); Assert.Equal(0U, s.Thumb.Flags & 0x100); Assert.Equal(s.Node.Thumb, s.LifeState.Focus);
    }
    /// <summary>패치 가로만 눌러 캡처한다.가로 release는 플래그를 지우지 않으며 방송은 여전히 y delta를 쓴다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, true)]
    [InlineData(OriginalEdition.Cd1072, false)]
    public void HorizontalEditionAndBroadcastQuirks(OriginalEdition edition, bool captured)
    {
        var s = new ScrollWidgetScene(edition); s.Widgets.Construct(2, 100, 10, 77, 171); s.Slider.Attach(2, 0, 0, 1, 0); ScreenRect b = s.Collection.Bounds(s.Node.Thumb); s.Slider.ThumbHandle(s.Node.Thumb, InputCode.LeftButton, (uint)(b.Left + 41), 0); Assert.Equal(captured ? s.Node.Thumb : 0U, s.LifeState.Focus); s.Thumb.Flags |= 0x100; s.Slider.ThumbHandle(s.Node.Thumb, InputCode.LeftButton | InputCode.Release, 0, 0); Assert.Equal(0x100U, s.Thumb.Flags & 0x100); s.Notices.Clear(); s.FrameHooks.Broadcast!(s.Node.Thumb, 0, uint.MaxValue, 74); Assert.Equal(50, s.Node.Position); Assert.Equal(74, s.Thumb.PreviousY); Assert.Equal((s.State.ChangeEvent, s.Node.Serial, 0U), Assert.Single(s.Notices)); s.Poll = new(InputCode.Release | InputCode.LeftButton, -999, -999); s.FrameHooks.Broadcast(s.Node.Thumb, 0, 0, 999); Assert.Equal(0U, s.Thumb.Flags & 0x100); Assert.Equal(74, s.Thumb.PreviousY);
    }
    /// <summary>세로 드래그 곱은64비트 확장이 아닌 DWORD 감김 후 CRT 절삭이다.</summary>
    [Fact]
    public void DragUsesWrappedProductAndOriginalPositionNumerator()
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); s.Node.Flags |= 8; s.Node.Minimum = 0; s.Node.Maximum = int.MaxValue; s.Node.Page = 0; s.Thumb.Handle = new(0, 1, 0, 0); s.Slider.DragVertical(2, 0); Assert.Equal(26843545, s.Node.Position); s.Slider.DragVertical(2, 1); Assert.Equal(0, s.Node.Position); s.Node.BaseHeight = 20; s.Slider.DragVertical(2, 2); Assert.Equal(0, s.Node.Position);
    }
    /// <summary>실제 돌 버튼 뒤 삼각형 세 선의 네 방향 끝점과 빈 내부를 현재 픽셀에서 검사한다.</summary>
    [Theory]
    [InlineData(0U, 6, 4)]
    [InlineData(1U, 8, 6)]
    [InlineData(2U, 6, 8)]
    [InlineData(3U, 4, 6)]
    public void ActualArrowPixels(uint direction, int x, int y)
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); uint t = s.Node.Decrement; var n = (ScrollGumpArrowNode)s.Nodes[t]; n.Direction = direction; n.BaseWidth = n.Width = n.BaseHeight = n.Height = 8; s.Tree.Move(t, 2, 2, 1); s.Draw.Draw(t); Assert.Equal(171, s.Pixels.Span[y * 64 + x]); Assert.NotEqual(171, s.Pixels.Span[6 * 64 + 6]); Assert.Equal(7, s.Guarded[0]); Assert.Equal(7, s.Guarded[^1]);
    }
    /// <summary>실제 질감 손잡이는 detail 인자를 쓰지 않고 기본 type/frame을 쓰며 끝점+1 없이 현재 클립에 그린다.</summary>
    [Fact]
    public void ActualThumbUsesBaseAssetsAndHalfOpenDrawRect()
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); s.Thumb.Handle = new(1, 5, 8, 17); s.Draw.Draw(s.Node.Thumb); Assert.Contains(new ScreenRect(3, 18, 10, 30), s.Clips); Assert.DoesNotContain(new ScreenRect(3, 18, 11, 31), s.Clips); Assert.NotEmpty(s.Shapes); Assert.All(s.Shapes, r => Assert.Equal((83U, 1U), (r.Type, r.Frame))); Assert.Contains(s.Pixels.ToArray(), p => p != 7); Assert.Equal(new ScreenRect(0, 0, 64, 128), s.Clip.Clip); Assert.Equal(7, s.Guarded[0]); Assert.Equal(7, s.Guarded[^1]);
    }
    /// <summary>자식 배경→화살표/손잡이→Slider를 실제 자식 우선 Cleanup으로 반납하고 캡처를 해제한다.</summary>
    [Fact]
    public void ActualLifetimeDestroysChildrenBeforeSlider()
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); uint a = s.Node.Decrement, b = s.Node.Increment, h = s.Node.Thumb, ab = ((ScrollGumpArrowNode)s.Nodes[a]).Background, bb = ((ScrollGumpArrowNode)s.Nodes[b]).Background; s.Input.Capture(h); s.Lifetime.Kill(2); s.Lifetime.Cleanup(); Assert.Equal(1U, s.Create.Count); Assert.Equal(0U, s.LifeState.Focus); Assert.Equal(6, s.Released.Count); Assert.True(s.Released.IndexOf(ab) < s.Released.IndexOf(a)); Assert.True(s.Released.IndexOf(bb) < s.Released.IndexOf(b)); Assert.Equal(2U, s.Released[^1]);
    }
    /// <summary>할당0은 생성만 생략한다.후속 원본 null 부착에서 이미 생성한 자식/부모 필드를 남긴다.</summary>
    [Fact]
    public void AllocationFailurePreservesPartialConstruction()
    {
        var s = new ScrollWidgetScene(); var allocate = s.WidgetHooks.Allocate; bool first = true; s.WidgetHooks.Allocate = size => { if (first) { first = false; return 0; } return allocate!(size); }; Assert.Throws<InvalidOperationException>(() => s.Widgets.Construct(2, 10, 100, 77, 171)); Assert.Equal(0U, s.Node.Decrement); Assert.NotEqual(0U, s.Node.Increment); Assert.NotEqual(0U, s.Node.Thumb); Assert.Equal(5U, s.Create.Count); Assert.Equal((0, 0, 0), (s.Node.Minimum, s.Node.Maximum, s.Node.Position));
    }
    /// <summary>같은 기본 저장소의 다중 줄 TextGump가 실제 Slider 네 경계로 범위/페이지/위치를 반영한다.</summary>
    [Fact]
    public void ActualTextViewportUsesSharedSlider()
    {
        var s = new ScrollWidgetScene(); s.ConstructAndAttach(); var node = new ScrollingTextGumpNode { Buffer = 40, Slider = 2 }; s.Nodes.Add(30, node); s.Factory.Construct(30, 0, 10, 10, 1); var fragments = new StyleTextState(); var styleHooks = new StyleTextHooks(); var fonts = StyleTextTestSupport.PixelFonts(); StyleText.BindStyleTextFonts(styleHooks, new(fonts)); var style = new StyleText(fragments, styleHooks); var state = new StyleTextBufferState(); var bufferHooks = new StyleTextBufferHooks(); StyleTextBuffer.BindStyleTextBufferFonts(bufferHooks, new(fonts)); var buffer = new StyleTextBuffer(state, fragments, style, 8, bufferHooks); buffer.AppendText("A\nA\n\0"u8, new uint[8]); buffer.Layout(99); var hooks = new ScrollingTextGumpHooks { Node = t => t == 30 ? node : null, BaseNode = t => s.Nodes.GetValueOrDefault(t), Buffer = t => t == 40 ? new(state, buffer) : default }; ScrollGump.BindScrollingTextSlider(hooks, s.Slider); var text = new ScrollingTextGump(s.Create, s.Collection, s.Clip, fragments, style, hooks); text.SetScroll(30, 100); Assert.Equal((28, 10, 18, 18), (s.Node.Maximum, s.Node.Page, s.Node.Position, node.Scroll));
    }
}
