namespace Netstorm.Assets;

/// <summary>원본 지면 타일로 구성한 미리보기 셀. 테마와 변형 선택은 시각 검증 전의 근사다.</summary>
public sealed record FortTerrainTile(int X, int Y, int Region, string Theme, int Cluster)
{
    /// <summary>해당 지면 영역을 만든 신전 또는 받침의 소유자. 중립 지면은 0이다.</summary>
    public int Owner { get; init; }
}

/// <summary>타일 외관과 작은 받침을 붙이기 전의 본섬 지면 칸.</summary>
public sealed record FortTerrainCell(int X, int Y, int Region);

/// <summary>Terrainbuilder의 연결 통로·시드 성장·빈 틈 메우기를 옮긴 개발용 지면 미리보기.</summary>
public sealed class FortTerrainPreview
{
    /// <summary>월드의 한 변 칸 수.</summary>
    private const int WorldSize = FortFile.WorldChunksX * FortMap.CellsPerChunk;
    /// <summary>원본 방향 A~P에 대응하는 북·동·남·서 연결 비트 (VA 0x52f910).</summary>
    private static readonly int[] Connections = [15, 7, 14, 13, 11, 6, 12, 9, 3, 5, 10, 4, 8, 1, 2, 0];
    /// <summary>네 방향 이웃 비트에서 방향 문자로 변환하는 원본 표 (VA 0x52f8fc).</summary>
    private const string MaskCharacters = "PNOILJFBMHKEGDCA";
    /// <summary>원본 시드가 0일 때 사용하는 대체 값.</summary>
    private const uint DefaultSeed = 0x0BAD0BAD;
    /// <summary>원본 영역 안 청크별 성장 시드 증가량.</summary>
    private const uint ChunkSeedStep = 0x10E3;
    /// <summary>원본 성장에서 후보를 찾는 최대 실패 횟수.</summary>
    private const int GrowthRetryLimit = 1000;
    /// <summary>원본 AA 지면 그림의 한 변 칸 수.</summary>
    private const int CorePatternSize = 3;
    /// <summary>원본 AA 지면의 원소별 3×3 변형 개수.</summary>
    private const int CoreVariantCount = 4;
    /// <summary>원소 하나가 차지하는 AA 지면 클러스터 개수.</summary>
    private const int CoreThemeFrameCount = CorePatternSize * CorePatternSize * CoreVariantCount;
    /// <summary>원본 00455970의 예측 가능 모드에 쓰는 MSVC rand 시드.</summary>
    private const uint PreviewCoreSeed = 0x38D535u;
    /// <summary>원본 00455970이 시드 설정 뒤 표 생성 전에 소비하는 최소 rand 호출 수.</summary>
    private const int CoreSeedWarmupCount = 103;
    /// <summary>원본 MSVC rand 상태 전이의 곱셈 계수.</summary>
    private const uint CoreRandomMultiplier = 0x343FDu;
    /// <summary>원본 MSVC rand 상태 전이의 덧셈 계수.</summary>
    private const uint CoreRandomIncrement = 0x269EC3u;
    /// <summary>원본 MSVC rand가 반환하는 15비트 값의 마스크.</summary>
    private const uint CoreRandomResultMask = 0x7FFFu;
    /// <summary>원본 004c04b0이 한 번 생성하는 변형 난수표의 항목 수.</summary>
    private const int CoreVariantTableSize = 99;
    /// <summary>원본 표 생성식을 고정 시드로 실행한 개발용 3×3 변형 선택표.</summary>
    private static readonly int[] CoreVariantTable = CreateCoreVariantTable();
    /// <summary>북부터 시계 방향으로 나열한 여덟 이웃 좌표 (VA 0x52f83c/0x52f85c).</summary>
    private static readonly (int X, int Y)[] Neighbors = [(0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1)];

    private readonly int[] _land = Enumerable.Repeat(-1, WorldSize * WorldSize).ToArray();
    private readonly Dictionary<(int X, int Y), int> _chunks = [];
    private readonly Dictionary<int, string> _themes = [];
    private readonly Dictionary<int, int> _owners = [];
    private readonly Dictionary<int, int[]> _targets = [];

    /// <summary>생성된 타일 목록. 원본과 같은 전투 지형으로 확정한 데이터는 아니다.</summary>
    public IReadOnlyList<FortTerrainTile> Tiles { get; }

    /// <summary>원본 생성 규칙 및 캡처와 대조할 본섬 마스크 (y·x 순서).</summary>
    public IReadOnlyList<FortTerrainCell> IslandCells { get; }

    /// <summary>저장된 9칸이 완전한 작은 받침. 별도 island·islandStalag로 표시한다.</summary>
    public IReadOnlyList<FortIslandSupport> Supports { get; }

    /// <summary>영역 통로와 시드 성장 결과에 원본 isle 타일을 대응시킨다.</summary>
    public FortTerrainPreview(FortMap map, TypeDefinition isle)
    {
        var chunks = new List<(FortTerritory Territory, IReadOnlyList<FortTerrainChunk> Chunks)>();
        // 전체 청크 소속을 먼저 등록하여 영역 바깥으로 지면이 자라는 것을 막는다.
        foreach (FortTerritory territory in map.Territories.Where(t => t.IsActive))
        {
            var cells = FortMap.TerritoryChunkShapes(territory);
            chunks.Add((territory, cells));
            FortMapObject? vortex = map.Objects.FirstOrDefault(o => o.Territory == territory.Index && o.Object.Type.Definition.HasFlag("vortex"));
            _themes[territory.Index] = vortex?.Object.Type.Definition.GetString("theme") ?? "sun";
            _owners[territory.Index] = vortex?.Object.Owner ?? 0;
            // 영역의 각 청크를 좌표로 찾을 수 있게 등록한다.
            foreach (FortTerrainChunk cell in cells)
            {
                _chunks[(cell.X, cell.Y)] = territory.Index;
            }
        }
        var ordered = chunks.SelectMany(group => group.Chunks.Select((cell, index) =>
            (group.Territory, Cell: cell, Index: index, Count: group.Chunks.Count)))
            .OrderBy(item => item.Cell.Y).ThenBy(item => item.Cell.X).ToArray();
        // 목표 개수 배열을 영역별로 준비한다.
        foreach (var (territory, cells) in chunks)
        {
            _targets[territory.Index] = new int[cells.Count];
        }
        // 원본처럼 모든 연결 통로를 만든 뒤 별도 단계에서 성장시킨다.
        foreach (var (territory, cell, i, _) in ordered)
        {
            int x = cell.X * FortMap.CellsPerChunk;
            int y = cell.Y * FortMap.CellsPerChunk;
            int cx = x + 4 + (territory.Appearance & 7);
            int cy = y + 4 + ((9999 - territory.Appearance) & 7);
            int initial = Fill(cx - 1, cy - 1, cx + 3, cy + 3, territory.Index);
            int mask = ConnectionMask(cell.Orientation);
            initial += Fill((mask & 8) != 0 ? x : cx, (mask & 1) != 0 ? y : cy,
                (mask & 2) != 0 ? x + 16 : cx + 2, (mask & 4) != 0 ? y + 16 : cy + 2, territory.Index);
            if (i == 1)
            {
                initial += Fill(x + 3, y + 3, x + 13, y + 13, territory.Index);
            }
            // 원본 004c117e~004c1199는 청크마다 증가 전 영역 카운터로 밀도 시드를 다시 만든다.
            uint densitySeed = unchecked((uint)i * ChunkSeedStep + territory.Appearance);
            int percentage = Math.Max(20, Next(ref densitySeed, 30) + 50 - initial * 100 / 256);
            _targets[territory.Index][i] = percentage * 256 / 100;
        }
        // 청크별 저장 시드로 작은 덩어리를 붙여 비정형 섬을 생성한다.
        foreach (var (territory, cell, i, count) in ordered)
        {
            // 성장 시 영역 카운터는 초기 통로 생성에서 이미 영역의 전체 청크 수만큼 증가했다.
            uint seed = unchecked((uint)(count + i) * ChunkSeedStep + territory.Appearance);
            Grow(cell, territory.Index, _targets[territory.Index][i], ref seed);
        }
        Smooth();
        IslandCells = Enumerable.Range(0, _land.Length).Where(i => _land[i] >= 0)
            .Select(i => new FortTerrainCell(i % WorldSize, i / WorldSize, _land[i])).ToArray();
        Supports = FortIslandSupports.Find(map.Objects);
        var supportCells = new HashSet<(int X, int Y)>();
        // 전용 스프라이트로 그릴 받침은 일반 isle 타일과 fringe가 중복되지 않게 표시한다.
        foreach (FortIslandSupport support in Supports)
        {
            // 받침의 세 행을 등록한다.
            for (int y = support.Y - FortIslandSupports.Size + 1; y <= support.Y; y++)
            {
                // 같은 행의 세 칸을 등록한다.
                for (int x = support.X - FortIslandSupports.Size + 1; x <= support.X; x++)
                {
                    supportCells.Add((x, y));
                }
            }
        }
        // 작은 받침 섬은 noIsland 칸과 createsisland 건물의 발자국으로 따로 구성한다.
        foreach (FortMapObject item in map.Objects)
        {
            if (supportCells.Contains((item.X, item.Y)))
            {
                continue;
            }
            if (item.Object.Type.Name == "noIsland")
            {
                // 불완전한 받침도 소유자별로 나누어 인접한 다른 색상의 칸을 합치지 않는다.
                int region = -2 - (item.Object.Owner ?? 0);
                _themes.TryAdd(region, "sun");
                _owners.TryAdd(region, item.Object.Owner ?? 0);
                Put(item.X, item.Y, region);
            }
            else if (item.Object.Type.Definition.HasFlag("createsisland"))
            {
                int region = -3 - item.Object.Type.LoadIndex;
                // 개발용 받침 타일은 소유자의 신전 원소를 사용한다. 정확한 원본 색 선택은 후속 검증 대상이다.
                string theme = map.Objects.FirstOrDefault(o => o.Object.Owner == item.Object.Owner && o.Object.Type.Definition.HasFlag("vortex"))
                    ?.Object.Type.Definition.GetString("theme") ?? "sun";
                // 같은 타입이어도 소유자의 신전 원소가 다를 수 있으므로 소유자별 영역을 구분한다.
                region = region * 10 - (item.Object.Owner ?? 0);
                _themes.TryAdd(region, theme);
                _owners.TryAdd(region, item.Object.Owner ?? 0);
                Fill(item.X - 2, item.Y - 2, item.X + 1, item.Y + 1, region);
            }
        }
        var tiles = new List<FortTerrainTile>();
        // y·x 순서로 지면 타일을 선택해 렌더링 순서를 고정한다.
        for (int y = 0; y < WorldSize; y++)
        {
            // 같은 행의 각 지면 칸에 가장자리에 맞는 타일을 붙인다.
            for (int x = 0; x < WorldSize; x++)
            {
                int region = At(x, y);
                if (region == -1 || supportCells.Contains((x, y)))
                {
                    continue;
                }
                string theme = _themes[region];
                char a = MaskOrientation(NeighborMask(x, y, region, false));
                char b = MaskOrientation(NeighborMask(x, y, region, true));
                int cluster = SelectCluster(isle, a, b, theme, x, y, region >= 0);
                tiles.Add(new FortTerrainTile(x, y, region, theme, cluster) { Owner = _owners.GetValueOrDefault(region) });
            }
        }
        Tiles = tiles;
    }

    /// <summary>영역의 지면 원소 (영역 신전의 theme, 신전이 없거나 영역 밖이면 sun).</summary>
    /// <param name="territory">영역 번호. null 은 영역 밖(Chaff) 오브젝트</param>
    public string TerritoryTheme(int? territory) =>
        territory.HasValue ? _themes.GetValueOrDefault(territory.Value, "sun") : "sun";

    /// <summary>방향 문자를 원본 연결 비트로 옮긴다.</summary>
    public static int ConnectionMask(char orientation) => Connections[orientation - 'A'];
    /// <summary>이웃 비트를 원본 방향 문자로 옮긴다.</summary>
    public static char MaskOrientation(int mask) => MaskCharacters[mask & 15];

    /// <summary>원본 정수 난수 함수: 32비트 오버플로 후 상위 16비트를 사용한다.</summary>
    private static int Next(ref uint state, int limit)
    {
        if (limit < 1)
        {
            return 0;
        }
        state = unchecked((state == 0 ? DefaultSeed : state) * 0x10003 + 3);
        return (int)((state >> 16) % (uint)limit);
    }

    /// <summary>사각형에 지면을 채우고 새로 생긴 칸 수를 센다.</summary>
    private int Fill(int left, int top, int right, int bottom, int region)
    {
        int count = 0;
        // 사각형의 각 행을 채운다.
        for (int y = top; y < bottom; y++)
        {
            // 같은 영역의 기존 칸은 중복 집계하지 않는다.
            for (int x = left; x < right; x++)
            {
                if (At(x, y) == -1 && Put(x, y, region))
                {
                    count++;
                }
            }
        }
        return count;
    }

    /// <summary>기존 지면에 붙는 3×3 덩어리로 청크를 성장시킨다 (004c0800).</summary>
    private void Grow(FortTerrainChunk cell, int region, int remaining, ref uint seed)
    {
        int retries = GrowthRetryLimit;
        // 새 칸이 생기면 실패 횟수를 초기화하며 목표 개수까지 성장한다.
        while (remaining > 0 && retries-- > 0)
        {
            int x = cell.X * 16 + Next(ref seed, 15);
            int y = cell.Y * 16 + Next(ref seed, 15);
            Next(ref seed, 1);
            if (At(x, y) != region)
            {
                continue;
            }
            int added = 0;
            // 후보 중심 주변의 세 행을 조사한다.
            for (int dy = -1; dy <= 1; dy++)
            {
                // 같은 영역 청크 안의 빈 칸에만 지면을 추가한다.
                for (int dx = -1; dx <= 1; dx++)
                {
                    int px = x + dx;
                    int py = y + dy;
                    if (_chunks.GetValueOrDefault((px / 16, py / 16), -1) == region && At(px, py) == -1 && Put(px, py, region))
                    {
                        added++;
                    }
                }
            }
            if (added > 0)
            {
                remaining -= added;
                retries = GrowthRetryLimit;
            }
        }
    }

    /// <summary>여덟 이웃 중 연속 다섯 칸 이상이 같은 영역인 빈 틈을 두 차례 메운다.</summary>
    private void Smooth()
    {
        // 원본 004c10a0은 두 차례의 빈 틈 보정으로 끝난다.
        for (int pass = 0; pass < 2; pass++)
        {
            // 원본과 같은 y·x 순서로 즉시 결과를 반영한다.
            for (int y = 1; y < WorldSize - 1; y++)
            {
                // 테두리를 제외한 빈 칸의 이웃을 조사한다.
                for (int x = 1; x < WorldSize - 1; x++)
                {
                    if (At(x, y) != -1)
                    {
                        continue;
                    }
                    // 원본 004c0cf0은 직선 방향에 기존 지면이 있는 빈 칸만 보정 후보로 삼는다.
                    if (At(x, y - 1) == -1 && At(x + 1, y) == -1 && At(x, y + 1) == -1 && At(x - 1, y) == -1)
                    {
                        continue;
                    }
                    int previous = -1;
                    int run = 0;
                    // 회전 경계에 걸친 연속 이웃도 검사하기 위해 13방향을 순회한다.
                    for (int j = 0; j <= 12; j++)
                    {
                        var (dx, dy) = Neighbors[j & 7];
                        int region = At(x + dx, y + dy);
                        run = region != -1 && (run == 0 || region == previous) ? run + 1 : 0;
                        previous = region;
                        if (run > 4 && (j & 1) != 0)
                        {
                            Put(x, y, region);
                            break;
                        }
                    }
                }
            }
        }
    }

    /// <summary>같은 영역의 네 방향 또는 대각선 연결 비트를 계산한다.</summary>
    private int NeighborMask(int x, int y, int region, bool diagonal)
    {
        int mask = 0;
        // 원본 비트 순서: 직선은 북·동·남·서, 대각선은 북서·북동·남동·남서.
        for (int i = 0; i < 4; i++)
        {
            var (dx, dy) = Neighbors[diagonal ? (7 + i * 2) & 7 : i * 2];
            if (At(x + dx, y + dy) == region)
            {
                mask |= 1 << i;
            }
        }
        return mask;
    }

    /// <summary>범위 밖은 빈 칸으로 처리하고 지면의 영역 번호를 반환한다.</summary>
    private int At(int x, int y) => (uint)x < WorldSize && (uint)y < WorldSize ? _land[y * WorldSize + x] : -1;

    /// <summary>범위 안의 빈 칸에 지면을 추가한다.</summary>
    private bool Put(int x, int y, int region)
    {
        if ((uint)x >= WorldSize || (uint)y >= WorldSize || At(x, y) != -1)
        {
            return false;
        }
        _land[y * WorldSize + x] = region;
        return true;
    }

    /// <summary>원소·연결 방향을 따르고 안쪽 AA 타일은 고정 시드의 원본 99항목 표로 3×3 그림을 선택한다.</summary>
    public static int SelectCluster(TypeDefinition definition, char a, char b, string theme, int x, int y,
        bool useCorePattern = true)
    {
        // 원본 004c04b0은 안쪽 AA 칸을 JJ00 다음의 연속 3×3 그림으로 먼저 선택한다.
        if (useCorePattern && a == 'A' && b == 'A'
            && CorePatternCluster(definition, theme, x, y) is int coreCluster)
        {
            return coreCluster;
        }
        int[] candidates = Candidates(definition, $"{a}{b}", theme);
        if (candidates.Length == 0)
        {
            (char normalizedA, char normalizedB) = NormalizeOrientation(a, b);
            candidates = Candidates(definition, $"{normalizedA}{normalizedB}", theme);
        }
        if (candidates.Length == 0)
        {
            candidates = Candidates(definition, $"{a}A", theme);
        }
        if (candidates.Length == 0)
        {
            candidates = Candidates(definition, $"{a}", theme);
        }
        if (candidates.Length == 0)
        {
            candidates = Candidates(definition, "AA", theme);
        }
        if (candidates.Length == 0)
        {
            throw new InvalidDataException($"{definition.Name}: 테마 {theme}의 지면 타일이 없습니다.");
        }
        return PickEdgeVariant(candidates, x, y);
    }

    /// <summary>원본 0049aa90의 원소별 후보 범위처럼 첫 프레임을 건너뛰고 변형을 고른다.</summary>
    private static int PickEdgeVariant(int[] candidates, int x, int y)
    {
        int firstVariant = candidates.Length > 1 ? 1 : 0;
        int variantCount = candidates.Length - firstVariant;
        return candidates[firstVariant + (x * 31 + y * 17) % variantCount];
    }

    /// <summary>JJ00 뒤 원소별 36프레임에서 같은 3×3 묶음의 좌표에 맞는 한 칸을 찾는다.</summary>
    private static int? CorePatternCluster(TypeDefinition definition, string theme, int x, int y)
    {
        int themeIndex = theme switch { "sun" => 0, "thunder" => 1, "wind" => 2, "rain" => 3, _ => -1 };
        if (themeIndex < 0)
        {
            return null;
        }
        int placeholder = -1;
        // 원본 타입 정의에서 3×3 그림 시작 전의 JJ00 자리표시자를 찾는다.
        for (int i = 0; i < definition.Clusters.Count; i++)
        {
            if (definition.Clusters[i].Name == "JJ00")
            {
                placeholder = i;
                break;
            }
        }
        int first = placeholder + 1 + themeIndex * CoreThemeFrameCount;
        if (placeholder < 0 || first + CoreThemeFrameCount > definition.Clusters.Count)
        {
            return null;
        }
        // 잘못된 타입 정의에서는 연속 프레임 규칙을 적용하지 않고 기존 방향 폴백을 사용한다.
        for (int i = first; i < first + CoreThemeFrameCount; i++)
        {
            Cluster cluster = definition.Clusters[i];
            if (cluster.Name != "AA00" || cluster.Layers.Count == 0
                || MapSpriteFrames.ImageTheme(cluster.Layers[0].Image) != theme)
            {
                return null;
            }
        }
        int variant = CoreVariant(x / CorePatternSize, y / CorePatternSize);
        return first + variant * CorePatternSize * CorePatternSize
            + (y % CorePatternSize) * CorePatternSize + x % CorePatternSize;
    }

    /// <summary>원본 004c04b0의 두 좌표 인덱스와 99개 난수표로 변형 번호를 고른다.</summary>
    private static int CoreVariant(int blockX, int blockY)
    {
        int index = (CoreVariantTable[blockY % CoreVariantTableSize]
            + CoreVariantTable[blockX % CoreVariantTableSize]) % CoreVariantTableSize;
        return (CoreVariantTable[index] + blockY) % CoreVariantCount;
    }

    /// <summary>원본 004c04b0처럼 연속 항목의 하위 2비트가 같으면 뒤 항목을 1 올린다.</summary>
    private static int[] CreateCoreVariantTable()
    {
        var values = new int[CoreVariantTableSize];
        uint state = PreviewCoreSeed;
        // 원본 00455970은 시드 직후 100개를 버리고 세 값을 추가로 소비한다.
        for (int i = 0; i < CoreSeedWarmupCount; i++)
        {
            NextCoreRandom(ref state);
        }
        // 원본이 _rand를 99회 호출해 표를 채우는 순서를 유지한다.
        for (int i = 0; i < values.Length; i++)
        {
            int value = NextCoreRandom(ref state);
            if (i > 0 && (value & (CoreVariantCount - 1)) == (values[i - 1] & (CoreVariantCount - 1)))
            {
                value++;
            }
            values[i] = value;
        }
        return values;
    }

    /// <summary>원본 MSVC _rand의 32비트 상태 전이와 15비트 반환값을 재현한다.</summary>
    private static int NextCoreRandom(ref uint state)
    {
        state = unchecked(state * CoreRandomMultiplier + CoreRandomIncrement);
        return (int)((state >> 16) & CoreRandomResultMask);
    }

    /// <summary>원본 0041cd20의 방향 정규화: 직선 이웃이 지지하는 대각선만 남기고 직선 방향을 복원한다.</summary>
    public static (char Cardinal, char Diagonal) NormalizeOrientation(char a, char b)
    {
        int cardinal = ConnectionMask(a);
        int diagonal = ConnectionMask(b);
        int supported = 0;
        // 대각선 비트 i는 직선 비트 i와 이전 직선 비트가 모두 있을 때 유효하다.
        for (int i = 0; i < 4; i++)
        {
            int pair = (1 << i) | (1 << ((i + 3) & 3));
            if ((cardinal & pair) == pair)
            {
                supported |= 1 << i;
            }
        }
        diagonal &= supported;
        cardinal = 0;
        // 유효 대각선으로부터 정규화된 직선 연결을 복원한다.
        for (int i = 0; i < 4; i++)
        {
            if ((diagonal & (1 << i)) != 0)
            {
                cardinal |= (1 << i) | (1 << ((i + 3) & 3));
            }
        }
        return (MaskOrientation(cardinal), MaskOrientation(diagonal));
    }

    /// <summary>연결 이름과 원소별 그림 이름에 맞는 타일만 골라 테마가 섞이는 것을 막는다.</summary>
    private static int[] Candidates(TypeDefinition definition, string prefix, string theme)
    {
        return Enumerable.Range(0, definition.Clusters.Count).Where(i =>
        {
            Cluster cluster = definition.Clusters[i];
            string image = cluster.Layers[0].Image;
            string actual = image.StartsWith("RA", StringComparison.OrdinalIgnoreCase) ? "rain"
                : image.StartsWith("TH", StringComparison.OrdinalIgnoreCase) ? "thunder"
                : image.StartsWith("WI", StringComparison.OrdinalIgnoreCase) ? "wind" : "sun";
            // AA00은 본섬의 3×3 조각이므로 가장자리와 불완전한 받침의 일반 후보에서 제외한다.
            return cluster.Name != "AA00" && cluster.Name.StartsWith(prefix, StringComparison.Ordinal)
                && actual == theme;
        }).ToArray();
    }
}
