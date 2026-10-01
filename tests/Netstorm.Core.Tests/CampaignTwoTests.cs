using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>원본 1-2 Master of Whirligigs 맵의 시작 조건·임시 AI(Whirlibase 방어)·결정론·다음 미션 잠금을 검사한다.</summary>
public sealed class CampaignTwoTests
{
    /// <summary>1-2 미션 파일 이름</summary>
    private const string Mission = "masterofwhirligigs";

    /// <summary>원본 머리 값대로 양측 2,000 SP·지식으로 시작하고 임시 AI 대상 미션으로 인식된다.</summary>
    [Fact]
    public void OriginalMission_StartsWithConfiguredState()
    {
        BattleSession session = Create();
        Assert.Equal("1-2", session.Mission!.Campaign!.Code);
        Assert.Equal(2000, session.Player(1).StormPower);
        Assert.Equal(2000, session.Player(2).StormPower);
        // 원본 myTech = sunWalker;windBattery;sunArcher;sunCannon, aiTech = sunWalker;rainBattery;sunAviary
        foreach (string name in new[] { "sunWalker", "windBattery", "sunArcher", "sunCannon" })
            Assert.Contains(name, session.Player(1).Deck.Knowledge, StringComparer.OrdinalIgnoreCase);
        foreach (string name in new[] { "sunWalker", "rainBattery", "sunAviary" })
            Assert.Contains(name, session.Player(2).Deck.Knowledge, StringComparer.OrdinalIgnoreCase);
        Assert.Contains(session.Entities, e => e.Owner == 2 && e.Kind == ObjectKind.Transport);
        Assert.Single(session.Entities, e => e.Owner == 2 && e.Kind == ObjectKind.Temple);
    }

    /// <summary>적 AI 는 골렘 수입으로 돈을 모아 Whirlibase(필요하면 Rain Generator)를 신전 주변에 짓는다.</summary>
    [Fact]
    public void DefensiveAi_BuildsWhirlibases()
    {
        BattleSession session = Create();
        session.RunTicks(session.TicksPerSecond * 240);
        int bases = session.Entities.Count(e => e.Owner == 2 && e.Type.Name.Equals("sunAviary", StringComparison.OrdinalIgnoreCase));
        Assert.True(bases > 0, "적 Whirlibase 가 하나도 없습니다: SP " + session.Player(2).StormPower + ", "
            + string.Join(", ", session.Entities.Where(e => e.Owner == 2).GroupBy(e => e.Type.Name).Select(g => $"{g.Key}×{g.Count()}")));
        Assert.True(bases <= session.Mission!.Campaign!.AiDefenseLimit);
    }

    /// <summary>
    /// 가이저는 2,000 SP(geyser.type cost)를 품고 채집한 만큼 줄어든다. 비면 GeyserDepleted 가 나고 더는 수확 명령을 받지 않는다.
    /// 신전 바로 옆 가이저도 결정마다 채집 시간을 거쳐 틱마다 SP 가 생기지 않는다.
    /// </summary>
    [Fact]
    public void Geysers_DepleteAndBoundIncome()
    {
        BattleSession session = Create();
        GameEntity[] geysers = [.. session.Entities.Where(e => e.Kind == ObjectKind.Geyser)];
        Assert.All(geysers, g => Assert.Equal(2000, g.StoredStormPower));
        int collected = 0;
        long firstDepletion = 0;
        // 4분 동안 채집량과 고갈 시각을 기록한다
        for (int second = 0; second < 240; second++)
        {
            session.RunTicks(session.TicksPerSecond);
            foreach (SessionEvent item in session.DrainEvents())
            {
                if (item.Kind == SessionEventKind.CrystalCollected) collected++;
                if (item.Kind == SessionEventKind.GeyserDepleted && firstDepletion == 0) firstDepletion = item.Tick;
            }
        }
        Assert.Equal(geysers.Length * 2000 - geysers.Sum(g => g.StoredStormPower), collected * StormPower.CrystalValue);
        // 결정 10개를 얻는 데는 적어도 채집 시간 × 10 이 걸린다
        Assert.True(firstDepletion >= (long)(10 * BattleSession.HarvestMineSeconds * session.TicksPerSecond));
        GameEntity empty = Assert.IsType<GameEntity>(geysers.FirstOrDefault(g => g.IsDepletedGeyser));
        GameEntity priest = session.Entities.First(e => e.Owner == 1 && e.Kind == ObjectKind.Priest);
        session.Submit(new HarvestGeyserCommand(1, empty.Id, priest.Id));
        session.RunTicks(1);
        Assert.Contains(session.DrainEvents(), e => e.Kind == SessionEventKind.CommandRejected && e.Failure == CommandFailure.GeyserEmpty);
    }

    /// <summary>같은 시작 조건이면 임시 AI 를 포함한 상태가 결정적이다.</summary>
    [Fact]
    public void AiState_IsDeterministic()
    {
        BattleSession first = Create(); BattleSession second = Create();
        first.RunTicks(first.TicksPerSecond * 90); second.RunTicks(second.TicksPerSecond * 90);
        Assert.Equal(first.Checksum(), second.Checksum());
    }

    /// <summary>1-2 성공 창의 다음 미션(Save The Island)은 아직 구현 범위 밖이다.</summary>
    [Fact]
    public void SuccessScript_KeepsNextMissionLocked()
    {
        MissionScript script = OriginalData.RequireResources().TryLoadMission(Mission)!.Script;
        Assert.Contains("MissionBegin,SaveTheIsland", script.Text.Replace(" ", ""), StringComparison.OrdinalIgnoreCase);
        Assert.True(CampaignAccess.IsAvailable(Mission));
        Assert.False(CampaignAccess.IsAvailable("SaveTheIsland"));
    }

    /// <summary>원본 1-2를 미션 머리 값과 실제 지면으로 시작한다 (loadFort 가 없으므로 미션 이름의 맵을 쓴다).</summary>
    private static BattleSession Create()
    {
        MissionStart start = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission(Mission)!.Script);
        return SessionData.FromMap(start.LoadFort ?? Mission, start);
    }
}
