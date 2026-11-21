using System.Buffers.Binary;

// DaisProcess의 실제40BYTE와 선택된 수신/송신 및 Kernel 수명을 보존한다.
namespace Netstorm.Core.Original;

/// <summary>outer/inner/dirty/기한을 보관하며 미작성 padding을 별도로 보존하는40BYTE다.</summary>
public sealed class DaisProcessFields
{
    /// <summary>원문 전체40BYTE의 같은 수정 가능 이미지다.</summary>
    public byte[] Bytes { get; }
    /// <summary>생성자 대조용 미초기화 BYTE 패턴을 준비한다. 실제 명시 쓰기는 Initialize가 수행한다.</summary>
    public DaisProcessFields(byte fill = 0) { Bytes = new byte[40]; Bytes.AsSpan().Fill(fill); }
    /// <summary>값으로 전달되는 원문 구조체처럼 같은40BYTE를 복사한다.</summary>
    public DaisProcessFields(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != 40) throw new ArgumentException("DaisProcess의40BYTE 구조체 계약", nameof(bytes)); Bytes = bytes.ToArray();
    }
}

/// <summary>RawDaisProcessData.cpp의44a2d4/41bd00/선택 decoder/4488c0 본체다.</summary>
public static class RawDaisProcessData
{
    /// <summary>선택된 현재 DWORD만 little endian으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(offset, 4), value);
    /// <summary>부모/폼→inner→기한→dirty→outer→inner의 명시 생성자 쓰기 외 padding은 보존한다.</summary>
    public static void Initialize(DaisProcessFields fields)
    {
        byte[] bytes = fields.Bytes; Put(bytes, 8, 0); Put(bytes, 4, 0); Put(bytes, 0x10, 0x50782c);
        BinaryPrimitives.WriteUInt64LittleEndian(bytes.AsSpan(0x20, 8), 0); bytes[0x18] = 0xff; Put(bytes, 0, 0x507834); Put(bytes, 0x10, 0x507830);
    }
    /// <summary>실제 소유권 해제와 구별되는 base VT와 두 signed sentinel DWORD만 기록한다.</summary>
    public static void Destruct(DaisProcessFields fields)
    {
        Put(fields.Bytes, 0, 0x5019ec); Put(fields.Bytes, 4, 0xffffcfc7); Put(fields.Bytes, 8, 0xffffcfc7);
    }
    /// <summary>신규일 때만 부모 DWORD를 읽고 기존 bit1은 위치만 건너뛴다. bit2 입력을 먼저 읽은 뒤 같은 실제 기한에 쓴다.</summary>
    public static uint Read(DaisProcessFields? fields, ReadOnlyMemory<byte> packet, bool newProcess)
    {
        ReadOnlySpan<byte> bytes = packet.Span; byte mask = bytes.Slice(8, 1)[0]; int cursor = 9; uint parent = 0;
        if ((mask & 1) != 0) { if (newProcess) parent = BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(cursor, 4)); cursor += 4; }
        if ((mask & 2) != 0)
        {
            ulong deadline = BinaryPrimitives.ReadUInt64LittleEndian(bytes.Slice(cursor, 8));
            if (fields is null) throw new InvalidOperationException("DaisProcess bit2 저장의 현재 NULL/다른 클래스 이미지 미지원");
            BinaryPrimitives.WriteUInt64LittleEndian(fields.Bytes.AsSpan(0x20, 8), deadline);
        }
        return parent;
    }
    /// <summary>force/부모 payload와 현재 dirty를 원문대로 사용한다. 실패 전 dirty/접두를 보존하고 정상 끝에서만 dirty를0으로 만든다.</summary>
    public static ushort Write(DaisProcessFields fields, Span<byte> packet, uint form, uint parentPayload, bool force, byte protocol = 1, byte message = 0x6a)
    {
        byte[] bytes = fields.Bytes; if (force) bytes[0x18] = 0xff;
        packet.Slice(0, 1)[0] = protocol; packet.Slice(1, 1)[0] = message; Put(packet, 4, form); ushort size = 9; int cursor = 9;
        if (parentPayload != 0) { bytes[0x18] |= 1; Put(packet, cursor, parentPayload); cursor += 4; size += 4; }
        if ((bytes[0x18] & 2) != 0) { ulong deadline = BinaryPrimitives.ReadUInt64LittleEndian(bytes.AsSpan(0x20, 8)); BinaryPrimitives.WriteUInt64LittleEndian(packet.Slice(cursor, 8), deadline); size += 8; }
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Slice(2, 2), size); packet[8] = bytes[0x18]; bytes[0x18] = 0; return size;
    }
}

/// <summary>실제 Host/Kernel이 단독 소유하는 DaisProcess다. DaisRegular와 구별되는 원문의 빈 실행 객체다.</summary>
public sealed class DaisProcess : SquidProcess
{
    /// <summary>현재 decoder와 writer가 함께 읽는 같은 원문 이미지다.</summary>
    public DaisProcessFields Fields { get; }
    /// <summary>값 구조체를 복사하고 실제 생성자 명시 필드만 쓴다.</summary>
    public DaisProcess(DaisProcessFields? fields = null)
    {
        Fields = fields is null ? new() : new(fields.Bytes); RawDaisProcessData.Initialize(Fields);
    }
    /// <summary>4437f0은 빈 RET다. 기한/dirty가 있어도 사건이나 예약을 만들지 않는다.</summary>
    public override void RunFrame() { }
    /// <summary>480260은 빈 RET12다. 통지 인자를 새 사건으로 바꾸지 않는다.</summary>
    public override void Notify(uint ev, uint info, uint value) { }
    /// <summary>Host가 먼저 정한 실제 Form/부모를 같은 이미지에 동기화한다.</summary>
    public override void OnAttached()
    {
        BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(4, 4), unchecked((uint)Form()));
        BinaryPrimitives.WriteUInt32LittleEndian(Fields.Bytes.AsSpan(8, 4), unchecked((uint)Parent()));
    }
    /// <summary>Kernel이 반납할 때 같은 이미지에 원문의 base 소멸 표식만 기록한다.</summary>
    public override void Dispose() => RawDaisProcessData.Destruct(Fields);
    /// <summary>4492f0의 현재 Host 식별자와 변경/force를 실제4488c0에 전달한다.</summary>
    public ushort WritePacket(Span<byte> packet, bool parentChanged, bool force = false, byte protocol = 1, byte message = 0x6a)
    {
        OnAttached(); uint parent = parentChanged || force ? BinaryPrimitives.ReadUInt32LittleEndian(Fields.Bytes.AsSpan(8, 4)) : 0;
        return RawDaisProcessData.Write(Fields, packet, BinaryPrimitives.ReadUInt32LittleEndian(Fields.Bytes.AsSpan(4, 4)), parent, force, protocol, message);
    }
}
