using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 공장/그림·건설·감시 쌍·정기·AI와 외부소리를 업그레이드에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp ConnectFactoryUpgrade의 실제 세션 장치다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 같은 풀·Host와 연결한 업그레이드 본체다.</summary>
    private RawFactoryUpgrade? _worldFactoryUpgrade;
    /// <summary>시설 입력/정기/AI가 같은 본체의 현재 상태를 조회한다.</summary>
    public RawFactoryUpgrade WorldFactoryUpgrade => _worldFactoryUpgrade ?? throw new InvalidOperationException("호스트 시설 업그레이드 미연결");

    /// <summary>외부음향/로그만입력받고기존실제장치를복제하지않고연결한다.</summary>
    public RawFactoryUpgrade BuildFactoryUpgradeWorldDevices(FactoryUpgradeHooks devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_worldFactoryUpgrade is not null) throw new InvalidOperationException("호스트 시설 업그레이드 중복 연결");
        if (devices.SoundAt is null || devices.Sound is null || devices.Log is null)
            throw new ArgumentException("시설 업그레이드 실제 소리/로그 장치 누락", nameof(devices));
        _ = WorldConstructionProgress; _ = WorldFlare;
        PlayerWorldHooks world = BindLivePlayerWorld();
        RawDependFormHost dependencies = _worldDependencies ?? BuildDependencyWorldDevices();
        var state = new FactoryUpgradeState
        {
            Outpost = () => Gameplay.OutpostTypeBox.Value, DependType = () => GameAux.DependFormTypeBox.Value,
            LocalPlayer = () => MissionPlayer.BattleOwner, Challenge = () => OwnerMode.Challenge,
            Boss = () => Gameplay.Authority.Boss, Checking = new(() => AiChecking.Read()),
        };
        var hooks = new FactoryUpgradeHooks
        {
            Level = parent =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(parent); uint table = BinaryPrimitives.ReadUInt32LittleEndian(raw);
                if (table is not (0x509148 or 0x509210)) throw new InvalidOperationException("업그레이드 현재Factory level 가상표 미복원");
                return (uint)(raw[11] >> 5);
            },
            SetLevel = SetFortObjectLevel, MaxHp = Reward.MaxHitPoints,
            AdjustFrame = sid => FactoryPostPop.AdjustFrame(sid), Display = sid => _terrainDisplay!.Update(Pool.Slot(sid)),
            SoundAt = devices.SoundAt, Sound = devices.Sound, Log = devices.Log,
            Flare = (sid, count, ev) => WorldFlare.Spawn(sid, count, ev),
            Create = world.Create, Owner = SetWorldObjectOwner, Pop = PopWorldObject,
            Construction = (sid, duration) => WorldConstructionProgress.Start(sid, duration),
            Depend = (watched, watcher) => { _ = dependencies.Create(watched, watcher); }, Watcher = dependencies.Watcher,
            Destroy = BuildSessionDestroy("업그레이드 그림", PlayerProcesses),
            AiPresent = parent =>
            {
                uint owner = Pool.Slot(parent)[34]; if (owner == 0) return false;
                if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(parent), "업그레이드현재AI Player 범위");
                return BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 0xb4 + 0xb0)) != 0;
            },
            NotifyAi = (owner, parent) =>
            {
                if (OwnerMode.Challenge || unchecked((int)owner) <= 0) return;
                if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "업그레이드완료AI Player 범위");
                uint brain = BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan((int)owner * 0xb4 + 0xb0));
                if (brain != 0) AiFactories.Upgraded(brain, parent);
            },
            Assertion = line => throw new InvalidOperationException($"Factory 업그레이드 원본 진단 줄:0x{line:X}"),
        };
        var upgrade = new RawFactoryUpgrade(Pool, PlayerProcesses, state, hooks); _worldFactoryUpgrade = upgrade; return upgrade;
    }
}
