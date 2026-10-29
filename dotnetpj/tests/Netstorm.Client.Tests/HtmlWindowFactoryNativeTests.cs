using System.Globalization;
using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Scripting;

namespace Netstorm.Tests; // 원본 메타/전체 생성/명령 해석 관측을 기대값 재작성 없이 대조한다.

/// <summary>같은 원시 입력과 명시 하위 장치로 실제 제품을 실행한다.전송 메타 입력은 사용자 결정대로 실행하지 않는다.</summary>
public sealed class HtmlWindowFactoryNativeTests
{
    /// <summary>원본16진수 BYTE 입력을 일대일 문자열로 읽는다.</summary>
    private static string Bytes(string text) => Encoding.Latin1.GetString(Convert.FromHexString(text));
    /// <summary>실제 전체 물리 배열의 FNV64를 관측한다.</summary>
    private static ulong Fnv(byte[] bytes) { ulong result = 14695981039346656037;
        // 첫 NUL 뒤까지 모든 실제 BYTE를 읽는다.
        foreach (byte value in bytes) result = unchecked((result ^ value) * 1099511628211); return result;
    }
    /// <summary>현재 명령 해석과 페이지 반납의204개 독립 원문 필드/고리/큐/효과를 대조한다.</summary>
    [Fact]
    public void All204NativeDialogExitResolutionsAndDisposalsMatch()
    {
        int rows = 0;
        // 원문 앞3열만 입력이며 뒤 반환/효과는 비교에만 쓴다.
        foreach (var row in X86Fixture.Read("dialog-exit-command")) {
            Assert.Equal(8, row.Length); var state = new DialogExitCommandState(); var trace = new List<string>(); uint ring = 0, queued = 0; var hooks = new DialogExitResolveHooks { Commands = new() { Lookup = name => { trace.Add("Lookup," + HtmlWindowFixture.Hex(name)); return name switch { "QuitApp" => 0x02000001U, "Tell" => 0x02000095U, "Config" => X86Fixture.UInt(row[2]) != 0 ? 0U : 0x0200006dU, "Pause" => 0x020000cfU, _ => uint.MaxValue }; }, Unknown = name => trace.Add("Diagnostic," + HtmlWindowFixture.Hex(name)) }, StoreText = text => { trace.Add("String," + HtmlWindowFixture.Hex(text)); ring++; return 0x12000000; } }; DialogExitCommand.Resolve(state, X86Fixture.Text(row[0]), X86Fixture.Text(row[1]), hooks); Assert.Equal((X86Fixture.UInt(row[3]), X86Fixture.UInt(row[4]), X86Fixture.UInt(row[5])), (state.Command, state.Parameter, ring)); DialogExitCommand.Dispose(state, (c, a, b) => { queued++; trace.Add($"Queue,{c},{a},{b}"); }); Assert.Equal((0U, 0U, X86Fixture.UInt(row[6])), (state.Command, state.Parameter, queued)); string expected = string.Join(';', row[7].Split(';').Where(x => !x.StartsWith("Digit,", StringComparison.Ordinal) && !x.StartsWith("Atol,", StringComparison.Ordinal))); Assert.Equal(expected, string.Join(';', trace)); rows++;
        }
        Assert.Equal(204, rows);
    }
    /// <summary>메타128행 중 전송 입력4행을 제외한124행의 전체8192BYTE/20행/현재전역·부분효과를 대조한다.</summary>
    [Fact]
    public void All124NativeNonSendingMetadataObservationsMatch()
    {
        int rows = 0, excluded = 0;
        // 앞12열만 실제 입력으로 쓰며 Zsend 입력은 실행 전에 제외한다.
        foreach (var f in X86Fixture.Read("html-window-metadata").Skip(1)) {
            Assert.Equal(22, f.Length); string source = Bytes(f[2]); if (source.Contains("$Zsend", StringComparison.Ordinal)) { excluded++; continue; }
            var node = new HtmlWindowGumpNode { Flags = 9, ControlCount = 2, MenuColumns = X86Fixture.UInt(f[4]), TimeoutParameter = 0xa5a5a5a5, OnExitParameter = 0x5a5a5a5a }; node.Controls[0].Child = 2; node.Controls[1].Child = 3; node.Controls[0].Command = node.Controls[1].Command = 7; node.Controls[0].Parameter = node.Controls[1].Parameter = 8; var children = new Dictionary<uint, ButtonGumpNode> { [2] = new(), [3] = new() }; uint next = 4, closed = 0x020000be; var trace = new List<string>(); var stores = new List<string>();
            // 실제 효과 뒤 독립 입력에 명시된 현재 필드 변경/중단만 적용한다.
            void Effect(string name, string args = "") { trace.Add(name + (args.Length == 0 ? "" : ':' + args)); if (f[10] != "-") { var change = f[10].Split('.'); if (name == change[0]) { if (change[1] == "columns") node.MenuColumns = X86Fixture.UInt(change[2]); else { Assert.Equal("closed", change[1]); closed = X86Fixture.UInt(change[2]); } } } if (f[11] == name) throw new HtmlWindowFixture.Stop(); }
            var coreHooks = new HtmlWindowGumpHooks { Node = t => t == 1 ? node : null, Base = t => t == 1 ? node : children.GetValueOrDefault(t), Button = t => children[t], Kill = t => Effect("kill", t.ToString()), Allocate = bytes => { Assert.Equal(0x170, bytes); uint t = next++; children.Add(t, new()); return t; }, ConstructButton = (t, text) => { children[t].ButtonFlags = 0; Effect("button", t + ":" + HtmlWindowFixture.Hex(text)); }, Attach = (t, x, y, p, m) => Effect("attach", HtmlWindowFixture.Pack([t, HtmlWindowFixture.Bits(x), HtmlWindowFixture.Bits(y), p, m])) }; var core = new HtmlWindowGump(new(), coreHooks);
            var hooks = new HtmlWindowMetadataHooks { Node = t => t == 1 ? node : null, ClearButtons = core.ClearButtons, AddButton = core.AddButton,
                Commands = new() { Lookup = text => { Effect("lookup", HtmlWindowFixture.Hex(text)); return X86Fixture.UInt(f[9]); }, Unknown = text => Effect("diagnostic", HtmlWindowFixture.Hex(text)) }, StoreText = text => { stores.Add(HtmlWindowFixture.Hex(text)); uint t = 0x60000000 + (uint)stores.Count; Effect("store", HtmlWindowFixture.Hex(text)); return t; },
                BeginMenu = flags => Effect("begin", flags.ToString()), MenuText = (label, command, parameter) => Effect("text", HtmlWindowFixture.Hex(label) + ':' + HtmlWindowFixture.Pack([command, parameter, 1, 1, 0])), Checked = (check, label, command, parameter, enabled) => Effect("checked", unchecked((uint)check) + ":" + HtmlWindowFixture.Hex(label) + ':' + HtmlWindowFixture.Pack([command, parameter, 1, enabled ? 1U : 0U])),
                Maps = (string label, ref uint _) => { Effect("mapRefresh", label == "*SOLOLIST*" ? "2" : "0"); Effect("mapRow", "0"); }, Tech = p => Effect("tech", p.ToString()), ThemeTech = (low, high) => Effect("theme", HtmlWindowFixture.Pack([low, unchecked((uint)high)])), NamedList = (key, command, argument) => Effect("named", HtmlWindowFixture.Hex(key) + ':' + command + ':' + HtmlWindowFixture.Hex(argument)), FileList = (key, command, argument) => Effect("files", HtmlWindowFixture.Hex(key) + ':' + command + ':' + HtmlWindowFixture.Hex(argument)), InlineList = (key, command, argument) => { Effect("inline", HtmlWindowFixture.Hex(key) + ':' + HtmlWindowFixture.Hex(command) + ':' + HtmlWindowFixture.Hex(argument)); return Bytes(f[8]); },
                WallSeconds = () => { Effect("clock"); return double.Parse(f[7], CultureInfo.InvariantCulture); }, Idle = (t, enabled) => { node.Flags = enabled ? node.Flags | 0x80 : node.Flags & ~0x80U; Effect("idle", HtmlWindowFixture.Pack([t, enabled ? 1U : 0U])); }, Read = address => address switch { 0x5cab30 => X86Fixture.UInt(f[5]), 0x55a480 => X86Fixture.UInt(f[6]), 0x5429ec => closed, _ => throw new ArgumentOutOfRangeException(nameof(address)) }, Input = (limit, label, argument) => Effect("input", unchecked((uint)limit) + ":" + HtmlWindowFixture.Hex(label) + ':' + HtmlWindowFixture.Hex(argument)), Send = (_, _, _) => throw new InvalidOperationException("사용자 결정상 전송 검사 금지"), Queue = (c, a, b) => Effect("queue", HtmlWindowFixture.Pack([c, a, b]))
            }; byte[] buffer = Enumerable.Repeat((byte)X86Fixture.UInt(f[3]), 8192).ToArray(); Encoding.Latin1.GetBytes(source, buffer); buffer[source.Length] = 0; bool completed = true; uint result = 0; try { result = new HtmlWindowMetadata(hooks).Process(1, buffer); } catch (HtmlWindowFixture.Stop) { completed = false; } ulong time = BitConverter.DoubleToUInt64Bits(node.TimeoutSeconds); uint[] fields = [node.Flags, node.ControlCount, node.MenuColumns, (uint)time, (uint)(time >> 32), node.TimeoutCommand, node.TimeoutParameter, node.OnExitCommand, node.OnExitParameter]; string output = Encoding.Latin1.GetString(buffer, 0, Array.IndexOf(buffer, (byte)0)); string[] actual = [completed ? "1" : "0", HtmlWindowFixture.Pack(fields), HtmlWindowFixture.Pack(node.Controls.SelectMany(e => new[] { e.Child, e.Command, e.Parameter })), Fnv(buffer).ToString(), HtmlWindowFixture.Hex(output), string.Join(';', stores), string.Join(';', trace), (completed ? result : 0).ToString()]; Assert.True(f.Skip(12).Take(8).SequenceEqual(actual), $"HTML 메타 원본행{f[0]}\n기대:{string.Join('|', f.Skip(12).Take(8))}\n실제:{string.Join('|', actual)}"); rows++;
        }
        Assert.Equal((124, 4), (rows, excluded));
    }
    /// <summary>전체 생성106행의 현재 표식/8192BYTE 링크·두번파서/메타·콜백변경/부분중단을 대조한다.</summary>
    [Fact]
    public void All106NativeWholeFactoryObservationsMatch()
    {
        int rows = 0;
        // 앞22열의 명시 입력/하위 장치만 실행에 사용한다.
        foreach (var f in X86Fixture.Read("html-window-factory").Skip(1)) {
            Assert.Equal(33, f.Length); var state = new HtmlWindowGumpState { CreatedSerial = 777, LastCloseTime = -88.5 }; var node = new HtmlWindowGumpNode(); Array.Fill(node.Html, (byte)0xa5); bool constructed = false; uint parses = 0, metas = 0; var trace = new List<string>(); string firstObserved = "", secondObserved = ""; uint[] geometry = HtmlWindowFixture.Numbers(f[13]), screen = HtmlWindowFixture.Numbers(f[14]); var globals = new Dictionary<uint, uint> { [0x5c78f0] = screen[0], [0x5c78f4] = screen[1], [0x563e38] = X86Fixture.UInt(f[15]) };
            // 실제 인자 기록 뒤에만 독립 입력의 현재 필드 변경/중단을 적용한다.
            void Effect(string phase, string args = "") { trace.Add(phase + (args.Length == 0 ? "" : ':' + args)); if (f[20] != "-") { var change = f[20].Split('.'); if (phase == change[0]) { uint value = X86Fixture.UInt(change[2]); switch (change[1]) { case "created": state.CreatedSerial = value; break; case "serial": node.Serial = value; break; case "width": node.BaseWidth = HtmlWindowFixture.Float(value); break; case "height": node.BaseHeight = HtmlWindowFixture.Float(value); break; default: globals[change[1] == "screenW" ? 0x5c78f0U : 0x563e38U] = value; break; } } } if (phase == f[21]) throw new HtmlWindowFixture.Stop(); }
            var hooks = new HtmlWindowFactoryHooks { Node = t => { Assert.Equal(1U, t); return node; }, Create = (width, height) => { Effect("allocate", "8680"); if (X86Fixture.UInt(f[9]) != 0) return 0; Effect("construct", width + ":" + height); node.Serial = X86Fixture.UInt(f[12]); node.BaseWidth = HtmlWindowFixture.Float(geometry[0]); node.BaseHeight = HtmlWindowFixture.Float(geometry[1]); node.Html[0] = 0; constructed = true; return 1; }, Substitute = (source, capacity) => { Effect("substitute", HtmlWindowFixture.Hex(source) + ':' + capacity + ":0:0:0"); return Bytes(f[3]); },
                Parse = (parser, root, source, capacity) => { parser.AfterHeading = 0; Effect("bind", capacity + ":" + (root != 0 ? "2:8192" : "0:0")); parses++; Effect("parse" + parses, HtmlWindowFixture.Hex(source) + ':' + (root != 0 ? "2" : "0") + ':' + parser.Shown + ':' + parser.HiddenDepth + ':' + parser.AfterHeading); string output = Bytes(f[parses == 1 ? 4 : 5]); if (root != 0) Encoding.Latin1.GetBytes(Bytes(f[parses == 1 ? 6 : 7]), node.Html); if (parses == 1) firstObserved = output; else secondObserved = output; return output; },
                WallSeconds = () => { Effect("clock"); return double.Parse(f[16], CultureInfo.InvariantCulture); }, Metadata = (root, buffer) => { Assert.Equal(1U, root); metas++; string text = Encoding.Latin1.GetString(buffer.Span[..buffer.Span.IndexOf((byte)0)]); Effect("meta", HtmlWindowFixture.Hex(text)); if (metas == 1) { if (f[18] != "-") { string after = Bytes(f[18]); Encoding.Latin1.GetBytes(after, buffer.Span); buffer.Span[after.Length] = 0; } firstObserved = Encoding.Latin1.GetString(buffer.Span[..buffer.Span.IndexOf((byte)0)]); } return X86Fixture.UInt(f[17]); }, Fallback = () => { Effect("fallback", "4f6b427574746f6e:0:0:0"); return Bytes(f[19]); }, Body = (root, text) => { Assert.Equal(1U, root); if (parses == 2) secondObserved = text; Effect("body", HtmlWindowFixture.Hex(text)); }, Read = address => globals[address], Attach = (root, x, y, parent, mode) => { Assert.Equal(1U, root); Effect("attach", $"{HtmlWindowFixture.Bits(x)}:{HtmlWindowFixture.Bits(y)}:{parent}:{mode}"); }, Modal = (root, enabled) => { Assert.Equal(1U, root); Effect("modal", enabled ? "1" : "0"); }, Kill = root => { Assert.Equal(1U, root); Effect("kill"); }
            }; bool completed = true; uint result = 0; try { result = new HtmlWindowFactory(state, hooks).Show(Bytes(f[2]), X86Fixture.UInt(f[8]), X86Fixture.UInt(f[10]), X86Fixture.UInt(f[11])); } catch (HtmlWindowFixture.Stop) { completed = false; } string[] actual = [completed ? "1" : "0", state.CreatedSerial.ToString(), BitConverter.DoubleToUInt64Bits(state.LastCloseTime).ToString(), (constructed ? node.Serial : 0).ToString(), (constructed ? Fnv(node.Html) : 0).ToString(), HtmlWindowFixture.Hex(firstObserved), HtmlWindowFixture.Hex(secondObserved), string.Join(';', trace), result.ToString()]; Assert.True(f.Skip(22).Take(9).SequenceEqual(actual), $"HTML 생성 원본행{f[0]}\n기대:{string.Join('|', f.Skip(22).Take(9))}\n실제:{string.Join('|', actual)}"); rows++;
        }
        Assert.Equal(106, rows);
    }
}
