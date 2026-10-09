using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 오브젝트 번호 할당기를 원본 x86 기대값(sid-x86.tsv)에 대조한다 (계획 4-2). 열 구성은 cpppj/tests/SidTests.cpp 와 같다.
/// 기대값은 반환 번호·카운터·목록 머리/꼬리와 "원본 풀 전체 바이트"·삭제 기록의 Adler-32 다.
/// C# 할당기는 원본 슬롯을 복제하지 않으므로, 이 검사가 번호마다의 다음 번호·타입·상태에서 원본 슬롯 바이트를 되살려 체크섬을 비교한다.
/// </summary>
public sealed class X86SidTests
{
    /// <summary>패치판 10.78 행의 판본 이름. 그 밖은 CD·10.37 이다</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 의 슬롯 한 개 바이트 수</summary>
    private const int PatchStride = 50;

    /// <summary>CD 판의 슬롯 한 개 바이트 수 (비교 자료)</summary>
    private const int CdStride = 36;

    /// <summary>CD 판의 서버 영역 첫 번호와 예측 머리 (비교 자료)</summary>
    private const int CdServerFirst = 6000, CdPredictableFirst = 14000;

    /// <summary>원본 슬롯에서 다음 번호(16비트)·타입·상태가 놓인 위치</summary>
    private const int NextOffset = 4, TypeOffset = 10, StateOffset = 11;

    /// <summary>풀 +14 에 겹쳐 놓인 서버 목록 머리의 다음 번호 위치 (0번 슬롯 안)</summary>
    private const int ServerHeadOffset = 14 + NextOffset;

    /// <summary>Adler-32 의 나머지 연산 값과, 나머지를 미뤄도 32비트가 넘치지 않는 최대 묶음 길이</summary>
    private const uint AdlerPrime = 65521, AdlerBlock = 5552;

    /// <summary>Python zlib 와 같은 Adler-32 (긴 버퍼용: 묶음마다 한 번만 나머지를 구한다).</summary>
    private static uint Adler(ReadOnlySpan<byte> bytes)
    {
        uint a = 1;
        uint b = 0;
        // 묶음 단위로 누적한다
        while (bytes.Length > 0)
        {
            int length = (int)Math.Min(AdlerBlock, (uint)bytes.Length);
            // 한 묶음의 바이트
            foreach (byte value in bytes[..length])
            {
                a += value;
                b += a;
            }
            a %= AdlerPrime;
            b %= AdlerPrime;
            bytes = bytes[length..];
        }
        return (b << 16) | a;
    }

    /// <summary>두 삭제 기록(클라이언트, 서버 순서)을 리틀 엔디언 32비트 두 개씩 늘어놓아 체크섬을 낸다.</summary>
    private static uint LogAdler(SidPool pool)
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
        return Adler(bytes.ToArray());
    }

    /// <summary>
    /// 할당기의 값에서 원본 풀 바이트를 되살린다. 빈 슬롯은 다음 번호·타입·상태만 있고 나머지는 0 이다.
    /// 합성 생성자 입력(Fill)을 받은 할당 슬롯은 그 바이트가 그대로 있고 타입·상태만 할당기 값이다.
    /// 0번 슬롯에는 서버 목록 머리가 겹쳐 있다.
    /// </summary>
    private static byte[] RawPool(SidPool pool, int stride, Dictionary<int, byte[]> payloads, ref byte[] buffer)
    {
        if (buffer.Length != pool.Capacity * stride)
        {
            buffer = new byte[pool.Capacity * stride];
        }
        byte[] bytes = buffer;
        Array.Clear(bytes);
        // 번호마다 슬롯 하나를 채운다
        for (int sid = 0; sid < pool.Capacity; sid++)
        {
            Span<byte> slot = bytes.AsSpan(sid * stride, stride);
            if (payloads.TryGetValue(sid, out byte[]? payload))
            {
                payload.CopyTo(slot);
            }
            else
            {
                int next = pool.Next(sid);
                slot[NextOffset] = (byte)next;
                slot[NextOffset + 1] = (byte)(next >> 8);
            }
            slot[TypeOffset] = pool.Type(sid);
            slot[StateOffset] = pool.State(sid);
        }
        bytes[ServerHeadOffset] = (byte)pool.ServerHead;
        bytes[ServerHeadOffset + 1] = (byte)(pool.ServerHead >> 8);
        return bytes;
    }

    /// <summary>
    /// 두 판본 × 풀 크기 4종 × 서버/클라이언트의 16개 세션, 3,168개 전이.
    /// C# 의 기준은 10.78 영역(15000·23001)이다. CD·10.37 세션은 그 판본의 영역 경계를 인자로 주어 같은 규칙을 대조한다.
    /// </summary>
    [Fact]
    public void Transitions_MatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("sid");
        Assert.Equal(16, rows.Count(row => row[0] == "Begin"));
        Assert.Equal(3168, rows.Count(row => row[0] == "Step"));
        SidPool? pool = null;
        int stride = PatchStride;
        var payloads = new Dictionary<int, byte[]>();
        // 원본 풀 바이트를 되살리는 버퍼는 세션 안에서 다시 쓴다
        byte[] buffer = [];
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                bool patch = row[1] == PatchEdition;
                stride = patch ? PatchStride : CdStride;
                pool = patch ? new SidPool(X86Fixture.Int(row[2]), row[3] == "1")
                    : new SidPool(X86Fixture.Int(row[2]), row[3] == "1", CdServerFirst, CdPredictableFirst);
                payloads.Clear();
                return true;
            }
            if (row[0] == "Fill")
            {
                // 합성 생성자 입력: 슬롯 전체를 seed + i × 37 로 채우고 타입·상태를 적는다 (기대값을 만드는 계산이 아니다)
                int sid = X86Fixture.Int(row[1]);
                int seed = X86Fixture.Int(row[2]);
                payloads[sid] = Enumerable.Range(0, stride).Select(i => (byte)(seed + i * 37)).ToArray();
                pool!.SetType(sid, (byte)X86Fixture.Int(row[4]));
                pool.SetState(sid, (byte)X86Fixture.Int(row[3]));
                return true;
            }
            int arg = X86Fixture.Int(row[2]);
            uint result = 0;
            switch (row[1])
            {
                case "Reset":
                    pool!.Reset();
                    payloads.Clear();
                    break;
                case "Allocate":
                    result = (uint)pool!.Allocate((SidAllocation)arg);
                    break;
                case "Release":
                    pool!.Release(arg);
                    payloads.Remove(arg);
                    break;
                case "Rebuild":
                    pool!.RebuildServer();
                    break;
                default:
                    return false;
            }
            uint[] actual =
            [
                result, pool.FreeCount, pool.PredictableCursor, (uint)pool.FirstFree(true), (uint)pool.Tail(true),
                (uint)pool.FirstFree(false), (uint)pool.Tail(false), Adler(RawPool(pool, stride, payloads, ref buffer)), LogAdler(pool),
            ];
            return actual.SequenceEqual(row.Skip(3).Select(X86Fixture.UInt));
        });
    }

    /// <summary>마지막 빈 번호는 예약 꼬리로 남고, 반납한 번호는 선입선출의 꼬리로 들어간다.</summary>
    [Fact]
    public void OrdinaryExhaustion_KeepsReservedTailAndFifo()
    {
        var pool = new SidPool(23300);
        uint initial = pool.FreeCount;
        // 클라이언트 영역 5~14998 까지만 할당할 수 있다 (14999 는 예약 꼬리)
        for (int sid = SidPool.FirstOrdinary; sid < SidPool.ServerFirst1078 - 1; sid++)
        {
            Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        }
        Assert.Equal((14999, 14999), (pool.FirstFree(true), pool.Tail(true)));
        Assert.Throws<InvalidOperationException>(() => pool.Allocate(SidAllocation.Client));
        pool.SetType(8, 77);
        pool.Release(8);
        Assert.Equal((8, 77, SidPool.FreeBit | SidPool.DeadBit), (pool.Tail(true), pool.Type(8), pool.State(8)));
        // 이전 예약 꼬리가 다음 번호가 되고 방금 반납한 번호가 새 예약 꼬리다
        Assert.Equal(14999, pool.Allocate(SidAllocation.Client));
        Assert.Equal(8, pool.FirstFree(true));
        Assert.Throws<InvalidOperationException>(() => pool.Allocate(SidAllocation.Client));
        Assert.Equal(initial - 14994, pool.FreeCount);
    }

    /// <summary>
    /// 싱글 플레이(서버)의 일반 할당은 15000 부터 번호순이다. 다리 붕괴 스캔이 훑는 범위와 같은 영역이다.
    /// 예측 번호는 커서로만 진행하고 예측 비트가 클라이언트 비트보다 우선한다. 서버는 마지막 예측 번호를 남긴다.
    /// </summary>
    [Fact]
    public void ServerAndPredictableRanges_FollowOriginalBoundaries()
    {
        var server = new SidPool(23005);
        Assert.Equal((15000, 15001), (server.Allocate(), server.Allocate()));
        int a = server.Allocate(SidAllocation.Predictable | SidAllocation.Client);
        int b = server.Allocate(SidAllocation.Predictable);
        Assert.Equal((23002, 23003), (a, b));
        server.Release(a);
        Assert.Equal(SidPool.FreeBit | SidPool.DeadBit, server.State(a));
        Assert.Throws<InvalidOperationException>(() => server.Allocate(SidAllocation.Predictable));
        Assert.Equal(3u, server.PredictableCursor);
        var client = new SidPool(23004, false);
        int first = client.Allocate(SidAllocation.Predictable);
        Assert.Equal(23002, first);
        uint count = client.FreeCount;
        client.Release(first);
        Assert.Equal((SidPool.FreeBit, count), (client.State(first), client.FreeCount));
        Assert.Equal(23003, client.Allocate(SidAllocation.Predictable));
        Assert.Throws<InvalidOperationException>(() => client.Allocate(SidAllocation.Predictable));
        Assert.Throws<InvalidOperationException>(() => client.Allocate());
    }

    /// <summary>서버 목록 재구성은 빈 번호 카운터를 다시 세지 않고 빈 서버 번호 수를 더한다. 목록은 번호순이 된다.</summary>
    [Fact]
    public void RebuildServer_AddsToCounterAndOrdersByNumber()
    {
        var pool = new SidPool(23300);
        int a = pool.Allocate();
        int b = pool.Allocate();
        Assert.Equal((15000, 15001), (a, b));
        pool.Release(a);
        uint count = pool.FreeCount;
        pool.RebuildServer();
        Assert.Equal((15000, 23000), (pool.FirstFree(false), pool.Tail(false)));
        Assert.Equal(count + 8000, pool.FreeCount);
        Assert.Equal(15000, pool.Allocate());
        Assert.Equal(15002, pool.Allocate());
    }

    /// <summary>반납하면 타입 번호만 남고, 삭제 기록은 초기화 뒤에도 남는다.</summary>
    [Fact]
    public void Release_KeepsTypeAndDeletionHistory()
    {
        var pool = new SidPool(23300);
        int sid = pool.Allocate();
        pool.SetType(sid, 123);
        pool.SetState(sid, SidPool.VoidBit | SidPool.DeadBit);
        pool.Release(sid);
        Assert.Equal((123, SidPool.FreeBit | SidPool.DeadBit, 0), (pool.Type(sid), pool.State(sid), pool.Next(sid)));
        Assert.Equal(new SidDeletion((uint)sid, 123), pool.Deletions(false)[0]);
        pool.Reset();
        Assert.Equal(123u, pool.Deletions(false)[0].Type);
        Assert.Equal(0, pool.Type(sid));
    }

    /// <summary>잘못된 크기·빈 번호 반납·머리 번호 쓰기·범위 밖 번호·void 가 아닌 번호의 반납은 거부한다.</summary>
    [Fact]
    public void InvalidInput_IsRejected()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new SidPool(23002));
        Assert.Throws<ArgumentOutOfRangeException>(() => new SidPool(SidPool.MaximumCapacity + 1));
        var pool = new SidPool(23300);
        Assert.Throws<InvalidOperationException>(() => pool.Release(5));
        Assert.Throws<InvalidOperationException>(() => pool.SetType(0, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => pool.State(23300));
        int sid = pool.Allocate(SidAllocation.Client);
        pool.SetState(sid, 0);
        Assert.Throws<InvalidOperationException>(() => pool.Release(sid));
        Assert.Equal(0, pool.State(sid));
    }
}
