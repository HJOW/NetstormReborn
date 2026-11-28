using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Mission;

/// <summary>Client가 미션 요청과 이후 State 실행에 공유하는 현재 저장소다.</summary>
public sealed class MissionFortRequestGlobals
{
    /// <summary>독립 요청만 소유하는36DWORD이며 실제 Client 결선에서는 만들지 않는다.</summary>
    private readonly uint[]? _next;
    /// <summary>실제 Client의 같은 next144BYTE다. 독립 요청에서는 비어 있다.</summary>
    private readonly Memory<byte> _stateNext;
    /// <summary>독립 요청의 쓰기 가능한 원문9BYTE 주소다.</summary>
    private readonly byte[]? _localAddress;
    /// <summary>실제 주소의 비소유 뷰이며 선택 효과 뒤 현재 바이트를 읽는다.</summary>
    private readonly ReadOnlyMemory<byte> _address;
    /// <summary>원문005c85c0의 실제 상자 또는 독립 요청의 자체값이다.</summary>
    private readonly UIntBox _sequence;
    /// <summary>현재 게임 포트의 실제 읽기다. 독립 요청에서는 자체값을 쓴다.</summary>
    private readonly Func<uint>? _readPort;
    /// <summary>독립 요청의 현재 포트다.</summary>
    private uint _port;
    /// <summary>독립 조합의 기존 배열이다. 실제 결선에서 사본 배열을 제공하지 않는다.</summary>
    public uint[] Next => _next ?? throw new InvalidOperationException("실제 next는 DWORD 접근 또는 바이트 뷰를 사용해야 합니다");
    /// <summary>독립 조합의 기존 주소다. 실제 결선은 Address 뷰를 읽는다.</summary>
    public byte[] LocalAddress => _localAddress ?? throw new InvalidOperationException("실제 주소는 비소유 Address 뷰를 사용해야 합니다");
    /// <summary>현재 원문 주소를 복사 없이 빌린다.</summary>
    public ReadOnlyMemory<byte> Address => _localAddress is not null ? _localAddress : _address;
    /// <summary>복제 효과가 바꾼 현재 순번도 같은 상자로 즉시 읽고 쓴다.</summary>
    public uint Sequence { get => _sequence.Value; set => _sequence.Value = value; }
    /// <summary>현재 포트를 선택 위치에서 읽으며 실제 const 전역의 대입은 거부한다.</summary>
    public uint Port { get => _readPort is not null ? _readPort() : _port; set { if (_readPort is not null) throw new InvalidOperationException("실제 포트는 원래 소유자에서 변경해야 합니다"); _port = value; } }

    /// <summary>기존 독립 원문 대조용 저장소를 만든다.</summary>
    public MissionFortRequestGlobals() { _next = new uint[36]; _localAddress = new byte[9]; _sequence = new(); }
    /// <summary>실제 Client의 next·예약 순번·주소·현재 포트만 빌리고 초기화하지 않는다.</summary>
    public MissionFortRequestGlobals(LocalMissionStateStorage state, UIntBox sequence, ReadOnlyMemory<byte> address, Func<uint> port)
    {
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(sequence); ArgumentNullException.ThrowIfNull(port);
        if (state.Next.Length != 144 || address.Length < 9) throw new ArgumentException("Fort 요청의 실제 next144BYTE·주소9BYTE 필요");
        _stateNext = state.Next; _sequence = sequence; _address = address; _readPort = port;
    }
    /// <summary>같은 next의 현재 한 DWORD를 원문 little endian으로 읽는다.</summary>
    public uint ReadNext(int index) => _next is not null ? _next[index] : BinaryPrimitives.ReadUInt32LittleEndian(_stateNext.Span.Slice(checked(index * 4), 4));
    /// <summary>같은 next의 지정 DWORD만 쓰고 stale 꼬리를 보존한다.</summary>
    public void WriteNext(int index, uint value)
    {
        if (_next is not null) _next[index] = value;
        else BinaryPrimitives.WriteUInt32LittleEndian(_stateNext.Span.Slice(checked(index * 4), 4), value);
    }
    /// <summary>요청 작성 후 관측용 캡처에 전체144BYTE를 복사한다. 실제 next를 대체하지 않는다.</summary>
    public void CopyNextTo(Span<byte> destination)
    {
        if (destination.Length != 144) throw new ArgumentException("요청 캡처144BYTE 필요", nameof(destination));
        // 독립·실제 경로 모두 같은 현재36DWORD를 원문 폭으로 복사한다.
        for (int index = 0; index < 36; index++) BinaryPrimitives.WriteUInt32LittleEndian(destination.Slice(index * 4, 4), ReadNext(index));
    }
}

/// <summary>요청이 소유한 Zone 복사본과 선택된 요새의 수명이다.</summary>
public sealed class MissionFortLoadRequest
{
    /// <summary>요청 선택 효과가 소유 복사한 요새 이름이다.</summary>
    public string Fort { get; set; } = "";
    /// <summary>관찰기가 확정한 실제86BYTE Zone 이미지다.</summary>
    public byte[] Zone { get; internal set; } = [];
    /// <summary>정상 설치 뒤 같은 요청의 재소비를 막는다.</summary>
    public bool Consumed { get; set; }
    /// <summary>Init 직후의 전체 next 캡처이며 현재 State7 블록을 덮는 데 사용하지 않는다.</summary>
    public byte[] NextSnapshot { get; } = new byte[144];
    /// <summary>요청 직후 관측한 순번이며 실제 ZonePending 상자와 별개다.</summary>
    public uint Sequence { get; set; }
    /// <summary>실제 State7이 정리 전에 보관한 지역값이며 미캡처 때만 null이다.</summary>
    public LocalMissionStartingSnapshot? Starting { get; set; }
}

/// <summary>실제 요새 선택과 패킷 복제 효과다.</summary>
public sealed class MissionFortRequestHooks
{
    public Action<string>? SelectFort { get; init; }
    public Func<byte[], uint>? Duplicate { get; init; }
}

/// <summary>현재 Guide의 Subscriber 변경과 복귀 뒤 handle·Zone 복제를 실제 세션에 위임한다.</summary>
public sealed class GuideFortRequestHooks
{
    /// <summary>같은 Template/설정/My를 변경하는 실제 Subscriber 부모다.</summary>
    public Action<uint, string>? Subscriber { get; init; }
    /// <summary>Subscriber 효과가 돌아온 뒤 현재 원문 handle을 읽는다.</summary>
    public Func<string>? Handle { get; init; }
    /// <summary>선언 길이4의512BYTE를 관찰·복제하고 소유 토큰을 반환한다.</summary>
    public Func<byte[], uint>? Duplicate { get; init; }
}

/// <summary>원본 004b51e0 요새 요청과 로컬 Zone 패킷을 작성한다.</summary>
public static class MissionFortRequest
{
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("Fort 요청 실제 효과 누락");
    private static void Put(byte[] buffer, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(buffer.AsSpan(offset, 4), value);

    /// <summary>512바이트 물리 저장소에 원본의 네 클라이언트 슬롯을 만든다.</summary>
    public static byte[] MakeLocalMissionZone()
    {
        byte[] packet = new byte[512]; Array.Fill(packet, (byte)0xff);
        packet[0] = 1; packet[1] = 0x1a;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), 86);
        Put(packet, 4, 0x82000004); packet[8] = packet[9] = 0; Put(packet, 10, 0); packet[14] = 1;
        string[] names = ["zsimpson", "jgreer", "kdemarest", "tbratton"];
        for (int i = 0; i < names.Length; i++)
        {
            int offset = 15 + i * 71; packet[offset] = (byte)i; Put(packet, offset + 1, (uint)i + 1); Put(packet, offset + 5, 0);
            int length = Encoding.ASCII.GetBytes(names[i], packet.AsSpan(offset + 9)); packet[offset + 9 + length] = 0; packet[offset + 41] = 0;
        }
        return packet;
    }

    /// <summary>실제 요새 선택 후 현재 주소와 포트를 쓰고 복제 뒤 순번을 증가한다.</summary>
    public static void Write(string name, MissionFortRequestGlobals globals, MissionFortRequestHooks hooks)
    {
        if (globals.Address.Length < 9) throw new ArgumentException("Fort 요청 현재9BYTE 주소 누락", nameof(globals));
        int nul = name.IndexOf('\0'); Need(hooks.SelectFort)(nul < 0 ? name : name[..nul]);
        byte[] packet = MakeLocalMissionZone(); packet[2] = 4; packet[3] = 0;
        globals.WriteNext(0x54 / 4, BinaryPrimitives.ReadUInt32LittleEndian(globals.Address.Span[..4]));
        globals.WriteNext(0x58 / 4, BinaryPrimitives.ReadUInt32LittleEndian(globals.Address.Span.Slice(4, 4)));
        globals.WriteNext(0x5c / 4, (globals.ReadNext(0x5c / 4) & 0xffffff00u) | globals.Address.Span[8]);
        globals.WriteNext(0, 7); globals.WriteNext(0x7c / 4, globals.Port);
        globals.WriteNext(0x80 / 4, Need(hooks.Duplicate)(packet)); globals.Sequence = unchecked(globals.Sequence + 1);
    }

    /// <summary>Fort를 재선택하지 않고 Champion 변경 뒤 현재 한 슬롯과 실제 next/예약 순번을 작성한다.</summary>
    public static void WriteCurrentGuide(MissionFortRequestGlobals globals, GuideFortRequestHooks hooks)
    {
        if (globals.Address.Length < 9) throw new ArgumentException("Guide Fort 요청 현재9BYTE 주소 누락", nameof(globals));
        Need(hooks.Subscriber)(1, "Champion");
        byte[] packet = LocalMissionPackets.MakeGuideMissionZone(Need(hooks.Handle)()).Storage;
        globals.WriteNext(0x54 / 4, BinaryPrimitives.ReadUInt32LittleEndian(globals.Address.Span[..4]));
        globals.WriteNext(0x58 / 4, BinaryPrimitives.ReadUInt32LittleEndian(globals.Address.Span.Slice(4, 4)));
        globals.WriteNext(0x5c / 4, (globals.ReadNext(0x5c / 4) & 0xffffff00u) | globals.Address.Span[8]);
        globals.WriteNext(0, 7); globals.WriteNext(0x7c / 4, globals.Port);
        globals.WriteNext(0x80 / 4, Need(hooks.Duplicate)(packet)); globals.Sequence = unchecked(globals.Sequence + 1);
    }

    /// <summary>관찰기의 실제 길이를 확정해 요청이 소유한 복사본을 만든다.</summary>
    public static uint CaptureZone(MissionFortLoadRequest request, byte[] packet)
    {
        if (request.Consumed || request.Zone.Length != 0 || packet.Length != 512) throw new InvalidOperationException("Fort 요청 Zone 중복/예약 크기 오류");
        int length = 15 + packet[14] * 71;
        if (length > packet.Length) throw new ArgumentOutOfRangeException(nameof(packet));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), (ushort)length);
        request.Zone = packet.AsSpan(0, length).ToArray(); return 1;
    }
}
