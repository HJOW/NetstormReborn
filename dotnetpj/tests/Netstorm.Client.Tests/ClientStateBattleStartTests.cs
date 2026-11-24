// 1a/1b 전투 시작 수신의 원문 observer·버전·길이와 상태 쓰기·복제·기록·분배 순서를 대조한다.
// cpppj/src/o/RawStateBattleStart.cpp 의 ObserveBattleStartZone/ReceiveBattleStartZone/ReceiveBattleStartPeer와
// cpppj/src/client/ClientCampaignTransport.cpp 의 InstallClientStateBattleStartReceivers를 대조한다.
// 0.951에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 1a/1b 전투 시작의 실제 상태·예약·포트·구독자·요청·검사·복제·기록·연결·실행 호출을 기록한다.
public sealed class ClientStateBattleStartTests
{
    /// <summary>같은 실제 상태 블록과 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
        public readonly LocalMissionStateStorage State = new()
        {
            Old = new byte[0x90],
            Phase = new byte[0x90],
            Immediate = new byte[0x90],
            Current = new byte[0x90],
            Next = new byte[0x90],
        };

        /// <summary>같은 실제 예약 수 owner다.</summary>
        public readonly UIntBox ZonePending = new();

        /// <summary>현재 게임 포트다.</summary>
        public uint GamePort = 6799;

        /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
        public readonly UIntBox Subscriber = new() { Value = 0x1234 };

        /// <summary>현재 요청 DWORD다.</summary>
        public uint Requested;

        /// <summary>같은 실제 현재 검사 뷰다.</summary>
        public readonly RawCheckingFlag Checking = new();

        /// <summary>현재 서버 여부다.</summary>
        public bool Server;

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 복제한 패킷 사본이다.</summary>
        public readonly List<byte[]> Duplicated = [];

        /// <summary>다음 복제 토큰이다.</summary>
        public uint NextToken = 77;

        /// <summary>실제 기록 이름이다.</summary>
        public readonly List<string> Records = [];

        /// <summary>실제 끊은 연결 번호다.</summary>
        public readonly List<int> Disconnected = [];

        /// <summary>실제 실행 횟수다.</summary>
        public int Runs;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientStateBattleStartHooks Hooks() => new()
        {
            State = State,
            ZonePending = ZonePending,
            GamePort = () => GamePort,
            Subscriber = Subscriber,
            RecordRequested = () => Requested,
            Checking = Checking,
            Server = () => Server,
            Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
            Duplicate = packet => { Duplicated.Add(packet.ToArray()); return NextToken; },
            OpenRecord = name => Records.Add(name),
            Disconnect = id => Disconnected.Add(id),
            Run = () => Runs++,
        };

        /// <summary>현재 phase를 쓴다.</summary>
        public void Phase(uint value) => BinaryPrimitives.WriteUInt32LittleEndian(State.Phase.Span.Slice(0, 4), value);

        /// <summary>원문 1a 15BYTE 이상 입력 패킷을 조립한다.</summary>
        public static byte[] ZonePacket(byte clients, uint address)
        {
            byte[] packet = new byte[20];
            packet[1] = 0x1a;
            packet[14] = clients;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(10, 4), address);
            return packet;
        }

        /// <summary>원문 1b 38BYTE 입력 패킷을 조립한다.</summary>
        public static byte[] PeerPacket(uint address, string text)
        {
            byte[] packet = new byte[38];
            packet[1] = 0x1b;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), address);
            byte[] body = System.Text.Encoding.ASCII.GetBytes(text);
            body.CopyTo(packet.AsSpan(8, Math.Min(body.Length, 30)));
            return packet;
        }
    }

    /// <summary>같은 장기 분배기를 준비한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078,
        new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>같은 실제 Source 대여를 흉내낸다.</summary>
    private static NativeBattleSource FakeSource()
    {
        // 내부 대여는 리플렉션으로만 빌리며 가짜 Source를 합성하지 않는다.
        var registry = new RawConnectionRegistry();
        byte[] players = new byte[1620];
        var hooks = new PacketDispatchHooks { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
        var transport = new CompositeMissionTransport(CompositeTransportRole.InternalMission, players, () => false, hooks, registry, new object());
        var borrow = typeof(CompositeMissionTransport).GetMethod("Borrow", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)
            ?? throw new InvalidOperationException("복합 전송의 실제 대여 입구 누락");
        return (NativeBattleSource)borrow.Invoke(transport, null)!;
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.ObserveZone(new byte[20], 1, null!));
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.ReceiveZone(null!, new byte[20]));
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.ReceivePeer(null!, new byte[38]));
        var bare = new ClientStateBattleStartHooks();
        Assert.Throws<InvalidOperationException>(() => ClientStateBattleStart.ReceiveZone(bare, new byte[20]));
        Assert.Throws<InvalidOperationException>(() => ClientStateBattleStart.ReceivePeer(bare, new byte[38]));
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.Install(null!, scene.Hooks(), _ => FakeSource(), (_, _) => { }));
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.Install(Dispatch(), null!, _ => FakeSource(), (_, _) => { }));
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.Install(Dispatch(), scene.Hooks(), null!, (_, _) => { }));
        Assert.Throws<ArgumentNullException>(() => ClientStateBattleStart.Install(Dispatch(), scene.Hooks(), _ => FakeSource(), null!));
    }

    /// <summary>원문 observer·버전·길이와 고정 F를 설치한다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsObserversVersionsLengthsAndCanonicalEntries()
    {
        var scene = new Scene();
        scene.Phase(29);
        var dispatch = Dispatch();
        int dispatched = 0;
        ClientStateBattleStart.Install(dispatch, scene.Hooks(), _ => FakeSource(), (_, peer) => dispatched += peer ? 10 : 1);
        var zone = dispatch.Registration(0x1a);
        Assert.Equal(1, zone.Version);
        Assert.Equal(0, zone.Mismatch);
        Assert.Equal(ushort.MaxValue, zone.Length);
        Assert.NotNull(zone.PacketObserver);
        Assert.Null(zone.Observer);
        Assert.Equal(0x004b4df0u, dispatch.SnapshotReceiver(0x1a)!.NativeEntry);
        var peer = dispatch.Registration(0x1b);
        Assert.Equal(1, peer.Version);
        Assert.Equal(0, peer.Mismatch);
        Assert.Equal(38, peer.Length);
        Assert.NotNull(peer.Observer);
        Assert.Null(peer.PacketObserver);
        Assert.Equal(0x004b8b20u, dispatch.SnapshotReceiver(0x1b)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>수신 방향 관찰기는 버퍼를 읽지 않는다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ObserveZoneReceivingIgnoresBuffer()
    {
        var scene = new Scene();
        byte[] packet = Scene.ZonePacket(0xff, 0);
        ClientStateBattleStart.ObserveZone(packet, 0, scene.Hooks());
        Assert.Equal(0xff, packet[14]);
        Assert.Empty(scene.Assertions);
    }

    /// <summary>송신 관찰기는 현재 개수로 선언 WORD를 정하고 0xff를 진단한다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ObserveZoneSendingWritesLengthAndAssertsOnMax()
    {
        var scene = new Scene();
        byte[] packet = Scene.ZonePacket(3, 0);
        ClientStateBattleStart.ObserveZone(packet, 1, scene.Hooks());
        Assert.Equal((ushort)(3 * 71 + 15), BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        Assert.Empty(scene.Assertions);
        byte[] full = Scene.ZonePacket(0xff, 0);
        ClientStateBattleStart.ObserveZone(full, 1, scene.Hooks());
        Assert.Single(scene.Assertions);
        Assert.Equal((ushort)(0xff * 71 + 15), BinaryPrimitives.ReadUInt16LittleEndian(full.AsSpan(2, 2)));
    }

    /// <summary>1a는 next zlocal·7/포트·복제/예약을 쓰고 기록 없이 10/11을 닫는다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ReceiveZoneWritesNextDuplicatesAndDisconnects()
    {
        var scene = new Scene();
        scene.Phase(29);
        ClientStateBattleStart.ReceiveZone(scene.Hooks(), Scene.ZonePacket(2, 0x11223344));
        Assert.Equal(0x636f6c7au, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0x54, 4)));
        Assert.Equal(0x2a2c6c61u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0x58, 4)));
        Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0, 4)));
        Assert.Equal(6799u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0x7c, 4)));
        Assert.Equal(77u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Next.Span.Slice(0x80, 4)));
        Assert.Equal(1u, scene.ZonePending.Value);
        Assert.Single(scene.Duplicated);
        Assert.Empty(scene.Records);
        Assert.Equal([10, 11], scene.Disconnected);
    }

    /// <summary>1a는 요청이 있으면 packet+10과 구독자로 기록을 연다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ReceiveZoneOpensRecordWhenRequested()
    {
        var scene = new Scene();
        scene.Phase(33);
        scene.Requested = 1;
        ClientStateBattleStart.ReceiveZone(scene.Hooks(), Scene.ZonePacket(1, 0x00ABCDEF));
        Assert.Equal(["00ABCDEF.1234.c.dat"], scene.Records);
    }

    /// <summary>1a는 검사가 켜진 phase 불일치를 진단한다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ReceiveZoneAssertsOnPhaseMismatchWhenChecking()
    {
        var scene = new Scene();
        scene.Phase(7);
        scene.Checking.Set(true);
        ClientStateBattleStart.ReceiveZone(scene.Hooks(), Scene.ZonePacket(1, 0));
        Assert.Single(scene.Assertions);
    }

    /// <summary>1b는 서버/예약이 있으면 Run 없이 그대로 돌아간다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ReceivePeerEarlyReturnsWithoutRun()
    {
        var scene = new Scene();
        scene.Phase(29);
        scene.Server = true;
        ClientStateBattleStart.ReceivePeer(scene.Hooks(), Scene.PeerPacket(0x10, "hi\0"));
        Assert.Equal(0, scene.Runs);
        scene.Server = false;
        scene.ZonePending.Value = 1;
        ClientStateBattleStart.ReceivePeer(scene.Hooks(), Scene.PeerPacket(0x10, "hi\0"));
        Assert.Equal(0, scene.Runs);
    }

    /// <summary>1b는 현재 주소·포트·7을 쓰고 문자열을 NUL까지 복사한 뒤 Run한다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ReceivePeerWritesCurrentCopiesStringAndRuns()
    {
        var scene = new Scene();
        scene.Phase(29);
        scene.Requested = 1;
        ClientStateBattleStart.ReceivePeer(scene.Hooks(), Scene.PeerPacket(0x00ABCDEF, "ab\0"));
        Assert.Equal(["00ABCDEF.1234.c.dat"], scene.Records);
        Assert.Equal((byte)'a', scene.State.Current.Span[0x54]);
        Assert.Equal((byte)'b', scene.State.Current.Span[0x55]);
        Assert.Equal(0, scene.State.Current.Span[0x56]);
        Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0, 4)));
        Assert.Equal(6799u, BinaryPrimitives.ReadUInt32LittleEndian(scene.State.Current.Span.Slice(0x7c, 4)));
        Assert.Equal(1, scene.Runs);
    }

    /// <summary>설치 뒤 실제 분배 수신도 같은 본문을 수행한다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void InstalledReceiversRunSameBodies()
    {
        var scene = new Scene();
        scene.Phase(29);
        var dispatch = Dispatch();
        var sources = new List<(NativeBattleSource Source, bool Peer)>();
        ClientStateBattleStart.Install(dispatch, scene.Hooks(), _ => FakeSource(), (source, peer) => sources.Add((source, peer)));
        dispatch.InvokeDirect(0x1a, 9, null, Scene.ZonePacket(1, 0x10));
        Assert.Equal(1u, scene.ZonePending.Value);
        Assert.Single(sources);
        Assert.False(sources[0].Peer);
        sources.Clear();
        scene.ZonePending.Value = 0;
        dispatch.InvokeDirect(0x1b, 9, null, Scene.PeerPacket(0x20, "x\0"));
        Assert.Equal(0, scene.Runs);
        Assert.Single(sources);
        Assert.True(sources[0].Peer);
        foreach (var entry in sources)
        {
            entry.Source.Dispose();
        }
    }

    /// <summary>짧은 패킷은 원문 DWORD 폭에서 거부한다. 0.951에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsThrow()
    {
        var zoneScene = new Scene();
        zoneScene.Phase(29);
        zoneScene.Requested = 1;
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientStateBattleStart.ReceiveZone(zoneScene.Hooks(), new byte[10]));
        var peerScene = new Scene();
        peerScene.Phase(29);
        peerScene.Requested = 1;
        byte[] noNul = new byte[11];
        noNul.AsSpan().Fill(0x41);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientStateBattleStart.ReceivePeer(peerScene.Hooks(), noNul));
    }
}
