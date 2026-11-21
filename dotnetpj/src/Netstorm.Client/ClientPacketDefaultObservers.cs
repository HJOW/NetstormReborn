using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 장기 기본 표의 C3/관찰 전용18행과 실제 송신 길이 관찰기를 공유한다.
namespace Netstorm.Client;

/// <summary>cpppj/src/client/ClientCampaignTransport.cpp의 기본18행과 ObserveClientPacketDefault다. 전체 설치 부모는 별도다.</summary>
public static class ClientPacketDefaultObservers
{
    /// <summary>원문 기본 등록 한 행의 종류/길이/C3/가변 관찰 여부다.</summary>
    /// <param name="Type">패킷 종류다.</param><param name="Length">고정 길이WORD이며ffff는 가변이다.</param>
    /// <param name="EmptyReceiver">실제4437f0 RET 수신기를 설치한다.</param><param name="MutableObserver">같은 현재 버퍼를 읽는 관찰기다.</param>
    private readonly record struct Entry(uint Type, ushort Length, bool EmptyReceiver, bool MutableObserver);
    /// <summary>관찰 전용11종과 실제 C3 수신7종이다. 폐기80h와 다른 등록은 포함하지 않는다.</summary>
    private static readonly Entry[] Entries =
    [
        new(0x0c,0xffff,true,true), new(0x42,12,true,false), new(0x4d,10,true,false),
        new(0x55,0xffff,true,true), new(0x56,0xffff,true,true), new(0x57,0xffff,true,true), new(0x5a,5,true,false),
        new(0x10,8,false,false), new(0x11,12,false,false), new(0x12,12,false,false), new(0x16,0xffff,false,true),
        new(0x17,14,false,false), new(0x1c,0xffff,false,true), new(0x1e,14,false,false), new(0x1f,12,false,false),
        new(0x68,5,false,false), new(0x85,0xffff,false,true), new(0x87,274,false,false),
    ];
    /// <summary>현재 물리의 지정 BYTE/WORD/DWORD만 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> packet, int offset, int width)
    {
        if (offset < 0 || offset > packet.Length || width > packet.Length - offset) throw new ArgumentOutOfRangeException(nameof(packet), "기본 관찰기 실제 필드 물리 부족");
        return width switch { 1 => packet[offset], 2 => BinaryPrimitives.ReadUInt16LittleEndian(packet.Slice(offset, 2)), 4 => BinaryPrimitives.ReadUInt32LittleEndian(packet.Slice(offset, 4)), _ => throw new ArgumentOutOfRangeException(nameof(width)) };
    }
    /// <summary>같은 버퍼의 선언WORD만 기록하며16비트 감김과 주변 바이트를 보존한다.</summary>
    private static void Length(Span<byte> packet, uint value)
    {
        if (packet.Length < 4) throw new ArgumentOutOfRangeException(nameof(packet), "기본 관찰기 선언 WORD 물리 부족");
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Slice(2, 2), unchecked((ushort)value));
    }
    /// <summary>선택한 sentinel 진단만 요구한다. 정상 복귀 뒤 필드는 같은 현재 물리에서 다시 읽는다.</summary>
    private static void Assert(Action<string, string, uint>? assertion, string expression, uint line) =>
        (assertion ?? throw new InvalidOperationException("기본 관찰기 실제 진단 미연결"))(expression, @"\Ns\O\_p_packets.cpp", line);

    /// <summary>수신 방향은 버퍼도 읽지 않는다. 송신은 원문 현재 필드/지정 폭/진단 순서로 같은 길이WORD만 쓴다.</summary>
    public static void Observe(uint type, Memory<byte> packet, uint sending, Action<string, string, uint>? assertion = null)
    {
        if (sending == 0) return;
        Span<byte> bytes = packet.Span;
        switch (type)
        {
            case 0x0b:
            case 0x0c:
                if (Read(bytes, 8, 4) == uint.MaxValue) Assert(assertion, "extraDataLen != (int)-1", type == 0x0b ? 0x36cu : 0x38cu);
                Length(bytes, Read(bytes, 8, 2) + 12); return;
            case 0x55:
                if (Read(bytes, 4, 4) == uint.MaxValue) Assert(assertion, "count != (int)-1", 0x8dc);
                Length(bytes, Read(bytes, 4, 2) + 8); return;
            case 0x56:
                if (Read(bytes, 8, 4) == uint.MaxValue) Assert(assertion, "dataLength != (int)-1", 0x8f8);
                Length(bytes, Read(bytes, 8, 2) + 12); return;
            case 0x57:
                if (Read(bytes, 8, 4) == uint.MaxValue) Assert(assertion, "specLength != (int)-1", 0x916);
                Length(bytes, Read(bytes, 8, 2) + 12); return;
            case 0x16:
                if (Read(bytes, 5, 1) == 0xff) Assert(assertion, "numFriends != (char)-1", 0x458);
                Length(bytes, unchecked((uint)(unchecked((sbyte)Read(bytes, 5, 1)) * 4 + 14))); return;
            case 0x1c:
                if (Read(bytes, 45, 1) == 0xff) Assert(assertion, "nameLen != (unsigned char)-1", 0x4ca);
                Length(bytes, Read(bytes, 45, 1) + 46); return;
            case 0x85:
                if (Read(bytes, 5, 2) == 0xffff) Assert(assertion, "dataLen != (unsigned short)-1", 0xa2c);
                Length(bytes, Read(bytes, 5, 2) + 7); return;
            case 0x7b:
                if (Read(bytes, 9, 1) == 0xff) Assert(assertion, "addrLen != (char)-1", 0xba8);
                Length(bytes, unchecked((uint)(unchecked((sbyte)Read(bytes, 9, 1)) + 10))); return;
            default: throw new ArgumentOutOfRangeException(nameof(type), "미등록 기본 mutable 관찰기 종류");
        }
    }
    /// <summary>장기 실제 진단을 사용하는18행만 설치한다. 관찰 전용 행의 기존 수신기와 나머지 표는 보존한다.</summary>
    public static void Install18(RawPacketDispatch dispatch, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(storage);
        // 원문 목록 순서로 observer/version/길이를 쓰고 C3인7행만 수신기를 교체한다.
        foreach (Entry entry in Entries)
        {
            if (entry.MutableObserver) dispatch.ObservePacket(entry.Type, (packet, sending) => Observe(entry.Type, packet, sending, storage.AssertDisplay));
            else dispatch.Observe(entry.Type, static _ => { }); //40b380의 실제 ret4이며 NULL observer가 아니다.
            dispatch.Version(entry.Type, 1, 0); dispatch.RequireLength(entry.Type, entry.Length);
            if (entry.EmptyReceiver) dispatch.RegisterCanonicalDirect(entry.Type, 0x004437f0, static (_, _, _) => { }); //4437f0의 원문 RET만 실행하며 게임 효과/성공값을 합성하지 않는다.
        }
    }
}
