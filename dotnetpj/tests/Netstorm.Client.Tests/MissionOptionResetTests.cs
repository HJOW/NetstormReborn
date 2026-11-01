using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>옵션과 이웃의 원본 초기화 결과를 대조한다.</summary>
public sealed class MissionOptionResetTests
{
    [Fact]
    public void ResetMatchesNativeStorage()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "mission-option-reset-x86.tsv");
        string[][] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').Select(x => x.Split('\t')).Where(x => x[0] is "R" or "P").ToArray();
        Assert.Equal(264, rows.Length);
        foreach (string[] row in rows)
        {
            byte[] bytes = Convert.FromHexString(row[4]);
            RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, bytes.AsSpan(8, 87));
            Assert.Equal(row[5], Convert.ToHexString(bytes).ToLowerInvariant());
        }
    }

    [Fact]
    public void InvalidInputDoesNotMutateStorage()
    {
        byte[] bytes = Enumerable.Repeat((byte)0x5a, 87).ToArray();
        Assert.Throws<ArgumentException>(() => RawMissionBattleOptions.Reset(OriginalEdition.Cd1072, bytes));
        Assert.Throws<ArgumentException>(() => RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, bytes.AsSpan(0, 86)));
        Assert.All(bytes, value => Assert.Equal(0x5a, value));
    }
}
