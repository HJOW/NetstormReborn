using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// AI 사제 행동을 같은 실제 본체/풀/현재 전역에 연결한다.
public sealed partial class HostSession
{
    /// <summary>같은 본체/풀과 실제 하위를 빌리는 사제 행동 본체다.</summary>
    private RawAiPriestActions? _aiPriests;

    /// <summary>조립된 사제 행동 본체를 빌려준다. 조립 전에는 명시 오류다.</summary>
    public RawAiPriestActions AiPriests => _aiPriests ?? throw new InvalidOperationException("AI 사제 미연결");

    /// <summary>같은 HP/수집자/예약/의식 장치를 사제 하위 열일곱 갈고리에 연결한다.</summary>
    /// <param name="processes">같은 실제 장기 프로세스 Host다. 전략 조립과 같은 저장소를 빌린다.</param>
    /// <remarks>2026-11-25 R4-5b 사제 호출자용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 3005~3008행의 priest 갈고리 조립과 2939행의 frameHooks다. 탐색·이동·건설 선택·자원 선택·제단·발자국·회수 명령의 실제 하위는 R6에서 잇는다.</remarks>
    private void BuildAiPriestDevice(SquidProcessHost processes)
    {
        if (_aiPriests is not null)
        {
            throw new InvalidOperationException("AI 사제 장치 중복 연결");
        }

        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("AI 사제 HP 미연결");
        _aiPriests = new RawAiPriestActions(AiBrainMemory, Pool,
            new AiPriestActionGlobals
            {
                Now = () => ProcessTime.Value,
                // 지도 상한은 독립 입력 기본값 256을 쓴다. 전략 조립과 같은 값이며 공유 Client 주소 연결은 R6에서 잇는다.
                MapLimit = () => 256,
                DaisType = () => GameAux.DaisTypeBox.Value,
            },
            new AiPriestActionHooks
            {
                Scan = (_, _) => throw new InvalidOperationException("AI 사제 탐색 미연결(R6): 탐색 본체"),
                Vulnerable = sid => AiCollectors.Vulnerable(sid.Value),
                Busy = actor => processes.FindEvent(actor.Value, 0x493) is not null,
                Path = _ => throw new InvalidOperationException("AI 사제 경로 조회 미연결(R6): 이동 본체"),
                MaxHitPoints = sid => hp.MaxHitPoints(sid.Value),
                Ritual = sid => AiPriestPoints.InRitual(Pool, WorldFrames, sid),
                AltarEnemy = (_, _) => throw new InvalidOperationException("AI 사제 제단 조회 미연결(R6): 제단 조회"),
                Target = (_, _, _, _) => throw new InvalidOperationException("AI 사제 대상 이동 미연결(R6): 이동 본체"),
                Distance = (_, _) => throw new InvalidOperationException("AI 사제 거리 조회 미연결(R6): 발자국 조회"),
                Select = _ => throw new InvalidOperationException("AI 사제 건설 선택 미연결(R6): 건설 선택"),
                Home = (_, _) => throw new InvalidOperationException("AI 사제 귀환 미연결(R6): 이동 본체"),
                Collect = (_, _) => throw new InvalidOperationException("AI 사제 수집 미연결(R6): 자원 선택"),
                Deploy = (token, type, point) => AiCollectors.DeployWithNotice(token, type, point.X, point.Y, 0, 0),
                Begin = (_, _) => throw new InvalidOperationException("AI 사제 탐색 초기화 미연결(R6): 탐색 본체"),
                Point = (_, _, _) => throw new InvalidOperationException("AI 사제 좌표 이동 미연결(R6): 이동 본체"),
                EnemyLowFunds = _ => throw new InvalidOperationException("AI 사제 적 자금 조회 미연결(R6): 건설 선택"),
                Salvage = _ => throw new InvalidOperationException("AI 사제 회수 요청 미연결(R6): 회수 명령"),
            });
    }

    /// <summary>현재 owner1..8의 실제 brain에서 사제 행동을 수행한다. brain이 없으면 거짓이다.</summary>
    /// <param name="owner">사제를 부릴 AI 소유자다.</param>
    /// <param name="priest">행동할 사제 SID 값이다. 0이면 빈 손으로 둔다.</param>
    /// <returns>brain이 있어 행동을 실행하면 참이다.</returns>
    /// <remarks>근거: cpppj RawCampaignSession.cpp 4940행의 ManageAiPriest다. SID 범위 검사는 런타임 갈고리에서 원문 순서로 하며 매 프레임 호출은 R4-5e 틱 결선에서 잇는다.</remarks>
    public bool ManageAiPriest(uint owner, ushort priest)
    {
        if (owner < 1 || owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 사제 소유자는 1..8이어야 합니다.");
        }

        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }

        AiPriests.Act(brain, new Sid(priest));
        return true;
    }
}
