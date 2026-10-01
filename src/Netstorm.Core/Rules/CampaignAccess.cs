namespace Netstorm.Core.Rules;

/// <summary>이번 구현 범위에 포함된 캠페인 미션만 공개한다.</summary>
public static class CampaignAccess
{
    /// <summary>플레이할 수 있는 첫 캠페인의 파일 이름.</summary>
    public const string FirstMission = "thewarbegins";

    /// <summary>완료 상태와 무관하게 범위 밖 미션은 잠금을 유지한다.</summary>
    public static bool IsAvailable(string mission) => mission.Equals(FirstMission, StringComparison.OrdinalIgnoreCase);
}
