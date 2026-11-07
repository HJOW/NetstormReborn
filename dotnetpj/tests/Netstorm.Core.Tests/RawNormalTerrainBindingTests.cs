// 일반지형 바인딩 계약의 분기·범위·연결 검사를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 지형 바인딩 계약을 검증하는 테스트 클래스다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.
public sealed class RawNormalTerrainBindingTests
{
    /// <summary>같은 연결 전역으로 유효 장면을 만든다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    private static (TerrainContextGlobals Globals, FortRandomPlacementState Placement, TerrainStampState Stamp, List<OriginalTypeRecord> Types, RawSidPool Pool) Scene()
    {
        var globals = new TerrainContextGlobals();
        var placement = new FortRandomPlacementState();
        var stamp = new TerrainStampState();
        var types = Enumerable.Range(0, OriginalConstructors.TypeCount).Select(_ => new OriginalTypeRecord()).ToList();
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity);
        types[(int)globals.IslandType].ConstructorAddress = OriginalConstructors.Address(pool.Edition, (int)globals.IslandType);
        types[(int)placement.EdgeFarmType].ConstructorAddress = OriginalConstructors.Address(pool.Edition, (int)placement.EdgeFarmType);
        return (globals, placement, stamp, types, pool);
    }

    /// <summary>일반 분기·내부 범위만 통과한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    [Fact]
    public void BranchAndBounds()
    {
        var valid = new NormalTerrainRequest(1u, new ChunkRect(3, 3, 5, 5), 127, NormalTerrainBranch.Normal);
        RawNormalTerrainChecks.RequireBranch(valid);
        RawNormalTerrainChecks.RequireBounds(valid);
        Assert.Throws<InvalidOperationException>(() => RawNormalTerrainChecks.RequireBranch(valid with { Branch = NormalTerrainBranch.Archipelago }));
        Assert.Throws<InvalidOperationException>(() => RawNormalTerrainChecks.RequireBranch(valid with { Branch = NormalTerrainBranch.UnrestoredParent }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(0, 3, 5, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 0, 5, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 3, 16, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 3, 5, 16) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(5, 3, 5, 5) }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.RequireBounds(valid with { Bounds = new ChunkRect(3, 5, 5, 5) }));
    }

    /// <summary>군도 상태에서는 일반 전용 요청을 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    [Fact]
    public void NormalRejectsArchipelago()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var farm = new FarmClusterState();
        RawNormalTerrainChecks.RequireNormal(farm, globals, placement, stamp, types, pool);
        farm.Archipelago = true;
        Assert.Throws<InvalidOperationException>(() => RawNormalTerrainChecks.RequireNormal(farm, globals, placement, stamp, types, pool));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.RequireNormal(null!, globals, placement, stamp, types, pool));
    }

    /// <summary>타입·지역 전역 불일치와 생성자 단절을 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    [Fact]
    public void StorageGuardsConnections()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var farm = new FarmClusterState();
        RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool);
        placement.IslandType = 98u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        placement.IslandType = 97u;
        stamp.IslandType = 99u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        stamp.IslandType = 97u;
        stamp.EdgeType = 99u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        stamp.EdgeType = 98u;
        stamp.NoRegion = 0;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        stamp.NoRegion = 127;
        types[(int)globals.IslandType].ConstructorAddress += 1u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireNormal(farm, globals, placement, stamp, types, pool));
        globals.IslandType = 999u;
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.RequireStorage(globals, placement, stamp, types, pool));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.RequireStorage(null!, placement, stamp, types, pool));
    }

    /// <summary>모든 방향·번호를 덮는 합성 프레임 표를 만든다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    private static List<RiftTypeFrames> CoveringFrames()
    {
        var codes = new List<RiftFrameCode>();
        // 회전표의 변형 열이 항상 'P'이므로 방향 A~P·변형 'P'·번호 0~10을 모두 넣으면 빈 칸이 아닌 모든 셀과 만난다
        for (byte side = (byte)'A'; side <= (byte)'P'; side++)
        {
            for (byte number = 0; number <= 10; number++)
            {
                codes.Add(new RiftFrameCode(side, (byte)'P', number, 0));
            }
        }

        return [new RiftTypeFrames(codes)];
    }

    /// <summary>패턴 표에서 빈 칸이 아닌 셀 수를 센다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    /// <param name="pattern">셀 수를 셀 패턴이다.</param>
    private static uint NonEmptyCells(CanonPattern pattern)
    {
        uint count = 0u;
        // 줄 우선 Width×Height 격자만 순회한다
        for (int y = 0; y < pattern.Height; y++)
        {
            for (int x = 0; x < pattern.Width; x++)
            {
                if ((char)pattern.Cells[y * pattern.Width + x][1] != '.')
                {
                    ++count;
                }
            }
        }

        return count;
    }

    /// <summary>조각 번호 밖·기록 길이 밖 입력을 거부한다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    [Fact]
    public void PatternCountGuardsRanges()
    {
        List<RiftTypeFrames> frames = CoveringFrames();
        byte[] record = [3, 1, 0, 7, 0, 0];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.PatternCount(frames, 1u, record, OriginalEdition.Patch1078, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawNormalTerrainChecks.PatternCount([], 0u, record, OriginalEdition.Patch1078, null));
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.PatternCount(frames, 0u, [3, 1, 0, 7, 0], OriginalEdition.Patch1078, null));
        Assert.Throws<ArgumentException>(() => RawNormalTerrainChecks.PatternCount(frames, 0u, [3, 1, 0, 7, 0, 0, 0], OriginalEdition.Patch1078, null));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.PatternCount(null!, 0u, record, OriginalEdition.Patch1078, null));
    }

    /// <summary>빈 프레임 표에서는 유효 칸이 0이다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    [Fact]
    public void PatternCountEmptyFramesYieldsZero()
    {
        var frames = new List<RiftTypeFrames> { new([]) };
        byte[] record = [3, 1, 0, 7, 0, 0];
        Assert.Equal(0u, RawNormalTerrainChecks.PatternCount(frames, 0u, record, OriginalEdition.Patch1078, null));
    }

    /// <summary>덮는 프레임 표에서는 패턴의 빈 칸이 아닌 셀 수와 같다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    [Fact]
    public void PatternCountMatchesNonEmptyCells()
    {
        List<RiftTypeFrames> frames = CoveringFrames();
        ReadOnlySpan<CanonPattern> patterns = CanonDecoder.TerritoryPatterns();
        // 패턴 0·3·32와 회전 3의 같은 패턴 3을 각각 대조한다
        foreach (byte head in new byte[] { 0x00, 0x03, 0x20, 0xC3 })
        {
            byte[] record = [head, 1, 0, 7, 0, 0];
            int index = head & 0x3f;
            uint expected = NonEmptyCells(patterns[index]);
            Assert.Equal(expected, RawNormalTerrainChecks.PatternCount(frames, 0u, record, OriginalEdition.Patch1078, null));
            // 같은 입력의 독립 디코더 순회와도 같다
            var decoder = new CanonDecoder(frames[0], patterns[index], (head >> 6) * 2, 0f, 0f, null, OriginalEdition.Patch1078, null);
            uint manual = 0u;
            for (; decoder.Valid(); decoder.Advance())
            {
                ++manual;
            }

            Assert.Equal(expected, manual);
        }
    }

    /// <summary>첫 기록 조회의 섬 목록 장면을 만든다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    /// <param name="edition">섬 목록 풀 판본이다.</param>
    /// <param name="count">섬 목록 개수다.</param>
    /// <param name="existing">존재 DWORD가 0이 아닌 섬 번호들이다.</param>
    private static RawIslandRuntime IslandScene(OriginalEdition edition, uint count, params int[] existing)
    {
        var pool = new RawSidPool(RawSurfaceStorage.PoolCapacity, true, null, edition);
        var hash = new RawSquidHash();
        byte[] records = new byte[RawIslandStorage.Bytes];
        // 개수 DWORD를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            records[index] = (byte)(count >> (index * 8));
        }

        // 존재 DWORD를 같은 리틀 엔디언 순서로 쓴다
        foreach (int island in existing)
        {
            int record = 4 + island * IslandRuntimeShape.Stride + 4;
            records[record] = 1;
        }

        var modes = new FortFinishOwnerState();
        var residences = new RawPostPopList();
        uint revision = 0u;
        uint changed = 0u;
        var hooks = new IslandRuntimeHooks
        {
            Assertion = _ => { },
            SetResidenceOwner = (_, _) => { },
        };
        return new RawIslandRuntime(pool, hash, records, modes, residences,
            () => revision, value => revision = value,
            () => changed, value => changed = value, hooks);
    }

    /// <summary>청크 한 칸의 섬·기록을 둔다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    /// <param name="chunks">쓸 청크 지도다.</param>
    /// <param name="x">가로 청크 좌표다.</param>
    /// <param name="y">세로 청크 좌표다.</param>
    /// <param name="island">둘 섬 바이트다.</param>
    /// <param name="record">둘 6바이트 기록이다.</param>
    private static void PutChunk(ChunkMap chunks, int x, int y, byte island, byte[] record)
    {
        chunks.At(x, y).Island = island;
        chunks.At(x, y).Record = (byte[])record.Clone();
    }

    /// <summary>빈 입력·null 입력을 거부한다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    [Fact]
    public void FirstRecordGuardsNulls()
    {
        var chunks = new ChunkMap();
        RawIslandRuntime islands = IslandScene(OriginalEdition.Patch1078, 1u, 0);
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.FirstRecord(null!, islands, 0, OriginalEdition.Patch1078, null));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.FirstRecord(chunks, null!, 0, OriginalEdition.Patch1078, null));
    }

    /// <summary>Y 바깥·X 안쪽 순서의 처음 일치를 반환한다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    [Fact]
    public void FirstRecordReturnsFirstInYOuterOrder()
    {
        var chunks = new ChunkMap();
        RawIslandRuntime islands = IslandScene(OriginalEdition.Patch1078, 4u, 0, 1, 2, 3);
        byte[] early = [7, 1, 2, 3, 4, 5];
        byte[] late = [9, 8, 7, 6, 5, 4];
        // y=0 행이 y=1 행보다 먼저이므로 (1,0)이 (0,1)보다 먼저다
        PutChunk(chunks, 1, 0, 2, early);
        PutChunk(chunks, 0, 1, 2, late);
        byte[] found = RawNormalTerrainChecks.FirstRecord(chunks, islands, 2, OriginalEdition.Patch1078, null);
        Assert.Equal(early, found);
        // 반환은 복사본이므로 호출자 쓰기가 지도를 바꾸지 않는다
        found[0] = 0xAA;
        Assert.Equal(early, RawNormalTerrainChecks.FirstRecord(chunks, islands, 2, OriginalEdition.Patch1078, null));
    }

    /// <summary>CD에서만 누락 진단을 순서대로 보고하고 패치에서는 조용히 영 기록을 둔다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    [Fact]
    public void FirstRecordReportsCdPostpassOnly()
    {
        var chunks = new ChunkMap();
        byte[] hit = [3, 1, 0, 7, 0, 0];
        PutChunk(chunks, 4, 5, 1, hit);
        // 패치 빗나감은 진단 없이 영 기록이다
        RawIslandRuntime patchMissing = IslandScene(OriginalEdition.Patch1078, 0u);
        int patchCalls = 0;
        byte[] patchMiss = RawNormalTerrainChecks.FirstRecord(chunks, patchMissing, 2, OriginalEdition.Patch1078, _ => patchCalls++);
        Assert.Equal(new byte[6], patchMiss);
        Assert.Equal(0, patchCalls);
        // 패치 적중은 섬 부재에도 진단하지 않는다
        byte[] patchHit = RawNormalTerrainChecks.FirstRecord(chunks, patchMissing, 1, OriginalEdition.Patch1078, _ => patchCalls++);
        Assert.Equal(hit, patchHit);
        Assert.Equal(0, patchCalls);
        // CD 빗나감은 부재 뒤 못 찾음을 순서대로 보고한다
        RawIslandRuntime cdMissing = IslandScene(OriginalEdition.Cd1072, 0u);
        var calls = new List<TerrainFarmPostpassAssertion>();
        byte[] cdMiss = RawNormalTerrainChecks.FirstRecord(chunks, cdMissing, 2, OriginalEdition.Cd1072, calls.Add);
        Assert.Equal(new byte[6], cdMiss);
        Assert.Equal([TerrainFarmPostpassAssertion.RecordMissingIsland, TerrainFarmPostpassAssertion.RecordNotFound], calls);
        // CD 적중은 부재만 보고하고 못 찾음은 생략한다
        calls.Clear();
        byte[] cdHitMissing = RawNormalTerrainChecks.FirstRecord(chunks, cdMissing, 1, OriginalEdition.Cd1072, calls.Add);
        Assert.Equal(hit, cdHitMissing);
        Assert.Equal([TerrainFarmPostpassAssertion.RecordMissingIsland], calls);
        // CD에 섬은 있으나 칸이 없으면 못 찾음만 보고한다
        RawIslandRuntime cdExisting = IslandScene(OriginalEdition.Cd1072, 4u, 0, 1, 2, 3);
        calls.Clear();
        byte[] cdNotFound = RawNormalTerrainChecks.FirstRecord(chunks, cdExisting, 3, OriginalEdition.Cd1072, calls.Add);
        Assert.Equal(new byte[6], cdNotFound);
        Assert.Equal([TerrainFarmPostpassAssertion.RecordNotFound], calls);
    }

    /// <summary>섬 바이트를 부호 있는 지역과 비교한다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    [Fact]
    public void FirstRecordComparesSignedIsland()
    {
        var chunks = new ChunkMap();
        RawIslandRuntime islands = IslandScene(OriginalEdition.Patch1078, 4u, 0, 1, 2, 3);
        byte[] record = [1, 2, 3, 4, 5, 6];
        // 0xFF는 부호 있게 -1이다
        PutChunk(chunks, 2, 3, 0xFF, record);
        Assert.Equal(record, RawNormalTerrainChecks.FirstRecord(chunks, islands, -1, OriginalEdition.Patch1078, null));
        // 부호 없는 255로는 만나지 않고 영 기록이다
        Assert.Equal(new byte[6], RawNormalTerrainChecks.FirstRecord(chunks, islands, 255, OriginalEdition.Patch1078, null));
    }

    /// <summary>후처리 배치 효과를 기록한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    private sealed class PlaceSpy
    {
        /// <summary>받은 사각·지역·개수다.</summary>
        public readonly List<(FortRandomRectangle Cells, int Region, uint Count)> Calls = [];

        /// <summary>군집 몸체 대신 호출을 기록한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
        /// <param name="cells">받은 사각이다.</param>
        /// <param name="region">받은 지역이다.</param>
        /// <param name="count">받은 개수다.</param>
        public void Place(FortRandomRectangle cells, int region, uint count)
        {
            Calls.Add((cells, region, count));
        }
    }

    /// <summary>세 섬 장면으로 후처리를 실행한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    /// <param name="postpass">받을 진단 목록이다.</param>
    /// <param name="spy">받을 배치 기록이다.</param>
    /// <param name="selectedRegion">고를 지역이다.</param>
    private static void RunPostprocess(List<TerrainFarmPostpassAssertion> postpass, PlaceSpy spy, int selectedRegion)
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var chunks = new ChunkMap();
        byte[] record = [3, 1, 0, 7, 0, 0];
        PutChunk(chunks, 2, 2, 0, record);
        PutChunk(chunks, 4, 5, 1, record);
        PutChunk(chunks, 6, 7, 2, record);
        RawIslandRuntime islands = IslandScene(OriginalEdition.Patch1078, 3u, 0, 1, 2);
        RawNormalTerrainChecks.PostprocessCurrentFarms(chunks, islands, globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Patch1078, null, postpass.Add, selectedRegion, spy.Place);
    }

    /// <summary>빈 입력·null 입력을 거부한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    [Fact]
    public void PostprocessGuardsNulls()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var chunks = new ChunkMap();
        RawIslandRuntime islands = IslandScene(OriginalEdition.Patch1078, 1u, 0);
        var spy = new PlaceSpy();
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.PostprocessCurrentFarms(null!, islands, globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Patch1078, null, null, 127, spy.Place));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.PostprocessCurrentFarms(chunks, null!, globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Patch1078, null, null, 127, spy.Place));
        Assert.Throws<ArgumentNullException>(() => RawNormalTerrainChecks.PostprocessCurrentFarms(chunks, islands, globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Patch1078, null, null, 127, null!));
    }

    /// <summary>현재 noRegion이면 모두, 고른 지역이면 그것만 실행한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    [Fact]
    public void PostprocessRespectsSelectedRegion()
    {
        var postpass = new List<TerrainFarmPostpassAssertion>();
        var spy = new PlaceSpy();
        RunPostprocess(postpass, spy, 127);
        Assert.Equal([0, 1, 2], spy.Calls.ConvertAll(call => call.Region));
        Assert.Empty(postpass);
        postpass.Clear();
        spy.Calls.Clear();
        RunPostprocess(postpass, spy, 1);
        Assert.Equal([1], spy.Calls.ConvertAll(call => call.Region));
        Assert.Empty(postpass);
    }

    /// <summary>사각·개수를 같은 첫 기록·범위·패턴 개수와 같이 계산한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    [Fact]
    public void PostprocessComputesCellsAndCounts()
    {
        var postpass = new List<TerrainFarmPostpassAssertion>();
        var spy = new PlaceSpy();
        RunPostprocess(postpass, spy, 1);
        Assert.Single(spy.Calls);
        (FortRandomRectangle cells, int region, uint count) = spy.Calls[0];
        Assert.Equal(1, region);
        // 한 칸 (4,5)의 범위는 (4,5,5,6)이며 16을 곱한다
        Assert.Equal(new FortRandomRectangle(64f, 80f, 80f, 96f), cells);
        byte[] record = [3, 1, 0, 7, 0, 0];
        Assert.Equal(RawNormalTerrainChecks.PatternCount(CoveringFrames(), 0u, record, OriginalEdition.Patch1078, null), count);
    }

    /// <summary>판본별 누락 진단을 순서대로 보고한다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    [Fact]
    public void PostprocessReportsDiagnostics()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        // 패치판은 없는 섬의 Bounds 부재만 보고한다
        var chunks = new ChunkMap();
        PutChunk(chunks, 2, 2, 0, [3, 1, 0, 7, 0, 0]);
        RawIslandRuntime patch = IslandScene(OriginalEdition.Patch1078, 2u, 0);
        var calls = new List<TerrainFarmPostpassAssertion>();
        var spy = new PlaceSpy();
        RawNormalTerrainChecks.PostprocessCurrentFarms(chunks, patch, globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Patch1078, null, calls.Add, 127, spy.Place);
        Assert.Equal([TerrainFarmPostpassAssertion.BoundsMissingIsland], calls);
        Assert.Equal(2, spy.Calls.Count);
        // CD판은 없는 섬의 네 진단을 순서대로 보고한다
        RawIslandRuntime cd = IslandScene(OriginalEdition.Cd1072, 1u);
        calls.Clear();
        spy.Calls.Clear();
        RawNormalTerrainChecks.PostprocessCurrentFarms(new ChunkMap(), cd, globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Cd1072, null, calls.Add, 127, spy.Place);
        Assert.Equal(
            [TerrainFarmPostpassAssertion.RecordMissingIsland, TerrainFarmPostpassAssertion.RecordNotFound,
                TerrainFarmPostpassAssertion.BoundsMissingIsland, TerrainFarmPostpassAssertion.BoundsNotFound],
            calls);
        Assert.Single(spy.Calls);
        // 적중한 CD·패치는 조용하다
        calls.Clear();
        spy.Calls.Clear();
        RawNormalTerrainChecks.PostprocessCurrentFarms(chunks, IslandScene(OriginalEdition.Cd1072, 1u, 0), globals, placement, stamp, types, pool,
            0u, CoveringFrames(), OriginalEdition.Cd1072, null, calls.Add, 0, spy.Place);
        Assert.Empty(calls);
        Assert.Single(spy.Calls);
    }

    /// <summary>묶은 바인딩 장면을 만든다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private static (ChunkMap Chunks, RawIslandRuntime Islands, TerrainContextGlobals Globals, FortRandomPlacementState Placement, TerrainStampState Stamp, List<OriginalTypeRecord> Types, RawSidPool Pool, UIntBox Piece, List<RiftTypeFrames> Frames, NormalTerrainDiagnostics Diagnostics) BindingScene()
    {
        (TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, List<OriginalTypeRecord> types, RawSidPool pool) = Scene();
        var chunks = new ChunkMap();
        byte[] record = [3, 1, 0, 7, 0, 0];
        PutChunk(chunks, 2, 2, 0, record);
        RawIslandRuntime islands = IslandScene(pool.Edition, 1u, 0);
        return (chunks, islands, globals, placement, stamp, types, pool, new UIntBox { Value = 0u }, CoveringFrames(), new NormalTerrainDiagnostics());
    }

    /// <summary>묶은 바인딩을 만든다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private static RawNormalTerrainWorldBinding Bind(
        ChunkMap chunks, RawIslandRuntime islands, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp,
        List<OriginalTypeRecord> types, RawSidPool pool, UIntBox piece, List<RiftTypeFrames> frames, NormalTerrainDiagnostics diagnostics) =>
        new(chunks, islands, globals, placement, stamp, types, pool, piece, frames, diagnostics);

    /// <summary>빈 묶음·어긋난 연결을 거부한다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingCtorGuards()
    {
        (ChunkMap chunks, RawIslandRuntime islands, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp,
            List<OriginalTypeRecord> types, RawSidPool pool, UIntBox piece, List<RiftTypeFrames> frames, NormalTerrainDiagnostics diagnostics) = BindingScene();
        Assert.Throws<ArgumentNullException>(() => Bind(null!, islands, globals, placement, stamp, types, pool, piece, frames, diagnostics));
        Assert.Throws<ArgumentNullException>(() => Bind(chunks, islands, globals, placement, stamp, types, pool, null!, frames, diagnostics));
        Assert.Throws<ArgumentNullException>(() => Bind(chunks, islands, globals, placement, stamp, types, pool, piece, frames, null!));
        placement.IslandType = 98u;
        Assert.Throws<ArgumentException>(() => Bind(chunks, islands, globals, placement, stamp, types, pool, piece, frames, diagnostics));
    }

    /// <summary>묶은 호출이 같은 정적 몸체와 같다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingDelegatesToStatics()
    {
        (ChunkMap chunks, RawIslandRuntime islands, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp,
            List<OriginalTypeRecord> types, RawSidPool pool, UIntBox piece, List<RiftTypeFrames> frames, NormalTerrainDiagnostics diagnostics) = BindingScene();
        var binding = Bind(chunks, islands, globals, placement, stamp, types, pool, piece, frames, diagnostics);
        binding.RequireStorage();
        binding.RequireNormal(new FarmClusterState());
        byte[] record = [3, 1, 0, 7, 0, 0];
        Assert.Equal(RawNormalTerrainChecks.PatternCount(frames, 0u, record, pool.Edition, null), binding.PatternCount(record));
        Assert.Equal(RawNormalTerrainChecks.FirstRecord(chunks, islands, 0, pool.Edition, null), binding.FirstRecord(0));
        var left = new PlaceSpy();
        var right = new PlaceSpy();
        binding.PostprocessCurrentFarms(127, left.Place);
        RawNormalTerrainChecks.PostprocessCurrentFarms(chunks, islands, globals, placement, stamp, types, pool,
            0u, frames, pool.Edition, null, null, 127, right.Place);
        Assert.Equal(right.Calls.Count, left.Calls.Count);
        // 같은 사각·지역·개수 순서다
        for (int index = 0; index < right.Calls.Count; ++index)
        {
            Assert.Equal(right.Calls[index], left.Calls[index]);
        }
    }

    /// <summary>공유 조각 번호를 호출 때마다 다시 읽는다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingReadsSharedPuzzlePieceLive()
    {
        (ChunkMap chunks, RawIslandRuntime islands, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp,
            List<OriginalTypeRecord> types, RawSidPool pool, UIntBox piece, List<RiftTypeFrames> frames, NormalTerrainDiagnostics diagnostics) = BindingScene();
        var binding = Bind(chunks, islands, globals, placement, stamp, types, pool, piece, frames, diagnostics);
        byte[] record = [3, 1, 0, 7, 0, 0];
        Assert.Equal(RawNormalTerrainChecks.PatternCount(frames, 0u, record, pool.Edition, null), binding.PatternCount(record));
        piece.Value = 1u;
        Assert.Throws<ArgumentOutOfRangeException>(() => binding.PatternCount(record));
    }

    /// <summary>진단 묶음은 복사해서 가진다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void BindingClonesDiagnostics()
    {
        (ChunkMap chunks, RawIslandRuntime _, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp,
            List<OriginalTypeRecord> types, RawSidPool pool, UIntBox piece, List<RiftTypeFrames> frames, NormalTerrainDiagnostics diagnostics) = BindingScene();
        var first = new List<TerrainFarmPostpassAssertion>();
        diagnostics.Postpass = first.Add;
        // 섬 없이 개수는 0이어서 Bounds 부재만 보고한다
        RawIslandRuntime islands = IslandScene(pool.Edition, 1u);
        var binding = Bind(new ChunkMap(), islands, globals, placement, stamp, types, pool, piece, frames, diagnostics);
        var second = new List<TerrainFarmPostpassAssertion>();
        diagnostics.Postpass = second.Add;
        binding.PostprocessCurrentFarms(127, (_, _, _) => { });
        Assert.Equal([TerrainFarmPostpassAssertion.BoundsMissingIsland], first);
        Assert.Empty(second);
    }
}
