// 무인도 최초 등록·받침 소유자 전파의 경계·분배를 직접 검사한다.
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// RawIslandPostPop의 지도·상태·소유자 전파를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandPostPopTests
{
    /// <summary>검사 풀 용량이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Capacity = 24000;

    /// <summary>표면 칸 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Surface = 50;

    /// <summary>무인도 표면 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint NoIsland = 157u;

    /// <summary>섬 받침 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint Island = 94u;

    /// <summary>섬 종유석 타입 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const uint Stalag = 95u;

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

    /// <summary>풀·타입·프레임·지도의 빈 장면을 만든다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <returns>풀·타입·프레임·표면 지도·소유자 지도·연결 전역 묶음이다.</returns>
    private static (RawSidPool Pool, OriginalTypeRecord[] Types, RiftTypeFrames[] Frames, ushort[] SurfaceMap, ushort[] OwnerMap, BridgeConnectState TypeState) Scene()
    {
        RawSidPool pool = new(Capacity, false);
        OriginalTypeRecord[] types = new OriginalTypeRecord[188];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        RiftTypeFrames[] frames = new RiftTypeFrames[types.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }

        ushort[] surfaceMap = new ushort[65536];
        ushort[] ownerMap = new ushort[65536];
        BridgeConnectState typeState = new()
        {
            IslandType = Island,
            StalagType = Stalag,
            NoIslandType = NoIsland,
        };
        return (pool, types, frames, surfaceMap, ownerMap, typeState);
    }

    /// <summary>원본 지도 조회는 범위 밖·다른 타입을 0으로 돌려준다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SurfaceLetterGuards()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        frames[(int)NoIsland] = new RiftTypeFrames(new[] { new RiftFrameCode((byte)'H', (byte)'P', 1, 0) });
        Span<byte> raw = pool.RawSlot(Surface);
        raw.Clear();
        raw[10] = (byte)NoIsland;
        Put(raw, 36, 0u, 4);
        ownerMap[30 * 256 + 29] = Surface;
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), new IslandPostPopHooks(), ownerMap);
        Assert.Equal(0, surface.SurfaceLetter(-1, 30));
        Assert.Equal(0, surface.SurfaceLetter(30, 256));
        Assert.Equal((byte)'H', surface.SurfaceLetter(29, 30));
        ownerMap[30 * 256 + 29] = 0;
        Assert.Equal(0, surface.SurfaceLetter(29, 30));
        raw[10] = (byte)Island;
        ownerMap[30 * 256 + 29] = Surface;
        Assert.Equal(0, surface.SurfaceLetter(29, 30));
    }

    /// <summary>지도·타입·훅 오류는 생성 때 거부한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void RejectsBadMapsAndHooks()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        IslandPostPopHooks hooks = new();
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(pool, Array.Empty<ushort>(), types, frames, typeState, new IslandPostPopState(), hooks));
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(pool, surfaceMap, types[..100], frames, typeState, new IslandPostPopState(), hooks));
        RawSidPool cd = new(Capacity, false, null, OriginalEdition.Cd1072);
        OriginalTypeRecord[] cdTypes = new OriginalTypeRecord[171];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < cdTypes.Length; ++i)
        {
            cdTypes[i] = new OriginalTypeRecord();
        }

        RiftTypeFrames[] cdFrames = new RiftTypeFrames[cdTypes.Length];
        // 모든 타입 번호마다 빈 프레임 표를 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int i = 0; i < cdFrames.Length; ++i)
        {
            cdFrames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }

        BridgeConnectState badTypes = new() { IslandType = 500u, NoIslandType = NoIsland, StalagType = Stalag };
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(cd, surfaceMap, cdTypes, cdFrames, badTypes, new IslandPostPopState(), hooks));
        BridgeConnectState cdState = new() { IslandType = Island, NoIslandType = 100u, StalagType = Stalag };
        Assert.Throws<ArgumentException>(() => new RawIslandPostPop(cd, surfaceMap, cdTypes, cdFrames, cdState, new IslandPostPopState(), hooks));
    }

    /// <summary>첫 비트가 없으면 현재 글자와 무관하게 아무 파생 효과를 수행하지 않는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixWithoutFirstBitIsNoOp()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), new IslandPostPopHooks(), ownerMap);
        surface.Prefix(Surface, 0u);
        surface.Prefix(Surface, 4u);
    }

    /// <summary>첫 글자가 받침이면 생성·소유자·Pop·연결 순서로 수행한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void PrefixFirstCreatesIslandSupport()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        types[(int)Island].FootX = 3;
        types[(int)Island].FootY = 3;
        frames[(int)NoIsland] = new RiftTypeFrames(new[] { new RiftFrameCode((byte)'F', (byte)'P', 1, 0) });
        Span<byte> raw = pool.RawSlot(Surface);
        raw.Clear();
        raw[10] = (byte)NoIsland;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(30f), 4);
        Put(raw, 18, BitConverter.SingleToUInt32Bits(30f), 4);
        raw[34] = 3;
        Put(raw, 36, 0u, 4);
        List<string> events = [];
        IslandPostPopHooks hooks = new()
        {
            Objects = new BridgeConnectHooks
            {
                Create = (type, flags) =>
                {
                    events.Add("C:" + type + ":" + flags);
                    Span<byte> born = pool.RawSlot(60);
                    born.Clear();
                    born[10] = (byte)type;
                    return 60;
                },
                SetOwner = (sid, owner) =>
                {
                    events.Add("O:" + sid + ":" + owner);
                    pool.RawSlot(sid)[34] = (byte)owner;
                },
                Pop = (sid, x, y, flags) => events.Add("P:" + sid + ":" + BitConverter.SingleToUInt32Bits(x) + ":" + BitConverter.SingleToUInt32Bits(y) + ":" + flags),
            },
            Connect = sid => events.Add("N:" + sid),
        };
        new RawIslandPostPop(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), hooks).Prefix(Surface, 1u);
        Assert.Equal(
        [
            "C:94:2",
            "O:60:3",
            "P:60:" + BitConverter.SingleToUInt32Bits(32f) + ":" + BitConverter.SingleToUInt32Bits(32f) + ":0",
            "N:50",
        ], events);
        Assert.Equal(3, pool.Slot(60)[34]);
    }

    /// <summary>3×3 받침·종유석에 소유자와 표시 갱신을 주고 누락은 진단한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SetSupportOwnerPropagatesAndReports()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        int sid = Surface;
        // 3×3 칸의 머리 번호를 심는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int y = 28; y <= 30; ++y)
        {
            // 한 행의 세 칸을 심는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (int x = 28; x <= 30; ++x)
            {
                Span<byte> cell = pool.RawSlot(sid);
                cell.Clear();
                cell[10] = (byte)NoIsland;
                surfaceMap[y * 256 + x] = (ushort)sid;
                ownerMap[y * 256 + x] = (ushort)sid;
                ++sid;
            }
        }

        List<string> events = [];
        int updates = 0;
        IslandPostPopHooks hooks = new()
        {
            Objects = new BridgeConnectHooks
            {
                SetOwner = (target, owner) =>
                {
                    events.Add("O:" + target + ":" + owner);
                    pool.RawSlot(target)[34] = (byte)owner;
                },
            },
            FindTypeAt = (x, y, type) => type == Island ? 70 : 71,
            UpdateDisplay = target =>
            {
                events.Add("U:" + target);
                ++updates;
            },
            IncompleteSupport = (cells, island, stalag) => events.Add("D"),
        };
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, new IslandPostPopState(), hooks, ownerMap);
        surface.SetSupportOwner(30f, 30f, 5u);
        Assert.Equal(11, updates);
        Assert.DoesNotContain("D", events);
        // 9칸·받침·종유석의 소유자를 확인하는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int check = Surface; check < Surface + 9; ++check)
        {
            Assert.Equal(5, pool.Slot(check)[34]);
        }

        ownerMap[28 * 256 + 28] = 0;
        events.Clear();
        surface.SetSupportOwner(30f, 30f, 5u);
        Assert.Contains("D", events);
    }

    /// <summary>저장 게임 값을 매 칸에서 다시 읽어 전파를 차단하거나 실행한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    [Fact]
    public void SavedGameGateBlocksHPropagation()
    {
        var (pool, types, frames, surfaceMap, ownerMap, typeState) = Scene();
        frames[(int)NoIsland] = new RiftTypeFrames(new[] { new RiftFrameCode((byte)'H', (byte)'P', 1, 0) });
        Span<byte> raw = pool.RawSlot(Surface);
        raw.Clear();
        raw[10] = (byte)NoIsland;
        Put(raw, 14, BitConverter.SingleToUInt32Bits(30f), 4);
        Put(raw, 18, BitConverter.SingleToUInt32Bits(30f), 4);
        raw[34] = 0;
        Put(raw, 36, 0u, 4);
        surfaceMap[28 * 256 + 28] = Surface;
        surfaceMap[30 * 256 + 30] = Surface;
        bool savedGame = true;
        IslandPostPopState state = new() { SavedGameSource = () => savedGame };
        List<string> events = [];
        IslandPostPopHooks hooks = new()
        {
            Objects = new BridgeConnectHooks(),
            Connect = sid => events.Add("N:" + sid),
            FindTypeAt = (x, y, type) => 0,
            UpdateDisplay = sid => events.Add("U:" + sid),
            IncompleteSupport = (cells, island, stalag) => events.Add("D"),
        };
        RawIslandPostPop surface = new(pool, surfaceMap, types, frames, typeState, state, hooks, ownerMap);
        surface.Prefix(Surface, 1u);
        Assert.Equal(["N:50"], events);
        savedGame = false;
        events.Clear();
        surface.Prefix(Surface, 1u);
        Assert.Contains("D", events);
    }

    /// <summary>실행기 풀의 마지막 번호다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const int LastSid = 127;

    /// <summary>새로 만드는 첫 번호다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const int FirstBorn = 100;

    /// <summary>새로 만들 수 있는 개수다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const int BornCount = 12;

    /// <summary>기계어 실행기의 가상 테이블 값이다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const uint Vtable = 0x11010000u;

    /// <summary>무인도 글자의 같은 순서다. 각 방향의 첫 프레임만 합성 입력으로 쓴다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private const string SurfaceLetters = "FCGBADIEH";

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

    /// <summary>같은 순서의 P1 코드 표를 만든다. 기대 프레임은 원본 fixture에서 읽는다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    /// <returns>무인도 타입의 프레임 표다.</returns>
    private static RiftTypeFrames SurfaceFrames()
    {
        List<RiftFrameCode> codes = [];
        // 방향마다 P 변형의 첫 프레임이다.
        foreach (char letter in SurfaceLetters)
        {
            codes.Add(new RiftFrameCode((byte)letter, (byte)'P', 1, 0));
        }

        return new RiftTypeFrames(codes);
    }

    /// <summary>같은 풀·해시·지도·연결로 장면 입력과 결과를 대조하는 전수 재생 장면이다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    private sealed class PostPopScene
    {
        /// <summary>객체 슬롯 풀이다.</summary>
        public RawSidPool Pool;

        /// <summary>좌표별 객체 후보를 찾는 공간 해시다.</summary>
        public RawSquidHash Hash = new();

        /// <summary>칸별 점유·통행 비트를 읽는 지도다.</summary>
        public byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 번호로 속성·발자국을 조회하는 타입 표다.</summary>
        public OriginalTypeRecord[] Types;

        /// <summary>이웃 탐색용 타입별 프레임 자료다.</summary>
        public List<IReadOnlyList<FrameCode>> BridgeFrames = [];

        /// <summary>후처리 탐색용 타입별 프레임 자료다.</summary>
        public RiftTypeFrames[] RiftFrames;

        /// <summary>이웃 탐색기다.</summary>
        public RawSquidNeighbors? Neighbors;

        /// <summary>연결 전역이다.</summary>
        public BridgeConnectState State = new();

        /// <summary>표면 지도 65,536칸이다.</summary>
        public ushort[] SurfaceMap = new ushort[65536];

        /// <summary>소유자 지도 65,536칸이다.</summary>
        public ushort[] OwnerMap = new ushort[65536];

        /// <summary>발생한 사건이다.</summary>
        public List<string> Events = [];

        /// <summary>만든 객체 수다.</summary>
        public ushort Born;

        /// <summary>패치판이면 참이다.</summary>
        public bool Patch;

        /// <summary>실행기와 같은 풀 번호를 확보한다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        public PostPopScene(OriginalEdition edition)
        {
            Patch = edition == OriginalEdition.Patch1078;
            Pool = new RawSidPool(Capacity, false, null, edition);
            // 예측·서버 목록과 무관한 번호를 입력 슬롯으로 확보하는 반복문이다.
            for (int sid = 5; sid <= LastSid; ++sid)
            {
                Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            }

            Types = new OriginalTypeRecord[Patch ? 188 : 171];
            RiftFrames = new RiftTypeFrames[Types.Length];
            // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
            for (int i = 0; i < Types.Length; ++i)
            {
                Types[i] = new OriginalTypeRecord();
                BridgeFrames.Add([]);
                RiftFrames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
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

        /// <summary>장면 입력 칸으로 슬롯·타입·체인·지도를 준비한다.</summary>
        /// <param name="columns">객체·점유·변형·소유자·전역 5칸이다.</param>
        public void Prepare(string[] columns)
        {
            Assert.Equal(5, columns.Length);
            int variant = int.Parse(columns[2], CultureInfo.InvariantCulture);
            Hash.Reset();
            Array.Clear(Spots);
            Array.Clear(SurfaceMap);
            Array.Clear(OwnerMap);
            Events.Clear();
            Born = 0;
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
                BridgeFrames[(int)input.Number] = Frames(variant);
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
                BridgeFrames[(int)type] = Frames(variant);
                float x = BitConverter.UInt32BitsToSingle(uint.Parse(n[4], CultureInfo.InvariantCulture));
                float y = BitConverter.UInt32BitsToSingle(uint.Parse(n[5], CultureInfo.InvariantCulture));
                int level = int.Parse(n[11], CultureInfo.InvariantCulture);
                ref ushort head = ref Hash.Bucket(level, x, y);
                Put(raw, 4, head, 2);
                head = (ushort)sid;
                // 표면·소유자 지도는 0레벨 버킷 머리만 둔다. 다른 레벨은 해시 체인에만 둔다.
                if (level == 0)
                {
                    int cell = (int)y * 256 + (int)x;
                    SurfaceMap[cell] = (ushort)sid;
                    OwnerMap[cell] = (ushort)sid;
                }
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

            State = new BridgeConnectState
            {
                ConnectorType = 90u,
                IslandType = 91u,
                StalagType = 92u,
                NoIslandType = 93u,
            };
            string[] globals = columns[4].Split(',');
            Assert.Equal(13, globals.Length);
            State.BridgeType = uint.Parse(globals[0], CultureInfo.InvariantCulture);
            State.Battle = globals[1] == "1";
            State.Mission = globals[2] == "1";
            // 색 표의 아홉 칸을 넣는 반복문이다.
            for (int i = 0; i < State.OwnerColors.Length; ++i)
            {
                State.OwnerColors[i] = int.Parse(globals[3 + i], CultureInfo.InvariantCulture);
            }

            Neighbors = new RawSquidNeighbors(Pool, Hash, Spots, Types, BridgeFrames);
        }

        /// <summary>원본 실행기의 대체와 같은 경계다.</summary>
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
                    Put(Pool.AllocatedBytes(sid), FrameOffset, unchecked((uint)frame), FrameWidth);
                    Events.Add("S:" + sid + ":" + unchecked((uint)frame) + ":" + flags);
                },
                NotifySurface = sid => Events.Add("N:" + sid),
            };
        }

        /// <summary>모든 원본 효과의 순서와 인자를 한 문자열로 만든다.</summary>
        /// <returns>사건 문자열이다. 없으면 "-"다.</returns>
        public string EventsText() => Events.Count == 0 ? "-" : string.Join(';', Events);

        /// <summary>만든 객체 슬롯 전체를 16진수 목록으로 만든다.</summary>
        /// <returns>쉼표로 이은 목록이다. 없으면 "-"다.</returns>
        public string BornText()
        {
            List<string> parts = [];
            // 만든 순서대로 쉼표로 잇는 반복문이다.
            for (int i = 0; i < Born; ++i)
            {
                ReadOnlySpan<byte> slot = Pool.Slot(FirstBorn + i);
                parts.Add(Convert.ToHexString(Patch ? slot : slot[..36]).ToLowerInvariant());
            }

            return parts.Count == 0 ? "-" : string.Join(',', parts);
        }

        /// <summary>실행기 풀 번호 0~127의 Adler-32다.</summary>
        /// <returns>십진수 문자열이다.</returns>
        public string PoolAdler()
        {
            int stride = Patch ? 50 : 36;
            byte[] image = new byte[(LastSid + 1) * stride];
            // 확보한 슬롯만 실제 바이트를 복사하는 반복문이다.
            for (int sid = 5; sid <= LastSid; ++sid)
            {
                Pool.Slot(sid)[..stride].CopyTo(image.AsSpan(sid * stride, stride));
            }

            return Netstorm.Tests.X86Fixture.Adler32(image).ToString(CultureInfo.InvariantCulture);
        }
    }

    /// <summary>한 판본의 후처리 관찰 행을 모두 재생한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    /// <param name="edition">판본 이름이다.</param>
    private static void Replay(string edition)
    {
        OriginalEdition version = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "islandpostpop-x86.tsv");
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
        // 각 행은 앞 행의 변경을 이어받지 않고 입력부터 다시 준비해 실행하는 반복문이다.
        foreach (string[] row in rows)
        {
            if (row[1] != edition)
            {
                continue;
            }

            string kind = row[0];
            string actual;
            try
            {
                PostPopScene scene = new(version);
                scene.Prepare(scenes[row[2]]);
                scene.RiftFrames[93] = SurfaceFrames();
                RawBridgeConnect connect = new(scene.Pool, scene.Neighbors!, scene.State, scene.Hooks());
                IslandPostPopState state = new()
                {
                    LoadingDepth = uint.Parse(row[4], CultureInfo.InvariantCulture),
                    OwnerPropagationSuppressed = row[5] == "1",
                };
                IslandPostPopHooks hooks = new()
                {
                    Objects = scene.Hooks(),
                    Connect = sid => connect.Connect(sid),
                    FindTypeAt = (x, y, type) => connect.FindTypeAt(x, y, type),
                    UpdateDisplay = sid => scene.Events.Add("U:" + sid),
                    IncompleteSupport = (cells, island, stalag) => scene.Events.Add("D"),
                };
                RawIslandPostPop surface = new(scene.Pool, scene.SurfaceMap, scene.Types, scene.RiftFrames, scene.State, state, hooks, scene.OwnerMap);
                if (kind == "SurfacePostPop")
                {
                    surface.Prefix(Surface, uint.Parse(row[3], CultureInfo.InvariantCulture));
                    scene.Events.Add("T:" + Surface + ":" + row[3]);
                    actual = scene.EventsText() + "|" + scene.BornText() + "|" + scene.PoolAdler();
                    string expected = row[6] + "|" + row[7] + "|" + row[8];
                    if (actual != expected && failures.Count < 8)
                    {
                        failures.Add($"{edition} {kind} 장면 {row[2]}: 실제 {actual} / 기대 {expected}");
                    }
                }
                else if (kind == "SupportOwner")
                {
                    ReadOnlySpan<byte> raw = scene.Pool.Slot(Surface);
                    float x = BitConverter.ToSingle(raw[14..18]);
                    float y = BitConverter.ToSingle(raw[18..22]);
                    surface.SetSupportOwner(x, y, uint.Parse(row[3], CultureInfo.InvariantCulture));
                    actual = scene.EventsText() + "|" + scene.BornText() + "|" + scene.PoolAdler();
                    string expected = row[6] + "|" + row[7] + "|" + row[8];
                    if (actual != expected && failures.Count < 8)
                    {
                        failures.Add($"{edition} {kind} 장면 {row[2]}: 실제 {actual} / 기대 {expected}");
                    }
                }
                else
                {
                    throw new InvalidOperationException("알 수 없는 행 종류");
                }
            }
            catch (Exception error)
            {
                if (failures.Count < 8)
                {
                    failures.Add($"{edition} {kind} 장면 {row[2]}: 예외 {error.Message}");
                }
            }

            counts[kind] = counts.TryGetValue(kind, out int had) ? had + 1 : 1;
        }

        Assert.True(failures.Count == 0, string.Join('\n', failures));
        Assert.Equal(402, counts.GetValueOrDefault("SurfacePostPop", 0));
        Assert.Equal(150, counts.GetValueOrDefault("SupportOwner", 0));
    }

    /// <summary>패치판의 후처리 관찰을 대조한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    [Fact]
    public void FullPatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD판의 후처리 관찰을 대조한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    [Fact]
    public void FullCdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본의 후처리 관찰을 대조한다. 2026-11-10 R4 전수 재생에서 추가됐다.</summary>
    [Fact]
    public void FullThirdX86()
    {
        Replay("original1037");
    }
}
