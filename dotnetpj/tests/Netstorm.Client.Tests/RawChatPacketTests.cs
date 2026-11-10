// 채팅 패킷 작성·채널 선택과 송신 길이 확정을 원본 계약 그대로 대조하는 검사 클래스다.
// 근거: cpppj `src/client/RawChatPacket.h`·`RawChatPacket.cpp`(0042d280·0040a8d0). fixture 없이 직접 둔다.
// 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
using System.Text;
using Netstorm.Client.Chat;

namespace Netstorm.Client.Tests;

// 채팅 패킷 본체의 작성·채널·감시를 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
public sealed class RawChatPacketTests
{
    /// <summary>작은 endian DWORD를 가짜 물리에 쓴다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    private static void WriteDword(byte[] bytes, int at, uint value)
    {
        // 네 바이트를 little endian 순서로 두는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            bytes[at + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>NUL 끝 문자열을 가짜 물리 구간에 둔다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    private static void WriteText(byte[] bytes, int at, string text)
    {
        // Latin1 바이트를 순방향으로 옮기는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
        for (int index = 0; index < text.Length; ++index)
        {
            bytes[at + index] = (byte)text[index];
        }

        bytes[at + text.Length] = 0;
    }

    /// <summary>매니저·수신자·버퍼 가짜 물리와 전역 묶음을 만든다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    private sealed class FakeWorld
    {
        /// <summary>토큰별 실제 물리다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public Dictionary<uint, byte[]> Stores { get; } = new();

        /// <summary>원문 전역 주소별 값이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public Dictionary<uint, uint> Globals { get; } = new();

        /// <summary>추가된 수신자 기록이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public List<(uint Recipients, uint Subscriber)> Added { get; } = new();

        /// <summary>송신된 채널·패킷 기록이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public List<(int Channel, byte[] Packet)> Sent { get; } = new();

        /// <summary>기록된 오류 기록이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public List<(uint Code, string Text)> Logs { get; } = new();

        /// <summary>0.0.0.0 준비와 로컬 수신 호출 횟수다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public int ZeroCount;

        /// <summary>로컬 수신 패킷 기록이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public List<byte[]> Local { get; } = new();

        /// <summary>매니저 1·수신자 2·버퍼 3 토큰으로 기본 물리를 둔다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public static FakeWorld Create(uint subscriber, uint[] present)
        {
            var world = new FakeWorld();
            world.Stores[1u] = new byte[64];
            world.Stores[2u] = new byte[16];
            var buffer = new byte[present.Length * 4];
            // 기존 수신자를 버퍼에 미리 두는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
            for (int index = 0; index < present.Length; ++index)
            {
                WriteDword(buffer, index * 4, present[index]);
            }

            world.Stores[3u] = buffer;
            WriteDword(world.Stores[2u], 0, (uint)present.Length);
            WriteDword(world.Stores[2u], 8, 3u);
            world.Globals[RawChatPacket.SubscriberAddress] = subscriber;
            world.Globals[0x52edf6] = 0x12u;
            world.Globals[0x544c5c] = 4u;
            return world;
        }

        /// <summary>가짜 효과 묶음을 만든다. 추가는 실제 버퍼에 그대로 잇는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
        public ChatPacketHooks Hooks(Func<byte[]>? name = null)
        {
            return new ChatPacketHooks
            {
                Memory = token => Stores[token],
                CurrentName = name ?? (() => [(byte)'B', (byte)'o', (byte)'b', 0]),
                Read = address => Globals.TryGetValue(address, out uint value) ? value : 0u,
                AddRecipient = (recipients, subscriber) =>
                {
                    Added.Add((recipients, subscriber));
                    byte[] old = Stores[3u];
                    var grown = new byte[old.Length + 4];
                    Array.Copy(old, grown, old.Length);
                    WriteDword(grown, old.Length, subscriber);
                    Stores[3u] = grown;
                    uint count = 0u;
                    // 버퍼 DWORD 개수를 세는 반복문이다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.
                    for (int at = 0; at + 4 <= grown.Length; at += 4)
                    {
                        count += 1u;
                    }

                    WriteDword(Stores[recipients], 0, count);
                },
                Send = (channel, packet) => Sent.Add((channel, packet)),
                Log = (code, text) => Logs.Add((code, text)),
                ZeroAddress = () => ZeroCount += 1,
                LocalReceive = packet => Local.Add(packet),
            };
        }
    }

    /// <summary>패킷의 이름 구간을 읽는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    private static string PacketName(byte[] packet)
    {
        int end = Array.IndexOf(packet, (byte)0, 15);
        return Encoding.Latin1.GetString(packet, 15, end - 15);
    }

    /// <summary>패킷의 본문 구간을 읽는다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    private static byte[] PacketMessage(byte[] packet, int nameSize)
    {
        int length = packet[12] | (packet[13] << 8);
        var message = new byte[length];
        Array.Copy(packet, 15 + nameSize, message, 0, length);
        return message;
    }

    /// <summary>누락된 자기 구독자를 추가하고 1번 채널로 보내는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketAddsMissingSelf()
    {
        // 수신자 0명에서 자기 5를 덧붙이고 채널 1로 보낸다.
        var world = FakeWorld.Create(5u, []);
        world.Globals[RawChatPacket.ChannelOneAddress] = 1u;
        var hooks = world.Hooks();
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(1u, ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
        Assert.Equal([(2u, 5u)], world.Added);
        Assert.Single(world.Sent);
        Assert.Equal(1, world.Sent[0].Channel);
        byte[] packet = world.Sent[0].Packet;
        Assert.Equal(3, packet[0]);
        Assert.Equal("Bob", PacketName(packet));
        Assert.Equal(body, PacketMessage(packet, 4));
    }

    /// <summary>이미 있는 자기 구독자는 덧붙이지 않는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketSkipsAddWhenPresent()
    {
        // 버퍼에 자기가 있으면 추가 효과를 부르지 않는다.
        var world = FakeWorld.Create(5u, [5u]);
        world.Globals[RawChatPacket.ChannelOneAddress] = 1u;
        var hooks = world.Hooks();
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(1u, ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
        Assert.Empty(world.Added);
        Assert.Single(world.Sent);
    }

    /// <summary>전투이면 깃발 십진 앞0 3자리와 본문을 붙이는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketBattlePrefix()
    {
        // 깃발 0은 000 세 자리로 바뀌어 본문 앞에 붙는다.
        var world = FakeWorld.Create(5u, [5u]);
        world.Globals[RawChatPacket.BattleAddress] = 1u;
        var hooks = world.Hooks();
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(1u, ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
        Assert.Single(world.Sent);
        Assert.Equal(9, world.Sent[0].Channel);
        Assert.Equal("000Hi"u8.ToArray(), PacketMessage(world.Sent[0].Packet, 4));
    }

    /// <summary>직접 표시가 있으면 0x100을 올린 깃발로 보내는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketDirectFlagAdds0x100()
    {
        // 매니저 직접 표시가 있으면 깃발에 0x100을 올린다.
        var world = FakeWorld.Create(5u, [5u]);
        world.Globals[RawChatPacket.BattleAddress] = 1u;
        WriteDword(world.Stores[1u], 12, 1u);
        var hooks = world.Hooks();
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(1u, ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
        Assert.Equal("256Hi"u8.ToArray(), PacketMessage(world.Sent[0].Packet, 4));
    }

    /// <summary>메타맵 채널이 서면 11번으로 보내는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketMetamapChannel()
    {
        // 1번이 꺼지고 메타맵이 서면 11번으로 보낸다.
        var world = FakeWorld.Create(5u, [5u]);
        world.Globals[RawChatPacket.MetamapAddress] = 1u;
        var hooks = world.Hooks();
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(1u, ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
        Assert.Equal(11, world.Sent[0].Channel);
    }

    /// <summary>채널이 하나도 없으면 오류 기록 뒤 로컬 수신으로 돌리는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketNoChannelLocal()
    {
        // 세 채널이 모두 꺼지면 오류를 적고 0.0.0.0 로컬 수신으로 돌린다.
        var world = FakeWorld.Create(5u, [5u]);
        var hooks = world.Hooks();
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Equal(1u, ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
        Assert.Empty(world.Sent);
        Assert.Equal([(9u, "ERROR: No transceiver on which to chat!\n")], world.Logs);
        Assert.Equal(1, world.ZeroCount);
        Assert.Single(world.Local);
    }

    /// <summary>이름·개수 한계를 넘으면 멈추는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void SendChatPacketRejectsOversize()
    {
        // NUL 없는 현재 이름은 명시 범위로 멈춘다.
        var world = FakeWorld.Create(5u, [5u]);
        world.Globals[RawChatPacket.ChannelOneAddress] = 1u;
        var hooks = world.Hooks(() => [(byte)'B']);
        byte[] body = [(byte)'H', (byte)'i', 0];
        Assert.Throws<ArgumentOutOfRangeException>(() => ChatPacketBody.SendChatPacket(1u, 2u, body, hooks));
    }

    /// <summary>송신 길이를 현재 워드·워드·바이트 합산으로 확정하는지 검증한다. 2026-11-11 F3 채팅 패킷 본체 이식에서 추가됐다.</summary>
    [Fact]
    public void ObserveChatPacketSumsLength()
    {
        // 꺼져 있으면 두지 않고, 서면 길이 바이트를 다시 쓴다.
        var packet = new byte[32];
        packet[10] = 4;
        packet[12] = 5;
        packet[14] = 2;
        ChatPacketBody.ObserveChatPacket(packet, 0u, _ => throw new InvalidOperationException("호출되면 안 됨"));
        Assert.Equal(0, packet[2] | (packet[3] << 8));
        var calls = new List<uint>();
        ChatPacketBody.ObserveChatPacket(packet, 1u, calls.Add);
        Assert.Empty(calls);
        Assert.Equal(2 * 4 + 5 + 4 + 15, packet[2] | (packet[3] << 8));
        ChatPacketBody.ObserveChatPacket(packet, 1u, calls.Add);
        packet[10] = 0xff;
        packet[11] = 0xff;
        ChatPacketBody.ObserveChatPacket(packet, 1u, calls.Add);
        Assert.Equal([0x81du], calls);
        packet[14] = 0xff;
        ChatPacketBody.ObserveChatPacket(packet, 1u, calls.Add);
        Assert.Equal([0x81du, 0x81du, 0x81fu], calls);
    }
}
