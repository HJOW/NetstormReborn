using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 튜토리얼 1(Bridge the Gap!)의 단계 판정이 실제 세션의 Totalmade 장부·간헐천 연결·Storm Power 와 맞물려 진행하는지 검사한다.
/// 원본 판정 자체의 기계어 대조는 <see cref="X86TutorialOneTests"/>가, 장부는 <see cref="X86TotalMadeTests"/>가 맡는다.
/// </summary>
/// <remarks>2026-10-15 감사 D05/D14 에서 추가. 표면 Graph 번호는 아직 없어 간헐천 연결은 사제의 도달 가능성으로 근사한다.</remarks>
public sealed class TutorialOneSessionTests
{
    /// <summary>다리 칸 타입의 런타임 번호. 누적 제작 수 표의 칸을 직접 채울 때 쓴다.</summary>
    private static int BridgeIndex => OriginalData.RequireTypes().Find(ProductionDeck.BridgeType)!.RuntimeIndex;

    /// <summary>같은 섬 영역에 실제 타입을 두는 오브젝트(소유자 0은 중립).</summary>
    private static FortMapObject Object(string name, int owner, int x, int y)
    {
        TypeInfo type = OriginalData.RequireTypes().Find(name)!;
        return new FortMapObject(x, y, 0, new FortObject(0, 0, type, null, null, null, null, null, owner, []));
    }

    /// <summary>
    /// 튜토리얼 1 세션을 직접 만든다: 사제·신전(소유자 1)과 간헐천들. 연결되지 않은 구성은 신전 섬과 간헐천 섬 사이에 지면이 없다.
    /// </summary>
    /// <param name="money">시작 Storm Power(myStartMoney).</param>
    /// <param name="connected">간헐천이 신전과 같은 섬에 있는지.</param>
    /// <param name="geysers">간헐천 개수.</param>
    private static BattleSession Create(int money, bool connected, int geysers)
    {
        var header = new Dictionary<string, string> { ["tutorialNumber"] = "1", ["myStartMoney"] = money.ToString(System.Globalization.CultureInfo.InvariantCulture) };
        MissionStart mission = MissionStart.FromHeader(key => header.GetValueOrDefault(key));
        var objects = new List<FortMapObject> { Object("priest", 1, 10, 10), Object("windVortex", 1, 14, 10) };
        // 간헐천은 연결 구성이면 같은 섬에, 아니면 지면이 없는 틈 너머에 둔다.
        for (int i = 0; i < geysers; i++) objects.Add(Object("geyser", 0, connected ? 20 + 6 * i : 60 + 6 * i, 10));
        var grid = new BridgeGrid((x, _) => connected || x < 30 || x > 45, (_, _) => false);
        return new BattleSession(new BattleMap(objects, (_, _) => 0), grid, OriginalData.RequireTypes(), mission);
    }

    /// <summary>F4(섬으로 복귀) 명령으로 단계 A 를 통과시킨다.</summary>
    private static void PassStageA(BattleSession session)
    {
        Assert.Equal('A', session.Tutorial!.Stage);
        session.Submit(new ReturnHomeCommand(1));
        session.RunTicks(1);
        Assert.Equal('B', session.Tutorial.Stage);
    }

    /// <summary>누적 다리 칸 수 표를 채워 단계 B·C 를 통과시켜 D 에 이르게 한다(장부 입력으로 판정만 검사).</summary>
    private static void PassBridgeStages(BattleSession session)
    {
        PassStageA(session);
        session.Ledger.LocalSecondaryCounts[BridgeIndex] = 8;
        session.RunTicks(1);
        Assert.Equal('C', session.Tutorial!.Stage);
        session.Ledger.LocalSecondaryCounts[BridgeIndex] = 19;
        session.RunTicks(1);
        Assert.Equal('D', session.Tutorial.Stage);
    }

    /// <summary>단계 B 는 누적 다리 칸 수가 7보다 커야(8) 통과하고, 단계 C 는 18보다 커야(19) 통과한다(원본 004c3a20).</summary>
    [Theory]
    [InlineData(7, 'B')]
    [InlineData(8, 'C')]
    public void StageB_UsesBuiltBridgeCountGreaterThanSeven(uint built, char expected)
    {
        BattleSession session = Create(0, true, 1);
        PassStageA(session);
        session.Ledger.LocalSecondaryCounts[BridgeIndex] = built;
        session.RunTicks(1);
        Assert.Equal(expected, session.Tutorial!.Stage);
    }

    /// <summary>단계 C 는 누적 다리 칸 수가 18 이하이면 머물고 19부터 통과한다.</summary>
    [Theory]
    [InlineData(18, 'C')]
    [InlineData(19, 'D')]
    public void StageC_UsesBuiltBridgeCountGreaterThanEighteen(uint built, char expected)
    {
        BattleSession session = Create(0, true, 1);
        PassStageA(session);
        session.Ledger.LocalSecondaryCounts[BridgeIndex] = 8;
        session.RunTicks(1);
        session.Ledger.LocalSecondaryCounts[BridgeIndex] = built;
        session.RunTicks(1);
        Assert.Equal(expected, session.Tutorial!.Stage);
    }

    /// <summary>누적 제작 수는 무너지거나 사라져도 줄지 않으므로 현재 수 표만 바뀌어도 단계가 되돌아가지 않는다.</summary>
    [Fact]
    public void BuiltCount_IgnoresLiveCountChanges()
    {
        BattleSession session = Create(0, true, 1);
        PassStageA(session);
        session.Ledger.LocalCounts[BridgeIndex] = 50;
        session.Ledger.GlobalCounts[BridgeIndex] = 50;
        session.RunTicks(3);
        Assert.Equal('B', session.Tutorial!.Stage);
        Assert.Equal(0, session.BuiltForGenus(TypeFlagBits.Bridge));
    }

    /// <summary>
    /// 단계 D: 신전과 같은 Graph(여기서는 사제가 걸어 닿는 곳)의 간헐천이 목록에 몇 개 있어도 한 번의 판정에서 단계는 한 번만 올라
    /// E 에 머문다(원본은 Advance 가 잠금을 걸어 같은 판정의 추가 호출을 거부한다).
    /// </summary>
    [Fact]
    public void StageD_AdvancesOnceEvenWithSeveralConnectedGeysers()
    {
        BattleSession session = Create(0, true, 3);
        PassBridgeStages(session);
        session.RunTicks(1);
        Assert.Equal('E', session.Tutorial!.Stage);
        // 이어지는 판정은 단계 E 의 SP 조건을 읽는다(SP 0 이므로 머문다).
        session.RunTicks(3);
        Assert.Equal('E', session.Tutorial.Stage);
    }

    /// <summary>신전 섬과 이어지지 않은 간헐천뿐이면 단계 D 에 머문다.</summary>
    [Fact]
    public void StageD_StaysWhileGeysersAreDisconnected()
    {
        BattleSession session = Create(0, false, 2);
        PassBridgeStages(session);
        session.RunTicks(5);
        Assert.Equal('D', session.Tutorial!.Stage);
    }

    /// <summary>
    /// 단계 E·F 는 float32 Storm Power 가 각각 200·600 이상일 때 통과한다. 경계 바로 아래는 머문다.
    /// 시작 금액별로 도달하는 최종 단계: 199 → E, 200 → F, 599 → F, 600 → G(마지막).
    /// </summary>
    [Theory]
    [InlineData(199, 'E')]
    [InlineData(200, 'F')]
    [InlineData(599, 'F')]
    [InlineData(600, 'G')]
    public void EnergyStages_UseFloatStormPowerThresholds(int money, char expectedStage)
    {
        BattleSession session = Create(money, true, 1);
        PassBridgeStages(session);
        // 단계 D → E → F → G 까지 이어서 판정한다.
        session.RunTicks(10);
        Assert.Equal(expectedStage, session.Tutorial!.Stage);
        Assert.Equal(expectedStage == 'G', session.Tutorial.Finished);
    }
}
