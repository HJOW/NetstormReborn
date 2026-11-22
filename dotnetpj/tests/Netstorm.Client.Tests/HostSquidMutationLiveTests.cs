using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 고정 수신이 같은 실제 공장/내용물/환불/SP 상태와 연결되는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>원문 변경 수신의 부분 효과와 실제 Host 장치를 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>현재 수신의 WORD 두 바이트만 작성한다.</summary>
    private static void MutationWord(byte[] packet, int offset, int value) => BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(offset), checked((ushort)value));
    /// <summary>실제 ContentForm6을 생산 부모의 정렬·통지와 같은 종속 체인에 부착한다.</summary>
    [Fact]
    public void NativeMutationContentUsesRealContentFormSortingAndCurrentFactory()
    {
        var (session, types, surface) = SquidPacketScene(); session.BuildFlareWorldDevices((_, _, _, _) => throw PathParentForbidden()); session.BuildProductionWorldDevices(ProductionDevices());
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int priest = PlacePriest(session, types, 0, 9);
        byte[] packet = new byte[18]; MutationWord(packet, 4, 30000); MutationWord(packet, 6, 0x8000); MutationWord(packet, 8, 71); MutationWord(packet, 10, 0); MutationWord(packet, 12, 23); MutationWord(packet, 14, priest); packet[16] = 17;
        int priorHead = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(priest)[6..]); session.ReceiveFixedGameplayPacket(0x4327b0, packet);
        ReadOnlySpan<byte> raw = session.Pool.Slot(30000); Assert.Equal(6, raw[10]); Assert.Equal(new RawSquidForms(session.Pool).Vtable(6), BinaryPrimitives.ReadUInt32LittleEndian(raw));
        Assert.Equal(0x8000, BinaryPrimitives.ReadUInt16LittleEndian(raw[12..])); Assert.Equal(71, BinaryPrimitives.ReadUInt16LittleEndian(raw[18..])); Assert.Equal(23, BinaryPrimitives.ReadUInt16LittleEndian(raw[24..]));
        // RawContentAttach는 수신한+23 순서를 같은 그룹의 실제 정렬 순번0으로 다시 쓴다.
        Assert.Equal(0, raw[23]);
        Assert.Equal(priest, RawContainedLink.CommonContainedParent(session.Pool, 30000)); Assert.Equal(0, raw[11] & 0x15);
        // 다른 그룹의 기존 회복 Form은 머리에 남고 새 ContentForm이 그 뒤에 붙는다.
        Assert.Equal(priorHead, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(priest)[6..])); Assert.Equal(30000, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(priorHead)[4..])); Assert.Equal(priorHead, BinaryPrimitives.ReadUInt16LittleEndian(raw[14..]));
        session.ReleasePlayerProcesses();
    }
    /// <summary>현재 Depend 역할이 다른 가상표를 만들면 실제74를 거부하고 앞선 raw 쓰기를 보존한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void NativeMutationDependencyUsesRealFactoryAndRejectsDifferentVirtualMethod(bool native)
    {
        var packets = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(native ? new() { Broadcast = bytes => packets.Add(bytes.ToArray()), Connected = _ => false, Send = (_, _) => throw PathParentForbidden() } : null);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int priest = PlacePriest(session, types, 0, 9); packets.Clear();
        byte[] packet = new byte[13]; MutationWord(packet, 4, 30000); MutationWord(packet, 6, priest); session.ReceiveFixedGameplayPacket(0x432850, packet);
        Assert.Equal(1, session.Pool.Slot(30000)[10]); Assert.Equal(priest, RawContainedLink.CommonContainedParent(session.Pool, 30000)); Assert.Equal(0, session.Pool.Slot(30000)[11] & 5);
        if (native) { byte[] sent = Assert.Single(packets); Assert.Equal(2048, sent.Length); Assert.Equal(0x65, sent[1]); Assert.Equal(30000, BinaryPrimitives.ReadUInt16LittleEndian(sent.AsSpan(4))); }
        else Assert.Contains(session.WorldPackets, sent => sent[1] == 0x65 && BinaryPrimitives.ReadUInt16LittleEndian(sent.AsSpan(4)) == 30000);
        session.GameAux.DependFormTypeBox.Value = 2; MutationWord(packet, 4, 30001); MutationWord(packet, 9, 0x1234); MutationWord(packet, 11, 0x5678);
        Assert.Contains("가상74", Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x432850, packet)).Message);
        Assert.Equal(2, session.Pool.Slot(30001)[10]); Assert.Equal(4, session.Pool.Slot(30001)[11] & 4); Assert.Equal(0x1234, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(30001)[18..]));
        session.ReleasePlayerProcesses();
    }
    /// <summary>삭제 로그 복귀 뒤 현재 HP/TYPE로 환불하고 실제 가상삭제 호출 전 같은 SP가 증가한다.</summary>
    [Fact]
    public void NativeMutationDeletedRefundUsesActualRewardBeforeVirtualDestroy()
    {
        var (session, types, surface) = SquidPacketScene(); int priest = PlacePriest(session, types, 0, 9); types[158].Cost = 1000; session.Gameplay.Authority.Boss = false;
        SquidRewardResult expected = session.Reward.Plan(priest, 1, true); Assert.True(expected.Applied); Assert.True(expected.Refund > 0); float money = BitConverter.UInt32BitsToSingle(session.Sp.Get(1)); var events = new List<string>();
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()
        {
            Log = (level, text) => { Assert.Equal(0x1d, level); Assert.StartsWith("destroying ", text); events.Add("log"); },
            VirtualDestroy = (sid, flags) => { events.Add("destroy"); Assert.Equal(priest, sid); Assert.Equal(0x200008u, flags); Assert.Equal(money + expected.Refund, BitConverter.UInt32BitsToSingle(session.Sp.Get(1))); throw new InvalidOperationException("선택된 실제 전체 삭제 경계"); },
        });
        byte[] packet = new byte[8]; MutationWord(packet, 4, priest); MutationWord(packet, 6, 0x20);
        Assert.Equal("선택된 실제 전체 삭제 경계", Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x433170, packet)).Message); Assert.Equal(new[] { "log", "destroy" }, events); session.ReleasePlayerProcesses();
    }
    /// <summary>이미 free인 삭제도 같은 현재 추적/표본/PlayerKnown으로 SP 감사 꼬리를 실행한다.</summary>
    [Fact]
    public void NativeMutationDeletedFreeUsesSharedPatchSampleAndConditionalSpGet()
    {
        var (session, types, surface) = SquidPacketScene(); session.Gameplay.Authority.Boss = false; session.PriestState.Regen.Patch.Remaining = 1; session.Gameplay.PatchSample.Value = 7;
        session.Sp.Set(1, BitConverter.SingleToUInt32Bits(8001f)); session.Gameplay.PatchDiagnostic.Value = 99; var logs = new List<string>(); types[0].Name = "F0\0ignored";
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { Log = (_, text) => logs.Add(text) });
        byte[] packet = new byte[8]; MutationWord(packet, 4, 30000); session.ReceiveFixedGameplayPacket(0x433170, packet);
        Assert.Equal(0u, session.PriestState.Regen.Patch.Remaining); Assert.Equal(7u, session.Gameplay.PatchDiagnostic.Value); Assert.Equal(new[] { "destroying F0\n", "~rdouble destroy" }, logs);
        session.MissionPlayer.PlayerKnown = 0; session.PriestState.Regen.Patch.Remaining = 1; int tracking = session.Sp.TrackingOffsetCurrent.Value;
        session.ReceiveFixedGameplayPacket(0x433170, packet); Assert.Equal(tracking, session.Sp.TrackingOffsetCurrent.Value); Assert.Equal(0u, session.PriestState.Regen.Patch.Remaining); session.ReleasePlayerProcesses();
    }
    /// <summary>원격53/62는 실제 ProcessForm 삭제를 실행하고 콜백 뒤 현재 목록을 다시 읽는다.</summary>
    [Fact]
    public void NativeMutationRemoteDeleteUsesActualHostAndCurrentPacketAfterCallback()
    {
        var (session, types, surface) = SquidPacketScene(); int priest = PlacePriest(session, types, 0, 9); int form = session.PlayerProcesses.FindEvent(priest, 602)!.Form(); byte[] packet = new byte[12]; var deleted = new List<int>();
        session.PlayerProcesses.Runtime().Asset = new() { Selected = () => 0, Emit = effect => Assert.Equal(SquidDestroyEffect.Transmit, effect.Effect) };
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { VirtualDestroy = (sid, flags) => { Assert.Equal(0u, flags); deleted.Add(sid); session.PlayerProcesses.Kill(session.PlayerProcesses.FromForm(sid)!, flags); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 1); } });
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 2); MutationWord(packet, 8, form); MutationWord(packet, 10, priest);
        session.ReceiveFixedGameplayPacket(0x433120, packet); Assert.Equal(new[] { form }, deleted); Assert.Null(session.PlayerProcesses.FromForm(form)); Assert.Equal(1, session.Pool.Slot(form)[11] & 1); Assert.Equal(0, session.Pool.Slot(priest)[11] & 5);
        byte[] single = new byte[6]; MutationWord(single, 4, form); session.ReceiveFixedGameplayPacket(0x4436b0, single); Assert.Single(deleted);
        session.ReleasePlayerProcesses();
    }
    /// <summary>실제 자산 삭제 분배를 설치하지 않으면 살아 있는53은 실패하고free는 조회만으로 끝난다.</summary>
    [Fact]
    public void NativeMutationRemoteDeleteRequiresActualVirtualDestroyOnlyForLiveForm()
    {
        var (session, types, surface) = SquidPacketScene(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int priest = PlacePriest(session, types, 0, 9);
        byte[] packet = new byte[6]; MutationWord(packet, 4, 30000); session.ReceiveFixedGameplayPacket(0x4436b0, packet); MutationWord(packet, 4, priest);
        Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x4436b0, packet)); Assert.Equal(0, session.Pool.Slot(priest)[11] & 5); session.ReleasePlayerProcesses();
    }
}
