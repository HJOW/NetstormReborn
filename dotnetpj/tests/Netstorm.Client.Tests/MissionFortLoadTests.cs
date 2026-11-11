using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>호스트 미션 요새 적재가 실제 자료를 이식된 판독기로 푸는지 검사한다.
/// 기대값은 cpppj `--inspect-fort`의 thewarbegins 출력과 같다(0.390 베이스라인).</summary>
public sealed class MissionFortLoadTests
{
    /// <summary>타입 번호별 빈 칸 제외 개수를 센다.</summary>
    private static Dictionary<string, int> CountByType(FortChunkSection section, RiftTypeTable types)
    {
        var counts = new Dictionary<string, int>(StringComparer.Ordinal);
        // 모든 청크의 칸을 순서대로 세며 빈 레코드(타입 0)는 제외한다.
        foreach (Netstorm.Core.Original.FortChunk chunk in section.Chunks)
        {
            foreach (Netstorm.Core.Original.FortObject record in chunk.Objects)
            {
                if (record.Type == 0) continue;
                string name = types.Types[record.Type].Name;
                counts.TryGetValue(name, out int count);
                counts[name] = count + 1;
            }
        }
        return counts;
    }

    /// <summary>thewarbegins 요새가 cpppj inspect-fort와 같은 목록을 내놓는지 확인한다.</summary>
    [Fact]
    public void LoadsTheWarBeginsFortLikeInspectFort()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        MissionFortLoadResult loaded = MissionFortLoad.Load(startup, "thewarbegins");
        Assert.Equal("thewarbegins", loaded.Mission);
        Assert.Equal("thewarbegins", loaded.FortName);
        Assert.EndsWith("thewarbegins.fort", loaded.FortPath.Replace('\\', '/'), StringComparison.OrdinalIgnoreCase);
        Assert.Equal("Tutorial", loaded.MissionType);
        FortTemplate fort = loaded.Fort;
        Assert.Equal(35, fort.Sections.Count);
        Assert.Equal(0, fort.Flag);
        Assert.Equal(56, fort.TrailingBytes);
        Assert.NotNull(fort.Subscriber);
        Assert.Equal(66888756U, fort.Subscriber.Id);
        Assert.Equal("Player", fort.Subscriber.Name);
        Assert.Equal(100000f, fort.Money);
        Assert.Equal(171, fort.TypeHashes.Count);
        Assert.Equal(26, fort.Technology.Items.Count);
        Assert.Equal(38, fort.Deck.Count);
        RiftTypeTable types = MissionFortLoad.LoadTypeTable(startup.Files);
        Assert.Equal(188, types.Types.Count);
        Assert.Equal(2, fort.Chaff.Version);
        Dictionary<string, int> chaff = CountByType(fort.Chaff, types);
        Assert.Equal(145, chaff.Values.Sum());
        Assert.Equal(15, chaff["bridge"]);
        Assert.Equal(13, chaff["geyser"]);
        Assert.Equal(117, chaff["noIsland"]);
        Dictionary<string, int> terr00 = CountByType(fort.Territories[0], types);
        Assert.Equal(new Dictionary<string, int>(StringComparer.Ordinal)
        { ["geyser"] = 1, ["priest"] = 1, ["rainVortex"] = 1, ["residence"] = 1 }, terr00);
        Dictionary<string, int> terr01 = CountByType(fort.Territories[1], types);
        Assert.Equal(new Dictionary<string, int>(StringComparer.Ordinal)
        { ["priest"] = 1, ["residence"] = 1, ["sunArcher"] = 9, ["sunBlocker"] = 8, ["windBattery"] = 3, ["windVortex"] = 1 }, terr01);
    }

    /// <summary>없는 미션은 명시 오류로 알린다.</summary>
    [Fact]
    public void MissingMissionThrows()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        Assert.Throws<InvalidDataException>(() => MissionFortLoad.Load(startup, "no-such-mission-xyz"));
    }
}
