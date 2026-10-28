using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 사제 panic 이동(RawPriestPanic)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/PriestPanicTests.cpp 와 같다: 같은 풀 슬롯·속도 표·난수로 Run 한 번을 실행한다.
/// 효과 순서·두 슬롯 전체·속도 비트·난수 상태를 대조한다. 기준 판본은 10.78 이다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawPriestPanicTests
{
    /// <summary>명시 32비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="at">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓸 폭. 생략하면 DWORD 4다.</param>
    private static void SpPut(Span<byte> bytes, int at, uint value, int width = 4)
    {
        // 인접 바이트는 보존한다
        for (int b = 0; b < width; b++)
        {
            bytes[at + b] = (byte)(value >> (b * 8));
        }
    }

    /// <summary>바이트 배열을 Native의 소문자 16진 표로 바꾼다.</summary>
    /// <param name="bytes">바꿀 물리 전체.</param>
    private static string SpHex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        var text = new System.Text.StringBuilder(bytes.Length * 2);
        // 원문 순서와 0을 생략하지 않는다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>
    /// 원본 600입력의 실제 효과 순서·두 슬롯·임시 속도 복원·난수 소비를 대조한다.
    /// 결과를 쓰지 않고 명시 경계의 입력·현재 자료 변화만 공급한다.
    /// </summary>
    [Fact]
    public void PriestPanicNative600()
    {
        string[][] rows = X86Fixture.Read("priest-panic");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length);
            uint[] c = row[0].Split(',').Select(X86Fixture.UInt).ToArray();
            Assert.Equal(9, c.Length);
            var pool = new RawSidPool(24000, true);
            // 준비·경계가 바꿀 실제 동일 슬롯만 제공한다 (Span은 람다에 잡지 않고 그때마다 다시 벤다)
            pool.RawSlot(50).Fill(0);
            pool.RawSlot(60).Fill(0);
            SpPut(pool.RawSlot(50), 10, 158, 1);
            SpPut(pool.RawSlot(50), 26, c[1]);
            SpPut(pool.RawSlot(50), 34, 1, 1);
            SpPut(pool.RawSlot(60), 34, c[2] == 0 ? 2u : 1u, 1);
            SpPut(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(50.25f));
            SpPut(pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(50.75f));
            DudeNoticeTypeData[] speeds = Enumerable.Range(0, OriginalConstructors.TypeCount)
                .Select(_ => new DudeNoticeTypeData()).ToArray();
            speeds[158].Speed = BitConverter.UInt32BitsToSingle(c[3]);
            speeds[159].Speed = 2.5f;
            var random = new NetstormRandom(c[7]);
            uint lands = 0;
            uint regions = 0;
            uint randoms = 0;
            string effects = string.Empty;
            void Event(string tag, params object[] values)
            {
                // 원문 호출 순서의 전체 DWORD 비트를 기록한다
                if (effects.Length != 0)
                {
                    effects += ';';
                }
                effects += tag;
                foreach (object value in values)
                {
                    effects += ":" + value;
                }
            }
            var hooks = new PriestPanicHooks();
            hooks.Temple = (x, y) =>
            {
                Event("T", BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y));
                if (c[8] == 1)
                {
                    SpPut(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(51.25f));
                    SpPut(pool.RawSlot(50), 26, 20);
                    SpPut(pool.RawSlot(50), 34, 2, 1);
                    SpPut(pool.RawSlot(60), 34, 2, 1);
                }
                return c[0] != 0 ? 60 : 0;
            };
            hooks.Maximum = sid =>
            {
                Event("H", sid);
                if (c[8] == 2)
                {
                    SpPut(pool.RawSlot(50), 10, 159, 1);
                    SpPut(pool.RawSlot(50), 34, 2, 1);
                    SpPut(pool.RawSlot(60), 34, 2, 1);
                }
                return 100;
            };
            hooks.Target = (sid, target) =>
            {
                Event("V", sid, target, 0, 0, BitConverter.SingleToUInt32Bits(speeds[pool.RawSlot(50)[10]].Speed));
                if (c[8] == 3)
                {
                    SpPut(pool.RawSlot(50), 10, 159, 1);
                    SpPut(pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(52.5f));
                    speeds[158].Speed = 9.25f;
                }
                return (ulong)c[4];
            };
            hooks.Land = (x, y) =>
            {
                Event("L", BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y));
                lands++;
                if (c[8] == 4 && lands == 1)
                {
                    SpPut(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(61.25f));
                    SpPut(pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(62.5f));
                }
                return c[5] != 0 && (c[5] != 1 || (lands != 2 && lands != 3));
            };
            hooks.Region = (x, y) =>
            {
                Event("G", BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y));
                regions++;
                if (c[8] == 6 && regions == 1)
                {
                    SpPut(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(81.25f));
                    SpPut(pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(82.5f));
                }
                return c[5] == 3 && (regions == 1 || regions == 3 || regions == 5) ? 2u : 1u;
            };
            hooks.Random = (low, high) =>
            {
                Event("R", BitConverter.SingleToUInt32Bits(low), BitConverter.SingleToUInt32Bits(high));
                randoms++;
                if (c[8] == 5 && randoms == 1)
                {
                    SpPut(pool.RawSlot(50), 14, BitConverter.SingleToUInt32Bits(71.25f));
                    SpPut(pool.RawSlot(50), 18, BitConverter.SingleToUInt32Bits(72.5f));
                }
                return (double)random.Next(65536) / 65536.0 * Math.Abs((double)high - low) + low;
            };
            hooks.Point = (sid, x, y) =>
            {
                Event("P", sid, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), 0, 0, 0, 0, 0);
                if (c[8] == 7)
                {
                    SpPut(pool.RawSlot(50), 34, 3, 1);
                }
                return (ulong)c[6];
            };
            new RawPriestPanic(pool, speeds, hooks).Run(50);
            string speedBits = BitConverter.SingleToUInt32Bits(speeds[158].Speed) + "," + BitConverter.SingleToUInt32Bits(speeds[159].Speed);
            Assert.Equal(row[1], effects);
            Assert.Equal(row[2], SpHex(pool.Slot(50)) + SpHex(pool.Slot(60)));
            Assert.Equal(row[3], speedBits);
            Assert.Equal(X86Fixture.UInt(row[4]), random.State);
            count++;
            return true;
        });
        Assert.Equal(600, count);
    }
}
