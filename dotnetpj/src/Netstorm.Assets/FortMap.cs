using System.Text.Json;

namespace Netstorm.Assets;

/// <summary>Territory의 6바이트 레코드. 미확정 필드는 원본 값을 보존한다.</summary>
public sealed record FortTerritory(int Index, byte ShapeAndRotation, byte Flags, byte ChunkPosition,
    byte Appearance, byte ReservedA, byte ReservedB)
{
    /// <summary>모양 번호: 하위 6비트.</summary>
    public int Shape => ShapeAndRotation & 63;
    /// <summary>저장된 방향 값: 상위 2비트. 원본 전체 활성 영역에서는 0만 관찰되었다.</summary>
    public int Direction => ShapeAndRotation >> 6;
    /// <summary>생성 함수가 방향을 2로 나누어 얻는 90도 단위 회전 (FUN_00425c20).</summary>
    public int Rotation => Direction / 2;
    /// <summary>생성 대상 영역인지 (Islandbuilder FUN_0046dd70).</summary>
    public bool IsActive => (Flags & 1) != 0 && (Flags & 2) == 0;
    /// <summary>기준 사각형에 상대적인 청크 x.</summary>
    public int ChunkX => ChunkPosition & 15;
    /// <summary>기준 사각형에 상대적인 청크 y.</summary>
    public int ChunkY => ChunkPosition >> 4;
}

/// <summary>월드 좌표가 결정된 오브젝트. 영역 번호는 소유자와 별개이며 Chaff는 null이다.</summary>
public sealed record FortMapObject(int X, int Y, int? Territory, FortObject Object);

/// <summary>영역에 속하는 월드 청크와 원본 패턴의 연결 방향 문자.</summary>
public sealed record FortTerrainChunk(int X, int Y, char Orientation);

/// <summary>화면에서 확인한 16×11 픽셀 좌표계와 영역 청크 배치를 제공한다.</summary>
public sealed class FortMap
{
    /// <summary>한 청크의 칸 수.</summary>
    public const int CellsPerChunk = 16;
    /// <summary>한 칸의 화면 가로 길이.</summary>
    public const int CellPixelWidth = 16;
    /// <summary>한 칸의 화면 세로 길이.</summary>
    public const int CellPixelHeight = 11;
    /// <summary>Territory 레코드 크기.</summary>
    private const int TerritoryRecordSize = 6;
    /// <summary>저장 위치를 사용하는 미션에서 기준 사각형의 시작 청크.</summary>
    public const int MissionChunkOrigin = 1;

    private static readonly Pattern[] Patterns = LoadPatterns();

    /// <summary>표 전체 68개 영역의 변형·번호까지 보존한 원본 패턴. 파일의 Shape는 하위 6비트만 사용한다.</summary>
    public static CanonicalPattern TerritoryPattern(int index)
    {
        Pattern pattern = Patterns[index];
        var cells = new PatternCell[pattern.Cells.Length];
        // 원본 행 우선 셀마다 방향·변형·'a' 기준 번호를 대응시킨다.
        for (int i = 0; i < cells.Length; i++)
            cells[i] = new PatternCell(pattern.Cells[i], pattern.Cells[i] == '.' ? 0 : pattern.Variations[i], pattern.Labels[i]);
        return new CanonicalPattern(pattern.Width, pattern.Height, cells);
    }

    /// <summary>영역 표 (항상 20개).</summary>
    public IReadOnlyList<FortTerritory> Territories { get; }
    /// <summary>내용물과 생성 지형을 제외한, 저장된 월드 오브젝트.</summary>
    public IReadOnlyList<FortMapObject> Objects { get; }

    /// <summary>저장 좌표로 맵을 구성한다. 요새 편집 기준 원점은 (6,6)이며 전투 재배치는 별도 구현 대상이다.</summary>
    public FortMap(FortFile fort, int originChunkX = MissionChunkOrigin, int originChunkY = MissionChunkOrigin)
    {
        ReadOnlySpan<byte> section = fort.Section("Territory");
        if (section.Length != FortFile.TerritoryCount * TerritoryRecordSize)
        {
            throw new InvalidDataException("Territory 섹션은 6바이트 × 20개여야 합니다.");
        }
        var territories = new List<FortTerritory>();
        var objects = new List<FortMapObject>();
        // Chaff는 전체 월드의 y·x 순서 청크로 저장된다.
        foreach (FortChunk chunk in fort.Chaff)
        {
            AddObjects(objects, chunk, chunk.Index % FortFile.WorldChunksX,
                chunk.Index / FortFile.WorldChunksX, null);
        }
        // 영역별 패턴을 회전하고 실제 월드 청크 순서에 맞춰 오브젝트에 좌표를 붙인다.
        for (int i = 0; i < FortFile.TerritoryCount; i++)
        {
            ReadOnlySpan<byte> row = section.Slice(i * TerritoryRecordSize, TerritoryRecordSize);
            var territory = new FortTerritory(i, row[0], row[1], row[2], row[3], row[4], row[5]);
            territories.Add(territory);
            IReadOnlyList<FortChunk> chunks = fort.Territories[i];
            if (chunks.Count == 0)
            {
                continue;
            }
            if (!territory.IsActive)
            {
                throw new InvalidDataException($"Terr{i:00}: 비활성 영역에 청크가 있습니다.");
            }
            var positions = TerritoryChunks(territory, originChunkX, originChunkY);
            if (positions.Count != chunks.Count)
            {
                throw new InvalidDataException($"Terr{i:00}: 패턴 {positions.Count}개와 저장 청크 {chunks.Count}개가 다릅니다.");
            }
            // TerrNN 레코드는 회전된 패턴에서 빈 칸을 제외한 월드 y·x 순서다.
            for (int j = 0; j < chunks.Count; j++)
            {
                AddObjects(objects, chunks[j], positions[j].X, positions[j].Y, i);
            }
        }
        Territories = territories;
        Objects = objects;
    }

    /// <summary>회전된 모양에서 비어 있지 않은 청크의 좌표를 월드 y·x 순서로 반환한다.</summary>
    public static IReadOnlyList<(int X, int Y)> TerritoryChunks(FortTerritory territory,
        int originChunkX = MissionChunkOrigin, int originChunkY = MissionChunkOrigin)
        => TerritoryChunkShapes(territory, originChunkX, originChunkY).Select(c => (c.X, c.Y)).ToArray();

    /// <summary>지면 생성용 청크 좌표와 연결 방향을 원본 패턴에서 구한다.</summary>
    public static IReadOnlyList<FortTerrainChunk> TerritoryChunkShapes(FortTerritory territory,
        int originChunkX = MissionChunkOrigin, int originChunkY = MissionChunkOrigin)
    {
        if (!territory.IsActive)
        {
            return [];
        }
        Pattern pattern = Patterns[territory.Shape];
        var positions = new List<FortTerrainChunk>();
        // 원본 패턴의 각 셀을 회전해 목적지 청크에 대응시킨다.
        for (int y = 0; y < pattern.Height; y++)
        {
            // 각 행의 빈 칸은 청크 목록에 포함하지 않는다.
            for (int x = 0; x < pattern.Width; x++)
            {
                if (pattern.Cells[y * pattern.Width + x] == '.')
                {
                    continue;
                }
                (int rx, int ry) = territory.Rotation switch
                {
                    0 => (x, y),
                    1 => (pattern.Height - 1 - y, x),
                    2 => (pattern.Width - 1 - x, pattern.Height - 1 - y),
                    _ => (y, pattern.Width - 1 - x),
                };
                char orientation = pattern.Cells[y * pattern.Width + x];
                int mask = FortTerrainPreview.ConnectionMask(orientation);
                // 청크 패턴을 회전한 만큼 북·동·남·서 연결 비트도 회전한다.
                for (int turn = 0; turn < territory.Rotation; turn++)
                {
                    mask = (mask << 1 & 15) | (mask >> 3);
                }
                positions.Add(new FortTerrainChunk(originChunkX + territory.ChunkX + rx,
                    originChunkY + territory.ChunkY + ry, FortTerrainPreview.MaskOrientation(mask)));
            }
        }
        return positions.OrderBy(p => p.Y).ThenBy(p => p.X).ToArray();
    }

    /// <summary>청크 상대 칸 좌표를 월드 칸 좌표로 옮긴다.</summary>
    private static void AddObjects(List<FortMapObject> objects, FortChunk chunk, int x, int y, int? territory)
    {
        // 저장된 모든 오브젝트를 상위 니블=x, 하위 니블=y 규칙으로 변환한다.
        foreach (FortObject obj in chunk.Objects)
        {
            objects.Add(new FortMapObject(x * CellsPerChunk + obj.CellHigh,
                y * CellsPerChunk + obj.CellLow, territory, obj));
        }
    }

    /// <summary>분석 도구로 생성한 패턴을 내장 리소스에서 읽는다.</summary>
    private static Pattern[] LoadPatterns()
    {
        using Stream stream = typeof(FortMap).Assembly.GetManifestResourceStream("Netstorm.Assets.TerritoryPatterns.json")
            ?? throw new InvalidDataException("영역 패턴 리소스가 없습니다.");
        return JsonSerializer.Deserialize<Pattern[]>(stream)
            ?? throw new InvalidDataException("영역 패턴을 읽을 수 없습니다.");
    }

    /// <summary>패턴 원본의 폭·높이와 행 우선 셀 문자 ('.'은 빈 칸).</summary>
    private sealed record Pattern(int Width, int Height, string Cells, int[] Variations, int[] Labels);
}
