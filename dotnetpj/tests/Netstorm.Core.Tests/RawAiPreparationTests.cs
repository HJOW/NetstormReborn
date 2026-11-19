// AI 준비(적 재선택·데모 기한·취약 사제 목표·SP·최초 등록·수량 판정)의 분기·순서를 실제 메모리로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 준비 본체를 검사하는 클래스다. 2026-11-19 0.645에서 R4-5 AI 초기화용으로 추가했다.
// 근거: cpppj o/RawAiPreparation.cpp의 Update·SelectEnemy·CountAttachedGeysers(원문 00416eb0)다.
public sealed class RawAiPreparationTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.645에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>같은 Player9 기록의 바이트 수다. 0.645에서 추가했다.</summary>
    private const int PlayerBytes = 9 * 180;

    /// <summary>같은 방향별 동맹 칸 수다. 0.645에서 추가했다.</summary>
    private const int AllianceCells = 81;

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.645에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>AI 준비 장면을 만든다. 0.645에서 추가했다.</summary>
    private static (RawAiBrainMemory Memory, RawSidPool Pool, uint Token, byte[] Players, uint[] Allies) BuildScene()
    {
        var memory = new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear());
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        uint token = memory.Allocate(RawAiBrainConstructor.BytesCount);
        return (memory, pool, token, new byte[PlayerBytes], new uint[AllianceCells]);
    }

    /// <summary>실패하면 안 되는 갈고리에 쓰는 기본 묶음이다. 0.645에서 추가했다.</summary>
    private static AiPreparationHooks StrictHooks() => new()
    {
        MissingEnemyAssertion = () => throw new InvalidOperationException("호출되면 안 된다"),
        Priest = _ => throw new InvalidOperationException("호출되면 안 된다"),
        Temple = _ => throw new InvalidOperationException("호출되면 안 된다"),
        MaxHitPoints = _ => throw new InvalidOperationException("호출되면 안 된다"),
        PointDistance = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Graph = _ => throw new InvalidOperationException("호출되면 안 된다"),
        RegionBounds = _ => throw new InvalidOperationException("호출되면 안 된다"),
        StoredSp = _ => throw new InvalidOperationException("호출되면 안 된다"),
        AddLocalSp = _ => throw new InvalidOperationException("호출되면 안 된다"),
        Demo = () => 0u,
        KnowledgeOpen = () => false,
        BlockingGumpOpen = () => false,
        InitialScan = _ => throw new InvalidOperationException("호출되면 안 된다"),
    };

    /// <summary>기본 전역을 만든다. 0.645에서 추가했다.</summary>
    private static AiPreparationGlobals BaseGlobals() => new()
    {
        LocalOwner = () => 1u,
        PlayerKnown = () => 1u,
        NoRegion = () => 127,
        InvalidGraph = () => 99u,
        Now = () => 10.0,
    };

    /// <summary>본체 필드를 읽는다. 0.645에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.645에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>HP·좌표를 가진 사제 슬롯을 만든다. 0.645에서 추가했다.</summary>
    private static int PreparePriest(RawSidPool pool, int hp, float x = 10.0f, float y = 20.0f)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(sid);
        raw.Fill(0);
        Put(raw, 0x1a, unchecked((uint)hp));
        Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
        Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
        return sid;
    }

    /// <summary>강제 대상이 있으면 그대로 선택한다. 0.645에서 추가했다.</summary>
    [Fact]
    public void SelectEnemyUsesForcedTarget()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), StrictHooks());
        Set(memory, token, 0x260, 2);
        Set(memory, token, 0x240, 5);
        body.SelectEnemy(token);
        Assert.Equal(5u, Get(memory, token, 0x264));
    }

    /// <summary>강제가 없으면 첫 사제 보유 비동맹을 선택한다. 0.645에서 추가했다.</summary>
    [Fact]
    public void SelectEnemyFindsFirstNonAlliedPriestOwner()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var hooks = StrictHooks();
        hooks.Priest = owner => owner == 3 ? 100u : 0u;
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x240, 0);
        body.SelectEnemy(token);
        Assert.Equal(3u, Get(memory, token, 0x264));
    }

    /// <summary>적이 없으면 진단 뒤에도 초기 스캔·수량 판정을 계속한다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateWithNoEnemyAssertsAndContinues()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        int asserted = 0, scanned = 0;
        var hooks = StrictHooks();
        hooks.MissingEnemyAssertion = () => asserted++;
        hooks.Priest = _ => 0u;
        hooks.Temple = _ => 0u;
        hooks.RegionBounds = _ => new float[] { 0, 0, 10, 10 };
        hooks.StoredSp = _ => 0.0f;
        hooks.InitialScan = _ => scanned++;
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x240, 0);
        body.Update(token);
        Assert.Equal(1, asserted);
        Assert.Equal(1, scanned);
        Assert.Equal(1u, Get(memory, token, 0x25c));
        Assert.Equal(1u, Get(memory, token, 0x2cc));
        Assert.Equal(1u, Get(memory, token, 0x2d0));
    }

    /// <summary>건강한 적 사제는 목표를 바꾸지 않는다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateKeepsHealthyEnemy()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        int enemyPriest = PreparePriest(pool, 100);
        Put(players, 2 * 180 + 0x2c, 1u);
        var hooks = StrictHooks();
        hooks.Priest = owner => owner == 2 ? unchecked((uint)enemyPriest) : 0u;
        hooks.Temple = _ => 0u;
        hooks.MaxHitPoints = _ => 100;
        hooks.RegionBounds = _ => new float[] { 0, 0, 10, 10 };
        hooks.StoredSp = _ => 0.0f;
        hooks.InitialScan = _ => { };
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 2);
        Set(memory, token, 0x25c, 1);
        body.Update(token);
        Assert.Equal(2u, Get(memory, token, 0x264));
        Assert.Equal(unchecked((uint)enemyPriest), Get(memory, token, 0x2a0));
    }

    /// <summary>절반 미만 적 사제는 가장 가까운 동맹 사제의 주인이면 목표로 삼는다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateRetargetsWeakEnemy()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        int ownPriest = PreparePriest(pool, 100, 11.0f, 21.0f);
        int enemyPriest = PreparePriest(pool, 10, 50.0f, 60.0f);
        Put(players, 1 * 180 + 0x2c, 1u);
        Put(players, 2 * 180 + 0x2c, 1u);
        allies[1 * 9 + 1] = 1u;
        var hooks = StrictHooks();
        hooks.Priest = owner => owner == 1 ? unchecked((uint)ownPriest) : owner == 2 ? unchecked((uint)enemyPriest) : 0u;
        hooks.Temple = _ => 0u;
        hooks.MaxHitPoints = _ => 100;
        hooks.PointDistance = (source, target) =>
        {
            double dx = source[0] - target[0], dy = source[1] - target[1];
            return Math.Sqrt(dx * dx + dy * dy);
        };
        hooks.RegionBounds = _ => new float[] { 0, 0, 10, 10 };
        hooks.StoredSp = _ => 0.0f;
        hooks.InitialScan = _ => { };
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 3);
        Set(memory, token, 0x248, 0x100u);
        Set(memory, token, 0x25c, 1);
        body.Update(token);
        Assert.Equal(2u, Get(memory, token, 0x264));
    }

    /// <summary>로컬 지갑이 상하면 차이를 가산한다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateAddsWalletDelta()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var added = new List<float>();
        var hooks = StrictHooks();
        hooks.Priest = _ => 0u;
        hooks.Temple = _ => 0u;
        hooks.RegionBounds = _ => new float[] { 0, 0, 10, 10 };
        hooks.StoredSp = _ => 5000.0f;
        hooks.AddLocalSp = delta => added.Add(delta);
        hooks.InitialScan = _ => { };
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x240, 0);
        Set(memory, token, 0x25c, 1);
        Set(memory, token, 0x2d8, 30000u);
        // 적이 없어 진단은 부르지 않도록 미리 강제한다. 진단 갈고리는 그대로 둔다.
        hooks.MissingEnemyAssertion = () => { };
        Set(memory, token, 0x264, 4);
        allies[1 * 9 + 4] = 1u;
        body.Update(token);
        Assert.Single(added);
        Assert.Equal(25000.0f, added[0]);
    }

    /// <summary>지갑이 풍부하면 수량과 무관하게 부착 충족을 세운다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateRichWalletSetsAttached()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var hooks = StrictHooks();
        hooks.Priest = _ => 0u;
        hooks.Temple = _ => 0u;
        hooks.RegionBounds = _ => new float[] { 0, 0, 10, 10 };
        hooks.StoredSp = _ => 0.0f;
        hooks.AddLocalSp = _ => { };
        hooks.InitialScan = _ => { };
        hooks.MissingEnemyAssertion = () => { };
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 4);
        allies[1 * 9 + 4] = 1u;
        Set(memory, token, 0x25c, 1);
        Set(memory, token, 0x32c, 5u);
        Set(memory, token, 0x230, 10u);
        Set(memory, token, 0x2d8, 13000u);
        body.Update(token);
        Assert.Equal(1u, Get(memory, token, 0x2d0));
    }

    /// <summary>적 지역 경계는 원래 연속 필드와 중심 계산 순서를 따른다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateWritesEnemyBoundsCenter()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var hooks = StrictHooks();
        hooks.Priest = _ => 0u;
        hooks.Temple = _ => 0u;
        hooks.RegionBounds = region => region == 5 ? new float[] { 0, 0, 100, 50 } : new float[] { 0, 0, 0, 0 };
        hooks.StoredSp = _ => 0.0f;
        hooks.InitialScan = _ => { };
        hooks.MissingEnemyAssertion = () => { };
        Put(players, 4 * 180 + 0x78, 5u);
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 4);
        // 4번을 비동맹으로 두어 재선택 없이 경계 계산에 쓴다.
        Set(memory, token, 0x25c, 1);
        body.Update(token);
        Assert.Equal(BitConverter.SingleToUInt32Bits(50.0f), Get(memory, token, 0x2b0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(25.0f), Get(memory, token, 0x2b4));
    }

    /// <summary>이미 스캔했으면 초기 등록을 다시 부르지 않는다. 0.645에서 추가했다.</summary>
    [Fact]
    public void UpdateSkipsInitialScanWhenDone()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var hooks = StrictHooks();
        hooks.Priest = _ => 0u;
        hooks.Temple = _ => 0u;
        hooks.RegionBounds = _ => new float[] { 0, 0, 10, 10 };
        hooks.StoredSp = _ => 0.0f;
        hooks.MissingEnemyAssertion = () => { };
        hooks.InitialScan = _ => throw new InvalidOperationException("호출되면 안 된다");
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 4);
        allies[1 * 9 + 4] = 1u;
        Set(memory, token, 0x25c, 1);
        body.Update(token);
        Assert.Equal(1u, Get(memory, token, 0x25c));
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.645에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawAiBrainMemory memory, RawSidPool pool, uint token, byte[] players, uint[] allies) = BuildScene();
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        Assert.Throws<ArgumentException>(() => new RawAiPreparation(memory, cdPool, players, allies, BaseGlobals(), StrictHooks()));
        Assert.Throws<ArgumentException>(() => new RawAiPreparation(memory, pool, new byte[10], allies, BaseGlobals(), StrictHooks()));
        Assert.Throws<ArgumentException>(() => new RawAiPreparation(memory, pool, players, new uint[10], BaseGlobals(), StrictHooks()));
        var body = new RawAiPreparation(memory, pool, players, allies, BaseGlobals(), new AiPreparationHooks());
        Assert.Throws<InvalidOperationException>(() => body.SelectEnemy(token));
        uint small = memory.Allocate(100);
        Assert.Throws<ArgumentException>(() => body.SelectEnemy(small));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Update(0xFFFFFFFFu));
    }
}
