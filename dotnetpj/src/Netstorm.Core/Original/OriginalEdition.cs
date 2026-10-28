namespace Netstorm.Core.Original;

/// <summary>
/// 원본 판본(10.78 패치와 CD 10.72). 슬롯 배치·필드 오프셋·동작이 다른 곳에서 이 값으로 갈라진다.
/// 근거: cpppj/src/o/RiftType.h 의 OriginalEdition. (R2d-2, 2026-10-29)
/// </summary>
public enum OriginalEdition
{
    /// <summary>10.78 패치판. dotnetpj 이식의 기준 판본이다.</summary>
    Patch1078 = 0,

    /// <summary>CD 10.72판. 필드 오프셋과 일부 동작이 패치판과 다르다.</summary>
    Cd1072 = 1,
}
