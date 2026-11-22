using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 Host의 같은 간헐천/수량/난수/장기 타입과 수신 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 사제 월드 구성기에 간헐천 채집 수신을 연결한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>같은 실제 지형/지역/자산 장치에 간헐천을 Pop하고 수량을 설정한다.</summary>
    private static int PlaceMinedGeyser(HostSession session, IReadOnlyList<OriginalTypeRecord> types, ushort quantity)
    {
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new() { SetResidenceOwner = (_, _) => throw PathParentForbidden(), Assertion = _ => throw PathParentForbidden() });
        session.BuildRegionOwnershipDevices(types); session.SetWorldLoadingDepth(1);
        int geyser = session.BuildPlayerWorldHooks().Create!(122, 2); session.PopWorldObject(geyser, 50.75f, 52.5f, 0); session.SetWorldLoadingDepth(0);
        BinaryPrimitives.WriteUInt16LittleEndian(session.Pool.AllocatedBytes(geyser)[12..], quantity); return geyser;
    }
    /// <summary>고정5bh는 실제 음향 뒤 바뀐 같은 단가TYPE/권한을 읽고 게임난수를 한 번 전진한다.</summary>
    [Fact]
    public void NativeMinedPacketSharesActualRandomQuantityAndCurrentGlobalsAfterSound()
    {
        var (session, types, surface) = SquidPacketScene(); int geyser = PlaceMinedGeyser(session, types, 100); int sounds = 0;
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()
        {
            PlayAtFile = (x, y, name, flags) => { sounds++; Assert.Equal(50.75f, x); Assert.Equal(52.5f, y); Assert.Matches(@"^geyserMined[1-6]\.wav$", name); Assert.Equal(0u, flags); session.GameAux.NuggetTypeBox.Value = 155; types[155].Cost = 21; session.Gameplay.Authority.Boss = false; },
            VirtualDestroy = (_, _) => throw PathParentForbidden(),
        });
        Assert.Same(session.Pool, session.GeyserMined.Pool); Assert.Same(session.GameAux.NuggetTypeBox, session.GeyserMinedState.SharedNuggetType);
        var expected = new GameRandom(session.Game.State()); _ = expected.Next(6000); byte[] packet = new byte[6]; MutationWord(packet, 4, geyser); session.ReceiveFixedGameplayPacket(0x4266c0, packet);
        Assert.Equal(79, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(geyser)[12..])); Assert.Equal(1, sounds); Assert.Equal(expected.State(), session.Game.State()); Assert.Single(session.SidLists.Geysers.Items().ToArray());
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.GeyserMined); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x4266c0, packet));
    }
    /// <summary>전체 가상삭제 미연결을 성공으로 감추지 않고 수량0과음향까지의 실제 부분 효과를 유지한다.</summary>
    [Fact]
    public void NativeMinedDepletionRequiresActualVirtualDestroyAfterQuantityWrite()
    {
        var (session, types, surface) = SquidPacketScene(); int geyser = PlaceMinedGeyser(session, types, 200); types[154].Cost = 200; var events = new List<string>();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()
        {
            PlayAtFile = (_, _, _, _) => events.Add("sound"),
            VirtualDestroy = (sid, flags) => { Assert.Equal(geyser, sid); Assert.Equal(0x1000u, flags); Assert.Equal(0, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(sid)[12..])); events.Add("destroy"); throw new InvalidOperationException("실제 전체 간헐천 삭제 경계"); },
        });
        byte[] packet = new byte[6]; MutationWord(packet, 4, geyser); Assert.Equal("실제 전체 간헐천 삭제 경계", Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x4266c0, packet)).Message);
        Assert.Equal(new[] { "sound", "destroy" }, events); Assert.Equal(0, session.Pool.Slot(geyser)[11] & 5); session.ReleasePlayerProcesses();
    }
    /// <summary>원문 등록 helper부터 같은 실제 handler를 실행하고 free는 난수/음향을 소비하지 않는다.</summary>
    [Fact]
    public void GeyserMinedRegistrationUsesActualDispatchAndFreeDoesNotConsumeEffects()
    {
        var (session, types, surface) = SquidPacketScene(); int geyser = PlaceMinedGeyser(session, types, 500); types[154].Cost = 200; int recorded = 0, sounds = 0; var connection = new PacketConnection();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { PlayAtFile = (_, _, _, _) => { Assert.Equal(1u, connection.Depth); Assert.Equal(1, recorded); sounds++; } });
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 723, Record = (_, _, _) => recorded++, Terminate = _ => throw PathParentForbidden(), Close = _ => throw PathParentForbidden(), Release = _ => throw PathParentForbidden() });
        GeyserMinedRegistration.Register(dispatch, session.GeyserMined); var row = dispatch.Registration(0x5b); Assert.Equal(6, row.Length); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Null(row.Observer); Assert.NotNull(row.Receive);
        byte[] packet = [0xff, 0x5b, 6, 0, 0, 0]; MutationWord(packet, 4, geyser); Assert.True(dispatch.Dispatch(connection, packet)); Assert.Equal(1, sounds); Assert.Equal(0u, connection.Depth); Assert.Equal(723u, connection.Received);
        Assert.Equal(300, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(geyser)[12..])); uint random = session.Game.State(); MutationWord(packet, 4, 30000); session.ReceiveFixedGameplayPacket(0x4266c0, packet); Assert.Equal(random, session.Game.State()); Assert.Equal(1, sounds); session.ReleasePlayerProcesses();
    }
}
