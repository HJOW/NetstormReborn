// 추적 로그의 채팅 전송 본체를 공유한다.
// 근거: cpppj `src/client/TraceChat.h`·`TraceChat.cpp`(434f40·42f190).
// 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.
using System.Text;

namespace Netstorm.Client.Chat;

// 추적 채팅 범주를 담는 네임스페이스다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.

// 추적 전송이 부르는 외부 효과 묶음이다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.
public sealed class TraceChatHooks
{
    /// <summary>005acd24 색 DWORD 조회다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    public Func<uint>? Color { get; set; }

    /// <summary>00548740 현재 채팅 번호 조회다. 0이면 덧붙이지 않는다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    public Func<uint>? CurrentChat { get; set; }

    /// <summary>42eca0 덧붙이기 효과다. root·완성 문자열·8 DWORD 맥락을 받는다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    public Action<uint, string, uint[]>? Append { get; set; }
}

// 추적 문자열 포맷·전송 본체다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.
public static class TraceChatBody
{
    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("추적 채팅의 선택된 실제 효과 미연결");
        }

        return callback;
    }

    /// <summary>일반 문자와 %%만 남기고 첫 NUL까지 자른다. 단독 %가 있으면 멈추고 1024바이트를 넘으면 멈춘다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    /// <param name="format">읽을 형식 문자열이다.</param>
    public static string FormatTraceChat(string format)
    {
        ArgumentNullException.ThrowIfNull(format);
        int nul = format.IndexOf('\0');
        if (nul >= 0)
        {
            format = format.Substring(0, nul);
        }

        var text = new StringBuilder();
        // 앞쪽부터 한 문자씩 읽는 반복문이다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.
        for (int index = 0; index < format.Length; ++index)
        {
            char character = format[index];
            if (character == '%')
            {
                ++index;
                if (index >= format.Length || format[index] != '%')
                {
                    throw new InvalidOperationException("추적 채팅 형식의 실제 % 이스케이프 오류");
                }

                character = '%';
            }

            if (text.Length == 1023)
            {
                throw new InvalidOperationException("추적 채팅의 실제 1024바이트 길이 초과");
            }

            text.Append(character);
        }

        return text.ToString();
    }

    /// <summary>켜져 있으면 색·현재 채팅을 읽어 덧붙인다. 꺼져 있으면 아무것도 하지 않는다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    /// <param name="enabled">0이면 끈 것이다.</param>
    /// <param name="format">읽을 형식 문자열이다.</param>
    /// <param name="hooks">부를 외부 효과다. 켜졌을 때만 필수로 요구한다.</param>
    public static void ForwardTraceToChat(uint enabled, string format, TraceChatHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(format);
        ArgumentNullException.ThrowIfNull(hooks);
        if (enabled == 0u)
        {
            return;
        }

        uint[] context = [0u, 1u, Need(hooks.Color)(), 0u, 0u, 0u, 1u, 1u];
        string text = FormatTraceChat(format);
        uint chat = Need(hooks.CurrentChat)();
        if (chat != 0u)
        {
            Need(hooks.Append)(chat, text, context);
        }
    }
}
