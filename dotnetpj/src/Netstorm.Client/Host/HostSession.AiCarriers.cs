using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// AI 운반 행동을 같은 실제 본체/풀/TYPE/현재 전역에 연결한다.
public sealed partial class HostSession
{
    /// <summary>같은 본체/풀/TYPE/공용 난수 대신 실제 하위를 빌리는 운반 행동 본체다.</summary>
    private RawAiCarrierActions? _aiCarriers;

    /// <summary>조립된 운반 행동 본체를 빌려준다. 조립 전에는 명시 오류다.</summary>
    public RawAiCarrierActions AiCarriers => _aiCarriers ?? throw new InvalidOperationException("AI 운반 미연결");

    /// <summary>장기 실패점과 같은 값의 다이스 접근점 무효점이다. 원문 BoardTargetState 기본값과 같다.</summary>
    private readonly RawPathPoint _aiCarrierInvalidPoint = new() { X = -10000f, Y = -10000f };

    /// <summary>같은 HP/내용물/그래프/파괴 장치를 운반 하위 스무 갈고리에 연결한다.</summary>
    /// <param name="types">같은 실제 PATCH188 TYPE 표다.</param>
    /// <param name="destroy">같은 실제 월드 파괴다. 전략 조립과 같은 대리자를 빌린다.</param>
    /// <remarks>2026-11-25 R4-5b 운반 호출자용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2964~2968행의 carriers 갈고리 조립과 2939행의 frameHooks다. 이동 본체·자원 선택·carrierDrop의 실제 하위는 R6에서 잇는다.</remarks>
    private void BuildAiCarriersDevice(IReadOnlyList<OriginalTypeRecord> types, Action<int, uint> destroy)
    {
        if (_aiCarriers is not null)
        {
            throw new InvalidOperationException("AI 운반 장치 중복 연결");
        }

        RawSquidReward hp = _fortHp ?? throw new InvalidOperationException("AI 운반 HP 미연결");
        ContainedFinderState contained = _aiFinderState ?? throw new InvalidOperationException("AI 운반 내용물 검색 미연결");
        RawAiGraph graph = _aiGraph ?? throw new InvalidOperationException("AI 운반 그래프 미연결");
        _aiCarriers = new RawAiCarrierActions(AiBrainMemory, Pool, types,
            new AiCarrierActionGlobals
            {
                Now = () => ProcessTime.Value,
                Boss = () => Gameplay.Authority.Boss,
                Checking = new RawCheckingFlag(() => AiChecking.Read()),
                InvalidGraph = () => TerrainBookkeeping.InvalidGraph,
            },
            new AiCarrierActionHooks
            {
                MaxHitPoints = sid => hp.MaxHitPoints(sid.Value),
                Prisoner = actor => AiContainedGenus.Find(Pool, contained, types, actor, 0x20, 0x200000, 0),
                Path = _ => throw new InvalidOperationException("AI 운반 경로 조회 미연결(R6): 이동 본체"),
                DaisPoint = dais => AiContainedGenus.DaisApproachPoint(Pool, types, new RawCheckingFlag(() => AiChecking.Read()), in _aiCarrierInvalidPoint, text => _aiRuntimeDiagnostics.Add("carrier:" + text), dais),
                Point = (_, _, _) => throw new InvalidOperationException("AI 운반 좌표 이동 미연결(R6): 이동 본체"),
                Home = (_, _) => throw new InvalidOperationException("AI 운반 귀환 미연결(R6): 이동 본체"),
                EnemyAvailable = _ => throw new InvalidOperationException("AI 운반 참여 상태 미연결(R6): flags22"),
                Vulnerable = sid => AiCollectors.Vulnerable(sid.Value),
                FlyingGetter = _ => throw new InvalidOperationException("AI 운반 비행 조회 미연결(R6): 자원 선택"),
                Graph = sid => graph.GraphFor(sid.Value),
                Crystals = actor => RawContainedFinder.CountContained(Pool, contained, actor.Value, 0x20, GameAux.NuggetTypeBox.Value),
                FirstCrystal = actor => FindFirstCarrierCrystal(contained, actor),
                DropRequest = _ => throw new InvalidOperationException("AI 운반 하역 요청 미연결(R6): carrierDrop"),
                Destroy = crystal => destroy(crystal.Value, 0),
                Target = (_, _, _) => throw new InvalidOperationException("AI 운반 대상 이동 미연결(R6): 이동 본체"),
                Remember = (_, _) => throw new InvalidOperationException("AI 운반 포획 등록 미연결(R6): 자원 선택"),
                Collect = (_, _) => throw new InvalidOperationException("AI 운반 수집 미연결(R6): 자원 선택"),
                Targets = _ => throw new InvalidOperationException("AI 운반 이동 목표 조회 미연결(R6)"),
                Reclassify = _ => throw new InvalidOperationException("AI 운반 재분류 미연결(R6): 자원 선택"),
                Stop = _ => throw new InvalidOperationException("AI 운반 정지 미연결(R6): 이동 본체"),
                Assertion = (text, line) => _aiRuntimeDiagnostics.Add("carrier:" + text + ":" + line),
            });
    }

    /// <summary>같은 내용물 체인에서 mask 0x20과 현재 크리스털 종류가 모두 맞는 첫 후보를 찾는다. 없으면 0이다.</summary>
    /// <param name="contained">같은 실제 내용물 검색 전역이다.</param>
    /// <param name="actor">조회할 운반자 SID다.</param>
    /// <returns>첫 크리스털 SID며 없으면 0이다.</returns>
    /// <remarks>원문 priests Carrier().FindContained(actor,0x20,현재 크리스털 TYPE,0)와 같은 순서다. DN에 같은 첫 후보 조회가 없어 CountContained와 같은 판정으로 직접 훑는다.</remarks>
    private Sid FindFirstCarrierCrystal(ContainedFinderState contained, Sid actor)
    {
        var finder = new RawContainedFinder(Pool, contained);
        // 원문 순서대로 앞에서부터 훑으며 첫 일치만 돌려주는 반복문이다.
        for (int sid = finder.Begin(actor.Value); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            uint kind = (uint)(raw[18] | (raw[19] << 8));
            uint mask = (uint)(raw[20] | (raw[21] << 8));
            if ((0x20u & mask) != 0 && GameAux.NuggetTypeBox.Value == kind)
            {
                return new Sid(unchecked((ushort)sid));
            }
        }

        return new Sid(0);
    }

    /// <summary>현재 owner1..8의 실제 brain에서 운반 행동을 수행한다. brain이 없으면 0이 아닌 거짓이다.</summary>
    /// <param name="owner">운반할 AI 소유자다.</param>
    /// <returns>brain이 있어 행동을 실행하면 참이다.</returns>
    /// <remarks>근거: cpppj RawCampaignSession.cpp 4930행의 ManageAiCarriers다. 매 프레임 호출은 R4-5e 틱 결선에서 잇는다.</remarks>
    public bool ManageAiCarriers(uint owner)
    {
        if (owner < 1 || owner > 8)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "AI 운반 소유자는 1..8이어야 합니다.");
        }

        if (!AiBrains.TryGetValue(owner, out uint brain))
        {
            return false;
        }

        AiCarriers.Act(brain);
        return true;
    }
}
