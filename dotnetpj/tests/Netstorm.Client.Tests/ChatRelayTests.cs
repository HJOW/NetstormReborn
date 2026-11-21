using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;
using Netstorm.Net;

// 원문132관측과 실제 mutable 등록표에서 중계의 현재 조회/부분 효과를 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>RawChatRelay의 원문 전수/재조회/고정 F와 observer 선택 경계를 검증한다.</summary>
public sealed class ChatRelayTests
{
    /// <summary>원문 관측에서 선택한 외부 장치의 중단 이유다.</summary>
    private sealed class Stop(string reason) : Exception(reason) { }
    /// <summary>현재 물리에 지정한 BYTE/WORD/DWORD만 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 인접 물리는 보존하고 낮은 BYTE부터 기록한다.
        for (int index = 0; index < width; index++) bytes[at + index] = (byte)(value >> (index * 8));
    }
    /// <summary>원문과 같은 소문자 전체 물리 직렬화다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexStringLower(bytes);
    /// <summary>원문 fixture의 명시 입력만 만들고 기대값을 계산하지 않는다.</summary>
    private static byte[] Packet(byte[] body, uint count)
    {
        byte[] packet = Enumerable.Repeat((byte)0xd3, 256).ToArray(); Put(packet, 0, 3, 1); Put(packet, 1, 0x41, 1); Put(packet, 2, (uint)(22 + body.Length + count * 4), 2); Put(packet, 5, 1, 1); Put(packet, 6, 100); Put(packet, 10, 7, 2); Put(packet, 12, (uint)body.Length, 2); Put(packet, 14, count, 1); "Local\0\0"u8.CopyTo(packet.AsSpan(15)); body.CopyTo(packet, 22);
        // 원문 독립 관측은 자기 subscriber100만 수신자에 넣었다.
        for (int index = 0; index < count; index++) Put(packet, 22 + body.Length + index * 4, 100);
        return packet;
    }
    /// <summary>원문132행의 전체256BYTE/마스크/주소·수신 순서/중단 상태를 그대로 대조한다.</summary>
    [Fact]
    public void IndependentNative132WholePacketAndMask()
    {
        int rows = 0;
        // 일반 헤더만 제외하고 모든 기존 원문 관측을 사본 없이 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-relay-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(9, row.Length); uint battle = uint.Parse(row[0], CultureInfo.InvariantCulture), server = uint.Parse(row[1], CultureInfo.InvariantCulture), count = uint.Parse(row[2], CultureInfo.InvariantCulture); byte[] packet = Packet(Convert.FromHexString(row[3]), count); var mask = new UIntBox { Value = 0xaabbccdd }; var trace = new List<string>(); string mode = row[4];
            var hooks = new ChatRelayHooks
            {
                Read = at => at switch { 0x594fbc => battle, 0x540bc0 => server, 0x540c74 => 100, _ => throw new InvalidOperationException("원문 관측에 없는 전역") },
                Address = sender => { trace.Add("address:" + sender.ToString(CultureInfo.InvariantCulture)); if (mode == "stopAddress") throw new Stop("address"); if (mode == "addressBattleOff") battle = 0; return Enumerable.Range(0, 20).Select(index => (byte)index).ToArray(); },
                Receive = (bytes, address) => { trace.Add("receive:" + Hex(bytes.Span) + ':' + Hex(address.Span)); if (mode == "stopReceive") throw new Stop("receive"); },
            };
            string status = "normal"; try { ChatRelayBody.RelayChatPacket(9, packet, mask, hooks); } catch (Stop stop) { status = stop.Message; }
            Assert.Equal(row[5], status); Assert.Equal(uint.Parse(row[6], CultureInfo.InvariantCulture), mask.Value); Assert.Equal(row[7], Hex(packet)); Assert.Equal(row[8], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(132, rows);
    }
    /// <summary>원격 callback 뒤 현재 본문 주소/채널을 읽되 처음 count와 주소 값 사본/마스크는 보존한다.</summary>
    [Fact]
    public void RemoteCallbacksRequeryCurrentPacketAndChannel()
    {
        byte[] packet = new byte[64]; Put(packet, 10, 0, 2); Put(packet, 12, 5, 2); Put(packet, 14, 2, 1); "005x\n"u8.CopyTo(packet.AsSpan(15)); Put(packet, 20, 200); Put(packet, 24, 100); var mask = new UIntBox(); byte[] address = new byte[20]; int channel = 2; var trace = new List<string>();
        ChatRelayBody.RelayChatPacket(9, packet, mask, new()
        {
            Address = _ => address,
            Read = at => { address[0] = 99; return at switch { 0x594fbc or 0x540bc0 => 1, 0x540c74 => 100, _ => throw new InvalidOperationException() }; },
            Player = subscriber => { Assert.Equal(200u, subscriber); return 2; }, Channel = _ => { trace.Add("channel:" + channel); return channel; },
            Connected = value => { Assert.Equal(2, value); channel = 7; return true; },
            Send = (target, bytes) => { Assert.Equal(7, target); trace.Add("send"); Put(bytes.Span, 12, 6, 2); bytes.Span[14] = 99; "009"u8.CopyTo(bytes.Span[15..]); Put(bytes.Span, 25, 100); },
            Receive = (bytes, source) => { trace.Add("receive"); Assert.Equal(0, source.Span[0]); Assert.Equal(100u, BinaryPrimitives.ReadUInt32LittleEndian(bytes.Span.Slice(21, 4))); },
        });
        Assert.Equal(new[] { "channel:2", "channel:7", "send", "receive" }, trace); Assert.Equal(5u, mask.Value); Assert.Equal(99, packet[14]); Assert.Equal(6, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(12, 2)));
    }
    /// <summary>장기 고정 F/observer가 같은 부분 물리를 보존하고 일반 재등록은 이미 설치한 실제 F를 유지한다.</summary>
    [Fact]
    public void CanonicalRegistrationPreservesOwnerAndMutableWindow()
    {
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } }); var mask = new UIntBox(); byte[] all = Enumerable.Repeat((byte)0xd3, 72).ToArray(); Memory<byte> packet = all.AsMemory(4, 64); Put(packet.Span, 10, 0, 2); Put(packet.Span, 12, 5, 2); Put(packet.Span, 14, 1, 1); "032x\n"u8.CopyTo(packet.Span[15..]); Put(packet.Span, 20, 100); int receives = 0;
        var hooks = new ChatRelayHooks { Address = _ => new byte[20], Read = at => at == 0x540c74 ? 100u : 1u, Receive = (_, _) => receives++ };
        ChatRelayBody.RegisterCanonicalChatPacket(dispatch, mask, hooks); var entry = dispatch.Registration(0x41); var fixedReceiver = dispatch.SnapshotReceiver(0x41); Assert.Equal(0x0042dc90u, fixedReceiver!.NativeEntry); Assert.Equal(1, entry.Version); Assert.Equal(0, entry.Mismatch); Assert.Equal(ushort.MaxValue, entry.Length); Assert.Null(entry.Observer); Assert.NotNull(entry.PacketObserver);
        hooks.Receive = (_, _) => throw new InvalidOperationException("외부 재연결이 캡처를 바꾸면 안 됨"); entry.PacketObserver!(packet, 1); Assert.Equal(24, BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(2, 2))); dispatch.InvokeDirect(0x41, 9, null, packet); Assert.Equal(1, receives); Assert.Equal(32u, mask.Value); Assert.Equal(0, packet.Span[19]); Assert.All(all.Take(4), value => Assert.Equal(0xd3, value)); Assert.All(all.Skip(68), value => Assert.Equal(0xd3, value));
        ChatRelayBody.RegisterChatPacket(dispatch, new UIntBox(), new()); Assert.Same(fixedReceiver, dispatch.SnapshotReceiver(0x41)); dispatch.InvokeDirect(0x41, 9, null, packet); Assert.Equal(2, receives); dispatch.Version(0x41, 2, 0); Assert.Throws<InvalidOperationException>(() => ChatRelayBody.RegisterChatPacket(dispatch, mask, new())); Assert.Equal(2, entry.Version);
    }
    /// <summary>관찰은 선택 진단만 요구하고 callback 뒤 현재 필드/WORD 절단과 도달 불가 bufferLen 진단을 보존한다.</summary>
    [Fact]
    public void ObserverKeepsLazyDiagnosticsAndCurrentFields()
    {
        ChatPacketBody.ObserveChatPacket(Memory<byte>.Empty, 0, null); byte[] bytes = Enumerable.Repeat((byte)0xd3, 40).ToArray(); Memory<byte> packet = bytes.AsMemory(4, 24); Put(packet.Span, 10, 0xffff, 2); Put(packet.Span, 12, 0xffff, 2); packet.Span[14] = 0xff; var lines = new List<uint>();
        ChatPacketBody.ObserveChatPacket(packet, 1, line => { lines.Add(line); Put(packet.Span, 10, 7, 2); packet.Span[14] = 2; }); Assert.Equal(new[] { 0x81du }, lines); Assert.Equal(29, BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(2, 2))); Assert.All(bytes.Take(4), value => Assert.Equal(0xd3, value)); Assert.All(bytes.Skip(28), value => Assert.Equal(0xd3, value));
        Put(packet.Span, 10, 0, 2); Put(packet.Span, 12, 0, 2); packet.Span[14] = 0; ChatPacketBody.ObserveChatPacket(packet, 1, null); Assert.Equal(15, BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(2, 2))); Put(packet.Span, 10, 0xffff, 2); Assert.Throws<InvalidOperationException>(() => ChatPacketBody.ObserveChatPacket(packet, 1, null));
    }
}
