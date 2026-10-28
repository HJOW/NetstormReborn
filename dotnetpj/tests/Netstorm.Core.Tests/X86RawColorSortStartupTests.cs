using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 ColorSort 시작 부모(RawColorSortStartup)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/ColorSortStartupTests.cpp 와 같다: 명시 슬롯·실패 무늬·변경으로 시작 부모 한 번을 실행한다.
/// 행마다 종류·불러옴·슬롯·실패 무늬·변경으로 장치를 세우고 상태·물리·자취를 대조한다.
/// 기준 판본은 10.78 이다 (R2d-1, 2026-10-28).
/// </summary>
public sealed class X86RawColorSortStartupTests
{
    /// <summary>원본 테스트와 같은 명시 토큰 블록 3개.</summary>
    private static readonly uint[] Blocks = [0x12001000u, 0x12006000u, 0x1200c000u];

    /// <summary>원본 테스트와 같은 명시 중단 표시. 경계 직전 변경을 흉내낸 검사 입력이며 게임 규칙과 무관하다.</summary>
    private sealed class StopException(string cause) : Exception
    {
        /// <summary>중단을 일으킨 경계 이름.</summary>
        public string Cause { get; } = cause;
    }

    /// <summary>
    /// 실제 슬롯과 블록·자취를 소유하며 원본 출력 기대가 아닌 입력·외부 효과만 구성한다.
    /// cpppj/tests/ColorSortStartupTests.cpp 의 Device와 같은 자리다.
    /// </summary>
    private sealed class StartupDevice
    {
        /// <summary>블록별 직접 vtable 저장을 관측하는 4바이트 물리 3개.</summary>
        private readonly byte[][] _blocks = [new byte[4], new byte[4], new byte[4]];

        /// <summary>생성자 진입 뒤에 읽는 같은 표시 입력 8바이트.</summary>
        private byte[] _source = [.. "original"u8];

        /// <summary>같은 Client 수명의 현재 슬롯 묶음.</summary>
        private readonly ColorSortStartupGlobals _globals = new();

        /// <summary>명시 하위 변경·중단 입력.</summary>
        private readonly string _mutation = string.Empty;

        /// <summary>호출 종류별 관측 수.</summary>
        private readonly Dictionary<string, uint> _calls = [];

        /// <summary>실제 경계 순서 자취.</summary>
        private readonly List<string> _events = [];

        /// <summary>장치 종류(startup·gradient).</summary>
        private readonly string _kind;

        /// <summary>
        /// 기록의 원시 입력만 해석하고 원본 출력을 역산하지 않는다. 입력 칸은 7개다.
        /// </summary>
        /// <param name="input">종류·불러옴·슬롯 3개·실패 무늬·변경.</param>
        public StartupDevice(string[] input)
        {
            if (input.Length != 7)
            {
                throw new InvalidOperationException("ColorSort 시작 입력 칸 수");
            }
            _kind = input[0];
            _globals.PaletteLoaded = X86Fixture.UInt(input[1]);
            _globals.Primary = Token(X86Fixture.UInt(input[2]));
            _globals.Search = Token(X86Fixture.UInt(input[3]));
            _globals.Gradient = Token(X86Fixture.UInt(input[4]));
            Profile = X86Fixture.UInt(input[5]);
            _mutation = input[6];
            _globals.Count = 0xffffffffu;
            // 미작성 물리는 독립 입력으로 보존한다
            foreach (byte[] block in _blocks)
            {
                Array.Fill(block, (byte)0xa5);
            }
        }

        /// <summary>원본 테스트와 같은 할당 실패 무늬(비트 0=0x3800·1=0x5008·2=0x5020).</summary>
        public uint Profile { get; }

        /// <summary>원본 테스트와 같은 토큰 계산. 0은 실패이며 11·22·33은 블록 번호다.</summary>
        private static uint Token(uint logical) => logical == 0 ? 0 : Blocks[(int)(logical / 11) - 1];

        /// <summary>호출 종류와 자취를 기록하고 이번 호출 번호를 붙인다.</summary>
        private string Event(string name, string text)
        {
            _events.Add(text);
            _calls.TryGetValue(name, out uint count);
            _calls[name] = count + 1;
            return name + (count + 1);
        }

        /// <summary>
        /// 외부 하위의 현재 필드 변경만 제공하며 부모 분기나 원문 기대는 계산하지 않는다.
        /// </summary>
        /// <param name="boundary">이번 선택 하위의 경계 이름.</param>
        private void Change(string boundary)
        {
            if (_mutation == boundary + "-stop")
            {
                throw new StopException(boundary);
            }
            if (_mutation == boundary + "-sort22")
            {
                _globals.Primary = Token(22);
            }
            else if (_mutation == boundary + "-sort33" || _mutation == boundary + "-current33")
            {
                _globals.Primary = Token(33);
            }
            else if (_mutation == boundary + "-search0")
            {
                _globals.Search = 0;
            }
            else if (_mutation == boundary + "-search33")
            {
                _globals.Search = Token(33);
            }
            else if (_mutation == boundary + "-count91")
            {
                _globals.Count = 91;
            }
            else if (_mutation == boundary + "-count7")
            {
                _globals.Count = 7;
            }
            else if (_mutation == boundary + "-source")
            {
                _source = [.. "changed\0"u8];
            }
        }

        /// <summary>토큰으로 같은 실제 블록을 찾는다.</summary>
        private byte[] Memory(uint token)
        {
            for (int i = 0; i < Blocks.Length; i++)
            {
                if (token == Blocks[i])
                {
                    return _blocks[i];
                }
            }
            throw new ArgumentOutOfRangeException(nameof(token), "ColorSort 검사 미등록 토큰");
        }

        /// <summary>하위 생성의 마커를 실제 생성 알고리즘과 무관한 명시 위치 입력으로 쓴다.</summary>
        private void Marker(uint token, string text)
        {
            byte[] bytes = Memory(token);
            byte[] marker = "SORT"u8.ToArray();
            if (text == "BASE")
            {
                marker = "BASE"u8.ToArray();
            }
            else if (text == "GRAD")
            {
                marker = "GRAD"u8.ToArray();
            }
            Array.Copy(marker, bytes, 4);
        }

        /// <summary>x86 스택 값을 리틀 엔디언 DWORD·QWORD로 그대로 쌓는다.</summary>
        private static void Append(List<byte> bytes, ulong value, int size)
        {
            // 표시 double 비트를 손실 없이 기록한다
            for (int i = 0; i < size; i++)
            {
                bytes.Add((byte)(value >> (i * 8)));
            }
        }

        /// <summary>부모가 호출하는 하위 경계를 관측하며 현재 슬롯 분기와 직접 저장을 원문과 같은 순서로 실행한다.</summary>
        private ColorSortStartupHooks Hooks()
        {
            var hooks = new ColorSortStartupHooks();
            hooks.Assertion = line =>
            {
                if (line != 451)
                {
                    throw new InvalidOperationException("ColorSort 진단 행 번호");
                }
                Event("assert", "A:451");
                Change("assert");
            };
            hooks.Release = token =>
            {
                string boundary = Event("free", "F:" + token);
                Change(boundary);
            };
            hooks.Allocate = amount =>
            {
                int index = amount == 0x3800 ? 0 : amount == 0x5008 ? 1 : amount == 0x5020 ? 2 : 3;
                if (index >= 3)
                {
                    throw new InvalidOperationException("ColorSort 할당 요청 크기");
                }
                uint token = (Profile & (1u << index)) != 0 ? 0u : Blocks[index];
                string boundary = Event("alloc", "N:" + amount + ":" + token);
                Change(boundary);
                return token;
            };
            hooks.Construct = (target, logical) =>
            {
                if (logical != 0x12400000u)
                {
                    throw new InvalidOperationException("ColorSort 생성 논리 포인터");
                }
                Event("sort", "C:" + target + ":" + logical + ":" + Hex(_source));
                Marker(target, "SORT");
                Change("sort");
                return _mutation == "sort-return0" ? 0u : target;
            };
            hooks.ConstructSearch = target =>
            {
                Event("search", "S:" + target + ":" + _globals.Primary);
                Marker(target, "BASE");
                Change("search");
            };
            hooks.Memory = Memory;
            hooks.Sort = (target, size, width, comparator) =>
            {
                Event("qsort", "Q:" + target + "," + size + "," + width + "," + comparator);
                Change("qsort");
            };
            hooks.ConstructGradient = (target, from, to, number, width) =>
            {
                var raw = new List<byte>();
                // 앞쪽 RGB 세 double이다
                foreach (double value in from)
                {
                    Append(raw, BitConverter.DoubleToUInt64Bits(value), 8);
                }
                // 뒤쪽 RGB 세 double이다
                foreach (double value in to)
                {
                    Append(raw, BitConverter.DoubleToUInt64Bits(value), 8);
                }
                Append(raw, number, 4);
                Append(raw, BitConverter.DoubleToUInt64Bits(width), 8);
                Event("gradient", "G:" + target + ":" + _globals.Primary + ":" + Hex(raw.ToArray()));
                Marker(target, "GRAD");
                Change("gradient");
                return _mutation == "gradient-return0" ? 0u : target;
            };
            return hooks;
        }

        /// <summary>선택 진입의 정상·명시 중단을 그대로 반환한다.</summary>
        public string Run()
        {
            ColorSortStartupHooks hooks = Hooks();
            try
            {
                if (_kind == "startup")
                {
                    RawColorSortStartup.InitializeColorSort(0x12400000u, _globals, hooks);
                }
                else
                {
                    RawColorSortStartup.InitializeColorSortGradient(_globals, hooks);
                }
            }
            catch (StopException stop)
            {
                return stop.Cause;
            }
            return "return";
        }

        /// <summary>현재 슬롯 5개의 원문 fixture 서식이다.</summary>
        public string Values() => _globals.Primary + "," + _globals.Search + "," + _globals.Gradient + "," + _globals.Count + "," + _globals.PaletteLoaded;

        /// <summary>현재 블록 prefix와 입력의 물리 전체를 모두 비교한다.</summary>
        public string Raw() => Hex(_blocks[0]) + ":" + Hex(_blocks[1]) + ":" + Hex(_blocks[2]) + ":" + Hex(_source);

        /// <summary>기록과 호출 순서를 연결한다.</summary>
        public string Trace() => string.Join(';', _events);

        /// <summary>같은 실제 바이트 서식의 원문 hex다.</summary>
        private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    }

    /// <summary>정상 100개와 부분 중단 16개의 모든 현재 슬롯·물리·자취를 최초 원문 종료와 대조한다.</summary>
    [Fact]
    public void StartupParentsMatchNative()
    {
        string[][] rows = X86Fixture.Read("colorsort-startup");
        int count = 0;
        int normal = 0;
        var failures = new List<string>();
        // 고유 입력만 계산하며 출력 칸을 입력으로 사용하지 않는다
        foreach (string[] row in rows)
        {
            if (row.Length != 6)
            {
                failures.Add("행 " + (count + 1) + ": 칸 수 " + row.Length);
                continue;
            }
            var device = new StartupDevice(row[1].Split(','));
            string status = device.Run();
            bool same = status == row[2] && device.Values() == row[3] && device.Raw() == row[4] && device.Trace() == row[5];
            if (!same)
            {
                failures.Add("행 " + (count + 1) + ": 상태 " + status + " 값 " + device.Values());
            }
            count++;
            if (status == "return")
            {
                normal++;
            }
        }
        Assert.True(failures.Count == 0, "원본 입력 " + count + "개 중 " + failures.Count + "개 불일치\n" + string.Join('\n', failures.Take(8)));
        Assert.Equal(116, count);
        Assert.Equal(100, normal);
    }

    /// <summary>비어 있지 않은 슬롯 해제의 명시 거부와 빈 할당의 전체 0 대입을 대조한다.</summary>
    [Fact]
    public void RequiredSelectedDevices()
    {
        var globals = new ColorSortStartupGlobals { Primary = 0, Search = 0, Gradient = 0, Count = 19, PaletteLoaded = 1 };
        var hooks = new ColorSortStartupHooks();
        uint allocations = 0;
        hooks.Allocate = _ => { allocations++; return 0u; };
        RawColorSortStartup.InitializeColorSort(7, globals, hooks);
        Assert.Equal(3u, allocations);
        Assert.Equal(0u, globals.Primary);
        Assert.Equal(0u, globals.Search);
        Assert.Equal(0u, globals.Gradient);
        Assert.Equal(10u, globals.Count);
        var loaded = new ColorSortStartupGlobals { Primary = 9, Search = 0, Gradient = 0, Count = 19, PaletteLoaded = 0 };
        bool rejected = false;
        try
        {
            RawColorSortStartup.InitializeColorSort(7, loaded, hooks);
        }
        catch (InvalidOperationException)
        {
            rejected = true;
        }
        Assert.True(rejected);
        Assert.Equal(9u, loaded.Primary);
        Assert.Equal(3u, allocations);
    }
}
