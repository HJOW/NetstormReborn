namespace Netstorm.Core.Simulation;

/// <summary>Whirligig의 출격·귀환·보급 단계.</summary>
public enum FlyerPhase
{
    /// <summary>기지에서 공격할 적을 기다린다.</summary>
    Docked,
    /// <summary>목표로 이동하거나 공격한다.</summary>
    Hunting,
    /// <summary>연료 보급 또는 목표 소멸로 기지에 돌아간다.</summary>
    Returning,
    /// <summary>기지에서 연료를 보충한다.</summary>
    Refuelling,
}

/// <summary>지상 점유와 분리한 비행 상태. 출발 좌표는 기지가 파괴된 뒤에도 사거리 판정에 남긴다.</summary>
public sealed class FlyerFlight
{
    /// <summary>생성한 기지 번호.</summary>
    public int BaseId { get; init; }
    /// <summary>출발 지점의 논리 칸 x.</summary>
    public double OriginX { get; init; }
    /// <summary>출발 지점의 논리 칸 y.</summary>
    public double OriginY { get; init; }
    /// <summary>연속 좌표 x. 발자국은 선택용 반올림 좌표만 저장한다.</summary>
    public double X { get; internal set; }
    /// <summary>연속 좌표 y.</summary>
    public double Y { get; internal set; }
    /// <summary>현재 비행 단계.</summary>
    public FlyerPhase Phase { get; internal set; }
    /// <summary>이번 출격에서 귀환을 시작할 틱.</summary>
    public long ReturnTick { get; internal set; }
    /// <summary>보급을 마칠 틱.</summary>
    public long ReadyTick { get; internal set; }
}
