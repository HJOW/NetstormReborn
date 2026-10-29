using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 실제 소유 메모리와 편집/선택/줄 표/클립보드 연결 검사다.

/// <summary>원본570행 대조와 별개인 실제 저장소 통합/유효 포인터 계약 검사.</summary>
public sealed class TextEntryStorageTests
{
    /// <summary>원본 BYTE 뷰를 관측용 문자열로 읽는다.UTF-8로 재해석하지 않는다.</summary>
    private static string Text(TextEntryBuffer buffer) => System.Text.Encoding.Latin1.GetString(buffer.Text());
    /// <summary>실제 LF 줄 표는CR을 보존하고 끝 빈 줄/선택 구간과 용량 거부를 구별한다.</summary>
    [Fact]
    public void ActualLinesSelectionAndUnchangedFailedOutputs()
    {
        var storage = new TextEntryStorage(); var b = storage.Create(1); b.Construct(); b.Reset("A\rB\nC\n\0"u8); b.Select(5, 1); Assert.Equal(3, b.LineCount()); Assert.Equal(new[] { 4, 2, 0 }, new[] { b.LineLength(0), b.LineLength(1), b.LineLength(2) }); byte[] output = Enumerable.Repeat((byte)0x6d, 8).ToArray(); int[] selection = [91, 92]; Assert.Equal(4U, b.CopyLine(0, output, selection)); Assert.Equal(new byte[] { 65, 13, 66, 10, 0, 0x6d, 0x6d, 0x6d }, output); Assert.Equal(new[] { 1, 4 }, selection); Assert.Equal(2U, b.CopyLine(1, output, selection)); Assert.Equal(new[] { 0, 1 }, selection); b.Select(6, 6); Assert.Equal(0U, b.CopyLine(2, output, selection)); Assert.Equal(new[] { 0, 0 }, selection); output.AsSpan().Fill(0x6d); selection[0] = 91; selection[1] = 92; Assert.Equal(uint.MaxValue, b.CopyLine(0, output.AsSpan(0, 4), selection)); Assert.All(output, value => Assert.Equal(0x6d, value)); Assert.Equal(new[] { 91, 92 }, selection); Assert.Equal(uint.MaxValue, b.CopyAll(output.AsSpan(0, 6))); b.Destroy(); storage.ReleaseBuffer(1); Assert.Equal((0, 0), (storage.BufferCount, storage.AllocationCount));
    }
    /// <summary>빈 선택 복사/실제 선택 Cut과 제외 문자 Paste가 같은 실제 본문을 사용한다.</summary>
    [Fact]
    public void ActualClipboardCopyCutAndFilteredPaste()
    {
        byte[] clipboard = "A\n\rB\t\0"u8.ToArray(); var copied = new List<string>(); var storage = new TextEntryStorage(target => clipboard.CopyTo(target), source => copied.Add(System.Text.Encoding.Latin1.GetString(source))); var b = storage.Create(1); b.Construct(); b.Reset("one two\0"u8); b.Copy(); Assert.Equal(7, b.State.SelectionBegin); b.Select(3, 0); b.Cut(); Assert.Equal(new[] { "one two", "one" }, copied); Assert.Equal(" two", Text(b)); Assert.Equal((0, 0), (b.State.SelectionBegin, b.State.SelectionEnd)); b.Paste("\n\t\0"u8); Assert.Equal("A\rB two", Text(b)); Assert.Equal(3, b.State.SelectionBegin); clipboard = [0]; b.State.Dirty = 0; b.Paste(); Assert.Equal(1U, b.State.Dirty); Assert.Equal("A\rB two", Text(b)); b.Destroy(); storage.ReleaseBuffer(1);
    }
    /// <summary>실제 재할당은 내용을 보존하고 이전 주소는 무효가 된다.버퍼 객체와 상태 주소는 유지한다.</summary>
    [Fact]
    public void ActualGrowthAndStableObject()
    {
        var storage = new TextEntryStorage(); var b = storage.Create(1); b.Construct(); uint old = b.State.Data; var state = b.State; byte[] input = [.. Enumerable.Repeat((byte)65, 2050), 0]; b.Reset(input); Assert.Same(b, storage.Find(1)); Assert.Same(state, b.State); Assert.NotEqual(old, b.State.Data); Assert.Equal((2050, 3072), (b.State.Length, b.State.Capacity)); Assert.True(b.Text().SequenceEqual(input.AsSpan(0, 2050))); Assert.Equal(0, b.Data().Span[2050]); Assert.Throws<ArgumentOutOfRangeException>(() => storage.Bytes(old)); Assert.Equal(2, storage.AllocationCount); b.Destroy(); storage.ReleaseBuffer(1);
    }
    /// <summary>실제131줄의 줄 표 확장은 앞선 offset을 보존하고 이전 표를 반납한다.</summary>
    [Fact]
    public void ActualOffsetExpansionAndRelease()
    {
        var storage = new TextEntryStorage(); var b = storage.Create(1); b.Construct(); uint old = b.State.Offsets; byte[] text = System.Text.Encoding.Latin1.GetBytes(string.Concat(Enumerable.Repeat("A\n", 130)) + "\0"); b.Reset(text); Assert.Equal(131, b.LineCount()); Assert.Equal(256, b.State.OffsetCapacity); Assert.NotEqual(old, b.State.Offsets); Assert.Throws<ArgumentOutOfRangeException>(() => storage.Bytes(old)); Assert.Equal((2, 0), (b.LineLength(129), b.LineLength(130))); Assert.Equal(2, storage.AllocationCount); b.Destroy(); storage.ReleaseBuffer(1);
    }
    /// <summary>입력과 목적지가 겹칠 때 REP DWORD 순서를 보존하고 자기 본문 Reset은 첫NUL 쓰기를 먼저 반영한다.</summary>
    [Fact]
    public void ActualOverlappingInputAndSelfReset()
    {
        var storage = new TextEntryStorage(); var b = storage.Create(1); b.Construct(); b.Reset("abcdefghijkl\0"u8); b.Select(1, 1); b.Insert(b.Data().Span); Assert.Equal("aabcddfghhjklbcdefghijkl", Text(b)); Assert.Equal((24, 13, 13), (b.State.Length, b.State.SelectionBegin, b.State.SelectionEnd)); b.Reset(b.Data().Span); Assert.Equal("", Text(b)); Assert.Equal((0, 0, 0), (b.State.Length, b.State.SelectionBegin, b.State.SelectionEnd)); b.Destroy(); storage.ReleaseBuffer(1);
    }
    /// <summary>요청 폭과2048 capacity 필드는 구별하며 범위 오류 앞의 길이/선택/본문 쓰기는 남긴다.</summary>
    [Fact]
    public void ActualRequestedSizeAndPartialOutOfBoundsWrite()
    {
        var storage = new TextEntryStorage(); var b = storage.Create(1); b.Construct(16); Assert.Equal((2048, 16), (b.State.Capacity, b.Data().Length)); b.Reset("123456789012345\0"u8); Assert.Throws<ArgumentOutOfRangeException>(() => b.Insert("X\0"u8)); Assert.Equal((16, 16, 16), (b.State.Length, b.State.SelectionBegin, b.State.SelectionEnd)); Assert.Equal(88, b.Data().Span[15]); b.Destroy(); storage.ReleaseBuffer(1);
    }
    /// <summary>같은 주소+offset/끝 뷰·새 재할당 보존·0크기와 객체 중복/반납 순서를 검사한다.</summary>
    [Fact]
    public void ActualAddressAndOwnershipContracts()
    {
        var storage = new TextEntryStorage(); uint address = storage.Allocate(4); new byte[] { 1, 2, 3, 4 }.CopyTo(storage.Bytes(address).Span); Assert.Equal(new byte[] { 3, 4 }, storage.Bytes(address + 2).ToArray()); Assert.Empty(storage.Bytes(address + 4).ToArray()); Assert.Throws<InvalidOperationException>(() => storage.Release(address + 1)); uint next = storage.Resize(address, 8); Assert.Equal(new byte[] { 1, 2, 3, 4 }, storage.Bytes(next).Span[..4].ToArray()); Assert.Throws<ArgumentOutOfRangeException>(() => storage.Bytes(address)); Assert.Equal(0U, storage.Resize(next, 0)); storage.Release(0); uint zero = storage.Allocate(0); Assert.Empty(storage.Bytes(zero).ToArray()); storage.Release(zero); Assert.Throws<OverflowException>(() => storage.Allocate(uint.MaxValue)); var b = storage.Create(7); Assert.Throws<InvalidOperationException>(() => storage.Create(7)); b.Construct(); Assert.Throws<InvalidOperationException>(() => storage.ReleaseBuffer(7)); b.Destroy(); storage.ReleaseBuffer(7); Assert.Null(storage.Find(7)); Assert.Equal((0, 0), (storage.BufferCount, storage.AllocationCount));
    }
    /// <summary>data0이면 원문 Destroy가 줄 표를 남기는 부분 상태와 미연결 클립보드의 도달 오류를 유지한다.</summary>
    [Fact]
    public void NullDataDestroyAndReachedClipboardContracts()
    {
        var storage = new TextEntryStorage(); var b = storage.Create(1); uint offsets = storage.Allocate(512); b.State.Offsets = offsets; b.Destroy(); Assert.Equal(offsets, b.State.Offsets); Assert.Equal(1, storage.AllocationCount); Assert.Throws<InvalidOperationException>(() => storage.ReleaseBuffer(1)); storage.Release(offsets); b.State.Offsets = 0; storage.ReleaseBuffer(1); var other = storage.Create(2); other.Construct(); other.Reset(); other.Copy(); Assert.Throws<InvalidOperationException>(() => other.Paste()); other.Reset("A\0"u8); Assert.Throws<InvalidOperationException>(() => other.Copy()); other.Destroy(); storage.ReleaseBuffer(2); Assert.Equal(0, storage.AllocationCount);
    }
}
