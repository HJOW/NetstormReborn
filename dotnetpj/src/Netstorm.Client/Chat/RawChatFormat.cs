// 채팅 형식 캡처(~색·~0~6 칸)의 읽기·쓰기 본체를 공유한다.
// 근거: cpppj `src/client/RawChatFormat.h`·`RawChatFormat.cpp`(0042d160·0042ddf0·0042def0).
// 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Client.Chat;

// 채팅 형식 범주를 담는 네임스페이스다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.

// 캡처 앞부분 80바이트+NUL 1바이트다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
public static class RawChatFormat
{
    /// <summary>캡처 버퍼 크기 81바이트다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public const int CaptureBytes = 81;
}

// 형식 읽기·쓰기가 함께 보는 매니저 번호·구독자 번호다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
public sealed class ChatFormatGlobals
{
    /// <summary>현재 매니저 번호다. 0이면 진단을 먼저 부른다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public uint Current { get; set; }

    /// <summary>부호 있는 10진 ID 키용 구독자 번호다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public uint Subscriber { get; set; }
}

// 형식 읽기·쓰기가 부르는 외부 효과 묶음이다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
public sealed class ChatFormatHooks
{
    /// <summary>매니저 바이트를 빌린다. 터미널 NUL을 찍는 곳이다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public Func<uint, byte[]>? Memory { get; set; }

    /// <summary>매니저 0 진단 번호(앞 184·뒤 157)다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>캡처의 ~0~6 칸을 지운다. 기본값은 원본 0042d160과 같다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public Action<byte[]> StripSlots { get; set; } = ChatFormatBody.StripChatFormatSlots;

    /// <summary>매니저·오프셋·캡처·한도를 복사한다. 앞 0x50·500바이트, 뒤 0x14·60바이트다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public Action<uint, uint, string, uint>? CopyFormat { get; set; }

    /// <summary>캡처 문자열을 Config에 쓴다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public Action<string, string>? WriteConfig { get; set; }

    /// <summary>형식 없음 알림이다. 매치 실패 입력을 그대로 받는다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    public Action<string>? IllegalFormat { get; set; }
}

// 채팅 형식 캡처·매니저·Config 연결 본체다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
public static class ChatFormatBody
{
    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("채팅 형식의 선택된 실제 효과 미연결");
        }

        return callback;
    }

    /// <summary>첫 NUL까지 Latin1 문자열로 읽는다. NUL이 없으면 멈춘다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    private static string Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0);
        if (end < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "채팅 형식의 실제 NUL 범위 오류");
        }

        return Encoding.Latin1.GetString(bytes[..end]);
    }

    /// <summary>스타일 바이트인지 본다. ~·CR·TAB·LF·빈칸은 아니고 VT·FF까지 ASCII로 취급한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    /// <param name="value">볼 바이트 값이다.</param>
    private static bool StyleByte(byte value) =>
        value != (byte)'~' && value != (byte)'\r' && value != (byte)'\t' && value != (byte)'\n' && value != (byte)' ';

    /// <summary>(~[^~CR TAB LF space]+)+의 첫 조각을 탐욕적으로 찾는다. ~가 두 개 이상인 묶음만 본다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    /// <param name="text">찾을 문자열이다. 첫 NUL까지만 본다.</param>
    private static string? Match(string text)
    {
        int nul = text.IndexOf('\0');
        if (nul >= 0)
        {
            text = text.Substring(0, nul);
        }

        // 찾을 수 있는 ~마다 첫 조각 매치를 시도하는 반복문이다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
        for (int first = 0; first < text.Length; ++first)
        {
            if (text[first] != '~')
            {
                continue;
            }

            int cursor = first, end = first;
            while (cursor < text.Length && text[cursor] == '~' && cursor + 1 < text.Length && StyleByte((byte)text[cursor + 1]))
            {
                ++cursor;
                while (cursor < text.Length && StyleByte((byte)text[cursor]))
                {
                    ++cursor;
                }

                end = cursor;
            }

            if (end != first)
            {
                return text.Substring(first, end - first);
            }
        }

        return null;
    }

    /// <summary>0042d160처럼 ~0~6 칸을 지우고 NUL까지 당긴다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    /// <param name="text">고를 가변 버퍼다. NUL로 끝나야 한다.</param>
    /// <exception cref="ArgumentOutOfRangeException">버퍼 끝·NUL 누락이다.</exception>
    public static void StripChatFormatSlots(byte[] text)
    {
        ArgumentNullException.ThrowIfNull(text);
        int cursor = 0;
        // 끝 NUL까지 매번 칸 표시를 찾아 ~0~6을 지우는 반복문이다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
        for (; ; )
        {
            if (cursor >= text.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(text), "채팅 형식의 실제 버퍼 끝 범위 오류");
            }

            if (text[cursor] == 0)
            {
                return;
            }

            if (text[cursor] == (byte)'~' && cursor + 1 < text.Length && text[cursor + 1] >= (byte)'0' && text[cursor + 1] <= (byte)'6')
            {
                int source = cursor + 2, destination = cursor;
                for (; ; )
                {
                    if (source >= text.Length)
                    {
                        throw new ArgumentOutOfRangeException(nameof(text), "채팅 형식의 실제 NUL 누락");
                    }

                    byte value = text[source++];
                    text[destination++] = value;
                    if (value == 0)
                    {
                        break;
                    }
                }
            }
            else
            {
                ++cursor;
            }
        }
    }

    /// <summary>0042ddf0·0042def0처럼 80바이트 캡처·칸 지우기·매니저 복사·터미널 NUL·Config 쓰기를 잇는다. 매치 실패면 불법 알림만 하고 0을 돌려준다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    /// <param name="front">앞 형식이면 참, 뒤 형식이면 거짓이다.</param>
    /// <param name="globals">읽을 매니저·구독자 번호다.</param>
    /// <param name="input">읽을 입력 문자열이다.</param>
    /// <param name="capture">쓸 81바이트 캡처 버퍼다. 호출자가 소유한다.</param>
    /// <param name="hooks">부를 외부 효과다. 도달한 분기마다 필수로 요구한다.</param>
    /// <exception cref="ArgumentOutOfRangeException">캡처 81바이트·매니저 범위 오류다.</exception>
    public static uint ApplyChatFormat(bool front, ChatFormatGlobals globals, string input, byte[] capture, ChatFormatHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(input);
        ArgumentNullException.ThrowIfNull(capture);
        ArgumentNullException.ThrowIfNull(hooks);
        if (globals.Current == 0u)
        {
            Need(hooks.Assertion)(front ? 184u : 157u);
        }

        string? match = Match(input);
        if (match is null)
        {
            int head = input.IndexOf('\0');
            Need(hooks.IllegalFormat)(head < 0 ? input : input.Substring(0, head));
            return 0u;
        }

        if (capture.Length < RawChatFormat.CaptureBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(capture), "채팅 형식 캡처의 실제 81바이트 필요");
        }

        int length = Math.Min(match.Length, 80);
        for (int index = 0; index < length; ++index)
        {
            capture[index] = (byte)match[index];
        }

        capture[length] = 0;
        Need(hooks.StripSlots)(capture);
        uint manager = globals.Current;
        Need(hooks.CopyFormat)(manager, front ? 0x50u : 0x14u, Text(capture), front ? 500u : 60u);
        byte[] bytes = Need(hooks.Memory)(manager);
        int terminal = front ? 0x243 : 0x4f;
        if (terminal >= bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(hooks), "채팅 형식 매니저의 실제 NUL 범위 오류");
        }

        bytes[terminal] = 0;
        string key = unchecked((int)globals.Subscriber) + (front ? "chatFormat2" : "chatFormat");
        Need(hooks.WriteConfig)(key, Text(capture));
        return 0u;
    }
}
