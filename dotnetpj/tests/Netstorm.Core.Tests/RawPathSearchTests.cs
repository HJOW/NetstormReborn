using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 독립 PE의 전체 거리표/큐와 경로 꼭짓점을 실제 탐색기로 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원문 입력만 공급하고 기대 결과는 기존 독립 관측에서 읽는다.</summary>
public sealed class RawPathSearchTests
{
    /// <summary>검색용 실제 풀/해시/표면 입력이며 결과를 미리 계산하지 않는다.</summary>
    internal sealed class SearchScene
    {
        /// <summary>부모5/표면6..30의 같은 실제 슬롯이다.</summary>
        internal readonly RawSidPool Pool;
        /// <summary>실제 현재 hash0와 이동용 hash1이다.</summary>
        internal readonly RawSquidHash Hash = new();
        /// <summary>판본별188/171행 실제 TYPE 입력이다.</summary>
        internal readonly OriginalTypeRecord[] Types;
        /// <summary>접합 함수에 직접 공급할 실제 FrameCode 표다.</summary>
        internal readonly RiftTypeFrames[] Frames;
        /// <summary>빈칸을 포함하는 실제65536BYTE 지도다.</summary>
        internal readonly byte[] Spots = new byte[65536];
        /// <summary>현재 방향/정책/진단 입력이다.</summary>
        internal readonly RawPathSearchState State = new();
        /// <summary>기존 prefix와 검색 결과를 같은 이미지에 저장한다.</summary>
        internal readonly RawPathRuntime Path;
        /// <summary>출발점의 원문 float 입력이다.</summary>
        internal RawPathPoint Start;
        /// <summary>목적점의 원문 float 입력이다.</summary>
        internal RawPathPoint Destination;
        /// <summary>원문과 같은 네 내장 생성자/표면 프레임만 준비한다.</summary>
        internal SearchScene(OriginalEdition edition)
        {
            Pool = new(32768, false, edition: edition); int count = edition == OriginalEdition.Patch1078 ? 188 : 171;
            Types = Enumerable.Range(0, count).Select(i => new OriginalTypeRecord { ConstructorAddress = i is 1 or 2 or 3 or 6 ? OriginalConstructors.Address(edition, i) : 0 }).ToArray();
            Frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToArray(); Path = new(edition);
            // 부모와 표면의 SID를 원문 관측 순서대로 확보한다.
            for (int sid = 5; sid < 32; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            // 접합 결과 대신 원문 입력 프레임6종을 제공한다.
            foreach (int type in new[] { 82, 83, 84 })
            {
                Types[type].Flags1 = OriginalTypeRecord.Surface; Types[type].Flags2 = type == 82 ? 4u : type == 83 ? 2u : 0x40000u;
                Frames[type] = new(new[] { new RiftFrameCode((byte)'A', (byte)'A', 1, 0), new((byte)'A', (byte)'P', 1, 0), new((byte)'M', (byte)'A', 1, 0), new((byte)'P', (byte)'P', 1, 0), new((byte)'K', (byte)'A', 1, 0), new((byte)'N', (byte)'A', 1, 0) });
            }
        }
        /// <summary>Python/CPP 관측과 같은 장애물/Graph/owner 입력을 쓴다.</summary>
        internal void Setup(int profile, int owner, bool avoid = true, int policy = 6, bool legacy = false, int guard = 0)
        {
            State.AvoidFlagTwo = avoid; State.PolicySelector = policy; State.GraphsEnabled = guard != 1; State.SearchEnabled = guard != 2;
            int side = profile == 4 ? 1 : profile == 5 ? 3 : 5, next = 6; bool patch = Pool.Edition == OriginalEdition.Patch1078;
            // 각 행의 실제 raw 표면과 spot를 같은 순서로 넣는다.
            for (int y = 10; y < 10 + side; y++)
            {
                // 같은 행의 좌표별 후보 입력이다.
                for (int x = 10; x < 10 + side; x++)
                {
                    byte type = 83, flag = 0, spot = 6, graph = 3; uint frame = 0; int sid = profile == 5 ? 6 : next;
                    if (profile == 1 && (x == 11 && y == 11 || x == 12 && y == 12 || x == 13 && y == 13)) spot = 8;
                    if (profile == 2 && x == 12) spot = 0; if (profile == 3 && (x + y) % 3 == 0) flag = 2;
                    if (profile == 6 && x == 12 && y < 14) { type = 82; frame = 4; } if (profile == 7 && x == 12 && y == 12) type = 84;
                    if (profile == 8 && x == 14 && y == 14) graph = 254; if (profile == 9 && x == 14 && y == 14) graph = 4;
                    if (profile != 5 || next == 6)
                    {
                        Span<byte> raw = Pool.AllocatedBytes(sid); raw.Clear(); raw[10] = type; raw[12] = flag;
                        Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y)); raw[patch ? 30 : 28] = graph; raw[patch ? 34 : 32] = (byte)owner;
                        if (patch) Put(raw, 36, frame); else raw[34] = (byte)frame;
                    }
                    Hash.Cell(0, x, y) = (ushort)sid; Spots[y * 256 + x] = spot; next++;
                }
            }
            Pool.AllocatedBytes(5)[10] = owner % 2 == 0 ? (byte)158 : (byte)134; Start = new(9.2f, 9.3f); Destination = new(9.2f + side - 1, 9.3f + side - 1);
            if (legacy) Path.SetPath([new(9, 9)], [1], [12.5]);
        }
        /// <summary>현재 결과를 원문 관측의 좌표/방향/시각 비트로 표현한다.</summary>
        internal static string Points(RawPathRuntime path) => path.Count == 0 ? "-" : string.Join(';', Enumerable.Range(0, path.Count).Select(i => $"{BitConverter.SingleToUInt32Bits(path.Point(i).X)},{BitConverter.SingleToUInt32Bits(path.Point(i).Y)},{path.Direction(i)},{BitConverter.DoubleToUInt64Bits(path.Time(i))}"));
        /// <summary>같은 실제 공간을 갱신하고 가상 부모 효과만 명시 입력으로 둔다.</summary>
        internal RawPathProcessHooks Effects(RawPop pop, RawUnpop unpop, List<string> events) => new()
        {
            Unpop = (sid, flags) => { events.Add("unpop"); unpop.Unpop(sid, Types[Pool.Slot(sid)[10]], flags); },
            Pop = (sid, point, flags) => { events.Add("pop"); Assert.Equal(RawPopResult.Registered, pop.Pop(sid, Types[Pool.Slot(sid)[10]], 1, 1, point.X, point.Y, flags)); },
            Repop = (sid, flags) => { events.Add("repop"); Assert.Equal(RawPopResult.Registered, pop.Pop(sid, Types[Pool.Slot(sid)[10]], 1, 1, BitConverter.UInt32BitsToSingle(Get(Pool.Slot(sid), 14)), BitConverter.UInt32BitsToSingle(Get(Pool.Slot(sid), 18)), flags)); },
            Halt = _ => 10, Arrival = (_, _, _) => { events.Add("arrival"); return 1000; }, SoundId = _ => 0,
            StepSound = (_, _, _, sound) => { Assert.Equal(0u, sound); events.Add("sound"); },
            ClearBlocked = (_, _, _, _) => throw new InvalidOperationException("이번 독립 입력에 겹침 없음"), Cleanup = (_, _, _) => events.Add("cleanup"),
        };
    }
    /// <summary>전체 signedWORD/큐의 미사용 흔적까지 원문 FNV32로 비교한다.</summary>
    private static void CheckWork(RawPathSearchWork work, string distance, string queue)
    {
        var bytes = new byte[work.Distance.Length * 2];
        // 호스트 endianness와 관계없이 원문 little endian 전체 바이트를 만든다.
        for (int i = 0; i < work.Distance.Length; i++) BinaryPrimitives.WriteInt16LittleEndian(bytes.AsSpan(i * 2), work.Distance[i]);
        Assert.Equal(U(distance), Fnv(bytes)); Assert.Equal(U(queue), Fnv(work.Queue));
    }
    /// <summary>기존 PE 관측과 같은 unsigned FNV를 계산한다.</summary>
    private static uint Fnv(ReadOnlySpan<byte> bytes)
    {
        uint value = 2166136261;
        // 전체 입력을 빠짐없이 비교한다.
        foreach (byte b in bytes) value = unchecked((value ^ b) * 16777619); return value;
    }
    /// <summary>두 판본644개의 경로/Graph/동률/거리/큐를 대조한다.</summary>
    [Fact]
    public void NativeBfs644MatchesEveryPointDistanceAndQueueByte()
    {
        string[][] rows = Rows("pathsearch-x86.tsv"); Assert.Equal(644, rows.Length);
        // 매 관측은 독립 raw 입력에서 실제 탐색한다.
        foreach (string[] row in rows)
        {
            var scene = new SearchScene(Edition(row[0])); scene.Setup((int)U(row[1]), (int)U(row[5]), U(row[3]) != 0, (int)U(row[4]), U(row[6]) != 0, (int)U(row[7]));
            var search = new RawPathSearch(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames, scene.State);
            if (U(row[9]) != 0) Assert.ThrowsAny<Exception>(() => search.Search(scene.Path, 5, scene.Start, scene.Destination, U(row[2])));
            else Assert.Equal(U(row[8]) != 0, search.Search(scene.Path, 5, scene.Start, scene.Destination, U(row[2])));
            Assert.Equal(row[10], SearchScene.Points(scene.Path)); CheckWork(search.Work, row[11], row[12]); Assert.Equal(U(row[13]), scene.State.DirectionPrerequisite[3]);
        }
    }
    /// <summary>두 판본 Init64개의 선행 표면/Pop/current/기존 prefix를 대조한다.</summary>
    [Fact]
    public void NativeInitialization64UsesActualSpatialPopAndSearch()
    {
        string[][] rows = Rows("pathinit-x86.tsv"); Assert.Equal(64, rows.Length);
        // 명시 Pop 효과도 같은 실제 공간을 갱신한다.
        foreach (string[] row in rows)
        {
            uint[] input = row[1].Split(',').Select(U).ToArray(); var scene = new SearchScene(Edition(row[0])); scene.Setup((int)input[0], 1, legacy: input[4] != 0);
            Span<byte> raw = scene.Pool.AllocatedBytes(5); Put(raw, 0, scene.Pool.Edition == OriginalEdition.Patch1078 ? 0x501dd0u : 0x5058d8u); raw[11] = input[3] != 0 ? (byte)4 : (byte)0;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(scene.Start.X)); Put(raw, 18, BitConverter.SingleToUInt32Bits(scene.Start.Y)); if (input[2] == 0) scene.Hash.Cell(0, 10, 10) = 0;
            var pop = new RawPop(scene.Pool, scene.Hash, scene.Spots); var unpop = new RawUnpop(scene.Pool, scene.Hash, scene.Spots); var events = new List<string>();
            var search = new RawPathSearch(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames, scene.State); var hooks = scene.Effects(pop, unpop, events); var actualPop = hooks.Pop!; string popEvent = "-";
            hooks = hooks with { Pop = (sid, point, flags) => { popEvent = $"pop,{BitConverter.SingleToUInt32Bits(point.X)},{BitConverter.SingleToUInt32Bits(point.Y)},{flags}"; actualPop(sid, point, flags); } };
            using var process = new RawPathProcess(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Types.Select(_ => new SquidDisplayShape()).ToArray(), scene.Spots, new(new()), RawPathSearch.MakeHooks(scene.Pool, search, hooks), false);
            scene.Path.Bytes.CopyTo(MemoryMarshal.AsMemory(process.Runtime.Bytes)); process.Runtime.RestoreStep(input[4] != 0 ? 1 : 0, 0); process.InitializePath(5, scene.Destination, scene.Start, input[1]);
            Assert.Equal(row[2], popEvent); Assert.Equal((int)U(row[3]), process.Runtime.Current); Assert.Equal((int)U(row[4]), process.Runtime.Count); Assert.Equal(U(row[5]), raw[11]);
            Assert.Equal(U(row[6]), Get(raw, 14)); Assert.Equal(U(row[7]), Get(raw, 18)); Assert.Equal(row[8], SearchScene.Points(process.Runtime)); CheckWork(search.Work, row[9], row[10]); Assert.Equal(U(row[11]), scene.State.DirectionPrerequisite[3]);
        }
    }
    /// <summary>현재 hash alias와 장기 작업 배열 및 현재 진단을 같은 검색 호출에서 소비한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void CurrentSurfaceDiagnosticAndLongLivedWorkAreShared(OriginalEdition edition)
    {
        var scene = new SearchScene(edition); scene.Setup(0, 2, policy: 5); var diagnostic = new UIntBox { Value = 6 }; scene.State.SharedPolicySelector = diagnostic; var storage = new RawPathSearchStorage();
        var search = new RawPathSearch(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames, scene.State, storage);
        Assert.Same(search.Work, storage.Work); Assert.False(storage.Initialized); Assert.ThrowsAny<Exception>(() => search.SearchCells(scene.Path, -1, 10, 14, 14, false)); Assert.False(storage.Initialized);
        // 동일 owner의 현재 값을 매번 읽으며 CD에는 패치 전용 변경을 적용하지 않는다.
        foreach (uint value in new uint[] { 6, 5, 1, 0, uint.MaxValue })
        {
            diagnostic.Value = value; scene.State.DirectionPrerequisite[3] = 20; scene.Path.SetPath([], []); search.SearchCells(scene.Path, 10, 10, 14, 14, false);
            Assert.Equal(edition == OriginalEdition.Patch1078 && value == 5 ? 5u : 20u, scene.State.DirectionPrerequisite[3]); Assert.Equal(5, scene.State.PolicySelector);
        }
        Assert.True(storage.Initialized); var next = new RawPathSearch(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames, scene.State, storage); Assert.Same(search.Work, next.Work);
        Assert.Equal(6, next.SurfaceAt(scene.Start)); Assert.Equal((byte)3, next.GraphAt(scene.Start)); scene.Hash.Cell(0, 10, 10) = 0;
        Assert.Equal(0, next.SurfaceAt(scene.Start)); Assert.Null(next.GraphAt(scene.Start)); scene.Path.SetPath([], []); Assert.False(next.Search(scene.Path, 5, scene.Start, scene.Destination, 0)); Assert.Equal(0, scene.Path.Count);
        Assert.Throws<ArgumentException>(() => RawPathSearch.MakeHooks(new(32768), next, new()));
    }
}
