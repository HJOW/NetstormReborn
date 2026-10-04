namespace Netstorm.Core.Rules;

/// <summary>
/// 캠페인 미션 하나의 공개 정보와 클론 임시 AI 설정.
/// </summary>
/// <param name="FileName">미션 파일 이름 (d/&lt;이름&gt;.english·.fort, 대소문자 무시)</param>
/// <param name="Title">원본 미션 스크립트의 title (loadFort 가 없는 캠페인 미션을 알아보는 데 쓴다)</param>
/// <param name="Code">화면에 표시할 장-미션 번호 (예: "1-2")</param>
/// <param name="AiDefenseLimit">임시 적 AI 가 유지할 방어 유닛 수 상한</param>
public sealed record CampaignMission(string FileName, string Title, string Code, int AiDefenseLimit);

/// <summary>이번 구현 범위에 포함된 캠페인 미션만 공개한다.</summary>
public static class CampaignAccess
{
    /// <summary>플레이할 수 있는 첫 캠페인의 파일 이름.</summary>
    public const string FirstMission = "thewarbegins";

    /// <summary>
    /// "Struggle For Freedom" 장의 미션 파일 이름 (원본 목록 순서, offical3.english). 메뉴 표시 순서로 쓴다.
    /// </summary>
    public static IReadOnlyList<string> FirstChapter { get; } =
        ["thewarbegins", "masterofwhirligigs", "savetheisland", "fragilefortune", "thunderingpower", "dissolvedalliance"];

    /// <summary>
    /// 구현·공개한 캠페인 미션. 방어 상한: 1-1 은 원본 맵의 시작 Sun Disc Thrower 9개,
    /// 1-2 는 2026-10-01 원본 녹화 07:30 에 보인 적 Whirlibase 약 14개(관찰 추정).
    /// </summary>
    public static IReadOnlyList<CampaignMission> Missions { get; } =
    [
        new(FirstMission, "The War Begins!", "1-1", 9),
        new("masterofwhirligigs", "Master of Whirligigs", "1-2", 14),
    ];

    /// <summary>완료 상태와 무관하게 범위 밖 미션은 잠금을 유지한다.</summary>
    /// <param name="mission">미션 파일 이름</param>
    public static bool IsAvailable(string mission) => Find(mission) != null;

    /// <summary>파일 이름 또는 원본 제목으로 공개 미션을 찾는다 (없으면 null).</summary>
    /// <param name="nameOrTitle">미션 파일 이름이나 title</param>
    public static CampaignMission? Find(string? nameOrTitle) => nameOrTitle == null ? null
        : Missions.FirstOrDefault(m => m.FileName.Equals(nameOrTitle, StringComparison.OrdinalIgnoreCase)
            || m.Title.Equals(nameOrTitle, StringComparison.OrdinalIgnoreCase));
}
