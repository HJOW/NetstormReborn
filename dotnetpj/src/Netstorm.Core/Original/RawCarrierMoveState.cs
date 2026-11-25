// 원본 운반체의 이동 요청 18BYTE 헤더와 작성기를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 운반 이동 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.

// 현재 요청 헤더는 TYPE/에디터 상태와 별개인 원본 52edec의 BYTE/BYTE/WORD다.
public sealed class CarrierMoveState
{
    /// <summary>40a850이 매번 읽는 원본 초기 헤더다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public FormPacketHeader Request = new(0x40, 1, 18);
}

// 0040a850의 현재 헤더와 18바이트 원문을 작성한다. 선택/동일 목적지 관문은 호출자가 수행한다.
public static class CarrierMoveCommand
{
    /// <summary>원본 생성자처럼 protocol의 bit7만 제거하며 length를 4로 보정하지 않는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="request">쓸 헤더다.</param>
    /// <param name="parent">쓸 부모 SID다.</param>
    /// <param name="destination">쓸 목적지다.</param>
    /// <param name="sacrifice">쓸 희생 값이다.</param>
    /// <param name="packet">쓸 18바이트 저장소다.</param>
    /// <returns>쓴 바이트 수다.</returns>
    public static int Write(FormPacketHeader request, Sid parent, RawPathPoint destination, uint sacrifice, Span<byte> packet)
    {
        if (packet.Length < 18)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "이동 명령 18바이트 저장소 부족");
        }

        packet[0] = (byte)(request.Protocol & 0x7f);
        packet[1] = request.Message;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Slice(2, 2), request.Length);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Slice(4, 2), parent.Value);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.Slice(6, 4), BitConverter.SingleToUInt32Bits(destination.X));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.Slice(10, 4), BitConverter.SingleToUInt32Bits(destination.Y));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.Slice(14, 4), sacrifice);
        return 18;
    }
}
