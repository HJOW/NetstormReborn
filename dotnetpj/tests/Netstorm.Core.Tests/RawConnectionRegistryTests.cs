using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 같은 표와 장치가 등록/재배치/콜백/반납 중에도 원문 순서를 유지하는지 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>Registry의 실제 현재값·생존노드·Source대여·초기복제를 창 없이 검증한다.</summary>
public sealed class RawConnectionRegistryTests
{
    /// <summary>실제 장치가 사용하는 배열과 최종 반납을 가진 독립 외부 장치다.</summary>
    private sealed class Device : IDisposable
    {
        /// <summary>장치 호출이 실제로 읽고 쓰며 반납 뒤 사용할 수 없는 메모리다.</summary>
        private byte[]? _storage = new byte[32];
        /// <summary>최종 반납 시점의 원문 부모 상태만 관측한다.</summary>
        public Action? OnDispose { get; set; }
        /// <summary>실제 장치 메모리가 아직 살아 있는지다.</summary>
        public bool Alive => _storage is not null;
        /// <summary>실제 선택된 장치의 같은 메모리를 사용한다.</summary>
        public void Use(byte value) { Assert.NotNull(_storage); _storage[0] = value; }
        /// <summary>현재 부모 상태를 관측한 뒤 실제 장치 메모리를 반납한다.</summary>
        public void Dispose() { Assert.NotNull(_storage); OnDispose?.Invoke(); _storage = null; }
    }
    /// <summary>분배기 자체를 실행하지 않는 초기 복제의 실제 외부 경계다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new()
    {
        Now = () => throw new InvalidOperationException("복제에서 시계"), Record = (_, _, _) => throw new InvalidOperationException("복제에서 수신기록"),
        Terminate = _ => throw new InvalidOperationException("복제에서 종료"), Close = _ => throw new InvalidOperationException("복제에서 close"), Release = _ => throw new InvalidOperationException("복제에서 free"),
    });
    /// <summary>같은 고정 표와 서로 다른 실제 동기화/개수 주소를 현재 뷰가 복사 없이 공유한다.</summary>
    [Fact]
    public void SharedTablesBorrowActualSynchronizationAndKeepRegistryCountSeparate()
    {
        var registry = new RawConnectionRegistry(); ConnectionCleanupGlobals original = registry.Cleanup();
        var first = new ConnectionSyncState { Synchronization = 7, PendingCount = 99 }; registry.BindSynchronization(first);
        ConnectionCleanupGlobals borrowed = registry.Cleanup(); borrowed.Sync.PendingCount = 3; borrowed.Sync.Synchronization = 8;
        Assert.Equal(99u, first.PendingCount); Assert.Equal(8u, first.Synchronization); Assert.Equal(3u, original.Sync.PendingCount);
        Assert.Same(original.Transceivers, borrowed.Transceivers); Assert.Same(original.Pending, borrowed.Pending); Assert.Same(original.Buffers, borrowed.Buffers);
        var second = new ConnectionSyncState { Synchronization = 11 }; registry.BindSynchronization(second); borrowed.Sync.Synchronization = 9;
        Assert.Equal(9u, first.Synchronization); Assert.Equal(11u, registry.Cleanup().Sync.Synchronization);
        registry.Cleanup().Sync.PendingCount = 0xffffffff; Assert.True(registry.CanAccept());
        Assert.Equal(32, borrowed.Pending.Length); Assert.Equal(22, borrowed.Transceivers.Length); Assert.Equal(22, borrowed.Buffers.Length);
        Assert.Equal(new byte[] { 2 }.Concat(new byte[19]), registry.Address(9));
        Assert.ThrowsAny<Exception>(() => registry.Address(-32)); Assert.ThrowsAny<Exception>(() => RawTransportPolling.CurrentTransportToken(registry, -32));
    }
    /// <summary>같은 실제 노드의 두 임시 별칭을buffered까지 보존하고 이후 개수/음수칸만 반납한다.</summary>
    [Fact]
    public void PendingAssignmentKeepsSameNodeUntilBufferedAndThenUsesNativeClose()
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection(); var trace = new List<string>();
        uint token = registry.Track(node, () => trace.Add("close"), () => trace.Add("free")); Assert.Equal(-1, registry.Accept(token));
        registry.Assign(-1, 3, 1, current =>
        {
            Assert.Equal(token, current); Assert.Same(node, registry.Node(current)); Assert.Equal(3u, node.Sender);
            Assert.Equal(token, registry.Cleanup().Pending[1]); Assert.Equal(token, registry.Cleanup().Transceivers[3]); Assert.Equal(1u, registry.Cleanup().Sync.PendingCount);
            Assert.Throws<InvalidOperationException>(registry.ShutdownTrackedDevices); node.Flags |= 2;
        });
        Assert.Equal(0u, registry.Cleanup().Pending[1]); Assert.Equal(0u, registry.Cleanup().Sync.PendingCount);
        registry.EndPlayers(0); Assert.Empty(trace); registry.EndPlayers(1);
        Assert.Equal(new[] { "close", "free" }, trace); Assert.False(registry.Owns(token)); Assert.Equal(0u, registry.Cleanup().Transceivers[3]); Assert.Equal(0x42u, node.Flags);
    }
    /// <summary>실제free실패는먼저쓴sync/flags를보존하고원래칸/Binding을남긴다.</summary>
    [Fact]
    public void FailedFreeKeepsActualBindingAndNativeWrittenPrefix()
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection(); int destroyed = 0;
        uint token = registry.Track(node, () => destroyed++, () => throw new InvalidOperationException("명시free중단")); registry.Register(9, token); registry.Cleanup().Sync.Synchronization = token;
        Assert.Throws<InvalidOperationException>(() => registry.Disconnect(9)); Assert.Equal(1, destroyed); Assert.Equal(0x40u, node.Flags);
        Assert.Equal(0u, registry.Cleanup().Sync.Synchronization); Assert.Equal(token, registry.Cleanup().Transceivers[9]); Assert.True(registry.Owns(token)); registry.Forget(token);
    }
    /// <summary>미설치/중단buffered모두이미쓴양수별칭/sender와아직남은pending/count를보존한다.</summary>
    [Theory, InlineData(false), InlineData(true)]
    public void BufferedFailureKeepsTemporaryAliases(bool installed)
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection(); uint token = registry.Track(node, () => { }, () => { }); registry.Accept(token);
        Action<uint>? buffered = installed ? _ => throw new InvalidOperationException("명시buffered중단") : null;
        Assert.Throws<InvalidOperationException>(() => registry.Assign(-1, 4, 1, buffered));
        Assert.Equal(token, registry.Cleanup().Transceivers[4]); Assert.Equal(token, registry.Cleanup().Pending[1]); Assert.Equal(1u, registry.Cleanup().Sync.PendingCount); Assert.Equal(4u, node.Sender);
        registry.Forget(token); Assert.Equal(0u, registry.Cleanup().Sync.PendingCount);
    }
    /// <summary>등록Source와새coordinator가다르면두실제장치를부모의마지막칸/count쓰기뒤까지보유한다.</summary>
    [Fact]
    public void CurrentAndRegisteredDevicesSurviveUntilLastNativeParentStore()
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection(); var older = new Device(); var current = new Device(); var trace = new List<string>();
        using var oldLife = new ConnectionDeviceLifetime(older, value => ((Device)value).Dispose());
        using var newLife = new ConnectionDeviceLifetime(current, value => ((Device)value).Dispose());
        registry.BindDevicePin(_ => oldLife.Pin(), true);
        uint token = registry.Track(node, () => { older.Use(1); current.Use(2); newLife.Dispose(); trace.Add("close"); }, () => { older.Use(3); current.Use(4); trace.Add("free"); });
        registry.Accept(token); oldLife.Dispose(); Assert.True(older.Alive); registry.BindDevicePin(_ => newLife.Pin(), true);
        older.OnDispose = () => { Assert.Equal(0u, registry.Cleanup().Pending[1]); Assert.Equal(0u, registry.Cleanup().Sync.PendingCount); Assert.False(registry.Owns(token)); trace.Add("old-source"); registry.Forget(token); };
        current.OnDispose = () => { Assert.Equal(0u, registry.Cleanup().Pending[1]); Assert.Equal(0u, registry.Cleanup().Sync.PendingCount); trace.Add("new-source"); };
        registry.Disconnect(-1); Assert.False(older.Alive); Assert.False(current.Alive);
        Assert.Equal(new[] { "close", "free" }, trace.Take(2)); Assert.Contains("old-source", trace); Assert.Contains("new-source", trace);
    }
    /// <summary>직접송신콜백이자기Binding을반납해도선택된등록Source는가상호출복귀까지유지한다.</summary>
    [Fact]
    public void DirectCallbackSelfReleaseKeepsSelectedPhysicalDeviceAlive()
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection(); var source = new Device();
        using var life = new ConnectionDeviceLifetime(source, value => ((Device)value).Dispose()); registry.BindDevicePin(_ => life.Pin(), true); uint token = 0;
        token = registry.Track(node, () => { }, () => source.Use(2), packet => { registry.Release(token); Assert.True(source.Alive); source.Use(packet.Span[0]); });
        registry.Register(9, token); life.Dispose(); registry.SendDirect(token, new byte[] { 7 }); Assert.False(source.Alive); Assert.False(registry.Owns(token));
    }
    /// <summary>현재token을선택한뒤Playerflag1만검사하고실제readiness/flush/receive와이전노드를직접사용한다.</summary>
    [Fact]
    public void CurrentSlotsPlayerGateAndDirectDevicesUseSameNode()
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection(); var trace = new List<string>(); byte[] players = new byte[1620];
        uint token = registry.Track(node, () => { }, () => { }, _ => trace.Add("send"), () => -1, flush: () => trace.Add("flush"), receive: () => new(true, false));
        registry.Register(3, token); Assert.False(registry.SendMutable(3, new byte[4], players)); players[3 * 180 + 0x50] = 1; Assert.True(registry.SendMutable(3, new byte[4], players));
        Assert.ThrowsAny<Exception>(() => registry.SendMutable(1, new byte[4], ReadOnlyMemory<byte>.Empty));
        Assert.Equal(-1, registry.SocketReadiness(token)); registry.FlushDirect(token); Assert.Equal(new(true, false), registry.ReceiveDirect(token)); Assert.Equal(new[] { "send", "flush" }, trace);
        Assert.Equal(token, registry.TokenFor(node)); Assert.Equal(token, RawTransportPolling.CurrentTransportToken(registry, 3));
        node.Address[3] = 7; byte[] address = registry.Address(3); address[3] = 9; Assert.Equal(7, node.Address[3]); registry.Forget(token);
    }
    /// <summary>Native논리진단복귀뒤물리경계에서실패하고-32는별도진단을추가하지않는다.</summary>
    [Fact]
    public void NativeSendRetainsOriginalDiagnosticAndPhysicalBoundaries()
    {
        var registry = new RawConnectionRegistry(); int calls = 0;
        ConnectionRegistryAssertion assertion = (expression, file, line) => { calls++; Assert.Equal(0x1eu, line); Assert.Equal("-zaMAX_PENDING <= id && id < NUM_TRANSCEIVERS", expression); Assert.Equal("\\Ns\\O\\Ztrans.cpp", file); };
        Assert.False(registry.SendMutableNative(21, new byte[4], ReadOnlyMemory<byte>.Empty, assertion)); Assert.Equal(0, calls);
        Assert.ThrowsAny<Exception>(() => registry.SendMutableNative(22, new byte[4], ReadOnlyMemory<byte>.Empty, assertion)); Assert.Equal(1, calls);
        Assert.ThrowsAny<Exception>(() => registry.SendMutableNative(-32, new byte[4], ReadOnlyMemory<byte>.Empty, assertion)); Assert.Equal(1, calls);
    }
    /// <summary>관리Source반납도실패하면원래부모예외와반납예외를모두로그가능한형태로보존한다.</summary>
    [Fact]
    public void ParentAndManagedSourceReleaseFailuresAreBothPreserved()
    {
        var registry = new RawConnectionRegistry(); var source = new Device { OnDispose = () => throw new InvalidOperationException("source") };
        using var life = new ConnectionDeviceLifetime(source, owner => ((Device)owner).Dispose()); registry.BindDevicePin(_ => life.Pin(), true);
        uint token = registry.Track(new(), () => { }, () => { }); registry.Accept(token); life.Dispose();
        AggregateException error = Assert.Throws<AggregateException>(() => registry.Assign(-1, 3, 1, _ => { registry.Forget(token); throw new ApplicationException("body"); }));
        Assert.Contains(error.Flatten().InnerExceptions, value => value is ApplicationException && value.Message == "body"); Assert.Contains(error.Flatten().InnerExceptions, value => value is InvalidOperationException && value.Message == "source");
        registry.BindDevicePin(null); uint later = registry.Track(new(), () => { }, () => { }); registry.ShutdownTrackedDevices(); Assert.False(registry.Owns(later));
    }
    /// <summary>초기버퍼는동일복제본을송신하고callback뒤현재칸을다시free하며bootstrap기존정책은보존한다.</summary>
    [Fact]
    public void BootstrapSendsOriginalOwnedCopyAndFreesCurrentReplacement()
    {
        var registry = new RawConnectionRegistry(); var dispatch = Dispatch(); byte[] packet = [0, 0x58, 5, 0, 77]; byte[]? sent = null;
        uint token = registry.Track(new(), () => { }, () => { }, bytes => { sent = bytes.ToArray(); registry.StoreConnectPacket(9, new byte[] { 0, 0x58, 5, 0, 88 }, dispatch); });
        registry.Register(9, token); dispatch.Version(1, 7, 1); int observed = 0; dispatch.ObservePacket(0x58, (_, direction) => { Assert.Equal(1u, direction); observed++; });
        RawConnectionRegistry.RegisterConnectPacketReceiver(dispatch, registry); Assert.Equal(7, dispatch.Registration(1).Version); Assert.Equal(1, dispatch.Registration(1).Mismatch);
        registry.StoreConnectPacket(9, packet, dispatch); packet[4] = 1; dispatch.InvokeDirect(1, 9, null, ReadOnlyMemory<byte>.Empty);
        Assert.Equal(new byte[] { 0, 0x58, 5, 0, 77 }, sent); Assert.Equal(2, observed); Assert.Equal(0u, registry.Cleanup().Buffers[9]);
        registry.SendConnectPacket(-1); registry.StoreConnectPacket(9, null, dispatch); registry.ShutdownTrackedDevices(); Assert.False(registry.Owns(token));
    }
    /// <summary>b6 진단복귀뒤현재pending을다시읽고buffered위치에서같은새노드를선택한다.</summary>
    [Fact]
    public void NativeAssertionCanChangeSelectedPendingBeforeAssignment()
    {
        var registry = new RawConnectionRegistry(); uint first = registry.Track(new(), () => { }, () => { }); registry.Register(3, first);
        uint pending = registry.Track(new(), () => { }, () => { }); registry.Accept(pending); var replacement = new PacketConnection(); int buffered = 0;
        uint next = registry.Track(replacement, () => { }, () => { }, buffered: () => { replacement.Flags |= 2; buffered++; });
        registry.AssignNative(-1, 3, 1, (expression, file, line) => { Assert.Equal(0xb6u, line); Assert.Equal("transceivers[transId] == NULL", expression); Assert.Equal("\\Ns\\O\\Ztrans.cpp", file); registry.Cleanup().Pending[1] = next; });
        Assert.Equal(next, registry.Cleanup().Transceivers[3]); Assert.Equal(3u, replacement.Sender); Assert.Equal(1, buffered); Assert.Equal(0u, registry.Cleanup().Sync.PendingCount);
        registry.ShutdownTrackedDevices(); Assert.False(registry.Owns(first)); Assert.False(registry.Owns(pending)); Assert.False(registry.Owns(next));
    }
    /// <summary>진행중노드최종종료는거부하고정상종료는덮인잔류노드/별도초기복제까지반납한다.</summary>
    [Fact]
    public void ShutdownPreservesActiveDepthThenCleansOverwrittenNodesAndBuffers()
    {
        var registry = new RawConnectionRegistry(); var node = new PacketConnection { Depth = 1 }; var trace = new List<uint>(); uint first = registry.Track(node, () => trace.Add(1), () => trace.Add(2)); registry.PublishConnect(9, first);
        Assert.Throws<InvalidOperationException>(registry.ShutdownTrackedDevices); node.Depth = 0;
        uint second = registry.Track(new(), () => trace.Add(3), () => trace.Add(4)); registry.PublishConnect(9, second);
        registry.StoreConnectPacket(9, new byte[] { 0, 0x58, 4, 0 }, Dispatch()); registry.Cleanup().Sync.PendingCount = 9;
        registry.ShutdownTrackedDevices(); Assert.Equal(new uint[] { 3, 4, 1, 2 }, trace); Assert.Equal(9u, registry.Cleanup().Sync.PendingCount);
        Assert.Equal(0u, registry.Cleanup().Buffers[9]); Assert.Throws<InvalidOperationException>(() => registry.Track(new(), () => { }, () => { }));
    }
    /// <summary>실제Source선택실패뒤에는rawcallback등록을남기지않고이미소비한token도재사용하지않는다.</summary>
    [Fact]
    public void FailedSourceSelectionPublishesNoBindingAndConsumesOpaqueToken()
    {
        var registry = new RawConnectionRegistry(); registry.BindDevicePin(_ => throw new InvalidOperationException("명시source선택중단"), true);
        var node = new PacketConnection(); Assert.Throws<InvalidOperationException>(() => registry.Track(node, () => { }, () => { })); Assert.False(registry.Owns(0x6d000000));
        registry.BindDevicePin(null); uint token = registry.Track(node, () => { }, () => { }); Assert.Equal(0x6d000001u, token); Assert.Throws<InvalidOperationException>(() => registry.Track(node, () => { }, () => { })); registry.Forget(token);
    }
}
