using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 독립 원본 목록514행의 전체tail/실제MetaImage 자식/가상효과 대조다.

/// <summary>명시 입력 장치만 제공하고 기대값은 상태/자식 산술에 사용하지 않는다.</summary>
public sealed class MetaImageListNativeTests
{
    /// <summary>현재float32의 원시DWORD.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>원문little endian DWORD를 관측 배열에 쓴다.</summary>
    private static void Put(byte[] raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(offset), value);
    /// <summary>원본과 같은 기본장치·현재목록/실제MetaImage를 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>기본/파생이 같은 실제 목록.</summary>
        public MetaImageListNode Node { get; } = new() { Count = 0x5a5a5a5a, CellHeight = BitConverter.UInt32BitsToSingle(0x5a5a5a5a), CellWidth = BitConverter.UInt32BitsToSingle(0x5a5a5a5a) };
        /// <summary>원문 한도+1의 실제자식 입력 저장소.</summary>
        private readonly MetaImageGumpNode[] _children = Enumerable.Range(0, 22).Select(_ => new MetaImageGumpNode()).ToArray();
        /// <summary>실제 자식이 빌리는 같은 동적 Image.</summary>
        public RawImage[] Images { get; } = Enumerable.Range(0, 22).Select(_ => new RawImage()).ToArray();
        /// <summary>같은 실제 등록/일련번호.</summary>
        public GumpCreateState Create { get; } = new();
        /// <summary>같은 실제 MetaImage 생성/부착 처리기.</summary>
        public MetaImageGump Meta { get; }
        /// <summary>현재 실제 목록 본체.</summary>
        public MetaImageList List { get; }
        /// <summary>원문 독립 입력의 실제외부효과 순서만 관측한다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>앞15열만 보관하는 독립 입력.</summary>
        private readonly string[] _input;
        /// <summary>같은 실제 기본/자식 저장소.</summary>
        private GumpCreateNode? Base(uint token) => token == 1 ? Node : token is >= 100 and < 122 ? _children[token - 100] : null;
        /// <summary>원문 장치가 공급하는 동일한 실제 비트맵만 준비한다.</summary>
        public void Prepare(uint index) { int width = new[] { 224, 6, 112 }[index % 3], height = new[] { 112, 4, 224 }[index % 3]; if (Images[index].Width() == 0) Images[index].Convert(new(width, height, Enumerable.Repeat((byte)7, width * height).ToArray()), false); }
        /// <summary>기본 부착은 현재 로컬좌표/부모와 명시 크기변경만 수행한다.</summary>
        private void AttachBase(uint token, float x, float y, uint parent, uint mode) { Trace.Add($"attach:{(token == 1 ? "list" : (token - 100).ToString())}:{Bits(x)}:{Bits(y)}"); var current = Base(token) ?? throw new ArgumentException("원문 기본 주소 없음"); current.X = x; current.Y = y; current.Parent = parent; if (token == 1 && X86Fixture.UInt(_input[13]) == 1) { Node.BaseWidth = 131.75f; Node.BaseHeight = 99.5f; } }
        /// <summary>앞15열 입력만 장치에 연결한다.기대관측을 생성에 사용하지 않는다.</summary>
        public Scene(string[] row)
        {
            _input = row.Take(15).ToArray(); Array.Fill(Node.Ids, 0x5a5a5a5aU); var factory = new GumpCreate(Create, new() { Node = Base }); var lifetime = new GumpLifetime(Create, new(), new() { Node = Base }); Meta = new(Create, factory, new() { Node = token => token is >= 100 and < 122 ? _children[token - 100] : null, Attach = AttachBase });
            var hooks = new MetaImageListHooks
            {
                Node = token => token == 1 ? Node : null, Image = token => token is >= 100 and < 122 ? _children[token - 100] : null, CreateImage = image => { Trace.Add("allocate"); return Meta.Construct(100 + Node.Count, image); }, BaseAttach = AttachBase, Attach = (token, x, y, parent, mode) => { if (token == 1) List!.Attach(token, x, y, parent, mode); else Meta.Attach(token, x, y, parent, mode); },
                Push = token => { Trace.Add("push:" + (token == 1 ? "list" : (token - 100).ToString())); Base(token)!.Flags |= 8; if (token == 100 && X86Fixture.UInt(_input[13]) == 2) { Node.Count = Math.Max(1U, Node.Count - 1); Node.CellWidth = 17.25f; Node.CellHeight = 13.5f; } }, Kill = token => Trace.Add("kill:" + (token - 100)), DestructBase = token => { Trace.Add("destruct"); lifetime.DestructBase(token); }, Bounds = _ => { Trace.Add("bounds"); return new(10, 20, 210, 140); }, Redraw = (_, erase) => Trace.Add("redraw:" + (erase ? 1 : 0)), Notify = (_, code, first, second) => Trace.Add($"notify:{code}:{first}:{second}"), Green = () => 43, Fill = (rect, color) => Trace.Add($"fill:{unchecked((uint)rect.Left)}:{unchecked((uint)rect.Top)}:{unchecked((uint)rect.Right)}:{unchecked((uint)rect.Bottom)}:{color}")
            }; List = new(Create, factory, hooks);
        }
        /// <summary>같은 실제 tail을 원문 위치로만 직렬화한다.자식 주소만 원본 관찰의 순번으로 정규화한다.</summary>
        public string Tail()
        {
            byte[] raw = new byte[0xbc]; Put(raw, 0, Node.Count); Put(raw, 0xac, unchecked((uint)Node.Selection)); Put(raw, 0xb0, unchecked((uint)Node.Columns)); Put(raw, 0xb4, Bits(Node.CellHeight)); Put(raw, 0xb8, Bits(Node.CellWidth));
            // 사용하지 않는 끝 슬롯도 원문과 같은 방법으로 관측한다.
            for (int i = 0; i < 21; i++) { Put(raw, 4 + i * 4, Node.Images[i] != 0 ? Node.Images[i] - 99 : 0); Put(raw, 0x58 + i * 4, Node.Ids[i]); } return Convert.ToHexStringLower(raw);
        }
        /// <summary>22개 실제자식의7개기하필드를 현재float32 저장비트로만 관측한다.</summary>
        public string Geometry()
        {
            byte[] raw = new byte[22 * 28];
            // 실제자식 순서대로 현재기하를 읽는다.
            for (int i = 0; i < _children.Length; i++) { var c = _children[i]; float[] values = [c.Scale, c.BaseWidth, c.BaseHeight, c.Width, c.Height, c.X, c.Y];
                // 기대기하를 계산하지 않고 원문DWORD로만 직렬화한다.
                for (int field = 0; field < values.Length; field++) Put(raw, i * 28 + field * 4, Bits(values[field])); } return Convert.ToHexStringLower(raw);
        }
    }
    /// <summary>두제어모드의514행을 같은실제목록/MetaImage에재생한다.</summary>
    [Fact]
    public void All514OriginalListAndActualChildObservationsMatch()
    {
        int count = 0;
        // 앞15열의 입력만 실행하고 뒤11열은 기대값으로만 읽는다.
        foreach (string[] row in X86Fixture.Read("meta-image-list"))
        {
            if (row[0] == "op") continue; Assert.Equal(26, row.Length); var s = new Scene(row); uint returned = s.List.Construct(1, X86Fixture.Int(row[2]));
            if (row[0] != "Construct")
            {
                var n = s.Node; n.Count = X86Fixture.UInt(row[1]); n.Selection = X86Fixture.Int(row[8]); n.BaseWidth = X86Fixture.FloatBits(row[3]); n.BaseHeight = X86Fixture.FloatBits(row[4]); n.Scale = X86Fixture.FloatBits(row[5]); n.CellWidth = X86Fixture.FloatBits(row[6]); n.CellHeight = X86Fixture.FloatBits(row[7]); n.X = 2.25f; n.Y = 3.5f;
                // 이미존재하는현재실제자식/ID를독립입력으로설치한다.
                for (uint index = 0; index < n.Count; index++) { s.Prepare(index); s.Meta.Construct(100 + index, s.Images[index]); n.Images[index] = 100 + index; n.Ids[index] = 4096 + index * 7; } if (row[0] == "Destruct") n.Flags |= 0x40000; s.Trace.Clear(); returned = 0;
                switch (row[0]) { case "Attach": s.List.Attach(1, 2.25f, 3.5f, 0, 0); break; case "Hit": returned = unchecked((uint)s.List.Hit(1, X86Fixture.Int(row[9]), X86Fixture.Int(row[10]))); break; case "Handle": returned = s.List.Handle(1, X86Fixture.UInt(row[11]), unchecked((uint)X86Fixture.Int(row[9])), unchecked((uint)X86Fixture.Int(row[10]))); break; case "Draw": s.List.Draw(1); break; case "Id": returned = s.List.Id(1, X86Fixture.Int(row[12])); break; case "Image": var image = s.List.Image(1, X86Fixture.Int(row[12])); returned = image is null ? 0 : (uint)Array.IndexOf(s.Images, image) + 1; break; case "Remove": returned = s.List.Remove(1, X86Fixture.Int(row[12])) ? 1U : 0U; break; case "Add": s.Prepare(n.Count); returned = s.List.Add(1, s.Images[n.Count], 0x81234567) ? 1U : 0U; break; case "Destruct": s.List.Destruct(1); break; default: throw new ArgumentException("원본 목록 진입 이름"); }
            }
            Assert.Equal(X86Fixture.UInt(row[15]), returned); Assert.True(row[22] == s.Tail(), $"{count}/{row[0]} 전체tail"); Assert.True(row[23] == s.Geometry(), $"{count}/{row[0]} 전체자식기하"); Assert.Equal(row[24], s.Trace.Count == 0 ? "-" : string.Join(';', s.Trace)); Assert.Equal(X86Fixture.UInt(row[16]), Bits(s.Node.BaseWidth)); Assert.Equal(X86Fixture.UInt(row[17]), Bits(s.Node.BaseHeight)); Assert.Equal(X86Fixture.UInt(row[18]), s.Node.Flags); Assert.Equal(X86Fixture.UInt(row[19]), s.Node.Vtable); Assert.Equal(X86Fixture.UInt(row[20]), s.Create.Count); Assert.Equal(X86Fixture.UInt(row[21]), s.Create.Serial); Assert.True(X86Fixture.UInt(row[25]) > 0); count++;
        }
        Assert.Equal(514, count);
    }
}
