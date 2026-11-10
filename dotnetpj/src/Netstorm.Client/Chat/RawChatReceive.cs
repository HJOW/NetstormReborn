// 일반 채팅 수신·필터·로그 문맥과 선택 UI 효과를 공유한다.
// 근거: cpppj `src/client/RawChatReceive.h`·`RawChatReceive.cpp`(0042ed30·0042f0e0).
// 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Client.Chat;

// 채팅 수신 범주를 담는 네임스페이스다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.

// 채팅 수신 옵션 열 개의 원문 키를 담는 정적 클래스다. 오타도 원문 그대로 둔다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
public static class RawChatReceive
{
    /// <summary>원문 키 열 개다. 순서대로 읽는다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public static readonly string[] OptionKeys =
    [
        "ShowBattle", "ShowEveryone", "ShowAllies", "ShowObserver", "ShowWisper",
        "ShowMyAllies", "ShowMyEmenys", "ShowMyNeutrals", "ShowChatWindow", "PlaySoundMessage",
    ];
}

// 채팅 수신이 부르는 외부 효과 묶음이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
public sealed class ChatReceiveHooks
{
    /// <summary>현재 모드·식별자·채널 마스크·관계·글자색·Chat 토큰의 원문 슬롯이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Func<uint, uint>? Read { get; set; }

    /// <summary>현재 구독자 무시 목록 조회다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Func<uint, bool>? Ignored { get; set; }

    /// <summary>열 개 옵션을 원문 순서대로 현재 설정에서 읽는다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Func<string, int>? Config { get; set; }

    /// <summary>현재 Identity 조회이며 실패도 초기 소유자 0을 반환한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Func<uint, uint>? Owner { get; set; }

    /// <summary>현재 알림 Gump 사건이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action<uint>? Wake { get; set; }

    /// <summary>실제 상대 메시지의 소리 재생 요청이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action<string>? Sound { get; set; }

    /// <summary>실제 Chat·완성 본문·8 DWORD 문맥이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action<uint, string, uint[]>? Append { get; set; }

    /// <summary>현재 Chat 표시 사건이다. 입력줄 표시와 구별한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action? Show { get; set; }

    /// <summary>새 버전·비ASCII 패킷의 원문 진단이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action<uint, string>? Log { get; set; }

    /// <summary>뉴스 저장과 원문 뉴스 설정 저장 효과를 함께 연결한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action<uint>? SetNewsTime { get; set; }

    /// <summary>시스템 뉴스의 표시 장치다. 일반 수신과 독립이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    public Action<string, uint, uint>? News { get; set; }
}

// 채팅 일반 수신·필터·분배 본체다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
public static class ChatReceiveBody
{
    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("채팅 수신의 선택된 실제 장치 미연결");
        }

        return callback;
    }

    /// <summary>실제 물리 안의 바이트·워드·더블워드를 낮은 바이트부터 읽는다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="at">읽을 바이트 위치다.</param>
    /// <param name="width">읽을 폭이다. 기본값은 4바이트다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        if (at < 0 || at > bytes.Length || width < 0 || width > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "채팅 수신 필드의 실제 물리 부족");
        }

        uint value = 0u;
        // 지정 폭 밖은 읽지 않는 반복문이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            value |= (uint)bytes[at + index] << (8 * index);
        }

        return value;
    }

    /// <summary>이름 길이를 signed 워드로 읽어 본문 시작 signed 주소를 구한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <param name="packet">읽을 패킷 물리다.</param>
    private static int BodyOffset(ReadOnlySpan<byte> packet)
    {
        int offset = 15 + unchecked((short)Read(packet, 10, 2));
        if (offset < 0 || offset > packet.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "채팅 수신 본문의 실제 signed 주소 범위");
        }

        return offset;
    }

    /// <summary>첫 NUL까지의 현재 문자열을 빌린다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="at">읽을 바이트 위치다.</param>
    private static string Text(ReadOnlySpan<byte> bytes, int at)
    {
        if (at < 0 || at > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "채팅 수신 문자열 시작 범위");
        }

        int end = bytes[at..].IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "채팅 수신 문자열의 실제 NUL 없음");
        }

        return Encoding.Latin1.GetString(bytes.Slice(at, end));
    }

    /// <summary>font 5·현재색·기본 두 1을 넣는 같은 8 DWORD 문맥이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <param name="color">들을 현재색이다.</param>
    private static uint[] Context(uint color) => [0u, 5u, color, 0u, 0u, 0u, 1u, 1u];

    /// <summary>일반 수신의 무시·채널·관계 관문과 UI·소리 호출의 현재값 읽기를 잇는다. 먼저 마지막 본문 바이트에 NUL을 쓰고, 무시·필터 뒤에도 그 효과를 유지한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <param name="chat">받을 Chat 번호다.</param>
    /// <param name="packet">읽고 고칠 패킷 물리다. 호출자가 소유한다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static void ReceiveChatPacket(uint chat, byte[] packet, ChatReceiveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(packet);
        ArgumentNullException.ThrowIfNull(hooks);
        uint Global(uint address) => Need(hooks.Read)(address);
        if (Read(packet, 0, 1) > 3u)
        {
            Need(hooks.Log)(29u, "Received new format chat packet.\n");
            return;
        }

        if ((Read(packet, 5, 1) & 15u) != 1u)
        {
            Need(hooks.Log)(29u, "Received non-ASCII chat packet.\n");
            return;
        }

        int body = BodyOffset(packet);
        uint length = Read(packet, 12, 2);
        if (body + (long)length == 0L || body + (long)length > packet.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "채팅 수신 마지막 본문 바이트의 실제 물리 부족");
        }

        packet[body + (int)length - 1] = 0;
        if ((Read(packet, 5, 1) & 0x80u) != 0u && Read(packet, 6) == 0u)
        {
            if (unchecked((short)Read(packet, 10, 2)) > 1 && Read(packet, 15, 1) == 1u)
            {
                uint stamp = Read(packet, 19);
                if (stamp <= Global(0x548748u))
                {
                    return;
                }

                Need(hooks.SetNewsTime)(stamp);
            }

            Need(hooks.News)(Text(packet, BodyOffset(packet)), 400u, 30u);
            return;
        }

        if (Need(hooks.Ignored)(Read(packet, 6)))
        {
            return;
        }

        if (Global(0x594fbcu) == 0u)
        {
            Need(hooks.Wake)(Global(0x5ca92cu));
            if (Read(packet, 6) != Global(0x540c74u) && Global(0x594fc8u) == 0u)
            {
                Need(hooks.Sound)("newChatMessage.wav");
            }

            uint[] context = Context(Global(0x5b5dd8u));
            Need(hooks.Append)(chat, Text(packet, BodyOffset(packet)), context);
            Need(hooks.Show)();
            return;
        }

        int[] options = new int[RawChatReceive.OptionKeys.Length];
        // 숨기는 메시지도 열 개 옵션을 모두 원문 순서대로 읽는 반복문이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
        for (int index = 0; index < options.Length; ++index)
        {
            options[index] = Need(hooks.Config)(RawChatReceive.OptionKeys[index]);
        }

        uint mask = Global(0x548710u);
        bool visible = ((mask & 8u) != 0u && options[0] != 0)
            || ((mask & 32u) != 0u && options[1] != 0)
            || ((mask & 64u) != 0u && options[2] != 0)
            || ((mask & 128u) != 0u && options[3] != 0)
            || ((mask & 256u) != 0u && options[4] != 0);
        if (visible)
        {
            uint owner = Need(hooks.Owner)(Read(packet, 6));
            uint relation = 0x595200u + (owner + Global(0x540c70u) * 9u) * 4u;
            if (Global(relation) == 0u)
            {
                visible = options[6] != 0;
            }
            else if (options[5] != 0)
            {
                if (Global(relation) == 0u)
                {
                    visible = options[6] != 0;
                }
            }
            else
            {
                visible = false;
            }

            if (visible && Global(0x52f81eu + owner) != 0u && options[7] == 0)
            {
                visible = false;
            }
        }

        string text = Text(packet, BodyOffset(packet));
        if (Global(0x594fbcu) != 0u)
        {
            if (text.Length < 3)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "채팅 수신의 실제 세 자리 접두 물리 부족");
            }

            text = text.Substring(3) + '\n';
        }

        if (visible)
        {
            Need(hooks.Wake)(Global(0x5ca92cu));
            if (Read(packet, 6) != Global(0x540c74u) && Global(0x594fc8u) == 0u && options[9] != 0)
            {
                Need(hooks.Sound)("newChatMessage.wav");
            }

            uint[] context = Context(Global(0x5b5dd8u));
            Need(hooks.Append)(chat, text, context);
            if (options[8] != 0)
            {
                Need(hooks.Show)();
            }
        }
    }

    /// <summary>현재 Chat 0이면 패킷·다른 장치를 건드리지 않고 조용히 끝낸다. 그 밖에는 같은 실제 수신 부모를 실행한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    /// <param name="packet">읽고 고칠 패킷 물리다. 호출자가 소유한다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static void DispatchChatPacket(byte[] packet, ChatReceiveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(packet);
        ArgumentNullException.ThrowIfNull(hooks);
        uint chat = Need(hooks.Read)(0x548740u);
        if (chat != 0u)
        {
            ReceiveChatPacket(chat, packet, hooks);
        }
    }
}
