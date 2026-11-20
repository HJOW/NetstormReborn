using Netstorm.Core.Original;

// AI 경제가 같은 세션의 현재 Player·사제 상태·SP를 읽도록 연결한다.
namespace Netstorm.Client.Host;

/// <summary>AI 경제와 실제 미션 상태의 연결을 제공한다.</summary>
public sealed partial class HostSession
{
    /// <summary>세션이 소유한 AI 경제 본체다. 런타임 조립 전에는 null이다.</summary>
    private RawAiEconomy? _aiEconomy;

    /// <summary>현재 미션의 데모 값을 읽는 장치다. 일반 미션은 0이며 데모 진입은 R4-7에서 연결한다.</summary>
    private Func<uint> _aiDemo = static () => 0u;

    /// <summary>같은 SP 저장소에 연결된 경제 본체다. 미조립 상태에서는 명시 오류다.</summary>
    public RawAiEconomy AiEconomy => _aiEconomy ?? throw new InvalidOperationException("AI 경제 미연결");

    /// <summary>현재 미션 소유자의 데모 조회기를 빌린다. 바인딩 이후에도 매번 현재 값을 읽는다.</summary>
    public void BindAiDemo(Func<uint> demo)
    {
        ArgumentNullException.ThrowIfNull(demo);
        _aiDemo = demo;
    }

    /// <summary>현재 참여 플래그와 같은 사제 상태9DWORD로 포획 상태를 판정한다.</summary>
    /// <remarks>근거: cpppj RawCampaignSession.cpp의 AiPriestCaptured와 RawMissionOutcome.cpp의 PriestCaptured다.
    /// 범위 밖은 진단 후 참이라는 원본 특수 결과를 보존한다. 실제 포획 행동은 실행하지 않는다.</remarks>
    private bool AiPriestCaptured(uint owner)
    {
        if (owner < 1 || owner > 8)
        {
            _aiRuntimeDiagnostics.Add("economy:playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS");
            return true;
        }
        return (PlayerRecords[(int)owner * 180 + 0x50] & 0x22) != 0
            && PriestState.Priests.GetOwnerState()[owner] == 1;
    }

    /// <summary>실제 본체·분산 SP·현재 시계·사제 상태를 경제 장치에 연결한다.</summary>
    private void BuildAiEconomyDevice()
    {
        if (_aiEconomy is not null)
        {
            throw new InvalidOperationException("AI 경제 장치 중복 연결");
        }
        _aiEconomy = new RawAiEconomy(AiBrainMemory, Sp, () => ProcessTime.Value, new AiEconomyHooks
        {
            Demo = () => _aiDemo() != 0,
            Captured = AiPriestCaptured,
        });
    }

    /// <summary>현재 owner1..8의 실제 brain 지갑을 SP로 정리한다. brain이 없으면 효과 없이 거짓이다.</summary>
    /// <remarks>근거: cpppj RawCampaignSession.cpp의 ReconcileAiEconomy다. 매 프레임 호출은 하위 행동 연결 뒤 잇는다.</remarks>
    public bool ReconcileAiEconomy(uint owner)
    {
        if (owner < 1 || owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 경제 소유자는 1..8이어야 합니다.");
        }
        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }
        AiEconomy.Reconcile(brain);
        return true;
    }
}
