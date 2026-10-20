using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 사각형 탐색기(<see cref="RawSquidFinder"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 rawfinder-x86.tsv(탐색 회귀, 판본별 Find 584회) 중 10.78 몫이다.
/// 각 탐색마다 커서 상태 15개와 방문 순서를 비교한다.
/// 열 구성은 cpppj/tests/RawFinderTests.cpp 와 같다.
/// CD 10.72·10.37 행과 Pre·Post 생애주기 행은 기준 판본·범위가 아니므로 읽지 않는다 (W02 탐색 조각, 2026-10-20).
/// </summary>
public sealed class X86RawFinderTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>판본별 Find 호출 수. 전체 1,752행 중 10.78 몫이다.</summary>
    private const int FindsPerEdition = 584;

    /// <summary>회귀 한 번의 최대 Next 호출 수(무한 순환 방지).</summary>
    private const int MaxSteps = 32;

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>탐색 커서 상태를 기계어 형식("면적 4·처음 2·마지막 2·단계·좌표 2·다음·현재·표면·높이")으로 만든다.</summary>
    /// <param name="finder">탐색기.</param>
    private static string Snapshot(RawSquidFinder finder)
    {
        SquidSearchState state = finder.State;
        int[] values =
        [
            state.Area.Left, state.Area.Top, state.Area.Right, state.Area.Bottom,
            state.FirstX, state.FirstY, state.LastX, state.LastY,
            state.Level, state.X, state.Y, state.Next, state.Current,
            state.SkipSurface ? 1 : 0, state.ExtendedHeight ? 4 : 0,
        ];
        return string.Join(',', values);
    }

    /// <summary>
    /// 사각형 탐색 fixture 의 10.78 몫 584회를 재생한다.
    /// Begin 뒤 Next 회귀의 커서 상태가 원본과 같다.
    /// </summary>
    [Fact]
    public void MachineCursorAndDynamicInputsMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("rawfinder");
        int count = 0;
        var failures = new List<string>();
        // 판본 이름이 originals 인 Find 행만 10.78 경로로 재생한다
        foreach (string[] row in rows)
        {
            if (row[0] != "Find" || row[1] != PatchEdition)
            {
                continue;
            }
            try
            {
                Assert.Equal(8, row.Length);
                var pool = new RawSidPool(24000, false);
                // 원본 관찰의 번호 50~64번을 클라이언트 영역에 확보한다
                for (int id = 5; id <= 64; id++)
                {
                    Assert.Equal(id, pool.Allocate(SidAllocation.Client));
                }
                var hash = new RawSquidHash();
                var types = new OriginalTypeRecord[256];
                // 256개 타입 번호 각각에 빈 타입 레코드를 채운다
                for (int i = 0; i < types.Length; i++)
                {
                    types[i] = new OriginalTypeRecord();
                }
                var heights = new int[256];
                hash.Reset();
                Array.Clear(heights);
                // 객체 설명(번호·타입·상태·추가·좌표 비트·발자국·높이·단계)으로 슬롯·타입·해시를 채운다
                foreach (string entry in row[6].Split(';'))
                {
                    string[] fields = entry.Split(',');
                    Assert.Equal(10, fields.Length);
                    uint[] values = fields.Select(X86Fixture.UInt).ToArray();
                    Span<byte> raw = pool.RawSlot((int)values[0]);
                    raw.Clear();
                    raw[RawSidPool.TypeOffset] = (byte)values[1];
                    raw[RawSidPool.StateOffset] = (byte)values[2];
                    raw[40] = (byte)values[3];
                    Put(raw, 14, values[4], 4);
                    Put(raw, 18, values[5], 4);
                    types[values[1]].FootX = (int)values[6];
                    types[values[1]].FootY = (int)values[7];
                    heights[values[1]] = (int)values[8];
                    ref ushort head = ref hash.Bucket((int)values[9], BitConverter.UInt32BitsToSingle(values[4]), BitConverter.UInt32BitsToSingle(values[5]));
                    Put(raw, 4, head, 2);
                    head = (ushort)values[0];
                }
                string[] area = row[4].Split(',');
                var finder = new RawSquidFinder(pool, hash, types, heights);
                int current = finder.Begin(
                    new SquidSearchArea(X86Fixture.Int(area[0]), X86Fixture.Int(area[1]), X86Fixture.Int(area[2]), X86Fixture.Int(area[3])),
                    X86Fixture.UInt(row[3]));
                string output = Snapshot(finder);
                int index = 0;
                // 원본처럼 회귀 중 슬롯을 직접 고치고 다음 커서를 받는다
                while (current != 0 && index < MaxSteps)
                {
                    if (index == 0)
                    {
                        int mutation = X86Fixture.Int(row[5]);
                        if (mutation == 1)
                        {
                            Put(pool.RawSlot(current), 4, 0, 2);
                        }
                        else if (mutation == 2)
                        {
                            pool.RawSlot(63)[40] = 8;
                        }
                        else if (mutation == 3)
                        {
                            Put(pool.RawSlot(63), 14, BitConverter.SingleToUInt32Bits(100.25f), 4);
                            Put(pool.RawSlot(63), 18, BitConverter.SingleToUInt32Bits(100.75f), 4);
                        }
                        else if (mutation == 4)
                        {
                            pool.RawSlot(63)[RawSidPool.TypeOffset] = (byte)X86Fixture.UInt(row[6].Split(';')[0].Split(',')[1]);
                        }
                    }
                    current = finder.Next();
                    output += ';' + Snapshot(finder);
                    index++;
                }
                if (index >= MaxSteps || output != row[7])
                {
                    failures.Add($"{count}: 기대 {row[7][..Math.Min(120, row[7].Length)]} / 실제 {output[..Math.Min(120, output.Length)]}");
                }
                count++;
            }
            catch (Exception error)
            {
                failures.Add($"{count}: {error.GetType().Name}: {error.Message}");
                count++;
            }
            if (failures.Count >= 6)
            {
                break;
            }
        }
        Assert.True(failures.Count == 0, $"원본 입력 {count}개 중 {failures.Count}개 불일치\n" + string.Join('\n', failures.Take(6)));
        Assert.Equal(FindsPerEdition, count);
    }
}
