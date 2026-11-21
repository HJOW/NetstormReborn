using System.Globalization;
using Netstorm.Core.Original;

// 제품과 독립된 원본644관측의 전체 메모리/부분 쓰기/호출 중 현재값 변경을 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>RawChatManagerPrefix 원문과 실제 장기 할당 소유자의 검증이다.</summary>
public sealed class ChatManagerPrefixTests
{
    /// <summary>명시 하위 장치가 중단하는 입력이며 CPU/물리 fault와 구별한다.</summary>
    private sealed class ChatStop : Exception { }
    /// <summary>PE 관측의 실제 할당/현재 입력만 제공하며 제품 분기를 대신 계산하지 않는다.</summary>
    private sealed class Device
    {
        /// <summary>원본 격리 실행의 두 manager 주소다.</summary>
        private static readonly uint[] ManagerTokens = [0x12001000, 0x12004000];
        /// <summary>내부128BYTE의 두 별도 주소다.</summary>
        private static readonly uint[] BufferTokens = [0x12008000, 0x12009000];
        /// <summary>미작성 필드까지 대조할 두 manager의 실제 물리다.</summary>
        public readonly byte[][] Managers = [new byte[2712], new byte[2712]];
        /// <summary>생성자가 초기화하지 않는 두 내부 물리다.</summary>
        public readonly byte[][] Buffers = [new byte[128], new byte[128]];
        /// <summary>직접 formatter의 원본 외부 출력이다.</summary>
        public readonly byte[] Output = new byte[256];
        /// <summary>같은 호출의 입력 지역이며 초기 seed 꼬리를 유지한다.</summary>
        private readonly byte[] _temporary = new byte[256];
        /// <summary>wrapper는 직접 출력과 다른 실제 지역을 사용한다.</summary>
        private readonly byte[] _wrapperOutput = new byte[256];
        /// <summary>현재 Core+4의 명시 하위 입력이다.</summary>
        private readonly byte[] _core = new byte[10];
        /// <summary>setter의 독립 NUL 포함 입력이다.</summary>
        private readonly byte[] _source;
        /// <summary>같은 현재 manager DWORD다.</summary>
        private readonly UIntBox _current = new();
        /// <summary>같은 현재 전체 subscriber DWORD다.</summary>
        private readonly UIntBox _subscriber = new();
        /// <summary>같은 현재 Core 활성 DWORD다.</summary>
        private readonly UIntBox _active = new();
        /// <summary>원시 진입점·미작성 seed·랭크 입력이다.</summary>
        private readonly int _mode, _seed, _rank;
        /// <summary>명시 현재 그림 개수이며 callback이 바꿀 수 있다.</summary>
        private int _minor, _maximum;
        /// <summary>호출 반복·앞/뒤 설정 길이 입력이다.</summary>
        private readonly int _repeat, _frontLength, _backLength;
        /// <summary>할당 실패 비트와 실제 manager 요청 횟수다.</summary>
        private uint _profile, _allocations;
        /// <summary>지정 경계의 현재값 변경/명시 중단 입력이다.</summary>
        private readonly string _mutation;
        /// <summary>종류별 callback 번호이며 반복 사이에 보존한다.</summary>
        private readonly Dictionary<string, int> _calls = new();
        /// <summary>모든 실제 호출의 순서와 인자다.</summary>
        private readonly List<string> _trace = [];
        /// <summary>signed 십진 원시 입력을 현재 문화권과 무관하게 읽는다.</summary>
        private static int Number(string value) => int.Parse(value, CultureInfo.InvariantCulture);
        /// <summary>모듈 호출 전에 PE와 같은 비0 독립 물리를 준비한다.</summary>
        public Device(string source)
        {
            string[] values = source.Split(','); Assert.Equal(13, values.Length);
            _mode = Number(values[0]); _seed = Number(values[1]); int selected = Number(values[2]); _current.Value = selected == 0 ? 0 : ManagerTokens[selected - 1];
            _active.Value = uint.Parse(values[3], CultureInfo.InvariantCulture); _rank = Number(values[4]); _subscriber.Value = uint.Parse(values[5], CultureInfo.InvariantCulture); _profile = uint.Parse(values[6], CultureInfo.InvariantCulture);
            _minor = Number(values[7]); _maximum = Number(values[8]); _repeat = Number(values[9]); _frontLength = Number(values[10]); _backLength = Number(values[11]); _mutation = values[12];
            // 원문이 쓰지 않은 BYTE도 서로 다른 두 할당에서 독립 대조한다.
            for (int allocation = 0; allocation < 2; allocation++)
            {
                // manager의 알려진 입력 패턴을 모든 바이트에 넣는다.
                for (int index = 0; index < Managers[allocation].Length; index++) Managers[allocation][index] = unchecked((byte)(_seed + index * 17 + allocation * 59));
                // 내부 버퍼의 다른 알려진 입력 패턴을 넣는다.
                for (int index = 0; index < Buffers[allocation].Length; index++) Buffers[allocation][index] = unchecked((byte)(_seed + index * 17 + allocation * 71 + 13));
            }
            Output.AsSpan().Fill(unchecked((byte)_seed)); _core[4] = unchecked((byte)_rank); _source = [];
            if (_mode == 2)
            {
                _source = new byte[_rank + 1]; _source.AsSpan(0, _rank).Fill((byte)'A');
                if (_mutation is "alias-forward" or "alias-backward") new byte[] { (byte)'A', (byte)'B', (byte)'C', (byte)'D', 0 }.CopyTo(Managers[0], _mutation == "alias-forward" ? 0x243 : 0x245);
            }
        }
        /// <summary>등록된 실제 물리만 같은 배열로 반환한다.</summary>
        private byte[] Memory(uint token)
        {
            // 각 실제 주소와 대응 배열을 같은 순서로 조회한다.
            for (int index = 0; index < 2; index++) { if (token == ManagerTokens[index]) return Managers[index]; if (token == BufferTokens[index]) return Buffers[index]; }
            throw new ArgumentOutOfRangeException(nameof(token), "Chat 실제 NULL/미등록 할당");
        }
        /// <summary>포인터의 logical 이름과 실제 offset만 표현한다.</summary>
        private static string Normalized(uint address)
        {
            if (address == 0) return "0";
            // 원본 관측과 같은 주소/범위 표현을 사용한다.
            for (int index = 0; index < 2; index++)
            {
                if (address >= ManagerTokens[index] && address - ManagerTokens[index] < RawChatManagerPrefix.ManagerBytes) return $"M{index + 1}+{address - ManagerTokens[index]}";
                if (address == BufferTokens[index]) return "B" + (index + 1);
            }
            return "0x" + address.ToString("x", CultureInfo.InvariantCulture);
        }
        /// <summary>실제 경계의 인자를 기록하고 해당 종류의 번호를 반환한다.</summary>
        private string Event(string kind, string text) { _trace.Add(text); int count = _calls.GetValueOrDefault(kind) + 1; _calls[kind] = count; return kind + count; }
        /// <summary>부모 밖 명시 장치가 지시한 같은 현재값만 바꾸거나 중단한다.</summary>
        private void Mutate(string boundary)
        {
            int split = _mutation.IndexOf('-'); if (split < 0 || _mutation[..split] != boundary) return;
            switch (_mutation[(split + 1)..])
            {
                case "stop": throw new ChatStop();
                case "manager0": _current.Value = 0; return;
                case "manager1": _current.Value = ManagerTokens[0]; return;
                case "manager2": _current.Value = ManagerTokens[1]; return;
                case "subscriber9": _subscriber.Value = 9; return;
                case "maximum0": _maximum = 0; return;
                case "maximumNegative": _maximum = -1; return;
                case "minor3": _minor = 3; return;
                default: throw new InvalidOperationException("Chat 미해석 변경");
            }
        }
        /// <summary>현재 슬롯/전체 subscriber/할당 시도를 원본 표기대로 출력한다.</summary>
        public string State() => $"{(_current.Value == 0 ? 0 : _current.Value == ManagerTokens[0] ? 1 : _current.Value == ManagerTokens[1] ? 2 : -1)},{_subscriber.Value},{_allocations}";
        /// <summary>callback이 없는 경우만 '-'이며 순서는 생략하지 않는다.</summary>
        public string Trace() => _trace.Count == 0 ? "-" : string.Join(';', _trace);
        /// <summary>각 원시 하위 장치의 입력/출력과 명시 변경만 제공한다.</summary>
        private ChatManagerPrefixHooks Hooks() => new()
        {
            Allocate = amount =>
            {
                uint result;
                if (amount == RawChatManagerPrefix.ManagerBytes) { _allocations++; result = (_profile & 1) != 0 ? 0 : ManagerTokens[(int)((_allocations - 1) % 2)]; }
                else if (amount == RawChatManagerPrefix.BufferBytes) result = (_profile & 2) != 0 ? 0 : BufferTokens[(int)((_allocations == 0 ? 0 : _allocations - 1) % 2)];
                else throw new InvalidOperationException("Chat 예상 밖 할당 크기");
                string boundary = Event("new", $"N:{amount}:{Normalized(result)}"); Mutate(boundary); return result;
            },
            Memory = token => Memory(token),
            Assertion = line => { string text = line switch { 220 => "address != NULL", 898 => "TheChatSystem != NULL", 2474 => "maxMinor > 0", _ => throw new InvalidOperationException("Chat 예상 밖 진단") }; string boundary = Event("assert", $"A:{line}:{text}"); Mutate(boundary); },
            RegisterCommand = (name, callback, flags) => { string boundary = Event("command", $"C:{name}:{Normalized(callback)}:{flags}"); Mutate(boundary); },
            ReadConfig = key =>
            {
                int ordinal = _calls.GetValueOrDefault("read") % 2, length = ordinal == 0 ? _frontLength : _backLength;
                string boundary = Event("read", $"R:{key}:{length}"); string? value = length < 0 ? null : new string(ordinal == 0 ? 'F' : 'G', length); Mutate(boundary); return value;
            },
            CopyFormat = (token, offset, text, count) => { string boundary = Event("strncpy", $"P:{Normalized(unchecked(token + offset))}:{count}"); byte[] bytes = Memory(token); if (offset > bytes.Length) throw new ArgumentOutOfRangeException(nameof(offset)); RawChatManagerPrefix.CopyChatManagerFormat(bytes.AsSpan((int)offset), text, count); Mutate(boundary); },
            MaximumRank = () => { int value = _maximum; string boundary = Event("max", "X:" + value); Mutate(boundary); return value; },
            MinorCount = () => { int value = _minor; string boundary = Event("minor", "I:" + value); Mutate(boundary); return value; },
            AppendIcon = (target, kind, value) => { RawChatManagerPrefix.AppendChatRankIcon(target, kind, value); string boundary = Event("append", $"F:~[Iicon.{kind}%d]:{value}"); Mutate(boundary); },
            Core = () => { string boundary = Event("core", "K"); Mutate(boundary); return _core; },
        };
        /// <summary>공개 본문만 선택하고 fault/중단 이후 재시도나 상태 복구를 하지 않는다.</summary>
        public string Run()
        {
            var hooks = Hooks(); var globals = new ChatManagerPrefixGlobals(_current, _subscriber, _active);
            try
            {
                // 호출별 지역 seed만 다시 공급하며 전역과 누적 효과는 유지한다.
                for (int invocation = 0; invocation < _repeat; invocation++)
                {
                    _temporary.AsSpan().Fill(unchecked((byte)_seed));
                    switch (_mode)
                    {
                        case 0: Assert.Equal(ManagerTokens[0], RawChatManagerPrefix.ConstructChatManager(ManagerTokens[0], hooks)); break;
                        case 1: RawChatManagerPrefix.InitializeChatManager(globals, hooks); break;
                        case 2:
                            ReadOnlySpan<byte> text = _source;
                            if (_mutation is "alias-forward" or "alias-backward") text = Managers[0].AsSpan(_mutation == "alias-forward" ? 0x243 : 0x245);
                            RawChatManagerPrefix.SetChatManagerRankPrefix(Managers[0], text); break;
                        case 3: Assert.Equal(256, RawChatManagerPrefix.ChatPrefixIcons(Output, _temporary, _rank, hooks).Length); break;
                        case 4: RawChatManagerPrefix.RefreshChatManagerRankPrefix(globals, hooks, _wrapperOutput, _temporary); break;
                        default: throw new InvalidOperationException("Chat fixture 진입점");
                    }
                }
                return "normal";
            }
            catch (ChatStop) { return "stop"; }
            catch (ArgumentOutOfRangeException) { return "fault"; }
            catch (DivideByZeroException) { return "fault"; }
        }
    }
    /// <summary>모든 물리 바이트를 원문 소문자 hex로 비교한다.</summary>
    private static string Hex(byte[] bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>양 x87 모드644관측의 정상524/fault94/중단26과 모든 실제 배열을 대조한다.</summary>
    [Fact]
    public void ReplaysAllIndependentNativeObservations()
    {
        int rows = 0; var statuses = new Dictionary<string, int>();
        // 고정 원문을 사본이나 기대값 변경 없이 전수 소비한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-manager-prefix-x86.tsv")))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] values = line.Split('\t'); Assert.Equal(11, values.Length); Assert.Contains(values[0], new[] { "639", "895" }); Assert.True(ulong.Parse(values[10], CultureInfo.InvariantCulture) > 0);
            var device = new Device(values[1]); string status = device.Run(); rows++; statuses[status] = statuses.GetValueOrDefault(status) + 1;
            Assert.True(status == values[2], $"row={rows} input={values[1]} status={status} expected={values[2]}"); Assert.Equal(values[3], device.State()); Assert.Equal(values[4], Hex(device.Output)); Assert.Equal(values[5], Hex(device.Managers[0])); Assert.Equal(values[6], Hex(device.Managers[1])); Assert.Equal(values[7], Hex(device.Buffers[0])); Assert.Equal(values[8], Hex(device.Buffers[1])); Assert.Equal(values[9], device.Trace());
        }
        Assert.Equal(644, rows); Assert.Equal(524, statuses["normal"]); Assert.Equal(94, statuses["fault"]); Assert.Equal(26, statuses["stop"]);
    }
    /// <summary>manager0/active0은 미선택 Core나 formatter 장치를 선행 요구하지 않는다.</summary>
    [Fact]
    public void NaturalGuardsReturnWithoutUnselectedDevices()
    {
        var current = new UIntBox(); var subscriber = new UIntBox { Value = 7 }; var active = new UIntBox { Value = 1 }; var globals = new ChatManagerPrefixGlobals(current, subscriber, active);
        RawChatManagerPrefix.RefreshChatManagerRankPrefix(globals, new(), [], []); current.Value = 0x1234; active.Value = 0; RawChatManagerPrefix.RefreshChatManagerRankPrefix(globals, new(), [], []);
        Assert.Equal(0x1234u, current.Value); Assert.Equal(7u, subscriber.Value); Assert.Equal(0u, active.Value);
    }
    /// <summary>CRT 어댑터는 같은 물리를 쓰고 기존 사용자 장치를 덮지 않으며 NUL padding을 보존한다.</summary>
    [Fact]
    public void MemoryCopyAdapterUsesSameBytesAndPreservesCustomCallback()
    {
        byte[] bytes = new byte[80]; bytes.AsSpan().Fill(0xa5); int reads = 0, custom = 0;
        var hooks = new ChatManagerPrefixHooks { Memory = token => { reads++; if (token != 17) throw new ArgumentOutOfRangeException(nameof(token)); return bytes; } };
        var bound = RawChatManagerPrefix.BindChatManagerMemoryCopy(hooks); bound.CopyFormat!(17, 20, "AB\0ignored", 60); Assert.Equal(1, reads); Assert.Equal(0xa5, bytes[19]); Assert.Equal((byte)'A', bytes[20]); Assert.Equal((byte)'B', bytes[21]); Assert.All(bytes[22..], value => Assert.Equal(0, value)); Assert.Null(hooks.CopyFormat);
        hooks.CopyFormat = (_, _, _, _) => custom++; bound = RawChatManagerPrefix.BindChatManagerMemoryCopy(hooks); bound.CopyFormat!(0, 0, "unused", 1); Assert.Equal(1, custom); Assert.Equal(1, reads);
        hooks.CopyFormat = null; bound = RawChatManagerPrefix.BindChatManagerMemoryCopy(hooks); Assert.Throws<ArgumentOutOfRangeException>(() => bound.CopyFormat!(0, 20, "AB", 60)); Assert.Equal(2, reads);
    }
    /// <summary>짧은 출력 실패도 앞선 DWORD만 쓰며 겹친 output/temporary는 순방향 DWORD 읽기를 유지한다.</summary>
    [Fact]
    public void FormatterKeepsPartialStoresAndOverlappingDwordReads()
    {
        byte[] temporary = Enumerable.Range(0, 256).Select(value => (byte)value).ToArray(), output = new byte[7]; output.AsSpan().Fill(0xa5);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawChatManagerPrefix.ChatPrefixIcons(output, temporary, -1, new())); Assert.Equal(new byte[] { 0, 1, 2, 3, 0xa5, 0xa5, 0xa5 }, output);
        byte[] shared = Enumerable.Range(0, 264).Select(value => unchecked((byte)value)).ToArray(); RawChatManagerPrefix.ChatPrefixIcons(shared.AsSpan(4, 256), shared.AsSpan(0, 256), -1, new());
        // 4BYTE 앞으로 겹치면 첫 DWORD의 결과를 다음 source가 다시 읽는다.
        for (int offset = 4; offset < 260; offset += 4) Assert.Equal(new byte[] { 0, 1, 2, 3 }, shared.AsSpan(offset, 4).ToArray());
    }
    /// <summary>실제 Client 할당의 current/종류/별도128BYTE와 재초기화 뒤 이전 manager를 검증한다.</summary>
    [Fact]
    public void ActualOwnerKeepsAllocationsCurrentAndRecipientLifetimeSeparate()
    {
        var owner = new ClientChatManagerGlobals(); Assert.Equal(0u, owner.Current.Value); Assert.Equal(81, owner.FormatCapture.Length); owner.FormatCapture[80] = 0xa5;
        uint first = owner.Allocate(RawChatManagerPrefix.ManagerBytes); byte[] original = owner.Bytes(first); original.AsSpan().Fill(0x7b); Assert.Equal(0u, owner.Current.Value);
        uint recipient = owner.AllocateRecipientBuffer(256); owner.Bytes(recipient)[0] = 0x71; owner.ReleaseRecipientBuffer(0); Assert.Throws<ArgumentOutOfRangeException>(() => owner.ReleaseRecipientBuffer(first)); owner.ReleaseRecipientBuffer(recipient); Assert.Throws<ArgumentOutOfRangeException>(() => owner.Bytes(recipient)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.ReleaseRecipientBuffer(recipient));
        var globals = new ChatManagerPrefixGlobals(owner.Current, new() { Value = uint.MaxValue }, new()); var hooks = owner.MakePrefixHooks(); var names = new List<string>(); hooks.RegisterCommand = (name, _, _) => names.Add(name); hooks.ReadConfig = _ => null;
        RawChatManagerPrefix.InitializeChatManager(globals, hooks); uint second = owner.Current.Value; byte[] secondBytes = owner.Bytes(second); Assert.NotEqual(first, second); Assert.Same(original, owner.Bytes(first)); Assert.Equal(0x7b, original[0]); Assert.Equal(0x20, secondBytes[4]); Assert.Equal(0, secondBytes[0x244]);
        uint buffer = System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(secondBytes.AsSpan(8, 4)); Assert.Equal(128, owner.Bytes(buffer).Length); owner.ReleaseRecipientBuffer(buffer); Assert.Throws<ArgumentOutOfRangeException>(() => owner.Bytes(buffer));
        RawChatManagerPrefix.InitializeChatManager(globals, hooks); Assert.Same(secondBytes, owner.Bytes(second)); Assert.Equal(0xa5, owner.FormatCapture[80]); Assert.Equal(new[] { "format", "back", "front", "format", "back", "front" }, names);
        Assert.Throws<ArgumentOutOfRangeException>(() => owner.Allocate(1)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.AllocateRecipientBuffer(0)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.AllocateRecipientBuffer(3)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.Bytes(0));
    }
}
