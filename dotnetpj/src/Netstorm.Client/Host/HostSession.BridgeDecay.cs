using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Client.Mission;

// 현재 세션의 실제 지형·표시·삭제·이동에 다리 붕괴를 결합한다.
namespace Netstorm.Client.Host;

/// <summary>별도 World 사본 없이 같은 물리 저장소의 칸 처리와 대피를 수행한다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 풀의 다리 수명과 프레임을 직접 변경하는 몸체다.</summary>
    private RawBridgeDecay? _bridgeDecay;
    /// <summary>현재 이동 부모와 같은 그래프·Form·경로를 사용하는 대피 몸체다.</summary>
    private RawBridgeCarriers? _bridgeCarriers;
    /// <summary>이른 칸 처리 전에 같은 입자·음향 프레임만 준비한다.</summary>
    private Action<FrameTime>? _bridgeDecayTime;

    /// <summary>이동 부모 뒤에 같은 지형·현재 권한과 실제 위치 소리/시각을 한 번 연결한다.</summary>
    public void BuildBridgeDecayDevices(RawSurfaceStorage storage, Action<int> sound, Action<FrameTime> time)
    {
        ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(sound); ArgumentNullException.ThrowIfNull(time);
        EnsureRuntimeCanRelease();
        if (_bridgeDecay is not null || _bridgeCarriers is not null) throw new InvalidOperationException("다리 붕괴 세션 중복 연결");
        if (!ReferenceEquals(_surfaceStorage, storage) || !ReferenceEquals(Pool, storage.Pool)
            || !ReferenceEquals(Gameplay.Authority, storage.Authority()))
            throw new ArgumentException("다리 붕괴의 같은 물리·권한 소유자 필요", nameof(storage));
        RawPlayerPlacementAnchor anchor = FixedPacketNeed(_motionAnchor);
        RawSquidHash hash = FixedPacketNeed(_terrainHash);
        RawPriestWorldBindings priests = PriestWorld;
        var lookup = new RawContainedLookup(Pool, (sid, flags) => { _ = DestroyForState(sid, flags); });
        var carriers = new RawBridgeCarriers(Pool, hash, storage.Types, () => Gameplay.Authority.Boss,
            storage.Checking(), () => priests.Movement.Priest, () => priests.Movement.RainWalker,
            () => Gameplay.BridgeAvoidLife.Value, new()
            {
                GraphFor = sid => anchor.GraphFor(unchecked((uint)sid)),
                MovementType = priests.MovementProcessType,
                MovementForm = (sid, kind) => lookup.Find(sid, kind, 0, false),
                Start = (sid, point) => PathStarter.Start(new(sid, point, 0, 0, 0, 0, 0)) is not null,
                Valid = point => AiPriestPoints.Valid(point, Gameplay.WorldCells.Value),
                WontWalkOffEdge = type => type == priests.Movement.Priest || type == priests.Movement.WindWalker
                    || type == priests.Movement.RainWalker || type == priests.Movement.Bulf || type == priests.Movement.Empty,
                Assertion = line => throw new InvalidOperationException($"원본 다리/Carrier 대피 진단 {line}"),
            });
        var decay = new RawBridgeDecay(Pool, storage.Types, WorldFrames, hash.BorrowEntries(0), storage.Spots,
            storage.Graph.Records, () => new BridgeDecayMode
            {
                Server = Gameplay.Authority.Server, Authority = Gameplay.Authority.Boss,
                Editor = Gameplay.World.Editor != 0, DebugRedraw = Gameplay.BridgeLifeNumbers.Value != 0,
            }, new()
            {
                Destroy = (sid, flags) => { _ = DestroyForState(sid, flags); },
                NotifySurface = sid => FixedPacketNeed(_fortBuilding).NotifySurface(sid),
                Refresh = sid => FixedPacketNeed(_terrainDisplay).Update(Pool.Slot(sid)),
                Carriers = carriers.Check, CrackSound = sound,
                Assertion = storage.AssertDisplay,
            }, storage.Authority(), storage.Checking());
        _bridgeCarriers = carriers; _bridgeDecay = decay; _bridgeDecayTime = time;
    }

    /// <summary>Client 스캔이 선택한 한 칸에만 현재 프레임을 공급하고 실제 붕괴를 수행한다.</summary>
    public void DecayBridgeCell(FrameTime time, int sid)
    {
        RawBridgeDecay decay = FixedPacketNeed(_bridgeDecay);
        FixedPacketNeed(_bridgeDecayTime)(time);
        decay.Cell(sid);
    }

    /// <summary>같은 세션의 프로세스 장치 반납과 함께 비소유 붕괴 참조를 해제한다.</summary>
    private void ReleaseBridgeDecayDevices() { _bridgeDecay = null; _bridgeCarriers = null; _bridgeDecayTime = null; _motionAnchor = null; }
}
