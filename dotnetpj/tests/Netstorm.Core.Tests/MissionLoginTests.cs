using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 로그인 부모의 원본 입력과 현재 기록을 실제 참여/집계 모듈에 공급한다.
namespace Netstorm.Core.Tests;

/// <summary>로그인48입력·집계394입력과 실제 연결/부분효과 경계를 검증한다.</summary>
public sealed class MissionLoginTests
{
    /// <summary>명시 입력의 DWORD만 같은 실제 저장소에 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>현재 원문 바이트의 fixture 표현이다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>단일바이트 원문 문자열의 비트를 보존한다.</summary>
    private static string Hex(string text) => Hex(System.Text.Encoding.Latin1.GetBytes(text));
    /// <summary>실제 요청 writer에 독립 명시 이름/이미지만 공급한다.</summary>
    private static ReadOnlyMemory<byte> LoginPacket(uint subscriber) => LocalMissionPackets.MakeLocalMissionLogin(subscriber, "N", new byte[] { 1, 3, 5, 7, 9 }).BytesMemory();

    /// <summary>원본 부모48입력의 동일 Player/참여/현재 전역과 외부 하위 관찰이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 Player9다.</summary>
        public byte[] Players { get; } = new byte[1620];
        /// <summary>같은 현재 옵션87바이트다.</summary>
        public byte[] Options { get; } = new byte[87];
        /// <summary>같은 실제 Player의 두 owner/known이다.</summary>
        public PlayerWorldState Player { get; } = new();
        /// <summary>현재 CoreData 포인터의 실제 메모리 별칭이다.</summary>
        public Memory<byte> Core { get; set; }
        /// <summary>현재 서버 관문이다.</summary>
        public bool Server { get; set; }
        /// <summary>독립 원본의 callback 변경 종류다.</summary>
        public uint Mutate { get; }
        /// <summary>독립 원본의 현재 clock 입력이다.</summary>
        public uint Clock { get; }
        /// <summary>한 번만 공급할 명시 변경을 실행했는지다.</summary>
        private bool _mutated;
        /// <summary>같은 실제 외부 효과 순서다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>실제 같은 참여 모듈이다.</summary>
        public RawPlayerParticipation Participation { get; }
        /// <summary>같은 현재 전역 참조 뷰다.</summary>
        public MissionLoginGlobals Globals { get; }
        /// <summary>원본 분기를 대신하지 않는 실제 외부 입력/관찰이다.</summary>
        public MissionLoginHooks<PacketConnection> Hooks { get; }
        /// <summary>같은 실제 연결을 받는 로그인 본체다.</summary>
        public RawMissionLogin<PacketConnection> Login { get; }
        /// <summary>명시 원본 입력만 같은 저장소에 준비하며 결과를 계산하지 않는다.</summary>
        public Scene(uint[] input)
        {
            Server = input[0] != 0; Player.PlayerKnown = input[1]; Player.BattleOwner = Player.TemplateOwner = input[2]; Mutate = input[6]; Clock = input[7];
            Put(Players, 180 + 0x2c, 1); Put(Players, 180 + 0x50, input[4]); if (input[5] != 0) { Put(Players, 360 + 0x2c, 2); Put(Players, 360 + 0x50, input[5]); Put(Players, 360 + 0x60, 88); }
            // 현재 옵션 원문 입력이며 packet 결과는 원본 fixture로만 대조한다.
            for (int index = 0; index < Options.Length; index++) Options[index] = (byte)index;
            RawMissionLogin<PacketConnection>? current = null;
            Participation = new(OriginalEdition.Patch1078, Players, new uint[81], new byte[9], new(), new()
            {
                AssignConnection = (pending, owner) => Trace.Add($"Assign,{pending},{owner}"), RejectLogin = owner => current!.SendReply(owner, 0), Disconnect = owner => Trace.Add("Disconnect," + owner),
            });
            Globals = new()
            {
                Server = () => Server, Boss = () => true, Simulconnect = () => false, MissionWorld = () => true, MissionPost = () => true, Metamap = () => false, Player = Player, MySubscriber = () => 1,
                LastLoginPlayer = new() { Value = 7 }, PredictableCount = new() { Value = 17 }, BeginSeed = () => 0x12345678, BattleSeed = () => 0x87654321, DebugBurst = () => 0, ClockStamp = new() { Value = 0x55 }, BaseClock = () => 3000, CaptureEnd = () => 500.75,
                ReadCoreData = () => Core, StoreCoreData = value => Core = value, CachedCoreData = new byte[10], ZoneAddress = new byte[] { 0x81, 0x82, 0x83, 0x84, 0x85, 0x86 }, BattleOptions = Options,
            };
            Hooks = new()
            {
                Disconnect = owner => Trace.Add("Disconnect," + owner), Send = (target, bytes) =>
                {
                    RawMissionPacketFields.ObserveClient(bytes.Span[1], bytes, 1, (text, _, _) => Trace.Add("A," + Hex(text))); int length = BinaryPrimitives.ReadUInt16LittleEndian(bytes.Span[2..]); Trace.Add($"S,{target},{Hex(bytes.Span[..length])}");
                    if (Mutate == 1 && !_mutated && bytes.Span[1] == 0x29) { Put(Players, 360 + 0x2c, 2); Put(Players, 360 + 0x50, 0x143); _mutated = true; }
                },
                Broadcast = bytes => Trace.Add("Broadcast," + Hex(bytes.Span)), PendingReply = (_, bytes) => Trace.Add("Pending," + Hex(bytes.Span)), Flush = () => Trace.Add("Flush"),
                WallSeconds = () => { Trace.Add("Wall"); return 100.5; }, Clock = () => { Trace.Add("Clock"); return Clock; }, SpBits = owner => { Trace.Add("SP," + owner); return BitConverter.SingleToUInt32Bits(owner * 1250.25f - 1700.5f); },
                ApplyOptions = bytes => Trace.Add("Options," + Hex(bytes.Span)), ClearTypes = () => Trace.Add("ClearTypes"), SendPlayer = (owner, target) => Trace.Add($"Player,{owner},{target}"), SendFortMetadata = owner => Trace.Add("Metadata," + owner),
                LoadCurrentFort = owner => { Trace.Add("FortName"); Trace.Add($"FortFile,{owner},6d697373696f6e2e666f7274"); },
                LoadFortImage = (owner, image) => { Trace.Add($"ImageName,{owner},1"); Trace.Add($"FortOpen,{owner},1"); Trace.Add($"FortImage,{owner},{Hex(image.Span)}"); },
                PeerAddress = owner => { Trace.Add("IP," + owner); return "127.0.0.1"; }, SendFort = (owner, target, count) => Trace.Add($"Fort,{owner},{target},{count}"), SynchronizePeer = owner => Trace.Add("Synchronize," + owner),
                LoadWorld = () => { Trace.Add("World"); if (Mutate == 2) { Player.BattleOwner = 3; Player.TemplateOwner = 4; Put(Players, 540 + 0x50, 0x200); } }, SendSettings = owner => Trace.Add("Settings," + owner), Assertion = text => Trace.Add("A," + Hex(text)),
            };
            Login = current = new(Players, Participation, Globals, Hooks);
        }
        /// <summary>현재9전역과 실제 CoreData offset을 원본 결과 순서로 직렬화한다.</summary>
        public string GlobalsText()
        {
            uint identity = 0; if (!Core.IsEmpty) { Assert.True(MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)Core, out ArraySegment<byte> segment)); Assert.Same(Players, segment.Array); identity = (uint)segment.Offset + 1; }
            return string.Join(',', new uint[] { Player.BattleOwner, Player.TemplateOwner, Player.PlayerKnown, Globals.FortCount.Value, Globals.LastLoginPlayer.Value, Globals.PredictableCount.Value, Globals.Begin.Value, Globals.ClockStamp.Value, identity });
        }
    }

    /// <summary>독립 원본48입력의 전체 Player1620바이트/9전역/모든패킷과 효과를 대조한다.</summary>
    [Fact]
    public void OriginalLogin48()
    {
        string[][] rows = X86Fixture.Read("mission-login").Where(row => row[0] != "input").ToArray(); Assert.Equal(48, rows.Length); var seen = new HashSet<string>();
        // 각 입력을 새 실제 참여 모듈과 같은 저장소에서 실행한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0])); uint[] input = row[0].Split(',').Select(X86Fixture.UInt).ToArray(); var scene = new Scene(input); scene.Login.Receive(-1, new(), LoginPacket(input[3]));
            Assert.Equal(row[1], Hex(scene.Players)); Assert.Equal(row[2], scene.GlobalsText()); Assert.Equal(row[3], string.Join(';', scene.Trace));
        }
    }

    /// <summary>원본 타이머 fixture 중C394입력의 전체5집계를 실제 같은 Player에서 읽고 변경하지 않는다.</summary>
    [Fact]
    public void OriginalPlayerCounts394()
    {
        uint[] flags = [0, 1, 2, 3, 0x40, 0x41, 0x42, 0x43, 0x100, 0x101, 0x102, 0x103, 0x140, 0x142, 0x143, uint.MaxValue];
        string[][] rows = X86Fixture.Read("mission-login-timer").Where(row => row[0] == "C").ToArray(); Assert.Equal(394, rows.Length); var seen = new HashSet<string>();
        // 독립 native의 pattern/subscriber/flags 명시 입력만 복원한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[1])); uint pattern = X86Fixture.UInt(row[1].Split('|')[0].Split(',')[0]); byte[] players = new byte[1620];
            // 집계가 읽지 않는 stale 원문도 동일 입력으로 보존한다.
            for (int index = 0; index < players.Length; index++) players[index] = unchecked((byte)(index * 37 + pattern * 11));
            // Player0은 의도적인 stale 입력이며 집계에서 제외한다.
            for (int owner = 0; owner < 9; owner++) { Put(players, owner * 180 + 0x2c, 0); Put(players, owner * 180 + 0x50, 0); }
            Put(players, 0x2c, 0xdeadbeef); Put(players, 0x50, uint.MaxValue);
            /// <summary>하나의 명시 구독자/flags 입력만 쓴다.</summary>
            void Record(int owner, uint subscriber, uint bits) { Put(players, owner * 180 + 0x2c, subscriber); Put(players, owner * 180 + 0x50, bits); }
            if (pattern >= 16) { uint ordinal = pattern - 16; uint[] subs = [0, 7, uint.MaxValue]; Record((int)(ordinal / 48) + 1, subs[ordinal / 16 % 3], flags[ordinal % 16]); }
            else if (pattern == 1) Record(1, 7, 3);
            else if (pattern == 2) Record(1, 7, 0x43);
            else if (pattern == 3) { Record(1, 7, 3); Record(2, 8, 3); }
            else if (pattern == 4) { Record(1, 7, 0x43); Record(2, 8, 0x102); }
            else if (pattern == 5) Record(1, 7, 2);
            else if (pattern == 6) Record(1, 7, 0x100);
            else if (pattern is >= 7 and <= 9)
            {
                // 여덟 명 모두 연결/Ready인 입력과 구독자0 stale flags를 구별한다.
                for (int owner = 1; owner < 9; owner++) Record(owner, pattern == 9 ? 0u : (uint)owner, pattern == 7 ? 0u : pattern == 8 ? 0x43u : uint.MaxValue);
            }
            byte[] before = (byte[])players.Clone(); var counts = new MissionLoginPlayerCounts(OriginalEdition.Patch1078, players);
            Assert.Equal(row[2], string.Join(',', new uint[] { counts.Subscribers(), counts.Participating(), counts.Disconnected(), counts.Ready(), counts.LoggedIn() })); Assert.Equal(before, players);
        }
    }

    /// <summary>순간 목록을 복제하지 않고 같은 현재 기록을 빌리며 Pending/Connecting을 구별한다.</summary>
    [Fact]
    public void CountsBorrowActualStorageAndKeepPendingSeparate()
    {
        byte[] players = new byte[1620]; var counts = new MissionLoginPlayerCounts(OriginalEdition.Patch1078, players); Assert.Equal(0u, counts.Subscribers());
        // 서로 다른 실제 참여 상태를 같은 현재 저장소에 공급한다.
        foreach ((int owner, uint flags) in new (int, uint)[] { (1, 2), (2, 0x102), (3, 0), (4, 3) }) { Put(players, owner * 180 + 0x2c, (uint)owner); Put(players, owner * 180 + 0x50, flags); }
        Assert.Equal(4u, counts.Subscribers()); Assert.Equal(1u, counts.Pending()); Assert.Equal(1u, counts.Connecting());
        Assert.True(MemoryMarshal.TryGetArray(counts.Record(2), out ArraySegment<byte> view)); Assert.Same(players, view.Array); Assert.Equal(360, view.Offset); Assert.Equal(180, view.Count);
        Put(players, 180 + 0x50, 0x43); Assert.Equal(0u, counts.Pending()); Assert.Equal(1u, counts.Ready()); Assert.Throws<ArgumentOutOfRangeException>(() => counts.Record(9));
        Assert.Throws<ArgumentException>(() => new MissionLoginPlayerCounts(OriginalEdition.Cd1072, players));
    }

    /// <summary>NULL 실제 연결은 미발견 응답에서만 필요하며 그 전 자기 owner/CoreData·벽시각 효과를 보존한다.</summary>
    [Fact]
    public void NullableConnectionIsRequiredOnlyAtActualRejectionSend()
    {
        var scene = new Scene([1, 0, 8, 9, 8, 0, 0, 7501]); Assert.Throws<InvalidOperationException>(() => scene.Login.ReceiveDirect(-1, null, LoginPacket(9)));
        Assert.Equal(1u, scene.Player.BattleOwner); Assert.True(scene.Core.Equals(scene.Players.AsMemory(180 + 0x6c, 10))); Assert.Equal(new[] { "Wall" }, scene.Trace);
        var node = new PacketConnection(); bool same = false; var login = new RawMissionLogin<PacketConnection>(scene.Players, scene.Participation, scene.Globals, scene.Hooks with { PendingReply = (actual, _) => { Assert.Same(node, actual); same = true; } }); login.Receive(-1, node, LoginPacket(9)); Assert.True(same);
        scene.Server = false; scene.Trace.Clear(); login.ReceiveDirect(-7, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(new[] { "Disconnect,-7" }, scene.Trace);
    }

    /// <summary>큰 metadata 하위가 누락되면 앞서 갱신한 CheckLogin flags와 전송만 남기고 월드/Begin을 만들지 않는다.</summary>
    [Fact]
    public void MissingSelectedEffectPreservesLoginPrefix()
    {
        var scene = new Scene([1, 0, 8, 1, 8, 0, 0, 7501]); var login = new RawMissionLogin<PacketConnection>(scene.Players, scene.Participation, scene.Globals, scene.Hooks with { SendFortMetadata = null });
        Assert.Throws<InvalidOperationException>(() => login.Receive(-1, new(), LoginPacket(1))); Assert.Equal(11u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Players.AsSpan(180 + 0x50)));
        Assert.Equal(0u, scene.Globals.FortCount.Value); Assert.Equal(0u, scene.Globals.Begin.Value); Assert.DoesNotContain("World", scene.Trace);
    }

    /// <summary>27h송신 observer의 두 진단 복귀 뒤 현재 필드 재조회/WORD 감김과 방향0 무읽기를 보존한다.</summary>
    [Fact]
    public void LoginObserverUsesCurrentFieldsAfterBothAssertions()
    {
        byte[] packet = new byte[24]; Array.Fill(packet, (byte)255); var lines = new List<uint>();
        RawMissionPacketFields.ObserveLogin(packet, 1, (_, file, line) => { Assert.Equal("\\Ns\\O\\_p_packets.cpp", file); lines.Add(line); if (line == 0x611) packet[12] = 3; else Put(packet, 13, 7); });
        Assert.Equal(new uint[] { 0x611, 0x612 }, lines); Assert.Equal(27, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2))); Assert.Equal(255, packet[23]);
        RawMissionPacketFields.ObserveLogin(Memory<byte>.Empty, 0); packet[12] = 254; Put(packet, 13, 65535); RawMissionPacketFields.ObserveLogin(packet, 1); Assert.Equal(270, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2)));
    }

    /// <summary>같은 공유표의 실제27h 요청이 현재 pending 노드와 원문 sender로 로그인 부모에 도달한다.</summary>
    [Fact]
    public void RegisteredLoginRunsThroughActualLocalTransport()
    {
        var scene = new Scene([1, 0, 8, 1, 8, 0, 0, 7501]);
        using var wire = new LocalMissionTransport(scene.Players, () => scene.Server, new() { Now = () => 1, Record = (_, _, _) => { }, Terminate = _ => throw new InvalidOperationException("예상 밖 종료"), Close = _ => { }, Release = _ => { } });
        bool same = false; var login = new RawMissionLogin<PacketConnection>(scene.Players, scene.Participation, scene.Globals, scene.Hooks with { PendingReply = (node, _) => { Assert.Same(wire.Server, node); same = true; } });
        MissionLoginRegistration.Register(wire.Dispatch, login); wire.Open("127.0.0.1", 6799); Assert.Equal(0, wire.Poll());
        Assert.Equal(1, wire.Dispatch.Registration(0x27).Version); Assert.Equal(65535, wire.Dispatch.Registration(0x27).Length); Assert.Equal(8, wire.Dispatch.Registration(0x63).Length);
        Assert.True(wire.Send(9, LoginPacket(9))); wire.Flush(); Assert.Equal(1, wire.Poll()); Assert.True(same); Assert.Equal(new[] { "Wall" }, scene.Trace);
    }

    /// <summary>재접속은 거부 CheckLogin 뒤에도 같은 기록/Ready 정리를 계속하고 미발견은 선택 진단 뒤 반환한다.</summary>
    [Fact]
    public void ReconnectContinuesAfterRejectedCheckLoginAndRereadsCurrentRecords()
    {
        var scene = new Scene([1, 1, 1, 1, 3, 0, 0, 7501]); byte[] packet = [1, 0x63, 8, 0, 1, 0, 0, 0]; bool cleaned = false;
        var login = new RawMissionLogin<PacketConnection>(scene.Players, scene.Participation, scene.Globals, scene.Hooks with
        {
            Log = text => scene.Trace.Add(text), CleanupReconnectClients = () => { cleaned = true; Put(scene.Players, 360 + 0x2c, 2); Put(scene.Players, 360 + 0x50, 0x102); },
        });
        login.ReceiveReconnect(-2, packet); Assert.True(cleaned); Assert.Equal(0x5bu, BinaryPrimitives.ReadUInt32LittleEndian(scene.Players.AsSpan(180 + 0x50))); Assert.Equal(1u, scene.Globals.Begin.Value);
        Assert.Contains("Disconnect,-2", scene.Trace); Assert.Contains(scene.Trace, item => item.StartsWith("S,-2,0128")); Assert.DoesNotContain("World", scene.Trace);
        Put(packet, 4, 99); scene.Trace.Clear(); login = new(scene.Players, scene.Participation, scene.Globals, scene.Hooks with { Log = scene.Trace.Add, CheckingAssertions = () => true }); login.ReceiveReconnect(-3, packet); Assert.Equal(new[] { "Reconnecting -3\n", "A,30" }, scene.Trace);
    }
}
