// 신규 장부의 전체 인접 물리를 제품 전에 고정한 관측값과 대조한다.
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawPlayerCoreRecords의 장부 초기화를 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRecordsTests
{
    /// <summary>바이트를 소문자 16진수로 만든다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>소문자 16진수다.</returns>
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>별도 owner 색표 6관측은 제외하고 Core 관련 186관측의 전체 물리를 대조한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ReplaysIndependentPe186AndSeparatesColorTable6()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-core-records-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int rows = 0;
        int compared = 0;
        int colorAudit = 0;
        // 최초 192관측을 빠짐없이 분류하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string rowText = line.EndsWith('\r') ? line.Substring(0, line.Length - 1) : line;
            ++rows;
            string[] values = rowText.Split('\t');
            Assert.Equal(7, values.Length);
            string[] input = values[1].Split(',');
            uint mode = uint.Parse(input[0], CultureInfo.InvariantCulture);
            uint seed = uint.Parse(input[1], CultureInfo.InvariantCulture);
            uint owner = unchecked((uint)ulong.Parse(input[2], CultureInfo.InvariantCulture));
            uint subscriber = unchecked((uint)ulong.Parse(input[3], CultureInfo.InvariantCulture));
            uint coreSeed = uint.Parse(input[4], CultureInfo.InvariantCulture);
            byte[] records = new byte[256];
            byte[] core = new byte[16];
            byte[] colors = new byte[44];
            byte[] argument = new byte[10];
            // Native의 장부 앞뒤 인접 물리도 같이 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < records.Length; ++index)
            {
                records[index] = unchecked((byte)(seed + (uint)index * 17u));
            }

            // 별도 지역 Core 물리를 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < core.Length; ++index)
            {
                core[index] = unchecked((byte)(seed + (uint)index * 13u));
            }

            // Core와 무관한 표를 별도로 유지하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < colors.Length; ++index)
            {
                colors[index] = unchecked((byte)(seed + (uint)index * 7u));
            }

            // 이미 값으로 복제한 10바이트 인자를 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < argument.Length; ++index)
            {
                argument[index] = unchecked((byte)(coreSeed + (uint)index * 23u));
            }

            if (mode == 0u)
            {
                RawPlayerCoreRecords.InitializePlayerCore(core);
            }
            else if (mode == 1u)
            {
                PlayerCoreRecordsView view = new() { Bytes = records, Origin = 64u };
                RawPlayerCoreRecords.WritePlayerCoreMetadata(view, owner, subscriber, argument);
            }
            else if (mode == 2u)
            {
                PlayerCoreRecordsView view = new() { Bytes = records, Origin = 64u };
                RawPlayerCoreRecords.InitializePlayerCoreRecords(view);
            }
            else if (mode == 3u)
            {
                RawPlayerCoreRecords.InitializePlayerCore(records.AsSpan(208, 16));
            }
            else if (mode == 4u)
            {
                ++colorAudit;
                Assert.StartsWith("O0:4:0;O4:4:1;", values[5]);
                continue;
            }
            else
            {
                throw new InvalidOperationException("Core 장부 진입 모드");
            }

            ++compared;
            Assert.Equal(values[2], Hex(records));
            Assert.Equal(values[3], Hex(core));
            Assert.Equal(values[4], Hex(colors));
        }

        Assert.Equal(192, rows);
        Assert.Equal(186, compared);
        Assert.Equal(6, colorAudit);
    }

    /// <summary>선택된 단일 쓰기 폭 부족에서 앞선 쓰기만 유지한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ShortPhysicalViewsPreserveOrderedPrefix()
    {
        // Core의 4·4·2 경계와 사이 짧은 물리를 검증하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int length = 0; length <= 10; ++length)
        {
            byte[] bytes = new byte[16];
            Array.Fill(bytes, (byte)0xa5);
            bool rejected = false;
            try
            {
                RawPlayerCoreRecords.InitializePlayerCore(bytes.AsSpan(0, length));
            }
            catch (ArgumentOutOfRangeException)
            {
                rejected = true;
            }

            Assert.Equal(length < 10, rejected);
            int zeroEnd = length >= 10 ? 10 : length >= 8 ? 8 : length >= 4 ? 4 : 0;
            // 성공한 쓰기 뒤의 물리는 그대로 두는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < bytes.Length; ++index)
            {
                Assert.Equal(index < zeroEnd ? 0 : 0xa5, bytes[index]);
            }
        }

        byte[] shortRecords = new byte[13];
        Array.Fill(shortRecords, (byte)0xa5);
        bool denied = false;
        try
        {
            RawPlayerCoreRecords.InitializePlayerCoreRecords(new PlayerCoreRecordsView { Bytes = shortRecords, Origin = 0u });
        }
        catch (ArgumentOutOfRangeException)
        {
            denied = true;
        }

        Assert.True(denied && shortRecords[0] == 0xa5 && shortRecords[3] == 0xa5 && shortRecords[4] == 0 && shortRecords[11] == 0 && shortRecords[12] == 0xa5);
        byte[]records2 = new byte[17];
        Array.Fill(records2, (byte)0xa5);
        denied = false;
        try
        {
            RawPlayerCoreRecords.InitializePlayerCoreRecords(new PlayerCoreRecordsView { Bytes = records2, Origin = 0u });
        }
        catch (ArgumentOutOfRangeException)
        {
            denied = true;
        }

        Assert.True(denied && records2.AsSpan(0, 14).ToArray().All(b => b == 0) && records2[14] == 0xa5 && records2[16] == 0xa5);
    }

    /// <summary>metadata는 입력값 복제와 패딩 2바이트를 보존한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void MetadataPreservesPaddingAndCopiesInputValues()
    {
        byte[] records = new byte[144];
        Array.Fill(records, (byte)0xa5);
        byte[] core = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
        RawPlayerCoreRecords.WritePlayerCoreMetadata(new PlayerCoreRecordsView { Bytes = records, Origin = 0u }, 8u, 0x12345678u, core);
        Array.Clear(core);
        Assert.True(records[128] == 0x78 && records[131] == 0x12 && records[132] == 1 && records[141] == 10 && records[142] == 0xa5 && records[143] == 0xa5);
        byte[] before = (byte[])records.Clone();
        bool rejected = false;
        try
        {
            RawPlayerCoreRecords.WritePlayerCoreMetadata(new PlayerCoreRecordsView { Bytes = records, Origin = 0u }, 9u, 0u, core);
        }
        catch (ArgumentOutOfRangeException)
        {
            rejected = true;
        }

        Assert.True(rejected && records.AsSpan().SequenceEqual(before));
    }
}
