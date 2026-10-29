using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 다중 줄 표시/스크롤/선택의 독립 고정 계약 검사다.

/// <summary>같은 실제 글꼴/버퍼/클립/픽셀 통합과 C++ 현재 조회 순서. 원본 실행 fixture/L1은 아니다.</summary>
public sealed class ScrollingTextGumpTests
{
    /// <summary>두 판본에서 실제 줄 배치/그림자 없는 글리프/선택과 공통Draw 분배를 연결한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void RealFontsLayoutPixelsHitAndClip(OriginalEdition edition)
    {
        var s = new StyleBufferScene(edition); s.Node.BaseWidth = s.Node.Width = 7; s.Scroll.AppendText(2, "A A A\n\0"u8); s.Draw.Draw(2); Assert.Equal(3, s.State.SoftCount); Assert.Equal(0U, s.Node.TextFlags & 1); Assert.Equal(0U, s.State.Dirty); Assert.Equal(171, s.Pixels.Span[34]); Assert.Equal(171, s.Pixels.Span[482]); Assert.Equal(171, s.Pixels.Span[930]); Assert.Equal(3, s.Pixels.Span.ToArray().Count(p => p == 171)); Assert.Equal(7, s.Guarded[0]); Assert.Equal(7, s.Guarded[^1]); Assert.Equal(new ScreenRect(0, 0, 32, 40), s.Clip.Clip);
        uint head = s.Rows()[0].Head; Assert.Equal(0U, s.Scroll.HitTest(2, new(2, 1))); Assert.Equal(head, s.Scroll.HitTest(2, new(2, 2))); Assert.Equal(0U, s.Scroll.HitTest(2, new(7, 2))); Assert.Equal(s.Rows()[1].Head, s.Scroll.HitTest(2, new(2, 16))); Assert.Equal(0U, s.Scroll.HitTest(2, new(9, 2))); s.Draw.Draw(3); Assert.Contains("fallback:3", s.Events);
    }
    /// <summary>기본 문맥은 통지 전에 복사하고 명시 문맥은 통지 뒤 현재 값을 읽는다.</summary>
    [Fact]
    public void DefaultContextSnapshotAndReplaceKeepsTailAndDisplay()
    {
        var s = new StyleBufferScene(); s.State.OnAppend = _ => s.Node.TextContext[2] = 77; s.Scroll.AppendText(2, "A\nTAIL\0"u8); Assert.Equal(171U, s.Fragments.Fragments[s.State.HardLines[0]].Context[2]); s.Scroll.EnsureLayout(2); uint display = s.Rows()[0].Head, tail = s.State.HardLines[1]; s.Scroll.ReplaceText(2, "A\n\0"u8, 42); Assert.Equal(tail, s.State.HardLines[0]); Assert.True(s.Fragments.Fragments.ContainsKey(display)); Assert.Equal(new[] { "TAIL", "A" }, s.Texts(tail)); uint added = s.Fragments.Fragments[tail].Next; Assert.Equal(77U, s.Fragments.Fragments[added].Context[2]); Assert.Equal(42U, s.Fragments.Fragments[added].Metadata); Assert.Equal(1, s.State.LineCount); Assert.Equal(1U, s.Node.TextFlags & 1);
    }
    /// <summary>플래그2일 때만 slider 폭을 빼며 배치 실패 전에 dirty를 지운다.0폭에서는 기존 표시를 보존한다.</summary>
    [Fact]
    public void SliderWidthDirtyAndFailedLayoutStorage()
    {
        var s = new StyleBufferScene(); s.Node.BaseWidth = 7; s.Scroll.AppendText(2, "A A\n\0"u8); s.Node.TextFlags |= 2; s.Scroll.EnsureLayout(2); Assert.Equal(new[] { 5, 2 }, s.Rows().Select(r => r.Width)); uint head = s.Rows()[0].Head; s.Node.BaseWidth = 2; s.Node.TextFlags |= 1; s.State.Dirty = 3; s.Scroll.EnsureLayout(2); Assert.Equal(head, s.Rows()[0].Head); Assert.Equal(0U, s.State.Dirty); s.Node.BaseWidth = 10; s.Node.TextFlags |= 1; s.BufferHooks.MeasureText = null; Assert.Throws<ArgumentException>(() => s.Scroll.EnsureLayout(2)); Assert.Equal(0U, s.State.Dirty); Assert.Equal(2U, s.Node.TextFlags);
    }
    /// <summary>강제 맞춤의 폭/높이/scale과 빈 내용의 높이+1을 고정값으로 검사한다.</summary>
    [Fact]
    public void FitContentHeightAndSliderPadding()
    {
        var s = new StyleBufferScene(); s.Node.Scale = .5f; s.Scroll.FitHeight(2); Assert.Equal((1f, .5f), (s.Node.BaseHeight, s.Node.Height)); s.Scroll.AppendText(2, "A A\n\0"u8); s.Node.TextFlags |= 2; s.Scroll.FitContent(2, 3, 4); Assert.Equal((13f, 6.5f, 18f, 9f), (s.Node.BaseWidth, s.Node.Width, s.Node.BaseHeight, s.Node.Height)); s.Scroll.FitHeight(2); Assert.Equal((15f, 7.5f), (s.Node.BaseHeight, s.Node.Height));
    }
    /// <summary>range/page/position 호출 직전에 현재 slider와 scroll을 다시 읽는다.</summary>
    [Fact]
    public void SliderCallbacksUseCurrentPointersAndValues()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\nA\nA\n\0"u8, s.Context); s.Buffer.Layout(99); s.Node.BaseHeight = 10.75f; s.ScrollHooks.SliderRange = (t, min, max) => { s.Events.Add($"r:{t},{min},{max}"); s.Node.Slider = 4; s.Node.Scroll = 9; }; s.ScrollHooks.SliderPage = (t, p) => { s.Events.Add($"p:{t},{p}"); s.Node.Slider = 5; }; s.ScrollHooks.SliderSetPosition = (t, p) => { s.Events.Add($"s:{t},{p}"); s.Node.Slider = 6; }; s.ScrollHooks.SliderPosition = t => { s.Events.Add($"g:{t}"); return 13; }; s.Scroll.SetScroll(2, 100); Assert.Equal(new[] { "r:3,0,42", "p:4,10", "s:5,9", "g:6" }, s.Events); Assert.Equal(13, s.Node.Scroll);
    }
    /// <summary>C++ 대입의 우변 콜백 뒤 현재 노드를 조회한다. C# 좌변 선평가로 옛 노드에 쓰면 실패한다.</summary>
    [Fact]
    public void PositionCallbackResolvesAssignmentTargetAfterReturning()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\nA\nA\n\0"u8, s.Context); s.Buffer.Layout(99); s.Node.BaseHeight = 10; var replacement = new ScrollingTextGumpNode { Scroll = 999 }; var current = s.Node;
        s.ScrollHooks.Node = _ => current; s.ScrollHooks.BaseNode = _ => current; s.ScrollHooks.SliderPosition = _ => { current = replacement; return 13; }; s.Scroll.SetScroll(2, 100); Assert.Equal(32, s.Node.Scroll); Assert.Equal(13, replacement.Scroll);
    }
    /// <summary>같은 원본 체인에서 실제 글꼴과 SHP/색표를 함께 출력하며 표시 복제의 mapped 초기화를 구별한다.</summary>
    [Fact]
    public void MixedTextPictureChainUsesActualSharedPixels()
    {
        var s = new StyleBufferScene(); var assets = new MarkupAssets(GumpBackgroundTestAssets.Shapes(), 90, [new(82, 0, 1, 0, 0)]); byte[] pictureColors = Enumerable.Range(0, 256).Select(i => unchecked((byte)(i + 40))).ToArray(); var hooks = new MarkupPixelTarget(new(s.Pixels, 32, 40, s.Clip), assets, s.Colors.VfxColors, pictureColors).Hooks(); var state = new MarkupState(); MarkupAssets.BindMarkupAssets(state, hooks, assets); var pictures = new Markup(s.Fragments, state, hooks); Markup.BindMarkup(s.StyleHooks, pictures); StyleTextBuffer.BindStyleTextBufferMarkup(s.BufferHooks, pictures);
        s.Scroll.AppendText(2, "A\0"u8); s.Scroll.AppendPicture(2, pictures, 82, 0); uint original = s.Fragments.Fragments[s.State.HardLines[0]].Next; pictures.SetMapped(original, 1); uint[] other = (uint[])s.Context.Clone(); other[2] = 77; s.Scroll.AppendText(2, "A\n\0"u8, other); s.Scroll.EnsureLayout(2); uint clone = s.Fragments.Fragments[s.Rows()[0].Head].Next; Assert.NotEqual(original, clone); Assert.Equal((byte)0, state.Pictures[clone].Mapped); pictures.SetMapped(clone, 1); s.Draw.Draw(2);
        Assert.Equal(171, s.Pixels.Span[34]); Assert.Equal(77, s.Pixels.Span[39]); Assert.Equal(new byte[] { 41, 42, 41 }, s.Pixels.Span.Slice(868, 3).ToArray()); Assert.Equal(44, s.Pixels.Span[901]); Assert.Equal(77, s.Colors.VfxColors[100]); Assert.Equal(7, s.Guarded[0]); Assert.Equal(7, s.Guarded[^1]);
    }
    /// <summary>상대 요청은 적용 scroll을 쓰며 정상 반환 뒤 콜백의 새 요청도-1로 덮어쓴다.</summary>
    [Fact]
    public void RequestOverwriteOverflowBottomAndFailure()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\nA\nA\n\0"u8, s.Context); s.Buffer.Layout(99); s.Node.BaseHeight = 10; s.Node.Scroll = 5; s.Scroll.RequestScroll(2, 20); s.Scroll.RequestRelativeScroll(2, 3); Assert.Equal(8, s.Node.ScrollRequest); s.ScrollHooks.SliderPosition = _ => { s.Node.ScrollRequest = 9; return 8; }; s.Scroll.ProcessRequest(2); Assert.Equal(-1, s.Node.ScrollRequest); s.Node.Scroll = int.MaxValue; s.Scroll.RequestRelativeScroll(2, 1); Assert.Equal(0, s.Node.ScrollRequest);
        s.ScrollHooks.SliderPosition = _ => s.Node.Scroll; s.Scroll.RequestBottom(2, 999); s.Scroll.ProcessRequest(2); Assert.Equal(32, s.Node.Scroll); Assert.Equal(999, s.Node.BottomPadding); Assert.True(s.Scroll.AtBottom(2)); s.Node.BaseHeight = float.NaN; Assert.False(s.Scroll.AtBottom(2)); s.Node.BaseHeight = 10; s.Scroll.RequestScroll(2, -3); Assert.Throws<ArgumentException>(() => s.Scroll.ProcessRequest(2)); Assert.Equal(-3, s.Node.ScrollRequest); s.Scroll.RequestScroll(2, 4); s.ScrollHooks.SliderPage = (_, _) => throw new InvalidOperationException("의도한 가상 경계 실패"); Assert.Throws<InvalidOperationException>(() => s.Scroll.ProcessRequest(2)); Assert.Equal((4, 4), (s.Node.Scroll, s.Node.ScrollRequest));
    }
    /// <summary>정확한 줄 끝 scroll에서는 첫 줄을 유지하고 저장y는 첫 줄에만 적용한다.</summary>
    [Fact]
    public void StoredFirstYThenAccumulatedHeightsAndStrictTopHit()
    {
        var s = new StyleBufferScene(); s.Scroll.AppendText(2, "A\nA\n\0"u8); s.Scroll.EnsureLayout(2); var rows = s.Rows(); rows[1].Y = 999; s.Node.Scroll = 14; s.Draw.Draw(2); Assert.Equal(171, s.Pixels.Span[34]); Assert.Equal(1, s.Pixels.Span.ToArray().Count(p => p == 171)); Assert.Equal(0U, s.Scroll.HitTest(2, new(2, 1))); Assert.Equal(rows[1].Head, s.Scroll.HitTest(2, new(2, 2))); s.Node.Scroll = 2000; Assert.Throws<InvalidOperationException>(() => s.Draw.Draw(2)); Assert.Equal(new ScreenRect(0, 0, 32, 40), s.Clip.Clip);
    }
    /// <summary>첫 최대상승 뒤 현재head를 다시 읽고 가상 출력 실패에도 내부 클립을 복구한다.</summary>
    [Fact]
    public void CurrentHeadAfterBaselineAndExceptionClip()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\n\0"u8, s.Context); s.Buffer.Layout(99); var row = s.Rows()[0]; uint old = row.Head, replacement = s.Style.NewText("A\0"u8, s.Context); int calls = 0; s.State.Dirty = 0;
        s.StyleHooks.TextMetrics = _ => { if (++calls == 1) row.Head = replacement; return (31, 26, 5); }; s.StyleHooks.TextDraw = (_, at, _) => { Assert.Equal(new ScreenPoint(2, 1), at); Assert.Equal(replacement, row.Head); Assert.Equal(new ScreenRect(2, 1, 14, 31), s.Clip.Clip); throw new InvalidOperationException("의도한 출력 실패"); }; Assert.Throws<InvalidOperationException>(() => s.Draw.Draw(2)); Assert.Equal(2, calls); Assert.True(s.Fragments.Fragments.ContainsKey(old)); Assert.Equal(new ScreenRect(0, 0, 32, 40), s.Clip.Clip);
    }
    /// <summary>NaN viewport에서는 CD만 행을 출력하며 경계 크기 자체는 유효 입력으로 유지한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0)]
    [InlineData(OriginalEdition.Cd1072, 1)]
    public void EditionNanViewportDraw(OriginalEdition edition, int count)
    {
        var s = new StyleBufferScene(edition); s.Buffer.AppendText("A\n\0"u8, s.Context); s.Buffer.Layout(99); s.State.Dirty = 0; s.Node.BaseHeight = float.NaN; s.Draw.Draw(2); Assert.Equal(count, s.Pixels.Span.ToArray().Count(p => p == 171));
    }
    /// <summary>노드/처리기/버퍼를 다른 복사본으로 연결하면 조용히 출력하지 않고 거부한다.</summary>
    [Fact]
    public void SharedStorageContractsAndNoAutomaticHitLayout()
    {
        var s = new StyleBufferScene(); s.Scroll.AppendText(2, "A\n\0"u8); Assert.Equal(0U, s.Scroll.HitTest(2, new(2, 2))); Assert.Empty(s.State.SoftLines); s.ScrollHooks.BaseNode = _ => new ScrollingTextGumpNode(); Assert.Throws<InvalidOperationException>(() => s.Scroll.EnsureLayout(2)); s.ScrollHooks.BaseNode = _ => s.Node; s.ScrollHooks.Buffer = _ => new(new() { Initialized = true }, s.Buffer); Assert.Throws<InvalidOperationException>(() => s.Scroll.EnsureLayout(2));
    }
}
