using System.Buffers;
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 CS 현재 슬롯/할당과7bh의 실제 부분 저장·별칭을 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>CS 장기 수명과 실제 공통 표의 주소 수신 경계를 검증한다.</summary>
public sealed class ClientChallengeGlobalsTests
{
    /// <summary>CS0 조기반환이 Span 조회 앞인지 감지한다.</summary>
    private sealed class Unreadable : MemoryManager<byte>
    {
        /// <summary>물리를 조회하지 않고 빌리는 메모리 핸들이다.</summary>
        public Memory<byte> Buffer => CreateMemory(12);
        /// <summary>미선택 Span을 요구하면 실패한다.</summary>
        public override Span<byte> GetSpan() => throw new IOException("CS0에서 미선택 Span 조회");
        /// <summary>이 검사에서는 포인터를 고정하지 않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();
        /// <summary>고정한 포인터가 없다.</summary>
        public override void Unpin() => throw new NotSupportedException();
        /// <summary>외부 소유 자원이 없다.</summary>
        protected override void Dispose(bool disposing) { }
    }

    /// <summary>현재 subscriber DWORD만 실제 참가자 물리에 쓴다.</summary>
    private static void Subscriber(Memory<byte> record, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(record.Span.Slice(4, 4), value);

    /// <summary>후속 저장의 원문 필드만 가진 수신 입력이다. lenBYTE는 의도적으로ff다.</summary>
    private static byte[] Packet(byte participant, uint subscriber, params byte[] text)
    {
        byte[] packet = new byte[10 + text.Length]; packet[4] = participant; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(5, 4), subscriber); packet[9] = 0xff; text.CopyTo(packet, 10); return packet;
    }

    /// <summary>생성/재사용은 동일 물리를 빌리고 새 현재 슬롯에서도 이전 할당과 장기 좌표는 유지된다.</summary>
    [Fact]
    public void PrepareKeepsActualAllocationsAndSeparateLongLivedGeometry()
    {
        var challenge = new ClientChallengeGlobals(); Assert.Equal(0u, challenge.Current); Assert.Equal(0u, challenge.Geometry.Initialized);
        Assert.Throws<InvalidOperationException>(() => challenge.Bytes); Assert.True(challenge.Prepare()); uint first = challenge.Current; Memory<byte> old = challenge.Bytes;
        old.Span.Fill(0xd3); challenge.Geometry.Grid[0, 0] = 123; challenge.Geometry.Width = -17;
        Assert.True(challenge.Prepare(() => throw new IOException("초기화된 배치 진단"))); Assert.Equal(first, challenge.Current); Assert.All(old.ToArray(), value => Assert.Equal(0, value));
        Assert.Equal(123f, challenge.Geometry.Grid[0, 0]); Assert.Equal(-17, challenge.Geometry.Width);
        old.Span[0] = 7; challenge.Current = 0; Assert.True(challenge.Prepare()); Assert.NotEqual(first, challenge.Current); uint second = challenge.Current;
        Assert.Equal(7, old.Span[0]); Assert.All(challenge.Bytes.ToArray(), value => Assert.Equal(0, value));
        challenge.Current = first; challenge.Bytes.Span[1] = 9; Assert.Equal(9, old.Span[1]); challenge.Current = second; Assert.Equal(0, challenge.Bytes.Span[1]);
        Assert.Throws<ArgumentOutOfRangeException>(() => challenge.Participant(40)); Assert.Throws<ArgumentOutOfRangeException>(() => challenge.Battle(10));
        challenge.Current = uint.MaxValue; Assert.Throws<InvalidOperationException>(() => challenge.Bytes);
    }

    /// <summary>기존 미초기화 배치의 진단이 현재 슬롯을 바꾸면 복귀 뒤 새 현재 기록만 Reset한다.</summary>
    [Fact]
    public void ReturningPrepareDiagnosticRereadsCurrentSlotAndStoppedDiagnosticPreservesRecords()
    {
        var challenge = new ClientChallengeGlobals(); challenge.Prepare(); uint first = challenge.Current; Memory<byte> old = challenge.Bytes; old.Span.Fill(0xd3);
        challenge.Current = 0; challenge.Prepare(); uint second = challenge.Current; Memory<byte> other = challenge.Bytes; other.Span.Fill(0xa7);
        challenge.Current = first; challenge.Geometry.Initialized = 0; int calls = 0;
        challenge.Prepare(() => { calls++; challenge.Current = second; }); Assert.Equal(1, calls); Assert.All(old.ToArray(), value => Assert.Equal(0xd3, value)); Assert.All(other.ToArray(), value => Assert.Equal(0, value)); Assert.Equal(0u, challenge.Geometry.Initialized);
        other.Span.Fill(0xb4); Assert.Throws<InvalidOperationException>(() => challenge.Prepare()); Assert.Throws<IOException>(() => challenge.Prepare(() => throw new IOException("선택 진단 중단")));
        Assert.All(other.ToArray(), value => Assert.Equal(0xb4, value));
    }

    /// <summary>실제7bh 표/저장 F는 CS0에서 버퍼/세션/연결을 요구하지 않고 설치 후 현재 진단을 사용한다.</summary>
    [Fact]
    public void FixedReceiverUsesSameOwnerAndCurrentDiagnosticWithoutWorldAtCsZero()
    {
        var challenge = new ClientChallengeGlobals(); var game = new ClientGameplayGlobals(); var surface = new ClientSurfaceGlobals(game, new(new ClientLocalStateGlobals(game, new())), new Kernel(), () => 0);
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        ClientChallengePacketDefaults.Install(dispatch, challenge, surface.Storage); var row = dispatch.Registration(0x7b); var saved = dispatch.SnapshotReceiver(0x7b)!;
        Assert.Equal(0x004b4790u, saved.NativeEntry); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(65535, row.Length); Assert.NotNull(row.PacketObserver); Assert.Null(row.Observer);
        using var unreadable = new Unreadable(); dispatch.InvokeDirect(0x7b, uint.MaxValue, null, unreadable.Buffer); row.NotifyObserver(unreadable.Buffer, 0);
        dispatch.RegisterCanonicalDirect(0x7b, 0x9999, (_, _, _) => throw new IOException("새 표의 다른 F")); saved.Invoke(0, null, unreadable.Buffer);
        byte[] outgoing = Packet(0, 0); int calls = 0;
        surface.BindDisplayDiagnostics((expression, file, line) => { calls++; Assert.Equal("addrLen != (char)-1", expression); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(0xba8u, line); outgoing[9] = 2; });
        row.NotifyObserver(outgoing, 1); Assert.Equal(1, calls); Assert.Equal(12, BinaryPrimitives.ReadUInt16LittleEndian(outgoing.AsSpan(2, 2)));
        challenge.Prepare(); saved.Invoke(uint.MaxValue, null, Packet(0, 0, (byte)'A', 0)); Assert.Equal((byte)'A', challenge.Participant(0).Span[0x1a0]); Assert.Equal(0u, game.PlayerRecords.Base);
    }

    /// <summary>inactive/참가자0/subscriber0/lenff도 원문 관문을 통과하고NUL 뒤 꼬리를 보존한다.</summary>
    [Fact]
    public void AddressWritesOnlyThroughNulWithoutExtraParticipantOrLengthGates()
    {
        var challenge = new ClientChallengeGlobals(); challenge.Prepare(); Memory<byte> record = challenge.Participant(0); record.Span.Fill(0xd3); record.Span[0x64] = 0; Subscriber(record, 0);
        byte[] expected = challenge.Bytes.ToArray(); expected[RawChallengeState.ParticipantOffset + 0x1a0] = (byte)'A'; expected[RawChallengeState.ParticipantOffset + 0x1a1] = 0;
        challenge.ReceivePeerAddress(Packet(0, 0, (byte)'A', 0, (byte)'Z'));
        Assert.Equal(expected, challenge.Bytes.ToArray());
    }

    /// <summary>subscriber 불일치는 len/문자열이 없는9BYTE에서도 그 뒤 물리를 요구하지 않는다.</summary>
    [Fact]
    public void SubscriberMismatchSkipsAddressAndStringReads()
    {
        var challenge = new ClientChallengeGlobals(); challenge.Prepare(); Subscriber(challenge.Participant(3), 17);
        byte[] packet = new byte[9]; packet[4] = 3; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(5, 4), 18); byte[] before = challenge.Bytes.ToArray();
        challenge.ReceivePeerAddress(packet); Assert.Equal(before, challenge.Bytes.ToArray());
    }

    /// <summary>입력 부족 또는44BYTE 꼬리 초과에서는 앞선 순차 저장을 유지하고 다음 기록을 건드리지 않는다.</summary>
    [Theory]
    [InlineData(2)] [InlineData(45)]
    public void MissingTerminatorOrAddressOverflowPreservesOnlyPriorStores(int count)
    {
        var challenge = new ClientChallengeGlobals(); challenge.Prepare(); Memory<byte> record = challenge.Participant(3); record.Span.Fill(0xd3); Subscriber(record, 17);
        byte[] before = challenge.Bytes.ToArray(); byte[] text = Enumerable.Repeat((byte)'A', count).ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => challenge.ReceivePeerAddress(Packet(3, 17, text)));
        before.AsSpan(RawChallengeState.ParticipantOffset + 3 * RawChallengeState.ParticipantBytes + 0x1a0, Math.Min(count, 44)).Fill((byte)'A');
        Assert.Equal(before, challenge.Bytes.ToArray());
    }

    /// <summary>출력보다 한BYTE 앞의 겹친 입력은 덮어쓴NUL도 다시 읽으며 안전 복사로 바꾸지 않는다.</summary>
    [Fact]
    public void OverlappingInputKeepsOriginalByteByBytePropagation()
    {
        var challenge = new ClientChallengeGlobals(); challenge.Prepare(); Memory<byte> record = challenge.Participant(3); record.Span.Fill(0xd3); Subscriber(record, 17);
        Memory<byte> packet = record.Slice(0x195); packet.Span[4] = 3; BinaryPrimitives.WriteUInt32LittleEndian(packet.Span.Slice(5, 4), 17); packet.Span[9] = 0xff; packet.Span[10] = (byte)'A'; packet.Span[11] = 0;
        byte[] expected = challenge.Bytes.ToArray(); expected.AsSpan(RawChallengeState.ParticipantOffset + 3 * RawChallengeState.ParticipantBytes + 0x1a0, 44).Fill((byte)'A');
        Assert.Throws<ArgumentOutOfRangeException>(() => challenge.ReceivePeerAddress(packet)); Assert.Equal(expected, challenge.Bytes.ToArray());
    }
}
