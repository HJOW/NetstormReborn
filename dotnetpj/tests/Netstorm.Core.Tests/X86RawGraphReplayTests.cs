using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 그래프 표·영역·추가/flood(<see cref="RawGraph"/>)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/RawGraphTests.cpp 의 ReplayRawGraph 와 같다: 실제 슬롯 풀·해시·점유 지도 위에서
/// 공통 Pop·공통 PostPop·그래프 Region/Add/Flood 를 순서대로 실행하고, 호출마다 7개 슬롯 50바이트·
/// 표/스택/해시/점유/풀 체크섬·TotalCost·깊이·세 Totalmade 표를 비교한다.
/// 표시(SquidDisplay) 효과는 슬롯·공간을 바꾸지 않으므로 이 재생에서는 끈 경로(null)를 쓴다.
/// 기준 판본은 10.78 이며 원본1037·CD 행은 읽지 않는다 (W02 열넷째 조각, 2026-10-20).
/// </summary>
public sealed class X86RawGraphReplayTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>연산 대상 타입(장면의 첫 표면). 나머지 표면은 75~80 이다.</summary>
    private const int FirstType = 74;

    /// <summary>기대값의 빈 칸 표기.</summary>
    private const string Empty = "-";

    /// <summary>"a,b;c,d" 형식의 정수 목록을 읽는다.</summary>
    /// <param name="text">목록 문자열.</param>
    private static int[][] Rows(string text) => text == Empty ? []
        : text.Split(';').Select(entry => entry.Split(',').Select(int.Parse).ToArray()).ToArray();

    /// <summary>리틀 엔디언 hex 를 바이트로 읽는다.</summary>
    /// <param name="hex">hex 문자열.</param>
    private static byte[] Bytes(string hex) => Convert.FromHexString(hex);

    /// <summary>1,530바이트 hex 를 표 레코드 255개로 읽는다(레코드마다 16비트 3개).</summary>
    /// <param name="hex">hex 문자열.</param>
    private static GraphRecord[] Records(string hex)
    {
        byte[] bytes = Bytes(hex);
        var records = new GraphRecord[RawGraph.TableSize];
        // 레코드마다 세 WORD 를 부호 있는 값으로 읽는다
        for (int i = 0; i < records.Length; i++)
        {
            records[i] = new GraphRecord
            {
                Surfaces = BitConverter.ToInt16(bytes, i * 6),
                InUse = BitConverter.ToInt16(bytes, i * 6 + 2),
                Reserved = BitConverter.ToInt16(bytes, i * 6 + 4),
            };
        }
        return records;
    }

    /// <summary>16,384바이트 hex 를 flood 스택 4,096 DWORD 로 읽는다.</summary>
    /// <param name="hex">hex 문자열.</param>
    private static uint[] Stack(string hex)
    {
        byte[] bytes = Bytes(hex);
        var stack = new uint[RawGraph.FloodSize];
        // DWORD 마다 리틀 엔디언으로 읽는다
        for (int i = 0; i < stack.Length; i++)
        {
            stack[i] = BitConverter.ToUInt32(bytes, i * 4);
        }
        return stack;
    }

    /// <summary>스택 시드 행을 원본처럼 seed+i 로 채운다.</summary>
    /// <param name="seed">스택 첫 값.</param>
    private static uint[] SeededStack(uint seed)
    {
        var stack = new uint[RawGraph.FloodSize];
        // 오래된 스택 흔적을 서로 다른 값으로 채운다
        for (int i = 0; i < stack.Length; i++)
        {
            stack[i] = unchecked(seed + (uint)i);
        }
        return stack;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>표 전체(레코드 255 × 2바이트 3개)를 바이트로 늘어놓는다.</summary>
    /// <param name="records">표 레코드.</param>
    private static byte[] RecordBytes(IReadOnlyList<GraphRecord> records)
    {
        var bytes = new byte[RawGraph.TableSize * 6];
        // 레코드마다 표면 수·사용 여부·예약 WORD 를 쓴다
        for (int i = 0; i < records.Count; i++)
        {
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6), records[i].Surfaces);
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6 + 2), records[i].InUse);
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6 + 4), records[i].Reserved);
        }
        return bytes;
    }

    /// <summary>스택 4,096 DWORD 를 리틀 엔디언 바이트로 늘어놓는다.</summary>
    /// <param name="stack">스택.</param>
    private static byte[] StackBytes(IReadOnlyList<uint> stack)
    {
        var bytes = new byte[RawGraph.FloodSize * 4];
        // DWORD 마다 네 바이트를 쓴다
        for (int i = 0; i < stack.Count; i++)
        {
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 4), stack[i]);
        }
        return bytes;
    }

    /// <summary>네 단계 해시 머리를 큰 단계부터 16비트 리틀 엔디언으로 늘어놓는다.</summary>
    /// <param name="hash">공간 해시.</param>
    private static byte[] HeadBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 단계 0~3 순서로 모든 머리를 쓴다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            // 머리마다 두 바이트
            foreach (ushort value in hash.Entries(level))
            {
                bytes.Add((byte)value);
                bytes.Add((byte)(value >> 8));
            }
        }
        return bytes.ToArray();
    }

    /// <summary>Totalmade 표 256 DWORD 를 리틀 엔디언 바이트로 늘어놓는다.</summary>
    /// <param name="table">표.</param>
    private static byte[] TableBytes(uint[] table)
    {
        var bytes = new byte[table.Length * 4];
        // DWORD 마다 네 바이트를 쓴다
        for (int i = 0; i < table.Length; i++)
        {
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 4), table[i]);
        }
        return bytes;
    }

    /// <summary>기대값 비교. 다르면 어느 열이 어떻게 다른지 담아 예외를 던진다.</summary>
    /// <param name="name">열 이름.</param>
    /// <param name="actual">실제 값.</param>
    /// <param name="expected">기대 값.</param>
    private static void Expect(string name, uint actual, uint expected)
    {
        if (actual != expected)
        {
            throw new InvalidOperationException($"{name}: 실제 {actual}, 기대 {expected}");
        }
    }

    /// <summary>한 Setup 장면의 풀·타입·해시·점유 지도·표·스택·공통 Pop/PostPop/그래프를 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>슬롯 풀(서버 아님, 32,768칸).</summary>
        public readonly RawSidPool Pool = new(32768, false);

        /// <summary>공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>점유 지도.</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개, 장면 기본값).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>타입별 프레임 코드.</summary>
        public readonly List<IReadOnlyList<FrameCode>> Frames;

        /// <summary>공통 후처리 상태(Totalmade stale 값 포함).</summary>
        public readonly RawPostPopState State = new();

        /// <summary>확보한 클라이언트 슬롯 7개.</summary>
        public readonly int[] Ids = new int[7];

        /// <summary>표·영역·추가를 수행할 그래프.</summary>
        public RawGraph Graph { get; private set; }

        /// <summary>공통 후처리.</summary>
        public RawPostPop Post { get; private set; }

        /// <summary>공통 Pop(표시를 끈 경로).</summary>
        public RawPop Pop { get; private set; }

        /// <summary>공통 Unpop(표시를 끈 경로, 삭제 재생에서 쓴다).</summary>
        public RawUnpop Unpop { get; private set; }

        /// <summary>Begin 행으로 풀과 타입·프레임을 준비하고 슬롯 7개를 확보한다.</summary>
        /// <param name="expectedIds">원본이 기대하는 슬롯 번호 순서.</param>
        public Scene(int[] expectedIds)
        {
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 장면의 base 초기값을 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord { MaxHitPoints = 91, ZOrder = 7, FootX = 1, FootY = 1, Cost = 1.25f };
            }
            Frames = Enumerable.Range(0, Types.Length).Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode>()).ToList();
            var factory = new RawSquidFactory(Pool, Types);
            // 원본 실행 파일의 client 할당 순서와 같은지 확인한다
            for (int i = 0; i < Ids.Length; i++)
            {
                Ids[i] = factory.Create(FirstType, SidAllocation.Client);
                if (Ids[i] != expectedIds[i])
                {
                    throw new InvalidOperationException($"Begin 슬롯 {i}: 실제 {Ids[i]}, 기대 {expectedIds[i]}");
                }
            }
            Graph = new RawGraph(Pool, Hash, Spots, Types);
            Post = new RawPostPop(Pool, Types, State);
            Pop = new RawPop(Pool, Hash, Spots);
            Unpop = new RawUnpop(Pool, Hash, Spots);
        }

        /// <summary>
        /// Setup 행의 노드·점유·표·스택을 장면에 반영하고 그래프/후처리/Pop 을 다시 만든다.
        /// 영역 fixture 는 해시 단계·체인·스택 시드가 달라 그 입력을 그대로 쓴다.
        /// </summary>
        /// <param name="row">Setup 행.</param>
        /// <param name="regionFixture">영역/삭제 계열 fixture 인지(단계·체인·시드 스택).</param>
        public void Setup(string[] row, bool regionFixture)
        {
            Hash.Reset();
            Array.Clear(Spots);
            int[][] nodes = Rows(row[1]);
            if (nodes.Length != Ids.Length)
            {
                throw new InvalidOperationException($"Setup 노드 {nodes.Length}개, 슬롯 {Ids.Length}개");
            }
            // 노드마다 타입·프레임·슬롯·해시 머리를 원본 순서로 쓴다
            for (int i = 0; i < nodes.Length; i++)
            {
                int[] n = nodes[i];
                OriginalTypeRecord type = Types[FirstType + i];
                type.Flags1 = (uint)n[4];
                type.Flags2 = (uint)n[5];
                type.FootX = n[2];
                type.FootY = n[3];
                Frames[FirstType + i] =
                [
                    new FrameCode((char)n[6], (char)n[7], 1, FrameCodeFlags.None),
                    new FrameCode('A', 'P', 2, FrameCodeFlags.None),
                ];
                int level = regionFixture ? n[12] : 0;
                bool mapped = (n[9] & 4) == 0 && (regionFixture || n[12] == 0);
                int next = mapped && regionFixture ? Hash.Bucket(level, n[0], n[1]) : 0;
                Node(Ids[i], (byte)(FirstType + i), n, next);
                if (mapped)
                {
                    Hash.Bucket(level, n[0], n[1]) = (ushort)Ids[i];
                }
            }
            // 내부 점유 후보를 별도로 준비한다
            foreach (int[] spot in Rows(row[2]))
            {
                Spots[spot[1] * RawSquidHash.WorldCells + spot[0]] = (byte)spot[2];
            }
            State.TotalCost = 53;
            State.Depth = 5;
            State.LocalOwner = 1;
            State.GraphsEnabled = true;
            // 세 Totalmade 표에 같은 stale DWORD 를 채운다
            for (int i = 0; i < RawPostPopState.TableSize; i++)
            {
                uint value = 31337U + (uint)i * 2654435761U;
                State.LocalCounts[i] = value;
                State.LocalSecondaryCounts[i] = unchecked(value + 17);
                State.GlobalCounts[i] = unchecked(value + 34);
            }
            uint[] stack = regionFixture ? SeededStack(X86Fixture.UInt(row[4])) : Stack(row[4]);
            Graph = new RawGraph(Pool, Hash, Spots, Types, Records(row[3]), stack, Frames);
            Post = new RawPostPop(Pool, Types, State, Graph);
            Pop = new RawPop(Pool, Hash, Spots, null, Post);
            Unpop = new RawUnpop(Pool, Hash, Spots);
        }

        /// <summary>노드 하나의 슬롯을 원본 Setup 입력으로 채운다.</summary>
        /// <param name="sid">슬롯 번호.</param>
        /// <param name="typeNumber">타입 번호.</param>
        /// <param name="n">노드 입력 13개.</param>
        /// <param name="next">해시 체인의 다음 슬롯(없으면 0).</param>
        private void Node(int sid, byte typeNumber, int[] n, int next)
        {
            Span<byte> bytes = Pool.AllocatedBytes(sid);
            bytes.Clear();
            Put(bytes, 0, OriginalConstructors.BaseVtable);
            Put(bytes, 4, (uint)next, 2);
            Put(bytes, 8, 23, 2);
            bytes[10] = typeNumber;
            bytes[11] = (byte)n[9];
            Put(bytes, 14, BitConverter.SingleToUInt32Bits(n[0]));
            Put(bytes, 18, BitConverter.SingleToUInt32Bits(n[1]));
            bytes[30] = (byte)n[11];
            bytes[33] = (byte)n[12];
            bytes[34] = 1;
            Put(bytes, 36, (uint)n[8]);
            bytes[40] = (byte)n[10];
        }
    }

    /// <summary>
    /// fixture 한 개를 재생한다. Begin/Setup 은 호출 수에 넣지 않고, 10.78 행만 실행한다.
    /// 각 호출 뒤 슬롯·표·스택·해시·점유·풀·후처리 상태를 원본 열과 비교한다.
    /// </summary>
    /// <param name="fixture">fixture 이름(확장자 제외).</param>
    /// <param name="regionFixture">영역/삭제 계열 장면인지.</param>
    /// <param name="expectedBegins">전체 Begin 행 수(모든 판본).</param>
    /// <param name="expectedCalls">10.78 호출 수.</param>
    private static void Replay(string fixture, bool regionFixture, int expectedBegins, Dictionary<string, int> expectedCalls)
    {
        string[][] rows = X86Fixture.ReadFile($"{fixture}-x86.tsv");
        Scene? scene = null;
        bool patch = false;
        int begins = 0;
        var calls = new Dictionary<string, int>();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] == "Begin")
            {
                Assert.Equal(4, row.Length);
                patch = row[1] == PatchEdition;
                begins++;
                scene = patch ? new Scene(Rows(row[3])[0]) : null;
                return true;
            }
            if (!patch || scene is null)
            {
                return true;
            }
            if (row[0] == "Setup")
            {
                scene.Setup(row, regionFixture);
                return true;
            }
            int[][] args = Rows(row[1]);
            int target = scene.Ids[0];
            switch (row[0])
            {
                case "Pop":
                {
                    byte[] expectedRoot = Bytes(row[3].Split(';')[0]);
                    var expected = (expectedRoot[11] & 4) != 0 ? RawPopResult.Overlap : RawPopResult.Registered;
                    RawPopResult actual = scene.Pop.Pop(target, scene.Types[FirstType], 1, 1, args[0][1], args[0][2], (uint)args[0][0]);
                    if (actual != expected)
                    {
                        throw new InvalidOperationException($"Pop 결과: 실제 {actual}, 기대 {expected}");
                    }
                    break;
                }
                case "PostPop":
                    scene.Post.PostPop(target, (uint)args[0][0]);
                    break;
                case "Add":
                    scene.Graph.Add(target);
                    break;
                case "Region":
                    scene.Graph.InvalidateRegion(target);
                    break;
                case "Flood":
                    Expect("Flood 반환", scene.Graph.Flood(target, (byte)args[0][0]), X86Fixture.UInt(row[2]));
                    break;
                case "Detach":
                    scene.Graph.Detach(target, args[0][0] != 0, (byte)args[0][1]);
                    break;
                case "Unpop":
                    scene.Unpop.Unpop(target, scene.Types[FirstType], (uint)args[0][0]);
                    break;
                case "Release":
                    scene.Pool.Release(target);
                    break;
                default:
                    return false;
            }
            calls[row[0]] = calls.GetValueOrDefault(row[0]) + 1;
            string[] slots = row[3].Split(';');
            // 7개 슬롯 전체 바이트를 원본과 비교한다
            for (int i = 0; i < scene.Ids.Length; i++)
            {
                string actualSlot = Convert.ToHexStringLower(scene.Pool.Slot(scene.Ids[i]));
                if (actualSlot != slots[i])
                {
                    throw new InvalidOperationException($"슬롯 {scene.Ids[i]} 바이트 불일치");
                }
            }
            Expect("표 체크섬", X86Fixture.Adler32(RecordBytes(scene.Graph.Records)), X86Fixture.UInt(row[4]));
            Expect("스택 체크섬", X86Fixture.Adler32(StackBytes(scene.Graph.FloodStack)), X86Fixture.UInt(row[5]));
            Expect("해시 머리 체크섬", X86Fixture.Adler32(HeadBytes(scene.Hash)), X86Fixture.UInt(row[6]));
            Expect("점유 지도 체크섬", X86Fixture.Adler32(scene.Spots), X86Fixture.UInt(row[7]));
            Expect("풀 체크섬", X86Fixture.Adler32(scene.Pool.Bytes), X86Fixture.UInt(row[8]));
            Expect("TotalCost", unchecked((uint)scene.State.TotalCost), X86Fixture.UInt(row[9]));
            Expect("깊이", scene.State.Depth, X86Fixture.UInt(row[10]));
            Expect("지역 장부", X86Fixture.Adler32(TableBytes(scene.State.LocalCounts)), X86Fixture.UInt(row[11]));
            Expect("보조 장부", X86Fixture.Adler32(TableBytes(scene.State.LocalSecondaryCounts)), X86Fixture.UInt(row[12]));
            Expect("전체 장부", X86Fixture.Adler32(TableBytes(scene.State.GlobalCounts)), X86Fixture.UInt(row[13]));
            return true;
        });
        Assert.Equal(expectedBegins, begins);
        Assert.Equal(expectedCalls.OrderBy(pair => pair.Key).ToArray(), calls.OrderBy(pair => pair.Key).ToArray());
    }

    /// <summary>할당 슬롯 사후처리·예측 Pop·추가/flood 장면을 10.78 몫 768회 재생한다.</summary>
    [Fact]
    public void AllocatedSidPostPopAndFloodMatchOriginalX86()
    {
        Replay("rawgraph", false, 4, new Dictionary<string, int>
        {
            ["Add"] = 256,
            ["Flood"] = 128,
            ["Pop"] = 64,
            ["PostPop"] = 320,
        });
    }

    /// <summary>일반 탐색·다리/섬 Pop 뒤 영역 무효화·추가 장면을 10.78 몫 512회 재생한다.</summary>
    [Fact]
    public void RegionAndPostPopMatchOriginalX86()
    {
        Replay("regiongraph", true, 6, new Dictionary<string, int>
        {
            ["Add"] = 96,
            ["Pop"] = 96,
            ["PostPop"] = 160,
            ["Region"] = 160,
        });
    }

    /// <summary>죽은 원천의 이웃 분리·다리/섬 해제·반납 장면을 10.78 몫 386회 재생한다.</summary>
    [Fact]
    public void DetachGeneralIslandBridgeUnpopAndReleaseMatchOriginalX86()
    {
        Replay("graphremove", true, 6, new Dictionary<string, int>
        {
            ["Detach"] = 128,
            ["Unpop"] = 256,
            ["Release"] = 2,
        });
    }

    /// <summary>같은 위치의 다른 번호 조회·원천 프레임 유지 장면을 10.78 몫 128회 재생한다.</summary>
    [Fact]
    public void DetachUsesPositionHeadGraphMatchOriginalX86()
    {
        Replay("graphlookup", true, 6, new Dictionary<string, int>
        {
            ["Detach"] = 128,
        });
    }
}
