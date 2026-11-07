// 제품 전 고정 관측의 Core·Pool 전체와 하위 trace를 대조한다.
using System.Globalization;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawPlayerCoreRefresh의 갱신 순서를 검증하는 테스트 클래스다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerCoreRefreshTests
{
    /// <summary>실제 하위의 명시 중단이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class RefreshStopException : Exception
    {
    }

    /// <summary>현재 원시 입력만 소유하는 장치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private sealed class RefreshDevice
    {
        /// <summary>현재 A·B·lastKnown의 같은 전체 물리다.</summary>
        public readonly byte[][] Cores = [new byte[16], new byte[16], new byte[16]];

        /// <summary>현재 Pool 변경 전후의 두 실제 raw 표다.</summary>
        public readonly byte[][] Pools = [new byte[400], new byte[400]];

        /// <summary>명시 Player 물리 64KiB다.</summary>
        public readonly byte[] Players = new byte[65536];

        /// <summary>Core getter에 공급하는 같은 현재 슬롯이다.</summary>
        public Memory<byte> Current;

        /// <summary>Core getter에 공급하는 같은 Player 뷰다.</summary>
        public Memory<byte> PlayerView;

        /// <summary>원문 각 전역의 실제 독립 저장소다.</summary>
        public uint PoolIndex;

        /// <summary>현재 용량 DWORD다.</summary>
        public uint Capacity;

        /// <summary>현재 소유자 DWORD다.</summary>
        public uint Owner;

        /// <summary>현재 다이스 타입 DWORD다.</summary>
        public uint PlayerType;

        /// <summary>담긴 타입 DWORD다.</summary>
        public uint ContainedType = 6u;

        /// <summary>현재 known 값이다.</summary>
        public uint Known = 1u;

        /// <summary>현재 metamap 값이다.</summary>
        public uint Metamap;

        /// <summary>현재 Template 소유자다.</summary>
        public uint TemplateOwner { get; set; }

        /// <summary>현재 활성 값이다.</summary>
        public uint Active { get; set; }

        /// <summary>타입별 signed 기술 bit 입력이다.</summary>
        public readonly int[] Bits = new int[3];

        /// <summary>물리 계약 길이 part다.</summary>
        public int CoreLength;

        /// <summary>가상 반환 열 모드다.</summary>
        public int LevelMode;

        /// <summary>물리 계약·가상 반환·한 명시 변경이다.</summary>
        public string Mutation = "";

        /// <summary>같은 장치 호출 번호다.</summary>
        public readonly Dictionary<string, uint> Calls = new();

        /// <summary>중첩에서도 같은 가상 호출 번호다.</summary>
        public readonly Dictionary<(uint, uint), int> LevelCalls = new();

        /// <summary>하위 호출 순서다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>dead 입력 대상 번호다.</summary>
        public uint CurrentLevelIndex;

        /// <summary>최초 정수·물리 입력만 구성한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="values">원문 입력 14칸이다.</param>
        public RefreshDevice(string[] values)
        {
            if (values.Length != 14)
            {
                throw new InvalidOperationException("CoreRefresh 입력 칸 수");
            }

            ulong floor = ulong.Parse(values[0], CultureInfo.InvariantCulture);
            ulong mask = ulong.Parse(values[1], CultureInfo.InvariantCulture);
            ulong free = ulong.Parse(values[5], CultureInfo.InvariantCulture);
            ulong dead = ulong.Parse(values[6], CultureInfo.InvariantCulture);
            // 타입별 signed 기술 bit를 보존하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < 3; ++index)
            {
                Bits[index] = int.Parse(values[index + 2], CultureInfo.InvariantCulture);
            }

            Owner = uint.Parse(values[7], CultureInfo.InvariantCulture);
            PlayerType = uint.Parse(values[8], CultureInfo.InvariantCulture);
            Capacity = uint.Parse(values[9], CultureInfo.InvariantCulture);
            LevelMode = int.Parse(values[10], CultureInfo.InvariantCulture);
            ulong kind = ulong.Parse(values[11], CultureInfo.InvariantCulture);
            CoreLength = (int)ulong.Parse(values[12], CultureInfo.InvariantCulture);
            Mutation = values[13];
            // Native 주소 차이 패턴을 입력 그대로 재현하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int block = 0; block < 3; ++block)
            {
                long bias = block == 0 ? 0L : block == 1 ? 29L : -33786073L;
                // 한 블록 16바이트를 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (int index = 0; index < 16; ++index)
                {
                    Cores[block][index] = unchecked((byte)(41 + bias + (long)index * 11));
                }

                Cores[block][5] = (byte)(block == 0 ? floor : floor + 17ul);
            }

            // 두 Pool의 같은 필드를 원문 입력과 같이 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int block = 0; block < 2; ++block)
            {
                // 여덟 슬롯을 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (int index = 0; index < 8; ++index)
                {
                    Put(Pools[block], index * 50, 0x12080000u);
                    Pools[block][index * 50 + 10] = (byte)((index == 2 || index == 6 || index == 7) ? 152 : 6);
                    Pools[block][index * 50 + 11] = (byte)(((free >> index) & 1ul) | (((dead >> index) & 1ul) << 1));
                    Pools[block][index * 50 + 34] = 1;
                }

                Put(Pools[block], 56, 3u, 2);
                // 기술 목록의 실제 next·kind·flags 입력인 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (uint index = 3u; index <= 5u; ++index)
                {
                    Put(Pools[block], (int)(index * 50u + 4u), index == 5u ? 0u : index + 1u, 2);
                    Put(Pools[block], (int)(index * 50u + 18u), index == 5u ? 65535u : index - 2u, 2);
                    Pools[block][(int)(index * 50u + 20u)] = (byte)(((mask & (1ul << (int)(index - 3u))) != 0ul) ? 4 : 0);
                }

                if (block == 1)
                {
                    Pools[block][111] = 2;
                }
            }

            Put(Players, 180 + 156, 1u);
            Put(Players, 8 * 180 + 156, 1u);
            PlayerView = Players;
            if (kind < 3ul)
            {
                Current = Cores[(int)kind].AsMemory(0, CoreLength);
            }
        }

        /// <summary>같은 입력 물리 주소로만 현재 Core를 정규화한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>주소 이름이다.</returns>
        public string Pointer()
        {
            if (!MemoryMarshal.TryGetArray(Current, out ArraySegment<byte> segment) || segment.Array is null || segment.Array.Length == 0)
            {
                return "0";
            }

            // 바이트 내용으로 주소를 추정하지 않는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < 3; ++index)
            {
                if (ReferenceEquals(segment.Array, Cores[index]))
                {
                    return "ABC"[index].ToString();
                }
            }

            throw new InvalidOperationException("CoreRefresh 미등록 현재 Core");
        }

        /// <summary>실제 하위의 같은 종류 호출 번호를 재진입에서도 공유한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="kind">호출 종류다.</param>
        /// <param name="text">적을 사건이다.</param>
        /// <returns>종류+순서 경계다.</returns>
        /// <exception cref="RefreshStopException">선택 중단이다.</exception>
        public string Event(string kind, string text)
        {
            Trace.Add(text);
            uint count = Calls.TryGetValue(kind, out uint had) ? had + 1u : 1u;
            Calls[kind] = count;
            string boundary = kind + count;
            if (Mutation == boundary + "-stop")
            {
                throw new RefreshStopException();
            }

            return boundary;
        }

        /// <summary>callback은 현재 입력만 바꾼다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="boundary">고를 경계다.</param>
        public void Mutate(string boundary)
        {
            int dash = Mutation.IndexOf('-');
            if (dash < 0 || Mutation.Substring(0, dash) != boundary)
            {
                return;
            }

            string action = Mutation.Substring(dash + 1);
            if (action == "stop")
            {
                throw new RefreshStopException();
            }
            else if (action == "currentB")
            {
                Current = Cores[1].AsMemory(0, CoreLength);
            }
            else if (action == "cache")
            {
                Current = Cores[2].AsMemory(0, CoreLength);
                Metamap = 1u;
            }
            else if (action == "poolB")
            {
                PoolIndex = 1u;
            }
            else if (action == "cap2")
            {
                Capacity = 2u;
            }
            else if (action == "known0")
            {
                Known = 0u;
            }
            else if (action == "owner8")
            {
                Owner = 8u;
            }
            else if (action == "owner0")
            {
                Owner = 0u;
            }
            else if (action == "contained152")
            {
                ContainedType = 152u;
            }
            else if (action == "playerType6")
            {
                PlayerType = 6u;
            }
            else if (action == "dead")
            {
                Pools[PoolIndex][(int)(CurrentLevelIndex * 50u + 11u)] = 2;
            }
            else if (action != "reenter")
            {
                throw new InvalidOperationException("CoreRefresh 미해석 명시 변경");
            }
        }

        /// <summary>선택한 진단은 현재 변경 후 복귀하거나 그 위치에서 중단한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="line">진단 행 번호다.</param>
        public void Assertion(uint line)
        {
            string boundary = Event("a", "A:" + line);
            Mutate(boundary);
        }

        /// <summary>기존 실제 몸체를 사용한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>현재 Core 메모리다.</returns>
        public Memory<byte> Core()
        {
            string boundary = Event("g", "G:" + Pointer());
            Mutate(boundary);
            PlayerCoreDataGlobals globals = new()
            {
                TemplateOwner = TemplateOwner,
                BattleOwner = Owner,
                PlayerKnown = Known,
                Active = Active,
                Metamap = Metamap,
                Current = Current,
                Cached = Cores[2],
                Players = PlayerView,
            };
            return RawPlayerCoreData.ReadPlayerCoreData(globals, new PlayerCoreDataHooks { Assertion = Assertion });
        }

        /// <summary>원문 Player+9c 하위 경계다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>기술 부모 값이다.</returns>
        public uint TechnologyParent()
        {
            string boundary = Event("p", "P");
            Mutate(boundary);
            if (Known == 0u)
            {
                return 0u;
            }

            return RefreshRead(Players, (int)(Owner * 180u + 156u), 4);
        }

        /// <summary>독립 원시 순회 장치의 next·current는 Begin별로 따로 소유한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="parent">부를 부모 번호다.</param>
        /// <returns>순회 커서다.</returns>
        public PlayerCoreRefreshCursor Begin(uint parent)
        {
            string boundary = Event("b", "B:" + parent);
            Mutate(boundary);
            uint current = 0u;
            uint next = RefreshRead(Pools[PoolIndex], (int)(parent * 50u + 6u), 2);
            void Advance()
            {
                for (;;)
                {
                    current = next;
                    if (current == 0u)
                    {
                        next = 0u;
                        return;
                    }

                    uint offset = current * 50u;
                    next = RefreshRead(Pools[PoolIndex], (int)(offset + 4u), 2);
                    if (RefreshRead(Pools[PoolIndex], (int)(offset + 10u), 1) == ContainedType)
                    {
                        return;
                    }
                }
            }

            Advance();
            return new PlayerCoreRefreshCursor
            {
                Current = () => current,
                Next = () =>
                {
                    string nested = Event("n", "N");
                    Mutate(nested);
                    Advance();
                },
            };
        }

        /// <summary>가상 level의 명시 반환만 공급한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="form">고를 폼이다.</param>
        /// <returns>명시 값이다.</returns>
        public int Level(PlayerCoreRefreshForm form)
        {
            uint block = form.Address >= 0x12010000u ? 1u : 0u;
            uint index = (form.Address - (block != 0u ? 0x12010000u : 0x12000000u)) / 50u;
            CurrentLevelIndex = index;
            bool second = (LevelCalls.TryGetValue((block, index), out int had) ? had + 1 : 1) % 2 == 0;
            LevelCalls[(block, index)] = (LevelCalls.TryGetValue((block, index), out int before) ? before : 0) + 1;
            int value = unchecked((int)(unchecked(4u - (index - 2u) % 3u) + block));
            if (LevelMode == 1 && second)
            {
                value = -3;
            }
            else if (LevelMode == 2 && second)
            {
                value = int.MinValue;
            }
            else if (LevelMode == 3)
            {
                value = int.MaxValue;
            }
            else if (LevelMode == 4)
            {
                value = -1;
            }

            AssertSameBacking(form, block, index);
            string boundary = Event("v", "V:" + block + ":" + index + ":" + value);
            Mutate(boundary);
            if (Mutation == boundary + "-reenter")
            {
                Trace.Add("R:begin");
                RawPlayerCoreRefresh.RefreshPlayerCore(Hooks());
                Trace.Add("R:end");
            }

            return value;
        }

        /// <summary>폼이 같은 Pool 실체를 가리키는지 확인한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="form">확인할 폼이다.</param>
        /// <param name="block">기대 블록이다.</param>
        /// <param name="index">기대 번호다.</param>
        private void AssertSameBacking(PlayerCoreRefreshForm form, uint block, uint index)
        {
            if (MemoryMarshal.TryGetArray(form.Bytes, out ArraySegment<byte> segment) && segment.Array is not null && segment.Array.Length != 0)
            {
                Assert.True(ReferenceEquals(segment.Array, Pools[block]) && segment.Offset == (int)(index * 50u));
            }
        }

        /// <summary>기존 실제 Core helper와 부모가 고른 하위장치만 연결한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>훅 묶음이다.</returns>
        public PlayerCoreRefreshHooks Hooks()
        {
            return new PlayerCoreRefreshHooks
            {
                Core = Core,
                TechnologyParent = TechnologyParent,
                Begin = Begin,
                Pool = () => new PlayerCoreRefreshPool { Bytes = Pools[PoolIndex], Address = PoolIndex != 0u ? 0x12010000u : 0x12000000u, Capacity = Capacity },
                TechnologyBit = kind => { string boundary = Event("t", "T:" + kind); Mutate(boundary); return Bits[kind == 1u ? 0 : kind == 2u ? 1 : 2]; },
                PlayerType = () => PlayerType,
                Owner = () => Owner,
                Level = Level,
                Assertion = Assertion,
            };
        }

        /// <summary>현재 전역·모든 Core와 Pool·trace를 원시 기대칸과 같은 순서로 출력한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>상태 문자열이다.</returns>
        public string State() => $"{PoolIndex},{Capacity},{Owner},{PlayerType},{ContainedType},{Known},{Metamap}";

        /// <summary>물리 세 벌 전체를 비교한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>16진수 문자열이다.</returns>
        public string CoresHex() => Hex(Cores[0]) + Hex(Cores[1]) + Hex(Cores[2]);

        /// <summary>두 Pool 전체를 비교한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>16진수 문자열이다.</returns>
        public string PoolsHex() => Hex(Pools[0]) + Hex(Pools[1]);

        /// <summary>순서·중복 호출을 보존한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <returns>합친 trace다.</returns>
        public string TraceText() => Trace.Count == 0 ? "-" : string.Join(';', Trace);

        /// <summary>같은 little endian 입력을 실제 물리에 직접 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="bytes">쓸 저장소다.</param>
        /// <param name="offset">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        /// <param name="width">쓰는 폭이다.</param>
        public static void Put(byte[] bytes, int offset, uint value, int width = 4)
        {
            // 지정 폭만 쓰는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < width; ++index)
            {
                bytes[offset + index] = (byte)(value >> (index * 8));
            }
        }

        /// <summary>장치가 필요로 하는 원시 WORD·바이트만 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="bytes">읽을 저장소다.</param>
        /// <param name="offset">읽을 위치다.</param>
        /// <param name="width">읽을 폭이다.</param>
        /// <returns>조합한 값이다.</returns>
        public static uint RefreshRead(byte[] bytes, int offset, int width)
        {
            uint value = 0u;
            // 장치 입력의 실제 바이트만 조합하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int index = 0; index < width; ++index)
            {
                value |= (uint)bytes[offset + index] << (index * 8);
            }

            return value;
        }

        /// <summary>현재 Core·Pool의 미작성 바이트까지 전부 비교한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="bytes">바꿀 바이트다.</param>
        /// <returns>소문자 16진수다.</returns>
        public static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    }

    /// <summary>논리 쓰기 정지 24행은 별도 관측이고 실제 제품과 같다고 세지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ReplaysIndependentPe618SupportedObservations()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-core-refresh-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int row = 0;
        int compared = 0;
        int excluded = 0;
        // 최초 642행을 빠짐없이 분류하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string rowText = line.EndsWith('\r') ? line.Substring(0, line.Length - 1) : line;
            ++row;
            string[] values = rowText.Split('\t');
            Assert.Equal(9, values.Length);
            if (values[2] == "fault")
            {
                ++excluded;
                continue;
            }

            RefreshDevice device = new(values[1].Split(','));
            string status = "normal";
            try
            {
                RawPlayerCoreRefresh.RefreshPlayerCore(device.Hooks());
            }
            catch (RefreshStopException)
            {
                status = "stop";
            }
            catch (ArgumentOutOfRangeException)
            {
                if (RawPlayerCoreData.HasAddress(device.Current))
                {
                    throw;
                }

                status = "fault-null";
            }

            ++compared;
            Assert.Equal(values[2], status);
            Assert.Equal(values[3], device.Pointer());
            Assert.Equal(values[4], device.State());
            Assert.Equal(values[5], device.CoresHex());
            Assert.Equal(values[6], device.PoolsHex());
            Assert.Equal(values[7], device.TraceText());
        }

        Assert.Equal(642, row);
        Assert.Equal(618, compared);
        Assert.Equal(24, excluded);
    }

    /// <summary>짧은 host 뷰는 단일 DWORD 쓰기 전에 거부한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ShortCoreRejectsWholeStoreAndMissingLaterDeviceKeepsZero()
    {
        // 실제 10바이트 미만의 모든 host 뷰를 검증하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int length = 0; length < 10; ++length)
        {
            byte[] bytes = new byte[16];
            Array.Fill(bytes, (byte)0x5a);
            PlayerCoreRefreshHooks hooks = new() { Core = () => bytes.AsMemory(0, length) };
            bool rejected = false;
            try
            {
                RawPlayerCoreRefresh.RefreshPlayerCore(hooks);
            }
            catch (ArgumentOutOfRangeException)
            {
                rejected = true;
            }

            Assert.True(rejected && bytes.AsSpan().ToArray().All(b => b == 0x5a));
        }

        byte[] full = new byte[10];
        Array.Fill(full, (byte)0x5a);
        PlayerCoreRefreshHooks hooks2 = new() { Core = () => full.AsMemory() };
        bool missing = false;
        try
        {
            RawPlayerCoreRefresh.RefreshPlayerCore(hooks2);
        }
        catch (InvalidOperationException)
        {
            missing = true;
        }

        Assert.True(missing && full[5] == 0x5a && RefreshDevice.RefreshRead(full, 6, 4) == 0u);
    }

    /// <summary>bit 범위 진단 뒤 current가 바뀌어도 같은 this에 쓴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void RequiredBitDiagnosticKeepsCapturedCoreAndAllowsReentry()
    {
        RefreshDevice device = new("0,7,32,31,-1,0,0,1,152,8,0,0,10,a1-currentB".Split(','));
        RawPlayerCoreRefresh.RefreshPlayerCore(device.Hooks());
        Assert.Equal("B", device.Pointer());
        Assert.Equal(1u, RefreshDevice.RefreshRead(device.Cores[0], 6, 4));
        Assert.True((RefreshDevice.RefreshRead(device.Cores[1], 6, 4) & 0x80000000u) != 0u);
        RefreshDevice nested = new("0,7,0,31,-1,0,0,1,152,8,0,0,10,v1-reenter".Split(','));
        RawPlayerCoreRefresh.RefreshPlayerCore(nested.Hooks());
        Assert.Contains("R:begin", nested.Trace);
        Assert.Equal(3, nested.Cores[0][5]);
    }

    /// <summary>실제 guard 20행 중 부분 쓰기 4행을 구별하고 지원 16행의 물리·trace를 대조한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    [Fact]
    public void ReplaysGuard16AndSeparatesPartialStore4()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "player-core-refresh-guard-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int rows = 0;
        int compared = 0;
        int excluded = 0;
        // 최초 20행을 모두 분류하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        foreach (string line in lines)
        {
            if (string.IsNullOrEmpty(line) || line[0] == '#')
            {
                continue;
            }

            string rowText = line.EndsWith('\r') ? line.Substring(0, line.Length - 1) : line;
            string[] values = rowText.Split('\t');
            string[] args = values[1].Split(',');
            ulong length = ulong.Parse(args[0], CultureInfo.InvariantCulture);
            ulong kind = ulong.Parse(args[1], CultureInfo.InvariantCulture);
            ++rows;
            if (length == 9ul)
            {
                ++excluded;
                continue;
            }

            byte[][] tails = [new byte[16], new byte[16]];
            byte[] pool = new byte[400];
            List<string> trace = [];
            // 원문 page 마지막 16바이트의 동일 입력인 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int block = 0; block < 2; ++block)
            {
                Array.Fill(tails[block], unchecked((byte)(0x5a + block)));
                // 마지막 length 바이트를 채우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
                for (ulong index = 0; index < length; ++index)
                {
                    tails[block][16 - (int)length + (int)index] = unchecked((byte)(0x31 + block * 29 + (int)index * 11));
                }
            }

            PlayerCoreRefreshHooks hooks = new()
            {
                Core = () => { trace.Add(kind != 0ul ? "G:B" : "G:A"); return tails[(int)kind].AsMemory(16 - (int)length, (int)length); },
                TechnologyParent = () => { trace.Add("P"); return 0u; },
                Begin = parent => { trace.Add("B:" + parent); return new PlayerCoreRefreshCursor { Current = () => 0u, Next = () => { } }; },
                Pool = () => new PlayerCoreRefreshPool { Bytes = pool, Address = 0x12000000u, Capacity = 0u },
            };
            string status = "normal";
            try
            {
                RawPlayerCoreRefresh.RefreshPlayerCore(hooks);
            }
            catch (ArgumentOutOfRangeException)
            {
                status = "fault-guard";
            }

            string actualTrace = trace.Count == 0 ? "-" : string.Join(';', trace);
            ++compared;
            Assert.Equal(values[2], status);
            Assert.Equal(values[3], RefreshDevice.Hex(tails[0]) + RefreshDevice.Hex(tails[1]));
            Assert.Equal(values[4], actualTrace);
        }

        Assert.Equal(20, rows);
        Assert.Equal(16, compared);
        Assert.Equal(4, excluded);
    }
}
