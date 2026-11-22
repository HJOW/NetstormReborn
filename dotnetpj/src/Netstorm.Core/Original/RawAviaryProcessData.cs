using System.Buffers.Binary;

// 두 아비어리의 공통 접두만 원문 순서로 복사한다.
namespace Netstorm.Core.Original;

/// <summary>RawAviaryProcessData.cpp의 별도 부모DWORD와 time/stage 선택 수신이다.</summary>
public static class RawAviaryProcessData
{
    /// <summary>앞선 쓰기는 되돌리지 않고 현재 소비할 packet 폭만 검사한다.</summary>
    private static void Require(int length, int offset, int width)
    {
        if (offset > length || width > length - offset) throw new ArgumentException("아비어리 수신의 선택 필드 부족");
    }
    /// <summary>선택된 packet을 먼저 검사하고 그다음 실제 객체 저장 폭을 검사한다.</summary>
    private static void Copy(Span<byte> fields, ReadOnlySpan<byte> packet, ref int offset, int destination, int width)
    {
        Require(packet.Length, offset, width);
        if (destination > fields.Length || width > fields.Length - destination) throw new ArgumentException("아비어리 수신의 실제 객체 필드 미연결");
        packet.Slice(offset, width).CopyTo(fields[destination..]); offset += width;
    }
    /// <summary>빈 객체도 선택 저장이 없으면 허용하며 변경 비트는 소비하지 않는다.</summary>
    public static uint Read(Span<byte> fields, ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentException("아비어리 수신의 실제 접두 물리 범위");
        byte dirty = packet[8]; int offset = 9; uint parent = 0;
        if ((dirty & 1) != 0) { Require(packet.Length, offset, 4); parent = BinaryPrimitives.ReadUInt32LittleEndian(packet[offset..]); offset += 4; }
        if ((dirty & 2) != 0) Copy(fields, packet, ref offset, 16, 8);
        if ((dirty & 4) != 0) Copy(fields, packet, ref offset, 24, 4);
        return parent;
    }
}
