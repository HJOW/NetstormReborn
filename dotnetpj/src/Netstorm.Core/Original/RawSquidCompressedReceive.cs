using System.Buffers.Binary;

// 현재 송수신 owner 캐시와 TYPE를 사용해 압축 입력을 실제35BYTE 이미지로 복원한다.
namespace Netstorm.Core.Original;

/// <summary>RawSquidCompressedReceive.h의 현재 기본TYPE/선택TYPE/HP/진단이다.</summary>
public sealed class SquidCompressedReceiveHooks
{
    /// <summary>생성자 위치에서 읽는 현재 기본TYPE DWORD다.</summary>
    public Func<uint>? DefaultType { get; set; }
    /// <summary>4번 필드 뒤의 현재TYPE 레코드를 선택한다.</summary>
    public Func<uint, OriginalTypeRecord>? Type { get; set; }
    /// <summary>이미 선택한 같은TYPE의 실제 최대HP다.</summary>
    public Func<OriginalTypeRecord, int>? MaximumHp { get; set; }
    /// <summary>상충 마스크/캐시/부족 읽기의 원문 진단이며 복귀 뒤 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawSquidCompressedReceive.cpp의7ah decoder다. 같은 전송 상태를 빌리고 새 owner 캐시를 만들지 않는다.</summary>
public static class RawSquidCompressedReceive
{
    /// <summary>현재 선택된 실제 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("압축 객체 수신의 선택된 실제 장치 미연결");
    /// <summary>원문64BYTE 지역 복사 뒤 고정16필드를 처리한다. 뒤쪽 실패 전의 owner 캐시 갱신을 유지한다.</summary>
    public static byte[] Decode(ReadOnlyMemory<byte> packet, RawSquidTransmissionState state, SquidCompressedReceiveHooks hooks)
    {
        ReadOnlySpan<byte> input = packet.Span;
        if (input.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "압축 객체의9BYTE 머리 부족");
        int count = input[8];
        if (count > 64 || input.Length < count + 9) throw new ArgumentOutOfRangeException(nameof(packet), "압축 객체의 실제64BYTE 지역 복사 범위");
        byte[] data = new byte[64]; input.Slice(9, count).CopyTo(data); uint mask = BinaryPrimitives.ReadUInt16LittleEndian(input.Slice(6, 2)); int index = 0;
        byte[] image = new byte[35]; image[0] = (byte)(state.Update.Protocol & 0x7f); image[1] = state.Update.Message;
        BinaryPrimitives.WriteUInt16LittleEndian(image.AsSpan(2, 2), unchecked((ushort)state.Update.Length));
        BinaryPrimitives.WriteUInt16LittleEndian(image.AsSpan(4, 2), BinaryPrimitives.ReadUInt16LittleEndian(input.Slice(4, 2)));
        BinaryPrimitives.WriteUInt32LittleEndian(image.AsSpan(6, 4), 0xc61c4000); BinaryPrimitives.WriteUInt32LittleEndian(image.AsSpan(10, 4), 0xc61c4000);
        image[16] = unchecked((byte)Need(hooks.DefaultType)()); image[18] = unchecked((byte)state.LastOwner);
        // 선택된 진단이 복귀하면 같은 현재 상태에서 다음 명령을 수행한다.
        void Diagnose(string expression, uint line) => Need(hooks.Assertion)(expression, @".\Clientboard.cpp", line);
        // 지역에 복사된 입력만 소비하며 미정의 스택 읽기를 임의0으로 대신하지 않는다.
        void Copy(int destination, int width, bool byteRead = false)
        {
            if (index + width > count) { Diagnose("index <= dataLength", byteRead ? 0x128u : 0x137u); throw new ArgumentOutOfRangeException(nameof(packet), "압축 객체의 미정의 지역 스택 읽기"); }
            data.AsSpan(index, width).CopyTo(image.AsSpan(destination, width)); index += width;
        }
        // 마스크 유무와 관계없이0..15를 순서대로 방문하며4번 뒤 기본HP/zOrder를 항상 선택한다.
        for (int field = 0; field < 16; field++)
        {
            if ((mask & (1u << field)) != 0)
            {
                switch (field)
                {
                    case 0:
                        if ((mask & 2) != 0) Diagnose("!testMask( i+1 )", 0x14f);
                        Copy(6, 1, true); byte x = image[6]; BinaryPrimitives.WriteSingleLittleEndian(image.AsSpan(6, 4), x);
                        Copy(10, 1, true); byte y = image[10]; BinaryPrimitives.WriteSingleLittleEndian(image.AsSpan(10, 4), y); break;
                    case 1:
                        if ((mask & 1) != 0) Diagnose("!testMask( i-1 )", 0x15c); Copy(6, 8); break;
                    case 2:
                        if ((mask & 8) != 0) Diagnose("!testMask( i+1 )", 0x169); Copy(14, 1); break;
                    case 3:
                        if ((mask & 4) != 0) Diagnose("!testMask( i-1 )", 0x174); Copy(14, 2); break;
                    case 4: Copy(16, 1); break;
                    case 5: Copy(17, 1); break;
                    case 6: image[17] |= 0x10; break;
                    case 7:
                        if ((mask & 0x100) != 0) Diagnose("!testMask( i+1 )", 0x1af);
                        if (state.LastOwner == 0xffffff9d) Diagnose("hackySameAsLastPlayerId != CRAZY_PLAYER_ID", 0x1b0);
                        image[18] = unchecked((byte)state.LastOwner); break;
                    case 8:
                        if ((mask & 0x80) != 0) Diagnose("!testMask( i-1 )", 0x1bd); Copy(18, 1); state.LastOwner = image[18]; break;
                    case 9:
                        if ((mask & 0x400) != 0) Diagnose("!testMask( i+1 )", 0x1cb); Copy(27, 1); break;
                    case 10:
                        if ((mask & 0x200) != 0) Diagnose("!testMask( i-1 )", 0x1d6); Copy(27, 2); break;
                    case 11: Copy(29, 1); break;
                    case 12: Copy(30, 1); break;
                    case 13: Copy(34, 1); break;
                    default: break; //원문14/15는 데이터를 소비하지 않는다.
                }
            }
            if (field == 4)
            {
                OriginalTypeRecord selected = Need(hooks.Type)(image[16]);
                if ((selected.Flags1 & 0x10) != 0) BinaryPrimitives.WriteUInt16LittleEndian(image.AsSpan(27, 2), unchecked((ushort)Need(hooks.MaximumHp)(selected)));
                image[29] = unchecked((byte)selected.ZOrder);
            }
        }
        return image;
    }
}
