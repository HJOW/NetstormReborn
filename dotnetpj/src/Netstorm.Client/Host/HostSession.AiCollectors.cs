using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// AI 수집자가 실제 생산·비용·배치·사제·음향 장치를 같은 세션에서 사용하도록 연결한다.
public sealed partial class HostSession
{
    /// <summary>같은 본체·풀·TYPE·난수·현재 전역을 빌리는 실제 수집자 본체다.</summary>
    private RawAiCollectors? _aiCollectors;

    /// <summary>조립된 수집자 본체를 빌려준다. 조립 전에는 명시 오류다.</summary>
    public RawAiCollectors AiCollectors => _aiCollectors ?? throw new InvalidOperationException("AI 수집자 미연결");

    /// <summary>같은 생산 조회·HP·로그 장치를 수집자 하위 여덟 갈고리에 연결한다.</summary>
    /// <remarks>2026-11-25 R4-5b 수집자 호출자용으로 추가했다. 근거: cpppj RawCampaignSession.cpp의 수집자 AiCollectorsHooks 조립이다. 지갑·참여·배치·생산·음향의 실제 하위는 R6에서 잇는다.</remarks>
    private void BuildAiCollectorsDevice(IReadOnlyList<OriginalTypeRecord> types, RawAiStrategySetup strategy)
    {
        if (_aiCollectors is not null)
        {
            throw new InvalidOperationException("AI 수집자 장치 중복 연결");
        }

        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("AI 수집자 HP 미연결");
        _aiCollectors = new RawAiCollectors(AiBrainMemory, Pool, types, Game,
            new AiCollectorsGlobals
            {
                Now = () => ProcessTime.Value,
                LocalOwner = () => MissionPlayer.BattleOwner,
                InvalidGraph = () => TerrainBookkeeping.InvalidGraph,
            },
            new AiCollectorsHooks
            {
                Product = strategy.FindProduct,
                CanAfford = (_, _) => throw new InvalidOperationException("AI 수집자 구매 비교 미연결(R6): 지갑·비용"),
                MaxHitPoints = priest => hp.MaxHitPoints(priest),
                EnemyAvailable = _ => throw new InvalidOperationException("AI 수집자 참여 상태 미연결(R6): flags22"),
                MayPlace = _ => throw new InvalidOperationException("AI 수집자 배치 검사 미연결(R6): 일반 MayPlace"),
                Deploy = (_, _, _, _, _, _) => throw new InvalidOperationException("AI 수집자 생산 배치 미연결(R6)"),
                Log = (brain, type, x, y) => _aiRuntimeLogs.Add(brain + ":collector:" + type + ":" + x + "," + y),
                DropSound = () => throw new InvalidOperationException("AI 수집자 효과음 미연결(R6)"),
            });
    }

    /// <summary>현재 owner1..8의 실제 brain에 수집자 확보를 실행한다. brain 부재는 효과 없이 거짓이다.</summary>
    /// <remarks>근거: cpppj RawCampaignSession.cpp의 전체 프레임 수집자 호출 순서다. 매 프레임 호출은 R4-5e 틱 결선에서 잇는다.</remarks>
    public bool ManageAiCollectors(uint owner)
    {
        if (owner < 1 || owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 수집자 소유자는 1..8이어야 합니다.");
        }

        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }

        AiCollectors.Ensure(brain);
        return true;
    }
}
