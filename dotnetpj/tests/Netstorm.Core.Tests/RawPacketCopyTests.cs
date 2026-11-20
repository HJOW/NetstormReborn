using System.Buffers.Binary;
using System.Buffers;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 독립 원본 관측과 실제 공유표/배열의 소유 및 중첩 경계를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 순방향 복제·현재observer·실제 복제 소유를 검증한다.</summary>
public sealed class RawPacketCopyTests
{
    /// <summary>실제 배열을복제하지않고각물리읽기시점의장치호출수명을관찰한다.</summary>
    private sealed class ReadObservedMemory : MemoryManager<byte>
    {
        /// <summary>검사가소유한같은실제배열이다.</summary>
        private readonly byte[] _bytes;
        /// <summary>Span대여시점의현재pin상태만관찰하며바이트결과를만들지않는다.</summary>
        private readonly Action _read;
        /// <summary>동일배열과읽기관찰을빌린다.</summary>
        public ReadObservedMemory(byte[] bytes, Action read) { _bytes = bytes; _read = read; }
        /// <summary>원래배열의같은물리만반환한다.</summary>
        public override Span<byte> GetSpan() { _read(); return _bytes; }
        /// <summary>이번관리배열검사는네이티브포인터대여를요구하지않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException("검사에없는Native핀");
        /// <summary>실제발급하지않은포인터의반납은거부한다.</summary>
        public override void Unpin() => throw new NotSupportedException("검사에없는Native핀반납");
        /// <summary>배열owner는호출자이며반납할외부자원은없다.</summary>
        protected override void Dispose(bool disposing) { }
    }
    /// <summary>실제observer 뒤에만 선언 길이를 다시 읽는 원본4836입력을 대조한다.</summary>
    [Fact]
    public void OriginalPacketCopy4836()
    {
        string[][] rows = X86Fixture.Read("packet-copy"); Assert.Equal(4836, rows.Length);
        var table = new PacketRegistrationTable(OriginalEdition.Patch1078);
        // 모든행은 동일 원본 입력창과현재 callback을 새로 설정한다.
        foreach (string[] row in rows)
        {
            int[] input = row[0].Split(',').Select(X86Fixture.Int).ToArray();
            byte[] buffer = Enumerable.Range(0, 256).Select(index => unchecked((byte)(index * 17 + 3))).ToArray();
            buffer[65] = 0x58; BinaryPrimitives.WriteUInt16LittleEndian(buffer.AsSpan(66), (ushort)input[3]);
            var effects = new List<string>();
            table.Registration(0x58).PacketObserver = input[4] == 0 ? null : (packet, direction) =>
            {
                Assert.Equal(1u, direction); effects.Add("O:1");
                if (input[4] == 2) { BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], 9); packet.Span[1] = 0; }
                else if (input[4] == 3) BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], 0);
                else if (input[4] == 4) { BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], 31); packet.Span[8] = 0xee; }
            };
            Memory<byte>? source = input[0] < 0 ? (Memory<byte>?)null : buffer.AsMemory(input[0]);
            Memory<byte>? destination = input[2] < 0 ? (Memory<byte>?)null : buffer.AsMemory(input[2]);
            Assert.Equal(input[0] >= 0, source.HasValue); Assert.Equal(input[2] >= 0, destination.HasValue);
            RawPacketCopyResult copy = RawPacketCopy.Duplicate(source, input[1], destination, table);
            int allocated = -1, returned = copy.Present ? input[2] : -1;
            if (copy.Allocation is { } allocation)
            {
                allocated = allocation.Length; returned = 192; effects.Add("A:" + allocated); copy.Bytes.CopyTo(buffer.AsMemory(192));
            }
            Assert.Equal(X86Fixture.Int(row[1]), returned); Assert.Equal(X86Fixture.Int(row[2]), allocated);
            Assert.Equal(row[3], effects.Count == 0 ? "-" : string.Join(';', effects));
            Assert.Equal(X86Fixture.Bytes(row[4]), buffer);
        }
    }
    /// <summary>NULL과길이0을구별하고 observer예외·현재길이·부분복사의 실제 물리오류 prefix를 보존한다.</summary>
    [Fact]
    public void CopyPreservesNullZeroLengthAndPhysicalFailurePrefixes()
    {
        var table = new PacketRegistrationTable(OriginalEdition.Patch1078);
        Assert.False(RawPacketCopy.Duplicate(null, int.MinValue, null, table).Present);
        byte[] source = [0, 0x58, 8, 0, 9, 10]; byte[] destination = [0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc];
        Assert.ThrowsAny<Exception>(() => RawPacketCopy.Duplicate(source, 8, destination, table));
        Assert.Equal(new byte[] { 0, 0x58, 8, 0, 0xcc, 0xcc, 0xcc, 0xcc }, destination);
        int called = 0;
        table.Registration(0x58).PacketObserver = (packet, _) => { called++; packet.Span[0] = 77; throw new InvalidOperationException("명시 observer 중단"); };
        Assert.Throws<InvalidOperationException>(() => RawPacketCopy.Duplicate(source, -1, destination, table));
        Assert.Equal(1, called); Assert.Equal(77, source[0]);
        table.Registration(0x58).PacketObserver = (packet, _) => { called++; BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], 0); };
        RawPacketCopyResult ownedZero = RawPacketCopy.Duplicate(source, 0, null, table);
        Assert.True(ownedZero.Present); Assert.NotNull(ownedZero.Allocation); Assert.Empty(ownedZero.Allocation);
        RawPacketCopyResult nullZero = RawPacketCopy.Duplicate(source, 1, null, table);
        Assert.False(nullZero.Present); Assert.Null(nullZero.Allocation); Assert.Empty(nullZero.Bytes.ToArray());
        RawPacketCopyResult borrowedZero = RawPacketCopy.Duplicate(source, 0, destination, table);
        Assert.True(borrowedZero.Present); Assert.Null(borrowedZero.Allocation); Assert.Equal(4, called);
        Assert.ThrowsAny<Exception>(() => RawPacketCopy.Duplicate(Memory<byte>.Empty, 0, null, table));
    }
    /// <summary>선택한 observer 세대를 복제 완료까지 pin하고 반납 후 새등록을 기존 snapshot으로 덮지 않는다.</summary>
    [Fact]
    public void CopyKeepsSelectedSessionPinnedUntilLastByteAndRejectsRebindInsideObserver()
    {
        var table = new PacketRegistrationTable(OriginalEdition.Patch1078); var owner = new object();
        using PacketRegistrationLease lease = table.BindSession(owner);
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new()
        {
            Now = () => throw new InvalidOperationException("복제에서 시계 조회"), Record = (_, _, _) => throw new InvalidOperationException("복제에서 수신 기록"),
            Terminate = _ => throw new InvalidOperationException("복제에서 종료 통지"), Close = _ => throw new InvalidOperationException("복제에서 노드 닫힘"), Release = _ => throw new InvalidOperationException("복제에서 노드 반납"),
        }, table);
        byte[] source = [0, 0x58, 5, 0, 77]; int called = 0, readsAfterObserver = 0; bool observerReturned = false;
        using var memory = new ReadObservedMemory(source, () =>
        {
            if (!observerReturned) return;
            readsAfterObserver++; Assert.Throws<InvalidOperationException>(() => table.BindSession(new object()));
        });
        dispatch.ObservePacket(0x58, (packet, direction) =>
        {
            called++; Assert.Equal(1u, direction); lease.Dispose();
            Assert.Throws<InvalidOperationException>(() => table.BindSession(new object()));
            BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], 4);
            observerReturned = true;
        });
        RawPacketCopyResult copy = RawPacketCopy.Duplicate(memory.Memory, 0, null, dispatch);
        Assert.True(readsAfterObserver >= 2); Assert.Equal(1, called); Assert.Equal(new byte[] { 0, 0x58, 4, 0 }, copy.Bytes.ToArray());
        using PacketRegistrationLease second = table.BindSession(new object());
        Assert.Null(table.Registration(0x58).PacketObserver);
    }
    /// <summary>완성된 실제 배열의 소유권·Acquire 대여·NULL·중복해제·비재사용 토큰을 확인한다.</summary>
    [Fact]
    public void OwnedCopiesKeepActualAllocationAndDoNotReuseReleasedTokens()
    {
        var copies = new RawPacketCopies(); var table = new PacketRegistrationTable(OriginalEdition.Patch1078);
        Assert.Equal(0u, copies.Duplicate(null, table)); copies.Release(0);
        byte[] source = [0, 0x58, 5, 0, 77]; uint first = copies.Duplicate(source, table);
        byte[] borrowed = copies.Acquire(first); Assert.NotSame(source, borrowed); Assert.Equal(source, borrowed);
        source[4] = 1; Assert.Equal(77, borrowed[4]); copies.Release(first);
        Assert.Equal(77, borrowed[4]); Assert.Throws<ArgumentOutOfRangeException>(() => copies.Acquire(first));
        Assert.Throws<ArgumentOutOfRangeException>(() => copies.Release(first));
        uint adopted = copies.Adopt(source); Assert.True(adopted > first); Assert.Same(source, copies.Acquire(adopted));
        source[4] = 2; Assert.Equal(2, copies.Acquire(adopted)[4]); copies.Release(adopted);
    }
}
