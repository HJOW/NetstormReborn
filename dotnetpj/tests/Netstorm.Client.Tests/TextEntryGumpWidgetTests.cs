using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원문 입력창을 실제 공통 UI에 연결한 헤드리스 통합 검사다.

/// <summary>기본 트리/Slider/실제 문자 물리·글꼴/픽셀/수명을 검증한다.화면 전체/L1 추가 주장은 하지 않는다.</summary>
public sealed class TextEntryGumpWidgetTests
{
    /// <summary>같은 실제 파생/버퍼/두 Slider·자식 등록과 원문 예약 필드 보존.</summary>
    [Fact]
    public void ActualConsoleCreatesSameBufferSlidersTreeAndPartialFields()
    {
        var s = new TextEntryWidgetScene(); uint before = s.Scene.Create.Count; s.Construct("AA AA"u8.ToArray()); Assert.Same(s.Node, s.Scene.Nodes[30]); Assert.Equal((0x506a6cU, 10U, 1U, before + 13), (s.Node.Vtable, s.Node.Command, s.Node.OwnsBuffer, s.Scene.Create.Count)); Assert.Equal((1, 2, 0, 5), (s.Storage.BufferCount, s.Storage.AllocationCount, s.Buffer.State.SelectionBegin, s.Buffer.State.SelectionEnd)); Assert.Equal(new[] { 0x28, 0xd8, 0xd8 }, s.Allocations); Assert.Equal((0x1234U, 0x5678U, 0xabcdU), (s.Node.ReservedC4, s.Node.Reserved10C, s.Node.Reserved124)); Assert.Equal(30U, s.Scene.Nodes[s.Node.VerticalSlider].Parent); Assert.Equal(30U, s.Scene.Nodes[s.Node.HorizontalSlider].Parent); Assert.Equal(0U, s.Scene.Nodes[s.Node.VerticalSlider].Flags & 1); Assert.Contains(30U, s.Scene.LifeState.Idle);
    }
    /// <summary>현재 실본문/선택으로 복사·잘라내기·붙여넣기/제외문자LF 규칙을 처리한다.</summary>
    [Fact]
    public void ActualInputClipboardExcludedBytesAndCopyOutput()
    {
        var s = new TextEntryWidgetScene(); s.Construct("AA"u8.ToArray()); Assert.Equal(1U, s.Handle('A')); Assert.True(s.Buffer.Text().SequenceEqual("A"u8)); s.Handle('A'); s.Buffer.Select(0, 1); s.Handle(0x102d0000); Assert.True(s.Clipboard.AsSpan().SequenceEqual("A"u8)); Assert.Equal(1, s.Writes); s.Entry.SetExcluded(30, "\t\n"u8.ToArray()); Assert.Equal(new byte[] { 9, 0 }, s.Storage.Bytes(s.Node.ExcludedChars).Span[..2].ToArray()); s.Clipboard = "A\tA\n"u8.ToArray(); s.Buffer.Select(1, 2); s.Handle(0x202d0000); Assert.True(s.Buffer.Text().SequenceEqual("AAA\n"u8)); Assert.Equal(1, s.Reads); byte[] output = new byte[5]; Assert.Equal(4U, s.Entry.CopyText(30, output)); Assert.Equal("AAA\n\0"u8.ToArray(), output); s.Entry.SetExcluded(30, null); s.Scene.Lifetime.Kill(30); s.Scene.Lifetime.Cleanup(); Assert.Equal((0, 0), (s.Storage.BufferCount, s.Storage.AllocationCount));
    }
    /// <summary>자동완성은 같은 현재 단어/null·기존 후보 길이·Shift를 전달하고Enter는 현재 본문을 제출한다.</summary>
    [Fact]
    public void ActualConsoleCompletionCycleAndSubmission()
    {
        var s = new TextEntryWidgetScene(); s.Construct("A: AA"u8.ToArray()); s.Buffer.Select(5, 5); s.Node.Autocomplete = 71; s.Node.Submit = 72; Assert.Equal(1U, s.Handle(9)); Assert.Equal("AA"u8.ToArray(), s.CompletionWord); Assert.Equal(2, s.CompletionLength); Assert.False(s.CompletionShift); Assert.True(s.Buffer.Text().SequenceEqual("A: AAAA"u8)); s.Completion = "A"u8.ToArray(); s.Handle(0x20000009); Assert.Null(s.CompletionWord); Assert.Equal(4, s.CompletionLength); Assert.True(s.CompletionShift); Assert.True(s.Buffer.Text().SequenceEqual("A: A"u8)); s.Node.Multiline = 0; s.Handle(13); Assert.Equal("A: A"u8.ToArray(), s.Submitted); Assert.Equal(1U, s.Node.ResetWord); Assert.Contains(s.Scene.Notices, n => n.Code == s.State.EnterEvent && n.Serial == s.Node.Serial);
    }
    /// <summary>실제Pixel/글꼴·선택 반전·캐럿과 외부클립/guard·실제프레임idle 분배를 확인한다.</summary>
    [Fact]
    public void ActualGlyphSelectionCaretClipAndIdle()
    {
        var s = new TextEntryWidgetScene(); s.Construct("AA A"u8.ToArray()); s.Node.RecipientHighlight = 0; s.Buffer.Select(0, 1); byte[] selected = s.Draw(); s.Buffer.Select(1, 1); s.Node.BlinkPhase = 0; byte[] plain = s.Draw(); s.Node.BlinkPhase = 1; byte[] caret = s.Draw(); Assert.False(selected.SequenceEqual(plain)); Assert.False(caret.SequenceEqual(plain)); Assert.Contains((byte)31, selected); s.Seconds = 2; s.Frame.Idle(); Assert.Equal((2U, 2.5), (s.Node.BlinkPhase, s.Node.BlinkAt)); s.Node.ReadOnly = 1; s.Seconds = 4; s.Frame.Idle(); Assert.Equal(2U, s.Node.BlinkPhase); var clip = new ScreenRect(3, 4, 8, 12); GumpDraw.SetGumpDrawClip(s.Scene.Clip, clip); s.Buffer.Select(0, 1); byte[] clipped = s.Draw(); int changed = 0;
        // 실제 출력이 공유 클립 밖이나 행/전체guard를 바꾸지 않는다.
        for (int y = 0; y < 128; y++) for (int x = 0; x < 64; x++) { byte value = clipped[y * 64 + x]; if (x >= clip.Left && x < clip.Right && y >= clip.Top && y < clip.Bottom) changed += value != 0x5a ? 1 : 0; else Assert.Equal(0x5a, value); }
        Assert.True(changed > 0); Assert.Equal((7, 7), ((int)s.Scene.Guarded[0], (int)s.Scene.Guarded[^1])); Assert.Equal(clip, s.Scene.Clip.Clip);
    }
    /// <summary>가상Attach의 현재표시·두자식크기와Slider 사건/페이지가 실제 같은position을 사용한다.</summary>
    [Fact]
    public void ActualScrollAttachRangeAndSliderEvent()
    {
        var s = new TextEntryWidgetScene(); s.Construct("AA\nAAA\nAAAA"u8.ToArray()); s.Node.ShowVertical = 1; s.Entry.SetHorizontal(30, 1); Assert.NotEqual(0U, s.Scene.Nodes[s.Node.VerticalSlider].Flags & 1); Assert.NotEqual(0U, s.Scene.Nodes[s.Node.HorizontalSlider].Flags & 1); var vertical = (ScrollGumpNode)s.Scene.Nodes[s.Node.VerticalSlider]; var horizontal = (ScrollGumpNode)s.Scene.Nodes[s.Node.HorizontalSlider]; Assert.Equal((28f, 40f), (vertical.BaseHeight, horizontal.BaseWidth)); Assert.Equal((2, 14), s.Hooks.Measure!(30, "M\0"u8, -1)); Assert.Equal((2, 2, 2), (vertical.Page, horizontal.Page, horizontal.Maximum)); vertical.Position = 1; s.Handle(s.State.SliderChange, vertical.Serial); Assert.Equal(vertical.Position, s.Node.ScrollLine); s.Scene.Nodes[1].Flags |= 1; Assert.Equal(0U, s.Scene.InputHooks.Handle!(1, 0x123456, 7, 8)); Assert.Contains((0x123456U, 7U, 8U), s.Scene.Notices);
    }
    /// <summary>기본미부착/실제자식우선Destroy에서본문반납은부모scalar단계이며idle도정리된다.</summary>
    [Fact]
    public void ActualChildFirstDestructionAndBorrowedBuffer()
    {
        var s = new TextEntryWidgetScene(); s.Construct("AA"u8.ToArray()); uint bufferToken = s.Node.Buffer; var released = new List<uint>(); var previous = s.Scene.LifetimeHooks.Release; s.Scene.LifetimeHooks.Release = t => { released.Add(t); if (t == s.Node.VerticalSlider || t == s.Node.HorizontalSlider) Assert.NotNull(s.Storage.Find(bufferToken)); previous!(t); }; s.Scene.Lifetime.Kill(30); Assert.NotNull(s.Storage.Find(bufferToken)); s.Scene.Lifetime.Cleanup(); Assert.Equal((1U, 0, 0, 0U), (s.Scene.Create.Count, s.Storage.BufferCount, s.Storage.AllocationCount, s.Node.Buffer)); Assert.DoesNotContain(30U, s.Scene.LifeState.Idle); Assert.Contains(s.Node.VerticalSlider, released); Assert.Contains(s.Node.HorizontalSlider, released);
        var borrowed = new TextEntryWidgetScene(); borrowed.Construct("A"u8.ToArray()); var actual = borrowed.Buffer; borrowed.Node.OwnsBuffer = 0; borrowed.Scene.Lifetime.Kill(30); borrowed.Scene.Lifetime.Cleanup(); Assert.Same(actual, borrowed.Storage.Find(500)); Assert.Equal(2, borrowed.Storage.AllocationCount); actual.Destroy(); borrowed.Storage.ReleaseBuffer(500);
    }
}
