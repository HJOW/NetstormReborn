using System.Buffers;
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 공유 표의 기본 등록과 원문 지정 폭/진단 뒤 현재 읽기를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>기본18행 및9종 mutable 관찰기의 선택/보존 경계를 확인한다.</summary>
public sealed class ClientPacketDefaultObserversTests
{
    /// <summary>Span 조회 자체를 거부해 수신 방향의 지연 관문을 관측한다.</summary>
    private sealed class Unreadable : MemoryManager<byte>
    {
        /// <summary>Span을 조회하지 않고 실제 관리 메모리 핸들만 제공한다.</summary>
        public Memory<byte> Buffer => CreateMemory(64);
        /// <summary>선택하지 않은 물리에 접근하면 즉시 실패한다.</summary>
        public override Span<byte> GetSpan() => throw new InvalidOperationException("미선택 물리 조회");
        /// <summary>이 관측에서 네이티브 주소 고정을 허용하지 않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();
        /// <summary>고정된 포인터가 존재하지 않는다.</summary>
        public override void Unpin() => throw new NotSupportedException();
        /// <summary>외부 물리를 할당하지 않았으므로 반납 효과가 없다.</summary>
        protected override void Dispose(bool disposing) { }
    }
    /// <summary>실제 지정 폭만 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 원문 필드 바깥은 보존한다.
        for (int index = 0; index < width; index++) bytes[offset + index] = (byte)(value >> (index * 8));
    }
    /// <summary>게임 장치 없이 실제 기본 길이표/등록표를 만든다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>PE 검증을 마친 기본 길이표를 재사용하고18행 밖/관찰 전용 수신기를 보존한다.</summary>
    [Fact]
    public void InstallationPreservesOtherRowsAndUsesActualEmptyNativeFunctions()
    {
        var dispatch = Dispatch(); ushort[] peLengths = Enumerable.Range(0, 138).Select(type => dispatch.Registrations.Registration((uint)type).Length).ToArray();
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var surface = new ClientSurfaceGlobals(game, new(local), new Kernel(), () => 0);
        int keptCalls = 0; dispatch.RegisterCanonicalDirect(0x16, 0x1234, (_, _, _) => keptCalls++); var kept = dispatch.SnapshotReceiver(0x16);
        dispatch.RegisterCanonicalDirect(0x20, 0x5678, (_, _, _) => throw new InvalidOperationException("18행 밖 호출")); dispatch.Observe(0x20, _ => throw new InvalidOperationException("18행 밖 관찰")); dispatch.Version(0x20, 7, 9); dispatch.RequireLength(0x20, 59); var other = dispatch.SnapshotReceiver(0x20); var otherObserver = dispatch.Registration(0x20).Observer;
        ClientPacketDefaultObservers.Install18(dispatch, surface.Storage); Assert.Equal(0u, game.PlayerRecords.Base);
        uint[] types = [0x0c,0x42,0x4d,0x55,0x56,0x57,0x5a,0x10,0x11,0x12,0x16,0x17,0x1c,0x1e,0x1f,0x68,0x85,0x87];
        uint[] c3 = [0x0c,0x42,0x4d,0x55,0x56,0x57,0x5a], mutable = [0x0c,0x55,0x56,0x57,0x16,0x1c,0x85];
        // 독립 PE 길이와 비NULL 빈observer/패킷observer의 종류를18행마다 구별한다.
        foreach (uint type in types)
        {
            var row = dispatch.Registration(type); Assert.Equal(peLengths[type], row.Length); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch);
            Assert.Equal(mutable.Contains(type), row.PacketObserver is not null); Assert.Equal(!mutable.Contains(type), row.Observer is not null);
            if (c3.Contains(type)) { Assert.Equal(0x004437f0u, dispatch.SnapshotReceiver(type)!.NativeEntry); dispatch.InvokeDirect(type, uint.MaxValue, null, ReadOnlyMemory<byte>.Empty); }
            else if (type == 0x16) Assert.Same(kept, dispatch.SnapshotReceiver(type)); else Assert.Null(dispatch.SnapshotReceiver(type));
        }
        dispatch.InvokeDirect(0x16, 0, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(1, keptCalls);
        Assert.Same(other, dispatch.SnapshotReceiver(0x20)); Assert.Same(otherObserver, dispatch.Registration(0x20).Observer); Assert.Equal(59, dispatch.Registration(0x20).Length); Assert.Equal(7, dispatch.Registration(0x20).Version); Assert.Equal(9, dispatch.Registration(0x20).Mismatch);
        var savedRet = dispatch.SnapshotReceiver(0x42)!; dispatch.RegisterCanonicalDirect(0x42, 0x9999, (_, _, _) => throw new InvalidOperationException("새 표의 다른 F")); savedRet.Invoke(0, null, ReadOnlyMemory<byte>.Empty);
    }
    /// <summary>다른 폭/감김/signed BYTE 결과와 부분 버퍼의 전체 물리를 고정 값에 대조한다.</summary>
    [Theory]
    [InlineData(0x0b,8,4,0x10002u,14)] [InlineData(0x0c,8,4,0xffffu,11)]
    [InlineData(0x55,4,4,0xffffu,7)] [InlineData(0x56,8,4,0x10002u,14)] [InlineData(0x57,8,4,0xffffu,11)]
    [InlineData(0x16,5,1,0x80u,65038)] [InlineData(0x1c,45,1,0x80u,174)] [InlineData(0x85,5,2,0xfffeu,5)] [InlineData(0x7b,9,1,0x80u,65418)]
    public void WidthWrapAndSignedByteKeepWholePartialStorage(uint type, int offset, int width, uint value, ushort expected)
    {
        byte[] all = Enumerable.Repeat((byte)0xd3, 80).ToArray(); Memory<byte> packet = all.AsMemory(7, 64); Put(packet.Span, offset, value, width); byte[] before = all.ToArray();
        ClientPacketDefaultObservers.Observe(type, packet, 1); BinaryPrimitives.WriteUInt16LittleEndian(before.AsSpan(9, 2), expected); Assert.Equal(before, all);
    }
    /// <summary>진단이 정상 복귀하면서 필드를 바꾸면 같은 현재 값을 지정 폭으로 다시 읽는다.</summary>
    [Theory]
    [InlineData(0x0b,8,4,0x36c,14)] [InlineData(0x0c,8,4,0x38c,14)] [InlineData(0x55,4,4,0x8dc,10)]
    [InlineData(0x56,8,4,0x8f8,14)] [InlineData(0x57,8,4,0x916,14)] [InlineData(0x16,5,1,0x458,22)]
    [InlineData(0x1c,45,1,0x4ca,48)] [InlineData(0x85,5,2,0xa2c,9)] [InlineData(0x7b,9,1,0xba8,12)]
    public void AssertionRereadsCurrentField(uint type, int offset, int width, uint line, ushort expected)
    {
        byte[] all = Enumerable.Repeat((byte)0xd3, 80).ToArray(); Memory<byte> packet = all.AsMemory(7, 64); Put(packet.Span, offset, uint.MaxValue, width); int assertions = 0;
        ClientPacketDefaultObservers.Observe(type, packet, uint.MaxValue, (expression, file, actual) => { assertions++; Assert.NotEmpty(expression); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(line, actual); Assert.Equal(0xd3d3, BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(2, 2))); Put(packet.Span, offset, 2, width); });
        Assert.Equal(1, assertions); Assert.Equal(expected, BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(2, 2))); Assert.All(all.Take(7), value => Assert.Equal(0xd3, value)); Assert.All(all.Skip(71), value => Assert.Equal(0xd3, value));
    }
    /// <summary>수신 방향은 미등록 종류/Span 접근/진단을 요구하지 않는다.</summary>
    [Fact]
    public void IncomingSkipsEvenSpanLookup()
    {
        using var memory = new Unreadable(); ClientPacketDefaultObservers.Observe(uint.MaxValue, memory.Buffer, 0); var dispatch = Dispatch(); var game = new ClientGameplayGlobals(); var surface = new ClientSurfaceGlobals(game, new(new ClientLocalStateGlobals(game, new())), new Kernel(), () => 0); ClientPacketDefaultObservers.Install18(dispatch, surface.Storage); dispatch.Registration(0x0c).NotifyObserver(memory.Buffer, 0);
    }
    /// <summary>실제 sentinel 진단 누락/중단은 길이 기록보다 먼저 실패한다.</summary>
    [Fact]
    public void MissingOrStoppedAssertionDoesNotWriteLength()
    {
        byte[] packet = Enumerable.Repeat((byte)0xff, 16).ToArray(); byte[] before = packet.ToArray(); Assert.Throws<InvalidOperationException>(() => ClientPacketDefaultObservers.Observe(0x85, packet, 1)); Assert.Equal(before, packet);
        Assert.Throws<IOException>(() => ClientPacketDefaultObservers.Observe(0x85, packet, 1, (_, _, _) => throw new IOException("선택 진단 중단"))); Assert.Equal(before, packet);
    }
}
