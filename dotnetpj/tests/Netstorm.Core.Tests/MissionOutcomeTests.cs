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
        // ai2AllyList = "1": 원본 AI 생성이 2 → 1 방향을 쓰고 로컬의 반대 방향(1 → 2)을 더해 양방향 동맹이 된다.
        Assert.NotEqual(0u, start.AllyMask(2) & (1u << 1));
        BattleSession session = SessionData.FromMap(start.LoadFort ?? "enemyterritory", start);
        Assert.True(session.Map.AreMutualAllies(1, 2));
        session.RunTicks(session.TicksPerSecond * 5);
        string[] told = [.. session.DrainEvents().Where(e => e.Kind == SessionEventKind.MissionTell).Select(e => e.Text)];
        Assert.DoesNotContain("ai2PriestSaved", told);
        Assert.DoesNotContain("ai2PriestDead", told);
        Assert.DoesNotContain("Failed", told);
    }

    /// <summary>
    /// 원본 동맹은 방향별이다: 사람 쪽 목록(myAllyList)은 원본 실행 파일에 없는 키라 읽지 않고, AI 의 목록이 자기 방향의 행만 정한다.
    /// AI 가 로컬을 선언했을 때만 로컬의 반대 방향이 더해지고, 다른 AI 사이의 관계는 대칭화하지 않는다.
    /// </summary>
    [Fact]
    public void AllyLists_AreDirectionalAndIgnoreMyAllyList()
    {
        var header = new Dictionary<string, string>
        {
            ["myAllyList"] = "\"4;6\"",
            ["ai2AllyList"] = "\"1\"",
            ["ai5AllyList"] = "\"7\"",
        };
        MissionStart start = MissionStart.FromHeader(key => header.GetValueOrDefault(key));
        PlayerParticipation relations = MissionParticipation.Create(start, 1, [2, 4, 5, 6, 7]);
        // ai2AllyList = "1": 2 → 1 선언 뒤 로컬의 반대 방향(1 → 2)이 더해져 양방향이다.
        Assert.True(relations.Allies(2, 1));
        Assert.True(relations.Allies(1, 2));
        Assert.True(relations.MutualAllies(1, 2));
        // myAllyList 는 읽지 않는다: 4·6 은 어느 방향으로도 로컬의 동맹이 아니다.
        Assert.False(relations.Allies(1, 4));
        Assert.False(relations.Allies(4, 1));
        Assert.False(relations.Allies(1, 6));
        // ai5AllyList = "7": 5 → 7 한 방향만 선언했고 7 은 선언하지 않았다.
        Assert.True(relations.Allies(5, 7));
        Assert.False(relations.Allies(7, 5));
        Assert.False(relations.MutualAllies(5, 7));
        // 자기 자신은 항상 동맹이고 참여는 사제가 있는 AI 와 로컬 사람뿐이다.
        Assert.True(relations.Allies(4, 4));
        Assert.True(relations.Participates(1));
        Assert.True(relations.Participates(2));
        Assert.False(relations.Participates(3));
        // 사제가 없는 소유자의 AI 는 만들지 않아 동맹 행도 설정하지 않는다(자기 비트만 있다).
        var noPriest = MissionParticipation.Create(start, 1, [4]);
        Assert.False(noPriest.Participates(2));
        Assert.False(noPriest.Allies(2, 1));
        Assert.False(noPriest.Allies(1, 2));
    }

    /// <summary>
    /// 동맹 마스크는 공통 aiAllyList 를 모든 AI 에 더한 뒤 소유자별 목록을 더하고, 항목은 ';' 로만 나눠 atol 을 적용한다
    /// (쉼표는 구분자가 아니다). 앞 10개 항목만 읽고 양수만 1 &lt;&lt; (값 &amp; 31) 비트가 된다.
    /// </summary>
    [Fact]
    public void AllyMask_AppliesCommonListThenOwnerListWithOriginalParsing()
    {
        var header = new Dictionary<string, string>
        {
            ["aiAllyList"] = "\"4\"",
            ["ai2AllyList"] = "\"1;3,5\"",
            ["ai3AllyList"] = "\"1;1;1;1;1;1;1;1;1;1;6\"",
            ["ai4AllyList"] = "\"0;-2;abc; 7;33\"",
        };
        MissionStart start = MissionStart.FromHeader(key => header.GetValueOrDefault(key));
        // 공통 4 + ai2 의 1, "3,5"(= 3 하나) → 비트 4·1·3
        Assert.Equal((1u << 4) | (1u << 1) | (1u << 3), start.AllyMask(2));
        // 11번째 항목(6)은 읽지 않는다.
        Assert.Equal((1u << 4) | (1u << 1), start.AllyMask(3));
        // 0·음수·숫자 아님은 버리고, 앞 공백은 atol 이 건너뛰며, 33 은 비트 31 & 33 = 1 로 감긴다.
        Assert.Equal((1u << 4) | (1u << 7) | (1u << 1), start.AllyMask(4));
        // 목록이 없는 소유자는 공통 목록만 받는다. 이 마스크에는 자기 비트가 없다.
        Assert.Equal(1u << 4, start.AllyMask(6));
    }
}
