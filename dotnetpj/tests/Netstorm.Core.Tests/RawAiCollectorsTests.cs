// AI 수집자 확보·배치의 분기·난수·목록 순서를 실제 메모리·슬롯으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 수집자를 검사하는 클래스다. 2026-11-19 0.655에서 R4-5 AI 수집용으로 추가했다.
// 근거: cpppj o/RawAiCollectors.cpp의 Ensure·PlaceInHome·DeployWithNotice·HasFlying·Vulnerable이다.
public sealed class RawAiCollectorsTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.655에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>본체 필드를 읽는다. 0.655에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.655에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.655에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>수집자 장면을 만든다. 0.655에서 추가했다.</summary>
    private static (RawSidPool Pool, RawAiBrainMemory Memory, uint Token, OriginalTypeRecord[] Types) BuildScene(uint seed = 1u)
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var memory = new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear());
        uint token = memory.Allocate(RawAiBrainConstructor.BytesCount);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }

        return (pool, memory, token, types);
    }

    /// <summary>슬롯을 할당하고 0으로 채운다. 0.655에서 추가했다.</summary>
    private static int AllocateZero(RawSidPool pool)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(sid).Fill(0);
        return sid;
    }

    /// <summary>목록 블록을 본체에 연결한다. 0.655에서 추가했다.</summary>
    private static void BindList(RawAiBrainMemory memory, uint token, uint offset, uint capacity, uint count = 0)
    {
        uint block = memory.Allocate(unchecked((int)capacity) * 4);
        Set(memory, token, offset, block);
        Set(memory, token, offset + 4, capacity);
        Set(memory, token, offset + 8, count);
    }

    /// <summary>기본 전역을 만든다. 0.655에서 추가했다.</summary>
    private static AiCollectorsGlobals BaseGlobals(double now = 10.0) => new()
    {
        Now = () => now,
        LocalOwner = () => 1u,
        InvalidGraph = () => 99u,
    };

    /// <summary>실패하면 안 되는 갈고리를 만든다. 0.655에서 추가했다.</summary>
    private static AiCollectorsHooks StrictHooks() => new()
    {
        Product = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        CanAfford = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
        EnemyAvailable = _ => throw new InvalidOperationException("호출되면 안 된다"),
        MayPlace = _ => throw new InvalidOperationException("호출되면 안 된다"),
        Deploy = (_, _, _, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Log = (_, _, _, _) => throw new InvalidOperationException("호출되면 안 된다"),
        DropSound = () => throw new InvalidOperationException("호출되면 안 된다"),
    };

    /// <summary>신전이 없으면 아무 것도 하지 않고 난수도 소비하지 않는다. 0.655에서 추가했다.</summary>
    [Fact]
    public void EnsureSkipsWithoutTemple()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        var random = new GameRandom(1u);
        uint before = random.State();
        var body = new RawAiCollectors(memory, pool, types, random, BaseGlobals(), StrictHooks());
        body.Ensure(token);
        Assert.Equal(before, random.State());
    }

    /// <summary>일반 경로는 생산·비용·배치·로그·소리를 순서대로 실행한다. 0.655에서 추가했다.</summary>
    [Fact]
    public void EnsureDeploysThroughNormalPath()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        int product = AllocateZero(pool);
        var random = new GameRandom(1u);
        var deployed = new List<(uint Token, uint Type, float X, float Y)>();
        var logged = new List<(uint Token, uint Type, int X, int Y)>();
        int sounded = 0;
        var queries = new List<CanonPlacementQuery>();
        var hooks = StrictHooks();
        hooks.Product = (_, type) => type == 60 ? product : 0;
        hooks.CanAfford = (_, _) => true;
        hooks.MayPlace = query => { queries.Add(query); return true; };
        hooks.Deploy = (brain, type, x, y, _, _) => deployed.Add((brain, type, x, y));
        hooks.Log = (brain, type, x, y) => logged.Add((brain, type, x, y));
        hooks.DropSound = () => sounded++;
        var body = new RawAiCollectors(memory, pool, types, random, BaseGlobals(), hooks);
        BindList(memory, token, 0x284, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x284)), 0, 60u);
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x294, 1u);
        Set(memory, token, 0x298, 5u);
        Set(memory, token, 0x2cc, 0u);
        Set(memory, token, 0x270, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x274, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x278, BitConverter.SingleToUInt32Bits(100.0f));
        Set(memory, token, 0x27c, BitConverter.SingleToUInt32Bits(50.0f));
        Set(memory, token, 0x228, BitConverter.SingleToUInt32Bits(2.0f));
        Set(memory, token, 0xb00, 1u);
        body.Ensure(token);
        Assert.Single(deployed);
        Assert.Equal(token, deployed[0].Token);
        Assert.Equal(60u, deployed[0].Type);
        Assert.InRange(deployed[0].X, 0.0f, 100.0f);
        Assert.InRange(deployed[0].Y, 0.0f, 50.0f);
        Assert.Single(queries);
        Assert.Equal(1u, queries[0].Owner);
        Assert.Single(logged);
        Assert.Equal(1, sounded);
        // 생각 기한 QWORD의 상위 워드가 0이 아니면 예약됐다.
        Assert.NotEqual(0u, Get(memory, token, 0x5a8 + 4));
    }

    /// <summary>대기 조각이면 생산물을 찾지 않고 끝낸다. 0.655에서 추가했다.</summary>
    [Fact]
    public void EnsureSkipsPendingProduct()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        int product = AllocateZero(pool);
        pool.AllocatedBytes(product)[21] = 1;
        var random = new GameRandom(1u);
        var hooks = StrictHooks();
        hooks.Product = (_, _) => product;
        hooks.CanAfford = (_, _) => throw new InvalidOperationException("호출되면 안 된다");
        hooks.MayPlace = _ => throw new InvalidOperationException("호출되면 안 된다");
        hooks.Deploy = (_, _, _, _, _, _) => throw new InvalidOperationException("호출되면 안 된다");
        var body = new RawAiCollectors(memory, pool, types, random, BaseGlobals(), hooks);
        BindList(memory, token, 0x284, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x284)), 0, 60u);
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x294, 1u);
        Set(memory, token, 0x298, 5u);
        Set(memory, token, 0x2cc, 0u);
        body.Ensure(token);
    }

    /// <summary>취약 적 사제이면 비행 수집자를 먼저 배치한다. 0.655에서 추가했다.</summary>
    [Fact]
    public void EnsureDeploysFlyingForVulnerablePriest()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        int priest = AllocateZero(pool);
        Put(pool.AllocatedBytes(priest), 0x1a, 10u);
        int product = AllocateZero(pool);
        types[61].Flags2 = 0x20000u;
        var random = new GameRandom(1u);
        var deployed = new List<uint>();
        var hooks = StrictHooks();
        hooks.Product = (_, type) => type == 61 ? product : 0;
        hooks.EnemyAvailable = _ => true;
        hooks.MaxHitPoints = _ => 100;
        hooks.MayPlace = _ => true;
        hooks.Deploy = (_, type, _, _, _, _) => deployed.Add(type);
        hooks.DropSound = () => deployed.Add(0xFFFFFFFFu);
        var body = new RawAiCollectors(memory, pool, types, random, BaseGlobals(), hooks);
        BindList(memory, token, 0x284, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x284)), 0, 61u);
        Set(memory, token, 0x260, 1u);
        Set(memory, token, 0x294, 1u);
        Set(memory, token, 0x298, 5u);
        Set(memory, token, 0x28c, 1u);
        Set(memory, token, 0x270, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x274, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x278, BitConverter.SingleToUInt32Bits(100.0f));
        Set(memory, token, 0x27c, BitConverter.SingleToUInt32Bits(50.0f));
        Set(memory, token, 0x248, 0x100u);
        Set(memory, token, 0x38c, 0u);
        Set(memory, token, 0x388, 1u);
        Set(memory, token, 0x2a0, unchecked((uint)priest));
        Set(memory, token, 0x2cc, 1u);
        body.Ensure(token);
        Assert.Equal([61u, 0xFFFFFFFFu], deployed);
    }

    /// <summary>수집자 목록의 비행 genus를 판정한다. 0.655에서 추가했다.</summary>
    [Fact]
    public void HasFlyingChecksGenus()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        int walker = AllocateZero(pool);
        pool.AllocatedBytes(walker)[10] = 62;
        types[62].Flags2 = 0x20000u;
        var body = new RawAiCollectors(memory, pool, types, new GameRandom(1u), BaseGlobals(), StrictHooks());
        BindList(memory, token, 0x2dc, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x2dc)), 0, unchecked((uint)walker));
        Set(memory, token, 0x2e4, 1u);
        Assert.True(body.HasFlying(token));
        types[62].Flags2 = 0u;
        Assert.False(body.HasFlying(token));
    }

    /// <summary>절반 미만이면 취약하다. 0.655에서 추가했다.</summary>
    [Fact]
    public void VulnerableComparesHalf()
    {
        (RawSidPool pool, _, _, OriginalTypeRecord[] types) = BuildScene();
        int priest = AllocateZero(pool);
        Put(pool.AllocatedBytes(priest), 0x1a, 49u);
        var hooks = StrictHooks();
        hooks.MaxHitPoints = _ => 100;
        var body = new RawAiCollectors(new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear()), pool, types, new GameRandom(1u), BaseGlobals(), hooks);
        Assert.True(body.Vulnerable(priest));
        Put(pool.AllocatedBytes(priest), 0x1a, 50u);
        Assert.False(body.Vulnerable(priest));
    }

    /// <summary>20회 모두 거부되면 배치하지 않는다. 0.655에서 추가했다.</summary>
    [Fact]
    public void PlaceInHomeGivesUpAfterTwenty()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        int sounded = 0;
        var hooks = StrictHooks();
        hooks.MayPlace = _ => false;
        hooks.Deploy = (_, _, _, _, _, _) => throw new InvalidOperationException("호출되면 안 된다");
        var body = new RawAiCollectors(memory, pool, types, new GameRandom(1u), BaseGlobals(), hooks);
        Set(memory, token, 0x260, 2u);
        Set(memory, token, 0x270, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x274, BitConverter.SingleToUInt32Bits(0.0f));
        Set(memory, token, 0x278, BitConverter.SingleToUInt32Bits(100.0f));
        Set(memory, token, 0x27c, BitConverter.SingleToUInt32Bits(50.0f));
        body.PlaceInHome(token, 60u);
        Assert.Equal(0, sounded);
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.655에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, OriginalTypeRecord[] types) = BuildScene();
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => new RawAiCollectors(memory, cdPool, types, new GameRandom(1u), BaseGlobals(), StrictHooks()));
        Assert.Throws<ArgumentException>(() => new RawAiCollectors(memory, pool, [], new GameRandom(1u), BaseGlobals(), StrictHooks()));
        Assert.Throws<ArgumentNullException>(() => new RawAiCollectors(memory, pool, types, new GameRandom(1u), BaseGlobals(), null!));
        var body = new RawAiCollectors(memory, pool, types, new GameRandom(1u), BaseGlobals(), new AiCollectorsHooks());
        BindList(memory, token, 0x284, 8, 1);
        Put(memory.Bytes(Get(memory, token, 0x284)), 0, 60u);
        Set(memory, token, 0x294, 1u);
        Set(memory, token, 0x298, 5u);
        Set(memory, token, 0x28c, 1u);
        Assert.Throws<InvalidOperationException>(() => body.Ensure(token));
        uint small = memory.Allocate(100);
        Assert.Throws<ArgumentException>(() => body.Ensure(small));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Vulnerable(PoolCapacity + 1));
    }
}
