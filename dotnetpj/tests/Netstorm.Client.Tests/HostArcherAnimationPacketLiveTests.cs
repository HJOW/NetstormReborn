using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Client;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using Netstorm.Net;

// 같은 실제 전투 공장을 빌리는 3b/3c 고정 수신과 전송 체인을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>전투 공장 공급·고정 항목 배송·F 등록·전체 26항목 분배를 확인한다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>같은 세션 풀/저장소를 빌리는 실제 전투 공장을 조립한다. 전송 장치가 이미 자산표를 올렸으면 재사용한다.</summary>
    private static RawCampaignArcherBindings ArcherCombat(HostSession session, RawSurfaceStorage storage, IReadOnlyList<OriginalTypeRecord> types, GameRandom random)
    {
        if (storage.Types.Count == 0)
        {
            string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
            var startup = new HostStartupScene(data);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
            List<SquidDisplayShape> shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(startup.Files.ReadAllBytes("d/_shapes.shp")));
            storage.InitializeAssetTables(types, session.WorldFrames, shapes);
        }
        IReadOnlyList<SquidDisplayShape> tableShapes = storage.Shapes;
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 97, storage, session.WorldFrames);
        var target = new RawGunTarget(storage.Pool, storage.Hash, storage.Types,
            Enumerable.Range(0, storage.Types.Count).Select(_ => new GunTargetType()).ToArray(), tableShapes, new GunTargetState());
        ArcherTypeData[] directions = Enumerable.Range(0, storage.Types.Count).Select(_ => new ArcherTypeData(20, 80)).ToArray();
        PriestPlainCanonType[] frames = Enumerable.Range(0, storage.Types.Count).Select(_ => new PriestPlainCanonType()).ToArray();
        MissileTypeData[] missiles = Enumerable.Range(0, storage.Types.Count).Select(_ => new MissileTypeData()).ToArray();
        return new(world, target, random, directions, frames, missiles, new ArcherAnimationState(), new MissileState(), new());
    }
    /// <summary>원문9BYTE 머리·Form·진단 뒤 부모DWORD와 전체 선택 필드를 붙이는 실제 수신 입력이다.</summary>
    private static byte[] ArcherPacket(int form, uint message, int parent)
    {
        var packet = new byte[25]; packet[0] = 1; packet[1] = (byte)message;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), 25); BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), (uint)form);
        packet[8] = 0xff; BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(9), (uint)parent);
        BinaryPrimitives.WriteDoubleLittleEndian(packet.AsSpan(17), 2.5); return packet;
    }
    /// <summary>두 고정 항목이 같은 실제 전투 공장에 파생을 붙이고 현재 타입/부모를 기록한다.</summary>
    [Theory]
    [InlineData(0x466d30, 128, 0x3b, nameof(CannonFire), 35)]
    [InlineData(0x466b60, 128, 0x3c, nameof(GunAnimation), 36)]
    public void ArcherAnimationFixedEntriesRouteToSuppliedCombat(uint entry, byte type, uint message, string expected, byte processType)
    {
        var sent = new List<byte[]>(); var (session, types, surface) = SquidPacketScene(new() { Broadcast = packet => sent.Add(packet.ToArray()), Connected = _ => false, Send = (_, _) => { } });
        int parent = session.BuildPlayerWorldHooks().Create!(type, 0); session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        RawCampaignArcherBindings combat = ArcherCombat(session, surface.Storage, types, session.Game); session.BindArcherAnimationCombat(combat);
        Assert.Same(combat, session.ArcherAnimationCombat);
        session.ReceiveFixedGameplayPacket(entry, ArcherPacket(16000, message, parent));
        SquidProcess animation = session.PlayerProcesses.FromForm(16000)!;
        Assert.Equal(expected, animation.GetType().Name); Assert.Equal(parent, animation.Parent()); Assert.Equal(processType, session.Pool.Slot(16000)[10]);
        session.ReleasePlayerProcesses();
    }
    /// <summary>공급 없는 수신·빈 공급·다른 풀 공급을 거부하고 반납 뒤 고정 항목을 닫는다.</summary>
    [Fact]
    public void ArcherAnimationRequiresSuppliedCombatWithSamePool()
    {
        var (session, types, surface) = SquidPacketScene(); int parent = session.BuildPlayerWorldHooks().Create!(128, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        Assert.Throws<ArgumentNullException>(() => session.BindArcherAnimationCombat(null!));
        string? foreignData = GameDataLocator.FindDataDirectory(); Assert.NotNull(foreignData);
        var foreignStartup = new HostStartupScene(foreignData);
        IReadOnlyList<RiftTypeDefinition> foreignDefinitions = MissionFortLoad.LoadTypeDefinitions(foreignStartup.Files);
        List<SquidDisplayShape> foreignShapes = TerrainDisplayShapes.Build(foreignDefinitions, new ShapeDatabase(foreignStartup.Files.ReadAllBytes("d/_shapes.shp")));
        var foreign = new RawSurfaceStorage(OriginalEdition.Patch1078, types, session.WorldFrames, foreignShapes);
        var foreignWorld = new RawSurfaceWorld(OriginalEdition.Patch1078, foreign.Types, 97, foreign, session.WorldFrames);
        var stranger = new RawCampaignArcherBindings(foreignWorld,
            new(foreign.Pool, foreign.Hash, foreign.Types, Enumerable.Range(0, foreign.Types.Count).Select(_ => new GunTargetType()).ToArray(), foreignShapes, new GunTargetState()),
            new GameRandom(7), Enumerable.Range(0, foreign.Types.Count).Select(_ => new ArcherTypeData(20, 80)).ToArray(),
            Enumerable.Range(0, foreign.Types.Count).Select(_ => new PriestPlainCanonType()).ToArray(),
            Enumerable.Range(0, foreign.Types.Count).Select(_ => new MissileTypeData()).ToArray(), new ArcherAnimationState(), new MissileState(), new());
        Assert.Throws<ArgumentException>(() => session.BindArcherAnimationCombat(stranger));
        foreignWorld.Dispose();
        var missing = Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x466d30, ArcherPacket(16000, 0x3b, parent)));
        Assert.Contains("실제 전투 본체 미연결", missing.Message);
        var missingRotate = Assert.Throws<InvalidOperationException>(() => session.ReceiveFixedGameplayPacket(0x466b60, ArcherPacket(16001, 0x3c, parent)));
        Assert.Contains("실제 전투 본체 미연결", missingRotate.Message);
        RawCampaignArcherBindings combat = ArcherCombat(session, surface.Storage, types, session.Game); session.BindArcherAnimationCombat(combat);
        session.BindArcherAnimationCombat(combat); Assert.Same(combat, session.ArcherAnimationCombat);
        session.ReleasePlayerProcesses();
        Assert.Throws<ObjectDisposedException>(() => session.ReceiveFixedGameplayPacket(0x466d30, ArcherPacket(16000, 0x3b, parent)));
        Assert.Throws<InvalidOperationException>(() => session.ArcherAnimationCombat);
    }
    /// <summary>두F만 설치하고 다른 전역 칸/Player 준비를 건드리지 않으며 metadata 불일치를 고치지 않는다.</summary>
    /// <param name="mutation">검사할 변이 번호다.</param><param name="type">검사할 메시지 번호다.</param>
    [Theory]
    [InlineData(0, 0x3b)] [InlineData(1, 0x3b)] [InlineData(2, 0x3b)] [InlineData(3, 0x3b)] [InlineData(4, 0x3b)] [InlineData(5, 0x3b)]
    [InlineData(0, 0x3c)] [InlineData(1, 0x3c)] [InlineData(2, 0x3c)] [InlineData(3, 0x3c)] [InlineData(4, 0x3c)] [InlineData(5, 0x3c)]
    public void ArcherAnimationRegistrationPreservesOtherRowsAndRejectsEveryMismatch(int mutation, uint type)
    {
        var game = new ClientGameplayGlobals(); RawPacketDispatch dispatch = GunDispatch(); dispatch.Version(0x32, 9, 7);
        NativeGameplayPacketReceivers.InstallArcherAnimation(game, dispatch);
        foreach (uint installed in new uint[] { 0x3b, 0x3c })
        {
            PacketRegistration row = dispatch.Registration(installed);
            Assert.Equal(installed == 0x3b ? 0x00466d30u : 0x00466b60u, row.CanonicalReceive!.NativeEntry);
            Assert.Equal(1, row.Version); Assert.Equal(0, row.Mismatch); Assert.Equal(0xffff, row.Length); Assert.Null(row.Observer); Assert.Null(row.PacketObserver);
        }
        using (dispatch.Registrations.BindSession(game)) NativeGameplayPacketReceivers.BindArcherAnimation(dispatch, type);
        Assert.Equal(9, dispatch.Registration(0x32).Version); Assert.Equal(7, dispatch.Registration(0x32).Mismatch); Assert.False(game.PlayerRecords.Initialized);
        PacketRegistration selected = dispatch.Registration(type);
        switch (mutation)
        {
            case 0: dispatch.RegisterCanonicalDirect(type, 7, (_, _, _) => { }); break;
            case 1: selected.Version = 2; break;
            case 2: selected.Mismatch = 1; break;
            case 3: selected.Length = 9; break;
            case 4: selected.Observer = _ => { }; break;
            case 5: selected.PacketObserver = (_, _) => { }; break;
        }
        CanonicalPacketReceiver? before = selected.CanonicalReceive;
        Assert.Throws<InvalidOperationException>(() => NativeGameplayPacketReceivers.BindArcherAnimation(dispatch, type)); Assert.Same(before, selected.CanonicalReceive);
        Assert.Throws<ArgumentException>(() => NativeGameplayPacketReceivers.BindArcherAnimation(dispatch, 0x3d));
    }
    /// <summary>설치한 두 수신기가 같은 고정 항목을 들고 세션 대여에 붙는다. 배송 호출은 고정 항목 직접 수신으로 검증한다.</summary>
    [Fact]
    public void ArcherAnimationInstalledReceiversCarryFixedEntries()
    {
        var (session, types, surface) = SquidPacketScene(); session.BuildPlayerWorldHooks().Create!(128, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        RawCampaignArcherBindings combat = ArcherCombat(session, surface.Storage, types, session.Game); session.BindArcherAnimationCombat(combat);
        RawPacketDispatch dispatch = GunDispatch(); NativeGameplayPacketReceivers.InstallArcherAnimation(session.Gameplay, dispatch);
        Assert.Equal(0x00466d30u, dispatch.SnapshotReceiver(0x3b)!.NativeEntry);
        Assert.Equal(0x00466b60u, dispatch.SnapshotReceiver(0x3c)!.NativeEntry);
        using (dispatch.Registrations.BindSession(session.Gameplay)) NativeGameplayPacketReceivers.BindArcherAnimation(dispatch, 0x3b);
        using (dispatch.Registrations.BindSession(session.Gameplay)) NativeGameplayPacketReceivers.BindArcherAnimation(dispatch, 0x3c);
        session.ReleasePlayerProcesses();
    }
    /// <summary>전투 공장 writer가 바깥 체인을 차지하고 실제 Gun 본체는 기존 fallback을 유지한다.</summary>
    [Fact]
    public void ArcherAnimationTransmissionChainsCombatWithGunFallback()
    {
        var sent = new List<byte[]>(); var (session, types, surface) = SquidPacketScene();
        int parent = session.BuildPlayerWorldHooks().Create!(128, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new() { FormPacket = packet => sent.Add(packet.ToArray()) });
        RawCampaignArcherBindings combat = ArcherCombat(session, surface.Storage, types, session.Game); session.BindArcherAnimationCombat(combat);
        session.ReceiveFixedGameplayPacket(0x466d30, ArcherPacket(16000, 0x3b, parent));
        session.ReceiveFixedGameplayPacket(0x46a0c0, GunPacket(16001, (uint)parent));
        Assert.IsType<CannonFire>(session.PlayerProcesses.FromForm(16000)); Assert.IsType<GunProcess>(session.PlayerProcesses.FromForm(16001));
        byte[] output = new byte[64]; int length = session.PlayerProcesses.WriteFormPacket(16000, 1, output, session.GameAux.SquidTransmission);
        Assert.True(length > 9); Assert.Equal(0x3b, output[1]);
        session.PlayerProcesses.Transmission()!.Transmit!(length);
        Assert.Single(sent); Assert.Equal(output.AsSpan(0, length).ToArray(), sent[0]);
        byte[] gunOutput = new byte[64]; int gunLength = session.PlayerProcesses.WriteFormPacket(16001, 1, gunOutput, session.GameAux.SquidTransmission);
        Assert.True(gunLength > 9); Assert.NotEqual(0x3b, gunOutput[1]);
        session.PlayerProcesses.Transmission()!.Transmit!(gunLength);
        Assert.Equal(2, sent.Count); Assert.Equal(gunOutput.AsSpan(0, gunLength).ToArray(), sent[1]);
        session.ReleasePlayerProcesses();
    }
    /// <summary>26개 고정 항목이 모두 실제 본체에 분배되고 미연결 기본 분기로 떨어지지 않는다.</summary>
    [Fact]
    public void AllTwentySixFixedEntriesDispatchToBodies()
    {
        var (session, types, surface) = SquidPacketScene(); session.BuildPlayerWorldHooks().Create!(128, 0);
        session.BindSquidPacketDevices(surface.Storage, SquidPacketNames(session, types), new());
        session.BindArcherAnimationCombat(ArcherCombat(session, surface.Storage, types, session.Game));
        uint[] entries = [0x4a71c0, 0x4333f0, 0x46e940, 0x489ee0, 0x44a280, 0x433c30, 0x46a0c0, 0x45e2b0, 0x497030, 0x426490,
            0x4436b0, 0x433120, 0x434080, 0x466d30, 0x466b60, 0x45e410, 0x45e5d0, 0x45e4f0, 0x432710, 0x4326e0,
            0x4327b0, 0x433170, 0x432850, 0x44dba0, 0x4cf980, 0x4266c0];
        Assert.Equal(26, entries.Length);
        var unconnected = new List<uint>();
        foreach (uint entry in entries)
        {
            try { session.ReceiveFixedGameplayPacket(entry, ReadOnlyMemory<byte>.Empty); }
            // 각 본체의 길이/범위 실패는 분배 성공이며 기본 분기의 실제 문구만 모은다.
            catch (Exception failure) when (failure.Message.Contains("나머지 고정 gameplay 실제 본문 미연결")) { unconnected.Add(entry); }
            catch { }
        }
        Assert.Empty(unconnected);
        session.ReleasePlayerProcesses();
    }
}
