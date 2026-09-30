using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>미션 머리 값 → 시작 조건 (시작 Storm Power·지식·기술 허용) 테스트</summary>
public sealed class MissionStartTests
{
    /// <summary>techAllowed: deny → all 로 전부 막고, allow 뒤의 이름만 허용 (원본 FUN_00482eb0 순서)</summary>
    [Fact]
    public void TechPermissions_AppliesTokensInOrder()
    {
        TechPermissions tech = TechPermissions.Parse("deny;all;allow;windVortex;sunArcher");
        Assert.True(tech.IsAllowed("windvortex"));
        Assert.True(tech.IsAllowed("SUNARCHER"));
        Assert.False(tech.IsAllowed("sunCannon"));
        // 값이 없으면 모두 허용, 나중의 "all" 은 앞선 개별 값을 덮어쓴다.
        Assert.True(TechPermissions.Parse(null).IsAllowed("sunCannon"));
        Assert.False(TechPermissions.Parse("allow;sunCannon;deny;all").IsAllowed("sunCannon"));
        Assert.False(TechPermissions.Parse("deny;bulf").IsAllowed("bulf"));
        Assert.True(TechPermissions.Parse("deny;bulf").IsAllowed("sunCannon"));
    }

    /// <summary>기술 허용 표는 실행 중에 바꿀 수 있다: 개별 허용·금지, 전체 다시 채우기 (원본 FUN_004c23e0·004c23c0)</summary>
    [Fact]
    public void TechPermissions_CanBeChangedAtRuntime()
    {
        TechPermissions tech = TechPermissions.Parse("deny;all;allow;windVortex");
        Assert.False(tech.IsAllowed("sunFactory"));
        tech.Set("sunFactory", true);
        Assert.True(tech.IsAllowed("SUNFACTORY"));
        tech.Set("windVortex", false);
        Assert.False(tech.IsAllowed("windVortex"));
        // SetAll 은 개별 값을 모두 지운다
        tech.SetAll(true);
        Assert.True(tech.IsAllowed("windVortex"));
        Assert.True(tech.IsAllowed("anything"));
    }

    /// <summary>원본 튜토리얼 1·2 의 머리 값: 0 SP / 10,000 SP·Sun Disc Thrower 지식 (원본 실행 관찰과 같음)</summary>
    [Fact]
    public void OriginalTutorials_MatchObservedStart()
    {
        GameResources resources = OriginalData.RequireResources();
        MissionStart one = MissionStart.FromScript(resources.TryLoadMission("tutorial1")!.Script);
        Assert.Equal(("Bridge the Gap!", "BridgeTheGap", 0, 1), (one.Title, one.LoadFort, one.StartStormPower, one.TutorialNumber));
        Assert.Empty(one.Knowledge);
        Assert.True(one.Tech.IsAllowed("windVortex"));
        Assert.False(one.Tech.IsAllowed("sunArcher"));
        Assert.True(one.DenySalvage && one.DenyAscend && one.AiOff);

        MissionStart two = MissionStart.FromScript(resources.TryLoadMission("tutorial2")!.Script);
        Assert.Equal(10000, two.StormPower(new BattleOptions()));
        Assert.Equal(["sunArcher"], two.Knowledge);
        var deck = new ProductionDeck();
        two.ApplyKnowledge(deck);
        Assert.Contains("sunArcher", deck.Knowledge);
    }

    /// <summary>myStartMoney 가 없으면 전투 옵션의 시작 금액을 쓴다</summary>
    [Fact]
    public void StormPower_FallsBackToBattleOption()
    {
        MissionStart start = MissionStart.FromHeader(_ => null);
        Assert.Equal(new BattleOptions().StartingStormPower, start.StormPower(new BattleOptions()));
    }
}
