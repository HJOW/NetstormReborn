using System.Buffers;
using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 장기 저장소/원문 고정 F와 실제 내부 Source의 버전 응답을 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>세션 없는 설정/observer 및 실제 Registry 송신의 공통 기본 결선이다.</summary>
public sealed class ClientMissionPacketDefaultsTests
{
    /// <summary>현재 실제7DWORD를 BYTE 물리로 복사 없이 빌린다.</summary>
    private sealed class Words(uint[] words) : MemoryManager<byte>
    {
        /// <summary>같은 장기 DWORD 배열이다.</summary>
        private readonly uint[] _words = words;
        /// <summary>실제28BYTE의 동일 별칭이다.</summary>
        public Memory<byte> Bytes => CreateMemory(_words.Length * 4);
        /// <summary>현재 같은 DWORD 물리를 BYTE로 해석한다.</summary>
        public override Span<byte> GetSpan() => MemoryMarshal.AsBytes(_words.AsSpan());
        /// <summary>호스트 포인터 노출은 필요하지 않다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException();
        /// <summary>고정한 포인터가 없다.</summary>
        public override void Unpin() => throw new NotSupportedException();
        /// <summary>배열 소유권은 실제 GameAux에 있으므로 반납하지 않는다.</summary>
        protected override void Dispose(bool disposing) { }
    }
    /// <summary>같은 실제 Client 장기 소유자를 구성한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 현재 게임/보조 전역이다.</summary>
        public readonly ClientGameplayGlobals Game = new();
        /// <summary>같은 게임에 속한 현재 LocalState다.</summary>
        public readonly ClientLocalStateGlobals Local;
        /// <summary>같은 현재 Registry와 Source 장부다.</summary>
        public readonly ClientConnectionGlobals Connections;
        /// <summary>같은 GameAux/진단/물리 표면이다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>선택된 실제 진단 줄의 관측이다.</summary>
        public readonly List<uint> Assertions = [];
        /// <summary>Player/Session/소켓/창을 만들지 않고 실제 장기 객체만 잇는다.</summary>
        public Scene() { Local = new(Game, new()); Connections = new(Local); Surface = new(Game, new(Local), new Kernel(), () => 1000); Surface.BindDisplayDiagnostics((_, _, line) => Assertions.Add(line)); }
    }
    /// <summary>현재 원문 지정 폭만 낮은 BYTE부터 기록한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 주변 물리를 보존한다.
        for (int index = 0; index < width; index++) bytes[offset + index] = (byte)(value >> (8 * index));
    }
    /// <summary>실제 원문 표만 관찰하며 이 입력에서 선택되지 않는 전송 효과를 추가하지 않는다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
    /// <summary>32h는 동일7DWORD를 쓰며 입력 별칭/부족에서도 원문 순서의 이전 store를 보존한다.</summary>
    [Fact]
    public void SettingsFixedFunctionUsesSameAuxAndPreservesPartialStores()
    {
        var scene = new Scene(); var aux = scene.Game.GameAux; Assert.Equal(new uint[] { 1, 0, 0, 0, 0, 1, 8 }, aux.MissionSettings); Assert.Equal(new byte[] { 0x0c, 1 }, aux.Packet0cHeader); Assert.Equal(0, aux.Packet0cInitialLength); Assert.Same(aux, scene.Surface.Storage.GameAux());
        var dispatch = Dispatch(); ClientMissionPacketDefaults.InstallSettings(dispatch, scene.Game); var saved = dispatch.SnapshotReceiver(0x32)!; Assert.Equal(0x0044dc20u, saved.NativeEntry); Assert.NotNull(dispatch.Registration(0x32).Observer); Assert.Null(dispatch.Registration(0x32).PacketObserver); Assert.Equal(32, dispatch.Registration(0x32).Length);
        byte[] packet = new byte[32];
        // 각 설정에 서로 다른 DWORD를 넣어 같은 실제 배열의 쓰기를 구별한다.
        for (int index = 0; index < 7; index++) Put(packet, 4 + index * 4, (uint)(11 + index));
        dispatch.InvokeDirect(0x32, uint.MaxValue, null, packet); Assert.Equal(new uint[] { 11, 12, 13, 14, 15, 16, 17 }, aux.MissionSettings); Assert.Equal(0u, scene.Game.PlayerRecords.Base);
        dispatch.RegisterCanonicalDirect(0x32, 0x9999, (_, _, _) => throw new InvalidOperationException("새 표의 다른 설정 F"));
        // 원문과 동일한 겹치는 입력에서 부족한 마지막DWORD 전까지의 store를 남긴다.
        for (int index = 0; index < 7; index++) aux.MissionSettings[index] = (uint)(index + 1);
        using var aliases = new Words(aux.MissionSettings); Assert.Throws<ArgumentOutOfRangeException>(() => saved.Invoke(0, null, aliases.Bytes)); Assert.Equal(new uint[] { 2, 3, 4, 5, 6, 7, 7 }, aux.MissionSettings); Assert.Same(aux, scene.Game.GameAux);
    }
    /// <summary>네 observer만 교체하고 metadata/F를 보존하며 설치 후 연결한 현재 진단을 사용한다.</summary>
    [Fact]
    public void MissionObserversKeepReceiversAndUseCurrentDiagnostic()
    {
        var scene = new Scene(); var dispatch = Dispatch(); uint[] types = [0x2a, 0x2b, 0x67, 0x81]; int[] offsets = [12, 51, 5, 4], widths = [4, 1, 2, 4]; uint[] lines = [0x672, 0x695, 0xa07, 0xc3d]; ushort[] lengths = [19, 61, 9, 16]; var saved = new CanonicalPacketReceiver?[4];
        // 임의의 기존 metadata와 실제 고정 F를 보존하는지 관찰한다.
        for (int index = 0; index < types.Length; index++) { uint type = types[index]; dispatch.RegisterCanonicalDirect(type, 0x1000 + type, (_, _, _) => { }); saved[index] = dispatch.SnapshotReceiver(type); dispatch.Version(type, 7, 9); dispatch.RequireLength(type, 53); dispatch.Observe(type, _ => throw new InvalidOperationException("교체 전 observer")); }
        ClientMissionPacketDefaults.InstallMissionSendObservers(dispatch, scene.Surface.Storage);
        // 각 현재 진단에서 필드를 바꾸며 그 뒤의 길이 계산과 미선택 수신을 확인한다.
        for (int index = 0; index < types.Length; index++)
        {
            uint type = types[index]; var row = dispatch.Registration(type); Assert.Same(saved[index], dispatch.SnapshotReceiver(type)); Assert.Equal(7, row.Version); Assert.Equal(9, row.Mismatch); Assert.Equal(53, row.Length); row.NotifyObserver(Memory<byte>.Empty, 0);
            byte[] bytes = new byte[80]; Put(bytes, offsets[index], uint.MaxValue, widths[index]); int called = 0;
            scene.Surface.BindDisplayDiagnostics((_, _, line) => { Assert.Equal(lines[index], line); called++; Put(bytes, offsets[index], 2, widths[index]); }); row.NotifyObserver(bytes, 1); Assert.Equal(1, called); Assert.Equal(lengths[index], BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(2, 2)));
        }
        Assert.Equal(0u, scene.Game.PlayerRecords.Base);
    }
    /// <summary>없는 연결은 실제 관문에서 반환하고 Session/수신 본문/Player 할당을 요구하지 않는다.</summary>
    [Fact]
    public void VersionQueryWithoutConnectionNeedsNoWorldOrPayload()
    {
        var scene = new Scene(); var dispatch = Dispatch(); ClientMissionPacketDefaults.InstallVersionReply(dispatch, scene.Game, scene.Connections, scene.Surface); Assert.Equal(0x004b3af0u, dispatch.SnapshotReceiver(0x0b)!.NativeEntry); dispatch.InvokeDirect(0x0b, 9, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(0u, scene.Game.PlayerRecords.Base); Assert.Empty(scene.Assertions);
        var other = new Scene(); Assert.Throws<ArgumentException>(() => ClientMissionPacketDefaults.InstallVersionReply(dispatch, scene.Game, other.Connections, scene.Surface));
    }
    /// <summary>실제 내부 Source/Registry/Player로50BYTE FF 예약과 현재 observer의11BYTE wire를 구별한다.</summary>
    [Fact]
    public void ActualInternalSourceSendsCurrentVersionReplyThroughObserver()
    {
        var scene = new Scene(); scene.Game.PlayerRecords.Initialize((_, _) => { }); scene.Game.PlayerRecords.Records.Span[2 * 180 + 0x50] = 1; scene.Game.Authority.Server = true; var sent = new List<byte[]>();
        using var source = new CompositeMissionTransport(CompositeTransportRole.InternalMission, scene.Game.PlayerRecords.Records, () => scene.Game.Authority.Server,
            new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { }, Sent = (_, bytes, length) => { if (bytes.Span[1] == 0x0c) sent.Add(bytes.Span[..length].ToArray()); }, Players = () => scene.Game.PlayerRecords.Records }, scene.Connections.Registry, scene.Game);
        scene.Connections.PacketSources.Publish(source);
        try
        {
            source.InternalTransport.RegisterLogin((_, _, _) => throw new InvalidOperationException("미선택 로그인")); source.Serve(6799); source.Connect(new byte[20], 6799); Assert.Equal(0, source.Poll()); source.Assign(-1, 2);
            ClientPacketDefaultObservers.Install18(source.Dispatch, scene.Surface.Storage); ClientMissionPacketDefaults.InstallVersionReply(source.Dispatch, scene.Game, scene.Connections, scene.Surface);
            scene.Game.GameAux.Packet0cHeader[1] = 0x81; var saved = source.Dispatch.SnapshotReceiver(0x0b)!; saved.Invoke(2, null, ReadOnlyMemory<byte>.Empty); source.Flush(); Assert.Equal("010c0b000a004e00ffffff", Convert.ToHexStringLower(Assert.Single(sent))); Assert.Equal(new uint[] { 0x38c }, scene.Assertions);
            source.Dispatch.RegisterCanonicalDirect(0x0b, 0x9999, (_, _, _) => throw new InvalidOperationException("새 표의 다른 F")); scene.Game.GameAux.Packet0cHeader[1] = 0x83; scene.Game.GameAux.Packet0cInitialLength = 32; saved.Invoke(2, null, ReadOnlyMemory<byte>.Empty); source.Flush(); Assert.Equal(2, sent.Count); Assert.Equal("030c0b000a004e00ffffff", Convert.ToHexStringLower(sent[1])); Assert.Equal(new uint[] { 0x38c, 0x38c }, scene.Assertions);
        }
        finally { source.CloseRoom(); }
    }
}
