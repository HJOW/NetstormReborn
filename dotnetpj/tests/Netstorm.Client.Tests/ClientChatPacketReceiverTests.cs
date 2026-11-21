using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Net;

// 실제 장기 전역/등록표/내부 전송을 통해41h 부모를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 Client41h의 설치/지연 UI/현재 Player와 실제 Registry 송신을 확인한다.</summary>
public sealed class ClientChatPacketReceiverTests
{
    /// <summary>같은 실제 Client/표면/연결을 조립하며 설치 전에는 Player를 할당하지 않는다.</summary>
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
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 원문 비정렬 필드 폭만 낮은 BYTE부터 기록한다.
        for (int index = 0; index < width; index++) bytes[at + index] = (byte)(value >> (index * 8));
    }
    /// <summary>선택하지 않는 외부 종료 장치를 추가로 실행하지 않는 실제 등록표다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
    /// <summary>설치에서 할당/현재 모드 읽기를 요구하지 않고 root0이면 본문/UI를 읽지 않는다.</summary>
    [Fact]
    public void InstallationKeepsFreshOwnersAndRootZeroSkipsPayload()
    {
        var scene = new Scene(); scene.Game.Authority.Server = false; scene.Surface.Storage.OwnerMode.Battle = false; var dispatch = Dispatch(); ClientChatPacketReceiver.Install(scene.Game, scene.Surface, scene.Connections, scene.Chat, dispatch, null); Assert.Equal(0u, scene.Game.PlayerRecords.Base); Assert.Equal(0x0042dc90u, dispatch.SnapshotReceiver(0x41)!.NativeEntry); dispatch.InvokeDirect(0x41, 9, null, ReadOnlyMemory<byte>.Empty); scene.Chat.Current.Value = 1; Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x41, 9, null, ReadOnlyMemory<byte>.Empty));
        var other = new Scene(); Assert.Throws<ArgumentException>(() => ClientChatPacketReceiver.Install(scene.Game, other.Surface, scene.Connections, scene.Chat, dispatch, null)); Assert.Equal(0u, scene.Game.PlayerRecords.Base);
    }
    /// <summary>저장된 같은 F도 현재 모드/마스크/root를 읽고 동일 부분 물리에 원문 변경을 보존한다.</summary>
    [Fact]
    public void SavedFunctionUsesCurrentGlobalsAndSamePartialPacket()
    {
        var scene = new Scene(); scene.Game.Authority.Server = false; scene.Surface.Storage.OwnerMode.Battle = true; var dispatch = Dispatch(); int receives = 0;
        ClientChatPacketReceiver.Install(scene.Game, scene.Surface, scene.Connections, scene.Chat, dispatch, incoming => { receives++; incoming.Span[0] = 77; }); var saved = dispatch.SnapshotReceiver(0x41)!; dispatch.RegisterCanonicalDirect(0x41, 5, (_, _, _) => throw new InvalidOperationException("현재 표의 다른 F")); byte[] all = Enumerable.Repeat((byte)0xd3, 40).ToArray(); Memory<byte> packet = all.AsMemory(4, 32); Put(packet.Span, 10, 0, 2); Put(packet.Span, 12, 5, 2); "032x\n"u8.CopyTo(packet.Span[15..]); saved.Invoke(9, null, packet); Assert.Equal(32u, scene.Chat.ReceivedMask.Value); Assert.Equal(0, packet.Span[19]); Assert.Equal(0, receives);
        scene.Surface.Storage.OwnerMode.Battle = false; scene.Chat.Current.Value = 9; saved.Invoke(9, null, packet); Assert.Equal(1, receives); Assert.Equal(77, all[4]); Assert.All(all.Take(4), value => Assert.Equal(0xd3, value)); Assert.All(all.Skip(36), value => Assert.Equal(0xd3, value)); Assert.Equal(0u, scene.Game.PlayerRecords.Base);
    }
    /// <summary>실제 내부 Source/Registry 노드와 현재 Player로 원격 전달 및 같은 로컬 수신을 실행한다.</summary>
    [Fact]
    public void ActualInternalSourceAndPlayerRegistryRelayWithoutCopies()
    {
        var scene = new Scene(); scene.Game.PlayerRecords.Initialize((_, _) => { }); scene.Subscribers.MySubscriber.Value = 100; scene.Surface.Storage.OwnerMode.Battle = true; scene.Game.Authority.Server = true; scene.Chat.Current.Value = 1; Put(scene.Game.PlayerRecords.Records.Span, 2 * 180 + 44, 200); scene.Game.PlayerRecords.Records.Span[2 * 180 + 0x50] = 1; var sent = new List<byte[]>(); int receives = 0;
        using var source = new CompositeMissionTransport(CompositeTransportRole.InternalMission, scene.Game.PlayerRecords.Records, () => scene.Game.Authority.Server,
            new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { }, Sent = (_, emitted, _) => { if (emitted.Span[1] == 0x41) sent.Add(emitted.ToArray()); }, Players = () => scene.Game.PlayerRecords.Records }, scene.Connections.Registry, scene.Game);
        scene.Connections.PacketSources.Publish(source);
        try
        {
            source.InternalTransport.RegisterLogin((_, _, _) => throw new InvalidOperationException("이 입력에 없는 로그인")); source.Serve(6799); source.Connect(new byte[20], 6799); Assert.Equal(0, source.Poll()); source.Assign(-1, 2);
            ClientChatPacketReceiver.Install(scene.Game, scene.Surface, scene.Connections, scene.Chat, source.Dispatch, incoming => { receives++; Assert.Equal(100u, BinaryPrimitives.ReadUInt32LittleEndian(incoming.Span.Slice(20, 4))); });
            byte[] packet = new byte[64]; packet[0] = 3; packet[1] = 0x41; Put(packet, 2, 4, 2); Put(packet, 10, 0, 2); Put(packet, 12, 5, 2); packet[14] = 2; "005x\n"u8.CopyTo(packet.AsSpan(15)); Put(packet, 20, 200); Put(packet, 24, 100); source.Dispatch.InvokeDirect(0x41, 9, null, packet); source.Flush();
            Assert.Equal(1, receives); Assert.Equal(5u, scene.Chat.ReceivedMask.Value); Assert.Equal(200u, BinaryPrimitives.ReadUInt32LittleEndian(Assert.Single(sent).AsSpan(20, 4))); Assert.Equal(1, packet[14]); Assert.Empty(scene.Assertions);
        }
        finally { source.CloseRoom(); }
    }
}
