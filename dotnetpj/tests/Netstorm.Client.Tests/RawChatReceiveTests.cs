// 일반 채팅 수신·필터·분배를 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/client/RawChatReceive.h`·`RawChatReceive.cpp`(0042ed30·0042f0e0). fixture 없이 직접 둔다.
// 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
using System.Text;
using Netstorm.Client.Chat;

namespace Netstorm.Client.Tests;

// 채팅 수신 본체의 버전·문자·무시·채널·관계·분배를 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
public sealed class RawChatReceiveTests
{
    /// <summary>작은 endian 값을 가짜 물리에 쓴다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    private static void WriteDword(byte[] bytes, int at, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 두는 반복문이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            bytes[at + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>가짜 효과 묶음과 기록 저장소를 만든다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    private sealed class FakeWorld
    {
        /// <summary>원문 전역 주소별 값이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public Dictionary<uint, uint> Globals { get; } = new();

        /// <summary>무시할 구독자 집합이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public HashSet<uint> IgnoredSet { get; } = new();

        /// <summary>옵션 이름별 값이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public Dictionary<string, int> Options { get; } = new();

        /// <summary>구독자별 소유자 값이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public Dictionary<uint, uint> Owners { get; } = new();

        /// <summary>읽은 옵션 순서 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<string> ReadOptions { get; } = new();

        /// <summary>깨운 값 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<uint> Woken { get; } = new();

        /// <summary>재생 요청한 소리 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<string> Sounds { get; } = new();

        /// <summary>덧붙인 채팅 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<(uint Chat, string Text, uint[] Context)> Appended { get; } = new();

        /// <summary>표시 호출 횟수다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public int Shown;

        /// <summary>기록한 진단 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<(uint Code, string Text)> Logs { get; } = new();

        /// <summary>저장한 뉴스 시각 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<uint> NewsTimes { get; } = new();

        /// <summary>표시한 시스템 뉴스 기록이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public List<(string Text, uint First, uint Second)> News { get; } = new();

        /// <summary>가짜 효과 묶음을 만든다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
        public ChatReceiveHooks Hooks()
        {
            return new ChatReceiveHooks
            {
                Read = address => Globals.TryGetValue(address, out uint value) ? value : 0u,
                Ignored = subscriber => IgnoredSet.Contains(subscriber),
                Config = name => { ReadOptions.Add(name); return Options.TryGetValue(name, out int value) ? value : 0; },
                Owner = subscriber => Owners.TryGetValue(subscriber, out uint owner) ? owner : 0u,
                Wake = value => Woken.Add(value),
                Sound = name => Sounds.Add(name),
                Append = (chat, text, context) => Appended.Add((chat, text, context)),
                Show = () => Shown += 1,
                Log = (code, text) => Logs.Add((code, text)),
                SetNewsTime = stamp => NewsTimes.Add(stamp),
                News = (text, first, second) => News.Add((text, first, second)),
            };
        }
    }

    /// <summary>이름·본문·수신자 없는 기본 부모 패킷을 만든다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    private static byte[] Packet(byte version, byte flags, uint sender, int nameLength, string name, string body)
    {
        byte[] nameRaw = Encoding.Latin1.GetBytes(name);
        byte[] bodyRaw = Encoding.Latin1.GetBytes(body);
        int total = 15 + nameRaw.Length + 1 + bodyRaw.Length + 1;
        var packet = new byte[Math.Max(total + 8, 32)];
        packet[0] = version;
        packet[1] = RawChatPacket.PacketKind;
        packet[5] = flags;
        WriteDword(packet, 6, sender);
        WriteDword(packet, 10, (uint)nameLength, 2);
        WriteDword(packet, 12, (uint)(bodyRaw.Length + 1), 2);
        packet[14] = 0;
        // 이름을 실제 위치에 옮기는 반복문이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
        for (int index = 0; index < nameRaw.Length; ++index)
        {
            packet[15 + index] = nameRaw[index];
        }

        packet[15 + nameRaw.Length] = 0;
        // 본문을 실제 위치에 옮기는 반복문이다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.
        for (int index = 0; index < bodyRaw.Length; ++index)
        {
            packet[15 + nameRaw.Length + 1 + index] = bodyRaw[index];
        }

        packet[15 + nameRaw.Length + 1 + bodyRaw.Length] = 0;
        return packet;
    }

    /// <summary>새 형식 버전이면 진단만 적고 끝내는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketRejectsNewFormat()
    {
        // 버전 4 이상은 새 형식 진단 뒤 덧붙이지 않는다.
        var world = new FakeWorld();
        var hooks = world.Hooks();
        byte[] packet = Packet(4, 0x01, 5u, 4, "Bob", "Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal([(29u, "Received new format chat packet.\n")], world.Logs);
        Assert.Empty(world.Appended);
    }

    /// <summary>비ASCII면 진단만 적고 끝내는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketRejectsNonAscii()
    {
        // 아래쪽 4비트가 1이 아니면 비ASCII 진단 뒤 덧붙이지 않는다.
        var world = new FakeWorld();
        var hooks = world.Hooks();
        byte[] packet = Packet(0, 0x02, 5u, 4, "Bob", "Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal([(29u, "Received non-ASCII chat packet.\n")], world.Logs);
        Assert.Empty(world.Appended);
    }

    /// <summary>뉴스 알림이면 시각을 저장하고 시스템 뉴스로 넘기는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketNewsPath()
    {
        // 0x80·수신자 0·이름 길이 2 초과·15번 1이면 뉴스 경로다.
        var world = new FakeWorld();
        world.Globals[0x548748u] = 0u;
        var hooks = world.Hooks();
        var packet = new byte[32];
        packet[0] = 0;
        packet[1] = RawChatPacket.PacketKind;
        packet[5] = 0x81;
        WriteDword(packet, 6, 0u);
        WriteDword(packet, 10, 6u, 2);
        WriteDword(packet, 12, 3u, 2);
        packet[14] = 0;
        packet[15] = 1;
        packet[19] = 5;
        packet[21] = (byte)'H';
        packet[22] = (byte)'i';
        packet[23] = 0;
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal([5u | ((uint)'H' << 16) | ((uint)'i' << 24)], world.NewsTimes);
        Assert.Single(world.News);
        Assert.Equal(("Hi", 400u, 30u), world.News[0]);
        Assert.Empty(world.Appended);
    }

    /// <summary>무시 목록이면 덧붙이지 않는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketSkipsIgnored()
    {
        // 무시 목록에 있으면 조용히 돌아간다.
        var world = new FakeWorld();
        world.IgnoredSet.Add(5u);
        var hooks = world.Hooks();
        byte[] packet = Packet(0, 0x01, 5u, 4, "Bob", "Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Empty(world.Appended);
        Assert.Empty(world.Logs);
    }

    /// <summary>비전투이면 깨우고 덧붙이고 표시하는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketNonBattleAppends()
    {
        // 전투가 꺼지면 관계 없이 바로 덧붙이고 표시한다.
        var world = new FakeWorld();
        world.Globals[0x594fbcu] = 0u;
        world.Globals[0x5ca92cu] = 11u;
        world.Globals[0x540c74u] = 5u;
        world.Globals[0x594fc8u] = 0u;
        world.Globals[0x5b5dd8u] = 9u;
        var hooks = world.Hooks();
        byte[] packet = Packet(0, 0x01, 5u, 4, "Bob", "Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal([11u], world.Woken);
        Assert.Empty(world.Sounds);
        Assert.Single(world.Appended);
        Assert.Equal(7u, world.Appended[0].Chat);
        Assert.Equal("Hi", world.Appended[0].Text);
        Assert.Equal([0u, 5u, 9u, 0u, 0u, 0u, 1u, 1u], world.Appended[0].Context);
        Assert.Equal(1, world.Shown);
    }

    /// <summary>비전투 타인이면 소리를 요청하는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketNonBattleSoundsForOther()
    {
        // 자기가 아니면 새 메시지 소리를 요청한다.
        var world = new FakeWorld();
        world.Globals[0x594fbcu] = 0u;
        world.Globals[0x5ca92cu] = 11u;
        world.Globals[0x540c74u] = 5u;
        world.Globals[0x594fc8u] = 0u;
        world.Globals[0x5b5dd8u] = 9u;
        var hooks = world.Hooks();
        byte[] packet = Packet(0, 0x01, 7u, 4, "Bob", "Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal(["newChatMessage.wav"], world.Sounds);
        Assert.Equal(1, world.Shown);
    }

    /// <summary>전투이면 세 자리 접두를 떼고 덧붙이는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketBattleStripsPrefix()
    {
        // 채널 마스크와 옵션이 서면 표시하고 앞 3자를 떼어 붙인다.
        var world = new FakeWorld();
        world.Globals[0x594fbcu] = 1u;
        world.Globals[0x548710u] = 8u;
        world.Globals[0x5ca92cu] = 11u;
        world.Globals[0x540c74u] = 5u;
        world.Globals[0x594fc8u] = 0u;
        world.Globals[0x5b5dd8u] = 9u;
        world.Globals[0x540c70u] = 0u;
        world.Owners[7u] = 3u;
        world.Globals[0x595200u + (3u + 0u) * 4u] = 1u;
        world.Globals[0x52f81eu + 3u] = 0u;
        foreach (string key in RawChatReceive.OptionKeys)
        {
            world.Options[key] = 1;
        }

        var hooks = world.Hooks();
        byte[] packet = Packet(0, 0x01, 7u, 4, "Bob", "000Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal(10, world.ReadOptions.Count);
        Assert.Single(world.Appended);
        Assert.Equal("Hi\n", world.Appended[0].Text);
        Assert.Equal(1, world.Shown);
        Assert.Equal(["newChatMessage.wav"], world.Sounds);
    }

    /// <summary>가리면 열 개 옵션을 읽고도 덧붙이지 않는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ReceiveChatPacketBattleHiddenSkipsAppend()
    {
        // 마스크가 비면 열 개 옵션을 읽고도 덧붙이지 않는다.
        var world = new FakeWorld();
        world.Globals[0x594fbcu] = 1u;
        world.Globals[0x548710u] = 0u;
        var hooks = world.Hooks();
        byte[] packet = Packet(0, 0x01, 7u, 4, "Bob", "000Hi");
        ChatReceiveBody.ReceiveChatPacket(7u, packet, hooks);
        Assert.Equal(10, world.ReadOptions.Count);
        Assert.Empty(world.Appended);
        Assert.Equal(0, world.Shown);
    }

    /// <summary>분배가 현재 Chat 0이면 조용히 끝나는지 검증한다. 2026-11-11 F3 채팅 수신 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void DispatchChatPacketGuardsCurrentChat()
    {
        // 현재 Chat 0이면 패킷을 건드리지 않는다.
        var world = new FakeWorld();
        world.Globals[0x548740u] = 0u;
        var hooks = world.Hooks();
        byte[] packet = Packet(4, 0x01, 5u, 4, "Bob", "Hi");
        ChatReceiveBody.DispatchChatPacket(packet, hooks);
        Assert.Empty(world.Logs);
        world.Globals[0x548740u] = 7u;
        world.Globals[0x594fbcu] = 0u;
        world.Globals[0x5ca92cu] = 11u;
        world.Globals[0x540c74u] = 5u;
        world.Globals[0x594fc8u] = 0u;
        world.Globals[0x5b5dd8u] = 9u;
        byte[] live = Packet(0, 0x01, 5u, 4, "Bob", "Hi");
        ChatReceiveBody.DispatchChatPacket(live, hooks);
        Assert.Single(world.Appended);
    }
}
