using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>건설 에너지 요구값과 공급 판정 테스트 (docs/gameplay/elements-energy.md, docs/exe/energy-requirements.md)</summary>
public sealed class EnergyRuleTests
{
    /// <summary>일반 미션의 공급 반지름² (30칸)</summary>
    private const double Radius30Squared = 900;

    /// <summary>원본 FUN_0049b0d0 기본 요구값 표: 원소 L1 = 원소 1, L2 = 원소 + Sun, L3 = 원소 2 + Sun, Sun = Sun × 레벨</summary>
    [Theory]
    [InlineData(Element.Sun, 1, "s")]
    [InlineData(Element.Sun, 2, "ss")]
    [InlineData(Element.Sun, 3, "sss")]
    [InlineData(Element.Thunder, 1, "t")]
    [InlineData(Element.Thunder, 2, "ts")]
    [InlineData(Element.Thunder, 3, "tts")]
    [InlineData(Element.Rain, 1, "r")]
    [InlineData(Element.Wind, 3, "wws")]
    [InlineData(Element.Rain, 0, "")]
    [InlineData(Element.Rain, 4, "")]
    public void DefaultRequirement_FollowsExeTable(Element element, int level, string expected)
    {
        Assert.Equal(expected, EnergyRequirement.Default(element, level).Letters);
    }

    /// <summary>.type 에 mana 가 있으면 그 문자열을 먼저 쓴다 (Generator 는 level 1 이어도 "s")</summary>
    [Fact]
    public void ForType_ExplicitManaWins()
    {
        TypeDefinition generator = TypeDefinition.Parse("typename g\n{\n theme = \"thunder\";\n level = 1;\n mana = \"s\";\n}\n");
        TypeDefinition bulf = TypeDefinition.Parse("typename b\n{\n theme = \"thunder\";\n level = 1;\n}\n");
        TypeDefinition temple = TypeDefinition.Parse("typename v\n{\n theme = \"rain\";\n cost = 5000;\n}\n");
        Assert.Equal("s", EnergyRequirement.ForType(generator).Letters);
        Assert.Equal("t", EnergyRequirement.ForType(bulf).Letters);
        Assert.Equal(0, EnergyRequirement.ForType(temple).Count);
    }

    /// <summary>요구 문자열 해석: 대문자·잡글자 처리와 설명 문구</summary>
    [Fact]
    public void Parse_AndDescribe()
    {
        EnergyRequirement r = EnergyRequirement.Parse("TTs?");
        Assert.Equal("tts", r.Letters);
        Assert.Equal(2, r.CountOf(Element.Thunder));
        Assert.Equal("Thunder 2 + 아무 1", r.Describe());
        Assert.Equal("없음", EnergyRequirement.Parse(null).Describe());
    }

    /// <summary>
    /// 사용자 확인(2026-09-29): Bulf(Thunder 1)는 다른 원소 공급원으로는 지을 수 없고,
    /// Thunder Generator 또는 Thunder Temple 이 하나 이상 있어야 한다.
    /// </summary>
    [Fact]
    public void Bulf_NeedsThunderSource()
    {
        EnergyRequirement bulf = EnergyRequirement.Parse("t");
        var rainAndWind = new[] { Source(1, Element.Rain), Source(2, Element.Wind) };
        Assert.False(EnergySupply.Match(bulf, rainAndWind).Satisfied);
        Assert.True(EnergySupply.Match(bulf, [Source(3, Element.Thunder)]).Satisfied);
    }

    /// <summary>Sun Cannon(아무 1)은 Rain Generator 하나로 지을 수 있다 (사용자 확인 예)</summary>
    [Fact]
    public void SunUnit_AcceptsAnyElement()
    {
        Assert.True(EnergySupply.Match(EnergyRequirement.Parse("s"), [Source(1, Element.Rain)]).Satisfied);
    }

    /// <summary>
    /// Vander Tower(Thunder 2 + 아무 1): Thunder Temple + Thunder Generator + Rain Generator 는 가능,
    /// Thunder Generator 1 + Rain Generator 2 는 Thunder 가 1개뿐이라 불가 (문서의 판정 예).
    /// </summary>
    [Fact]
    public void VanderTower_NeedsDistinctSources()
    {
        EnergyRequirement vander = EnergyRequirement.Parse("tts");
        EnergyCheck ok = EnergySupply.Match(vander, [Source(1, Element.Thunder), Source(2, Element.Thunder), Source(3, Element.Rain)]);
        Assert.True(ok.Satisfied);
        Assert.Equal([1, 2, 3], ok.Assigned.Select(s => s!.Id));
        Assert.False(EnergySupply.Match(vander, [Source(1, Element.Thunder), Source(2, Element.Rain), Source(3, Element.Rain)]).Satisfied);
        // 공급원 하나를 두 글자에 쓰지 않는다: Thunder 공급원 둘만으로는 세 개를 못 채운다.
        Assert.False(EnergySupply.Match(vander, [Source(1, Element.Thunder), Source(2, Element.Thunder)]).Satisfied);
    }

    /// <summary>Sun 글자가 앞에 와도 원소 글자를 먼저 배정해 결과가 나빠지지 않는다</summary>
    [Fact]
    public void Match_AssignsElementLettersBeforeSun()
    {
        EnergyCheck check = EnergySupply.Match(EnergyRequirement.Parse("st"), [Source(1, Element.Thunder), Source(2, Element.Rain)]);
        Assert.True(check.Satisfied);
        Assert.Equal(2, check.Assigned[0]!.Id);
        Assert.Equal(1, check.Assigned[1]!.Id);
    }

    /// <summary>공급 범위: 중심 거리² ≤ 반지름² 이면 포함 (30칸 경계)</summary>
    [Fact]
    public void Covering_UsesSquaredDistance()
    {
        var sources = new[]
        {
            new EnergySource(1, Element.Rain, 30, 0, 1),   // 거리 30 → 포함
            new EnergySource(2, Element.Rain, 18, 24, 1),  // 거리 30 (18² + 24² = 900) → 포함
            new EnergySource(3, Element.Rain, 30.1, 0, 1), // 30 초과 → 제외
        };
        IReadOnlyList<EnergySource> covering = EnergySupply.Covering(sources, 0, 0, Radius30Squared, _ => true);
        Assert.Equal([1, 2], covering.Select(s => s.Id));
    }

    /// <summary>적 소유 공급원은 쓰지 않고, 동맹 공급원은 쓴다</summary>
    [Fact]
    public void Covering_FiltersByFriendship()
    {
        var sources = new[] { new EnergySource(1, Element.Thunder, 0, 0, 2), new EnergySource(2, Element.Thunder, 1, 0, 3) };
        Assert.Empty(EnergySupply.Covering(sources, 0, 0, Radius30Squared, owner => owner == 1));
        Assert.Single(EnergySupply.Covering(sources, 0, 0, Radius30Squared, owner => owner is 1 or 3));
    }

    /// <summary>튜토리얼 2 의 14칸 범위에서는 20칸 떨어진 공급원을 쓸 수 없다</summary>
    [Fact]
    public void Check_UsesBattleOptionRadius()
    {
        var options = new BattleOptions();
        var source = new EnergySource(1, Element.Wind, 20, 10, 1);
        var target = new Footprint(0, 10, 1, 1);
        Assert.True(EnergySupply.Check(EnergyRequirement.Parse("s"), [source], target, options.GeneratorRadiusSquared, _ => true).Satisfied);
        options.ApplyTutorialTwoOverrides();
        Assert.False(EnergySupply.Check(EnergyRequirement.Parse("s"), [source], target, options.GeneratorRadiusSquared, _ => true).Satisfied);
    }

    /// <summary>
    /// 원본 .type 전체에 규칙을 적용한 요구값이 문서 표(elements-energy.md 4절)와 같다.
    /// 신전·워크샵·알타·사제는 요구값이 없다.
    /// </summary>
    [Fact]
    public void OriginalTypes_MatchDocumentedTable()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        var expected = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["sunArcher"] = "s", ["sunBlocker"] = "s", ["suncannon"] = "s", ["sunFence"] = "s",
            ["sunBalloon"] = "ss", ["sunaviary"] = "ss", ["sunwalker"] = "s",
            ["rainBattery"] = "s", ["rainwalker"] = "r", ["raincannon"] = "rs", ["rainFence"] = "rs",
            ["rainBlocker"] = "rs", ["rainaviary"] = "rrs", ["rainBalloon"] = "rrs",
            ["windBattery"] = "s", ["windwalker"] = "ws", ["windArcher"] = "ws", ["windBlocker"] = "ws",
            ["windaviary"] = "wws", ["windBalloon"] = "wws",
            ["thunderBattery"] = "s", ["bulf"] = "t", ["thunderFence"] = "t", ["thunderBlocker"] = "ts",
            ["thundercannon"] = "ts", ["thunderArcher"] = "tts", ["outpost"] = "s",
            ["rainVortex"] = "", ["windVortex"] = "", ["thunderVortex"] = "", ["sunFactory"] = "", ["rainFactory"] = "",
            ["Priest"] = "",
        };
        // 표의 각 타입이 로딩 목록에 있고 요구값이 일치하는지 확인한다.
        foreach ((string name, string letters) in expected)
        {
            TypeInfo type = types.Find(name) ?? throw new InvalidDataException($"타입 없음: {name}");
            Assert.Equal(letters, EnergyRequirement.ForType(type.Definition).Letters);
        }
    }

    /// <summary>시험용 공급원 (위치는 판정에 쓰지 않는 Match 전용)</summary>
    private static EnergySource Source(int id, Element element) => new(id, element, 0, 0, 1);
}
