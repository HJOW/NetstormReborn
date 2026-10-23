using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 실제 플라이어의 가상 사전 파괴(RawFlyerPreDestroy)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/FlyerPreDestroyTests.cpp 와 같다: 부모 50 과 두 대상 60·61, 높은 WORD 슬롯 32767 을
/// 모두 0 으로 두고 가상 표·타입·상태·좌표를 넣은 뒤, +8 WORD 의 가상 삭제와 Damageable 순서를 기록한다.
/// 호출마다 효과 문자열과 네 슬롯 200바이트 전체를 비교한다.
/// 기준 판본은 10.78 이다 (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class X86RawFlyerPreDestroyTests
{
    /// <summary>원문이 읽는 부모 번호.</summary>
    private const int Source = 50;

    /// <summary>원문이 읽는 두 대상과 높은 WORD 슬롯 번호.</summary>
    private static readonly int[] Ids = [50, 60, 61, 32767];

    /// <summary>10.78 플라이어 클래스의 가상 표 값(원본 관찰 입력).</summary>
    private const uint PatchFlyerVtable = 0x12400000;

    /// <summary>원본 관찰이 플라이어에 넣는 타입 번호.</summary>
    private const byte FlyerType = 144;

    /// <summary>원본 관찰의 풀 크기(클라이언트 풀).</summary>
    private const int Capacity = 32768;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>16비트 WORD 를 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutWord(Span<byte> bytes, int offset, uint value)
    {
        // 두 바이트만 낮은 바이트부터 쓴다
        bytes[offset] = (byte)value;
        bytes[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>플라이어 사전 파괴 fixture 96회를 재생한다.</summary>
    [Fact]
    public void FlyerPreDestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("flyer-predestroy");
        Assert.Equal(96, rows.Length);
        var pool = new RawSidPool(Capacity, false);
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length);
            uint child = X86Fixture.UInt(row[0]);
            uint flags = X86Fixture.UInt(row[1]);
            uint mutation = X86Fixture.UInt(row[2]);
            // 실제 TYPE·상태가 +8 WORD 위에 있으므로 넓게 읽으면 다른 대상이 된다
            foreach (int sid in Ids)
            {
                Span<byte> bytes = pool.RawSlot(sid);
                bytes.Clear();
                Put(bytes, 0, PatchFlyerVtable);
                bytes[10] = FlyerType;
                bytes[11] = 2;
                Put(bytes, 14, BitConverter.SingleToUInt32Bits(2.125f));
                Put(bytes, 18, BitConverter.SingleToUInt32Bits(3.25f));
                // 부모에만 대상 WORD 를 둔다
                if (sid == Source)
                {
                    PutWord(bytes, 8, child);
                }
            }
            var parts = new List<string>();
            void Event(string text)
            {
                // 사건 사이에 세미콜론을 둔다
                if (parts.Count != 0)
                {
                    parts.Add(";");
                }
                parts.Add(text);
            }
            var hooks = new FlyerPreDestroyHooks
            {
                Destroy = (sid, value) =>
                {
                    Event($"D:{sid}:{value}");
                    Assert.True(sid == (int)child && value == 0);
                    // 변형 1은 대상 WORD 와 추가 비트를, 변형 2는 대상 0 과 상태를 바꾼다
                    if (mutation == 1)
                    {
                        PutWord(pool.RawSlot(Source), 8, 61);
                        pool.RawSlot(Source)[40] = 9;
                    }
                    if (mutation == 2)
                    {
                        PutWord(pool.RawSlot(Source), 8, 0);
                        pool.RawSlot(sid)[11] = 1;
                    }
                },
                Damageable = (sid, value) =>
                {
                    Event($"P:{sid}:{value}");
                    // 변형 3은 하위 호출 뒤 좌표를 바꾼다
                    if (mutation == 3)
                    {
                        Put(pool.RawSlot(Source), 14, BitConverter.SingleToUInt32Bits(9.75f));
                    }
                },
            };
            var body = new RawFlyerPreDestroy(pool, hooks);
            body.PreDestroy(Source, flags);
            string trace = string.Concat(parts);
            var slots = new List<byte>();
            // 네 슬롯의 미변경 필드도 포함한다
            foreach (int sid in Ids)
            {
                slots.AddRange(pool.Slot(sid).ToArray());
            }
            count++;
            return trace == row[3] && Convert.ToHexStringLower(slots.ToArray()) == row[4];
        });
        Assert.Equal(96, count);
    }
}
