using System.Buffers.Binary;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 현재 실제 Gun 수신/Host/전송/소유자와 고정F의 배송 수명을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>실제 미션 장면을 공유하며 발사/표적 생성 없이3ah 상태 수신을 검사한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>원문 writer가 만드는31BYTE 전체 상태를 입력으로 마련한다. 출력 기대값은 직접 확인한다.</summary>
    private static byte[] GunPacket(uint form, uint parent, uint stage = 0, ushort target = 0, int group = 1)
    {
        var fields = new GunProcessFields(); fields.Bytes[8] = 0xff; BinaryPrimitives.WriteDoubleLittleEndian(fields.Bytes.AsSpan(16), 1.25);
        BinaryPrimitives.WriteUInt16LittleEndian(fields.Bytes.AsSpan(24), target); BinaryPrimitives.WriteUInt32LittleEndian(fields.Bytes.AsSpan(28), stage); BinaryPrimitives.WriteInt32LittleEndian(fields.Bytes.AsSpan(32), group);
        byte[] packet = new byte[31]; _ = RawGunProcess.WriteData(fields, packet, form, parent, false); return packet;
    }
    /// <summary>설치 검사에서 Session/연결이 없어도 생성할 수 있는 실제 dispatcher다.</summary>
    private static RawPacketDispatch GunDispatch() => new(OriginalEdition.Patch1078, new()
    {
        Now = () => 42, Record = (_, _, _) => { }, Terminate = _ => throw new IOException("미선택 terminate"), Close = _ => throw new IOException("미선택 close"), Release = _ => throw new IOException("미선택 release"),
    });

    /// <summary>실제 Native writer/현재 권한/역할/동맹/친군 사격과 프레임 준비/반납이 같은 owner를 쓴다.</summary>
    [Fact]
    public void NativeGunNewReceiveUsesSameHostCurrentOwnersWriterAndFramePreparation()
    {
        var sent = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(new() { Broadcast = packet => sent.Add(packet.ToArray()), Connected = _ => false, Send = (_, _) => { } });
        int parent = session.BuildPlayerWorldHooks().Create!(128, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(16000, (uint)parent)); var process = Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16000));
        Assert.Equal(parent, process.Parent()); Assert.Same(process, session.Kernel.Get(session.Kernel.Find(process))); Assert.Equal(34, session.Pool.Slot(16000)[10]);
        Assert.Same(session.Gameplay.Authority, session.GunState.SharedAuthority); session.GunState.Boss = true; session.Gameplay.Authority.Boss = false; Assert.False(session.GunState.IsBoss());
        session.GameAux.MissionSettings[3] = 0x80000000; Assert.True(session.GunState.FriendlyFire.IsEnabled); session.Alliances[11] = 7; session.Alliances[19] = 0; Assert.True(session.GunState.IsAlly(1, 2)); Assert.False(session.GunState.IsAlly(2, 1));
        session.GameAux.WindArcherTypeBox.Value = 141; Assert.Equal(141u, session.GunState.ReadWindArcherType());
        session.Gameplay.Player.FortMode = true; session.ProcessTime.Value = 20; session.RunMissionFrame(); Assert.Equal(20, session.GunState.Now); Assert.True(session.GunState.FortMode); Assert.Equal(1u, process.Fields.Stage());
        session.GameAux.GunProcessHeader[0] = 0x73; session.GameAux.GunProcessHeader[1] = 0x81; int before = sent.Count;
        byte[] output = Enumerable.Repeat((byte)0xa5, 64).ToArray(); Assert.Equal(31, session.PlayerProcesses.WriteFormPacket(16000, 1, output, session.GameAux.SquidTransmission));
        Assert.Equal(0x81, output[0]); Assert.Equal(0x73, output[1]); Assert.Equal((uint)parent, BinaryPrimitives.ReadUInt32LittleEndian(output.AsSpan(9))); Assert.Equal(0xa5, output[31]);
        session.PlayerProcesses.Transmission()!.Transmit!(31); Assert.Equal(before + 1, sent.Count); Assert.Equal(2048, sent[^1].Length);
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size()); Assert.Throws<InvalidOperationException>(() => session.GunBody); Assert.Throws<InvalidOperationException>(() => session.GunState);
        Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, output));
    }

    /// <summary>ff 진단 복귀는 최초 Form을 유지하며 현재 패킷과005410f8 TYPE/시각을 다시 읽는다.</summary>
    [Fact]
    public void NativeGunNewDiagnosticKeepsCapturedFormAndCurrentPacketType()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(128, 0); byte[] packet = GunPacket(16000, (uint)parent); packet[8] = 0; int diagnostics = 0;
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", text); Assert.Equal(@"c:\ns\o\_p_gunprocessdata.cpp", file); Assert.Equal(0x4eu, line); diagnostics++;
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); packet[8] = 0xff; session.GameAux.GunProcessTypeBox.Value = 59; session.ProcessTime.Value = 27;
            BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(13), 9.5); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(23), 3);
        });
        session.ReceiveFixedGameplayPacket(0x46a0c0, packet); GunProcess process = Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16000));
        Assert.Equal(1, diagnostics); Assert.Equal(59, session.Pool.Slot(16000)[10]); Assert.Equal(1, session.Pool.Slot(16001)[11] & 1); Assert.Equal(9.5, process.Fields.Time()); Assert.Equal(2u, process.Fields.Stage()); Assert.Equal(27, session.GunState.Now);
        session.ReleasePlayerProcesses();
    }

    /// <summary>기존 PID 갱신의 전 observer는 현재 역할의 실제 Kernel 객체를40삭제하고 현재 상태만 읽는다.</summary>
    [Fact]
    public void NativeGunExistingUpdateStopsCurrentAnimationAndPreservesIdentity()
    {
        var (session, types, surface) = SquidPacketScene(new() { Broadcast = _ => { }, Connected = _ => false, Send = (_, _) => { } });
        int parent = session.BuildPlayerWorldHooks().Create!(128, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(16000, (uint)parent)); GunProcess process = Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16000)); uint pid = session.Kernel.Find(process);
        var animation = new SquidProcess(); Assert.Same(animation, session.PlayerProcesses.Attach(animation, session.PlayerProcessClock.FireAnimationType(), parent, 0x10)); uint animationPid = session.Kernel.Find(animation);
        BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(28), 3); process.Fields.Bytes[8] = 0x80;
        byte[] packet = GunPacket(16000, uint.MaxValue); packet[8] = 1; int diagnostics = 0;
        surface.Storage.BindDisplayDiagnostics(new(), (text, file, line) =>
        {
            Assert.Equal("(*(char *)(( (char *)zacket+8) ) & 1) == 0", text); Assert.Equal(@"c:\ns\o\_p_gunprocessdata.cpp", file); Assert.Equal(0x56u, line); diagnostics++;
            packet[8] = 8; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 16001); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), 4);
        });
        session.ReceiveFixedGameplayPacket(0x46a0c0, packet); Assert.Equal(1, diagnostics); Assert.Same(process, session.PlayerProcesses.FromForm(16000)); Assert.Equal(pid, session.Kernel.Find(process)); Assert.Equal(parent, process.Parent());
        Assert.Equal(4u, process.Fields.Stage()); Assert.Equal(0x80, process.Fields.Dirty()); Assert.Null(session.Kernel.Get(animationPid)); Assert.True((session.Pool.Slot(animation.Form())[11] & 3) != 0); session.ReleasePlayerProcesses();
    }

    /// <summary>선택된 헤더/부모 DWORD 범위를 확인하고 없는 현재 PID를 새 객체로 대체하지 않는다.</summary>
    [Fact]
    public void NativeGunBoundsMissingPidAndUnconnectedSearchRemainFailures()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(128, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new()); int count = session.Kernel.Size();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, new byte[8]));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(0, 0)));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(0x10003e80, (uint)parent)));
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(16000, 0x10003a98)));
        byte[] missing = GunPacket((uint)parent, 0); missing[8] = 0; Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x46a0c0, missing)); Assert.Equal(count, session.Kernel.Size()); Assert.Null(session.PlayerProcesses.FromForm(16000));
        session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(16000, (uint)parent, group: types[128].Group)); GunProcess process = Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16000));
        session.Gameplay.Authority.Boss = false; BinaryPrimitives.WriteUInt32LittleEndian(process.Fields.Bytes.AsSpan(28), 1); session.ProcessTime.Value = 20;
        Assert.Equal("고정수신의실제선택본문/효과미연결", Assert.Throws<InvalidOperationException>(() => session.RunMissionFrame()).Message); Assert.Equal(1u, process.Fields.Stage()); session.ReleasePlayerProcesses();
    }

    /// <summary>실제 양방향 감시 생성/제거와 같은 Gun 이미지의 재전송, 외부 함수 값 사본을 확인한다.</summary>
    [Fact]
    public void NativeGunTargetUsesActualDependencyPairsRefreshAndCopiedEffects()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(128, 0), target = session.BuildPlayerWorldHooks().Create!(128, 0);
        var events = new List<string>(); var effects = new GunProcessHooks { RefreshRange = sid => { Assert.Equal(parent, sid); events.Add("range"); } };
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { Gun = effects, FormPacket = packet => { Assert.Equal(0x3a, packet.Span[1]); events.Add("packet"); } });
        effects.RefreshRange = _ => throw new IOException("복사 뒤 외부 함수 교체"); session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(16000, (uint)parent)); GunProcess process = Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16000));
        session.GunBody.SetTarget(process.Fields, (ushort)target); Assert.True(session.WorldDependencies.Has(target, process.Form())); Assert.Equal((ushort)target, process.Fields.Target()); Assert.Equal(2u, process.Fields.Stage()); Assert.Equal(new[] { "range", "packet" }, events);
        session.GunBody.ClearTarget(process.Fields, true); Assert.False(session.WorldDependencies.Has(target, process.Form())); Assert.Equal(0, process.Fields.Target()); Assert.Equal(1u, process.Fields.Stage()); Assert.Equal(new[] { "range", "packet", "packet" }, events); session.ReleasePlayerProcesses();
    }

    /// <summary>고정F는 현재 표/packet종류와 무관하고 실제 Runtime pin은 진단 실패/종료 요청 뒤 반납된다.</summary>
    [Fact]
    public void NativeGunFixedReceiverPinsActualRuntimeAcrossFailureAndTableReplacement()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(128, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        using var runtime = HostMissionRuntime.Create(session, surface.Storage); RawPacketDispatch dispatch = GunDispatch(); NativeGameplayPacketReceivers.InstallGun(session.Gameplay, dispatch); CanonicalPacketReceiver receiver = dispatch.SnapshotReceiver(0x3a)!;
        dispatch.RegisterCanonicalDirect(0x3a, 7, (_, _, _) => throw new ArithmeticException("교체된 다른 본문")); byte[] packet = GunPacket(16000, (uint)parent); packet[1] = 0x2c;
        receiver.Invoke(99, null, packet); Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16000)); Assert.False(session.Gameplay.RuntimeCallActive(session));
        byte[] failurePacket = GunPacket(16001, (uint)parent); failurePacket[8] = 0; var failure = new IOException("명시 원문 진단 중단");
        surface.Storage.BindDisplayDiagnostics(new(), (_, _, _) => { Assert.True(session.Gameplay.RuntimeCallActive(session)); runtime.Dispose(); Assert.False(runtime.Released); throw failure; });
        Assert.Same(failure, Assert.Throws<IOException>(() => receiver.Invoke(77, null, failurePacket))); Assert.True(runtime.Released); Assert.False(session.Gameplay.RuntimeCallActive(session)); Assert.False(session.Gameplay.Borrowed()); session.ReleasePlayerProcesses();
    }

    /// <summary>3ah만 설치하고 다른 전역 칸/Player 준비를 건드리지 않으며 metadata 불일치를 고치지 않는다.</summary>
    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)] [InlineData(4)] [InlineData(5)]
    public void NativeGunRegistrationPreservesOtherRowsAndRejectsEveryMismatch(int mutation)
    {
        var game = new ClientGameplayGlobals(); RawPacketDispatch dispatch = GunDispatch(); dispatch.Version(0x32, 9, 7); NativeGameplayPacketReceivers.InstallGun(game, dispatch);
        PacketRegistration row = dispatch.Registration(0x3a); Assert.Equal(0x46a0c0u, row.CanonicalReceive!.NativeEntry); Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(0xffff, row.Length); Assert.Null(row.Observer); Assert.Null(row.PacketObserver);
        using (dispatch.Registrations.BindSession(game)) NativeGameplayPacketReceivers.BindGun(dispatch);
        Assert.Equal(9, dispatch.Registration(0x32).Version); Assert.Equal(7, dispatch.Registration(0x32).Mismatch); Assert.False(game.PlayerRecords.Initialized);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(0x3a, 7, (_, _, _) => { }); break;
            case 1: row.Version = 2; break;
            case 2: row.Mismatch = 1; break;
            case 3: row.Length = 9; break;
            case 4: row.Observer = _ => { }; break;
            case 5: row.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = row.CanonicalReceive; Assert.Throws<InvalidOperationException>(() => NativeGameplayPacketReceivers.BindGun(dispatch)); Assert.Same(before, row.CanonicalReceive);
    }
}
