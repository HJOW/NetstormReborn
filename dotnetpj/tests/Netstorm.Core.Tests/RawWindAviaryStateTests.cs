using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;

// 데빌메이커 실제80BYTE와 독립 PE의1894관측 및 실패 전 쓰기 순서를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>RawWindAviaryState.cpp의 원문 전체 이미지/패킷/부모/길이 관측이다.</summary>
public sealed class RawWindAviaryStateTests
{
    /// <summary>독립 입력의 DWORD 십진수를 그대로 읽는다.</summary>
    private static uint N(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>실제 DWORD 폭만 기록한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);
    /// <summary>불투명64비트를 계산 없이 기록한다.</summary>
    private static void Bits(Span<byte> bytes, int offset, ulong value) => BinaryPrimitives.WriteUInt64LittleEndian(bytes[offset..], value);
    /// <summary>원문 전체바이트를 소문자 hex로 표시한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>생성18/전체writer1108/부분reader768의 독립 전체 관측을 각각 대조한다.</summary>
    [Theory]
    [InlineData("C", 18)] [InlineData("W", 1108)] [InlineData("R", 768)]
    public void StateMatchesAllIndependentPeObservations(string kind, int expected)
    {
        int count = 0;
        // 독립자료의 해당범위만 입력으로 재생하며 기대값은 원문 그대로 유지한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86/wind-aviary-state-x86.tsv")))
        {
            if (!line.StartsWith(kind + "\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(6, row.Length); uint[] input = row[1].Split(',').Select(N).ToArray(); var fields = new WindAviaryProcessFields(0xa5);
            Bits(fields.Bytes, 16, BitConverter.DoubleToUInt64Bits(123.25)); Put(fields.Bytes, 24, 7); Bits(fields.Bytes, 32, 0x7ff0000000000001); Bits(fields.Bytes, 40, 0x8000000000000000); Put(fields.Bytes, 48, 0xfedcba98); Put(fields.Bytes, 60, 77); Put(fields.Bytes, 64, 50);
            byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); uint parent = 0xdadadada; ushort result = 0;
            if (kind == "C") { Array.Fill(fields.Bytes, (byte)input[2]); RawWindAviaryState.Initialize(fields, input[0] != 0, new[] { 0d, 100.25, -5.25 }[input[1]]); }
            else if (kind == "W") { fields.Bytes.AsSpan(0, 8).Fill((byte)input[4]); fields.Bytes[8] = (byte)input[0]; Put(fields.Bytes, 64, input[3]); result = RawWindAviaryState.WriteProcess(fields, packet, input[1] != 0, input[2] != 0, 0x43, 0x49); }
            else
            {
                Array.Fill(fields.Bytes, (byte)input[1]);
                // 원문 caller의 불투명64BYTE packet 입력을 같은 순서로 준비한다.
                for (int index = 0; index < packet.Length; index++) packet[index] = unchecked((byte)(index * 17 + 3));
                packet[8] = (byte)input[0]; parent = RawWindAviaryState.ReadProcess(fields, packet);
            }
            Assert.Equal(row[2], Hex(fields.Bytes)); Assert.Equal(row[3], Hex(packet)); Assert.Equal(N(row[4]), parent); Assert.Equal(N(row[5]), (uint)result); count++;
        }
        Assert.Equal(expected, count);
    }
    /// <summary>선택packet 전체폭은 첫쓰기전에 검사하고 force도 변경전에 전체폭을 검사한다.</summary>
    [Fact]
    public void ShortPacketsKeepFieldsAndWriterBufferUnchanged()
    {
        var fields = new WindAviaryProcessFields(0xa5); byte[] before = fields.Bytes.ToArray(); byte[] full = new byte[64]; full[8] = 0x3f;
        // 전체선택45BYTE보다 짧은모든길이에서 어떤 부분쓰기라도 생기면 실패한다.
        for (int size = 0; size < 45; size++)
        {
            byte[] small = Enumerable.Repeat((byte)0xa7, size).ToArray(); Assert.Throws<ArgumentException>(() => RawWindAviaryState.WriteProcess(fields, small, true, true)); Assert.Equal(before, fields.Bytes); Assert.All(small, value => Assert.Equal(0xa7, value));
            Assert.Throws<ArgumentException>(() => RawWindAviaryState.ReadProcess(fields, full.AsSpan(0, size))); Assert.Equal(before, fields.Bytes);
        }
        _ = RawWindAviaryState.WriteProcess(fields, full, false); Assert.Equal(1, full[0]); Assert.Equal(0x74, full[1]);
    }
    /// <summary>빈객체는미선택저장만허용하고 실제객체범위오류의앞선선택쓰기는보존한다.</summary>
    [Fact]
    public void NullObjectAndPartialObjectBoundsFollowSelectedStores()
    {
        byte[] packet = new byte[64]; Assert.Equal(0u, RawWindAviaryState.ReadProcess(Span<byte>.Empty, packet.AsSpan(0, 9))); packet[8] = 1; Put(packet, 9, 0xfedcba98); Assert.Equal(0xfedcba98u, RawWindAviaryState.ReadProcess(Span<byte>.Empty, packet.AsSpan(0, 13)));
        packet[8] = 0x3e; Bits(packet, 9, 0x7ff8123456789abc); Put(packet, 17, 7); Bits(packet, 21, 0x8000000000000000); byte[] fields = Enumerable.Repeat((byte)0xa5, 32).ToArray();
        Assert.Throws<ArgumentException>(() => RawWindAviaryState.ReadProcess(fields, packet)); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(fields.AsSpan(16))); Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(fields.AsSpan(24))); Assert.Equal(0xa5, fields[8]);
    }
    /// <summary>Wind writer는Sun/Rain과다르게전체DWORD식별자를기록하고caller꼬리/NaN을보존한다.</summary>
    [Fact]
    public void FullDwordIdentityRawBitsAndUnwrittenPaddingArePreserved()
    {
        var fields = new WindAviaryProcessFields(0xa5); RawWindAviaryState.Initialize(fields, false, 123); Assert.Equal(0xa5a5a5a5u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(24))); Assert.Equal(0xa5a5a5a5u, BinaryPrimitives.ReadUInt32LittleEndian(fields.Bytes.AsSpan(48))); Assert.All(fields.Bytes.AsSpan(68).ToArray(), value => Assert.Equal(0xa5, value));
        Put(fields.Bytes, 60, 0xabcd004d); Put(fields.Bytes, 64, 0xdcba0032); Bits(fields.Bytes, 16, 0x7ff8123456789abc); byte[] packet = Enumerable.Repeat((byte)0xa7, 64).ToArray(); Assert.Equal(45, RawWindAviaryState.WriteProcess(fields, packet, true, true)); Assert.Equal(0xabcd004du, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4))); Assert.Equal(0xdcba0032u, BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(9))); Assert.Equal(0xa7, packet[45]); var copy = new WindAviaryProcessFields(0x5a); Assert.Equal(0xdcba0032u, RawWindAviaryState.ReadProcess(copy, packet)); Assert.Equal(0x7ff8123456789abcul, BinaryPrimitives.ReadUInt64LittleEndian(copy.Bytes.AsSpan(16))); Assert.Equal(0x5a, copy.Bytes[8]);
    }
}
