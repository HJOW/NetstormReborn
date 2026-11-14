using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

// TYPE에너지의실제후처리와보호자산연결을검사한다.
namespace Netstorm.Tests;

/// <summary>cpppj/tests/TypeRequiredEnergyTests.cpp의원본2352입력·실제188타입·AST경계대조다.</summary>
public sealed class TypeRequiredEnergyTests
{
    /// <summary>원본기계어관측의머리글을제외한모든입력/출력을읽는다.</summary>
    private static string[][] Observations() => X86Fixture.Read("type-required-energy").Where(row => row[0] != "group").ToArray();
    /// <summary>현재실제TYPE입력에대응하는독립원본관측의문자열만읽는다.</summary>
    private static string Expected(string[][] rows, OriginalTypeRecord type, string mana)
    {
        string[] row = Assert.Single(rows, row => X86Fixture.Int(row[0]) == (type.Group == 10 ? 10 : 0)
            && X86Fixture.UInt(row[1]) == (type.Flags2 & 0x200000u) && X86Fixture.Int(row[2]) == type.Level
            && X86Fixture.Int(row[3]) == type.Theme && row[4] == mana);
        return Encoding.Latin1.GetString(Convert.FromHexString(row[5]));
    }

    /// <summary>음수/98레벨·문자표끝NUL·제외그룹/플래그·명시mana의원본2352입력을전수대조한다.</summary>
    [Fact]
    public void Native2352()
    {
        string[][] rows = Observations(); Assert.Equal(2352, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var type = new OriginalTypeRecord { Group = X86Fixture.Int(row[0]), Flags2 = X86Fixture.UInt(row[1]), Level = X86Fixture.Int(row[2]), Theme = X86Fixture.Int(row[3]) };
            var definition = new RiftTypeDefinition(); definition.Properties.Add(new TypeProperty("mana", row[4]));
            return RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, definition)
                == Encoding.Latin1.GetString(Convert.FromHexString(row[5]));
        });
    }

    /// <summary>실제116개AST·내부/빈타입을같은188TYPEgetter에연결하고각입력의독립원본결과를비교한다.</summary>
    [Fact]
    public void Protected188Bindings()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        RiftTypeTable table = MissionFortLoad.LoadTypeTable(startup.Files);
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        Func<uint, string> getter = AiTypeRequiredEnergy.BindAiTypeRequiredEnergy(table.Types, definitions);
        string[][] rows = Observations();
        // 실제자산70..185와내부0..69·빈186/187을현재TYPE입력에대응하는원본관측으로대조한다.
        for (int number = 0; number < table.Types.Count; number++)
        {
            string mana = number >= 70 && number < 186 ? definitions[number - 70].String("mana") ?? "" : "";
            Assert.Equal(Expected(rows, table.Types[number], mana), getter((uint)number));
        }
        table.Types[69].Group = 0; table.Types[69].Flags2 = 0; table.Types[69].Level = 2; table.Types[69].Theme = 0;
        Assert.Equal(Expected(rows, table.Types[69], ""), getter(69));
        Assert.Throws<ArgumentOutOfRangeException>(() => getter(188));
        Assert.Throws<ArgumentException>(() => AiTypeRequiredEnergy.BindAiTypeRequiredEnergy(table.Types.Take(187).ToArray(), definitions));
    }

    /// <summary>순서있는중복지정·숫자AST·9바이트범위·비ASCII·판본/문자표범위를별도로검증한다.</summary>
    [Fact]
    public void BoundariesAndOrderedAssignments()
    {
        var type = new OriginalTypeRecord { Group = 0, Level = 1, Theme = 3 };
        var definition = RiftTypeDefinition.Parse("typename probe { mana = \"S\"; MANA = \"WRTS\"; }");
        Assert.Equal("wrts", RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, definition));
        var invalid = RiftTypeDefinition.Parse("typename probe { mana = 1; mana = \"s\"; }");
        Assert.Throws<ArgumentException>(() => RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, invalid));
        invalid.Properties.Clear(); invalid.Properties.Add(new TypeProperty("mana", "0123456789"));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, invalid));
        invalid.Properties.Clear(); invalid.Properties.Add(new TypeProperty("mana", "\u00c3\u00a9"));
        Assert.Throws<ArgumentException>(() => RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, invalid));
        Assert.Throws<ArgumentException>(() => RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Cd1072, type, definition));
        type.Theme = 12;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, null));
        type.Theme = -1;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawTypeRequiredEnergy.TypeRequiredEnergy(OriginalEdition.Patch1078, type, null));
    }
}
