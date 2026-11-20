using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 내부 전송과 공유 Registry로 Source 수명/이전 노드/단일ID 순회를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>수신 콜백과 Session 교체 중에도 실제 장치와 마지막 쓰기를 보존한다.</summary>
public sealed class CompositeMissionTransportTests
{
    /// <summary>실제 장기 Client 입력과 외부 장치 관찰이다.</summary>
    private sealed class Devices
    {
        /// <summary>같은 실제 Player9 저장소다.</summary>
        public byte[] Players { get; } = new byte[1620];
        /// <summary>현재 실제 서버 권한이다.</summary>
        public bool Server { get; set; } = true;
        /// <summary>같은 장기 Registry다.</summary>
        public RawConnectionRegistry Registry { get; } = new();
        /// <summary>종료/반납의 실제 같은 노드를 관찰한다.</summary>
        public List<(string Kind, PacketConnection Node)> Effects { get; } = [];
        /// <summary>외부 시계 입력이며 매 조회를 구별한다.</summary>
        private uint _now = 100;
        /// <summary>실제 전송이 호출하는 장치를 제공한다.</summary>
        public PacketDispatchHooks Hooks() => new()
        {
            Now = () => ++_now, Record = (_, _, _) => { }, Terminate = node => Effects.Add(("terminate", node)), Close = node => Effects.Add(("close", node)), Release = node => Effects.Add(("release", node)), Players = () => Players,
        };
        /// <summary>현재 실제 Client 물리를 유지하는 내부 Source를 아직 열지 않고 생성한다.</summary>
        public CompositeMissionTransport Source(PacketRegistrationTable? table = null) => new(CompositeTransportRole.InternalMission, Players, () => Server, Hooks(), Registry, this, table);
        /// <summary>같은 Registry의 실제 Source 경로를 노드 생성 전에 바인딩한다.</summary>
        public ClientPacketSources Sources() { var sources = new ClientPacketSources(); sources.BindRegistry(Registry); return sources; }
    }
    /// <summary>실제 송신/수신을 구별할 독립 메시지 입력이다.</summary>
    private static byte[] Message(byte value = 77) => [1, 0x24, 5, 0, value];
    /// <summary>이 검사에서 선택되지 않는 로그인도 실제 실패 수신기로 등록한다.</summary>
    private static void NoLogin(CompositeMissionTransport source) => source.InternalTransport.RegisterLogin((_, _, _) => throw new InvalidOperationException("이 입력에 없는 로그인"));
    /// <summary>실제 내부 connect/accept/owner 배치를 선택한다.</summary>
    private static void Assigned(CompositeMissionTransport source)
    {
        NoLogin(source); source.Serve(6799); source.Connect(new byte[20], 6799); Assert.Equal(0, source.Poll()); source.Assign(-1, 1);
    }

    /// <summary>실제 pending 로그인→재배치→같은 Client9 응답과 표/Player 보존을 검증한다.</summary>
    [Fact]
    public void ActualLoginAndReplyUseOneRegistryTableAndPendingNode()
    {
        var devices = new Devices(); devices.Players[180 + 0x50] = 1; byte[] before = devices.Players.ToArray(); ClientPacketSources sources = devices.Sources(); using var source = devices.Source(); sources.Publish(source);
        int login = 0, reply = 0, observed = 0;
        source.InternalTransport.RegisterLogin((sender, node, packet) =>
        {
            Assert.Equal(uint.MaxValue, sender); Assert.Same(source.InternalTransport.Server, node); Assert.Equal(1u, node.Depth); using NativeBattleSource? selected = sources.Resolve(node); Assert.NotNull(selected); Assert.Same(source, selected.Device);
            Assert.Equal("Alice", LocalMissionPackets.ReadLocalMissionLogin(packet).Handle); login++; source.Assign(-1, 1); Assert.True(source.SendMutable(1, Message(88))); source.Flush();
        });
        source.Dispatch.Version(0x24, 1, 0); source.Dispatch.ObservePacket(0x24, (_, sending) => { if (sending != 0) observed++; });
        source.Dispatch.Register(0x24, (sender, node, packet) => { Assert.Equal(9u, sender); Assert.Same(source.InternalTransport.Client, node); Assert.Equal((byte)88, packet.Span[4]); reply++; });
        source.Serve(6799); Assert.False(source.Exists(9)); Assert.Equal(0u, devices.Registry.Cleanup().Pending[1]); source.Serve(9999); Assert.Equal(6799u, source.InternalTransport.Port()); source.Connect(new byte[20], 9999);
        var request = LocalMissionPackets.MakeLocalMissionLogin(1, "Alice", new byte[] { 1, 2, 3 }); Assert.True(source.Send(9, request.BytesMemory())); source.Flush(); Assert.Equal(2, source.Poll());
        Assert.Equal(1, login); Assert.Equal(1, reply); Assert.Equal(1, observed); Assert.Equal(1u, source.InternalTransport.Owner()); Assert.Same(source.Dispatch.Registrations, source.InternalTransport.Dispatch.Registrations); Assert.Equal(before, devices.Players);
        source.CloseRoom(); Assert.DoesNotContain(devices.Registry.Cleanup().Transceivers, token => token != 0);
    }

    /// <summary>외부 owner 반납 뒤에도 등록 노드/배송 대여가 실제 장치를 유지하고 마지막에만 반납한다.</summary>
    [Fact]
    public void RegisteredNodesAndDeliveryKeepActualSourceUntilLastRelease()
    {
        var devices = new Devices(); ClientPacketSources sources = devices.Sources(); var source = devices.Source(); sources.Publish(source); Assigned(source);
        NativeBattleSource delivery = Assert.IsType<NativeBattleSource>(sources.Resolve(source.InternalTransport.Client)); var weak = new WeakNativeBattleSource(delivery); delivery.Dispose(); source.Dispose(); Assert.False(source.Released);
        NativeBattleSource restored = Assert.IsType<NativeBattleSource>(weak.Lock()); Assert.Same(source, restored.Device); Assert.Null(sources.Resolve(new PacketConnection { Sender = 9 }));
        source.CloseRoom(); Assert.False(source.Released); restored.Dispose(); Assert.True(source.Released); Assert.Null(weak.Lock()); Assert.Null(sources.Resolve(source.InternalTransport.Client));
    }

    /// <summary>새 미션의 Poll이 이전 Source server1의 실제 type2 종료를 소비하여 재사용 칸을 반납한다.</summary>
    [Fact]
    public void NewSourcePollConsumesOldActualServerEndAndCloseRoomKeepsOtherOwners()
    {
        var devices = new Devices(); ClientPacketSources sources = devices.Sources(); using var old = devices.Source(); sources.Publish(old); Assigned(old); PacketConnection oldServer = old.InternalTransport.Server; old.Close(9); Assert.Same(oldServer, old.Connection(1));
        using var current = devices.Source(old.Dispatch.Registrations); sources.Publish(current); NoLogin(current); current.Serve(6799); current.Connect(new byte[20], 6799); int ended = 0; current.Dispatch.Register(2, (_, node, _) => { Assert.Same(oldServer, node); ended++; old.Close(1); });
        Assert.Throws<InvalidOperationException>(() => current.Connection(1)); Assert.Throws<InvalidOperationException>(() => current.Send(1, Message())); Assert.Equal(0, current.Poll()); Assert.Equal(1, ended);
        Assert.Equal(0u, devices.Registry.Cleanup().Transceivers[1]); Assert.Contains(devices.Effects, effect => effect.Kind == "release" && ReferenceEquals(effect.Node, oldServer)); Assert.Same(current.InternalTransport.Server, current.Connection(-1));
        old.CloseRoom(); Assert.True(current.IsOpen(9)); Assert.Same(current.InternalTransport.Server, current.Connection(-1)); current.CloseRoom();
    }

    /// <summary>외부 실제 가상 장치도 같은 pending각1회와 현재 정상ID 재선택 순서를 사용한다.</summary>
    [Fact]
    public void GlobalIdPassUsesActualRegisteredCallbacksAndRereadsAfterReplacement()
    {
        var devices = new Devices(); using var source = devices.Source(); var trace = new List<string>();
        var pendingNode = new PacketConnection(); uint pending = devices.Registry.Track(pendingNode, () => { }, () => { }, receive: () => { trace.Add("pending"); return new(true, true); }); Assert.Equal(-1, devices.Registry.Accept(pending));
        var replacementNode = new PacketConnection(); uint replacement = devices.Registry.Track(replacementNode, () => { }, () => { }, flush: () => trace.Add("new-flush"), receive: () => { trace.Add("new-receive"); return new(true, false); });
        var initialNode = new PacketConnection(); uint initial = devices.Registry.Track(initialNode, () => { }, () => { }, flush: () => { trace.Add("old-flush"); devices.Registry.PublishConnect(0, replacement); }, receive: () => { trace.Add("old-receive"); devices.Registry.PublishConnect(0, replacement); return new(true, true); });
        devices.Registry.Register(0, initial); source.Flush(); Assert.Equal(new[] { "old-flush" }, trace); trace.Clear(); devices.Registry.PublishConnect(0, initial);
        Assert.Equal(3, source.Poll()); Assert.Equal(new[] { "pending", "old-receive", "new-receive" }, trace); source.CloseRoom(); Assert.Equal(replacement, devices.Registry.Cleanup().Transceivers[0]); Assert.Equal(pending, devices.Registry.Cleanup().Pending[1]);
        devices.Registry.CloseAll(); devices.Registry.ShutdownTrackedDevices();
    }

    /// <summary>실제 수신 callback 안에서 외부/노드가 반납돼도 Poll의 끝까지 장치를 유지한다.</summary>
    [Fact]
    public void ReleaseInsideActualReceiveWaitsForPollEndAndResetsGuards()
    {
        var devices = new Devices(); devices.Players[180 + 0x50] = 1; ClientPacketSources sources = devices.Sources(); var source = devices.Source(); sources.Publish(source); Assigned(source); using PacketRegistrationLease lease = source.BindSession(devices); bool received = false;
        source.Dispatch.Version(0x24, 1, 0); source.Dispatch.Register(0x24, (_, node, _) =>
        {
            Assert.True(source.ConnectionCallbackActive()); Assert.Equal(1u, node.Depth); Assert.Throws<InvalidOperationException>(() => source.Poll()); source.Dispose(); source.CloseRoom(); Assert.False(source.Released); received = true;
        });
        Assert.True(source.Send(9, Message())); source.Flush(); _ = source.Poll(); Assert.True(received); Assert.True(source.Released); Assert.Equal(0u, source.InternalTransport.Client.Depth | source.InternalTransport.Server.Depth); Assert.False(source.ConnectionCallbackActive());
    }

    /// <summary>App 최종 정리는 실제 Source를 pin한 채 노드→listener 순서로 닫고 약한 경로를 비운다.</summary>
    [Fact]
    public void ShutdownRetainsSourceAcrossNodeAndListenerCleanup()
    {
        var devices = new Devices(); ClientPacketSources sources = devices.Sources(); var source = devices.Source(); sources.Publish(source); Assigned(source); source.Dispose(); Assert.False(source.Released); bool closed = false;
        sources.Shutdown(() => { devices.Registry.CloseAll(); Assert.False(source.Released); closed = true; }); Assert.True(closed); Assert.True(source.Released); Assert.Null(sources.Resolve(source.InternalTransport.Client));
    }

    /// <summary>실제 수신 실패 뒤 coordinator와 backend guard가 모두 회수돼 다음Poll을 허용한다.</summary>
    [Fact]
    public void ReceiveFailurePreservesExceptionAndAllowsNextPoll()
    {
        var devices = new Devices(); ClientPacketSources sources = devices.Sources(); using var source = devices.Source(); sources.Publish(source); Assigned(source); var failure = new InvalidOperationException("명시 수신 실패");
        source.Dispatch.Version(0x24, 1, 0); source.Dispatch.Register(0x24, (_, _, _) => throw failure); Assert.True(source.Send(9, Message())); source.Flush(); Assert.Same(failure, Assert.Throws<InvalidOperationException>(() => source.Poll())); Assert.Equal(0, source.Poll()); source.CloseRoom();
    }

    /// <summary>미이식 실제 backend를 내부 연결 성공으로 대신하지 않는다.</summary>
    [Theory]
    [InlineData(CompositeTransportRole.Editor)]
    [InlineData(CompositeTransportRole.LanHost)]
    [InlineData(CompositeTransportRole.LanClient)]
    public void UnportedBackendIsExplicitAndDoesNotTouchRegistry(CompositeTransportRole role)
    {
        var devices = new Devices(); Assert.Throws<NotSupportedException>(() => new CompositeMissionTransport(role, devices.Players, () => devices.Server, devices.Hooks(), devices.Registry, devices)); Assert.All(devices.Registry.Cleanup().Transceivers, token => Assert.Equal(0u, token)); Assert.Empty(devices.Effects);
    }
    /// <summary>이전 실제 장치의 수신 예외를 보존하고 두 Poll guard를 회수해 다음 진입을 허용한다.</summary>
    [Fact]
    public void ForeignReceiveFailureIsPreservedAndBothPollGuardsReset()
    {
        var devices = new Devices(); using var source = devices.Source(); bool fail = true; var expected = new InvalidOperationException("실제 이전 수신 실패");
        uint token = devices.Registry.Track(new PacketConnection(), () => { }, () => { }, receive: () => { if (fail) { fail = false; throw expected; } return default; }); devices.Registry.Register(12, token);
        Assert.Same(expected, Assert.Throws<InvalidOperationException>(() => source.Poll())); Assert.Equal(0, source.Poll()); Assert.Equal(token, devices.Registry.Cleanup().Transceivers[12]); devices.Registry.CloseAll();
    }

}
