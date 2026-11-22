using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client.Audio;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// 실제 Host의 경로 시작·이동·수신·현재 조회와 반납을 같은 월드에서 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>기존 실자산 사제 구성기에 Motion 전체 부모를 연결한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>같은 풀/섬/지형/시계에 실제 Motion을 설치한다. 선택하지 않은 큰 효과는 실패시킨다.</summary>
    private static (HostSession Session, ClientSurfaceGlobals Surface, int Priest, HostMotionState State, List<byte[]> Packets) MotionScene(bool native = false, Action<int, string>? trace = null)
    {
        var packets = new List<byte[]>();
        var (session, types, surface) = SquidPacketScene(native ? new() { Broadcast = buffer => packets.Add(buffer.ToArray()), Connected = _ => false, Send = (_, _) => throw PathParentForbidden() } : null);
        if (trace is not null) surface.Storage.BindGraphTrace(trace);
        session.BuildIslandPostPopDevices(types, session.WorldFrames);
        session.BuildTerrainBinding(types, session.TerrainDiagnostics, new() { SetResidenceOwner = (_, _) => throw PathParentForbidden(), Assertion = _ => throw PathParentForbidden() });
        session.BuildRegionOwnershipDevices(types);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = packet => packets.Add(packet.ToArray()) });
        // Form 자체의 Pre/Post는 실제 Host가 처리한다. 외부 송신만 기록하고 자산 가상삭제는 허용하지 않는다.
        session.PlayerProcesses.Runtime().Asset = new() { Selected = () => 0, Emit = effect => { if (effect.Effect != SquidDestroyEffect.Transmit) throw PathParentForbidden(); packets.Add(BitConverter.GetBytes(effect.Sid)); } };
        TerrainStampHooks stamp = session.BuildTerrainStampHooks(types);
        // 실제 도장으로 시작과 목적지를 포함하는 연속 지면을 만든다. 사제 helper가 만들 칸은 남긴다.
        for (int y = 59; y <= 61; y++)
            // 행마다 실제 다른 지면SID를 Pop한다.
            for (int x = 49; x <= 52; x++) { if (x == 50 && y == 60) continue; int ground = stamp.Create!(97, 1); stamp.Pop!(ground, x, y, 0x02000061); }
        int priest = PlacePriest(session, types, 0, 9);
        var startup = new HostStartupScene(GameDataLocator.FindDataDirectory()!); var definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        var anchor = new RawPlayerPlacementAnchor(session.Pool, types, surface.Storage.Hash.BorrowEntries(0), session.TerrainBookkeeping, new(), new(), new() { GraphReady = 1 });
        var state = new HostMotionState();
        var regionFrames = session.WorldFrames.Select((frames, index) => new PriestPlainCanonType { Frames = frames, DefaultFrame = index < 70 || index - 70 >= definitions.Count ? 0 : definitions[index - 70].SpecialFrames.DefaultFrame }).ToArray();
        session.BuildMotionWorldDevices(session.TerrainBinding!.Islands, anchor, regionFrames, state, new()
        {
            Cleanup = new() { ReserveShared = () => throw PathParentForbidden(), Ritual = (_, _, _) => throw PathParentForbidden() },
            Halt = new() { CarrierPickup = _ => throw PathParentForbidden(), NuggetTransfer = _ => throw PathParentForbidden() },
            Carrier = PathParentEffects(), Priest = new() { NeutralReturn = _ => throw PathParentForbidden() }, VirtualDestroy = (_, _) => throw PathParentForbidden(),
            Path = new()
            {
                Metadata = sid => { RiftTypeDefinition definition = definitions[session.Pool.Slot(sid)[10] - 70]; return new((float)definition.Number("speed")!.Value, (float)definition.Number("turningSpeed")!.Value, definition.SpecialFrames.DefaultFrame); },
                // 음향은 이 검사의 선택 경계다. 소리0 외 실제 재생/겹침/재탐색은 호출되면 실패한다.
                SoundId = _ => 0, StepSound = (_, _, _, _) => throw PathParentForbidden(), StartBlocked = (_, _, _) => throw PathParentForbidden(), ArrivalBlocked = (_, _, _, _) => throw PathParentForbidden(), Retarget = (_, _) => throw PathParentForbidden(),
            },
        });
        packets.Clear(); return (session, surface, priest, state, packets);
    }

    /// <summary>같은 실제 BFS·시계·공간에서 사제 이동과 정상 도착/경로 정리가 실행된다.</summary>
    [Fact]
    public void MotionStartsMovesAndArrivesUsingActualHostClockAndSpace()
    {
        var (session, surface, priest, state, packets) = MotionScene();
        Assert.Same(session.PlayerProcessClock, session.PathStartState.Clock); Assert.Same(session.PlayerProcessClock, session.PathProcessState.Clock);
        Assert.Same(surface.Storage.PathSearch.Work, session.PathSearch.Work); Assert.Same(session.Pool, session.MotionTerrain.Pool);
        session.PlayerProcessClock.Now = 12.5;
        RawPathProcess path = Assert.IsType<RawPathProcess>(session.StartPriestPath(new(priest, new(51, 60), TailWord: 0x12345678, TailByte: 0xaabbccdd)));
        Assert.Same(path.Runtime, session.CurrentPathRuntime(priest)); Assert.Same(path, session.PlayerProcesses.FromForm(path.Form())); Assert.True(path.Runtime.Count > 0); Assert.NotEmpty(packets);
        Assert.True(session.SetCurrentPathTargets(priest, 0x11223344, 0x55667788)); Assert.Equal(0x11223344u, path.Runtime.TargetA); Assert.Equal(0x55667788u, path.Runtime.TargetB);
        session.SetCurrentPathTargets(priest, 0, 0);
        // 게임 시계만 전진하고 같은 Kernel의 실제 경로를 실행한다.
        for (int tick = 0; tick < 600 && session.CurrentPathRuntime(priest) is not null; tick++) { session.PlayerProcessClock.Now += .02; session.Kernel.RunFrame(); }
        Assert.Null(session.CurrentPathRuntime(priest)); Assert.Equal(0, session.Pool.Slot(priest)[11] & 5);
        float x = BinaryPrimitives.ReadSingleLittleEndian(session.Pool.Slot(priest)[14..]); Assert.InRange(x, 50.9f, 51.1f);
        session.Gameplay.Authority.Boss = false; Assert.False(state.Cleanup.CurrentBoss()); session.Gameplay.Authority.Boss = true; Assert.True(state.Cleanup.CurrentBoss());
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size()); Assert.Throws<InvalidOperationException>(() => session.PathStarter); Assert.Throws<InvalidOperationException>(() => session.PlayerProcessClock);
    }

    /// <summary>고정 constructor8종과 mutable 역할을 구별하고 명령의 실제 부모 가상표를 검증한다.</summary>
    [Fact]
    public void MotionConstructorsKeepFixedClassWhenRoleChangesAndRejectWrongParents()
    {
        var (session, _, priest, _, _) = MotionScene();
        uint[] addresses = [0x48be40, 0x48be70, 0x48bea0, 0x48bed0, 0x48bf00, 0x48bf30, 0x48bf60, 0x48bf90]; uint[] types = [23, 42, 39, 38, 41, 24, 54, 40];
        // 고정 생성자는 rawTYPE를 추측하지 않고 대응 클래스만 만든다.
        for (int index = 0; index < addresses.Length; index++) { using RawPathProcess path = Assert.IsType<RawPathProcess>(session.CreatePathByConstructor(addresses[index])); Assert.Equal(types[index], path.MovementType()); Assert.Same(session.Pool, path.Pool); }
        session.PathProcessState.PriestProcess = 999; using (RawPathProcess path = Assert.IsType<RawPathProcess>(session.CreatePathByConstructor(0x48bf60))) Assert.Equal(999u, path.MovementType()); session.PathProcessState.PriestProcess = 54;
        Assert.Null(session.CreatePathByConstructor(0xdeadbeef)); Assert.Throws<InvalidOperationException>(() => session.CreatePathType(999));
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(session.Pool.Slot(priest)); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(priest), 0x501dd0);
        Assert.Throws<ArgumentException>(() => session.StartPriestPath(new(priest, new(51, 60)))); BinaryPrimitives.WriteUInt32LittleEndian(session.Pool.AllocatedBytes(priest), table);
        Assert.Throws<InvalidOperationException>(() => session.StartCarrierPath(new(priest, new(51, 60)))); Assert.Throws<InvalidOperationException>(() => session.StartConstructionPath(new(priest, new(51, 60)))); Assert.Throws<InvalidOperationException>(() => session.StartDudePath(new(priest, new(51, 60))));
        session.ReleasePlayerProcesses();
    }

    /// <summary>같은 Native 전체 공통버퍼와 현재 헤더/로그 수준으로 작성한30h를 실제 다른 Host에 부착한다.</summary>
    [Fact]
    public void NativePathPacketRoundTripUsesSharedBufferCurrentHeaderAndActualReceiver()
    {
        var levels = new List<int>(); var (source, _, priest, _, packets) = MotionScene(true, (level, _) => levels.Add(level));
        source.GameAux.PathPacketHeader.PacketFlags = 0xa5; source.GameAux.PathPacketHeader.PacketLength = 0x123456; source.GameAux.PathTraceLevel = 17;
        RawPathProcess path = Assert.IsType<RawPathProcess>(source.StartPriestPath(new(priest, new(51, 60))));
        byte[] packet = Assert.Single(packets); Assert.Equal(2048, packet.Length); Assert.Equal(0x25, packet[0]); Assert.Equal(0x30, packet[1]); Assert.Equal(0x3456, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(2))); Assert.Contains(17, levels);
        Assert.Equal(path.Form(), BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(4))); Assert.Equal(priest, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(6)));
        var (target, _, targetPriest, _, _) = MotionScene(); Assert.Equal(priest, targetPriest); target.Gameplay.Authority.Boss = false; target.PlayerProcessClock.Now = 25;
        target.ReceiveFixedGameplayPacket(0x489ee0, packet);
        RawPathProcess received = Assert.IsType<RawPathProcess>(target.PlayerProcesses.FromForm(path.Form())); Assert.Same(received.Runtime, target.CurrentPathRuntime(priest)); Assert.Equal(54u, received.MovementType()); Assert.Equal(path.Runtime.Point(0), received.Runtime.Point(0));
        Assert.Equal(0, target.Pool.Slot(priest)[11] & 5); source.ReleasePlayerProcesses(); target.ReleasePlayerProcesses();
    }

    /// <summary>live Form은 덮어쓰지 않으며 boss 진단 복귀 뒤 현재 Form WORD를 다시 읽는다.</summary>
    [Fact]
    public void NativePathLiveFormDiagnosticReadsCurrentWordAfterBossCallback()
    {
        var (session, surface, priest, _, _) = MotionScene(); var detail = new StringBox(); var lines = new List<uint>(); byte[] packet = new byte[6];
        surface.Storage.BindDisplayDiagnostics(detail, (_, _, line) => { lines.Add(line); if (line == 0x224) BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4), (ushort)priest); });
        byte[] before = session.Pool.Slot(priest).ToArray(); session.ReceiveFixedGameplayPacket(0x489ee0, packet);
        Assert.Equal(new uint[] { 0x224, 0x24f }, lines); Assert.Contains("Receiving an invalid update for a path process.", detail.Value); Assert.Equal(before, session.Pool.Slot(priest).ToArray()); Assert.Null(session.CurrentPathRuntime(priest)); session.ReleasePlayerProcesses();
    }

    /// <summary>미완성 payload는 Form을 부착하지 않으며 현재 Runtime pin은 로그 중 반납 요청을 지연시킨다.</summary>
    [Fact]
    public void NativePathReceiveKeepsRuntimePinnedAndRejectsTruncatedPayloadBeforeAttach()
    {
        var (session, surface, priest, _, _) = MotionScene(); session.Gameplay.Authority.Boss = false;
        byte[] packet = new byte[24]; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4), 30000); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6), (ushort)priest); packet[15] = 50; packet[16] = 60;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(13), 1); int before = session.Kernel.Size();
        Assert.Throws<ArgumentOutOfRangeException>(() => session.ReceiveFixedGameplayPacket(0x489ee0, packet)); Assert.Equal(before, session.Kernel.Size()); Assert.Null(session.PlayerProcesses.FromForm(30000)); Assert.Null(session.CurrentPathRuntime(priest));
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(13), 0);
        using var owner = HostMissionRuntime.Create(session, surface.Storage); int traces = 0;
        surface.Storage.BindGraphTrace((_, _) => { traces++; Assert.True(session.Gameplay.RuntimeCallActive(session)); owner.Dispose(); Assert.False(owner.Released); });
        using (session.Gameplay.PinCurrentRuntime(session))
        {
            session.ReceiveFixedGameplayPacket(0x489ee0, packet); Assert.NotNull(session.CurrentPathRuntime(priest)); Assert.False(owner.Released);
        }
        Assert.True(traces > 0); Assert.True(owner.Released);
        // Runtime 대여 반납과 실제 프로세스 정리는 별도 계약이므로 외곽 종료자가 Kernel을 비운다.
        session.ReleasePlayerProcesses(); Assert.Equal(0, session.Kernel.Size()); Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x489ee0, packet));
    }
}
