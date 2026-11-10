// 섬 받침 사전 파괴와 표면 사후 파괴의 경계·분배를 직접 검사한다.
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawIslandLifecycle의 훅 검증·추상 건너뜀·걷는 유닛 분배를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandLifecycleTests
{
    /// <summary>검사 풀 용량이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Capacity = 24000;

    /// <summary>받침 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Island = 50;

    /// <summary>표면 칸 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Surface = 50;

    /// <summary>걷는 유닛 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Walker = 60;

    /// <summary>걷지 않는 이웃 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Other = 61;

    /// <summary>걷는 유닛 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int WalkerType = 70;

    /// <summary>일반 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int PlainType = 71;

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>풀·해시·타입·프레임의 빈 장면을 만든다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <returns>풀·해시·점유·타입·프레임 묶음이다.</returns>
    private static (RawSidPool Pool, RawSquidHash Hash, byte[] Spots, OriginalTypeRecord[] Types, List<IReadOnlyList<FrameCode>> Frames) Scene()
    {
        RawSidPool pool = new(Capacity, false);
        RawSquidHash hash = new();
        byte[] spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
        OriginalTypeRecord[] types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호에 빈 타입 레코드를 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        List<IReadOnlyList<FrameCode>> frames = [];
        // 모든 타입 번호마다 빈 프레임 목록을 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            frames.Add([]);
        }

        return (pool, hash, spots, types, frames);
    }

    /// <summary>훅 누락과 다른 풀의 탐색기는 생성 때 거부한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsMissingHooksAndForeignPool()
    {
        var (pool, hash, spots, types, frames) = Scene();
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        IslandLifecycleHooks hooks = new()
        {
            ScheduleFall = (sid, x, y) => { },
            FallWalker = sid => { },
            BasePreDestroy = (sid, flags) => { },
            BasePostDestroy = (sid, flags) => { },
        };
        IslandLifecycleHooks broken = new()
        {
            ScheduleFall = null,
            FallWalker = hooks.FallWalker,
            BasePreDestroy = hooks.BasePreDestroy,
            BasePostDestroy = hooks.BasePostDestroy,
        };
        Assert.Throws<ArgumentException>(() => new RawIslandLifecycle(pool, neighbors, broken));
        RawSidPool other = new(Capacity, false);
        Assert.Throws<ArgumentException>(() => new RawIslandLifecycle(other, neighbors, hooks));
    }

    /// <summary>추상·매립 받침은 이웃을 읽지 않고 공통 사전 파괴만 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void AbstractIslandSkipsWalkAndCallsBaseOnly()
    {
        var (pool, hash, spots, types, frames) = Scene();
        pool.RawSlot(Island)[40] = 9;
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        List<string> events = [];
        RawIslandLifecycle lifecycle = new(pool, neighbors, new IslandLifecycleHooks
        {
            ScheduleFall = (sid, x, y) => events.Add("fall"),
            FallWalker = sid => events.Add("walker"),
            BasePreDestroy = (sid, flags) => events.Add("pre:" + sid + ":" + flags),
            BasePostDestroy = (sid, flags) => events.Add("post"),
        });
        lifecycle.IslandPreDestroy(Island, 0u);
        Assert.Equal(["pre:50:0"], events);
    }

    /// <summary>표면 칸의 걷는 유닛에만 가상 낙하를 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SurfacePostDestroyFallsWalkersOnly()
    {
        var (pool, hash, spots, types, frames) = Scene();
        types[WalkerType].Flags2 = TypeFlag2.Walker;
        types[WalkerType].FootX = 1;
        types[WalkerType].FootY = 1;
        types[PlainType].Flags2 = 0u;
        types[PlainType].FootX = 1;
        types[PlainType].FootY = 1;
        Span<byte> surface = pool.RawSlot(Surface);
        surface.Clear();
        surface[RawSidPool.TypeOffset] = (byte)PlainType;
        Put(surface, 14, BitConverter.SingleToUInt32Bits(20.75f), 4);
        Put(surface, 18, BitConverter.SingleToUInt32Bits(21.9f), 4);
        Span<byte> walker = pool.RawSlot(Walker);
        walker.Clear();
        walker[RawSidPool.TypeOffset] = (byte)WalkerType;
        Put(walker, 14, BitConverter.SingleToUInt32Bits(20.5f), 4);
        Put(walker, 18, BitConverter.SingleToUInt32Bits(21.5f), 4);
        Span<byte> other = pool.RawSlot(Other);
        other.Clear();
        other[RawSidPool.TypeOffset] = (byte)PlainType;
        Put(other, 14, BitConverter.SingleToUInt32Bits(20.6f), 4);
        Put(other, 18, BitConverter.SingleToUInt32Bits(21.4f), 4);
        Put(walker, 4, (uint)Other, 2);
        Put(other, 4, 0u, 2);
        hash.Cell(1, 10, 10) = (ushort)Walker;
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        List<string> events = [];
        RawIslandLifecycle lifecycle = new(pool, neighbors, new IslandLifecycleHooks
        {
            ScheduleFall = (sid, x, y) => events.Add("fall"),
            FallWalker = sid => events.Add("walker:" + sid),
            BasePreDestroy = (sid, flags) => events.Add("pre"),
            BasePostDestroy = (sid, flags) => events.Add("post:" + sid + ":" + flags),
        });
        lifecycle.SurfacePostDestroy(Surface, 0u);
        Assert.Equal(["walker:60", "post:50:0"], events);
    }

    /// <summary>추상·매립 표면은 칸 탐색을 건너뛰고 공통 사후 파괴만 부른다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void AbstractSurfaceSkipsScanAndCallsBaseOnly()
    {
        var (pool, hash, spots, types, frames) = Scene();
        Span<byte> surface = pool.RawSlot(Surface);
        surface.Clear();
        surface[40] = 9;
        RawSquidNeighbors neighbors = new(pool, hash, spots, types, frames);
        List<string> events = [];
        RawIslandLifecycle lifecycle = new(pool, neighbors, new IslandLifecycleHooks
        {
            ScheduleFall = (sid, x, y) => events.Add("fall"),
            FallWalker = sid => events.Add("walker"),
            BasePreDestroy = (sid, flags) => events.Add("pre"),
            BasePostDestroy = (sid, flags) => events.Add("post:" + sid + ":" + flags),
        });
        lifecycle.SurfacePostDestroy(Surface, 7u);
        Assert.Equal(["post:50:7"], events);
    }

    /// <summary>실행기 풀의 마지막 번호다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const int LastSid = 127;

    /// <summary>기계어 실행기의 가상 테이블 값이다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const uint Vtable = 0x11010000u;

    /// <summary>입력된 합성 코드 표를 만든다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    /// <param name="variant">변형 번호다.</param>
    /// <returns>16개 코드 목록이다.</returns>
    private static List<FrameCode> Frames(int variant)
    {
        const string letters = "JKALMNOJKLBJPPCD";
        List<FrameCode> codes = [];
        // 변형별 경화 비트는 원본 도구와 같은 입력인 반복문이다.
        for (int i = 0; i < letters.Length; ++i)
        {
            bool hard = variant == 1 ? i == 0 || i == 1 || i == 7 || i == 8 || i == 11 : variant == 2 && (i == 0 || i == 8);
            FrameCodeFlags flags = (FrameCodeFlags)(hard ? 0x40 : letters[i] == 'L' || letters[i] == 'M' ? 0x20 : 0);
            codes.Add(new FrameCode(letters[i], 'P', i + 1, flags));
        }

        return codes;
    }

    /// <summary>같은 풀·해시·이웃으로 장면 입력과 결과를 대조하는 전수 재생 장면이다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private sealed class LifecycleScene
    {
        /// <summary>객체 슬롯 풀이다.</summary>
        public RawSidPool Pool;

        /// <summary>좌표별 객체 후보를 찾는 공간 해시다.</summary>
        public RawSquidHash Hash = new();

        /// <summary>칸별 점유·통행 비트를 읽는 지도다.</summary>
        public byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 번호로 속성·발자국을 조회하는 타입 표다.</summary>
        public OriginalTypeRecord[] Types;

        /// <summary>타입별 프레임 자료다.</summary>
        public List<IReadOnlyList<FrameCode>> Frames = [];

        /// <summary>이웃 탐색기다.</summary>
        public RawSquidNeighbors? Neighbors;

        /// <summary>발생한 사건이다.</summary>
        public List<string> Events = [];

        /// <summary>패치판이면 참이다.</summary>
        public bool Patch;

        /// <summary>실행기와 같은 풀 번호를 확보한다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        public LifecycleScene(OriginalEdition edition)
        {
            Patch = edition == OriginalEdition.Patch1078;
            Pool = new RawSidPool(Capacity, false, null, edition);
            // 예측·서버 목록과 무관한 번호를 입력 슬롯으로 확보하는 반복문이다.
            for (int sid = 5; sid <= LastSid; ++sid)
            {
                Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            }

            Types = new OriginalTypeRecord[Patch ? 188 : 171];
            // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
            for (int i = 0; i < Types.Length; ++i)
            {
                Types[i] = new OriginalTypeRecord();
                Frames.Add([]);
            }
        }

        /// <summary>판본별 소유자 위치다.</summary>
        public int OwnerOffset => Patch ? 34 : 32;

        /// <summary>판본별 추가 비트 위치다.</summary>
        public int ExtraOffset => Patch ? 40 : 35;

        /// <summary>판본별 프레임 위치다.</summary>
        public int FrameOffset => Patch ? 36 : 34;

        /// <summary>판본별 프레임 폭이다.</summary>
        public int FrameWidth => Patch ? 4 : 1;

        /// <summary>장면 입력 칸으로 슬롯·타입·체인·전역을 준비한다.</summary>
        /// <param name="columns">객체·점유·변형·소유자·전역 5칸이다.</param>
        public void Prepare(string[] columns)
        {
            Assert.Equal(5, columns.Length);
            int variant = int.Parse(columns[2], CultureInfo.InvariantCulture);
            Hash.Reset();
            Array.Clear(Spots);
            Events.Clear();
            // 앞선 행의 슬롯이 남지 않도록 풀 전체를 지우는 반복문이다.
            for (int sid = 5; sid <= LastSid; ++sid)
            {
                Pool.RawSlot(sid).Clear();
            }

            (uint Number, uint Flags1, uint Flags2, int FootX, int FootY)[] inputs =
            [
                (82, 0x00000802u, 0x00000004u, 1, 1), (83, 0x28000803u, 0x00000002u, 1, 1),
                (84, 0x08000803u, 0x01000002u, 3, 3), (85, 0x00069012u, 0x00210000u, 1, 1),
                (90, 0x28000002u, 0x02000000u, 1, 1), (91, 0x28000003u, 0x01000000u, 3, 3),
                (92, 0x28000002u, 0u, 3, 3), (93, 0x08000803u, 0x01000002u, 1, 1),
            ];
            // 객체가 없는 타입도 조회 대상이므로 합성 타입 표 전체를 넣는 반복문이다.
            foreach (var input in inputs)
            {
                OriginalTypeRecord type = Types[(int)input.Number];
                type.Flags1 = input.Flags1;
                type.Flags2 = input.Flags2;
                type.FootX = input.FootX;
                type.FootY = input.FootY;
                Frames[(int)input.Number] = Frames(variant);
            }

            // 각 기록의 12개 정수 필드를 읽는 반복문이다.
            foreach (string entry in columns[0].Split(';'))
            {
                string[] n = entry.Split(',');
                Assert.Equal(12, n.Length);
                int sid = int.Parse(n[0], CultureInfo.InvariantCulture);
                uint type = uint.Parse(n[1], CultureInfo.InvariantCulture);
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw.Clear();
                Put(raw, 0, Vtable, 4);
                raw[10] = (byte)type;
                raw[11] = (byte)uint.Parse(n[2], CultureInfo.InvariantCulture);
                raw[ExtraOffset] = (byte)uint.Parse(n[3], CultureInfo.InvariantCulture);
                Put(raw, 14, uint.Parse(n[4], CultureInfo.InvariantCulture), 4);
                Put(raw, 18, uint.Parse(n[5], CultureInfo.InvariantCulture), 4);
                Put(raw, FrameOffset, uint.Parse(n[10], CultureInfo.InvariantCulture), FrameWidth);
                Types[(int)type].FootX = int.Parse(n[6], CultureInfo.InvariantCulture);
                Types[(int)type].FootY = int.Parse(n[7], CultureInfo.InvariantCulture);
                Types[(int)type].Flags1 = uint.Parse(n[8], CultureInfo.InvariantCulture);
                Types[(int)type].Flags2 = uint.Parse(n[9], CultureInfo.InvariantCulture);
                Frames[(int)type] = Frames(variant);
                float x = BitConverter.UInt32BitsToSingle(uint.Parse(n[4], CultureInfo.InvariantCulture));
                float y = BitConverter.UInt32BitsToSingle(uint.Parse(n[5], CultureInfo.InvariantCulture));
                ref ushort head = ref Hash.Bucket(int.Parse(n[11], CultureInfo.InvariantCulture), x, y);
                Put(raw, 4, head, 2);
                head = (ushort)sid;
            }

            Put(Pool.AllocatedBytes(Surface), 8, 0x2a2bu, 2);
            Put(Pool.AllocatedBytes(Surface), 12, 0x1234u, 2);
            if (columns[1] != "-")
            {
                // 점유 입력을 읽는 반복문이다.
                foreach (string entry in columns[1].Split(';'))
                {
                    string[] n = entry.Split(',');
                    Spots[int.Parse(n[1], CultureInfo.InvariantCulture) * 256 + int.Parse(n[0], CultureInfo.InvariantCulture)] = (byte)uint.Parse(n[2], CultureInfo.InvariantCulture);
                }
            }

            if (columns[3] != "-")
            {
                // 소유자 입력을 읽는 반복문이다.
                foreach (string entry in columns[3].Split(';'))
                {
                    string[] n = entry.Split(',');
                    Pool.AllocatedBytes(int.Parse(n[0], CultureInfo.InvariantCulture))[OwnerOffset] = (byte)uint.Parse(n[1], CultureInfo.InvariantCulture);
                }
            }

            Neighbors = new RawSquidNeighbors(Pool, Hash, Spots, Types, Frames);
        }

        /// <summary>모든 원본 효과의 순서와 인자를 한 문자열로 만든다.</summary>
        /// <returns>사건 문자열이다. 없으면 "-"다.</returns>
        public string EventsText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>한 판본의 수명주기 관찰 행을 모두 재생한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    /// <param name="edition">판본 이름이다.</param>
    private static void Replay(string edition)
    {
        OriginalEdition version = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "islandlifecycle-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        Dictionary<string, string[]> scenes = new();
        List<string[]> rows = [];
        // 장면 행은 번호로 따로 두고 관찰 행을 모으는 반복문이다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            if (row[0] == "Scene")
            {
                scenes[row[1]] = [row[2], row[3], row[4], row[5], row[6]];
            }
            else
            {
                rows.Add(row);
            }
        }

        Dictionary<string, int> counts = new();
        List<string> failures = [];
        // 각 행을 앞 행과 독립된 입력부터 다시 준비해 실행하는 반복문이다.
        foreach (string[] row in rows)
        {
            if (row[1] != edition)
            {
                continue;
            }

            string kind = row[0];
            string actual;
            bool unchanged = false;
            try
            {
                LifecycleScene scene = new(version);
                scene.Prepare(scenes[row[2]]);
                byte[] before = scene.Pool.Bytes.ToArray();
                RawIslandLifecycle lifecycle = new(scene.Pool, scene.Neighbors!, Hooks(scene));
                if (kind == "IslandPre")
                {
                    lifecycle.IslandPreDestroy(Surface, uint.Parse(row[3], CultureInfo.InvariantCulture));
                }
                else if (kind == "SurfacePost")
                {
                    lifecycle.SurfacePostDestroy(Surface, uint.Parse(row[3], CultureInfo.InvariantCulture));
                }
                else
                {
                    throw new InvalidOperationException("알 수 없는 행 종류");
                }

                actual = scene.EventsText();
                unchanged = before.AsSpan().SequenceEqual(scene.Pool.Bytes.ToArray());
            }
            catch (Exception error)
            {
                actual = "예외: " + error.Message;
            }

            // 두 재정의는 풀을 바꾸지 않아야 한다.
            if (actual != row[5] || !unchanged)
            {
                if (failures.Count < 6)
                {
                    failures.Add($"{edition} {kind} 장면 {row[2]}: 실제 {actual} / 기대 {row[5]} (풀 변경 {(!unchanged)})");
                }
            }

            counts[kind] = counts.TryGetValue(kind, out int had) ? had + 1 : 1;
        }

        Assert.True(failures.Count == 0, string.Join('\n', failures));
        Assert.Equal(260, counts.GetValueOrDefault("IslandPre", 0));
        Assert.Equal(260, counts.GetValueOrDefault("SurfacePost", 0));
    }

    /// <summary>원본 실행기의 대체와 같은 경계다. 호출 사실과 인자만 기록한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    /// <param name="scene">기록할 장면이다.</param>
    private static IslandLifecycleHooks Hooks(LifecycleScene scene) => new()
    {
        ScheduleFall = (bridge, x, y) => scene.Events.Add("G:" + bridge + ":" + BitConverter.SingleToUInt32Bits(x) + ":" + BitConverter.SingleToUInt32Bits(y)),
        FallWalker = walker => scene.Events.Add("W:" + walker),
        BasePreDestroy = (sid, flags) => scene.Events.Add("X:" + sid + ":" + flags),
        BasePostDestroy = (sid, flags) => scene.Events.Add("Y:" + sid + ":" + flags),
    };

    /// <summary>패치판의 받침·표면 파괴 관찰을 대조한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    [Fact]
    public void FullPatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD판의 받침·표면 파괴 관찰을 대조한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    [Fact]
    public void FullCdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본의 받침·표면 파괴 관찰을 대조한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    [Fact]
    public void FullThirdX86()
    {
        Replay("original1037");
    }
}
