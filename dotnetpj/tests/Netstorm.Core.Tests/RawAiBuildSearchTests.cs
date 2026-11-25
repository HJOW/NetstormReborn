// 실제 574 상태와 spot으로 초기화/행 재개/시간 관문/거리 저장을 검사한다.
using System.Buffers.Binary;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// AI 배치 탐색을 검사하는 클래스다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.
// 근거: cpppj tests/AiBuildSearchTests.cpp의 13건 전부다.
public sealed class RawAiBuildSearchTests
{
    /// <summary>검사의 opaque32 TYPE 표 기준이며 500 stride를 보존한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private const uint TypeBase = 0x3e000000u;

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>같은 실제 본체/TYPE/spot과 명시 벽시각/배치 효과를 준비한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    private sealed class AiSearchScene
    {
        /// <summary>실제 본체와 고유 핸들이다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x4d000000u, bytes => bytes.Fill(0x5a));

        /// <summary>실제 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>현재 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>월드 spot이다.</summary>
        public readonly byte[] Spots = Enumerable.Repeat((byte)2, 65536).ToArray();

        /// <summary>명시 하위다.</summary>
        public AiBuildSearchHooks Hooks = new();

        /// <summary>실제 호출이다.</summary>
        public readonly List<CanonPlacementQuery> Queries = [];

        /// <summary>벽시각 입력이다.</summary>
        public List<double> Times = [];

        /// <summary>시계 호출 횟수다.</summary>
        public int ClockCalls;

        /// <summary>현재 벽시각이다.</summary>
        public double Now;

        /// <summary>배치 결과다.</summary>
        public bool Allow = true;

        /// <summary>기본 범위는 x 2..4/y 2..3이고 모든 칸이 비어 있다.</summary>
        public AiSearchScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Types[80].FootX = 1;
            Types[80].FootY = 1;
            Types[81].FootX = 1;
            Types[81].FootY = 1;
            Hooks.Wall = () =>
            {
                int index = ClockCalls++;
                if (Times.Count == 0)
                {
                    return Now;
                }

                Assert.True(index < Times.Count);
                return Times[index];
            };
            Hooks.MayPlace = query => { Queries.Add(query); return Allow; };
            Make().Initialize(Token, 80, new AiBuildSearchBounds(1, 1, 5, 4), new RawPathPoint(0, 0), 0x102);
        }

        /// <summary>현재 state 상대 DWORD만 쓴다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
        /// <param name="at">상태 상대 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), 0x574 + at, value);

        /// <summary>현재 state 상대 DWORD를 읽는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
        /// <param name="at">상태 상대 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), 0x574 + at);

        /// <summary>같은 현재 저장소/전역 효과를 빌린다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
        /// <returns>검사할 배치 탐색 몸체다.</returns>
        public RawAiBuildSearch Make() => new(Memory, Types, Spots, TypeBase, Hooks);
    }

    /// <summary>footprint를 더한 왼쪽/위 절삭과 전체 owner/cache를 쓰고 34 BYTE 주변 원문을 보존한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void InitializeRawStateBoundsAndSurroundings()
    {
        var input = new AiSearchScene();
        input.Types[80].FootX = 2;
        input.Types[80].FootY = 3;
        byte[] before = (byte[])input.Memory.Bytes(input.Token).Clone();
        float nan = BitConverter.UInt32BitsToSingle(0x7fc01234u);
        input.Make().Initialize(input.Token, 80, new AiBuildSearchBounds(-1.9f, -2.9f, 7.9f, 9.9f), new RawPathPoint(nan, -0.0f), 0xdead0102);
        Assert.Equal(80u, input.Field(0));
        Assert.Equal(0u, input.Field(4));
        Assert.Equal(0xc61c4000u, input.Field(8));
        Assert.Equal(0xc61c4000u, input.Field(0xc));
        Assert.Equal(0x7fc01234u, input.Field(0x10));
        Assert.Equal(0x80000000u, input.Field(0x14));
        Assert.Equal(TypeBase + 80 * 500, input.Field(0x18));
        Assert.Equal(0xdead0102u, input.Field(0x1c));
        Assert.Equal(0u, input.Field(0x20));
        Assert.Equal(0u, input.Field(0x24));
        Assert.Equal(7u, input.Field(0x28));
        Assert.Equal(9u, input.Field(0x2c));
        Assert.Equal(0u, input.Field(0x30));
        byte[] raw = input.Memory.Bytes(input.Token);
        Assert.True(before.AsSpan(0, 0x574).SequenceEqual(raw.AsSpan(0, 0x574)));
        Assert.True(before.AsSpan(0x5a8).SequenceEqual(raw.AsSpan(0x5a8)));
    }

    /// <summary>3f 마스크/전체 footprint를 검사하며 0 이하 폭/높이는 칸 접근 없이 성공할 수 있다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void FreeFootprintMaskAndEmptyDimensions()
    {
        var input = new AiSearchScene();
        input.Types[80].FootX = 2;
        input.Types[80].FootY = 3;
        input.Spots[5 * 256 + 4] = 0xc2;
        Assert.True(input.Make().FreeFootprint(80, 5, 6));
        input.Spots[5 * 256 + 4] = 0x42;
        Assert.True(input.Make().FreeFootprint(80, 5, 6));
        input.Spots[5 * 256 + 4] = 3;
        Assert.False(input.Make().FreeFootprint(80, 5, 6));
        input.Types[80].FootY = 0;
        Assert.True(input.Make().FreeFootprint(80, -2, -2));
        input.Types[80].FootY = 3;
        input.Types[80].FootX = 0;
        Assert.True(input.Make().FreeFootprint(80, -2, -2));
    }

    /// <summary>TYPE 0은 시계도 요구하지 않고 TYPE 비영의 빈 행 범위는 최초 시계만 읽는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void TypeZeroAndCompletedClockOrder()
    {
        var zero = new AiSearchScene();
        zero.Field(0, 0);
        zero.Hooks = new AiBuildSearchHooks();
        zero.Make().Scan(zero.Token, 1);
        Assert.Equal(0, zero.ClockCalls);
        var empty = new AiSearchScene();
        empty.Field(0x30, empty.Field(0x2c));
        empty.Hooks.MayPlace = null;
        empty.Make().Scan(empty.Token, 1);
        Assert.Equal(1, empty.ClockCalls);
        Assert.Empty(empty.Queries);
    }

    /// <summary>순서대로 strict 최대 거리만 전체 배치에 넘기며 owner는 현재 BYTE/나머지 인자는 원본값이다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void FarthestCandidateAndActualQueryArguments()
    {
        var input = new AiSearchScene();
        input.Make().Scan(input.Token, 0);
        Assert.Equal(1, input.ClockCalls);
        Assert.Equal(4, input.Queries.Count);
        Assert.Equal(BitConverter.SingleToUInt32Bits(25.0f), input.Field(4));
        Assert.Equal(BitConverter.SingleToUInt32Bits(4.0f), input.Field(8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(3.0f), input.Field(0xc));
        Assert.Equal(4u, input.Field(0x30));
        Assert.Equal(80u, input.Field(0));
        // 선택된 네 후보의 현재 인자를 비교하며 실제 허용 결과를 다시 계산하지 않는 반복문이다.
        foreach (var query in input.Queries)
        {
            Assert.Equal(80u, query.Type);
            Assert.Equal(0u, query.Argument);
            Assert.Equal(0u, query.Flags);
            Assert.Equal(2u, query.Owner);
            Assert.Equal(1u, query.Mode);
        }

        Assert.Equal(2f, input.Queries[0].X);
        Assert.Equal(2f, input.Queries[0].Y);
        Assert.Equal(4f, input.Queries[3].X);
        Assert.Equal(3f, input.Queries[3].Y);
        input.Make().Scan(input.Token, 0);
        Assert.Equal(2, input.ClockCalls);
        Assert.Equal(4, input.Queries.Count);
    }

    /// <summary>동률은 앞선 후보를 유지하고 거리 0/NaN은 최초 score 0보다 크지 않다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void StrictTieZeroAndUnorderedScore()
    {
        var tie = new AiSearchScene();
        tie.Make().Initialize(tie.Token, 80, new AiBuildSearchBounds(0, 0, 3, 2), new RawPathPoint(1.5f, 1), 2);
        tie.Make().Scan(tie.Token, 0);
        Assert.Single(tie.Queries);
        Assert.Equal(BitConverter.SingleToUInt32Bits(1.0f), tie.Field(8));
        var zero = new AiSearchScene();
        zero.Make().Initialize(zero.Token, 80, new AiBuildSearchBounds(0, 0, 2, 2), new RawPathPoint(1, 1), 2);
        zero.Make().Scan(zero.Token, 0);
        Assert.Empty(zero.Queries);
        Assert.Equal(0xc61c4000u, zero.Field(8));
        var nan = new AiSearchScene();
        nan.Field(4, 0x7fc01234u);
        nan.Make().Scan(nan.Token, 0);
        Assert.Empty(nan.Queries);
        Assert.Equal(0x7fc01234u, nan.Field(4));
    }

    /// <summary>spot 탈락은 현재 TYPE를 조회하지 않고 선택된 칸에서만 실제 footprint를 요구한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void SpotGateKeepsLazyTypeAccess()
    {
        var input = new AiSearchScene();
        input.Field(0, 200);
        Array.Fill(input.Spots, (byte)0);
        input.Make().Scan(input.Token, 0);
        Assert.Empty(input.Queries);
        Assert.Equal(4u, input.Field(0x30));
        input.Field(0x30, 2);
        input.Spots[2 * 256 + 2] = 2;
        Assert.ThrowsAny<Exception>(() => input.Make().Scan(input.Token, 0));
        Assert.Equal(2u, input.Field(0x30));
    }

    /// <summary>현재 행을 완료하고 다음 행 예산에서 멈추며 다음 호출은 그 행부터 현재 score를 유지해 재개한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void BudgetedRowPauseAndResume()
    {
        var input = new AiSearchScene();
        input.Times = [0, 0.02, 0.031];
        input.Make().Scan(input.Token, 1);
        Assert.Equal(3, input.ClockCalls);
        Assert.Equal(3u, input.Field(0x30));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20.0f), input.Field(4));
        Assert.Equal(3, input.Queries.Count);
        input.Times = [1, 1];
        input.ClockCalls = 0;
        input.Make().Scan(input.Token, 1);
        Assert.Equal(2, input.ClockCalls);
        Assert.Equal(4u, input.Field(0x30));
        Assert.Equal(BitConverter.SingleToUInt32Bits(25.0f), input.Field(4));
        Assert.Equal(4, input.Queries.Count);
    }

    /// <summary>deadline 동일은 진행하고 unordered는 중단한다. 비예산 모드는 최초 시각 NaN도 행 완료를 막지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void BudgetEqualityAndUnorderedClock()
    {
        var equal = new AiSearchScene();
        equal.Times = [0, (double)0.03f, double.NaN];
        equal.Make().Scan(equal.Token, 1);
        Assert.Equal(3u, equal.Field(0x30));
        Assert.Equal(3, equal.Queries.Count);
        var unordered = new AiSearchScene();
        unordered.Times = [double.NaN, 0];
        unordered.Make().Scan(unordered.Token, 1);
        Assert.Equal(2u, unordered.Field(0x30));
        Assert.Empty(unordered.Queries);
        var full = new AiSearchScene();
        full.Times = [double.NaN];
        full.Make().Scan(full.Token, 0);
        Assert.Equal(1, full.ClockCalls);
        Assert.Equal(4u, full.Field(0x30));
    }

    /// <summary>배치 효과가 시간을 초과시켜도 행 안에서는 시계를 추가 조회하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void BudgetChecksOnlyBetweenRows()
    {
        var input = new AiSearchScene();
        input.Hooks.MayPlace = query => { input.Queries.Add(query); input.Now = 1; return true; };
        input.Make().Scan(input.Token, 1);
        Assert.Equal(3, input.ClockCalls);
        Assert.Equal(3, input.Queries.Count);
        Assert.Equal(3u, input.Field(0x30));
        Assert.Equal(BitConverter.SingleToUInt32Bits(4.0f), input.Field(8));
    }

    /// <summary>행 snapshot과 cached TYPE/owner의 현재 재조회 및 배치 후 무조건 score 저장을 보존한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentCachedTypeOwnerAndRowSnapshot()
    {
        var input = new AiSearchScene();
        input.Hooks.MayPlace = query =>
        {
            input.Queries.Add(query);
            input.Field(4, BitConverter.SingleToUInt32Bits(100000.0f));
            input.Field(0x18, TypeBase + 81 * 500);
            input.Field(0x1c, 0x1ff);
            input.Field(0x20, 100);
            input.Field(0x30, 10);
            return true;
        };
        input.Make().Scan(input.Token, 0);
        Assert.Equal(3, input.Queries.Count);
        Assert.Equal(80u, input.Queries[0].Type);
        Assert.Equal(81u, input.Queries[1].Type);
        Assert.Equal(255u, input.Queries[1].Owner);
        Assert.Equal(2f, input.Queries[2].Y);
        Assert.Equal(BitConverter.SingleToUInt32Bits(20.0f), input.Field(4));
        Assert.Equal(11u, input.Field(0x30));
        Assert.Equal(100u, input.Field(0x20));
    }

    /// <summary>maxX는 후보 효과 뒤 현재값을 읽고 cached TYPE/선택 하위 누락은 앞선 시계 효과까지만 보존한다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentMaxAndSelectedMissingEffect()
    {
        var bound = new AiSearchScene();
        bound.Field(0x2c, 3);
        bound.Hooks.MayPlace = query => { bound.Queries.Add(query); bound.Field(0x28, 3); return true; };
        bound.Make().Scan(bound.Token, 0);
        Assert.Single(bound.Queries);
        Assert.Equal(3u, bound.Field(0x30));
        var invalid = new AiSearchScene();
        invalid.Field(0x18, TypeBase + 1);
        Assert.ThrowsAny<Exception>(() => invalid.Make().Scan(invalid.Token, 0));
        Assert.Equal(1, invalid.ClockCalls);
        Assert.Equal(0u, invalid.Field(4));
        Assert.Equal(2u, invalid.Field(0x30));
        var missing = new AiSearchScene();
        missing.Hooks.MayPlace = null;
        Assert.ThrowsAny<Exception>(() => missing.Make().Scan(missing.Token, 0));
        Assert.Equal(1, missing.ClockCalls);
        Assert.Equal(0u, missing.Field(4));
        Assert.Equal(2u, missing.Field(0x30));
    }

    /// <summary>x 차이는 float 저장/y 차이는 미축소이며 거리 비교는 float score 저장보다 먼저다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void DistanceComparisonBeforeFloatStore()
    {
        var input = new AiSearchScene();
        input.Make().Initialize(input.Token, 80, new AiBuildSearchBounds(0, 0, 2, 2), new RawPathPoint(-33554432.0f, -33554432.0f), 2);
        uint same = BitConverter.SingleToUInt32Bits((float)(1ul << 51));
        input.Field(4, same);
        input.Make().Scan(input.Token, 0);
        Assert.Single(input.Queries);
        Assert.Equal(same, input.Field(4));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1.0f), input.Field(8));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1.0f), input.Field(0xc));
    }

    /// <summary>초기화의 TYPE 조회 실패는 먼저 쓴 TYPE를 유지하며 지도 밖 주소를 합법 칸으로 clamp하지 않는다. 2026-11-25 R4-5b 배치 탐색 이식에서 추가됐다.</summary>
    [Fact]
    public void InitializeFailurePrefixAndPhysicalBounds()
    {
        var input = new AiSearchScene();
        uint cached = input.Field(0x18);
        Assert.ThrowsAny<Exception>(() => input.Make().Initialize(input.Token, 200, new AiBuildSearchBounds(), new RawPathPoint(), 2));
        Assert.Equal(200u, input.Field(0));
        Assert.Equal(cached, input.Field(0x18));
        var address = new AiSearchScene();
        address.Field(0x20, 0xffffffffu);
        Assert.ThrowsAny<Exception>(() => address.Make().Scan(address.Token, 0));
        Assert.Equal(1, address.ClockCalls);
        Assert.Equal(2u, address.Field(0x30));
    }
}
