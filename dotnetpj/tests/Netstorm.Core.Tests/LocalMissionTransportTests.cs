using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 실제 양끝/현재표/버퍼·종료 통지가 원문 호출 순서대로 동작하는지 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>실제Connect/accept·재배치·buffered집합·이전Source·반납을검증한다.</summary>
public sealed class LocalMissionTransportTests
{
    /// <summary>시계/기록/실제close/free를관측하고현재Player기억장치를제공한다.</summary>
    private sealed class Devices
    {
        /// <summary>같은실제Player9 원문이다.</summary>
        public byte[] Players { get; } = new byte[1620];
        /// <summary>독립시계의현재DWORD다.</summary>
        public uint Now { get; set; } = 100;
        /// <summary>실제외부장치호출의관측순서다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>현재외부시계/기록/소멸장치의값사본을제공한다.</summary>
        public PacketDispatchHooks Hooks() => new()
        {
            Now = () => ++Now, Record = (node, bytes, length) => Trace.Add($"R:{node.Sender}:{bytes.Span[1]}:{length}"),
            Terminate = node => { Trace.Add("terminate"); node.Flags |= 0x40; },
            Close = node => { Assert.Equal(0u, node.Depth); Assert.NotEqual(0u, node.Flags & 0x40); Trace.Add("close:" + node.Sender); },
            Release = node => Trace.Add("free:" + node.Sender), Players = () => Players,
        };
        /// <summary>같은실제자료와선택공유표를쓰는새내부운반이다.아직열지않는다.</summary>
        public LocalMissionTransport Wire(RawConnectionRegistry? registry = null, PacketRegistrationTable? table = null) => new(Players, () => true, Hooks(), registry, table);
    }
    /// <summary>독립입력의완성된24h/version1/5바이트패킷이다.</summary>
    private static byte[] Message(byte value = 77) => [1, 0x24, 5, 0, value];
    /// <summary>로그인은선택하지않고실제Poll accept뒤현재서버끝을owner1로옮긴다.</summary>
    private static void OpenAssigned(LocalMissionTransport wire)
    {
        wire.RegisterLogin((_, _, _) => throw new InvalidOperationException("이검사에없는로그인")); wire.Open("127.0.0.1", 6799);
        Assert.Equal(0, wire.Poll()); wire.AssignConnection(-1, 1);
    }
    /// <summary>Open/Connect와Poll accept의서로다른실제생성/두시계읽기/주소를검증한다.</summary>
    [Fact]
    public void OpenConnectAndPollAcceptCreateDistinctActualNodesInOrder()
    {
        var devices = new Devices(); using LocalMissionTransport wire = devices.Wire();
        Assert.Throws<InvalidOperationException>(() => wire.Open("127.0.0.1", 6799)); wire.RegisterLogin((_, _, _) => { }); wire.Open("127.0.0.1", 6799, false);
        Assert.False(wire.HasClientConnection()); Assert.False(wire.HasPending()); Assert.Equal(0u, wire.Registry.Cleanup().Transceivers[9]);
        Assert.Throws<InvalidOperationException>(wire.AcceptPending); wire.Connect(); Assert.True(wire.HasClientConnection()); Assert.False(wire.HasPending());
        Assert.Equal(101u, wire.Client.Received); Assert.Equal(102u, wire.Client.Heartbeat); Assert.Equal(0x26u, wire.Client.Flags); Assert.Equal(0u, wire.Registry.Cleanup().Pending[1]);
        Assert.Equal(0, wire.Poll()); Assert.True(wire.HasPending()); Assert.Equal(0xffffffffu, wire.Server.Sender); Assert.Equal(104u, wire.Server.Heartbeat); Assert.Equal(105u, wire.Server.Received); Assert.Equal(106u, wire.Client.Received);
        Assert.Same(wire.Client, wire.Connection(9)); Assert.Same(wire.Server, wire.Connection(-1)); Assert.NotSame(wire.Client, wire.Server);
        Assert.Equal(20, wire.Client.Address[0]); Assert.Equal(6799u, (uint)(wire.Client.Address[2] << 8 | wire.Client.Address[3])); Assert.Equal(0x10du, BinaryPrimitives.ReadUInt32LittleEndian(wire.Client.Address.AsSpan(16)));
        wire.CloseAll(); Assert.False(wire.HasClientConnection()); Assert.False(wire.HasPending()); Assert.Equal(4, devices.Trace.Count);
    }
    /// <summary>실제로그인이pending노드안에서재배치하고같은서버의buffered응답을Client9가받는다.</summary>
    [Fact]
    public void LoginReassignsCurrentPendingAndReplyUsesActualClientNine()
    {
        var devices = new Devices(); devices.Players[180 + 0x50] = 1; using LocalMissionTransport wire = devices.Wire(); var received = new List<uint>();
        wire.RegisterLogin((sender, node, bytes) =>
        {
            Assert.Equal(uint.MaxValue, sender); Assert.Same(wire.Server, node); Assert.Equal(1u, node.Depth);
            var login = LocalMissionPackets.ReadLocalMissionLogin(bytes); Assert.Equal("Alice", login.Handle); Assert.Equal(new byte[] { 1, 2, 3 }, login.FortImage.ToArray());
            Assert.True(System.Runtime.InteropServices.MemoryMarshal.TryGetArray(bytes, out ArraySegment<byte> frame)); Assert.True(System.Runtime.InteropServices.MemoryMarshal.TryGetArray(login.FortImage, out ArraySegment<byte> image)); Assert.Same(frame.Array, image.Array); Assert.Equal(frame.Offset + 17 + bytes.Span[12], image.Offset);
            wire.AssignConnection(-1, 1); Assert.True(wire.Send(1, Message(88))); Assert.Equal(0, wire.QueuedFrames()); wire.Flush();
        });
        wire.Dispatch.Version(0x24, 1, 0); wire.Dispatch.Register(0x24, (sender, node, bytes) => { Assert.Same(wire.Client, node); Assert.Equal(88, bytes.Span[4]); Assert.Equal(1u, node.Depth); received.Add(sender); });
        wire.Open("127.0.0.1", 6799); var packet = LocalMissionPackets.MakeLocalMissionLogin(1, "Alice", new byte[] { 1, 2, 3 }); Assert.True(wire.Send(9, packet.BytesMemory())); Assert.Equal(0, wire.QueuedFrames()); wire.Flush();
        Assert.Equal(2, wire.Poll()); Assert.Equal(new uint[] { 9 }, received); Assert.Equal(1u, wire.Owner()); Assert.Equal(0u, wire.Registry.Cleanup().Pending[1]); Assert.Equal(0u, wire.Registry.Cleanup().Sync.PendingCount);
        Assert.Equal(2, devices.Trace.Count); Assert.Equal(0u, wire.Client.Depth | wire.Server.Depth); wire.CloseAll();
    }
    /// <summary>기존pending은보존하고첫다음빈칸을사용하며서버owner송신은현재Playerflag1을읽는다.</summary>
    [Fact]
    public void SharedPendingSlotsAndPlayerGateUseCurrentRegistryAndPlayers()
    {
        var devices = new Devices(); var registry = new RawConnectionRegistry(); uint older = registry.Track(new(), () => { }, () => { }); Assert.Equal(-1, registry.Accept(older));
        using LocalMissionTransport wire = devices.Wire(registry); wire.RegisterLogin((_, _, _) => { }); wire.Open("127.0.0.1", 6799); Assert.Equal(0, wire.Poll());
        Assert.Same(wire.Server, wire.Connection(-2)); Assert.Equal(0xfffffffeu, wire.Server.Sender); Assert.Throws<InvalidOperationException>(() => wire.AssignConnection(-1, 3)); wire.AssignConnection(-2, 3);
        Assert.Equal(1u, registry.Cleanup().Sync.PendingCount); Assert.Equal(older, registry.Cleanup().Pending[1]); Assert.False(wire.Send(3, Message())); devices.Players[3 * 180 + 0x50] = 1;
        int received = 0; wire.Dispatch.Version(0x24, 1, 0); wire.Dispatch.Register(0x24, (sender, _, _) => { Assert.Equal(9u, sender); received++; }); Assert.True(wire.Send(3, Message())); wire.Flush(); Assert.Equal(1, wire.Poll()); Assert.Equal(1, received);
        wire.CloseAll(); Assert.True(registry.Owns(older)); registry.Disconnect(-1);
    }
    /// <summary>수신중서버종료는깊이복귀까지실제free를미루고Client의반대끝종료는한번만통지한다.</summary>
    [Fact]
    public void CallbackCloseDefersFreeAndPeerEndIsNotRepeated()
    {
        var devices = new Devices(); using LocalMissionTransport wire = devices.Wire(); OpenAssigned(wire); uint server = wire.Registry.TokenFor(wire.Server); int ended = 0;
        wire.Dispatch.Version(0x24, 1, 0); wire.Dispatch.Register(0x24, (_, node, _) =>
        {
            Assert.Equal(1u, node.Depth); wire.Registry.Disconnect(1); Assert.True(wire.Registry.Owns(server)); Assert.DoesNotContain(devices.Trace, value => value.StartsWith("close:"));
        });
        wire.Dispatch.Register(2, (sender, node, _) => { Assert.Equal(9u, sender); Assert.Same(wire.Client, node); ended++; }); Assert.True(wire.Send(9, Message())); wire.Flush(); Assert.Equal(1, wire.Poll());
        Assert.False(wire.Registry.Owns(server)); Assert.Equal(0u, wire.Owner()); Assert.Equal(1, ended); Assert.Equal(0, wire.Poll()); Assert.Equal(1, ended); Assert.True(wire.HasClientConnection()); Assert.False(wire.ConnectionOpen(9)); wire.CloseAll();
    }
    /// <summary>World장치인계는같은노드/버퍼/등록을유지하고실제기록/시계만현재값으로쓴다.</summary>
    [Fact]
    public void RebindKeepsCurrentNodesAndQueuedBytesWhileChangingActualDevices()
    {
        var oldDevices = new Devices(); using LocalMissionTransport wire = oldDevices.Wire(); OpenAssigned(wire); PacketConnection server = wire.Server; uint token = wire.Registry.TokenFor(server);
        var next = new Devices { Now = 500 }; int received = 0; wire.Dispatch.Version(0x24, 1, 0); wire.Dispatch.Register(0x24, (_, node, _) => { Assert.Same(server, node); received++; });
        Assert.True(wire.Send(9, Message())); wire.RebindHooks(next.Hooks()); wire.Flush(); Assert.Equal(1, wire.Poll()); Assert.Equal(1, received);
        Assert.Equal(501u, server.Received); Assert.Equal(token, wire.Registry.TokenFor(server)); Assert.Empty(oldDevices.Trace); Assert.Single(next.Trace); wire.CloseAll(); Assert.Equal(5, next.Trace.Count);
    }
    /// <summary>새Source가전역표를빌려도이전Source의실제flush/수신장치를직접선택할수있다.</summary>
    [Fact]
    public void SharedCoordinatorCanFlushAndReceivePreviousActualSource()
    {
        var devices = new Devices(); var registry = new RawConnectionRegistry(); var table = new PacketRegistrationTable(OriginalEdition.Patch1078);
        using LocalMissionTransport older = devices.Wire(registry, table); OpenAssigned(older); uint client = registry.TokenFor(older.Client), server = registry.TokenFor(older.Server);
        using LocalMissionTransport newer = devices.Wire(registry, table); int received = 0, ended = 0;
        newer.Dispatch.Version(0x24, 1, 0); newer.Dispatch.Register(0x24, (sender, node, _) => { Assert.Same(older.Server, node); Assert.Equal(1u, sender); received++; });
        newer.Dispatch.Register(2, (_, node, _) => { Assert.Same(older.Client, node); ended++; }); Assert.True(older.Send(9, Message()));
        registry.FlushDirect(client); Assert.Equal(new(true, true), registry.ReceiveDirect(server)); Assert.Equal(1, received);
        registry.EndPlayers(1); Assert.Equal(default, registry.ReceiveDirect(client)); Assert.Equal(1, ended); Assert.False(newer.OwnsConnection(older.Client)); older.CloseAll();
    }
    /// <summary>mutable 관찰1로확정한현재WORD만송신하고수신관찰0은같은집합버퍼를빌린다.</summary>
    [Fact]
    public void MutableObserverSelectsCurrentLengthAndSharesActualReceiveBuffer()
    {
        var devices = new Devices(); using LocalMissionTransport wire = devices.Wire(); OpenAssigned(wire); var directions = new List<uint>(); int received = 0;
        wire.Dispatch.Version(0x24, 1, 0); wire.Dispatch.Register(0x24, (_, _, bytes) => { Assert.Equal(5, bytes.Length); Assert.Equal(99, bytes.Span[4]); received++; });
        wire.Dispatch.ObservePacket(0x24, (packet, direction) => { directions.Add(direction); if (direction == 1) { BinaryPrimitives.WriteUInt16LittleEndian(packet.Span[2..], 5); packet.Span[4] = 99; } });
        byte[] storage = new byte[16]; storage[0] = 1; storage[1] = 0x24; storage[2] = 16; Assert.True(wire.SendMutable(9, storage)); Assert.Equal(0, wire.QueuedFrames()); wire.Flush(); Assert.Equal(1, wire.Poll());
        Assert.Equal(new uint[] { 1, 0 }, directions); Assert.Equal(1, received); Assert.False(wire.SendMutable(9, new byte[] { 1, 0x7f, 4, 0 })); Assert.Equal(2, directions.Count); wire.CloseAll();
    }
}
