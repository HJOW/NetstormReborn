using System.Buffers.Binary;
using Netstorm.Core.Original;

// 생산 패널 유지가 같은 미션의 실제 Player·내용물·난수·SP를 사용한다.
namespace Netstorm.Client.Host;

/// <summary>현재 세션의 생산 유지 장치를 조립한다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 플레이어 기록의 지정 DWORD를 실제 물리 범위 안에서 읽는다.</summary>
    /// <param name="owner">조회할 플레이어 번호다.</param>
    /// <param name="offset">180바이트 기록 안의 DWORD 오프셋이다.</param>
    /// <returns>현재 플레이어 기록 값이다.</returns>
    private uint ProductionPlayerField(uint owner, int offset)
    {
        if (owner >= 9 || offset < 0 || offset > 176) throw new ArgumentOutOfRangeException(nameof(owner), "생산 유지 Player 기록 범위 오류");
        return BinaryPrimitives.ReadUInt32LittleEndian(PlayerRecords.AsSpan(checked((int)owner * 180 + offset), 4));
    }

    /// <summary>같은 현재 세션에 벽시계와 패널 갱신을 빌려 생산 유지 본체를 만든다.</summary>
    /// <param name="globals">패널보다 오래 유지하는 생산 전역이다.</param>
    /// <param name="panel">같은 패널의 공급 및 이전 상태다.</param>
    /// <param name="wall">현재 벽시계 초 조회다.</param>
    /// <param name="refresh">현재 텔레포트·다리 격자 갱신이다.</param>
    /// <returns>동일 풀과 내용물 생성·삭제를 사용하는 유지 처리기다.</returns>
    /// <remarks>cpppj RawCampaignSession::MakeProductionMaintenance와 같이 매 호출에 현재 값을 읽는다.</remarks>
    public RawProductionMaintenance MakeProductionMaintenance(ProductionMaintenanceGlobals globals, ProductionMaintenancePanel panel,
        Func<double> wall, Action<uint> refresh)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(panel);
        ArgumentNullException.ThrowIfNull(wall);
        ArgumentNullException.ThrowIfNull(refresh);
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("생산 유지 현재 TYPE 미연결");
        var reads = new ProductionMaintenanceReads
        {
            Active = () => _nativeLoaded && (OwnerMode.Fort || OwnerMode.Battle) && !OwnerMode.Challenge,
            Technology = () => PlayerWorld.CurrentTechnology(),
            TemplateTechnology = () => PlayerWorld.Technology(unchecked((int)MissionPlayer.TemplateOwner)),
            Temple = () => ProductionPlayerField(MissionPlayer.TemplateOwner, 0x7c),
            Editor = () => Gameplay.World.Editor,
            GeneratorTimer = () => GameAux.MissionSettings[0],
            StockRate = () => BattleOptions[2],
            Artificial = parent =>
            {
                uint owner = Pool.Slot(checked((int)parent))[34];
                if (owner >= 9) throw new ArgumentOutOfRangeException(nameof(parent), "생산 유지 현재 AI 소유자 범위 오류");
                return owner != 0 && ProductionPlayerField(owner, 0xb0) != 0;
            },
        };
        var hooks = new ProductionMaintenanceHooks
        {
            WallSeconds = wall,
            GameSeconds = () => ProcessTime.Value,
            Money = () => MissionPlayer.PlayerKnown != 0 ? BitConverter.UInt32BitsToSingle(Sp.Get(MissionPlayer.BattleOwner)) : 0f,
            TouchDeck = () => WorldProductionDeck.Get(MissionPlayer.TemplateOwner),
            Pattern = () => unchecked((uint)CanonDecoder.SelectBridgePattern(unchecked((int)Game.Next(10000)))),
            Create = (parent, type, argument, flags, attach, quality) => WorldContentCreate.Create(checked((int)parent), type, argument, flags, attach, quality),
            Destroy = sid => DestroyWorldObject(checked((int)sid), 0),
            Allowed = type => WorldProduction.Allowed(checked((int)type)),
            Cost = type => Reward.TypeCostUnits(type),
            Refresh = refresh,
        };
        return new RawProductionMaintenance(Pool, types.Count, SessionContainedFinder, globals, panel, reads, hooks);
    }
}
