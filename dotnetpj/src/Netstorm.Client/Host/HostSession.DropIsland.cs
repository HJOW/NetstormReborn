using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

// 현재16.16 패킷을 같은 실제 건설 효과/파편/낙하/전체 삭제에 전달한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignSession.cpp의46e940과 ConnectBuilding 섬 정리 결선이다.</summary>
public sealed partial class HostSession
{
    /// <summary>Displace 전용 본체와 구별하는 실제 섬 정리 효과다.</summary>
    private RawConstructionEffects? _dropIslandEffects;
    /// <summary>같은 장기 전역과 현재 준비된 동맹 표를 읽는다.</summary>
    private ConstructionEffectsState? _dropIslandState;

    /// <summary>같은 실제 파편/HP 진행/공간/사제와 전체 가상 삭제를 설치한다. 방송은 원격 전용 실제 분배여야 한다.</summary>
    public RawConstructionEffects BuildDropIslandWorldDevices(Action<ReadOnlyMemory<byte>> remoteBroadcast,
        Action<float, float, string, uint> playAt, Action<string, string, uint> assertion)
    {
        ArgumentNullException.ThrowIfNull(remoteBroadcast); ArgumentNullException.ThrowIfNull(playAt); ArgumentNullException.ThrowIfNull(assertion);
        if (_dropIslandEffects is not null) throw new InvalidOperationException("호스트 섬 정리 중복 설치");
        _ = WorldShrapnel; _ = WorldConstructionProgress; _ = PriestWorld;
        var lookup = new RawContainedLookup(Pool, DestroyWorldObject);
        var state = new ConstructionEffectsState
        {
            SharedBlocked = () => GameAux.IslandDropBlockedBox.Value, SharedStumpType = () => GameAux.IslandStumpTypeBox.Value,
            SharedIslandType = () => BridgeState.IslandType, SharedNoIslandType = () => BridgeState.NoIslandType,
            SharedStalagType = () => BridgeState.StalagType, SharedServer = () => Gameplay.Authority.Server,
        };
        var effects = new RawConstructionEffects(Pool, FixedPacketNeed(_terrainHash), FixedPacketNeed(_terrainPopTypes), FixedPacketNeed(_terrainSpots), state, new()
        {
            PathType = type => PriestWorld.MovementProcessType(type), ClearPath = (sid, type) => { _ = lookup.Find(sid, type, 0, true); },
            Unpop = UnpopWorldObject, Pop = PopWorldObject, Fall = sid => { _ = AddWorldShrapnel(sid, ShrapnelKind.Variant2); },
            Sound = (x, y) => playAt(x, y, "islandFall.wav", 0), Destroy = DestroyWorldObject, Notify = sid => PriestWorld.FallWalker(sid),
            Broadcast = (x, y, flags, owner) => { byte[] packet = new byte[20]; BattlePacketWriter.WriteDropIslandPacket(packet, x, y, flags, owner); remoteBroadcast(packet); },
            Assertion = (expression, line) => assertion(expression, @"\Ns\O\Islanddropper.cpp", unchecked((uint)line)),
        });
        _dropIslandState = state; _dropIslandEffects = effects; return effects;
    }
    /// <summary>원문 순서로 좌표/owner/flags를 읽고 현재 건설 시각/소유자/동맹을 준비한 뒤 실제 정리를 실행한다.</summary>
    private void ReceiveDropIslandPacket(ReadOnlyMemory<byte> packet)
    {
        int rawX = BinaryPrimitives.ReadInt32LittleEndian(packet.Span.Slice(4, 4));
        uint owner = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span.Slice(16, 4)), flags = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span.Slice(12, 4));
        float x = (float)((double)rawX * (1.0 / 65536.0)), y = (float)((double)BinaryPrimitives.ReadInt32LittleEndian(packet.Span.Slice(8, 4)) * (1.0 / 65536.0));
        RawConstructionEffects effects = _dropIslandEffects ?? throw new InvalidOperationException("섬 정리 수신의 실제 건설 효과 미설치");
        ConstructionProcessState building = WorldConstructionProgressState; building.Now = PlayerProcessClock.Now; building.LocalPlayer = MissionPlayer.BattleOwner;
        _dropIslandState!.Alliances = Alliances.ToArray(); effects.DropIsland(x, y, flags, owner);
    }
    /// <summary>같은 건설/파편 장치보다 먼저 수신 전용 참조를 반납한다.</summary>
    private void ReleaseDropIslandWorld() { _dropIslandEffects = null; _dropIslandState = null; }
}
