// 일반 채팅 패킷 작성·채널 선택과 송신 길이 확정을 공유한다.
// 근거: cpppj `src/client/RawChatPacket.h`·`RawChatPacket.cpp`(0042d280·0040a8d0).
// 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Client.Chat;

// 채팅 패킷 범주를 담는 네임스페이스다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.

// 채팅 패킷이 읽는 원문 전역 주소와 종류·버전 상수를 담는 정적 클래스다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
public static class RawChatPacket
{
    /// <summary>현재 구독자 번호를 읽는 원문 전역 주소다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public const uint SubscriberAddress = 0x540c74;

    /// <summary>전투 모드를 읽는 원문 전역 주소다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public const uint BattleAddress = 0x594fbc;

    /// <summary>1번 채널 선택을 읽는 원문 전역 주소다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public const uint ChannelOneAddress = 0x594fb8;

    /// <summary>메타맵 채널 선택을 읽는 원문 전역 주소다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public const uint MetamapAddress = 0x594fc8;

    /// <summary>채팅 패킷 종류 바이트다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public const byte PacketKind = 0x41;
}

// 채팅 패킷 작성이 부르는 외부 효과 묶음이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
public sealed class ChatPacketHooks
{
    /// <summary>같은 매니저와 수신자 할당의 실제 물리를 빌린다. 조회 자체는 순수해야 한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Func<uint, byte[]>? Memory { get; set; }

    /// <summary>첫 NUL을 포함한 현재 이름의 물리를 빌린다. 추가 뒤에도 처음 포인터를 읽는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Func<byte[]>? CurrentName { get; set; }

    /// <summary>현재 구독자·세 모드·초기 패킷 헤더의 원문 전역을 읽는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Func<uint, uint>? Read { get; set; }

    /// <summary>같은 대상 매니저에 누락된 자기 구독자를 추가한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Action<uint, uint>? AddRecipient { get; set; }

    /// <summary>실제 선택 채널로 보낸다. 같은 패킷의 감시는 이 경계에서 실행한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Action<int, byte[]>? Send { get; set; }

    /// <summary>채널이 하나도 없을 때만 원문 오류를 기록한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Action<uint, string>? Log { get; set; }

    /// <summary>0.0.0.0 주소 장치를 준비한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Action? ZeroAddress { get; set; }

    /// <summary>위 주소로 수신을 호출한다. 수신 자체를 빈 성공으로 바꾸지 않는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    public Action<byte[]>? LocalReceive { get; set; }
}

// 채팅 일반 패킷 작성·송신 길이 확정 본체다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
public static class ChatPacketBody
{
    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("채팅 패킷의 선택된 실제 장치 미연결");
        }

        return callback;
    }

    /// <summary>명시 물리 안의 실제 바이트·워드·더블워드만 낮은 바이트부터 읽는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="at">읽을 바이트 위치다.</param>
    /// <param name="width">읽을 폭이다. 기본값은 4바이트다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        if (at < 0 || at > bytes.Length || width < 0 || width > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "채팅 패킷 읽기의 실제 물리 부족");
        }

        uint value = 0u;
        // 현재 폭 밖의 인접 바이트를 읽지 않는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            value |= (uint)bytes[at + index] << (8 * index);
        }

        return value;
    }

    /// <summary>주변 꼬리를 유지하며 지정 원문 폭만 쓴다. 워드·바이트 절단도 이 저장에서 발생한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 물리다.</param>
    /// <param name="at">쓸 바이트 위치다.</param>
    /// <param name="value">적을 값이다.</param>
    /// <param name="width">쓸 폭이다. 기본값은 4바이트다.</param>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        if (at < 0 || at > bytes.Length || width < 0 || width > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "채팅 패킷 쓰기의 실제 물리 부족");
        }

        // 낮은 바이트부터 폭만큼 적는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            bytes[at + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>첫 NUL까지의 현재 바이트를 빌린다. 호출자 물리 밖 읽기는 명시적으로 거부한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    private static string Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "채팅 패킷 문자열의 실제 NUL 없음");
        }

        return Encoding.Latin1.GetString(bytes[..end]);
    }

    /// <summary>원문 0042d280의 일반 채팅 패킷 작성·채널 선택과 0040a8d0의 송신 길이 확정을 잇는다. 본문은 호출자의 명시 길이 전체다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    /// <param name="manager">읽을 매니저 토큰이다.</param>
    /// <param name="recipients">읽고 고칠 수신자 매니저 토큰이다.</param>
    /// <param name="body">실을 본문이다. 호출자의 명시 길이 전체를 쓴다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static uint SendChatPacket(uint manager, uint recipients, ReadOnlySpan<byte> body, ChatPacketHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        byte[] Name() => Need(hooks.CurrentName)();
        byte[] Memory(uint token) => Need(hooks.Memory)(token);
        uint Global(uint address) => Need(hooks.Read)(address);
        byte[] name = Name();
        const string nameless = "++Nameless++";
        bool empty = Text(name).Length == 0;
        bool found = false;
        byte[] target = Memory(recipients);
        uint count = Read(target, 0);
        // 현재 개수와 실제 배열에서 자기 구독자 중복을 찾는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (uint index = 0u; unchecked((int)index) < unchecked((int)count); ++index)
        {
            if (Read(Memory(Read(target, 8)), unchecked((int)(index * 4u))) == Global(RawChatPacket.SubscriberAddress))
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            Need(hooks.AddRecipient)(recipients, Global(RawChatPacket.SubscriberAddress));
        }

        string battleText = string.Empty;
        bool transformed = false;
        if (Global(RawChatPacket.BattleAddress) != 0u)
        {
            uint flags = Read(Memory(manager), 16);
            if (Read(Memory(manager), 12) != 0u)
            {
                flags |= 0x100u;
            }

            battleText = unchecked((int)flags).ToString();
            while (battleText.Length < 3)
            {
                battleText = "0" + battleText;
            }

            battleText += Text(body);
            if (Global(RawChatPacket.BattleAddress) != 0u)
            {
                transformed = true;
            }
        }

        string actualName = empty ? nameless : Text(name);
        int nameBytes = Encoding.Latin1.GetByteCount(actualName);
        int nameSize = nameBytes + 1;
        byte[] message = transformed ? Encoding.Latin1.GetBytes(battleText) : body.ToArray();
        uint total = Read(Memory(recipients), 0);
        int recipientsSize = unchecked((int)(total * 4u));
        if (nameSize > 0x7fff || message.Length > 0xffff || recipientsSize > Memory(Read(Memory(recipients), 8)).Length)
        {
            throw new ArgumentOutOfRangeException(nameof(body), "채팅 패킷의 실제 이름·본문·수신자 복사 범위 부족");
        }

        byte[] packet = new byte[15 + nameSize + message.Length + recipientsSize];
        // 원문 fill의 미작성과 뒤쪽 실제 복사까지 안전하게 두는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < packet.Length; ++index)
        {
            packet[index] = 0xff;
        }

        packet[1] = (byte)Global(0x52edf6);
        Put(packet, 2, Math.Max(Global(0x544c5c) & 0xffffu, 4u), 2);
        packet[0] = 3;
        packet[5] = 1;
        Put(packet, 6, Global(RawChatPacket.SubscriberAddress));
        Put(packet, 10, (uint)nameSize, 2);
        Put(packet, 12, (uint)message.Length, 2);
        Put(packet, 14, total, 1);
        // 본문을 실제 복사 위치에 옮기는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < message.Length; ++index)
        {
            packet[15 + nameSize + index] = message[index];
        }

        byte[] nameRaw = Encoding.Latin1.GetBytes(actualName);
        // 이름을 실제 복사 위치에 옮기는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < nameRaw.Length; ++index)
        {
            packet[15 + index] = nameRaw[index];
        }

        packet[15 + nameRaw.Length] = 0;
        byte[] array = Memory(Read(Memory(recipients), 8));
        // 수신자 배열을 실제 복사 위치에 옮기는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < recipientsSize; ++index)
        {
            packet[15 + nameSize + message.Length + index] = array[index];
        }

        if (Global(RawChatPacket.ChannelOneAddress) != 0u)
        {
            Need(hooks.Send)(1, packet);
        }
        else if (Global(RawChatPacket.MetamapAddress) != 0u)
        {
            Need(hooks.Send)(11, packet);
        }
        else if (Global(RawChatPacket.BattleAddress) != 0u)
        {
            Need(hooks.Send)(9, packet);
        }
        else
        {
            Need(hooks.Log)(9u, "ERROR: No transceiver on which to chat!\n");
            Need(hooks.ZeroAddress)();
            Need(hooks.LocalReceive)(packet);
        }

        return 1u;
    }

    /// <summary>송신 중일 때만 현재 워드·워드·바이트를 합산해 길이를 확정한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    /// <param name="packet">고를 패킷 물리다. 호출자가 소유한다.</param>
    /// <param name="sending">송신 중이면 0이 아니다.</param>
    /// <param name="assertion">부를 원문 진단이다. 도달한 분기마다 필수로 요구한다.</param>
    public static void ObserveChatPacket(byte[] packet, uint sending, Action<uint> assertion)
    {
        ArgumentNullException.ThrowIfNull(packet);
        ArgumentNullException.ThrowIfNull(assertion);
        if (sending == 0u)
        {
            return;
        }

        if (Read(packet, 10, 2) == 0xffffu)
        {
            Need(assertion)(0x81du);
        }

        if (Read(packet, 14, 1) == 0xffu)
        {
            Need(assertion)(0x81fu);
        }

        uint length = Read(packet, 14, 1) * 4u + Read(packet, 12, 2) + Read(packet, 10, 2) + 15u;
        Put(packet, 2, length, 2);
    }
}
