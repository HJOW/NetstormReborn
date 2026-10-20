using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 사각형 탐색기(<see cref="RawSquidFinder"/>)의 보호 경로와 생애주기 계약을 확인한다.
/// 원본 기대값 재생(X86RawFinderTests)은 탐색 회귀의 커서 상태를 대조하고,
/// 이 검사들은 cpppj/tests/RawFinderTests.cpp 의 보호·생애주기 검사만 10.78 몫으로 옮겼다 (W02 탐색 조각, 2026-10-20).
/// </summary>
public sealed class RawFinderTests
{
    /// <summary>관찰 대상 번호들.</summary>
    private static readonly int[] Ids = [50, 60, 61, 62, 63, 64];

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Write(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>번호 50~64번을 클라이언트 영역에 확보한 풀·해시·타입·높이를 만든다.</summary>
    private static (RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, int[] heights) Scene()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호를 클라이언트 영역에 확보한다
        for (int id = 5; id <= 64; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        var hash = new RawSquidHash();
        var types = new OriginalTypeRecord[256];
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        return (pool, hash, types, new int[256]);
    }

    /// <summary>생애주기 설명을 만든다. 60·63번은 75번, post 면 60번은 77번·64번은 76번 타입이다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">공간 해시.</param>
    /// <param name="types">타입 표.</param>
    /// <param name="heights">높이 확장.</param>
    /// <param name="post">참이면 파괴 뒤 타입으로 바꾼다.</param>
    /// <param name="reverse">참이면 번호를 거꾸로 심는다.</param>
    private static void Lifecycle(RawSidPool pool, RawSquidHash hash, OriginalTypeRecord[] types, int[] heights, bool post, bool reverse)
    {
        int[] ids = reverse ? [.. Ids.Reverse()] : Ids;
        var parts = new List<string>();
        // 같은 버킷의 체인·콜백 순서에 따라 심는 순서를 바꾼다
        foreach (int sid in ids)
        {
            int type = (sid == 60 || sid == 63) ? 75 : 74;
            if (post)
            {
                if (sid == 60)
                {
                    type = 77;
                }
                else if (sid == 64)
                {
                    type = 76;
                }
            }
            parts.Add($"{sid},{type},{(sid == 61 ? 2 : 0)},0,{BitConverter.SingleToUInt32Bits(20.75f)},{BitConverter.SingleToUInt32Bits(21.9f)},1,1,0,{(sid == 50 ? 0 : 1)}");
        }
        hash.Reset();
        Array.Clear(heights);
        // 각 객체의 슬롯·타입·해시 머리를 채운다
        foreach (string entry in string.Join(';', parts).Split(';'))
        {
            string[] fields = entry.Split(',');
            uint[] values = fields.Select(uint.Parse).ToArray();
            Span<byte> raw = pool.AllocatedBytes((int)values[0]);
            raw.Clear();
            raw[RawSidPool.TypeOffset] = (byte)values[1];
            raw[RawSidPool.StateOffset] = (byte)values[2];
            raw[40] = (byte)values[3];
            Write(raw, 14, values[4], 4);
            Write(raw, 18, values[5], 4);
            types[values[1]].FootX = (int)values[6];
            types[values[1]].FootY = (int)values[7];
            heights[values[1]] = (int)values[8];
            ref ushort head = ref hash.Bucket((int)values[9], BitConverter.UInt32BitsToSingle(values[4]), BitConverter.UInt32BitsToSingle(values[5]));
            Write(raw, 4, head, 2);
            head = (ushort)values[0];
        }
        types[74].Flags2 = 4;
        types[75].Flags2 = 0;
        types[76].Flags2 = 0x10004;
        types[77].Flags2 = 0x10000;
        Write(pool.AllocatedBytes(60), 12, 61, 2);
        Write(pool.AllocatedBytes(60), 8, 62, 2);
        Write(pool.AllocatedBytes(63), 12, 60, 2);
        Write(pool.AllocatedBytes(63), 8, 62, 2);
    }

    /// <summary>
    /// 발자국이 깨지면(가로 0·세로 0) 조회 전에 거부한다.
    /// </summary>
    [Fact]
    public void FootprintValidationRejectsBadFeet()
    {
        var (pool, hash, types, heights) = Scene();
        Lifecycle(pool, hash, types, heights, false, false);
        types[74].FootX = 0;
        var finder = new RawSquidFinder(pool, hash, types);
        // 첫 후보의 발자국 검사에서 멈춘다
        Assert.ThrowsAny<Exception>(() => finder.Begin(new SquidSearchArea(20, 21, 20, 21)));
    }

    /// <summary>
    /// 타입 번호가 표를 벗어나면 sid 범위로 거부한다.
    /// </summary>
    [Fact]
    public void TypeRangeRejectsOutOfTableNumber()
    {
        var (pool, hash, _, _) = Scene();
        var types = new OriginalTypeRecord[100];
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        Span<byte> raw = pool.AllocatedBytes(50);
        raw.Clear();
        raw[RawSidPool.TypeOffset] = 150;
        raw[RawSidPool.StateOffset] = 4;
        Write(raw, 14, BitConverter.SingleToUInt32Bits(20.75f), 4);
        Write(raw, 18, BitConverter.SingleToUInt32Bits(21.9f), 4);
        hash.Bucket(0, 20.75f, 21.9f) = (ushort)50;
        var finder = new RawSquidFinder(pool, hash, types);
        // 명시 범위 검사가 sid 이름으로 보고한다
        var error = Assert.Throws<ArgumentOutOfRangeException>(() => finder.Begin(new SquidSearchArea(20, 21, 20, 21)));
        Assert.Equal("sid", error.ParamName);
    }

    /// <summary>
    /// 좌표가 보드 밖이면 후보에서 제외한다. 절삭 전 좌표로 판정하므로 큰 좌표도 걸러진다.
    /// </summary>
    [Fact]
    public void OutOfBoardCoordinatesAreSkipped()
    {
        var (pool, hash, types, heights) = Scene();
        Lifecycle(pool, hash, types, heights, false, false);
        Write(pool.AllocatedBytes(50), 14, BitConverter.SingleToUInt32Bits(1000.5f), 4);
        var visited = new List<int>();
        var finder = new RawSquidFinder(pool, hash, types);
        for (int current = finder.Begin(new SquidSearchArea(20, 21, 20, 21), 0, sid => { visited.Add(sid); return true; });
            current != 0;
            current = finder.Next())
        {
        }
        Assert.DoesNotContain(50, visited);
    }

    /// <summary>
    /// 시작하지 않은 호출·지원 밖 플래그·방향을 잃은 체인은 쓰기 전에 거부한다.
    /// </summary>
    [Fact]
    public void RejectsUnstartedBrokenChainAndUnsupportedFlags()
    {
        var (pool, hash, types, heights) = Scene();
        Lifecycle(pool, hash, types, heights, false, false);
        var finder = new RawSquidFinder(pool, hash, types);
        // 시작 전 Next 와 지원 밖 플래그는 거부한다
        Assert.ThrowsAny<Exception>(() => finder.Next());
        Assert.ThrowsAny<Exception>(() => finder.Begin(default, 8));
        Assert.ThrowsAny<Exception>(() => finder.Begin(default, 4));
        // 자기 순환은 순환 호출에서 거부한다
        Write(pool.AllocatedBytes(50), 4, 50, 2);
        Assert.Equal(50, finder.Begin(new SquidSearchArea(20, 21, 20, 21)));
        Assert.ThrowsAny<Exception>(() => finder.Next());
        // 풀 용량을 넘긴 머리 번호도 거부한다
        hash.Cell(0, 20, 21) = 65535;
        var rangeError = Assert.Throws<ArgumentOutOfRangeException>(() => finder.Begin(new SquidSearchArea(20, 21, 20, 21)));
        Assert.Equal("Current", rangeError.ParamName);
    }

    /// <summary>
    /// 필터는 캐시한 next 를 쓰고 살아 있는 후보를 받는다.
    /// 필터가 끊은 번호는 건너뛰고 다음 머리로 이어간다.
    /// </summary>
    [Fact]
    public void FilterUsesCachedNextAndLiveCandidates()
    {
        var (pool, hash, types, heights) = Scene();
        Lifecycle(pool, hash, types, heights, false, false);
        // 60번 뒤에 63번을 잇고 63번 뒤를 끊는다
        hash.Cell(0, 20, 21) = 60;
        Write(pool.AllocatedBytes(60), 4, 63, 2);
        Write(pool.AllocatedBytes(63), 4, 0, 2);
        var finder = new RawSquidFinder(pool, hash, types);
        var visited = new List<int>();
        int result = finder.Begin(new SquidSearchArea(20, 21, 20, 21), 0, sid =>
        {
            visited.Add(sid);
            if (sid == 60)
            {
                Write(pool.AllocatedBytes(60), 4, 0, 2);
                return false;
            }
            return true;
        });
        Assert.Equal(63, result);
        Assert.Equal(new[] { 60, 63 }, visited);
        // 다음 버킷 머리의 살아 있는 번호를 바로 받는다
        hash.Cell(0, 21, 21) = 62;
        Assert.Equal(62, finder.Next());
    }

    /// <summary>
    /// 조회는 raw 풀과 해시 머리를 바꾸지 않는다. 0 좌표 객체는 건너뛴다.
    /// </summary>
    [Fact]
    public void CoordinateReadOnlyPoolContract()
    {
        var (pool, hash, types, heights) = Scene();
        Lifecycle(pool, hash, types, heights, false, false);
        byte[] before = pool.Bytes.ToArray();
        var heads = new List<ushort[]>();
        // 조회 전 네 단계의 버킷 머리를 모두 보관한다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            heads.Add(hash.Entries(level).ToArray());
        }
        var finder = new RawSquidFinder(pool, hash, types);
        for (int current = finder.Begin(new SquidSearchArea(20, 21, 20, 21)); current != 0; current = finder.Next())
        {
        }
        Assert.Equal(before, pool.Bytes.ToArray());
        // 풀을 바꾸지 않았으므로 해시 머리도 그대로 둔다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            Assert.Equal(heads[level], hash.Entries(level).ToArray());
        }
        // 0 좌표 객체는 건너뛴다
        Write(pool.AllocatedBytes(50), 14, 0, 4);
        Assert.NotEqual(50, finder.Begin(new SquidSearchArea(20, 21, 20, 21)));
    }
}
