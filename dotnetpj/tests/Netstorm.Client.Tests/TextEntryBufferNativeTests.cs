using System.Globalization;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 실제 원문 편집 버퍼570행의 전체 메모리/상태/효과와 부분 중단 대조다.

/// <summary>기대값은 읽기 전용 fixture에서만 읽는다.PE/게임/클립보드 장치를 실행하지 않는다.</summary>
public sealed class TextEntryBufferNativeTests
{
    /// <summary>입력 signed 십진수도 하위 DWORD 비트로 읽는다.</summary>
    private static uint U(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원문 DWORD 목록의 순서와 부호 없는 표기를 유지한다.</summary>
    private static string Pack(params uint[] values) => string.Join(':', values);
    /// <summary>입력의 명시 장치 중단.제품 오류나 정상 반환과 구별한다.</summary>
    private sealed class ExplicitStop : Exception;
    /// <summary>전체 가드/반납 상태까지 관측하는 독립 입력 메모리 블록.</summary>
    private sealed class Block
    {
        /// <summary>원문 요청 폭.</summary>
        public uint Nominal;
        /// <summary>실제 반납 상태.</summary>
        public uint Live;
        /// <summary>요청 폭+16가드의 실제 바이트.</summary>
        public byte[] Bytes = [];
    }
    /// <summary>앞11열 입력만 장치에 사용한다.뒤 관찰 열로 상태/효과를 만들지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제10DWORD.</summary>
        public TextEntryBufferState State { get; } = new();
        /// <summary>같은 원문 주소 순서의 실제 물리 메모리.</summary>
        public SortedDictionary<uint, Block> Blocks { get; } = [];
        /// <summary>기본 외부 효과 뒤 적용할 명시 입력.</summary>
        private readonly string[][] _effects;
        /// <summary>입력 실패 호출 번호.</summary>
        private readonly HashSet<string> _failures;
        /// <summary>실제 효과별 호출 수.</summary>
        private readonly Dictionary<string, int> _calls = [];
        /// <summary>현재 다음 고정 가상 블록 번호.</summary>
        private uint _next;
        /// <summary>명시 allocator 초기 BYTE.</summary>
        private readonly byte _fill;
        /// <summary>원문 NUL 입력.</summary>
        public byte[] Text { get; }
        /// <summary>원문 NUL 제외 문자 입력.</summary>
        public byte[] Excluded { get; }
        /// <summary>명시 클립보드 입력.</summary>
        private readonly byte[] _clipboard;
        /// <summary>원문2048바이트 출력의 전체 실제 내용.</summary>
        public byte[] Output { get; } = Enumerable.Repeat((byte)0x6d, 2048).ToArray();
        /// <summary>별도 선택쌍 출력.</summary>
        public int[] Selection { get; } = [0x11223344, 0x55667788];
        /// <summary>실제 쓰기 장치가 받은 클립보드.</summary>
        public byte[] ClipboardOutput { get; private set; } = [];
        /// <summary>실제 외부 효과 순서/인자.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>검증 대상 실제 버퍼.</summary>
        public TextEntryBuffer Buffer { get; }
        /// <summary>원문10DWORD 순서의 현재 값만 관측한다.</summary>
        public uint[] Fields() => [State.Vtable, State.Offsets, unchecked((uint)State.OffsetCapacity), unchecked((uint)State.LineCount), State.Dirty, unchecked((uint)State.SelectionBegin), unchecked((uint)State.SelectionEnd), unchecked((uint)State.Length), unchecked((uint)State.Capacity), State.Data];
        /// <summary>실제 입력 필드를 같은 객체에 설치한다.객체 주소를 교체하지 않는다.</summary>
        private void Set(uint[] f) { State.Vtable = f[0]; State.Offsets = f[1]; State.OffsetCapacity = unchecked((int)f[2]); State.LineCount = unchecked((int)f[3]); State.Dirty = f[4]; State.SelectionBegin = unchecked((int)f[5]); State.SelectionEnd = unchecked((int)f[6]); State.Length = unchecked((int)f[7]); State.Capacity = unchecked((int)f[8]); State.Data = f[9]; }
        /// <summary>관측 부작용 없는 현재 주소의 남은 물리 범위.반납 메모리도 관찰용으로 보존한다.</summary>
        private Memory<byte> Bytes(uint address)
        { var entry = Blocks.LastOrDefault(b => b.Key <= address); if (entry.Value is null) throw new InvalidOperationException("관찰 메모리 주소 없음"); uint offset = address - entry.Key; if (offset >= entry.Value.Bytes.Length) throw new InvalidOperationException("관찰 물리 범위"); return entry.Value.Bytes.AsMemory((int)offset); }
        /// <summary>원문 주소별 전체 가드/반납 메모리의 현재 상태만 관측한다.</summary>
        public string BlockOutput() => Blocks.Count == 0 ? "-" : string.Join(';', Blocks.Select(b => $"{b.Key}:{b.Value.Nominal}:{b.Value.Live}:" + Convert.ToHexStringLower(b.Value.Bytes)));
        /// <summary>외부 장치의 기본 효과 뒤 같은 호출 번호의 입력 변경/중단만 적용한다.</summary>
        private void Event(string name, params string[] values)
        {
            Trace.Add(name + ":" + string.Join(':', values)); int call = _calls.GetValueOrDefault(name) + 1; _calls[name] = call; string trigger = name + call; bool stopped = false;
            // 같은 효과 경계의 여러 명시 변경을 입력 순서로 적용한다.
            foreach (var e in _effects) { if (e[0] != trigger) continue; if (e[1] == "control") { if (e[2] == "stop" && e[3] == "1") stopped = true; continue; } Assert.Equal("state", e[1]); string[] names = ["vtable", "offsets", "offsetCapacity", "lineCount", "dirty", "selectionBegin", "selectionEnd", "length", "capacity", "data"]; int at = Array.IndexOf(names, e[2]); Assert.True(at >= 0); uint[] fields = Fields(); fields[at] = U(e[3]); Set(fields); } if (stopped) throw new ExplicitStop();
        }
        /// <summary>현재 실패 입력과 명시 초기 바이트로 실제 새 블록을 만든다.</summary>
        private uint Allocate(uint size, bool failed)
        { if (failed) return 0; Assert.True(size <= 16384); uint token = 0x01010000 + _next++ * 0x10000; Blocks.Add(token, new() { Nominal = size, Live = 1, Bytes = Enumerable.Repeat(_fill, (int)size + 16).ToArray() }); return token; }
        /// <summary>앞11열의 초기 상태/블록/실패/효과/본문만 읽고 실제 버퍼에 연결한다.</summary>
        public Scene(string[] row)
        {
            Text = [.. X86Fixture.Bytes(row[5]), 0]; Excluded = [.. X86Fixture.Bytes(row[6]), 0]; _clipboard = X86Fixture.Bytes(row[7]); _fill = unchecked((byte)U(row[10])); _effects = row[9] == "-" ? [] : row[9].Split(';').Select(s => s.Split('/')).ToArray(); _failures = row[8] == "-" ? [] : row[8].Split(',').ToHashSet(); Set(row[2].Split(':').Select(U).ToArray());
            // 초기 메모리는 입력 블록에만 의존한다.
            if (row[3] != "-") foreach (string part in row[3].Split(';')) { string[] p = part.Split(':'); Blocks.Add(U(p[0]), new() { Nominal = U(p[1]), Live = U(p[2]), Bytes = X86Fixture.Bytes(p[3]) }); } _next = (uint)Blocks.Count;
            var hooks = new TextEntryBufferHooks { Bytes = Bytes, Allocate = size => { uint token = Allocate(size, _failures.Contains("allocate" + (_calls.GetValueOrDefault("allocate") + 1))); Event("allocate", size.ToString(), token.ToString()); return token; }, Release = token => { if (token != 0) Blocks[token].Live = 0; Event("release", token.ToString()); }, Resize = (old, size) => { uint token = Allocate(size, _failures.Contains("resize" + (_calls.GetValueOrDefault("resize") + 1))); if (token != 0 && old != 0) { var previous = Blocks[old]; previous.Bytes.AsSpan(0, (int)Math.Min(previous.Nominal, size)).CopyTo(Blocks[token].Bytes); previous.Live = 0; } Event("resize", old.ToString(), size.ToString(), token.ToString()); return token; }, ClipboardRead = target => { target.Clear(); _clipboard.AsSpan(0, Math.Min(_clipboard.Length, target.Length)).CopyTo(target); Event("clipboardRead", target.Length.ToString()); }, ClipboardWrite = source => { ClipboardOutput = source.ToArray(); Event("clipboardWrite", source.IsEmpty ? "-" : Convert.ToHexStringLower(source)); } }; Buffer = new(State, hooks);
        }
    }
    /// <summary>원본285입력의 두 관찰570행에서 정상/명시 중단·10DWORD/전체 메모리/출력/효과/의미 반환을 대조한다.</summary>
    [Fact]
    public void AllIndependentNativeOperationsAndPartialStops()
    {
        var rows = X86Fixture.Read("text-entry-buffer").Where(r => r[0] != "name").ToArray(); Assert.Equal(570, rows.Length); int stopped = 0;
        // 각 행은 독립된 실제 입력 상태/장치를 사용한다.
        foreach (var r in rows)
        {
            Assert.Equal(22, r.Length); var s = new Scene(r); uint[] args = r[4] == "-" ? [] : r[4].Split(':').Select(U).ToArray(); uint result = 0; bool completed = true, meaningful = false; int parameters = args.Length;
            try
            {
                switch (r[1]) { case "construct": s.Buffer.Construct(args[0]); result = 1; meaningful = true; break; case "destroy": s.Buffer.Destroy(); break; case "select": s.Buffer.Select(unchecked((int)args[0]), unchecked((int)args[1])); break; case "insert": s.Buffer.Insert(args[0] != 0 ? s.Text : []); break; case "reset": s.Buffer.Reset(args[0] != 0 ? s.Text : []); break; case "lines": result = s.Buffer.EnsureLines() ? 1U : 0; meaningful = true; break; case "lineCount": result = unchecked((uint)s.Buffer.LineCount()); meaningful = true; break; case "lineLength": result = unchecked((uint)s.Buffer.LineLength(unchecked((int)args[0]))); meaningful = true; break; case "copyLine": result = s.Buffer.CopyLine(unchecked((int)args[0]), s.Output.AsSpan(0, (int)args[1]), s.Selection); meaningful = true; parameters = 4; break; case "copyAll": result = s.Buffer.CopyAll(s.Output.AsSpan(0, (int)args[0])); meaningful = true; parameters = 2; break; case "copy": s.Buffer.Copy(); break; case "cut": s.Buffer.Cut(); break; case "paste": s.Buffer.Paste(args[0] != 0 ? s.Excluded : []); break; default: Assert.Fail("원문 편집 연산 없음"); break; }
            }
            catch (ExplicitStop) { completed = false; stopped++; }
            Assert.True(completed == (r[11] == "1"), r[0] + ": 완료 상태"); Assert.True(Pack(s.Fields()) == r[12], r[0] + ": 전체10DWORD"); Assert.True(s.BlockOutput() == r[13], r[0] + ": 전체 메모리/가드/반납"); Assert.True(Convert.ToHexStringLower(s.Output) == r[14], r[0] + ": 전체 출력"); Assert.Equal(r[15], Pack(unchecked((uint)s.Selection[0]), unchecked((uint)s.Selection[1]))); Assert.Equal(r[16], s.ClipboardOutput.Length == 0 ? "-" : Convert.ToHexStringLower(s.ClipboardOutput)); Assert.Equal(r[17], s.Trace.Count == 0 ? "-" : string.Join(';', s.Trace)); if (completed && meaningful) Assert.Equal(U(r[18]), result); if (completed) Assert.Equal((uint)(4 + parameters * 4), U(r[19])); Assert.True(U(r[20]) > 0); Assert.Equal("0", r[21]);
        }
        Assert.Equal(10, stopped);
    }
}
