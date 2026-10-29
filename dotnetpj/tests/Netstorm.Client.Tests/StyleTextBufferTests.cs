using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 입력/표시 소유와 원본 줄바꿈의 고정 계약 검사다.

/// <summary>독립 원본 실행 fixture가 없는 정적 C++ 계약/실제 글꼴 통합 검사. L1로 세지 않는다.</summary>
public sealed class StyleTextBufferTests
{
    /// <summary>LF만 완료 줄로 세며 CR/탭/중간NUL과 미완료 끝 칸을 분리한다.</summary>
    [Fact]
    public void CompletedLinesOnlyAndExplicitNul()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText([65, 13, 9, 0, 66, 10, 65], s.Context, 7, 91); Assert.Equal(1, s.State.LineCount); Assert.Equal(new byte[] { 65, 13, 9, 0, 66, 0 }, s.Fragments.Fragments[s.State.HardLines[0]].Text); Assert.Equal(new[] { "A" }, s.Texts(s.State.HardLines[1])); s.Buffer.Layout(100); var row = Assert.Single(s.Rows()); Assert.Equal((2, 14, 0), (row.Width, row.Height, row.Y)); Assert.Equal(91U, s.Fragments.Fragments[row.Head].Metadata); Assert.NotEqual(s.State.HardLines[0], row.Head); Assert.Equal(1U, s.State.Dirty);
    }
    /// <summary>엄격한 폭 비교는 공백을 앞 조각에 포함하고 마지막NUL도 명시 복사한다.</summary>
    [Fact]
    public void WrapKeepsSpacesAndTerminalNul()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A A A\nTAIL\0"u8, s.Context); s.Buffer.Layout(7); var rows = s.Rows(); Assert.Equal(new[] { 5, 5, 2 }, rows.Select(r => r.Width)); Assert.Equal(new[] { 0, 14, 28 }, rows.Select(r => r.Y)); Assert.Equal(new[] { "A ", "A ", "A" }, rows.Select(r => Assert.Single(s.Texts(r.Head)))); Assert.Equal(new byte[] { 65, 0, 0 }, s.Fragments.Fragments[rows[2].Head].Text); Assert.Equal((5, 42), (s.Buffer.MaximumWidth(), s.Buffer.TotalHeight())); Assert.Equal(3, s.State.SoftCount);
    }
    /// <summary>폭과 첫 단어가 같아도 강제 배치 뒤 빈 표시 줄이 생긴다. NoWrap은 이를 생략한다.</summary>
    [Fact]
    public void ExactWidthCreatesEmptyRowAndNoWrapPreservesWhole()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\n\0"u8, s.Context); s.Buffer.Layout(2); Assert.Equal(new[] { (2, 14, 0), (0, 0, 14) }, s.Rows().Select(r => (r.Width, r.Height, r.Y))); Assert.Equal(14, s.Buffer.TotalHeight()); uint old = s.Rows()[0].Head; s.State.NoWrap = 8; s.Buffer.Layout(2); Assert.False(s.Fragments.Fragments.ContainsKey(old)); Assert.Single(s.Rows()); Assert.Equal(new byte[] { 65, 0 }, s.Fragments.Fragments[s.Rows()[0].Head].Text);
    }
    /// <summary>0이하 폭은 기존 표시/토큰/개수를 그대로 유지한다.</summary>
    [Theory]
    [InlineData(0)]
    [InlineData(-1)]
    public void NonpositiveWidthPreservesDisplay(int width)
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\n\0"u8, s.Context); s.Buffer.Layout(9); uint head = s.Rows()[0].Head, next = s.State.NextSoftToken; s.Buffer.AppendText("A\n\0"u8, s.Context); s.Buffer.Layout(width); Assert.Equal(head, Assert.Single(s.Rows()).Head); Assert.Equal(next, s.State.NextSoftToken); Assert.Equal(1, s.State.SoftCount); Assert.Equal(2, s.State.LineCount);
    }
    /// <summary>입력 칸이 차면 첫 완료 줄만 버리고 모든 칸을 옮기며 이전 표시 복제는 남는다.</summary>
    [Fact]
    public void EvictionMovesUnfinishedTailWithoutDeletingDisplay()
    {
        var s = new StyleBufferScene(maxLines: 3); s.Buffer.AppendText("old\nA\nA\0"u8, s.Context); uint old = s.State.HardLines[0], tail = s.State.HardLines[2]; s.Buffer.Layout(99); uint display = s.Rows()[0].Head; s.Buffer.AppendText("\n\0"u8, s.Context); Assert.False(s.Fragments.Fragments.ContainsKey(old)); Assert.True(s.Fragments.Fragments.ContainsKey(display)); Assert.Equal(tail, s.State.HardLines[1]); Assert.Equal(0U, s.State.HardLines[2]); Assert.Equal(2, s.State.LineCount); s.Buffer.RemoveLine(0); Assert.Equal(tail, s.State.HardLines[0]); Assert.Throws<ArgumentOutOfRangeException>(() => s.Buffer.RemoveLine(1));
    }
    /// <summary>추가 통지는 길이 확인 이전이며 명시 문맥의 현재 값을 이후 복사한다.</summary>
    [Fact]
    public void NoticeBeforeValidationAndContextCopy()
    {
        var s = new StyleBufferScene(); int calls = 0; s.State.OnAppend = bytes => { calls++; s.Context[2] = 77; }; Assert.Throws<ArgumentException>(() => s.Buffer.AppendText([65], s.Context, -9)); Assert.Equal(1, calls); Assert.Equal(0U, s.State.Dirty); s.Buffer.AppendText("A\n\0"u8, s.Context, -5); Assert.Equal(77U, s.Fragments.Fragments[s.State.HardLines[0]].Context[2]); Assert.Equal(2, calls); Assert.Throws<ArgumentOutOfRangeException>(() => s.Buffer.AppendText([0], s.Context, 2)); Assert.Equal(3, calls);
    }
    /// <summary>소멸은 완료 입력/표시/센티널만 반납하며 미완료 조각과 카운터는 보존한다.</summary>
    [Fact]
    public void DestructionDoesNotOwnUnfinishedTail()
    {
        var s = new StyleBufferScene(); s.Buffer.AppendText("A\nTAIL\0"u8, s.Context); s.Buffer.Layout(9); uint hard = s.State.HardLines[0], tail = s.State.HardLines[1], soft = s.Rows()[0].Head, sentinel = s.Style.NewText("A\0"u8, s.Context); s.State.Sentinel.Head = sentinel; var released = new List<uint>(); s.StyleHooks.Release = t => { if (t == hard) Assert.Equal(hard, s.State.HardLines[0]); released.Add(t); }; s.Buffer.Destroy(); Assert.Equal(new[] { hard, soft, sentinel }, released); Assert.Single(s.Fragments.Fragments); Assert.True(s.Fragments.Fragments.ContainsKey(tail)); Assert.Empty(s.State.HardLines); Assert.Empty(s.State.SoftLines); Assert.False(s.State.Initialized); Assert.Equal((1, 1), (s.State.LineCount, s.State.SoftCount)); Assert.Throws<ArgumentException>(() => s.Buffer.TotalHeight()); s.Style.Destroy(tail, 1);
    }
    /// <summary>비문자 조각의 현재 측정 높이는 새 줄로 이동하기 전의 줄에 쓴다.</summary>
    [Fact]
    public void ExternalHeightIsAssignedBeforeMovingAndClonesIndependently()
    {
        var s = new StyleBufferScene(); var pictures = new Markup(s.Fragments, new() { TypeCount = 3 }, new() { Type = _ => new() { Token = 2, Shape = 2, FrameCount = 1 }, Frame = (_, _, _) => new(3, 30, 0, 9, 0, 0) }); Markup.BindMarkup(s.StyleHooks, pictures); StyleTextBuffer.BindStyleTextBufferMarkup(s.BufferHooks, pictures);
        s.Buffer.AppendText("A\0"u8, s.Context); s.Buffer.AppendPicture(pictures, 2, 0, 99); s.Buffer.AppendText("\n\0"u8, s.Context); s.Buffer.Layout(5); var rows = s.Rows(); Assert.Equal(new[] { (2, 30, 0), (3, 14, 30) }, rows.Select(r => (r.Width, r.Height, r.Y))); uint original = s.Fragments.Fragments[s.State.HardLines[0]].Next; Assert.NotEqual(original, rows[1].Head); Assert.Equal(99U, s.Fragments.Fragments[rows[1].Head].Metadata); Assert.Equal(44, s.Buffer.TotalHeight());
    }
    /// <summary>잘못된 저장소/개수/토큰 소진을 거부하고 정상 마지막 링크를 유지한다.</summary>
    [Fact]
    public void StorageExhaustionAndCounterContracts()
    {
        var s = new StyleBufferScene(); Assert.Throws<ArgumentException>(() => new StyleTextBuffer(s.State, s.Fragments, s.Style, 8, new())); s.Buffer.AppendText("A\n\0"u8, s.Context); s.State.NextSoftToken = uint.MaxValue; s.Buffer.Layout(9); Assert.Equal(uint.MaxValue, s.State.Sentinel.Next); Assert.Equal(0U, s.State.NextSoftToken); Assert.Equal(0U, s.Rows()[0].Next); s.State.SoftCount = 2; Assert.Throws<ArgumentOutOfRangeException>(() => s.Buffer.MaximumWidth()); Assert.Throws<InvalidOperationException>(() => s.Buffer.Layout(9)); Assert.Empty(s.State.SoftLines);
    }
}
