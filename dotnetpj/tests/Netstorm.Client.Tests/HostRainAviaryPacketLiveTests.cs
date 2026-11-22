using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 같은 Host/Kernel/현재 전역과73h 고정F의 수신 수명을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>같은 실제 미션 장면에서 맨 오 워 풀 상태/생산 예약과 공통 비행 연결을 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>원문25BYTE의 전체 상태 입력을 실제 writer로 구성한다.</summary>
    private static byte[] RainAviaryPacket(uint form, uint parent)
    {
        var fields = new RainAviaryProcessFields(); fields.Bytes[8] = 0xff; BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(36), form); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(40), parent);
        BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(16), 1.25); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(24), 3); byte[] packet = new byte[25]; _ = RawRainAviaryProcess.WriteProcess(fields, packet, true); return packet;
    }
    /// <summary>신규 상태는 같은 현재 Host/Kernel과 Native 전체 예약/현재 헤더를 사용한다.</summary>
    [Fact]
    public void NativeRainAviaryNewReceiveUsesSameHostCurrentTypeAndNativeWriter()
    {
        var sends = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(new() { Broadcast = packet => sends.Add(packet.ToArray()), Connected = _ => false, Send = (_, _) => { } }); int parent = session.BuildPlayerWorldHooks().Create!(143, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.GameAux.RainAviaryProcessTypeBox.Value = 44; int count = session.Kernel.Size(); session.ReceiveFixedGameplayPacket(0x45e5d0, RainAviaryPacket(16000, (uint)parent));
        RainAviaryProcess process = Assert.IsType<RainAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(parent, process.Parent()); Assert.Equal(16000, process.Fields.Form()); Assert.Equal(1.25, process.Fields.Time()); Assert.Equal(3u, process.Fields.Stage()); Assert.Equal(count + 1, session.Kernel.Size()); Assert.Same(process, session.Kernel.Get(session.Kernel.Find(process)));
        session.GameAux.RainAviaryHeader[0] = 0x74; session.GameAux.RainAviaryHeader[1] = 0x81; byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); int before = sends.Count;
        Assert.Equal(25, session.PlayerProcesses.WriteFormPacket(16000, 1, packet, session.GameAux.SquidTransmission)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x74, packet[1]); Assert.Equal(0xa7, packet[25]); session.PlayerProcesses.Transmission()!.Transmit!(25); Assert.Equal(before + 1, sends.Count); Assert.Equal(2048, sends[^1].Length);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.RainAviaryBody); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x45e5d0, packet));
    }
    /// <summary>진단이 바꾼 현재mask/서버/시각/역할을 읽되 최초Form/free는 유지한다.</summary>
    [Fact]
    public void NativeRainAviaryNewDiagnosticKeepsFormAndRereadsCurrentInputs()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(143, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); byte[] packet = RainAviaryPacket(16000, (uint)parent); packet[8] = 0; session.Gameplay.Authority.Server = false; int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (expression, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", expression); Assert.Equal(@"c:\ns\o\_p_rainaviaryprocessdata.cpp", file); Assert.Equal(0x3cu, line); calls++;
            packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); session.GameAux.RainAviaryProcessTypeBox.Value = 44; session.Gameplay.Authority.Server = true; session.ProcessTime.Value = 73.5;
        });
        session.ReceiveFixedGameplayPacket(0x45e5d0, packet); RainAviaryProcess process = Assert.IsType<RainAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Null(session.PlayerProcesses.FromForm(16001)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(1u, process.Fields.Stage()); Assert.Equal(73.5, process.Fields.Time()); Assert.Equal(1, calls); session.ReleasePlayerProcesses();
    }
    /// <summary>기존 수신은 첫Form/PID/부모와dirty를 보존하고 진단 뒤 선택한raw 필드만 쓴다.</summary>
    [Fact]
    public void NativeRainAviaryExistingReceivePreservesIdentityAndPartialEffects()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(143, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.ReceiveFixedGameplayPacket(0x45e5d0, RainAviaryPacket(16000, (uint)parent)); RainAviaryProcess process = Assert.IsType<RainAviaryProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process); process.Fields.Bytes[8] = 0x80; byte[] packet = RainAviaryPacket(16000, 0); int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (expression, file, line) => { Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", expression); Assert.Equal(@"c:\ns\o\_p_rainaviaryprocessdata.cpp", file); Assert.Equal(0x44u, line); calls++; packet[8] = 6; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(9), 0x7ff8123456789abc); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(17), 9); });
        session.ReceiveFixedGameplayPacket(0x45e5d0, packet); Assert.Equal(1, calls); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Equal(parent, process.Parent()); Assert.Equal(9u, process.Fields.Stage()); Assert.Equal(0x80, process.Fields.Dirty()); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(process.Fields.Bytes.AsSpan(16)));
        packet[8] = 6; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(9), 77); Assert.Throws<ArgumentException>(() => session.ReceiveFixedGameplayPacket(0x45e5d0, packet.AsMemory(0, 20))); Assert.Equal(77, process.Fields.Time()); Assert.Equal(9u, process.Fields.Stage()); session.ReleasePlayerProcesses();
    }
    /// <summary>기존PID가 없어도 선택 저장이 없으면 반환하며 실제 time/stage 저장만 실패한다.</summary>
    [Fact]
    public void NativeRainAviaryMissingPidUsesSelectedNullDecoderBoundary()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(143, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int count = session.Kernel.Size(); byte[] packet = RainAviaryPacket((uint)parent, 0); packet[8] = 0;
        session.ReceiveFixedGameplayPacket(0x45e5d0, packet.AsMemory(0, 9)); int calls = 0; surface.Storage.BindDisplayDiagnostics(new(), (_, _, line) => { Assert.Equal(0x44u, line); calls++; }); packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), uint.MaxValue); session.ReceiveFixedGameplayPacket(0x45e5d0, packet.AsMemory(0, 13)); Assert.Equal(1, calls);
        packet[8] = 2; ArgumentException failure = Assert.Throws<ArgumentException>(() => session.ReceiveFixedGameplayPacket(0x45e5d0, packet)); Assert.Contains("객체 필드", failure.Message); Assert.Equal(count, session.Kernel.Size()); Assert.Null(session.PlayerProcesses.FromForm(parent)); session.ReleasePlayerProcesses();
    }
    /// <summary>신규 부모DWORD/실제접두/NULL할당과 선택 누락은 새 가짜객체를 남기지 않는다.</summary>
    [Fact]
    public void NativeRainAviaryBoundsAllocationNullAndMissingEffectsKeepActualHost()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(143, 0); var hooks = new RainAviaryProcessHooks { Allocate = _ => null }; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { RainAviary = hooks }); int count = session.Kernel.Size();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e5d0, new byte[8])); byte[] packet = RainAviaryPacket(16000, (uint)parent); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 0x10003e80); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e5d0, packet));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), 0x10003a98); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e5d0, packet)); Assert.Null(session.PlayerProcesses.FromForm(16000));
        Assert.Throws<OutOfMemoryException>(() => session.RainAviaryBody.PostPop((ushort)parent, 1)); Assert.Equal(count, session.Kernel.Size()); session.ReleasePlayerProcesses();
        (session, types, surface) = SquidPacketScene(); parent = session.BuildPlayerWorldHooks().Create!(143, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); Assert.Throws<InvalidOperationException>(() => session.RainAviaryBody.PostPop((ushort)parent, 1)); session.ReleasePlayerProcesses();
    }
    /// <summary>고정F는표/packet종류교체와무관하고 실제Runtime pin은진단실패/반납요청뒤복귀한다.</summary>
    [Fact]
    public void NativeRainAviaryFixedReceiverKeepsEntryAndActualRuntimePin()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(143, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); using var runtime = HostMissionRuntime.Create(session, surface.Storage); RawPacketDispatch dispatch = GunDispatch(); NativeGameplayPacketReceivers.InstallRainAviary(session.Gameplay, dispatch); CanonicalPacketReceiver receive = dispatch.SnapshotReceiver(0x73)!;
        dispatch.RegisterCanonicalDirect(0x73, 7, (_, _, _) => throw new ArithmeticException("교체된 수신")); byte[] packet = RainAviaryPacket(16000, (uint)parent); packet[1] = 0x3a; receive.Invoke(99, null, packet); Assert.IsType<RainAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.False(session.Gameplay.RuntimeCallActive(session));
        byte[] invalid = RainAviaryPacket(16002, (uint)parent); invalid[8] = 0; var failure = new IOException("명시 원문 진단 중단"); surface.Storage.BindDisplayDiagnostics(new(), (_, _, _) => { Assert.True(session.Gameplay.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => receive.Invoke(77, null, invalid))); Assert.True(runtime.Released); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed()); session.ReleasePlayerProcesses();
    }
    /// <summary>73h 한행만 설치하고 다른표/Player준비를보존하며각metadata불일치를고치지않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)] [InlineData(5)]
    public void NativeRainAviaryRegistrationPreservesOtherRowsAndRejectsMismatch(int mutation)
    {
        var game = new ClientGameplayGlobals(); RawPacketDispatch dispatch = GunDispatch(); dispatch.Version(0x32, 9, 7); NativeGameplayPacketReceivers.InstallRainAviary(game, dispatch); PacketRegistration row = dispatch.Registration(0x73);
        Assert.Equal(0x45e5d0u, row.CanonicalReceive!.NativeEntry); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(0xffff, row.Length); Assert.Null(row.Observer); Assert.Null(row.PacketObserver); using (dispatch.Registrations.BindSession(game)) NativeGameplayPacketReceivers.BindRainAviary(dispatch); Assert.Equal(9, dispatch.Registration(0x32).Version); Assert.Equal(7, dispatch.Registration(0x32).Mismatch); Assert.False(game.PlayerRecords.Initialized);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(0x73, 7, (_, _, _) => { }); break; case 1: row.Version = 2; break; case 2: row.Mismatch = 1; break; case 3: row.Length = 9; break; case 4: row.Observer = _ => { }; break; case 5: row.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = row.CanonicalReceive; Assert.Throws<InvalidOperationException>(() => NativeGameplayPacketReceivers.BindRainAviary(dispatch)); Assert.Same(before, row.CanonicalReceive);
    }
    /// <summary>38/73은현재PID의Sun/Rain두48BYTE배치를같은decoder로갱신한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void NativeAviaryExistingCrossReceiverKeepsSameActualPidAndFields(bool rain)
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(rain ? 143u : 72u, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.ReceiveFixedGameplayPacket(rain ? 0x45e5d0u : 0x45e410u, rain ? RainAviaryPacket(16000, (uint)parent) : SunAviaryPacket(16000, (uint)parent)); SquidProcess current = session.PlayerProcesses.FromForm(16000)!; uint pid = session.Kernel.Find(current);
        byte[] fields = rain ? Assert.IsType<RainAviaryProcess>(current).Fields.Bytes : Assert.IsType<SunAviaryProcess>(current).Fields.Bytes; fields[8] = 0x80;
        byte[] packet = new byte[17]; packet[8] = 2; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(9), 0x7ff8123456789abc);
        session.ReceiveFixedGameplayPacket(rain ? 0x45e410u : 0x45e5d0u, packet); Assert.Same(current, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(current)); Assert.Equal(parent, current.Parent()); Assert.Equal(0x80, fields[8]); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(fields.AsSpan(16))); session.ReleasePlayerProcesses();
    }

    /// <summary>현재fort/friendly/Editor/server/clock과같은Host의정수난수홈통지를연결한다.</summary>
    [Fact]
    public void NativeRainAviaryCurrentControlsAndHomeUseActualSameHostAndRandom()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(143, 0); var events = new List<string>(); int allocations = 0; var packets = new List<byte[]>();
        var hooks = new RainAviaryProcessHooks
        {
            Allocate = size => { Assert.Equal(48, size); allocations++; return new(0xa5); }, Damageable = (_, _) => events.Add("base"),
            SetFrame = (sid, frame) => { BinaryPrimitives.WriteInt32LittleEndian(session.Pool.AllocatedBytes(sid)[36..], frame); events.Add($"frame:{frame}"); }, Sound = (_, _, name) => { Assert.Equal("manOWarBuild.wav", name); events.Add("sound"); },
        };
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { RainAviary = hooks, FormPacket = packet => { Assert.Equal(new[] { 25, 21, 21 }[packets.Count], packet.Length); Assert.Equal(new byte[] { 0xff, 6, 6 }[packets.Count], packet.Span[8]); Assert.Equal(1, packet.Span[0]); Assert.Equal(0x73, packet.Span[1]); packets.Add(packet.ToArray()); } });
        hooks.Allocate = _ => throw new IOException("외부 함수 교체"); session.Gameplay.Player.FortMode = true; session.GameAux.MissionSettings[3] = 0; int count = session.Kernel.Size(); session.RainAviaryBody.PostPop((ushort)parent, 1); Assert.Equal(0, allocations); Assert.Equal(count, session.Kernel.Size()); Assert.Equal(new[] { "base" }, events);
        session.GameAux.MissionSettings[3] = 1; session.GameAux.RainAviaryHeader[0] = 0x74; session.GameAux.RainAviaryHeader[1] = 0x81; session.RainAviaryBody.PostPop((ushort)parent, 1); Assert.Equal(1, allocations); Assert.Single(packets); int form = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]); RainAviaryProcess process = Assert.IsType<RainAviaryProcess>(session.PlayerProcesses.FromForm(form));
        session.Gameplay.World.Editor = 1; session.Kernel.RunFrame(); Assert.Equal(1u, process.Fields.Stage()); session.Gameplay.World.Editor = 0; session.Kernel.RunFrame(); Assert.Equal(2u, process.Fields.Stage()); Assert.Contains("sound", events);
        session.Gameplay.Authority.Server = false; session.ProcessTime.Value = 200; BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(24), 6); process.RunFrame(); Assert.Equal(5u, process.Fields.Stage()); Assert.Equal(203, process.Fields.Time()); session.PlayerProcesses.RefreshForm(process); Assert.Single(packets); Assert.Equal(6, process.Fields.Dirty());
        session.Gameplay.Authority.Server = true; session.PlayerProcesses.RefreshForm(process); Assert.Equal(2, packets.Count); var expected = new GameRandom(session.Game.State()); uint delay = (expected.Next(5000) + 10000) / 1000;
        session.RainAviaryBody.FlyerDied(process.Fields, session.Game, fields => { Assert.Same(process.Fields, fields); session.PlayerProcesses.RefreshForm(process); }); Assert.Equal(10u, process.Fields.Stage()); Assert.Equal(200 + (double)delay, process.Fields.Time()); Assert.Equal(expected.State(), session.Game.State()); Assert.Equal(0, process.Fields.Dirty()); Assert.Equal(3, packets.Count); session.ReleasePlayerProcesses();
    }
}
