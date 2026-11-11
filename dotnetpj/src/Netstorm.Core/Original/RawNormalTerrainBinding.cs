// 일반 지형 소비자의 분기·범위·타입 연결 계약을 공유한다.

namespace Netstorm.Core.Original;

// 일반지형 바인딩 계약 모듈을 담는 네임스페이스다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.

// 군도와 미복원 부모를 정상 지형으로 대체하지 않는 검사다. 패턴 개수·첫 기록·군집 후처리 잎까지 포함하며 생성·도장 본체는 후속 단위에서 잇는다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐고 0.202에서 패턴 개수를, 0.203에서 첫 기록을 이었다.
public static class RawNormalTerrainChecks
{
    /// <summary>일반 분기가 아니면 거부한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="request">확인한 분기 요청이다.</param>
    public static void RequireBranch(NormalTerrainRequest request)
    {
        if (request.Branch != NormalTerrainBranch.Normal)
        {
            throw new InvalidOperationException("군도·전체 부모를 일반 지형으로 실행할 수 없습니다.");
        }
    }

    /// <summary>유효 내부 청크 범위를 요구한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="request">검사할 요청이다.</param>
    public static void RequireBounds(NormalTerrainRequest request)
    {
        ChunkRect bounds = request.Bounds;
        if (bounds.Left < 1 || bounds.Top < 1 || bounds.Right > 15 || bounds.Bottom > 15 ||
            bounds.Left >= bounds.Right || bounds.Top >= bounds.Bottom)
        {
            throw new ArgumentOutOfRangeException(nameof(request), "일반 지형 소비자의 유효 내부 청크 범위가 필요합니다.");
        }
    }

    /// <summary>군도와 서로 다른 원본 전역을 묵시적인 정상 지형·지역 기본값으로 바꾸지 않는다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="farm">군집 상태다.</param>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    public static void RequireNormal(FarmClusterState farm, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool)
    {
        ArgumentNullException.ThrowIfNull(farm);
        if (farm.CurrentArchipelago())
        {
            throw new InvalidOperationException("일반 전용 지형 요청에 군도 상태를 사용할 수 없습니다.");
        }

        RequireStorage(globals, placement, stamp, types, pool);
    }

    /// <summary>현재 원본 모드의 선택과 분리하여 같은 실제 TYPE·지역·생성자 연결을 확인한다. 2026-11-07 F2d 지형 바인딩 계약 이식에서 추가됐다.</summary>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    public static void RequireStorage(TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(stamp);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(pool);
        if (globals.IslandType != placement.IslandType || globals.IslandType != stamp.IslandType ||
            globals.EdgeType != stamp.EdgeType || globals.NoRegion != stamp.NoRegion)
        {
            throw new ArgumentException("일반 지형 같은 타입·지역 전역 연결 오류");
        }

        // 지면 97의 실제 0 슬롯·base 계약은 보존하며 121의 실제 생성자를 임의 0 슬롯으로 대신하지 않는다
        foreach (uint type in new[] { globals.IslandType, placement.EdgeFarmType })
        {
            if (type >= (uint)types.Count ||
                types[(int)type].ConstructorAddress != OriginalConstructors.Address(pool.Edition, (int)type))
            {
                throw new ArgumentException("일반 지형 실제 타입 생성자 연결 누락");
            }
        }
    }

    /// <summary>6바이트 청크 기록의 패턴 번호·회전으로 영역 디코더를 돌려 유효 칸 수를 센다. 2026-11-07 F2d 지형 패턴 개수 이식에서 추가됐다.</summary>
    /// <param name="frames">같은 실제 월드의 타입별 프레임 표다.</param>
    /// <param name="puzzlePieceType">패턴을 펼칠 때 쓰는 현재 조각 타입 번호다.</param>
    /// <param name="record">셀·회전을 담은 6바이트 기록이다.</param>
    /// <param name="edition">디코더 판본이다.</param>
    /// <param name="decoderDiagnostics">디코더 진단 묶음이다. null이면 금 간 검사를 건너뛴다.</param>
    /// <returns>빈·누락 프레임을 건너뛰고 센 유효 칸 수다.</returns>
    public static uint PatternCount(IReadOnlyList<RiftTypeFrames> frames, uint puzzlePieceType, ReadOnlySpan<byte> record, OriginalEdition edition, CanonDecoderDiagnostics? decoderDiagnostics)
    {
        ArgumentNullException.ThrowIfNull(frames);
        if (puzzlePieceType >= (uint)frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(puzzlePieceType), "일반 지형 현재 puzzlePiece 타입 범위 오류");
        }

        if (record.Length != 6)
        {
            throw new ArgumentException("일반 지형 패턴 기록은 6바이트여야 합니다.", nameof(record));
        }

        ReadOnlySpan<CanonPattern> patterns = CanonDecoder.TerritoryPatterns();
        int pattern = record[0] & 0x3f;
        if (pattern >= patterns.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "일반 지형 패턴 번호 범위 오류");
        }

        var decoder = new CanonDecoder(frames[(int)puzzlePieceType], patterns[pattern], (record[0] >> 6) * 2, 0f, 0f, null, edition, decoderDiagnostics);
        uint count = 0u;
        // 원본 생성자의 최초 유효 칸을 포함하고 끝날 때까지 센다
        for (; decoder.Valid(); decoder.Advance())
        {
            ++count;
        }

        return count;
    }

    /// <summary>해당 지역의 처음 일치한 청크 6바이트 기록을 Y 바깥·X 안쪽 순서로 찾는다. 2026-11-07 F2d 첫 기록 이식에서 추가됐다.</summary>
    /// <param name="chunks">같은 실제 월드의 청크 지도다.</param>
    /// <param name="islands">같은 실제 월드의 섬 목록이다.</param>
    /// <param name="region">찾을 지역 번호다.</param>
    /// <param name="edition">CD 판본에서만 후처리 진단을 보고하는 판본이다.</param>
    /// <param name="postpass">CD 전용 첫 기록·누락 진단이다. null이면 보고를 건너뛴다.</param>
    /// <returns>처음 일치한 6바이트 복사본이며 없으면 6바이트 영 기록이다.</returns>
    public static byte[] FirstRecord(ChunkMap chunks, RawIslandRuntime islands, int region, OriginalEdition edition, Action<TerrainFarmPostpassAssertion>? postpass)
    {
        ArgumentNullException.ThrowIfNull(chunks);
        ArgumentNullException.ThrowIfNull(islands);
        bool cd = edition == OriginalEdition.Cd1072;
        if (cd && islands.Exists(region) == 0)
        {
            postpass?.Invoke(TerrainFarmPostpassAssertion.RecordMissingIsland);
        }

        // Y청크가 바깥이며 각 행의 X청크를 처음부터 찾는다
        for (int y = 0; y < chunks.YLength(); y++)
        {
            // 원본은 처음 일치한 기록 6바이트만 반환한다
            for (int x = 0; x < chunks.XLength(); x++)
            {
                ChunkEntry entry = chunks.At(x, y);
                if (unchecked((int)unchecked((sbyte)entry.Island)) == region)
                {
                    byte[] found = new byte[6];
                    if (entry.Record is not null)
                    {
                        Array.Copy(entry.Record, found, Math.Min(entry.Record.Length, found.Length));
                    }

                    return found;
                }
            }
        }

        if (cd)
        {
            postpass?.Invoke(TerrainFarmPostpassAssertion.RecordNotFound);
        }

        return new byte[6];
    }

    /// <summary>도장 뒤 현재 섬별로 첫 기록·범위를 읽어 같은 군집 몸체에 순서대로 넣는다. 2026-11-07 F2d 군집 후처리 이식에서 추가됐다.</summary>
    /// <param name="chunks">같은 실제 월드의 청크 지도다.</param>
    /// <param name="islands">같은 실제 월드의 섬 목록이다.</param>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    /// <param name="puzzlePieceType">패턴을 펼칠 때 쓰는 현재 조각 타입 번호다.</param>
    /// <param name="frames">같은 실제 월드의 타입별 프레임 표다.</param>
    /// <param name="edition">디코더·후처리 판본이다.</param>
    /// <param name="decoderDiagnostics">디코더 진단 묶음이다. null이면 금 간 검사를 건너뛴다.</param>
    /// <param name="postpass">상위의 첫 record/Bounds 조회 진단이다. null이면 보고를 건너뛴다.</param>
    /// <param name="selectedRegion">현재 noRegion이면 모든 섬을 실행한다.</param>
    /// <param name="place">같은 군집 몸체의 배치 효과다.</param>
    public static void PostprocessCurrentFarms(ChunkMap chunks, RawIslandRuntime islands, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool, uint puzzlePieceType, IReadOnlyList<RiftTypeFrames> frames, OriginalEdition edition, CanonDecoderDiagnostics? decoderDiagnostics, Action<TerrainFarmPostpassAssertion>? postpass, int selectedRegion, Action<FortRandomRectangle, int, uint> place)
    {
        ArgumentNullException.ThrowIfNull(chunks);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(place);
        RequireStorage(globals, placement, stamp, types, pool);
        bool cd = edition == OriginalEdition.Cd1072;
        // 원본 상위처럼 count는 unsigned 저장값을 signed DWORD로 해석하며 회마다 다시 읽는다
        for (int region = 0; region < unchecked((int)islands.Count()); region++)
        {
            if (selectedRegion != globals.NoRegion && selectedRegion != region)
            {
                continue;
            }

            byte[] record = FirstRecord(chunks, islands, region, edition, postpass);
            if (islands.Exists(region) == 0)
            {
                postpass?.Invoke(TerrainFarmPostpassAssertion.BoundsMissingIsland);
            }

            ChunkRect bounds = chunks.Bounds(region);
            if (cd && bounds.Left == 16 && bounds.Top == 16)
            {
                postpass?.Invoke(TerrainFarmPostpassAssertion.BoundsNotFound);
            }

            var cells = new FortRandomRectangle(bounds.Left * 16f, bounds.Top * 16f, bounds.Right * 16f, bounds.Bottom * 16f);
            place(cells, region, PatternCount(frames, puzzlePieceType, record, edition, decoderDiagnostics));
        }
    }
}

// 실제 004c1510/CD0049e620이 읽는 현재 전역과 선택된 군도 하위의 같은 장치를 빌린다. 생성·도장 본체는 후속 단위에서 잇는다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.
public sealed class TerrainGenerationInputs
{
    /// <summary>후보 조건의 현재 00540a2c를 읽는 플래그다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public RawCheckingFlag Enabled { get; set; } = new();

    /// <summary>후보 조건의 현재 00594fbc를 읽는 플래그다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public RawCheckingFlag Battle { get; set; } = new();

    /// <summary>후보 조건의 현재 00594fa0을 읽는 플래그다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public RawCheckingFlag Mission { get; set; } = new();

    /// <summary>PATCH는 constructor 뒤 현재 0052f7e5를 읽는다. CD root에는 이 관문이 없다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public Func<byte>? Layout { get; set; }

    /// <summary>PATCH0041ca10을 두 번 따로 호출한다. 첫 결과를 두 번째에 재사용하지 않는다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public Func<int>? Spacing { get; set; }

    /// <summary>실제 00542654/CD00537188이며 간헐천·군집과 같은 장기 owner다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public IntBox Span { get; set; } = new();

    /// <summary>밀도 난수의 하한을 만드는 현재 15 DWORD다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public IntBox Minimum { get; set; } = new();

    /// <summary>밀도 난수의 상한을 만드는 현재 35 DWORD다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public IntBox Maximum { get; set; } = new();

    /// <summary>bb4 강제 밀도이며 0이면 같은 로컬 난수로 두 번 추출한다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public UIntBox Percent { get; set; } = new();

    /// <summary>도장·정규화 뒤 1을 쓰는 실제 bb0이며 일반 분기에서 지우지 않는다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public UIntBox Archipelago { get; set; } = new();

    /// <summary>각 플레이어의 현재 base+owner*stride+2c DWORD를 읽는다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public Func<int, uint>? Subscriber { get; set; }

    /// <summary>같은 실제 PlayerBounds의 00432050이다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public Func<int, byte[], ChunkRect>? Placement { get; set; }

    /// <summary>성장 전에 원문 1d 채널로 signed 백분율을 출력한다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public Action<int>? LogPercent { get; set; }

    /// <summary>첫 시드·청크 getter가 읽는 현재 checking과 진단 복귀 장치다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    public TerrainGrowthDiagnostics Growth { get; set; } = new();

    /// <summary>플래그는 복사하고 상자·대리자·성장은 같은 owner를 공유하는 사본을 만든다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    /// <returns>같은 상자를 공유하는 입력 묶음이다.</returns>
    public TerrainGenerationInputs Clone()
    {
        return new TerrainGenerationInputs
        {
            Enabled = new RawCheckingFlag(Enabled),
            Battle = new RawCheckingFlag(Battle),
            Mission = new RawCheckingFlag(Mission),
            Layout = Layout,
            Spacing = Spacing,
            Span = Span,
            Minimum = Minimum,
            Maximum = Maximum,
            Percent = Percent,
            Archipelago = Archipelago,
            Subscriber = Subscriber,
            Placement = Placement,
            LogPercent = LogPercent,
            Growth = Growth,
        };
    }
}

// 같은 실제 월드 소유에 묶어 분기·개수·기록·후처리를 위임하는 바인딩 껍데기다. 생성·도장·군집 조립은 후속 단위에서 잇는다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.
public sealed class RawNormalTerrainWorldBinding
{
    /// <summary>같은 실제 월드의 청크 지도다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly ChunkMap _chunks;

    /// <summary>같은 실제 월드의 섬 목록이다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly RawIslandRuntime _islands;

    /// <summary>문자 mapping 전역이다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly TerrainContextGlobals _globals;

    /// <summary>매 실제 배치가 읽는 타입 전역이다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly FortRandomPlacementState _placement;

    /// <summary>수명이 긴 초기 캐시 상태다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly TerrainStampState _stamp;

    /// <summary>같은 실제 TYPE 표다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>같은 실제 풀이다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>매 요청 읽는 현재 조각 타입의 공유 상자다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly UIntBox _puzzlePieceType;

    /// <summary>같은 실제 월드의 타입별 프레임 표다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>호출자가 준 진단 묶음의 복사본이다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    private readonly NormalTerrainDiagnostics _diagnostics;

    /// <summary>원문 상위의 비소유 전역 참조·선택 장치 묶음이며 모드나 Player 사본이 아니다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    private TerrainGenerationInputs? _generation;

    /// <summary>같은 군도·확장 owner를 공유하는 군집 상태다. 생성 입력 연결 때 함께 보관하며 일반 생성의 군도 검사가 읽는다. 2026-11-10 R4 F2c 정상 생성 본체에서 추가됐다.</summary>
    private FarmClusterState? _farm;

    /// <summary>같은 실제 저장소를 묶고 타입·지역 연결을 확인한다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    /// <param name="chunks">같은 실제 월드의 청크 지도다.</param>
    /// <param name="islands">같은 실제 월드의 섬 목록이다.</param>
    /// <param name="globals">문자 mapping 전역이다.</param>
    /// <param name="placement">매 실제 배치가 읽는 타입 전역이다.</param>
    /// <param name="stamp">수명이 긴 초기 캐시 상태다.</param>
    /// <param name="types">같은 실제 TYPE 표다.</param>
    /// <param name="pool">같은 실제 풀이다.</param>
    /// <param name="puzzlePieceType">매 요청 읽는 현재 조각 타입의 공유 상자다.</param>
    /// <param name="frames">같은 실제 월드의 타입별 프레임 표다.</param>
    /// <param name="diagnostics">원본 진단 묶음이다. 복사해서 가진다.</param>
    public RawNormalTerrainWorldBinding(ChunkMap chunks, RawIslandRuntime islands, TerrainContextGlobals globals, FortRandomPlacementState placement, TerrainStampState stamp, IReadOnlyList<OriginalTypeRecord> types, RawSidPool pool, UIntBox puzzlePieceType, IReadOnlyList<RiftTypeFrames> frames, NormalTerrainDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(chunks);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(stamp);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(puzzlePieceType);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(diagnostics);
        _chunks = chunks;
        _islands = islands;
        _globals = globals;
        _placement = placement;
        _stamp = stamp;
        _types = types;
        _pool = pool;
        _puzzlePieceType = puzzlePieceType;
        _frames = frames;
        _diagnostics = diagnostics.Clone();
        RequireStorage();
    }

    /// <summary>군도와 서로 다른 원본 전역을 묵시적인 정상 지형·지역 기본값으로 바꾸지 않는다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    /// <param name="farm">군집 상태다.</param>
    public void RequireNormal(FarmClusterState farm) => RawNormalTerrainChecks.RequireNormal(farm, _globals, _placement, _stamp, _types, _pool);

    /// <summary>현재 원본 모드의 선택과 분리하여 같은 실제 TYPE·지역·생성자 연결을 확인한다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    public void RequireStorage() => RawNormalTerrainChecks.RequireStorage(_globals, _placement, _stamp, _types, _pool);

    /// <summary>묶은 조각 타입·프레임·진단으로 영역 디코더를 돌려 유효 칸 수를 센다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    /// <param name="record">셀·회전을 담은 6바이트 기록이다.</param>
    /// <returns>빈·누락 프레임을 건너뛰고 센 유효 칸 수다.</returns>
    public uint PatternCount(ReadOnlySpan<byte> record) =>
        RawNormalTerrainChecks.PatternCount(_frames, _puzzlePieceType.Value, record, _pool.Edition, _diagnostics.DecoderDiagnostics);

    /// <summary>묶은 지도·목록·진단으로 해당 지역의 처음 일치한 청크 기록을 찾는다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    /// <param name="region">찾을 지역 번호다.</param>
    /// <returns>처음 일치한 6바이트 복사본이며 없으면 6바이트 영 기록이다.</returns>
    public byte[] FirstRecord(int region) =>
        RawNormalTerrainChecks.FirstRecord(_chunks, _islands, region, _pool.Edition, _diagnostics.Postpass);

    /// <summary>묶은 저장소로 도장 뒤 현재 섬별 후처리를 같은 군집 몸체에 순서대로 넣는다. 2026-11-07 F2d 정상 바인딩 껍데기 이식에서 추가됐다.</summary>
    /// <param name="selectedRegion">현재 noRegion이면 모든 섬을 실행한다.</param>
    /// <param name="place">같은 군집 몸체의 배치 효과다.</param>
    public void PostprocessCurrentFarms(int selectedRegion, Action<FortRandomRectangle, int, uint> place) =>
        RawNormalTerrainChecks.PostprocessCurrentFarms(_chunks, _islands, _globals, _placement, _stamp, _types, _pool,
            _puzzlePieceType.Value, _frames, _pool.Edition, _diagnostics.DecoderDiagnostics, _diagnostics.Postpass, selectedRegion, place);

    /// <summary>
    /// 이미 초기 지면을 등록한 실제 월드에서 상위의 섬별 후처리만 실행한다. 군도 상태는 거부한다.
    /// 원문 0.211 껍데기의 위임 조각에 공개 관문을 둔다(생성·도장 본체는 월드 본체 조립 때 잇는다).
    /// 근거: cpppj/src/client/RawNormalTerrainWorldBinding.cpp PostprocessFarms (F2d 정상 지형 후처리 관문, 2026-11-07).
    /// </summary>
    /// <param name="farm">군집 상태다. 군도이면 거부한다.</param>
    /// <param name="selectedRegion">현재 noRegion이면 모든 섬을 실행한다.</param>
    /// <param name="place">같은 군집 몸체의 배치 효과다.</param>
    public void PostprocessFarms(FarmClusterState farm, int selectedRegion, Action<FortRandomRectangle, int, uint> place)
    {
        RequireNormal(farm);
        PostprocessCurrentFarms(selectedRegion, place);
    }

    /// <summary>
    /// 원본 일반 context의 성장·초기 도장까지이며 군집 postpass 전에 끝난다.
    /// 근거: cpppj/src/client/RawNormalTerrainWorldBinding.cpp Generate. (2026-11-10 R4 F2c 정상 생성 본체에서 추가했다)
    /// </summary>
    /// <param name="request">소유자·청크 범위·선택 지역과 확인한 정상 분기다.</param>
    /// <param name="random">도장이 소비하는 게임 난수다. 월드 조립 때 실제 난수를 넘긴다.</param>
    /// <param name="crt">도장이 소비하는 공유 CRT holdrand 상자다. 월드 조립 때 실제 상자를 넘긴다.</param>
    /// <param name="world">도장의 생성·owner·Pop·지역 효과다. ChunkRegion·Assertion은 현재 지도·진단으로 덮는다.</param>
    /// <param name="place">같은 군집 몸체의 배치 효과다.</param>
    /// <returns>도장이 만든 마지막 SID다.</returns>
    public int Generate(NormalTerrainRequest request, GameRandom random, UIntBox crt, TerrainStampHooks world, Action<FortRandomRectangle, int, uint> place)
    {
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(crt);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(place);
        RawNormalTerrainChecks.RequireBranch(request);
        if (_farm is null)
            throw new InvalidOperationException("실제 지형 생성 군집 미연결");
        RequireNormal(_farm);
        RawNormalTerrainChecks.RequireBounds(request);
        if (_diagnostics.Context is null)
            throw new InvalidOperationException("실제 지형 context 진단 미연결");
        var context = new RawTerrainContext(_pool.Edition, request.Owner, request.SelectedRegion, request.Bounds, _globals, _diagnostics.Context);
        var growth = new RawTerrainGrowth(context, _chunks, _globals);
        growth.RunNormal();
        int last = StampContext(context, random, crt, world);
        PostprocessCurrentFarms(request.SelectedRegion, place);
        return last;
    }

    /// <summary>
    /// context를 도장·보정하고 같은 실제 월드에 실행한다.
    /// 근거: cpppj/src/client/RawNormalTerrainWorldBinding.cpp StampContext. (2026-11-10 R4 F2c 정상 생성 본체에서 추가했다)
    /// C# 도장은 공유 GameRandom과 CRT 읽기·쓰기 대리자를 받으므로 호출자가 넘긴다.
    /// </summary>
    /// <param name="context">도장할 성장 context다.</param>
    /// <param name="random">도장이 소비하는 게임 난수다.</param>
    /// <param name="crt">도장이 소비하는 공유 CRT holdrand 상자다.</param>
    /// <param name="world">도장의 생성·owner·Pop·지역 효과다. ChunkRegion·Assertion은 현재 지도·진단으로 덮는다.</param>
    /// <returns>도장이 만든 마지막 SID다.</returns>
    public int StampContext(RawTerrainContext context, GameRandom random, UIntBox crt, TerrainStampHooks world)
    {
        ArgumentNullException.ThrowIfNull(context);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(crt);
        ArgumentNullException.ThrowIfNull(world);
        context.MarkEdges('x', (byte)'=');
        var effects = new TerrainStampHooks
        {
            Create = world.Create,
            SetOwner = world.SetOwner,
            Pop = world.Pop,
            SetRegion = world.SetRegion,
            ChunkRegion = (x, y) => unchecked((sbyte)_chunks.At(x, y).Island),
            Assertion = _diagnostics.Stamp,
        };
        var stamp = new RawTerrainStamp(_pool, _types, _frames, context.Map, random,
            () => crt.Value, value => crt.Value = value, _stamp, effects);
        stamp.Stamp((sbyte)'x');
        context.NormalizeAfterStamp();
        return context.Map.LastSid;
    }

    /// <summary>현재 장기 전역·Player 장치의 수명은 이 소비자보다 길어야 하며 누락 분기를 일반으로 대신하지 않는다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    /// <param name="farm">같은 군도·확장 owner를 공유할 군집 상태다. 기존 빌림은 첫 연결 때 덮어쓴다(단일 owner 한계).</param>
    /// <param name="inputs">보관할 상위 입력 묶음이다. 상자·대리자·성장은 공유하고 플래그는 복사한다.</param>
    public void BindGenerationInputs(FarmClusterState farm, TerrainGenerationInputs inputs)
    {
        ArgumentNullException.ThrowIfNull(farm);
        ArgumentNullException.ThrowIfNull(inputs);
        ArgumentNullException.ThrowIfNull(inputs.Span);
        ArgumentNullException.ThrowIfNull(inputs.Minimum);
        ArgumentNullException.ThrowIfNull(inputs.Maximum);
        ArgumentNullException.ThrowIfNull(inputs.Percent);
        ArgumentNullException.ThrowIfNull(inputs.Archipelago);
        if (_generation is not null &&
            (!ReferenceEquals(_generation.Span, inputs.Span) || !ReferenceEquals(_generation.Archipelago, inputs.Archipelago)))
        {
            throw new ArgumentException("지형 root와 군집의 같은 군도/확장 owner 불일치");
        }

        // 원문처럼 군집의 두 포인터를 입력의 같은 주소로 교체한다. 기존 Func 빌림은 첫 연결 때 덮어쓴다.
        farm.SharedArchipelago = () => inputs.Archipelago.Value;
        farm.SharedArchipelagoSpan = () => inputs.Span.Value;
        _farm = farm;
        _generation = inputs.Clone();
    }

    /// <summary>상위 입력 묶음을 보관했는지 확인한다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    /// <returns>보관했으면 참이다.</returns>
    public bool HasGenerationInputs() => _generation is not null;

    /// <summary>보관한 상위 입력 묶음을 돌려준다. 2026-11-07 F2d 지형 생성 입력 이식에서 추가됐다.</summary>
    /// <returns>보관한 입력 묶음이다.</returns>
    public TerrainGenerationInputs GenerationInputs() =>
        _generation ?? throw new InvalidOperationException("실제 지형 생성 상위의 현재 전역/장치 미연결");

    /// <summary>
    /// 후보 조건보다 먼저 캐시하고 PATCH layout·두 spacing과 constructor 뒤 현재값을 쓴다.
    /// 근거: cpppj/src/client/RawNormalTerrainWorldBinding.cpp GenerateOriginal. (2026-11-10 R4 F2c 정상 생성 본체에서 추가했다)
    /// 32비트 감김 곱셈·뺄셈과 signed 나눗셈은 원본 비트 해석 그대로 unchecked에서 계산한다.
    /// </summary>
    /// <param name="owner">생성 요청 소유자다.</param>
    /// <param name="bounds">청크 범위다.</param>
    /// <param name="selectedRegion">현재 noRegion이면 모든 섬의 후처리를 실행한다.</param>
    /// <param name="random">도장이 소비하는 게임 난수다.</param>
    /// <param name="crt">도장이 소비하는 공유 CRT holdrand 상자다.</param>
    /// <param name="world">도장의 생성·owner·Pop·지역 효과다. ChunkRegion·Assertion은 현재 지도·진단으로 덮는다.</param>
    /// <param name="place">같은 군집 몸체의 배치 효과다.</param>
    /// <returns>도장이 만든 마지막 SID다.</returns>
    public int GenerateOriginal(uint owner, ChunkRect bounds, int selectedRegion, GameRandom random, UIntBox crt, TerrainStampHooks world, Action<FortRandomRectangle, int, uint> place)
    {
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(crt);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(place);
        TerrainGenerationInputs input = GenerationInputs();
        bool candidate = input.Enabled.IsEnabled && input.Battle.IsEnabled && !input.Mission.IsEnabled && _islands.Count() == 1u;
        RequireStorage();
        if (_diagnostics.Context is null)
            throw new InvalidOperationException("실제 지형 context 진단 미연결");
        var context = new RawTerrainContext(_pool.Edition, owner, selectedRegion, bounds, _globals, _diagnostics.Context);
        var growth = new RawTerrainGrowth(context, _chunks, _globals, input.Growth);
        bool cd = _pool.Edition == OriginalEdition.Cd1072;
        if (!cd && input.Layout is null)
            throw new InvalidOperationException("지형 root의 현재 layout 미연결");
        bool archipelago = cd ? candidate : input.Layout!() == 0 && candidate;
        if (archipelago)
        {
            int extension;
            if (cd)
            {
                // CD는 기존 span을 쓰고 mapSize를 따로 조회하지 않는다.
                extension = input.Span.Value;
            }
            else
            {
                if (input.Spacing is null)
                    throw new InvalidOperationException("지형 root의 현재 spacing 조회 미연결");
                extension = unchecked((int)unchecked(unchecked((uint)input.Spacing()) - 2u));
                int spacing = input.Spacing();
                input.Span.Value = spacing == 3 ? 1 : spacing == 4 ? 2 : 3;
            }

            context.ArchipelagoMode = 1u;
            ChunkRect before = context.Chunks;
            context.SetBounds(new ChunkRect(
                unchecked((int)unchecked(unchecked((uint)before.Left) - unchecked((uint)extension) + 1u)),
                unchecked((int)unchecked(unchecked((uint)before.Top) - unchecked((uint)extension) + 1u)),
                unchecked((int)unchecked(unchecked((uint)before.Right) + unchecked((uint)extension))),
                unchecked((int)unchecked(unchecked((uint)before.Bottom) + unchecked((uint)extension)))));
            uint scatterWidth = unchecked(unchecked((uint)input.Span.Value) * 2u + 2u);
            growth.Scatter(unchecked(unchecked((int)unchecked(scatterWidth * scatterWidth * 0x55u)) / 0x19));
            uint seed = unchecked(growth.FirstChunkSeed() * 0x1bbu);
            ChunkRect grown = context.Chunks;
            uint area = unchecked(unchecked(unchecked((uint)grown.Bottom) - unchecked((uint)grown.Top)) * unchecked(unchecked((uint)grown.Right) - unchecked((uint)grown.Left)) * 0x100u);
            uint target;
            if (input.Percent.Value == 0u)
            {
                int high = unchecked((int)unchecked(unchecked((uint)input.Maximum.Value) * area)) / 200;
                int low = unchecked((int)unchecked(unchecked((uint)input.Minimum.Value) * area)) / 200;
                int first = growth.NextRange(ref seed, low, high);
                int second = growth.NextRange(ref seed, low, high);
                target = unchecked(unchecked((uint)first) + unchecked((uint)second));
            }
            else
            {
                target = unchecked((uint)unchecked(unchecked((int)unchecked(input.Percent.Value * area)) / 100));
            }

            if (input.LogPercent is null)
                throw new InvalidOperationException("지형 root의 현재 백분율 로그 미연결");
            if (area == 0u)
                throw new DivideByZeroException("지형 root의 원문 백분율 0 나눗셈");
            input.LogPercent(unchecked(unchecked((int)unchecked(target * 100u)) / unchecked((int)area)));
            growth.GrowRectangle(0, context.Chunks, unchecked((int)target), ref seed);
            if (input.Subscriber is null)
                throw new InvalidOperationException("지형 root의 현재 Player 미연결");
            // 매 반복에서 현재 Player base의 subscriber를 다시 읽는다.
            for (int player = 1; player <= 8; player++)
            {
                if (input.Subscriber(player) == 0u)
                    continue;
                if (input.Placement is null)
                    throw new InvalidOperationException("지형 root의 선택 칸 실제 배치 미연결");
                // 원문 0x107/WORD0의 정확한 BYTE이며 추가 DWORD 상위16을 포함하지 않는다.
                byte[] record = [7, 1, 0, 0, 0, 0];
                ChunkRect placement = input.Placement(player, record);
                growth.ClearPlacement(placement);
            }

            growth.FixGaps(1);
            growth.RemoveSparse();
        }
        else
        {
            growth.RunNormal();
        }

        int last = StampContext(context, random, crt, world);
        if (archipelago)
            input.Archipelago.Value = 1u;
        PostprocessCurrentFarms(selectedRegion, place);
        return last;
    }
}
