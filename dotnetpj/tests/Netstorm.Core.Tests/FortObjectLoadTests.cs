using System.Globalization;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// Fort 객체·내용물 적재(<see cref="RawFortObjectLoad"/>)를 독립 원본 입력과 대조한다.
/// cpppj <c>FortObjectLoadTests</c>의 장면 5종(패치·CD 전수 재생, 실제 생성자·소유자·HP·Pop·내용물, 누락·외부 풀 거부, 공유 FullDeck)을 옮겼다. (F2c-2, 2026-11-03)
/// </summary>
public sealed class FortObjectLoadTests
{
    /// <summary>고유 fixture 입력 칸 순서다. 상태·자료 입력만 공급하며 기대 owner·frame·함수는 계산하지 않는다.</summary>
    private enum Field
    {
        /// <summary>저장 타입 바이트다.</summary>
        Stored = 0,
        /// <summary>요청 타입 번호다.</summary>
        FortTypeInput = 1,
        /// <summary>청크 안 위치다(상위 4비트 x, 하위 4비트 y).</summary>
        Position = 2,
        /// <summary>타입 flags1이다.</summary>
        Flags1 = 3,
        /// <summary>타입 flags2다.</summary>
        Flags2 = 4,
        /// <summary>판본 DWORD다.</summary>
        Version = 5,
        /// <summary>미션 기본 소유자다.</summary>
        DefaultOwner = 6,
        /// <summary>요청 소유자 바이트다.</summary>
        OwnerInput = 7,
        /// <summary>편집 사용 여부다.</summary>
        Special = 8,
        /// <summary>미션 사용 여부다.</summary>
        Mission = 9,
        /// <summary>Fort 사용 여부다.</summary>
        Fort = 10,
        /// <summary>파괴 차단 여부다.</summary>
        Blocked = 11,
        /// <summary>사제 4분의 1 HP 여부다.</summary>
        Quarter = 12,
        /// <summary>난수 seed다.</summary>
        Seed = 13,
        /// <summary>프레임 입력 번호다.</summary>
        FramesInput = 14,
        /// <summary>지형 입력(0 없음, 1 지면, 2 빈 프레임 지면)이다.</summary>
        TerrainInput = 15,
        /// <summary>외부 변형 번호다.</summary>
        Mutation = 16,
        /// <summary>원점 x다.</summary>
        OriginX = 17,
        /// <summary>원점 y다.</summary>
        OriginY = 18,
        /// <summary>내용물 입력 번호다.</summary>
        ContentsInput = 19,
        /// <summary>프레임 바이트다.</summary>
        FrameInput = 20,
        /// <summary>수량 QA 바이트다.</summary>
        Qa = 21,
        /// <summary>수량 QB WORD다.</summary>
        Qb = 22,
        /// <summary>다리 프레임 바이트다.</summary>
        BridgeFrame = 23,
        /// <summary>구버전 상태 바이트다.</summary>
        Legacy = 24,
        /// <summary>공장 상태 바이트다.</summary>
        Factory = 25,
        /// <summary>Chaff 모드 여부다.</summary>
        ChaffMode = 26,
        /// <summary>Template 칸이다.</summary>
        Slot = 27,
    }

    /// <summary>native 실제 factory 효과가 있는 가변 기록값과 입력 같다.</summary>
    private const uint FortVtable = 0x13000000u;

    /// <summary>판본별 행 수다.</summary>
    private const int Cases = 609;

    /// <summary>장면 풀 용량이다.</summary>
    private const int Capacity = 24000;

    /// <summary>비정렬 DWORD를 little endian으로 읽는다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 너비(BYTE 수)다.</param>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint value = 0;
        // 낮은 바이트부터 지정 너비만 합친다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>좁은 필드에 직접 payload를 보존한다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 실제 지정 바깥 바이트만 기록한다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>float 입력을 비트 그대로 비교한다.</summary>
    /// <param name="value">바꿀 값이다.</param>
    private static uint Bits(float value)
    {
        return BitConverter.SingleToUInt32Bits(value);
    }

    /// <summary>모든 주변 바이트를 고정 소문자 hex로 관측한다.</summary>
    /// <param name="bytes">표시할 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> bytes)
    {
        const string digits = "0123456789abcdef";
        StringBuilder result = new();
        // 바이트마다 두 글자다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>쉼표 구분 입력 칸을 부호 없는 DWORD 목록으로 읽는다.</summary>
    /// <param name="text">입력 열이다.</param>
    private static List<uint> Inputs(string text)
    {
        List<uint> values = [];
        // 칸마다 부호 없는 수로 읽는다
        foreach (string field in text.Split(','))
        {
            values.Add(uint.Parse(field, CultureInfo.InvariantCulture));
        }
        return values;
    }

    /// <summary>고정 P/P·fallback·무방향·256 후보 입력을 만든다.</summary>
    /// <param name="profile">입력 번호다.</param>
    private static RiftTypeFrames Frames(uint profile)
    {
        List<RiftFrameCode> codes = [];
        if (profile == 1)
        {
            codes.Add(new RiftFrameCode((byte)'P', (byte)'P', 0, 0));
            codes.Add(new RiftFrameCode((byte)'P', (byte)'P', 1, 255));
            codes.Add(new RiftFrameCode((byte)'A', (byte)'A', 0, 0));
        }
        else if (profile == 2)
        {
            codes.Add(new RiftFrameCode((byte)'P', (byte)'A', 0, 0));
            codes.Add(new RiftFrameCode((byte)'P', (byte)'A', 1, 0));
        }
        else if (profile == 3)
        {
            codes.Add(new RiftFrameCode((byte)'P', (byte)'C', 0, 0));
            codes.Add(new RiftFrameCode((byte)'P', (byte)'D', 0, 0));
        }
        else if (profile == 4)
        {
            codes.Add(new RiftFrameCode((byte)'A', (byte)'A', 0, 0));
        }
        else if (profile == 5)
        {
            // 번호와 변형 바이트는 실제 PE fixture에서도 깎인다
            for (int index = 0; index < 300; index++)
            {
                codes.Add(new RiftFrameCode((byte)'P', (byte)'P', (byte)index, (byte)index));
            }
        }
        return new RiftTypeFrames(codes);
    }

    /// <summary>중첩 부모의 실제 ContentForm 붙임 전 목록 입력을 만든다.</summary>
    /// <param name="profile">입력 번호다.</param>
    private static FortContentList Contents(uint profile)
    {
        FortContent a = new() { StoredType = 110, Type = 110, Quantity = 9 };
        FortContent b = new() { StoredType = 111, Type = 111, Quantity = -3, ListFlags = 0x20, ListFlagsStored = true };
        FortContent nested = new() { StoredType = 112, Type = 112, Contents = [b] };
        if (profile == 1)
        {
            return new FortContentList { Items = [a, b], Complete = true };
        }
        if (profile == 2)
        {
            return new FortContentList { Items = [nested], Complete = true };
        }
        if (profile == 3)
        {
            return new FortContentList { Items = [], Complete = false };
        }
        return new FortContentList { Items = [], Complete = true };
    }

    /// <summary>native packet 값이지만 FortObject 모델로 공급한다. 실제 분기는 없는 optional 값을 쓰지 않는다.</summary>
    /// <param name="v">입력 28개 목록이다.</param>
    /// <param name="position">덮어쓸 위치 바이트다. null이면 입력 값을 쓴다.</param>
    private static FortObject Record(IReadOnlyList<uint> v, byte? position = null)
    {
        return new FortObject
        {
            Position = position ?? (byte)v[(int)Field.Position],
            StoredType = (byte)v[(int)Field.Stored],
            Type = (int)v[(int)Field.FortTypeInput],
            Frame = (byte)v[(int)Field.FrameInput],
            QuantityByte = (byte)v[(int)Field.Qa],
            QuantityWord = unchecked((short)v[(int)Field.Qb]),
            BridgeFrame = (byte)v[(int)Field.BridgeFrame],
            LegacyState = (byte)v[(int)Field.Legacy],
            FactoryState = (byte)v[(int)Field.Factory],
            Owner = (byte)v[(int)Field.OwnerInput],
            Contents = Contents(v[(int)Field.ContentsInput]),
        };
    }

    /// <summary>native 입력 경계와 같은 실제 pool 방식 자료를 공급하는 검사 장면이다.</summary>
    private sealed class FortLoadScene
    {
        /// <summary>객체 슬롯 풀이다. 생성·조회·반납에 쓰는 SID 풀이다.</summary>
        public RawSidPool Pool;

        /// <summary>좌표로 객체 정보를 찾는 공간 표시다.</summary>
        public RawSquidHash Hash = new();

        /// <summary>타입 번호로 생성·플래그를 조회하는 타입 표다. 판본에 맞는 188·171개다.</summary>
        public List<OriginalTypeRecord> Types;

        /// <summary>생성·소유자·내용물·보상 모듈용 188개 표다. CD 판본에서는 뒤에 빈 기록을 채운 복사본이다.</summary>
        /// <remarks>C#의 생성·소유자·내용물·보상 모듈은 10.78 기준 188개 가드를 두므로 CD 장면에서는 패딩 복사본을 쓴다. 로더 본체(검사 대상)는 판본에 맞는 표를 쓴다.</remarks>
        public List<OriginalTypeRecord> ModuleTypes;

        /// <summary>타입별 프레임 자료다.</summary>
        public List<RiftTypeFrames> Frames = [];

        /// <summary>원본 게임 난수다.</summary>
        public NetstormRandom Random;

        /// <summary>검사 입력과 수행 결과가 같이 쓰고 있는 실제 기본 참조 로딩 상태다.</summary>
        public FortObjectLoadState State = new();

        /// <summary>실제 객체 후처리 장부며 수행 결과 조회에 사용한다.</summary>
        public RawPostPopState Books = new();

        /// <summary>보상 계산이 읽거나 갱신하는 판본별 전역 AI 지갑이다.</summary>
        public SquidRewardState RewardState = new();

        /// <summary>위치가 흩어져도 SP를 32개의 9 슬롯 DWORD 배열로 뿌려 보관하는 방식이다.</summary>
        public RawSpStore? Store;

        /// <summary>객체의 HP를 조회·변경하는 처리다.</summary>
        public RawSquidReward Hp;

        /// <summary>입력 28개 목록이다.</summary>
        public List<uint> V;

        /// <summary>발생한 이벤트를 받는 이벤트 처리다.</summary>
        public List<string> Events = [];

        /// <summary>생성 시각이다.</summary>
        public int Born;

        /// <summary>자식 수다.</summary>
        public int Children;

        /// <summary>10.78 패치의 상태 조회용 자료다.</summary>
        public bool Patch;

        /// <summary>
        /// native 경계와 같은 고정 객체·Content 번호를 확보한다.
        /// </summary>
        /// <param name="edition">판본이다.</param>
        /// <param name="values">입력 28개 목록이다.</param>
        public FortLoadScene(OriginalEdition edition, List<uint> values)
        {
            Pool = new RawSidPool(Capacity, true, null, edition);
            Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToList();
            ModuleTypes = edition == OriginalEdition.Patch1078 ? Types : [.. Types, .. Enumerable.Range(0, 17).Select(_ => new OriginalTypeRecord())];
            Random = new NetstormRandom(values[(int)Field.Seed]);
            V = values;
            Patch = edition == OriginalEdition.Patch1078;
            // native 경계와 같은 고정 객체·Content 번호를 확보한다
            for (int sid = 5; sid <= 240; sid++)
            {
                Assert.Equal(sid, Pool.Allocate((SidAllocation)2));
            }
            // 타입 번호마다 빈 프레임 입력을 만든다. 실제 코드 입력은 아래 필요 요소에서 둔다
            for (int index = 0; index < Types.Count; index++)
            {
                Frames.Add(new RiftTypeFrames([]));
            }
            // 실제 PE에 공급한 상태 타입·HP·코드를 같은 자료로 정한다
            foreach (uint type in new[] { values[(int)Field.FortTypeInput], 101u })
            {
                Types[(int)type].Flags1 = values[(int)Field.Flags1];
                Types[(int)type].Flags2 = values[(int)Field.Flags2];
                Types[(int)type].MaxHitPoints = 803;
                Types[(int)type].FootX = Types[(int)type].FootY = 1;
                Frames[(int)type] = Frames(values[(int)Field.FramesInput]);
            }
            Types[164].Flags1 = 0x10;
            Types[164].MaxHitPoints = 803;
            Types[110].Flags1 = 0x1000;
            Types[110].ContentListFlags = 0x110;
            Types[111].Flags1 = 0x2000;
            Types[111].ContentListFlags = 0x20;
            Types[112].Flags1 = 0x20000;
            Types[112].ContentListFlags = 0x20;
            State.IslandType = 97;
            State.LargeIslandType = 98;
            State.Version = values[(int)Field.Version];
            State.LoadingDepth = 3;
            State.ChunksX = State.ChunksY = 2;
            State.Special = values[(int)Field.Special] != 0;
            State.Mission = values[(int)Field.Mission] != 0;
            State.Fort = values[(int)Field.Fort] != 0;
            State.FortDestroyBlocked = values[(int)Field.Blocked] != 0;
            State.ChaffMode = values[(int)Field.ChaffMode] != 0;
            RewardState.PriestQuarterHp = values[(int)Field.Quarter] != 0;
            // C# 보상은 패치판 가드(188개 표·비어 있지 않은 저장소)를 두므로 CD 장면에서는 쓰지 않는 저장소를 둔다.
            // HasHitPoints·MaxHitPoints는 저장소를 읽지 않으며 난수 상태 대조로 어긋남이 없음을 확인한다.
            Store = new RawSpStore(Random, () => 1u);
            Hp = new RawSquidReward(Pool, ModuleTypes, Books, RewardState, Store);
            if (values[(int)Field.TerrainInput] != 0)
            {
                const int ground = 50;
                Span<byte> raw = Pool.AllocatedBytes(ground);
                raw.Fill(0);
                Put(raw, 0, FortVtable, 4);
                raw[10] = 97;
                int x = unchecked((int)values[(int)Field.OriginX]) + (unchecked((int)values[(int)Field.Position]) >> 4);
                int y = unchecked((int)values[(int)Field.OriginY]) + (unchecked((int)values[(int)Field.Position]) & 15);
                Put(raw, 14, Bits(x), 4);
                Put(raw, 18, Bits(y), 4);
                uint frame = values[(int)Field.TerrainInput] == 2 ? 0xffffffffu : 7u;
                if (Patch)
                {
                    Put(raw, 36, frame, 4);
                }
                else
                {
                    raw[34] = (byte)frame;
                }
                Types[97].Flags2 = 2;
                Types[97].FootX = Types[97].FootY = 1;
                Hash.Bucket(0, x, y) = (ushort)ground;
            }
        }

        /// <summary>
        /// HP 보유 여부를 묻는다. CD 36바이트 슬롯 밖 읽기(40번지)는 다음 슬롯 +4번지를 본다.
        /// 원본 연속 메모리와 같은 관찰이며, C# 풀(50바이트)에서는 같은 칸 패턴을 읽어 결과가 달라진다.
        /// </summary>
        /// <param name="sid">대상 슬롯 번호다.</param>
        public bool SceneHasHitPoints(int sid)
        {
            if (Patch)
            {
                return Hp.HasHitPoints(sid);
            }
            Span<byte> slot = Pool.AllocatedBytes(sid);
            byte saved = slot[40];
            slot[40] = Pool.Slot(sid + 1)[4];
            try
            {
                return Hp.HasHitPoints(sid);
            }
            finally
            {
                Pool.AllocatedBytes(sid)[40] = saved;
            }
        }

        /// <summary>
        /// 최대 HP를 묻는다. CD extra 읽기는 <see cref="SceneHasHitPoints"/>와 같은 에뮬레이션을 쓴다.
        /// </summary>
        /// <param name="sid">대상 슬롯 번호다.</param>
        public int SceneMaxHitPoints(int sid)
        {
            if (Patch)
            {
                return Hp.MaxHitPoints(sid);
            }
            Span<byte> slot = Pool.AllocatedBytes(sid);
            byte saved = slot[40];
            slot[40] = Pool.Slot(sid + 1)[4];
            try
            {
                return Hp.MaxHitPoints(sid);
            }
            finally
            {
                Pool.AllocatedBytes(sid)[40] = saved;
            }
        }

        /// <summary>실제 경계 호출을 DWORD 순서로 기록한다.</summary>
        /// <param name="letter">사건 글자다.</param>
        /// <param name="args">값이다.</param>
        public void Event(char letter, params uint[] args)
        {
            StringBuilder text = new();
            text.Append(letter);
            // 값은 endian 변환 없이 코드 계산 없이 그대로 직렬화한다
            foreach (uint value in args)
            {
                text.Append(':');
                text.Append(value.ToString(CultureInfo.InvariantCulture));
            }
            Events.Add(text.ToString());
        }

        /// <summary>native와 같은 명시 효과만 확인하며 owner 변경된 실제 원시 상태와 실제 raw 상태를 둔다.</summary>
        public FortObjectLoadHooks Hooks()
        {
            FortObjectLoadHooks hooks = new()
            {
                Create = (type, flags) =>
                {
                    int sid = 100 + Born++;
                    Span<byte> raw = Pool.AllocatedBytes(sid);
                    // 기본 factory 경계와 같은 턴 보존 전 비교한다
                    for (int index = 0; index < raw.Length; index++)
                    {
                        raw[index] = (byte)(index * 37 + 11);
                    }
                    Put(raw, 0, FortVtable, 4);
                    raw[10] = (byte)type;
                    raw[11] = 4;
                    raw[6] = raw[7] = 0;
                    Event('C', type, flags, (uint)sid);
                    return sid;
                },
                NotifySurface = sid => Event('N', (uint)sid),
                SetState = (sid, value) => Event('S', (uint)sid, value),
                SetOwner = (sid, player) =>
                {
                    Event('O', (uint)sid, player);
                    Pool.AllocatedBytes(sid)[Patch ? 34 : 32] = (byte)player;
                    if (V[(int)Field.Mutation] == 1)
                    {
                        Pool.AllocatedBytes(sid)[10] = 101;
                    }
                    if (V[(int)Field.Mutation] == 2)
                    {
                        State.FortDestroyBlocked = true;
                    }
                },
                HasHitPoints = sid => SceneHasHitPoints(sid),
                MaxHitPoints = sid => SceneMaxHitPoints(sid),                VirtualCount = sid =>
                {
                    Event('Q', (uint)sid);
                    return unchecked((int)V[(int)Field.Qa]);
                },
                Pop = (sid, x, y, flags) =>
                {
                    Event('P', (uint)sid, Bits(x), Bits(y), flags);
                    if (V[(int)Field.Mutation] == 3)
                    {
                        Types[Pool.Slot(sid)[10]].Flags1 = 0;
                    }
                },
                Destroy = (sid, flags) => Event('D', (uint)sid, flags),
                CreateContent = (parent, type, quantity, listFlags, flags, quality) =>
                {
                    int sid = 200 + Children++;
                    Span<byte> raw = Pool.AllocatedBytes(sid);
                    raw.Fill(0);
                    Put(raw, 0, FortVtable, 4);
                    raw[10] = 6;
                    raw[11] = 8;
                    Event('I', (uint)parent, type, quantity, listFlags, flags, quality, (uint)sid);
                    return sid;
                },
                ChunkRegion = (x, y) =>
                {
                    Event('G', unchecked((uint)x), unchecked((uint)y));
                    return unchecked((sbyte)((x + y * 2) % 3));
                },
                TerritoryAllowed = (region, owner, territory) =>
                {
                    Event('A', unchecked((uint)region), owner, unchecked((uint)territory));
                    return (region + territory + unchecked((int)V[(int)Field.Qa])) % 2 == 0;
                },
                LoadingDepthChanged = depth => Assert.Equal(State.LoadingDepth, depth),
                FinishOwner = owner => Event('F', owner),
            };
            return hooks;
        }

        /// <summary>실제 관측 문자열을 같은 구분 기본 형식으로 묶는다.</summary>
        public string EventsText()
        {
            return Events.Count == 0 ? "-" : string.Join(';', Events);
        }

        /// <summary>객체 root stride 전체를 native 기록과 대조한다. 가변 Pop·contained 경계는 별도 결합 검사다.</summary>
        public string Raws()
        {
            List<string> parts = [];
            // C# 풀은 10.78 스트라이드(50바이트)만 지원하므로 CD 장면도 50바이트 풀로 돌린다.
            // 원본 CD 슬롯(36바이트) 대조는 앞부분으로 자르며, 필드 오프셋이 36 안에 있어 로더 논리는 완전히 대조된다.
            int stride = Patch ? 50 : 36;
            // 생성된 슬롯을 순서대로 hex로 둔다
            for (int index = 0; index < Born; index++)
            {
                parts.Add(Hex(Pool.Slot(100 + index)[..stride]));
            }
            return parts.Count == 0 ? "-" : string.Join(';', parts);
        }
    }

    /// <summary>전체 template fixture에서 독립 입력만 공급한다. 선택 결과·위치는 실제 Load가 계산한다.</summary>
    /// <param name="v">입력 28개 목록이다.</param>
    private static FortTemplate TemplateInput(IReadOnlyList<uint> v)
    {
        FortObject @object = Record(v, 0x21);
        FortChunk chunk = new() { Objects = [@object] };
        List<FortChunk> chunks = [chunk, chunk, chunk, chunk];
        FortChunkSection section = new() { Present = true, Version = 2, Chunks = chunks };
        FortTemplate fort = new();
        fort.Territories[0] = section;
        fort.Territories[1] = section;
        fort.Chaff = section;
        return fort;
    }

    /// <summary>단일 입력의 모든 효과·객체 raw·실제 난수·깊이·scope를 실제 PE 관찰과 비교한다.</summary>
    /// <param name="edition">들을 판본 이름이다(originals·originalCD).</param>
    private static void Replay(string edition)
    {
        string[][] rows = X86Fixture.Read("fortobjectload");
        int count = 0;
        // 각기 다른 실제 자료·원시로 만들고 callback 변경이 다음 행에 남지 않는다
        foreach (string[] row in rows)
        {
            if (row[0] != edition)
            {
                continue;
            }
            Assert.Equal(8, row.Length);
            List<uint> values = Inputs(row[2]);
            FortLoadScene scene = new(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, values);
            RawFortObjectLoad loader = new(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Random, scene.State, scene.Hooks());
            if (row[1] == "O")
            {
                loader.Object(Record(scene.V), unchecked((int)scene.V[(int)Field.OriginX]), unchecked((int)scene.V[(int)Field.OriginY]), scene.V[(int)Field.DefaultOwner]);
            }
            else
            {
                loader.Load(TemplateInput(scene.V), (int)scene.V[(int)Field.Slot], 0, 20);
            }
            string state = scene.State.Version.ToString(CultureInfo.InvariantCulture) + ',' + scene.State.LoadingDepth.ToString(CultureInfo.InvariantCulture) + ',' + scene.State.Scope.ToString(CultureInfo.InvariantCulture) + ',' + (scene.State.FortDestroyBlocked ? "1" : "0");
            bool same = scene.EventsText() == row[4] && scene.Raws() == row[5] && scene.Random.State.ToString(CultureInfo.InvariantCulture) == row[6] && state == row[7];
            Assert.True(same, "Fort object " + row[1] + " 행 " + count + " 입력 " + row[2] + "\n기대 events " + row[4] + "\n실제 events " + scene.EventsText() + "\n기대 raws " + row[5] + "\n실제 raws " + scene.Raws() + "\n기대 rng " + row[6] + " 실제 " + scene.Random.State + "\n기대 state " + row[7] + " 실제 " + state);
            count++;
        }
        Assert.Equal(Cases, count);
    }

    /// <summary>위치값의 실제 Fort 객체·내용물 전체 조회 관찰이다.</summary>
    [Fact]
    public void ReplaysPatch()
    {
        Replay("originals");
    }

    /// <summary>CD판의 BYTE 프레임·WORD HP·기본 소유 차이를 전체 조회로 별도로 대조한다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        Replay("originalCD");
    }

    /// <summary>실제 factory·owner·HP 조회·Pop9·ContentForm 연결을 실제 hash·부분 알림·검사로 확인한다.</summary>
    [Fact]
    public void ActualFactoryOwnerHpPopAndContent()
    {
        // 각 판본 모두 실제 일반 Pop·버퍼 SID 생성·ContentForm 직렬 부착을 수행한다
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            List<uint> values = Inputs(X86Fixture.Read("fortobjectload")[0][2]);
            values[(int)Field.Version] = 2;
            values[(int)Field.Flags1] = 0x23030;
            values[(int)Field.Flags2] = 0;
            values[(int)Field.ContentsInput] = 1;
            values[(int)Field.Mission] = 1;
            FortLoadScene scene = new(edition, values);
            RawSquidFactory factory = new(scene.Pool, scene.ModuleTypes);
            SquidOwnerMode mode = new() { Battle = true };
            SquidOwner owner = new(scene.Pool, scene.Types, scene.Books, mode);
            byte[] spots = new byte[65536];
            RawPop pop = new(scene.Pool, scene.Hash, spots);
            List<int> notices = [];
            int transmits = 0;
            RawContentFormHost content = new(scene.Pool, scene.ModuleTypes,
                (parent, form, reason) =>
                {
                    Assert.Equal(1u, reason);
                    Assert.Equal(0, scene.Pool.Slot(parent)[11] & 4);
                    notices.Add(form);
                },
                (sid, flags) => transmits++);
            ContainedCreateState containedState = new();
            RawContainedCreate contained = new(scene.Pool, containedState, content.BindCreate(factory, new ContainedCreateHooks
            {
                NotifySurface = sid => scene.Event('N', (uint)sid),
            }));
            FortObjectLoadHooks hooks = RawFortObjectLoad.MakeFortObjectLoadHooks(scene.Pool, factory, scene.Hp, contained, scene.Hooks());
            hooks.SetOwner = (sid, player) => owner.Set(sid, player);
            hooks.Pop = (sid, x, y, flags) =>
            {
                Assert.Equal(9u, flags);
                Assert.Equal(RawPopResult.Registered, pop.Pop(sid, scene.Types[scene.Pool.Slot(sid)[10]], 1f, 1f, x, y, flags));
            };
            RawFortObjectLoad loader = new(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Random, scene.State, hooks);
            int sid = loader.Object(Record(values), 16, 32, 2);
            // C# 풀은 10.78 스트라이드(서버 첫 번호 15000)만 지원하므로 CD 장면도 같은 풀 번호를 확인한다.
            // 원본 CD 서버 첫 번호(6000)와의 차이는 풀 모듈 범위이며 로더 동작(소유자·HP·Pop·내용물)은 판본별로 대조된다.
            Assert.Equal(RawSidPool.ServerFirst, sid);
            Assert.Equal((byte)2, scene.Pool.Slot(sid)[scene.Patch ? 34 : 32]);
            Assert.Equal(803u, Read(scene.Pool.Slot(sid), 26, scene.Patch ? 4 : 2));
            Assert.Equal(2, notices.Count);
            Assert.Equal(0, transmits);
            Assert.Equal((ushort)sid, scene.Hash.Bucket(1, 16f + (values[(int)Field.Position] >> 4), 32f + (values[(int)Field.Position] & 15)));
            RawSquidForms forms = new(scene.Pool);
            Assert.Equal(sid, forms.Parent(notices[0]));
            Assert.Equal(sid, forms.Parent(notices[1]));
        }
    }

    /// <summary>누락 필드·실제 효과·다른 풀 어댑터의 범위 오류를 임의 기본값으로 통과시키지 않는다.</summary>
    [Fact]
    public void RejectsMissingFieldsEffectsAndForeignPools()
    {
        FortLoadScene scene = new(OriginalEdition.Patch1078, Inputs(X86Fixture.Read("fortobjectload")[0][2]));
        FortObjectLoadHooks hooks = scene.Hooks();
        hooks.Pop = null;
        Assert.Throws<ArgumentException>(() => new RawFortObjectLoad(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Random, scene.State, hooks));
        RawFortObjectLoad loader = new(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Random, scene.State, scene.Hooks());
        FortObject record = Record(scene.V);
        scene.Types[(int)scene.V[(int)Field.FortTypeInput]].Flags1 = 0x20;
        FortObject frameless = new()
        {
            Position = record.Position,
            StoredType = record.StoredType,
            Type = record.Type,
            QuantityByte = record.QuantityByte,
            QuantityWord = record.QuantityWord,
            BridgeFrame = record.BridgeFrame,
            LegacyState = record.LegacyState,
            FactoryState = record.FactoryState,
            Owner = record.Owner,
            Contents = record.Contents,
        };
        Assert.Throws<InvalidOperationException>(() => loader.Object(frameless, 16, 32, 1));
        Assert.Throws<ArgumentException>(() => loader.Load(new FortTemplate(), 1, 0, 0));
        Assert.Equal(3u, scene.State.LoadingDepth);
        RawSidPool foreign = new(Capacity, true, null, OriginalEdition.Patch1078);
        RawSquidFactory factory = new(foreign, scene.Types);
        ContainedCreateState state = new();
        RawContainedCreate contained = new(scene.Pool, state, new ContainedCreateHooks
        {
            Create = (a, b) => 5,
            NotifySurface = sid => scene.Event('N', (uint)sid),
            Attach = (a, b, c) => scene.Event('N', 5u),
        });
        Assert.Throws<ArgumentException>(() => RawFortObjectLoad.MakeFortObjectLoadHooks(scene.Pool, factory, scene.Hp, contained, scene.Hooks()));
    }

    /// <summary>Pop·Contents가 같은 owner를 바꾼 뒤 실제 Fort 파괴 분기가 실제 DWORD를 다시 읽는다.</summary>
    [Fact]
    public void SharedFullDeckReadsAfterPopAndContents()
    {
        // 모든 효과 위치의 비영 실제 DWORD를 편집 화면으로 검사한다
        foreach (bool afterContents in new[] { false, true })
        {
            foreach (uint value in new uint[] { 0u, 0x80000000u, 0xffffffffu })
            {
                uint fullDeck = value != 0 ? 0u : 0xffffffffu;
                List<uint> values = Inputs(X86Fixture.Read("fortobjectload")[0][2]);
                values[(int)Field.Stored] = values[(int)Field.FortTypeInput] = 100u;
                values[(int)Field.Flags1] = afterContents ? 0x20000u : 0u;
                values[(int)Field.Flags2] = 0;
                values[(int)Field.Version] = 2;
                values[(int)Field.Fort] = 1;
                values[(int)Field.Blocked] = 1;
                values[(int)Field.Mutation] = 0;
                values[(int)Field.ContentsInput] = afterContents ? 1u : 0u;
                FortLoadScene scene = new(OriginalEdition.Patch1078, values);
                scene.State.SharedFullDeck = () => fullDeck;
                FortObjectLoadHooks hooks = scene.Hooks();
                int destroyed = 0;
                int effects = 0;
                Action<int, float, float, uint> pop = hooks.Pop!;
                hooks.Pop = (sid, x, y, flags) =>
                {
                    pop(sid, x, y, flags);
                    if (!afterContents)
                    {
                        fullDeck = value;
                        effects++;
                    }
                };
                Func<int, uint, uint, uint, uint, uint, int> createContent = hooks.CreateContent!;
                hooks.CreateContent = (parent, type, quantity, listFlags, flags, quality) =>
                {
                    int created = createContent(parent, type, quantity, listFlags, flags, quality);
                    fullDeck = value;
                    effects++;
                    return created;
                };
                hooks.Destroy = (sid, flags) =>
                {
                    Assert.Equal(0u, flags);
                    destroyed++;
                };
                RawFortObjectLoad loader = new(scene.Pool, scene.Hash, scene.Types, scene.Frames, scene.Random, scene.State, hooks);
                Assert.NotEqual(0, loader.Object(Record(values), 16, 32, 1));
                Assert.Equal(afterContents ? 2 : 1, effects);
                Assert.Equal(value != 0 ? 0 : 1, destroyed);
                Assert.Equal(value, fullDeck);
                Assert.True(scene.State.FortDestroyBlocked);
            }
        }
    }
}
