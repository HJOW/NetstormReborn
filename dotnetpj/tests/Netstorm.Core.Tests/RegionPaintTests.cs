// 두 원본 PE의 지역 도장 출력과 같은 raw 슬롯·지형 getter·게임 난수를 전수 재생한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawRegionPaint의 도장·소유자·경계·난수를 검증하는 테스트 클래스다. 2026-11-18 0.618에서 R4-2 지역 칠하기 결선용으로 추가했다.
// 근거: cpppj RegionPaintTests.cpp와 regionpaint-x86.tsv(원본 386행·CD 98행)다.
public sealed class RegionPaintTests
{
    /// <summary>원본 실행기에 제공한 실제 지면 타입 번호다. 2026-11-18 0.618에서 추가했다.</summary>
    private const uint Terrain = 83u;

    /// <summary>원본 실행기에 제공한 실제 테두리 타입 번호다. 2026-11-18 0.618에서 추가했다.</summary>
    private const uint Edge = 84u;

    /// <summary>원본 실행기에 제공한 실제 기타 타입 번호다. 2026-11-18 0.618에서 추가했다.</summary>
    private const uint Other = 85u;

    /// <summary>지형 여섯 칸의 고정 좌표다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly (int X, int Y)[] Points = [(10, 10), (11, 10), (10, 11), (11, 11), (12, 10), (12, 11)];

    /// <summary>장면별 지형 칸 선택이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly int[][] Scenes = [[], [0], [0, 1, 4], [0, 1, 2, 3], [0, 1, 2, 3, 4, 5], [0, 1, 2, 3, 4, 5]];

    /// <summary>장면별 초기 프레임이다. 2026-11-18 0.618에서 추가했다.</summary>
    private static readonly uint[] InitialFrames = [0, 145, 3];

    /// <summary>원본 입력 프레임은 AA와 144개 JA 구간 및 비연결 PP다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <returns>프레임 코드 목록이다.</returns>
    private static List<RiftFrameCode> Codes()
    {
        var codes = new List<RiftFrameCode> { new((byte)'A', (byte)'A', 0, 0) };
        // J 시작 번호 1과 원소별 36프레임 구간을 입력한다.
        for (int i = 0; i < 144; ++i)
        {
            codes.Add(new RiftFrameCode((byte)'J', (byte)'A', unchecked((byte)i), 0));
        }

        // 비연결 PP도 유효한 네 quartile의 12개 후보를 공급한다.
        for (int i = 0; i < 12; ++i)
        {
            codes.Add(new RiftFrameCode((byte)'P', (byte)'P', unchecked((byte)i), 0));
        }

        return codes;
    }

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="raw">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>원본으로 기록한 모든 입력을 같은 판본의 실제 모듈에 재생한다. 2026-11-18 0.618에서 추가했다.</summary>
    /// <param name="edition">판본 이름이다. originals 또는 originalCD 중 하나다.</param>
    private static void Replay(string edition)
    {
        bool patch = edition == "originals";
        OriginalEdition version = patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "regionpaint-x86.tsv");
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

        int count = 0;
        // 행마다 새 풀·지도·난수·상태로 실행하며 이전 행이 섞이지 않게 하는 반복문이다.
        foreach (string[] row in fixture)
        {
            if (row[0] != edition)
            {
                continue;
            }

            Assert.Equal(14, row.Length);
            uint scene = Number(row[1]);
            uint variant = Number(row[2]);
            var pool = new RawSidPool(24000, false, null, version);
            var types = new OriginalTypeRecord[patch ? 188 : 171];
            // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
            for (int i = 0; i < types.Length; ++i)
            {
                types[i] = new OriginalTypeRecord();
            }

            var frames = new RiftTypeFrames[types.Length];
            List<RiftFrameCode> codes = Codes();
            // 모든 타입 번호마다 같은 원본 프레임 입력을 두는 반복문이다.
            for (int i = 0; i < frames.Length; ++i)
            {
                frames[i] = new RiftTypeFrames(codes);
            }

            types[(int)Terrain].Flags2 = 2u;
            var islands = new ushort[65536];
            int ownerOffset = patch ? 34 : 32;
            int frameOffset = patch ? 36 : 34;
            int frameWidth = patch ? 4 : 1;
            // 출력에서 읽을 슬롯을 원본 합성 pool처럼 0으로 초기화하는 반복문이다.
            foreach (int number in new[] { 50, 51, 52, 53, 54, 55, 70, 71 })
            {
                pool.RawSlot(number).Fill(0);
            }

            // 선행 소유자·장벽·잘못된 시작 타입을 입력으로만 작성한다.
            foreach (int index in Scenes[scene])
            {
                (int px, int py) = Points[index];
                int sid = 50 + index;
                Span<byte> raw = pool.RawSlot(sid);
                raw[10] = (byte)(scene == 5 && index == 0 ? Other : Terrain);
                raw[ownerOffset] = (byte)((index % 3 + variant) % 3);
                Put(raw, 8, (uint)(px < 12 ? 7 : 9), 2);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(px + 0.125F), 4);
                Put(raw, 18, BitConverter.SingleToUInt32Bits(py + 0.25F), 4);
                Put(raw, frameOffset, InitialFrames[variant], frameWidth);
                islands[py * 256 + px] = (ushort)sid;
            }

            var fallback = new Dictionary<(int, int), int> { [(9, 10)] = 70, [(12, 12)] = 71 };
            // edgeFarm은 표면 지도 밖 일반 finder 후보로만 공급한다.
            foreach (var pair in fallback)
            {
                Span<byte> raw = pool.RawSlot(pair.Value);
                raw.Fill(0);
                raw[10] = (byte)Edge;
                raw[ownerOffset] = 2;
                Put(raw, 14, BitConverter.SingleToUInt32Bits(pair.Key.Item1 + 0.125F), 4);
                Put(raw, 18, BitConverter.SingleToUInt32Bits(pair.Key.Item2 + 0.25F), 4);
            }

            var plain = new PriestPlainCanonType[types.Length];
            // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다.
            for (int i = 0; i < plain.Length; ++i)
            {
                plain[i] = new PriestPlainCanonType();
            }

            var terrain = new RawCanonPlacementTerrain(pool, types, plain, islands, new CanonPlacementTerrainState(), new CanonPlacementTerrainHooks
            {
                CandidatePermission = (_, _) => throw new InvalidOperationException("도장 재생에 배치 권한 호출"),
                ShapePermission = (_, _) => throw new InvalidOperationException("도장 재생에 주변 배치 호출"),
                FinishRelations = (_, _) => throw new InvalidOperationException("도장 재생에 최종 배치 호출"),
            });
            var random = new GameRandom(0x23456789u);
            var state = new RegionPaintState { Owner = 6u, Theme = 3, Revision = 0xffffffffu };
            var ownership = new RegionOwnershipState();
            ownership.Affected.Add(new OwnershipAffectedRegion(1.0F, 2.0F, 7u));
            var events = new List<string>();
            var hooks = new RegionPaintHooks
            {
                SetOwner = (sid, owner) =>
                {
                    events.Add($"O:{sid}:{owner}");
                    pool.RawSlot(sid)[ownerOffset] = (byte)owner;
                },
                BoundaryAt = (x, y, type) =>
                {
                    if (type != Edge)
                    {
                        throw new InvalidOperationException("도장 재생에 edgeFarm 일반 finder 타입 오류");
                    }

                    var key = ((int)x, (int)y);
                    int sid = fallback.TryGetValue(key, out int found) ? found : 0;
                    events.Add($"F:{(int)x}:{(int)y}:{sid}");
                    return sid;
                },
                RegionAt = (x, y) => terrain.RegionAt((int)x, (int)y),
            };
            var paint = new RawRegionPaint(pool, frames, islands, Terrain, Edge, random, state, ownership, hooks);
            paint.Paint(new RegionOwnershipPaint(int.Parse(row[5], CultureInfo.InvariantCulture), int.Parse(row[6], CultureInfo.InvariantCulture), Number(row[3]), Number(row[4])));
            var objects = new List<string>();
            // 슬롯 출력을 원본 실행기가 기록한 동일 폭으로 읽는 반복문이다.
            foreach (int number in new[] { 50, 51, 52, 53, 54, 55, 70, 71 })
            {
                ReadOnlySpan<byte> raw = pool.Slot(number);
                objects.Add($"{number}:{raw[ownerOffset]}:{(frameWidth == 4 ? BitConverter.ToUInt32(raw.Slice(frameOffset, 4)) : raw[frameOffset])}");
            }

            var boundary = new List<string>();
            // 후보 삽입 순서를 그대로 옮기는 반복문이다.
            foreach (CellPoint point in state.Boundary)
            {
                boundary.Add($"{point.X}:{point.Y}");
            }

            var affected = new List<string>();
            if (patch)
            {
                // 전체 영향 기록 비트를 그대로 옮기는 반복문이다.
                foreach (OwnershipAffectedRegion value in ownership.Affected)
                {
                    affected.Add($"{BitConverter.SingleToUInt32Bits(value.X)}:{BitConverter.SingleToUInt32Bits(value.Y)}:{value.Region}");
                }
            }

            var drawer = new List<string>();
            // 생성자의 float 비트와 마지막 유효 요청을 대조한다.
            foreach (float value in state.Bounds)
            {
                drawer.Add(BitConverter.SingleToUInt32Bits(value).ToString(CultureInfo.InvariantCulture));
            }

            drawer.Add(state.Owner.ToString(CultureInfo.InvariantCulture));
            drawer.Add(unchecked((uint)state.Theme).ToString(CultureInfo.InvariantCulture));
            string eventsText = events.Count == 0 ? "-" : string.Join(";", events);
            string objectsText = string.Join(",", objects);
            string boundaryText = boundary.Count == 0 ? "-" : string.Join(",", boundary);
            string affectedText = affected.Count == 0 ? "-" : string.Join(",", affected);
            string drawerText = string.Join(":", drawer);
            bool same = eventsText == row[7] && objectsText == row[8]
                && boundaryText == row[9] && affectedText == row[10] && drawerText == row[11]
                && state.Revision == Number(row[12]) && (patch ? random.State() : 0u) == Number(row[13]);
            Assert.True(same, $"도장 {edition} 행 {count} scene {scene} variant {variant} 불일치: {eventsText} / {row[7]}, {objectsText} / {row[8]}");
            ++count;
        }

        Assert.Equal(patch ? 386 : 98, count);
    }

    /// <summary>패치의 실제 frame/경계 확장/고유 지역 기록과 두 난수 경로를 대조한다.</summary>
    [Fact]
    public void ReplaysOriginals()
    {
        Replay("originals");
    }

    /// <summary>CD의 프레임이나 후보를 바꾸지 않고 소유자 flood와 revision만 수행한다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        Replay("originalCD");
    }

    /// <summary>실제 지면 flood 효과 뒤 월드 DWORD만 증가시키며 미니맵 요청과 지역 fallback은 건드리지 않는다.</summary>
    [Fact]
    public void SharedWorldRedrawRechecksOwnerAfterFlood()
    {
        foreach (OriginalEdition version in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            bool patch = version == OriginalEdition.Patch1078;
            var pool = new RawSidPool(24000, false, null, version);
            int surface = pool.Allocate(SidAllocation.Client);
            Span<byte> raw = pool.AllocatedBytes(surface);
            raw.Clear();
            raw[10] = (byte)Terrain;
            raw[patch ? 34 : 32] = 2;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(10.125F), 4);
            Put(raw, 18, BitConverter.SingleToUInt32Bits(10.25F), 4);
            Put(raw, patch ? 36 : 34, 0u, patch ? 4 : 1);
            var types = new OriginalTypeRecord[patch ? 188 : 171];
            // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
            for (int i = 0; i < types.Length; ++i)
            {
                types[i] = new OriginalTypeRecord();
            }

            var frames = new RiftTypeFrames[types.Length];
            List<RiftFrameCode> codes = Codes();
            // 모든 타입 번호마다 같은 원본 프레임 입력을 두는 반복문이다.
            for (int i = 0; i < frames.Length; ++i)
            {
                frames[i] = new RiftTypeFrames(codes);
            }

            var islands = new ushort[65536];
            islands[10 * 256 + 10] = (ushort)surface;
            var random = new GameRandom(0x23456789u);
            var redraw = new UIntBox { Value = 17u };
            var mapRevision = new UIntBox { Value = 0xffffffffu };
            var state = new RegionPaintState { Revision = 0x76543210u, SharedWorldRedraw = redraw };
            var ownership = new RegionOwnershipState();
            uint assignments = 0;
            bool mutate = false;
            int ownerOffset = patch ? 34 : 32;
            var hooks = new RegionPaintHooks
            {
                SetOwner = (sid, owner) =>
                {
                    ++assignments;
                    pool.RawSlot(sid)[ownerOffset] = (byte)owner;
                    if (mutate)
                    {
                        redraw.Value = 37u;
                    }
                },
                BoundaryAt = (_, _, _) => 0,
                RegionAt = (_, _) => 7u,
            };
            var paint = new RawRegionPaint(pool, frames, islands, Terrain, Edge, random, state, ownership, hooks);
            paint.Paint(new RegionOwnershipPaint(10, 10, 3u, 0u));
            Assert.Equal(1u, assignments);
            Assert.Equal(3, pool.Slot(surface)[ownerOffset]);
            Assert.Equal(18u, redraw.Value);
            Assert.Equal(0xffffffffu, mapRevision.Value);
            Assert.Equal(0x76543210u, state.Revision);
            mutate = true;
            paint.Paint(new RegionOwnershipPaint(10, 10, 4u, 0u));
            Assert.Equal(2u, assignments);
            Assert.Equal(4, pool.Slot(surface)[ownerOffset]);
            Assert.Equal(38u, redraw.Value);
            Assert.Equal(0xffffffffu, mapRevision.Value);
            Assert.Equal(0x76543210u, state.Revision);
            redraw.Value = 0xffffffffu;
            paint.Paint(new RegionOwnershipPaint(0, 0, 1u, 0u));
            Assert.Equal(2u, assignments);
            Assert.Equal(0u, redraw.Value);
            Assert.Equal(0xffffffffu, mapRevision.Value);
            Assert.Equal(0x76543210u, state.Revision);
        }
    }
}
