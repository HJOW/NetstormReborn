using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// SID 풀 정리(RawSquidPoolReset·ClearDeletionHistory·공유 서버 판정)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/SquidPoolResetTests.cpp 와 같다: 같은 실제 할당·반납으로 삭제 기록을 채우고 Reset·정리를 실행한다.
/// 풀 카운터·목록·기록·실패 좌표를 대조한다. 기준 판본은 10.78 이며 CD 행은 C# 패치 범위가 아니라 건너뛴다 (R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class X86RawSquidPoolResetTests
{
    /// <summary>실패 좌표 값(-10000). Native는 Y 다음 X의 두 DWORD만 쓴다.</summary>
    private const uint InvalidCoordinateBits = 0xc61c4000u;

    /// <summary>원본과 같은 실제 할당·반납으로 양쪽 20개 삭제 기록을 채운다.</summary>
    /// <param name="pool">채울 풀.</param>
    private static void FillHistory(RawSidPool pool)
    {
        // client와 other의 실제 서로 다른 번호 영역을 사용한다
        foreach (SidAllocation flags in new[] { SidAllocation.Client, SidAllocation.Server })
        {
            var allocated = new List<int>();
            for (uint index = 0; index < 20; index++)
            {
                int sid = pool.Allocate(flags);
                Span<byte> raw = pool.AllocatedBytes(sid);
                // 파생 생성자의 명시 입력이며 출력 계산이 아니다
                for (int b = 0; b < raw.Length; b++)
                {
                    raw[b] = (byte)(83 + index + (uint)b * 37);
                }
                raw[10] = (byte)(83 + index);
                raw[11] = 4;
                allocated.Add(sid);
            }
            // 반납 순서로 실제 20쌍 이동과 FIFO를 실행한다
            foreach (int sid in allocated)
            {
                pool.Release(sid);
            }
        }
    }

    /// <summary>호스트 패딩을 읽지 않고 실제 SID/type 두 DWORD를 순서대로 직렬화한다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="client">클라이언트 영역이면 참.</param>
    private static byte[] PoolRecords(RawSidPool pool, bool client)
    {
        var bytes = new byte[RawSidPool.DeletionHistory * 8];
        int offset = 0;
        // 각 20쌍의 낮은 바이트부터 읽는다
        foreach (SidDeletion record in pool.Deletions(client))
        {
            SpPut(bytes, offset, record.Sid);
            SpPut(bytes, offset + 4, record.Type);
            offset += 8;
        }
        return bytes;
    }

    /// <summary>실제 하위 Reset의 기존 주요 기록과 부모의 삭제 기록을 같은 열로 관측한다.</summary>
    /// <param name="pool">관측할 풀.</param>
    private static uint[] PoolSnapshot(RawSidPool pool)
    {
        byte[] logs = PoolRecords(pool, true);
        byte[] other = PoolRecords(pool, false);
        var joined = new byte[logs.Length + other.Length];
        logs.CopyTo(joined, 0);
        other.CopyTo(joined, logs.Length);
        return new uint[]
        {
            0, pool.FreeCount, pool.PredictableCursor,
            (uint)pool.FirstFree(true), (uint)pool.Tail(true),
            (uint)pool.FirstFree(false), (uint)pool.Tail(false),
            X86Fixture.Adler32(pool.Bytes), X86Fixture.Adler32(joined),
        };
    }

    /// <summary>명시 32비트 값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">쓸 물리.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void SpPut(Span<byte> bytes, int offset, uint value)
    {
        // 인접 바이트는 보존한다
        for (int b = 0; b < 4; b++)
        {
            bytes[offset + b] = (byte)(value >> (b * 8));
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
    /// 원문 부모·하위 Pool·80DWORD·실패 좌표를 실제 CPU로 먼저 고정한 12사례 중 패치 6개를 재생한다.
    /// </summary>
    [Fact]
    public void SquidPoolResetReplaysBothNativeParentsPatch6()
    {
        string[][] rows = X86Fixture.Read("squid-pool-reset");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(21, row.Length);
            if (row[0] != "originals")
            {
                return true;
            }
            var pool = new RawSidPool(32768, true);
            FillHistory(pool);
            Assert.Equal(row[3], SpHex(PoolRecords(pool, true)));
            Assert.Equal(row[4], SpHex(PoolRecords(pool, false)));
            var invalid = new RawPathPoint
            {
                X = BitConverter.UInt32BitsToSingle(0x3f123456u),
                Y = BitConverter.UInt32BitsToSingle(0xc0123456u),
            };
            int argument = unchecked((int)X86Fixture.UInt(row[1]));
            RawSquidPoolReset.Reset(pool, ref invalid, argument);
            uint[] snapshot = PoolSnapshot(pool);
            for (int column = 0; column < snapshot.Length; column++)
            {
                Assert.Equal(X86Fixture.UInt(row[5 + column]), snapshot[column]);
            }
            Assert.Equal(row[14], SpHex(PoolRecords(pool, true)));
            Assert.Equal(row[15], SpHex(PoolRecords(pool, false)));
            Assert.Equal(X86Fixture.UInt(row[16]), BitConverter.SingleToUInt32Bits(invalid.X));
            Assert.Equal(X86Fixture.UInt(row[17]), BitConverter.SingleToUInt32Bits(invalid.Y));
            // 82store는 Native CPU 감사이며 C# store 계측으로 가장하지 않는다
            Assert.Equal("wrapper,pool,history", row[18]);
            Assert.Equal(81, row[19].Count(c => c == ';'));
            Assert.True(X86Fixture.ULong(row[20]) > 0);
            count++;
            return true;
        });
        Assert.Equal(6, count);
    }

    /// <summary>
    /// 기존 하위 Reset의 기록 보존과 새 부모의 기록 삭제·재확보를 구별한다.
    /// </summary>
    [Fact]
    public void SquidPoolResetLowerPreservesHistoryParentClearsAndReallocates()
    {
        var pool = new RawSidPool(32768, true);
        FillHistory(pool);
        string beforeClient = SpHex(PoolRecords(pool, true));
        string beforeOther = SpHex(PoolRecords(pool, false));
        pool.Reset();
        Assert.Equal(beforeClient, SpHex(PoolRecords(pool, true)));
        Assert.Equal(beforeOther, SpHex(PoolRecords(pool, false)));
        var invalid = new RawPathPoint { X = 11.25f, Y = -7.5f };
        RawSquidPoolReset.Reset(pool, ref invalid, 40000);
        Assert.Equal(40000, pool.Capacity);
        Assert.Equal(39995u, pool.FreeCount);
        Assert.Equal(1u, pool.PredictableCursor);
        Assert.Equal(-10000f, invalid.X);
        Assert.Equal(-10000f, invalid.Y);
        Assert.All(pool.Deletions(true), record => Assert.Equal(new SidDeletion(0, 0), record));
        Assert.All(pool.Deletions(false), record => Assert.Equal(new SidDeletion(0, 0), record));
    }

    /// <summary>
    /// 하위 실패 뒤 꼬리를 앞당기지 않고, 기록 정리만 실행할 때는 다른 Pool 필드를 보존한다.
    /// </summary>
    [Fact]
    public void SquidPoolResetFailureAndHistoryOnlyKeepOtherState()
    {
        var pool = new RawSidPool(32768, true);
        FillHistory(pool);
        uint[] before = PoolSnapshot(pool);
        var invalid = new RawPathPoint { X = 11.25f, Y = -7.5f };
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidPoolReset.Reset(pool, ref invalid, 5));
        Assert.Equal(before, PoolSnapshot(pool));
        Assert.Equal(11.25f, invalid.X);
        Assert.Equal(-7.5f, invalid.Y);
        pool.ClearDeletionHistory();
        uint[] after = PoolSnapshot(pool);
        // 기록 80DWORD 외의 현재 Pool·좌표는 그대로다
        for (int index = 0; index < 8; index++)
        {
            Assert.Equal(before[index], after[index]);
        }
        Assert.Equal(11.25f, invalid.X);
        Assert.Equal(-7.5f, invalid.Y);
    }

    /// <summary>
    /// 공유 서버 판정이 연결되면 생성자 입력을 대신 읽는다. 판정이 바뀌면 할당 권한도 바로 바뀐다.
    /// </summary>
    [Fact]
    public void SquidPoolSharedServerBindingSwitchesAuthority()
    {
        bool flag = false;
        var pool = new RawSidPool(32768, false, () => flag);
        Assert.False(pool.IsServer);
        Assert.Throws<InvalidOperationException>(() => pool.Allocate(SidAllocation.Server));
        flag = true;
        Assert.True(pool.IsServer);
        int sid = pool.Allocate(SidAllocation.Server);
        Assert.True(sid >= RawSidPool.ServerFirst);
        var compact = new SidPool(23300, false, sharedServer: () => flag);
        flag = false;
        Assert.False(compact.IsServer);
        Assert.Throws<InvalidOperationException>(() => compact.Allocate(SidAllocation.Server));
    }
}
