// AI 전략 설정(초기 영역·다리·생산·목표 선택)의 분기·순서를 실제 메모리·슬롯으로 대조한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 전략 설정을 검사하는 클래스다. 2026-11-19 0.646에서 R4-5 AI 초기화용으로 추가했다.
// 근거: cpppj o/RawAiStrategySetup.cpp의 Initialize·SelectGoal·FindProduct·AddProduct·SeedBridges·RefreshBridges다.
public sealed class RawAiStrategySetupTests
{
    /// <summary>실제 풀의 슬롯 수다. 0.646에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다. 0.646에서 추가했다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 낮은 바이트부터 네 바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다(2바이트). 0.646에서 추가했다.</summary>
    private static void PutWord(Span<byte> bytes, int offset, uint value)
    {
        bytes[offset] = (byte)value;
        bytes[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>AI 전략 설정 장면을 만든다. 0.646에서 추가했다.</summary>
    private static (RawSidPool Pool, RawAiBrainMemory Memory, uint Token, byte[] Players, uint[] Allies, RawPlayerWorld Player,
        RawAiPreparation Preparation, RawCarrierPreDestroy Finder, RawContainedCreate Create, OriginalTypeRecord[] Types, GameRandom Random) BuildScene()
    {
        var pool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Patch1078);
        var memory = new RawAiBrainMemory(0x20000000u, bytes => bytes.Clear());
        uint token = memory.Allocate(RawAiBrainConstructor.BytesCount);
        byte[] players = new byte[9 * 180];
        uint[] allies = new uint[81];
        var types = new OriginalTypeRecord[188];
        // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }

        var state = new PlayerWorldState { PlayerKnown = 1u, BattleOwner = 1u };
        var player = new RawPlayerWorld(pool, new RawPostPop(pool, types, new RawPostPopState()), players, state, new PlayerWorldHooks());
        var preparation = new RawAiPreparation(memory, pool, players, allies,
            new AiPreparationGlobals { LocalOwner = () => 1u, PlayerKnown = () => 1u, NoRegion = () => 127, InvalidGraph = () => 99u, Now = () => 10.0 },
            new AiPreparationHooks
            {
                MissingEnemyAssertion = () => { }, Priest = _ => 0u, Temple = _ => 0u, MaxHitPoints = _ => 0,
                PointDistance = (_, _) => 0.0, Graph = _ => 99u, RegionBounds = _ => new float[] { 0, 0, 10, 10 },
                StoredSp = _ => 0.0f, AddLocalSp = _ => { }, Demo = () => 0u,
                KnowledgeOpen = () => false, BlockingGumpOpen = () => false, InitialScan = _ => { },
            });
        var finderState = new ContainedFinderState();
        var finder = new RawCarrierPreDestroy(pool, finderState, new CarrierPreDestroyHooks
        {
            ValidateDamageable = (_, _) => { },
            DamageablePre = (_, _) => { },
            DependentFound = () => { },
        });
        // 검사에서는 공장 전체 파이프라인 대신 풀에 직접 할당한다. 생성·연결 순서는 실제와 같다.
        var create = new RawContainedCreate(pool, new ContainedCreateState(), new ContainedCreateHooks
        {
            Create = (_, _) =>
            {
                int sid = pool.Allocate(SidAllocation.Client);
                Span<byte> created = pool.AllocatedBytes(sid);
                created.Fill(0);
                // finder가 타입 바이트로 내용물을 식별하므로 실제 번호(6)를 쓴다.
                created[RawSidPool.TypeOffset] = 6;
                return sid;
            },
            NotifySurface = _ => { },
            Attach = (form, parent, _) =>
            {
                // 다음 링크(+4)에 부모 head(+6 현재값)를 먼저 저장한 뒤 부모 head를 갱신한다. 원문 순서와 같다.
                Span<byte> formRaw = pool.AllocatedBytes(form);
                ReadOnlySpan<byte> parentRaw = pool.Slot(parent);
                PutWord(formRaw, 4, BinaryPrimitives.ReadUInt16LittleEndian(parentRaw.Slice(6, 2)));
                BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(parent).Slice(6, 2), unchecked((ushort)form));
            },
        });
        var random = new GameRandom(1u);
        return (pool, memory, token, players, allies, player, preparation, finder, create, types, random);
    }

    /// <summary>기본 전역을 만든다. 0.646에서 추가했다.</summary>
    private static AiStrategySetupGlobals BaseGlobals() => new()
    {
        LocalOwner = () => 1u,
        NoRegion = () => 127,
        TempleProduct = () => 60u,
        BridgeType = () => 82u,
        MapLimit = () => 256,
        Now = () => 10.0,
    };

    /// <summary>기본 갈고리를 만든다. 0.646에서 추가했다.</summary>
    private static AiStrategySetupHooks BaseHooks(List<string>? asserts = null) => new()
    {
        RegionBounds = _ => new float[] { 0, 0, 10, 10 },
        Temple = _ => 0u,
        Destroy = (_, _) => throw new InvalidOperationException("호출되면 안 된다"),
        Assertion = text => asserts?.Add(text),
        Demo = () => false,
    };

    /// <summary>설정 본체를 만든다. 0.646에서 추가했다.</summary>
    private static RawAiStrategySetup BuildBody(
        RawAiBrainMemory memory, RawPlayerWorld player, RawAiPreparation preparation, RawCarrierPreDestroy finder,
        RawContainedCreate create, ContainedFinderState finderState, OriginalTypeRecord[] types, GameRandom random, uint[] allies,
        AiStrategySetupGlobals? globals = null, AiStrategySetupHooks? hooks = null, List<string>? asserts = null) => new(
            memory, player, preparation, finder, create, finderState, types, random, allies,
            globals ?? BaseGlobals(), hooks ?? BaseHooks(asserts));

    /// <summary>본체 필드를 읽는다. 0.646에서 추가했다.</summary>
    private static uint Get(RawAiBrainMemory memory, uint token, uint offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4));

    /// <summary>본체 필드를 쓴다. 0.646에서 추가했다.</summary>
    private static void Set(RawAiBrainMemory memory, uint token, uint offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(memory.Bytes(token).AsSpan((int)offset, 4), value);

    /// <summary>슬롯을 할당하고 0으로 채운다. 0.646에서 추가했다.</summary>
    private static int AllocateZero(RawSidPool pool)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        pool.AllocatedBytes(sid).Fill(0);
        return sid;
    }

    /// <summary>적·양쪽 지역이 없으면 258을 쓰지 않고 돌아간다. 0.646에서 추가했다.</summary>
    [Fact]
    public void InitializeSkipsWithoutRegions()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        var asserts = new List<string>();
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies, asserts: asserts);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 0);
        body.Initialize(token);
        Assert.Equal(0u, Get(memory, token, 0x258));
    }

    /// <summary>양쪽 지역이 있으면 자기 영역·첫 재고·258을 기록한다. 0.646에서 추가했다.</summary>
    [Fact]
    public void InitializeWritesHomeBoundsAndFlag()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        Put(players, 1 * 180 + 0x78, 3u);
        Put(players, 2 * 180 + 0x78, 4u);
        int technology = AllocateZero(pool);
        Put(players, 1 * 180 + 0x9c, unchecked((uint)technology));
        var hooks = BaseHooks();
        hooks.RegionBounds = region => region == 3 ? new float[] { 0, 0, 100, 50 } : new float[] { 0, 0, 0, 0 };
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies, hooks: hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 2);
        allies[1 * 9 + 2] = 0u;
        body.Initialize(token);
        Assert.Equal(1u, Get(memory, token, 0x258));
        Assert.Equal(BitConverter.SingleToUInt32Bits(50.0f), Get(memory, token, 0x268));
        Assert.Equal(BitConverter.SingleToUInt32Bits(25.0f), Get(memory, token, 0x26c));
    }

    /// <summary>신전이 있고 생산물이 없으면 신전 생산물을 만들고 theme을 기록한다. 0.646에서 추가했다.</summary>
    [Fact]
    public void InitializeAddsTempleProduct()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        Put(players, 1 * 180 + 0x78, 3u);
        Put(players, 2 * 180 + 0x78, 4u);
        int technology = AllocateZero(pool);
        Put(players, 1 * 180 + 0x9c, unchecked((uint)technology));
        int temple = AllocateZero(pool);
        pool.AllocatedBytes(temple)[10] = 40;
        types[40].Theme = 7;
        var hooks = BaseHooks();
        hooks.Temple = region => region == 3 ? unchecked((uint)temple) : 0u;
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies, hooks: hooks);
        Set(memory, token, 0x260, 1);
        Set(memory, token, 0x264, 2);
        body.Initialize(token);
        Assert.Equal(1u, Get(memory, token, 0x258));
        Assert.Equal(7u, Get(memory, token, 0x390));
        // 신전 쪽 생산물은 플래그 0x10으로 만들므로 마스크 0x10으로 조회한다. 원본도 마스크 8로는 찾지 않는다.
        Assert.NotEqual(0, finder.FindContained(temple, 0x10, 60, 0));
    }

    /// <summary>자금·플래그·그래프 조건이면 목표 1을 고르고 관련 필드를 초기화한다. 0.646에서 추가했다.</summary>
    [Fact]
    public void SelectGoalOneResetsRelatedFields()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies);
        Set(memory, token, 0x2d0, 1);
        Set(memory, token, 0x2d8, 20000u);
        Set(memory, token, 0x290, 1000u);
        Set(memory, token, 0x248, 4u);
        Set(memory, token, 0x298, 1u);
        Set(memory, token, 0x2ac, 2u);
        Set(memory, token, 0x2d4, 0u);
        body.SelectGoal(token);
        Assert.Equal(1u, Get(memory, token, 0x2d4));
        Assert.Equal(0u, Get(memory, token, 0x3cc));
        Assert.Equal(0xc61c4000u, Get(memory, token, 0x3d4));
        Assert.Equal(0xc61c4000u, Get(memory, token, 0x3d0));
        Assert.Equal(1u, Get(memory, token, 0x3d8));
        Assert.Equal(0u, Get(memory, token, 0x5f0));
        // 같은 목표면 바꾸지 않는다.
        Set(memory, token, 0x3d4, 9u);
        body.SelectGoal(token);
        Assert.Equal(9u, Get(memory, token, 0x3d4));
    }

    /// <summary>미부착·기능 플래그이면 목표 2, 유효 특정 좌표이면 목표 4, 아니면 3을 고른다. 0.646에서 추가했다.</summary>
    [Theory]
    [InlineData(0u, 0x10u, 0u, 0u, 2u)]
    [InlineData(0u, 0u, 50.0f, 60.0f, 4u)]
    [InlineData(1u, 0u, 0u, 0u, 3u)]
    public void SelectGoalBranches(uint attached, uint flags, float x, float y, uint expected)
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies);
        Set(memory, token, 0x2d0, attached);
        Set(memory, token, 0x2d8, 0u);
        Set(memory, token, 0x290, 100000u);
        Set(memory, token, 0x248, flags);
        Set(memory, token, 0xaf0, BitConverter.SingleToUInt32Bits(x));
        Set(memory, token, 0xaf4, BitConverter.SingleToUInt32Bits(y));
        Set(memory, token, 0x2d4, 9u);
        body.SelectGoal(token);
        Assert.Equal(expected, Get(memory, token, 0x2d4));
    }

    /// <summary>기술 부모의 mask8 내용물을 찾는다. 0.646에서 추가했다.</summary>
    [Fact]
    public void FindProductReadsTechnologyParent()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        int technology = AllocateZero(pool);
        Put(players, 1 * 180 + 0x9c, unchecked((uint)technology));
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies);
        Set(memory, token, 0x260, 1);
        Assert.Equal(0, body.FindProduct(token, 60u));
        int made = create.Create(technology, 60u, 0, 8, 0x10, 0);
        Assert.Equal(made, body.FindProduct(token, 60u));
    }

    /// <summary>원격 소유자는 기존 생산물 경고를 남기고 두 곳에 만든다. 0.646에서 추가했다.</summary>
    [Fact]
    public void AddProductRemoteReplacesAndCreatesBoth()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        int technology = AllocateZero(pool);
        Put(players, 2 * 180 + 0x9c, unchecked((uint)technology));
        int old = create.Create(technology, 60u, 0, 8, 0x10, 0);
        var asserts = new List<string>();
        var destroyed = new List<int>();
        var hooks = BaseHooks(asserts);
        hooks.Destroy = (sid, flags) => destroyed.Add(sid);
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies, hooks: hooks);
        Set(memory, token, 0x260, 2);
        int parent = AllocateZero(pool);
        body.AddProduct(token, parent, 60u);
        Assert.Contains("!haveProd( typeNum )", asserts);
        Assert.Equal([old], destroyed);
        Assert.NotEqual(0, finder.FindContained(technology, 8, 60, 0));
        // 부모 쪽 생산물은 플래그 0x10으로 만들므로 마스크 0x10으로 조회한다. 원본도 마스크 8로는 찾지 않는다.
        Assert.NotEqual(0, finder.FindContained(parent, 0x10, 60, 0));
    }

    /// <summary>부족분만큼 다리를 만들고 목표·재고·마감을 갱신한다. 0.646에서 추가했다.</summary>
    [Fact]
    public void SeedBridgesBuildsShortfall()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        int technology = AllocateZero(pool);
        create.Create(technology, 82u, 0, 0x40, 0x10, 0);
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies);
        Set(memory, token, 0x600, BitConverter.SingleToUInt32Bits(5.0f));
        body.SeedBridges(token, technology, 2u);
        Assert.Equal(2u, Get(memory, token, 0x604));
        Assert.Equal(2u, Get(memory, token, 0x628));
        var counter = new RawContainedFinder(pool, new ContainedFinderState());
        int counted = 0;
        // 같은 finder로 처음부터 끝까지 세는 반복문이다.
        for (int sid = counter.Begin(technology); sid != 0; sid = counter.Next())
        {
            counted++;
        }

        Assert.Equal(2, counted);
    }

    /// <summary>목표 달성·기한 전 비로컬이면 보충하지 않는다. 0.646에서 추가했다.</summary>
    [Fact]
    public void RefreshBridgesRespectsGates()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        int technology = AllocateZero(pool);
        pool.AllocatedBytes(technology)[34] = 1;
        Put(players, 1 * 180 + 0x7c, 1u);
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies);
        // 목표 0이면 총수 0과 같아 즉시 돌아간다.
        Set(memory, token, 0x604, 0u);
        body.RefreshBridges(token, technology);
        Assert.Equal(0u, Get(memory, token, 0x628));
        // 기한 전이면 비로컬 주인은 돌아간다. 기한은 큰 double의 비트로 둔다.
        pool.AllocatedBytes(technology)[34] = 2;
        Put(players, 2 * 180 + 0x7c, 1u);
        Set(memory, token, 0x604, 5u);
        ulong dueBits = BitConverter.DoubleToUInt64Bits(1e15);
        Set(memory, token, 0x630, unchecked((uint)dueBits));
        Set(memory, token, 0x634, unchecked((uint)(dueBits >> 32)));
        body.RefreshBridges(token, technology);
        Assert.Equal(0u, Get(memory, token, 0x628));
    }

    /// <summary>계약 위반·미연결·잘못된 본체는 거부한다. 0.646에서 추가했다.</summary>
    [Fact]
    public void InvalidInputsThrow()
    {
        (RawSidPool pool, RawAiBrainMemory memory, uint token, byte[] players, uint[] allies, RawPlayerWorld player,
            RawAiPreparation preparation, RawCarrierPreDestroy finder, RawContainedCreate create, OriginalTypeRecord[] types, GameRandom random) = BuildScene();
        var cdPool = new RawSidPool(PoolCapacity, false, null, OriginalEdition.Cd1072);
        var cdPlayer = new RawPlayerWorld(cdPool, new RawPostPop(cdPool, types, new RawPostPopState()), new byte[9 * 0xac], new PlayerWorldState { PlayerKnown = 1u }, new PlayerWorldHooks
        {
            Assertion = _ => { }, Find = (_, _, _) => 0, Create = (_, _) => 0, SetOwner = (_, _) => { },
            Pop = (_, _, _, _) => { }, ReserveRegular = () => true, AddRegular = (_, _, _) => { },
        });
        Assert.Throws<ArgumentException>(() => BuildBody(memory, cdPlayer, preparation, finder, create, new ContainedFinderState(), types, random, allies));
        Assert.Throws<ArgumentException>(() => BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), Array.Empty<OriginalTypeRecord>(), random, allies));
        Assert.Throws<ArgumentNullException>(() => new RawAiStrategySetup(memory, player, preparation, finder, create, new ContainedFinderState(), null!, random, allies, BaseGlobals(), BaseHooks()));
        var body = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies, hooks: new AiStrategySetupHooks());
        // else 분기는 지도 상한을 읽으므로 비운 전역에서 거부된다.
        var noLimitGlobals = BaseGlobals();
        noLimitGlobals.MapLimit = null;
        var noLimitBody = BuildBody(memory, player, preparation, finder, create, new ContainedFinderState(), types, random, allies, globals: noLimitGlobals, hooks: new AiStrategySetupHooks());
        Assert.Throws<InvalidOperationException>(() => noLimitBody.SelectGoal(token));
        uint small = memory.Allocate(100);
        Assert.Throws<ArgumentException>(() => body.SelectGoal(small));
    }
}
