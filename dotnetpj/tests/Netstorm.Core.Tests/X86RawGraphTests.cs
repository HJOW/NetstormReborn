using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 그래프 표 저장소(<see cref="RawGraph"/>) 초기화를 원본 x86 기대값에 대조한다.
/// 기대값은 graph-reset-x86.tsv 17행(헤더 포함)이며, 입력은 원본과 같은 1,530바이트 표(byte*37+seed)이고
/// 각 행은 초기화 뒤의 표 전체 1,530바이트와 풀·스택 불변을 확인한다.
/// 근거: cpppj/tests/GraphResetTests.cpp (W02 열셋째 조각, 2026-10-20).
/// </summary>
public sealed class X86RawGraphTests
{
    /// <summary>표를 바이트로 늘어놓는다(레코드마다 리틀 엔디언 16비트 3개).</summary>
    /// <param name="records">표 레코드.</param>
    private static byte[] RecordBytes(IReadOnlyList<GraphRecord> records)
    {
        var bytes = new byte[records.Count * 6];
        // 레코드마다 표면 수·사용 여부·예약 WORD 순서로 쓴다
        for (int i = 0; i < records.Count; i++)
        {
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6), records[i].Surfaces);
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6 + 2), records[i].InUse);
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6 + 4), records[i].Reserved);
        }
        return bytes;
    }

    /// <summary>1,530바이트 입력을 16비트 3개씩 읽어 표 레코드 255개로 만든다.</summary>
    /// <param name="bytes">입력 바이트.</param>
    private static GraphRecord[] ParseRecords(byte[] bytes)
    {
        var records = new GraphRecord[RawGraph.TableSize];
        // 레코드마다 세 WORD 를 읽는다
        for (int i = 0; i < records.Length; i++)
        {
            records[i] = new GraphRecord
            {
                Surfaces = BitConverter.ToInt16(bytes, i * 6),
                InUse = BitConverter.ToInt16(bytes, i * 6 + 2),
                Reserved = BitConverter.ToInt16(bytes, i * 6 + 4),
            };
        }
        return records;
    }

    /// <summary>생성자 주소만 채운 10.78 타입 레코드 188개.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, OriginalConstructors.TypeCount)
        .Select(number => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(number) })
        .ToArray();

    /// <summary>
    /// 16가지 입력 표 + 헤더 행을 재생한다. 표의 초기화 결과가 원본과 같고, 풀 슬롯 5 와 스택 4,096개가 바뀌지 않는다.
    /// </summary>
    [Fact]
    public void ResetTableMatchesOriginalX86AndLeavesPoolAndStack()
    {
        string[][] rows = X86Fixture.ReadFile("graph-reset-x86.tsv");
        Assert.Equal(17, rows.Length);
        Assert.Equal("seed", rows[0][0]);
        int rowsPlayed = 0;
        X86Fixture.CheckRows(rows.Skip(1).ToArray(), row =>
        {
            Assert.Equal(2, row.Length);
            int seed = X86Fixture.Int(row[0]);
            var input = new byte[RawGraph.TableSize * 6];
            // reserved/gap 도 서로 다른 바이트로 채운다
            for (int i = 0; i < input.Length; i++)
            {
                input[i] = unchecked((byte)(i * 37 + seed));
            }
            var pool = new RawSidPool(24000, false);
            var hash = new RawSquidHash();
            byte[] spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
            uint[] stack = Enumerable.Repeat(0x12345678U, RawGraph.FloodSize).ToArray();
            var graph = new RawGraph(pool, hash, spots, Types(), ParseRecords(input), stack);
            string before = Convert.ToHexStringLower(pool.Slot(RawSidPool.FirstOrdinary));
            graph.ResetTable();
            rowsPlayed++;
            return Convert.ToHexStringLower(RecordBytes(graph.Records)) == row[1]
                && Convert.ToHexStringLower(pool.Slot(RawSidPool.FirstOrdinary)) == before
                && graph.FloodStack.All(value => value == 0x12345678U);
        });
        Assert.Equal(16, rowsPlayed);
    }
}
