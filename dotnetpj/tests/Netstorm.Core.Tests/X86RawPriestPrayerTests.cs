using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 사제 기도 명령·수신·진행(RawPriestPrayer)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/PriestPrayerTests.cpp 와 같다: 같은 풀 슬롯·타입·권한으로 명령·수신·진행 1126행을 실행한다.
/// 효과 순서·슬롯 전체·반환 비트·최종 권한을 대조한다. 기준 판본은 10.78 이다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawPriestPrayerTests
{
    /// <summary>명시 32비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="at">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void SpPut(Span<byte> bytes, int at, uint value)
    {
        // 인접 바이트는 보존한다
        for (int b = 0; b < 4; b++)
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
        // 전체 60바이트를 순서대로 바꾼다
        foreach (byte value in bytes)
        {
            text.Append(digits[value >> 4]);
            text.Append(digits[value & 15]);
        }
        return text.ToString();
    }

    /// <summary>
    /// 원문이 계산한 호출 순서·전체 슬롯·반환 비트·최종 권한을 그대로 대조한다.
    /// 명령·수신·진행 605와 모든 명시 관문·변형을 소비한다.
    /// </summary>
    [Fact]
    public void PriestPrayerIndependentNative1126()
    {
        string[][] rows = X86Fixture.Read("priest-prayer");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length);
            string[] parts = row[0].Split(',');
            Assert.Equal(15, parts.Length);
            string mode = parts[0];
            long[] a = parts.Skip(1).Select(value => long.Parse(value)).ToArray();
            var pool = new RawSidPool(24000, true);
            Span<byte> raw = pool.RawSlot(60);
            raw.Fill(0);
            SpPut(raw, 0, 0x12200000u);
            raw[10] = 158;
            raw[11] = (byte)a[0];
            SpPut(raw, 36, 7);
            SpPut(raw, 42, unchecked((uint)a[12]));
            SpPut(raw, 46, 0xaabbccdd);
            PriestPrayerType[] types = Enumerable.Range(0, OriginalConstructors.TypeCount)
                .Select(_ => new PriestPrayerType()).ToArray();
            types[158].DefaultFrame = 7;
            types[159].DefaultFrame = 9;
            types[113].PrayTime = types[168].PrayTime = (int)a[9];
            bool boss = a[3] != 0;
            string effects = string.Empty;
            uint parameter = unchecked((uint)(((ulong)a[12] << 16) | 60u));
            void Event(string value)
            {
                // 효과 순서만 기록하며 실제 판정은 제품이 수행한다
                effects += effects.Length == 0 ? value : ";" + value;
            }
            var hooks = new PriestPrayerHooks();
            hooks.HasEvent = (sid, code) =>
            {
                Event("E:" + code);
                return a[1] != 0;
            };
            hooks.Direction = sid =>
            {
                Event("D:" + sid);
                return (int)a[2];
            };
            hooks.MovementType = kind =>
            {
                Event("M:" + kind);
                return 54u;
            };
            hooks.Movement = (sid, type, ordinal, remove) =>
            {
                Event("L:" + sid + ':' + type + ':' + ordinal + ':' + (remove ? 1 : 0));
                if (a[13] == 2)
                {
                    SpPut(pool.RawSlot(60), 42, 113);
                }
                return a[5] != 0 ? 77 : 0;
            };
            hooks.Request = (target, packet) =>
            {
                Event("P:" + target + ':' + SpHex(packet));
                if (a[13] == 3)
                {
                    boss = false;
                    pool.RawSlot(60)[10] = 159;
                }
            };
            hooks.Broadcast = packet =>
            {
                Event("B:" + SpHex(packet));
                if (a[13] == 3)
                {
                    boss = false;
                    pool.RawSlot(60)[10] = 159;
                }
            };
            hooks.ReserveShared = () =>
            {
                Event("A:40");
                return a[4] != 0;
            };
            hooks.AddShared = (sid, code, payload) =>
            {
                Event("R:" + code + ':' + sid + ':' + BitConverter.SingleToUInt32Bits(payload));
            };
            hooks.Ritual = (sid, stage) =>
            {
                Event("S:" + sid + ':' + stage);
            };
            hooks.Immobile = sid =>
            {
                Event("I:" + sid);
                if (a[13] == 4)
                {
                    boss = false;
                    SpPut(pool.RawSlot(60), 42, 113);
                }
                return a[6] != 0;
            };
            hooks.ContainedType = (sid, mask, kind, ordinal) =>
            {
                Event("H:" + sid + ':' + mask + ':' + kind + ':' + ordinal);
                if (a[13] == 5)
                {
                    boss = false;
                }
                return a[7] != 0 ? 168u : 0u;
            };
            hooks.Discard = sid =>
            {
                Event("Q:" + sid);
                if (a[13] == 6)
                {
                    SpPut(pool.RawSlot(60), 42, 113);
                    pool.RawSlot(60)[10] = 159;
                }
            };
            hooks.Create = (sid, kind, argument, flags, attachFlags, quality) =>
            {
                Event("C:" + sid + ':' + kind + ':' + argument + ':' + flags + ':' + attachFlags + ':' + quality);
            };
            hooks.SetFrame = (sid, frame, flags) =>
            {
                Event("F:" + sid + ':' + frame + ':' + flags);
                SpPut(pool.RawSlot(60), 36, unchecked((uint)frame));
            };
            hooks.Panic = sid =>
            {
                Event("V:" + sid);
            };
            hooks.Parent = (sid, code, value) =>
            {
                Event("X:" + sid + ':' + code + ':' + value);
                return 0x12345678u;
            };
            var logic = new RawPriestPrayer(pool, types, () => boss, hooks);
            uint result;
            if (mode == "C")
            {
                result = logic.Command(60, unchecked((uint)a[11]), parameter);
            }
            else if (mode == "R")
            {
                byte[] packet =
                [
                    1, 0x6d, 8, 0, 60, 0,
                    (byte)a[12], (byte)(a[12] >> 8),
                ];
                logic.Receive(packet);
                result = 0;
            }
            else
            {
                result = BitConverter.SingleToUInt32Bits(logic.Tick(60, unchecked((uint)a[8]), BitConverter.UInt32BitsToSingle(unchecked((uint)a[10]))));
            }
            if (effects.Length == 0)
            {
                effects = "-";
            }
            Assert.Equal(row[1], effects);
            Assert.Equal(row[2], SpHex(pool.Slot(60)));
            Assert.Equal(row[3], result.ToString());
            Assert.Equal(row[4], (boss ? 1 : 0).ToString());
            count++;
            return true;
        });
        Assert.Equal(1126, count);
    }
}
