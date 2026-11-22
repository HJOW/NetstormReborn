using System.Runtime.InteropServices;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 장기 Root 전송의 현재 Player 관문·공유 버퍼·Source 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>ClientMain의 첫 Squid 송신/선택 장치를 실제 내부 전송과 결합한 검사다.</summary>
public sealed class ClientSurfaceTransmissionTests
{
    /// <summary>게임 실행 없이 같은 Client 물리와 실제 내부 Source를 준비한다.</summary>
    private sealed class Scene : IDisposable
    {
        /// <summary>현재 Player·선택 값을 소유하는 장기 전역이다.</summary>
        public ClientGameplayGlobals Game { get; } = new();
        /// <summary>같은 Template 로딩 깊이 owner다.</summary>
        public CampaignTemplateStorage Templates { get; } = new();
        /// <summary>현재 실제 Registry와 약한 Source 장부다.</summary>
        public ClientConnectionGlobals Connections { get; }
        /// <summary>제품과 같은 최초 설치 경로의 Surface다.</summary>
        public ClientSurfaceGlobals Surface { get; }
        /// <summary>실제 선택 해제의 선택된 위치만 관찰한다.</summary>
        public Action Clear { get; set; } = () => throw new IOException("미선택 선택 해제");
        /// <summary>실제 송신 장치가 받은 선언 길이 바이트다.</summary>
        public List<byte[]> Sent { get; } = [];
        /// <summary>열린 같은 내부 Source이며 종료 때 현재 노드를 먼저 반납한다.</summary>
        private CompositeMissionTransport? _source;

        /// <summary>장치 설치 중 상태를 조회하지 않고 Process 생성 앞에 전송/선택 함수를 연결한다.</summary>
        public Scene(bool initializePlayers = true)
        {
            var local = new ClientLocalStateGlobals(Game, new()); Connections = new(local);
            Surface = new(Game, new(local), new(), () => throw new IOException("설치 중 시각 조회"), installDevices: surface =>
            {
                surface.Storage.BindSerializationGlobals(Templates.SerializationGlobals);
                surface.BindDisplayDiagnostics((_, _, _) => throw new IOException("미선택 원본 진단"));
                surface.BindNativeTransmission(Connections, () => Clear());
            });
            if (initializePlayers) Game.PlayerRecords.Initialize((_, _) => { });
        }

        /// <summary>첫 자산 표를 설치해 실제 Squid writer와 공장을 사용할 수 있게 한다.</summary>
        public int Squid()
        {
            var types = Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(index), FootX = 1, FootY = 1 }).ToArray();
            Surface.Storage.InitializeAssetTables(types, Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray(), Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray());
            Game.GameAux.SquidTransmission.Compress = 0; Game.GameAux.SquidTransmission.ClientSidAllowed = 1;
            return Surface.Storage.FactoryExecutor().Create(74);
        }

        /// <summary>Player2와 Client9의 실제 내부 양끝을 열며 observer만 주입한다.</summary>
        public CompositeMissionTransport Open(Action<Memory<byte>, uint>? observe = null)
        {
            Game.Authority.Server = true;
            _source = new(CompositeTransportRole.InternalMission, Game.PlayerRecords.Records, () => Game.Authority.Server,
                new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { }, Players = () => Game.PlayerRecords.Records, Sent = (_, packet, length) => Sent.Add(packet.Span[..length].ToArray()) }, Connections.Registry, Game);
            Connections.PacketSources.Publish(_source);
            _source.InternalTransport.RegisterLogin((_, _, _) => throw new IOException("미선택 로그인"));
            _source.Serve(6799); _source.Connect(new byte[20], 6799); Assert.Equal(0, _source.Poll()); _source.Assign(-1, 2);
            if (observe is not null) { _source.Dispatch.ObservePacket(0x24, observe); _source.Dispatch.ObservePacket(0x2c, observe); }
            return _source;
        }

        /// <summary>같은 장기 Player의 활성 BYTE만 검사 입력으로 바꾼다.</summary>
        public void Active(int owner, bool active) => Game.PlayerRecords.Records.Span[owner * 180 + 0x50] = active ? (byte)1 : (byte)0;
        /// <summary>같은2048BYTE 버퍼의 앞5BYTE만 테스트 메시지로 쓴다.</summary>
        public void Broadcast()
        {
            byte[] buffer = Game.GameAux.SquidTransmission.Buffer;
            new byte[] { 1, 0x24, 5, 0, 77 }.CopyTo(buffer, 0);
            Surface.Storage.BroadcastProcessForm(5);
        }
        /// <summary>검사용 별칭을 해제한 뒤 실제 노드와 Source의 외부 수명을 반납한다.</summary>
        public void Dispose()
        {
            Connections.Registry.Cleanup().Transceivers[3] = 0;
            if (_source is { Released: false } source) { source.CloseRoom(); source.Dispose(); }
        }
    }

    /// <summary>첫 설치는 미초기화 Player·시계·선택을 읽지 않으며 조회는 현재 전체 DWORD를 보존한다.</summary>
    [Fact]
    public void FirstInstallationPreservesUninitializedPlayersAndCurrentSelection()
    {
        using var scene = new Scene(false);
        Assert.True(scene.Surface.Storage.UsesNativeTransmissionDevices()); Assert.True(scene.Surface.Processes.HasDevices());
        Assert.False(scene.Game.PlayerRecords.Initialized); Assert.False(scene.Game.Borrowed());
        scene.Game.Selection.Selected.Value = 0xfedcba98; Assert.Equal(0xfedcba98u, scene.Surface.Storage.CurrentProcessSelection());
        scene.Game.Selection.Selected.Value = 7; Assert.Equal(7u, scene.Surface.Storage.CurrentProcessSelection());
        Assert.Throws<ArgumentOutOfRangeException>(scene.Broadcast);
    }

    /// <summary>활성 Player의 NULL 연결도 Source나 Runtime을 요구하지 않고 자연 반환한다.</summary>
    [Fact]
    public void NullConnectionsAndInactiveTargetsNeedNoSourceOrRuntime()
    {
        using var scene = new Scene(); scene.Active(2, true); scene.Broadcast();
        int sid = scene.Squid(); var writer = scene.Surface.Storage.TransmissionExecutor();
        writer.SendTo(sid, 2, 0); writer.SendTo(sid, 0, 0); writer.SendTo(sid, 9, 0);
        Assert.Empty(scene.Sent); Assert.False(scene.Game.Borrowed());
    }

    /// <summary>같은 전체 버퍼를 전달하며 방송 도중 바뀐 로컬 owner와 다음 Player flag를 다시 읽는다.</summary>
    [Theory]
    [InlineData(true)] [InlineData(false)]
    public void BroadcastRereadsCurrentOwnerAndNextPlayerAfterObserver(bool changeLocalOwner)
    {
        using var scene = new Scene(); scene.Active(2, true); scene.Active(3, true); scene.Game.Player.BattleOwner = 1;
        byte[] buffer = scene.Game.GameAux.SquidTransmission.Buffer; buffer[2047] = 0xa5; int observed = 0;
        var source = scene.Open((packet, sending) =>
        {
            Assert.Equal(1u, sending); Assert.Equal(2048, packet.Length);
            Assert.True(MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)packet, out var segment)); Assert.Same(buffer, segment.Array);
            Assert.Equal(0xa5, packet.Span[2047]); packet.Span[4]++; observed++;
            if (changeLocalOwner) scene.Game.Player.BattleOwner = 3; else scene.Active(3, false);
        });
        scene.Connections.Registry.Cleanup().Transceivers[3] = scene.Connections.Registry.TokenFor(source.InternalTransport.Client);
        scene.Broadcast(); Assert.Equal(1, observed); Assert.Equal(new byte[] { 1, 0x24, 5, 0, 78 }, Assert.Single(scene.Sent));
        Assert.Equal(0xa5, buffer[2047]); Assert.False(scene.Game.Borrowed());
    }

    /// <summary>방송의 이미 고른 가상+4는 로컬 번호와 활성 관문을 중복 적용하지 않는다.</summary>
    [Fact]
    public void BroadcastPassesSameBufferAcrossBothActualEndpoints()
    {
        using var scene = new Scene(); scene.Active(2, true); scene.Active(3, true); scene.Game.Player.BattleOwner = 1;
        int observed = 0; var source = scene.Open((packet, _) => { packet.Span[4]++; observed++; });
        scene.Connections.Registry.Cleanup().Transceivers[3] = scene.Connections.Registry.TokenFor(source.InternalTransport.Client);
        scene.Broadcast(); Assert.Equal(2, observed); Assert.Equal(new byte[] { 78, 79 }, scene.Sent.Select(packet => packet[4]).ToArray());
    }

    /// <summary>대상 writer는 현재 활성 상태와 실제 Source를 사용하고 observer 예외 뒤에도 수명을 반납한다.</summary>
    [Fact]
    public void TargetUsesActualSourceAndObserverExceptionReleasesPins()
    {
        using var scene = new Scene(); int sid = scene.Squid(); int observed = 0;
        var failure = new IOException("선택된 observer 실패"); bool fail = true;
        var source = scene.Open((packet, _) => { Assert.Equal(2048, packet.Length); packet.Span[2] = 35; observed++; if (fail) throw failure; });
        var writer = scene.Surface.Storage.TransmissionExecutor();
        writer.SendTo(sid, 2, 0); Assert.Equal(0, observed);
        scene.Active(2, true); Assert.Same(failure, Assert.Throws<IOException>(() => writer.SendTo(sid, 2, 0)));
        fail = false; writer.SendTo(sid, 2, 0); Assert.Equal(2, observed); Assert.Equal(35, Assert.Single(scene.Sent).Length);
        source.CloseRoom(); source.Dispose(); Assert.True(source.Released);
    }

    /// <summary>빈 메뉴 선택 정리는 World를 만들지 않으며 실제 Session이 있으면 전체 Runtime을 호출 끝까지 고정한다.</summary>
    [Fact]
    public void ClearSelectionPinsOnlyActualBorrowedRuntime()
    {
        using var scene = new Scene(); int calls = 0;
        scene.Clear = () => { calls++; Assert.False(scene.Game.Borrowed()); };
        var clear = new SquidDestroyEvent(SquidDestroyEffect.ClearSelection, 5, 0, 0);
        scene.Surface.Storage.DefaultProcessAssetHooks().Emit!(clear); Assert.Equal(1, calls);
        var session = new HostSession(scene.Surface.Kernel, new(17), new(23), sharedSurface: scene.Surface, sharedTemplates: scene.Templates);
        var runtime = HostMissionRuntime.Create(session, scene.Surface.Storage);
        scene.Clear = () =>
        {
            calls++; Assert.True(scene.Game.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released);
            Assert.Throws<InvalidOperationException>(session.ReleasePlayerRecordViews);
        };
        scene.Surface.Storage.DefaultProcessAssetHooks().Emit!(clear); Assert.Equal(2, calls); Assert.True(runtime.Released);
        session.ReleasePlayerRecordViews(); Assert.False(scene.Game.Borrowed());
    }

    /// <summary>다른 Client 연결표는 최초 장치 쓰기 전에 거부한다.</summary>
    [Fact]
    public void RejectsDifferentClientBeforeInstallingTransmission()
    {
        var game = new ClientGameplayGlobals(); var local = new ClientLocalStateGlobals(game, new()); var other = new ClientConnectionGlobals(new(new(), new()));
        RawSurfaceStorage? prefix = null;
        Assert.Throws<ArgumentException>(() => new ClientSurfaceGlobals(game, new(local), new(), () => 0, installDevices: surface =>
        {
            prefix = surface.Storage; surface.BindNativeTransmission(other, () => throw new IOException("거부 중 선택 금지"));
        }));
        Assert.NotNull(prefix); Assert.False(prefix.UsesNativeTransmissionDevices());
    }
}
