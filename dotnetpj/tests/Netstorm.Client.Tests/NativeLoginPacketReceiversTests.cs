using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Net;

// 원문고정App등록과현재실제Session/Player/활동표의배송을검사한다.
namespace Netstorm.Client.Tests;

/// <summary>고정F/metadata/현재모듈/전체Runtime호출수명을실제장치로대조한다.</summary>
public sealed class NativeLoginPacketReceiversTests
{
    /// <summary>원문15행을기대값으로독립기록한다.</summary>
    private static readonly (uint Type, uint Entry, ushort Length, bool Mutable)[] Rows =
    [
        (0x27,0x004a7800,0xffff,true),(0x63,0x004a7510,8,false),(0x29,0x004b3b40,34,false),
        (0x67,0x004b4900,0xffff,true),(0x2b,0x004912d0,0xffff,true),(0x2a,0x004b3d30,0xffff,true),
        (0x28,0x004b8940,18,false),(0x81,0x004b3e90,0xffff,true),(0x84,0x004a6f30,8,false),
        (0x6e,0x004a7050,8,false),(0x72,0x004b3bc0,0xffff,true),(0x3d,0x00491220,9,false),
        (0x78,0x0048f880,6,false),(0x52,0x004a71a0,5,false),(0x64,0x004b6180,5,false),
    ];
    /// <summary>원문DWORD를정확히읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[at..]);
    /// <summary>원문DWORD를정확히쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>필수분배장치만제공하고실제등록표를사용한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
    /// <summary>기본현재활동효과를제공한다.</summary>
    private static ConnectionActivityHooks Activity(Action<uint, uint>? erase = null) => new() { WallSeconds = () => 12.5, FrameNumber = () => 37, Server = () => true, EraseStatus = erase ?? ((_, _) => { }) };
    /// <summary>같은실제전역으로고정F를설치한다.</summary>
    private static ClientConnectionGlobals Install(ClientGameplayGlobals game, RawPacketDispatch dispatch, ConnectionActivityHooks? activity = null, Action<string, string, uint>? assertion = null)
    {
        var connections = new ClientConnectionGlobals(new(game, new())); NativeLoginPacketReceivers.Install(game, connections, dispatch, assertion ?? ((_, _, _) => { }), activity ?? Activity()); return connections;
    }
    /// <summary>같은Player/옵션숨김/동맹을실제참여본문에연결한다.</summary>
    private static RawPlayerParticipation Participation(HostSession session) => new(OriginalEdition.Patch1078, session.PlayerRecords, session.Alliances, session.HiddenPlayers,
        new() { Battle = false }, new() { ClearChat = _ => { }, AddChat = (_, _, _) => { } });
    /// <summary>같은실제Player/현재상태/옵션을빌리는큰로그인본문을만든다.</summary>
    private static HostLoginPacketModules Modules(HostSession session, RawPlayerParticipation participation, MissionClientPacketState state, MissionClientPacketHooks hooks, Action<uint, uint, uint>? status = null) => new()
    {
        Server = new(session.PlayerRecords, participation, new() { Player = session.MissionPlayer, ZoneAddress = new byte[6], CachedCoreData = new byte[10], BattleOptions = session.BattleOptions }, new()),
        Client = new(session.PlayerRecords, participation, new(new GameRandom(42), () => 9), state, hooks),
        Metadata = new(session.PlayerRecords, new() { ZoneAddress = new byte[6] }, new()), Status = status,
    };
    /// <summary>동일현재Player와87바이트옵션의정확한부분뷰를공급한다.</summary>
    private static MissionClientPacketState State(HostSession session) => new() { Player = session.MissionPlayer, CachedCoreData = new byte[10], ZoneAddress = new byte[6], BattleOptions = session.BattleOptions, Hidden = session.HiddenPlayers };
    /// <summary>실제같은Pool저장소/Session과배송중소유자를만든다.</summary>
    private static HostMissionRuntime Runtime(ClientGameplayGlobals game)
    {
        var storage = new RawSurfaceStorage(); var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), pool: storage.Pool, sharedGameplay: game);
        return HostMissionRuntime.Create(session, storage, []);
    }

    /// <summary>설치는Session/Player할당을요구하지않고기존67h와다른App행을보존한다.</summary>
    [Fact]
    public void InstallPreservesExistingAppOptionsAndExactFourteenRows()
    {
        var game = new ClientGameplayGlobals(); var dispatch = Dispatch(); dispatch.RegisterCanonicalDirect(0x67, 0x004b4900, (_, _, _) => { });
        dispatch.ObservePacket(0x67, (_, _) => { }); dispatch.Version(0x67, 1, 0); dispatch.RequireLength(0x67, 0xffff);
        CanonicalPacketReceiver options = dispatch.SnapshotReceiver(0x67)!; var observer = dispatch.Registration(0x67).PacketObserver;
        dispatch.Version(0x10, 7, 3); Install(game, dispatch); Assert.False(game.PlayerRecords.Initialized); Assert.Same(options, dispatch.SnapshotReceiver(0x67)); Assert.Same(observer, dispatch.Registration(0x67).PacketObserver);
        // 15행의실제주소/길이/버전/observer슬롯과같은F대여를전수확인한다.
        foreach (var row in Rows)
        {
            PacketRegistration actual = dispatch.Registration(row.Type); Assert.Equal(row.Entry, actual.CanonicalReceive!.NativeEntry); Assert.Equal(row.Length, actual.Length); Assert.Equal(1, actual.Version); Assert.Equal(0, actual.Mismatch);
            Assert.Equal(row.Mutable, actual.PacketObserver is not null); Assert.Equal(!row.Mutable, actual.Observer is not null);
            using var lease = dispatch.Registrations.BindSession(game); NativeLoginPacketReceivers.Bind(dispatch, row.Type); Assert.Same(actual.CanonicalReceive, dispatch.SnapshotReceiver(row.Type));
        }
        Assert.Equal(7, dispatch.Registration(0x10).Version); Assert.Equal(3, dispatch.Registration(0x10).Mismatch);
    }

    /// <summary>불일치각항목은본문을호출하거나metadata를고치지않고거부한다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)] [InlineData(5)]
    public void BindRejectsEveryMetadataOrReceiverMismatch(int mutation)
    {
        var dispatch = Dispatch(); Install(new(), dispatch); PacketRegistration row = dispatch.Registration(0x63);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(0x63, 1, (_, _, _) => { }); break;
            case 1: row.Version = 2; break;
            case 2: row.Mismatch = 1; break;
            case 3: row.Length = 9; break;
            case 4: row.Observer = null; break;
            case 5: row.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = row.CanonicalReceive; Assert.Throws<InvalidOperationException>(() => NativeLoginPacketReceivers.Bind(dispatch, 0x63)); Assert.Same(before, row.CanonicalReceive);
        Assert.Throws<ArgumentException>(() => NativeLoginPacketReceivers.Bind(dispatch, 0xfe));
    }

    /// <summary>실제mutableobserver는진단복귀뒤변경된필드를다시읽고빈observer는물리를읽지않는다.</summary>
    [Fact]
    public void ObserversKeepMutableBufferAndAssertionRereads()
    {
        var dispatch = Dispatch(); byte[] packet = new byte[40]; packet[12] = 255; Put(packet, 13, uint.MaxValue); var trace = new List<uint>();
        Install(new(), dispatch, assertion: (_, file, line) => { Assert.Equal(@"\Ns\O\_p_packets.cpp", file); trace.Add(line); if (line == 0x611) packet[12] = 3; else Put(packet, 13, 7); });
        dispatch.Registration(0x27).NotifyObserver(packet, 1); Assert.Equal(new uint[] { 0x611, 0x612 }, trace); Assert.Equal(27, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2)));
        dispatch.Registration(0x63).NotifyObserver(Memory<byte>.Empty, 1); dispatch.Registration(0x27).NotifyObserver(Memory<byte>.Empty, 0);
        Put(packet, 6, 15); dispatch.Registration(0x72).NotifyObserver(packet, 1); Assert.Equal(25, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2)));
    }

    /// <summary>52h/64h는Session/연결/payload없이현재장기표만읽고기존F는표교체와무관하다.</summary>
    [Fact]
    public void DirectLongOwnersDoNotRequireCurrentRuntimeOrPayload()
    {
        var game = new ClientGameplayGlobals(); var dispatch = Dispatch(); var effects = new List<(uint, uint)>(); var hooks = Activity((kind, player) => effects.Add((kind, player))); var connections = Install(game, dispatch, hooks);
        game.PlayerRecords.Initialize((_, _) => { }); Put(game.PlayerRecords.Records.Span, 3 * 180 + 0x50, 0x1230); CanonicalPacketReceiver leave = dispatch.SnapshotReceiver(0x52)!;
        dispatch.RegisterCanonicalDirect(0x52, 5, (_, _, _) => throw new IOException("현재표의다른함수")); leave.Invoke(3, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(0x1234u, Read(game.PlayerRecords.Records.Span, 3 * 180 + 0x50));
        hooks.FrameNumber = () => throw new IOException("복사뒤외부함수교체"); dispatch.InvokeDirect(0x64, 3, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(12u, connections.Activity.WallSeconds[3]); Assert.Equal(37u, connections.Activity.Frames[3]); Assert.Equal(new[] { (2u, 3u) }, effects);
    }

    /// <summary>고정F는현재packet번호/등록표로다시고르지않고매배송시현재실제Login을선택한다.</summary>
    [Fact]
    public void CapturedEntryUsesCurrentActualModulesAndReturnsRuntimePin()
    {
        var game = new ClientGameplayGlobals(); var dispatch = Dispatch(); Install(game, dispatch); using var runtime = Runtime(game); HostSession session = runtime.Session; RawPlayerParticipation participation = Participation(session);
        MissionClientPacketState first = State(session); session.BindFixedLoginPackets(Modules(session, participation, first, new() { Clock = () => { Assert.True(game.RuntimeCallActive(session)); Assert.Throws<InvalidOperationException>(() => session.ReleasePlayerRecordViews()); return 9500; } }));
        CanonicalPacketReceiver fixedF = dispatch.SnapshotReceiver(0x84)!; dispatch.RegisterCanonicalDirect(0x84, 7, (_, _, _) => throw new IOException("교체본문")); byte[] packet = [1, 0x52, 8, 0, 3, 0, 0, 0]; fixedF.Invoke(19, null, packet); Assert.Equal(9500u, first.ClockStamp.Value); Assert.Equal(6500u, first.BaseClock.Value); Assert.False(game.RuntimeCallActive(session));
        MissionClientPacketState second = State(session); session.BindFixedLoginPackets(Modules(session, participation, second, new() { Clock = () => 10000 })); fixedF.Invoke(19, null, packet); Assert.Equal(7000u, second.BaseClock.Value); Assert.Equal(6500u, first.BaseClock.Value);
        session.BindFixedLoginPackets(null); Assert.Throws<InvalidOperationException>(() => fixedF.Invoke(19, null, packet)); Assert.False(game.RuntimeCallActive(session)); session.ReleasePlayerRecordViews();
    }

    /// <summary>실제큰하위실패와외곽종료요청은마지막배송복귀뒤Runtime을정리한다.</summary>
    [Fact]
    public void ActualCallbackFailureUnwindsWholeRuntimeLease()
    {
        var game = new ClientGameplayGlobals(); var dispatch = Dispatch(); Install(game, dispatch); var runtime = Runtime(game); HostSession session = runtime.Session; var participation = Participation(session); var failure = new IOException("실제시계효과실패");
        session.BindFixedLoginPackets(Modules(session, participation, State(session), new() { Clock = () => { runtime.Dispose(); Assert.False(runtime.Released); throw failure; } }));
        Assert.Same(failure, Assert.Throws<IOException>(() => dispatch.InvokeDirect(0x84, 0, null, new byte[8]))); Assert.True(runtime.Released); Assert.False(game.Borrowed()); Assert.False(game.RuntimeCallActive(session)); session.ReleasePlayerRecordViews();
    }

    /// <summary>Login없는동맹3d는같은원문부분버퍼에self bit를쓰고그물리를방송한다.</summary>
    [Fact]
    public void AllianceWithoutLoginMutatesAndBroadcastsSamePhysicalSlice()
    {
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23)); var participation = Participation(session); byte[] bytes = Enumerable.Repeat((byte)0xcc, 13).ToArray(); Memory<byte> packet = bytes.AsMemory(2, 9); packet.Span.Clear(); packet.Span[4] = 2; Put(packet.Span, 5, 8);
        bool sent = false; session.BindFixedAlliancePackets(participation, new() { Broadcast = original => { Assert.True(original.Equals((ReadOnlyMemory<byte>)packet)); Assert.Equal(12u, Read(original.Span, 5)); sent = true; } });
        session.ReceiveFixedLoginPacket(0x00491220, 99, null, packet); Assert.True(sent); Assert.Equal(12u, Read(session.PlayerRecords, 2 * 180 + 0x54)); Assert.Equal(1u, session.Alliances[2 * 9 + 3]); Assert.Equal(0xcc, bytes[1]); Assert.Equal(0xcc, bytes[11]); session.ReleasePlayerRecordViews();
    }

    /// <summary>78h는최초Login상태효과를보유하고양방향쓰기/방송뒤같은선택에알린다.</summary>
    [Fact]
    public void AllianceKeepsSelectedLoginAcrossCurrentReplacement()
    {
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23)); var participation = Participation(session); var trace = new List<string>(); byte[] packet = [1, 0x84, 6, 0, 2, 3];
        session.BindFixedLoginPackets(Modules(session, participation, State(session), new(), (kind, owner, target) => { Assert.Equal((7u, 2u, 3u), (kind, owner, target)); trace.Add("login"); }));
        session.BindFixedAlliancePackets(participation, new() { Boss = () => { session.BindFixedLoginPackets(null); return true; }, Broadcast = memory => { Assert.True(memory.Equals((ReadOnlyMemory<byte>)packet)); Assert.Equal(1u, session.Alliances[2 * 9 + 3]); Assert.Equal(1u, session.Alliances[3 * 9 + 2]); trace.Add("broadcast"); }, Status = (_, _, _) => trace.Add("editor") });
        session.ReceiveFixedLoginPacket(0x0048f880, 0, null, packet); Assert.Equal(new[] { "broadcast", "login" }, trace); session.ReleasePlayerRecordViews();
    }

    /// <summary>미연결상태창은실제앞선양방향쓰기/방송을보존하고다른물리의연결은거부한다.</summary>
    [Fact]
    public void AllianceFailurePreservesPrefixAndBindingRejectsForeignStorage()
    {
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23)); var participation = Participation(session); bool sent = false;
        session.BindFixedAlliancePackets(participation, new() { Boss = () => true, Broadcast = _ => sent = true }); Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedLoginPacket(0x0048f880, 0, null, new byte[] { 1, 0, 6, 0, 2, 3 })); Assert.True(sent); Assert.Equal(1u, session.Alliances[2 * 9 + 3]); Assert.Equal(1u, session.Alliances[3 * 9 + 2]);
        var foreign = new RawPlayerParticipation(OriginalEdition.Patch1078, session.PlayerRecords, new uint[81], session.HiddenPlayers, new(), new()); Assert.Throws<ArgumentException>(() => session.BindFixedAlliancePackets(foreign, new()));
        Assert.Throws<ArgumentException>(() => session.BindFixedLoginPackets(Modules(session, participation, State(session) with { Player = new() }, new()))); session.ReleasePlayerRecordViews(); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedLoginPacket(0x00491220, 0, null, new byte[9]));
    }
}
