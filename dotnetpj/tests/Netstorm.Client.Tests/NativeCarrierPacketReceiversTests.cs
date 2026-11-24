using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 원문 Carrier 15행의 고정 F/metadata와 세 송신 옵저버를 대조한다.
// cpppj/src/client/NativeCarrierPacketReceivers.cpp 의 설치·대여와
// cpppj/src/o/RawBombEffect.cpp·RawBombTimerPacket.cpp·RawMeltdown.cpp 의 송신 옵저버를 대조한다.
// 0.963에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client.Tests;

/// <summary>Carrier 고정 F 설치·대여와 송신 옵저버의 경계를 확인한다.</summary>
public sealed class NativeCarrierPacketReceiversTests
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
        Assert.Throws<ArgumentNullException>(() => NativeCarrierPacketReceivers.Install(null!, game, storage));
        Assert.Throws<ArgumentNullException>(() => NativeCarrierPacketReceivers.Install(Dispatch(), null!, storage));
        Assert.Throws<ArgumentNullException>(() => NativeCarrierPacketReceivers.Install(Dispatch(), game, null!));
        Assert.Throws<ArgumentException>(() => NativeCarrierPacketReceivers.Install(Dispatch(), game, new RawSurfaceStorage()));
        Assert.Throws<ArgumentNullException>(() => NativeCarrierPacketReceivers.Bind(null!, 0x77));
        Assert.Throws<ArgumentException>(() => NativeCarrierPacketReceivers.Bind(Dispatch(), 0x00));
    }

    /// <summary>원문 15행의 고정 F·버전·길이·옵저버 구분을 설치한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void InstallSetsAllFifteenRows()
    {
        var (game, storage) = Scene();
        var dispatch = Dispatch();
        NativeCarrierPacketReceivers.Install(dispatch, game, storage);
        (uint Type, uint Entry, ushort Length, bool Packet)[] rows =
        [
            (0x77, 0x0048f7f0u, 6, false), (0x79, 0x0048f8f0u, 6, false), (0x6f, 0x004900e0u, 6, false),
            (0x40, 0x00428570u, 18, false), (0x4f, 0x00426520u, 9, false), (0x50, 0x00444bc0u, 9, false),
            (0x60, 0x004267c0u, 10, false), (0x61, 0x0048bfe0u, 11, false), (0x5c, 0x0041e860u, 6, false),
            (0x5f, 0x00420bf0u, 0xffff, true), (0x7d, 0x0041e320u, 0xffff, true), (0x6b, 0x00448e90u, 10, false),
            (0x6d, 0x00493f80u, 8, false), (0x75, 0x004205f0u, 0xffff, true), (0x76, 0x004207a0u, 6, false),
        ];
        // 검토한 15행의 등록 값과 고정 F를 모두 대조한다.
        foreach (var (type, entry, length, packet) in rows)
        {
            var row = dispatch.Registration(type);
            Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(length, row.Length);
            Assert.Equal(packet, row.PacketObserver is not null); Assert.Equal(!packet, row.Observer is not null);
            Assert.Equal(entry, dispatch.SnapshotReceiver(type)!.NativeEntry);
        }
    }

    /// <summary>주문 효과 송신 옵저버는 count low WORD로 길이를 확정한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void BombEffectObserverFinalizesSendingLength()
    {
        byte[] packet = new byte[12];
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(6, 4), 3);
        NativeCarrierPacketReceivers.ObserveBombEffectPacket(packet, 0, null);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        NativeCarrierPacketReceivers.ObserveBombEffectPacket(packet, 1, null);
        Assert.Equal((3u + 5u) * 2u, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        int diagnoses = 0;
        byte[] full = new byte[12];
        BinaryPrimitives.WriteUInt32LittleEndian(full.AsSpan(6, 4), 0xffffffffu);
        NativeCarrierPacketReceivers.ObserveBombEffectPacket(full, 1, () => diagnoses++);
        Assert.Equal(1, diagnoses);
        Assert.Throws<InvalidOperationException>(() => NativeCarrierPacketReceivers.ObserveBombEffectPacket(full, 1, null));
    }

    /// <summary>상태 타이머 송신 옵저버는 현재 수로 선언 길이를 확정한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void BombTimerObserverFinalizesSendingLength()
    {
        byte[] packet = new byte[20];
        NativeCarrierPacketReceivers.FinalizeBombTimerPacket(packet, false);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(16, 2), 4);
        NativeCarrierPacketReceivers.FinalizeBombTimerPacket(packet, true);
        Assert.Equal((4u + 11u) * 2u, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(16, 2), 0xffff);
        Assert.Throws<InvalidOperationException>(() => NativeCarrierPacketReceivers.FinalizeBombTimerPacket(packet, true));
    }

    /// <summary>Meltdown 송신 옵저버는 현재 count로 WORD 길이를 갱신한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void MeltdownObserverFinalizesSendingLength()
    {
        byte[] packet = new byte[24];
        NativeCarrierPacketReceivers.FinalizeMeltdownPacket(packet, false, null);
        Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), 5);
        NativeCarrierPacketReceivers.FinalizeMeltdownPacket(packet, true, null);
        Assert.Equal((5u + 10u) * 2u, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2, 2)));
        int diagnoses = 0;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), 0xffffffffu);
        NativeCarrierPacketReceivers.FinalizeMeltdownPacket(packet, true, () => diagnoses++);
        Assert.Equal(1, diagnoses);
        Assert.Throws<InvalidOperationException>(() => NativeCarrierPacketReceivers.FinalizeMeltdownPacket(packet, true, null));
    }

    /// <summary>원래 caller 위치에서 같은 F/metadata가 일치할 때만 대여에 붙는다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void BindVerifiesMetadataAndReleasesLease()
    {
        var (game, storage) = Scene();
        var dispatch = Dispatch();
        NativeCarrierPacketReceivers.Install(dispatch, game, storage);
        NativeCarrierPacketReceivers.Bind(dispatch, 0x5f);
        NativeCarrierPacketReceivers.Bind(dispatch, 0x75);
        dispatch.RegisterCanonicalDirect(0x5f, 0x9999, (_, _, _) => { });
        Assert.Throws<InvalidOperationException>(() => NativeCarrierPacketReceivers.Bind(dispatch, 0x5f));
    }

    /// <summary>수신은 현재 Session을 선택한 뒤 고정 본체 자리에서 명시 실패한다. 0.963에서 추가됐다.</summary>
    [Fact]
    public void ReceiveRoutesToSessionWithExplicitBodyFailure()
    {
        var (game, storage) = Scene();
        var dispatch = Dispatch();
        NativeCarrierPacketReceivers.Install(dispatch, game, storage);
        Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x77, 0, null, new byte[6]));
        var session = new HostSession(new Kernel(), new GameRandom(17), new CrtRandom(23), sharedGameplay: game, pool: storage.Pool);
        try
        {
            using var owner = HostMissionRuntime.Create(session, storage);
            var error = Assert.Throws<InvalidOperationException>(() => dispatch.InvokeDirect(0x77, 3, null, new byte[6]));
            Assert.Contains("48f7f0", error.Message);
            Assert.False(game.RuntimeCallActive(session)); Assert.False(owner.Released);
            Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedCarrierPacket(0x9999, 0, new byte[6]));
        }
        finally { session.ReleasePlayerProcesses(); }
    }
}
