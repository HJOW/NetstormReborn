using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 슬롯 풀(<see cref="RawSidPool"/>)을 원본 x86 기대값(sid-x86.tsv)의 10.78 행에 대조한다.
/// 기존 <see cref="X86SidTests"/>는 압축 표현에서 슬롯 바이트를 되살려 체크섬을 비교했지만, 이 풀은 슬롯 바이트를 직접 가지므로
/// 풀 전체 바이트의 Adler-32 를 그대로 비교한다. 열 구성은 cpppj/tests/SidTests.cpp 와 같다 (W02 첫 조각, 2026-10-16).
/// CD 10.72 행은 기준 판본이 아니므로 읽지 않는다.
/// </summary>
public sealed class X86RawSidPoolTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>
    /// 삭제 기록 두 목록(클라이언트, 서버 순서)의 번호와 타입을 리틀 엔디언 32비트로 늘어놓은 바이트의 체크섬.
    /// </summary>
    /// <param name="pool">대상 풀.</param>
    private static uint LogAdler(RawSidPool pool)
    {
        var bytes = new List<byte>();
        // 클라이언트 기록 다음에 서버 기록
        foreach (bool client in new[] { true, false })
        {
            // 최신 기록부터 20개
            foreach (SidDeletion record in pool.Deletions(client))
            {
                bytes.AddRange(BitConverter.GetBytes(record.Sid));
                bytes.AddRange(BitConverter.GetBytes(record.Type));
            }
        }
        return X86Fixture.Adler32(bytes.ToArray());
    }

    /// <summary>
    /// 10.78 세션 8개(풀 크기 4종 × 서버/클라이언트)의 전이를 재생한다. 각 전이 뒤 반환 번호·카운터·목록 머리/꼬리·풀 전체 바이트·삭제 기록이
    /// 원본과 같아야 한다. 합성 생성자 입력(Fill)은 슬롯 전체를 seed + i × 37 로 채우고 타입·상태를 쓴다(기대값을 만드는 계산이 아니다).
    /// </summary>
    [Fact]
    public void PatchTransitionsMatchOriginalX86IncludingEverySlotByte()
    {
        string[][] rows = X86Fixture.Read("sid");
        RawSidPool? pool = null;
        bool patch = false;
        int begins = 0;
        int steps = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                patch = row[1] == PatchEdition;
                if (patch)
                {
                    pool = new RawSidPool(X86Fixture.Int(row[2]), row[3] == "1");
                    begins++;
                }
                return true;
            }
            if (!patch)
            {
                return true;
            }
            if (row[0] == "Fill")
            {
                int sid = X86Fixture.Int(row[1]);
                int seed = X86Fixture.Int(row[2]);
                Span<byte> slot = pool!.AllocatedBytes(sid);
                // 슬롯 전체를 합성 입력으로 채운 뒤 타입·상태를 쓴다
                for (int i = 0; i < slot.Length; i++)
                {
                    slot[i] = (byte)(seed + i * 37);
                }
                slot[RawSidPool.TypeOffset] = (byte)X86Fixture.Int(row[4]);
                slot[RawSidPool.StateOffset] = (byte)X86Fixture.Int(row[3]);
                return true;
            }
            int arg = X86Fixture.Int(row[2]);
            uint result = 0;
            switch (row[1])
            {
                case "Reset":
                    pool!.Reset();
                    break;
                case "Allocate":
                    result = (uint)pool!.Allocate((SidAllocation)arg);
                    break;
                case "Release":
                    pool!.Release(arg);
                    break;
                case "Rebuild":
                    pool!.RebuildServer();
                    break;
                default:
                    return false;
            }
            steps++;
            uint[] actual =
            [
                result, pool.FreeCount, pool.PredictableCursor, (uint)pool.FirstFree(true), (uint)pool.Tail(true),
                (uint)pool.FirstFree(false), (uint)pool.Tail(false), X86Fixture.Adler32(pool.Bytes), LogAdler(pool),
            ];
            return actual.SequenceEqual(row.Skip(3).Select(X86Fixture.UInt));
        });
        Assert.Equal(8, begins);
        Assert.Equal(1584, steps);
    }

    /// <summary>할당된 슬롯은 전부 0 으로 지워지고 void 로 시작하며, 반납하면 타입 바이트만 남는다.</summary>
    [Fact]
    public void AllocateClearsSlotAndReleaseKeepsOnlyType()
    {
        var pool = new RawSidPool(23300);
        int sid = pool.Allocate();
        Assert.Equal(RawSidPool.ServerFirst, sid);
        Span<byte> slot = pool.AllocatedBytes(sid);
        Assert.Equal(RawSidPool.VoidBit, slot[RawSidPool.StateOffset]);
        Assert.All(slot.ToArray().Where((_, index) => index != RawSidPool.StateOffset), value => Assert.Equal(0, value));
        // 필드를 채우고 반납하면 타입만 남고 나머지는 지워진다
        slot.Fill(0xAB);
        slot[RawSidPool.TypeOffset] = 99;
        slot[RawSidPool.StateOffset] = RawSidPool.VoidBit;
        pool.Release(sid);
        ReadOnlySpan<byte> freed = pool.Slot(sid);
        Assert.Equal(99, freed[RawSidPool.TypeOffset]);
        Assert.Equal(RawSidPool.FreeBit | RawSidPool.DeadBit, freed[RawSidPool.StateOffset]);
        Assert.Equal(1, pool.Deletions(false).Count(record => record.Sid == RawSidPool.ServerFirst && record.Type == 99));
        // 반납한 슬롯과 예측 머리에는 쓸 수 없다
        Assert.Throws<InvalidOperationException>(() => pool.AllocatedBytes(sid));
        Assert.Throws<InvalidOperationException>(() => pool.AllocatedBytes(RawSidPool.PredictableFirst));
    }

    /// <summary>번호 범위와 풀 크기 오류를 거부하고, 서버가 아닌 풀은 서버 번호를 할당하지 않는다.</summary>
    [Fact]
    public void RejectsBadRangesAndMissingServerAuthority()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSidPool(RawSidPool.PredictableFirst + 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSidPool(RawSidPool.MaximumCapacity + 1));
        var pool = new RawSidPool(23300, server: false);
        Assert.Throws<InvalidOperationException>(() => pool.Allocate());
        Assert.Equal(RawSidPool.FirstOrdinary, pool.Allocate(SidAllocation.Client));
        Assert.Throws<ArgumentOutOfRangeException>(() => pool.Slot(23300));
        Assert.Throws<ArgumentOutOfRangeException>(() => pool.Slot(-1));
    }
}
