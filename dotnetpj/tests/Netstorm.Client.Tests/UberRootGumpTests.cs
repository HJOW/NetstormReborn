using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests; // 원문 전체 HUD 부모와 공통 등록/수명 연결을 대조하는 영역이다.

/// <summary>원본160관측의 앞11입력만 실행하고 뒤7출력과 전체 원시 상태를 비교한다.</summary>
public sealed class UberRootGumpTests
{
    /// <summary>격리 원본의 고정 노드 시작 주소.</summary>
    private const uint NodeBase = 0x1100000;
    /// <summary>격리 원본 노드 사이의 주소 간격.</summary>
    private const uint Stride = 0x200;
    /// <summary>명시 기본 생성 장치의 고정 vtable 표식.</summary>
    private const uint Table = 0x1000000;
    /// <summary>같은 현재 메시지 포인터.</summary>
    private const uint Title = 0x565e70;
    /// <summary>명시 표현식 작성 장치의 반환 포인터.</summary>
    private const uint Format = 0x1003100;
    /// <summary>같은 공통 생성 count 주소.</summary>
    private const uint CountAddress = 0x565dc8;
    /// <summary>같은 공통 생성 serial 주소.</summary>
    private const uint SerialAddress = 0x540418;
    /// <summary>원본 자료의13전역 출력 순서.</summary>
    private static readonly uint[] GlobalAddresses = [0x540451, 0x5411c8, 0x542664, 0x5ca920, 0x5ca924, 0x5ca928, 0x5c78f0, 0x5c78f4, 0x531860, 0x531864, 0x540cec, 0x540cf8, 0x542a04];
    /// <summary>명시 callback 중단을 정상 복귀와 구별한다.</summary>
    private sealed class ExplicitStop : Exception;
    /// <summary>signed 숫자도 같은 하위DWORD로 해석한다.</summary>
    private static uint N(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>실제 단정도 비트를 복원한다.</summary>
    private static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>현재 단정도의 원시 비트.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>현재 실제 숫자를 문화권 없이 기록한다.</summary>
    private static string Num(uint value) => value.ToString(CultureInfo.InvariantCulture);
    /// <summary>원문 포인터에서 물리32슬롯 번호만 구한다.</summary>
    private static int Index(uint token)
    {
        if (token < NodeBase || (token - NodeBase) % Stride != 0 || (token - NodeBase) / Stride >= 32) throw new InvalidDataException("Uber 검사 토큰 범위");
        return (int)((token - NodeBase) / Stride);
    }
    /// <summary>같은 물리 슬롯의 명시 원문 주소.</summary>
    private static uint Address(int index) => NodeBase + (uint)index * Stride;
    /// <summary>문자열 경계의 실제 바이트를 원문 trace 형식으로 기록한다.</summary>
    private static string TextHex(string text) => Convert.ToHexStringLower(Encoding.Latin1.GetBytes(text));
    /// <summary>little endian DWORD를 원본 offset에서 읽는다.</summary>
    private static uint Get(byte[] raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset));
    /// <summary>little endian DWORD를 원본 offset에 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);

    /// <summary>0x180 전체 guard와 같은 실제 기본/파생 주소를 함께 보유한다.</summary>
    private sealed class Slot
    {
        /// <summary>명시 입력의 전체 원시 바이트. 의미 필드 밖 guard도 보존한다.</summary>
        public byte[] Raw;
        /// <summary>같은 슬롯의 실제 기본 또는 파생 객체.</summary>
        public GumpCreateNode Value = new();
        /// <summary>현재 실제 할당 종류.</summary>
        public char Kind = 'b';
        /// <summary>공통 수명 검사에서 실제 반납한 표식.</summary>
        public bool Released;
        /// <summary>원문 미작성 바이트까지 같은 입력값으로 채운다.</summary>
        public Slot(byte fill) { Raw = Enumerable.Repeat(fill, 0x180).ToArray(); Decode('b'); }
        /// <summary>같은 부모의 뒤10DWORD를 원문 필드 순서로 관찰한다.</summary>
        private static uint[] RootWords(UberRootNode node) => [node.Vertical, node.MinimapDivider, node.ChatDivider, node.Minimap, node.Production, node.ReservedB4, node.Chat, node.ReservedBC, node.Knowledge, node.Message];
        /// <summary>값만 복원하며 같은 종류의 실제 객체 주소는 교체하지 않는다.</summary>
        public void Decode(char kind)
        {
            if (kind != Kind) Value = kind switch { 'r' => new UberRootNode(), 'd' => new HudDividerNode(), _ => new GumpCreateNode() };
            Kind = kind; GumpCreateNode basis = GumpRawSupport.Decode(Raw);
            // 공통 의미 필드만 같은 객체에 대입한다.
            foreach (var property in typeof(GumpCreateNode).GetProperties()) if (property.CanWrite) property.SetValue(Value, property.GetValue(basis));
            basis.Positioning.CopyTo(Value.Positioning, 0); basis.TextContext.CopyTo(Value.TextContext, 0);
            if (Value is UberRootNode root)
            {
                root.Vertical = Get(Raw, 0xa0); root.MinimapDivider = Get(Raw, 0xa4); root.ChatDivider = Get(Raw, 0xa8); root.Minimap = Get(Raw, 0xac); root.Production = Get(Raw, 0xb0);
                root.ReservedB4 = Get(Raw, 0xb4); root.Chat = Get(Raw, 0xb8); root.ReservedBC = Get(Raw, 0xbc); root.Knowledge = Get(Raw, 0xc0); root.Message = Get(Raw, 0xc4);
            }
            if (Value is HudDividerNode divider)
            {
                divider.Shape = Get(Raw, 0xa0); divider.TileWidth = unchecked((int)Get(Raw, 0xa4)); divider.TileHeight = unchecked((int)Get(Raw, 0xa8)); divider.Type = Get(Raw, 0xac); divider.Frame = Get(Raw, 0xb0); divider.BackgroundFlags = Get(Raw, 0xb4); divider.Axis = Get(Raw, 0xb8);
                divider.MinX = unchecked((int)Get(Raw, 0xbc)); divider.MaxX = unchecked((int)Get(Raw, 0xc0)); divider.MinY = unchecked((int)Get(Raw, 0xc4)); divider.MaxY = unchecked((int)Get(Raw, 0xc8));
                // 고정10슬롯도 원시 순서로 복원한다.
                for (int index = 0; index < divider.Slaves.Length; index++) divider.Slaves[index] = (Get(Raw, 0xcc + index * 8), Get(Raw, 0xd0 + index * 8));
            }
        }
        /// <summary>현재 의미 필드만 원본 offset에 쓰고 미작성 guard는 그대로 둔다.</summary>
        public byte[] Encode()
        {
            byte[] raw = (byte[])Raw.Clone(); Convert.FromHexString(GumpRawSupport.Encode(Value)).CopyTo(raw, 0);
            if (Value is UberRootNode root)
            {
                uint[] words = RootWords(root);
                // 원문 확장10DWORD를 같은 순서로만 쓴다.
                for (int index = 0; index < words.Length; index++) Put(raw, 0xa0 + index * 4, words[index]);
            }
            if (Value is HudDividerNode divider)
            {
                Put(raw, 0xa0, divider.Shape); Put(raw, 0xa4, unchecked((uint)divider.TileWidth)); Put(raw, 0xa8, unchecked((uint)divider.TileHeight)); Put(raw, 0xac, divider.Type); Put(raw, 0xb0, divider.Frame); Put(raw, 0xb4, divider.BackgroundFlags); Put(raw, 0xb8, divider.Axis);
                Put(raw, 0xbc, unchecked((uint)divider.MinX)); Put(raw, 0xc0, unchecked((uint)divider.MaxX)); Put(raw, 0xc4, unchecked((uint)divider.MinY)); Put(raw, 0xc8, unchecked((uint)divider.MaxY));
                // 실제 현재 슬롯만 원래 물리 위치에 쓴다.
                for (int index = 0; index < divider.Slaves.Length; index++) { Put(raw, 0xcc + index * 8, divider.Slaves[index].Serial); Put(raw, 0xd0 + index * 8, divider.Slaves[index].Flags); }
            }
            return raw;
        }
    }

    /// <summary>앞11열의 명시 입력/외부 장치와 실제 공통 Gump 생성기. 자식 공장 자체는 독립 경계다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 고정32주소의 실제 원시/파생 저장소.</summary>
        public readonly Slot[] Slots;
        /// <summary>같은 공통 등록표/count/serial.</summary>
        public readonly GumpCreateState Create = new();
        /// <summary>같은 노드/등록표의 실제 기본 생성기.</summary>
        private readonly GumpCreate _factory;
        /// <summary>현재 원문 주소별 입력 전역.</summary>
        private readonly Dictionary<uint, uint> _globals = [];
        /// <summary>미니맵/지식/메시지 치수의 네 원시 float 입력.</summary>
        private readonly uint[] _scales;
        /// <summary>현재 실제로 소비한 원문 토큰과 할당 종류.</summary>
        private readonly SortedDictionary<uint, char> _used = [];
        /// <summary>현재 메시지의 독립 바이트 입력.</summary>
        private readonly string _title;
        /// <summary>조건 검사 장치의 명시 반환값.</summary>
        private readonly uint _test;
        /// <summary>명시 할당 실패 호출 번호.</summary>
        public readonly uint Fail;
        /// <summary>명시 callback 중단 위치.</summary>
        private readonly string _stop;
        /// <summary>앞11열에서 입력한 경계/주소/값 변경.</summary>
        private readonly (string When, uint Address, uint Value)[] _effects;
        /// <summary>실제 경계별 호출 횟수.</summary>
        public readonly Dictionary<string, uint> Calls = [];
        /// <summary>실제 공개 호출 순서와 인자.</summary>
        public readonly List<string> Trace = [];
        /// <summary>고정 물리 새 슬롯의 다음 번호.</summary>
        private int _next = 6;
        /// <summary>allocate 경계의 현재 호출 횟수.</summary>
        private uint _allocations;
        /// <summary>현재 유효한 같은 기본 주소만 반환한다.</summary>
        public GumpCreateNode? Base(uint token) => token == 0 || Slots[Index(token)].Released ? null : Slots[Index(token)].Value;
        /// <summary>실제 원문 파생 부모의 같은 주소.</summary>
        public UberRootNode Root(uint token) => (UberRootNode)(Base(token) ?? throw new InvalidOperationException("Uber 해제 토큰"));
        /// <summary>현재 원시 주소만 쓰고 같은 typed 객체에 반영한다.</summary>
        private void Write(uint address, uint value)
        {
            if (address >= NodeBase && address < NodeBase + Slots.Length * Stride)
            {
                Slot slot = Slots[(address - NodeBase) / Stride]; slot.Raw = slot.Encode(); Put(slot.Raw, (int)((address - NodeBase) % Stride), value); slot.Decode(slot.Kind);
            }
            else if (address == CountAddress) Create.Count = value;
            else if (address == SerialAddress) Create.Serial = value;
            else _globals[address] = value;
        }
        /// <summary>현재 전역과 메시지 첫 BYTE를 읽는다.</summary>
        private uint Read(uint address) => address == Title ? (_title.Length == 0 ? 0U : (uint)(byte)_title[0]) : _globals[address];
        /// <summary>실제 숫자 인자를 문화권 없이 기록한다.</summary>
        public void Event(string name, params uint[] values) => Record(name, name + string.Concat(values.Select(value => ":" + Num(value))));
        /// <summary>기본 장치 효과 다음에 입력 mutation/중단만 적용한다.</summary>
        private void Record(string name, string text)
        {
            Trace.Add(text); uint count = Calls.GetValueOrDefault(name) + 1; Calls[name] = count; string current = name + Num(count);
            // 같은 callback 안의 명시 주소 쓰기를 순서대로 적용한다.
            foreach (var effect in _effects) if (effect.When == current) Write(effect.Address, effect.Value);
            if (_stop == current) throw new ExplicitStop();
        }
        /// <summary>실제 파생을 고정 새 주소에 설치한다.</summary>
        private uint Fresh(char kind) { uint token = Address(_next++); Slots[Index(token)].Decode(kind); _used[token] = kind; return token; }
        /// <summary>원문의 명시 기본 생성 장치와 같은 실제 GumpCreate를 사용한다.</summary>
        private void BaseDevice(uint token, uint command = 0, float width = 0, float height = 0, uint flags = 1)
        {
            _factory.Construct(token, command, width, height, flags); Base(token)!.Vtable = Table;
        }
        /// <summary>명시 네 공장 장치의 현재 결과만 공급한다. 실제 자식 본체 검증으로 세지 않는다.</summary>
        private uint Child(string name)
        {
            uint token = Fresh('b'); BaseDevice(token, flags: name == "knowledge" ? 0U : 1U);
            if (name is "minimap" or "knowledge") Base(token)!.Scale = Float(_scales[name == "minimap" ? 0 : 1]); Event(name, token); return token;
        }
        /// <summary>원본 부모가 선택하는 모든 명시 외부 경계를 현재 같은 저장소에 공급한다.</summary>
        public UberRootHooks Hooks() => new()
        {
            Node = token => Base(token) as UberRootNode, Divider = token => Base(token) as HudDividerNode, Base = Base, Read = Read, Write = Write,
            Allocate = size => { uint token = ++_allocations == Fail ? 0 : Fresh(size == 0xc8 ? 'r' : size == 0x11c ? 'd' : 'm'); Event("alloc", (uint)size, token); return token; },
            ConstructBase = (token, command, width, height, flags) => { BaseDevice(token, command, width, height, flags); Event("base", token, command, Bits(width), Bits(height), flags); },
            ConstructDivider = (token, axis, width, height, flags, type, frame) => { BaseDevice(token, 55, width, height); var node = (HudDividerNode)Base(token)!; node.Flags |= 0x60200; node.Axis = axis; Array.Clear(node.Slaves); Event("divider", token, axis, unchecked((uint)width), unchecked((uint)height), flags, type, frame); },
            CreateMinimap = () => Child("minimap"), CreateProduction = () => Child("production"), CreateKnowledge = () => Child("knowledge"), CreateChat = () => Child("chat"),
            EmptyObserver = () => Event("map"), RefreshTechnology = () => Event("refresh"), Cleanup = () => Event("cleanup"),
            Attach = (token, x, y, parent, mode) => { GumpCreateNode node = Base(token)!; node.Flags &= ~8U; node.Parent = parent; node.X = x; node.Y = y; Event("attach", token, Bits(x), Bits(y), parent, mode); },
            Move = (token, x, y, parent) => Event("move", token, Bits(x), Bits(y), parent), Push = token => Event("push", token), RegisterSlave = (master, slave, flags) => Event("slave", master, slave, flags), Type = type => { Event("type", type); return 0; },
            Translate = value => { Record("translate", "translate:" + TextHex(value)); return 0x12345678; }, Help = (token, text) => Event("help", token, text), HelpWord = (token, word) => Record("helpword", "helpword:" + Num(token) + ":" + TextHex(word)),
            SavedMessage = () => Title, EqualText = (name, value) => { bool equal = string.Equals(_title, value, StringComparison.OrdinalIgnoreCase); Record("equal", "equal:" + Num(name) + ":" + TextHex(value) + ":" + (equal ? "1" : "0")); return equal; },
            ConstructMessage = (token, name) => { BaseDevice(token, 0, Float(_scales[2]), Float(_scales[3])); Event("message", token, name); },
            FormatMessage = args => { Event("format", args); return Format; }, TestMessage = condition => { Event("test", condition); return _test; }, VisitMessage = name => Event("visit", name),
            MessageEvent = (token, eventCode, first, second) => Event("event", token, eventCode, first, second), Overlay = (token, value) => Event("overlay", token, value ? 1U : 0U), SetDepth = (token, depth) => Event("depth", token, depth), ReplaceRoot = token => Event("replace", token),
        };
        /// <summary>앞11입력만 사용해 같은 기본 등록/자식 링크/전역을 구성한다.</summary>
        public Scene(string[] row)
        {
            Slots = Enumerable.Range(0, 32).Select(_ => new Slot((byte)N(row[7]))).ToArray(); Slots[0].Decode('r'); _used[Address(0)] = 'r'; _used[Address(1)] = 'b'; Create.Collection.Edition = OriginalEdition.Patch1078; Create.Serial = 31;
            // 원문 주소별 독립 전역 입력을 복원한다.
            foreach (string pair in row[2].Split(',')) { string[] parts = pair.Split(':'); _globals[N(parts[0])] = N(parts[1]); }
            _scales = row[3].Split(',').Select(N).ToArray(); _title = row[4] == "-" ? "" : Encoding.Latin1.GetString(Convert.FromHexString(row[4])); _test = N(row[5]); _stop = row[9]; Fail = N(row[10]);
            _effects = row[8].Length == 0 ? [] : row[8].Split(';').Select(effect => { string[] p = effect.Split('/'); return (p[0], N(p[1]), N(p[2])); }).ToArray();
            _factory = new(Create, new() { Node = Base }); BaseDevice(Address(1)); Base(Address(1))!.Flags = 0x801; uint children = N(row[6]); Base(Address(1))!.Child = children != 0 ? Address(2) : 0;
            // 기존 직계 링크와 위치의 명시 입력을 같은 주소에 쓴다.
            for (uint index = 0; index < children; index++)
            {
                uint token = Address((int)index + 2); _used[token] = 'b'; BaseDevice(token); GumpCreateNode node = Base(token)!; node.Parent = Address(1); node.Next = index + 1 < children ? Address((int)index + 3) : 0; node.X = 13.5F + index; node.Y = -7.25F - index;
            }
        }
        /// <summary>현재 원문 전역과 같은 공통 count/serial을 자료 순서로 관찰한다.</summary>
        public string Globals() => string.Join(',', GlobalAddresses.Select(address => Num(address) + ":" + Num(_globals[address]))) + "," + Num(CountAddress) + ":" + Num(Create.Count) + "," + Num(SerialAddress) + ":" + Num(Create.Serial);
        /// <summary>실제1000등록칸의 연속값만 압축하며 의미를 계산하지 않는다.</summary>
        public string Registry()
        {
            var rows = new List<string>();
            // 같은 현재 등록값의 연속 구간을 원문 순서로 기록한다.
            for (int index = 0; index < Create.Entries.Length;)
            {
                int end = index + 1;
                // 빈 칸을 포함해 동일한 연속값만 묶는다.
                while (end < Create.Entries.Length && Create.Entries[end] == Create.Entries[index]) end++;
                rows.Add(Num((uint)(end - index)) + ":" + Num(Create.Entries[index])); index = end;
            }
            return string.Join(';', rows);
        }
        /// <summary>관측된 실제 파생과 미작성 guard 전체를 주소 순서로 기록한다.</summary>
        public string Nodes() => string.Join(';', _used.Select(pair => Num(pair.Key) + ":" + pair.Value + ":" + Convert.ToHexStringLower(Slots[Index(pair.Key)].Encode())));
    }

    /// <summary>명시 하위 공장은 유지하고 실제 공통 트리/지연 삭제/등록표 반납을 연결하는 검사 장치다.</summary>
    private sealed class LifetimeRig
    {
        /// <summary>같은 실제 입력 노드와 공통 생성기.</summary>
        public readonly Scene Scene;
        /// <summary>같은 공통 생성/트리와 연결한 부모 장치.</summary>
        public readonly UberRootHooks Hooks;
        /// <summary>같은 공통 위치/형제 링크 본체.</summary>
        public readonly GumpTree Tree;
        /// <summary>같은 지연 삭제/실제 반납 본체.</summary>
        public readonly GumpLifetime Lifetime;
        /// <summary>같은 실제 장치를 사용한 전체 부모 실행기.</summary>
        public readonly UberRootGump Root;
        /// <summary>기존 세 자식/새 전체 부모를 같은 공통 수명에 연결한다.</summary>
        public LifetimeRig()
        {
            string[] input = (string[])Rows().First(row => row[0].StartsWith("replace-1024-768-empty-", StringComparison.Ordinal) && row[0].EndsWith("27f", StringComparison.Ordinal)).Clone(); input[6] = "3"; Scene = new(input);
            Scene.Create.ScreenWidth = 1024; Scene.Create.ScreenHeight = 768; Hooks = Scene.Hooks();
            // 기존 자식의 실제 부착 완료/양방향 형제 링크를 공급한다.
            for (int index = 2; index < 5; index++) { GumpCreateNode node = Scene.Base(Address(index))!; node.Flags &= ~8U; node.Previous = index == 2 ? 0 : Address(index - 1); }
            Tree = new(Scene.Create, new(), new() { Node = Scene.Base, Redraw = (token, erase) => Scene.Event("redraw", token, erase ? 1U : 0U), Push = token => Tree!.Push(token), Attach = (token, x, y, parent, mode) => Tree!.Attach(token, x, y, parent, mode), Kill = token => Lifetime!.Kill(token) });
            Lifetime = new(Scene.Create, new(), new()
            {
                Node = Scene.Base, Push = Tree.Push, DeletingDestructor = (token, flags) => Lifetime!.DeleteScalar(token, flags),
                Release = token => { Scene.Slots[Index(token)].Released = true; Scene.Event("release", token); },
            });
            Hooks.Attach = Tree.Attach; Hooks.Push = Tree.Push;
            Hooks.Cleanup = () => { Scene.Event("cleanup"); Lifetime.Cleanup(); };
            Hooks.ReplaceRoot = token => { Scene.Event("replace", token); Lifetime.ReplaceRoot(token); };
            Hooks.Kill = token => { Scene.Event("kill", token); Lifetime.Kill(token); };
            Root = new(Scene.Create, Hooks);
        }
        /// <summary>현재 부모 아래 실제 직계 링크를 순환 검사와 함께 관찰한다.</summary>
        public uint[] Children(uint root)
        {
            var found = new List<uint>();
            // 현재 실제 링크를 한 번씩 관찰하며 기대 자식을 계산하지 않는다.
            for (uint child = Scene.Base(root)!.Child; child != 0; child = Scene.Base(child)!.Next)
            {
                Assert.DoesNotContain(child, found); Assert.Equal(root, Scene.Base(child)!.Parent); found.Add(child);
            }
            return found.ToArray();
        }
    }

    /// <summary>80입력×두x87 정밀도의 전체 노드/등록/전역/호출·부분효과160관측을 대조한다.</summary>
    [Fact]
    public void IndependentNative160()
    {
        int count = 0;
        // 앞11입력만 실행한 뒤 뒤7CPU 관측을 비교한다.
        foreach (string[] row in Rows())
        {
            Assert.Equal(18, row.Length); var scene = new Scene(row); var root = new UberRootGump(scene.Create, scene.Hooks()); bool completed = false; uint result = 0;
            try { if (row[1] == "construct") result = root.Construct(Address(0)); else root.Replace(); completed = true; }
            catch (ExplicitStop) { }
            catch (InvalidOperationException) { Assert.NotEqual(0U, N(row[10])); }
            Assert.True(completed == (N(row[11]) != 0), row[0] + " 완료 관문");
            if (completed && row[1] == "construct") Assert.Equal(N(row[12]), result);
            Assert.True(scene.Globals() == row[13], row[0] + " 현재 전역"); Assert.True(scene.Registry() == row[14], row[0] + " 전체 등록표");
            Assert.True(scene.Nodes() == row[15], row[0] + " 전체 노드/guard"); Assert.True(string.Join(';', scene.Trace) == row[16], row[0] + " 실제 호출 순서/인자"); count++;
        }
        Assert.Equal(160, count);
    }
    /// <summary>같은 실제3기존/7신규 자식과 유일한 루트 등록을 보존하고 마지막 전체 반납까지 검사한다.</summary>
    [Fact]
    public void RealTreeAndLifetimeTransfer()
    {
        var rig = new LifetimeRig(); uint before = rig.Scene.Create.Count; uint[] serials = Enumerable.Range(2, 3).Select(index => rig.Scene.Base(Address(index))!.Serial).ToArray();
        rig.Root.Replace(); uint current = rig.Scene.Create.Entries[0]; Assert.NotEqual(Address(1), current); Assert.Equal(current, rig.Scene.Create.Collection.Root); Assert.Null(rig.Scene.Base(Address(1))); Assert.Equal(before + 7, rig.Scene.Create.Count);
        Assert.Equal(10, rig.Children(current).Length); Assert.Equal(serials, Enumerable.Range(2, 3).Select(index => rig.Scene.Base(Address(index))!.Serial));
        Assert.All(Enumerable.Range(2, 3), index => Assert.Equal(current, rig.Scene.Base(Address(index))!.Parent)); Assert.Equal(1, rig.Scene.Create.Entries.Count(token => token == current));
        rig.Lifetime.Kill(current); rig.Lifetime.Cleanup(); Assert.Equal(0U, rig.Scene.Create.Count); Assert.Equal(0U, rig.Scene.Create.Entries[0]); Assert.Equal(0U, rig.Scene.Create.Collection.Root);
    }
    /// <summary>재생성은 기존 부모/자식을 먼저 실제 반납하고7새 자식만 가진 부모를 두 번 연속 게시한다.</summary>
    [Fact]
    public void RecreateDeletesOldChildrenBeforePublishingFreshRoot()
    {
        var rig = new LifetimeRig(); rig.Root.Recreate(); uint first = rig.Scene.Create.Entries[0]; Assert.Equal(8U, rig.Scene.Create.Count); Assert.Equal(7, rig.Children(first).Length);
        Assert.All(Enumerable.Range(1, 4), index => Assert.Null(rig.Scene.Base(Address(index)))); Assert.Equal("kill:" + Num(Address(1)), rig.Scene.Trace[0]);
        int firstAlloc = rig.Scene.Trace.FindIndex(text => text.StartsWith("alloc:", StringComparison.Ordinal)), oldRelease = rig.Scene.Trace.IndexOf("release:" + Num(Address(1))); Assert.True(oldRelease >= 0 && oldRelease < firstAlloc);
        rig.Root.Recreate(); uint second = rig.Scene.Create.Entries[0]; Assert.NotEqual(first, second); Assert.Null(rig.Scene.Base(first)); Assert.Equal(8U, rig.Scene.Create.Count); Assert.Equal(7, rig.Children(second).Length); Assert.Equal(second, rig.Scene.Create.Collection.Root);
        rig.Lifetime.Kill(second); rig.Lifetime.Cleanup(); Assert.Equal(0U, rig.Scene.Create.Count); Assert.All(rig.Scene.Create.Entries, token => Assert.Equal(0U, token));
    }
    /// <summary>기존 부모가0인 재생성도 먼저 정리한 뒤 새 부모를 만들며 이전 객체를 되살리지 않는다.</summary>
    [Fact]
    public void RecreateWithEmptyRegistryStillRunsCleanupBeforeAllocation()
    {
        var rig = new LifetimeRig(); rig.Lifetime.Kill(Address(1)); rig.Lifetime.Cleanup(); rig.Scene.Trace.Clear(); rig.Root.Recreate();
        Assert.Equal("cleanup", rig.Scene.Trace[0]); Assert.StartsWith("alloc:", rig.Scene.Trace[1]); Assert.Equal(8U, rig.Scene.Create.Count); Assert.Equal(7, rig.Children(rig.Scene.Create.Entries[0]).Length);
    }
    /// <summary>재생성 중 공장 실패는 이미 반납한 이전 HUD를 복구하지 않고 새 생성의 부분 상태를 남긴다.</summary>
    [Fact]
    public void RecreateFailureKeepsDeletionAndNewConstructionPrefix()
    {
        var rig = new LifetimeRig(); rig.Hooks.CreateProduction = null; Assert.Throws<InvalidOperationException>(() => rig.Root.Recreate());
        Assert.All(Enumerable.Range(1, 4), index => Assert.Null(rig.Scene.Base(Address(index)))); Assert.Equal(5U, rig.Scene.Create.Count); Assert.NotEqual(Address(1), rig.Scene.Create.Entries[0]);
        Assert.Equal(1U, rig.Scene.Calls["minimap"]); Assert.Equal(0U, rig.Scene.Calls.GetValueOrDefault("production")); Assert.Equal(0U, rig.Scene.Calls.GetValueOrDefault("replace"));
    }
    /// <summary>DockView는 등록0의 이전 부모 대신 생성 중인 this의 현재 자식을 읽고 같은 도킹 본체에 공급한다.</summary>
    [Fact]
    public void DockReadsLiveThisFields()
    {
        var scene = new Scene(Rows().First()); UberRootHooks effects = scene.Hooks(); var root = new UberRootGump(scene.Create, effects); root.Construct(Address(0)); Func<HudDockPart, uint> view = root.DockView(Address(0));
        Assert.Equal(Address(0), view(HudDockPart.Root)); Assert.Equal(Address(1), scene.Create.Entries[0]); var create = new GumpCreateState { ScreenWidth = 1024, ScreenHeight = 768 }; var tree = new GumpTreeState(); var globals = new HudDockGlobals(create, tree, new() { Value = 1 }, new()) { MapLeft = 77, MapWidth = 947, MapHeight = 700 };
        var dirty = new List<(int X, int Y, int Width, int Height)>(); var dock = new HudDock(view, globals, () => 55, new()
        {
            Node = effects.Base, Divider = effects.Divider, BaseAttach = effects.Attach, Attach = effects.Attach, Push = effects.Push, Move = effects.Move,
            ResolveSerial = serial => scene.Create.Entries.FirstOrDefault(token => token != 0 && scene.Base(token)!.Serial == serial && (scene.Base(token)!.Flags & 0x10000) == 0),
            Bubble = (token, eventCode, first, second) => scene.Event("bubble", token, eventCode, first, second), ClosePopups = () => scene.Event("close"), Invalidate = (x, y, width, height, _) => dirty.Add((x, y, width, height)),
        });
        UberRootNode node = scene.Root(Address(0)); node.Vertical = node.MinimapDivider; node.Minimap = node.Production;
        Assert.Equal(node.MinimapDivider, view(HudDockPart.Vertical)); Assert.Equal(node.Production, view(HudDockPart.Minimap)); Assert.Equal(1U, dock.Handle(0x0200003a, scene.Base(node.Vertical)!.Serial, 0));
        Assert.Equal(80, globals.MapLeft); Assert.Equal(944, globals.MapWidth); Assert.Equal(80, tree.MapView.Left); Assert.Equal(1024, tree.MapView.Right); Assert.Equal(2, dirty.Count);
    }
    /// <summary>전체 필수 공장 누락은 실제 도달 위치에서 실패하며 이전 구분선/미니맵 효과를 남긴다.</summary>
    [Fact]
    public void MissingFactoryKeepsPriorEffects()
    {
        var scene = new Scene(Rows().First()); UberRootHooks hooks = scene.Hooks(); hooks.CreateProduction = null; var root = new UberRootGump(scene.Create, hooks);
        Assert.Throws<InvalidOperationException>(() => root.Construct(Address(0))); UberRootNode node = scene.Root(Address(0)); Assert.NotEqual(0U, node.Vertical); Assert.NotEqual(0U, node.MinimapDivider); Assert.NotEqual(0U, node.ChatDivider); Assert.NotEqual(0U, node.Minimap);
        Assert.Equal(1U, scene.Calls["minimap"]); Assert.Equal(0U, scene.Calls.GetValueOrDefault("production"));
    }
    /// <summary>없는 이전 부모는 새 전체 생성/임시 등록 반납 뒤 실패하며0루트 별칭을 함께 유지한다.</summary>
    [Fact]
    public void MissingOldRootFailsAfterConstructionEffects()
    {
        var scene = new Scene(Rows().First()); scene.Create.Entries[0] = 0; scene.Create.Collection.Root = 0; scene.Create.Count = 0; var root = new UberRootGump(scene.Create, scene.Hooks());
        Assert.Throws<InvalidOperationException>(root.Replace); Assert.Equal(0U, scene.Create.Entries[0]); Assert.Equal(0U, scene.Create.Collection.Root); Assert.Equal(8U, scene.Create.Count);
        Assert.Equal(1U, scene.Calls["minimap"]); Assert.Equal(1U, scene.Calls["chat"]); Assert.Equal(0U, scene.Calls.GetValueOrDefault("replace"));
    }
    /// <summary>실제 빈 RET에 장치를 요구하지 않아도 나머지 전체 공장/효과를 그대로 수행한다.</summary>
    [Fact]
    public void EmptyLeafNeedsNoGameHook()
    {
        var scene = new Scene(Rows().First()); UberRootHooks hooks = scene.Hooks(); hooks.EmptyObserver = null; var root = new UberRootGump(scene.Create, hooks); Assert.Equal(Address(0), root.Construct(Address(0)));
        Assert.Equal(0U, scene.Calls.GetValueOrDefault("map")); Assert.Equal(3U, scene.Calls["divider"]); Assert.All(new[] { "minimap", "production", "knowledge", "chat" }, name => Assert.Equal(1U, scene.Calls[name]));
    }
    /// <summary>다른 파생 주소나 다른 판본은 실제 전역/생성 효과를 실행하기 전에 거부한다.</summary>
    [Fact]
    public void RejectsDifferentDerivedStorageAndEditionBeforeEffects()
    {
        var scene = new Scene(Rows().First()); UberRootHooks hooks = scene.Hooks(); hooks.Node = _ => new UberRootNode(); var root = new UberRootGump(scene.Create, hooks);
        Assert.Throws<InvalidOperationException>(() => root.Construct(Address(0))); Assert.Empty(scene.Trace); scene.Create.Collection.Edition = (OriginalEdition)(-1); Assert.Throws<ArgumentException>(() => new UberRootGump(scene.Create, hooks));
    }
    /// <summary>주석/빈 줄을 제외한 기존 원본 관측 행이다.</summary>
    private static IEnumerable<string[]> Rows() => File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "uber-root-x86.tsv")).Where(line => line.Length != 0 && !line.StartsWith('#')).Select(line => line.Split('\t'));
}
