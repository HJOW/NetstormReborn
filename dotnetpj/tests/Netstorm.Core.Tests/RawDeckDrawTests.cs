using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 직접 Deck 추첨582입력의 전체 바이트·순서·시각 효과·독립 난수를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>GemReward 원본 자료의 직접 Deck 부분만 사용하며 보상 선택/공격/변환은 실행하지 않는다.</summary>
public sealed class RawDeckDrawTests
{
    /// <summary>지정 DWORD의 원문 입력을 기록한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>원본 직접 추첨에 공급한 동일 물리 항목을 구성한다.</summary>
    private static byte[] Input(uint[] row)
    {
        var raw = new byte[ProductionDeckLayout.Bytes]; raw.AsSpan().Fill(0xa5);
        List<byte[]> entries = [[70, 20, 1, 255, 254], [71, 30, 1, 255, 0], [74, 50, 1, 255, 255]];
        switch (row[10])
        {
            case 1: entries[0] = [70, 20, 0, 255, 0]; break;
            case 2: entries[0] = [70, 20, 255, 255, 0]; break;
            case 3: entries.Clear(); break;
            case 4: entries = [[70, 0, 1, 255, 0], [71, 0, 1, 255, 0]]; break;
            case 5: entries = [[70, 255, 127, 255, 255]]; break;
            case 6: entries = [[70, 20, 128, 255, 0], [71, 30, 1, 255, 0]]; break;
        }
        uint sum = 0;
        // 초기 항목만 입력하고 계산 결과나 추첨 선택값은 만들지 않는다.
        for (int index = 0; index < entries.Count; index++)
        {
            entries[index].CopyTo(raw, index * 5); sum += entries[index][1];
        }
        Put(raw, 0x14c, row[10] == 7 ? uint.MaxValue : (uint)entries.Count);
        Put(raw, 0x150, row[10] is 1 or 2 ? 999 : sum); Put(raw, 0x154, uint.MaxValue);
        Put(raw, 0x158, row[11]); Put(raw, 0x15c, row[12]); return raw;
    }
    /// <summary>독립 원본의582개 직접 입력과 양쪽x87 정밀도의 기대값을 대조한다.</summary>
    [Fact]
    public void OriginalDirectDraw582()
    {
        string[][] rows = X86Fixture.Read("gem-reward").Where(fields => fields[0].StartsWith("2,", StringComparison.Ordinal)).ToArray();
        Assert.Equal(582, rows.Length);
        // 새 물리 Deck과 전역 난수로 각 행을 독립 실행한다.
        foreach (string[] fields in rows)
        {
            uint[] row = fields[0].Split(',').Select(X86Fixture.UInt).ToArray();
            byte[] raw = Input(row); var global = new GameRandom(row[3]); var trace = new List<string>();
            var draw = new RawDeckDraw(raw, global, new()
            {
                Assertion = line => trace.Add("I:" + line),
                Mismatch = (stored, sum) =>
                {
                    trace.Add($"M:{stored}:{sum}");
                    if (row[9] == 2) { Put(raw, 0x14c, 0); Put(raw, 0x150, 999); }
                },
                WallSeconds = () =>
                {
                    trace.Add($"W:{row[4]}:{row[5]}");
                    if (row[9] == 3) raw[2] = 0;
                    return BitConverter.UInt64BitsToDouble(row[4] | ((ulong)row[5] << 32));
                },
                IsProduct = type => type is >= 70 and <= 73,
            });
            int power = 0x55555555; uint result = row[13] != 0 ? draw.Draw(out power) : draw.Draw();
            Assert.Equal(X86Fixture.UInt(fields[1]), result); Assert.Equal(fields[2], trace.Count == 0 ? "-" : string.Join(';', trace));
            Assert.Equal(X86Fixture.UInt(fields[3]), global.State()); Assert.Equal(fields[4], Convert.ToHexString(raw).ToLowerInvariant());
            Assert.Equal(X86Fixture.UInt(fields[6]), unchecked((uint)power));
            // 제공된 두 원문 정밀도의 모든 관측 열도 같은 자료인지 확인한다.
            for (int column = 1; column <= 7; column++) Assert.Equal(fields[column], fields[column + 7]);
        }
    }
    /// <summary>Open의0seed만 전역 난수를 소비하고 Close는 다른 플래그/seed를 유지한다.</summary>
    [Fact]
    public void OpenAndClosePreserveFieldsAndUseOnlySelectedRandom()
    {
        byte[] raw = new byte[352]; RawDeckDraw.Initialize(raw); Put(raw, 0x15c, 0x80000200);
        var global = new GameRandom(123); var draw = new RawDeckDraw(raw, global, new());
        draw.Open(7); Assert.Equal(123u, global.State()); Assert.Equal(7u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x158)));
        Assert.Equal(0x80000201u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x15c)));
        draw.Close(); Assert.Equal(0x80000200u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x15c)));
        draw.Open(0); Assert.NotEqual(123u, global.State()); Assert.InRange(BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x158)), 0u, 9999u);
    }
    /// <summary>NULL power에서는 분류 장치를 요구하지 않고 선택된 미연결은 해당 효과 위치에서 실패한다.</summary>
    [Fact]
    public void OptionalPowerAndSelectedEffectsKeepPartialWrites()
    {
        byte[] raw = new byte[352]; RawDeckDraw.Initialize(raw); RawDeckDraw.Append(raw, 77, 1, null);
        var draw = new RawDeckDraw(raw, new GameRandom(1), new() { WallSeconds = () => 0 }); draw.Open(1);
        Assert.Equal(77u, draw.Draw()); Assert.Throws<InvalidOperationException>(() => draw.Draw(out _));
        Assert.Equal(2, raw[4]); Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(0x154)));
        draw.Close(); Assert.Throws<InvalidOperationException>(() => draw.Draw());
    }
}
