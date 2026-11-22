using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 같은 Host/Kernel/현재 전역과74h 고정F의 수신 수명을 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>같은 실제 미션 장면에서 데빌메이커 상태/생산 예약과 공통 비행 연결을 대조한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제공유Surface/Template/Native삭제장치를준비해같은Host의생산후종료까지실행한다.</summary>
    private static (HostSession Session, IReadOnlyList<OriginalTypeRecord> Types, ClientSurfaceGlobals Surface, CampaignTemplateStorage Templates) WindNativeScene(Action<byte[]> broadcast)
    {
        var game = new ClientGameplayGlobals(); game.Authority.Server = true; var local = new ClientLocalStateGlobals(game, new()); var templates = new CampaignTemplateStorage();
        var surface = new ClientSurfaceGlobals(game, new(local), new(), () => 0, new() { Broadcast = broadcast, Connected = _ => false, Send = (_, _) => { } }, installDevices: owner =>
        {
            owner.Storage.BindSerializationGlobals(templates.SerializationGlobals); owner.Storage.BindGraphTrace((_, _) => { }); owner.Storage.BindDisplayDiagnostics(new(), (_, _, _) => { });
            owner.Storage.BindProcessDevices(new() { Selected = () => game.Selection.Selected.Value, ClearSelection = () => game.Selection.Selected.Value = 0 });
            owner.Storage.BindDisplayDevices(new() { View = () => new(), Suppressed = () => true, Invalidate = (_, _) => throw new IOException("미선택그림출력") });
        });
        var session = new HostSession(surface.Kernel, new(1), new(), sharedSurface: surface, sharedTemplates: templates); RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, session.BattleOptions);
        IReadOnlyList<OriginalTypeRecord> types = HostPostPopRuntimeTests.Build(session, surface.Storage, true); _ = session.BuildPlayerWorldHooks(); return (session, types, surface, templates);
    }
    /// <summary>원문45BYTE의 전체 상태 입력을 실제 writer로 구성한다.</summary>
    private static byte[] WindAviaryPacket(uint form, uint parent)
    {
        var fields = new WindAviaryProcessFields(); RawWindAviaryState.Initialize(fields, false, 0); fields.Bytes[8] = 0xff; BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(60), form); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(64), parent);
        BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(16), 1.25); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(24), 3); BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(32), 2.5); BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(40), 3.5); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(48), 0); byte[] packet = new byte[45]; _ = RawWindAviaryState.WriteProcess(fields, packet, true); return packet;
    }
    /// <summary>실제Wind시각8BYTE를읽는다.</summary>
    private static double WindTime(WindAviaryProcessFields fields) => BinaryPrimitives.ReadDoubleLittleEndian(fields.Bytes.AsSpan(16));
    /// <summary>실제Wind단계DWORD를읽는다.</summary>
    private static uint WindStage(WindAviaryProcessFields fields) => BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(24));
    /// <summary>현재Wind FormDWORD의하위WORD를읽는다.</summary>
    private static int WindForm(WindAviaryProcessFields fields) => unchecked((ushort)BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(60)));
    /// <summary>신규 상태는 같은 현재 Host/Kernel과 Native 전체 예약/현재 헤더를 사용한다.</summary>
    [Fact]
    public void NativeWindAviaryNewReceiveUsesSameHostCurrentTypeAndNativeWriter()
    {
        var sends = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(new() { Broadcast = packet => sends.Add(packet.ToArray()), Connected = _ => false, Send = (_, _) => { } }); int parent = session.BuildPlayerWorldHooks().Create!(139, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.GameAux.WindAviaryProcessTypeBox.Value = 44; int count = session.Kernel.Size(); session.ReceiveFixedGameplayPacket(0x45e4f0, WindAviaryPacket(16000, (uint)parent));
        WindAviaryProcess process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(parent, process.Parent()); Assert.Equal(16000, WindForm(process.Fields)); Assert.Equal(1.25, WindTime(process.Fields)); Assert.Equal(3u, WindStage(process.Fields)); Assert.Equal(count + 1, session.Kernel.Size()); Assert.Same(process, session.Kernel.Get(session.Kernel.Find(process)));
        session.GameAux.WindAviaryHeader[0] = 0x73; session.GameAux.WindAviaryHeader[1] = 0x81; byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); int before = sends.Count;
        Assert.Equal(45, session.PlayerProcesses.WriteFormPacket(16000, 1, packet, session.GameAux.SquidTransmission)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x73, packet[1]); Assert.Equal(0xa7, packet[45]); session.PlayerProcesses.Transmission()!.Transmit!(45); Assert.Equal(before + 1, sends.Count); Assert.Equal(2048, sends[^1].Length);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.WindAviaryBody); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x45e4f0, packet));
    }
    /// <summary>진단이 바꾼 현재mask/서버/시각/역할을 읽되 최초Form/free는 유지한다.</summary>
    [Fact]
    public void NativeWindAviaryNewDiagnosticKeepsFormAndRereadsCurrentInputs()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(139, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); byte[] packet = WindAviaryPacket(16000, (uint)parent); packet[8] = 0; session.Gameplay.Authority.Server = false; int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (expression, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", expression); Assert.Equal(@"c:\ns\o\_p_windaviaryprocessdata.cpp", file); Assert.Equal(0x4eu, line); calls++;
            packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); session.GameAux.WindAviaryProcessTypeBox.Value = 44; session.Gameplay.Authority.Server = true; session.ProcessTime.Value = 73.5;
        });
        session.ReceiveFixedGameplayPacket(0x45e4f0, packet); WindAviaryProcess process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.Null(session.PlayerProcesses.FromForm(16001)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(1u, WindStage(process.Fields)); Assert.Equal(73.5, WindTime(process.Fields)); Assert.Equal(1, calls); session.ReleasePlayerProcesses();
    }
    /// <summary>기존수신은첫Form/PID/부모/dirty와진단뒤현재필드·전체packet사전검사를보존한다.</summary>
    [Fact]
    public void NativeWindAviaryExistingReceivePreservesIdentityAndFullPacketPrecheck()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(139, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.ReceiveFixedGameplayPacket(0x45e4f0, WindAviaryPacket(16000, (uint)parent)); WindAviaryProcess process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process); process.Fields.Bytes[8] = 0x80; byte[] packet = WindAviaryPacket(16000, 0); int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (expression, file, line) => { Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", expression); Assert.Equal(@"c:\ns\o\_p_windaviaryprocessdata.cpp", file); Assert.Equal(0x56u, line); calls++; packet[8] = 0x3e; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(9), 0x7ff8123456789abc); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(17), 9); });
        session.ReceiveFixedGameplayPacket(0x45e4f0, packet); Assert.Equal(1, calls); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Equal(parent, process.Parent()); Assert.Equal(9u, WindStage(process.Fields)); Assert.Equal(0x80, process.Fields.Bytes[8]); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(process.Fields.Bytes.AsSpan(16)));
        packet[8] = 0x3e; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(9), 77); byte[] before = process.Fields.Bytes.ToArray(); Assert.Throws<ArgumentException>(() => session.ReceiveFixedGameplayPacket(0x45e4f0, packet.AsMemory(0, 40))); Assert.Equal(before, process.Fields.Bytes); session.ReleasePlayerProcesses();
    }
    /// <summary>기존PID가 없어도 선택 저장이 없으면 반환하며 실제 time/stage 저장만 실패한다.</summary>
    [Fact]
    public void NativeWindAviaryMissingPidUsesSelectedNullDecoderBoundary()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(139, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int count = session.Kernel.Size(); byte[] packet = WindAviaryPacket((uint)parent, 0); packet[8] = 0;
        session.ReceiveFixedGameplayPacket(0x45e4f0, packet.AsMemory(0, 9)); int calls = 0; surface.Storage.BindDisplayDiagnostics(new(), (_, _, line) => { Assert.Equal(0x56u, line); calls++; }); packet[8] = 1; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), uint.MaxValue); session.ReceiveFixedGameplayPacket(0x45e4f0, packet.AsMemory(0, 13)); Assert.Equal(1, calls);
        packet[8] = 2; ArgumentException failure = Assert.Throws<ArgumentException>(() => session.ReceiveFixedGameplayPacket(0x45e4f0, packet)); Assert.Contains("객체 필드", failure.Message); Assert.Equal(count, session.Kernel.Size()); Assert.Null(session.PlayerProcesses.FromForm(parent)); session.ReleasePlayerProcesses();
    }
    /// <summary>신규 부모DWORD/실제접두/NULL할당과 선택 누락은 새 가짜객체를 남기지 않는다.</summary>
    [Fact]
    public void NativeWindAviaryBoundsAllocationNullAndMissingEffectsKeepActualHost()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(139, 0); var hooks = new WindAviaryHooks { Allocate = () => null }; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { WindAviary = hooks }); int count = session.Kernel.Size();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e4f0, new byte[8])); byte[] packet = WindAviaryPacket(16000, (uint)parent); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 0x10003e80); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e4f0, packet));
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16000); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), 0x10003a98); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e4f0, packet)); Assert.Null(session.PlayerProcesses.FromForm(16000));
        Assert.Throws<OutOfMemoryException>(() => session.WindAviaryBody.PostPop((ushort)parent, 1)); Assert.Equal(count, session.Kernel.Size()); session.ReleasePlayerProcesses();
        (session, types, surface) = SquidPacketScene(); parent = session.BuildPlayerWorldHooks().Create!(139, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); Assert.Throws<InvalidOperationException>(() => session.WindAviaryBody.PostPop((ushort)parent, 1)); session.ReleasePlayerProcesses();
    }
    /// <summary>고정F는표/packet종류교체와무관하고 실제Runtime pin은진단실패/반납요청뒤복귀한다.</summary>
    [Fact]
    public void NativeWindAviaryFixedReceiverKeepsEntryAndActualRuntimePin()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(139, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); using var runtime = HostMissionRuntime.Create(session, surface.Storage); RawPacketDispatch dispatch = GunDispatch(); NativeGameplayPacketReceivers.InstallWindAviary(session.Gameplay, dispatch); CanonicalPacketReceiver receive = dispatch.SnapshotReceiver(0x74)!;
        dispatch.RegisterCanonicalDirect(0x74, 7, (_, _, _) => throw new ArithmeticException("교체된 수신")); byte[] packet = WindAviaryPacket(16000, (uint)parent); packet[1] = 0x3a; receive.Invoke(99, null, packet); Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(16000)); Assert.False(session.Gameplay.RuntimeCallActive(session));
        byte[] invalid = WindAviaryPacket(16002, (uint)parent); invalid[8] = 0; var failure = new IOException("명시 원문 진단 중단"); surface.Storage.BindDisplayDiagnostics(new(), (_, _, _) => { Assert.True(session.Gameplay.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => receive.Invoke(77, null, invalid))); Assert.True(runtime.Released); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed()); session.ReleasePlayerProcesses();
    }
    /// <summary>74h 한행만 설치하고 다른표/Player준비를보존하며각metadata불일치를고치지않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)] [InlineData(5)]
    public void NativeWindAviaryRegistrationPreservesOtherRowsAndRejectsMismatch(int mutation)
    {
        var game = new ClientGameplayGlobals(); RawPacketDispatch dispatch = GunDispatch(); dispatch.Version(0x32, 9, 7); NativeGameplayPacketReceivers.InstallWindAviary(game, dispatch); PacketRegistration row = dispatch.Registration(0x74);
        Assert.Equal(0x45e4f0u, row.CanonicalReceive!.NativeEntry); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(0xffff, row.Length); Assert.Null(row.Observer); Assert.Null(row.PacketObserver); using (dispatch.Registrations.BindSession(game)) NativeGameplayPacketReceivers.BindWindAviary(dispatch); Assert.Equal(9, dispatch.Registration(0x32).Version); Assert.Equal(7, dispatch.Registration(0x32).Mismatch); Assert.False(game.PlayerRecords.Initialized);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(0x74, 7, (_, _, _) => { }); break; case 1: row.Version = 2; break; case 2: row.Mismatch = 1; break; case 3: row.Length = 9; break; case 4: row.Observer = _ => { }; break; case 5: row.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = row.CanonicalReceive; Assert.Throws<InvalidOperationException>(() => NativeGameplayPacketReceivers.BindWindAviary(dispatch)); Assert.Same(before, row.CanonicalReceive);
    }

    /// <summary>현재실제SHP의walkStepY와현재TYPE발자국을일반생산좌표에서다시읽는다.</summary>
    [Fact]
    public void NativeWindAviaryNormalProductionReadsCurrentActualShpAndFootprint()
    {
        List<SquidDisplayShape>? shapes = null; var (session, types, surface) = SquidPacketScene(inspectShapes: value => shapes = value); Assert.NotNull(shapes); PlayerWorldHooks world = session.BuildPlayerWorldHooks(); int parent = world.Create!(139, 0); session.Gameplay.Authority.Boss = true; types[139].FootY = 3;
        BinaryPrimitives.WriteSingleLittleEndian(session.Pool.AllocatedBytes(parent)[14..], 50.25f); BinaryPrimitives.WriteSingleLittleEndian(session.Pool.AllocatedBytes(parent)[18..], 60.5f); shapes[139].Frames[0].WalkStepY = 44; (float X, float Y)? popped = null; int flyer = 0;
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { WindAviary = new() { Pop = (sid, x, y) => { flyer = sid; popped = (x, y); } } }); session.ReceiveFixedGameplayPacket(0x45e4f0, WindAviaryPacket(16000, (uint)parent)); WindAviaryProcess process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(16000));
        shapes[139].Frames[0].WalkStepY = 88; BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(24), 2); BinaryPrimitives.WriteDoubleLittleEndian(process.Fields.Bytes.AsSpan(32), 0); BinaryPrimitives.WriteDoubleLittleEndian(process.Fields.Bytes.AsSpan(40), 95); session.ProcessTime.Value = 100; process.RunFrame();
        Assert.Equal((48.85f, 65.5f), popped); Assert.NotEqual(0, flyer); Assert.Equal(138, session.Pool.Slot(flyer)[10]); Assert.Equal(13u, BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(flyer)[36..])); Assert.Equal((uint)flyer, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(48))); Assert.Equal(parent, BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(flyer)[12..])); session.ReleasePlayerProcesses();
    }

    /// <summary>실제원주SHP와nullable104BYTE/현재속도/동일Flyer예약·기본부착뒤전문화/실제종료를연결한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void NativeWindAviaryCircleProductionUsesSameFlyerReservationAndActualFinish(bool allocationFails)
    {
        Action<ReadOnlyMemory<byte>>? broadcast = null; var (session, types, surface, templates) = WindNativeScene(packet => broadcast!(packet)); List<SquidDisplayShape> shapes = surface.Storage.Shapes; PlayerWorldHooks world = session.BuildPlayerWorldHooks(); int parent = world.Create!(139, 0); session.Gameplay.Authority.Boss = true; types[139].FootX = 2; types[139].FootY = 4;
        BinaryPrimitives.WriteSingleLittleEndian(session.Pool.AllocatedBytes(parent)[14..], 50.25f); BinaryPrimitives.WriteSingleLittleEndian(session.Pool.AllocatedBytes(parent)[18..], 60.5f); shapes[138].Frames[13].WalkStepY = 44; shapes[138].Frames[13].Height = 10; shapes[138].Frames[13].HotspotY = 3; shapes[138].Frames[13].CellHeight = 999;
        int allocated = 0, speedReads = 0; float speed = 4; FlyerProcess? flying = null; (float X, float Y)? popped = null; var packets = new List<byte[]>();
        // 실제Native전송/선택장치가설치된수명에서ProcessForm종료의2eh까지검사한다.
        broadcast = packet =>
        {
            Assert.Equal(2048, packet.Length); packets.Add(packet.Span[..45].ToArray());
            if (packet.Span[1] == 0x39) { flying = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm((int)BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]))); Assert.Equal(0x509cc8u, BinaryPrimitives.ReadUInt32LittleEndian(flying.Fields.Bytes)); } else Assert.Equal(0x74, packet.Span[1]);
        };
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()
        {
            WindAviary = new() { Allocate = () => new(0xa5), Damageable = (_, _) => { }, Pop = (_, x, y) => popped = (x, y) },
            FlyerAllocation = size => { Assert.Equal(104, size); allocated++; speed = 6.75f; return allocationFails ? null : Enumerable.Repeat((byte)0xa5, 104).ToArray(); },
            FlyerSpeed = sid => { Assert.False(allocationFails); Assert.Equal(138, session.Pool.Slot(sid)[10]); speedReads++; return speed; },
        });
        session.WindAviaryBody.PostPop((ushort)parent, 1); int form = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]); WindAviaryProcess process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(form)); session.Kernel.RunFrame(); Assert.Equal(2u, WindStage(process.Fields)); shapes[138].Frames[13].Height = 14;
        BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(24), 12); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(48), 0); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(68), 4); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(72), 0); process.RunFrame();
        Assert.Equal((48.85f, 74.5f), popped); Assert.Equal(1, allocated); Assert.Equal(13u, WindStage(process.Fields)); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(48)));
        if (allocationFails) { Assert.Equal(0, speedReads); Assert.Null(flying); Assert.Single(packets); }
        else
        {
            Assert.Equal(1, speedReads); Assert.NotNull(flying); Assert.Equal(2, packets.Count); Assert.Equal(parent, flying.Fields.Home()); Assert.Equal(0x509f98u, BinaryPrimitives.ReadUInt32LittleEndian(flying.Fields.Bytes)); Assert.Equal(BitConverter.SingleToUInt32Bits(6.75f), BinaryPrimitives.ReadUInt32LittleEndian(flying.Fields.Bytes.AsSpan(16))); Assert.All(flying.Fields.Bytes.AsSpan(104, 8).ToArray(), value => Assert.Equal(0, value)); Assert.Equal(0xa5, flying.Fields.Bytes[88]); Assert.Same(flying, session.PlayerProcesses.FromForm(flying.Form()));
        }
        int deleted = 0; broadcast = packet => { Assert.Equal(0x2e, packet.Span[1]); Assert.Equal(form, BinaryPrimitives.ReadUInt16LittleEndian(packet.Span[4..])); deleted++; };
        process.RunFrame(); Assert.Equal(1, deleted); Assert.Null(session.PlayerProcesses.FromForm(form)); Assert.Equal(allocationFails ? 0 : 1, session.Kernel.Size()); session.ReleasePlayerRecordViews(); templates.ClearAll();
    }

    /// <summary>속도조회실패의미부착예약을반납하고같은임시플라이어로재시도하며재진입예약은거부한다.</summary>
    [Fact]
    public void NativeWindAviaryFailedAndReentrantReservationsKeepOneActualFlyer()
    {
        var (session, types, surface) = SquidPacketScene(); PlayerWorldHooks world = session.BuildPlayerWorldHooks(); int parent = world.Create!(139, 0); session.Gameplay.Authority.Boss = true; WindAviaryProcess? process = null; int allocations = 0, speedReads = 0, packets = 0; bool fail = true, reenter = false; var failure = new IOException("명시현재속도조회실패");
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()
        {
            WindAviary = new() { Allocate = () => new(), Damageable = (_, _) => { }, Pop = (_, _, _) => { } }, FlyerAllocation = _ => { allocations++; return new byte[104]; },
            FlyerSpeed = _ => { speedReads++; if (fail) throw failure; if (reenter) { reenter = false; Assert.Throws<InvalidOperationException>(() => process!.RunFrame()); } return 3.25f; }, FormPacket = packet => { Assert.Equal(45, packet.Length); packets++; },
        });
        session.WindAviaryBody.PostPop((ushort)parent, 1); int form = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot(parent)[6..]); process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(form)); session.Kernel.RunFrame(); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(24), 12); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(48), 0); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(68), 4); BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(72), 0);
        Assert.Same(failure, Assert.Throws<IOException>(() => process.RunFrame())); Assert.Equal(1, session.Kernel.Size()); Assert.Equal(12u, WindStage(process.Fields)); uint flyer = BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes.AsSpan(48)); Assert.NotEqual(0u, flyer); fail = false; reenter = true; process.RunFrame(); Assert.Equal(13u, WindStage(process.Fields)); Assert.Equal(2, session.Kernel.Size()); Assert.Equal(2, packets); Assert.Equal(3, allocations); Assert.Equal(2, speedReads);
        ushort flyerForm = BinaryPrimitives.ReadUInt16LittleEndian(session.Pool.Slot((int)flyer)[6..]); FlyerProcess flying = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(flyerForm)); Assert.Equal(BitConverter.SingleToUInt32Bits(3.25f), BinaryPrimitives.ReadUInt32LittleEndian(flying.Fields.Bytes.AsSpan(16))); Assert.Throws<InvalidOperationException>(() => session.FlyerBody.Construct(new(), (ushort)flyer, (ushort)parent, 0, .5f)); Assert.Equal(2, session.Kernel.Size()); session.ReleasePlayerProcesses();
    }

    /// <summary>현재boss와server를별개로읽고서버복귀뒤같은이미지의홈통지만송신한다.</summary>
    [Theory]
    [InlineData(false)] [InlineData(true)]
    public void NativeWindAviaryReadsCurrentBossSeparatelyFromServer(bool boss)
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(139, 0); int sent = 0;
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = packet => { Assert.Equal(13, packet.Length); Assert.Equal(4, packet.Span[8]); sent++; } });
        session.ReceiveFixedGameplayPacket(0x45e4f0, WindAviaryPacket(16000, (uint)parent)); WindAviaryProcess process = Assert.IsType<WindAviaryProcess>(session.PlayerProcesses.FromForm(16000)); process.Fields.Bytes[8] = 0; BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(24), 12);
        session.Gameplay.Authority.Server = false; session.Gameplay.Authority.Boss = boss; process.RunFrame(); Assert.Equal(boss ? 12u : 13u, WindStage(process.Fields));
        byte[] before = process.Fields.Bytes.ToArray(); session.WindAviaryBody.FlyerDied(process.Fields, _ => throw new IOException("비서버홈호출")); Assert.Equal(before, process.Fields.Bytes); Assert.Equal(0, sent);
        session.Gameplay.Authority.Server = true; process.Fields.Bytes[8] = 0; session.WindAviaryBody.FlyerDied(process.Fields, fields => { Assert.Same(process.Fields, fields); session.PlayerProcesses.RefreshForm(process); }); Assert.Equal(2u, WindStage(process.Fields)); Assert.Equal(1, sent); session.ReleasePlayerProcesses();
    }
}
