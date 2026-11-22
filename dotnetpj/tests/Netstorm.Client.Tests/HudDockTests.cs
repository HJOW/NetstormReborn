using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 원본 HUD의 전체 바이트와 명시 외부 효과를 비교하는 영역이다.

/// <summary>기존 독립 PE 관측을 재생하며 기대 기하를 검사 코드에서 계산하지 않는다.</summary>
public sealed class HudDockTests
{
    /// <summary>격리 원본 노드0의 포인터.</summary>
    private const uint NodeAddress = 0x01000100;
    /// <summary>격리 노드 사이의 원시 주소 간격.</summary>
    private const uint NodeStride = 0x200;
    /// <summary>각 노드에서 관측한 전체 바이트 폭.</summary>
    private const int RawBytes = 0x11c;
    /// <summary>기본 HUD와7자식의 고정 관측 수.</summary>
    private const int NodeCount = 8;
    /// <summary>명시 callback 중단이며 정상 복귀와 구별한다.</summary>
    private sealed class ExplicitStop : Exception;
    /// <summary>signed 입력도 같은 하위DWORD로 읽는다.</summary>
    private static uint Number(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>원시 단정도 비트를 복원한다.</summary>
    private static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>현재 단정도의 모든 비트를 관찰한다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>격리 포인터를1기반 검사 토큰으로만 변환한다.</summary>
    private static uint Token(uint address)
    {
        if (address == 0) return 0;
        if (address < NodeAddress || (address - NodeAddress) % NodeStride != 0 || (address - NodeAddress) / NodeStride >= NodeCount) throw new InvalidDataException("HUD 원시 포인터 범위");
        return (address - NodeAddress) / NodeStride + 1;
    }
    /// <summary>검사 토큰을 같은 원문 격리 주소로 직렬화한다.</summary>
    private static uint Address(uint token) => token == 0 ? 0 : NodeAddress + (token - 1) * NodeStride;
    /// <summary>little endian DWORD를 읽는다.</summary>
    private static uint Get(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset));
    /// <summary>little endian DWORD를 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);
    /// <summary>실제 인자만 문화권 없이 순서대로 관찰한다.</summary>
    private static string Packed(IEnumerable<uint> values) => string.Join(':', values.Select(value => value.ToString(CultureInfo.InvariantCulture)));

    /// <summary>앞6열 입력으로만 같은 실제 노드/전역과 명시 외부 장치를 구성한다.</summary>
    private sealed class Scene
    {
        /// <summary>서로 다른 기본/파생 사본 없이 공유하는8노드.</summary>
        public readonly HudDividerNode[] Nodes = Enumerable.Range(0, NodeCount).Select(_ => new HudDividerNode()).ToArray();
        /// <summary>현재 실제 HUD this의8필드.</summary>
        public readonly uint[] Parts = new uint[NodeCount];
        /// <summary>현재 signed 화면 크기의 실제 owner.</summary>
        public readonly GumpCreateState Create = new();
        /// <summary>지도 정렬기가 읽는 같은 실제 정수 영역.</summary>
        public readonly GumpTreeState TreeState = new();
        /// <summary>현재 Deck 표시 DWORD.</summary>
        public readonly UIntBox ShowHud = new();
        /// <summary>같은 미니맵 갱신 DWORD.</summary>
        public readonly UIntBox Revision = new();
        /// <summary>현재 표시 상태와 두 실제 DWORD에 연결된 HUD 전역.</summary>
        public readonly HudDockGlobals Globals;
        /// <summary>같은 노드/명시 가상 Attach를 사용하는 공통 Move.</summary>
        private readonly GumpTree _tree;
        /// <summary>실행하는 제품 전체 본체.</summary>
        public readonly HudDock Dock;
        /// <summary>선택된 원문 진입 이름.</summary>
        private readonly string _operation;
        /// <summary>원문 serial 조회 장치의 현재 존재 마스크.</summary>
        private uint _presentMask;
        /// <summary>앞6열의 명시 mutation만 저장한다.</summary>
        private readonly string[][] _effects;
        /// <summary>경계별1기반 호출 번호.</summary>
        private readonly Dictionary<string, int> _calls = [];
        /// <summary>실제 공개 호출 순서와 인자.</summary>
        public readonly List<string> Trace = [];
        /// <summary>현재 유효 토큰의 같은 실제 객체.</summary>
        public HudDividerNode Node(uint token) => token is >= 1 and <= NodeCount ? Nodes[token - 1] : throw new InvalidDataException("HUD 검사 토큰");
        /// <summary>기본 노드와 확장 필드를 원문 offset으로만 복원하며 객체 참조는 유지한다.</summary>
        private void DecodeNode(int index, byte[] raw)
        {
            HudDividerNode node = Nodes[index]; GumpCreateNode basis = GumpRawSupport.Decode(raw);
            // 기본 필드만 같은 객체에 대입하며 빌린 참조를 교체하지 않는다.
            foreach (var property in typeof(GumpCreateNode).GetProperties()) if (property.CanWrite) property.SetValue(node, property.GetValue(basis));
            basis.Positioning.CopyTo(node.Positioning, 0); basis.TextContext.CopyTo(node.TextContext, 0); node.Parent = Token(node.Parent);
            node.Shape = Get(raw, 0xa0); node.TileWidth = unchecked((int)Get(raw, 0xa4)); node.TileHeight = unchecked((int)Get(raw, 0xa8)); node.Type = Get(raw, 0xac); node.Frame = Get(raw, 0xb0); node.BackgroundFlags = Get(raw, 0xb4);
            node.Axis = Get(raw, 0xb8); node.MinX = unchecked((int)Get(raw, 0xbc)); node.MaxX = unchecked((int)Get(raw, 0xc0)); node.MinY = unchecked((int)Get(raw, 0xc4)); node.MaxY = unchecked((int)Get(raw, 0xc8));
            // 원문의 고정20DWORD를 슬롯 순서대로 읽는다.
            for (int slot = 0; slot < node.Slaves.Length; slot++) node.Slaves[slot] = (Get(raw, 0xcc + 8 * slot), Get(raw, 0xd0 + 8 * slot));
            if (_operation == "hud" && index == 0)
            {
                uint[] parts = [1, Token(node.Shape), Token(unchecked((uint)node.TileWidth)), Token(unchecked((uint)node.TileHeight)), Token(node.Type), Token(node.Frame), Token(node.Axis), Token(unchecked((uint)node.MaxX))]; parts.CopyTo(Parts, 0);
            }
        }
        /// <summary>기대 계산 없이 현재 필드를 원문 offset으로만 직렬화한다.</summary>
        private byte[] EncodeNode(int index)
        {
            HudDividerNode node = Nodes[index]; GumpCreateNode basis = GumpRawSupport.Decode(Convert.FromHexString(GumpRawSupport.Encode(node)));
            basis.Vtable = 0; basis.Parent = Address(basis.Parent); byte[] raw = new byte[RawBytes]; Convert.FromHexString(GumpRawSupport.Encode(basis)).CopyTo(raw, 0);
            Put(raw, 0xa0, node.Shape); Put(raw, 0xa4, unchecked((uint)node.TileWidth)); Put(raw, 0xa8, unchecked((uint)node.TileHeight)); Put(raw, 0xac, node.Type); Put(raw, 0xb0, node.Frame); Put(raw, 0xb4, node.BackgroundFlags);
            Put(raw, 0xb8, node.Axis); Put(raw, 0xbc, unchecked((uint)node.MinX)); Put(raw, 0xc0, unchecked((uint)node.MaxX)); Put(raw, 0xc4, unchecked((uint)node.MinY)); Put(raw, 0xc8, unchecked((uint)node.MaxY));
            // 쓰지 않은 슬롯도 전체 비교에 포함한다.
            for (int slot = 0; slot < node.Slaves.Length; slot++) { Put(raw, 0xcc + 8 * slot, node.Slaves[slot].Serial); Put(raw, 0xd0 + 8 * slot, node.Slaves[slot].Flags); }
            if (_operation == "hud" && index == 0)
            {
                Put(raw, 0xa0, Address(Parts[1])); Put(raw, 0xa4, Address(Parts[2])); Put(raw, 0xa8, Address(Parts[3])); Put(raw, 0xac, Address(Parts[4])); Put(raw, 0xb0, Address(Parts[5])); Put(raw, 0xb8, Address(Parts[6])); Put(raw, 0xc0, Address(Parts[7]));
            }
            return raw;
        }
        /// <summary>현재22DWORD를 원문 장부 순서로 관찰한다.</summary>
        public uint[] GlobalWords() => [unchecked((uint)Create.ScreenWidth), unchecked((uint)Create.ScreenHeight), ShowHud.Value,
            unchecked((uint)Globals.MapLeft), unchecked((uint)Globals.MapTop), unchecked((uint)Globals.MapWidth), unchecked((uint)Globals.MapHeight),
            Bits(Globals.MapFloatRect[0]), Bits(Globals.MapFloatRect[1]), Bits(Globals.MapFloatRect[2]), Bits(Globals.MapFloatRect[3]),
            unchecked((uint)Globals.MapRect.Left), unchecked((uint)Globals.MapRect.Top), unchecked((uint)Globals.MapRect.Right), unchecked((uint)Globals.MapRect.Bottom),
            unchecked((uint)Globals.LeftBottomRect.Left), unchecked((uint)Globals.LeftBottomRect.Top), unchecked((uint)Globals.LeftBottomRect.Right), unchecked((uint)Globals.LeftBottomRect.Bottom), Revision.Value, Bits(Globals.SavedChatY), _presentMask];
        /// <summary>명시 입력값만 현재 실제 전역에 쓴다.</summary>
        private void SetGlobals(uint[] words)
        {
            Create.ScreenWidth = unchecked((int)words[0]); Create.ScreenHeight = unchecked((int)words[1]); ShowHud.Value = words[2];
            Globals.MapLeft = unchecked((int)words[3]); Globals.MapTop = unchecked((int)words[4]); Globals.MapWidth = unchecked((int)words[5]); Globals.MapHeight = unchecked((int)words[6]);
            // float32 네 필드도 입력 비트를 보존한다.
            for (int index = 0; index < 4; index++) Globals.MapFloatRect[index] = Float(words[7 + index]);
            Globals.MapRect = new(unchecked((int)words[11]), unchecked((int)words[12]), unchecked((int)words[13]), unchecked((int)words[14]));
            Globals.LeftBottomRect = new(unchecked((int)words[15]), unchecked((int)words[16]), unchecked((int)words[17]), unchecked((int)words[18])); Revision.Value = words[19]; Globals.SavedChatY = Float(words[20]); _presentMask = words[21];
        }
        /// <summary>callback 입력이 지정한 원문 저장 위치만 변경한다.</summary>
        private void Effect(string[] effect)
        {
            uint value = Number(effect[3]);
            if (effect[1].StartsWith("node", StringComparison.Ordinal))
            {
                Dictionary<string, int> offsets = new() { ["flags"] = 4, ["parent"] = 0x14, ["type"] = 0x20, ["serial"] = 0x2c, ["scale"] = 0x38, ["baseWidth"] = 0x3c, ["baseHeight"] = 0x40, ["width"] = 0x44, ["height"] = 0x48, ["x"] = 0x4c, ["y"] = 0x50, ["axis"] = 0xb8, ["minX"] = 0xbc, ["maxX"] = 0xc0, ["minY"] = 0xc4, ["maxY"] = 0xc8, ["link0serial"] = 0xcc, ["link0flags"] = 0xd0, ["link1serial"] = 0xd4, ["link1flags"] = 0xd8, ["vertical"] = 0xa0, ["minimapDivider"] = 0xa4, ["chatDivider"] = 0xa8, ["minimap"] = 0xac, ["production"] = 0xb0, ["chat"] = 0xb8, ["knowledge"] = 0xc0 };
                int index = int.Parse(effect[1].AsSpan(4), CultureInfo.InvariantCulture); byte[] raw = EncodeNode(index); Put(raw, offsets[effect[2]], value); DecodeNode(index, raw);
            }
            else if (effect[1] == "global")
            {
                string[] names = ["screenWidth", "screenHeight", "showHud", "mapLeft", "mapTop", "mapWidth", "mapHeight", "floatLeft", "floatTop", "floatRight", "floatBottom", "rectLeft", "rectTop", "rectRight", "rectBottom", "bottomLeft", "bottomTop", "bottomRight", "bottomBottom", "fullRefresh", "savedChatY", "presentMask"];
                int index = Array.IndexOf(names, effect[2]); if (index < 0) throw new InvalidDataException("HUD 전역 효과 이름"); uint[] words = GlobalWords(); words[index] = value; SetGlobals(words);
            }
            else if (effect[1] != "control") throw new InvalidDataException("HUD 효과 대상");
        }
        /// <summary>명시 기본효과 뒤 같은 호출번호의 mutation을 순서대로 적용한다.</summary>
        private void Event(string name, params uint[] values)
        {
            Trace.Add(name + (values.Length == 0 ? "" : ":" + Packed(values))); int count = _calls.GetValueOrDefault(name) + 1; _calls[name] = count; string trigger = name + count.ToString(CultureInfo.InvariantCulture); bool stop = false;
            // 원문 경계에서 입력한 부분 쓰기와 중단을 보존한다.
            foreach (string[] effect in _effects) if (effect[0] == trigger) { Effect(effect); if (effect[1] == "control" && effect[2] == "stop" && effect[3] == "1") stop = true; }
            if (stop) throw new ExplicitStop();
        }
        /// <summary>관측에서 명시한 기본 Attach 장치의 상태 변경만 수행한다.</summary>
        private void BaseAttach(string name, uint token, float x, float y, uint parent, uint mode)
        {
            HudDividerNode node = Node(token); node.Flags &= ~8U; node.X = x; node.Y = y; if (parent != 0) node.Parent = parent; Event(name, token, Bits(x), Bits(y), parent, mode);
        }
        /// <summary>관측의 명시 가상 Push이며 공통 링크 전체 검사로 확대하지 않는다.</summary>
        private void Push(uint token) { Node(token).Flags |= 8; Event("push", token); }
        /// <summary>원문 중첩 구분선 본체와 일반 자식의 명시 Attach를 구별한다.</summary>
        private void Attach(uint token, float x, float y, uint parent, uint mode)
        {
            if (_operation == "hud" && token is >= 2 and <= 4) Dock.Attach(token, x, y, parent, mode); else BaseAttach("attach", token, x, y, parent, mode);
        }
        /// <summary>관측에서 명시한 외부 장치만 공급하며 제품 기하를 계산하지 않는다.</summary>
        private HudDockHooks Hooks() => new()
        {
            Node = token => Node(token), Divider = token => Node(token),
            ConstructBackground = (token, width, height, flags, type, frame) => { HudDividerNode node = Node(token); node.Flags = 9; node.Scale = 1; node.BaseWidth = node.Width = width; node.BaseHeight = node.Height = height; node.Type = type; node.Frame = frame; node.BackgroundFlags = flags; Event("background", token, Bits(width), Bits(height), flags, 1, type, frame); },
            Activate = (token, active) => Event("activate", token, active), BaseAttach = (token, x, y, parent, mode) => BaseAttach("baseAttach", token, x, y, parent, mode),
            Push = Push, Attach = Attach, Move = (token, x, y, parent) => _tree.Move(token, x, y, parent),
            ResolveSerial = serial =>
            {
                uint token = 0;
                // 존재 마스크가 허용하는 첫 현재 일치 serial만 해결한다.
                for (int index = 0; index < Nodes.Length; index++) if ((_presentMask & (1U << index)) != 0 && Nodes[index].Serial == serial) { token = (uint)index + 1; break; }
                Event("resolve", serial, token); return token;
            },
            Bubble = (token, eventCode, first, second) => { Event("bubble", token, eventCode, first, second); if (_operation == "hud") Dock.Handle(eventCode, first, second); },
            ClosePopups = () => Event("closePopups"), Invalidate = (x, y, width, height, flags) => Event("invalidate", unchecked((uint)x), unchecked((uint)y), unchecked((uint)width), unchecked((uint)height), flags),
            FinishResize = () => Event("finish"), Assertion = line => Event("assert", line), BaseHandle = (eventCode, first, second) => { Event("baseHandle", eventCode, first, second); return 73; },
            ChatVisible = value => Event("chatVisible", value ? 1U : 0U), ChatInput = value => Event("chatInput", value ? 1U : 0U),
        };
        /// <summary>앞6열만 해석한다. 뒤8열의 CPU 출력은 실행 완료 후 비교에만 사용한다.</summary>
        public Scene(string[] row)
        {
            _operation = row[1]; Globals = new(Create, TreeState, ShowHud, Revision); byte[] raw = Convert.FromHexString(row[2]); Assert.Equal(RawBytes * NodeCount, raw.Length);
            // 고정8노드의 원시 입력을 같은 객체에 공급한다.
            for (int index = 0; index < Nodes.Length; index++) DecodeNode(index, raw.AsSpan(index * RawBytes, RawBytes).ToArray());
            SetGlobals(row[3].Split(':').Select(Number).ToArray()); _effects = row[5] == "-" ? [] : row[5].Split(';').Select(effect => effect.Split('/')).ToArray();
            _tree = new(Create, TreeState, new() { Node = token => Node(token), Redraw = (_, _) => { }, Push = Push, Attach = Attach, Kill = _ => { } });
            Dock = new(part => Parts[(int)part], Globals, () => 55, Hooks());
        }
        /// <summary>현재 전체8노드를 원문 주소 순서로 관찰한다.</summary>
        public string NodesOutput() => string.Concat(Enumerable.Range(0, NodeCount).Select(index => Convert.ToHexStringLower(EncodeNode(index))));
        /// <summary>현재22DWORD만 원문 순서로 관찰한다.</summary>
        public string GlobalsOutput() => Packed(GlobalWords());
    }

    /// <summary>1075입력×두x87 정밀도의 전체 바이트·현재 전역·효과·부분 중단2150관측을 대조한다.</summary>
    [Fact]
    public void IndependentNativeBodiesAndNestedViewport()
    {
        int rows = 0, stopped = 0;
        // 앞6열 입력으로만 실행한 뒤 원문 CPU 관측과 비교한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "hud-dock-geometry-x86.tsv")))
        {
            if (line.Length == 0 || line.StartsWith("name\t", StringComparison.Ordinal)) continue;
            string[] row = line.Split('\t'); Assert.Equal(14, row.Length); var scene = new Scene(row); uint[] args = row[4].Split(':').Select(Number).ToArray(); uint result = 0; bool completed = true;
            try
            {
                switch (row[1])
                {
                    case "construct": result = scene.Dock.Construct(1, args[0], unchecked((int)args[1]), unchecked((int)args[2]), args[3], args[4], args[5]); break;
                    case "register": scene.Dock.RegisterSlave(1, args[0], args[1]); break;
                    case "reflow": scene.Dock.Attach(1, Float(args[0]), Float(args[1]), args[2], args[3]); break;
                    case "hud": result = scene.Dock.Handle(args[0], args[1], args[2]); break;
                    default: throw new InvalidDataException("HUD 진입 이름");
                }
            }
            catch (ExplicitStop) { completed = false; stopped++; }
            Assert.True(completed == (row[6] == "1"), row[0] + " 완료 관문");
            Assert.True(scene.NodesOutput() == row[7], row[0] + " 전체 노드 바이트"); Assert.True(scene.GlobalsOutput() == row[8], row[0] + " 전체 전역");
            Assert.True((scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace)) == row[9], row[0] + " 실제 호출 순서/인자");
            if (completed && row[1] is "construct" or "hud") Assert.Equal(Number(row[10]), result);
            if (completed) Assert.Equal((uint)(4 + args.Length * 4), Number(row[11])); Assert.True(Number(row[12]) > 0 && Number(row[12]) >= Number(row[13])); rows++;
        }
        Assert.Equal(2150, rows); Assert.Equal(6, stopped);
    }
}
