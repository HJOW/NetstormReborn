using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;

// 수신 부모와 현재 창 관문의 원문238관측을 전체 물리/효과 순서로 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 원문238행의 현재 읽기/선택 필터/콜백 중단을 그대로 검증한다.</summary>
public sealed class ChatReceiveNativeTests
{
    /// <summary>원문 동일 Chat 주소 별칭이다.</summary>
    private const uint Chat = 0x12008000;
    /// <summary>선택한 외부 장치 중단만 구별한다.</summary>
    private sealed class Stop(string reason) : Exception(reason) { }
    /// <summary>원문 명시 필드의 지정 폭만 낮은 BYTE부터 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 인접 물리와 미작성 꼬리는 보존한다.
        for (int index = 0; index < width; index++) bytes[at + index] = (byte)(value >> (8 * index));
    }
    /// <summary>출력 BYTE 전체를 같은 소문자 hex로 비교한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexStringLower(bytes);
    /// <summary>표시 BYTE 문자열을 UTF8 재해석 없이 직렬화한다.</summary>
    private static string Hex(string text) => Hex(Encoding.Latin1.GetBytes(text));
    /// <summary>실제8DWORD 문맥을 원문 little endian32BYTE로 직렬화한다.</summary>
    private static string Context(uint[] context)
    {
        Assert.Equal(8, context.Length); byte[] bytes = new byte[32];
        // 각 DWORD 비트를 같은 원문 순서로 쓴다.
        for (int index = 0; index < context.Length; index++) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(index * 4, 4), context[index]);
        return Hex(bytes);
    }
    /// <summary>기존 원문 전체 패킷/현재 전역/호출 순서와 반환·중단 상태를 전수 대조한다.</summary>
    [Fact]
    public void IndependentNative238WholePacketAndEffects()
    {
        int rows = 0;
        // 일반 헤더1행만 제외하고 기존 독립 관측을 사본 없이 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-receive-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(18, row.Length);
            // fixture에 명시한 원문 DWORD를 현재 문화권과 무관하게 읽는다.
            uint Number(int at) => uint.Parse(row[at], CultureInfo.InvariantCulture);
            uint battle = Number(3), subscriber = 100, color = 44, mask = Number(4), sender = Number(5), flags = Number(6), allied = Number(7), neutral = Number(8); bool ignored = Number(9) != 0; string mode = row[10]; byte[] source = Convert.FromHexString(row[11]), packet = Enumerable.Repeat((byte)0xd3, 256).ToArray(); var trace = new List<string>();
            Put(packet, 0, Number(1), 1); Put(packet, 1, 0x41, 1); Put(packet, 2, (uint)(22 + source.Length + 4), 2); Put(packet, 5, Number(2), 1); Put(packet, 6, sender); Put(packet, 10, 7, 2); Put(packet, 12, (uint)source.Length, 2); Put(packet, 14, 1, 1); "Sender\0"u8.CopyTo(packet.AsSpan(15)); source.CopyTo(packet, 22); Put(packet, 22 + source.Length, 100);
            var hooks = new ChatReceiveHooks
            {
                Read = at => at switch { 0x540c74 => subscriber, 0x594fbc => battle, 0x594fc8 => 0, 0x548710 => mask, 0x548748 => 7, 0x540c70 => 2, 0x5ca92c => 0x1234, 0x5b5dd8 => color, 0x548740 => row[0] == "noRoot" ? 0 : Chat, 0x595200 + (2 * 9 + 3) * 4 => allied, 0x52f81e + 3 => neutral, _ => throw new InvalidOperationException("원문에 없는 전역:" + at.ToString("x", CultureInfo.InvariantCulture)) },
                Ignored = actual => { trace.Add("ignored:" + actual.ToString(CultureInfo.InvariantCulture)); return ignored; },
                Config = key => { trace.Add("config:" + key); if (mode == "stopConfig") throw new Stop("config"); if (mode == "configBattleOff" && key == RawChatReceive.OptionKeys[^1]) battle = 0; int index = Array.IndexOf(RawChatReceive.OptionKeys, key); Assert.InRange(index, 0, 9); return (flags & (1u << index)) != 0 ? 1 : 0; },
                Owner = actual => { trace.Add("owner:" + actual.ToString(CultureInfo.InvariantCulture)); return 3; },
                Wake = target => { trace.Add("wake:" + target.ToString(CultureInfo.InvariantCulture)); if (mode == "wakeSwitch") { subscriber = sender; color = 99; } if (mode == "stopWake") throw new Stop("wake"); },
                Sound = text => trace.Add("sound:" + text), Append = (target, text, context) => { Assert.Equal(Chat, target); trace.Add("append:" + Hex(text) + ':' + Context(context)); if (mode == "stopAppend") throw new Stop("append"); },
                Show = () => { trace.Add("show"); if (mode == "stopShow") throw new Stop("show"); }, Log = (channel, text) => trace.Add("log:" + channel.ToString(CultureInfo.InvariantCulture) + ':' + Hex(text)),
            };
            string status = "normal"; try { if (row[0] == "direct") ChatReceiveBody.ReceiveChatPacket(Chat, packet, hooks); else ChatReceiveBody.DispatchChatPacket(packet, hooks); } catch (Stop stop) { status = stop.Message; }
            Assert.Equal(row[12], status); Assert.Equal(row[13], Hex(packet)); Assert.Equal("7", row[14]); Assert.Equal(Number(15), subscriber); Assert.Equal(Number(16), battle); Assert.Equal(row[17], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(238, rows);
    }

    /// <summary>부분 버퍼 앞뒤를 보존하면서 원문처럼 콜백 뒤의 본문·발신자·색을 다시 읽는다.</summary>
    [Fact]
    public void PartialPacketKeepsAliasesAndCurrentReadsAfterWake()
    {
        byte[] storage = Enumerable.Repeat((byte)0xd3, 64).ToArray();
        Memory<byte> packet = storage.AsMemory(7, 48);
        Put(packet.Span, 0, 1, 1); Put(packet.Span, 5, 1, 1); Put(packet.Span, 6, 200);
        Put(packet.Span, 10, 2, 2); Put(packet.Span, 12, 4, 2); "old!"u8.CopyTo(packet.Span[17..]);
        var trace = new List<string>(); uint color = 44;
        ChatReceiveBody.DispatchChatPacket(packet.Span, new()
        {
            Read = at => at switch { 0x548740 => Chat, 0x594fbc => 0, 0x5ca92c => 12, 0x540c74 => 100, 0x594fc8 => 0, 0x5b5dd8 => color, _ => throw new InvalidOperationException("미선택 전역") },
            Ignored = sender => { Assert.Equal(200u, sender); Assert.Equal(0, packet.Span[20]); return false; },
            Wake = serial => { Assert.Equal(12u, serial); trace.Add("wake"); Put(packet.Span, 6, 100); Put(packet.Span, 10, 8, 2); "new\0"u8.CopyTo(packet.Span[23..]); color = 99; },
            Append = (chat, text, context) => { Assert.Equal(Chat, chat); Assert.Equal("new", text); Assert.Equal(new uint[] { 0, 5, 99, 0, 0, 0, 1, 1 }, context); trace.Add("append"); },
            Show = () => trace.Add("show"),
        });
        Assert.Equal(new[] { "wake", "append", "show" }, trace);
        Assert.All(storage.Take(7), value => Assert.Equal(0xd3, value));
        Assert.All(storage.Skip(55), value => Assert.Equal(0xd3, value));
    }

    /// <summary>창0의 빈 버퍼는 건드리지 않고 선택 장치 오류 전까지의 실제 NUL 기록은 보존한다.</summary>
    [Fact]
    public void EmptyRootSkipsStorageAndMissingEffectKeepsPriorWrite()
    {
        ChatReceiveBody.DispatchChatPacket(Span<byte>.Empty, new() { Read = at => { Assert.Equal(0x548740u, at); return 0; } });
        byte[] storage = Enumerable.Repeat((byte)0xd3, 32).ToArray(); Memory<byte> packet = storage.AsMemory(4, 24);
        Put(packet.Span, 0, 1, 1); Put(packet.Span, 5, 1, 1); Put(packet.Span, 10, 0, 2); Put(packet.Span, 12, 4, 2);
        Assert.Throws<InvalidOperationException>(() => ChatReceiveBody.DispatchChatPacket(packet.Span, new() { Read = at => { Assert.Equal(0x548740u, at); return Chat; } }));
        Assert.Equal(0, storage[22]); Assert.Equal(0xd3, storage[21]); Assert.Equal(0xd3, storage[23]);
        Assert.All(storage.Take(4), value => Assert.Equal(0xd3, value)); Assert.All(storage.Skip(28), value => Assert.Equal(0xd3, value));
    }
}
