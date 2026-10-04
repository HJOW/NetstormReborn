using Netstorm.Assets;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 이동형 유닛마다 다른 .type speed 값을 읽는다. 칸/초 변환은 아직 원본 실행 검증 전의 근사다.
/// 후속 비행형 이동 구현에서는 출발 시 이륙, 이동 후 목적지 착륙을 별도 단계로 처리해야 한다.
/// 현재 이 값은 사제의 지상 수집 이동에만 적용된다.
/// </summary>
public static class MovementRate
{
    /// <summary>speed 속성이 없는 이동체의 임시 속도.</summary>
    private const double DefaultSpeed = 1.0;

    /// <summary>잘못된 파일 값이 한 틱의 이동을 무한하게 만들지 않도록 제한할 최소값.</summary>
    private const double MinimumSpeed = 0.1;

    /// <summary>잘못된 파일 값이 한 틱의 이동을 무한하게 만들지 않도록 제한할 최대값.</summary>
    private const double MaximumSpeed = 20.0;

    /// <summary>해당 유닛의 정의에 저장된 이동 속도를 읽는다. 비행체의 이륙·착륙 시간은 이 값과 별도로 분석한다.</summary>
    public static double CellsPerSecond(TypeInfo type) =>
        Math.Clamp(type.Definition.GetDouble("speed") ?? DefaultSpeed, MinimumSpeed, MaximumSpeed);
}
