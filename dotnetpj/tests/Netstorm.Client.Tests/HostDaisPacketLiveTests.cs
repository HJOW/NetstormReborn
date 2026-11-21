using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

// 같은 실제 Host/Kernel/장기 헤더·카메라·공간에서 고정 entry 수신을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실제 미션 구성기를 공유하는 Dais/Regular/위치/카메라/Tell 경로 검사다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>6ah의 Form DWORD/mask/부모/기한을 원문 폭으로 작성한다.</summary>
    private static byte[] DaisPacket(uint form, uint parent, double deadline, byte mask = 0xff)
    {
        var packet = new byte[21]; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), form); packet[8] = mask;
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9, 4), parent); BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(13, 8), deadline); return packet;
    }
    /// <summary>같은 실제 공장으로 만든 부모에 새 Dais를 명시 Form으로 붙이고 빈 프레임·현재 헤더 writer·반납을 확인한다.</summary>
    [Fact]
    public void NativeDaisReceiverOwnsActualKernelObjectAndCurrentFullWriter()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(152, 0);
        const int form = 16000; // 신규 수신이 직접 지정하는 서버 영역 Form SID다.
        byte[]? sent = null; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = packet => sent = packet.ToArray() });
        session.ReceiveFixedGameplayPacket(0x44a280, DaisPacket(form, (uint)parent, 1.25)); var process = Assert.IsType<DaisProcess>(session.PlayerProcesses.FromForm(form));
        Assert.Equal(form, process.Form()); Assert.Equal(parent, process.Parent()); Assert.Equal(58, session.Pool.Slot(form)[10]);
        Assert.Same(process, session.Kernel.Get(session.Kernel.Find(process))); byte[] before = process.Fields.Bytes.ToArray(); session.ProcessTime.Value = 10; session.Kernel.RunFrame(); Assert.Equal(before, process.Fields.Bytes);
        session.GameAux.DaisProcessHeader[0] = 0x6b; session.GameAux.DaisProcessHeader[1] = 0x81;
        int size = session.PlayerProcesses.WriteFormPacket(form, 1, session.GameAux.SquidTransmission.Buffer, session.GameAux.SquidTransmission);
        Assert.Equal(21, size); session.PlayerProcesses.Transmission()!.Transmit!(size); Assert.NotNull(sent); Assert.Equal(0x81, sent[0]); Assert.Equal(0x6b, sent[1]);
        Assert.Equal((uint)form, BinaryPrimitives.ReadUInt32LittleEndian(sent.AsSpan(4, 4))); Assert.Equal((uint)parent, BinaryPrimitives.ReadUInt32LittleEndian(sent.AsSpan(9, 4))); Assert.Equal(1.25, BinaryPrimitives.ReadDoubleLittleEndian(sent.AsSpan(13, 8)));
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size()); Assert.Equal(0x5019ecu, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes));
    }
    /// <summary>신규 진단은 첫 Form을 유지하고 복귀 뒤 현재 mask/TYPE/부모/기한을 읽는다.</summary>
    [Fact]
    public void NativeDaisNewDiagnosticKeepsSelectedFormAndRereadsPacketAndType()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(152, 0); byte[] packet = DaisPacket(16000, (uint)parent, 1, 0); int count = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", text); Assert.Equal(@"..\o\_p_daisprocessdata.cpp", file); Assert.Equal(0x36u, line); count++;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4, 4), 16001); packet[8] = 0xff; session.GameAux.DaisProcessTypeBox.Value = 59; BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(13, 8), 3.5);
        });
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.ReceiveFixedGameplayPacket(0x44a280, packet);
        var process = Assert.IsType<DaisProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Equal(1, count); Assert.Equal(59, session.Pool.Slot(16000)[10]);
        Assert.Equal(3.5, BinaryPrimitives.ReadDoubleLittleEndian(process.Fields.Bytes.AsSpan(32, 8))); Assert.Equal(1, session.Pool.Slot(16001)[11] & 1); session.ReleasePlayerProcesses();
    }
    /// <summary>기존 수신은 같은 PID/부모를 보존하고 진단 뒤 현재 mask로 기한만 쓴다.</summary>
    [Fact]
    public void NativeDaisExistingReceiveKeepsObjectParentAndKernelIdentity()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(152, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.ReceiveFixedGameplayPacket(0x44a280, DaisPacket(16000, (uint)parent, 1)); var process = Assert.IsType<DaisProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process); byte[] packet = DaisPacket(16000, uint.MaxValue, 9, 3);
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", text); Assert.Equal(@"..\o\_p_daisprocessdata.cpp", file); Assert.Equal(0x3eu, line); packet[8] = 2; BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(9, 8), 2.75);
        });
        session.ReceiveFixedGameplayPacket(0x44a280, packet); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Equal(parent, process.Parent());
        Assert.Equal(2.75, BinaryPrimitives.ReadDoubleLittleEndian(process.Fields.Bytes.AsSpan(32, 8))); session.ReleasePlayerProcesses();
    }
    /// <summary>기존NULL PID는 field 선택이 없으면 반환하고 신규 DWORD 부모를 WORD로 먼저 자르지 않는다.</summary>
    [Fact]
    public void NativeDaisNullPidAndDwordBoundsDoNotCreateReplacementObjects()
    {
        var (session, types, surface) = SquidPacketScene(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); var empty = new byte[9];
        session.ReceiveFixedGameplayPacket(0x44a280, empty); Assert.Equal(0, session.Kernel.Size());
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x44a280, DaisPacket(0x10003e80, 0, 1)));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x44a280, DaisPacket(16000, 0x10003a98, 1)));
        Assert.Equal(0, session.Kernel.Size()); Assert.Equal(1, session.Pool.Slot(16000)[11] & 1); session.ReleasePlayerProcesses();
    }
    /// <summary>7e는 입력Form WORD를 읽지 않고 같은 장기 공유TYPE로 받은 시각/횟수/사건/인자를 복원한다.</summary>
    [Fact]
    public void NativeSharedRegularReceiveUsesActualCurrentTypeAndStoredDeadline()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(152, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.GameAux.SharedRegularProcessType = 60; session.ProcessTime.Value = 123; var packet = new byte[28]; packet[1] = 0x31; packet[4] = 0xff; packet[5] = 0xff;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), (ushort)parent); BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(8, 8), 1.25);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(16, 4), 12345); BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(20, 4), 2.5f); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(24, 4), 0x11223344);
        session.ReceiveFixedGameplayPacket(0x497030, packet); RegularProcess process = Assert.IsType<RegularProcess>(session.PlayerProcesses.FindEvent(parent, 12345));
        Assert.True(process.Shared()); Assert.Equal(1.25, process.Time()); Assert.Equal(2.5f, process.Payload()); Assert.Equal(0x11223344u, process.Count()); Assert.Equal(60, session.Pool.Slot(process.Form())[10]);
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size());
    }
    /// <summary>위치 수신의 client/현재 비활성 분기는 뒤 payload가 없어도 반환한다.</summary>
    [Fact]
    public void NativePositionReceiverSkipsClientAndFreeBeforeReadingPayload()
    {
        var (session, types, surface) = SquidPacketScene(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); byte[] packet = [0, 0, 0, 0, 5, 0];
        session.ReceiveFixedGameplayPacket(0x4a71c0, packet); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), 16000); session.ReceiveFixedGameplayPacket(0x4a71c0, packet); Assert.Equal(0, session.Kernel.Size()); session.ReleasePlayerProcesses();
    }
    /// <summary>실제 살아 있는 사제를 Unpop→frame 직접 저장→signed16.16 좌표 Pop으로 옮기며 소유자/기존 회복Form을 보존한다.</summary>
    [Fact]
    public void NativePositionReceiverUsesActualSpatialUnpopAndPop()
    {
        var (session, types, surface) = SquidPacketScene(); int sid = PlacePriest(session, types, 0, 9); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        RegularProcess? regen = session.PlayerProcesses.FindEvent(sid, 602); int count = session.Kernel.Size(); var packet = new byte[18]; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), (ushort)sid);
        // 사제의 거의 올림 지면 조회에서도 같은 기존50/60칸에 남는 실제 소수 좌표다.
        BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(6, 4), 50 * 65536 - 16384); BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(10, 4), 60 * 65536 - 8192);
        int frame = BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4)); BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(14, 4), frame);
        session.ReceiveFixedGameplayPacket(0x4a71c0, packet); Assert.Equal(49.75f, BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(sid).Slice(14, 4))); Assert.Equal(59.875f, BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(sid).Slice(18, 4)));
        Assert.Equal(frame, BinaryPrimitives.ReadInt32LittleEndian(session.Pool.Slot(sid).Slice(36, 4))); Assert.Equal(1, session.Pool.Slot(sid)[34]); Assert.Equal(count, session.Kernel.Size()); Assert.Same(regen, session.PlayerProcesses.FindEvent(sid, 602)); session.ReleasePlayerProcesses();
    }
    /// <summary>두 float를 크기 효과 전에 선택하고 실제 목표/메타 카운터만 갱신하며 현재 뷰를 변경하지 않는다.</summary>
    [Fact]
    public void NativeCameraReceiverUsesStoredFloatsAndCurrentLogicalSize()
    {
        var (session, types, surface) = SquidPacketScene(); var packet = new byte[12]; BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(4, 4), 12.25f); BinaryPrimitives.WriteSingleLittleEndian(packet.AsSpan(8, 4), 13.75f);
        session.Gameplay.Camera.Snapshot.Pixels[0] = 17; session.Gameplay.Camera.RendererView.Pixels[1] = 19; session.LocalState.MetaRebuild.Value = uint.MaxValue; session.Gameplay.Camera.Target.Zoom = 0x1234;
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { ScreenSize = () => { packet.AsSpan(4, 8).Clear(); return (1024, 768); } });
        session.ReceiveFixedGameplayPacket(0x4333f0, packet); Assert.Equal(new[] { -316, -233 }, session.Gameplay.Camera.Target.Pixels); Assert.Equal(0u, session.LocalState.MetaRebuild.Value);
        Assert.Equal(17, session.Gameplay.Camera.Snapshot.Pixels[0]); Assert.Equal(19, session.Gameplay.Camera.RendererView.Pixels[1]); Assert.Equal(0x1234u, session.Gameplay.Camera.Target.Zoom); session.ReleasePlayerProcesses();
    }
    /// <summary>70h는 다른 종류/길이DWORD도 첫NUL BYTE를 실제 현재Tell에 보내며 실제 배송 중 owner 반납을 지연한다.</summary>
    [Fact]
    public void NativeTellReceiverKeepsRawBytesAndActualRuntimeThroughCallback()
    {
        var (session, types, surface) = SquidPacketScene(); using var owner = HostMissionRuntime.Create(session, surface.Storage); byte[] packet = [0xff, 0x2c, 0, 0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0, 1]; ReadOnlyMemory<byte> received = default;
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { Tell = text => { received = text; Assert.True(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released); } });
        using (session.Gameplay.PinCurrentRuntime(session)) session.ReceiveFixedGameplayPacket(0x4cf980, packet);
        Assert.True(owner.Released); packet[9] = 0x81; Assert.Equal(new byte[] { 0xff, 0x81 }, received.ToArray()); session.ReleasePlayerProcesses();
    }
}
