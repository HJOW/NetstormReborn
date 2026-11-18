// 독립 원본 투표 관찰과 실제 지역 getter/outpost 수명 연결을 검사한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawRegionOwnership의 투표·도장·재투표를 검증하는 테스트 클래스다. 2026-11-18 0.618에서 R4-2 지역 소유권 투표 결선용으로 추가했다.
// 근거: cpppj RegionOwnershipTests.cpp와 regionownership-x86.tsv(원본·CD·1037 각 440행)다.
public sealed class RegionOwnershipTests
{
    /// <summary>독립 원본 관찰의 판본별 행 수다. 2026-11-18 0.618에서 추가했다.</summary>
    private const int Rows = 440;

    /// <summary>예상 밖 목록 항목이 있을 때 쓰는 채움 상수다. 2026-11-18 0.618에서 추가했다.</summary>
    private const uint FillPattern = 0xababababu;

    /// <summary>각 장면의 추가·작업장 목록 입력이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly (uint[] Additional, uint[] Workshops)[] Scenes =
    [
        ([], []), ([50], []), ([50, 53], [60]), ([50], [60, 63]), ([50, 51, 53], [60, 61]),
        ([50, 51, 52, 53, 54, 55], [60, 61, 62, 63, 64, 65]), ([50, 50, 50], [61, 64]), ([51, 54], [60, 63, 64]),
        ([], [60, 60, 63, 61]), ([52, 55], [62, 65]), ([50, 51], [60, 61, 63, 64, 65]), ([55, 50, 53, 50], [65, 60, 60, 63]),
    ];

    /// <summary>독립 좌표 입력이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly (float X, float Y)[] Coords = [(10.2F, 20.3F), (30.000001F, 21.00002F), (40.5F, 22.5F), (10.00002F, 20.000001F), (30.5F, 21.3F), (40.000001F, 22.000001F)];

    /// <summary>추가 목록 소유자 입력이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly byte[][] Owners = [[1, 2, 0, 1, 2, 8], [3, 3, 3, 3, 3, 3], [0, 0, 8, 2, 2, 8]];

    /// <summary>작업장 목록 소유자 입력이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly byte[][] WorkshopOwners = [[1, 2, 8, 1, 2, 8], [3, 4, 3, 3, 3, 3], [2, 0, 8, 2, 0, 8]];

    /// <summary>타입 테마 입력이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly uint[] Kinds = [0, 1, 2, 3, 0xffffffffu, 0];

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>원본 float 비트를 슬롯에 리틀 엔디언으로 쓴다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="slot">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutFloat(Span<byte> slot, int offset, float value)
    {
        uint bits = BitConverter.SingleToUInt32Bits(value);
        // 낮은 바이트부터 4바이트를 쓰는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            slot[offset + i] = (byte)(bits >> (i * 8));
        }
    }

    /// <summary>원본 값을 리틀 엔디언으로 쓴다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="slot">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> slot, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다.
        for (int i = 0; i < width; ++i)
        {
            slot[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>누적 입력 기록을 만들며 중복 지역을 지우지 않는다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="variant">지역 변형 번호다.</param>
    private static List<OwnershipAffectedRegion> Affected(uint variant)
    {
        uint[] regions = variant == 0 ? [7u, 9u] : variant == 1 ? [7u, 7u, 9u] : [9u, 9u, 7u];
        var result = new List<OwnershipAffectedRegion>();
        // 원본이 투표에 사용하지 않는 좌표도 같은 float 기록으로 공급한다.
        for (int i = 0; i < regions.Length; ++i)
        {
            result.Add(new OwnershipAffectedRegion(90.5F + i, 91.25F + i, regions[i]));
        }

        return result;
    }

    /// <summary>기대 출력은 저장된 기계어 관찰에서 한 번만 읽는다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="edition">판본 이름이다. originals·originalCD·original1037 중 하나다.</param>
    private static void Replay(string edition)
    {
        bool patch = edition == "originals";
        OriginalEdition version = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "regionownership-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        var fixture = new List<string[]>();
        // 주석·빈 줄을 빼고 행을 모으는 반복문이다.
        foreach (string line in lines)
        {
            if (!string.IsNullOrEmpty(line) && line[0] != '#')
            {
                fixture.Add(line.Split('\t'));
            }
        }

        Assert.Equal(3 * Rows, fixture.Count);
        var pool = new RawSidPool(24000, false, null, version);
        var types = new OriginalTypeRecord[patch ? 188 : 171];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        var frames = new PriestPlainCanonType[types.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new PriestPlainCanonType();
        }

        var islands = new ushort[65536];
        // 합성 고정 지도는 두 섬·다리·빈 칸의 실제 SID를 가리킨다.
        for (int y = 0; y < 256; ++y)
        {
            // 원본 실행기와 같은 x 구간을 공급하며 결과는 계산하지 않는다.
            for (int x = 0; x < 256; ++x)
            {
                islands[y * 256 + x] = (ushort)(x < 20 ? 100 : x < 40 ? 101 : x < 60 ? 102 : 0);
            }
        }

        var terrainState = new CanonPlacementTerrainState();
        var terrain = new RawCanonPlacementTerrain(pool, types, frames, islands, terrainState, new CanonPlacementTerrainHooks
        {
            CandidatePermission = (_, _) => false,
            ShapePermission = (_, _) => false,
            FinishRelations = (_, _) => false,
        });
        int count = 0;
        // 입력마다 새 목록·전역을 준비하며 이전 행의 테마나 누적 지역을 재사용하지 않는다.
        foreach (string[] row in fixture)
        {
            if (row[0] != edition)
            {
                continue;
            }

            Assert.Equal(13, row.Length);
            uint scene = Number(row[1]);
            uint variant = Number(row[2]);
            // 지역 getter는 원본 raw와 현재 genus를 읽으며 bridge는 sentinel을 반환한다.
            for (int i = 0; i < Coords.Length; ++i)
            {
                var themes = new uint[types.Length];
                themes[83 + i] = Kinds[(i + variant) % Kinds.Length];
                // outpost 추가 목록과 작업장 목록의 소유자 입력은 독립적이다.
                foreach (bool second in new[] { false, true })
                {
                    Span<byte> raw = pool.RawSlot((second ? 60 : 50) + i);
                    raw.Fill(0);
                    raw[10] = (byte)(83 + i);
                    raw[patch ? 34 : 32] = (second ? WorkshopOwners : Owners)[variant][i];
                    PutFloat(raw, 14, Coords[i].X);
                    PutFloat(raw, 18, Coords[i].Y);
                }
            }

            // 섬 SID의 raw 타입·지역·genus를 원본과 같이 둔다.
            foreach (int sid in new[] { 100, 101, 102 })
            {
                Span<byte> raw = pool.RawSlot(sid);
                raw.Fill(0);
                raw[10] = (byte)(sid - 17);
                Put(raw, 8, (uint)(sid == 100 ? 7 : sid == 101 ? 9 : 42), 2);
                types[sid - 17].Flags2 = sid == 100 ? 2u : sid == 101 ? 0x1000002u : 4u;
            }

            terrainState.EmptyRegion = Number(row[8]);
            var additionalEntries = new uint[8];
            var workshopEntries = new uint[8];
            Array.Fill(additionalEntries, 0xababababu);
            Array.Fill(workshopEntries, 0xababababu);
            Array.Copy(Scenes[scene].Additional, additionalEntries, Scenes[scene].Additional.Length);
            Array.Copy(Scenes[scene].Workshops, workshopEntries, Scenes[scene].Workshops.Length);
            var additional = new RawPostPopList { Entries = additionalEntries, Count = (uint)Scenes[scene].Additional.Length };
            var workshops = new RawPostPopList { Entries = workshopEntries, Count = (uint)Scenes[scene].Workshops.Length };
            uint[] beforeAdditional = (uint[])additionalEntries.Clone();
            uint[] beforeWorkshops = (uint[])workshopEntries.Clone();
            var themesTable = new uint[types.Length];
            var paintState = new RegionOwnershipState { Affected = Affected(variant), Revision = 0xffffffffu, Dirty = 0x12345678u };
            var events = new List<string>();
            uint paints = 0;
            var hooks = RawRegionOwnership.MakeTerrainRegionOwnershipHooks(pool, terrain, new RegionOwnershipHooks
            {
                Paint = paint =>
                {
                    events.Add($"P:{paint.X}:{paint.Y}:{paint.Owner}:{paint.Theme}");
                    ++paints;
                    if (patch && Number(row[5]) != 0 && paints == 2)
                    {
                        paintState.Affected = Affected(variant);
                    }
                },
                ThemeMode = () =>
                {
                    events.Add("C:" + row[4]);
                    return Number(row[4]);
                },
            });
            Func<float, float, uint> actualRegionAt = hooks.RegionAt!;
            hooks.RegionAt = (x, y) =>
            {
                events.Add($"R:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}");
                return actualRegionAt(x, y);
            };
            var ownership = new RawRegionOwnership(pool, additional, workshops, themesTable, paintState, hooks);
            // 테마 표를 실제 목록 항목과 같은 값으로 채운다.
            for (int i = 0; i < Coords.Length; ++i)
            {
                themesTable[83 + i] = Kinds[(i + variant) % Kinds.Length];
            }

            ownership.Update(BitConverter.UInt32BitsToSingle(Number(row[6])), BitConverter.UInt32BitsToSingle(Number(row[7])), Number(row[3]));
            var regions = new List<string>();
            if (patch)
            {
                // 누적 지역을 순서대로 옮기는 반복문이다.
                foreach (OwnershipAffectedRegion value in paintState.Affected)
                {
                    regions.Add(value.Region.ToString(CultureInfo.InvariantCulture));
                }
            }

            string regionsText = regions.Count == 0 ? "-" : string.Join(",", regions);
            string eventsText = events.Count == 0 ? "-" : string.Join(";", events);
            bool same = eventsText == row[9] && regionsText == row[10]
                && paintState.Revision == Number(row[11]) && paintState.Dirty == Number(row[12])
                && additionalEntries.AsSpan().SequenceEqual(beforeAdditional)
                && workshopEntries.AsSpan().SequenceEqual(beforeWorkshops);
            Assert.True(same, $"지역 소유 {edition} 행 {count} 불일치: {eventsText} / {row[9]}");
            ++count;
        }

        Assert.Equal(Rows, count);
    }

    /// <summary>패치의 두 도장과 테마·재귀·DWORD 감김을 전체 원본 관찰에 대조한다.</summary>
    [Fact]
    public void ReplaysOriginals()
    {
        Replay("originals");
    }

    /// <summary>CD의 단일 도장과 테마·누적 지역이 없는 동작을 별도로 대조한다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        Replay("originalCD");
    }

    /// <summary>추가 10.37 PE의 독립 원본 관찰도 같은 구현으로 대조한다.</summary>
    [Fact]
    public void ReplaysExtra1037()
    {
        Replay("original1037");
    }

    /// <summary>부정확한 연결·좌표를 묵시적인 기본 판정으로 바꾸지 않는다.</summary>
    [Fact]
    public void RejectsMissingBoundariesAndForeignPools()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var foreign = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var additional = new RawPostPopList { Entries = [50u], Count = 1u };
        var workshops = new RawPostPopList();
        var themes = new uint[188];
        var state = new RegionOwnershipState { Revision = 5u, Dirty = 9u };
        state.Affected.Add(new OwnershipAffectedRegion(1f, 2f, 7u));
        var hooks = new RegionOwnershipHooks
        {
            RegionAt = (_, _) => 7u,
            Paint = _ => { },
            ThemeMode = () => 1u,
        };
        var missing = new RegionOwnershipHooks
        {
            RegionAt = hooks.RegionAt,
            Paint = null,
            ThemeMode = hooks.ThemeMode,
        };
        Assert.Throws<ArgumentException>(() => new RawRegionOwnership(pool, additional, workshops, themes, state, missing));
        missing = new RegionOwnershipHooks
        {
            RegionAt = hooks.RegionAt,
            Paint = hooks.Paint,
            ThemeMode = null,
        };
        Assert.Throws<ArgumentException>(() => new RawRegionOwnership(pool, additional, workshops, themes, state, missing));
        Assert.Throws<ArgumentException>(() => new RawRegionOwnership(pool, additional, additional, themes, state, hooks));
        var ownership = new RawRegionOwnership(pool, additional, workshops, themes, state, hooks);
        Assert.Throws<ArgumentOutOfRangeException>(() => ownership.Update(float.NaN, 1f));
        uint paints = 0;
        var counting = new RegionOwnershipHooks
        {
            RegionAt = hooks.RegionAt,
            Paint = _ => ++paints,
            ThemeMode = hooks.ThemeMode,
        };
        var countingOwnership = new RawRegionOwnership(pool, additional, workshops, themes, state, counting);
        pool.RawSlot(50).Fill(0);
        pool.RawSlot(50)[34] = 9;
        Assert.Throws<ArgumentOutOfRangeException>(() => countingOwnership.Update(1f, 2f));
        Assert.Equal(0u, paints);
        additional.Count = 2u;
        Assert.Throws<InvalidOperationException>(() => countingOwnership.Update(1f, 2f));
        Assert.Equal(0u, paints);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        var plain = new PriestPlainCanonType[types.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다.
        for (int i = 0; i < plain.Length; ++i)
        {
            plain[i] = new PriestPlainCanonType();
        }

        var islands = new ushort[65536];
        var terrain = new RawCanonPlacementTerrain(pool, types, plain, islands, new CanonPlacementTerrainState(), new CanonPlacementTerrainHooks
        {
            CandidatePermission = (_, _) => false,
            ShapePermission = (_, _) => false,
            FinishRelations = (_, _) => false,
        });
        Assert.Throws<ArgumentException>(() => RawRegionOwnership.MakeTerrainRegionOwnershipHooks(foreign, terrain, hooks));
    }

    /// <summary>원문 0059a8b0의 증가와 005949d4의 1쓰기를 같은 Client의 서로 다른 현재 owner에 적용한다.</summary>
    [Fact]
    public void SharedWorldRedrawAndMapRequestRemainDistinct()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        int outpost = pool.Allocate(SidAllocation.Client);
        Span<byte> raw = pool.AllocatedBytes(outpost);
        raw.Fill(0);
        raw[10] = 83;
        raw[34] = 2;
        PutFloat(raw, 14, 10.2F);
        PutFloat(raw, 18, 20.3F);
        var additional = new RawPostPopList { Entries = [(uint)outpost], Count = 1u };
        var workshops = new RawPostPopList();
        var themes = new uint[188];
        var redraw = new UIntBox { Value = 17u };
        var mapRevision = new UIntBox { Value = 0xffffffffu };
        var state = new RegionOwnershipState { Revision = 0x12345678u, Dirty = 0x87654321u, SharedWorldRedraw = redraw, SharedMapRevision = mapRevision };
        var paints = new List<RegionOwnershipPaint>();
        bool mutate = false;
        var hooks = new RegionOwnershipHooks
        {
            RegionAt = (_, _) => 7u,
            ThemeMode = () => 1u,
            Paint = paint =>
            {
                paints.Add(paint);
                if (mutate && paints.Count == 2)
                {
                    redraw.Value = 37u;
                    mapRevision.Value = 0xffffffffu;
                }
            },
        };
        var ownership = new RawRegionOwnership(pool, additional, workshops, themes, state, hooks);
        ownership.Update(10.2F, 20.3F);
        Assert.Equal(2, paints.Count);
        Assert.Equal(0u, paints[0].Owner);
        Assert.Equal(2u, paints[1].Owner);
        Assert.Equal(10, paints[1].X);
        Assert.Equal(20, paints[1].Y);
        Assert.Equal(18u, redraw.Value);
        Assert.Equal(1u, mapRevision.Value);
        Assert.Equal(0x12345678u, state.Revision);
        Assert.Equal(0x87654321u, state.Dirty);
        paints.Clear();
        mutate = true;
        mapRevision.Value = 19u;
        ownership.Update(10.2F, 20.3F);
        Assert.Equal(2, paints.Count);
        Assert.Equal(38u, redraw.Value);
        Assert.Equal(1u, mapRevision.Value);
        Assert.Equal(0x12345678u, state.Revision);
        Assert.Equal(0x87654321u, state.Dirty);
    }
}
