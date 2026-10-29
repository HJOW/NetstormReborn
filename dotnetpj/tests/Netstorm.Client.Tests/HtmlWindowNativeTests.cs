using System.Text;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 동결된 원본 HTML 창/Idle/전체 배치를 같은 입력과 현재 저장소로 대조한다.

/// <summary>독립 관측의 입력 해석과 실제 현재 필드 직렬화만 담당한다.</summary>
internal static class HtmlWindowFixture
{
    /// <summary>명시 장치에서의 실행 중단. 정상 복귀와 구별한다.</summary>
    internal sealed class Stop : Exception;
    /// <summary>원시 DWORD 목록을 입력으로 해석한다.</summary>
    internal static uint[] Numbers(string value) => value.Length == 0 ? [] : value.Split(':').Select(X86Fixture.UInt).ToArray();
    /// <summary>현재 DWORD를 독립 관측과 같은 십진수 목록으로 쓴다.</summary>
    internal static string Pack(IEnumerable<uint> values) => string.Join(':', values);
    /// <summary>float32의 실제 raw 비트.</summary>
    internal static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);
    /// <summary>raw DWORD를 float32로 읽는다.</summary>
    internal static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>원문 BYTE 문자열의16진수.</summary>
    internal static string Hex(string value) => Convert.ToHexString(Encoding.Latin1.GetBytes(value)).ToLowerInvariant();
    /// <summary>창의 현재24개 필드를 원본 선언 순서로 읽는다.</summary>
    internal static uint[] Fields(HtmlWindowGumpNode n) { ulong time = BitConverter.DoubleToUInt64Bits(n.TimeoutSeconds); return [n.Vtable, n.Flags, n.Command, n.SecondaryDepth, Bits(n.Scale), Bits(n.BaseWidth), Bits(n.BaseHeight), Bits(n.Width), Bits(n.Height), Bits(n.X), Bits(n.Y), n.ControlCount, n.Body, n.UpdateDepth, n.RequestedWidth, n.RequestedHeight, n.MenuColumns, n.AuxiliarySerial, (uint)time, (uint)(time >> 32), n.TimeoutCommand, n.TimeoutParameter, n.OnExitCommand, n.OnExitParameter]; }
    /// <summary>원본 입력 또는 명시 콜백 변경만 같은 현재 객체에 설치한다.</summary>
    internal static void Set(HtmlWindowGumpNode n, uint[] f) { n.Vtable = f[0]; n.Flags = f[1]; n.Command = f[2]; n.SecondaryDepth = f[3]; n.Scale = Float(f[4]); n.BaseWidth = Float(f[5]); n.BaseHeight = Float(f[6]); n.Width = Float(f[7]); n.Height = Float(f[8]); n.X = Float(f[9]); n.Y = Float(f[10]); n.ControlCount = f[11]; n.Body = f[12]; n.UpdateDepth = f[13]; n.RequestedWidth = f[14]; n.RequestedHeight = f[15]; n.MenuColumns = f[16]; n.AuxiliarySerial = f[17]; n.TimeoutSeconds = BitConverter.UInt64BitsToDouble(f[18] | (ulong)f[19] << 32); n.TimeoutCommand = f[20]; n.TimeoutParameter = f[21]; n.OnExitCommand = f[22]; n.OnExitParameter = f[23]; }
    /// <summary>배치 관측의 기본9필드.</summary>
    internal static uint[] Geometry(GumpCreateNode n) => [n.Flags, n.Depth, Bits(n.Scale), Bits(n.BaseWidth), Bits(n.BaseHeight), Bits(n.Width), Bits(n.Height), Bits(n.X), Bits(n.Y)];
    /// <summary>배치의 명시 입력9필드를 같은 노드에 설치한다.</summary>
    internal static void GeometrySet(GumpCreateNode n, ReadOnlySpan<uint> f) { n.Flags = f[0]; n.Depth = f[1]; n.Scale = Float(f[2]); n.BaseWidth = Float(f[3]); n.BaseHeight = Float(f[4]); n.Width = Float(f[5]); n.Height = Float(f[6]); n.X = Float(f[7]); n.Y = Float(f[8]); }
    /// <summary>배치 관측의 부모6필드.</summary>
    internal static uint[] Root(HtmlWindowGumpNode n) => [n.ControlCount, n.Body, n.RequestedWidth, n.RequestedHeight, n.MenuColumns, n.AuxiliarySerial];
    /// <summary>배치의 명시 부모 입력만 설치한다.</summary>
    internal static void RootSet(HtmlWindowGumpNode n, uint[] f) { n.ControlCount = f[0]; n.Body = f[1]; n.RequestedWidth = f[2]; n.RequestedHeight = f[3]; n.MenuColumns = f[4]; n.AuxiliarySerial = f[5]; }
}
/// <summary>cpppj의 독립 최초 관측250/88/160행을 제품으로 재생한다. 기대값은 실행 입력에 쓰지 않는다.</summary>
public sealed class HtmlWindowNativeTests
{
    /// <summary>생성/소멸/버튼/본문/입력/Idle의 전체24필드·20행·8192BYTE·자식·효과를 대조한다.</summary>
    [Theory]
    [InlineData("html-window-gump", 250)]
    [InlineData("html-window-idle-v2", 88)]
    public void AllNativeLifecycleBodyInputAndIdleObservationsMatch(string fixture, int expectedRows)
    {
        string[] names = ["vtable", "flags", "kind", "depth", "scale", "bw", "bh", "width", "height", "x", "y", "count", "body", "update", "rw", "rh", "columns", "aux", "timeLo", "timeHi", "timeout", "timeoutArg", "exit", "exitArg"]; int rows = 0;
        // 앞14열만 입력이며 뒤 원본 관측은 최종 비교에만 쓴다.
        foreach (var f in X86Fixture.Read(fixture).Skip(1)) {
            Assert.Equal(25, f.Length); var node = new HtmlWindowGumpNode(); HtmlWindowFixture.Set(node, HtmlWindowFixture.Numbers(f[3])); Array.Fill(node.Html, (byte)X86Fixture.UInt(f[5])); uint[] slots = HtmlWindowFixture.Numbers(f[4]);
            // 독립 입력의20행 전체를 같은 물리 행에 설치한다.
            for (int i = 0; i < 20; i++) { node.Controls[i].Child = slots[i * 3]; node.Controls[i].Command = slots[i * 3 + 1]; node.Controls[i].Parameter = slots[i * 3 + 2]; }
            var children = Enumerable.Range(0, 24).Select(i => new ButtonGumpNode { Serial = (uint)(1000 + i), SecondaryDepth = 0x12345678, ButtonFlags = 0x2468 }).ToArray(); var state = new HtmlWindowGumpState { Instances = X86Fixture.UInt(f[6]), Kind = unchecked((sbyte)X86Fixture.UInt(f[7])), LastCloseTime = -7.5 }; var trace = new List<string>();
            // 각 실제 하위 주소와 호출 뒤 명시 변경/중단 입력만 공급한다.
            ButtonGumpNode Child(uint token) { Assert.InRange(token, 2U, 25U); return children[token - 2]; }
            void Effect(string name, string arguments = "") { trace.Add(name + (arguments.Length == 0 ? "" : ':' + arguments)); if (f[12] != "-") { string[] change = f[12].Split('.'); if (change[0] == name) { uint[] fields = HtmlWindowFixture.Fields(node); int index = Array.IndexOf(names, change[1]); Assert.True(index >= 0); fields[index] = X86Fixture.UInt(change[2]); HtmlWindowFixture.Set(node, fields); } } if (f[13] == name) throw new HtmlWindowFixture.Stop(); }
            var hooks = new HtmlWindowGumpHooks {
                Node = t => t == 1 ? node : null, Base = t => t == 1 ? node : t is >= 2 and <= 25 ? Child(t) : null, Button = Child,
                ConstructDecoration = _ => { node.Vtable = 0x50ac70; node.Flags = 9; node.Command = 57; node.SecondaryDepth = 0; node.Scale = 1; node.BaseWidth = node.BaseHeight = node.Width = node.Height = node.X = node.Y = 0; Effect("base"); }, DestructBase = _ => Effect("destruct"), Push = t => Effect("push", t.ToString()), Kill = t => Effect("kill", t.ToString()),
                Allocate = size => { Effect("alloc", size.ToString()); Assert.Contains(size, new[] { 0x170, 0xd8 }); return size == 0x170 ? 24U : 25U; }, ConstructButton = (t, text) => { Child(t).ButtonFlags = 0; Effect("button", t + ":" + HtmlWindowFixture.Hex(text)); }, ConstructBody = t => { Child(t).SecondaryDepth = 0; Effect("body", t.ToString()); }, ParseBody = (t, text) => Effect("parse", t + ":" + HtmlWindowFixture.Hex(text)), Scrollable = (t, value) => Effect("scroll", $"{t}:{unchecked((uint)value)}"), Attach = (t, x, y, p, m) => Effect("attach", HtmlWindowFixture.Pack([t, HtmlWindowFixture.Bits(x), HtmlWindowFixture.Bits(y), p, m])),
                Reference = serial => { Effect("reference", serial.ToString()); return X86Fixture.UInt(f[10]); }, WallSeconds = () => { Effect("wall"); return BitConverter.UInt64BitsToDouble(ulong.Parse(f[11])); }, Queue = (c, a, b) => Effect("queue", HtmlWindowFixture.Pack([c, a, b])), Link = t => Effect("link", t.ToString()), BaseHandle = (_, c, a, b) => { Effect("baseHandle", HtmlWindowFixture.Pack([c, a, b])); return 0xfedcba98; }
            }; var body = new HtmlWindowGump(state, hooks); uint[] args = HtmlWindowFixture.Numbers(f[8]); string text = Encoding.Latin1.GetString(Convert.FromHexString(f[9])); bool completed = true; uint result = 0;
            try { switch (f[1]) { case "construct": result = body.Construct(1, args[0], args[1]); break; case "destroy": body.Destruct(1); break; case "clear": body.ClearButtons(1); break; case "add": body.AddButton(1, text, args[1], args[2], args[3]); break; case "body": body.SetBody(1, text); break; case "idle": body.Idle(1); break; default: Assert.Equal("handle", f[1]); result = body.Handle(1, args[0], args[1], args[2]); break; } } catch (HtmlWindowFixture.Stop) { completed = false; }
            ulong fnv = 14695981039346656037;
            // 첫 NUL 뒤까지 실제8192BYTE 전부 관측한다.
            foreach (byte value in node.Html) fnv = unchecked((fnv ^ value) * 1099511628211);
            string[] actual = [completed ? "1" : "0", HtmlWindowFixture.Pack(HtmlWindowFixture.Fields(node)), HtmlWindowFixture.Pack(node.Controls.SelectMany(e => new[] { e.Child, e.Command, e.Parameter })), fnv.ToString(), HtmlWindowFixture.Pack(children.SelectMany(c => new[] { c.Serial, c.SecondaryDepth, c.ButtonFlags })), state.Instances.ToString(), BitConverter.DoubleToUInt64Bits(state.LastCloseTime).ToString(), string.Join(';', trace), (completed ? result : 0).ToString()];
            Assert.True(f.Skip(14).Take(9).SequenceEqual(actual), $"HTML 원본행 {f[0]}\n기대:{string.Join('|', f.Skip(14).Take(9))}\n실제:{string.Join('|', actual)}"); rows++;
        }
        Assert.Equal(expectedRows, rows);
    }
    /// <summary>26개 전체 노드/현재20행·메뉴 스택·전역·모든 측정/부착 효과를 독립160행과 대조한다.</summary>
    [Fact]
    public void All160NativeFitGeometryMenusAndCurrentEffectsMatch()
    {
        uint[] addresses = [0x5ca9b4, 0x531860, 0x531864, 0x5c78f0, 0x5c78f4, 0x5ca9cc, 0x5ca9d4, 0x5ca9d8, 0x5ca9d0]; string[] rootNames = ["count", "body", "rw", "rh", "columns", "aux"]; int rows = 0;
        // 앞14열의 입력과 명시 장치만 실행에 사용한다.
        foreach (var f in X86Fixture.Read("html-window-fit").Skip(1)) {
            Assert.Equal(24, f.Length); var node = new HtmlWindowGumpNode(); var objects = Enumerable.Range(0, 26).Select(i => i == 0 ? node : new GumpCreateNode()).ToArray(); uint[] fields = HtmlWindowFixture.Numbers(f[2]);
            //26개 실제 객체에 독립 입력9필드씩 설치한다.
            for (int i = 0; i < 26; i++) HtmlWindowFixture.GeometrySet(objects[i], fields.AsSpan(i * 9, 9)); HtmlWindowFixture.RootSet(node, HtmlWindowFixture.Numbers(f[3])); uint[] slots = HtmlWindowFixture.Numbers(f[4]);
            // 실제20개 행의 child 입력만 설치한다.
            for (int i = 0; i < 20; i++) node.Controls[i].Child = slots[i]; uint[] globals = HtmlWindowFixture.Numbers(f[5]), natural = HtmlWindowFixture.Numbers(f[6]), widths = HtmlWindowFixture.Numbers(f[7]), heights = HtmlWindowFixture.Numbers(f[8]); var menus = HtmlWindowFixture.Numbers(f[9]).ToList(); int heightCalls = 0; var trace = new List<string>();
            // 받은 원시 장치 값만 적용하고 기대 배치를 계산하지 않는다.
            GumpCreateNode Object(uint t) { Assert.InRange(t, 1U, 26U); return objects[t - 1]; }
            void Effect(string name, params uint[] args) { trace.Add(name + (args.Length == 0 ? "" : ':' + HtmlWindowFixture.Pack(args))); if (f[12] != "-") { string[] change = f[12].Split('.'); if (name == change[0]) { uint value = X86Fixture.UInt(change[2]); if (change[1] == "view") globals[0] = value; else { uint[] root = HtmlWindowFixture.Root(node); int index = Array.IndexOf(rootNames, change[1]); Assert.True(index >= 0); root[index] = value; HtmlWindowFixture.RootSet(node, root); } } } if (f[13] == name) throw new HtmlWindowFixture.Stop(); }
            void Attach(string name, uint t, float x, float y, uint parent, uint mode) { var target = Object(t); target.X = x; target.Y = y; target.Flags &= ~8U; Effect(name, t, HtmlWindowFixture.Bits(x), HtmlWindowFixture.Bits(y), parent, mode); }
            var hooks = new HtmlWindowFitHooks {
                Node = t => t == 1 ? node : null, Base = Object, FitButton = t => { Assert.InRange(t, 4U, 23U); Object(t).BaseWidth = HtmlWindowFixture.Float(widths[t - 4]); Effect("fitButton", t); }, Push = t => { Object(t).Flags |= 8; Effect("push", t); }, FitContent = (t, x, y) => { Assert.Equal((0, 0), (x, y)); Object(t).BaseWidth = HtmlWindowFixture.Float(natural[0]); Object(t).BaseHeight = HtmlWindowFixture.Float(natural[1]); Effect("fitContent", t); }, FitHeight = t => { Assert.True(heightCalls < heights.Length); Object(t).BaseHeight = HtmlWindowFixture.Float(heights[heightCalls++]); Effect("fitHeight", t); }, Slider = (t, v) => Effect("slider", t, v), Scrollable = (t, v) => Effect("scroll", t, v), RequestScroll = (t, v) => Effect("request", t, unchecked((uint)v)), Attach = (t, x, y, p, m) => Attach("attach", t, x, y, p, m), AttachBase = (t, x, y, p, m) => Attach("attachBase", t, x, y, p, m),
                CurrentMenu = () => { Assert.NotEmpty(menus); uint t = menus[^1]; Effect("menuCurrent", t); return t; }, AttachMenu = (x, y, parent) => { Assert.NotEmpty(menus); uint t = menus[^1]; var target = Object(t); target.X = x; target.Y = y; target.Flags &= ~8U; Effect("menuAttach", t, unchecked((uint)x), unchecked((uint)y), parent); menus.RemoveAt(menus.Count - 1); }, Reference = serial => { Effect("reference", serial); return X86Fixture.UInt(f[11]); }, Read = address => { int index = Array.IndexOf(addresses, address); Assert.True(index >= 0); return globals[index]; }, AddDepth = (t, d) => { Object(t).Depth = unchecked(Object(t).Depth + d); Effect("depth", t, d); }
            }; uint[] args = HtmlWindowFixture.Numbers(f[10]); bool completed = true; try { new HtmlWindowFit(hooks).Attach(1, HtmlWindowFixture.Float(args[0]), HtmlWindowFixture.Float(args[1]), args[2], args[3]); } catch (HtmlWindowFixture.Stop) { completed = false; }
            string[] actual = [completed ? "1" : "0", HtmlWindowFixture.Pack(objects.SelectMany(HtmlWindowFixture.Geometry)), HtmlWindowFixture.Pack(HtmlWindowFixture.Root(node)), HtmlWindowFixture.Pack(node.Controls.Select(e => e.Child)), HtmlWindowFixture.Pack(globals), HtmlWindowFixture.Pack(menus), heightCalls.ToString(), string.Join(';', trace)]; Assert.True(f.Skip(14).Take(8).SequenceEqual(actual), $"HTML 배치 원본행 {f[0]}\n기대:{string.Join('|', f.Skip(14).Take(8))}\n실제:{string.Join('|', actual)}"); rows++;
        }
        Assert.Equal(160, rows);
    }
}
