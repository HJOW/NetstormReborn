using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Client.Gump;
using Netstorm.Tests;

// 같은 실제 Console/편집 물리를 사용해 독립 ChatWindow 부모180관측을 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>기존2292관측의 입력 장면을 재사용하며 기대값을 주입하지 않는다.</summary>
public sealed class ChatWindowNativeTests
{
    /// <summary>독립 원문 root 주소다.</summary>
    private const uint RootToken = 0x01003000;
    /// <summary>독립 원문 helper 주소다.</summary>
    private const uint HelperToken = 0x01004000;
    /// <summary>독립 원문 배경 주소다.</summary>
    private const uint BackgroundToken = 0x01005000;
    /// <summary>독립 원문 로그 주소다.</summary>
    private const uint ScrollToken = 0x01006000;
    /// <summary>독립 원문 공유 버퍼 주소다.</summary>
    private const uint SharedToken = 0x01007000;
    /// <summary>독립 원문 문자열 payload 주소다.</summary>
    private const uint PayloadToken = 0x01008000;
    /// <summary>같은 원문 Console 주소다.</summary>
    private const uint EntryToken = 0x01009000;
    /// <summary>같은 실제 편집 버퍼 주소다.</summary>
    private const uint BufferToken = 0x01001000;
    /// <summary>명시 외부 자식의 가상 표 식별자다.</summary>
    private const uint ChildVtable = 0x0117f000;
    /// <summary>signed 숫자도 같은 DWORD 비트로 읽는다.</summary>
    private static uint U(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>현재 단정도 비트다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>실제 DWORD 순서의 관측이다.</summary>
    private static string Pack(params uint[] values) => string.Join(':', values);
    /// <summary>원문 빈 표시를 포함한 원시 BYTE를 읽는다.</summary>
    private static byte[] Bytes(string text) => text == "-" ? [] : Convert.FromHexString(text);
    /// <summary>좁은 원문 문자열을 인코딩 해석 없이 복원한다.</summary>
    private static string Input(string text) => Encoding.Latin1.GetString(Bytes(text));
    /// <summary>실제 문자열 장치의 빈 관측은 '-'다.</summary>
    private static string Hex(string text) => text.Length == 0 ? "-" : Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();
    /// <summary>실제 BYTE 본문의 같은 빈/소문자hex 관측이다.</summary>
    private static string Hex(ReadOnlySpan<byte> text) => text.IsEmpty ? "-" : Convert.ToHexString(text).ToLowerInvariant();
    /// <summary>첫NUL 앞의 실제 접두 BYTE만 읽는다.</summary>
    private static string Prefix(byte[] bytes) { int end = bytes.AsSpan().IndexOf((byte)0); Assert.True(end >= 0); return Encoding.Latin1.GetString(bytes, 0, end); }
    /// <summary>앞21열에서 기존 실제 편집기 입력23열만 구성한다.</summary>
    private static string[] EntryInput(string[] row)
    {
        string[] input = Enumerable.Repeat("", 23).ToArray(); input[2] = row[18]; input[3] = row[19]; input[4] = row[20]; input[6] = row[8] == "-" ? "" : row[8]; input[8] = "4109420a43"; input[9] = "0.5:0.75"; input[10] = "776f726c64"; input[11] = row[13]; input[12] = "165"; return input;
    }
    /// <summary>원문 외부 장치 계약과 같은 실제 Console/Buffer/trace를 사용한다.</summary>
    private sealed class Scene : TextEntryGumpNativeTests.Scene
    {
        /// <summary>같은 기본 저장소를 공유하는 실제 root다.</summary>
        public readonly HudChatNode Root = new();
        /// <summary>같은 실제 helper다.</summary>
        public readonly GumpBackgroundNode Helper = new();
        /// <summary>같은 실제 배경이다.</summary>
        public readonly GumpBackgroundNode Background = new();
        /// <summary>같은 실제 로그 자식이다.</summary>
        public readonly ScrollingTextGumpNode Scrolling = new();
        /// <summary>현재 포인터/등록/후보의 실제 원문 상태다.</summary>
        public readonly ChatWindowState ChatState = new();
        /// <summary>명시 원문 전역 입력이다.</summary>
        public readonly Dictionary<uint, uint> Values = [];
        /// <summary>같은 부모 제품 본체다.</summary>
        public readonly ChatWindow Chat;
        /// <summary>현재 명시 메뉴 토큰이다.</summary>
        private uint _menu;
        /// <summary>원문 이름 조회의 명시 반환 입력이다.</summary>
        private readonly string? _resolved;
        /// <summary>원문 유효 대상 장치의 명시 입력이다.</summary>
        private readonly bool _recipient;
        /// <summary>기존 Console/Slider와 새 네 노드가 같은 기본 주소를 공유한다.</summary>
        protected override GumpCreateNode? Base(uint token) => token switch { RootToken => Root, HelperToken => Helper, BackgroundToken => Background, ScrollToken => Scrolling, _ => base.Base(token) };
        /// <summary>앞21열 입력만 설치하며 뒤 관측 열은 읽지 않는다.</summary>
        public Scene(string[] row) : base(EntryInput(row))
        {
            Root.Vtable = Helper.Vtable = Background.Vtable = Scrolling.Vtable = ChildVtable; Root.Flags = Helper.Flags = Background.Flags = Scrolling.Flags = 1; Root.Scale = Helper.Scale = Background.Scale = Scrolling.Scale = 1; Root.BaseWidth = unchecked((int)U(row[2])); Root.BaseHeight = unchecked((int)U(row[3])); Root.Console = EntryToken; Root.Text = ScrollToken; Root.Helper = HelperToken; Root.Background = BackgroundToken; Node.Serial = 21; Helper.Serial = 23; Scrolling.Buffer = SharedToken; ChatState.Current.Value = RootToken; ChatState.Registered = U(row[9]); ChatState.CompletionReady = row[15] == "~" ? (byte)0 : (byte)1; _resolved = row[14] == "~" ? null : Input(row[14]); _recipient = row[13] == "1";
            Values.Add(0, U(row[12])); Values.Add(0x540424, 9); Values.Add(0x5acd24, 9); Values.Add(0x5b5dc0, 32); Values.Add(0x5b5e60, 30); Values.Add(0x5b5dd4, 31); Values.Add(0x5b5dd8, 33); Values.Add(0x594fbc, U(row[10])); Values.Add(0x594fc8, U(row[11])); Values.Add(0x548714, 0x11223344); Values.Add(0x5cae94, 11); Values.Add(0x5cae98, 22); Values.Add(0x563e38, 33); Values.Add(0x542b24, 0x2000201); Values.Add(0x5427a8, 0x2000021); Values.Add(0x5429f0, 0x20000bf); Values.Add(0x542914, 0x2000202); Values.Add(0x5427d8, 0x2000038); Values.Add(0x542794, 0x2000203); Values.Add(0x542af8, 0x2000204); Values.Add(0x54298c, 0x2000205); Values.Add(0x542b08, 0x2000206); Values.Add(0x542b0c, 0x2000207); Values.Add(0x5428ac, 0x2000051); Values.Add(0x541348, 2);
            // 세 실제 후보 벡터의 원문 입력만 복원한다.
            foreach (var (list, column) in new[] { (ChatState.Colors, 15), (ChatState.Backgrounds, 16), (ChatState.Icons, 17) }) if (row[column] != "~") list.AddRange(row[column].Split(':').Select(Input));
            Chat = new(ChatState, ChatHooks());
        }
        /// <summary>같은 실제 부모/큰 공통 장치의 인자와 순서만 관찰한다.</summary>
        private ChatWindowHooks ChatHooks() => new()
        {
            Chat = token => token == RootToken ? Root : null, Base = Base, Console = token => token == EntryToken ? Node : null, Text = token => token == ScrollToken ? Scrolling : null, Buffer = token => token == BufferToken ? Buffer : null, Entry = Gump, Global = address => Values[address],
            Allocate = size => { uint token = size switch { 0x13c => RootToken, 0x170 => HelperToken, 0xb8 => BackgroundToken, 0x128 => EntryToken, 0xd8 => ScrollToken, _ => 0 }; Event("new", (uint)size, token); return token; },
            ConstructBase = (token, width, height) => { Root.Vtable = ChildVtable; Root.Flags = 1; Root.Scale = 1; Root.BaseWidth = Root.Width = width; Root.BaseHeight = Root.Height = height; Event("constructBase", token, 0, Bits(width), Bits(height), 1); },
            ConstructHelper = (token, label, brown) => { Helper.Vtable = ChildVtable; Helper.Flags = 1; Helper.Scale = 1; Words("constructHelper", token.ToString(CultureInfo.InvariantCulture), Hex(label), brown.ToString(CultureInfo.InvariantCulture)); },
            FitHelper = token => { Helper.BaseWidth = 9; Helper.BaseHeight = 14; Event("fitHelper", token); }, Title = (token, label) => Words("title", token.ToString(CultureInfo.InvariantCulture), Hex(label)), Help = (token, label) => Words("help", token.ToString(CultureInfo.InvariantCulture), Hex(label)), Translate = label => { Words("translate", Hex(label)); return label; },
            Measure = (token, label) => { Words("measureChat", token.ToString(CultureInfo.InvariantCulture), Hex(label)); return (6, 14); },
            ConstructBackground = (token, width, height, flags) => { Background.Vtable = ChildVtable; Background.Flags = 1; Background.Scale = 1; Background.BaseWidth = width; Background.BaseHeight = height; Background.BackgroundFlags = flags; Event("constructChatBackground", token, Bits(width), Bits(height), flags); },
            ConstructScrolling = (token, width, height, font, ink) => { Scrolling.Vtable = ChildVtable; Scrolling.Flags = 1; Scrolling.Scale = 1; Scrolling.BaseWidth = width; Scrolling.BaseHeight = height; Scrolling.Buffer = SharedToken + 0x100; Scrolling.TextFlags = 0x10; Event("constructScrolling", token, unchecked((uint)width), unchecked((uint)height), font, ink); },
            ReplaceBuffer = (token, shared, owns) => { Scrolling.Buffer = shared; Scrolling.OwnsBuffer = owns; Event("replaceBuffer", token, shared, owns); }, SharedBuffer = () => 0x548758, SetSlider = (token, value) => Event("setSlider", token, value), SetScrollable = (token, value) => Event("setScrollable", token, value),
            Attach = (token, x, y, parent, mode) => { if (token == EntryToken) Gump.Attach(token, x, y, parent, mode); else Event("attach", token, Bits(x), Bits(y), parent, mode); }, Push = token => Event("push", token), DestructBase = token => Event("destructBase", token),
            RegisterCommand = (name, callback, flag) => Words("register", Hex(name), callback.ToString(CultureInfo.InvariantCulture), flag.ToString(CultureInfo.InvariantCulture)), BufferAppendCallback = (token, callback) => Event("appendCallback", token, callback), FrameCount = type => { Event("frameCount", type, 7); return 7; }, FrameHeight = (type, frame) => { short value = new short[] { -1, 7, 8, 9, 16, 17, 32767 }[frame]; Event("frameHeight", type, frame, unchecked((uint)(int)value)); return value; },
            RequestScroll = (token, value) => Event("requestScroll", token, unchecked((uint)value)), RelativeScroll = (token, value) => Event("relativeScroll", token, unchecked((uint)value)), RequestBottom = token => Event("requestBottom", token), BaseHandle = (token, input, first, second) => { Event("baseHandle", token, input, first, second); return 77; }, Bubble = (token, input, first, second) => Event("bubble", token, input, first, second), Recipient = label => { Words("recipient", Hex(label)); return _recipient; }, SendChat = (manager, label) => Words("sendChat", manager.ToString(CultureInfo.InvariantCulture), Hex(label)),
            Resolve = record => { uint subscriber = BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(4, 4)); Event("resolve", subscriber); if (_resolved is null) return false; Encoding.Latin1.GetBytes(_resolved + '\0').CopyTo(record, 8); return true; }, Log = (level, label, subscriber) => Words("log", level.ToString(CultureInfo.InvariantCulture), Hex(label), subscriber.ToString(CultureInfo.InvariantCulture)), Config = key => { Words("config", Hex(key)); return unchecked((int)Values[0]); },
            MenuBegin = () => Event("menuBegin"), MenuTitle = label => Words("menuTitle", Hex(label)), MenuItem = (label, input, payload) => Words("menuItem", Hex(label), input.ToString(CultureInfo.InvariantCulture), payload is null ? "~" : Hex(payload)), MenuChecked = (check, label, input, payload) => Words("menuChecked", check.ToString(CultureInfo.InvariantCulture), Hex(label), input.ToString(CultureInfo.InvariantCulture), Hex(payload)), MenuTake = () => { Event("menuTake", ++_menu); return _menu; }, MenuSubmenu = (label, child) => Words("menuSubmenu", Hex(label), child.ToString(CultureInfo.InvariantCulture)), MenuFinalize = () => Event("menuFinalize"), MenuOpen = (x, y, reference) => Event("menuOpen", x, y, reference),
        };
        /// <summary>현재 실제 root의 원문16DWORD 관측이다.</summary>
        public string RootFields() => Pack(Root.Vtable, Root.Flags, Root.Command, Bits(Root.BaseWidth), Bits(Root.BaseHeight), Bits(Root.Width), Bits(Root.Height), Root.Console, Root.Text, Root.Helper, Root.Background, Root.Capacity, unchecked((uint)Root.LeftPadding), ChatState.Current.Value, ChatState.Registered, ChatState.CompletionReady);
        /// <summary>실제 현재 후보 문자열을 원문 순서대로 관찰한다.</summary>
        public string CompletionFields() => string.Join(';', new[] { ChatState.Colors, ChatState.Backgrounds, ChatState.Icons }.Select(list => list.Count == 0 ? "~" : string.Join(':', list.Select(value => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant()))));
    }
    /// <summary>독립180관측의 전체 root/Console/버퍼/후보/trace/반환을 대조한다.</summary>
    [Fact]
    public void ReplaysNative180WithActualConsoleAndBuffer()
    {
        int rows = 0; var fullObservations = new HashSet<string>(StringComparer.Ordinal);
        // 앞21열 입력과 뒤10열 관측을 구별하고 기대값은 원문에서만 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-window-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(31, row.Length); fullObservations.Add(string.Join('\t', row.Where((_, index) => index != 0 && index != 26))); var scene = new Scene(row); uint result = 0;
            if (row[1] == "construct") scene.Chat.Construct(RootToken, unchecked((int)U(row[2])), unchecked((int)U(row[3]))); else if (row[1] == "create") scene.Chat.Create(); else if (row[1] == "destroy") scene.Chat.Destruct(RootToken); else if (row[1] == "normalize") scene.Chat.NormalizePrefix(RootToken); else { Assert.Equal("handle", row[1]); result = scene.Chat.Handle(RootToken, U(row[4]), U(row[5]), row[6] == "~" ? PayloadToken : U(row[6]), row[6] == "~" ? Input(row[7]) : null); }
            Assert.Equal(row[21], scene.RootFields()); Assert.Equal(row[22], scene.Fields()); Assert.Equal(row[23], Pack(scene.Scrolling.Buffer, scene.Scrolling.OwnsBuffer, scene.Scrolling.TextFlags, scene.Scrolling.Callback)); Assert.Equal(row[24], Hex(Prefix(scene.Root.InputPrefix))); Assert.Equal(row[25], Hex(scene.Buffer.Text())); Assert.Equal(row[26], scene.CompletionFields()); Assert.Equal(row[27], scene.Trace()); Assert.Equal(U(row[28]), result); Assert.Equal(row[1] == "construct" ? 12u : row[1] == "handle" ? 16u : 4u, U(row[29])); Assert.True(U(row[30]) > 0); rows++;
        }
        Assert.Equal(180, rows); int baseRows = 0;
        // 이름 재번호/후보열을 제외한 기본166관측도 실행한 전체180관측의 정확한 부분집합이어야 한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-window-base-x86.tsv")).Skip(1)) { string[] row = line.Split('\t'); Assert.Equal(30, row.Length); Assert.Contains(string.Join('\t', row.Skip(1)), fullObservations); baseRows++; }
        Assert.Equal(166, baseRows);
    }
    /// <summary>로그 문맥은 자식에서 저장해 부모에 복원하며 예외에서는 현재 부분 효과를 유지한다.</summary>
    [Fact]
    public void AppendLogRestoresChildContextToParentOnlyOnSuccess()
    {
        var root = new HudChatNode { Text = 2 }; var child = new ScrollingTextGumpNode(); uint[] context = [10, 11, 12, 13, 14, 15, 16, 17]; context.CopyTo(child.TextContext, 0); bool fail = false; int bottom = 0;
        var chat = new ChatWindow(new(), new() { Chat = token => token == 1 ? root : null, Base = token => token == 1 ? root : token == 2 ? child : null, AtBottom = _ => true, AppendLog = (_, _) => { Assert.Equal(20u, root.TextContext[0]); root.TextContext[0] = 999; if (fail) throw new OperationCanceledException(); }, RequestBottom = _ => bottom++ });
        chat.AppendLog(1, "x", new uint[] { 20, 21, 22, 23, 24, 25, 26, 27 }); Assert.Equal(context, root.TextContext); Assert.Equal(context, child.TextContext); Assert.Equal(1, bottom); fail = true; Assert.Throws<OperationCanceledException>(() => chat.AppendLog(1, "x", new uint[] { 20, 21, 22, 23, 24, 25, 26, 27 })); Assert.Equal(999u, root.TextContext[0]); Assert.Equal(1, bottom);
    }
    /// <summary>생성은 노드를 조회하지 않고 소멸은 다른 current도 무조건0으로 쓴다.</summary>
    [Fact]
    public void BindingIsLazyAndDestructorClearsOtherCurrent()
    {
        var root = new HudChatNode(); var state = new ChatWindowState(); state.Current.Value = 77; int reads = 0; bool destroyed = false; var chat = new ChatWindow(state, new() { Chat = _ => { reads++; return root; }, Base = _ => root, DestructBase = token => { Assert.Equal(1u, token); Assert.Equal(0u, state.Current.Value); destroyed = true; } }); Assert.Equal(0, reads); chat.Destruct(1); Assert.True(destroyed); Assert.Equal(0x504a94u, root.Vtable); Assert.Equal(1, reads);
    }
}
