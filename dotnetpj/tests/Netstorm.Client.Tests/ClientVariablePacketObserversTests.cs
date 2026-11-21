using System.Buffers;
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 원문 가변 관찰기의 현재 물리·진단·송수신 관문을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>다섯 기본 송신 관찰기의 실제 표 설치와 부호/폭/부분 효과를 확인한다.</summary>
public sealed class ClientVariablePacketObserversTests
{
    /// <summary>수신 관문이 메모리 조회보다 먼저 실행되는지 관측한다.</summary>
    private sealed class Unreadable : MemoryManager<byte>
    {
        /// <summary>물리 Span을 요구하지 않는 메모리 핸들이다.</summary>
        public Memory<byte> Buffer => CreateMemory(64);
        /// <summary>미선택 물리에 접근하면 검사에 실패한다.</summary>
        public override Span<byte> GetSpan() => throw new InvalidOperationException("미선택 Span 조회");
        /// <summary>호스트 포인터 고정은 이 검사에서 사용하지 않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();
        /// <summary>고정된 포인터가 없다.</summary>
        public override void Unpin() => throw new NotSupportedException();
        /// <summary>소유한 외부 자원이 없다.</summary>
        protected override void Dispose(bool disposing) { }
    }

    /// <summary>관측 필드의 지정 폭만 기록하고 주변 물리를 보존한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 필드의 낮은 바이트부터 원문 폭만 기록한다.
        for (int index = 0; index < width; index++) bytes[offset + index] = (byte)(value >> (index * 8));
    }

    /// <summary>세션과 전송 장치 없이 실제 등록표를 만든다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>원문 지정 폭/부호/감김만 적용하고 전체 입력의 길이WORD 외 물리를 보존한다.</summary>
    [Theory]
    [InlineData(0x3f,26,1,0xfeu,535)]
    [InlineData(0x30,13,2,0x8000u,32792)]
    [InlineData(0x30,13,2,0xffe0u,65528)]
    [InlineData(0x7a,8,1,0xfeu,263)]
    [InlineData(0x5f,6,4,0x1234ffffu,8)]
    [InlineData(0x5f,6,4,0x10002u,14)]
    [InlineData(0x7d,16,2,0xfffeu,18)]
    public void OutgoingUsesOriginalFieldWidthsAndPreservesWholeStorage(uint type, int offset, int width, uint value, ushort expected)
    {
        byte[] all = Enumerable.Repeat((byte)0xd3, 80).ToArray();
        Memory<byte> packet = all.AsMemory(7, 64);
        Put(packet.Span, offset, value, width);
        byte[] expectedStorage = all.ToArray();
        BinaryPrimitives.WriteUInt16LittleEndian(expectedStorage.AsSpan(9, 2), expected);
        ClientVariablePacketObservers.Observe(type, packet, 1);
        Assert.Equal(expectedStorage, all);
    }

    /// <summary>복귀형 진단 뒤 현재 물리를 다시 읽고 실제 진단 인자를 보존한다.</summary>
    [Theory]
    [InlineData(0x3f,26,1,"sidCount != (unsigned char)-1",0x7c9,31)]
    [InlineData(0x30,13,2,"nodeCount != (short)-1",0x716,26)]
    [InlineData(0x7a,8,1,"dataLength != (unsigned char)-1",0xb83,11)]
    [InlineData(0x5f,6,4,"count != (int)-1",0x96b,14)]
    public void ReturningAssertionRereadsSameCurrentField(uint type, int offset, int width, string expression, uint line, ushort expected)
    {
        byte[] packet = Enumerable.Repeat((byte)0xff, 64).ToArray();
        int calls = 0;
        ClientVariablePacketObservers.Observe(type, packet, uint.MaxValue, (actualExpression, file, actualLine) =>
        {
            calls++;
            Assert.Equal(expression, actualExpression); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(line, actualLine);
            Assert.Equal(ushort.MaxValue, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
            Put(packet, offset, 2, width);
        });
        Assert.Equal(1, calls);
        Assert.Equal(expected, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
    }

    /// <summary>같은 실제 표의 수신기와 다른 행을 보존하고 설치 뒤 재연결한 현재 진단을 사용한다.</summary>
    [Fact]
    public void InstallationPreservesReceiversAndUsesCurrentStorageDiagnostic()
    {
        var game = new ClientGameplayGlobals();
        var surface = new ClientSurfaceGlobals(game, new(new ClientLocalStateGlobals(game, new())), new Kernel(), () => 0);
        var dispatch = Dispatch();
        uint[] types = [0x3f, 0x30, 0x7a, 0x5f, 0x7d];
        // 각 행의 기존 F를 저장하여 설치 뒤 수신기 보존을 확인한다.
        foreach (uint type in types) dispatch.RegisterCanonicalDirect(type, 0x1000 + type, (_, _, _) => { });
        var receivers = types.Select(dispatch.SnapshotReceiver).ToArray();
        dispatch.Version(0x20, 7, 9); dispatch.RequireLength(0x20, 59);
        dispatch.Observe(0x20, _ => throw new IOException("다른 행 관찰"));
        var other = dispatch.Registration(0x20).Observer;
        surface.BindDisplayDiagnostics((_, _, _) => throw new IOException("교체 전 진단"));
        ClientVariablePacketObservers.Install(dispatch, surface.Storage);
        // 같은 표의 metadata/observer만 바뀌고 Player나 World는 생성되지 않는다.
        for (int index = 0; index < types.Length; index++)
        {
            var row = dispatch.Registration(types[index]);
            Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(65535, row.Length);
            Assert.NotNull(row.PacketObserver); Assert.Null(row.Observer);
            Assert.Same(receivers[index], dispatch.SnapshotReceiver(types[index]));
        }
        Assert.Same(other, dispatch.Registration(0x20).Observer);
        Assert.Equal(7, dispatch.Registration(0x20).Version); Assert.Equal(9, dispatch.Registration(0x20).Mismatch); Assert.Equal(59, dispatch.Registration(0x20).Length);
        byte[] packet = Enumerable.Repeat((byte)0xff, 27).ToArray();
        int diagnostics = 0;
        surface.BindDisplayDiagnostics((_, _, line) => { Assert.Equal(0x7c9u, line); diagnostics++; packet[26] = 3; });
        dispatch.Registration(0x3f).NotifyObserver(packet, 1);
        Assert.Equal(1, diagnostics); Assert.Equal(33, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        Assert.Equal(0u, game.PlayerRecords.Base);
    }

    /// <summary>수신은 미등록 종류와 미선택 Span도 읽지 않고 다섯 설치 observer에도 적용한다.</summary>
    [Fact]
    public void IncomingSkipsSpanLookup()
    {
        using var memory = new Unreadable();
        ClientVariablePacketObservers.Observe(uint.MaxValue, memory.Buffer, 0);
        var dispatch = Dispatch();
        var game = new ClientGameplayGlobals();
        var surface = new ClientSurfaceGlobals(game, new(new ClientLocalStateGlobals(game, new())), new Kernel(), () => 0);
        ClientVariablePacketObservers.Install(dispatch, surface.Storage);
        // 실제 다섯 종류에서도 동일 수신 관문을 사용한다.
        foreach (uint type in new uint[] { 0x3f, 0x30, 0x7a, 0x5f, 0x7d }) dispatch.Registration(type).NotifyObserver(memory.Buffer, 0);
    }

    /// <summary>필수 필드 부족에서는 길이WORD나 주변 물리를 먼저 변경하지 않는다.</summary>
    [Theory]
    [InlineData(0x3f,26)] [InlineData(0x30,14)] [InlineData(0x7a,8)] [InlineData(0x5f,9)] [InlineData(0x7d,17)]
    public void MissingPhysicalFieldDoesNotChangePacket(uint type, int size)
    {
        byte[] packet = Enumerable.Repeat((byte)0xd3, size).ToArray(); byte[] before = packet.ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientVariablePacketObservers.Observe(type, packet, 1));
        Assert.Equal(before, packet);
    }

    /// <summary>7dh의 중단형 sentinel은 복귀형 표시 진단으로 우회하지 않는다.</summary>
    [Fact]
    public void TimerSentinelStopsBeforeLengthAndDoesNotCallReturningAssertion()
    {
        byte[] packet = Enumerable.Repeat((byte)0xff, 18).ToArray(); byte[] before = packet.ToArray();
        var error = Assert.Throws<InvalidOperationException>(() => ClientVariablePacketObservers.Observe(0x7d, packet, 1, (_, _, _) => throw new IOException("호출되면 안 되는 복귀 진단")));
        Assert.Contains("bda", error.Message); Assert.Equal(before, packet);
    }

    /// <summary>복귀형 진단의 누락이나 예외는 길이 기록 전에 실패한다.</summary>
    [Fact]
    public void MissingOrStoppedReturningAssertionPreservesLength()
    {
        byte[] packet = Enumerable.Repeat((byte)0xff, 27).ToArray(); byte[] before = packet.ToArray();
        Assert.Throws<InvalidOperationException>(() => ClientVariablePacketObservers.Observe(0x3f, packet, 1)); Assert.Equal(before, packet);
        Assert.Throws<IOException>(() => ClientVariablePacketObservers.Observe(0x3f, packet, 1, (_, _, _) => throw new IOException("선택 진단 중단"))); Assert.Equal(before, packet);
    }
}
