using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 실제 mutable 패킷과 Client 수명의 마스크를 원문 중계에 빌린다.
namespace Netstorm.Client.Chat;

/// <summary>RawChatRelay.h의 현재 주소/Player/연결/실제 전송과 수신 경계다.</summary>
public sealed class ChatRelayHooks
{
    /// <summary>현재 battle/server/subscriber DWORD 조회다.</summary>
    public Func<uint, uint>? Read { get; set; }
    /// <summary>현재 전송 ID의 실제20BYTE 주소를 값으로 반환한다.</summary>
    public Func<uint, byte[]>? Address { get; set; }
    /// <summary>현재 subscriber의 첫 Player 또는 실패0이다.</summary>
    public Func<uint, uint>? Player { get; set; }
    /// <summary>선택 Player의 현재 signed 전송 번호다.</summary>
    public Func<uint, int>? Channel { get; set; }
    /// <summary>해당 현재 연결의 살아 있는 관문이다.</summary>
    public Func<int, bool>? Connected { get; set; }
    /// <summary>같은 원문 패킷을 선택한 현재 원격 번호로 보낸다.</summary>
    public Action<int, Memory<byte>>? Send { get; set; }
    /// <summary>같은 패킷과 주소 값 사본을 로컬 수신 부모에 전달한다.</summary>
    public Action<Memory<byte>, ReadOnlyMemory<byte>>? Receive { get; set; }
    /// <summary>실제 송신 observer의 선택 진단 줄 번호다.</summary>
    public Action<uint>? Assertion { get; set; }
    /// <summary>등록은 함수 객체를 값 복사하여 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal ChatRelayHooks Copy() => (ChatRelayHooks)MemberwiseClone();
}

/// <summary>RawChatRelay.cpp의 주소→전투 접두→서버 중계→선택 로컬 수신 본체다.</summary>
public static class ChatRelayBody
{
    /// <summary>실제 선택된 장치만 요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("Chat 중계의 실제 선택 장치 미연결");
    /// <summary>현재 실제 물리에서 지정 폭만 낮은 BYTE부터 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        if (at < 0 || at > bytes.Length || width > bytes.Length - at) throw new ArgumentOutOfRangeException(nameof(at), "Chat 중계 실제 필드 범위 부족"); uint value = 0;
        // 지정 폭 이외의 인접 물리는 읽지 않는다.
        for (int index = 0; index < width; index++) value |= (uint)bytes[at + index] << (8 * index);
        return value;
    }
    /// <summary>getter의 signed nameLen으로 같은 물리 본문 주소를 구한다.</summary>
    private static int Body(ReadOnlySpan<byte> bytes)
    {
        int at = 15 + unchecked((short)Read(bytes, 10, 2)); if (at < 0 || at > bytes.Length) throw new ArgumentOutOfRangeException(nameof(bytes), "Chat 중계 실제 signed 본문 주소"); return at;
    }
    /// <summary>첫NUL을 먼저 확인한 뒤 앞3BYTE만 CRT 공백/부호/십진수로 읽는다.</summary>
    private static uint PrefixMask(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "Chat 중계 접두 실제 NUL 없음"); int length = Math.Min(3, end), at = 0;
        // CRT의 여섯 앞 공백만 건너뛰며3BYTE 한도를 보존한다.
        while (at < length && bytes[at] is (byte)' ' or (byte)'\r' or (byte)'\n' or (byte)'\t' or (byte)'\v' or (byte)'\f') at++;
        bool negative = false; if (at < length && bytes[at] is (byte)'-' or (byte)'+') { negative = bytes[at] == '-'; at++; } uint value = 0;
        // 첫 비숫자에서 끝내고 음수는 원문 DWORD로 감긴다.
        while (at < length && bytes[at] is >= (byte)'0' and <= (byte)'9') { value = value * 10 + bytes[at] - '0'; at++; }
        return negative ? unchecked(0u - value) : value;
    }
    /// <summary>같은 패킷의 부분 변경을 보존하고 callback 뒤 현재 데이터/번호를 다시 조회한다.</summary>
    public static void RelayChatPacket(uint sender, Memory<byte> packet, UIntBox mask, ChatRelayHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(mask); ArgumentNullException.ThrowIfNull(hooks); byte[] supplied = Need(hooks.Address)(sender); if (supplied.Length != 20) throw new ArgumentException("Chat 실제 주소20BYTE 필요"); byte[] address = supplied.ToArray();
        if (Need(hooks.Read)(0x594fbc) != 0)
        {
            int body = Body(packet.Span); uint length = Read(packet.Span, 12, 2); long end = body + (long)length; if (end == 0 || end > packet.Length) throw new ArgumentOutOfRangeException(nameof(packet), "Chat 중계 실제 마지막 본문 BYTE 없음"); packet.Span[(int)end - 1] = 0; mask.Value = PrefixMask(packet.Span[body..]);
        }
        bool local = false;
        if (Need(hooks.Read)(0x540bc0) != 0)
        {
            uint count = Read(packet.Span, 14, 1); packet.Span[14] = 1; if (count == 0) return;
            // 처음 읽은 BYTE 수만큼 현재 본문 주소/길이/수신자 배열을 다시 읽는다.
            for (uint index = 0; index < count; index++)
            {
                int start = checked(Body(packet.Span) + (int)Read(packet.Span, 12, 2)); uint subscriber = Read(packet.Span, checked(start + (int)index * 4)); if (start < 0 || start > packet.Length || packet.Length - start < 4) throw new ArgumentOutOfRangeException(nameof(packet), "Chat 중계 실제 DWORD 쓰기 범위 부족"); BinaryPrimitives.WriteUInt32LittleEndian(packet.Span.Slice(start, 4), subscriber);
                if (subscriber == Need(hooks.Read)(0x540c74)) local = true;
                else { uint player = Need(hooks.Player)(subscriber); if (player != 0 && Need(hooks.Connected)(Need(hooks.Channel)(player))) Need(hooks.Send)(Need(hooks.Channel)(player), packet); }
            }
            if (!local) return;
        }
        Need(hooks.Receive)(packet, address);
    }
    /// <summary>원문의 const_cast와 같이 소유한 동일 mutable 물리를 직접 입구에 전달한다.사본은 만들지 않는다.</summary>
    private static Memory<byte> Mutable(ReadOnlyMemory<byte> packet) => MemoryMarshal.AsMemory(packet);
    /// <summary>일반 재등록은 같은 검토된 고정 F를 보존하며 metadata 불일치는 덮어쓰지 않는다.</summary>
    public static void RegisterChatPacket(RawPacketDispatch dispatch, UIntBox mask, ChatRelayHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(mask); ArgumentNullException.ThrowIfNull(hooks); var baseline = dispatch.Registration(0x41);
        if (baseline.CanonicalReceive is { NativeEntry: 0x0042dc90 } selected)
        {
            if (baseline.Version != 1 || baseline.Mismatch != 0 || baseline.Length != 0xffff || baseline.PacketObserver is null || baseline.Observer is not null) throw new InvalidOperationException("Native Chat의 같은 metadata 불일치"); dispatch.RegisterCanonical(0x41, selected); return;
        }
        var copy = hooks.Copy(); dispatch.Version(0x41, 1, 0); dispatch.ObservePacket(0x41, (bytes, sending) => ChatPacketBody.ObserveChatPacket(bytes, sending, copy.Assertion));
        dispatch.Register(0x41, (sender, _, bytes) => RelayChatPacket(sender, Mutable(bytes), mask, copy));
    }
    /// <summary>현재 Client 장치와 같은 마스크 주소만 장기42dc90 F에 보관하고 가변 metadata/observer를 설치한다.</summary>
    public static void RegisterCanonicalChatPacket(RawPacketDispatch dispatch, UIntBox mask, ChatRelayHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(mask); ArgumentNullException.ThrowIfNull(hooks); var copy = hooks.Copy();
        dispatch.Version(0x41, 1, 0); dispatch.RequireLength(0x41, 0xffff); dispatch.ObservePacket(0x41, (packet, sending) => ChatPacketBody.ObserveChatPacket(packet, sending, copy.Assertion));
        dispatch.RegisterCanonicalDirect(0x41, 0x0042dc90, (sender, _, packet) => RelayChatPacket(sender, Mutable(packet), mask, copy));
    }
}
