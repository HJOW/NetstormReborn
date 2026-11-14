using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 기대값은 실제 PE의 기본027f 관찰을 읽으며 C# 결과로 재생성하지 않는다.
namespace Netstorm.Core.Tests;

/// <summary>48색 띠의256개 의미항목과72병합·실제 생성자6팔레트를 전체비트로 대조한다.</summary>
public sealed class ParticleGradientOracleTests
{
    /// <summary>실제 RGBX 입력의 DWORD 순서와 예약BYTE를 그대로 복구한다.</summary>
    private static uint[] Palette(string text)
    {
        byte[] raw = X86Fixture.Bytes(text); Assert.Equal(1024, raw.Length); var palette = new uint[256];
        // 각 원본DWORD는 임의 RGB 재변환 없이 읽는다.
        for (int index = 0; index < palette.Length; index++) palette[index] = BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(index * 4));
        return palette;
    }
    /// <summary>fixture의 입력binary64·요청색수·폭만 실제 제품 함수에 공급한다.</summary>
    private static ParticleGradient Make(uint[] palette, string[] row, int column)
    {
        double[] rgb = row[column].Split(',').Select(X86Fixture.DoubleBits).ToArray(); Assert.Equal(6, rgb.Length);
        return ParticleGradients.Build(palette, rgb[..3], rgb[3..], X86Fixture.Int(row[column + 1]), X86Fixture.DoubleBits(row[column + 2]));
    }
    /// <summary>원문48바이트의 의미필드만 직렬화하며 HSV/패딩을 기대값에 끼워 넣지 않는다.</summary>
    private static byte[] Bytes(ParticleGradient actual)
    {
        var raw = new byte[256 * 48];
        // 색 번호/거리/투영/선택을 포함하는 전체 항목 순서와 모든double비트를 보존한다.
        for (int index = 0; index < actual.Entries.Length; index++)
        {
            ParticleGradientEntry entry = actual.Entries[index]; Span<byte> target = raw.AsSpan(index * 48, 48);
            BinaryPrimitives.WriteDoubleLittleEndian(target, entry.Translated[0]); BinaryPrimitives.WriteDoubleLittleEndian(target[8..], entry.Translated[1]);
            BinaryPrimitives.WriteDoubleLittleEndian(target[16..], entry.Translated[2]); BinaryPrimitives.WriteUInt32LittleEndian(target[24..], entry.Index);
            BinaryPrimitives.WriteDoubleLittleEndian(target[28..], entry.Distance); BinaryPrimitives.WriteDoubleLittleEndian(target[36..], entry.Position);
            BinaryPrimitives.WriteUInt32LittleEndian(target[44..], entry.Selected);
        }
        return raw;
    }
    /// <summary>긴 hex를 실패 출력에 반복하지 않고 첫 의미바이트 차이를 함께 보고한다.</summary>
    private static void EqualBytes(string context, string expected, byte[] actual)
    {
        byte[] bytes = X86Fixture.Bytes(expected); Assert.Equal(bytes.Length, actual.Length);
        int first = Array.FindIndex(Enumerable.Range(0, actual.Length).ToArray(), i => bytes[i] != actual[i]);
        Assert.True(first < 0, $"{context} 첫 바이트 차이 {first}");
    }
    /// <summary>실제 PE의 머리4값과 전체256항목을 허용오차 없이 확인한다.</summary>
    private static void Check(ParticleGradient actual, string header, string payload)
    {
        string[] values = header.Split(','); Assert.Equal(4, values.Length);
        Assert.Equal(X86Fixture.Int(values[0]), actual.From); Assert.Equal(X86Fixture.Int(values[1]), actual.To);
        Assert.Equal(X86Fixture.Int(values[2]), actual.Selected); Assert.Equal(X86Fixture.ULong(values[3]), BitConverter.DoubleToUInt64Bits(actual.Threshold));
        EqualBytes("색 띠 의미필드", payload, Bytes(actual));
    }
    /// <summary>색수만큼 DWORD를 쓰며 인접 할당 메모리를 합성하지 않는다.</summary>
    private static byte[] ColorBytes(uint[] colors)
    {
        var bytes = new byte[colors.Length * 4];
        // 요청색 순서대로 원문little endian을 기록한다.
        for (int index = 0; index < colors.Length; index++) BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(index * 4), colors[index]);
        return bytes;
    }
    /// <summary>기본53비트24입력/48색띠·1/2/3색띠72병합을 대조한다.037f는 완료로 세지 않는다.</summary>
    [Fact]
    public void DefaultPrecisionMatchesEveryOriginalFieldAndMerge()
    {
        string[][] all = X86Fixture.Read("particle-gradient"); Assert.Equal(24, all.Count(row => X86Fixture.UInt(row[1]) == 0x37f));
        string[][] rows = all.Where(row => X86Fixture.UInt(row[1]) == 0x27f).ToArray(); Assert.Equal(24, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(16, row.Length); uint[] palette = Palette(row[2]), before = palette.ToArray();
            ParticleGradient a = Make(palette, row, 3), b = Make(palette, row, 6);
            Assert.Equal(X86Fixture.Int(row[4]), a.Count); Assert.Equal(X86Fixture.Int(row[7]), b.Count);
            Check(a, row[9], row[10]); Check(b, row[11], row[12]);
            EqualBytes("2색 띠 병합", row[13], ColorBytes(ParticleGradients.Merge(a, b)));
            EqualBytes("3색 띠 병합", row[14], ColorBytes(ParticleGradients.Merge(a, b, a)));
            EqualBytes("1색 띠 병합", row[15], ColorBytes(ParticleGradients.Merge(a)));
            Assert.Equal(before, palette); return true;
        });
    }
    /// <summary>실제 두 생성자의 기본cold입력 각3COL 팔레트 전체11색을 비교한다.</summary>
    [Theory]
    [InlineData("fierytail-lifetime", ".COL,0,0,-1,50,127,0,1", 6)]
    [InlineData("lightning-lifetime", ".COL,0,0,-1,50,127,0,0,1", 7)]
    public void ConstructorPaletteConstantsMatchOriginal(string name, string arguments, int expected)
    {
        string[][] rows = X86Fixture.Read(name).Where(row => X86Fixture.UInt(row[1]) == 0x27f && row[2].Contains(arguments, StringComparison.Ordinal)).ToArray();
        Assert.Equal(3, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            uint[] palette = Palette(row[3]); uint[] actual = name == "fierytail-lifetime" ? ParticleGradients.FieryTail(palette) : ParticleGradients.Lightning(palette);
            Assert.Equal(11, actual.Length); EqualBytes(name, row[expected], ColorBytes(actual)); return true;
        });
    }
}
