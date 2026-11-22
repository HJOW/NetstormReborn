using System.Buffers.Binary;

// 데빌메이커80BYTE의 생성/고정폭상태와 실제수신실패순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawWindAviaryState.h의 caller padding을 포함한80BYTE 원문 이미지다.</summary>
public sealed class WindAviaryProcessFields
{
    /// <summary>outer/inner/예약double/임시플라이어가 공유하는 유일한80BYTE다.</summary>
    public byte[] Bytes { get; } = new byte[80];
    /// <summary>생성자가 쓰지 않는 caller 입력을 같은 패턴으로 준비한다.</summary>
    public WindAviaryProcessFields(byte fill = 0) => Array.Fill(Bytes, fill);
    /// <summary>원문 값 구조체처럼 전체 이미지를 복사한다.</summary>
    public WindAviaryProcessFields(WindAviaryProcessFields other) => other.Bytes.CopyTo(Bytes, 0);
}

/// <summary>RawWindAviaryState.cpp의 생성/쓰기/읽기와 전체packet 사전검사다.</summary>
public static class RawWindAviaryState
{
    /// <summary>dirty2/4/8/10/20의 실제 offset/폭이다.</summary>
    private static readonly (int Offset, int Width)[] DataFields = [(16, 8), (24, 4), (32, 8), (40, 8), (48, 4)];
    /// <summary>지정DWORD만 little endian으로 기록한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>실제 원문double의8BYTE만 기록한다.</summary>
    private static void Time(Span<byte> bytes, int offset, double value) => BinaryPrimitives.WriteDoubleLittleEndian(bytes[offset..], value);
    /// <summary>미사용bit를 지우지 않고 실제 선택폭만 합산한다.</summary>
    private static int PayloadSize(byte dirty)
    {
        int size = 0;
        // 다섯 원문 필드의 bit와폭을 순서대로 합한다.
        for (int i = 0; i < DataFields.Length; i++) if ((dirty & (2 << i)) != 0) size += DataFields[i].Width;
        return size;
    }
    /// <summary>비서버stage/1c/30/34/inner꼬리의 caller 바이트는 쓰지 않는다.</summary>
    public static void Initialize(WindAviaryProcessFields fields, bool server, double now)
    {
        Span<byte> raw = fields.Bytes; Time(raw, 16, 0); Time(raw, 32, 0); Time(raw, 40, 0); raw[8] = 0xff;
        Put(raw, 56, 0x5019ec); Put(raw, 64, 0); Put(raw, 60, 0); Put(raw, 0, 0x509dfc); Put(raw, 56, 0x509ddc);
        if (server) { Time(raw, 16, now); raw[8] |= 4; Put(raw, 24, 1); }
    }
    /// <summary>전체선택폭을 먼저검사하고 fullDWORD식별자/raw필드/미작성tail과dirty소비를 보존한다.</summary>
    public static ushort WriteProcess(WindAviaryProcessFields fields, Span<byte> packet, bool includeParent, bool force = false, byte protocol = 1, byte message = 0x74)
    {
        Span<byte> raw = fields.Bytes; uint parent = includeParent || force ? BinaryPrimitives.ReadUInt32LittleEndian(raw[64..]) : 0u; byte dirty = force ? (byte)0xff : raw[8]; int required = 9 + (parent != 0 ? 4 : 0) + PayloadSize(dirty);
        if (packet.Length < required) throw new ArgumentException("데빌메이커 상태 전송 버퍼 부족");
        if (force) raw[8] = 0xff; packet[0] = protocol; packet[1] = message; Put(packet, 4, BinaryPrimitives.ReadUInt32LittleEndian(raw[60..])); int offset = 9;
        if (parent != 0) { raw[8] |= 1; Put(packet, offset, parent); offset += 4; }
        // 현재 선택된 raw 필드를 계산 없이 원문 폭으로 복사한다.
        for (int i = 0; i < DataFields.Length; i++)
        {
            if ((raw[8] & (2 << i)) == 0) continue; (int source, int width) = DataFields[i]; raw.Slice(source, width).CopyTo(packet[offset..]); offset += width;
        }
        BinaryPrimitives.WriteUInt16LittleEndian(packet[2..], (ushort)offset); packet[8] = raw[8]; raw[8] = 0; return (ushort)offset;
    }
    /// <summary>같은80BYTE를 빌려 실제빈객체 수신과 공통 본체를 사용한다.</summary>
    public static uint ReadProcess(WindAviaryProcessFields fields, ReadOnlySpan<byte> packet) => ReadProcess(fields.Bytes, packet);
    /// <summary>전체packet 선택폭을 첫쓰기전에 검사하고 객체저장범위는 각 선택위치에서 검사한다.</summary>
    public static uint ReadProcess(Span<byte> fields, ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentException("데빌메이커 상태 수신 머리 부족");
        byte dirty = packet[8]; int required = 9 + ((dirty & 1) != 0 ? 4 : 0) + PayloadSize(dirty);
        if (packet.Length < required) throw new ArgumentException("데빌메이커 상태 수신 필드 부족");
        int offset = 9; uint parent = 0; if ((dirty & 1) != 0) { parent = BinaryPrimitives.ReadUInt32LittleEndian(packet[offset..]); offset += 4; }
        // 실제선택객체 저장의 앞선 효과는 이후객체폭 오류에서도 보존한다.
        for (int i = 0; i < DataFields.Length; i++)
        {
            if ((dirty & (2 << i)) == 0) continue; (int target, int width) = DataFields[i];
            if (target > fields.Length || width > fields.Length - target) throw new ArgumentException("데빌메이커 수신의 실제 객체 필드 미연결");
            packet.Slice(offset, width).CopyTo(fields[target..]); offset += width;
        }
        return parent;
    }
}
