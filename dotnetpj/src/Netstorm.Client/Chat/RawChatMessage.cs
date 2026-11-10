// 일반 채팅 본문의 채널·대상·이름 조합과 say/wrapper 호출 순서를 공유한다.
// 근거: cpppj `src/client/RawChatMessage.h`·`RawChatMessage.cpp`(0042f1e0·0042dc30·0042d640).
// 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Client.Chat;

// 채팅 본문 조합 범주를 담는 네임스페이스다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.

// 채팅 본문 출력 오프셋·이름 버퍼 상수를 담는 정적 클래스다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public static class RawChatMessage
{
    /// <summary>매니저 2712바이트 안의 2048바이트 본문 출력 시작 오프셋이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public const int MessageOffset = 0x298;

    /// <summary>실제 대상 이름 지역 버퍼 폭 256바이트다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public const int NameBytes = 256;
}

// 채팅 본문 조합이 함께 보는 현재 매니저·구독자 번호다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public sealed class ChatMessageGlobals
{
    /// <summary>현재 매니저 번호다. say의 선행 관문과 wrapper 호출에 쓴다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public uint Current { get; set; }

    /// <summary>현재 구독자 번호다. 첫 비자기 수신자를 찾을 때 읽는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public uint Subscriber { get; set; }
}

// 바이트 구간에 제자리로 쓰는 채팅 이름 서식 효과 형식이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public delegate void ChatNameFormat(Span<byte> output, string pattern, string name, string other);

// 바이트 구간에 제자리로 쓰는 채팅 슬롯 제거 효과 형식이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public delegate void ChatSpanAction(Span<byte> output);

// 채팅 본문 조합이 부르는 외부 효과 묶음이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public sealed class ChatMessageHooks
{
    /// <summary>매니저·수신자 배열의 같은 실제 물리를 빌린다. 조회 자체는 순수해야 한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public Func<uint, byte[]>? Memory { get; set; }

    /// <summary>현재 이름 포인터의 C 문자열이다. 빈 값은 ++Nameless++로 바뀐다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public Func<string>? CurrentName { get; set; }

    /// <summary>현재 이름 또는 SOMEONE을 목적 포인터에 쓰고 0·비영 토큰을 돌려준다. 조회 성공 여부가 아니다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public Func<byte[], uint, uint>? NameForSubscriber { get; set; }

    /// <summary>같은 목적 위치에 세 원문 CRT 서식을 작성한다. 기본값은 원본 고정 서식과 같다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public ChatNameFormat FormatName { get; set; } = ChatMessageBody.WriteChatNameFormat;

    /// <summary>실제 제자리 슬롯 제거 본체다. 기본값은 원본 0042d160과 같다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public ChatSpanAction StripSlots { get; set; } = ChatMessageBody.DefaultStripSlots;

    /// <summary>출력 this·대상 매니저·NUL 포함 현재 본문을 0042d280에 넘긴다. 패킷·전송을 빈 성공으로 대체하지 않는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public Func<uint, uint, byte[], uint>? Deliver { get; set; }

    /// <summary>say 빈 본문의 0042f270(0) 효과다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    public Action<uint>? InputVisible { get; set; }
}

// 채팅 일반 본문 조합·입력 wrapper·say 관문 본체다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public static class ChatMessageBody
{
    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("채팅 본문 조합의 실제 장치 미연결");
        }

        return callback;
    }

    /// <summary>실제 물리 안의 첫 NUL까지 Latin1 문자열로 읽는다. NUL이 없으면 멈춘다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    private static string Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "채팅 본문의 실제 NUL 물리 없음");
        }

        return Encoding.Latin1.GetString(bytes[..end]);
    }

    /// <summary>같은 토큰의 순수 물리를 다시 빌린다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="hooks">부를 외부 효과다.</param>
    /// <param name="token">빌릴 매니저 토큰이다.</param>
    private static byte[] Memory(ChatMessageHooks hooks, uint token) => Need(hooks.Memory)(token);

    /// <summary>실제 비정렬 DWORD의 네 바이트를 little endian으로 읽는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="at">읽을 바이트 위치다.</param>
    private static uint Read(byte[] bytes, int at)
    {
        if (at < 0 || at > bytes.Length || bytes.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "채팅 본문 DWORD 물리 범위 오류");
        }

        uint value = 0u;
        // 네 바이트를 little endian 순서로 모으는 반복문이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
        for (uint index = 0u; index < 4u; ++index)
        {
            value |= (uint)bytes[at + (int)index] << (8 * (int)index);
        }

        return value;
    }

    /// <summary>매니저 안의 현재 출력 물리를 돌려준다. 원문 영역을 임의로 넓히지 않는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="hooks">부를 외부 효과다.</param>
    /// <param name="manager">볼 매니저 토큰이다.</param>
    private static Span<byte> Output(ChatMessageHooks hooks, uint manager)
    {
        byte[] bytes = Memory(hooks, manager);
        if (bytes.Length <= RawChatMessage.MessageOffset)
        {
            throw new ArgumentOutOfRangeException(nameof(manager), "채팅 본문 출력 매니저 물리 부족");
        }

        return bytes.AsSpan(RawChatMessage.MessageOffset);
    }

    /// <summary>입력 NUL 앞부분과 끝 NUL만 순방향으로 복사하며 뒤쪽 흔적은 보존한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="destination">쓸 바이트 구간이다.</param>
    /// <param name="text">옮길 문자열이다. 첫 NUL까지만 본다.</param>
    private static void Copy(Span<byte> destination, string text)
    {
        int nul = text.IndexOf('\0');
        if (nul >= 0)
        {
            text = text.Substring(0, nul);
        }

        if (destination.Length <= text.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(destination), "채팅 본문 복사의 실제 물리 부족");
        }

        // 같은 원문 순방향 문자 복사를 수행하는 반복문이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
        for (int index = 0; index < text.Length; ++index)
        {
            destination[index] = (byte)text[index];
        }

        destination[text.Length] = 0;
    }

    /// <summary>실제 현재 출력 끝을 매번 찾은 뒤 같은 물리에 이어 쓴다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="hooks">부를 외부 효과다.</param>
    /// <param name="manager">쓸 매니저 토큰이다.</param>
    /// <param name="text">이을 문자열이다.</param>
    private static void Append(ChatMessageHooks hooks, uint manager, string text)
    {
        Span<byte> bytes = Output(hooks, manager);
        Copy(bytes.Slice(Text(bytes).Length), text);
    }

    /// <summary>0042d250처럼 signed 개수와 첫 비자기 구독자를 실제 대상 매니저에서 찾는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="recipients">볼 수신자 매니저 토큰이다.</param>
    /// <param name="subscriber">가릴 자기 구독자 번호다.</param>
    /// <param name="hooks">부를 외부 효과다.</param>
    private static uint FirstOther(uint recipients, uint subscriber, ChatMessageHooks hooks)
    {
        if (unchecked((int)Read(Memory(hooks, recipients), 0)) <= 0)
        {
            return 0u;
        }

        uint buffer = Read(Memory(hooks, recipients), 8);
        uint index = 0u;
        // 비교 포인터를 먼저 고정하고 반환할 때 현재 배열 포인터를 다시 읽는 반복문이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
        do
        {
            if (Read(Memory(hooks, buffer), unchecked((int)(index * 4u))) != subscriber)
            {
                return Read(Memory(hooks, Read(Memory(hooks, recipients), 8)), unchecked((int)(index * 4u)));
            }

            ++index;
        }
        while (unchecked((int)index) < unchecked((int)Read(Memory(hooks, recipients), 0)));

        return 0u;
    }

    /// <summary>세 실제 호출 서식만 NUL 포함으로 작성하며 이름 안의 %s를 다시 해석하지 않는다. 그 밖은 명시 오류로 남긴다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="output">쓸 바이트 구간이다.</param>
    /// <param name="pattern">고를 원문 서식이다. 첫 NUL까지만 본다.</param>
    /// <param name="name">앞 이름이다. 첫 NUL까지만 본다.</param>
    /// <param name="other">뒤 대상 이름이다. 첫 NUL까지만 본다.</param>
    public static void WriteChatNameFormat(Span<byte> output, string pattern, string name, string other)
    {
        ArgumentNullException.ThrowIfNull(pattern);
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(other);
        int end = pattern.IndexOf('\0');
        if (end >= 0)
        {
            pattern = pattern.Substring(0, end);
        }

        end = name.IndexOf('\0');
        if (end >= 0)
        {
            name = name.Substring(0, end);
        }

        end = other.IndexOf('\0');
        if (end >= 0)
        {
            other = other.Substring(0, end);
        }

        string result;
        if (pattern == "%s> ")
        {
            result = name + "> ";
        }
        else if (pattern == "%s ")
        {
            result = name + ' ';
        }
        else if (pattern == "%s (to %s)> ")
        {
            result = name + " (to " + other + ")> ";
        }
        else
        {
            throw new InvalidOperationException("채팅 이름의 미지원 원문 서식");
        }

        Copy(output, result);
    }

    /// <summary>기존 0042d160 제거 본체를 출력 구간에 그대로 잇는 기본 효과다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="output">고를 바이트 구간이다. NUL로 끝나야 한다.</param>
    public static void DefaultStripSlots(Span<byte> output)
    {
        int end = output.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(output), "채팅 본문의 실제 NUL 물리 없음");
        }

        byte[] kept = output[..(end + 1)].ToArray();
        ChatFormatBody.StripChatFormatSlots(kept);
        kept.AsSpan().CopyTo(output);
    }

    /// <summary>0042d640처럼 본문 빈 관문→현재 채널·대상·back→이름→현재 front·본문→슬롯 제거→전달 순서로 잇는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="manager">쓸 매니저 토큰이다.</param>
    /// <param name="recipients">볼 수신자 매니저 토큰이다.</param>
    /// <param name="body">읽을 NUL 포함 본문 물리다.</param>
    /// <param name="pattern">쓸 이름 서식이다. 없으면 %s&gt; 를 쓴다.</param>
    /// <param name="nameWork">쓸 256바이트 대상 이름 버퍼다. 호출자가 소유한다.</param>
    /// <param name="globals">읽을 현재 매니저·구독자 번호다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static uint FormatChatMessage(uint manager, uint recipients, ReadOnlySpan<byte> body, string? pattern, byte[] nameWork, ChatMessageGlobals globals, ChatMessageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(nameWork);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Text(body).Length == 0)
        {
            return 0u;
        }

        Output(hooks, manager)[0] = 0;
        uint count = Read(Memory(hooks, recipients), 0);
        uint flags = Read(Memory(hooks, manager), 16);
        if ((flags & 8u) != 0u)
        {
            Append(hooks, manager, "~[Iicon.D36]");
        }

        if ((flags & 0x40u) != 0u)
        {
            Append(hooks, manager, "~[Iicon.D37]");
        }

        if ((flags & 0x80u) != 0u)
        {
            Append(hooks, manager, "~[Iicon.D39]");
        }

        if ((flags & 0x20u) != 0u)
        {
            Copy(Output(hooks, manager), "~[Iicon.D41]");
        }

        if (Read(Memory(hooks, manager), 12) != 0u)
        {
            Append(hooks, manager, count == 1u ? "~[Iicon.D38]" : "~[Iicon.D40]");
        }

        Append(hooks, manager, Text(Memory(hooks, manager).AsSpan(0x14)));
        int nameAt = Text(Output(hooks, manager)).Length;
        if (pattern is null)
        {
            pattern = "%s> ";
        }

        if (nameWork.Length < RawChatMessage.NameBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(nameWork), "채팅 대상 이름의 실제 256바이트 필요");
        }

        nameWork[0] = 0;
        if (Read(Memory(hooks, recipients), 12) != 0u)
        {
            uint other = FirstOther(recipients, globals.Subscriber, hooks);
            if (other != 0u && Need(hooks.NameForSubscriber)(nameWork, other) == 0u)
            {
                nameWork[0] = 0;
            }
        }

        string name = Need(hooks.CurrentName)();
        int nul = name.IndexOf('\0');
        if (nul >= 0)
        {
            name = name.Substring(0, nul);
        }

        if (name.Length == 0)
        {
            name = "++Nameless++";
        }

        Need(hooks.FormatName)(Output(hooks, manager).Slice(nameAt), pattern, name, Text(nameWork));
        if (Read(Memory(hooks, recipients), 12) == 0u)
        {
            Append(hooks, manager, Text(Memory(hooks, manager).AsSpan(0x50)));
        }

        Append(hooks, manager, Text(body));
        Need(hooks.StripSlots)(Output(hooks, manager));
        byte[] bytes = Output(hooks, manager).ToArray();
        int length = Text(bytes).Length + 1;
        byte[] payload = bytes[..length];
        return Need(hooks.Deliver)(manager, recipients, payload);
    }

    /// <summary>0042dc30처럼 현재 입력의 첫 글자와 this의 현재 직접 대상 표시만 세 서식 선택에 쓴다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="manager">쓸 매니저 토큰이다.</param>
    /// <param name="input">읽을 NUL 포함 입력 물리다.</param>
    /// <param name="nameWork">쓸 256바이트 대상 이름 버퍼다. 호출자가 소유한다.</param>
    /// <param name="globals">읽을 현재 매니저·구독자 번호다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static uint FormatChatInput(uint manager, ReadOnlySpan<byte> input, byte[] nameWork, ChatMessageGlobals globals, ChatMessageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(nameWork);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (input.IsEmpty)
        {
            throw new ArgumentOutOfRangeException(nameof(input), "채팅 wrapper의 실제 첫 입력 바이트 없음");
        }

        if (input[0] == (byte)'!')
        {
            return FormatChatMessage(manager, manager, input.Slice(1), "%s ", nameWork, globals, hooks);
        }

        return FormatChatMessage(manager, manager, input, Read(Memory(hooks, manager), 12) != 0u ? "%s (to %s)> " : "%s> ", nameWork, globals, hooks);
    }

    /// <summary>0042f1e0처럼 현재 매니저 0·앞 space·빈 본문 관문을 지키고 하위 전달값과 무관하게 0을 돌려준다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    /// <param name="input">읽을 NUL 포함 입력 물리다.</param>
    /// <param name="nameWork">쓸 256바이트 대상 이름 버퍼다. 호출자가 소유한다.</param>
    /// <param name="globals">읽을 현재 매니저·구독자 번호다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    public static uint SayChatMessage(ReadOnlySpan<byte> input, byte[] nameWork, ChatMessageGlobals globals, ChatMessageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(nameWork);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Current == 0u)
        {
            return 0u;
        }

        if (input.IsEmpty)
        {
            throw new ArgumentOutOfRangeException(nameof(input), "채팅 say의 실제 첫 입력 바이트 없음");
        }

        if (input[0] == (byte)' ')
        {
            return 0u;
        }

        if (input[0] == 0)
        {
            Need(hooks.InputVisible)(0u);
            return 0u;
        }

        FormatChatInput(globals.Current, input, nameWork, globals, hooks);
        return 0u;
    }
}
