// 실제 일반 초기 지형의 통로·청크별 성장·빈틈 보정을 현재 소유된 컨텍스트에 연결한다.
using System;

// 원본 초기 지형 성장 및 희소 셀 제거·빈틈 보정 모듈을 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Original;

// 첫 시드/청크 조회자의 원문 진단만 연결하는 클래스다. 정상 입력은 진단 장치를 요구하지 않으며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class TerrainGrowthDiagnostics
{
    // 첫 시드 실패가 그 시점의 전체 checking DWORD를 읽는 비소유 뷰 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawCheckingFlag Checking { get; init; } = new();

    // 표현식/원본 파일/행을 보존하며 정상 복귀 후 계속하는 진단 콜백 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public Action<string, string, uint>? Assertion { get; init; }
}

// 패치 004c10a0 / CD 0049e160의 일반 생성 분기를 복원하는 클래스다. 군도 상위 분기는 별도로 조립하며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class RawTerrainGrowth
{
    // 패치 로컬 난수의 청크 방문 번호 증가량 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const uint RawChunkSeedStep = 0x10e3u;

    // 성장 연속 실패 한계 횟수 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const int RawGrowthFailures = 1000;

    // 새 지도 사본 없이 실제 code/region/현재 bounds를 쓰는 소유된 컨텍스트 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly RawTerrainContext _context;

    // 원본 12바이트 항목의 섬 번호/방향/seed/ordinal을 읽는 현재 청크 지도 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly ChunkMap _chunks;

    // 선택 지역/빈 청크 비교가 읽는 현재 noRegion 전역 필드다. 생성자 초기값으로 복제하지 않으며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly TerrainContextGlobals _globals;

    // 현재 checking 소유자와 선택된 진단 경계를 빌리는 진단 설정 필드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private readonly TerrainGrowthDiagnostics _diagnostics;

    // 실제 조회 자료의 수명만 빌리며 지도/난수를 변경하지 않는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public RawTerrainGrowth(
        RawTerrainContext context,
        ChunkMap chunks,
        TerrainContextGlobals globals,
        TerrainGrowthDiagnostics? diagnostics = null)
    {
        ArgumentNullException.ThrowIfNull(context);
        ArgumentNullException.ThrowIfNull(chunks);
        ArgumentNullException.ThrowIfNull(globals);
        _context = context;
        _chunks = chunks;
        _globals = globals;
        _diagnostics = diagnostics ?? new TerrainGrowthDiagnostics();
    }

    // 원본 Y외부/X내부 통로 → 현재 전체 청크 성장 → 두 번 빈틈 보정을 같은 배열에 수행하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void RunNormal()
    {
        Span<int> targets = stackalloc int[256];
        Span<uint> visits = stackalloc uint[100];
        // 전체 통로를 Y청크 순서로 먼저 구성하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int y = _context.Chunks.Top; y < _context.Chunks.Bottom; ++y)
        {
            // 각 Y청크 행의 X청크를 현재 bounds에서 다시 읽어 통로와 목표 밀도를 계산하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int x = _context.Chunks.Left; x < _context.Chunks.Right; ++x)
            {
                ref ChunkEntry chunk = ref _chunks.At(x, y);
                int island = ChunkRegion(x, y);
                if (island == _globals.NoRegion || (_context.Region != _globals.NoRegion && island != _context.Region))
                {
                    continue;
                }

                if ((uint)island >= 100u)
                {
                    throw new ArgumentOutOfRangeException(nameof(island), "초기 성장 섬 방문 배열 범위 오류");
                }

                byte seedByte = chunk.Record[3];
                uint seed = (visits[island] * RawChunkSeedStep) + seedByte;
                ++visits[island];
                int left = x * 16;
                int top = y * 16;
                int cx = left + 4 + (seedByte & 7);
                int cy = top + 4 + ((9999 - seedByte) & 7);
                int mask = TerrainBuilder.Connection((char)(byte)chunk.Side);
                int added = Fill(cx - 1, cy - 1, cx + 3, cy + 3, (byte)island);
                added += Fill(
                    (mask & 8) != 0 ? left : cx,
                    (mask & 1) != 0 ? top : cy,
                    (mask & 2) != 0 ? left + 16 : cx + 2,
                    (mask & 4) != 0 ? top + 16 : cy + 2,
                    (byte)island);
                if (visits[island] == 2u)
                {
                    added += Fill(left + 3, top + 3, left + 13, top + 13, (byte)island);
                }

                int density = Math.Max(20, Next(ref seed, 30u) + 50 - ((added * 100) / 256));
                targets[(x * 16) + y] = (density * 256) / 100;
            }
        }

        // 통로가 모두 만들어진 뒤 모든 비noRegion 청크의 실제 성장 도우미를 호출하는 외부 Y 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int y = _context.Chunks.Top; y < _context.Chunks.Bottom; ++y)
        {
            // 현재 청크를 같은 X순서로 읽고 방문 수 증가를 원본처럼 유지하는 내부 X 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int x = _context.Chunks.Left; x < _context.Chunks.Right; ++x)
            {
                ref ChunkEntry chunk = ref _chunks.At(x, y);
                int island = ChunkRegion(x, y);
                if (island == _globals.NoRegion)
                {
                    continue;
                }

                if ((uint)island >= 100u)
                {
                    throw new ArgumentOutOfRangeException(nameof(island), "초기 성장 현재 섬 배열 범위 오류");
                }

                uint seed = (visits[island] * RawChunkSeedStep) + chunk.Record[3];
                ++visits[island];
                Grow(island, x, y, targets[(x * 16) + y], ref seed);
            }
        }

        FixGaps(2);
    }

    // 004c0e20 / CD 0049de40: 첫 비noRegion 청크의 raw+5 BYTE를 반환하고 실패 시 0x2333을 보존하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public uint FirstChunkSeed()
    {
        int cdNoRegion = _globals.NoRegion;
        // 원본 청크 범위는 Y행이 외부이며 진단 복귀 뒤 right/bottom을 다시 읽는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int y = _context.Chunks.Top; y < _context.Chunks.Bottom; ++y)
        {
            // 현재 행의 left에서 시작하고 첫 비어 있지 않은 raw 청크만 반환하는 내부 X 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int x = _context.Chunks.Left; x < _context.Chunks.Right; ++x)
            {
                ref ChunkEntry chunk = ref SeedChunkAt(x, y);
                int noRegion = _context.Edition == OriginalEdition.Patch1078 ? _globals.NoRegion : cdNoRegion;
                if (unchecked((sbyte)chunk.Island) != noRegion)
                {
                    return chunk.Record[3];
                }
            }
        }

        if (_context.Edition == OriginalEdition.Patch1078 && _diagnostics.Checking.IsEnabled)
        {
            Assert("0", "\\Ns\\O\\Terrainbuilder.cpp", 0x25fu);
        }

        return 0x2333u;
    }

    // 004c13f0 / CD 0049e4e0: 첫 시드×93의 로컬 난수로 count개의 성공을 흩뿌리는 메서드다. 중복도 성공이며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void Scatter(int count)
    {
        uint seed = unchecked(FirstChunkSeed() * 0x5du);
        uint remaining = unchecked((uint)count);
        // 거부된 칸은 count를 줄이지 않고 남은 개수가 소진될 때까지 반복하는 루프다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        while (remaining != 0u)
        {
            int[] bounds = _context.Map.Bounds;
            int x = NextRange(
                ref seed,
                unchecked((int)((uint)bounds[0] + 2u)),
                unchecked((int)((uint)bounds[2] - 2u)));
            int y = NextRange(
                ref seed,
                unchecked((int)((uint)bounds[1] + 2u)),
                unchecked((int)((uint)bounds[3] - 2u)));
            int chunkX = CoordinateToChunk(x);
            int chunkY = CoordinateToChunk(y);
            bool hasIsland = unchecked((sbyte)SeedChunkAt(chunkX, chunkY).Island) != _globals.NoRegion;
            if (hasIsland && SeedChunkAt(chunkX, chunkY).Island != 0)
            {
                continue;
            }

            int index = Index(x, y);
            _context.Codes[index] = (byte)'x';
            _context.Regions[index] = 0;
            --remaining;
        }
    }

    // 004c0800 / CD 0049d690: 청크 사각형의 X폭을 두 좌표 난수에 사용하고 호출자의 시드를 갱신하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void GrowRectangle(int island, ChunkRect bounds, int target, ref uint seed)
    {
        int width = unchecked((int)((((uint)bounds.Right - (uint)bounds.Left) * 16u) - 1u));
        int failures = RawGrowthFailures;
        // 성공이 없는 연속 1000회 또는 목표 추가량 소진에서 종료하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        while (target > 0 && failures != 0)
        {
            --failures;
            int cx = AddWrapped(CellShift(bounds.Left), NextRange(ref seed, 0, width));
            int cy = AddWrapped(CellShift(bounds.Top), NextRange(ref seed, 0, width));
            _ = Next(ref seed, 1u);
            int index = Index(cx, cy);
            if (_context.Codes[index] != (byte)'x' || (_context.ArchipelagoMode == 0u && _context.Regions[index] != unchecked((byte)island)))
            {
                continue;
            }

            int before = target;
            // 현재 원본 3×3 영역의 Y행을 먼저 순회하는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int ny = cy - 1; ny <= cy + 1; ++ny)
            {
                // 각 행의 X를 처음부터 다시 읽으며 비어 있는 칸을 추가하는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int nx = cx - 1; nx <= cx + 1; ++nx)
                {
                    if (Put(nx, ny, unchecked((byte)island)))
                    {
                        --target;
                    }
                }
            }

            if (target < before)
            {
                failures = RawGrowthFailures;
            }
        }
    }

    // 004c0b30 / CD 0049da00: 현재 x의 직교 2 / 대각 1 가중 이웃 합이 threshold 이하이면 제거하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public bool RemoveSparseCell(int x, int y, int threshold)
    {
        int index = Index(x, y);
        if (_context.Codes[index] != (byte)'x')
        {
            return false;
        }

        int weight = 0;
        (int Dx, int Dy)[] neighbors = TerrainBuilder.NeighborCells;
        // 북부터 시계방향으로 읽으며 짝수 방향의 직교 이웃에만 추가 가중치를 주는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int direction = 0; direction < neighbors.Length; ++direction)
        {
            (int dx, int dy) = neighbors[direction];
            if (_context.Codes[Index(x + dx, y + dy)] == (byte)'x')
            {
                weight += (direction & 1) != 0 ? 1 : 2;
            }
        }

        if (weight > threshold)
        {
            return false;
        }

        _context.Regions[index] = unchecked((byte)_globals.NoRegion);
        _context.Codes[index] = (byte)'.';
        return true;
    }

    // 004c0db0 / CD 0049dbb0: 내부 칸을 Y외부/X내부로 즉시 제거하며 변화가 없을 때까지 반복하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void RemoveSparse()
    {
        bool changed;
        // 한 패스 동안 제거된 칸이 하나라도 있으면 다시 전체 패스를 돌리는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        do
        {
            changed = false;
            // 원문 내부 Y행 끝은 bottom-1 직전까지 순회하는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int y = _context.Map.Bounds[1] + 1; y < _context.Map.Bounds[3] - 1; ++y)
            {
                // 현재 right를 각 칸 뒤 다시 읽고 임계값 4로 지우는 내부 X 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int x = _context.Map.Bounds[0] + 1; x < _context.Map.Bounds[2] - 1; ++x)
                {
                    changed = RemoveSparseCell(x, y, 4) || changed;
                }
            }
        }
        while (changed);
    }

    // 004c0fe0 / CD 0049e030: 플레이어 청크 사각형을 float32 칸 경계로 바꿔 code/지역을 지우는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void ClearPlacement(ChunkRect bounds)
    {
        float left = CellFloat(bounds.Left);
        float top = CellFloat(bounds.Top);
        float right = CellFloat(bounds.Right);
        float bottom = CellFloat(bounds.Bottom);
        // Y행을 먼저 진행하며 원문 context의 현재 bounds로 잘라내지 않는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int y = FirstFtol(top); (double)y < bottom; ++y)
        {
            // 각 행의 시작 X는 변환된 같은 left이며 오른쪽 경계는 포함하지 않는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int x = FirstFtol(left); (double)x < right; ++x)
            {
                int index = Index(x, y);
                _context.Regions[index] = unchecked((byte)_globals.NoRegion);
                _context.Codes[index] = (byte)'.';
            }
        }
    }

    // 원본 군도 루트도 같은 로컬 시드와 비양수 폭 계약으로 두 번 밀도를 추출하는 범위 난수 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public int NextRange(ref uint seed, int low, int high)
    {
        int width = unchecked((int)((uint)high - (uint)low));
        if (width < 1)
        {
            return low;
        }

        return unchecked((int)((uint)low + (uint)Next(ref seed, (uint)width)));
    }

    // 004c0cf0 / CD 0049db10: 군도 루트의 한 패스와 일반 성장의 두 패스를 같은 몸체로 실행하는 빈틈 보정 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    public void FixGaps(int passes)
    {
        // 각 패스의 현재 문자 변경을 즉시 다음 칸에 반영하는 패스 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int pass = 0; pass < passes; ++pass)
        {
            // Y는 현재 top+1부터 bottom 직전까지 순회하는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int y = _context.Map.Bounds[1] + 1; y < _context.Map.Bounds[3]; ++y)
            {
                // X는 현재 left+1부터 right-1 직전까지 순회하는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int x = _context.Map.Bounds[0] + 1; x < _context.Map.Bounds[2] - 1; ++x)
                {
                    if (_context.Codes[Index(x, y)] != (byte)'.')
                    {
                        continue;
                    }

                    if (_context.Codes[Index(x - 1, y)] == (byte)'x' ||
                        _context.Codes[Index(x + 1, y)] == (byte)'x' ||
                        _context.Codes[Index(x, y - 1)] == (byte)'x' ||
                        _context.Codes[Index(x, y + 1)] == (byte)'x')
                    {
                        FixCell(x, y);
                    }
                }
            }
        }
    }

    // 실제 지도 저장 범위를 검사하고 X열 우선 주소를 얻는 정적 메서드다. 손상 좌표를 clamp하지 않으며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int Index(int x, int y)
    {
        if ((uint)x >= 256u || (uint)y >= 256u)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "초기 성장 지도 범위 오류");
        }

        return (x * 256) + y;
    }

    // 원본 signed DWORD 왼쪽 시프트의 하위 32비트를 보존하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int CellShift(int value)
    {
        return unchecked((int)((uint)value << 4));
    }

    // 원본 32비트 시프트 후 float32 저장 지점을 보존하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static float CellFloat(int value)
    {
        return (float)CellShift(value);
    }

    // float32를 int64로 변환한 뒤 ftol의 하위 DWORD만 사용하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int FirstFtol(float value)
    {
        return unchecked((int)(uint)(long)value);
    }

    // 좌표 덧셈도 호스트 signed overflow 없이 원본 감김을 보존하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int AddWrapped(int left, int right)
    {
        return unchecked((int)((uint)left + (uint)right));
    }

    // 두 판본의 실제 호출자 로컬 시드를 소비하는 난수 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int Next(ref uint seed, uint limit)
    {
        return TerrainBuilder.Next(ref seed, unchecked((int)limit));
    }

    // 선택된 원문 진단만 보고하며 장치 누락을 무진단 성공으로 숨기지 않는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private void Assert(string expression, string file, uint line)
    {
        if (_diagnostics.Assertion is null)
        {
            throw new InvalidOperationException("초기 지형 시드 진단 장치 누락");
        }

        _diagnostics.Assertion(expression, file, line);
    }

    // 패치 조회자의 mode/범위 진단 뒤 같은 현재 청크를 clamp 조회하는 메서드다. CD는 보고 없이 조회하며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private ref ChunkEntry SeedChunkAt(int x, int y)
    {
        if (_context.Edition == OriginalEdition.Patch1078)
        {
            if (_chunks.Mode == 0u)
            {
                Assert("isValid()", "\\Ns\\O\\Chunkmap.cpp", 0xb7u);
            }

            if (x < 0 || y < 0 || x >= _chunks.XLength() || y >= _chunks.YLength())
            {
                Assert("cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen", "\\Ns\\O\\Chunkmap.cpp", 0xb8u);
            }
        }

        return ref _chunks.At(x, y);
    }

    // 패치 float32→ftol 하위 DWORD와 CD 정수의 0방향 나눗셈으로 청크 좌표를 구하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private int CoordinateToChunk(int value)
    {
        if (_context.Edition == OriginalEdition.Cd1072)
        {
            return value / 16;
        }

        long rounded = (long)(float)value;
        return unchecked((int)(uint)rounded) / 16;
    }

    // 원본 청크 BYTE를 signed로 읽는 메서드다. 현재 섬 목록의 존재 조회와 별개이며, 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private int ChunkRegion(int x, int y)
    {
        return unchecked((sbyte)_chunks.At(x, y).Island);
    }

    // 004c0930 / CD 0049d810: X외부/Y내부 사각형에서 code가 다를 때만 code/region을 쓰는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private int Fill(int left, int top, int right, int bottom, byte region)
    {
        int added = 0;
        // 원본과 동일하게 X열을 먼저 진행하는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int x = left; x < right; ++x)
        {
            // 각 X열의 Y를 현재 끝까지 진행하는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int y = top; y < bottom; ++y)
            {
                int index = Index(x, y);
                if (_context.Codes[index] == (byte)'x')
                {
                    continue;
                }

                _context.Codes[index] = (byte)'x';
                _context.Regions[index] = region;
                ++added;
            }
        }

        return added;
    }

    // 004c0780 / CD 0049d5b0: inclusive bounds/현재 청크 지역을 만족하는 비x 칸만 추가하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private bool Put(int x, int y, byte region)
    {
        int[] bounds = _context.Map.Bounds;
        if (x < bounds[0] || y < bounds[1] || x > bounds[2] || y > bounds[3])
        {
            return false;
        }

        int index = Index(x, y);
        if (_context.Codes[index] == (byte)'x')
        {
            return false;
        }

        if (_context.ArchipelagoMode == 0u && ChunkRegion(x / 16, y / 16) != unchecked((sbyte)region))
        {
            return false;
        }

        _context.Regions[index] = region;
        _context.Codes[index] = (byte)'x';
        return true;
    }

    // 004c0800 / CD 0049d690: 단일 청크 사각형에 대해 GrowRectangle을 위임하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private void Grow(int island, int x, int y, int target, ref uint seed)
    {
        GrowRectangle(island, new ChunkRect(x, y, x + 1, y + 1), target, ref seed);
    }

    // 004c0c30 / CD 0049da90: 원래 이웃 13개를 읽어 같은 지역 연속 5개/대각선 끝에서 빈 칸을 메우는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private void FixCell(int x, int y)
    {
        int run = 0;
        int region = _globals.NoRegion;
        (int Dx, int Dy)[] neighbors = TerrainBuilder.NeighborCells;
        // 북부터 13번의 원본 조회 순서를 유지하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (uint direction = 0u; direction <= 12u; ++direction)
        {
            (int dx, int dy) = neighbors[(int)(direction & 7u)];
            int index = Index(x + dx, y + dy);
            if (_context.Codes[index] != (byte)'x')
            {
                run = 0;
                continue;
            }

            int current = unchecked((sbyte)_context.Regions[index]);
            if (run == 0)
            {
                region = current;
            }

            if (region != current)
            {
                run = 0;
                continue;
            }

            if (++run > 4 && (direction & 1u) != 0u)
            {
                int target = Index(x, y);
                _context.Regions[target] = unchecked((byte)region);
                _context.Codes[target] = (byte)'x';
                return;
            }
        }
    }
}
