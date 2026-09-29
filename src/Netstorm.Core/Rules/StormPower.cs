namespace Netstorm.Core.Rules;

/// <summary>생산 창 맨 위 Storm Power 숫자의 표시 색 (원본 형식 문자 ~w·~y·~r)</summary>
public enum StormPowerColor
{
    /// <summary>흰색: 2001 이상</summary>
    White,

    /// <summary>노란색: 1001~2000</summary>
    Yellow,

    /// <summary>빨간색: 1000 이하</summary>
    Red,
}

/// <summary>Storm Power(게임 내 재화) 관련 수치 규칙.</summary>
public static class StormPower
{
    /// <summary>이 값 이하이면 빨간색 (Combatgump.cpp 0043da10)</summary>
    public const int RedAtOrBelow = 1000;

    /// <summary>이 값 이하이면 노란색 (Combatgump.cpp 0043da10)</summary>
    public const int YellowAtOrBelow = 2000;

    /// <summary>Storm Crystal 1개의 가치 (매뉴얼·nugget.type cost 200)</summary>
    public const int CrystalValue = 200;

    /// <summary>자기 유닛을 건강한 상태에서 회수할 때 돌려받는 비율(%) — 매뉴얼, 컨텍스트 메뉴 "Salvage gains"(템플 5000 → 1250, Sun Workshop 800 → 200)와 일치</summary>
    public const int SalvagePercent = 25;

    /// <summary>숫자 표시 색을 고른다.</summary>
    /// <param name="stormPower">현재 Storm Power</param>
    public static StormPowerColor DisplayColor(int stormPower) => stormPower switch
    {
        <= RedAtOrBelow => StormPowerColor.Red,
        <= YellowAtOrBelow => StormPowerColor.Yellow,
        _ => StormPowerColor.White,
    };

    /// <summary>
    /// 건강한 유닛·건물의 회수 금액 = 비용 × 25% (정수 나눗셈).
    /// 손상된 유닛은 매뉴얼상 "더 적다"지만 감소 공식은 미확인이라 여기서는 다루지 않는다.
    /// </summary>
    /// <param name="cost">.type cost</param>
    public static int SalvageValue(int cost) => cost * SalvagePercent / 100;

    /// <summary>적 유닛·건물 파괴 보상 = 비용 × Kill Reward 비율 (기본 25%). 비용이 없는 비행체는 0.</summary>
    /// <param name="cost">파괴한 대상의 .type cost</param>
    /// <param name="rewardPercent">BattleOptions.KillRewardPercent</param>
    public static int KillReward(int cost, int rewardPercent) => cost * rewardPercent / 100;
}
