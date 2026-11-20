using System.Buffers.Binary;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;

// 원문 State 부모294입력의 전체 물리/현재 Core/호출 순서를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>정상 복귀와 명시 자식 중단의 부분 효과를 원문 관측 그대로 검사한다.</summary>
public sealed class StateWorldCleanupTests
{
    /// <summary>격리 PE의 실제 Player 세 물리를 구별하는 불투명 식별자다.</summary>
    private static readonly uint[] PlayerTokens = [0x12200000, 0x12202000, 0x12204000];
    /// <summary>원문 표와 같은 숫자 표현을 보존한다.</summary>
    private static readonly CultureInfo Invariant = CultureInfo.InvariantCulture;
    /// <summary>선택 자식에서 명시 중단한 입력을 다른 오류와 구분한다.</summary>
    private sealed class Stop : Exception { }
    /// <summary>원문 표가 선언한 관측 물리 범위만 읽는다.</summary>
    private static (uint Address, int Width)[] Fields(string line) => line[9..].Split(',').Select(item =>
    {
        string[] value = item.Split('.'); return (uint.Parse(value[0], NumberStyles.HexNumber, Invariant), int.Parse(value[1], Invariant));
    }).ToArray();
    /// <summary>고정 Native 원문을 한 소비자에서 읽는다.</summary>
    private static string[] Fixture() => File.ReadAllLines(Path.Combine(AppContext.BaseDirectory, "X86", "state-world-cleanup-x86.tsv"));
    /// <summary>원문이 쓰지 않은 BYTE까지 포함해 소문자 hex로 비교한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>실제 전역/Player/Core/Template/TYPE 물리와 명시 자식 효과만 제공하는 관측 장치다.</summary>
    private sealed class Device
    {
        /// <summary>선언 순서를 보존하는 원문 관측 범위다.</summary>
        private readonly (uint Address, int Width)[] _fields;
        /// <summary>Core 별칭 이외 실제 DWORD 저장소다.</summary>
        private readonly Dictionary<uint, uint> _words = [];
        /// <summary>실제 double 필드의64비트 저장소다.</summary>
        private readonly Dictionary<uint, ulong> _wides = [];
        /// <summary>동결된 실제 Core 선택 자식이 직접 변경하는 같은 전역/캐시/현재 뷰다.</summary>
        private readonly PlayerCoreDataGlobals _core = new();
        /// <summary>현재 base와 예전 포인터를 구별하는 세1620바이트다.</summary>
        public readonly byte[][] Players = [new byte[1620], new byte[1620], new byte[1620]];
        /// <summary>이웃 필드 보존을 검사하는 실제9×20바이트 Template 헤더다.</summary>
        public readonly byte[] Headers = new byte[180];
        /// <summary>이전 현재 포인터로 선택할 수 있는 실제 외부10바이트다.</summary>
        private readonly byte[] _external = new byte[10];
        /// <summary>선택된 TYPE+c4를 가진 실제 레코드다.</summary>
        public readonly OriginalTypeRecord Type = new() { Cost = BitConverter.UInt32BitsToSingle(0x3f123456) };
        /// <summary>선택 자식의 호출 횟수다.</summary>
        public readonly Dictionary<string, uint> Calls = [];
        /// <summary>종류별 정렬 없이 실제 진입 순서를 기록한다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>부모의 원시 reason 인자다.</summary>
        public readonly uint Reason;
        /// <summary>각 해제 반복이 현재 값을 읽는 실제 정지 깊이다.</summary>
        public uint Pause;
        /// <summary>오직 선택 자식의 명시 변경/중단만 기술한 입력이다.</summary>
        private readonly string _mutation;

        /// <summary>원시 입력 패턴을 최대8바이트의 little endian 비트로 만든다.</summary>
        private static ulong Input(uint address, int offset, int width, uint seed)
        {
            ulong result = 0;
            // 원문 입력 패턴을 구성하며 출력 기대를 역산하지 않는다.
            for (int index = 0; index < width; index++) result |= (ulong)unchecked((byte)(seed + address + (offset + index) * 13)) << (index * 8);
            return result;
        }
        /// <summary>전체 물리를 명시 입력으로 채운 뒤 원문 인자 필드만 덮어쓴다.</summary>
        public Device(string[] input, (uint Address, int Width)[] fields)
        {
            Assert.Equal(15, input.Length); _fields = fields; uint seed = uint.Parse(input[0], Invariant); Reason = uint.Parse(input[1], Invariant); Pause = uint.Parse(input[11], Invariant); _mutation = input[14];
            // 각 Native 관측 폭의 초기 BYTE를 보존한다.
            foreach (var (address, width) in fields)
            {
                if (width == 4) _words[address] = (uint)Input(address, 0, 4, seed);
                else if (width == 8) _wides[address] = Input(address, 0, 8, seed);
                else if (width == 10)
                {
                    Assert.Equal(0x5953f0u, address);
                    // 같은 실제 Core 캐시10바이트의 입력이다.
                    for (int index = 0; index < 10; index++) _core.Cached[index] = unchecked((byte)(seed + address + index * 13));
                }
                else if (width == 88)
                {
                    // 원문의 연속22DWORD 입력만 구성한다.
                    for (int offset = 0; offset < 88; offset += 4) _words[address + (uint)offset] = (uint)Input(address, offset, 4, seed);
                }
                else throw new InvalidOperationException("미지원State관측폭");
            }
            _core.TemplateOwner = _words[0x540cac]; _core.BattleOwner = _words[0x540c70]; _core.PlayerKnown = _words[0x595344]; _core.Active = _words[0x595348]; _core.Metamap = _words[0x594fc8];
            // 세 Player의 모든 초기 BYTE를 정의해 이전 base도 전수 대조한다.
            for (int array = 0; array < Players.Length; array++)
            {
                // 한 물리의 미작성 필드도 보존 여부를 검사한다.
                for (int index = 0; index < Players[array].Length; index++) Players[array][index] = unchecked((byte)(seed + array * 47 + index * 17));
            }
            // 외부 Core 입력은 부모가 읽기만 하며 이웃 값을 보존한다.
            for (int index = 0; index < _external.Length; index++) _external[index] = unchecked((byte)(seed + index * 7 + 31));
            _core.Players = Players[0]; _core.Current = Players[0].AsMemory(180 + 108, 10);
            uint modes = uint.Parse(input[3], Invariant); Write(0x5c85c0, uint.Parse(input[2], Invariant)); Write(0x594fb8, modes & 1); Write(0x594fbc, (modes >> 1) & 1); Write(0x594fc8, (modes >> 2) & 1);
            (uint Address, int Index)[] numbers = [(0x5c85a4,4),(0x594fa4,5),(0x5c8758,6),(0x595344,7),(0x540c70,8),(0x540cac,9),(0x594fa0,13),(0x540bdc,12)];
            // 부호 있는 defaultSP도 원문 DWORD 그대로 입력한다.
            foreach (var (address,index) in numbers) Write(address, unchecked((uint)long.Parse(input[index], Invariant)));
            Write(0x541240, 152); Write(0x5949d4, seed != 0 ? 0xffffffff : 7);
            // Template의 이웃180바이트를 같은 입력 패턴으로 구성한다.
            for (int index = 0; index < Headers.Length; index++) Headers[index] = unchecked((byte)(seed + index * 3));
            uint mask = uint.Parse(input[10], Invariant);
            // 첫 DWORD만 실제 manager 존재 입력으로 바꾼다.
            for (uint owner = 0; owner < 9; owner++) BinaryPrimitives.WriteUInt32LittleEndian(Headers.AsSpan((int)owner * 20, 4), (mask & (1u << (int)owner)) != 0 ? 0x11110000u + owner : 0u);
        }
        /// <summary>같은 실제 메모리 주소의 동일성을 구한다. 내용으로 포인터를 추정하지 않는다.</summary>
        private static bool Address(Memory<byte> value, byte[] array, int offset = 0) => MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)value, out ArraySegment<byte> segment) && ReferenceEquals(segment.Array, array) && segment.Offset == offset;
        /// <summary>현재 Player base를 격리 PE의 불투명 식별자로 표현한다.</summary>
        private uint PlayerToken()
        {
            // 등록한 실제 세 물리만 허용한다.
            for (int index = 0; index < Players.Length; index++) if (Address(_core.Players, Players[index])) return PlayerTokens[index];
            throw new InvalidOperationException("미등록현재Player물리");
        }
        /// <summary>현재 Core의 실제 같은 주소를 원문 포인터 표기로 반환한다.</summary>
        public string Pointer()
        {
            if (_core.Current.Equals(default(Memory<byte>))) return "0";
            if (Address(_core.Current, _core.Cached)) return "CACHED"; if (Address(_core.Current, _external)) return "EXTERNAL";
            // Player 배열/기록의 같은 시작 주소만 대응한다.
            for (int array = 0; array < Players.Length; array++)
            {
                // 레코드별 실제+6c 위치를 확인한다.
                for (int owner = 0; owner < 9; owner++) if (Address(_core.Current, Players[array], owner * 180 + 108)) return $"P{array}:{owner}";
            }
            throw new InvalidOperationException("미등록현재Core물리");
        }
        /// <summary>Core 자식도 직접 쓰는 같은 실제 전역을 읽는다.</summary>
        public uint Read(uint address) => address switch
        {
            0x540cac => _core.TemplateOwner, 0x540c70 => _core.BattleOwner, 0x595344 => _core.PlayerKnown, 0x595348 => _core.Active, 0x594fc8 => _core.Metamap, 0x59534c => PlayerToken(), _ => _words[address],
        };
        /// <summary>같은 전역/Core 또는 현재 Player base 슬롯에만 쓴다.</summary>
        private void Write(uint address, uint value)
        {
            switch (address)
            {
                case 0x540cac: _core.TemplateOwner = value; return; case 0x540c70: _core.BattleOwner = value; return;
                case 0x595344: _core.PlayerKnown = value; return; case 0x595348: _core.Active = value; return; case 0x594fc8: _core.Metamap = value; return;
                case 0x59534c:
                    int index = Array.IndexOf(PlayerTokens, value); if (index < 0) throw new InvalidOperationException("미등록Player토큰"); _core.Players = Players[index]; return;
                default: _words[address] = value; return;
            }
        }
        /// <summary>실제 자식의 진입 인자/순서와 주소별 호출 횟수를 기록한다.</summary>
        private string Event(uint address, params uint[] args)
        {
            string key = address.ToString("x", Invariant); Trace.Add("C:" + key + ":" + string.Join(',', args.Select(value => value.ToString(Invariant))));
            uint count = Calls.GetValueOrDefault(key) + 1; Calls[key] = count; return key + "." + count.ToString(Invariant);
        }
        /// <summary>선택한 자식 경계에서만 현재값을 바꾸거나 명시 중단한다.</summary>
        private void Mutate(string boundary)
        {
            int split = _mutation.IndexOf('-'); if (split < 0 || _mutation[..split] != boundary) return;
            switch (_mutation[(split + 1)..])
            {
                case "stop": throw new Stop(); case "client1": Write(0x5c85c0, 1); break; case "template2": Write(0x540cac, 2); break;
                case "battle1": Write(0x594fbc, 1); break; case "editor1": Write(0x5c85a4, 1); break; case "owner8": Write(0x540c70, 8); break;
                case "playerB": _core.Players = Players[1]; break; case "known0": Write(0x595344, 0); break; case "known1": Write(0x595344, 1); break;
                case "knownEditor": Write(0x595344, 1); Write(0x5c85a4, 1); break; case "defaultNegative": Write(0x540bdc, 0x80000000); break;
                case "state29": Write(0x5c8758, 29); break; case "coreExternal": _core.Current = _external; break; case "counterMax": Write(0x5949d4, 0xffffffff); break;
                default: throw new InvalidOperationException("미해석State변경");
            }
        }
        /// <summary>실제 현재 물리와 TYPE 레코드만 부모에게 빌려준다.</summary>
        public StateWorldCleanupGlobals Globals() => new()
        {
            Read32 = Read, Write32 = Write, Write16 = (address,value) => Write(address, (Read(address) & 0xffff0000u) | value), Write64Bits = (address,bits) => _wides[address] = bits,
            Players = () => _core.Players, TemplateManager = slot => BinaryPrimitives.ReadUInt32LittleEndian(Headers.AsSpan(checked((int)slot * 20), 4)),
            TypeDefault = type => { string boundary = Event(0x49a840, type); Mutate(boundary); return Type; },
        };
        /// <summary>동결된 실제 Core 선택 자식과 원문 명시 외부 효과를 제공한다.</summary>
        public StateWorldCleanupHooks Hooks() => new()
        {
            Effect = effect =>
            {
                if (effect == StateWorldCleanupEffect.ResumeGame) Pause--;
                if (effect == StateWorldCleanupEffect.InitializePlayers) { Trace.Add("BEFOREPLAYER:" + Pointer()); _core.Players = Players[2]; }
                uint[] args = effect switch { StateWorldCleanupEffect.Reset568b60 => [0x568b60], StateWorldCleanupEffect.Reset54c8a0 => [0x54c8a0], StateWorldCleanupEffect.ResetZone => [0x5c86c0], _ => [] };
                string boundary = Event((uint)effect, args); Mutate(boundary);
            },
            Disconnect = owner => Mutate(Event(0x4e2940, owner)), Assertion = line => Mutate(Event(0x4e0620, line)),
            PauseDepth = () => { uint value = Pause; Mutate(Event(0x460de0, value)); return value; },
            SaveFort = slot => Mutate(Event(0x4bf9a0, slot)), SaveBattle = slot => Mutate(Event(0x4bf880, slot)),
            ResetAi = owner => Mutate(Event(0x413ae0, owner)), ResetPriest = owner => Mutate(Event(0x492240, owner)),
            ResetOtherPlayer = (owner,value) => Mutate(Event(0x447810, owner, value)), SetSp = (owner,value) => Mutate(Event(0x40edb0, owner, BitConverter.SingleToUInt32Bits(value))),
            SelectCore = (owner,selected) => { Mutate(Event(0x48f3b0, owner, selected)); RawPlayerCoreData.SelectPlayerCoreData(_core, owner, selected); },
            ResetPool = size => Mutate(Event(0x4af170, size)), WallSeconds = () => { Mutate(Event(0x460d70)); return 123.5; },
        };
        /// <summary>원문 선언 순서의 모든 전역 BYTE를 현재 실제 값에서 직렬화한다.</summary>
        public string RawGlobals()
        {
            var result = new StringBuilder(); byte[] bytes = new byte[8];
            // 관측 범위의 원래 순서/폭과 미작성 BYTE를 모두 보존한다.
            foreach (var (address,width) in _fields)
            {
                if (width == 10) { result.Append(Hex(_core.Cached)); continue; }
                //22DWORD도 각 실제 DWORD를 현재 위치에서 조회한다.
                for (int offset = 0; offset < width;)
                {
                    int size = width == 8 ? 8 : 4;
                    if (size == 8) BinaryPrimitives.WriteUInt64LittleEndian(bytes, _wides[address]); else BinaryPrimitives.WriteUInt32LittleEndian(bytes, Read(address + (uint)offset));
                    result.Append(Hex(bytes.AsSpan(0, size))); offset += size;
                }
            }
            return result.ToString();
        }
    }
    /// <summary>두 원문 제어어의294행 전체 물리·현재 포인터·순서를 대조한다.</summary>
    [Fact]
    public void ReplaysIndependentPe294()
    {
        string[] lines = Fixture(); var fields = Fields(lines[0]); int rows = 0, normal = 0, stopped = 0;
        // 정상194행과 명시 중단100행을 생략 없이 대조한다.
        foreach (string line in lines.Skip(1))
        {
            if (line.Length == 0 || line[0] == '#') continue; string[] cells = line.Split('\t'); Assert.Equal(13, cells.Length); Assert.Contains(cells[0], new[] { "639", "895" }); Assert.True(uint.Parse(cells[12], Invariant) > 0);
            var device = new Device(cells[1].Split(','), fields); string status = "normal";
            try { RawStateWorldCleanup.CleanupStateWorld(device.Reason, device.Globals(), device.Hooks()); } catch (Stop) { status = "stop"; }
            rows++; if (status == "normal") normal++; else stopped++;
            Assert.Equal(cells[2], status); Assert.Equal(cells[3], device.Pause.ToString(Invariant)); Assert.Equal(cells[4], device.Pointer()); Assert.Equal(cells[5], device.RawGlobals());
            Assert.Equal(cells[6], Hex(device.Players[0])); Assert.Equal(cells[7], Hex(device.Players[1])); Assert.Equal(cells[8], Hex(device.Players[2]));
            byte[] cost = new byte[4]; BinaryPrimitives.WriteUInt32LittleEndian(cost, BitConverter.SingleToUInt32Bits(device.Type.Cost)); Assert.Equal(cells[9], Hex(cost)); Assert.Equal(cells[10], Hex(device.Headers)); Assert.Equal(cells[11], string.Join(';', device.Trace));
        }
        Assert.Equal(294, rows); Assert.Equal(194, normal); Assert.Equal(100, stopped);
    }
    /// <summary>선택 Core 경계 누락은 앞선 효과만 남기고 Pool/Player 재구성으로 진행하지 않는다.</summary>
    [Fact]
    public void MissingCorePreservesEarlierEffects()
    {
        var device = new Device("165,1,0,0,0,0,14,1,1,1,6,2,10000,1,none".Split(','), Fields(Fixture()[0])); StateWorldCleanupHooks hooks = device.Hooks(); hooks.SelectCore = null;
        Assert.Throws<InvalidOperationException>(() => RawStateWorldCleanup.CleanupStateWorld(device.Reason, device.Globals(), hooks)); Assert.Equal(0u, device.Read(0x595344)); Assert.Equal(0u, device.Read(0x5c89b4) & 0xffffu); Assert.Equal("P0:1", device.Pointer());
        Assert.Equal(0u, device.Calls.GetValueOrDefault("4910a0")); Assert.Equal(0u, device.Calls.GetValueOrDefault("4af170")); Assert.Equal("C:43b540:", device.Trace[^1]);
    }
    /// <summary>경고255DWORD와 이웃 물리의 경계를 보존한다.</summary>
    [Fact]
    public void WarningResetPreservesNeighborWords()
    {
        uint[] words = Enumerable.Repeat(0x12345678u, 258).ToArray(); RawStateWorldCleanup.ResetStatePlayerNoMakeWarnings(words); Assert.All(words[..255], value => Assert.Equal(0u,value)); Assert.All(words[255..], value => Assert.Equal(0x12345678u,value));
        Assert.Throws<ArgumentException>(() => RawStateWorldCleanup.ResetStatePlayerNoMakeWarnings(new uint[254]));
    }
    /// <summary>현재 spot65536바이트만 초기화하고 이웃 BYTE를 보존한다.</summary>
    [Fact]
    public void SpotResetPreservesNeighborBytes()
    {
        byte[] bytes = Enumerable.Repeat((byte)0xa5, 65540).ToArray(); RawStateWorldCleanup.ResetStateSpotMap(bytes); Assert.All(bytes[..65536], value => Assert.Equal(0,value)); Assert.All(bytes[65536..], value => Assert.Equal(0xa5,value));
        Assert.Throws<ArgumentException>(() => RawStateWorldCleanup.ResetStateSpotMap(new byte[65535]));
    }
}
