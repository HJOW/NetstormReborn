using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// <see cref="RawSectionBuffer"/>의 크기·용량 규칙과 <see cref="FortTemplate"/>의 섹션 버퍼 전환(<c>Clone</c>, <c>Section</c>)을 직접 확인한다.
/// 원본 x86 fixture가 없는 계층이라 L1이 아니며, MSVC <c>std::vector</c>의 <c>resize</c>·<c>reserve</c>·이동·<c>swap</c> 규칙을 기대값으로 삼는다.
/// 이 규칙은 Template 저장소가 용량으로 정하는 분기(섹션 존재 여부, 압축 건너뛰기, 쓰기 전 재확장)의 근거다 (0.130 TS0, 2026-11-01).
/// </summary>
public sealed class RawSectionBufferTests
{
    /// <summary>범위 생성과 복사는 용량이 크기와 같고 빈 범위는 용량 0이다.</summary>
    [Fact]
    public void FromBytes_CapacityEqualsCountAndEmptyHasNone()
    {
        RawSectionBuffer filled = RawSectionBuffer.FromBytes([1, 2, 3]);
        Assert.Equal(3, filled.Count);
        Assert.Equal(3, filled.Capacity);
        Assert.Equal(new byte[] { 1, 2, 3 }, filled.ToArray());

        RawSectionBuffer empty = RawSectionBuffer.FromBytes([]);
        Assert.Equal(0, empty.Count);
        Assert.Equal(0, empty.Capacity);

        // 기본 생성도 용량 0이다
        Assert.Equal(0, new RawSectionBuffer().Capacity);
    }

    /// <summary>복사(<c>Clone</c>)는 예약 용량을 따라가지 않고 내용만 독립 복사한다.</summary>
    [Fact]
    public void Clone_CopiesOnlyCountAndDoesNotShare()
    {
        RawSectionBuffer source = RawSectionBuffer.FromBytes([1, 2, 3]);
        source.Reserve(100);
        RawSectionBuffer copy = source.Clone();
        Assert.Equal(100, source.Capacity);
        Assert.Equal(3, copy.Capacity);
        Assert.Equal(source.ToArray(), copy.ToArray());
        // 한쪽을 바꿔도 다른 쪽은 그대로다
        copy[0] = 9;
        Assert.Equal(1, source[0]);
        // 빈 버퍼의 복사는 용량 0이다
        Assert.Equal(0, new RawSectionBuffer().Clone().Capacity);
    }

    /// <summary>용량 안의 <c>Resize</c>는 재할당하지 않고, 줄였다가 다시 늘려도 이전 바이트를 되살리지 않는다.</summary>
    [Fact]
    public void Resize_WithinCapacityZeroFillsGrownRegion()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3, 4]);
        buffer.Resize(2);
        Assert.Equal(2, buffer.Count);
        Assert.Equal(4, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2 }, buffer.ToArray());
        // 줄이기 전에 있던 3, 4가 아니라 0으로 채워진다
        buffer.Resize(4);
        Assert.Equal(4, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2, 0, 0 }, buffer.ToArray());
        // 비운 뒤 다시 늘려도 마찬가지다
        buffer.Clear();
        Assert.Equal(0, buffer.Count);
        Assert.Equal(4, buffer.Capacity);
        buffer.Resize(3);
        Assert.Equal(new byte[] { 0, 0, 0 }, buffer.ToArray());
    }

    /// <summary>용량을 넘는 <c>Resize</c>는 <c>max(용량 + 용량/2, 요청)</c>로 재할당하고 내용을 보존한다.</summary>
    [Fact]
    public void Resize_BeyondCapacityFollowsMsvcGrowth()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
        // 1.5배(15)가 요청(12)보다 크면 1.5배다
        buffer.Resize(12);
        Assert.Equal(12, buffer.Count);
        Assert.Equal(15, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0 }, buffer.ToArray());
        // 다시 용량을 넘으면 15 + 7 = 22가 요청 16보다 크다
        buffer.Resize(16);
        Assert.Equal(22, buffer.Capacity);
        // 요청이 1.5배보다 크면 요청 크기 그대로다
        buffer.Resize(100);
        Assert.Equal(100, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0, 0 }, buffer.Span[..12].ToArray());
        // 빈 버퍼에서는 요청 크기가 용량이 된다
        RawSectionBuffer fresh = new();
        fresh.Resize(5);
        Assert.Equal(5, fresh.Capacity);
        Assert.Equal(new byte[5], fresh.ToArray());
    }

    /// <summary>줄이기 전에 있던 바이트는 용량을 넘는 재할당 뒤에도 되살아나지 않는다(재할당은 현재 크기만큼만 옮긴다).</summary>
    [Fact]
    public void Resize_BeyondCapacityDoesNotResurrectShrunkBytes()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3, 4]);
        buffer.Resize(2);
        buffer.Resize(10);
        Assert.Equal(10, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2, 0, 0, 0, 0, 0, 0, 0, 0 }, buffer.ToArray());
    }

    /// <summary><c>Resize</c>의 범위 오류는 상태를 바꾸지 않는다.</summary>
    [Fact]
    public void Resize_RejectsInvalidSizeWithoutChangingState()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3]);
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer.Resize(-1));
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer.Resize(Array.MaxLength + 1));
        Assert.Equal(3, buffer.Count);
        Assert.Equal(3, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2, 3 }, buffer.ToArray());
    }

    /// <summary><c>Reserve</c>는 요청 용량 그대로 늘리고 줄이지 않으며 크기와 내용을 보존한다.</summary>
    [Fact]
    public void Reserve_GrowsExactlyAndKeepsContent()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3]);
        buffer.Reserve(10);
        Assert.Equal(3, buffer.Count);
        Assert.Equal(10, buffer.Capacity);
        Assert.Equal(new byte[] { 1, 2, 3 }, buffer.ToArray());
        // 작은 요청은 용량을 줄이지 않는다
        buffer.Reserve(5);
        Assert.Equal(10, buffer.Capacity);
        // 예약한 용량 안의 Resize는 재할당하지 않는다
        buffer.Resize(10);
        Assert.Equal(10, buffer.Capacity);
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer.Reserve(-1));
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer.Reserve(Array.MaxLength + 1));
        // 빈 버퍼의 예약은 크기를 0으로 둔다 (용량만 있고 크기가 0인 상태가 존재해야 한다)
        RawSectionBuffer reserved = new();
        reserved.Reserve(7);
        Assert.Equal(0, reserved.Count);
        Assert.Equal(7, reserved.Capacity);
    }

    /// <summary><c>Release</c>는 크기와 용량을 모두 0으로 돌려 "섹션 없음" 상태로 만든다.</summary>
    [Fact]
    public void Release_DropsCountAndCapacity()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3]);
        buffer.Release();
        Assert.Equal(0, buffer.Count);
        Assert.Equal(0, buffer.Capacity);
        Assert.True(buffer.Span.IsEmpty);
    }

    /// <summary>이동 대입은 배열을 넘기고 원본을 용량 0으로 비우며, 자기 자신에게는 아무 일도 하지 않는다.</summary>
    [Fact]
    public void TakeFrom_MovesStorageAndEmptiesSource()
    {
        RawSectionBuffer source = RawSectionBuffer.FromBytes([1, 2, 3]);
        source.Reserve(8);
        RawSectionBuffer target = RawSectionBuffer.FromBytes([9, 9]);
        target.TakeFrom(source);
        Assert.Equal(new byte[] { 1, 2, 3 }, target.ToArray());
        Assert.Equal(8, target.Capacity);
        Assert.Equal(0, source.Count);
        Assert.Equal(0, source.Capacity);

        // 같은 객체에 대한 이동은 내용을 지우지 않는다
        target.TakeFrom(target);
        Assert.Equal(new byte[] { 1, 2, 3 }, target.ToArray());
        Assert.Equal(8, target.Capacity);
        Assert.Throws<ArgumentNullException>(() => target.TakeFrom(null!));
    }

    /// <summary>인덱서는 용량 안이라도 크기 밖이면 거부하고, <c>Span</c>·<c>Memory</c>는 크기만큼만 빌려 준다.</summary>
    [Fact]
    public void IndexerAndViewsStopAtCount()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3, 4]);
        buffer.Resize(2);
        Assert.Equal(2, buffer[1]);
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer[2]);
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer[-1]);
        Assert.Throws<ArgumentOutOfRangeException>(() => buffer[2] = 7);
        Assert.Equal(2, buffer.Span.Length);
        Assert.Equal(2, buffer.Memory.Length);
        // 빌린 범위에 쓰면 같은 버퍼에 반영된다
        buffer.Span[0] = 8;
        buffer.Memory.Span[1] = 9;
        Assert.Equal(new byte[] { 8, 9 }, buffer.ToArray());
    }

    /// <summary>
    /// 재할당 뒤에는 이전에 빌린 <c>Memory</c>가 옛 배열을 가리킨다. C++에서는 정의되지 않은 동작이지만 C#에서는 쓰기가 조용히 사라지므로,
    /// 이 동작을 고정해 두어 호출자가 재할당을 일으키는 호출을 넘어 빌린 범위를 쓰지 않도록 상기시킨다.
    /// </summary>
    [Fact]
    public void Reallocation_LeavesEarlierBorrowOnOldArray()
    {
        RawSectionBuffer buffer = RawSectionBuffer.FromBytes([1, 2, 3]);
        Memory<byte> borrowed = buffer.Memory;
        buffer.Resize(1000);
        borrowed.Span[0] = 99;
        Assert.Equal(1, buffer[0]);
        // 용량 안에서는 재할당하지 않으므로 같은 배열이다
        Memory<byte> sameArray = buffer.Memory;
        buffer.Resize(10);
        sameArray.Span[0] = 77;
        Assert.Equal(77, buffer[0]);
    }

    /// <summary>
    /// 파일 읽기는 섹션 버퍼의 용량을 크기와 같게 만들며(빈 섹션은 용량 0), 이름 조회는 크기만큼의 범위를 돌려준다.
    /// 합성 파일: 'F', flag 0xfe, Subscriber 3바이트, State 1바이트, Mission 빈 섹션.
    /// </summary>
    [Fact]
    public void ReadSections_CreatesBuffersWithCapacityEqualToCount()
    {
        byte[] file = [0x46, 0xfe, 0x05, 0x00, 1, 2, 3, 0x03, 0x00, 9, 0x02, 0x00];
        FortTemplate fort = FortTemplate.ReadSections(file);
        Assert.Equal(3, fort.Sections.Count);
        Assert.Equal(0xfe, fort.Flag);
        Assert.Equal(new[] { 3, 1, 0 }, fort.Sections.Select(section => section.Count));
        Assert.Equal(new[] { 3, 1, 0 }, fort.Sections.Select(section => section.Capacity));
        Assert.Equal(new byte[] { 9 }, fort.Section("state").ToArray());
        Assert.True(fort.Section("Mission").IsEmpty);
        // 파일에 없는 섹션도 빈 범위다
        Assert.True(fort.Section("Money").IsEmpty);
    }

    /// <summary>
    /// <c>FortTemplate.Clone</c>은 flag·뒤꼬리·타입 해시·변환표·섹션을 모두 독립적으로 복사하고, 섹션 용량은 크기와 같아진다.
    /// </summary>
    [Fact]
    public void FortTemplateClone_CopiesEverythingWithoutSharing()
    {
        byte[] file = [0x46, 0x00, 0x05, 0x00, 1, 2, 3, 0x02, 0x00, 0xee];
        FortTemplate source = FortTemplate.ReadSections(file);
        source.Sections[0].Reserve(50);
        source.TypeHashes.AddRange([11u, 22u]);
        source.HasTypeConversion = true;
        source.TypeConversion[5] = 77;

        FortTemplate copy = source.Clone();
        Assert.Equal(source.Flag, copy.Flag);
        Assert.Equal(source.TrailingBytes, copy.TrailingBytes);
        Assert.Equal(1, copy.TrailingBytes);
        Assert.True(copy.HasTypeConversion);
        Assert.Equal(new[] { 11u, 22u }, copy.TypeHashes);
        Assert.Equal(77, copy.ConvertType(5));
        Assert.Equal(2, copy.Sections.Count);
        // 원본의 예약 용량(50)은 따라가지 않는다
        Assert.Equal(new[] { 3, 0 }, copy.Sections.Select(section => section.Capacity));
        Assert.Equal(new byte[] { 1, 2, 3 }, copy.Sections[0].ToArray());

        // 한쪽을 바꿔도 다른 쪽은 그대로다
        copy.Sections[0][0] = 100;
        copy.TypeHashes[0] = 99;
        copy.TypeConversion[5] = 1;
        copy.Flag = 0xfe;
        Assert.Equal(1, source.Sections[0][0]);
        Assert.Equal(11u, source.TypeHashes[0]);
        Assert.Equal(77, source.TypeConversion[5]);
        Assert.Equal(0, source.Flag);
        Assert.NotSame(source.Sections[0], copy.Sections[0]);
    }
}
