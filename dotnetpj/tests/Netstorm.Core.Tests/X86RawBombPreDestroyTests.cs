using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 Point Blast Bomb 가상 사전 파괴(RawBombPreDestroy)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/LightningScreenTests.cpp 의 BombPreDestroyNative 절과 같다: 실제 풀의 Bomb 50 에
/// 가상 표 0x502370·타입 113·상태 2·추가 비트를 넣고, 폭발·소리·공통 전처리를 순서대로 기록한다.
/// 호출마다 효과 문자열과 슬롯 전체 50바이트를 비교하고, 32비트·64비트 원본 관찰이 같은지도 확인한다.
/// 기준 판본은 10.78 이다 (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class X86RawBombPreDestroyTests
{
    /// <summary>원본 관찰이 쓰는 Bomb 슬롯 번호.</summary>
    private const int Source = 50;

    /// <summary>10.78 Point Blast Bomb 클래스의 가상 표 값.</summary>
    private const uint PatchBombVtable = 0x00502370;

    /// <summary>원본 관찰이 Bomb 에 넣는 타입 번호.</summary>
    private const byte BombType = 113;

    /// <summary>원본 관찰의 풀 크기(서버 풀).</summary>
    private const int Capacity = 24000;

    /// <summary>폭발 효과가 바꾸는 좌표(원본 관찰 변형 1).</summary>
    private const uint MutatedX = 0x42cb0000;

    /// <summary>폭발 효과가 바꾸는 좌표(원본 관찰 변형 1).</summary>
    private const uint MutatedY = 0xc0880000;

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

    /// <summary>슬롯 전체를 소문자 hex 로 만든다.</summary>
    /// <param name="span">슬롯 바이트.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>Bomb 사전 파괴 fixture 96회를 재생한다.</summary>
    [Fact]
    public void BombPreDestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("bomb-predestroy");
        Assert.Equal(96, rows.Length);
        var pool = new RawSidPool(Capacity, true);
        // 각 행이 같은 슬롯을 다시 쓰도록 50 까지 실제 할당한다
        while (pool.Allocate(SidAllocation.Client) != Source)
        {
        }
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(5, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(3, input.Length);
            uint extra = X86Fixture.UInt(input[0]);
            uint flags = X86Fixture.UInt(input[1]);
            uint mutation = X86Fixture.UInt(input[2]);
            Span<byte> raw = pool.RawSlot(Source);
            raw.Fill(0xa5);
            Put(raw, 0, PatchBombVtable);
            raw[10] = BombType;
            raw[11] = 2;
            raw[40] = (byte)extra;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(98.0f));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(122.0f));
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
            var hooks = new BombPreDestroyHooks
            {
                Explosion = sid =>
                {
                    Event("E:" + sid);
                    // 변형 1은 좌표를, 변형 2는 추가 비트와 가상 표를 바꾼다
                    if (mutation == 1)
                    {
                        Put(pool.RawSlot(Source), 14, MutatedX);
                        Put(pool.RawSlot(Source), 18, MutatedY);
                    }
                    if (mutation == 2)
                    {
                        pool.RawSlot(Source)[40] = 9;
                        Put(pool.RawSlot(Source), 0, 0x00501dd0);
                    }
                },
                Sound = (x, y) =>
                {
                    Event($"S:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:0");
                    // 변형 3은 소리 뒤 추가 비트를 지우고 다른 필드를 덮는다
                    if (mutation == 3)
                    {
                        pool.RawSlot(Source)[40] = 0;
                        pool.RawSlot(Source)[12] = 0xef;
                        pool.RawSlot(Source)[13] = 0xbe;
                    }
                },
                Base = (sid, value) => Event($"B:{sid}:{value}"),
            };
            var body = new RawBombPreDestroy(pool, hooks);
            Assert.True(body.Handles(Source));
            body.PreDestroy(Source, flags);
            string trace = string.Concat(parts);
            count++;
            // 32비트·64비트 원본 관찰이 같은 결과인지도 함께 확인한다
            return trace == row[1] && Hex(pool.Slot(Source)) == row[2] && row[1] == row[3] && row[2] == row[4];
        });
        Assert.Equal(96, count);
    }
}
