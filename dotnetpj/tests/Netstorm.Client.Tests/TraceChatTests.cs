// 추적 문자열 포맷·전송 본체를 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/client/TraceChat.h`·`TraceChat.cpp`(434f40·42f190). fixture 없이 직접 둔다.
// 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.
using Netstorm.Client.Chat;

namespace Netstorm.Client.Tests;

/// <summary>추적 채팅 본체의 포맷·전송을 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
public sealed class TraceChatTests
{
    /// <summary>일반 문자와 %%를 그대로 두는지 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatTraceChatKeepsPlainAndEscapedPercent()
    {
        // "%%"는 "%" 하나로 줄고 나머지는 그대로 둔다.
        Assert.Equal("100% done", TraceChatBody.FormatTraceChat("100%% done"));
        Assert.Equal("", TraceChatBody.FormatTraceChat(""));
        Assert.Equal("ab", TraceChatBody.FormatTraceChat("ab\0cd"));
    }

    /// <summary>단독 %를 거부하는지 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatTraceChatRejectsLonePercent()
    {
        // 뒤에 %가 없으면 멈춘다. 끝의 %도 마찬가지다.
        Assert.Throws<InvalidOperationException>(() => TraceChatBody.FormatTraceChat("a%b"));
        Assert.Throws<InvalidOperationException>(() => TraceChatBody.FormatTraceChat("%"));
    }

    /// <summary>1024바이트 한도를 지키는지 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatTraceChatGuardsLength1024()
    {
        // 1023자는 그대로 두고 1024번째에서 멈춘다.
        Assert.Equal(1023, TraceChatBody.FormatTraceChat(new string('x', 1023)).Length);
        Assert.Throws<InvalidOperationException>(() => TraceChatBody.FormatTraceChat(new string('x', 1024)));
    }

    /// <summary>꺼져 있으면 아무것도 부르지 않는지 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ForwardTraceToChatSkipsWhenDisabled()
    {
        // enabled 0이면 효과 묶음을 보지 않고 돌아간다.
        var hooks = new TraceChatHooks
        {
            Color = () => throw new InvalidOperationException("호출되면 안 됨"),
            CurrentChat = () => throw new InvalidOperationException("호출되면 안 됨"),
            Append = (chat, text, context) => throw new InvalidOperationException("호출되면 안 됨"),
        };
        TraceChatBody.ForwardTraceToChat(0u, "anything", hooks);
    }

    /// <summary>켜져 있으면 색·현재 채팅·덧붙이기를 잇는지 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ForwardTraceToChatAppendsWithContext()
    {
        // 맥락 8 DWORD는 [0,1,색,0,0,0,1,1] 고정이다.
        var trace = new List<string>();
        var hooks = new TraceChatHooks
        {
            Color = () => 9u,
            CurrentChat = () => 4u,
            Append = (chat, text, context) => trace.Add($"{chat},{text},{string.Join(",", context)}"),
        };
        TraceChatBody.ForwardTraceToChat(1u, "Go", hooks);
        Assert.Equal(["4,Go,0,1,9,0,0,0,1,1"], trace);
    }

    /// <summary>현재 채팅이 0이면 덧붙이지 않는지 검증한다. 2026-11-11 F3 추적 채팅 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ForwardTraceToChatSkipsAppendWhenNoChat()
    {
        // 색까지는 읽고 덧붙이기는 건너뛴다.
        var trace = new List<string>();
        var hooks = new TraceChatHooks
        {
            Color = () => { trace.Add("C"); return 9u; },
            CurrentChat = () => 0u,
            Append = (chat, text, context) => trace.Add("A"),
        };
        TraceChatBody.ForwardTraceToChat(1u, "Go", hooks);
        Assert.Equal(["C"], trace);
    }
}
