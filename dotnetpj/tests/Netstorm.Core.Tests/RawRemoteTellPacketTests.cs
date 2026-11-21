using System.Buffers.Binary;
using Netstorm.Core.Original;

// 원격 Tell의 BYTE/NUL/선택 진단과 현재 재조회·감김을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>RawRemoteTellPacket.cpp의 실제264BYTE writer와 송신 관찰/수신 계약이다.</summary>
public sealed class RawRemoteTellPacketTests
{
    /// <summary>첫NUL 길이와 ff꼬리를 유지하며 선언WORD는 observer까지4로 남는다.</summary>
    [Fact]
    public void WriterPreservesExactReservationAndFirstNul()
    {
        byte[] packet = RawRemoteTellPacket.Write(new byte[] { 0xff, 0x80, 0, 1 }, null, 0x4ef);
        Assert.Equal(264, packet.Length); Assert.Equal(new byte[] { 1, 0x70, 4, 0, 3, 0, 0, 0, 0xff, 0x80, 0 }, packet.AsSpan(0, 11).ToArray());
        Assert.All(packet.Skip(11), value => Assert.Equal(0xff, value)); RawRemoteTellPacket.Observe(packet, 1, null); Assert.Equal(11, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
    }
    /// <summary>경계255문자 입력은 진단이 복귀하면 실제256BYTE를 복사하고256문자는 진단 뒤 물리 실패한다.</summary>
    [Fact]
    public void WriterSeparatesReturningLengthDiagnosticFromPhysicalLimit()
    {
        int assertions = 0;
        void Diagnose(string text, string file, uint line) { Assert.Equal("z->tellLength < 256", text); Assert.Equal(@".\Ui.cpp", file); Assert.Equal(0x4fbu, line); assertions++; }
        byte[] packet = RawRemoteTellPacket.Write(Enumerable.Repeat((byte)'x', 255).ToArray(), Diagnose, 0x4fb); Assert.Equal(256u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4, 4))); Assert.Equal(0, packet[263]);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawRemoteTellPacket.Write(Enumerable.Repeat((byte)'x', 256).ToArray(), Diagnose, 0x4fb)); Assert.Equal(2, assertions);
    }
    /// <summary>길이 진단 중 입력을 바꿔도 이미 선택한 길이/끝은 유지하고 실제 복사 BYTE는 현재값을 사용한다.</summary>
    [Fact]
    public void WriterUsesCurrentInputBytesAfterDiagnosticWithStoredLength()
    {
        byte[] text = Enumerable.Repeat((byte)'x', 255).ToArray(); byte[] packet = RawRemoteTellPacket.Write(text, (_, _, _) => { text[0] = (byte)'y'; text[1] = 0; }, 0x4ef);
        Assert.Equal(256u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4, 4))); Assert.Equal(new byte[] { (byte)'y', 0, (byte)'x' }, packet.AsSpan(8, 3).ToArray()); Assert.Equal(0, packet[263]);
    }
    /// <summary>송신0은 물리 머리 없이 반환하고 sentinel 진단이 복귀하면 현재DWORD로 WORD 감김을 계산한다.</summary>
    [Fact]
    public void ObserverRereadsCurrentLengthAndPreservesWordWrap()
    {
        RawRemoteTellPacket.Observe(Memory<byte>.Empty, 0, null); byte[] packet = new byte[8]; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), uint.MaxValue);
        RawRemoteTellPacket.Observe(packet, 1, (text, file, line) => { Assert.Equal("tellLength != (int)-1", text); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(0xac7u, line); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 0x1fffe); });
        Assert.Equal(6, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2))); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), uint.MaxValue); RawRemoteTellPacket.Observe(packet, 1, (_, _, _) => { }); Assert.Equal(7, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
    }
    /// <summary>수신은 선언길이/종류 대신 첫NUL을 사용하고 같은 현재 입력 뷰를 소비자에 전달한다.</summary>
    [Fact]
    public void ReceiveIgnoresDeclaredLengthAndKeepsActualMutableView()
    {
        byte[] packet = [0xff, 0x31, 0, 0, 0xff, 0xff, 0xff, 0xff, 0xff, (byte)'b', 0, (byte)'z']; ReadOnlyMemory<byte> received = default;
        RawRemoteTellPacket.Receive(packet, text => received = text); packet[9] = (byte)'c'; Assert.Equal(new byte[] { 0xff, (byte)'c' }, received.ToArray());
        packet[8] = 0; RawRemoteTellPacket.Receive(packet, text => Assert.Empty(text.ToArray()));
    }
    /// <summary>NUL/물리 입력 검사는 선택 소비자 누락보다 먼저 수행하고 빈 본문도 실제 소비자가 필요하다.</summary>
    [Fact]
    public void ReceiveDoesNotInventTerminatorOrConsumer()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => RawRemoteTellPacket.Receive(new byte[8], null));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawRemoteTellPacket.Receive(Enumerable.Repeat((byte)1, 9).ToArray(), null));
        Assert.Throws<InvalidOperationException>(() => RawRemoteTellPacket.Receive(new byte[9], null));
    }
}
