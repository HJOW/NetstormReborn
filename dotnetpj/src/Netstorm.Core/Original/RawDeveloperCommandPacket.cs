using System.Buffers.Binary;

// 선언 길이와 첫 NUL의 물리 범위를 구별하는 개발자 명령 패킷 본체다.
namespace Netstorm.Core.Original;

/// <summary>cpppj RawDeveloperCommandPacket의40a6b0 송신 관찰과44dba0 수신이다.</summary>
public static class RawDeveloperCommandPacket
{
    /// <summary>송신에서만 현재 len을 진단하고 진단 뒤 다시 읽은 len+8의 하위WORD를 기록한다.</summary>
    public static void Observe(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        if (sending == 0) return;
        if (BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]) == uint.MaxValue)
            (assertion ?? throw new InvalidOperationException("개발자 명령 len 진단 장치 미연결"))("len != (int)-1", @"\Ns\O\_p_packets.cpp", 0x74b);
        uint length = unchecked(BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]) + 8);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], unchecked((ushort)length));
    }
    /// <summary>sender/선언DWORD와 무관하게 +8의 첫NUL 전 BYTE를 실제 실행기에 전달한다.</summary>
    public static void Receive(ReadOnlyMemory<byte> packet, Action<ReadOnlyMemory<byte>>? execute)
    {
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "개발자 명령 문자열 물리 부족");
        ReadOnlyMemory<byte> body = packet[8..]; int end = body.Span.IndexOf((byte)0);
        if (end < 0) throw new ArgumentOutOfRangeException(nameof(packet), "개발자 명령 문자열 NUL 누락");
        (execute ?? throw new InvalidOperationException("개발자 명령 실제 해석기 미연결"))(body[..end]);
    }
}
