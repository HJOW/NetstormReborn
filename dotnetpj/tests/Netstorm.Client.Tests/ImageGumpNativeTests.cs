using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 이미지 UI221입력의 전체0xb4노드/효과/반환 대조다.

/// <summary>기존 독립원본 기대값을 읽기 전용으로만 사용한다.</summary>
public sealed class ImageGumpNativeTests
{
    /// <summary>원본 격리 노드 주소.</summary>
    private const uint Token = 0x12401000;
    /// <summary>원본 명시SHP 주소.</summary>
    private const uint Shape = 0x16000000;
    /// <summary>원본 현재TYPE 소유자색표 주소.</summary>
    private const uint OwnerTable = 0x12408000;
    /// <summary>부호 입력도하위DWORD로 보존한다.</summary>
    private static uint U(string value) => unchecked((uint)long.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원시float32 입력의같은비트.</summary>
    private static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>현재float32의원시DWORD.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>생성자가 쓰지않는모든원문바이트도 같은입력으로설치한다.</summary>
    private static ImageGumpNode Initial()
    {
        var n = new ImageGumpNode(); var input = GumpRawSupport.Decode(Enumerable.Repeat((byte)0xa5, 160).ToArray());
        // 모든 기본필드의같은초기값만복사하고참조는교체하지않는다.
        foreach (var property in typeof(GumpCreateNode).GetProperties()) if (property.CanWrite) property.SetValue(n, property.GetValue(input)); input.Positioning.CopyTo(n.Positioning, 0); input.TextContext.CopyTo(n.TextContext, 0); n.Type = n.Frame = n.Mode = 0xa5a5a5a5; n.HotspotX = n.HotspotY = Float(0xa5a5a5a5); return n;
    }
    /// <summary>현재실제기본/파생필드를원본주소순서의바이트로만관측한다.</summary>
    private static string Encode(ImageGumpNode n) { byte[] raw = Convert.FromHexString(GumpRawSupport.Encode(n)); Array.Resize(ref raw, 0xb4); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(160), n.Type); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(164), n.Frame); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(168), n.Mode); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(172), Bits(n.HotspotX)); BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(176), Bits(n.HotspotY)); return Convert.ToHexStringLower(raw); }
    /// <summary>현재 입력상태/장치효과만소유한다.뒤쪽기대값은참조하지않는다.</summary>
    private sealed class Scene
    {
        /// <summary>같은실제등록표.</summary>
        public GumpCreateState Create { get; } = new() { Serial = 23 };
        /// <summary>같은실제기본/파생노드.</summary>
        public ImageGumpNode Node { get; } = Initial();
        /// <summary>실제기본생성기.</summary>
        public GumpCreate Factory { get; }
        /// <summary>현재원문owner/TYPE표/선행위치.</summary>
        public ImageGumpState State { get; } = new() { OwnerMaps = new uint[188], ShadowOffset = new(5, 5) };
        /// <summary>명시프레임헤더입력.</summary>
        public ImageGumpHeader Header { get; set; }
        /// <summary>명시현재TYPE입력.</summary>
        public ImageGumpType Type { get; set; } = new(Shape, 0, 0);
        /// <summary>명시실제위치입력.</summary>
        public ScreenPoint Point { get; set; } = new(17, -9);
        /// <summary>명시적중응답입력.</summary>
        public uint Answer { get; set; }
        /// <summary>실제장치호출관측만기록한다.</summary>
        private readonly List<string> _trace = [];
        /// <summary>현재같은파생/효과경계.</summary>
        public ImageGumpHooks Hooks { get; }
        /// <summary>원문구분자와부호없는DWORD표기로관측한다.</summary>
        private void Event(string name, params uint[] values) => _trace.Add(values.Length == 0 ? name : name + "," + string.Join(',', values));
        /// <summary>현재실제효과순서의관측.</summary>
        public string Trace => string.Join(';', _trace);
        /// <summary>기대값을계산하지않고동일한원문장치입력만연결한다.</summary>
        public Scene()
        {
            Factory = new(Create, new() { Node = token => token == Token ? Node : null }); Hooks = new()
            {
                Node = token => token == Token ? Node : null, Type = number => { Assert.Equal(128U, number); return Type; }, Header = (number, frame) => { Event("Header", number, 0, frame); return Header; }, Position = token => { Assert.Equal(Token, token); Event("Position"); return Point; }, Color = token => Event("Color", token), Attach = (token, x, y, parent, mode) => { Assert.Equal(Token, token); Event("BaseAttach", Bits(x), Bits(y), parent, mode); },
                Draw = r => { string name = r.Scaled ? (r.Mapped ? "0x4dab40" : "0x4daaf0") : (r.Mapped ? "0x4daab0" : "0x4daa70"); if (r.Scaled) Event(name, r.Shape, r.Frame, unchecked((uint)r.Position.X), unchecked((uint)r.Position.Y), 0, unchecked((uint)r.Scale), unchecked((uint)r.Scale)); else Event(name, r.Shape, r.Frame, unchecked((uint)r.Position.X), unchecked((uint)r.Position.Y)); },
                Hit = r => { Event("Hit", unchecked((uint)r.Point.X), unchecked((uint)r.Point.Y), r.Shape, r.Frame, unchecked((uint)r.Origin.X), unchecked((uint)r.Origin.Y), 0, unchecked((uint)r.Scale), unchecked((uint)r.Scale)); return Answer; }
            };
        }
    }
    /// <summary>독립221개입력의전체생성/재설정/그리기/적중/부착상태를대조한다.</summary>
    [Fact]
    public void All221OriginalImageObservationsMatch()
    {
        string[][] rows = X86Fixture.Read("inventory-image"); Assert.Equal(221, rows.Length);
        // 각행을독립입력으로실행하고뒤3열은관측비교에만사용한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(5, row.Length); var s = new Scene(); uint[] a = row[1].Split(',').Select(U).ToArray(); string result = "-"; var image = new ImageGump(s.Create, s.Factory, s.State, s.Hooks);
            if (row[0] is "Construct" or "Reset") { s.Header = new(unchecked((short)a[0]), unchecked((short)a[1]), unchecked((short)a[2]), unchecked((short)a[3])); s.Type = s.Type with { HotspotX = unchecked((int)a[4]), HotspotY = unchecked((int)a[5]) }; s.Node.Scale = Float(a[6]); if (row[0] == "Construct") result = image.Construct(Token, 128, 7, a[7]).ToString(CultureInfo.InvariantCulture); else image.SetFrame(Token, 128, 7, a[7]); }
            else { s.Node.Vtable = 0x501bb0; s.Node.Type = 128; s.Node.Frame = 7; s.Node.HotspotX = 12.25f; s.Node.HotspotY = -3.75f; s.Node.X = 2.5f; s.Node.Y = -4.5f;
                if (row[0] == "Draw") { s.Node.Mode = a[0]; s.Node.Scale = Float(a[1]); s.State.LocalOwner = a[2]; s.State.OwnerMaps[128] = a[3] != 0 ? OwnerTable : 0; s.Node.HotspotX = Float(a[4]); s.Node.HotspotY = Float(a[5]); s.Point = new(unchecked((int)a[6]), unchecked((int)a[7])); s.State.ShadowOffset = new(unchecked((int)a[8]), unchecked((int)a[9])); image.Draw(Token); }
                else if (row[0] == "Hit") { s.Node.Scale = Float(a[0]); s.Answer = a[3]; result = image.Hit(Token, Float(a[1]), Float(a[2])).ToString(CultureInfo.InvariantCulture); } else { Assert.Equal("Attach", row[0]); image.Attach(Token, 4, 5, 0x1234, 7); }
            }
            Assert.Equal(row[2], result); Assert.True(row[3] == Encode(s.Node), $"{row[0]} 전체이미지노드"); Assert.Equal(row[4], s.Trace);
        }
    }
}
