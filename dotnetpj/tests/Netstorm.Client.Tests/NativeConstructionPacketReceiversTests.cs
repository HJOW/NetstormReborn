using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 원문 건설/연구 8행의 고정 F/metadata와 건설 송신 옵저버를 대조한다.
// cpppj/src/client/NativeConstructionPacketReceivers.cpp 의 설치·대여와
// cpppj/src/o/RawConstructionPacket.cpp 의 송신 옵저버를 대조한다.
// 0.963에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>건설/연구 고정 F 설치·대여와 송신 옵저버의 경계를 확인한다.</summary>
public sealed class NativeConstructionPacketReceiversTests
{
    /// <summary>연결/소켓 없이 실제 기본 표를 구성한다.</summary>
    private static RawPacketDispatch Dispatch() => new(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });

    /// <summary>같은 장기 전역·진단을 공유하는 실제 저장소다.</summary>
    private static (ClientGameplayGlobals Game, RawSurfaceStorage Storage) Scene()
    {
        var game = new ClientGameplayGlobals();
        return (game, new RawSurfaceStorage(OriginalEdition.Patch1078, game.Authority, game.GameAux));
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var (game, storage) = Scene();
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.Install(null!, game, storage));
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.Install(Dispatch(), null!, storage));
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.Install(Dispatch(), game, null!));
        Assert.Throws<ArgumentException>(() => NativeConstructionPacketReceivers.Install(Dispatch(), game, new RawSurfaceStorage()));
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.BindCommandReceivers(null!));
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.BindDataReceiver(null!));
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.BindFactoryResearchReceiver(null!));
        Assert.Throws<ArgumentNullException>(() => NativeConstructionPacketReceivers.BindReceiver(null!, 0x3e));
        Assert.Throws<ArgumentException>(() => NativeConstructionPacketReceivers.BindReceiver(Dispatch(), 0x00));
    }

    /// <summary>원문 8행의 고정 F·버전·길이·옵저버 구분을 설치한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsAllEightRows()
    {
        var (game, storage) = Scene();
        var dispatch = Dispatch();
        NativeConstructionPacketReceivers.Install(dispatch, game, storage);
        (uint Type, uint Entry, ushort Length)[] rows =
        [
            (0x3e, 0x00444af0u, 25), (0x3f, 0x004441b0u, 0xffff), (0x37, 0x004427b0u, 0xffff), (0x58, 0x00453d70u, 16),
            (0x54, 0x004436f0u, 8), (0x51, 0x004dc050u, 8), (0x2f, 0x004332c0u, 15), (0x6c, 0x004d56f0u, 11),
        ];
        // 검토한 8행의 등록 값과 고정 F를 모두 대조한다.
        foreach (var (type, entry, length) in rows)
        {
            var row = dispatch.Registration(type);
            Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(length, row.Length);
            Assert.Equal(entry, dispatch.SnapshotReceiver(type)!.NativeEntry);
        }

        Assert.NotNull(dispatch.Registration(0x3f).PacketObserver); Assert.Null(dispatch.Registration(0x3f).Observer);
        Assert.Null(dispatch.Registration(0x37).PacketObserver); Assert.Null(dispatch.Registration(0x37).Observer);
        Assert.NotNull(dispatch.Registration(0x3e).Observer); Assert.Null(dispatch.Registration(0x3e).PacketObserver);
    }

    /// <summary>건설 송신 옵저버는 현재 sidCount로 선언 길이를 갱신한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void ConstructionObserverFinalizesSendingLength()
    {
        byte[] packet = new byte[0x1b];
        NativeConstructionPacketReceivers.FinalizeConstructionPacket(packet, false, null);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        packet[26] = 3;
        NativeConstructionPacketReceivers.FinalizeConstructionPacket(packet, true, null);
        Assert.Equal(3u * 2u + 0x1bu, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        int diagnoses = 0;
        packet[26] = 0xff;
        NativeConstructionPacketReceivers.FinalizeConstructionPacket(packet, true, () => diagnoses++);
        Assert.Equal(1, diagnoses);
        Assert.Throws<InvalidOperationException>(() => NativeConstructionPacketReceivers.FinalizeConstructionPacket(packet, true, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => NativeConstructionPacketReceivers.FinalizeConstructionPacket(new byte[4], true, null));
    }

    /// <summary>대여는 원래 등록 순서의 행만 허용한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void BindFollowsOriginalRegistrationOrder()
    {
        var (game, storage) = Scene();
        var dispatch = Dispatch();
        NativeConstructionPacketReceivers.Install(dispatch, game, storage);
        NativeConstructionPacketReceivers.BindCommandReceivers(dispatch);
        NativeConstructionPacketReceivers.BindDataReceiver(dispatch);
        NativeConstructionPacketReceivers.BindFactoryResearchReceiver(dispatch);
        NativeConstructionPacketReceivers.BindReceiver(dispatch, 0x6c);
        dispatch.RegisterCanonicalDirect(0x3e, 0x9999, (_, _, _) => { });
        Assert.Throws<InvalidOperationException>(() => NativeConstructionPacketReceivers.BindCommandReceivers(dispatch));
    }

    /// <summary>수신은 현재 Session을 선택한 뒤 고정 본체 자리에서 명시 실패한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void ReceiveRoutesToSessionWithExplicitBodyFailure()
    {
        var (game, storage) = Scene();
        var dispatch = Dispatch();
        NativeConstructionPacketReceivers.Install(dispatch, game, storage);
        Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x3e, 0, null, new byte[25]));
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game, pool: storage.Pool);
        try
        {
            using var owner = HostMissionRuntime.Create(session, storage);
            var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x3e, 5, null, new byte[25]));
            Assert.Contains("444af0", error.Message);
            Assert.False(game.RuntimeCallActive(session)); Assert.False(owner.Released);
            Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedConstructionPacket(0x9999, 0, new byte[25]));
        }
        finally { session.ReleasePlayerProcesses(); }
    }
}
