namespace Netstorm.Core.Rules;

/// <summary>
/// 전투 옵션 하나의 정의 (원본 옵션 표 VA 0x52f5d0 의 24바이트 레코드, docs/exe/battle-options.md 2절).
/// </summary>
/// <param name="Number">옵션 번호 (저장 위치 0x52f7e0 + 번호)</param>
/// <param name="Name">원본 옵션 이름</param>
/// <param name="Minimum">옵션 화면에서 고를 수 있는 최소 인덱스</param>
/// <param name="Maximum">최대 인덱스</param>
/// <param name="Default">기본 인덱스</param>
/// <param name="Choices">선택지 이름 (인덱스 0부터)</param>
public sealed record BattleOptionDefinition(int Number, string Name, int Minimum, int Maximum, int Default,
    IReadOnlyList<string> Choices);

/// <summary>
/// 전투 옵션 값 모음과 그 값에서 파생되는 규칙 수치. 기본값은 원본 옵션 표의 기본 인덱스다.
/// 게임 규칙은 이 객체만 보고 수치를 얻으므로 미션·튜토리얼이 값을 바꾸면 곧바로 반영된다.
/// </summary>
public sealed class BattleOptions
{
    /// <summary>Bridge Slots 옵션 번호</summary>
    public const int BridgeSlots = 1;

    /// <summary>Unit Rate 옵션 번호 (생산 창 재충전 속도)</summary>
    public const int UnitRate = 2;

    /// <summary>Generator Range 옵션 번호 (에너지 공급 반지름)</summary>
    public const int GeneratorRange = 4;

    /// <summary>Starting Cash 옵션 번호</summary>
    public const int StartingCash = 14;

    /// <summary>Kill Reward 옵션 번호</summary>
    public const int KillReward = 15;

    /// <summary>Money per Geyser 옵션 번호</summary>
    public const int MoneyPerGeyser = 16;

    /// <summary>Generator Range 인덱스별 반지름(칸) — 원본 float 표 VA 0x5425f4</summary>
    private static readonly double[] GeneratorRangeTable = [14, 22, 30, 38];

    /// <summary>원본 004b4860 이 제한하는 반지름 하한 = 표[0]</summary>
    private const double MinimumGeneratorRange = 14;

    /// <summary>원본 004b4860 이 제한하는 반지름 상한 = 표[2] (Very Long 38 도 30 이 된다)</summary>
    private const double MaximumGeneratorRange = 30;

    /// <summary>Bridge Slots 인덱스별 다리 칸 수</summary>
    private static readonly int[] BridgeSlotTable = [2, 4, 6];

    /// <summary>Starting Cash 인덱스별 시작 Storm Power</summary>
    private static readonly int[] StartingCashTable = [5000, 6500, 7000];

    /// <summary>Kill Reward 인덱스별 보상 비율(%)</summary>
    private static readonly int[] KillRewardTable = [0, 25, 50, 75, 100, 150];

    /// <summary>Money per Geyser 인덱스별 가이저 초기 보유량</summary>
    private static readonly int[] MoneyPerGeyserTable = [1000, 2000, 3000, 5000];

    /// <summary>원본 옵션 표에서 규칙에 쓰는 항목 (번호 순). 나머지 옵션은 맵 생성·멀티플레이 구현 때 추가한다.</summary>
    public static IReadOnlyList<BattleOptionDefinition> Definitions { get; } =
    [
        new(BridgeSlots, "Bridge Slots", 0, 2, 2, ["2", "4", "6"]),
        new(UnitRate, "Unit Rate", 1, 2, 2, ["Slow", "Medium", "Fast"]),
        new(GeneratorRange, "Generator Range", 1, 3, 3, ["Short", "Normal", "Long", "Very Long"]),
        new(StartingCash, "Starting Cash", 0, 2, 1, ["5000", "6500", "7000"]),
        new(KillReward, "Kill Reward", 1, 5, 2, ["0%", "25%", "50%", "75%", "100%", "150%"]),
        new(MoneyPerGeyser, "Money per Geyser", 0, 3, 2, ["1000", "2000", "3000", "5000"]),
    ];

    /// <summary>옵션 번호 → 현재 인덱스</summary>
    private readonly Dictionary<int, int> _values = [];

    /// <summary>모든 옵션을 원본 기본 인덱스로 둔다</summary>
    public BattleOptions()
    {
        // 정의된 옵션마다 기본 인덱스를 넣는다.
        foreach (BattleOptionDefinition definition in Definitions)
        {
            _values[definition.Number] = definition.Default;
        }
    }

    /// <summary>옵션 인덱스를 읽는다. 정의되지 않은 번호는 0.</summary>
    /// <param name="number">옵션 번호</param>
    public int Get(int number) => _values.GetValueOrDefault(number);

    /// <summary>
    /// 옵션 인덱스를 바꾼다. 선택지 개수 범위(0 ~ 최대)로 제한한다.
    /// 원본 튜토리얼 2 처럼 옵션 화면의 최소값(Minimum)보다 작은 값도 코드로는 넣을 수 있다.
    /// </summary>
    /// <param name="number">옵션 번호</param>
    /// <param name="index">선택지 인덱스</param>
    public void Set(int number, int index)
    {
        BattleOptionDefinition? definition = Definitions.FirstOrDefault(d => d.Number == number);
        _values[number] = definition == null ? index : Math.Clamp(index, 0, definition.Maximum);
    }

    /// <summary>에너지 공급 반지름(칸). 원본 004b4860: 표 값을 14~30 으로 제한한다.</summary>
    public double GeneratorRadius =>
        Math.Clamp(GeneratorRangeTable[Math.Clamp(Get(GeneratorRange), 0, GeneratorRangeTable.Length - 1)],
            MinimumGeneratorRange, MaximumGeneratorRange);

    /// <summary>에너지 공급 반지름의 제곱 (원본 DAT_0052f48c, 칸 거리² 비교용)</summary>
    public double GeneratorRadiusSquared => GeneratorRadius * GeneratorRadius;

    /// <summary>생산 창의 다리 칸 수 (2·4·6)</summary>
    public int BridgeSlotCount => BridgeSlotTable[Math.Clamp(Get(BridgeSlots), 0, BridgeSlotTable.Length - 1)];

    /// <summary>시작 Storm Power (멀티플레이 옵션. 캠페인 미션은 미션 설정을 따른다)</summary>
    public int StartingStormPower => StartingCashTable[Math.Clamp(Get(StartingCash), 0, StartingCashTable.Length - 1)];

    /// <summary>적 파괴 보상 비율(%)</summary>
    public int KillRewardPercent => KillRewardTable[Math.Clamp(Get(KillReward), 0, KillRewardTable.Length - 1)];

    /// <summary>가이저 초기 보유량</summary>
    public int GeyserStormPower => MoneyPerGeyserTable[Math.Clamp(Get(MoneyPerGeyser), 0, MoneyPerGeyserTable.Length - 1)];

    /// <summary>
    /// 원본 튜토리얼 2(Secret Workshop) 단계 'A' 처리(Totalmade.cpp 004c3bb0): Generator Range = Short(14칸), Unit Rate = Fast.
    /// </summary>
    public void ApplyTutorialTwoOverrides()
    {
        Set(GeneratorRange, 0);
        Set(UnitRate, 2);
    }

    /// <summary>
    /// 캠페인·튜토리얼(싱글 플레이) 전투의 처치 보상 25%. exe 전투 초기화 FUN_004b2df0 이 보상 비율 DAT_005424bc 를 0x19(25)로 두고,
    /// 옵션 표 값(FUN_0041ca70, 기본 인덱스 2 = 50%)으로 바꾸는 곳은 네트워크 옵션 동기화 FUN_004b4900 뿐이다.
    /// 캠페인 1-2 녹화의 적 템플 파괴 보상(사용자 설명: 비용의 25%)과 같다. 멀티플레이는 옵션 표 값을 쓴다.
    /// </summary>
    public void ApplySinglePlayerKillReward() => Set(KillReward, 1);
}
