using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>생산 기점의 스톰 파워와 공급원의 원소 에너지를 기다리는 유닛 생산 상태.</summary>
public sealed class UnitProduction(int sourceId, IReadOnlyList<ProductionDelivery> deliveries)
{
    /// <summary>덱 등록에 사용한 워크샵 번호. 골렘은 템플 번호다.</summary>
    public int SourceId { get; } = sourceId;
    /// <summary>스톰 파워 한 줄기와 필요 원소 글자마다 하나씩의 운송.</summary>
    public IReadOnlyList<ProductionDelivery> Deliveries { get; } = deliveries;
    /// <summary>아직 도착하지 않은 자원이 있는지. 모두 도착해야 실체화가 시작된다.</summary>
    public bool AwaitingDeliveries => Deliveries.Any(d => !d.HasArrived);
}

/// <summary>점유·충돌 없이 이동하는 생산 자원 하나. 출발 뒤에는 공급원 소멸과 무관하게 계속 이동한다.</summary>
public sealed class ProductionDelivery
{
    /// <summary>출발한 워크샵·템플·제네레이터 번호.</summary>
    public int SourceId { get; internal init; }
    /// <summary>실제 공급 원소. null은 스톰 파워 줄기다('s' 요구도 실제 공급원 원소로 표시).</summary>
    public Element? Element { get; internal init; }
    /// <summary>스톰 파워 줄기인지.</summary>
    public bool IsStormPower => Element == null;
    /// <summary>현재 논리 칸 좌표 x.</summary>
    public double X { get; internal set; }
    /// <summary>현재 논리 칸 좌표 y.</summary>
    public double Y { get; internal set; }
    /// <summary>직전 틱의 x (화면 보간용).</summary>
    public double PreviousX { get; internal set; }
    /// <summary>직전 틱의 y (화면 보간용).</summary>
    public double PreviousY { get; internal set; }
    /// <summary>도착할 예약 발자국 중심 x.</summary>
    public double TargetX { get; internal init; }
    /// <summary>도착할 예약 발자국 중심 y.</summary>
    public double TargetY { get; internal init; }
    /// <summary>초당 논리 칸 이동 속도.</summary>
    public double Speed { get; internal init; }
    /// <summary>공중 직선 이동인지. 지상 경로 상실로 전환한 뒤에는 지상으로 돌아가지 않는다.</summary>
    public bool IsAirborne { get; internal set; }
    /// <summary>목적지에 도착했는지.</summary>
    public bool HasArrived { get; internal set; }
    /// <summary>지상 경로. 원소 에너지와 공중 전환 뒤에는 비어 있다.</summary>
    public IReadOnlyList<(int X, int Y)> Path { get; internal set; } = [];
    /// <summary>다음에 도착할 경로 칸 번호.</summary>
    public int NextIndex { get; internal set; }
    /// <summary>마지막으로 경로를 탐색한 지형 버전.</summary>
    public int RouteVersion { get; internal set; }
}
