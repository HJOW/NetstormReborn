using System.Buffers.Binary;

// 원문 비행 이미지와 고정 폭의 상태 codec/수신 초기화만 보존한다.
namespace Netstorm.Core.Original;

/// <summary>RawFlyerProcess.h의 기본112BYTE와 Sun전용 추가24BYTE 이미지다.</summary>
public sealed class FlyerProcessFields
{
    /// <summary>outer/inner/padding/Rain 추가 필드의112BYTE다.</summary>
    public byte[] Bytes { get; } = new byte[112];
    /// <summary>Sun 훨리기그의+70..87이며 기본 이미지와 별도 보존하는24BYTE다.</summary>
    public byte[] SunTail { get; } = new byte[24];
    /// <summary>원문 caller의 두 이미지 바이트 패턴만 준비한다.</summary>
    public FlyerProcessFields(byte fill = 0) { Array.Fill(Bytes, fill); Array.Fill(SunTail, fill); }
    /// <summary>원문 값 구조체처럼 두 이미지를 모두 복사한다.</summary>
    public FlyerProcessFields(FlyerProcessFields other) { other.Bytes.CopyTo(Bytes, 0); other.SunTail.CopyTo(SunTail, 0); }
    /// <summary>outer+34의 Form DWORD 하위WORD다.</summary>
    public ushort Form() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(52)));
    /// <summary>outer+38의 부모 DWORD 하위WORD다.</summary>
    public ushort Parent() => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(56)));
    /// <summary>outer+a의 현재 홈 WORD다.</summary>
    public ushort Home() => BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(10));
}

/// <summary>RawFlyerProcess.cpp의 raw 상태 읽기/쓰기와 실제 leaf 초기화다.</summary>
public sealed partial class RawFlyerProcess
{
    /// <summary>dirty2..80의 실제 원문 offset/폭이다.</summary>
    private static readonly (int Offset, int Width)[] DataFields = [(10, 2), (12, 2), (16, 4), (20, 4), (24, 4), (32, 8), (40, 8)];
    /// <summary>정렬되지 않은 실제 DWORD를 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[offset..]);
    /// <summary>다른 필드/padding을 보존하고 같은 DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>원문 홈/표적의 실제 WORD 폭만 쓴다.</summary>
    private static void Word(Span<byte> bytes, int offset, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(bytes[offset..], value);
    /// <summary>현재8BYTE 시각을 산술 없이 읽는다.</summary>
    private static double Time(FlyerProcessFields fields, int offset) => BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(fields.Bytes.AsSpan(offset)));
    /// <summary>원문8BYTE 시각만 기록하며 별도 dirty를 더하지 않는다.</summary>
    private static void Time(FlyerProcessFields fields, int offset, double value) => BinaryPrimitives.WriteUInt64LittleEndian(fields.Bytes.AsSpan(offset), BitConverter.DoubleToUInt64Bits(value));
    /// <summary>실제 네 outer VT의480260 빈 observer만 허용한다.</summary>
    private static void Observe(FlyerProcessFields fields)
    {
        if (Read(fields.Bytes, 0) is not (0x509cc8 or 0x509f24 or 0x509f60 or 0x509f98)) throw new InvalidOperationException("비행 데이터의 현재 가상0 observer 미연결");
    }
    /// <summary>기본 부착 뒤 맨 오 워의 전용 표/추가 타이머만 쓴다.</summary>
    public static void SpecializeRain(FlyerProcessFields fields) { Put(fields.Bytes, 0, 0x509f60); Put(fields.Bytes, 48, 0x509f40); Time(fields, 96, 0); Time(fields, 104, 0); Put(fields.Bytes, 84, 0); }
    /// <summary>기본 부착 뒤 훨리기그의 전용 표/104·112·120 타이머와84필드만 쓴다.</summary>
    public static void SpecializeSun(FlyerProcessFields fields) { Put(fields.Bytes, 0, 0x509f24); Put(fields.Bytes, 48, 0x509f04); Time(fields, 104, 0); fields.SunTail.AsSpan(0, 16).Clear(); Put(fields.Bytes, 84, 0); }
    /// <summary>RawWindFlyer.cpp의 Specialize만 이식한다. 실제 첫 Frame 초기화와는 다르다.</summary>
    public static void SpecializeWind(FlyerProcessFields fields) { Put(fields.Bytes, 0, 0x509f98); Put(fields.Bytes, 48, 0x509f78); Time(fields, 96, 0); Put(fields.Bytes, 84, 0); }
    /// <summary>수신 기본 생성자와 theme1/0/기타의 Rain/Wind/Sun 전문화만 수행한다.</summary>
    public static void InitializeReceived(FlyerProcessFields fields, int theme)
    {
        Time(fields, 32, 0); Time(fields, 40, 0); fields.Bytes[8] = 0xff; Put(fields.Bytes, 48, 0x5019ec); Put(fields.Bytes, 56, 0); Put(fields.Bytes, 52, 0);
        Put(fields.Bytes, 0, 0x509cc8); Put(fields.Bytes, 48, 0x509ca8); Time(fields, 64, 0); Time(fields, 72, 0); Word(fields.Bytes, 12, 0); Put(fields.Bytes, 84, 0);
        if (theme == 1) SpecializeRain(fields); else if (theme == 0) SpecializeWind(fields); else SpecializeSun(fields);
    }
    /// <summary>임시 수신 객체 해제 직전 실제 inner VT와 두전체DWORD -12345만 쓴다.</summary>
    public static void DestructFields(FlyerProcessFields fields) { Put(fields.Bytes, 48, 0x5019ec); Put(fields.Bytes, 52, 0xffffcfc7); Put(fields.Bytes, 56, 0xffffcfc7); }
    /// <summary>현재 필드 소비 직전에 검사하며 이미 완료한 부분 쓰기는 되돌리지 않는다.</summary>
    private static void Require(int length, int offset, int width)
    {
        if (offset > length || width > length - offset) throw new ArgumentException("비행 수신의 선택된 dirty 필드 부족");
    }
    /// <summary>현재 네VT의 빈 전/후 observer와 WORD/DWORD/8BYTE 부분 쓰기·별도 부모 반환이다.</summary>
    public static uint ReadData(FlyerProcessFields fields, ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 9) throw new ArgumentException("비행 수신의 실제9BYTE 머리 부족");
        byte dirty = packet[8]; int offset = 9; uint parent = 0; Observe(fields);
        if ((dirty & 1) != 0) { Require(packet.Length, offset, 4); parent = Read(packet, offset); offset += 4; }
        // 원문 두WORD/세DWORD/두8BYTE의 순서와 비트만 복사한다.
        for (int i = 0; i < DataFields.Length; i++)
        {
            if ((dirty & (2 << i)) == 0) continue; (int target, int width) = DataFields[i]; Require(packet.Length, offset, width);
            packet.Slice(offset, width).CopyTo(fields.Bytes.AsSpan(target)); offset += width;
        }
        Observe(fields); return parent;
    }
    /// <summary>실제 선택 폭의 버퍼를 먼저 검사하고 헤더/현재 dirty 필드/길이를 쓴 뒤 dirty만 소비한다.</summary>
    public static ushort WriteProcess(FlyerProcessFields fields, Span<byte> packet, bool includeParent, bool force = false, byte protocol = 1, byte message = 0x39)
    {
        uint parent = includeParent || force ? Read(fields.Bytes, 56) : 0; byte dirty = force ? (byte)0xff : fields.Bytes[8]; int required = 9 + (parent != 0 ? 4 : 0);
        // force를 저장하기 전 선택된 전체 폭을 계산한다.
        for (int i = 0; i < DataFields.Length; i++) if ((dirty & (2 << i)) != 0) required += DataFields[i].Width;
        if (packet.Length < required) throw new ArgumentException("비행 프로세스 상태 패킷 크기 오류");
        if (force) fields.Bytes[8] = 0xff; packet[0] = protocol; packet[1] = message; Put(packet, 4, Read(fields.Bytes, 52)); int offset = 9;
        if (parent != 0) { fields.Bytes[8] |= 1; Put(packet, offset, parent); offset += 4; }
        // 같은 metadata가 지정한 현재 raw 필드를 원문 폭으로 복사한다.
        for (int i = 0; i < DataFields.Length; i++)
        {
            if ((fields.Bytes[8] & (2 << i)) == 0) continue; (int source, int width) = DataFields[i]; fields.Bytes.AsSpan(source, width).CopyTo(packet[offset..]); offset += width;
        }
        Word(packet, 2, (ushort)offset); packet[8] = fields.Bytes[8]; fields.Bytes[8] = 0; return (ushort)offset;
    }
}
