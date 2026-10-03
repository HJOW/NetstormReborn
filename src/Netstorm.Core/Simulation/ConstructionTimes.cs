namespace Netstorm.Core.Simulation;

/// <summary>
/// 사제가 짓는 건물의 건설 시간(초).
/// 건설 시간은 **사제가 현장에 도착한 뒤** 건물이 완성되기까지다. 원본은 설치 클릭에서 비용을 차감하고 공사장 그림자를 놓은 뒤
/// 사제가 걸어가 도착해야 건설을 시작한다 (2026-10-03 원본 자동 분석: 설치 클릭에서 완성까지 약 23초 중 걷기를 뺀 건설이 10~11초,
/// docs/videos/auto-war-begins-20261003.md 4.5절). 원본 .type 의 <c>constructionRate</c> 는 템플·워크샵·아웃포스트 모두 10(생략 시 기본값
/// 10.0, docs/exe/priest-construction.md)이고, 웹 팬게임도 모든 건물을 도착 뒤 10초에 짓는다.
/// 이전에 쓰던 템플 16초·알타 14.5초는 사제가 걸어간 시간이 섞인 "클릭부터 완공" 관찰값이었다.
/// 실제 시간 계산식(constructionRate 의 소비 경로)은 아직 분석하지 않아 건물 종류와 무관하게 같은 값을 쓴다.
/// 게임 중 놓는 유닛(생산 창 → 배치)은 관찰된 지연이 없어 곧바로 완성으로 본다.
/// </summary>
public static class ConstructionTimes
{
    /// <summary>사제가 도착한 뒤 건물이 완성되기까지의 시간(초) — 워크샵 관찰 10~11초, constructionRate 10</summary>
    public const double BuildingSeconds = 10;
}
