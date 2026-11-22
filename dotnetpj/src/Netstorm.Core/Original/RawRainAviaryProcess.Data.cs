using System.Buffers.Binary;

// 맨 오 워 풀의 실제48BYTE와 현재 변경 비트의 고정 폭을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawRainAviaryProcess.h의 padding을 포함한 실제 객체 이미지다.</summary>
public sealed class RainAviaryProcessFields
{
    /// <summary>outer/inner와 미정 padding의 원문48BYTE를 소유한다.</summary>
    public byte[] Bytes { get; } = new byte[48];
    /// <summary>생성자가 쓰지 않는 caller 입력도 지정한 패턴으로 보존한다.</summary>
    public RainAviaryProcessFields(byte fill = 0) => Array.Fill(Bytes, fill);
    /// <summary>원문 값 구조체처럼 전체 이미지를 복사한다.</summary>
    public RainAviaryProcessFields(RainAviaryProcessFields other) => other.Bytes.CopyTo(Bytes, 0);
    /// <summary>outer+8의 현재 변경 비트다.</summary>
    public byte Dirty() => Bytes[8];
    /// <summary>outer+10의 원문8BYTE 시각이다.</summary>
    public double Time() => BinaryPrimitives.ReadDoubleLittleEndian(Bytes.AsSpan(16));
    /// <summary>outer+18의 현재 AP 상태DWORD다.</summary>
    public uint Stage() => BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(24));
    /// <summary>inner+4의 Form DWORD 하위WORD다.</summary>
    public ushort Form() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(36)));
    /// <summary>inner+8의 부모DWORD 하위WORD다.</summary>
    public ushort Parent() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(40)));
}

/// <summary>실제73h writer의 현재 lowWORD 식별자와 raw 선택 필드다.</summary>
public sealed partial class RawRainAviaryProcess
{
    /// <summary>다른 padding을 보존하고 같은DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>현재 raw double의8BYTE만 쓴다.</summary>
    private static void StoreTime(RainAviaryProcessFields fields, double value) => BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(16), value);
    /// <summary>원문 stage 변경의 dirty4를 더한다.</summary>
    private static void Stage(RainAviaryProcessFields fields, uint value) { fields.Bytes[8] |= 4; Put(fields.Bytes, 24, value); }
    /// <summary>현재 시각을 두 원문 위치에서 다시 읽고 마지막 지연을 저장한다.</summary>
    private void SetTime(RainAviaryProcessFields fields, double delay) { fields.Bytes[8] |= 2; StoreTime(fields, _globals.Now()); StoreTime(fields, _globals.Now() + delay); }
    /// <summary>전체 선택 폭을 먼저 검사하고 현재 필드를 기록한 뒤 dirty를 소비한다.</summary>
    public static ushort WriteProcess(RainAviaryProcessFields fields, Span<byte> packet, bool includeParent, bool force = false, byte protocol = 1, byte message = 0x73)
    {
        uint parent = includeParent || force ? fields.Parent() : 0u; byte dirty = force ? (byte)0xff : fields.Dirty();
        int required = 9 + (parent != 0 ? 4 : 0) + ((dirty & 2) != 0 ? 8 : 0) + ((dirty & 4) != 0 ? 4 : 0);
        if (packet.Length < required) throw new ArgumentException("맨 오 워 풀 패킷 버퍼 크기 오류");
        if (force) fields.Bytes[8] = 0xff; packet[0] = protocol; packet[1] = message; Put(packet, 4, fields.Form()); int offset = 9;
        if (parent != 0) { fields.Bytes[8] |= 1; Put(packet, offset, parent); offset += 4; }
        if ((fields.Dirty() & 2) != 0) { fields.Bytes.AsSpan(16, 8).CopyTo(packet[offset..]); offset += 8; }
        if ((fields.Dirty() & 4) != 0) { Put(packet, offset, fields.Stage()); offset += 4; }
        BinaryPrimitives.WriteUInt16LittleEndian(packet[2..], (ushort)offset); packet[8] = fields.Dirty(); fields.Bytes[8] = 0; return (ushort)offset;
    }
}
