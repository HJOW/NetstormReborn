using System.Globalization;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 Editgump/Console의 같은 버퍼/파생/자식/장치 전수 대조다.

/// <summary>앞13열만 입력에 사용하고 뒤10열은 관측 비교에만 사용한다.</summary>
public sealed class TextEntryGumpNativeTests
{
    /// <summary>원본 격리 노드 주소.</summary>
    private const uint Entry = 0x01009000;
    /// <summary>원본 버퍼 객체 주소.</summary>
    private const uint BufferToken = 0x01001000;
    /// <summary>원본 세로 Slider 주소.</summary>
    private const uint Vertical = 0x0100a000;
    /// <summary>원본 가로 Slider 주소.</summary>
    private const uint Horizontal = 0x0100b000;
    /// <summary>명시 자식 장치의 표 식별자.</summary>
    private const uint ChildVtable = 0x0100c000;
    /// <summary>signed 입력도 원시DWORD로 읽는다.</summary>
    private static uint U(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>현재DWORD 순서대로 직렬화한다.</summary>
    private static string Pack(params uint[] values) => string.Join(':', values);
    /// <summary>원시float32의 현재DWORD다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>원시DWORD의 같은float32다.</summary>
    private static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>NUL 앞의 현재 바이트.입력 인코딩을 변환하지 않는다.</summary>
    private static ReadOnlySpan<byte> CString(ReadOnlySpan<byte> value) { int end = value.IndexOf((byte)0); return end >= 0 ? value[..end] : throw new ArgumentException("문자 장치 NUL 없음"); }
    /// <summary>빈BYTE도 빈16진수로 유지한다.</summary>
    private static string Hex(ReadOnlySpan<byte> value) => Convert.ToHexString(value).ToLowerInvariant();
    /// <summary>표시 장치에서만 빈BYTE를-로 관측한다.</summary>
    private static string VisibleHex(ReadOnlySpan<byte> value) => value.IsEmpty ? "-" : Hex(value);
    /// <summary>물리 가드와 해제 상태를 포함하는 독립 입력 블록.</summary>
    private sealed class Block
    {
        /// <summary>원문 요청 폭.</summary>
        public uint Nominal;
        /// <summary>해제하면0인 관측 필드.</summary>
        public uint Live;
        /// <summary>실제 요청폭+16가드 바이트.</summary>
        public byte[] Bytes = [];
    }
    /// <summary>원문13입력 열만 소유한 명시 외부 장치.기대상태를 주입하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>기본/TextEntry/Console이 같은 실제 객체다.</summary>
        public ConsoleGumpNode Node { get; } = new();
        /// <summary>세로 Slider의 같은 실제 객체.</summary>
        public ScrollGumpNode V { get; } = new();
        /// <summary>가로 Slider의 같은 실제 객체.</summary>
        public ScrollGumpNode H { get; } = new();
        /// <summary>같은 실제10DWORD.</summary>
        public TextEntryBufferState State { get; } = new();
        /// <summary>현재 실제 버퍼 처리기.</summary>
        public TextEntryBuffer Buffer { get; }
        /// <summary>현재 실제 위젯 처리기.</summary>
        public TextEntryGump Gump { get; }
        /// <summary>주소순서의 현재/반납 블록 모두.</summary>
        public SortedDictionary<uint, Block> Blocks { get; } = [];
        /// <summary>명시 원문NUL 입력.</summary>
        public byte[] Text { get; }
        /// <summary>원문 별도 제외문자 입력.</summary>
        private readonly byte[] _excluded;
        /// <summary>명시 클립보드 입력.</summary>
        private readonly byte[] _clipboard;
        /// <summary>실제 클립보드 출력만 기록한다.</summary>
        public byte[] ClipboardOutput { get; private set; } = [];
        /// <summary>독립 실제 시각 입력.</summary>
        private readonly double[] _times;
        /// <summary>다음 실제 시각 순번.</summary>
        private int _timeIndex;
        /// <summary>명시 자동완성 반환.빈 후보/null 구별.</summary>
        private readonly byte[]? _completion;
        /// <summary>명시 수신 대상 검사 반환.</summary>
        private readonly bool _recipient;
        /// <summary>명시 새 메모리 초기BYTE.</summary>
        private readonly byte _fill;
        /// <summary>다음 물리 블록 번호.</summary>
        private uint _next;
        /// <summary>Slider 객체 할당 순번.</summary>
        private uint _slider;
        /// <summary>호출 순서의 실제 장치 관측.</summary>
        private readonly List<string> _trace = [];
        /// <summary>원본DWORD48개를 같은 파생/기본 필드에 설치한다.</summary>
        private void SetFields(uint[] a)
        {
            Assert.Equal(48, a.Length); Node.Vtable = a[0]; Node.Flags = a[1]; Node.Command = a[2]; Node.Serial = a[3]; Node.Scale = Float(a[4]); Node.BaseWidth = Float(a[5]); Node.BaseHeight = Float(a[6]); Node.Width = Float(a[7]); Node.Height = Float(a[8]); Node.X = Float(a[9]); Node.Y = Float(a[10]); Array.Copy(a, 11, Node.TextContext, 0, 8); Node.BackgroundFlags = a[19]; Node.VerticalSlider = a[20]; Node.HorizontalSlider = a[21]; Node.OwnsBuffer = a[22]; Node.ReservedC4 = a[23]; Node.BlinkAt = BitConverter.UInt64BitsToDouble(a[24] | ((ulong)a[25] << 32)); Node.BlinkPhase = a[26]; Node.ReadOnly = a[27]; Node.ShiftAnchor = unchecked((int)a[28]); Node.ShiftEnd = unchecked((int)a[29]); Node.ScrollLine = unchecked((int)a[30]); Node.ScrollColumn = unchecked((int)a[31]); Node.ShowVertical = a[32]; Node.ShowHorizontal = a[33]; Node.ForwardInput = a[34]; Node.Multiline = a[35]; Node.ChangeNotice = a[36]; Node.RecipientHighlight = a[37]; Node.MaxLength = unchecked((int)a[38]); Node.ExcludedChars = a[39]; Node.Buffer = a[40]; Node.Reserved10C = a[41]; Node.Autocomplete = a[42]; Node.Submit = a[43]; Node.ResetWord = a[44]; Node.WordStart = a[45]; Node.WordLength = a[46]; Node.Reserved124 = a[47];
        }
        /// <summary>같은 실제 파생/기본48DWORD의 현재값만 관측한다.</summary>
        public string Fields() { ulong time = BitConverter.DoubleToUInt64Bits(Node.BlinkAt); return Pack(Node.Vtable, Node.Flags, Node.Command, Node.Serial, Bits(Node.Scale), Bits(Node.BaseWidth), Bits(Node.BaseHeight), Bits(Node.Width), Bits(Node.Height), Bits(Node.X), Bits(Node.Y), Node.TextContext[0], Node.TextContext[1], Node.TextContext[2], Node.TextContext[3], Node.TextContext[4], Node.TextContext[5], Node.TextContext[6], Node.TextContext[7], Node.BackgroundFlags, Node.VerticalSlider, Node.HorizontalSlider, Node.OwnsBuffer, Node.ReservedC4, (uint)time, (uint)(time >> 32), Node.BlinkPhase, Node.ReadOnly, unchecked((uint)Node.ShiftAnchor), unchecked((uint)Node.ShiftEnd), unchecked((uint)Node.ScrollLine), unchecked((uint)Node.ScrollColumn), Node.ShowVertical, Node.ShowHorizontal, Node.ForwardInput, Node.Multiline, Node.ChangeNotice, Node.RecipientHighlight, unchecked((uint)Node.MaxLength), Node.ExcludedChars, Node.Buffer, Node.Reserved10C, Node.Autocomplete, Node.Submit, Node.ResetWord, Node.WordStart, Node.WordLength, Node.Reserved124); }
        /// <summary>같은 실제 편집 상태10DWORD의 현재값만 관측한다.</summary>
        public string BufferFields() => Pack(State.Vtable, State.Offsets, unchecked((uint)State.OffsetCapacity), unchecked((uint)State.LineCount), State.Dirty, unchecked((uint)State.SelectionBegin), unchecked((uint)State.SelectionEnd), unchecked((uint)State.Length), unchecked((uint)State.Capacity), State.Data);
        /// <summary>현재 자식의 크기/위치/값11DWORD다.</summary>
        public static string ChildFields(ScrollGumpNode n) => Pack(n.Vtable, n.Flags, n.Serial, Bits(n.Scale), Bits(n.BaseWidth), Bits(n.BaseHeight), Bits(n.Width), Bits(n.Height), Bits(n.X), Bits(n.Y), unchecked((uint)n.Position));
        /// <summary>현재 실제 기본 주소다.</summary>
        private GumpCreateNode? Base(uint token) => token switch { Entry => Node, Vertical => V, Horizontal => H, _ => null };
        /// <summary>도달한 실제 Slider만 반환한다.</summary>
        private ScrollGumpNode Slider(uint token) => token switch { Vertical => V, Horizontal => H, _ => throw new ArgumentException("장치 Slider 주소") };
        /// <summary>현재 블록/내부 주소+16가드의 순수 뷰다.</summary>
        private Memory<byte> Memory(uint address) { if (address == 0x01006000) return _excluded; var block = Blocks.LastOrDefault(p => p.Key <= address); uint offset = unchecked(address - block.Key); if (block.Value is null || offset >= block.Value.Bytes.Length) throw new ArgumentException("장치 물리 메모리 범위"); return block.Value.Bytes.AsMemory((int)offset); }
        /// <summary>원문과 같은 고정 주소 순서로 물리 메모리를 확보한다.</summary>
        private uint Allocate(uint size) { uint address = 0x01010000 + _next++ * 0x10000; Blocks.Add(address, new() { Nominal = size, Live = 1, Bytes = Enumerable.Repeat(_fill, checked((int)size + 16)).ToArray() }); return address; }
        /// <summary>장치 관측에만 추가한다.</summary>
        private void Event(string name, params uint[] values) => _trace.Add(values.Length == 0 ? name : name + ":" + Pack(values));
        /// <summary>원시 문자열/64비트 관측을 부호 변환 없이 추가한다.</summary>
        private void Words(string name, params string[] values) => _trace.Add(name + ":" + string.Join(':', values));
        /// <summary>현재 장치 호출의 순수 관측.</summary>
        public string Trace() => _trace.Count == 0 ? "-" : string.Join(';', _trace);
        /// <summary>반납 블록과 가드도 포함한 현재 메모리 관측.</summary>
        public string BlocksOutput() => Blocks.Count == 0 ? "-" : string.Join(';', Blocks.Select(p => $"{p.Key}:{p.Value.Nominal}:{p.Value.Live}:{Hex(p.Value.Bytes)}"));
        /// <summary>실제 버퍼에 독립 allocator/클립보드 장치를 연결한다.</summary>
        private TextEntryBufferHooks BufferHooks() => new()
        {
            Bytes = Memory,
            Allocate = size => { uint address = Allocate(size); Event("allocate", size, address); return address; },
            Release = address => { if (address != 0) Blocks[address].Live = 0; Event("release", address); },
            Resize = (previous, size) => { uint address = Allocate(size); if (previous != 0) { var old = Blocks[previous]; old.Bytes.AsSpan(0, (int)Math.Min(old.Nominal, size)).CopyTo(Blocks[address].Bytes); old.Live = 0; } Event("resize", previous, size, address); return address; },
            ClipboardRead = target => { target.Clear(); _clipboard.AsSpan(0, Math.Min(target.Length, _clipboard.Length)).CopyTo(target); Event("clipboardRead", (uint)target.Length); },
            ClipboardWrite = value => { ClipboardOutput = value.ToArray(); Words("clipboardWrite", VisibleHex(value)); }
        };
        /// <summary>현재 실제 로컬 좌표만 쓰는 원문 명시Attach 장치다.</summary>
        private void Attach(string name, uint token, float x, float y, uint parent, uint mode) { var n = Base(token) ?? throw new ArgumentException("장치 기본 주소"); n.X = x; n.Y = y; Event(name, token, Bits(x), Bits(y), parent, mode); }
        /// <summary>독립 관측의 기본/Slider/글자/픽셀 장치 계약만 연결한다.</summary>
        private TextEntryGumpHooks Hooks() => new()
        {
            Node = token => token == Entry ? Node : null, Console = token => token == Entry ? Node : null, Base = Base, Buffer = token => token == BufferToken ? Buffer : null, Slider = Slider,
            Allocate = size => { uint token = size == 40 ? BufferToken : (_slider++ != 0 ? Horizontal : Vertical); Event("new", (uint)size, token); return token; },
            ReleaseBuffer = token => Event("delete", token),
            ConstructBackground = (token, flags) => { Node.Flags = 1; Node.Scale = 1; Node.BaseWidth = 0; Node.BaseHeight = 0; Node.BackgroundFlags = flags; Event("constructBackground", token, flags); },
            ConstructSlider = (token, width, height, brown, black) => { var c = Slider(token); c.Vtable = ChildVtable; c.Flags = 1; c.Scale = 1; c.BaseWidth = c.Width = width; c.BaseHeight = c.Height = height; c.Position = 0; Event("constructSlider", token, unchecked((uint)width), unchecked((uint)height), brown, black); },
            BaseAttach = (token, x, y, parent, mode) => Attach("baseAttach", token, x, y, parent, mode), Attach = (token, x, y, parent, mode) => Attach("attach", token, x, y, parent, mode),
            Push = token => Event("push", token), Redraw = (token, value) => Event("redraw", token, value ? 1U : 0U), Idle = (token, value) => Event("idle", token, value ? 1U : 0U), Bubble = (token, code, first, second) => Event("bubble", token, code, first, second),
            SliderRange = (token, first, last) => Event("range", token, unchecked((uint)first), unchecked((uint)last)), SliderPage = (token, value) => Event("page", token, unchecked((uint)value)), SliderPosition = (token, value) => { Event("position", token, unchecked((uint)value)); Slider(token).Position = value; },
            Now = () => { double value = _times[Math.Min(_timeIndex++, _times.Length - 1)]; Words("now", BitConverter.DoubleToUInt64Bits(value).ToString(CultureInfo.InvariantCulture)); return value; },
            Measure = (token, value, length) => { var visible = length < 0 ? CString(value) : value[..length]; int width = 0;
                // 원문 명시 측정 장치의BYTE별 폭을 같은 순서로 더한다.실제 글꼴의 대체물이 아니다.
                foreach (byte b in visible) width = unchecked(width + 4 + b % 5); Words("measure", VisibleHex(visible), unchecked((uint)length).ToString(CultureInfo.InvariantCulture), width.ToString(CultureInfo.InvariantCulture), "14"); return (width, 14); },
            Position = token => { Event("screen", token); return new(17, 23); }, DrawBackground = token => Event("background", token),
            DrawText = (token, value, point, context) => Words("text", VisibleHex(CString(value)), unchecked((uint)point.X).ToString(CultureInfo.InvariantCulture), unchecked((uint)point.Y).ToString(CultureInfo.InvariantCulture), context is null ? "0" : "1", (context is null ? Node.TextContext[2] : context[2]).ToString(CultureInfo.InvariantCulture)),
            Fill = (rect, color) => Event("fill", unchecked((uint)rect.Left), unchecked((uint)rect.Top), unchecked((uint)rect.Right), unchecked((uint)rect.Bottom), color), Recipient = value => { Words("recipient", VisibleHex(value)); return _recipient; },
            AllocateBytes = size => { uint token = Allocate(size); Event("new", size, token); return token; }, ReleaseBytes = token => { if (Blocks.TryGetValue(token, out var block)) block.Live = 0; Event("delete", token); }, Bytes = Memory,
            Complete = (callback, word, length, shift) => { Words("complete", word.HasValue ? Hex(word.Value.Span) : "~", unchecked((uint)length).ToString(CultureInfo.InvariantCulture), shift ? "1" : "0"); return _completion; }, Submit = (callback, value) => Words("submit", VisibleHex(value)), Assertion = line => Event("assert", line), DestructBase = token => Event("destructBase", token)
        };
        /// <summary>독립 입력 앞13열만 설치한다.</summary>
        public Scene(string[] row)
        {
            SetFields(row[2].Split(':').Select(U).ToArray()); uint[] a = row[3].Split(':').Select(U).ToArray(); Assert.Equal(10, a.Length); State.Vtable = a[0]; State.Offsets = a[1]; State.OffsetCapacity = unchecked((int)a[2]); State.LineCount = unchecked((int)a[3]); State.Dirty = a[4]; State.SelectionBegin = unchecked((int)a[5]); State.SelectionEnd = unchecked((int)a[6]); State.Length = unchecked((int)a[7]); State.Capacity = unchecked((int)a[8]); State.Data = a[9];
            // 기존 입력의 전체 물리 바이트만 설치하고 기대 관측은 사용하지 않는다.
            if (row[4] != "-") foreach (string item in row[4].Split(';')) { string[] p = item.Split(':'); Blocks.Add(U(p[0]), new() { Nominal = U(p[1]), Live = U(p[2]), Bytes = Convert.FromHexString(p[3]) }); }
            _next = (uint)Blocks.Count; _fill = (byte)U(row[12]); Text = [.. Convert.FromHexString(row[6]), 0]; _excluded = [.. Convert.FromHexString(row[7]), 0]; _clipboard = Convert.FromHexString(row[8]); _times = row[9].Split(':').Select(v => double.Parse(v, CultureInfo.InvariantCulture)).ToArray(); _completion = row[10] == "~" ? null : Convert.FromHexString(row[10]); _recipient = row[11] == "1";
            // 각 Slider의 명시 입력 초기 상태.부모/트리는 이 fixture 장치 범위 밖이다.
            foreach (uint token in new[] { Vertical, Horizontal }) { var c = Slider(token); c.Vtable = ChildVtable; c.Flags = 1; c.Serial = token == Vertical ? 31U : 32U; c.Scale = 1; c.BaseWidth = c.Width = token == Vertical ? 10 : 100; c.BaseHeight = c.Height = token == Vertical ? 100 : 10; c.Position = 3; }
            Buffer = new(State, BufferHooks()); Gump = new(new() { Brown = 9, Black = 0, Yellow = 5 }, Hooks());
        }
    }
    /// <summary>독립 원본2292행의 전체 상태/메모리/효과/반환을 비교한다.</summary>
    [Fact]
    public void AllOriginalEntryAndConsoleObservationsMatch()
    {
        int count = 0;
        // 각 행을 독립 입력으로 실행하고 뒤10열을 순수 기대값으로만 사용한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "text-entry-gump-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith("name\t", StringComparison.Ordinal)) continue; string[] row = line.Split('\t'); Assert.Equal(23, row.Length); var s = new Scene(row); uint[] a = row[5] == "-" ? [] : row[5].Split(':').Select(U).ToArray(); uint result = 0;
            switch (row[1])
            {
                case "construct": s.Gump.Construct(Entry, unchecked((int)a[0]), unchecked((int)a[1]), a[2] != 0 ? (ReadOnlyMemory<byte>)s.Text : (ReadOnlyMemory<byte>?)null, a[3], a[4], a[5]); break;
                case "consoleConstruct": s.Gump.ConstructConsole(Entry, unchecked((int)a[0]), unchecked((int)a[1]), a[2] != 0 ? (ReadOnlyMemory<byte>)s.Text : (ReadOnlyMemory<byte>?)null, a[3], a[4], a[5]); break;
                case "attach": s.Gump.Attach(Entry, Float(a[0]), Float(a[1]), a[2], a[3]); break;
                case "scroll": s.Gump.SetScroll(Entry, unchecked((int)a[0]), unchecked((int)a[1])); break;
                case "relative": s.Gump.RelativeScroll(Entry, unchecked((int)a[0]), unchecked((int)a[1])); break;
                case "handle": result = s.Gump.Handle(Entry, a[0], a[1], a[2]); break;
                case "console": result = s.Gump.HandleConsole(Entry, a[0], a[1], a[2]); break;
                case "draw": s.Gump.Draw(Entry); break;
                case "idle": s.Gump.Idle(Entry); break;
                case "setText": s.Gump.SetText(Entry, a[0] != 0 ? s.Text : []); break;
                case "append": s.Gump.AppendText(Entry, a[0] != 0 ? s.Text : [], a[1] != 0); break;
                case "excluded": s.Gump.SetExcluded(Entry, a[0] != 0 ? (ReadOnlyMemory<byte>)s.Text : (ReadOnlyMemory<byte>?)null); break;
                case "horizontal": s.Gump.SetHorizontal(Entry, a[0]); break;
                case "destroy": s.Gump.Destruct(Entry); break;
                default: throw new ArgumentException("원문 문자 입력창 진입 이름");
            }
            Check(row, 13, s.Fields()); Check(row, 14, Scene.ChildFields(s.V)); Check(row, 15, Scene.ChildFields(s.H)); Check(row, 16, s.BufferFields()); Check(row, 17, s.BlocksOutput()); Check(row, 18, VisibleHex(s.ClipboardOutput)); Check(row, 19, s.Trace()); Check(row, 20, result.ToString(CultureInfo.InvariantCulture)); Assert.Equal((uint)(4 + a.Length * 4), U(row[21])); Assert.True(U(row[22]) > 0, row[0]); count++;
        }
        Assert.Equal(2292, count);
    }
    /// <summary>큰 메모리는 실패 출력에 복사하지 않고 입력명/필드만 표시한다.</summary>
    private static void Check(string[] row, int column, string actual) => Assert.True(row[column] == actual, $"{row[0]}/{row[1]} 열{column}" + (column == 17 ? "" : $" 기대={row[column]} 실제={actual}"));
}
