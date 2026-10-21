using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 공통 삭제 수명주기(<see cref="RawSquidDestroyLifecycle"/>)의 경계 계약을 합성 검사로 확인한다.
/// 장면은 cpppj/tests/DestroyLifecycleTests.cpp 의 합성 검사와 같은 구성(10.78)이다.
/// 기대값은 원본 기계어 관찰(destroylifecycle-x86.tsv)에서 확인한 규칙에서 직접 적었다.
/// </summary>
public sealed class RawDestroyLifecycleTests
{
    /// <summary>삭제 대상 슬롯 번호. 원본 시험과 같다.</summary>
    private const int Root = 5;

    /// <summary>장면이 확보하는 클라이언트 슬롯 7개.</summary>
    private static readonly int[] SlotIds = [5, 6, 7, 8, 9, 10, 11];

    /// <summary>루트 타입 번호(자산).</summary>
    private const int RootType = 74;

    /// <summary>타입 비용(원본 시험과 같은 600.0f 비트).</summary>
    private const uint RootCostBits = 1142292480;

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

    /// <summary>원본 시험의 기본 입력값 목록을 만든다.</summary>
    private static string[] Basic() =>
    [
        "74", "0", "0", RootCostBits.ToString(), "0", "1", "1", "0", "2097152", "0",
        "0", "0", "0", "148", "3", "1000", "0", "1", "123",
    ];

    /// <summary>목록의 여섯 칸을 채운다. 원본 시험과 같은 크기다.</summary>
    /// <param name="list">채울 목록.</param>
    /// <param name="pattern">패턴 번호(0~5).</param>
    /// <param name="baseValue">목록 항목의 기준 값.</param>
    private static void ListInput(RawPostPopList list, int pattern, uint baseValue)
    {
        // 여섯 패턴의 활성 항목 수다
        uint[] counts = [0, 1, 5, 6, 4, 3];
        list.Count = counts[pattern];
        list.Entries = new uint[6];
        // 기준 값을 순서대로 넣는다
        for (int i = 0; i < 6; i++)
        {
            list.Entries[i] = baseValue + (uint)i;
        }
        // 패턴마다 중복 위치가 다르다
        if (pattern == 1)
        {
            list.Entries[0] = 5;
        }
        if (pattern == 3)
        {
            list.Entries[0] = 5;
            list.Entries[2] = 5;
            list.Entries[5] = 5;
        }
        if (pattern == 4)
        {
            // 앞 네 칸을 같은 번호로 채운다
            for (int i = 0; i < 4; i++)
            {
                list.Entries[i] = 5;
            }
        }
        if (pattern == 5)
        {
            list.Entries[1] = 5;
        }
    }

    /// <summary>합성 검사용 삭제 수명주기 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버).</summary>
        public readonly RawSidPool Pool = new(32768, true);

        /// <summary>공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>점유 지도(65,536칸, 항상 0).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];

        /// <summary>자산 뿌리의 공간 해제기.</summary>
        public readonly RawUnpop Unpop;

        /// <summary>삭제 본체.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>공유 장부.</summary>
        public readonly RawPostPopState Bookkeeping = new();

        /// <summary>삭제 상태.</summary>
        public readonly SquidDeletionState State = new();

        /// <summary>삭제 수명주기.</summary>
        public readonly RawSquidDestroyLifecycle Lifecycle;

        /// <summary>삭제에 적용되는 플래그 비트.</summary>
        public uint Flags;

        /// <summary>나간 효과 문자열.</summary>
        public readonly List<string> Events = [];

        /// <summary>원본 시험의 기본 입력값으로 장면을 만든다.</summary>
        /// <param name="input">쉼표로 나눈 19개 입력값.</param>
        public Scene(string[] input)
        {
            Assert.Equal(19, input.Length);
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Unpop = new RawUnpop(Pool, Hash, Spots);
            Destroy = new RawSquidDestroy(Pool, Unpop, Types);
            // 부모 실제 Create와 같은 클라이언트 할당만 준비한다
            for (int i = 0; i < SlotIds.Length; i++)
            {
                Assert.Equal(SlotIds[i], Pool.Allocate(SidAllocation.Client));
            }
            // 같은 위치의 0단계(뿌리)·1단계(주변) 버킷 체인을 넣는다
            for (int i = 0; i < SlotIds.Length; i++)
            {
                Span<byte> raw = Pool.AllocatedBytes(SlotIds[i]);
                raw.Clear();
                Put(raw, 0, OriginalConstructors.BaseVtable);
                raw[10] = (byte)(RootType + i);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f));
                Types[RootType + i].FootX = 1;
                Types[RootType + i].FootY = 1;
                ref ushort head = ref Hash.Bucket(i == 0 ? 0 : 1, 20.75f, 21.9f);
                Put(raw, 4, head, 2);
                head = (ushort)SlotIds[i];
            }
            int number = int.Parse(input[0]);
            OriginalTypeRecord type = Types[number];
            type.Flags1 = uint.Parse(input[1]);
            type.Flags2 = uint.Parse(input[2]);
            type.Cost = BitConverter.UInt32BitsToSingle(uint.Parse(input[3]));
            type.Group = int.Parse(input[4]);
            type.FootX = 1;
            type.FootY = 1;
            Span<byte> root = Pool.AllocatedBytes(Root);
            root[10] = (byte)number;
            root[11] = input[17] == "1" ? (byte)4 : (byte)0;
            root[34] = (byte)int.Parse(input[5]);
            root[40] = (byte)int.Parse(input[7]);
            // 뿌리가 공간에서 빠졌으면 0단계 버킷 머리를 비운다
            if ((root[11] & 4) != 0)
            {
                Hash.Bucket(0, 20.75f, 21.9f) = 0;
            }
            Flags = uint.Parse(input[8]);
            State.Selected = input[9] == "1" ? Root : 0;
            Bookkeeping.LocalOwner = (byte)int.Parse(input[6]);
            Bookkeeping.Suppressed = input[10] == "1";
            State.UnitLostSuppressed1 = input[11] == "1";
            State.UnitLostSuppressed2 = input[12] == "1";
            State.SilentType = uint.Parse(input[13]);
            int pattern = int.Parse(input[14]);
            Bookkeeping.TotalCost = int.Parse(input[15]);
            Bookkeeping.ProductionDirty = uint.Parse(input[16]);
            Bookkeeping.Depth = 3;
            ListInput(Bookkeeping.Providers, pattern, 0x60000000);
            ListInput(Bookkeeping.Factories, pattern, 0x61000000);
            // 원본 Player 0의 미사용 목록까지 같은 입력을 넣는다
            for (int i = 0; i < RawPostPopState.OwnerCount; i++)
            {
                ListInput(Bookkeeping.OwnerFactories[i], pattern, 0x70000000 + (uint)i * 100);
            }
            uint seed = uint.Parse(input[18]);
            // 통계 표 전체의 비대상 타입 보존과 DWORD 감김도 비교한다
            for (int i = 0; i < RawPostPopState.TableSize; i++)
            {
                Bookkeeping.LocalCounts[i] = unchecked(seed + (uint)i * 2654435761U);
                Bookkeeping.LocalSecondaryCounts[i] = unchecked(Bookkeeping.LocalCounts[i] + 17U);
                Bookkeeping.GlobalCounts[i] = unchecked(Bookkeeping.LocalCounts[i] + 34U);
            }
            State.PlacementX = -7.25f;
            State.PlacementY = 8.5f;
            State.LostX = 90.25f;
            State.LostY = -91.5f;
            Lifecycle = new RawSquidDestroyLifecycle(Pool, Types, Bookkeeping, State, Destroy, new SquidDeletionHooks
            {
                Emit = Emit,
                Forward = Forward,
            });
        }

        /// <summary>삭제 수명주기가 낸 외부 효과를 기록한다.</summary>
        /// <param name="effect">나온 효과.</param>
        private void Emit(SquidDeletionEvent effect)
        {
            switch (effect.Effect)
            {
                case SquidDeletionEffect.ClearSelection:
                    Events.Add($"S:{State.Selected}");
                    break;
                case SquidDeletionEffect.Refund:
                    Events.Add($"Q:{effect.Sid}:{effect.Recipient}");
                    break;
                case SquidDeletionEffect.UnitLostSound:
                    Events.Add($"L:{effect.Sid}");
                    break;
                default:
                    throw new InvalidOperationException("알 수 없는 삭제 수명주기 효과");
            }
        }

        /// <summary>공통 몸체가 다루지 않는 종속·전파 효과를 기록한다.</summary>
        /// <param name="effect">나온 파생 효과.</param>
        private void Forward(SquidDestroyEvent effect)
        {
            if (effect.Effect != SquidDestroyEffect.Transmit)
            {
                throw new InvalidOperationException("장면에 없는 종속 가상 효과");
            }
            Events.Add($"T:{effect.Sid}:{effect.Flags}");
        }

        /// <summary>사전·사후를 직접 호출하는 경로다. 깊이가 한 번씩 감긴다.</summary>
        public void RunPair()
        {
            Lifecycle.PreDestroy(Root, Flags);
            Lifecycle.PostDestroy(Root, Flags);
        }
    }

    /// <summary>장부의 현재 값들을 비교용으로 복사한다.</summary>
    /// <param name="state">복사할 장부.</param>
    private static (int TotalCost, uint ProductionDirty, uint Depth, uint[] Local, uint[] Secondary, uint[] Global,
        uint ProviderCount, uint[] Providers, uint FactoryCount, uint[] Factories, uint[][] Owners) Snapshot(RawPostPopState state) =>
    (
        state.TotalCost,
        state.ProductionDirty,
        state.Depth,
        (uint[])state.LocalCounts.Clone(),
        (uint[])state.LocalSecondaryCounts.Clone(),
        (uint[])state.GlobalCounts.Clone(),
        state.Providers.Count,
        (uint[])state.Providers.Entries.Clone(),
        state.Factories.Count,
        (uint[])state.Factories.Entries.Clone(),
        state.OwnerFactories.Select(list => (uint[])list.Entries.Clone()).ToArray()
    );

    /// <summary>장부 복사본이 같은지 확인한다.</summary>
    /// <param name="expected">기대값.</param>
    /// <param name="actual">실제 장부.</param>
    private static void AssertSame((int TotalCost, uint ProductionDirty, uint Depth, uint[] Local, uint[] Secondary, uint[] Global,
        uint ProviderCount, uint[] Providers, uint FactoryCount, uint[] Factories, uint[][] Owners) expected, RawPostPopState actual)
    {
        Assert.Equal(expected.TotalCost, actual.TotalCost);
        Assert.Equal(expected.ProductionDirty, actual.ProductionDirty);
        Assert.Equal(expected.Depth, actual.Depth);
        Assert.Equal(expected.Local, actual.LocalCounts);
        Assert.Equal(expected.Secondary, actual.LocalSecondaryCounts);
        Assert.Equal(expected.Global, actual.GlobalCounts);
        Assert.Equal(expected.ProviderCount, actual.Providers.Count);
        Assert.Equal(expected.Providers, actual.Providers.Entries);
        Assert.Equal(expected.FactoryCount, actual.Factories.Count);
        Assert.Equal(expected.Factories, actual.Factories.Entries);
        // 소유자별 작업장 목록도 같은 순서로 비교한다
        for (int i = 0; i < RawPostPopState.OwnerCount; i++)
        {
            Assert.Equal(expected.Owners[i], actual.OwnerFactories[i].Entries);
        }
    }

    /// <summary>
    /// 생성은 현재 수와 비용을 되돌리고 누적 생산 수만 늘린 채 유지하는지 확인한다.
    /// 공급원과 작업장은 등록 때와 삭제 때 각각 한 번씩 재계산 알림을 낸다(합계 4회).
    /// </summary>
    [Fact]
    public void PostPopSharesCurrentCountsAndPreservesMade()
    {
        string[] args = Basic();
        // 공급원(0x10000000)·작업장(0x200) 플래그를 켠다
        args[1] = "268435456";
        args[2] = "512";
        args[14] = "0";
        var scene = new Scene(args);
        var before = Snapshot(scene.Bookkeeping);
        var post = new RawPostPop(scene.Pool, scene.Types, scene.Bookkeeping);
        post.Activate(Root, 1);
        uint made = scene.Bookkeeping.LocalSecondaryCounts[RootType];
        scene.Destroy.Destroy(Root, scene.Flags, scene.Lifecycle.Hooks());
        Assert.Equal(before.TotalCost, scene.Bookkeeping.TotalCost);
        Assert.Equal(before.Local, scene.Bookkeeping.LocalCounts);
        Assert.Equal(before.Global, scene.Bookkeeping.GlobalCounts);
        Assert.Equal(made, scene.Bookkeeping.LocalSecondaryCounts[RootType]);
        Assert.Equal(unchecked(before.Secondary[RootType] + 1), made);
        Assert.Equal(0u, scene.Bookkeeping.Providers.Count);
        Assert.Equal(0u, scene.Bookkeeping.Factories.Count);
        Assert.Equal(0u, scene.Bookkeeping.OwnerFactories[1].Count);
        // 등록과 삭제가 각각 공급원·작업장 재계산 알림을 한 번씩 낸다
        Assert.Equal(4u, scene.Bookkeeping.ProductionDirty);
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
    }

    /// <summary>
    /// 그래프·AI·손상 목록·누락 외부 효과는 장부 쓰기와 깊이 감소 전에 거부하는지 확인한다.
    /// </summary>
    [Fact]
    public void RejectsUnsupportedEffectsBeforeBookkeepingWrites()
    {
        string[] args = Basic();
        // 표면(0x800)·공급원 플래그를 켠다
        args[1] = "268437504";
        args[2] = "512";
        var scene = new Scene(args);
        var before = Snapshot(scene.Bookkeeping);
        uint poolBytes = X86Fixture.Adler32(scene.Pool.Bytes);
        // 그래프가 켜졌는데 그래프가 없으면 사전·사후 모두 거부한다
        scene.Bookkeeping.GraphsEnabled = true;
        Assert.Throws<InvalidOperationException>(() => scene.Lifecycle.PreDestroy(Root, 0));
        AssertSame(before, scene.Bookkeeping);
        Assert.Throws<InvalidOperationException>(() => scene.Lifecycle.PostDestroy(Root, 0));
        AssertSame(before, scene.Bookkeeping);
        scene.Bookkeeping.GraphsEnabled = false;
        // AI 가 붙은 소유자의 삭제는 통지 미복원 상태로 거부한다
        scene.Bookkeeping.AiAttached[1] = true;
        Assert.Throws<InvalidOperationException>(() => scene.Lifecycle.PreDestroy(Root, scene.Flags));
        AssertSame(before, scene.Bookkeeping);
        scene.Bookkeeping.AiAttached[1] = false;
        // 손상된 공급원 목록(개수가 배열보다 큼)은 쓰기 전에 거부한다
        scene.Bookkeeping.Providers.Count = 7;
        var corrupt = Snapshot(scene.Bookkeeping);
        Assert.Throws<InvalidOperationException>(() => scene.Lifecycle.PreDestroy(Root, scene.Flags));
        AssertSame(corrupt, scene.Bookkeeping);
        scene.Bookkeeping.Providers.Count = 6;
        // 외부 콜백이 없으면 보상·선택 해제 효과를 요구하는 삭제를 거부한다
        var missing = new RawSquidDestroyLifecycle(scene.Pool, scene.Types, scene.Bookkeeping, scene.State, scene.Destroy, new SquidDeletionHooks());
        Assert.Throws<InvalidOperationException>(() => missing.PreDestroy(Root, 0));
        scene.State.Selected = Root;
        Assert.Throws<InvalidOperationException>(() => missing.PreDestroy(Root, scene.Flags));
        // 거부된 삭제는 풀·슬롯·깊이를 바꾸지 않는다
        Assert.Equal(poolBytes, X86Fixture.Adler32(scene.Pool.Bytes));
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
    }

    /// <summary>
    /// 억제는 사전 통계만 생략하고 사후 비용은 남기는지, 비유한 비용은 거부하는지 확인한다.
    /// </summary>
    [Fact]
    public void SuppressionAndCostBoundaryGuards()
    {
        string[] args = Basic();
        // 억제 상태로 바꾼다
        args[10] = "1";
        var scene = new Scene(args);
        int before = scene.Bookkeeping.TotalCost;
        scene.RunPair();
        // 억제 상태에서도 ordinary 비용(600)은 차감된다
        Assert.Equal(before - 600, scene.Bookkeeping.TotalCost);
        // 직접 부른 사전·사후는 깊이 카운터를 한 번씩 감근다
        Assert.Equal(uint.MaxValue, scene.Destroy.PreDepth);
        Assert.Equal(uint.MaxValue, scene.Destroy.PostDepth);
        // 비용이 무한대면 쓰기 전에 거부한다
        scene.Types[RootType].Cost = float.PositiveInfinity;
        var invalid = new RawSquidDestroyLifecycle(scene.Pool, scene.Types, scene.Bookkeeping, scene.State, scene.Destroy, new SquidDeletionHooks());
        int cost = scene.Bookkeeping.TotalCost;
        Assert.Throws<ArgumentException>(() => invalid.PostDestroy(Root, 0));
        Assert.Equal(cost, scene.Bookkeeping.TotalCost);
        Assert.Equal(uint.MaxValue, scene.Destroy.PostDepth);
    }

    /// <summary>
    /// 다른 풀 연결과 자산이 아닌 루트·내용물 루트를 명시적으로 거부하는지 확인한다.
    /// </summary>
    [Fact]
    public void RejectsWrongPoolAndFormOrContained()
    {
        var scene = new Scene(Basic());
        var other = new RawSidPool(32768, true);
        Assert.Throws<ArgumentException>(() =>
            new RawSquidDestroyLifecycle(other, scene.Types, scene.Bookkeeping, scene.State, scene.Destroy, new SquidDeletionHooks()));
        // 자산이 아닌 타입은 공통 삭제 대상이 아니다
        Span<byte> raw = scene.Pool.AllocatedBytes(Root);
        raw[10] = 20;
        Assert.Throws<InvalidOperationException>(() => scene.Lifecycle.PreDestroy(Root, scene.Flags));
        // 내용물이 든 루리도 공통 삭제 대상이 아니다
        raw[10] = RootType;
        raw[11] |= 8;
        Assert.Throws<InvalidOperationException>(() => scene.Lifecycle.PostDestroy(Root, 0));
        Assert.Equal(0u, scene.Destroy.PreDepth);
        Assert.Equal(0u, scene.Destroy.PostDepth);
    }
}
