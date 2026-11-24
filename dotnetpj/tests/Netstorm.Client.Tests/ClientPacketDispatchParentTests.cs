using System.Buffers.Binary;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 공통 패킷 분배 부모의 41h·로그인·Native gameplay·Carrier/Construction·장기 CS 설치를 검사하는 클래스다. 2026-11-23 0.897에서 R4-6 전송·수신 묶음용으로 추가하고 0.899에서 Native gameplay 묶음을, 0.964에서 Carrier/Construction 묶음을, 0.966에서 장기 CS 묶음(0x7b·0x7c)을 이었다.
// 근거: R4-6 남음의 "공통 41h가 Root.ReceiveNativePacketChat을 부르는 전송 설치"와 "NativeLoginPacketReceivers의 실제 MonoGame Root 설치 호출"이다. 0.969에서 연결 종료 2번을, 0.970에서 전투 시작 1a/1b를 이었다.
public sealed class ClientPacketDispatchParentTests
{
    /// <summary>같은 실제 두 구독자 DWORD다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 두 구독자 DWORD다.</summary>
        public readonly ClientSubscriberGlobals Subscribers = new();
        /// <summary>같은 실제 Player/권한/현재 세션 소유자다.</summary>
        public readonly ClientGameplayGlobals Game;
        /// <summary>같은 실제 State/동기화 장기 주소다.</summary>
        public readonly ClientLocalStateGlobals Local;
        /// <summary>같은 실제 연결 Registry/Source 장부다.</summary>
        public readonly ClientConnectionGlobals Connections;
        /// <summary>같은 실제 표면/시계/커널이다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>창이 없어도 존재하는 실제 마스크/현재 UI 슬롯이다.</summary>
        public readonly ClientChatWindowGlobals Chat = new();
        /// <summary>같은 Client 수명으로 유지하는 실제 CS 슬롯·할당 owner다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();
        /// <summary>외부 표시 진단의 실제 선택 줄 번호다.</summary>
        public readonly List<uint> Assertions = [];
        /// <summary>실제 공통 소유자만 만들고 창/소리/네트워크는 열지 않는다.</summary>
        public Scene()
        {
            Game = new(Subscribers); Local = new(Game, new()); Connections = new(Local); Surface = new(Game, new(Local), new Kernel(), () => 1000); Surface.BindDisplayDiagnostics((_, _, line) => Assertions.Add(line));
        }
    }
    /// <summary>실제 현재 물리의 DWORD/WORD/BYTE만 기록한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="at">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 폭이다.</param>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 원문 비정렬 필드 폭만 낮은 BYTE부터 기록한다.
        for (int index = 0; index < width; index++) bytes[at + index] = (byte)(value >> (8 * index));
    }
    /// <summary>실제 시계/기록/종료 장치를 빈 몸체로 둔다.</summary>
    private static PacketDispatchHooks DispatchHooks() => new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };
    /// <summary>실제 장기 연결 활성을 고정 값으로 둔다.</summary>
    private static ConnectionActivityHooks ActivityHooks() => new() { WallSeconds = () => 100.0, FrameNumber = () => 100, Server = () => true, EraseStatus = (_, _) => { } };
    /// <summary>같은 장기 Client 장치로 실제 분배기를 묶는다.</summary>
    private static RawPacketDispatch Build(Scene scene, Action<Memory<byte>> receive, Func<uint>? recordRequested = null, Action<string>? openRecord = null,
        Func<string, int>? config = null, Action<uint, string>? diagnosticLog = null, Action<string>? tell = null) => ClientPacketDispatch.Build(
        DispatchHooks(), scene.Game, scene.Connections, scene.Surface, scene.Chat, receive,
        (expression, file, line) => scene.Assertions.Add(line), ActivityHooks(), scene.Challenge, recordRequested, openRecord, config, diagnosticLog, tell);
    /// <summary>같은 장부로 실제 수송을 만들고 소유 중인 현재 연결을 돌려준다. Source 선택·복제·연결 효과가 같은 실제 장치를 쓰는지 대조한다.</summary>
    private static CompositeMissionTransport MakeOwnedConnection(Scene scene, out PacketConnection connection)
    {
        // 수송 생성·listener·연결·재배치는 같은 실제 owner 묶음으로 수행한다.
        scene.Game.Authority.Server = true;
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        var transport = ClientCampaignTransport.Make(scene.Game, scene.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { },
            new() { WallSeconds = () => 100.0, FrameNumber = () => 100, Server = () => scene.Game.Authority.Server, EraseStatus = (_, _) => { } },
            scene.Challenge, CompositeTransportRole.InternalMission, new OriginalGameClock(0), () => 100u,
            (_, _, _, _) => { }, null);
        transport.InternalTransport.RegisterLogin((_, _, _) => { });
        transport.Serve(6799);
        transport.Connect(new byte[20], 6799);
        Assert.Equal(0, transport.Poll());
        transport.Assign(-1, 1);
        connection = transport.Connection(1) ?? throw new InvalidOperationException("소유 중인 현재 연결 누락");
        return transport;
    }
    /// <summary>원문 1a 입력 패킷을 조립한다. 길이는 개수×71+15의 실제 선언 길이를 따른다.</summary>
    private static byte[] ZonePacket(byte clients, uint address)
    {
        // 종류·개수·주소를 같은 위치에 두고 나머지는 0으로 둔다.
        byte[] packet = new byte[clients * 71 + 15];
        packet[1] = 0x1a;
        packet[14] = clients;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(10, 4), address);
        return packet;
    }
    /// <summary>원문 1b 38BYTE 입력 패킷을 조립한다.</summary>
    private static byte[] PeerPacket(uint address, string text)
    {
        // 주소와 NUL 끝 문자열을 같은 위치에 둔다.
        byte[] packet = new byte[38];
        packet[1] = 0x1b;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), address);
        byte[] body = System.Text.Encoding.ASCII.GetBytes(text);
        body.CopyTo(packet.AsSpan(8, System.Math.Min(body.Length, 30)));
        return packet;
    }
    /// <summary>부모는 41h 실제 F와 로그인 묶음을 같은 분배기에 설치한다.</summary>
    [Fact]
    public void ParentInstallsChatAndLoginEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.NotNull(dispatch.SnapshotReceiver(0x27));
        Assert.NotNull(dispatch.SnapshotReceiver(0x2b));
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모는 설정·버전·기술 묶음을 같은 분배기에 설치한다. 가변 묶음은 기존 관찰기 검사가 대조한다.</summary>
    [Fact]
    public void ParentInstallsSettingsVersionAndTechEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        Assert.Equal(0x0044dc20u, dispatch.SnapshotReceiver(0x32)!.NativeEntry);
        Assert.Equal(0x004b3af0u, dispatch.SnapshotReceiver(0x0b)!.NativeEntry);
        Assert.Equal(0x004b49e0u, dispatch.SnapshotReceiver(0x69)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모는 Native gameplay 6종 묶음을 같은 분배기에 설치한다.</summary>
    [Fact]
    public void ParentInstallsNativeGameplayEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        Assert.Equal(0x0045e4f0u, dispatch.SnapshotReceiver(0x74)!.NativeEntry);
        Assert.Equal(0x0045e410u, dispatch.SnapshotReceiver(0x38)!.NativeEntry);
        Assert.Equal(0x0045e5d0u, dispatch.SnapshotReceiver(0x73)!.NativeEntry);
        Assert.Equal(0x0046a0c0u, dispatch.SnapshotReceiver(0x3a)!.NativeEntry);
        Assert.Equal(0x0045e2b0u, dispatch.SnapshotReceiver(0x39)!.NativeEntry);
        Assert.Equal(0x00426490u, dispatch.SnapshotReceiver(0x86)!.NativeEntry);
        Assert.Equal(0x00466d30u, dispatch.SnapshotReceiver(0x3b)!.NativeEntry);
        Assert.Equal(0x00466b60u, dispatch.SnapshotReceiver(0x3c)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모는 장기 기본 18행을 같은 분배기에 설치한다. C3 수신·버전·길이·관찰기 행을 대조한다. 0.934에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsBasic18Entries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        uint[] c3 = [0x0c, 0x42, 0x4d, 0x55, 0x56, 0x57, 0x5a];
        uint[] mutable = [0x0c, 0x55, 0x56, 0x57, 0x16, 0x1c, 0x85];
        foreach (uint type in c3)
        {
            Assert.Equal(0x004437f0u, dispatch.SnapshotReceiver(type)!.NativeEntry);
        }
        Assert.Equal(1, dispatch.Registration(0x0c).Version);
        Assert.Equal(0, dispatch.Registration(0x0c).Mismatch);
        Assert.Equal(ushort.MaxValue, dispatch.Registration(0x0c).Length);
        Assert.Equal(12, dispatch.Registration(0x42).Length);
        Assert.Equal(5, dispatch.Registration(0x5a).Length);
        Assert.Equal(274, dispatch.Registration(0x87).Length);
        Assert.NotNull(dispatch.Registration(0x0c).PacketObserver);
        Assert.NotNull(dispatch.Registration(0x42).Observer);
        Assert.Null(dispatch.Registration(0x42).PacketObserver);
        Assert.NotNull(dispatch.Registration(0x16).PacketObserver);
        // 같은 묶음의 기존 설치가 기본 행에 밀리지 않는다.
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Equal(0x0044dc20u, dispatch.SnapshotReceiver(0x32)!.NativeEntry);
        Assert.Equal(0x0045e4f0u, dispatch.SnapshotReceiver(0x74)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모는 네 미션 송신 관찰기를 같은 분배기에 설치한다. 순수 관찰기만 두고 수신 F는 바꾸지 않는다. 0.935에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsMissionSendObservers()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        foreach (uint type in new uint[] { 0x2a, 0x2b, 0x67, 0x81 })
        {
            Assert.NotNull(dispatch.Registration(type).PacketObserver);
            Assert.Null(dispatch.Registration(type).Observer);
        }
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모는 버전 응답과 가변 길이 관찰기를 같은 분배기에 설치한다. 0.935에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsVersionReplyAndVariableObservers()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        Assert.NotNull(dispatch.Registration(0x0b).PacketObserver);
        Assert.Equal(1, dispatch.Registration(0x0b).Version);
        Assert.Equal(ushort.MaxValue, dispatch.Registration(0x0b).Length);
        Assert.Equal(0x004b3af0u, dispatch.SnapshotReceiver(0x0b)!.NativeEntry);
        foreach (uint type in new uint[] { 0x3f, 0x30, 0x7a, 0x5f, 0x7d })
        {
            Assert.NotNull(dispatch.Registration(type).PacketObserver);
            Assert.Null(dispatch.Registration(type).Observer);
            Assert.Equal(1, dispatch.Registration(type).Version);
            Assert.Equal(ushort.MaxValue, dispatch.Registration(type).Length);
        }
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모는 Carrier 15행·건설/연구 8행 고정 F를 같은 분배기에 설치한다. 0.964에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsCarrierAndConstructionEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        Assert.Equal(0x0048f7f0u, dispatch.SnapshotReceiver(0x77)!.NativeEntry);
        Assert.Equal(6, dispatch.Registration(0x77).Length);
        Assert.NotNull(dispatch.Registration(0x77).Observer);
        Assert.Null(dispatch.Registration(0x77).PacketObserver);
        Assert.Equal(0x00420bf0u, dispatch.SnapshotReceiver(0x5f)!.NativeEntry);
        Assert.Equal(0xffff, dispatch.Registration(0x5f).Length);
        Assert.NotNull(dispatch.Registration(0x5f).PacketObserver);
        Assert.Equal(0x004205f0u, dispatch.SnapshotReceiver(0x75)!.NativeEntry);
        Assert.Equal(0x00444af0u, dispatch.SnapshotReceiver(0x3e)!.NativeEntry);
        Assert.Equal(25, dispatch.Registration(0x3e).Length);
        Assert.NotNull(dispatch.Registration(0x3e).Observer);
        Assert.Equal(0x004441b0u, dispatch.SnapshotReceiver(0x3f)!.NativeEntry);
        Assert.NotNull(dispatch.Registration(0x3f).PacketObserver);
        Assert.Null(dispatch.Registration(0x37).Observer);
        Assert.Null(dispatch.Registration(0x37).PacketObserver);
        Assert.Equal(0x004427b0u, dispatch.SnapshotReceiver(0x37)!.NativeEntry);
        // 같은 묶음의 기존 설치가 Carrier/Construction 행에 밀리지 않는다.
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Equal(0x0045e4f0u, dispatch.SnapshotReceiver(0x74)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모 설치 뒤 BindSession·대여·반납하면 C3/관찰기/metadata가 부모 기준으로 복구된다. 0.964에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallPrecedesBindSessionAndLeaseRestoresBaseline()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        // 호출자의 BindSession보다 먼저 부모가 같은 기본 표를 채운다.
        using (var lease = dispatch.Registrations.BindSession(scene.Game))
        {
            NativeCarrierPacketReceivers.Bind(dispatch, 0x77);
            NativeConstructionPacketReceivers.BindReceiver(dispatch, 0x3e);
        }

        using (var lease = dispatch.Registrations.BindSession(scene.Game))
        {
            dispatch.RegisterCanonicalDirect(0x77, 0x9999, static (_, _, _) => { });
            dispatch.Version(0x77, 9, 9);
            dispatch.RegisterCanonicalDirect(0x3e, 0x9999, static (_, _, _) => { });
            dispatch.Observe(0x3e, null);
            lease.Detach();
            // 임대 반납 뒤 고정 F·관찰기·metadata가 부모 설치 기준으로 복구된다.
            Assert.Equal(0x0048f7f0u, dispatch.SnapshotReceiver(0x77)!.NativeEntry);
            Assert.Equal(1, dispatch.Registration(0x77).Version);
            Assert.Equal(0, dispatch.Registration(0x77).Mismatch);
            Assert.Equal(6, dispatch.Registration(0x77).Length);
            Assert.NotNull(dispatch.Registration(0x77).Observer);
            Assert.Null(dispatch.Registration(0x77).PacketObserver);
            Assert.Equal(0x00444af0u, dispatch.SnapshotReceiver(0x3e)!.NativeEntry);
            Assert.Equal(1, dispatch.Registration(0x3e).Version);
            Assert.Equal(25, dispatch.Registration(0x3e).Length);
            Assert.NotNull(dispatch.Registration(0x3e).Observer);
        }
    }
    /// <summary>부모는 장기 CS 묶음(0x7b·0x7c)을 같은 분배기에 설치한다. 0.966에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsChallengeEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        var peer = dispatch.Registration(0x7b);
        Assert.Equal(1, peer.Version);
        Assert.Equal(0, peer.Mismatch);
        Assert.Equal(ushort.MaxValue, peer.Length);
        Assert.Null(peer.Observer);
        Assert.NotNull(peer.PacketObserver);
        Assert.Equal(0x004b4790u, dispatch.SnapshotReceiver(0x7b)!.NativeEntry);
        var login = dispatch.Registration(0x7c);
        Assert.Equal(1, login.Version);
        Assert.Equal(0, login.Mismatch);
        Assert.Equal(28, login.Length);
        Assert.NotNull(login.Observer);
        Assert.Null(login.PacketObserver);
        Assert.Equal(0x004b2cc0u, dispatch.SnapshotReceiver(0x7c)!.NativeEntry);
        // 같은 묶음의 기존 설치가 장기 CS 행에 밀리지 않는다.
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Equal(0x0048f7f0u, dispatch.SnapshotReceiver(0x77)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>CS0이면 0x7b는 버퍼도 읽지 않고 조용히 돌아간다. 0.966에서 추가됐다.</summary>
    [Fact]
    public void ParentChallengeDefaultsIgnoresCsZero()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        byte[] packet = [1, 0x7b, 9, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff];
        dispatch.InvokeDirect(0x7b, 3, null, packet);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>같은 구독자면 0x7b는 참가자 주소 꼬리에 NUL까지 BYTE별 저장한다. 0.966에서 추가됐다.</summary>
    [Fact]
    public void ParentChallengeDefaultsCopiesPeerAddress()
    {
        var scene = new Scene();
        scene.Challenge.Prepare();
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Challenge.Participant(0).Span.Slice(4, 4), 0x111);
        var dispatch = Build(scene, _ => { });
        byte[] packet = [1, 0x7b, 12, 0, 0, 0, 0, 0, 0, 0, (byte)'A', 0];
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(5, 4), 0x111);
        dispatch.InvokeDirect(0x7b, 3, null, packet);
        Assert.Equal((byte)'A', scene.Challenge.Participant(0).Span[0x1a0]);
        Assert.Equal(0, scene.Challenge.Participant(0).Span[0x1a1]);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>상태 2이면 0x7c는 캐시한 DWORD만 쓰고 송신·연결을 건드리지 않는다. 0.966에서 추가됐다.</summary>
    [Fact]
    public void ParentMetaGameLoginClearsCachedPeerOnly()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        byte[] packet = new byte[28];
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 2);
        dispatch.InvokeDirect(0x7c, 7, null, packet);
        Assert.Equal(0xffffffffu, scene.Local.CachedPeerIndex.Value);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>상태 1에서 master가 없으면 0x7c는 그대로 돌아간다. 0.966에서 추가됐다.</summary>
    [Fact]
    public void ParentMetaGameLoginStatus1MasterMissingReturns()
    {
        var scene = new Scene();
        scene.Challenge.Prepare();
        var dispatch = Build(scene, _ => { });
        byte[] packet = new byte[28];
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 1);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), 0x5555);
        dispatch.InvokeDirect(0x7c, 7, null, packet);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>부모가 묶은 41h는 실제 Chat UI 입구로 전달한다.</summary>
    [Fact]
    public void ParentRelaysChatToReceive()
    {
        var scene = new Scene(); scene.Game.PlayerRecords.Initialize((_, _) => { }); scene.Subscribers.MySubscriber.Value = 100;
        scene.Surface.Storage.OwnerMode.Battle = true; scene.Game.Authority.Server = true; scene.Chat.Current.Value = 1;
        Put(scene.Game.PlayerRecords.Records.Span, 2 * 180 + 44, 200); scene.Game.PlayerRecords.Records.Span[2 * 180 + 0x50] = 1;
        int receives = 0;
        var dispatch = Build(scene, incoming => { receives++; Assert.Equal(100u, BinaryPrimitives.ReadUInt32LittleEndian(incoming.Span.Slice(20, 4))); });
        byte[] packet = new byte[64]; packet[0] = 3; packet[1] = 0x41; Put(packet, 2, 4, 2); Put(packet, 10, 0, 2); Put(packet, 12, 5, 2); packet[14] = 2; "005x\n"u8.CopyTo(packet.AsSpan(15)); Put(packet, 20, 200); Put(packet, 24, 100);
        dispatch.InvokeDirect(0x41, 9, null, packet);
        Assert.Equal(1, receives);
    }
    /// <summary>빈 장치는 묶지 않고 실제 소유자 불일치는 거부한다.</summary>
    [Fact]
    public void ParentRejectsNullDevicesAndMismatchedOwners()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientPacketDispatch.Build(null!, scene.Game, scene.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, ActivityHooks(), scene.Challenge));
        Assert.Throws<ArgumentNullException>(() => ClientPacketDispatch.Build(DispatchHooks(), scene.Game, scene.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, ActivityHooks(), null!));
        var other = new Scene();
        Assert.Throws<ArgumentException>(() => ClientPacketDispatch.Build(DispatchHooks(), scene.Game, other.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, ActivityHooks(), scene.Challenge));
    }
    /// <summary>부모는 연결 종료 2번 고정 F를 같은 분배기에 설치한다. observer/version/length 표는 바꾸지 않는다. 0.969에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsConnectionEndedEntry()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        Assert.Equal(0x004b6660u, dispatch.SnapshotReceiver(2)!.NativeEntry);
        Assert.Null(dispatch.Registration(2).Observer);
        Assert.Null(dispatch.Registration(2).PacketObserver);
        // 같은 묶음의 기존 설치가 2번 행에 밀리지 않는다.
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Equal(0x0048f7f0u, dispatch.SnapshotReceiver(0x77)!.NativeEntry);
        Assert.Equal(0x004b4790u, dispatch.SnapshotReceiver(0x7b)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>서버가 아니면 2번은 실제 세션 없이 성공으로 꾸미지 않는다. 0.969에서 추가됐다.</summary>
    [Fact]
    public void ParentConnectionEndedNonServerRequiresSession()
    {
        var scene = new Scene();
        scene.Game.Authority.Server = false;
        var dispatch = Build(scene, _ => { });
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(2, 9, null, new byte[4]));
        Assert.Contains("Runtime", error.Message);
    }
    /// <summary>서버이면 2번 소유자 방송도 실제 세션 없이 성공으로 꾸미지 않는다. 0.969에서 추가됐다.</summary>
    [Fact]
    public void ParentConnectionEndedServerRequiresSession()
    {
        var scene = new Scene();
        scene.Game.Authority.Server = true;
        scene.Game.PlayerRecords.Initialize((_, _) => { });
        scene.Connections.Activity.Frames[5] = 7;
        var dispatch = Build(scene, _ => { });
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(2, 5, null, new byte[4]));
        Assert.Contains("Runtime", error.Message);
    }
    /// <summary>부모는 전투 시작 1a/1b 고정 F를 같은 분배기에 설치한다. observer·버전·길이 표를 공유한다. 0.970에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsBattleStartEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
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
        // 같은 묶음의 기존 설치가 1a/1b 행에 밀리지 않는다.
        Assert.Equal(0x004b6660u, dispatch.SnapshotReceiver(2)!.NativeEntry);
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>1a는 next·7/포트·복제를 쓰고 10/11을 닫은 뒤 실제 UI State 부모 없이 성공으로 꾸미지 않는다. 요청이 없으면 기록을 열지 않는다. 0.970에서 추가됐다.</summary>
    [Fact]
    public void ParentBattleStartZoneWritesNextWithoutRecord()
    {
        var scene = new Scene();
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Local.State.Phase.Span.Slice(0, 4), 29);
        using var transport = MakeOwnedConnection(scene, out PacketConnection connection);
        scene.Game.Authority.Server = false;
        var dispatch = Build(scene, _ => { });
        // 원문 순서대로 next7 인계 앞에서 실제 UI State 부모 요구로 멈춘다.
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x1a, 9, connection, ZonePacket(1, 0x10)));
        Assert.Contains("UI State", error.Message);
        Assert.Equal(1u, scene.Local.ZonePending.Value);
        Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Next.Span.Slice(0, 4)));
        Assert.Equal(6799u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Next.Span.Slice(0x7c, 4)));
        Assert.NotEqual(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Next.Span.Slice(0x80, 4)));
        Assert.Empty(scene.Assertions);
    }
    /// <summary>1a는 요청이 있으면 packet+10과 구독자로 기록을 연다. Root 기록 갈래를 대조한다. 0.970에서 추가됐다.</summary>
    [Fact]
    public void ParentBattleStartZoneOpensRecordWhenRequested()
    {
        var scene = new Scene();
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Local.State.Phase.Span.Slice(0, 4), 33);
        scene.Subscribers.MySubscriber.Value = 0x1234;
        using var transport = MakeOwnedConnection(scene, out PacketConnection connection);
        scene.Game.Authority.Server = false;
        var names = new List<string>();
        var dispatch = Build(scene, _ => { }, () => 1u, names.Add);
        // 기록을 연 뒤 next7 인계 앞에서 실제 UI State 부모 요구로 멈춘다.
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x1a, 9, connection, ZonePacket(1, 0x00ABCDEF)));
        Assert.Contains("UI State", error.Message);
        Assert.Equal(["00ABCDEF.1234.c.dat"], names);
    }
    /// <summary>1b는 실제 Source까지 닿은 뒤 실제 UI State 부모 없이 성공으로 꾸미지 않는다. 0.970에서 추가됐다.</summary>
    [Fact]
    public void ParentBattleStartPeerRequiresMenu()
    {
        var scene = new Scene();
        BinaryPrimitives.WriteUInt32LittleEndian(scene.Local.State.Phase.Span.Slice(0, 4), 29);
        using var transport = MakeOwnedConnection(scene, out PacketConnection connection);
        scene.Game.Authority.Server = false;
        var dispatch = Build(scene, _ => { });
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x1b, 9, connection, PeerPacket(0x20, "x\0")));
        Assert.Contains("UI State", error.Message);
    }
    /// <summary>원문 0x66 12BYTE 입력 패킷을 조립한다.</summary>
    private static byte[] DiscoveryPacket(uint command, uint value)
    {
        // 명령과 값을 같은 위치에 둔다.
        byte[] packet = new byte[12];
        packet[1] = 0x66;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), command);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), value);
        return packet;
    }
    /// <summary>부모는 로컬 서버 발견 0x66·메타 geometry 0x26 고정 F를 같은 분배기에 설치한다. 0.971에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsDiscoveryAndGeometryEntries()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        var discovery = dispatch.Registration(0x66);
        Assert.Equal(1, discovery.Version);
        Assert.Equal(0, discovery.Mismatch);
        Assert.Equal(12, discovery.Length);
        Assert.NotNull(discovery.Observer);
        Assert.Null(discovery.PacketObserver);
        Assert.Equal(0x004b8a10u, dispatch.SnapshotReceiver(0x66)!.NativeEntry);
        var geometry = dispatch.Registration(0x26);
        Assert.Equal(1, geometry.Version);
        Assert.Equal(0, geometry.Mismatch);
        Assert.Equal(ushort.MaxValue, geometry.Length);
        Assert.NotNull(geometry.PacketObserver);
        Assert.Null(geometry.Observer);
        Assert.Equal(0x004b45a0u, dispatch.SnapshotReceiver(0x26)!.NativeEntry);
        // 같은 묶음의 기존 설치가 0x66·0x26 행에 밀리지 않는다.
        Assert.Equal(0x004b4df0u, dispatch.SnapshotReceiver(0x1a)!.NativeEntry);
        Assert.Equal(0x004b6660u, dispatch.SnapshotReceiver(2)!.NativeEntry);
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>0x66 명령 0은 값이 토큰보다 signed로 작을 때만 실제 기한을 12초 연장한다. 메뉴를 요구하지 않는다. 0.971에서 추가됐다.</summary>
    [Fact]
    public void ParentDiscoveryExtendsSearchOnlyWhenTokenGreater()
    {
        var scene = new Scene();
        scene.Local.LocalServerToken.Value = 200;
        var dispatch = Build(scene, _ => { });
        dispatch.InvokeDirect(0x66, 9, null, DiscoveryPacket(0, 100));
        Assert.Equal(112.0, scene.Local.LocalServerSearchEnd.Value);
        var later = new Scene();
        later.Local.LocalServerToken.Value = 50;
        var next = Build(later, _ => { });
        next.InvokeDirect(0x66, 9, null, DiscoveryPacket(0, 100));
        Assert.Equal(0.0, later.Local.LocalServerSearchEnd.Value);
    }
    /// <summary>0x66 명령 1은 현재 36·주소·포트를 쓴 뒤 실제 UI State 부모 없이 성공으로 꾸미지 않는다. 0.971에서 추가됐다.</summary>
    [Fact]
    public void ParentDiscoveryCommandRequiresMenu()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x66, 9, null, DiscoveryPacket(1, 0x4444)));
        Assert.Contains("UI State", error.Message);
        Assert.Equal(36u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Current.Span.Slice(0, 4)));
        Assert.Equal(0x4444u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Current.Span.Slice(0x30, 4)));
    }
    /// <summary>0x26은 화면 한계 앞에서 실제 화면 장치 없이 성공으로 꾸미지 않는다. 0.971에서 추가됐다.</summary>
    [Fact]
    public void ParentGeometryRequiresScreen()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        byte[] packet = new byte[16];
        packet[1] = 0x26;
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x26, 9, null, packet));
        Assert.Contains("화면 한계", error.Message);
    }
    /// <summary>원문 0x1d 13BYTE 입력 패킷을 조립한다.</summary>
    private static byte[] ReplyPacket(uint major, uint minor, uint subscriber, byte code)
    {
        // 종류·버전·구독자·응답코드를 같은 위치에 둔다.
        byte[] packet = new byte[13];
        packet[1] = 0x1d;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), unchecked((ushort)major));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), unchecked((ushort)minor));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(8, 4), subscriber);
        packet[12] = code;
        return packet;
    }
    /// <summary>부모는 챌린지 로그인 응답 0x1d 고정 F를 같은 분배기에 설치한다. 0.973에서 추가됐다.</summary>
    [Fact]
    public void ParentInstallsChallengeReplyEntry()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        var reply = dispatch.Registration(0x1d);
        Assert.Equal(1, reply.Version);
        Assert.Equal(0, reply.Mismatch);
        Assert.Equal(13, reply.Length);
        Assert.NotNull(reply.Observer);
        Assert.Null(reply.PacketObserver);
        Assert.Equal(0x004b8ac0u, dispatch.SnapshotReceiver(0x1d)!.NativeEntry);
        // 같은 묶음의 기존 설치가 0x1d 행에 밀리지 않는다.
        Assert.Equal(0x004b8a10u, dispatch.SnapshotReceiver(0x66)!.NativeEntry);
        Assert.Equal(0x004b45a0u, dispatch.SnapshotReceiver(0x26)!.NativeEntry);
        Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry);
        Assert.Empty(scene.Assertions);
    }
    /// <summary>0x1d 일치 버전은 구독자·28을 쓰고 동기 Run 앞에서 실제 UI State 부모 요구로 멈춘다. 0.973에서 추가됐다.</summary>
    [Fact]
    public void ParentChallengeReplyWritesStateOnMatchingVersion()
    {
        var scene = new Scene();
        var logs = new List<(uint Code, string Text)>();
        var tells = new List<string>();
        var dispatch = Build(scene, _ => { }, config: _ => 0, diagnosticLog: (code, text) => logs.Add((code, text)), tell: tells.Add);
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x1d, 9, null, ReplyPacket(10, 78, 0x4444, 3)));
        Assert.Contains("UI State", error.Message);
        Assert.Equal(0x4444u, scene.Subscribers.MySubscriber.Value);
        Assert.Equal(28u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Current.Span.Slice(0, 4)));
        Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Current.Span.Slice(0x38, 4)));
        Assert.Contains(logs, e => e.Code == 0x1d && e.Text.Contains("ZChalLoginReply"));
        Assert.Empty(tells);
    }
    /// <summary>0x1d 주 버전 불일치는 12·0을 쓰고 Tell 뒤 동기 Run 앞에서 멈춘다. 0.973에서 추가됐다.</summary>
    [Fact]
    public void ParentChallengeReplyTellsOnMajorMismatch()
    {
        var scene = new Scene();
        var logs = new List<(uint Code, string Text)>();
        var tells = new List<string>();
        var dispatch = Build(scene, _ => { }, config: _ => 0, diagnosticLog: (code, text) => logs.Add((code, text)), tell: tells.Add);
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x1d, 9, null, ReplyPacket(11, 0, 0x5555, 7)));
        Assert.Contains("UI State", error.Message);
        Assert.Equal(12u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Current.Span.Slice(0, 4)));
        Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(scene.Local.State.Current.Span.Slice(0x38, 4)));
        Assert.Contains(logs, e => e.Code == 0x1d && e.Text.Contains("VERSION DIFFERENCE"));
        Assert.Equal(["MetaVerDiff(11.0)"], tells);
    }
    /// <summary>0x1d은 진단 로그·설정·Tell 없이 성공으로 꾸미지 않는다. 0.973에서 추가됐다.</summary>
    [Fact]
    public void ParentChallengeReplyRequiresOwners()
    {
        var scene = new Scene();
        var dispatch = Build(scene, _ => { });
        var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x1d, 9, null, ReplyPacket(10, 78, 0x4444, 3)));
        Assert.Contains("진단 로그", error.Message);
        var logged = new List<(uint Code, string Text)>();
        var configured = Build(scene, _ => { }, diagnosticLog: (code, text) => logged.Add((code, text)));
        var configError = Assert.Throws<InvalidOperationException>(() => configured.InvokeDirect(0x1d, 9, null, ReplyPacket(11, 0, 0x5555, 7)));
        Assert.Contains("설정", configError.Message);
        var untold = Build(scene, _ => { }, config: _ => 0, diagnosticLog: (code, text) => logged.Add((code, text)));
        var tellError = Assert.Throws<InvalidOperationException>(() => untold.InvokeDirect(0x1d, 9, null, ReplyPacket(11, 0, 0x5555, 7)));
        Assert.Contains("Tell", tellError.Message);
    }
}
