using Netstorm.Assets;

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

    /// <summary>cost 가 없는 타입의 기본 비용 단가(레벨당). exe 타입 로더 Rifttype.cpp: (level − 1 + 1) × 200.0 (VA 0x510298).</summary>
    public const int DefaultCostPerLevel = 200;

    /// <summary>cost 가 없는 walker·balloon(수송체) 타입의 레벨당 기본 비용. exe 상수 VA 0x51029c = 400.0.</summary>
    public const int DefaultTransportCostPerLevel = 400;

    /// <summary>
    /// 타입의 Storm Power 비용. .type 에 cost 가 있으면 그 값을, 없으면 원본 타입 로더처럼 group 이 있는 타입에 한해
    /// level × 200(수송체 walker·balloon 은 level × 400)을 쓴다. 골렘(level 1) 400 — 원본 녹화의 판매 환급 100 과 일치,
    /// Whirlibase(level 2) 400 — 매뉴얼 값과 일치. group·level 이 없는 비행체(Whirligig 등)와 사제는 0 이다.
    /// </summary>
    /// <param name="type">타입 정의</param>
    public static int TypeCost(TypeDefinition type)
    {
        if (type.GetInt("cost") is int cost) return cost;
        if (string.IsNullOrEmpty(type.GetString("group"))) return 0;
        int level = Math.Max(0, type.GetInt("level") ?? 0);
        bool transport = type.HasFlag("walker") || type.HasFlag("balloon");
        return level * (transport ? DefaultTransportCostPerLevel : DefaultCostPerLevel);
    }

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
