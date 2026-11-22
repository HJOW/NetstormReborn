using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 같은 Host/Kernel/현재 전역과38h 고정F의 수신 수명을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>같은 실제 미션 장면에서 훨리베이스 상태/생산 예약과 공통 비행 연결을 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>원문25BYTE의 전체 상태 입력을 실제 writer로 구성한다.</summary>
    private static byte[] SunAviaryPacket(uint form, uint parent)
    {
        var fields = new SunAviaryProcessFields(); fields.Bytes[8] = 0xff; BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(36), form); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(40), parent);
        BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(16), 1.25); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(24), 3); byte[] packet = new byte[25]; _ = RawSunAviaryProcess.WriteProcess(fields, packet, true); return packet;
    }
    /// <summary>신규 상태는 같은 현재 Host/Kernel과 Native 전체 예약/현재 헤더를 사용한다.</summary>
    [Fact]
    public void NativeSunAviaryNewReceiveUsesSameHostCurrentTypeAndNativeWriter()
    {
        var sends = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(new() { Broadcast = packet => sends.Add(packet.ToArray()), Connected = _ => false, Send = (_, _) => { } }); int parent = session.BuildPlayerWorldHooks().Create!(72, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.GameAux.SunAviaryProcessTypeBox.Value = 44; int count = session.Kernel.Size(); session.ReceiveFixedGameplayPacket(0x45e410, SunAviaryPacket(16000, (uint)parent));
        SunAviaryProcess process = Assert.IsType<SunAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(parent, process.Parent()); Assert.Equal(16000, process.Fields.Form()); Assert.Equal(1.25, process.Fields.Time()); Assert.Equal(3u, process.Fields.Stage()); Assert.Equal(count + 1, session.Kernel.Size()); Assert.Same(process, session.Kernel.Get(session.Kernel.Find(process)));
        session.GameAux.SunAviaryHeader[0] = 0x73; session.GameAux.SunAviaryHeader[1] = 0x81; byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); int before = sends.Count;
        Assert.Equal(25, session.PlayerProcesses.WriteFormPacket(16000, 1, packet, session.GameAux.SquidTransmission)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x73, packet[1]); Assert.Equal(0xa7, packet[25]); session.PlayerProcesses.Transmission()!.Transmit!(25); Assert.Equal(before + 1, sends.Count); Assert.Equal(2048, sends[^1].Length);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.SunAviaryBody); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x45e410, packet));
    }
    /// <summary>진단이 바꾼 현재mask/서버/시각/역할을 읽되 최초Form/free는 유지한다.</summary>
    [Fact]
    public void NativeSunAviaryNewDiagnosticKeepsFormAndRereadsCurrentInputs()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(72, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); byte[] packet = SunAviaryPacket(16000, (uint)parent); packet[8] = 0; session.Gameplay.Authority.Server = false; int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (expression, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", expression); Assert.Equal(@"c:\ns\o\_p_sunaviaryprocessdata.cpp", file); Assert.Equal(0x3cu, line); calls++;
            packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); session.GameAux.SunAviaryProcessTypeBox.Value = 44; session.Gameplay.Authority.Server = true; session.ProcessTime.Value = 73.5;
        });
        session.ReceiveFixedGameplayPacket(0x45e410, packet); SunAviaryProcess process = Assert.IsType<SunAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Null(session.PlayerProcesses.FromForm(16001)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(1u, process.Fields.Stage()); Assert.Equal(73.5, process.Fields.Time()); Assert.Equal(1, calls); session.ReleasePlayerProcesses();
    }
    /// <summary>기존 수신은 첫Form/PID/부모와dirty를 보존하고 진단 뒤 선택한raw 필드만 쓴다.</summary>
    [Fact]
    public void NativeSunAviaryExistingReceivePreservesIdentityAndPartialEffects()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(72, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.ReceiveFixedGameplayPacket(0x45e410, SunAviaryPacket(16000, (uint)parent)); SunAviaryProcess process = Assert.IsType<SunAviaryProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process); process.Fields.Bytes[8] = 0x80; byte[] packet = SunAviaryPacket(16000, 0); int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (expression, file, line) => { Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", expression); Assert.Equal(@"c:\ns\o\_p_sunaviaryprocessdata.cpp", file); Assert.Equal(0x44u, line); calls++; packet[8] = 6; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(9), 0x7ff8123456789abc); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(17), 9); });
        session.ReceiveFixedGameplayPacket(0x45e410, packet); Assert.Equal(1, calls); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Equal(parent, process.Parent()); Assert.Equal(9u, process.Fields.Stage()); Assert.Equal(0x80, process.Fields.Dirty()); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(process.Fields.Bytes.AsSpan(16)));
        packet[8] = 6; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(9), 77); Assert.Throws<ArgumentException>(() => session.ReceiveFixedGameplayPacket(0x45e410, packet.AsMemory(0, 20))); Assert.Equal(77, process.Fields.Time()); Assert.Equal(9u, process.Fields.Stage()); session.ReleasePlayerProcesses();
    }
    /// <summary>기존PID가 없어도 선택 저장이 없으면 반환하며 실제 time/stage 저장만 실패한다.</summary>
    [Fact]
    public void NativeSunAviaryMissingPidUsesSelectedNullDecoderBoundary()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(72, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int count = session.Kernel.Size(); byte[] packet = SunAviaryPacket((uint)parent, 0); packet[8] = 0;
        session.ReceiveFixedGameplayPacket(0x45e410, packet.AsMemory(0, 9)); int calls = 0; surface.Storage.BindDisplayDiagnostics(new(), (_, _, line) => { Assert.Equal(0x44u, line); calls++; }); packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), uint.MaxValue); session.ReceiveFixedGameplayPacket(0x45e410, packet.AsMemory(0, 13)); Assert.Equal(1, calls);
        packet[8] = 2; ArgumentException failure = Assert.Throws<ArgumentException>(() => session.ReceiveFixedGameplayPacket(0x45e410, packet)); Assert.Contains("객체 필드", failure.Message); Assert.Equal(count, session.Kernel.Size()); Assert.Null(session.PlayerProcesses.FromForm(parent)); session.ReleasePlayerProcesses();
    }
    /// <summary>신규 부모DWORD/실제접두/NULL할당과 선택 누락은 새 가짜객체를 남기지 않는다.</summary>
    [Fact]
    public void NativeSunAviaryBoundsAllocationNullAndMissingEffectsKeepActualHost()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(72, 0); var hooks = new SunAviaryProcessHooks { Allocate = _ => null }; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { SunAviary = hooks }); int count = session.Kernel.Size();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e410, new byte[8])); byte[] packet = SunAviaryPacket(16000, (uint)parent); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 0x10003e80); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e410, packet));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), 0x10003a98); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e410, packet)); Assert.Null(session.PlayerProcesses.FromForm(16000));
        Assert.Throws<OutOfMemoryException>(() => session.SunAviaryBody.PostPop((ushort)parent, 1)); Assert.Equal(count, session.Kernel.Size()); session.ReleasePlayerProcesses();
        (session, types, surface) = SquidPacketScene(); parent = session.BuildPlayerWorldHooks().Create!(72, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); Assert.Throws<InvalidOperationException>(() => session.SunAviaryBody.PostPop((ushort)parent, 1)); session.ReleasePlayerProcesses();
    }
    /// <summary>같은 현재Editor/clock/서버와 copied 효과를 실제Kernel/Host/공통Flyer Bind/홈통지에 연결한다.</summary>
    [Fact]
    public void NativeSunAviaryCurrentFrameProductionAndHomeUseActualSameObjects()
    {
        var (session, types, surface) = SquidPacketScene(); PlayerWorldHooks world = session.BuildPlayerWorldHooks(); int parent = world.Create!(72, 0); types[73].Theme = 2; types[73].Flags2 |= 0x100000; var events = new List<string>(); SunAviaryProcess? process = null; int flyer = 0;
        var hooks = new SunAviaryProcessHooks
        {
            Allocate = _ => new(0xa5), Damageable = (_, _) => events.Add("base"), SetFrame = (sid, frame) => { events.Add($"frame:{sid}:{frame}"); BinaryPrimitives.WriteInt32LittleEndian(session.Pool.AllocatedBytes(sid)[36..], frame); }, Sound = (_, _, _) => events.Add("sound"),
            Create = (type, flags) => { Assert.Equal(73u, type); Assert.Equal(0u, flags); flyer = world.Create!(type, flags); session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(16001, (uint)flyer)); return (ushort)flyer; },
            Pop = (sid, _, _, flags) => { Assert.Equal(flyer, sid); Assert.Equal(0u, flags); events.Add("pop"); }, Unpop = (_, _) => events.Add("unpop"), Repop = (_, _) => events.Add("repop"),
        };
        var packets = new List<byte[]>(); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { SunAviary = hooks, FormPacket = packet => { Assert.Equal(new[] { 25, 21, 13 }[packets.Count], packet.Length); Assert.Equal(new byte[] { 0xff, 6, 4 }[packets.Count], packet.Span[8]); Assert.Equal(1, packet.Span[0]); Assert.Equal(0x38, packet.Span[1]); packets.Add(packet.ToArray()); } });
        session.GameAux.SunAviaryHeader[0] = 0x73; session.GameAux.SunAviaryHeader[1] = 0x81;
        hooks.SetFrame = (_, _) => throw new IOException("외부 함수 교체"); session.SunAviaryBody.PostPop((ushort)parent, 1); int form = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]); process = Assert.IsType<SunAviaryProcess>(session.PlayerProcesses.FromForm(form)); Assert.Equal(new[] { "base" }, events); Assert.Single(packets);
        session.Gameplay.World.Editor = 1; process.RunFrame(); Assert.Equal(new[] { "base" }, events); session.Gameplay.World.Editor = 0; process.RunFrame(); Assert.Equal(new[] { "base", $"frame:{parent}:27", "sound" }, events); Assert.Equal(8u, process.Fields.Stage());
        BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(24), 6); session.ProcessTime.Value = 100; process.RunFrame(); FlyerProcess flying = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16001)); Assert.Equal(parent, flying.Fields.Home()); Assert.Equal(flyer, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[12..])); Assert.Equal(3u, process.Fields.Stage()); Assert.Equal(103, process.Fields.Time()); Assert.Equal(0, process.Fields.Dirty()); Assert.Contains("pop", events);
        session.SunAviaryBody.FlyerDied((ushort)parent); Assert.Equal(8u, process.Fields.Stage()); Assert.Equal(27u, BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(parent)[36..])); Assert.Equal(new[] { "unpop", "repop" }, events.TakeLast(2)); Assert.Equal(0, process.Fields.Dirty()); Assert.Equal(3, packets.Count);
        byte[] other = new byte[64]; Assert.Equal(45, session.PlayerProcesses.WriteFormPacket(flying.Form(), 1, other, session.GameAux.SquidTransmission)); Assert.Equal(0x39, other[1]); session.ReleasePlayerProcesses();
    }
    /// <summary>고정F는표/packet종류교체와무관하고 실제Runtime pin은진단실패/반납요청뒤복귀한다.</summary>
    [Fact]
    public void NativeSunAviaryFixedReceiverKeepsEntryAndActualRuntimePin()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(72, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); using var runtime = HostMissionRuntime.Create(session, surface.Storage); RawPacketDispatch dispatch = GunDispatch(); NativeGameplayPacketReceivers.InstallSunAviary(session.Gameplay, dispatch); CanonicalPacketReceiver receive = dispatch.SnapshotReceiver(0x38)!;
        dispatch.RegisterCanonicalDirect(0x38, 7, (_, _, _) => throw new ArithmeticException("교체된 수신")); byte[] packet = SunAviaryPacket(16000, (uint)parent); packet[1] = 0x3a; receive.Invoke(99, null, packet); Assert.IsType<SunAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.False(session.Gameplay.RuntimeCallActive(session));
        byte[] invalid = SunAviaryPacket(16002, (uint)parent); invalid[8] = 0; var failure = new IOException("명시 원문 진단 중단"); surface.Storage.BindDisplayDiagnostics(new(), (_, _, _) => { Assert.True(session.Gameplay.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => receive.Invoke(77, null, invalid))); Assert.True(runtime.Released); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed()); session.ReleasePlayerProcesses();
    }
    /// <summary>38h 한행만 설치하고 다른표/Player준비를보존하며각metadata불일치를고치지않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)] [InlineData(5)]
    public void NativeSunAviaryRegistrationPreservesOtherRowsAndRejectsMismatch(int mutation)
    {
        var game = new ClientGameplayGlobals(); RawPacketDispatch dispatch = GunDispatch(); dispatch.Version(0x32, 9, 7); NativeGameplayPacketReceivers.InstallSunAviary(game, dispatch); PacketRegistration row = dispatch.Registration(0x38);
        Assert.Equal(0x45e410u, row.CanonicalReceive!.NativeEntry); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(0xffff, row.Length); Assert.Null(row.Observer); Assert.Null(row.PacketObserver); using (dispatch.Registrations.BindSession(game)) NativeGameplayPacketReceivers.BindSunAviary(dispatch); Assert.Equal(9, dispatch.Registration(0x32).Version); Assert.Equal(7, dispatch.Registration(0x32).Mismatch); Assert.False(game.PlayerRecords.Initialized);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(0x38, 7, (_, _, _) => { }); break; case 1: row.Version = 2; break; case 2: row.Mismatch = 1; break; case 3: row.Length = 9; break; case 4: row.Observer = _ => { }; break; case 5: row.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = row.CanonicalReceive; Assert.Throws<InvalidOperationException>(() => NativeGameplayPacketReceivers.BindSunAviary(dispatch)); Assert.Same(before, row.CanonicalReceive);
    }
}
