using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

// 같은 세션 풀·TYPE·공급 목록·동맹에서 배치 에너지 판정을 실행한다.
public sealed partial class HostSession
{
    /// <summary>같은 세션 공급원·선택·동맹·반경으로 조립한 배치 에너지 본체다. 비어 있으면 미조립이다.</summary>
    private RawEnergySupply? _energySupply;

    /// <summary>같은 세션 공급 판정의 현재 원문 전역이다.</summary>
    public EnergySupplyState EnergyState { get; } = new();

    /// <summary>커서·생산·Builder의 마지막 선택 목록이다. 수집이 30칸으로 다시 쓴다.</summary>
    public RawPostPopList EnergySelected { get; } = new() { Entries = new uint[RawEnergySupplyLimits.SourceCapacity] };

    /// <summary>실제 입력 경로가 공급 판정을 요청하는 같은 세션 에너지 본체다.</summary>
    public RawEnergySupply EnergySupply => _energySupply ?? throw new InvalidOperationException("세션 에너지 본체 미연결");

    /// <summary>같은 세션 풀·공급 목록·동맹으로 배치 에너지 본체를 연결한다.</summary>
    /// <param name="types">같은 실제 TYPE 표다. 188개여야 한다.</param>
    /// <param name="requirement">현재 TYPE의 실제 mana 바이트다. TYPE+a0 보호 AST 원천은 R6에서 잇는다.</param>
    /// <param name="providerEntry">현재 가상 표의 실제+a0 주소다. 비어 있으면 실제 83표 분배를 쓴다.</param>
    /// <remarks>2026-11-26 1.055에서 SL-3b 에너지 결선용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 1322행의 energySupply 조립이다. 공급 목록·동맹·반경은 실제 세션 값이며 요구 문자열 원천과 호출자 결선은 R6 배치·AI에서 잇는다.</remarks>
    public void BuildEnergyDevices(IReadOnlyList<OriginalTypeRecord> types, Func<uint, string> requirement, Func<uint, uint>? providerEntry = null)
    {
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(requirement);
        if (_energySupply is not null) throw new InvalidOperationException("세션 에너지 장치 중복 연결");
        EnergyState.UserCheat = Gameplay.UserCheat.Value;
        _energySupply = new RawEnergySupply(Pool, types, SidLists.Providers, EnergySelected,
            Gameplay.PlayerRecords.Alliances, Gameplay.WorldCells.Value, EnergyState,
            new EnergySupplyHooks { ProviderEntry = providerEntry ?? RawEnergySupply.EnergyProviderEntry, Requirement = requirement });
    }

    /// <summary>실제 공급원을 모아 공급 문자를 만든다. 입력 경로의 Collect 호출자다.</summary>
    public string CollectEnergy(float x, float y, uint type, uint owner, int theme = -1) => EnergySupply.Collect(x, y, type, owner, theme);

    /// <summary>실제 공급 문자를 요구 문자에 대응시킨다. 입력 경로의 Match 호출자다.</summary>
    public uint MatchEnergy(ref string available, string required, out string remainder, int theme = -1) => EnergySupply.Match(ref available, required, out remainder, theme);

    /// <summary>실제 배치 허용을 판정한다. 입력 경로의 Evaluate 호출자다.</summary>
    public uint EvaluateEnergy(float x, float y, uint type, uint owner, out string output) => EnergySupply.Evaluate(x, y, type, owner, out output);

    /// <summary>실제 공급 목록을 앞에서부터 소비한다. 입력 경로의 TakeSource 호출자다.</summary>
    public uint TakeEnergySource() => EnergySupply.TakeSource();

    /// <summary>실제 공급원을 가까운 순으로 다시 보관한다. 입력 경로의 SortSources 호출자다.</summary>
    public void SortEnergySources(string demand, float x, float y) => EnergySupply.SortSources(demand, x, y);
}
