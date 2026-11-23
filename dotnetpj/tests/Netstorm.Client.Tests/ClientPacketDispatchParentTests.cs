using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client.Tests;

// 공통 패킷 분배 부모의 41h·로그인·Native gameplay 설치를 검사하는 클래스다. 2026-11-23 0.897에서 R4-6 전송·수신 묶음용으로 추가하고 0.899에서 Native gameplay 묶음을 이었다.
// 근거: R4-6 남음의 "공통 41h가 Root.ReceiveNativePacketChat을 부르는 전송 설치"와 "NativeLoginPacketReceivers의 실제 MonoGame Root 설치 호출"이다.
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
    private static RawPacketDispatch Build(Scene scene, Action<Memory<byte>> receive) => ClientPacketDispatch.Build(
        DispatchHooks(), scene.Game, scene.Connections, scene.Surface, scene.Chat, receive,
        (expression, file, line) => scene.Assertions.Add(line), ActivityHooks());
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
        Assert.Throws<ArgumentNullException>(() => ClientPacketDispatch.Build(null!, scene.Game, scene.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, ActivityHooks()));
        var other = new Scene();
        Assert.Throws<ArgumentException>(() => ClientPacketDispatch.Build(DispatchHooks(), scene.Game, other.Connections, scene.Surface, scene.Chat, _ => { }, (_, _, _) => { }, ActivityHooks()));
    }
}
