// 서피스 저장소 껍데기의 fresh 상태·진단·바람·대여 관문을 검증한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 서피스 저장소 껍데기를 검증하는 테스트 클래스다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.
public sealed class RawSurfaceStorageTests
{
    /// <summary>독립 저장소는 fresh 기본값으로 시작한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void DefaultsAreFreshSharedState()
    {
        var storage = new RawSurfaceStorage();
        Assert.True(storage.Authority().Server);
        Assert.True(storage.Authority().Boss);
        Assert.Equal(0.0f, storage.Wind.X);
        Assert.Equal(1.0f, storage.Wind.Y);
        Assert.Equal(0.0f, storage.Wind.Angle);
        Assert.Equal(0u, storage.DisallowSquidAllocation);
        Assert.False(storage.OwnerMode.Fort);
        Assert.False(storage.OwnerMode.Battle);
        Assert.False(storage.OwnerMode.Challenge);
        Assert.Equal(82u, storage.Links.BridgeType);
        Assert.Equal(94u, storage.Links.IslandType);
        Assert.Equal(95u, storage.Links.StalagType);
        Assert.Equal(155u, storage.Links.ConnectorType);
        Assert.Equal(157u, storage.Links.NoIslandType);
        Assert.Equal(new uint[RawSurfaceStorage.PriestOwnerCount], storage.PriestOwnerState);
        Assert.Equal(-10000.0f, storage.BoardTargetState.Invalid.X);
        Assert.Equal(-10000.0f, storage.BoardTargetState.Invalid.Y);
        Assert.Equal(-1, storage.BridgeDecayScan.First);
        Assert.Equal(-1, storage.BridgeDecayScan.Last);
        Assert.Equal(0, storage.BridgeDecayScan.Cursor);
        Assert.Equal(0.0, storage.BridgeDecayScan.Next);
        Assert.Equal(10.0f, storage.BridgeDecayPeriod);
        Assert.False(storage.Borrowed());
        Assert.Equal(0ul, storage.Generation());
    }

    /// <summary>CD 판본의 스캔 첫·마지막은 0을 유지한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CdScanDefaultsToZero()
    {
        var storage = new RawSurfaceStorage(OriginalEdition.Cd1072);
        Assert.Equal(0, storage.BridgeDecayScan.First);
        Assert.Equal(0, storage.BridgeDecayScan.Last);
    }

    /// <summary>넘긴 owner는 고정하고 넘기지 않으면 독립 owner를 만든다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void SharedOwnersArePinnedOrIndependent()
    {
        var authority = new RawAuthorityState();
        var gameAux = new RawGameAuxState();
        var first = new RawSurfaceStorage(OriginalEdition.Patch1078, authority, gameAux);
        Assert.Same(authority, first.Authority());
        Assert.Same(gameAux, first.GameAux());
        var second = new RawSurfaceStorage();
        Assert.NotSame(first.Authority(), second.Authority());
        Assert.NotSame(first.GameAux(), second.GameAux());
    }

    /// <summary>검사는 같은 owner를 복사해 호출 때마다 다시 읽는다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void CheckingBindReadsLiveOwner()
    {
        var storage = new RawSurfaceStorage();
        uint checking = 0u;
        storage.BindChecking(new RawCheckingFlag(() => checking));
        Assert.False(storage.Checking());
        checking = 0x80000000u;
        Assert.True(storage.Checking());
        Assert.Throws<ArgumentNullException>(() => storage.BindChecking(null!));
    }

    /// <summary>상세 기록·원문 진단은 같은 Client owner에 닿고 장치가 없으면 명시 실패한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void DisplayRecordAndAssert()
    {
        var storage = new RawSurfaceStorage();
        Assert.Throws<InvalidOperationException>(() => storage.RecordAssertionDetail("x"));
        Assert.Throws<InvalidOperationException>(() => storage.AssertDisplay("e", "f", 1u));
        var detail = new StringBox();
        string seen = string.Empty;
        storage.BindDisplayDiagnostics(detail, (expression, file, line) => seen = line + ":" + expression + ":" + file);
        storage.RecordAssertionDetail("문구");
        Assert.Equal("문구", detail.Value);
        storage.AssertDisplay("angle < 361.0f", "\\Ns\\O\\Boardcoord.cpp", 0xb3u);
        Assert.Equal("179:angle < 361.0f:\\Ns\\O\\Boardcoord.cpp", seen);
        Assert.Throws<ArgumentNullException>(() => storage.BindDisplayDiagnostics(null!, (_, _, _) => { }));
    }

    /// <summary>그래프 시계·로그는 기본 0·무출력이며 연결 뒤 현재 장치를 그대로 쓴다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void GraphClockAndTrace()
    {
        var storage = new RawSurfaceStorage();
        Assert.Equal(0.0, storage.ReadGraphClock());
        storage.TraceGraph(1, "조용");
        double now = 16777216.25;
        storage.BindGraphClock(() => now);
        Assert.Equal(now, storage.ReadGraphClock());
        int seenLevel = 0;
        string seenText = string.Empty;
        storage.BindGraphTrace((level, text) => { seenLevel = level; seenText = text; });
        storage.TraceGraph(2, "원문");
        Assert.Equal(2, seenLevel);
        Assert.Equal("원문", seenText);
    }

    /// <summary>바람 갱신은 같은 Client 난수를 직접 소비해 원문 1걸음과 비트가 같다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void WindUpdateMatchesNativeSingleStep()
    {
        string[][] rows = X86Fixture.ReadFile("ambient-wind-hardware-x86.tsv");
        string[]? row = rows.FirstOrDefault(candidate =>
            candidate.Length == 14 && candidate[0] == "originals" && candidate[1] == "0" &&
            candidate[2] == "55" && candidate[3] == "1" && candidate[7] == "0");
        Assert.True(row is not null, "원문 최초 바람 입력 누락");
        var storage = new RawSurfaceStorage();
        var random = new GameRandom(uint.Parse(row![2], CultureInfo.InvariantCulture));
        string events = string.Empty;
        storage.BindDisplayDiagnostics(new StringBox(), (expression, file, line) =>
        {
            if (events.Length > 0)
            {
                events += "|";
            }

            events += line.ToString(CultureInfo.InvariantCulture) + ":" + expression + ":" + file;
        });
        storage.UpdateAmbientWind(random);
        Assert.Equal(uint.Parse(row[8], CultureInfo.InvariantCulture), BitConverter.SingleToUInt32Bits(storage.Wind.X));
        Assert.Equal(uint.Parse(row[9], CultureInfo.InvariantCulture), BitConverter.SingleToUInt32Bits(storage.Wind.Y));
        Assert.Equal(uint.Parse(row[10], CultureInfo.InvariantCulture), BitConverter.SingleToUInt32Bits(storage.Wind.Angle));
        Assert.Equal(uint.Parse(row[11], CultureInfo.InvariantCulture), random.State());
        Assert.Equal(row[13], events.Length == 0 ? "-" : events);
    }

    /// <summary>각도 부모는 같은 현재 검사와 같은 실제 진단 장치를 값 뷰로 보관한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void AngleDiagnosticsUsesLiveCheckingAndForwardsAssertion()
    {
        var storage = new RawSurfaceStorage();
        uint checking = 0u;
        storage.BindChecking(new RawCheckingFlag(() => checking));
        int assertions = 0;
        storage.BindDisplayDiagnostics(new StringBox(), (_, _, _) => ++assertions);
        BoardAngleDiagnostics view = storage.AngleDiagnostics();
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Patch1078, view, 720.0));
        Assert.Equal(0, assertions);
        checking = 0x80000000u;
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Patch1078, view, 720.0));
        Assert.Equal(1, assertions);
        checking = 0u;
        Assert.Equal(359.9999f, (float)RawBoardAngle.Normalize(361.0f, OriginalEdition.Patch1078, view, 720.0));
        Assert.Equal(1, assertions);
    }

    /// <summary>간헐천 타입은 같은 게임 보조 owner의 현재 DWORD다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void GeyserTypePassthrough()
    {
        var gameAux = new RawGameAuxState();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, null, gameAux);
        Assert.Equal(122u, storage.GeyserType);
        storage.GeyserType = 130u;
        Assert.Equal(130u, gameAux.GeyserType);
        gameAux.GeyserType = 131u;
        Assert.Equal(131u, storage.GeyserType);
    }

    /// <summary>준비 대여는 중복을 거부하고 반납 뒤 다시 빌리며 성공 횟수를 센다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void PreparedLeaseGuard()
    {
        var storage = new RawSurfaceStorage();
        using (RawSurfaceStorage.Use first = storage.AcquirePrepared())
        {
            Assert.True(storage.Borrowed());
            Assert.Equal(1ul, storage.Generation());
            Assert.Throws<InvalidOperationException>(() => storage.AcquirePrepared());
        }

        Assert.False(storage.Borrowed());
        using (RawSurfaceStorage.Use second = storage.AcquirePrepared())
        {
            Assert.True(storage.Borrowed());
            Assert.Equal(2ul, storage.Generation());
        }

        Assert.False(storage.Borrowed());
    }

    /// <summary>할당 진단은 World보다 긴 Client 수명으로 연결한다. 2026-11-07 F2d 서피스 저장소 껍데기 이식에서 추가됐다.</summary>
    [Fact]
    public void AllocationDiagnosticsBind()
    {
        var storage = new RawSurfaceStorage();
        storage.BindAllocationDiagnostics(() => { }, _ => { });
        Assert.Throws<ArgumentNullException>(() => storage.BindAllocationDiagnostics(null!, _ => { }));
    }

    /// <summary>세 자산 목록으로 실제 저장소를 만든다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.</summary>
    private static RawSurfaceStorage FullStorage(int types = 3)
    {
        var inputTypes = new List<OriginalTypeRecord>();
        var inputFrames = new List<RiftTypeFrames>();
        var inputShapes = new List<SquidDisplayShape>();
        // 세 자산 목록을 같은 개수로 채우는 반복문이다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.
        for (int index = 0; index < types; ++index)
        {
            inputTypes.Add(new OriginalTypeRecord());
            inputFrames.Add(new RiftTypeFrames([new RiftFrameCode((byte)'A', (byte)'P', (byte)index, 0)]));
            inputShapes.Add(new SquidDisplayShape());
        }

        return new RawSurfaceStorage(OriginalEdition.Patch1078, inputTypes, inputFrames, inputShapes);
    }

    /// <summary>전체 생성자는 자산 벡터·풀·spot을 최초 한 번만 확보한다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.</summary>
    [Fact]
    public void FullCtorBuildsVectorsPoolAndSpots()
    {
        RawSurfaceStorage storage = FullStorage();
        Assert.Equal(3, storage.Types.Count);
        Assert.Equal(3, storage.Frames.Count);
        Assert.Equal(3, storage.Shapes.Count);
        Assert.Equal(3, storage.Codes.Count);
        Assert.Equal([new RiftFrameCode((byte)'A', (byte)'P', 1, 0)], storage.Codes[1]);
        Assert.Equal(RawSurfaceStorage.PoolCapacity, storage.Pool.Capacity);
        Assert.True(storage.Pool.IsServer);
        Assert.Equal(RawSurfaceStorage.SpotCells, storage.Spots.Length);
        Assert.True(storage.Spots.All(value => value == 0));
        Assert.Equal(-10000.0f, storage.BoardTargetState.Invalid.X);
        Assert.Equal(-10000.0f, storage.BoardTargetState.Invalid.Y);
        Assert.False(storage.Borrowed());
        Assert.Equal(0ul, storage.Generation());
    }

    /// <summary>자산 배열 크기가 어긋나면 실제 저장소를 만들지 않는다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.</summary>
    [Fact]
    public void FullCtorRejectsMismatchedAssets()
    {
        var types = new List<OriginalTypeRecord> { new(), new() };
        var frames = new List<RiftTypeFrames> { new([]) };
        var shapes = new List<SquidDisplayShape> { new(), new() };
        Assert.Throws<ArgumentException>(() => new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes));
        var shapesShort = new List<SquidDisplayShape> { new() };
        var framesFull = new List<RiftTypeFrames> { new([]), new([]) };
        Assert.Throws<ArgumentException>(() => new RawSurfaceStorage(OriginalEdition.Patch1078, types, framesFull, shapesShort));
    }

    /// <summary>풀은 같은 권한 owner의 현재 서버 값을 빌린다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.</summary>
    [Fact]
    public void FullCtorSharesAuthorityServer()
    {
        var authority = new RawAuthorityState { Server = false };
        var types = new List<OriginalTypeRecord> { new() };
        var frames = new List<RiftTypeFrames> { new([]) };
        var shapes = new List<SquidDisplayShape> { new() };
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes, authority);
        Assert.False(storage.Pool.IsServer);
        authority.Server = true;
        Assert.True(storage.Pool.IsServer);
    }

    /// <summary>일반 설치는 이전 물리를 초기화하되 TYPE·배열 주소는 유지한다. 첫 대여는 풀·해시·spot을 건너뛴다. 2026-11-07 F2d 저장소 풀 생성에서 추가됐다.</summary>
    [Fact]
    public void AcquireResetsWorldButKeepsTypes()
    {
        RawSurfaceStorage storage = FullStorage();
        uint free = storage.Pool.FreeCount;
        storage.Spots[100] = 7;
        storage.PriestOwnerState[2] = 5u;
        storage.TotalMade.LocalCounts[7] = 9u;
        storage.Graph.Records[5].InUse = 9;
        storage.Islands.Records[10] = 9;
        storage.SidLists.Factories.Count = 5;
        storage.SidLists.Factories.Entries[0] = 9u;
        storage.BoardTargetState.Invalid = new RawPathPoint { X = 1.0f, Y = 2.0f };
        storage.Authority().Server = false;
        storage.Authority().Boss = false;
        storage.GameAux().Archipelago = 3u;
        var freshIslands = new RawIslandStorage();
        RawIslandRuntime.ResetRawIslandRecords(freshIslands.Records);
        byte freshChunkIsland = new ChunkMap().At(0, 0).Island;
        storage.Chunks.At(0, 0).Island = 3;
        using (RawSurfaceStorage.Use first = storage.Acquire())
        {
            Assert.True(storage.Borrowed());
            Assert.Equal(1ul, storage.Generation());
            Assert.Equal(7, storage.Spots[100]);
            Assert.Equal(0u, storage.PriestOwnerState[2]);
            Assert.True(storage.Authority().Server);
            Assert.True(storage.Authority().Boss);
            Assert.Equal(0u, storage.GameAux().Archipelago);
            Assert.Equal(-10000.0f, storage.BoardTargetState.Invalid.X);
            Assert.Equal(0, storage.Graph.Records[5].InUse);
            Assert.Equal(0u, storage.TotalMade.LocalCounts[7]);
            Assert.Equal(freshIslands.Records, storage.Islands.Records);
            Assert.Equal(0u, storage.SidLists.Factories.Count);
            Assert.Equal(0u, storage.SidLists.Factories.Entries[0]);
            Assert.Equal(freshChunkIsland, storage.Chunks.At(0, 0).Island);
            Assert.Equal(3, storage.Types.Count);
            Assert.Equal(3, storage.Codes.Count);
            Assert.Equal(free, storage.Pool.FreeCount);
            storage.Pool.Allocate();
            Assert.Equal(free - 1u, storage.Pool.FreeCount);
            Assert.Throws<InvalidOperationException>(() => storage.Acquire());
        }

        Assert.False(storage.Borrowed());
        storage.Spots[200] = 11;
        using (RawSurfaceStorage.Use second = storage.Acquire())
        {
            Assert.Equal(2ul, storage.Generation());
            Assert.Equal(0, storage.Spots[100]);
            Assert.Equal(0, storage.Spots[200]);
            Assert.Equal(free, storage.Pool.FreeCount);
            Assert.Equal(3, storage.Types.Count);
        }

        Assert.False(storage.Borrowed());
    }
}
