using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>지식 창 카드 격자 규칙 테스트 (원본 플레이 녹화 화면과 대조)</summary>
public sealed class KnowledgeCatalogTests
{
    /// <summary>
    /// The War Begins! 의 .fort Technology 지식으로 만든 격자가 2026-09-30 녹화(00:14·09:26)의 25장과 행·순서·이름이 같다.
    /// </summary>
    [Fact]
    public void TheWarBegins_MatchesRecordedGrid()
    {
        GameResources resources = OriginalData.RequireResources();
        TypeCatalog types = OriginalData.RequireTypes();
        FortFile fort = resources.LoadFort("thewarbegins", types);
        IReadOnlyList<string> known = KnowledgeCatalog.KnownFromFort(fort);
        Assert.Equal(26, known.Count);

        IReadOnlyList<KnowledgeRow> rows = KnowledgeCatalog.Rows(types, known);
        Assert.Equal([Element.Sun, Element.Wind, Element.Rain, Element.Thunder], rows.Select(r => r.Element));
        string[][] titles = [.. rows.Select(r => r.Cards.Select(c => c.Title).ToArray())];
        Assert.Equal(["Sun Cannon", "Sun Disc Thrower", "Stone Tower", "Sun Barricade", "Whirlibase", "Balloon"], titles[0]);
        Assert.Equal(["Wind Generator", "Crossbow", "Sail Skater", "Wind Tower", "Devil Maker", "Air Ship"], titles[1]);
        Assert.Equal(["Rain Generator", "Ice Cannon", "Crystal Crab", "Ice Tower", "Acid Barricade", "Man o'War Pool", "Cloud Floater"], titles[2]);
        Assert.Equal(["Thunder Generator", "Thunder Cannon", "Vander Tower", "Bulf", "Bulwark", "Arc Spire"], titles[3]);
    }

    /// <summary>골렘은 알아도 카드가 없고, 카드가 없는 원소도 행은 남는다</summary>
    [Fact]
    public void Rows_ExcludeGolemAndKeepEmptyRows()
    {
        TypeCatalog types = OriginalData.RequireTypes();
        IReadOnlyList<KnowledgeRow> rows = KnowledgeCatalog.Rows(types, ["sunWalker", "RAINBATTERY", "sunCannon"]);
        Assert.Equal(4, rows.Count);
        Assert.Equal(["sunCannon"], rows[0].Cards.Select(c => c.Type.Name));
        Assert.Empty(rows[1].Cards);
        Assert.Equal(["rainBattery"], rows[2].Cards.Select(c => c.Type.Name));
        Assert.Empty(rows[3].Cards);
    }
}
