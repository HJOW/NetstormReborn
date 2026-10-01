using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 사제가 짓는 건물의 건설 시간(초).
/// 원본 exe 의 건설 시간 계산(.type <c>constructionRate</c> 의 쓰임)은 아직 분석하지 않았다. 지금 값은 사용자 직접 조작으로
/// 관찰한 튜토리얼 2(2026-09-30)의 "배치 클릭부터 완공까지" 시간이며, 사제가 걸어가는 시간이 섞여 있다
/// (docs/screens/README.md 1.7절, LEFT_JOBS.md). 사제 이동을 구현하고 exe 를 분석하면 이 표를 교체한다.
/// 게임 중 놓는 유닛(생산 창 → 배치)은 관찰된 지연이 없어 곧바로 완성으로 본다.
/// </summary>
public static class ConstructionTimes
{
    /// <summary>템플 건설 시간(초) — 튜토리얼 2 관찰: 배치 클릭부터 완공까지 약 16초 (이동 포함)</summary>
    public const double TempleSeconds = 16;

    /// <summary>알타 건설 시간(초) — 캠페인 1-5 영상의 클릭부터 완공까지 약 14.5초 (사제 이동 포함, 이동 시간과 분리되지 않은 근사)</summary>
    public const double AltarSeconds = 14.5;

    /// <summary>워크샵 건설 시간(초) — 튜토리얼 2 관찰: 약 10초 (이동 포함)</summary>
    public const double WorkshopSeconds = 10;

    /// <summary>아웃포스트 등 그 밖의 건물 건설 시간(초) — 관찰값이 없어 워크샵과 같게 둔 임시 값</summary>
    public const double OtherBuildingSeconds = 10;

    /// <summary>건물 분류별 건설 시간(초)</summary>
    /// <param name="kind">건물 분류</param>
    public static double Seconds(ObjectKind kind) => kind switch
    {
        ObjectKind.Temple => TempleSeconds,
        ObjectKind.Altar => AltarSeconds,
        ObjectKind.Workshop => WorkshopSeconds,
        _ => OtherBuildingSeconds,
    };
}
