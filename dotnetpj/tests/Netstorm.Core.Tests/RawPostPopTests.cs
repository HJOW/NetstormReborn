using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공통 후처리(<see cref="RawPostPop"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawPostPopTests)이 정상 호출의 결과를 대조하고, 이 검사들은 cpppj/tests/PostPopTests.cpp 의 보호 검사를 10.78 로 옮기며
/// "거부한 호출은 상태·풀을 한 바이트도 바꾸지 않는다", 최초 등록의 중복 집계 금지, Graph 연결 요구, 다리 접두의 쓰기 순서를 더해 확인한다.
/// 파생 몸체는 아직 없으므로 HandlesDerived 는 항상 거짓이다 (W02 PostPop 조각, 2026-10-20).
/// </summary>
public sealed class RawPostPopTests
{
    /// <summary>검사에 쓰는 타입 번호. 파생 생성자가 없고 기본 가상 표를 쓴다(cpppj 보호 검사와 같다).</summary>
    private const uint TestType = 74;

    /// <summary>기본 가상 표. 공통 표시 지원 표에 있다.</summary>
    private const uint BaseVtable = OriginalConstructors.BaseVtable;

    /// <summary>생성자 주소 표를 채운 10.78 타입 레코드 188개.</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) };
        }
        return types;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>후처리 상태·슬롯의 스냅샷을 복사한다.</summary>
    /// <param name="state">상태.</param>
    /// <param name="pool">풀.</param>
    private static (int Cost, uint Dirty, uint Depth, byte[] Pool) Snapshot(RawPostPopState state, RawSidPool pool) =>
        (state.TotalCost, state.ProductionDirty, state.Depth, pool.Bytes.ToArray());

    /// <summary>void 상태의 합성 슬롯을 만든다. 팩토리가 만든 슬롯에 소유자·추가 비트를 덮는다.</summary>
    /// <param name="factory">팩토리.</param>
    /// <param name="pool">풀.</param>
    /// <param name="owner">소유자 바이트.</param>
    /// <param name="extra">추가 비트 바이트.</param>
    private static int VoidSlot(RawSquidFactory factory, RawSidPool pool, byte owner = 0, byte extra = 0)
    {
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[34] = owner;
        bytes[40] = extra;
        return sid;
    }

    /// <summary>호출 순서를 기록하는 Graph 대역.</summary>
    private sealed class RecordingGraph(RawSidPool pool, List<string> log) : IRawPostPopGraph
    {
        /// <summary>연결된 풀.</summary>
        public RawSidPool Pool => pool;

        /// <summary>호출 기록.</summary>
        public void ValidatePostPop(int sid, bool region, bool add, in PopProjection projection) => log.Add($"GraphValidate:{region}:{add}");

        /// <summary>호출 기록.</summary>
        public void PostPop(int sid, bool region, bool add) => log.Add($"GraphPostPop:{region}:{add}");

        /// <summary>호출 기록.</summary>
        public void ValidateSpace(RawSquidHash hash, byte[] spots) => log.Add("GraphValidateSpace");
    }

    /// <summary>
    /// 손상된 공통 경로·미복원 효과는 상태·풀을 바꾸기 전에 거부한다:
    /// 번호 범위·빈·내용물·자산 타입 밖·지원하지 않는 가상 표·손상된 목록 개수.
    /// </summary>
    [Fact]
    public void GuardsRejectBeforeChangingStateOrPool()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { Depth = 3 };
        var post = new RawPostPop(pool, types, state);
        int sid = VoidSlot(factory, pool);
        (int cost, uint dirty, uint depth, byte[] before) = Snapshot(state, pool);
        // 번호 0~4 는 대상이 아니다
        Assert.Throws<InvalidOperationException>(() => post.PostPop(0, 1));
        Assert.Throws<InvalidOperationException>(() => post.PostPop(4, 1));
        // 빈 슬롯은 거부한다(목록·카운터는 바뀌지만 슬롯 바이트 비교는 그 뒤부터 한다)
        int free = pool.Allocate(SidAllocation.Client);
        pool.Release(free);
        Assert.Throws<InvalidOperationException>(() => post.PostPop(free, 1));
        before = pool.Bytes.ToArray();
        // 들어 있는 내용물은 거부한다
        pool.AllocatedBytes(sid)[RawSidPool.StateOffset] |= 8;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        pool.AllocatedBytes(sid)[RawSidPool.StateOffset] &= 0xF7;
        // 자산 타입 밖은 거부한다
        pool.AllocatedBytes(sid)[RawSidPool.TypeOffset] = 69;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        pool.AllocatedBytes(sid)[RawSidPool.TypeOffset] = (byte)TestType;
        // 지원하지 않는 가상 표는 파생 몸체가 없으므로 거부한다
        Put(pool.AllocatedBytes(sid), 0, 0x12345678);
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        Put(pool.AllocatedBytes(sid), 0, BaseVtable);
        // 소유자 9 이상은 범위 오류다
        pool.AllocatedBytes(sid)[34] = 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => post.PostPop(sid, 1));
        pool.AllocatedBytes(sid)[34] = 0;
        // 손상된 공급원 목록 개수는 쓰기 전에 거부한다
        state.Providers.Entries = new uint[1];
        state.Providers.Count = 2;
        types[TestType].Flags1 = 0x10000000;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        state.Providers.Count = 0;
        // 손상된 작업장 목록 개수도 쓰기 전에 거부한다
        state.Factories.Entries = new uint[1];
        state.Factories.Count = 2;
        types[TestType].Flags2 = 0x4200;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        state.Factories.Count = 0;
        types[TestType].Flags2 = 0;
        Assert.Equal((cost, dirty, depth), (state.TotalCost, state.ProductionDirty, state.Depth));
        Assert.Equal(before, pool.Bytes.ToArray());
        // 잘못된 풀·타입 표·공간 인자는 만들 때·연결할 때 거부한다
        Assert.Throws<ArgumentException>(() => new RawPostPop(pool, new OriginalTypeRecord[171], state));
        Assert.Throws<ArgumentException>(() => new RawPostPop(pool, types, state, new RecordingGraph(new RawSidPool(32768), [])));
        Assert.Throws<ArgumentNullException>(() => post.ValidateSpace(null!, new byte[10]));
    }

    /// <summary>AI 가 붙은 소유자·보류 배치는 최초 등록의 비용 집계를 거부한다.</summary>
    [Fact]
    public void AttachedOwnerAndPendingPlacementReject()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags1 = 0x10000000;
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { Depth = 3, LocalOwner = 1 };
        var post = new RawPostPop(pool, types, state);
        int sid = VoidSlot(factory, pool, owner: 1);
        (int cost, uint dirty, uint depth, byte[] before) = Snapshot(state, pool);
        state.AiAttached[1] = true;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        state.AiAttached[1] = false;
        state.PendingPlacement = true;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        state.PendingPlacement = false;
        // 최초 등록(flags 1)이 아니면 AI 부착·보류 배치 검사도 건드리지 않고 깊이만 줄인다
        state.AiAttached[1] = true;
        state.PendingPlacement = true;
        post.PostPop(sid, 0);
        Assert.Equal(2u, state.Depth);
        Assert.Equal(cost, state.TotalCost);
        state.AiAttached[1] = false;
        state.PendingPlacement = false;
        Assert.Equal((cost, dirty, depth - 1), (state.TotalCost, state.ProductionDirty, state.Depth));
        Assert.Equal(before, pool.Bytes.ToArray());
    }

    /// <summary>유한하지 않거나 표·누적 범위를 벗어난 비용은 통계·깊이·슬롯에 쓰기 전에 거부한다.</summary>
    [Fact]
    public void BadCostsRejectBeforeMutation()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { Depth = 3 };
        var post = new RawPostPop(pool, types, state);
        int sid = VoidSlot(factory, pool);
        (int cost, uint dirty, uint depth, byte[] before) = Snapshot(state, pool);
        types[TestType].Cost = float.NaN;
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        types[TestType].Cost = 1e30f;
        Assert.Throws<ArgumentOutOfRangeException>(() => post.PostPop(sid, 1));
        // int32 두 개(int 단위·int 집계)의 합은 uint 범위를 벗어날 수 없으므로 누적 검사는 경계를 통과하고 low DWORD 로 감긴다
        types[TestType].Cost = 1f;
        state.TotalCost = int.MaxValue;
        post.PostPop(sid, 1);
        Assert.Equal(int.MinValue, state.TotalCost);
        Assert.Equal(depth - 1, state.Depth);
        Assert.Equal(dirty, state.ProductionDirty);
        Assert.Equal(before, pool.Bytes.ToArray());
    }

    /// <summary>억제 경로는 통계·목록·슬롯을 건드리지 않고 깊이만 1 줄인다.</summary>
    [Fact]
    public void SuppressedSkipsStatsButDecrementsDepth()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags1 = 0x10000000;
        types[TestType].Cost = 5f;
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { Suppressed = true, Depth = 3, TotalCost = 7 };
        var post = new RawPostPop(pool, types, state);
        int sid = VoidSlot(factory, pool, owner: 1);
        byte[] before = pool.Bytes.ToArray();
        post.PostPop(sid, 1);
        Assert.Equal(2u, state.Depth);
        Assert.Equal(7, state.TotalCost);
        Assert.Equal(0u, state.ProductionDirty);
        Assert.Equal(0u, state.GlobalCounts[TestType]);
        Assert.Equal(before, pool.Bytes.ToArray());
        // 억제 경로는 AI 부착 검사도 건너뛰고 깊이만 줄인다(원본은 suppressed 를 가장 먼저 본다)
        state.AiAttached[1] = true;
        post.PostPop(sid, 1);
        Assert.Equal(1u, state.Depth);
        Assert.Equal(7, state.TotalCost);
        state.AiAttached[1] = false;
    }

    /// <summary>
    /// 최초 Pop 의 비용·통계는 재등록에서 자동 중복 증가하지 않으며 직접 호출의 flags 1 은 다시 집계한다
    /// (cpppj SquidPostPop_FirstRegistrationAndProviderDuplicate 와 같다).
    /// </summary>
    [Fact]
    public void FirstRegistrationAndProviderDuplicatePreserveCounters()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags1 = 0x10000000;
        types[TestType].Flags2 = 0x10000;
        types[TestType].Cost = 3.75f;
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { LocalOwner = 1 };
        state.Providers.Entries = new uint[1];
        var post = new RawPostPop(pool, types, state);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var pop = new RawPop(pool, hash, spots, null, post);
        var unpop = new RawUnpop(pool, hash, spots);
        int sid = factory.Create(TestType);
        pool.AllocatedBytes(sid)[34] = 1;
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 20, 20));
        Assert.Equal(3, state.TotalCost);
        Assert.Equal(1u, state.GlobalCounts[TestType]);
        Assert.Equal(0u, state.Depth);
        unpop.Unpop(sid, types[TestType]);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 30));
        Assert.Equal(3, state.TotalCost);
        Assert.Equal(1u, state.Providers.Count);
        Assert.Equal(1u, state.ProductionDirty);
        post.Activate(sid, 1);
        Assert.Equal(6, state.TotalCost);
        Assert.Equal(1u, state.Providers.Count);
        Assert.Equal(2u, state.ProductionDirty);
        Assert.Equal(2u, state.LocalCounts[TestType]);
        Assert.Equal(2u, state.LocalSecondaryCounts[TestType]);
        Assert.Equal(2u, state.GlobalCounts[TestType]);
        Assert.Equal(0u, state.Depth);
    }

    /// <summary>영역·표면 등록은 Graph 연결을 요구하고, 파괴 경로는 원본 assert 로 멈춘다.</summary>
    [Fact]
    public void GraphConnectionIsRequiredForRegionOrSurface()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = 0x50444208;
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { GraphsEnabled = true, Depth = 3 };
        var post = new RawPostPop(pool, types, state);
        int sid = VoidSlot(factory, pool);
        (int cost, uint dirty, uint depth, byte[] before) = Snapshot(state, pool);
        // 영역 등록인데 Graph 가 없으면 거부한다
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 1));
        // 파괴 비트는 NoGraph 가 없어도 원본 assert 로 멈춘다
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 0x803));
        Assert.Equal((cost, dirty, depth), (state.TotalCost, state.ProductionDirty, state.Depth));
        Assert.Equal(before, pool.Bytes.ToArray());
        // NoGraph 표면 등록은 Graph 없이 Graph 바이트에 무효 번호를 쓴다
        types[TestType].Flags1 = OriginalTypeRecord.Surface;
        types[TestType].Flags2 = 0x2;
        post.Validate(sid, 0x2000001, default);
        post.PostPopBase(sid, 0x2000001, default);
        Assert.Equal(254, pool.Slot(sid)[30]);
    }

    /// <summary>Graph 가 연결되면 영역 검사가 Graph 몸체로 위임되고 같은 풀·공간이어야 한다.</summary>
    [Fact]
    public void ConnectedGraphReceivesRegionValidationAndEffects()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = 0x50444208;
        var factory = new RawSquidFactory(pool, types);
        var state = new RawPostPopState { GraphsEnabled = true, Depth = 3 };
        state.Factories.Entries = new uint[1];
        var log = new List<string>();
        var post = new RawPostPop(pool, types, state, new RecordingGraph(pool, log));
        int sid = VoidSlot(factory, pool);
        post.PostPop(sid, 1);
        Assert.Contains("GraphValidate:True:False", log);
        Assert.Contains("GraphPostPop:True:False", log);
        Assert.Equal(0, state.TotalCost);
        Assert.Equal(1u, state.ProductionDirty);
        Assert.Equal(1u, state.Factories.Count);
        Assert.Equal(1u, state.LocalCounts[TestType]);
        Assert.Equal(1u, state.GlobalCounts[TestType]);
        Assert.Equal(2u, state.Depth);
        byte[] afterFirst = pool.Bytes.ToArray();
        // 파괴 비트는 Graph 가 연결돼 있어도 원본 assert 로 멈추며 깊이·슬롯을 바꾸지 않는다
        Assert.Throws<InvalidOperationException>(() => post.PostPop(sid, 0x803));
        Assert.Equal(2u, state.Depth);
        Assert.Equal(afterFirst, pool.Bytes.ToArray());
    }

    /// <summary>다리 접두의 플래그·추가 비트 조건과 연결 효과의 쓰기 순서를 확인한다.</summary>
    [Fact]
    public void BridgePrefixFlagsExtraAndCallbackOrder()
    {
        var pool = new RawSidPool(24000, false);
        // 원본 fixture 의 번호 50 을 클라이언트 영역에 확보한다
        for (int id = 5; id <= 50; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        // flag 0·abstract 켜짐이면 연결하지 않고 쓰지도 않는다
        pool.AllocatedBytes(50)[40] = 1;
        bool called = false;
        RawPostPop.BridgePrefix(pool, 50, 0, _ => called = true);
        Assert.False(called);
        Assert.Equal(1, pool.Slot(50)[40]);
        // flag 1 은 추가 비트를 먼저 쓰고 연결한다
        string order = string.Empty;
        RawPostPop.BridgePrefix(pool, 50, 1, sid =>
        {
            order += $"L:{sid}";
            Assert.Equal(3, pool.Slot(sid)[40]);
        });
        Assert.Equal("L:50", order);
        Assert.Equal(3, pool.Slot(50)[40]);
        // flag 4·abstract 꺼짐이면 비트를 쓰지 않고 연결만 한다
        pool.AllocatedBytes(50)[40] = 0;
        RawPostPop.BridgePrefix(pool, 50, 4, _ => called = true);
        Assert.True(called);
        Assert.Equal(0, pool.Slot(50)[40]);
        // 연결이 필요한데 효과가 없으면 쓰기 전에 거부한다
        byte[] before = pool.Bytes.ToArray();
        Assert.Throws<ArgumentException>(() => RawPostPop.BridgePrefix(pool, 50, 1, null));
        Assert.Equal(before, pool.Bytes.ToArray());
    }
}
