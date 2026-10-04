using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 커스텀 맵 TEST01(originals/d/TEST01.fort)에서 읽는 값이 2026-10-03 시험 전투 녹화와 맞는지 검사한다.
/// </summary>
public sealed class Test01MapTests
{
    /// <summary>TEST01의 myTech=all은 실제 지식으로 확장돼 저장된 워크샵에 아이스·썬더 캐논을 등록할 수 있다.</summary>
    [Fact]
    public void AllKnowledge_AllowsCannonRegistrationInStoredWorkshops()
    {
        MissionStart start = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission("TEST01")!.Script);
        BattleSession session = SessionData.FromMap("TEST01", start);
        PlayerState player = session.Player(1);
        Assert.DoesNotContain("all", player.Deck.Knowledge);
        Assert.Contains("rainCannon", player.Deck.Knowledge);
        Assert.Contains("thunderCannon", player.Deck.Knowledge);
        // 각 캐논 원소의 첫 저장 워크샵에 실제 명령으로 등록한다.
        foreach (string name in new[] { "rainCannon", "thunderCannon" })
        {
            Element element = name == "rainCannon" ? Element.Rain : Element.Thunder;
            GameEntity workshop = session.Entities.First(e => e.Owner == 1 && e.Kind == ObjectKind.Workshop
                && Elements.FromTheme(e.Type.Definition.GetString("theme")) == element);
            session.Submit(new RegisterKnowledgeCommand(1, workshop.Id, name));
            session.RunTicks(1);
            Assert.Contains(player.Deck.Entries(), entry => entry.TypeName.Equals(name, StringComparison.OrdinalIgnoreCase));
        }
    }

    /// <summary>
    /// 맵에 저장된 워크샵은 저장 상태(0·1·2)가 레벨 I·II·III 이 된다. 녹화에서 저장 상태 2 인 Sun 워크샵의
    /// 풍선 도움말이 "Sun Workshop Level III" 였다.
    /// </summary>
    [Fact]
    public void StoredWorkshops_StartAtSavedLevel()
    {
        BattleSession session = SessionData.FromMap("TEST01");
        GameEntity[] workshops = [.. session.Entities.Where(e => e.Kind == ObjectKind.Workshop)];
        Assert.NotEmpty(workshops);
        // 워크샵마다 생산 창의 레벨이 저장 상태 + 1 인지 본다
        foreach (GameEntity workshop in workshops)
        {
            int expected = (workshop.Source!.Object.FactoryState ?? 0) + 1;
            Assert.Equal(expected, session.Player(workshop.Owner).Deck.WorkshopLevel(workshop.Id));
        }
        // 세 레벨이 모두 섞여 있는 맵이어야 이 검사가 의미가 있다
        Assert.Equal([1, 2, 3], workshops.Select(w => session.Player(w.Owner).Deck.WorkshopLevel(w.Id)).Distinct().Order());
    }

    /// <summary>
    /// TEST01 의 머리 값 ai2color=orange·ai3color=red 는 색 번호 8·2 로 읽힌다. 시험 전투에서는 게임 본체가 이 덮어쓰기를
    /// 비우므로(녹화의 소유자 2 빨강·3 흰색) 여기서는 읽기만 확인한다.
    /// </summary>
    [Fact]
    public void Header_AiColorsAreParsed()
    {
        MissionStart start = MissionStart.FromScript(OriginalData.RequireResources().TryLoadMission("TEST01")!.Script);
        Assert.Equal(8, start.AiColors[2]);
        Assert.Equal(2, start.AiColors[3]);
    }
}
