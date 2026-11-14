using System.Globalization;

// 단독 캠페인에서도 실행되는 실제 Squid writer·압축·현재 연결 분배를 복원한다.
namespace Netstorm.Core.Original;

/// <summary>같은 Kernel 프로세스의 writer가 공통 버퍼에 패킷을 쓰고 원문 signed 크기를 반환한다.</summary>
public delegate int SquidProcessPacketWriter(int sid, Span<byte> buffer, uint argument);

/// <summary>현재 TYPE/HP·실제 패킷 분배·진단을 같은 World 수명으로 빌리는 효과다.</summary>
public sealed class RawSquidTransmissionHooks
{
    /// <summary>저장 TYPE 번호의 현재 물리 칸을 고른다.</summary>
    public Func<byte, SquidPopTypeReference>? Type { get; set; }
    /// <summary>현재 기본 다리 타입 DWORD를 읽고 비교 때만 BYTE로 줄인다.</summary>
    public Func<uint>? DefaultType { get; set; }
    /// <summary>현재 약화 조건까지 포함한 타입 최대 HP다.</summary>
    public Func<byte, int>? MaxHp { get; set; }
    /// <summary>양수 반환 크기로 제한한 같은 버퍼를 일반 방송에 전달한다.</summary>
    public Action<ReadOnlyMemory<byte>>? Broadcast { get; set; }
    /// <summary>압축 누적255 경계의 수준4 원문 로그다.</summary>
    public Action<int, string>? Trace { get; set; }
    /// <summary>정상 복귀 가능한 현재 원문 전송 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>writer 반환 뒤 현재 대상 연결 여부를 읽는다.</summary>
    public Func<int, bool>? Connected { get; set; }
    /// <summary>현재 대상에 반환 크기로 제한한 같은 버퍼를 보낸다.</summary>
    public Action<int, ReadOnlyMemory<byte>>? Send { get; set; }
    /// <summary>실제 ProcessForm+54의 같은 Kernel writer다.</summary>
    public SquidProcessPacketWriter? ProcessPacket { get; set; }
    /// <summary>Native 분배기가 선언 WORD를 다시 읽도록 같은 전체2048BYTE를 보낸다.</summary>
    public Action<byte[]>? BroadcastMutable { get; set; }
    /// <summary>현재 Player/NULL 관문 뒤 같은 전체 예약을 대상 Registry에 보낸다.</summary>
    public Action<int, byte[]>? SendMutable { get; set; }
    /// <summary>원문 함수 객체처럼 값 복사하고 캡처 대상은 같은 장기 소유자를 빌린다.</summary>
    internal RawSquidTransmissionHooks Copy() => new()
    {
        Type = Type, DefaultType = DefaultType, MaxHp = MaxHp, Broadcast = Broadcast, Trace = Trace,
        Assertion = Assertion, Connected = Connected, Send = Send, ProcessPacket = ProcessPacket,
        BroadcastMutable = BroadcastMutable, SendMutable = SendMutable,
    };
}

/// <summary>원본004ac260·004ad170·00432fe0의35BYTE writer·16필드 압축·실제 송신 분배다.</summary>
public sealed class RawSquidTransmission
{
    /// <summary>모든 조회가 공유하는 실제 PATCH 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>버퍼·헤더·최근 소유자·누적을 공유하는 실제 상태다.</summary>
    private readonly RawSquidTransmissionState _state;
    /// <summary>원문처럼 값 복사한 장기 효과다.</summary>
    private readonly RawSquidTransmissionHooks _hooks;

    /// <summary>같은 물리 상태와 장기 효과를 빌리며 CD를 PATCH 배치로 해석하지 않는다.</summary>
    public RawSquidTransmission(RawSidPool pool, RawSquidTransmissionState state, RawSquidTransmissionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("현재 Squid 전송은10.78 전용");
        _pool = pool; _state = state; _hooks = hooks.Copy();
    }

    /// <summary>선택된 함수만 값으로 읽어 자기 연결 교체 중에도 현재 호출 수명을 유지한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("Squid 전송의 선택된 실제 장치 미연결");
    /// <summary>필요한 폭만 little endian으로 읽고 이웃 payload는 섞지 않는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint value = 0;
        // 낮은 바이트부터 선택한 필드 폭만 조합한다.
        for (int index = 0; index < width; index++) value |= (uint)raw[offset + index] << (index * 8);
        return value;
    }
    /// <summary>선택한 WORD/DWORD 폭만 쓰고 공통 버퍼의 나머지는 보존한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 정확한 원문 폭으로 쓴다.
        for (int index = 0; index < width; index++) raw[offset + index] = unchecked((byte)(value >> (index * 8)));
    }
    /// <summary>erase0 헤더의 세 필드만 쓰고 DWORD 길이의 최소4 비교 뒤 낮은 WORD를 기록한다.</summary>
    private static void Header(Span<byte> packet, byte message, byte protocol, uint length)
    {
        packet[0] = (byte)(protocol & 0x7f); packet[1] = message; Put(packet, 2, Math.Max(4u, length), 2);
    }
    /// <summary>원문8비트 좌표 압축의 유한·정수·0..255 동일 비교다.</summary>
    private static bool ByteCoordinate(float value) => float.IsFinite(value) && value >= 0 && value <= 255 && value == MathF.Truncate(value);

    /// <summary>현재 가상+54를 선택해 실제35BYTE/빈 writer·Form/Process writer의 signed 크기를 돌려준다.</summary>
    private int Write(int sid, uint writerArgument)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid); uint table = Read(raw, 0, 4);
        if (table == 0x512098) return Need(_hooks.ProcessPacket)(sid, _state.Buffer, writerArgument);
        if (table == 0x5092d0) return 0;
        if (!RawSquidFactory.SupportsConstructedVtable(_pool.Edition, table))
            return new RawFormPacket(_pool, _state.Forms, _state.Deletion).WriteData(sid, _state.Buffer);
        byte[] packet = _state.Buffer; FormPacketHeader header = _state.Update;
        Header(packet, header.Message, header.Protocol, header.Length); Put(packet, 4, unchecked((uint)sid), 2);
        raw.Slice(14, 8).CopyTo(packet.AsSpan(6)); raw.Slice(12, 2).CopyTo(packet.AsSpan(14));
        packet[16] = raw[10]; packet[17] = raw[11]; packet[18] = raw[34];
        raw.Slice(42, 8).CopyTo(packet.AsSpan(19)); raw.Slice(26, 2).CopyTo(packet.AsSpan(27)); packet[29] = raw[35];
        raw.Slice(36, 4).CopyTo(packet.AsSpan(30)); packet[34] = raw[40];
        if (_state.Compress == 0) return 35;
        return Compress(packet.AsSpan(0, 35).ToArray());
    }

    /// <summary>같은 버퍼를 덮기 전에 저장한35BYTE에서0..15 필드 순서와 현재 소유자/누적을 인코딩한다.</summary>
    private int Compress(byte[] input)
    {
        SquidPopTypeReference type = Need(_hooks.Type)(input[16]); byte[] data = new byte[64]; int count = 0; uint mask = 0;
        // 선택한 입력 필드의 낮은 BYTE/WORD를 순서대로 붙이고 실제 마스크 비트를 세운다.
        void Append(int offset, int width, int field)
        {
            if (count + width > data.Length) throw new ArgumentOutOfRangeException(nameof(input), "압축 Squid의 원문64BYTE 범위");
            input.AsSpan(offset, width).CopyTo(data.AsSpan(count)); count += width; mask |= 1u << field;
        }
        float x = BitConverter.UInt32BitsToSingle(Read(input, 6, 4)), y = BitConverter.UInt32BitsToSingle(Read(input, 10, 4));
        if (ByteCoordinate(x) && ByteCoordinate(y)) { data[count++] = (byte)x; data[count++] = (byte)y; mask |= 1; }
        else Append(6, 8, 1);
        uint quantity = Read(input, 14, 2); if (quantity != 0) Append(14, quantity <= 255 ? 1 : 2, quantity <= 255 ? 2 : 3);
        if (input[16] != unchecked((byte)Need(_hooks.DefaultType)())) Append(16, 1, 4);
        if ((input[17] & 0xef) != 0) Append(17, 1, 5);
        if ((input[17] & 0x10) != 0) mask |= 0x40;
        if (_state.SameOwnerEnabled != 0 && _state.LastOwner != 0xffffff9d && input[18] == _state.LastOwner) mask |= 0x80;
        else { Append(18, 1, 8); _state.LastOwner = input[18]; }
        if ((type.Current.Flags1 & 0x10) != 0)
        {
            uint hp = Read(input, 27, 2);
            if (unchecked((short)hp) != Need(_hooks.MaxHp)(input[16])) Append(27, hp <= 255 ? 1 : 2, hp <= 255 ? 9 : 10);
        }
        if (unchecked((sbyte)input[29]) != type.Current.ZOrder) Append(29, 1, 11);
        if (Read(input, 30, 4) != 0) Append(30, 1, 12);
        Append(34, 1, 13);
        byte[] packet = _state.Buffer; SquidCompressedPacketHeader header = _state.Compressed;
        Header(packet, header.Message, header.Protocol, header.Length); Put(packet, 4, Read(input, 4, 2), 2); Put(packet, 6, mask, 2);
        data.AsSpan(0, count).CopyTo(packet.AsSpan(9)); packet[8] = (byte)count;
        uint length = (uint)count + 9; _state.CompressedCount = unchecked(_state.CompressedCount + 1); _state.CompressedBytes = unchecked(_state.CompressedBytes + length);
        if ((_state.CompressedCount & 0xff) == 0xff)
        {
            int numerator = unchecked((int)(_state.CompressedBytes * 100u)), denominator = unchecked((int)(_state.CompressedCount * 35u));
            if (denominator == 0 || (numerator == int.MinValue && denominator == -1)) throw new ArgumentOutOfRangeException(nameof(input), "압축 Squid 누적 비율의 원문 idiv 범위");
            string text = "compressedUpdateSquid " + (numerator / denominator).ToString(CultureInfo.InvariantCulture) + "%\n";
            Need(_hooks.Trace)(4, text);
        }
        return (int)length;
    }

    /// <summary>보호된 자산/네 Form의 실제 가상+58을 선택해 같은 삭제 표식을 소비한다.</summary>
    private int WriteDeletion(int sid, uint flags)
    {
        uint table = Read(_pool.Slot(sid), 0, 4);
        if (!RawSquidFactory.SupportsConstructedVtable(_pool.Edition, table) && table != 0x512110 && table != 0x512098 && table != 0x512188 && table != 0x512208)
            throw new InvalidOperationException("현재 Squid 삭제 가상+58 미복원");
        return new RawFormPacket(_pool, _state.Forms, _state.Deletion).WriteDeletion(sid, flags, _state.Buffer, _hooks.Assertion);
    }

    /// <summary>공통 진단 뒤 -2만 삭제 writer를 고르고, 양수 크기의 현재 대상/방송 연결을 실제로 실행한다.</summary>
    public void Transmit(int sid, int target, int selector, uint writerArgument, uint flags)
    {
        if (_state.ClientSidAllowed == 0 && sid >= 5 && sid < 15000) Need(_hooks.Assertion)("!isClientSid()", @"\Ns\O\Squid.cpp", 0x2d8);
        int size = selector == -2 ? WriteDeletion(sid, flags) : Write(sid, writerArgument);
        if (size >= 2048) Need(_hooks.Assertion)("transmitSize < TRANSMIT_BUFFER_SIZE", @"\Ns\O\Squid.cpp", 0x2e3);
        if (size > 0)
        {
            if (size > _state.Buffer.Length) throw new ArgumentOutOfRangeException(nameof(size), "Squid 송신 공통 버퍼 범위");
            ReadOnlyMemory<byte> packet = _state.Buffer.AsMemory(0, size);
            if (target == -1)
            {
                if (_hooks.BroadcastMutable is not null) Need(_hooks.BroadcastMutable)(_state.Buffer);
                else Need(_hooks.Broadcast)(packet);
            }
            else if (Need(_hooks.Connected)(target))
            {
                if (_hooks.SendMutable is not null) Need(_hooks.SendMutable)(target, _state.Buffer);
                else Need(_hooks.Send)(target, packet);
            }
        }
    }

    /// <summary>일반 Pop의 원문(-1,-1,0,flags) 인자를 같은 공통 송신 부모에 전달한다.</summary>
    public void Pop(int sid, uint flags) => Transmit(sid, -1, -1, 0, flags);
    /// <summary>동기화의 대상 번호·현재 writer 인자를 유지하며 방송으로 바꾸지 않는다.</summary>
    public void SendTo(int sid, int target, uint writerArgument) => Transmit(sid, target, -3, writerArgument, 1);
    /// <summary>모드가 같아도 원문처럼 최근 소유자를 signed -99 비트로 초기화한다.</summary>
    public void SameOwnerMode(uint mode) { _state.SameOwnerEnabled = mode; _state.LastOwner = 0xffffff9d; }
}
