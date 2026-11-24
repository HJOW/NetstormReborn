// Client 원문의 지연 분포 선택과 실제 연결 FIFO의 enqueue/pump 순서를 대조한다.
// cpppj/src/o/RawPacketDelay.h·RawPacketDelay.cpp 의 PacketDelayGlobals/RawPacketDelayQueue/PreparePacketDelay를 대조한다.
// 0.965에서 R4-6 전송·수신(수송 팩토리의 hooks.delay 자리) 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Core.Tests;

// 지연 분포와 실제 FIFO 경계를 확인한다.
public sealed class RawPacketDelayTests
{
    /// <summary>같은 실제 분배 표를 만든다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>벽시각·난수를 묶은 같은 실제 분포 owner다.</summary>
    private static PacketDelayGlobals Globals(double wall = 1000.0, Func<uint>? random = null)
    {
        var globals = new PacketDelayGlobals();
        globals.Wall = () => wall;
        globals.Random = random ?? (() => 0u);
        return globals;
    }

    /// <summary>같은 실제 노드다.</summary>
    private static PacketConnection Connection(uint sender = 7)
    {
        var connection = new PacketConnection { Sender = sender };
        connection.Address[0] = 0xAA;
        connection.Address[19] = 0x55;
        return connection;
    }

    /// <summary>선언 길이를 읽는 실제 고정 F를 같은 표에 둔다.</summary>
    private static CanonicalPacketReceiver Receive(RawPacketDispatch dispatch, uint type, uint entry, List<(uint Sender, byte[] Packet)> received)
    {
        dispatch.RegisterCanonicalDirect(type, entry, (sender, _, packet) => received.Add((sender, packet.ToArray())));
        return dispatch.SnapshotReceiver(type)!;
    }

    /// <summary>새 Client는 기본01ff를 선택하고 최초 지도 모드 재설정이 복원한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void DefaultDistributionIs01ffAndResetRestores()
    {
        var globals = new PacketDelayGlobals();
        Assert.Equal(new sbyte[] { 1, -1 }, globals.Distribution);
        globals.SelectLag(2);
        globals.ResetDefault();
        Assert.Equal(new sbyte[] { 1, -1 }, globals.Distribution);
    }

    /// <summary>435220의1/2/3만 분포를 교체하고 누락/다른 값은 보존한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void SelectLagReplacesOnlyFor123()
    {
        var globals = new PacketDelayGlobals();
        globals.SelectLag(1);
        Assert.Equal(new sbyte[] { 0, 4, 12, 6, 2, 1, 1, 1, 1, -1 }, globals.Distribution);
        globals.SelectLag(2);
        Assert.Equal(new sbyte[] { 0, 0, 0, 0, 1, -1 }, globals.Distribution);
        globals.SelectLag(3);
        Assert.Equal(new sbyte[] { 0, 0, 0, 0, 0, 0, 0, 1, -1 }, globals.Distribution);
        globals.SelectLag(0);
        globals.SelectLag(4);
        globals.SelectLag(-1);
        Assert.Equal(new sbyte[] { 0, 0, 0, 0, 0, 0, 0, 1, -1 }, globals.Distribution);
    }

    /// <summary>같은 Client가 실제 장기 선택 분포 owner를 공유한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void ConnectionGlobalsShareOneDelayDistribution()
    {
        var local = new Netstorm.Client.ClientLocalStateGlobals(new Netstorm.Client.ClientGameplayGlobals(), new AmbientSoundState());
        var connections = new Netstorm.Client.ClientConnectionGlobals(local);
        Assert.Same(connections.PacketDelays(), connections.PacketDelays());
        Assert.Equal(new sbyte[] { 1, -1 }, connections.PacketDelays().Distribution);
    }

    /// <summary>분포/벽시각/난수 미연결은 큐 생성을 거부한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void QueueRejectsMissingDevices()
    {
        Assert.Throws<ArgumentNullException>(() => new RawPacketDelayQueue(null!));
        Assert.Throws<ArgumentException>(() => new RawPacketDelayQueue(new PacketDelayGlobals()));
        var noWall = new PacketDelayGlobals { Random = () => 0u };
        Assert.Throws<ArgumentException>(() => new RawPacketDelayQueue(noWall));
        var noRandom = new PacketDelayGlobals { Wall = () => 0.0 };
        Assert.Throws<ArgumentException>(() => new RawPacketDelayQueue(noRandom));
    }

    /// <summary>NULL receiver는 0을 반환하고 FIFO에 넣지 않는다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void EnqueueNullReceiverReturnsZero()
    {
        var dispatch = Dispatch();
        var connection = Connection();
        var queue = new RawPacketDelayQueue(Globals());
        Assert.Equal(0, queue.Enqueue(connection, null, dispatch, new byte[8] { 1, 0x24, 8, 0, 0, 0, 0, 0 }, -1));
        Assert.Equal(0, queue.Pump(connection));
    }

    /// <summary>짧은 패킷·초과 선언·0번 종류를 거부한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void EnqueueRejectsBadPackets()
    {
        var dispatch = Dispatch();
        var connection = Connection();
        var received = new List<(uint Sender, byte[] Packet)>();
        var receiver = Receive(dispatch, 0x24, 0x00444bc0, received);
        var queue = new RawPacketDelayQueue(Globals());
        Assert.Throws<ArgumentOutOfRangeException>(() => queue.Enqueue(connection, receiver, dispatch, new byte[3], 5));
        Assert.Throws<ArgumentOutOfRangeException>(() => queue.Enqueue(connection, receiver, dispatch, new byte[8] { 1, 0x24, 9, 0, 0, 0, 0, 0 }, 5));
        Assert.Throws<ArgumentOutOfRangeException>(() => queue.Enqueue(connection, receiver, dispatch, new byte[8] { 0, 0, 8, 0, 0, 0, 0, 0 }, 5));
        Assert.Empty(received);
    }

    /// <summary>observer1 실행 뒤 선언 길이만 복사하고 호출자 버퍼는 바꾸지 않는다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void EnqueueRunsObserverAndCopiesDeclaredLength()
    {
        var dispatch = Dispatch();
        dispatch.ObservePacket(0x24, (packet, sending) => { if (sending != 0) BinaryPrimitives.WriteUInt16LittleEndian(packet.Span.Slice(2, 2), 8); });
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (_, _, _) => { });
        var receiver = dispatch.SnapshotReceiver(0x24)!;
        var connection = Connection();
        var queue = new RawPacketDelayQueue(Globals());
        byte[] packet = [1, 0x24, 12, 0, 9, 9, 9, 9, 9, 9, 9, 9];
        Assert.Equal(7, queue.Enqueue(connection, receiver, dispatch, packet, 7));
        // 호출자 버퍼의 선언 길이는 그대로 둔다.
        Assert.Equal(12, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        int delivered = 0;
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (_, _, body) => { delivered++; Assert.Equal(8, body.Length); });
        double wall2 = 2000.0;
        var globals2 = Globals();
        globals2.Wall = () => wall2;
        var queue2 = new RawPacketDelayQueue(globals2);
        Assert.Equal(0, queue2.Enqueue(connection, dispatch.SnapshotReceiver(0x24)!, dispatch, packet, 0));
        wall2 = 2001.0;
        Assert.Equal(1, queue2.Pump(connection));
        Assert.Equal(1, delivered);
    }

    /// <summary>고정 지연은 그대로 쓰고 -1은 난수·bucket으로 정한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void EnqueueSelectsFixedAndDerivedDelay()
    {
        var dispatch = Dispatch();
        var received = new List<(uint Sender, byte[] Packet)>();
        var receiver = Receive(dispatch, 0x24, 0x00444bc0, received);
        var connection = Connection();
        var rolls = new Queue<uint>([5u, 37u, 99u]);
        var globals = Globals(random: () => rolls.Dequeue());
        globals.SelectLag(1);
        var queue = new RawPacketDelayQueue(globals);
        byte[] packet = [1, 0x24, 8, 0, 1, 2, 3, 4];
        // draw 5: bucket2, delay 37 + 200.
        Assert.Equal(237, queue.Enqueue(connection, receiver, dispatch, packet, -1));
        Assert.Equal(237, queue.Enqueue(connection, receiver, dispatch, packet, 237));
    }

    /// <summary>선두 기한 순서로 배송하고 주소/깊이를 복구한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void PumpDeliversInOrderWithDeadlineGate()
    {
        var dispatch = Dispatch();
        var order = new List<uint>();
        var seen = new List<byte[]>();
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (sender, node, _) => { order.Add(sender); seen.Add((byte[])node!.Address.Clone()); });
        var receiver = dispatch.SnapshotReceiver(0x24)!;
        double wall = 1000.0;
        var globals = Globals();
        globals.Wall = () => wall;
        var queue = new RawPacketDelayQueue(globals);
        var connection = Connection();
        byte[] first = [1, 0x24, 5, 0, 11];
        byte[] second = [1, 0x24, 5, 0, 22];
        connection.Sender = 11;
        Assert.Equal(10, queue.Enqueue(connection, receiver, dispatch, first, 10));
        connection.Sender = 22;
        Assert.Equal(20, queue.Enqueue(connection, receiver, dispatch, second, 20));
        // 둘 다 기한 전이면 0을 돌려준다.
        Assert.Equal(0, queue.Pump(connection));
        Assert.Empty(order);
        // 첫째 기한만 지나면 하나만 배송한다.
        wall = 1000.015;
        Assert.Equal(1, queue.Pump(connection));
        Assert.Equal([11u], order);
        Assert.Equal(0xAA, (int)seen[0][0]);
        Assert.Equal(0u, connection.Depth);
        Assert.Equal(0xAA, (int)connection.Address[0]);
        Assert.Equal(0x55, (int)connection.Address[19]);
        wall = 2000.0;
        Assert.Equal(1, queue.Pump(connection));
        Assert.Equal([11u, 22u], order);
        Assert.Equal(0, queue.Pump(connection));
    }

    /// <summary>닫힘 표시를 만나면 -1로 멈춘다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void PumpStopsWithMinusOneOnCloseFlag()
    {
        var dispatch = Dispatch();
        int calls = 0;
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (_, node, _) => { if (calls++ == 0) node!.Flags |= 0x40u; });
        var receiver = dispatch.SnapshotReceiver(0x24)!;
        double wall = 4000.0;
        var globals = Globals();
        globals.Wall = () => wall;
        var queue = new RawPacketDelayQueue(globals);
        var connection = Connection();
        byte[] packet = [1, 0x24, 5, 0, 1];
        Assert.Equal(0, queue.Enqueue(connection, receiver, dispatch, packet, 0));
        Assert.Equal(0, queue.Enqueue(connection, receiver, dispatch, packet, 0));
        wall = 5000.0;
        Assert.Equal(-1, queue.Pump(connection));
        // 닫힘 표시는 연결에 남으므로 다음 배송 전에 지운다.
        connection.Flags &= ~0x40u;
        Assert.Equal(1, queue.Pump(connection));
    }

    /// <summary>수신 실패는 주소/깊이 복구 뒤 전파하고 선두를 보존한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void PumpFailureRestoresAddressAndKeepsHead()
    {
        var dispatch = Dispatch();
        bool fail = true;
        int calls = 0;
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (_, _, _) => { calls++; if (fail) throw new InvalidOperationException("수신 실패"); });
        var receiver = dispatch.SnapshotReceiver(0x24)!;
        double wall = 4000.0;
        var globals = Globals();
        globals.Wall = () => wall;
        var queue = new RawPacketDelayQueue(globals);
        var connection = Connection();
        byte[] packet = [1, 0x24, 5, 0, 1];
        Assert.Equal(0, queue.Enqueue(connection, receiver, dispatch, packet, 0));
        wall = 5000.0;
        Assert.Throws<InvalidOperationException>(() => queue.Pump(connection));
        Assert.Equal(0u, connection.Depth);
        Assert.Equal(0xAA, (int)connection.Address[0]);
        fail = false;
        Assert.Equal(1, queue.Pump(connection));
        Assert.Equal(2, calls);
    }

    /// <summary>다른 연결의 선두는 명시 실패한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void PumpRejectsForeignEntry()
    {
        var dispatch = Dispatch();
        var received = new List<(uint Sender, byte[] Packet)>();
        var receiver = Receive(dispatch, 0x24, 0x00444bc0, received);
        double wall = 4000.0;
        var globals = Globals();
        globals.Wall = () => wall;
        var queue = new RawPacketDelayQueue(globals);
        var first = Connection(1);
        byte[] packet = [1, 0x24, 5, 0, 1];
        Assert.Equal(0, queue.Enqueue(first, receiver, dispatch, packet, 0));
        wall = 5000.0;
        Assert.Throws<InvalidOperationException>(() => queue.Pump(Connection(2)));
        Assert.Equal(1, queue.Pump(first));
        Assert.Single(received);
    }

    /// <summary>100칸을 넘기면 ring이 두 배로 늘고 순서를 유지한다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void GrowPreservesOrderBeyondInitialCapacity()
    {
        var dispatch = Dispatch();
        var order = new List<uint>();
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (sender, _, _) => order.Add(sender));
        var receiver = dispatch.SnapshotReceiver(0x24)!;
        double wall = 8000.0;
        var globals = Globals();
        globals.Wall = () => wall;
        var queue = new RawPacketDelayQueue(globals);
        var connection = Connection();
        for (uint sender = 0; sender < 101; sender++)
        {
            connection.Sender = sender;
            byte[] packet = [1, 0x24, 5, 0, (byte)sender];
            Assert.Equal(0, queue.Enqueue(connection, receiver, dispatch, packet, 0));
        }

        wall = 9000.0;
        Assert.Equal(101, queue.Pump(connection));
        Assert.Equal(101, order.Count);
        for (uint sender = 0; sender < 101; sender++)
        {
            Assert.Equal(sender, order[(int)sender]);
        }
    }

    /// <summary>이미 설치된 큐는 다시 초기화하지 않으며 NULL/빈 분포는 설치하지 않는다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void PrepareInstallsOnceAndSetsDelayed()
    {
        var connection = Connection();
        Assert.Throws<ArgumentNullException>(() => RawPacketDelay.Prepare(null!, Globals()));
        RawPacketDelay.Prepare(connection, null);
        Assert.Null(connection.DelayQueue);
        Assert.False(connection.Delayed);
        var globals = Globals();
        RawPacketDelay.Prepare(connection, globals);
        Assert.NotNull(connection.DelayQueue);
        Assert.True(connection.Delayed);
        var installed = connection.DelayQueue;
        RawPacketDelay.Prepare(connection, Globals());
        Assert.Same(installed, connection.DelayQueue);
    }

    /// <summary>분배기는 지연 표시가 있는 연결을 실제 FIFO로 보낸다. 0.965에서 추가됐다.</summary>
    [Fact]
    public void DeliverReceiverRoutesThroughDelayQueue()
    {
        var dispatch = Dispatch();
        int received = 0;
        dispatch.RegisterCanonicalDirect(0x24, 0x00444bc0, (_, _, _) => received++);
        var connection = Connection();
        double wall = 1000.0;
        var globals = Globals();
        globals.Wall = () => wall;
        RawPacketDelay.Prepare(connection, globals);
        byte[] packet = [1, 0x24, 5, 0, 3];
        int delay = dispatch.DeliverReceiver(0x24, connection, packet);
        Assert.Equal(0, delay);
        Assert.Equal(0, received);
        var queue = new RawPacketDelayQueue(globals);
        Assert.Equal(0, queue.Enqueue(connection, dispatch.SnapshotReceiver(0x24)!, dispatch, packet, delay));
        wall = 5000.0;
        Assert.Equal(1, queue.Pump(connection));
        Assert.Equal(1, received);
    }
}
