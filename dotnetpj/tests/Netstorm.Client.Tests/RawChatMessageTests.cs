// 채팅 일반 본문 조합·입력 wrapper·say 관문을 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/client/RawChatMessage.h`·`RawChatMessage.cpp`(0042f1e0·0042dc30·0042d640). fixture 없이 직접 둔다.
// 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
using System.Text;
using Netstorm.Client.Chat;

namespace Netstorm.Client.Tests;

// 채팅 본문 조합 본체의 서식·wrapper·say를 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
public sealed class RawChatMessageTests
{
    /// <summary>매니저 물리를 빌리는 가짜 효과를 만든다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    private static ChatMessageHooks Hooks(Dictionary<uint, byte[]> stores, Action<uint, uint, byte[]>? delivered = null, Func<string>? name = null, Func<byte[], uint, uint>? named = null, Action<uint>? visible = null)
    {
        // 같은 토큰의 같은 배열을 돌려주는 대리자다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
        return new ChatMessageHooks
        {
            Memory = token => stores[token],
            CurrentName = name ?? (() => "Bob"),
            NameForSubscriber = named ?? ((work, other) => 1u),
            Deliver = (manager, recipients, payload) => { delivered?.Invoke(manager, recipients, payload); return 7u; },
            InputVisible = visible ?? (_ => { }),
        };
    }

    /// <summary>작은 endian DWORD를 가짜 물리에 쓴다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    private static void WriteDword(byte[] bytes, int at, uint value)
    {
        // 네 바이트를 little endian 순서로 두는 반복문이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[at + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>NUL 끝 문자열을 가짜 물리 구간에 둔다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    private static void WriteText(byte[] bytes, int at, string text)
    {
        // Latin1 바이트를 순방향으로 옮기는 반복문이다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.
        for (int index = 0; index < text.Length; ++index)
        {
            bytes[at + index] = (byte)text[index];
        }

        bytes[at + text.Length] = 0;
    }

    /// <summary>NUL 포함 바이트를 Latin1 문자열로 읽는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    private static string ReadPayload(byte[] payload) => Encoding.Latin1.GetString(payload, 0, Array.IndexOf(payload, (byte)0));

    /// <summary>2712바이트 매니저와 16바이트 수신자 가짜 물리를 만든다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    private static Dictionary<uint, byte[]> Stores(uint manager, uint recipients)
    {
        if (manager == recipients)
        {
            var shared = new Dictionary<uint, byte[]>
            {
                [manager] = new byte[0x298 + 2048],
            };
            WriteDword(shared[recipients], 0, 0u);
            return shared;
        }

        var stores = new Dictionary<uint, byte[]>
        {
            [manager] = new byte[0x298 + 2048],
            [recipients] = new byte[16],
        };
        WriteDword(stores[recipients], 0, 0u);
        return stores;
    }

    /// <summary>세 고정 %s 서식을 그대로 쓰는지 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void WriteChatNameFormatSupportsThreePatterns()
    {
        // 세 실제 호출 서식만 NUL 포함으로 적는다.
        var output = new byte[64];
        ChatMessageBody.WriteChatNameFormat(output, "%s> ", "Bob", "Al");
        Assert.Equal("Bob> ", ReadPayload(output));
        ChatMessageBody.WriteChatNameFormat(output, "%s ", "Bob", "Al");
        Assert.Equal("Bob ", ReadPayload(output));
        ChatMessageBody.WriteChatNameFormat(output, "%s (to %s)> ", "Bob", "Al");
        Assert.Equal("Bob (to Al)> ", ReadPayload(output));
        Assert.Throws<InvalidOperationException>(() => ChatMessageBody.WriteChatNameFormat(output, "%d", "Bob", "Al"));
    }

    /// <summary>빈 본문이면 아무것도 쓰지 않고 0을 돌려주는지 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatChatMessageRejectsEmptyBody()
    {
        // 빈 본문 관문에서는 전달 효과를 부르지 않는다.
        var stores = Stores(1u, 2u);
        bool called = false;
        var hooks = Hooks(stores, (_, _, _) => called = true);
        byte[] nameWork = new byte[RawChatMessage.NameBytes];
        var globals = new ChatMessageGlobals { Current = 1u, Subscriber = 9u };
        Assert.Equal(0u, ChatMessageBody.FormatChatMessage(1u, 2u, new byte[] { 0 }, null, nameWork, globals, hooks));
        Assert.False(called);
    }

    /// <summary>채널·이름·앞쪽·본문이 같은 순서로 붙는지 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatChatMessageBuildsChannelNameFrontBody()
    {
        // 채널(0x14)→이름→앞쪽(0x50)→본문 순서가 원문과 같다.
        var stores = Stores(1u, 2u);
        WriteText(stores[1u], 0x14, "CH");
        WriteText(stores[1u], 0x50, "FR");
        string? payload = null;
        var hooks = Hooks(stores, (_, _, bytes) => payload = ReadPayload(bytes));
        byte[] nameWork = new byte[RawChatMessage.NameBytes];
        var globals = new ChatMessageGlobals { Current = 1u, Subscriber = 9u };
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(7u, ChatMessageBody.FormatChatMessage(1u, 2u, body, null, nameWork, globals, hooks));
        Assert.Equal("CHBob> FRHi", payload);
    }

    /// <summary>직접 대상이면 to 서식으로 다른 이름을 넣는지 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatChatInputUsesToPatternForDirectTarget()
    {
        // 매니저 직접 표시가 있으면 수신자 버퍼의 첫 비자기를 to 서식에 쓴다.
        var stores = Stores(1u, 1u);
        stores[3u] = new byte[8];
        WriteDword(stores[1u], 0, 2u);
        WriteDword(stores[1u], 8, 3u);
        WriteDword(stores[1u], 12, 1u);
        WriteDword(stores[3u], 0, 9u);
        WriteDword(stores[3u], 4, 4u);
        WriteText(stores[1u], 0x14, "CH");
        string? payload = null;
        var hooks = Hooks(
            stores,
            (_, _, bytes) => payload = ReadPayload(bytes),
            named: (work, other) => { WriteText(work, 0, "Al"); return 1u; });
        byte[] nameWork = new byte[RawChatMessage.NameBytes];
        var globals = new ChatMessageGlobals { Current = 1u, Subscriber = 9u };
        byte[] input = [(byte)'H', (byte)'i', 0];
        ChatMessageBody.FormatChatInput(1u, input, nameWork, globals, hooks);
        Assert.Equal("~[Iicon.D40]CHBob (to Al)> Hi", payload);
    }

    /// <summary>앞 !이면 이름+space 서식으로 같은 매니저에 넘기는지 검증한다. 직접 표시가 없으면 앞쪽(0x50)을 잇는다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void FormatChatInputBangUsesNameSpacePattern()
    {
        // 선행 !은 이름과 space만 쓰고 직접 표시가 없으므로 앞쪽(0x50)을 잇는다.
        var stores = Stores(1u, 1u);
        WriteText(stores[1u], 0x14, "CH");
        WriteText(stores[1u], 0x50, "FR");
        string? payload = null;
        var hooks = Hooks(stores, (_, _, bytes) => payload = ReadPayload(bytes));
        byte[] nameWork = new byte[RawChatMessage.NameBytes];
        var globals = new ChatMessageGlobals { Current = 1u, Subscriber = 9u };
        byte[] input = [(byte)'!', (byte)'H', (byte)'i', 0];
        ChatMessageBody.FormatChatInput(1u, input, nameWork, globals, hooks);
        Assert.Equal("CHBob FRHi", payload);
    }

    /// <summary>say의 매니저 0·앞 space·빈 본문 관문을 지키는지 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void SayChatMessageGuardsManagerSpaceAndEmpty()
    {
        // 매니저 0이면 효과를 보지 않고 돌아간다.
        var idle = Hooks(new Dictionary<uint, byte[]>(), (_, _, _) => throw new InvalidOperationException("호출되면 안 됨"));
        Assert.Equal(0u, ChatMessageBody.SayChatMessage(new byte[] { (byte)'H', 0 }, new byte[RawChatMessage.NameBytes], new ChatMessageGlobals(), idle));

        // 앞 space·NUL·뒤 전달이 원문 관문과 같다.
        var stores = Stores(1u, 1u);
        WriteText(stores[1u], 0x14, "CH");
        uint visible = 99u;
        string? payload = null;
        var hooks = Hooks(stores, (_, _, bytes) => payload = ReadPayload(bytes), visible: value => visible = value);
        byte[] nameWork = new byte[RawChatMessage.NameBytes];
        var globals = new ChatMessageGlobals { Current = 1u, Subscriber = 9u };
        Assert.Equal(0u, ChatMessageBody.SayChatMessage(new byte[] { (byte)' ', (byte)'H', 0 }, nameWork, globals, hooks));
        Assert.Equal(0u, ChatMessageBody.SayChatMessage(new byte[] { 0 }, nameWork, globals, hooks));
        Assert.Equal(0u, visible);
        Assert.Equal(0u, ChatMessageBody.SayChatMessage(new byte[] { (byte)'H', (byte)'i', 0 }, nameWork, globals, hooks));
        Assert.Equal("CHBob> Hi", payload);
    }

    /// <summary>입력이 비면 wrapper·say가 멈추는지 검증한다. 2026-11-11 F3 채팅 본문 조합 이식에서 추가됐다.</summary>
    [Fact]
    public void ChatInputRejectsEmptySpan()
    {
        // 실제 첫 입력 바이트가 없으면 명시 범위로 멈춘다.
        var stores = Stores(1u, 1u);
        var hooks = Hooks(stores);
        byte[] nameWork = new byte[RawChatMessage.NameBytes];
        var globals = new ChatMessageGlobals { Current = 1u, Subscriber = 9u };
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatMessageBody.FormatChatInput(1u, ReadOnlySpan<byte>.Empty, nameWork, globals, hooks));
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatMessageBody.SayChatMessage(ReadOnlySpan<byte>.Empty, nameWork, globals, hooks));
    }
}
