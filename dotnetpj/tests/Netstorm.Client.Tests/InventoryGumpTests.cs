using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 실제 생성/목록/점멸의 전체 원본 상태를 대조한다.

/// <summary>독립170격자/110점멸과 실제 공통 처리기의 조합 검사다.</summary>
public sealed class InventoryGumpTests
{
    /// <summary>원본 관측의 단일 노드 주소다.</summary>
    private const uint Token = 0x12401000;
    /// <summary>첫 원본 자식 주소다.</summary>
    private const uint Born = 0x12403000;
    /// <summary>명시 외부 입력과 실제 기본 생성기를 공급한다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 내용물 물리다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>같은 실제 순회 정책이다.</summary>
        public readonly ContainedFinderState Finder = new();
        /// <summary>같은 단일 등록표다.</summary>
        public readonly GumpCreateState Create = new() { Serial = 23 };
        /// <summary>실제 같은 격자 주소다.</summary>
        public readonly InventoryGridNode Grid = InventoryRawSupport.Grid(Canary(196));
        /// <summary>실제 생성한 항목의 주소 순서다.</summary>
        public readonly SortedDictionary<uint, InventoryItemNode> Items = [];
        /// <summary>실제 같은 기본 생성기다.</summary>
        public readonly GumpCreate Factory;
        /// <summary>원문과 같은 외부 장치다.</summary>
        public readonly InventoryGumpHooks Hooks;
        /// <summary>현재 실제 처리기다.</summary>
        public readonly InventoryGump Handler;
        /// <summary>현재 외부 커서/부모/반환/범위 응답이다.</summary>
        public uint Cursor, Parent = 50, Forward, BaseReturn, Inside;
        /// <summary>현재 명시 벽시계다.</summary>
        public double Now;
        /// <summary>전체 원문 효과 순서다.</summary>
        public string Trace = "";
        /// <summary>동일 파생 저장소에 실제 기본 생성과 목록 검색을 연결한다.</summary>
        public Scene(bool item = false)
        {
            Factory = new(Create, new() { Node = t => item ? Items.GetValueOrDefault(t) : t == Token ? Grid : Items.GetValueOrDefault(t) });
            Hooks = new()
            {
                Grid = t => t == Token ? Grid : null, Item = t => Items.GetValueOrDefault(t), Assertion = text => Event("Assert," + Hex(text)),
                ClearChildren = t => { Assert.Equal(Token, t); Event("Clear"); }, BaseAttach = (t, x, y, parent, mode) => { Assert.Equal(Token, t); Event("BaseAttach", B(x), B(y), parent, mode); },
                AllocateItem = size => { Assert.Equal(192, size); uint t = Born + (uint)Items.Count * 256; Items.Add(t, Item(Canary(192))); Event("New", (uint)size, t); return t; },
                Attach = (t, x, y, parent, mode) => { if (t == Token) Event("V", B(x), B(y), parent, mode); else Event("ItemAttach", t, B(x), B(y), parent, mode); },
                Push = t => { Assert.Equal(Token, t); Event("Push"); }, Bind = (t, sid) => { Assert.Equal(Token, t); Event("Bind", sid); Grid.Object = sid; },
                Idle = (t, enabled) => { Assert.True(enabled); Event("Idle", t, 1); (item ? Items[t] : t == Token ? Grid : (GumpCreateNode)Items[t]).Flags |= 0x80; },
                Now = () => { Event("Now"); return Now; }, ContentParent = sid => { Event("Parent", sid); return Parent; },
                Forward = (t, code, first, second) => { Assert.Equal(Token, t); Event("Forward", code, first, second); return Forward; },
                BaseHandle = (t, code, first, second) => { Assert.Equal(Token, t); Event("BaseHandle", code, first, second); return BaseReturn; },
                Local = (t, code, point) => { Assert.Equal(Token, t); Event("Local", code, unchecked((uint)point.X), unchecked((uint)point.Y)); return (7.5f, -3.25f); },
                Inside = (t, point) => { Assert.Equal(Token, t); Event("Inside", B(point.X), B(point.Y)); return Inside != 0; }, CancelCursor = () => Event("Cancel"),
            };
            Handler = new(Create, Factory, Pool, Finder, new() { CursorPoint = () => new(7, -3), CursorFlags = () => Cursor }, Hooks);
        }
        /// <summary>호출 인자의 전체 DWORD만 기록한다.</summary>
        public void Event(string name, params uint[] args) { if (Trace.Length != 0) Trace += ';'; Trace += name + string.Concat(args.Select(value => "," + value.ToString(CultureInfo.InvariantCulture))); }
        /// <summary>입력 flags/slot을 실제 목록 WORD와 내용물 바이트에 쓴다.</summary>
        public void Contents(string text)
        {
            var parts = text == "-" ? [] : text.Split(';'); BinaryPrimitives.WriteUInt16LittleEndian(Raw(Pool, 50)[6..], (ushort)(parts.Length == 0 ? 0 : 100));
            // 원문 입력 순서의 실제 next/종류/flags/slot만 쓴다.
            for (int i = 0; i < parts.Length; i++) { var fields = parts[i].Split(':'); var raw = Raw(Pool, 100 + i); raw[10] = 6; BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], (ushort)(i + 1 < parts.Length ? 101 + i : 0)); BinaryPrimitives.WriteUInt16LittleEndian(raw[20..], (ushort)U(fields[0])); raw[23] = (byte)U(fields[1]); }
        }
    }
    /// <summary>독립170관측의 전체0xc4/0xc0·반환·모든 생성 항목/순서를 대조한다.</summary>
    [Fact]
    public void Native170GridItemLayoutEventsAndWholeMemory()
    {
        int count = 0;
        // 각 관측은 독립 명시 입력이며 기대 출력은 비교에만 사용한다.
        foreach (var row in Rows("inventory-grid"))
        {
            Assert.Equal(6, row.Length); var s = new Scene(row[0] == "Item"); string result = "-", encoded = "";
            if (row[0] == "Grid") { var a = Inputs(row[1]); result = s.Handler.ConstructGrid(Token, unchecked((int)a[0]), unchecked((int)a[1]), a[2]).ToString(CultureInfo.InvariantCulture); }
            else if (row[0] == "Item") { var a = row[1].Split(','); s.Now = double.Parse(a[4], CultureInfo.InvariantCulture); uint content = U(a[2]); BinaryPrimitives.WriteUInt16LittleEndian(Raw(s.Pool, (int)content)[20..], (ushort)U(a[3])); s.Items.Add(Token, Item(Canary(192))); result = s.Handler.ConstructItem(Token, F(U(a[0])), F(U(a[1])), content).ToString(CultureInfo.InvariantCulture); encoded = Encode(s.Items[Token]); s.Items.Clear(); }
            else
            {
                s.Handler.ConstructGrid(Token, 2, 6, 8); s.Grid.Object = 50; s.Grid.X = 1.25f; s.Grid.Y = -2.5f; s.Trace = "";
                if (row[0] == "Attach") { var parts = row[1].Split('|'); var a = Inputs(parts[0]); s.Grid.MinimumColumns = unchecked((int)a[0]); s.Grid.MinimumRows = unchecked((int)a[1]); s.Grid.ContentMask = a[2]; s.Grid.Fixed = a[3]; s.Grid.ItemScale = F(a[4]); s.Grid.RequireImageHit = a[5]; s.Grid.BaseWidth = F(a[6]); s.Grid.BaseHeight = F(a[7]); s.Grid.Width = F(a[8]); s.Grid.Height = F(a[9]); s.Contents(parts[1]); s.Handler.AttachGrid(Token, 5, 6, 0, a[10]); }
                else if (row[0] == "Handle") { var a = Inputs(row[1]); s.Parent = a[1]; BinaryPrimitives.WriteUInt16LittleEndian(Raw(s.Pool, 100)[20..], (ushort)a[2]); s.Inside = a[3]; s.Cursor = a[4]; s.Grid.AcceptedCursor = a[5]; s.Forward = a[6]; s.BaseReturn = a[7]; result = s.Handler.HandleGrid(Token, a[0], 100, 9).ToString(CultureInfo.InvariantCulture); }
                else { Assert.Equal("Gone", row[0]); s.Handler.ParentGone(Token, U(row[1])); }
            }
            if (encoded.Length == 0) encoded = Encode(s.Grid); string children = s.Items.Count == 0 ? "-" : string.Join(';', s.Items.Select(pair => pair.Key.ToString(CultureInfo.InvariantCulture) + "=" + Encode(pair.Value)));
            string trace = string.Join(';', row[5].Split(';').Where(value => value != "Next" && value != "Begin,50")); Assert.Equal(row[2], result); Assert.Equal(row[3], encoded); Assert.Equal(row[4], children); Assert.Equal(trace, s.Trace.Length == 0 ? "-" : s.Trace); count++;
        }
        Assert.Equal(170, count);
    }
    /// <summary>110관측의 현재 시각 재조회·표시 앞뒤·Idle500칸·내용물 비트·실제 dirty 요청을 대조한다.</summary>
    [Fact]
    public void Native110BlinkWholeStateIdleTableContentAndActualDirtyRegions()
    {
        int count = 0;
        // 같은 실제 기본 위치/가시성/수명 처리를 사용하고 원문 출력은 그대로 대조한다.
        foreach (var row in Rows("inventory-blink"))
        {
            Assert.Equal(11, row.Length); byte[] rootRaw = Convert.FromHexString(row[1]); Array.Resize(ref rootRaw, 160); var root = GumpRawSupport.Decode(rootRaw); var item = Item(Convert.FromHexString(row[2])); const uint rootToken = 0x15000000, itemToken = rootToken + 128;
            // 원본이 기록한 두 안정된 주소만 기본/파생에서 공유한다.
            GumpCreateNode? Node(uint token) => token == rootToken ? root : token == itemToken ? item : null;
            var create = new GumpCreateState(); create.Collection.Root = rootToken; var factory = new GumpCreate(create, new() { Node = Node }); var state = new GumpLifetimeState(); if ((item.Flags & 0x80) != 0) state.Idle[0] = itemToken;
            var life = new GumpLifetime(create, state, new() { Node = Node }); var collection = new GumpCollection(create.Collection, new() { Node = Node }); var pool = new RawSidPool(24000, true); Raw(pool, 1).Clear(); Raw(pool, 1)[21] = 255; int calls = 0; string trace = "", requests = ""; var times = row[3].Split(',');
            // 조작 앞의 원문 위치에서 현재 플래그를 기록한다.
            void Event(string name, string value) { if (trace.Length != 0) trace += ';'; trace += name + ',' + item.Flags.ToString(CultureInfo.InvariantCulture) + ',' + value; }
            var redraw = new GumpRedraw(collection, new() { Node = Node, Map = () => default, Invalidate = (rect, flags) => { if (requests.Length != 0) requests += ';'; requests += string.Join(',', unchecked((uint)rect.Left), unchecked((uint)rect.Top), unchecked((uint)rect.Right), unchecked((uint)rect.Bottom), flags); } });
            var hooks = new InventoryGumpHooks
            {
                Item = t => t == itemToken ? item : null, Idle = (t, on) => { Event("idle", on ? "1" : "0"); life.SetIdle(t, on); },
                Now = () => { if (calls == 0 && row[10] != "-") item.BlinkUntil = BitConverter.UInt64BitsToDouble(ulong.Parse(row[10], NumberStyles.HexNumber, CultureInfo.InvariantCulture)); ulong bits = ulong.Parse(times[calls++], NumberStyles.HexNumber, CultureInfo.InvariantCulture); double value = BitConverter.UInt64BitsToDouble(bits); Event("clock", bits.ToString("x16", CultureInfo.InvariantCulture)); return value; },
                Redraw = (t, erase) => { Event("redraw", erase ? "1" : "0"); redraw.Whole(t, erase ? 1U : 0U); }, ClearBlink = sid => { Assert.Equal(1u, sid); Raw(pool, 1)[21] &= 0xfd; },
            };
            var handler = new InventoryGump(create, factory, pool, new(), new() { CursorPoint = () => default, CursorFlags = () => 0 }, hooks); handler.IdleItem(itemToken);
            Assert.Equal(row[4], Encode(item)); Assert.Equal(row[5], trace.Length == 0 ? "-" : trace); Assert.Equal(Inputs(row[6]), state.Idle); Assert.Equal(U(row[7]), Raw(pool, 1)[21]); Assert.Equal(row[8], requests.Length == 0 ? "-" : requests); Assert.Equal(U(row[9]), (uint)calls); Assert.Equal(Convert.ToHexStringLower(rootRaw), GumpRawSupport.Encode(root)); count++;
        }
        Assert.Equal(110, count);
    }
}
