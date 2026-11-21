using System.Buffers.Binary;

// 현재 첫NUL/선언DWORD·관찰 뒤 WORD 쓰기를 원격70h 문자열에서 보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawRemoteTellPacket.cpp의264BYTE writer와40b090/cf980 관찰·수신 본체다.</summary>
public static class RawRemoteTellPacket
{
    /// <summary>선택된 실제 진단/소비자만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("원격 Tell의 선택된 실제 장치 미연결");
    /// <summary>첫NUL의 현재 BYTE 길이로 ff예약/헤더/길이DWORD→선택 진단→본문/NUL을 작성한다. 줄은 실제4ef/4fb caller가 준다.</summary>
    public static byte[] Write(ReadOnlyMemory<byte> text, Action<string, string, uint>? assertion, uint line)
    {
        int zero = text.Span.IndexOf((byte)0); if (zero >= 0) text = text[..zero]; uint length = (uint)text.Length + 1;
        var packet = new byte[264]; packet.AsSpan().Fill(0xff); packet[0] = 1; packet[1] = 0x70; packet[2] = 4; packet[3] = 0;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), (uint)length);
        if (length > 255) Need(assertion)("z->tellLength < 256", @".\Ui.cpp", line);
        if (length > 256) throw new ArgumentOutOfRangeException(nameof(text), "원격 Tell264BYTE 복사 물리 범위");
        text.Span.CopyTo(packet.AsSpan(8)); packet[8 + text.Length] = 0; return packet;
    }
    /// <summary>송신일 때만 현재DWORD 진단 뒤 현재값을 다시 읽어 하위WORD의 감김을 기록한다.</summary>
    public static void Observe(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        if (sending == 0) return;
        if (BinaryPrimitives.ReadUInt32LittleEndian(packet.Span.Slice(4, 4)) == uint.MaxValue) Need(assertion)("tellLength != (int)-1", @"\Ns\O\_p_packets.cpp", 0xac7);
        uint length = unchecked(BinaryPrimitives.ReadUInt32LittleEndian(packet.Span.Slice(4, 4)) + 8);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), unchecked((ushort)length));
    }
    /// <summary>선언 길이/종류/sender를 읽지 않고+8의 첫NUL까지 같은 mutable 입력 뷰를 실제 소비자에 전달한다.</summary>
    public static void Receive(ReadOnlyMemory<byte> packet, Action<ReadOnlyMemory<byte>>? tell)
    {
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "원격 Tell 실제 문자열 물리 누락");
        ReadOnlyMemory<byte> body = packet[8..]; int zero = body.Span.IndexOf((byte)0);
        if (zero < 0) throw new ArgumentOutOfRangeException(nameof(packet), "원격 Tell 실제 NUL 누락"); Need(tell)(body[..zero]);
    }
}
