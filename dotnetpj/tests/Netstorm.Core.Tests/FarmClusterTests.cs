using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 군집의 모든 배치 진입·반환과 같은 raw 풀의 실제 단일 배치·해시 변경을 전수 대조한다
/// (cpppj <c>tests/FarmClusterTests.cpp</c>를 직역했다).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 원본 x86 fixture로 진입점 전체를 관통하므로 PORT_MAP 상태는 L1이다.</remarks>
public sealed class FarmClusterTests
{
    /// <summary>독립 실행기 입력의 지면 범위·시작 SID와 생성 SID·원본 가상 테이블 입력이다.</summary>
    private const int FarmFirst = 60;

    /// <summary>지면 범위 끝 SID다.</summary>
    private const int FarmLast = 132;

    /// <summary>지면 한 변의 칸 수다.</summary>
    private const int FarmSide = FarmLast - FarmFirst + 1;

    /// <summary>지면 시작 SID다.</summary>
    private const int FarmGround = 200;

    /// <summary>생성 시작 SID다.</summary>
    private const int FarmBorn = 10000;

    /// <summary>원본 가상 테이블 값이다.</summary>
    private const uint FarmVtable = 0x13000000u;

    /// <summary>원본이 관찰한 기록을 읽으며 기대 군집·성공 순회 값을 재계산하지 않는다.</summary>
    /// <returns>원본 행 목록이다.</returns>
    private static string[][] FarmRows()
    {
        return Netstorm.Tests.X86Fixture.Read("farmcluster");
    }

    /// <summary>원본 little endian 바이트 필드를 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 버퍼다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    /// <returns>결합한 값이다.</returns>
    private static uint Get(byte[] raw, int offset, int width = 4)
    {
        return Get((ReadOnlySpan<byte>)raw, offset, width);
    }

    /// <summary>원본 little endian 슬롯 필드를 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯이다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    /// <returns>결합한 값이다.</returns>
    private static uint Get(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // 낮은 바이트부터 결합한다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>원본 DWORD를 기록 끝에 덧붙인다.</summary>
    /// <param name="trace">쓸 기록이다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Append(List<byte> trace, uint value)
    {
        // 낮은 바이트부터 네 개를 잇는다
        for (int index = 0; index < 4; index++)
        {
            trace.Add((byte)(value >> (index * 8)));
        }
    }

    /// <summary>원본 little endian raw 필드를 쓰고 주변 바이트는 보존한다.</summary>
    /// <param name="raw">쓸 버퍼다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(byte[] raw, int offset, uint value, int width = 4)
    {
        // 원본 필드 폭의 낮은 바이트만 쓴다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본 little endian raw 필드를 슬롯 범위에 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 원본 필드 폭의 낮은 바이트만 쓴다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    /// <returns>원본 DWORD 값이다.</returns>
    private static uint Number(string value)
    {
        return uint.Parse(value, System.Globalization.CultureInfo.InvariantCulture);
    }

    /// <summary>슬롯 전체를 원본 관측의 소문자 16진수로 표현한다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <returns>16진수 문자열이다.</returns>
    private static string Hex(ReadOnlySpan<byte> bytes)
    {
        char[] digits = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'];
        char[] text = new char[bytes.Length * 2];
        // 바이트마다 두 글자를 둔다
        for (int index = 0; index < bytes.Length; index++)
        {
            text[index * 2] = digits[bytes[index] >> 4];
            text[index * 2 + 1] = digits[bytes[index] & 15];
        }
        return new string(text);
    }

    /// <summary>zlib과 같은 Adler-32다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <returns>검사 값이다.</returns>
    private static uint Adler(byte[] bytes)
    {
        uint a = 1;
        uint b = 0;
        // 각 바이트를 65521로 나눈 나머지로 누적한다
        foreach (byte value in bytes)
        {
            a = (a + value) % 65521u;
            b = (b + a) % 65521u;
        }
        return (b << 16) | a;
    }

    /// <summary>실제 단일 풀·해시·spot·타입·지역·두 난수에서 군집과 단일 배치를 조립하는 화면이다.</summary>
    private sealed class FarmScene
    {
        /// <summary>판본별 raw 슬롯이며 기록 입력용 슬롯만 직접 초기화한다.</summary>
        public readonly RawSidPool Pool;

        /// <summary>지면과 현재 가장자리 농지를 동시에 등록하는 단일 해시다.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>실제 단일 배치가 먼저 읽는 spot 바이트 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>원본 기록의 현재 타입 플래그·발자국·HP 입력이다.</summary>
        public List<OriginalTypeRecord> Types;

        /// <summary>일치 프레임과 명시 합성 랜덤 프레임 모양이다.</summary>
        public List<RiftTypeFrames> Frames;

        /// <summary>단일 프레임 선택이 소비하는 같은 전체 난수다.</summary>
        public NetstormRandom Random = new(0);

        /// <summary>실제 가장자리 농지 121·섬 97·owner 전역이다.</summary>
        public readonly FortRandomPlacementState PlacementState = new();

        /// <summary>군집 진입의 현재 군도·범위다.</summary>
        public readonly FarmClusterState State = new();

        /// <summary>지역 record와 단일 지역 검사에 같은 지도다.</summary>
        public readonly ChunkMap Chunks = new();

        /// <summary>실제 mutable 섬 100기록이다.</summary>
        public readonly byte[] IslandRecords = new byte[RawIslandStorage.Bytes];

        /// <summary>기존 목록 생성에 필요한 같은 월드 모드다.</summary>
        public FortFinishOwnerState Modes = new();

        /// <summary>군집 기록에서는 owner setter를 호출하지 않는다.</summary>
        public readonly RawPostPopList Residences = new();

        /// <summary>기존 목록의 독립 전역이다.</summary>
        public uint Revision;

        /// <summary>기존 목록의 독립 변경 값이다.</summary>
        public uint Changed;

        /// <summary>실제 지역 owner·Exists getter다.</summary>
        public RawIslandRuntime Islands;

        /// <summary>기존 실제 가장자리 농지 121·기본 대체 생성자 몸체를 같은 풀에 실행한다.</summary>
        public RawSquidFactory? Factory;

        /// <summary>완료된 실제 단일 배치 몸체다.</summary>
        public RawFortRandomPlacement? Placement;

        /// <summary>위 같은 자료를 소비하는 신규 군집이다.</summary>
        public RawFarmCluster? Cluster;

        /// <summary>원본 기본 효과의 순서·인자다. 단순 조회는 별도 실행기 통계다.</summary>
        public readonly List<string> Events = [];

        /// <summary>모든 단일 진입·반환의 DWORD 비트다.</summary>
        public readonly List<byte> Trace = [];

        /// <summary>독립 실행기의 명시 입력이다.</summary>
        public readonly List<uint> Input = [];

        /// <summary>생성 수·배치 시도·마지막 local 상태·원본 진단이다.</summary>
        public uint Born;

        /// <summary>배치 시도 횟수다.</summary>
        public uint Attempts;

        /// <summary>마지막 local 난수 상태다.</summary>
        public uint LastLocal;

        /// <summary>원본 진단 횟수다.</summary>
        public uint Assertions;

        /// <summary>현재 가상 테이블·owner·HP·프레임 폭이다.</summary>
        public readonly bool Patch;

        /// <summary>원본 영역보다 큰 풀과 필수 목록 경계를 유지한다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        public FarmScene(OriginalEdition edition)
        {
            Patch = edition == OriginalEdition.Patch1078;
            Pool = new RawSidPool(32768, false, null, edition);
            int typeCount = Patch ? 188 : 171;
            Types = [];
            Frames = [];
            // 타입 표를 판본 크기로 둔다
            for (int index = 0; index < typeCount; index++)
            {
                Types.Add(new OriginalTypeRecord());
                Frames.Add(new RiftTypeFrames([]));
            }
            Islands = new RawIslandRuntime(Pool, Hash, IslandRecords, Modes, Residences,
                () => Revision, value => Revision = value,
                () => Changed, value => Changed = value,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (sid, owner) => throw new InvalidOperationException("군집 예상 밖 거주 구역 owner"),
                    Assertion = reason => throw new InvalidOperationException("군집 예상 밖 목록 진단"),
                });
            // 지면·생성 슬롯을 미리 확보한다. 원본처럼 빈 목록 상태는 관측하지 않는다.
            // 생성 번호는 10000번부터 증가하며 기록 최대 10143번을 덮는다. 고객 영역 끝(14999번은 예약 꼬리)까지 확보한다
            for (int sid = 5; sid <= 14998; sid++)
            {
                Assert.Equal(sid, Pool.Allocate((SidAllocation)2));
            }
        }

        /// <summary>실행기 합성 입력 슬롯을 구성한다. 게임 할당기 행동의 대조로 계수하지 않는다.</summary>
        /// <param name="sid">쓸 슬롯 번호다.</param>
        /// <returns>쓸 슬롯 범위다.</returns>
        public Span<byte> Raw(int sid)
        {
            return Pool.AllocatedBytes(sid);
        }

        /// <summary>경계 효과의 원본 순서·인자를 같은 콜론 형식으로 저장한다.</summary>
        /// <param name="kind">효과 종류다.</param>
        /// <param name="values">효과 인자다.</param>
        public void Event(string kind, params uint[] values)
        {
            string text = kind;
            // 각 인자를 콜론으로 잇는다
            foreach (uint value in values)
            {
                text += ":" + value;
            }
            Events.Add(text);
        }

        /// <summary>현재 타입이 표면이면 레벨 0, 일반 1칸 가장자리 농지면 레벨 1에 등록하는 명시 공간 경계다.</summary>
        /// <param name="sid">조회할 슬롯 번호다.</param>
        /// <returns>공간 레벨이다.</returns>
        public int Level(int sid)
        {
            return (Types[(int)Pool.Slot(sid)[10]].Flags2 & 6u) != 0 ? 0 : 1;
        }

        /// <summary>실제 Unpop 경계는 현재 단어 체인에서 같은 SID를 제거한다.</summary>
        /// <param name="sid">제거할 슬롯 번호다.</param>
        public void Unhash(int sid)
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            int x = (int)BitConverter.UInt32BitsToSingle(Get(raw, 14));
            int y = (int)BitConverter.UInt32BitsToSingle(Get(raw, 18));
            int level = Level(sid);
            int scale = level == 0 ? 1 : 2;
            int current = Hash.Cell(level, x / scale, y / scale);
            int previous = 0;
            // 실제 next를 따라가며 해당 SID 이외의 후행 객체를 유지한다
            while (current != 0)
            {
                int after = (int)Get(Pool.Slot(current), 4, 2);
                if (current == sid)
                {
                    if (previous != 0)
                    {
                        Put(Raw(previous), 4, (uint)after, 2);
                    }
                    else
                    {
                        Hash.Cell(level, x / scale, y / scale) = (ushort)after;
                    }
                    Put(Raw(sid), 4, 0, 2);
                    return;
                }
                previous = current;
                current = after;
            }
        }

        /// <summary>현재 타입·원래 좌표를 다시 읽어 같은 해시에 등록한다.</summary>
        /// <param name="sid">등록할 슬롯 번호다.</param>
        public void Rehash(int sid)
        {
            ReadOnlySpan<byte> raw = Pool.Slot(sid);
            int x = (int)BitConverter.UInt32BitsToSingle(Get(raw, 14));
            int y = (int)BitConverter.UInt32BitsToSingle(Get(raw, 18));
            int level = Level(sid);
            int scale = level == 0 ? 1 : 2;
            ref ushort cell = ref Hash.Cell(level, x / scale, y / scale);
            Put(Raw(sid), 4, cell, 2);
            cell = (ushort)sid;
        }

        /// <summary>실제 배치의 적법·생성·가상 공간·owner 경계를 기계어 실행기와 동일한 명시 입력으로 연결한다.</summary>
        /// <returns>연결할 훅 묶음이다.</returns>
        public FortRandomPlacementHooks PlacementHooks()
        {
            FortRandomPlacementHooks hooks = new();
            hooks.MayPlace = query =>
            {
                Event("L", query.Type, query.Argument, BitConverter.SingleToUInt32Bits(query.X),
                    BitConverter.SingleToUInt32Bits(query.Y), query.Flags, query.Owner, query.Mode);
                return Input[6] != 0;
            };
            hooks.ChunkRegion = (x, y) => unchecked((sbyte)Chunks.At(x, y).Island);
            hooks.RegionOwner = region =>
            {
                Event("I", unchecked((uint)region));
                return Islands.Owner(region);
            };
            hooks.Create = (type, flags) =>
            {
                int sid = (int)(FarmBorn + Born++);
                Span<byte> raw = Raw(sid);
                // 명시 생성자 입력은 원본과 같은 잔여 내용을 제공한다
                for (int index = 0; index < raw.Length; index++)
                {
                    raw[index] = (byte)(index * 37 + 11);
                }
                Put(raw, 0, FarmVtable);
                raw[10] = (byte)type;
                raw[11] = 4;
                Put(raw, 6, 0, 2);
                Event("C", type, flags, (uint)sid);
                return sid;
            };
            hooks.Construct = (sid, type) =>
            {
                Event("K", (uint)sid, type);
                Factory!.Construct(type, sid);
            };
            hooks.HasHitPoints = sid => (Types[(int)Pool.Slot(sid)[10]].Flags1 & 0x10u) != 0;
            hooks.TypeHitPoints = type => 803;
            hooks.SetOwner = (sid, owner) =>
            {
                Event("O", (uint)sid, owner);
                Raw(sid)[Patch ? 34 : 32] = (byte)owner;
            };
            hooks.Unpop = (sid, flags) =>
            {
                Event("U", (uint)sid, flags);
                Unhash(sid);
                Raw(sid)[11] |= 4;
            };
            hooks.Repop = (sid, flags) =>
            {
                Event("R", (uint)sid, flags);
                Raw(sid)[11] &= unchecked((byte)~4u);
                Rehash(sid);
            };
            hooks.Pop = (sid, x, y, flags) =>
            {
                Event("P", (uint)sid, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), flags);
                Span<byte> raw = Raw(sid);
                Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
                raw[11] &= unchecked((byte)~4u);
                Rehash(sid);
            };
            return hooks;
        }

        /// <summary>원본 배치 진입·반환을 실제 배치의 앞뒤에서 기록한다.</summary>
        /// <returns>연결할 훅 묶음이다.</returns>
        public FarmClusterHooks ClusterHooks()
        {
            FarmClusterHooks hooks = new();
            hooks.Assertion = assertion =>
            {
                Assertions++;
                Event("A", assertion == FarmClusterAssertion.MissingIsland ? 294u : 300u);
            };
            hooks = RawFarmCluster.MakeFarmClusterPlacementHook(Pool, Placement!, hooks);
            Func<NetstormRandom, FortRandomPlaceRequest, uint> actual = hooks.Place!;
            hooks.Place = (local, request) =>
            {
                Attempts++;
                LastLocal = local.State;
                uint[] values = [LastLocal, request.Type, BitConverter.SingleToUInt32Bits(request.X),
                    BitConverter.SingleToUInt32Bits(request.Y), unchecked((uint)request.Region), request.Mode, request.Force];
                // 단일 진입의 원본 DWORD 일곱 개를 순서대로 기록한다
                foreach (uint value in values)
                {
                    Append(Trace, value);
                }
                uint result = actual(local, request);
                Append(Trace, result);
                return result;
            };
            return hooks;
        }

        /// <summary>원본 입력만 구성하며 군집 결과는 실제 두 복원 몸체가 만든다.</summary>
        /// <param name="row">원본 행이다.</param>
        public void Prepare(string[] row)
        {
            Input.Clear();
            Events.Clear();
            Trace.Clear();
            Born = 0;
            Attempts = 0;
            LastLocal = 0;
            Assertions = 0;
            Hash.Reset();
            Array.Fill(IslandRecords, (byte)0);
            Modes = new FortFinishOwnerState();
            int typeCount = Patch ? 188 : 171;
            Types = [];
            Frames = [];
            // 원본 DWORD 비트를 그대로 읽는다
            foreach (string number in row[1].Split(','))
            {
                Input.Add(Number(number));
            }
            Assert.Equal(12, Input.Count);
            Random.SetState(Input[8]);
            State.Archipelago = Input[3] != 0;
            State.ArchipelagoSpan = unchecked((int)Input[4]);
            Array.Fill(Spots, (byte)(Input[6] != 0 ? 1 : 0));
            // 타입 표를 판본 크기로 둔다
            for (int index = 0; index < typeCount; index++)
            {
                Types.Add(new OriginalTypeRecord());
                Frames.Add(new RiftTypeFrames([]));
            }
            Types[97].Flags1 = 0x10;
            Types[97].Flags2 = 2;
            Types[97].FootX = 1;
            Types[97].FootY = 1;
            Types[121].Flags1 = Input[5] != 0 ? 0x100010u : 0x200010u;
            Types[121].Flags2 = 0x80000u;
            Types[121].FootX = 1;
            Types[121].FootY = 1;
            Types[121].ConstructorAddress = Input[11] != 0 ? OriginalConstructors.Address(Pool.Edition, 121) : 0;
            Frames[121] = new RiftTypeFrames([new RiftFrameCode(80, 80, 0, 0), new RiftFrameCode(80, 80, 1, 255), new RiftFrameCode(65, 65, 0, 0)]);
            // 같은 정수 좌표 지면을 실제 raw·머리·hash0에 배치한다
            for (int x = FarmFirst; x <= FarmLast; x++)
            {
                // x열 우선 SID를 y 순서대로 등록한다
                for (int y = FarmFirst; y <= FarmLast; y++)
                {
                    int sid = FarmGround + (x - FarmFirst) * FarmSide + (y - FarmFirst);
                    Span<byte> raw = Raw(sid);
                    raw.Fill(0);
                    Put(raw, 0, FarmVtable);
                    raw[10] = 97;
                    Put(raw, 14, BitConverter.SingleToUInt32Bits((float)x));
                    Put(raw, 18, BitConverter.SingleToUInt32Bits((float)y));
                    Put(raw, Patch ? 36 : 34, 7, Patch ? 4 : 1);
                    raw[Patch ? 34 : 32] = (byte)Input[7];
                    Hash.Cell(0, x, y) = (ushort)sid;
                }
            }
            // 현재 지역 record의 시드 바이트는 별도 입력이다
            for (int x = 0; x < 16; x++)
            {
                // 모든 덩어리가 같은 현재 지역을 조회한다
                for (int y = 0; y < 16; y++)
                {
                    ref ChunkEntry entry = ref Chunks.At(x, y);
                    entry.Island = (byte)(Input[9] != 0 ? Input[1] : 127);
                    entry.Record = [0, 0, 0, (byte)Input[2], 0, 0];
                }
            }
            Put(IslandRecords, 0, 100);
            // 기존 owner·Exists가 읽는 실제 100기록이다
            for (int region = 0; region < 100; region++)
            {
                Put(IslandRecords, 4 + region * 32 + 4, Input[10]);
                Put(IslandRecords, 4 + region * 32 + 8, Input[7]);
            }
            Factory = new RawSquidFactory(Pool, FactoryTypes());
            Placement = new RawFortRandomPlacement(Pool, Hash, Spots, Types, Frames, Random, PlacementState, PlacementHooks());
            Cluster = new RawFarmCluster(Pool, Chunks, Islands, PlacementState, State, ClusterHooks());
        }

        /// <summary>실제 생성자는 10.78 표 188개 가드를 두므로 CD 장면에서는 뒤에 빈 기록을 채운 복사본을 쓴다.</summary>
        /// <returns>생성자용 타입 표다.</returns>
        private List<OriginalTypeRecord> FactoryTypes()
        {
            // 로더 본체가 쓰는 판본 표와 달리 생성자는 패딩 복사본으로 둔다(F2c-2 선례)
            List<OriginalTypeRecord> padded = new(Types);
            while (padded.Count < OriginalConstructors.TypeCount)
            {
                padded.Add(new OriginalTypeRecord());
            }
            return padded;
        }

        /// <summary>실제 변경된 지면·생성 객체의 전체 raw를 원본과 같은 형식으로 읽는다.</summary>
        /// <returns>바뀐 슬롯의 번호·16진수 목록이다.</returns>
        public string ChangedRaw()
        {
            // C# 풀은 10.78 스트라이드(50바이트) 전용이므로 CD 장면도 50바이트 풀로 돌린다.
            // 원본 CD 슬롯(36바이트) 대조는 앞부분으로 자르고, 로더가 쓰는 오프셋이 모두 36 안에 있어 로더 논리는 완전히 대조된다(F2c-2 선례)
            int stride = Patch ? 50 : 36;
            string result = "";
            // 군집 뒤 타입이 달라진 실제 지면만 읽는다
            for (int number = FarmGround; number < FarmGround + FarmSide * FarmSide; number++)
            {
                ReadOnlySpan<byte> raw = Pool.Slot(number);
                if (raw[10] != 97)
                {
                    if (result.Length != 0)
                    {
                        result += ';';
                    }
                    result += number + ":" + Hex(raw[..stride]);
                }
            }
            // 실제 생성자 경계가 제공한 새 슬롯도 포함한다
            for (uint number = FarmBorn; number < FarmBorn + Born; number++)
            {
                if (result.Length != 0)
                {
                    result += ';';
                }
                result += number + ":" + Hex(Pool.Slot((int)number)[..stride]);
            }
            return result.Length == 0 ? "-" : result;
        }
    }

    /// <summary>모든 군집 입력의 배치 시도·기록·기본 효과·raw·두 난수·assert를 비교한다.</summary>
    /// <param name="edition">원본 판본 이름이다.</param>
    /// <param name="gameOwner">참이면 공유 군도 값을 쓰는 실제 게임 보조 상태를 잇는다.</param>
    private static void ReplayFarm(string edition, bool gameOwner = false)
    {
        RawGameAuxState owner = new();
        FarmScene scene = new(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        int count = 0;
        // 실제 실행 파일에서 관찰한 모든 고유 입력을 순회한다
        foreach (string[] row in FarmRows())
        {
            if (row[0] != edition)
            {
                continue;
            }
            Assert.Equal(9, row.Length);
            scene.Prepare(row);
            if (gameOwner)
            {
                owner.Archipelago = scene.Input[3];
                scene.State.SharedArchipelago = () => owner.Archipelago;
                scene.State.Archipelago = scene.Input[3] == 0;
            }
            scene.Cluster!.Place(new FortRandomRectangle(80.25f, 80.75f, 96.625f, 96.125f),
                unchecked((int)scene.Input[1]), scene.Input[0]);
            string events = "";
            // 기록한 실제 경계 효과를 원본 순서로 결합한다
            foreach (string effect in scene.Events)
            {
                if (events.Length != 0)
                {
                    events += ';';
                }
                events += effect;
            }
            byte[] effectBytes = System.Text.Encoding.Latin1.GetBytes(events);
            uint actualAttempts = scene.Attempts;
            uint actualTrace = Adler([.. scene.Trace]);
            uint actualEffects = Adler(effectBytes);
            string actualRaw = scene.ChangedRaw();
            uint actualGlobal = scene.Random.State;
            uint actualLocal = scene.LastLocal;
            uint actualAssert = scene.Assertions;
            bool same = actualAttempts == Number(row[2]) && actualTrace == Number(row[3])
                && actualEffects == Number(row[4]) && actualRaw == row[5]
                && actualGlobal == Number(row[6]) && actualLocal == Number(row[7])
                && actualAssert == Number(row[8]);
            Assert.True(same, $"{edition} 군집 {row[1]} 시도 {actualAttempts}/{row[2]} 기록 {actualTrace}/{row[3]} 효과 {actualEffects}/{row[4]} "
                + $"전체난수 {actualGlobal}/{row[6]} 지역난수 {actualLocal}/{row[7]} 진단 {actualAssert}/{row[8]}");
            count++;
        }
        Assert.Equal(104, count);
    }

    /// <summary>패치 군집 104개를 기존 실제 단일 배치·가장자리 농지 생성자와 같은 풀에서 대조한다.</summary>
    [Fact]
    public void ReplaysPatch()
    {
        ReplayFarm("originals");
    }

    /// <summary>CD 군집 104개를 지역 record 원본 진단·실제 단일 배치·가장자리 농지 생성자와 같은 풀에서 대조한다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        ReplayFarm("originalCD");
    }

    /// <summary>필수 단일 배치·진단과 같은 월드 목록 연결이 빠지면 거부한다.</summary>
    [Fact]
    public void RejectsMissingEffectsAndOtherWorld()
    {
        FarmScene scene = new(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => new RawFarmCluster(scene.Pool, scene.Chunks, scene.Islands,
            scene.PlacementState, scene.State, new FarmClusterHooks()));
        RawSidPool other = new(32768, false, null, OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => new RawFarmCluster(other, scene.Chunks, scene.Islands,
            scene.PlacementState, scene.State, new FarmClusterHooks
            {
                Place = (random, request) => 0u,
                Assertion = reason => { },
            }));
    }

    /// <summary>실제 FarmCluster는 로그인과 별개인 현재 bb0 DWORD를 읽고 반대 bool 사본을 무시한다.</summary>
    [Fact]
    public void GameAuxFarmClusterUsesSharedArchipelagoNative104()
    {
        ReplayFarm("originals", true);
    }

    /// <summary>게임 보조 상태의 초기값·전체 되돌리기·소실 위치·자원 비트 왕복을 확인한다.</summary>
    [Fact]
    public void GameAuxStateResetsAndSharesBits()
    {
        RawGameAuxState state = new();
        // 보호 실행 파일의 초기값이다
        Assert.Equal(25u, state.RefundPercent);
        Assert.Equal(2000, state.DefaultSp);
        Assert.Equal(122u, state.GeyserType);
        Assert.Equal(97u, state.TerrainType);
        Assert.Equal(121u, state.EdgeFarmType);
        Assert.Equal(2, state.WorldGeyserMargin);
        Assert.Equal([100u, 184u, 101u, 185u], state.WorldDecorationTypes);
        // 부호와 무관하게 같은 DWORD 비트를 공유한다
        state.StoreResourcePoolBits(0x80000001u);
        Assert.Equal(-2147483647, state.ResourcePool);
        Assert.Equal(0x80000001u, state.ResourcePoolBits());
        // 기존 지역 목록을 비어 있는 입력으로 되돌린다
        state.OrdinaryGeysers = 5;
        state.Archipelago = 1;
        state.LostX = 3.0f;
        state.ResetForGeneralWorld();
        Assert.Equal(0u, state.OrdinaryGeysers);
        Assert.Equal(0, state.TotalCost);
        Assert.Equal(0, state.ResourcePool);
        Assert.Equal(25u, state.RefundPercent);
        Assert.Equal(0u, state.Archipelago);
        Assert.Equal(0.0f, state.LostX);
        Assert.Equal(122u, state.GeyserType);
        Assert.False(state.PriestQuarterHp);
        // 실제 값 저장은 Y를 X보다 먼저 실행한다
        state.ResetLostPositionForCleanup();
        Assert.Equal(BitConverter.UInt32BitsToSingle(0xc61c4000u), state.LostY);
        Assert.Equal(BitConverter.UInt32BitsToSingle(0xc61c4000u), state.LostX);
    }
}
