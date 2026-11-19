// AI 그래프 조회(SID·좌표→그래프 번호)의 분기·순서를 실제 풀·표면으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 그래프 조회를 검사하는 클래스다. 2026-11-19 0.657에서 R4-5 AI 그래프용으로 추가했다.
// 근거: cpppj o/RawPlayerPlacementAnchor.cpp의 GraphAt·GraphFor(원문 00462b80·00462bc0)다.
public sealed class RawAiGraphTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.657에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.657에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>그래프 장면을 만든다. 0.657에서 추가했다.</summary>
    private static (RawSidPool Pool, OriginalTypeRecord[] Types, ushort[] Surfaces) BuildScene(bool patch = true, int typeCount = 188)
    {
        var pool = new RawSidPool(PoolCapacity, false, null, patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        var types = new OriginalTypeRecord[typeCount];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }

        return (pool, types, new ushort[65536]);
    }

    /// <summary>기본 갈고리를 만든다. 0.657에서 추가했다.</summary>
    private static AiGraphHooks BaseHooks(List<string>? asserted = null) => new()
    {
        GraphReady = () => true,
        FenceType = () => 136u,
        WindArcherType = () => 137u,
        GraphAssertion = (a, b, c) => asserted?.Add(a + ":" + b + ":" + c),
        Contained = new ContainedFinderState(),
    };

    /// <summary>슬롯을 할당하고 0으로 채운다. 0.657에서 추가했다.</summary>
    private static int AllocateZero(RawSidPool pool)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(sid).Fill(0);
        return sid;
    }

    /// <summary>SID 0은 진단 뒤 0을 돌려준다. 0.657에서 추가했다.</summary>
    [Fact]
    public void ZeroSidReturnsZero()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, ushort[] surfaces) = BuildScene();
        var asserted = new List<string>();
        var finder = new ContainedFinderState();
        var checking = new RawCheckingFlag();
        checking.Bind(() => true);
        finder.CheckingDead = checking;
        finder.Assertion = (_, _, _) => { };
        var hooks = BaseHooks(asserted);
        hooks.Contained = finder;
        var body = new RawAiGraph(pool, types, surfaces, hooks);
        Assert.Equal(0u, body.GraphFor(0));
        Assert.Single(asserted);
    }

    /// <summary>표면 타입은 자신의 raw 번호를 쓴다. 0.657에서 추가했다.</summary>
    [Fact]
    public void SurfaceTypeUsesOwnByte()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, ushort[] surfaces) = BuildScene();
        types[40].Flags1 = 0x800u;
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 40;
        pool.AllocatedBytes(sid)[30] = 12;
        var body = new RawAiGraph(pool, types, surfaces, BaseHooks());
        Assert.Equal(12u, body.GraphFor(unchecked((uint)sid)));
    }

    /// <summary>비표면은 좌표의 표면 sid 번호를 쓴다. 0.657에서 추가했다.</summary>
    [Fact]
    public void NonSurfaceUsesSurfaceMap()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, ushort[] surfaces) = BuildScene();
        types[41].Flags1 = 0u;
        int unit = AllocateZero(pool);
        pool.AllocatedBytes(unit)[10] = 41;
        Put(pool.AllocatedBytes(unit), 14, BitConverter.SingleToUInt32Bits(10.2f));
        Put(pool.AllocatedBytes(unit), 18, BitConverter.SingleToUInt32Bits(20.8f));
        types[42].Flags1 = 0x800u;
        int ground = AllocateZero(pool);
        pool.AllocatedBytes(ground)[10] = 42;
        pool.AllocatedBytes(ground)[30] = 9;
        surfaces[21 * 256 + 11] = unchecked((ushort)ground);
        var body = new RawAiGraph(pool, types, surfaces, BaseHooks());
        Assert.Equal(9u, body.GraphFor(unchecked((uint)unit)));
    }

    /// <summary>표면·펜스·윈드아처가 아니면 거부한다. 0.657에서 추가했다.</summary>
    [Fact]
    public void UnsupportedTypeThrows()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, ushort[] surfaces) = BuildScene();
        types[43].Flags1 = 0u;
        int sid = AllocateZero(pool);
        pool.AllocatedBytes(sid)[10] = 43;
        // 표면 지도에서 해당 sid를 만나면 표 검사가 동작한다. 5.2·6.8은 절삭 뒤 (6, 7)이다.
        surfaces[7 * 256 + 6] = unchecked((ushort)sid);
        var body = new RawAiGraph(pool, types, surfaces, BaseHooks());
        Assert.Throws<ArgumentException>(() => body.GraphAt(5.2f, 6.8f));
    }

    /// <summary>비활성·범위 밖·빈 표면은 254를 돌려준다. 0.657에서 추가했다.</summary>
    [Fact]
    public void InactiveOrMissingReturnsInvalid()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, ushort[] surfaces) = BuildScene();
        var hooks = BaseHooks();
        hooks.GraphReady = () => false;
        var body = new RawAiGraph(pool, types, surfaces, hooks);
        Assert.Equal(254u, body.GraphAt(float.NaN, 0.0f));
        hooks.GraphReady = () => true;
        Assert.Equal(254u, body.GraphAt(300.0f, 10.0f));
        Assert.Equal(254u, body.GraphAt(10.0f, 10.0f));
    }

    /// <summary>계약 위반·미연결·범위 밖은 거부한다. 0.657에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, OriginalTypeRecord[] types, ushort[] surfaces) = BuildScene();
        Assert.Throws<ArgumentException>(() => new RawAiGraph(pool, new OriginalTypeRecord[10], surfaces, BaseHooks()));
        Assert.Throws<ArgumentException>(() => new RawAiGraph(pool, types, new ushort[10], BaseHooks()));
        Assert.Throws<ArgumentNullException>(() => new RawAiGraph(pool, types, surfaces, null!));
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        var cdTypes = new OriginalTypeRecord[171];
        for (int i = 0; i < cdTypes.Length; i++)
        {
            cdTypes[i] = new OriginalTypeRecord();
        }

        var cd = new RawAiGraph(cdPool, cdTypes, surfaces, BaseHooks());
        int sid = cdPool.Allocate(SidAllocation.Client);
        cdPool.AllocatedBytes(sid).Fill(0);
        cdTypes[0].Flags1 = 0x800u;
        cdPool.AllocatedBytes(sid)[28] = 7;
        Assert.Equal(7u, cd.GraphFor(unchecked((uint)sid)));
        var body = new RawAiGraph(pool, types, surfaces, new AiGraphHooks());
        Assert.Throws<InvalidOperationException>(() => body.GraphAt(10.0f, 10.0f));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.GraphFor(unchecked((uint)PoolCapacity) + 1));
    }
}
