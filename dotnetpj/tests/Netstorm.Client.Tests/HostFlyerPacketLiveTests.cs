using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 현재 실제 Flyer 객체/상태/고정F와 Runtime 배송 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>같은 실제 미션 장면에서39h/86h를 수신하며 비행 물리나 공격을 실행하지 않는다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 writer의45BYTE 전체 상태 입력이다. 결과 기대값은 별도로 확인한다.</summary>
    private static byte[] FlyerPacket(uint form, uint parent)
    {
        var fields = new FlyerProcessFields(); RawFlyerProcess.InitializeReceived(fields, 1); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(52), form); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(56), parent);
        BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(10), 23); BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(12), 61); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(16), 0x3f400000);
        BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(32), 1.25); BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(40), 2.5); byte[] packet = new byte[45]; _ = RawFlyerProcess.WriteProcess(fields, packet, true); return packet;
    }
    /// <summary>원문이 command1/2/3에서 읽는16BYTE 입력이다. 선언 길이20과 직접 입구를 구별한다.</summary>
    private static byte[] FlyerCommand(uint parent, uint command)
    {
        byte[] packet = new byte[16]; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), parent); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(12), command); return packet;
    }

    /// <summary>새 객체는 현재 부모theme로 leaf를 고르고 실제 Host/Native 현재 헤더/전체 writer를 사용한다.</summary>
    [Theory]
    [InlineData(0, 0x509f98u)] [InlineData(1, 0x509f60u)] [InlineData(2, 0x509f24u)] [InlineData(-1, 0x509f24u)]
    public void NativeFlyerNewReceiveChoosesCurrentThemeAndSameNativeWriter(int theme, uint table)
    {
        var sends = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(new() { Broadcast = packet => sends.Add(packet.ToArray()), Connected = _ => false, Send = (_, _) => { } });
        int parent = session.BuildPlayerWorldHooks().Create!(144, 0); types[144].Theme = theme; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int count = session.Kernel.Size();
        session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(16000, (uint)parent)); FlyerProcess process = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16000));
        Assert.Equal(table, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes)); Assert.Equal(parent, process.Parent()); Assert.Equal(16000, process.Form()); Assert.Equal(23, process.Fields.Home()); Assert.Equal(1.25, BinaryPrimitives.ReadDoubleLittleEndian(process.Fields.Bytes.AsSpan(32)));
        Assert.Same(process, session.Kernel.Get(session.Kernel.Find(process))); Assert.Equal(count + 1, session.Kernel.Size()); Assert.Equal(31, session.Pool.Slot(16000)[10]);
        Assert.Equal("고정수신의실제선택본문/효과미연결", Assert.Throws<InvalidOperationException>(() => process.RunFrame()).Message);
        session.GameAux.FlyerProcessHeader[0] = 0x73; session.GameAux.FlyerProcessHeader[1] = 0x81; byte[] packet = Enumerable.Repeat((byte)0xa5, 64).ToArray(); int before = sends.Count;
        Assert.Equal(45, session.PlayerProcesses.WriteFormPacket(16000, 1, packet, session.GameAux.SquidTransmission)); Assert.Equal(0x81, packet[0]); Assert.Equal(0x73, packet[1]); Assert.Equal(0xa5, packet[45]);
        session.PlayerProcesses.Transmission()!.Transmit!(45); Assert.Equal(before + 1, sends.Count); Assert.Equal(2048, sends[^1].Length);
        session.ReleasePlayerProcesses(); Assert.Throws<InvalidOperationException>(() => session.FlyerBody); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x45e2b0, packet));
    }

    /// <summary>신규 진단 복귀 뒤 최초 Form을 보존하고 현재 TYPE/theme/패킷 본문을 사용한다.</summary>
    [Fact]
    public void NativeFlyerNewDiagnosticKeepsCapturedFormAndCurrentThemeTypeAndPacket()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(144, 0); types[144].Theme = 1; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        byte[] packet = FlyerPacket(16000, (uint)parent); packet[8] = 0; int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", text); Assert.Equal(@"c:\ns\o\_p_baseflyerprocessdata.cpp", file); Assert.Equal(0x60u, line); calls++;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); packet[8] = 0xff; types[144].Theme = 0; session.GameAux.FlyerProcessTypeBox.Value = 44; BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(29), 9.5);
        });
        session.ReceiveFixedGameplayPacket(0x45e2b0, packet); FlyerProcess process = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16000));
        Assert.Equal(1, calls); Assert.Equal(0x509f98u, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes)); Assert.Equal(44, session.Pool.Slot(16000)[10]); Assert.Equal(9.5, BinaryPrimitives.ReadDoubleLittleEndian(process.Fields.Bytes.AsSpan(32))); Assert.Equal(1, session.Pool.Slot(16001)[11] & 1); session.ReleasePlayerProcesses();
    }

    /// <summary>기존 수신은 같은 PID/부모/leaf/dirty를 보존하고 진단 뒤 현재 부분 필드만 쓴다.</summary>
    [Fact]
    public void NativeFlyerExistingUpdatePreservesIdentityAndRawDoubleBits()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(144, 0); types[144].Theme = 2; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(16000, (uint)parent)); FlyerProcess process = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process); int count = session.Kernel.Size(); process.Fields.Bytes[8] = 0x80;
        byte[] packet = FlyerPacket(16000, uint.MaxValue); int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", text); Assert.Equal(@"c:\ns\o\_p_baseflyerprocessdata.cpp", file); Assert.Equal(0x76u, line); calls++;
            packet[8] = 0x40; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); BinaryPrimitives.WriteUInt64LittleEndian(packet.AsSpan(9), 0x7ff8123456789abc);
        });
        session.ReceiveFixedGameplayPacket(0x45e2b0, packet); Assert.Equal(1, calls); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Equal(count, session.Kernel.Size()); Assert.Equal(parent, process.Parent());
        Assert.Equal(0x509f24u, BinaryPrimitives.ReadUInt32LittleEndian(process.Fields.Bytes)); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(process.Fields.Bytes.AsSpan(32))); Assert.Equal(0x80, process.Fields.Bytes[8]); session.ReleasePlayerProcesses();
    }

    /// <summary>현재 역할/시각을 소비해 세 명령의 기한만 바꾸고 dirty/예약/첫Frame은 추가하지 않는다.</summary>
    [Theory]
    [InlineData(1u, 105.0)] [InlineData(2u, 160.0)] [InlineData(3u, 0.0)]
    public void NativeFlyerCommandsUseCurrentTypeClockAndExistingFields(uint command, double expected)
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(144, 0); types[144].Theme = 2; session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); session.GameAux.FlyerProcessTypeBox.Value = 44;
        session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(16000, (uint)parent)); FlyerProcess process = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process); int count = session.Kernel.Size();
        session.ProcessTime.Value = 100; process.Fields.Bytes[8] = 0x80; BinaryPrimitives.WriteDoubleLittleEndian(process.Fields.Bytes.AsSpan(104), 90);
        session.ReceiveFixedGameplayPacket(0x426490, FlyerCommand((uint)parent, command)); Assert.Equal(expected, BinaryPrimitives.ReadDoubleLittleEndian(process.Fields.Bytes.AsSpan(command == 1 ? 104 : 40)));
        Assert.Equal(0x80, process.Fields.Bytes[8]); Assert.Equal(count, session.Kernel.Size()); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); session.ReleasePlayerProcesses();
    }

    /// <summary>미지 명령은 parent/비행 본체/장치 설치를 요구하지 않고16BYTE에서 자연 반환한다.</summary>
    [Theory]
    [InlineData(0u)] [InlineData(4u)] [InlineData(uint.MaxValue)]
    public void NativeFlyerUnknownCommandsNeverReadParentOrRequireBody(uint command)
    {
        var (session, _, _) = SquidPacketScene(); int count = session.Kernel.Size(); session.ReceiveFixedGameplayPacket(0x426490, FlyerCommand(uint.MaxValue, command)); Assert.Equal(count, session.Kernel.Size()); session.ReleasePlayerProcesses();
    }

    /// <summary>명령 진단에서 역할이 바뀌어도 최초Form0을 재조회하거나 새 객체로 대신하지 않는다.</summary>
    [Fact]
    public void NativeFlyerCommandDiagnosticDoesNotRetryAfterRoleChanges()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(144, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(16000, (uint)parent)); FlyerProcess process = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16000)); byte[] before = process.Fields.Bytes.ToArray(); session.GameAux.FlyerProcessTypeBox.Value = 44; int calls = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) => { Assert.Equal("processSid", text); Assert.Equal(@"\Ns\O\Flyer.cpp", file); Assert.Equal(0xe4u, line); calls++; session.GameAux.FlyerProcessTypeBox.Value = 31; });
        Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x426490, FlyerCommand((uint)parent, 2))); Assert.Equal(1, calls); Assert.Equal(before, process.Fields.Bytes); session.ReleasePlayerProcesses();
    }

    /// <summary>선택된 머리/부모 DWORD/현재 PID 경계는 앞선 생성 소유권을 반납하고 거부한다.</summary>
    [Fact]
    public void NativeFlyerBoundsAndMissingPidNeverCreateReplacementObjects()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(144, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int count = session.Kernel.Size();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e2b0, new byte[8])); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(0, 0)));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(0x10003e80, (uint)parent))); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x45e2b0, FlyerPacket(16000, 0x10003a98)));
        byte[] missing = FlyerPacket((uint)parent, 0); missing[8] = 0; Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x45e2b0, missing)); Assert.Equal(count, session.Kernel.Size()); Assert.Null(session.PlayerProcesses.FromForm(16000));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x426490, FlyerCommand(uint.MaxValue, 1))); Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x426490, new byte[12])); session.ReleasePlayerProcesses();
    }

    /// <summary>두 고정F는 현재 표/packet종류로 다시 선택하지 않고 실제 Runtime pin을 실패 때도 반납한다.</summary>
    [Fact]
    public void NativeFlyerFixedReceiversKeepCapturedEntryAndActualRuntimePin()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(144, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); using var runtime = HostMissionRuntime.Create(session, surface.Storage);
        RawPacketDispatch dispatch = GunDispatch(); NativeGameplayPacketReceivers.InstallFlyer(session.Gameplay, dispatch); CanonicalPacketReceiver receive = dispatch.SnapshotReceiver(0x39)!, command = dispatch.SnapshotReceiver(0x86)!;
        dispatch.RegisterCanonicalDirect(0x39, 7, (_, _, _) => throw new ArithmeticException("교체된 수신")); dispatch.RegisterCanonicalDirect(0x86, 8, (_, _, _) => throw new ArithmeticException("교체된 명령")); byte[] packet = FlyerPacket(16000, (uint)parent); packet[1] = 0x3a;
        receive.Invoke(99, null, packet); FlyerProcess process = Assert.IsType<FlyerProcess>(session.PlayerProcesses.FromForm(16000)); session.ProcessTime.Value = 100; command.Invoke(88, null, FlyerCommand((uint)parent, 3)); Assert.Equal(0ul, BinaryPrimitives.ReadUInt64LittleEndian(process.Fields.Bytes.AsSpan(40))); Assert.False(session.Gameplay.RuntimeCallActive(session));
        session.GameAux.FlyerProcessTypeBox.Value = 44; var failure = new IOException("명시 비행 진단 중단"); surface.Storage.BindDisplayDiagnostics(new(), (_, _, _) => { Assert.True(session.Gameplay.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => command.Invoke(77, null, FlyerCommand((uint)parent, 2)))); Assert.True(runtime.Released); Assert.False(session.Gameplay.Borrowed()); Assert.False(session.Gameplay.RuntimeCallActive(session)); session.ReleasePlayerProcesses();
    }

    /// <summary>두 행의 NULL/빈observer를 구별하고 원문F/metadata 불일치를 자동 수정하지 않는다.</summary>
    [Theory]
    [InlineData(0x39u, 0)] [InlineData(0x39u, 1)] [InlineData(0x39u, 2)] [InlineData(0x39u, 3)] [InlineData(0x39u, 4)] [InlineData(0x39u, 5)]
    [InlineData(0x86u, 0)] [InlineData(0x86u, 1)] [InlineData(0x86u, 2)] [InlineData(0x86u, 3)] [InlineData(0x86u, 4)] [InlineData(0x86u, 5)]
    public void NativeFlyerRegistrationChecksExactObserverAndMetadata(uint type, int mutation)
    {
        var game = new ClientGameplayGlobals(); RawPacketDispatch dispatch = GunDispatch(); dispatch.Version(0x32, 9, 7); NativeGameplayPacketReceivers.InstallFlyer(game, dispatch); PacketRegistration row = dispatch.Registration(type);
        Assert.Equal(type == 0x39 ? 0x45e2b0u : 0x426490u, row.CanonicalReceive!.NativeEntry); Assert.Equal(type == 0x39 ? 0xffff : 20, row.Length); Assert.Equal(type == 0x86, row.Observer is not null); Assert.Null(row.PacketObserver); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch);
        using (dispatch.Registrations.BindSession(game)) NativeGameplayPacketReceivers.BindFlyer(dispatch, type); Assert.False(game.PlayerRecords.Initialized); Assert.Equal(9, dispatch.Registration(0x32).Version); Assert.Equal(7, dispatch.Registration(0x32).Mismatch);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(type, 7, (_, _, _) => { }); break;
            case 1: row.Version = 2; break;
            case 2: row.Mismatch = 1; break;
            case 3: row.Length = 9; break;
            case 4: row.Observer = type == 0x39 ? _ => { } : null; break;
            case 5: row.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = row.CanonicalReceive; Assert.Throws<InvalidOperationException>(() => NativeGameplayPacketReceivers.BindFlyer(dispatch, type)); Assert.Same(before, row.CanonicalReceive); Assert.Throws<ArgumentException>(() => NativeGameplayPacketReceivers.BindFlyer(dispatch, 0xfe));
    }
}
