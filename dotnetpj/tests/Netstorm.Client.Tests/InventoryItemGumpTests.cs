using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;
using static Netstorm.Client.Tests.InventoryRawSupport;

namespace Netstorm.Client.Tests; // 실제 이미지/문자 생성기와 전체 항목 원본 관측을 조합한다.

/// <summary>132개 독립 원본의 전체 항목/모든 생성 자식/현재 선택/효과를 대조한다.</summary>
public sealed class InventoryItemGumpTests
{
    /// <summary>원본의 안정된 항목 토큰이다.</summary>
    private const uint Token = 0x12401000;
    /// <summary>원본의 실제 첫 할당 주소다.</summary>
    private const uint Born = 0x12404000;
    /// <summary>명시 외부 입력과 실제 기존 생성기를 연결한다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 내용물의 같은 물리다.</summary>
        public readonly RawSidPool Pool = new(24000, true);
        /// <summary>단일 실제 등록표다.</summary>
        public readonly GumpCreateState Create = new() { Serial = 23 };
        /// <summary>안정된 실제 항목이다.</summary>
        public readonly InventoryItemNode Item = InventoryRawSupport.Item(Canary(192));
        /// <summary>실제 생성한 모든 자식 기본/파생이다.</summary>
        public readonly Dictionary<uint, GumpCreateNode> Nodes = [];
        /// <summary>실제 할당 순서와 크기다.</summary>
        public readonly List<(uint Token, int Bytes)> Allocated = [];
        /// <summary>같은 최근 TYPE의 실제 쓰기 주소다.</summary>
        public readonly UIntBox Last = new();
        /// <summary>현재 입력 gumpFrame이다.</summary>
        public int GumpFrame = -1;
        /// <summary>현재 실제 정수 비용 입력이다.</summary>
        public int Cost = 800;
        /// <summary>현재 수량/입력/기하/관문/전달 응답이다.</summary>
        public uint Stock, Disabled, Bounds, Cells, Opened, SecondOpen, KnowledgeCalls, Children, Denied, Forward, Robust = 1;
        /// <summary>현재 실제 SP 입력이다.</summary>
        public float Money = 800;
        /// <summary>현재 실제 에디터 관문이다.</summary>
        public bool Editor;
        /// <summary>원문 실제 효과 순서다.</summary>
        public string Trace = "";
        /// <summary>현재 실제 연결 장치다.</summary>
        public readonly InventoryItemHooks Hooks;
        /// <summary>실제 검사 본체다.</summary>
        public readonly InventoryItemGump Handler;
        /// <summary>실제 기본/이미지/문자 생성과 명시 경계만 준비한다.</summary>
        public Scene()
        {
            Item.Vtable = 0x5068f4; Item.Flags = 9; Item.Child = 0; Item.Serial = 41; Item.Content = 128; BinaryPrimitives.WriteUInt16LittleEndian(Raw(Pool, 128)[18..], 128);
            var create = Create; var factory = new GumpCreate(create, new() { Node = BaseNode }); var life = new GumpLifetime(create, new(), new() { Node = BaseNode }); var collection = new GumpCollection(create.Collection, new() { Node = BaseNode }); var style = new StyleText(new(), new());
            var image = new ImageGump(create, factory, new() { LocalOwner = 1, OwnerMaps = new uint[188], ShadowOffset = new(5, 5) }, new() { Node = t => Nodes.GetValueOrDefault(t) as ImageGumpNode, Type = _ => new(0x16000000, 2, 3), Header = (kind, frame) => { Event("Header", kind, 0, frame); return new(40, 60, 12, -7); } });
            var text = new TextGump(create, new(), factory, life, collection, new(), style, new() { Node = t => Nodes.GetValueOrDefault(t) as TextGumpNode, BaseNode = BaseNode });
            var codes = Enumerable.Repeat(new RiftFrameCode((byte)'A', (byte)'P', 0, 0), 16).ToArray(); codes[8] = codes[8] with { Side = (byte)'B' }; codes[9] = codes[9] with { Flags = 0x20 }; codes[10] = new((byte)'B', (byte)'P', 0, 0x20); codes[11] = codes[11] with { Flags = 0x40 }; codes[12] = new((byte)'B', (byte)'P', 0, 0x40); var frames = new RiftTypeFrames(codes);
            Hooks = new()
            {
                Node = t => t == Token ? Item : null, Base = BaseNode, Type = _ => new(GumpFrame, frames),
                BaseAttach = (t, x, y, parent, mode) => Event("Attach", t, B(x), B(y), parent, mode), ClearChildren = _ => Event("Clear"), Cleanup = () => Event("Cleanup"),
                Robust = (sid, allow) => { Event("Robust", sid, allow ? 1u : 0u); return Robust != 0; }, Assertion = message => Event("Assert," + Hex(message)),
                Measure = (kind, argument, explicitFrame) => { Event("Measure", kind, argument, 0, explicitFrame ? 1u : 0u); PriestPlacementRect[] rects = [new(0, 0, 64, 96), new(-10, -7, 10, 30), new(1000, 1000, 0, 0)]; return new(rects[Bounds], 16, 11); },
                Walk = (query, visit) => { Event("Walk", query.Type, unchecked((uint)query.Argument), unchecked((uint)query.Direction), B(query.X), B(query.Y), query.ExplicitFrame ? 1u : 0u); InventoryItemCell[] cells = [new(7, Cells == 2 ? -0.5f : 0, Cells == 2 ? 2.25f : 0), new(8, 1.5f, -1)];
                    // 같은 실제 decoder 경계의 명시 입력만 전달한다.
                    for (int i = 0; i < Cells; i++) { visit(cells[i]); Event("Advance"); } },
                Allocate = bytes => { uint t = Born + (uint)Allocated.Count * 256; Allocated.Add((t, bytes)); Event("New", (uint)bytes, t); GumpCreateNode n = bytes == 180 ? Base(new ImageGumpNode(), Canary(160)) : Base(new TextGumpNode { Head = 0xa5a5a5a5 }, Canary(160)); Nodes.Add(t, n); return t; },
                Image = (t, kind, frame, mode) => image.Construct(t, kind, unchecked((uint)frame), mode), Text = t => text.Construct(t, 0, 0, 0, 0), StyledText = (t, value) => Event("Text," + t.ToString(CultureInfo.InvariantCulture) + ',' + Hex(value)), FitText = t => Event("Fit", t, 0, 0),
                CostUnits = _ => Cost, Money = owner => { Event("Money", owner); return Money; }, Position = _ => { Event("Position"); return new(17, -9); },
                Background = (r, flags, kind, frame) => Event("Background", unchecked((uint)r.Left), unchecked((uint)r.Top), unchecked((uint)r.Right), unchecked((uint)r.Bottom), flags, kind, frame),
                Local = (_, code, first, second) => { Event("Local", code, first, second); return (7.5f, -3.25f); }, KnowledgeOpen = () => { uint result = KnowledgeCalls++ == 0 ? Opened : SecondOpen; Event("Knowledge", result); return result; },
                ImageHit = (t, x, y) => { Event("ImageHit", t, B(x), B(y)); return t == 0x12403200 || Children != 3 ? 1u : 0u; }, Denied = (first, flags) => { Event("Denied", first, flags); return Denied; },
                Forward = (_, code, first, second) => { Event("Forward", code, first, second); return Forward; }, Tell = value => Event("Tell," + Hex(value)), About = (_, first, second) => { Event("About", first, second); return 0x80000009; },
            };
            Hooks.Attach = Hooks.BaseAttach;
            Handler = new(Pool, new() { LocalOwner = () => 1, Editor = () => Editor, StockEnabled = () => Stock, InputDisabled = () => Disabled, LastType = Last, BridgeType = () => 82, PixelsX = () => 16, PixelsY = () => 11, BackgroundType = () => 96, BackgroundFrame = () => 2 }, Hooks);
        }
        /// <summary>기본/파생의 동일 주소다.</summary>
        private GumpCreateNode? BaseNode(uint token) => token == Token ? Item : Nodes.GetValueOrDefault(token);
        /// <summary>전체 실제 인자를 원문 순서로 기록한다.</summary>
        private void Event(string name, params uint[] args) { if (Trace.Length != 0) Trace += ';'; Trace += name + string.Concat(args.Select(value => "," + value.ToString(CultureInfo.InvariantCulture))); }
        /// <summary>실제 할당 순서의 모든 이미지/문자 필드를 원문 offset으로 기록한다.</summary>
        public string Created()
        {
            var values = new List<string>();
            // 실제 자식만 직렬화하고 기대 결과에서 필드를 복사하지 않는다.
            foreach (var (token, bytes) in Allocated)
            {
                byte[] raw = Convert.FromHexString(GumpRawSupport.Encode(Nodes[token])); Array.Resize(ref raw, bytes);
                if (Nodes[token] is ImageGumpNode image) { Put(raw, 160, image.Type); Put(raw, 164, image.Frame); Put(raw, 168, image.Mode); Put(raw, 172, B(image.HotspotX)); Put(raw, 176, B(image.HotspotY)); } else Put(raw, 160, ((TextGumpNode)Nodes[token]).Head);
                values.Add(token.ToString(CultureInfo.InvariantCulture) + '=' + Convert.ToHexStringLower(raw));
            }
            return values.Count == 0 ? "-" : string.Join(';', values);
        }
    }
    /// <summary>원본132관측의 전체 항목/자식·현재 lastType·반환과 효과 순서를 모두 비교한다.</summary>
    [Fact]
    public void Native132ImagesTextAffordableInputAndWholeMemory()
    {
        int count = 0;
        // 입력 열만 장치에 공급하며 관측 열은 비교에만 사용한다.
        foreach (var row in Rows("inventory-item"))
        {
            Assert.Equal(7, row.Length); var s = new Scene(); var a = Inputs(row[1]); string result = "-";
            if (row[0] == "Attach") { var raw = Raw(s.Pool, 128); BinaryPrimitives.WriteUInt16LittleEndian(raw[18..], (ushort)a[0]); BinaryPrimitives.WriteUInt16LittleEndian(raw[20..], (ushort)a[1]); BinaryPrimitives.WriteUInt16LittleEndian(raw[12..], (ushort)a[2]); raw[30] = (byte)a[3]; s.GumpFrame = unchecked((int)a[4]); s.Item.Width = F(a[5]); s.Item.Height = F(a[6]); s.Item.IconScale = F(a[7]); s.Stock = a[8]; s.Cells = a[9]; s.Bounds = a[10]; s.Money = F(a[11]); s.Editor = a[12] != 0; s.Item.Flags = 9 | (a[13] != 0 ? 0x10000u : 0u); s.Robust = a[14]; s.Handler.Attach(Token, 5, 6, 0, 7); }
            else if (row[0] == "Handle")
            {
                s.Opened = a[1]; s.Disabled = a[2]; s.Item.RequireImageHit = a[3]; s.Children = a[4]; s.Denied = a[5]; s.Forward = a[6]; s.Last.Value = a[7]; s.SecondOpen = a[8];
                if (s.Children != 0) { uint[] tokens = [0x12403000, 0x12403100, 0x12403200]; int n = s.Children == 3 ? 3 : 1; s.Item.Child = tokens[0];
                    // 현재 실제 자식 체인의 명시 입력만 작성한다.
                    for (int i = 0; i < n; i++) { var node = Base(new GumpCreateNode(), Canary(160)); node.Next = i + 1 < n ? tokens[i + 1] : 0; node.Command = (s.Children != 2 && i != 0) || s.Children == 1 ? 3u : 45u; s.Nodes.Add(tokens[i], node); } }
                result = s.Handler.Handle(Token, a[0], 17, 0xfffffff7).ToString(CultureInfo.InvariantCulture);
            }
            else if (row[0] == "Draw") { s.Item.Decoration = (byte)a[0]; s.Item.BaseWidth = F(a[1]); s.Item.BaseHeight = F(a[2]); s.Handler.Draw(Token); }
            else { Assert.Equal("Afford", row[0]); s.Cost = unchecked((int)a[0]); s.Money = F(a[1]); s.Editor = a[2] != 0; result = s.Handler.Affordable(128).ToString(CultureInfo.InvariantCulture); }
            Assert.Equal(row[2], result); Assert.Equal(row[3], Encode(s.Item)); Assert.Equal(U(row[4]), s.Last.Value); Assert.Equal(row[5], s.Created()); Assert.Equal(row[6], s.Trace.Length == 0 ? "-" : s.Trace); count++;
        }
        Assert.Equal(132, count);
    }
}
