using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 서버 writer의 독립 원문 관측과 현재 전역 재조회·실제 로컬 운반을 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>원본187입력의 전체 Player/전역/wire·부분 효과와 공유 observer를 검증한다.</summary>
public sealed class MissionServerPacketsTests
{
    /// <summary>지정 DWORD 입력만 같은 실제 저장소에 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>현재 선언길이 WORD를 읽는다.</summary>
    private static ushort Length(ReadOnlySpan<byte> packet) => BinaryPrimitives.ReadUInt16LittleEndian(packet[2..]);
    /// <summary>원문 단일바이트 문자열을 fixture와 같은 hex로 나타낸다.</summary>
    private static string Hex(string text) => Hex(Encoding.Latin1.GetBytes(text));
    /// <summary>바이트 결과의 비트와 대소문자를 원본 관측 형식에 맞춘다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>원본에 공급한 명시 입력이며 정제/송신 결과를 계산하지 않는다.</summary>
    private static byte[] Input(uint owner, ReadOnlySpan<byte> name, uint flags)
    {
        byte[] players = new byte[1620];
        // native와 같은 전체 stale 입력을 구성한다.
        for (int index = 0; index < players.Length; index++) players[index] = unchecked((byte)(index * 37 + 11));
        int offset = checked((int)owner * 180); players.AsSpan(offset + 12, 32).Clear(); name.CopyTo(players.AsSpan(offset + 12)); players[offset + 12 + name.Length] = 0;
        Put(players, offset + 0x2c, 0x12345680 + owner); Put(players, offset + 0x50, flags); return players;
    }
    /// <summary>같은 실제 getter를 가진 최소 검사 전역이다. 중앙섬 미생성과 자기 번호를 독립 선택한다.</summary>
    private static MissionServerPacketState State(byte[] zone, Func<bool>? server = null, Func<bool>? metamap = null, Func<uint>? local = null, Func<uint>? created = null, Func<uint>? archipelago = null)
        => new() { ZoneAddress = zone, Server = server ?? (() => true), Metamap = metamap ?? (() => false), LocalOwner = local ?? (() => 1), CentralCreated = created ?? (() => 1), Archipelago = archipelago ?? (() => 0) };

    /// <summary>원본160 Player·24 metadata writer·3수신 입력의 모든 바이트와 외부 효과를 대조한다.</summary>
    [Fact]
    public void OriginalServerPackets187()
    {
        string[][] rows = X86Fixture.Read("mission-server-packets").Where(row => row[0] != "input").ToArray(); Assert.Equal(187, rows.Length); var seen = new HashSet<string>();
        // 각 원본 입력을 새 실제 저장소에서 실행해 행 사이 상태가 누적되지 않게 한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0])); string[] fields = row[0].Split(','); Assert.Equal(10, fields.Length);
            uint owner = X86Fixture.UInt(fields[1]), flags = X86Fixture.UInt(fields[3]); byte[] name = fields[2] == "-" ? [] : X86Fixture.Bytes(fields[2]); byte[] players = Input(owner, name, flags), zone = new byte[6];
            bool server = X86Fixture.UInt(fields[4]) != 0, metamap = X86Fixture.UInt(fields[5]) != 0; uint local = 1, created = X86Fixture.UInt(fields[7]), archipelago = X86Fixture.UInt(fields[9]); Put(zone, 0, X86Fixture.UInt(fields[8]));
            var trace = new List<string>(); RawMissionServerPackets? current = null;
            var hooks = new MissionServerPacketHooks
            {
                Assertion = text => trace.Add("A," + Hex(text)), Log = text => trace.Add("L," + Hex(text)), PeerAddress = player => { trace.Add("IP," + player); return "127.0.0.1"; },
                Send = (target, packet) =>
                {
                    if (packet.Span[1] == 0x72) current!.ObserveMetadata(packet, 1);
                    else RawMissionPacketFields.ObserveClient(packet.Span[1], packet, 1, (text, _, _) => trace.Add("A," + Hex(text)));
                    trace.Add($"S,{unchecked((uint)target)},{Hex(packet.Span[..Length(packet.Span)])}");
                },
            };
            current = new(players, State(zone, () => server, () => metamap, () => local, () => created, () => archipelago), hooks); int target = unchecked((int)X86Fixture.UInt(fields[6]));
            if (fields[0] == "P") current.SendPlayer(owner, target);
            else if (fields[0] == "M") current.SendMetadata(target);
            else { byte[] packet = new byte[32]; Array.Fill(packet, (byte)255); name.CopyTo(packet.AsSpan(10)); packet[10 + name.Length] = 0; current.ReceiveMetadata(packet); }
            Assert.Equal(X86Fixture.Bytes(row[1]), players); Assert.Equal(row[2], $"{(server ? 1 : 0)},{local},{created},{BinaryPrimitives.ReadUInt32LittleEndian(zone)},{archipelago}");
            string expected = string.Join(';', row[3].Split(';').Where(item => !item.StartsWith("N,"))); // 이름 함수 내부 진입 통계만 제외하며 결과 원문은 위에서 전수 비교한다.
            Assert.Equal(string.IsNullOrEmpty(expected) ? "-" : expected, trace.Count == 0 ? "-" : string.Join(';', trace));
        }
    }

    /// <summary>주소 callback 뒤 현재 필드는 다시 읽되 먼저 복사한 이름/구독자는 원문처럼 유지한다.</summary>
    [Fact]
    public void PlayerWriterReadsCurrentFieldsAfterPeerCallback()
    {
        byte[] players = Input(1, "Old Name"u8, 1), zone = new byte[6]; bool metamap = true; var trace = new List<string>(); Memory<byte> sent = default;
        var packets = new RawMissionServerPackets(players, State(zone, metamap: () => metamap), new()
        {
            Assertion = text => { Assert.Equal("!inMetamapMode", text); trace.Add("assert"); },
            PeerAddress = owner =>
            {
                Assert.Equal(1u, owner); trace.Add("peer"); Put(players, 180 + 0x50, 0x11223344); Put(players, 180 + 0x54, 0x55667788); Put(players, 180 + 0x2c, 7);
                players[192] = (byte)'X'; return "127.0.0.1\0ignored";
            },
            Send = (target, packet) => { Assert.Equal(-7, target); Assert.Equal(100, packet.Length); Assert.Equal(4, Length(packet.Span)); trace.Add("send"); sent = packet; },
        });
        packets.SendPlayer(1, -7); Assert.Equal(new[] { "assert", "assert", "peer", "send" }, trace);
        Assert.Equal(0x11223344u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Span[0x2a..])); Assert.Equal(0x55667788u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Span[0x22..]));
        Assert.Equal(0x12345681u, BinaryPrimitives.ReadUInt32LittleEndian(sent.Span[0x26..])); Assert.Equal("OldName\0"u8.ToArray(), sent.Span.Slice(0x3b, 8).ToArray()); Assert.Equal(255, sent.Span[99]);
    }

    /// <summary>실제 도달한 누락만 거부하며 앞서 정제한 이름과 원본 자기 metadata 조기 반환을 보존한다.</summary>
    [Fact]
    public void MissingSelectedHooksPreservePrefixAndUnselectedHooksAreNotRequired()
    {
        byte[] players = Input(1, [], 1), zone = new byte[6]; var packets = new RawMissionServerPackets(players, State(zone), new());
        Assert.Throws<InvalidOperationException>(() => packets.SendPlayer(1, 2)); Assert.Equal("Cheat129\0"u8.ToArray(), players.AsSpan(192, 9).ToArray());
        packets.SendMetadata(1); packets.ObserveMetadata(Memory<byte>.Empty, 0);
        Put(zone, 0, 0x100); packets.ReceiveMetadata(new byte[11]); Assert.Throws<ArgumentOutOfRangeException>(() => packets.ReceiveMetadata(new byte[10]));
        byte[] central = new byte[11]; central[10] = (byte)'$'; Assert.Throws<InvalidOperationException>(() => packets.ReceiveMetadata(central));
        Assert.Throws<ArgumentException>(() => new RawMissionServerPackets(new byte[1619], State(zone), new()));
        Assert.Throws<ArgumentOutOfRangeException>(() => packets.SendPlayer(9, 2));
    }

    /// <summary>현재 배치가2이면 전용 중앙섬 하위 뒤 생성표시/개수 로그 없이 반환한다.</summary>
    [Theory]
    [InlineData((byte)0)]
    [InlineData((byte)2)]
    public void MetadataUsesCurrentLayoutAndSnapshotsSixZoneBytes(byte layout)
    {
        byte[] zone = [0, 1, 2, 3, 4, 5], packet = new byte[18]; packet[5] = 7; packet[10] = (byte)'$'; uint archipelago = 0, created = 0; var trace = new List<string>();
        var packets = new RawMissionServerPackets(new byte[1620], State(zone, created: () => created, archipelago: () => archipelago), new()
        {
            CreatingIslands = () => { trace.Add("creating"); packet[5] = 9; }, Log = text => trace.Add(text), SetArchipelago = value => { archipelago = value; trace.Add("arch:" + value); zone[4] = 22; },
            Layout = () => { trace.Add("layout"); zone[5] = 33; return layout; }, SurroundedCentral = () => trace.Add("surrounded"),
            RandomCentral = record => { trace.Add("random"); Assert.Equal(new byte[] { 0, 1, 2, 3, 22, 33 }, record.ToArray()); zone[0] = 44; Assert.Equal(0, record.Span[0]); },
            SetCentralCreated = value => { trace.Add("created:" + value); created = value; }, PredictableCount = () => { trace.Add("count"); return uint.MaxValue; },
        });
        packets.ReceiveMetadata(packet); Assert.Equal(9u, archipelago);
        Assert.Equal(layout == 2 ? new[] { "creating", "createArchipelago... ", "arch:9", "layout", "surrounded" } : new[] { "creating", "createArchipelago... ", "arch:9", "layout", "random", "created:1", "count", "predictableCount = -1\n" }, trace);
        Assert.Equal(layout == 2 ? 0u : 1, created);
    }

    /// <summary>metadata 자기 수신 callback 뒤 현재 로컬 번호를 다시 읽고 같은 최초 packet 밀도를 보낸다.</summary>
    [Fact]
    public void MetadataWriterRereadsLocalOwnerAfterSelfDelivery()
    {
        byte[] zone = [0, 1, 0, 0, 0, 0]; uint local = 1, archipelago = 0x1234, created = 0; Memory<byte> sent = default;
        var packets = new RawMissionServerPackets(new byte[1620], State(zone, local: () => local, created: () => created, archipelago: () => archipelago), new()
        {
            CreatingIslands = () => { local = 2; archipelago = 99; }, Log = _ => { }, SetArchipelago = value => archipelago = value, Layout = () => 2,
            SurroundedCentral = () => { }, Send = (target, packet) => { Assert.Equal(1, target); Assert.Equal(4, Length(packet.Span)); sent = packet; },
        });
        packets.SendMetadata(1); Assert.Equal(0x34, sent.Span[5]); Assert.Equal(18, sent.Length); Assert.Equal("$Random\0"u8.ToArray(), sent.Span[10..].ToArray());
    }

    /// <summary>송신 진단이 바꾼 현재 필드·실제 줄번호·WORD 감김과 방향0 조기 반환을 검사한다.</summary>
    [Theory]
    [InlineData(0x2au, 12, 4, 0x672u, "fortFileSize != (int)-1", 20)]
    [InlineData(0x2bu, 0x33, 1, 0x695u, "nameLen != (unsigned char)-1", 62)]
    [InlineData(0x67u, 5, 2, 0xa07u, "dataLen != (unsigned short)-1", 10)]
    [InlineData(0x81u, 4, 4, 0xc3du, "numPlayers != (int)-1", 20)]
    public void ClientObserverReReadsAfterReturningAssertion(uint type, int at, int width, uint line, string text, int length)
    {
        byte[] packet = new byte[100]; Array.Fill(packet, (byte)0xa5); packet.AsSpan(at, width).Fill(255); int assertions = 0;
        RawMissionPacketFields.ObserveClient(type, packet, 1, (actualText, file, actualLine) =>
        {
            assertions++; Assert.Equal(text, actualText); Assert.Equal("\\Ns\\O\\_p_packets.cpp", file); Assert.Equal(line, actualLine);
            packet.AsSpan(at, width).Clear(); packet[at] = 3;
        });
        Assert.Equal(1, assertions); Assert.Equal(length, Length(packet)); Assert.Equal(0xa5, packet[99]);
        RawMissionPacketFields.ObserveClient(type, Memory<byte>.Empty, 0); RawMissionPacketFields.ObserveClient(0x29, Memory<byte>.Empty, 1);
        packet.AsSpan(at, width).Fill(255); if (type == 0x2b) packet[at] = 254;
        RawMissionPacketFields.ObserveClient(type, packet, 1, (_, _, _) => { });
        Assert.Equal(type switch { 0x2a => 16, 0x2b => 313, 0x67 => 6, _ => 4 }, Length(packet));
    }

    /// <summary>독립 PE의 기존 fort observer10입력을 공유 2a 본체에도 전수 대조한다.</summary>
    [Fact]
    public void OriginalFortObserver10()
    {
        string[][] rows = X86Fixture.Read("fort-image-packets").Where(row => row[0] == "F").ToArray(); Assert.Equal(10, rows.Length);
        // 기존 기대값은 수정하지 않고 같은 명시 입력만 공급한다.
        foreach (string[] row in rows)
        {
            uint[] input = row[1].Split(',').Select(X86Fixture.UInt).ToArray(); byte[] packet = new byte[20]; Array.Fill(packet, (byte)0xa5); Put(packet, 12, input[8]); var trace = new List<string>();
            RawMissionPacketFields.ObserveClient(0x2a, packet, input[12] != 0 ? 1u : 0, (_, _, _) => trace.Add("A,packet"));
            Assert.Equal(X86Fixture.Bytes(row[3]), packet); Assert.Equal(row[6], trace.Count == 0 ? "-" : string.Join(';', trace));
        }
    }

    /// <summary>현재 metadata lowWORD 재조회와 진단 전후 주변바이트를 보존한다.</summary>
    [Fact]
    public void MetadataObserverPreservesPhysicalTailAndReturningAssertion()
    {
        byte[] packet = new byte[18]; Array.Fill(packet, (byte)255); int assertions = 0;
        RawMissionPacketFields.ObserveMetadata(packet, 1, text => { assertions++; Assert.Equal("len != (int)-1", text); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6), 7); });
        Assert.Equal(1, assertions); Assert.Equal(17, Length(packet)); Assert.Equal(255, packet[17]);
        Put(packet, 6, 65535); RawMissionPacketFields.ObserveMetadata(packet, 1); Assert.Equal(9, Length(packet));
        RawMissionPacketFields.ObserveMetadata(Memory<byte>.Empty, 0);
    }

    /// <summary>이름 삭제 뒤 stale 꼬리·빈 이름 대체와 실제 물리 범위 오류를 구별한다.</summary>
    [Fact]
    public void NameSanitizerKeepsStaleBytesAndRejectsMissingNul()
    {
        byte[] name = [(byte)'A', (byte)' ', (byte)' ', (byte)'B', 0, 0x77]; RawMissionPacketFields.SanitizePlayerName(0, name);
        Assert.Equal(new byte[] { 65, 66, 0, 0, 0, 0x77 }, name); byte[] empty = new byte[12]; RawMissionPacketFields.SanitizePlayerName(0x1ff, empty); Assert.Equal("Cheat255\0"u8.ToArray(), empty[..9]);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawMissionPacketFields.SanitizePlayerName(0, new byte[] { 1, 2 }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawMissionPacketFields.SanitizePlayerName(0, new byte[2]));
    }

    /// <summary>실제 같은 양끝/공유표/복제 observer로 metadata를 운반한다.</summary>
    [Fact]
    public void MetadataTraversesActualLocalTransportAndRegistration()
    {
        byte[] players = new byte[1620], zone = new byte[6]; players[180 + 0x50] = 1; var received = new List<string>();
        using var wire = new LocalMissionTransport(players, () => true, new()
        {
            Now = () => 1, Record = (_, _, _) => { }, Terminate = _ => throw new InvalidOperationException("예상 밖 종료"), Close = _ => throw new InvalidOperationException("예상 밖 닫기"), Release = _ => throw new InvalidOperationException("예상 밖 반납"),
        });
        wire.RegisterLogin((_, _, _) => throw new InvalidOperationException("이 검사는 로그인 요청을 보내지 않는다"));
        var packets = new RawMissionServerPackets(players, State(zone), new() { Send = (target, packet) => Assert.True(wire.SendMutable(target, packet)), Log = received.Add }); MissionMetadataRegistration.Register(wire.Dispatch, packets);
        PacketRegistration registration = wire.Dispatch.Registration(0x72); Assert.Equal(1, registration.Version); Assert.Equal(0, registration.Mismatch); Assert.Equal(65535, registration.Length); Assert.NotNull(registration.Receive); Assert.NotNull(registration.PacketObserver);
        wire.Open("127.0.0.1", 6799); Assert.Equal(0, wire.Poll()); wire.AssignConnection(-1, 1); packets.SendMetadata(9); Assert.Empty(received); wire.Flush(); Assert.Equal(1, wire.Poll()); Assert.Equal(new[] { "No Central Island.\n" }, received);
    }
}
