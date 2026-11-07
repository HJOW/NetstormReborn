// 다리·섬 연결과 색 프레임·소유자 전파의 원본 관찰을 대조한다.
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// RawBridgeConnect의 연결·색·전파를 검증하는 테스트 클래스다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
public sealed class BridgeConnectTests
{
    /// <summary>기계어 실행기의 입력 번호다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int Source = 50;

    /// <summary>대체 생성이 돌려주는 첫 번호다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int FirstBorn = 100;

    /// <summary>대체 생성이 돌려주는 개수다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int BornCount = 12;

    /// <summary>실행기 풀의 마지막 번호다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int LastSid = 127;

    /// <summary>기계어 실행기의 가상 표 기록값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const uint Vtable = 0x11010000u;

    /// <summary>실행기 풀 크기다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int Capacity = 24000;

    /// <summary>값을 리틀 엔디언으로 쓴다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="raw">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓰는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>입력된 합성 코드 표를 만든다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="variant">변형 번호다.</param>
    /// <returns>16개 코드 목록이다.</returns>
    private static List<FrameCode> Frames(int variant)
    {
        const string letters = "JKALMNOJKLBJPPCD";
        List<FrameCode> codes = [];
        // 변형별 경화 비트는 원본 도구와 같은 입력인 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int i = 0; i < letters.Length; ++i)
        {
            bool hard = variant == 1 ? i == 0 || i == 1 || i == 7 || i == 8 || i == 11 : variant == 2 && (i == 0 || i == 8);
            FrameCodeFlags flags = (FrameCodeFlags)(hard ? 0x40 : letters[i] == 'L' || letters[i] == 'M' ? 0x20 : 0);
            codes.Add(new FrameCode(letters[i], 'P', i + 1, flags));
        }

        return codes;
    }

    /// <summary>같은 풀·상태·처리기를 조립해 입력과 결과를 대조하는 장면이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private sealed class BridgeScene
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

        /// <summary>연결 전역이다.</summary>
        public BridgeConnectState State = new();

        /// <summary>발생한 사건이다.</summary>
        public List<string> Events = [];

        /// <summary>만든 객체 수다.</summary>
        public ushort Born;

        /// <summary>패치판이면 참이다.</summary>
        public bool Patch;

        /// <summary>실행기와 같은 풀 번호를 확보한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        public BridgeScene(OriginalEdition edition)
        {
            Patch = edition == OriginalEdition.Patch1078;
            Pool = new RawSidPool(Capacity, false, null, edition);
            // 예측·서버 목록과 무관한 번호를 입력 슬롯으로 확보하는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
            for (int sid = 5; sid <= LastSid; ++sid)
            {
                Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            }

            Types = new OriginalTypeRecord[Patch ? 188 : 171];
            // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
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

        /// <summary>장면 입력 칸으로 슬롯·타입·체인·전역을 준비한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
        /// <param name="columns">객체·점유·변형·소유자·전역 5칸이다.</param>
        public void Prepare(string[] columns)
        {
            Assert.Equal(5, columns.Length);
            int variant = int.Parse(columns[2], CultureInfo.InvariantCulture);
            Hash.Reset();
            Array.Clear(Spots);
            Events.Clear();
            Born = 0;
            // 앞선 행의 슬롯이 남지 않도록 풀 전체를 지우는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
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
            // 객체가 없는 타입도 조회 대상이므로 합성 타입 표 전체를 넣는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
            foreach (var input in inputs)
            {
                OriginalTypeRecord type = Types[(int)input.Number];
                type.Flags1 = input.Flags1;
                type.Flags2 = input.Flags2;
                type.FootX = input.FootX;
                type.FootY = input.FootY;
                Frames[(int)input.Number] = Frames(variant);
            }

            // 각 기록의 12개 정수 필드를 읽는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
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

            Put(Pool.AllocatedBytes(Source), 8, 0x2a2bu, 2);
            Put(Pool.AllocatedBytes(Source), 12, 0x1234u, 2);
            if (columns[1] != "-")
            {
                // 점유 입력을 읽는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
                foreach (string entry in columns[1].Split(';'))
                {
                    string[] n = entry.Split(',');
                    Spots[int.Parse(n[1], CultureInfo.InvariantCulture) * 256 + int.Parse(n[0], CultureInfo.InvariantCulture)] = (byte)uint.Parse(n[2], CultureInfo.InvariantCulture);
                }
            }

            if (columns[3] != "-")
            {
                // 소유자 입력을 읽는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
                foreach (string entry in columns[3].Split(';'))
                {
                    string[] n = entry.Split(',');
                    Pool.AllocatedBytes(int.Parse(n[0], CultureInfo.InvariantCulture))[OwnerOffset] = (byte)uint.Parse(n[1], CultureInfo.InvariantCulture);
                }
            }

            string[] globals = columns[4].Split(',');
            Assert.Equal(13, globals.Length);
            State = new BridgeConnectState
            {
                ConnectorType = 90u,
                IslandType = 91u,
                StalagType = 92u,
                NoIslandType = 93u,
                BridgeType = uint.Parse(globals[0], CultureInfo.InvariantCulture),
                Battle = globals[1] == "1",
                Mission = globals[2] == "1",
            };
            // 색 표의 아홉 칸을 넣는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
            for (int i = 0; i < State.OwnerColors.Length; ++i)
            {
                State.OwnerColors[i] = int.Parse(globals[3 + i], CultureInfo.InvariantCulture);
            }

            Neighbors = new RawSquidNeighbors(Pool, Hash, Spots, Types, Frames);
        }

        /// <summary>프레임 필드를 판본의 폭으로 쓴다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
        /// <param name="sid">쓸 번호다.</param>
        /// <param name="frame">쓸 프레임 값이다.</param>
        public void PutFrame(int sid, uint frame) => Put(Pool.AllocatedBytes(sid), FrameOffset, frame, FrameWidth);

        /// <summary>원본 실행기의 대체와 같은 경계다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
        /// <returns>훅 묶음이다.</returns>
        public BridgeConnectHooks Hooks()
        {
            return new BridgeConnectHooks
            {
                Create = (type, flags) =>
                {
                    if (Born >= BornCount)
                    {
                        throw new InvalidOperationException("새 객체 번호 소진");
                    }

                    int sid = FirstBorn + Born++;
                    Span<byte> raw = Pool.AllocatedBytes(sid);
                    raw.Clear();
                    Put(raw, 0, Vtable, 4);
                    raw[10] = (byte)type;
                    Events.Add("C:" + type + ":" + flags);
                    return sid;
                },
                SetOwner = (sid, owner) =>
                {
                    Pool.AllocatedBytes(sid)[OwnerOffset] = (byte)owner;
                    Events.Add("O:" + sid + ":" + owner);
                },
                Pop = (sid, x, y, flags) => Events.Add("P:" + sid + ":" + BitConverter.SingleToUInt32Bits(x) + ":" + BitConverter.SingleToUInt32Bits(y) + ":" + flags),
                SetFrame = (sid, frame, flags) =>
                {
                    PutFrame(sid, unchecked((uint)frame));
                    Events.Add("S:" + sid + ":" + unchecked((uint)frame) + ":" + flags);
                },
                NotifySurface = sid => Events.Add("N:" + sid),
            };
        }

        /// <summary>모든 원본 효과의 순서와 인자를 한 문자열로 만든다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
        /// <returns>사건 문자열이다.</returns>
        public string EventsText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>소유자 색 표 9칸을 원본 관찰과 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="edition">판본 이름이다.</param>
    private static void ReplayColor(string edition)
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "bridgeconnect-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int count = 0;
        // 색 행만 골라 같은 입력으로 실행하는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string[] row = line.Split('\t');
            if (row[0] != "Color" || row[1] != edition)
            {
                continue;
            }

            Assert.Equal(6, row.Length);
            BridgeConnectState state = new() { Battle = row[2] == "1" };
            string[] colors = row[3].Split(',');
            // 색 표의 아홉 칸을 넣는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
            for (int i = 0; i < state.OwnerColors.Length; ++i)
            {
                state.OwnerColors[i] = int.Parse(colors[i], CultureInfo.InvariantCulture);
            }

            int owner = int.Parse(row[4], CultureInfo.InvariantCulture);
            if (state.Battle && owner >= state.OwnerColors.Length)
            {
                Assert.Throws<ArgumentOutOfRangeException>(() => RawBridgeConnect.OwnerColorFrame(state, owner));
            }
            else
            {
                Assert.Equal(row[5], RawBridgeConnect.OwnerColorFrame(state, owner).ToString(CultureInfo.InvariantCulture));
            }

            ++count;
        }

        Assert.Equal(42, count);
    }

    /// <summary>세 판본의 색 표를 각각 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ColorPatchX86()
    {
        ReplayColor("originals");
    }

    /// <summary>CD판의 색 표를 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ColorCdX86()
    {
        ReplayColor("originalCD");
    }

    /// <summary>추가 판본의 색 표를 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ColorThirdX86()
    {
        ReplayColor("original1037");
    }

    /// <summary>훅 누락·다른 풀·표 밖 번호·주인 없는 전파를 상태를 바꾸기 전에 거부한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsMissingHooksAndInvalidInputs()
    {
        BridgeScene scene = new(OriginalEdition.Patch1078);
        scene.Prepare(["50,82,0,0,1101004800,1101529088,1,1,2050,4,2,0;70,83,0,0,1100480512,1101529088,1,1,671090691,2,2,0;71,91,0,0,1100480512,1101529088,3,3,671088643,16777216,8,3", "-", "0", "50,0;71,0", "82,1,0,0,1,2,3,4,5,6,7,8,1"]);
        BridgeConnectHooks hooks = scene.Hooks();
        BridgeConnectHooks broken = scene.Hooks();
        broken.SetFrame = null;
        Assert.Throws<ArgumentException>(() => new RawBridgeConnect(scene.Pool, scene.Neighbors!, scene.State, broken));
        broken = scene.Hooks();
        broken.Create = null;
        Assert.Throws<ArgumentException>(() => new RawBridgeConnect(scene.Pool, scene.Neighbors!, scene.State, broken));
        BridgeScene other = new(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => new RawBridgeConnect(other.Pool, scene.Neighbors!, scene.State, hooks));
        Assert.Throws<ArgumentException>(() => new RawSquidNeighborWalk(scene.Neighbors!, Source, 0x10u));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBridgeConnect.OwnerColorFrame(scene.State, 9));
        Assert.Throws<ArgumentException>(() => RawBridgeConnect.SetIslandOwner(scene.Pool, scene.State, Source, 1u, null!));
        Assert.Throws<ArgumentException>(() => RawBridgeConnect.MakeOwnerDispatch(scene.Pool, scene.State, null!));
        RawBridgeConnect connect = new(scene.Pool, scene.Neighbors!, scene.State, hooks);
        byte[] before = scene.Pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => connect.PropagateOwner(Source, 70));
        Assert.True(scene.Events.Count == 0 && before.AsSpan().SequenceEqual(scene.Pool.Bytes.ToArray()));
        Assert.Throws<ArgumentException>(() => connect.IslandPostPop(Source, 0u, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => connect.FindTypeAt(20f, 21f, 5000u));
        scene.Pool.RawSlot(Source)[scene.OwnerOffset] = 4;
        connect.PropagateOwner(Source, 70);
        Assert.Equal("O:71:4;S:71:3:0", scene.EventsText());
    }

    /// <summary>공통 장치 누락 앞에 실제 접두의 생성·소유자·Pop 효과가 남는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void IslandPrefixKeepsEarlierEffects()
    {
        BridgeScene scene = new(OriginalEdition.Patch1078);
        scene.Prepare(["50,91,0,0,1106247680,1106247680,3,3,671088643,16777216,0,0", "-", "0", "50,3", "82,1,0,0,1,2,3,4,5,6,7,8,1"]);
        BridgeConnectHooks hooks = scene.Hooks();
        RawBridgeConnect connect = new(scene.Pool, scene.Neighbors!, scene.State, hooks);
        Assert.Throws<ArgumentException>(() => connect.IslandPostPop(Source, 1u, null));
        Assert.Equal(1, scene.Born);
        Assert.Equal("C:92:2;O:100:3;P:100:1106247680:1106247680:0", scene.EventsText());
    }

    /// <summary>목록 쓰기가 다음 생성 재진입 전에 보인다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void PreviewAppendVisibleBeforeNextCreation()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "bridgeconnect-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        List<string[]> fixture = [];
        // 주석·빈 줄을 빼고 행을 모으는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            fixture.Add(line.Split('\t'));
        }

        Dictionary<string, string[]> scenes = new();
        // 장면 행을 번호로 찾는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        foreach (string[] row in fixture)
        {
            if (row[0] == "Scene")
            {
                scenes[row[1]] = [row[2], row[3], row[4], row[5], row[6]];
            }
        }

        bool found = false;
        // 독립 원문의 다중 연결 장면을 고르는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        foreach (string[] row in fixture)
        {
            if (row[0] != "Connect" || row[1] != "originals" || row[3] != "1" || !row[7].Contains(',', StringComparison.Ordinal))
            {
                continue;
            }

            BridgeScene scene = new(OriginalEdition.Patch1078);
            scene.Prepare(scenes[row[2]]);
            BridgeConnectHooks hooks = scene.Hooks();
            Func<uint, uint, int> create = hooks.Create!;
            List<int> written = [];
            int calls = 0;
            hooks.Create = (type, flags) =>
            {
                Assert.Equal(written.Count, calls++);
                return create(type, flags);
            };
            RawBridgeConnect connect = new(scene.Pool, scene.Neighbors!, scene.State, hooks);
            Assert.Throws<ArgumentException>(() => connect.ConnectPreview(Source, null!));
            connect.ConnectPreview(Source, written.Add);
            Assert.True(calls > 1);
            Assert.Equal(row[7], written.Count == 0 ? "empty" : string.Join(',', written));
            Assert.Same(scene.Pool, connect.Pool);
            found = true;
            break;
        }

        Assert.True(found);
    }

    /// <summary>순회 도중 뒤쪽 버킷의 새 객체를 다음 호출에서 읽는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void WalkReadsLiveChainBetweenNextCalls()
    {
        BridgeScene scene = new(OriginalEdition.Patch1078);
        scene.Prepare(["50,82,0,0,1101004800,1101529088,1,1,2050,4,2,0;70,83,0,0,1100480512,1101529088,1,1,671090691,2,2,0;71,83,0,0,1101529088,1101529088,1,1,671090691,2,2,0", "-", "0", "-", "82,1,0,0,1,2,3,4,5,6,7,8,1"]);
        RawSquidNeighborWalk walk = new(scene.Neighbors!, Source, NeighborFlags.SkipAbstract);
        Assert.Equal(70, walk.Current);
        Add(scene, 72, 20f, 22f);
        Add(scene, 73, 20f, 20f);
        Assert.Equal(71, walk.Next());
        Assert.Equal(72, walk.Next());
        Assert.Equal(0, walk.Next());
        Assert.Equal(0, walk.Next());
    }

    /// <summary>만든 객체 슬롯의 16진수 목록이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="scene">읽을 장면이다.</param>
    /// <returns>쉼표로 이은 목록이다. 없으면 "-"다.</returns>
    private static string BornText(BridgeScene scene)
    {
        List<string> parts = [];
        // 만든 순서대로 쉼표로 잇는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int i = 0; i < scene.Born; ++i)
        {
            ReadOnlySpan<byte> slot = scene.Pool.Slot(FirstBorn + i);
            parts.Add(Convert.ToHexString(scene.Patch ? slot : slot.Slice(0, 36)).ToLowerInvariant());
        }

        return parts.Count == 0 ? "-" : string.Join(',', parts);
    }

    /// <summary>실행기 풀 번호 0~127의 Adler-32다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="scene">읽을 장면이다.</param>
    /// <returns>십진수 문자열이다.</returns>
    private static string PoolAdler(BridgeScene scene)
    {
        int stride = scene.Patch ? 50 : 36;
        byte[] image = new byte[(LastSid + 1) * stride];
        // 확보한 슬롯만 실제 바이트를 복사하는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int sid = 5; sid <= LastSid; ++sid)
        {
            scene.Pool.Slot(sid).Slice(0, stride).CopyTo(image.AsSpan(sid * stride, stride));
        }

        return Netstorm.Tests.X86Fixture.Adler32(image).ToString(CultureInfo.InvariantCulture);
    }

    /// <summary>프레임 필드를 판본의 폭으로 읽는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="scene">읽을 장면이다.</param>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>십진수 문자열이다.</returns>
    private static string FrameText(BridgeScene scene, int sid)
    {
        ReadOnlySpan<byte> raw = scene.Pool.Slot(sid);
        uint frame = scene.Patch ? System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(36, 4)) : raw[34];
        return frame.ToString(CultureInfo.InvariantCulture);
    }

    /// <summary>미리보기 목록을 실행기의 표기로 만든다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="preview">쓴 목록이다.</param>
    /// <param name="used">목록을 썼으면 참이다.</param>
    /// <returns>표기 문자열이다.</returns>
    private static string ListedText(List<int> preview, bool used)
    {
        if (!used)
        {
            return "-";
        }

        return preview.Count == 0 ? "empty" : string.Join(',', preview);
    }

    /// <summary>한 판본의 장면 관찰 행을 모두 재생한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="edition">판본 이름이다.</param>
    private static void Replay(string edition)
    {
        OriginalEdition version = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "bridgeconnect-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        Dictionary<string, string[]> scenes = new();
        List<string[]> rows = [];
        // 장면 행은 번호로 따로 두고 관찰 행을 모으는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
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
        // 각 행을 앞 행과 독립된 입력부터 다시 준비해 실행하는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        foreach (string[] row in rows)
        {
            if (row[1] != edition || row[0] == "Color")
            {
                continue;
            }

            string kind = row[0];
            string actual;
            string expected;
            try
            {
                if (kind == "Walk")
                {
                    BridgeScene scene = new(version);
                    scene.Prepare(scenes[row[2]]);
                    RawSquidNeighborWalk walk = new(scene.Neighbors!, Source, uint.Parse(row[3], CultureInfo.InvariantCulture));
                    List<string> found = [];
                    // 0이 나올 때까지 이웃 번호를 모으는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
                    for (int current = walk.Current; current != 0; current = walk.Next())
                    {
                        found.Add(current.ToString(CultureInfo.InvariantCulture));
                    }

                    actual = found.Count == 0 ? "-" : string.Join(',', found);
                    expected = row[4];
                }
                else if (kind == "FindAt")
                {
                    BridgeScene scene = new(version);
                    scene.Prepare(scenes[row[2]]);
                    RawBridgeConnect connect = new(scene.Pool, scene.Neighbors!, scene.State, scene.Hooks());
                    actual = connect.FindTypeAt(
                        BitConverter.UInt32BitsToSingle(uint.Parse(row[3], CultureInfo.InvariantCulture)),
                        BitConverter.UInt32BitsToSingle(uint.Parse(row[4], CultureInfo.InvariantCulture)),
                        uint.Parse(row[5], CultureInfo.InvariantCulture)).ToString(CultureInfo.InvariantCulture);
                    expected = row[6];
                }
                else if (kind == "IslandOwner" || kind == "StalagOwner")
                {
                    BridgeScene scene = new(version);
                    scene.Prepare(scenes[row[2]]);
                    RawBridgeConnect.SetIslandOwner(scene.Pool, scene.State, Source, uint.Parse(row[3], CultureInfo.InvariantCulture), (sid, owner) =>
                    {
                        scene.Pool.AllocatedBytes(sid)[scene.OwnerOffset] = (byte)owner;
                        scene.Events.Add("B:" + sid + ":" + owner);
                    });
                    actual = scene.EventsText() + "|" + FrameText(scene, Source);
                    expected = row[4] + "|" + row[5];
                }
                else
                {
                    BridgeScene scene = new(version);
                    scene.Prepare(scenes[row[2]]);
                    RawBridgeConnect connect = new(scene.Pool, scene.Neighbors!, scene.State, scene.Hooks());
                    List<int> preview = [];
                    if (kind == "Connect")
                    {
                        bool listed = row[3] == "1";
                        if (listed)
                        {
                            connect.ConnectPreview(Source, preview.Add);
                        }
                        else
                        {
                            connect.Connect(Source);
                        }

                        actual = scene.EventsText() + "|" + BornText(scene) + "|" + PoolAdler(scene) + "|" + ListedText(preview, listed);
                        expected = row[4] + "|" + row[5] + "|" + row[6] + "|" + row[7];
                    }
                    else if (kind == "Link")
                    {
                        bool listed = row[5] == "1";
                        connect.Link(int.Parse(row[3], CultureInfo.InvariantCulture), int.Parse(row[4], CultureInfo.InvariantCulture), uint.Parse(row[6], CultureInfo.InvariantCulture), listed ? preview : null);
                        actual = scene.EventsText() + "|" + BornText(scene) + "|" + PoolAdler(scene) + "|" + ListedText(preview, listed);
                        expected = row[7] + "|" + row[8] + "|" + row[9] + "|" + row[10];
                    }
                    else if (kind == "Owner")
                    {
                        connect.PropagateOwner(int.Parse(row[3], CultureInfo.InvariantCulture), int.Parse(row[4], CultureInfo.InvariantCulture));
                        actual = scene.EventsText() + "|" + PoolAdler(scene);
                        expected = row[5] + "|" + row[6];
                    }
                    else if (kind == "IslandPost")
                    {
                        connect.IslandPostPop(Source, uint.Parse(row[3], CultureInfo.InvariantCulture), (sid, flags) => scene.Events.Add("T:" + sid + ":" + flags));
                        actual = scene.EventsText() + "|" + BornText(scene) + "|" + PoolAdler(scene);
                        expected = row[4] + "|" + row[5] + "|" + row[6];
                    }
                    else
                    {
                        throw new InvalidOperationException("알 수 없는 행 종류");
                    }
                }
            }
            catch (Exception error)
            {
                actual = "예외: " + error.Message;
                expected = "(일치 기대)";
            }

            if (actual != expected)
            {
                if (failures.Count < 6)
                {
                    failures.Add($"{edition} {kind} 장면 {row[2]}: 실제 {actual} / 기대 {expected}");
                }
            }

            counts[kind] = counts.TryGetValue(kind, out int had) ? had + 1 : 1;
        }

        Assert.True(failures.Count == 0, string.Join('\n', failures));
        Assert.Equal(1856, counts.GetValueOrDefault("Walk", 0));
        Assert.Equal(464, counts.GetValueOrDefault("Connect", 0));
        Assert.Equal(392, counts.GetValueOrDefault("Link", 0));
        Assert.Equal(330, counts.GetValueOrDefault("Owner", 0));
        Assert.Equal(270, counts.GetValueOrDefault("FindAt", 0));
        Assert.Equal(96, counts.GetValueOrDefault("IslandOwner", 0));
        Assert.Equal(96, counts.GetValueOrDefault("StalagOwner", 0));
        Assert.Equal(54, counts.GetValueOrDefault("IslandPost", 0));
    }

    /// <summary>세 판본의 장면 관찰을 각각 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void FullPatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD판의 장면 관찰을 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void FullCdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본의 장면 관찰을 대조한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void FullThirdX86()
    {
        Replay("original1037");
    }

    /// <summary>표면 칸을 직접 등록한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="scene">쓸 장면이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    private static void Add(BridgeScene scene, int sid, float x, float y)
    {
        Span<byte> raw = scene.Pool.AllocatedBytes(sid);
        Put(raw, 0, Vtable, 4);
        raw[10] = 83;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(x), 4);
        Put(raw, 18, BitConverter.SingleToUInt32Bits(y), 4);
        Put(raw, 36, 2u, 4);
        ref ushort head = ref scene.Hash.Bucket(0, x, y);
        Put(raw, 4, head, 2);
        head = (ushort)sid;
    }
}
