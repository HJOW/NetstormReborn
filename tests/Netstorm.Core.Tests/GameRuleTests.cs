using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>전투 옵션·Storm Power·생산 창·섬 소유권·고정 틱·난수 규칙 테스트</summary>
public sealed class GameRuleTests
{
    /// <summary>공급 반지름: 기본 Very Long(38) 은 상한 30, Normal 22, 튜토리얼 2 는 Short 14</summary>
    [Fact]
    public void GeneratorRadius_ClampsLikeOriginal()
    {
        var options = new BattleOptions();
        Assert.Equal(30, options.GeneratorRadius);
        Assert.Equal(900, options.GeneratorRadiusSquared);
        options.Set(BattleOptions.GeneratorRange, 1);
        Assert.Equal(22, options.GeneratorRadius);
        options.ApplyTutorialTwoOverrides();
        Assert.Equal(14, options.GeneratorRadius);
        Assert.Equal(2, options.Get(BattleOptions.UnitRate));
        // 범위를 넘는 인덱스는 선택지 최대값으로 제한한다.
        options.Set(BattleOptions.GeneratorRange, 9);
        Assert.Equal(30, options.GeneratorRadius);
    }

    /// <summary>원본 옵션 표의 기본값: 다리 6칸, 시작 6500, 보상 50%, 가이저 3000</summary>
    [Fact]
    public void BattleOptions_Defaults()
    {
        var options = new BattleOptions();
        Assert.Equal(6, options.BridgeSlotCount);
        Assert.Equal(6500, options.StartingStormPower);
        Assert.Equal(50, options.KillRewardPercent);
        Assert.Equal(3000, options.GeyserStormPower);
    }

    /// <summary>발자국: 기준점은 오른쪽 아래 칸, 중심은 사각형 가운데</summary>
    [Fact]
    public void Footprint_AnchorIsBottomRight()
    {
        var temple = new Footprint(20, 10, 8, 6);
        Assert.Equal((13, 5), (temple.Left, temple.Top));
        Assert.Equal((16.5, 7.5), (temple.CenterX, temple.CenterY));
        Assert.True(temple.Contains(13, 5));
        Assert.False(temple.Contains(21, 10));
        Assert.Equal(48, temple.Cells().Count());
        Assert.True(temple.Overlaps(new Footprint(13, 5, 1, 1)));
        Assert.False(temple.Overlaps(new Footprint(12, 5, 1, 1)));
    }

    /// <summary>Storm Power 색: ≤1000 빨강, 1001~2000 노랑, 2001 이상 흰색 (0043da10)</summary>
    [Theory]
    [InlineData(0, StormPowerColor.Red)]
    [InlineData(1000, StormPowerColor.Red)]
    [InlineData(1001, StormPowerColor.Yellow)]
    [InlineData(2000, StormPowerColor.Yellow)]
    [InlineData(2001, StormPowerColor.White)]
    public void StormPowerColor_Thresholds(int value, StormPowerColor expected)
    {
        Assert.Equal(expected, StormPower.DisplayColor(value));
    }

    /// <summary>회수 금액 25%: 템플 5000 → 1250, Sun Workshop 800 → 200 (컨텍스트 메뉴 캡처), 파괴 보상 비율 적용</summary>
    [Fact]
    public void Salvage_AndKillReward()
    {
        Assert.Equal(1250, StormPower.SalvageValue(5000));
        Assert.Equal(200, StormPower.SalvageValue(800));
        Assert.Equal(300, StormPower.KillReward(1200, 25));
        Assert.Equal(0, StormPower.KillReward(0, 150));
    }

    /// <summary>재충전 간격: 기본 10/5/1초, 요새 모드 0.0001초, useProductTimers 30/15/8초</summary>
    [Fact]
    public void ProductionTimers_Intervals()
    {
        Assert.Equal(10, ProductionTimers.RefreshInterval(0, false));
        Assert.Equal(5, ProductionTimers.RefreshInterval(1, false));
        Assert.Equal(1, ProductionTimers.RefreshInterval(2, false));
        Assert.Equal(0.0001, ProductionTimers.RefreshInterval(0, true));
        Assert.Equal(15, ProductionTimers.RefreshInterval(1, true, useProductTimers: true));
    }

    /// <summary>템플이 있으면 다리·골렘이 덱에 있고, 템플이 없어지면 사라진다</summary>
    [Fact]
    public void Deck_TempleSuppliesBridgeAndGolem()
    {
        var deck = new ProductionDeck();
        Assert.Empty(deck.Entries());
        deck.SetTemple(7);
        Assert.Equal([DeckEntryKind.Bridge, DeckEntryKind.Golem], deck.Entries().Select(e => e.Kind));
        deck.RemoveTemple();
        Assert.Empty(deck.Entries());
    }

    /// <summary>
    /// 워크샵 등록: 자기 원소만(Sun Workshop 은 Generator 예외), 한 유닛은 한 워크샵에만, Level I 은 2칸,
    /// 파괴되면 등록이 풀리고 다른 워크샵에 다시 등록할 수 있다.
    /// </summary>
    [Fact]
    public void Deck_WorkshopRegistrationRules()
    {
        var sunCannon = new ProducibleUnit("suncannon", Element.Sun, false);
        var rainGenerator = new ProducibleUnit("rainBattery", Element.Rain, true);
        var iceCannon = new ProducibleUnit("raincannon", Element.Rain, false);
        var whirlibase = new ProducibleUnit("sunaviary", Element.Sun, false);
        var deck = new ProductionDeck();
        // 획득한 지식을 모두 등록 가능하게 한다.
        foreach (ProducibleUnit unit in new[] { sunCannon, rainGenerator, iceCannon, whirlibase })
        {
            deck.LearnKnowledge(unit.Name);
        }
        deck.AddWorkshop(1, Element.Sun);
        deck.AddWorkshop(2, Element.Rain);
        // The War Begins! 의 Sun Workshop 목록처럼 Rain Generator·Sun Cannon·Whirlibase 가 보이고 Ice Cannon 은 없다.
        Assert.Equal(["suncannon", "rainBattery", "sunaviary"],
            deck.AvailableKnowledge(1, [sunCannon, rainGenerator, iceCannon, whirlibase]).Select(u => u.Name));
        Assert.Equal(RegisterResult.WrongElement, deck.Register(1, iceCannon));
        Assert.Equal(RegisterResult.Registered, deck.Register(1, rainGenerator));
        Assert.Equal(RegisterResult.AlreadyRegistered, deck.Register(2, rainGenerator));
        Assert.Equal(RegisterResult.Registered, deck.Register(1, sunCannon));
        Assert.Equal(RegisterResult.NoFreeSlot, deck.Register(1, whirlibase));
        Assert.True(deck.UpgradeWorkshop(1));
        Assert.Equal(1, deck.FreeSlots(1));
        Assert.Equal(RegisterResult.Registered, deck.Register(1, whirlibase));
        // 파괴 → 등록 해제 → Rain Workshop 에 다시 등록 가능
        deck.RemoveWorkshop(1);
        Assert.Empty(deck.Entries());
        Assert.Equal(RegisterResult.Registered, deck.Register(2, rainGenerator));
        Assert.Equal(RegisterResult.NoWorkshop, deck.Register(1, sunCannon));
        Assert.Equal(RegisterResult.UnknownKnowledge, deck.Register(2, new ProducibleUnit("rainBlocker", Element.Rain, false)));
        // 재건한 워크샵은 빈 상태로 시작한다.
        deck.AddWorkshop(1, Element.Sun);
        Assert.Empty(deck.RegisteredAt(1));
    }

    /// <summary>섬 상태와 배치 조건 (사용자 확인 규칙)</summary>
    [Fact]
    public void Placement_IslandRules()
    {
        var ownership = new IslandOwnership();
        ownership.SetTemple(3, 1);
        ownership.SetTemple(5, 2);
        Assert.Equal(IslandState.Mine, ownership.StateFor(3, 1));
        Assert.Equal(IslandState.Others, ownership.StateFor(5, 1));
        Assert.Equal(IslandState.Empty, ownership.StateFor(4, 1));
        Assert.Equal(IslandState.NoIsland, ownership.StateFor(null, 1));
        Assert.Equal(3, ownership.TempleTerritoryOf(1));
        // 유닛: 남의 섬 불가, 빈 섬은 연결 필요, 섬 밖은 내 다리 끝만
        Assert.Equal(PlacementProblem.OthersIsland, PlacementRules.CheckUnitSite(IslandState.Others, true, true));
        Assert.Equal(PlacementProblem.NotConnected, PlacementRules.CheckUnitSite(IslandState.Empty, false, false));
        Assert.Equal(PlacementProblem.None, PlacementRules.CheckUnitSite(IslandState.Empty, true, false));
        Assert.Equal(PlacementProblem.NotOnIslandOrBridgeEnd, PlacementRules.CheckUnitSite(IslandState.NoIsland, false, false));
        Assert.Equal(PlacementProblem.None, PlacementRules.CheckUnitSite(IslandState.NoIsland, false, true));
        // 건물: 템플은 빈 섬에만·1기, 워크샵은 빈 섬도 가능, 다리 끝 불가
        Assert.Equal(PlacementProblem.TempleNeedsEmptyIsland, PlacementRules.CheckBuildingSite(ObjectKind.Temple, IslandState.Mine, false));
        Assert.Equal(PlacementProblem.TempleAlreadyExists, PlacementRules.CheckBuildingSite(ObjectKind.Temple, IslandState.Empty, true));
        Assert.Equal(PlacementProblem.None, PlacementRules.CheckBuildingSite(ObjectKind.Workshop, IslandState.Empty, true));
        Assert.Equal(PlacementProblem.BuildingNeedsIsland, PlacementRules.CheckBuildingSite(ObjectKind.Altar, IslandState.NoIsland, true));
        // 템플이 사라지면 빈 섬
        ownership.RemoveTemple(5);
        Assert.Equal(IslandState.Empty, ownership.StateFor(5, 1));
    }

    /// <summary>원본 타입 분류: 신전·워크샵·아웃포스트·알타·사제·Generator·건물형·수송·비행체·다리·주문</summary>
    [Fact]
    public void ObjectKinds_ClassifyOriginalTypes()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        var expected = new Dictionary<string, ObjectKind>
        {
            ["rainVortex"] = ObjectKind.Temple, ["sunFactory"] = ObjectKind.Workshop, ["outpost"] = ObjectKind.Outpost,
            ["Altar"] = ObjectKind.Altar, ["Dais"] = ObjectKind.Altar, ["Priest"] = ObjectKind.Priest,
            ["thunderBattery"] = ObjectKind.Generator, ["suncannon"] = ObjectKind.Emplacement, ["bulf"] = ObjectKind.Transport,
            ["windBalloon"] = ObjectKind.Transport, ["sunFlyer"] = ObjectKind.Flyer, ["bridge"] = ObjectKind.Bridge,
            ["bombHeal"] = ObjectKind.Spell, ["geyser"] = ObjectKind.Geyser, ["nugget"] = ObjectKind.Nugget,
        };
        // 각 타입의 분류가 기대와 같은지 확인한다.
        foreach ((string name, ObjectKind kind) in expected)
        {
            Assert.Equal(kind, ObjectKinds.Of(types.Find(name) ?? throw new InvalidDataException(name)));
        }
    }

    /// <summary>
    /// The War Begins! 저장 맵: 플레이어 1 의 비 신전 영역은 플레이어 1 소유이고,
    /// 신전 근처에 Bulf(Thunder 1)는 에너지 부족, Sun Cannon(아무 1)은 에너지 충족이다.
    /// </summary>
    [Fact]
    public void BattleMap_FromOriginalMission()
    {
        GameResources resources = OriginalData.RequireResources();
        TypeCatalog types = OriginalData.RequireTypes();
        var map = new FortMap(resources.LoadFort("thewarbegins", types));
        FortMapObject temple = map.Objects.Single(o => o.Object.Type.Name == "rainVortex" && o.Object.Owner == 1);
        var battle = new BattleMap(map.Objects, (x, y) => temple.Territory);
        Assert.Equal(IslandState.Mine, battle.Ownership.StateFor(temple.Territory, 1));
        Assert.Contains(battle.Sources, s => s.Element == Element.Rain && s.Owner == 1);
        // 신전 주변 20칸 안에서 Sun Cannon 을 놓을 수 있는 첫 빈 자리를 찾는다 (영역은 모두 신전 영역으로 본다).
        TypeInfo sunCannon = types.Find("suncannon")!;
        (int x, int y) = Enumerable.Range(-20, 41).SelectMany(dy => Enumerable.Range(-20, 41).Select(dx => (temple.X + dx, temple.Y + dy)))
            .First(p => battle.CheckUnit(sunCannon, p.Item1, p.Item2, 1, 10000, true, false).Allowed);
        PlacementCheck bulf = battle.CheckUnit(types.Find("bulf")!, x, y, 1, 10000, true, false);
        Assert.Equal(PlacementProblem.NotEnoughEnergy, bulf.Problem);
        Assert.Equal(PlacementProblem.NotEnoughStormPower, battle.CheckUnit(sunCannon, x, y, 1, 399, true, false).Problem);
        // 놓은 뒤에는 같은 자리가 점유된다.
        battle.PlaceUnit(sunCannon, battle.CheckUnit(sunCannon, x, y, 1, 10000, true, false), 1);
        Assert.Equal(PlacementProblem.Occupied, battle.CheckUnit(sunCannon, x, y, 1, 10000, true, false).Problem);
    }

    /// <summary>
    /// 튜토리얼 3 의 "전력선": Wind 템플 범위 밖이라 Sail Skater(Wind 1 + 아무 1)를 못 짓다가,
    /// 범위 안에 Wind Generator 를 줄지어 놓으면 멀리까지 공급이 이어진다 (매뉴얼 튜토리얼 3 설명).
    /// </summary>
    [Fact]
    public void GeneratorChain_ExtendsSupply()
    {
        TypeInfo generator = Synthetic("windBattery", "Source of Energy", "wind", 1, "s", 3, "createsisland emplacement");
        TypeInfo skater = Synthetic("windwalker", "Ground Transport", "wind", 2, null, 1, "walker");
        var battle = new BattleMap([], (x, y) => 0);
        battle.Ownership.SetTemple(0, 1);
        battle.AddSource(new EnergySource(100, Element.Wind, 0, 0, 1));
        // 템플에서 50칸 떨어진 곳은 범위 밖
        Assert.Equal(PlacementProblem.NotEnoughEnergy, battle.CheckUnit(skater, 50, 0, 1, 10000, true, false).Problem);
        // 25칸·50칸에 Generator 를 차례로 놓는다 (각 Generator 는 이전 공급원 범위 안이어야 한다 — 아무 1).
        foreach (int x in new[] { 26, 51 })
        {
            PlacementCheck check = battle.CheckUnit(generator, x, 1, 1, 10000, true, false);
            Assert.True(check.Allowed, PlacementRules.Describe(check.Problem));
            battle.PlaceUnit(generator, check, 1);
        }
        // 이제 50칸 위치를 Generator 둘(Wind 2개)이 덮는다 → Wind 1 + 아무 1 충족
        Assert.True(battle.CheckUnit(skater, 50, 4, 1, 10000, true, false).Allowed);
    }

    /// <summary>원본 파일 없이 규칙 시험용 타입을 만든다</summary>
    private static TypeInfo Synthetic(string name, string @class, string theme, int level, string? mana, int foot, string flags)
    {
        string manaLine = mana == null ? "" : $" mana = \"{mana}\";\n";
        TypeDefinition definition = TypeDefinition.Parse(
            $"typename {name}\ntypeflags {flags};\n{{\n class = \"{@class}\";\n theme = \"{theme}\";\n level = {level};\n{manaLine}" +
            $" cost = 400;\n foot_x = {foot};\n foot_y = {foot};\n}}\n");
        (uint f1, uint f2) = TypeCatalog.ComputeFlags(definition.Flags);
        return new TypeInfo(0, name, definition, f1, f2);
    }

    /// <summary>고정 틱: 24Hz 누적, 따라잡기 한도, 간격 → 틱 수 올림</summary>
    [Fact]
    public void FixedTimestep_AccumulatesTicks()
    {
        var step = new FixedTimestep();
        Assert.Equal(0, step.Advance(0.02));
        Assert.Equal(1, step.Advance(0.03));
        // 남은 0.0083초 + 1초 = 24.2틱 분량 → 24틱
        Assert.Equal(24, Enumerable.Range(0, 60).Sum(_ => step.Advance(1.0 / 60)));
        Assert.Equal(FixedTimestep.DefaultMaxCatchUpTicks, step.Advance(10));
        Assert.Equal(24, step.TicksFor(1));
        Assert.Equal(1, step.TicksFor(0.0001));
        Assert.Equal(1, step.TicksFor(1.0 / 24));
    }

    /// <summary>MSVC rand: srand(1) 뒤 처음 값들은 41, 18467, 6334, 26500 (CRT 공개 수열)</summary>
    [Fact]
    public void MsvcRandom_MatchesCrtSequence()
    {
        var random = new MsvcRandom(1);
        Assert.Equal([41, 18467, 6334, 26500, 19169], Enumerable.Range(0, 5).Select(_ => random.Next()));
        Assert.InRange(new MsvcRandom(7).Next(10), 0, 9);
    }
}
