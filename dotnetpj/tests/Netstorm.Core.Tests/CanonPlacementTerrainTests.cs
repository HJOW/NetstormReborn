// 후보 지형·모양 종료·지역 얻기의 독립 원본 관찰을 대조한다.
using System.Globalization;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawCanonPlacementTerrain의 지형·지역·종료를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class CanonPlacementTerrainTests
{
    /// <summary>판본별 원본 관찰 행 수다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Rows = 3740;

    /// <summary>합성 배치 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint Own = 82u;

    /// <summary>합성 후보 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint Other = 83u;

    /// <summary>원본 실행기의 입력 배열을 복구한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="state">심을 상태다.</param>
    /// <param name="seed">입력 번호다.</param>
    private static void Seed(CanonPlacementTerrainState state, uint seed)
    {
        byte[] values = [(byte)state.EmptyRegion, 255, 4, 127, 128, 128];
        Array.Fill(state.Regions, values[(int)seed]);
        if (seed == 3u)
        {
            state.Regions[0] = 4;
        }

        if (seed == 5u)
        {
            state.Regions[13] = 4;
        }
    }

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="raw">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>바이트를 실행기와 같은 소문자 16진수로 만든다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="bytes">바꿀 바이트다.</param>
    /// <returns>소문자 16진수다.</returns>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>동일 입력을 C# 모듈에 재생하여 전체 지역·바이트와 각 상태를 대조한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="name">판본 이름이다.</param>
    private static void Replay(string name)
    {
        OriginalEdition edition = name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        bool patch = edition == OriginalEdition.Patch1078;
        RawSidPool pool = new(24000, false, null, edition);
        OriginalTypeRecord[] types = new OriginalTypeRecord[patch ? 188 : 171];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        PriestPlainCanonType[] frames = new PriestPlainCanonType[types.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new PriestPlainCanonType();
        }

        ushort[] islands = new ushort[65536];
        CanonPlacementTerrainState state = new();
        RawCanonPlacementTerrain terrain = new(pool, types, frames, islands, state, new CanonPlacementTerrainHooks
        {
            CandidatePermission = (query, sid) => false,
            ShapePermission = (query, cell) => false,
            FinishRelations = (query, value) => false,
        });
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "canonterrain-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        List<string[]> fixture = [];
        // 주석·빈 줄을 빼고 행을 모으는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            fixture.Add(line.Split('\t'));
        }

        Assert.Equal(3 * Rows, fixture.Count);
        int count = 0;
        // 행마다 입력과 배열을 초기화하여 이전 순회 결과가 다음 행에 섞이지 않게 하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        foreach (string[] row in fixture)
        {
            if (row[0] != name)
            {
                continue;
            }

            Assert.Equal(28, row.Length);
            OriginalTypeRecord own = types[(int)Own];
            OriginalTypeRecord other = types[(int)Other];
            own.Flags1 = uint.Parse(row[4], CultureInfo.InvariantCulture);
            own.Flags2 = uint.Parse(row[1], CultureInfo.InvariantCulture);
            own.Group = int.Parse(row[2], CultureInfo.InvariantCulture);
            own.FootX = int.Parse(row[9], CultureInfo.InvariantCulture);
            own.FootY = int.Parse(row[10], CultureInfo.InvariantCulture);
            other.Flags2 = uint.Parse(row[5], CultureInfo.InvariantCulture);
            other.FootX = int.Parse(row[11], CultureInfo.InvariantCulture);
            other.FootY = int.Parse(row[12], CultureInfo.InvariantCulture);
            frames[(int)Other].Frames = new RiftTypeFrames(new[]
            {
                new RiftFrameCode(0, 0, 0, 0),
                new RiftFrameCode(0, 0, 0, (byte)uint.Parse(row[6], CultureInfo.InvariantCulture)),
            });
            Span<byte> raw = pool.RawSlot(50);
            raw.Fill(0xcd);
            raw[10] = (byte)Other;
            Put(raw, 8, uint.Parse(row[15], CultureInfo.InvariantCulture), 2);
            Put(raw, 14, uint.Parse(row[7], CultureInfo.InvariantCulture), 4);
            Put(raw, 18, uint.Parse(row[8], CultureInfo.InvariantCulture), 4);
            raw[patch ? 40 : 35] = (byte)uint.Parse(row[13], CultureInfo.InvariantCulture);
            Put(raw, patch ? 36 : 34, 1u, patch ? 4 : 1);
            Array.Clear(islands);
            int x = (int)BitConverter.UInt32BitsToSingle(uint.Parse(row[7], CultureInfo.InvariantCulture));
            int y = (int)BitConverter.UInt32BitsToSingle(uint.Parse(row[8], CultureInfo.InvariantCulture));
            if (x >= 0 && x < 256 && y >= 0 && y < 256)
            {
                islands[y * 256 + x] = uint.Parse(row[16], CultureInfo.InvariantCulture) != 0u ? (ushort)50 : (ushort)0;
            }

            state.EmptyRegion = uint.Parse(row[18], CultureInfo.InvariantCulture);
            state.NoIslandType = uint.Parse(row[14], CultureInfo.InvariantCulture);
            CanonPlacementQuery query = new(Own, Own, 20f, 21f, 0u, 1u, 0u);
            terrain.Begin(query);
            if (row[3] != "B")
            {
                terrain.BeginShape(query, new CanonPlacementCell(0, 0, 0, 20f, 21f, 20f, 21f, default));
            }

            Seed(state, uint.Parse(row[17], CultureInfo.InvariantCulture));
            state.GroundComplete = uint.Parse(row[19], CultureInfo.InvariantCulture) != 0u;
            if (row[3] != "B")
            {
                state.Permission = true;
            }

            uint result = 0u;
            if (row[3] == "B")
            {
            }
            else if (row[3] == "C")
            {
                terrain.Candidate(query, 50);
            }
            else if (row[3] == "E")
            {
                Assert.True(terrain.EndShape(query));
            }
            else if (row[3] == "F")
            {
                state.BridgeOverlap = uint.Parse(row[13], CultureInfo.InvariantCulture) != 0u;
                state.NoIslandOnly = uint.Parse(row[14], CultureInfo.InvariantCulture) != 0u;
                state.Permission = uint.Parse(row[16], CultureInfo.InvariantCulture) != 0u;
                result = terrain.Finish(query) ? 1u : 0u;
            }
            else
            {
                result = terrain.RegionAt(x, y);
            }

            bool same = result == uint.Parse(row[20], CultureInfo.InvariantCulture)
                && state.BridgeOverlap == (uint.Parse(row[21], CultureInfo.InvariantCulture) != 0u)
                && state.NoIslandOnly == (uint.Parse(row[22], CultureInfo.InvariantCulture) != 0u)
                && state.GroundComplete == (uint.Parse(row[23], CultureInfo.InvariantCulture) != 0u)
                && state.Permission == (uint.Parse(row[24], CultureInfo.InvariantCulture) != 0u)
                && state.CanPlaceGround == (uint.Parse(row[25], CultureInfo.InvariantCulture) != 0u)
                && Hex(state.Regions) == row[26]
                && Hex(patch ? pool.Slot(50) : pool.Slot(50).Slice(0, 36)) == row[27];
            Assert.True(same, $"Canon 지형 {name} 행 {count} ({row[3]}) 불일치");
            ++count;
        }

        Assert.Equal(Rows, count);
    }

    /// <summary>패치판의 실제 다리·섬·지역 얻기와 모양 종료를 대조한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void PatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD판의 바이트 프레임과 지역 얻기를 별도로 대조한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void CdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본도 독립 원본 관찰로 대조한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void ThirdX86()
    {
        Replay("original1037");
    }

    /// <summary>필수 경계·자료·풀과 순회 계약을 검사한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void RequiredBoundariesAndGuards()
    {
        RawSidPool pool = new(24000, false);
        RawSidPool otherPool = new(24000, false);
        OriginalTypeRecord[] types = new OriginalTypeRecord[188];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        PriestPlainCanonType[] frames = new PriestPlainCanonType[188];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new PriestPlainCanonType();
        }

        ushort[] islands = new ushort[65536];
        CanonPlacementTerrainState state = new();
        CanonPlacementTerrainHooks hooks = new()
        {
            CandidatePermission = (query, sid) => false,
            ShapePermission = (query, cell) => false,
            FinishRelations = (query, value) => false,
        };
        Assert.Throws<ArgumentException>(() => new RawCanonPlacementTerrain(pool, types, frames, islands, state, new CanonPlacementTerrainHooks()));
        RawCanonPlacementTerrain terrain = new(pool, types, frames, islands, state, hooks);
        CanonPlacementQuery query = new(Own, 0u, 20f, 21f, 0u, 1u, 0u);
        Assert.Throws<InvalidOperationException>(() => terrain.EndShape(query));
        terrain.Begin(query);
        terrain.BeginShape(query, new CanonPlacementCell(0, 0, 0, 20f, 21f, 20f, 21f, default));
        Assert.True(terrain.EndShape(query));
        Assert.False(terrain.Finish(query));
        Assert.False(state.GroundComplete);
        types[(int)Own].Flags2 = 4u;
        terrain.Begin(query);
        state.GroundComplete = false;
        Assert.False(terrain.Finish(query));
        Assert.True(state.CanPlaceGround);
        Assert.False(state.Permission);
        Assert.Throws<ArgumentException>(() => new RawCanonPlacementTerrain(pool, types, frames, new ushort[7], state, hooks));
        types[(int)Own].Flags2 = 0u;
        types[(int)Own].FootX = 13;
        Assert.Throws<ArgumentOutOfRangeException>(() => terrain.Begin(query));
        types[(int)Own].FootX = 0;
        terrain.Begin(query);
        Assert.Throws<ArgumentOutOfRangeException>(() => terrain.BeginShape(query, new CanonPlacementCell(0, 0, 0, 0f, 0f, float.PositiveInfinity, 21f, default)));
        Assert.Throws<ArgumentOutOfRangeException>(() => terrain.Begin(new CanonPlacementQuery(188u, 0u, 20f, 21f, 0u, 1u, 0u)));
        Assert.Equal(127u, terrain.RegionAt(-1, 300));
        Assert.NotSame(pool, otherPool);
    }
}
