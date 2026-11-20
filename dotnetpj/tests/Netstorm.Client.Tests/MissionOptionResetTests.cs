using System.Buffers.Binary;
using Netstorm.Core.Original;

// 단일 소비자의 기존 원본296입력으로 초기화·네 번째 공급 범위를 함께 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>옵션과 이웃의 원본 초기화 결과를 대조한다.</summary>
public sealed class MissionOptionResetTests
{
    /// <summary>이웃/예약 바이트와 기본/명시 범위·파생값의 원본296입력을 전수 비교한다.</summary>
    [Fact]
    public void ResetMatchesNativeStorage()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "mission-option-reset-x86.tsv");
        string[][] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').Select(x => x.Split('\t')).ToArray();
        Assert.Equal(296, rows.Length);
        // 기존 기대 fixture는 수정하지 않고 공급한 같은 입력에서 실제 본체를 실행한다.
        foreach (string[] row in rows)
        {
            byte[] bytes = Convert.FromHexString(row[4]);
            if (row[0] == "F") bytes[8 + 4] = byte.Parse(row[2]); else RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, bytes.AsSpan(8, 87));
            if (row[0] is "P" or "F")
            {
                var range = new FloatBox(); var squared = new FloatBox(); var options = new RawMissionBattleOptions(OriginalEdition.Patch1078, new() { Options = bytes.AsMemory(8, 87), Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(), TypeUse = new byte[256], RangeTable = new float[] { 14, 22, 30, BitConverter.UInt32BitsToSingle(uint.Parse(row[3])) }, SelectedType = () => 0, Range = range, RangeSquared = squared });
                options.UpdateRange(); byte[] raw = new byte[8]; BinaryPrimitives.WriteUInt32LittleEndian(raw, BitConverter.SingleToUInt32Bits(range.Value)); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(4), BitConverter.SingleToUInt32Bits(squared.Value)); Assert.Equal(row[6], Convert.ToHexString(raw).ToLowerInvariant());
                if (row[0] == "P") { Assert.Equal(row[7], options.TypeCost().ToString()); Assert.Equal(row[8], options.CurrentValue().ToString()); }
            }
            Assert.Equal(row[5], Convert.ToHexString(bytes).ToLowerInvariant());
        }
    }

    /// <summary>잘못된 판본/폭의 초기화는 원문 전체를 쓰기 전에 거부한다.</summary>
    [Fact]
    public void InvalidInputDoesNotMutateStorage()
    {
        byte[] bytes = Enumerable.Repeat((byte)0x5a, 87).ToArray();
        Assert.Throws<ArgumentException>(() => RawMissionBattleOptions.Reset(OriginalEdition.Cd1072, bytes));
        Assert.Throws<ArgumentException>(() => RawMissionBattleOptions.Reset(OriginalEdition.Patch1078, bytes.AsSpan(0, 86)));
        Assert.All(bytes, value => Assert.Equal(0x5a, value));
    }
}
