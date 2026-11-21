using System.Buffers.Binary;
using Netstorm.Core.Original;

// 실제 Host의 같은 사제/공간/내용물에 정지·도착 부모를 연결한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 Host 조회와 원문 부모의 실제 세션 수명을 검사한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>선택하지 않은 큰 이동/의식 하위가 성공으로 삼켜지지 않도록 검사에서 거부한다.</summary>
    private static InvalidOperationException PathParentForbidden() => new("선택하지 않은 큰 경로 효과");
    /// <summary>실제 장치를 별도로 설치하지 않은 큰 효과는 선택 위치에서만 실패한다.</summary>
    private static CarrierArrivalHooks PathParentEffects() => new()
    {
        CdPriestPickupPriest = () => throw PathParentForbidden(), Pickup = _ => throw PathParentForbidden(), Escape = (_, _, _) => throw PathParentForbidden(),
        ChooseReturn = _ => throw PathParentForbidden(), ChooseGeyser = _ => throw PathParentForbidden(), GoReturn = (_, _) => throw PathParentForbidden(), GoGeyser = (_, _) => throw PathParentForbidden(),
        MinedPacket = _ => throw PathParentForbidden(), Destroy = (_, _) => throw PathParentForbidden(), CdAddSp = (_, _) => throw PathParentForbidden(), CdOwnerPacket = _ => throw PathParentForbidden(), PlayAt = (_, _, _, _) => throw PathParentForbidden(), Robust = _ => true,
    };
    /// <summary>같은 실제 사제/Host의정지·현재경로없음·도착/내용물getter와 반납을 확인한다.</summary>
    [Fact]
    public void PathParentsUseActualPriestWorldAndCurrentHostLookup()
    {
        var (session, types, surface) = SquidPacketScene(); session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int priest = PlacePriest(session, types, 0, 9);
        Assert.Throws<ArgumentException>(() => session.BuildPriestHaltWorldDevices(new()));
        RawPriestHalt halt = session.BuildPriestHaltWorldDevices(new() { CarrierPickup = _ => throw PathParentForbidden(), NuggetTransfer = _ => throw PathParentForbidden() });
        Assert.Same(session.Pool, halt.Pool); Assert.Equal(10u, halt.CommonHalt(priest));
        var anchor = new RawPlayerPlacementAnchor(session.Pool, types, surface.Storage.Hash.BorrowEntries(0), session.TerrainBookkeeping, new(), new(), new() { GraphReady = 1 });
        var cleanup = session.BuildPathCleanupWorldDevices(session.PriestWorld.Carrier, anchor, new(), new() { ReserveShared = () => throw PathParentForbidden(), Ritual = (_, _, _) => throw PathParentForbidden(), Destroy = (_, _) => throw PathParentForbidden() });
        var state = new CarrierArrivalState(); RawCarrierArrival arrival = session.BuildCarrierArrivalWorldDevices(cleanup, anchor, state, PathParentEffects());
        Assert.Same(session.Pool, arrival.Pool); Assert.Null(session.CurrentPathRuntime(priest)); Assert.Equal(1000u, arrival.Arrival(priest, 0, 0));
        session.GameAux.NuggetTypeBox.Value = 150; session.GameAux.AllowAnyCapture = 2;
        Assert.Equal(150u, state.CurrentNuggetType()); Assert.True(state.CurrentAllowAlliedPickup());
        // 현재Host에경로가없어도잘못된부모가상표를정상null로감추지않는다.
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(priest)); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(priest), 0x11018000);
        Assert.Throws<InvalidOperationException>(() => session.CurrentPathRuntime(priest)); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(priest), table);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.PathPriestHalt); Assert.Throws<InvalidOperationException>(() => session.PathCarrierArrival); Assert.Throws<ObjectDisposedException>(() => session.CurrentPathRuntime(priest));
    }
}
