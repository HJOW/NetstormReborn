using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>원본 미션을 막 시작했을 때 승패·AI 이벤트가 잘못 터지지 않는지 모든 미션으로 검사한다.</summary>
public sealed class MissionOutcomeTests
{
    /// <summary>시작 직후 확인할 게임 시간(초)</summary>
    private const double StartupSeconds = 5;

    /// <summary>
    /// 지도가 있는 모든 원본 미션을 열어 몇 초 돌린다. 시작하자마자 성공(BadTeamDead)·실패(Failed) 안내가 나오면
    /// 승패 판정이 미션 데이터를 잘못 읽는 것이므로 목록으로 보고한다.
    /// </summary>
    [Fact]
    public void OriginalMissions_DoNotEndImmediately()
    {
        GameResources resources = OriginalData.RequireResources();
        string[] names = [.. resources.Files.Find(resources.Settings.ExpandSpec("missionSpec", "*"))
            .Select(path => Path.GetFileNameWithoutExtension(path)!)
            .Where(name => resources.TryLoadMission(name) is not null)
            .Distinct(StringComparer.OrdinalIgnoreCase).Order(StringComparer.OrdinalIgnoreCase)];
        var problems = new List<string>();
        int checkedMissions = 0;
        // 미션마다 세션을 만들 수 있으면(지도가 있고 튜토리얼이 아니면) 몇 초 돌려 본다
        foreach (string name in names)
        {
            MissionStart start = MissionStart.FromScript(resources.TryLoadMission(name)!.Script);
            if (start.TutorialNumber != null)
            {
                continue;
            }
            BattleSession session;
            try
            {
                session = SessionData.FromMap(start.LoadFort ?? name, start);
            }
            catch (Exception error) when (error is FileNotFoundException or IOException or InvalidDataException or KeyNotFoundException)
            {
                continue;
            }
            checkedMissions++;
            session.RunTicks((int)(session.TicksPerSecond * StartupSeconds));
            string[] told = [.. session.DrainEvents().Where(e => e.Kind == SessionEventKind.MissionTell).Select(e => e.Text)];
            // 사망·전멸·승패 안내는 아무것도 잃지 않은 시작 직후에 나오면 안 된다 (구출 안내 PriestSaved 는 지도 배치에 달려 있어 제외)
            string[] early = [.. told.Where(section => section is "BadTeamDead" or "GoodTeamDead" or "Failed"
                || section.EndsWith("TempleDead", StringComparison.Ordinal) || section.EndsWith("TempleHalfDead", StringComparison.Ordinal)
                || section.EndsWith("PriestDead", StringComparison.Ordinal))];
            if (early.Length > 0)
            {
                problems.Add($"{name}: {string.Join(",", early)}");
            }
        }
        Assert.True(checkedMissions > 20, $"검사한 미션이 너무 적음: {checkedMissions}");
        Assert.True(problems.Count == 0, "시작 직후 승패 안내가 나온 미션: " + string.Join(" | ", problems));
    }

    /// <summary>구출 미션(Enemy Territory)은 시작 직후 구출 성공·실패 안내가 나오지 않고, 동맹 사제만 포획 대상이 된다.</summary>
    [Fact]
    public void EnemyTerritory_StartsWithoutRescueOutcome()
    {
        MissionStart start = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission("enemyterritory")!.Script);
        Assert.True(start.AllowAnyCapture);
        Assert.True(start.AreAllied(1, 2));
        BattleSession session = SessionData.FromMap(start.LoadFort ?? "enemyterritory", start);
        session.RunTicks(session.TicksPerSecond * 5);
        string[] told = [.. session.DrainEvents().Where(e => e.Kind == SessionEventKind.MissionTell).Select(e => e.Text)];
        Assert.DoesNotContain("ai2PriestSaved", told);
        Assert.DoesNotContain("ai2PriestDead", told);
        Assert.DoesNotContain("Failed", told);
    }

    /// <summary>머리 값 myAllyList 와 aiNAllyList 는 한쪽만 적어도 동맹이며, 목록에 없는 상대는 동맹이 아니다.</summary>
    [Fact]
    public void AllyLists_ReadMyAndAiLists()
    {
        var header = new Dictionary<string, string>
        {
            ["myAllyList"] = "\"4;6\"",
            ["ai2AllyList"] = "\"1\"",
            ["ai5AllyList"] = "\"7\"",
        };
        MissionStart start = MissionStart.FromHeader(key => header.GetValueOrDefault(key));
        Assert.True(start.AreAllied(1, 4));
        Assert.True(start.AreAllied(6, 1));
        Assert.True(start.AreAllied(2, 1));
        Assert.True(start.AreAllied(7, 5));
        Assert.False(start.AreAllied(1, 3));
        Assert.False(start.AreAllied(4, 5));
        // 사람 플레이어 번호가 다르면 myAllyList 의 주인도 바뀐다
        Assert.True(start.AreAllied(3, 4, human: 3));
        Assert.False(start.AreAllied(1, 4, human: 3));
    }
}
