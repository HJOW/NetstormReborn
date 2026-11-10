// 채팅 형식 캡처·매니저·Config 연결을 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/client/RawChatFormat.h`·`RawChatFormat.cpp`(0042d160·0042ddf0·0042def0). fixture 없이 직접 둔다.
// 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.
using Netstorm.Client.Chat;

namespace Netstorm.Client.Tests;

/// <summary>채팅 형식 본체의 칸 지우기·캡처·매니저·Config 연결을 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
public sealed class RawChatFormatTests
{
    /// <summary>~0~6 칸을 지우고 NUL까지 당기는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void StripChatFormatSlotsRemovesSlotMarkers()
    {
        // "~0Hello"는 "~"와 "0"을 버리고 뒤를 당긴다.
        byte[] text = [(byte)'~', (byte)'0', (byte)'H', (byte)'e', (byte)'l', (byte)'l', (byte)'o', 0];
        ChatFormatBody.StripChatFormatSlots(text);
        Assert.Equal("Hello\0o\0", System.Text.Encoding.Latin1.GetString(text));
        Assert.Equal("Hello", System.Text.Encoding.Latin1.GetString(text, 0, text.AsSpan().IndexOf((byte)0)));
    }

    /// <summary>칸 표시가 아닌 ~는 그대로 두는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void StripChatFormatSlotsKeepsOtherTildes()
    {
        // "~x"와 "~7"은 칸 표시가 아니라 그대로 둔다.
        byte[] text = [(byte)'~', (byte)'x', (byte)'~', (byte)'7', (byte)'A', 0];
        ChatFormatBody.StripChatFormatSlots(text);
        Assert.Equal([(byte)'~', (byte)'x', (byte)'~', (byte)'7', (byte)'A', (byte)0], text);
    }

    /// <summary>NUL 없는 버퍼를 거부하는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void StripChatFormatSlotsRejectsMissingNul()
    {
        // 끝까지 NUL이 없으면 버퍼 끝에서 멈춘다.
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatFormatBody.StripChatFormatSlots([(byte)'A', (byte)'B']));
        // 당긴 뒤 NUL이 버퍼를 넘으면 멈춘다.
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatFormatBody.StripChatFormatSlots([(byte)'~', (byte)'0', (byte)'A']));
    }

    /// <summary>수락이 캡처·매니저 복사·터미널 NUL·Config 쓰기를 잇는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyChatFormatWritesCaptureAndConfig()
    {
        // "~rHello" 앞 형식은 0x50·500·0x243·chatFormat2 경로를 탄다.
        var trace = new List<string>();
        byte[] manager = new byte[0x300];
        var hooks = new ChatFormatHooks
        {
            Assertion = code => trace.Add("A," + code),
            CopyFormat = (owner, offset, text, limit) => trace.Add($"C,{owner},{offset},{text},{limit}"),
            Memory = owner => { Assert.Equal(5u, owner); return manager; },
            WriteConfig = (key, text) => trace.Add("W," + key + "," + text),
            IllegalFormat = text => trace.Add("I," + text),
        };
        byte[] capture = new byte[81];
        var globals = new ChatFormatGlobals { Current = 5u, Subscriber = 7u };
        Assert.Equal(0u, ChatFormatBody.ApplyChatFormat(true, globals, "~rHello", capture, hooks));
        Assert.Equal("~rHello", System.Text.Encoding.Latin1.GetString(capture, 0, 7));
        Assert.Equal(0, capture[7]);
        Assert.Equal(0, manager[0x243]);
        Assert.Equal(["C,5,80,~rHello,500", "W,7chatFormat2,~rHello"], trace);
    }

    /// <summary>매니저 0이면 진단을 먼저 부르는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyChatFormatAssertsWhenManagerMissing()
    {
        // 뒤 형식 매니저 0은 157 진단을 부르고 나머지는 그대로 잇는다.
        var trace = new List<string>();
        byte[] manager = new byte[0x100];
        var hooks = new ChatFormatHooks
        {
            Assertion = code => trace.Add("A," + code),
            CopyFormat = (owner, offset, text, limit) => trace.Add($"C,{owner},{offset},{text},{limit}"),
            Memory = owner => manager,
            WriteConfig = (key, text) => trace.Add("W," + key + "," + text),
        };
        byte[] capture = new byte[81];
        var globals = new ChatFormatGlobals { Current = 0u, Subscriber = 2u };
        Assert.Equal(0u, ChatFormatBody.ApplyChatFormat(false, globals, "~gHi", capture, hooks));
        Assert.Equal(["A,157", "C,0,20,~gHi,60", "W,2chatFormat,~gHi"], trace);
        Assert.Equal(0, manager[0x4f]);
    }

    /// <summary>형식 없으면 불법 알림만 하고 0을 돌려주는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyChatFormatRejectsIllegalFormat()
    {
        // "~" 없는 입력은 매치 실패로 불법 알림 경로만 탄다.
        var trace = new List<string>();
        var hooks = new ChatFormatHooks
        {
            IllegalFormat = text => trace.Add("I," + text),
            CopyFormat = (owner, offset, text, limit) => trace.Add("C"),
            WriteConfig = (key, text) => trace.Add("W"),
        };
        byte[] capture = new byte[81];
        var globals = new ChatFormatGlobals { Current = 5u, Subscriber = 1u };
        Assert.Equal(0u, ChatFormatBody.ApplyChatFormat(true, globals, "plain", capture, hooks));
        Assert.Equal(["I,plain"], trace);
        Assert.All(capture, value => Assert.Equal(0, value));
    }

    /// <summary>81바이트 미만 캡처를 거부하는지 검증한다. 2026-11-11 F3 채팅 형식 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ApplyChatFormatRejectsShortCapture()
    {
        // 매치 뒤 캡처 크기 검사가 먼저 동작한다.
        var hooks = new ChatFormatHooks
        {
            IllegalFormat = text => { },
            CopyFormat = (owner, offset, text, limit) => { },
            Memory = owner => new byte[0x300],
            WriteConfig = (key, text) => { },
        };
        var globals = new ChatFormatGlobals { Current = 5u, Subscriber = 7u };
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatFormatBody.ApplyChatFormat(true, globals, "~rHello", new byte[80], hooks));
    }
}
