using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// 원문 일반 본문524/패킷256관측을 전체 물리와 효과 순서로 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>독립 원문 관측을 기존 본체에 적용하며 기대값을 생성하지 않는다.</summary>
public sealed class ChatNativeMessagePacketTests
{
    /// <summary>원문 CPU가 사용한 첫 manager 토큰이다.</summary>
    private const uint First = 0x12001000;
    /// <summary>원문 본문 검사의 둘째 manager 토큰이다.</summary>
    private const uint Second = 0x12004000;
    /// <summary>원문 본문 검사의 수신자 배열 토큰이다.</summary>
    private const uint Recipients = 0x12007000;
    /// <summary>선택된 외부 장치에서 중단하되 앞선 쓰기는 보존한다.</summary>
    private sealed class Stop(string where) : Exception
    {
        /// <summary>원문 관측에 대응하는 중단 위치다.</summary>
        public string Where { get; } = where;
    }
    /// <summary>원문 unsigned 숫자를 문화권 없이 읽는다.</summary>
    private static uint Number(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>원문 빈 표시와 원시 hex를 실제 BYTE로 변환한다.</summary>
    private static byte[] Bytes(string text) => text == "-" ? [] : Convert.FromHexString(text);
    /// <summary>전체 물리의 소문자 hex다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>좁은 표시 BYTE 문자열을 trace용 hex로 변환한다.</summary>
    private static string Hex(string text) => Hex(Encoding.Latin1.GetBytes(text));
    /// <summary>폭 밖의 BYTE를 보존하는 원문 비정렬 입력 쓰기다.</summary>
    private static void Put(byte[] bytes, int at, uint value, int width = 4)
    {
        // 지정한 폭만 낮은 바이트부터 쓴다.
        for (int index = 0; index < width; index++) bytes[at + index] = (byte)(value >> (index * 8));
    }
    /// <summary>실제 현재 DWORD 입력을 읽는다.</summary>
    private static uint Read(byte[] bytes, int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(at, 4));
    /// <summary>내부NUL 이후 입력도 보존하며 끝NUL만 추가한다.</summary>
    private static void Store(Span<byte> bytes, int at, string text) { Encoding.Latin1.GetBytes(text).CopyTo(bytes[at..]); bytes[at + text.Length] = 0; }
    /// <summary>비영 미작성 꼬리를 원문과 같은 규칙으로 구성한다.</summary>
    private static byte[] Pattern(int first)
    {
        byte[] bytes = new byte[2712];
        // 전체 manager 입력은 원문과 같은17 증가 패턴이다.
        for (int index = 0; index < bytes.Length; index++) bytes[index] = unchecked((byte)(first + index * 17));
        return bytes;
    }
    /// <summary>세 호출자·부분쓰기·중첩 변경·선택 중단을524관측에 대조한다.</summary>
    [Fact]
    public void ReplaysNativeMessage524()
    {
        int rows = 0;
        // 일반 헤더 뒤의 독립 원문만 사용한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-message-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(20, row.Length); uint count = Number(row[4]), receiver = row[5] == "1" ? Second : First;
            byte[] first = Pattern(0xa5), second = Pattern(0xb0), recipients = new byte[128], input = new byte[256], local = new byte[256]; input.AsSpan().Fill(0xd3); local.AsSpan().Fill(0xa5);
            // 두 manager의 실제 입력 필드만 초기화한다.
            foreach (byte[] memory in new[] { first, second }) { Put(memory, 0, count); Put(memory, 4, 32); Put(memory, 8, Recipients); Put(memory, 12, Number(row[3])); Put(memory, 16, Number(row[2])); Store(memory, 0x14, "~rBACK"); Store(memory, 0x50, "~gFRONT"); }
            if (count == 1) Put(recipients, 0, 200); else if (count == 2) { Put(recipients, 0, 100); Put(recipients, 4, 200); } Store(input, 0, Encoding.Latin1.GetString(Bytes(row[6])));
            var current = new UIntBox { Value = row[10] == "1" ? First : 0 }; var subscriber = new UIntBox { Value = 100 }; uint currentName = 1, result = 0; bool localUsed = false; var trace = new List<string>(); string mode = row[8];
            // 같은 토큰의 실제 물리를 복사 없이 빌리는 순수 조회다.
            byte[] Memory(uint token) => token switch { First => first, Second => second, Recipients => recipients, _ => throw new ArgumentOutOfRangeException(nameof(token)) };
            var hooks = new ChatMessageHooks
            {
                Memory = Memory, CurrentName = () => currentName == 2 ? "Changed" : Encoding.Latin1.GetString(Bytes(row[7])),
                NameForSubscriber = (destination, target) => { localUsed = true; trace.Add("lookup:" + target); if (mode == "stopResolve") throw new Stop("resolve"); if (mode == "resolveSwitch") { currentName = 2; subscriber.Value = 7; Put(Memory(receiver), 12, 0); Store(first, 0x14, "~pLateBack"); Store(first, 0x50, "~uLateFront"); Store(input, 0, "new~0body"); } Store(destination, 0, row[9] == "1" ? "Ta%s" : "SOMEONE"); return 1; },
                FormatName = (destination, pattern, name, other) => { localUsed = true; trace.Add("format:" + Hex(pattern) + ':' + Hex(name) + ':' + Hex(other) + ':' + (2712 - destination.Length)); ChatMessageBody.WriteChatNameFormat(destination, pattern, name, other); if (mode == "renderSwitch") { Put(Memory(receiver), 12, 0); Put(first, 16, 32); Store(first, 0x50, "~lRender"); Store(input, 0, "render~1changed"); } if (mode == "stopFormat") throw new Stop("format"); },
                StripSlots = text => { trace.Add("strip"); ChatMessageBody.DefaultStripSlots(text); },
                Deliver = (manager, target, body) => { Assert.Equal(First, manager); trace.Add("deliver:" + (target == Second ? "2" : "1") + ':' + body.Length + ':' + Hex(body)); if (mode == "stopDeliver") throw new Stop("deliver"); return 0x55aacc33; },
                InputVisible = visible => trace.Add("visible:" + visible),
            };
            var globals = new ChatMessageGlobals(current, subscriber); string status = "normal";
            try { result = row[0] switch { "format" => ChatMessageBody.FormatChatMessage(First, receiver, input, new string?[] { "%s> ", "%s ", "%s (to %s)> ", null }[Number(row[1])], local, globals, hooks), "wrap" => ChatMessageBody.FormatChatInput(First, input, local, globals, hooks), "say" => ChatMessageBody.SayChatMessage(input, local, globals, hooks), _ => throw new InvalidOperationException() }; }
            catch (Stop stopped) { status = stopped.Where; }
            Assert.Equal(row[11], status); if (status == "normal") Assert.Equal(Number(row[12]), result); Assert.Equal(Number(row[13]), currentName); Assert.Equal(Number(row[14]), subscriber.Value);
            Assert.Equal(row[15], Hex(first)); Assert.Equal(row[16], Hex(second)); Assert.Equal(row[17], Hex(input)); Assert.Equal(row[18], localUsed ? Hex(local) : "-"); Assert.Equal(row[19], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(524, rows);
    }
    /// <summary>전투/일반 복사폭·자기추가·중단·observer의256관측을 대조한다.</summary>
    [Fact]
    public void ReplaysNativePacket256()
    {
        int rows = 0;
        // 일반 헤더를 제외한256행의 기대값은 원문에서만 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-packet-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(18, row.Length); uint count = Number(row[4]), battle = Number(row[1]), subscriber = 100, result = 0; byte[] manager = Pattern(0xa5), recipients = new byte[1200], name = new byte[256]; recipients.AsSpan().Fill(0xa5); Bytes(row[7]).CopyTo(name, 0); Put(manager, 0, count); Put(manager, 4, 300); Put(manager, 8, Second); Put(manager, 12, Number(row[2])); Put(manager, 16, Number(row[3]));
            // 원문과 같은 중간 자기 항목 또는 순서대로 증가하는 구독자를 둔다.
            for (uint index = 0; index < count; index++) Put(recipients, (int)index * 4, row[5] == "1" && index == count / 2 ? 100 : 200 + index);
            var trace = new List<string>(); byte[] packet = []; string mode = row[8]; var hooks = new ChatPacketHooks
            {
                Memory = token => token == First ? manager : token == Second ? recipients : throw new ArgumentOutOfRangeException(nameof(token)), CurrentName = () => name,
                Read = address => address switch { RawChatPacket.SubscriberAddress => subscriber, RawChatPacket.BattleAddress => battle, RawChatPacket.ChannelOneAddress or RawChatPacket.MetamapAddress or 0x544c5c => 0, 0x52edf6 => 0x41, _ => throw new ArgumentOutOfRangeException(nameof(address)) },
                AddRecipient = (target, added) => { Assert.Equal(First, target); trace.Add("add:" + added); if (mode == "stopAdd") throw new Stop("add"); uint current = Read(manager, 0); Put(recipients, (int)current * 4, added); Put(manager, 0, current + 1); if (mode == "addSwitch") { subscriber = 7; battle = 1 - battle; Put(manager, 16, 128); Put(manager, 12, 1); Store(name, 0, "Modified"); } },
                Send = (target, bytes) => { Assert.Equal(9, target); packet = bytes; trace.Add("send:9:" + Hex(bytes)); if (mode == "stopSend") throw new Stop("send"); },
                Log = (channel, text) => { trace.Add("log:" + channel + ':' + Hex(text)); if (mode == "stopLog") throw new Stop("log"); }, ZeroAddress = () => trace.Add("address:0.0.0.0:0"),
                LocalReceive = bytes => { packet = bytes; trace.Add("receive:" + Hex(bytes)); if (mode == "stopReceive") throw new Stop("receive"); },
            };
            string status = "normal";
            try { if (row[0] == "send") result = ChatPacketBody.SendChatPacket(First, First, Bytes(row[6]), hooks); else { Assert.Equal("observe", row[0]); packet = new byte[32]; packet.AsSpan().Fill(0xd3); Put(packet, 10, Number(row[2]), 2); Put(packet, 12, Number(row[3]), 2); Put(packet, 14, count, 1); ChatPacketBody.ObserveChatPacket(packet, Number(row[9]), number => trace.Add("assert:" + number)); } }
            catch (Stop stopped) { status = stopped.Where; }
            Assert.Equal(row[10], status); if (row[11] != "-") Assert.Equal(Number(row[11]), result); Assert.Equal(Number(row[12]), subscriber); Assert.Equal(Number(row[13]), battle); Assert.Equal(row[14], Hex(manager)); Assert.Equal(row[15], Hex(recipients)); Assert.Equal(row[16], packet.Length == 0 ? "-" : Hex(packet)); Assert.Equal(row[17], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(256, rows);
    }
}
