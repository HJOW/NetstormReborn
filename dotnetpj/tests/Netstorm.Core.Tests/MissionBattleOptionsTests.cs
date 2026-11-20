using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 현재 옵션 파생값의 원본3305입력을 실제 TYPE/사용표/float 주소로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>전체 옵션·비용·사용표·x87 특수float와 현재 저장소 변경을 검증한다.</summary>
public sealed class MissionBattleOptionsTests
{
    /// <summary>실제float32 비트 두 개를 원문 순서로 표현한다.</summary>
    private static string Range(FloatBox range, FloatBox squared)
    {
        byte[] bytes = new byte[8]; BinaryPrimitives.WriteUInt32LittleEndian(bytes, BitConverter.SingleToUInt32Bits(range.Value)); BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(4), BitConverter.SingleToUInt32Bits(squared.Value)); return Hex(bytes);
    }
    /// <summary>원문 전체 바이트를 fixture 형식으로 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>기존 각TYPE 비용을 독립 명시 유한 입력으로 준비한다.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, 188).Select(index => new OriginalTypeRecord { Cost = 100.25f + index }).ToArray();
    /// <summary>원본3305입력의 결과·float8바이트·현재값·188비용·이웃 포함 사용표·87옵션을 전수 비교한다.</summary>
    [Fact]
    public void OriginalBattleOptions3305()
    {
        string[][] rows = X86Fixture.Read("mission-battle-options").Where(row => row[0] != "input").ToArray(); Assert.Equal(3305, rows.Length); var seen = new HashSet<string>();
        // 독립 원본에 공급한 같은 입력만 구성하며 출력은 실제 본체에 맡긴다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0])); string[] input = row[0].Split(','); uint[] number = input[1..].Select(X86Fixture.UInt).ToArray(); uint selected = number[3], pattern = number[4]; byte[] bytes = new byte[87], use = new byte[264];
            // 주변 stale 옵션과 canary 사용표의 명시 입력이다.
            for (int index = 0; index < bytes.Length; index++) bytes[index] = unchecked((byte)(index * 11 + pattern));
            // 양쪽 이웃4바이트도 보존 비교에 포함한다.
            for (int index = 0; index < use.Length; index++) use[index] = unchecked((byte)(index * 7 + pattern));
            bytes[4] = (byte)number[0]; bytes[0x10] = (byte)number[1]; bytes[0xf] = (byte)number[2]; float[] choices = number[5..8].Select(BitConverter.UInt32BitsToSingle).ToArray(); OriginalTypeRecord[] types = Types();
            var range = new FloatBox { Value = BitConverter.UInt32BitsToSingle(0x418c0000) }; var squared = new FloatBox { Value = BitConverter.UInt32BitsToSingle(0x43992000) }; var value = new UIntBox { Value = 0x12345678 };
            var options = new RawMissionBattleOptions(OriginalEdition.Patch1078, new() { Options = bytes, Types = types, TypeUse = use.AsMemory(4, 256), RangeTable = choices, SelectedType = () => selected, Range = range, RangeSquared = squared, Value = value }); uint result = 0;
            switch (input[0]) { case "Range": options.UpdateRange(); break; case "Cost": result = (uint)options.TypeCost(); break; case "Value": result = options.CurrentValue(); break; case "Clear": options.ClearTypes(); break; case "Apply": options.ApplyDerived(); break; default: Assert.Fail("알 수 없는 원본 옵션 입력"); break; }
            Assert.Equal(row[1], result.ToString()); Assert.Equal(row[2], Range(range, squared)); Assert.Equal(row[3], value.Value.ToString()); byte[] costs = new byte[188 * 4];
            // 선택 대상 밖188비용의 실제 보존도 원문 비트로 비교한다.
            for (int index = 0; index < types.Length; index++) BinaryPrimitives.WriteUInt32LittleEndian(costs.AsSpan(index * 4), BitConverter.SingleToUInt32Bits(types[index].Cost));
            Assert.Equal(row[4], input[0] == "Apply" ? Hex(costs) : "-"); Assert.Equal(row[5], input[0] == "Clear" ? Hex(use) : "-"); Assert.Equal(row[6], Hex(bytes));
        }
    }

    /// <summary>현재 선택/옵션/표 변경과 signaling NaN quiet 쓰기·거부 전 부분효과를 구별한다.</summary>
    [Fact]
    public void CurrentStorageAndSignalingNanPreserveOriginalOrder()
    {
        byte[] bytes = new byte[87], use = new byte[256]; OriginalTypeRecord[] types = Types(); float[] choices = [14, 22, 30]; uint selected = 1; var range = new FloatBox(); var squared = new FloatBox(); var value = new UIntBox();
        var state = new MissionBattleOptionState { Options = bytes, Types = types, TypeUse = use, RangeTable = choices, SelectedType = () => selected, Range = range, RangeSquared = squared, Value = value }; var options = new RawMissionBattleOptions(OriginalEdition.Patch1078, state);
        options.ApplyDerived(); Assert.Equal(1000f, types[1].Cost); bytes[4] = 1; bytes[0x10] = 3; bytes[0xf] = 5; selected = 187; choices[1] = 18; options.ApplyDerived(); Assert.Equal(18f, range.Value); Assert.Equal(324f, squared.Value); Assert.Equal(5000f, types[187].Cost); Assert.Equal(1000f, types[1].Cost); Assert.Equal(150u, value.Value);
        // 실제 x87 FLD/FSTP의 양쪽 signaling NaN 원시 비트를 보존한다.
        foreach (uint bits in new uint[] { 0x7f800001, 0xff800001 }) { choices[1] = BitConverter.UInt32BitsToSingle(bits); options.UpdateRange(); Assert.Equal(bits | 0x00400000, BitConverter.SingleToUInt32Bits(range.Value)); Assert.Equal(bits | 0x00400000, BitConverter.SingleToUInt32Bits(squared.Value)); }
        Assert.Throws<ArgumentException>(() => new RawMissionBattleOptions(OriginalEdition.Cd1072, state)); Assert.Throws<ArgumentException>(() => new RawMissionBattleOptions(OriginalEdition.Patch1078, state with { TypeUse = new byte[255] }));
        bytes[4] = 3; Assert.Throws<ArgumentOutOfRangeException>(options.UpdateRange); selected = 188; Assert.Throws<ArgumentOutOfRangeException>(() => options.SetSelectedTypeCost(1));
        bytes[4] = 0; bytes[0x10] = 4; Assert.Throws<ArgumentOutOfRangeException>(() => options.TypeCost()); bytes[0xf] = 6; Assert.Throws<ArgumentOutOfRangeException>(() => options.CurrentValue());
    }

    /// <summary>CD는 현재8바이트만 초기화하고 잘못된 크기는 원문/이웃 전체를 보존한다.</summary>
    [Fact]
    public void CdResetEightByteInputPreservesBothCanaries()
    {
        byte[] bytes = new byte[16]; Array.Fill(bytes, (byte)0xa5); RawMissionBattleOptions.Reset(OriginalEdition.Cd1072, bytes.AsSpan(4, 8)); Assert.Equal(new byte[] { 0xa5, 0xa5, 0xa5, 0xa5, 1, 2, 0, 1, 1, 0, 1, 0, 0xa5, 0xa5, 0xa5, 0xa5 }, bytes);
        byte[] before = (byte[])bytes.Clone(); Assert.Throws<ArgumentException>(() => RawMissionBattleOptions.Reset(OriginalEdition.Cd1072, bytes.AsSpan(4, 7))); Assert.Equal(before, bytes);
    }
}
