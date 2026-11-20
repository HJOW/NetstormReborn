using System.Buffers.Binary;
using System.Runtime.Versioning;
using Netstorm.Client.Host;
using Netstorm.Core.Original;
using Netstorm.Net;

// 창 없이 실제 시설 요청·로컬 큐·SP·공유 정기·AI 요청과 반납을 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>제품과 같은 실제 시설 장치에 명시적인 로컬 운반 장치를 설치하는 검사다.</summary>
public sealed partial class HostPriestWorldLiveTests
{
    /// <summary>실제 요청을 큐에 넣고 Poll 수신 뒤 비용·사건3·그림/완료까지 같은 본체로 진행한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void FactoryUpgradeCommandUsesLocalQueueMoneyAndSharedProcesses()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        session.Gameplay.Authority.Boss = true; session.Sp.Set(1, BitConverter.SingleToUInt32Bits(5000));
        using var wire = new LocalMissionTransport(); wire.Open("127.0.0.1", 6799); wire.AssignConnection(-1, 1);
        var broadcasts = new List<byte[]>(); var logs = new List<(uint Kind, string Text)>();
        RawFactoryUpgradeCommand command = session.BuildFactoryUpgradeCommandWorldDevices(new()
        {
            Send = (channel, packet) => Assert.True(wire.Send(unchecked((int)channel), packet.Span)),
            Broadcast = packet => broadcasts.Add(packet.ToArray()), Log = (kind, text) => logs.Add((kind, text)),
        });
        Assert.Same(session.Pool, command.Pool); double requestedAt = session.ProcessTime.Value;
        wire.RegisterDispatch(packet => { command.Receive(packet.ToArray()); return true; });
        command.Request(parent);
        Assert.Equal(1, wire.QueuedFrames()); Assert.Equal(5000f, BitConverter.UInt32BitsToSingle(session.Sp.Get(1)));
        RegularProcess waiting = Assert.IsType<RegularProcess>(session.PlayerProcesses.FindEvent(parent, 4));
        Assert.Equal(20f, waiting.Payload()); Assert.Equal(requestedAt, waiting.Time());
        Assert.Equal(1, wire.Poll()); Assert.Equal(0, wire.QueuedFrames());
        Assert.Equal(5000f - session.Reward.TypeCostUnits(85), BitConverter.UInt32BitsToSingle(session.Sp.Get(1)));
        byte[] original = Assert.Single(broadcasts); Assert.Equal(0, original[6]); Assert.Equal(1, original[7]);
        Assert.Equal(requestedAt + 15, BitConverter.ToDouble(original, 8)); Assert.Equal((4u, "new ftUPGRADE sharedRegPro\n"), Assert.Single(logs));
        RegularProcess upgrading = Assert.IsType<RegularProcess>(session.PlayerProcesses.FindEvent(parent, 3));
        Assert.Equal(15f, upgrading.Payload()); Assert.Equal(requestedAt, upgrading.Time());
        session.Kernel.RunFrame(); Assert.Equal(1u, upgrading.Count()); Assert.NotEqual(0, UpgradePicture(session, parent));
        scene.Visuals.PrepareFrame(requestedAt + 15.1, 2); session.AdvanceMissionFrame(15.1);
        Assert.Equal(1, session.Pool.Slot(parent)[11] >> 5); Assert.False(session.WorldFactoryUpgrade.IsUpgrading(parent));
        session.ReleasePlayerProcesses();
        Assert.Throws<InvalidOperationException>(() => session.WorldFactoryUpgradeCommand);
    }

    /// <summary>실제 AI 업그레이드 대기 목록의 요청이 같은 채널9 원문/사건4를 만들고 원소를 제거한다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void AiFactoryUpgradeUsesInstalledCommandAndCurrentTime()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session; int parent = UpgradeFactory(scene);
        var sent = new List<byte[]>(); session.BuildFactoryUpgradeCommandWorldDevices(new()
        {
            Send = (channel, packet) => { Assert.Equal(9u, channel); sent.Add(packet.ToArray()); },
            Broadcast = _ => throw new InvalidOperationException("요청 검사에서 방송 불필요"), Log = (_, _) => { },
        });
        uint brain = session.AiBrainMemory.Allocate(RawAiBrainConstructor.BytesCount), list = session.AiBrainMemory.Allocate(4);
        try
        {
            byte[] raw = session.AiBrainMemory.Bytes(brain); raw.AsSpan().Clear();
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x294), 1);
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x298), session.TerrainBookkeeping.InvalidGraph ^ 1u);
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x2cc), 1); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x2d8), 5000);
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x318), list); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x31c), 1);
            BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(0x320), 1); BinaryPrimitives.WriteUInt32LittleEndian(session.AiBrainMemory.Bytes(list), (uint)parent);
            session.ProcessTime.Value = 37.25; session.AiFactories.Manage(brain);
            byte[] packet = Assert.Single(sent); Assert.Equal(52.25, BitConverter.ToDouble(packet, 8));
            Assert.Equal(parent, BinaryPrimitives.ReadUInt16LittleEndian(packet.AsSpan(4))); Assert.Equal(1, packet[7]);
            Assert.NotNull(session.PlayerProcesses.FindEvent(parent, 4)); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x320)));
        }
        finally { session.AiBrainMemory.Free(list); session.AiBrainMemory.Free(brain); }
    }

    /// <summary>실제 전송 누락과 중복 설치를 변경 전에 거부하고 설치 전 AI 요청도 명시 오류다.</summary>
    [Fact, SupportedOSPlatform("windows")]
    public void FactoryUpgradeCommandInstallationRequiresRealDevices()
    {
        if (!OperatingSystem.IsWindows()) return;
        using var scene = new BurningContext(); HostSession session = scene.Session;
        Assert.Throws<InvalidOperationException>(() => session.WorldFactoryUpgradeCommand);
        Assert.Throws<ArgumentException>(() => session.BuildFactoryUpgradeCommandWorldDevices(new()));
        var devices = new FactoryUpgradeCommandHooks { Send = (_, _) => { }, Broadcast = _ => { }, Log = (_, _) => { } };
        session.BuildFactoryUpgradeCommandWorldDevices(devices);
        Assert.Throws<InvalidOperationException>(() => session.BuildFactoryUpgradeCommandWorldDevices(devices));
    }
}
