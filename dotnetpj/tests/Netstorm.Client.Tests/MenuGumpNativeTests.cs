using System.Buffers.Binary;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 두 PE의 메뉴 전체 현재 필드·스택·콜백/CRC·반환값을 원문과 대조한다.

/// <summary>기대값은 읽기 전용 원본 관찰에서만 읽는다.게임/PE/에뮬레이터를 실행하지 않는다.</summary>
public sealed class MenuGumpNativeTests
{
    /// <summary>실제 문맥/측정 응답의 DWORD 비트만 읽는다.</summary>
    private static uint[] Words(string hex) { byte[] raw = X86Fixture.Bytes(hex); return Enumerable.Range(0, raw.Length / 4).Select(i => BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(i * 4))).ToArray(); }
    /// <summary>관찰의 실제 반환/문맥 DWORD를 직렬화한다.</summary>
    private static string HexWords(params uint[] words)
    {
        byte[] raw = new byte[words.Length * 4];
        // 기대 산술은 없이 현재 DWORD만 같은 순서로 쓴다.
        for (int i = 0; i < words.Length; i++) BinaryPrimitives.WriteUInt32LittleEndian(raw.AsSpan(i * 4), words[i]); return Convert.ToHexStringLower(raw);
    }
    /// <summary>문자열 입력/콜백의 첫NUL 앞 실제 BYTE만 기록한다.</summary>
    private static string TextHex(ReadOnlySpan<byte> bytes) { int end = bytes.IndexOf((byte)0); Assert.True(end >= 0); return Convert.ToHexStringLower(bytes[..end]); }
    /// <summary>두 판본 각154행의 전체 노드/controlled 표/idle/가상 호출 당시 상태/반환을 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void NativeMenuBody(string edition)
    {
        string[][] rows = X86Fixture.Read("menugump", edition); Assert.Equal(154, rows.Length);
        // 한 행의 연속 공개 연산은 같은 상태를 공유한다.
        foreach (string[] r in rows)
        {
            var raw = new MenuRawSupport(r[3], true); var create = new GumpCreateState(); create.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; var state = new MenuGumpState(); var life = new GumpLifetimeState(); string[] parts = r[4].Split('#'); state.Index = X86Fixture.UInt(parts[0]); parts[1].Split(',').Select(X86Fixture.UInt).ToArray().CopyTo(state.Entries, 0); parts[2].Split(',').Select(X86Fixture.UInt).ToArray().CopyTo(life.Idle, 0); create.Entries[0] = create.Collection.Root = X86Fixture.UInt(parts[3]); var calls = new List<string>();
            // 원본 표의 현재 값만 관측한다.
            string State() => state.Index + "#" + string.Join(',', state.Entries) + "#" + string.Join(',', life.Idle.Take(4)) + "#" + create.Entries[0];
            string[][] patches = r[5] == "-" ? [] : r[5].Split(';').Select(s => s.Split(',')).ToArray();
            // 가상 경계 직전 전체 상태를 기록한 뒤 명시 입력 쓰기만 반영한다.
            void Respond(string kind, uint token, params uint[] args)
            {
                calls.Add(kind + "," + token + (args.Length == 0 ? "" : "," + string.Join(',', args)) + "@" + raw.Snapshot(true) + "@" + State());
                // 원문 콜백 입력과 일치하는 패치만 실행한다.
                foreach (string[] p in patches) if (p[0] == kind && X86Fixture.UInt(p[1]) == token) { uint target = X86Fixture.UInt(p[2]); if (target == 0) state.Index = X86Fixture.UInt(p[4]); else raw.Patch(target, X86Fixture.Int(p[3]), X86Fixture.UInt(p[4])); }
            }
            var lifetime = new GumpLifetime(create, life, new() { Node = t => raw.Nodes.GetValueOrDefault(t) }); var hooks = new MenuGumpHooks { Node = t => raw.Nodes.GetValueOrDefault(t), BaseNode = t => raw.Nodes.GetValueOrDefault(t), Push = t => Respond("push", t), Pop = t => Respond("pop", t), Redraw = (t, e) => Respond("redraw", t, e ? 1U : 0), BaseRedraw = (t, e) => Respond("base_redraw", t, e ? 1U : 0), Attach = (t, x, y, p, m) => Respond("attach", t, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), p, m), AttachObject = (t, o, x, y, p, m) => Respond("object", t, o, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), p, m) }; var menu = new MenuGump(create, state, lifetime, hooks); Assert.Equal(r[3], raw.Snapshot(true)); Assert.Equal(r[4], State()); var results = new List<string>();
            // 정수/float 비트를 그대로 공개 연산에 전달한다.
            foreach (string action in r[2].Split(';'))
            {
                string[] p = action.Split(','); uint token = MenuRawSupport.Tokens[0]; string result = "-";
                // 각 공개 인자만 원문 DWORD로 읽는다.
                uint A(int index) => X86Fixture.UInt(p[index]);
                switch (p[0])
                {
                    case "Width": menu.SetWidth(token, BitConverter.UInt32BitsToSingle(A(1))); break; case "Height": menu.SetHeight(token, BitConverter.UInt32BitsToSingle(A(1))); break; case "Redraw": menu.Redraw(token, A(1) != 0); break; case "Expand": menu.StartExpand(token, A(1)); break; case "Idle": menu.Idle(token); break;
                    case "Item": var point = menu.ItemPosition(token, unchecked((int)A(1))); result = BitConverter.SingleToUInt32Bits(point.X) + "," + BitConverter.SingleToUInt32Bits(point.Y); break; case "Take": result = menu.TakeCurrent().ToString(); break; case "Current": result = menu.Current().ToString(); break; case "Count": result = menu.ItemCount().ToString(); break; case "Attach": menu.AttachCurrent(unchecked((int)A(1)), unchecked((int)A(2)), A(3)); break; case "Object": menu.AttachObjectCurrent(A(1), unchecked((int)A(2)), unchecked((int)A(3))); break; default: Assert.Fail("원문 메뉴 action 없음"); break;
                }
                results.Add(result);
            }
            Assert.True(r[6] == raw.Snapshot(true), $"{edition} 메뉴{r[1]} {r[2]} 최종 노드 불일치"); Assert.Equal(r[7], State()); Assert.Equal(r[8], calls.Count == 0 ? "-" : string.Join('|', calls)); Assert.Equal(r[9], string.Join(';', results)); Assert.All(life.Idle.Skip(4), t => Assert.Equal(0U, t));
        }
    }
    /// <summary>각130행의 전체 메뉴/기본 자식 바이트·콜백 인자/CRC·반환을 실제 PE 관찰과 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    public void NativeMenuLayout(string edition)
    {
        string[][] rows = X86Fixture.ReadFile("recovery-menulayout-observations.tsv").Where(r => r[0] == edition).ToArray(); Assert.Equal(130, rows.Length);
        // 원본의 독립 입력 행마다 전체 저장소를 다시 만든다.
        foreach (string[] r in rows)
        {
            var raw = new MenuRawSupport(r[3], false); uint token = MenuRawSupport.Tokens[0]; var create = new GumpCreateState(); create.Collection.Edition = edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072; var calls = new List<string>(); uint[] metrics = Words(r[4]), context = Words(r[6]); byte[]? text = r[5] == "-" ? null : [.. X86Fixture.Bytes(r[5]), 0]; string[][] patches = r[7] == "-" ? [] : r[7].Split(';').Select(s => s.Split(',')).ToArray();
            // 콜백 입력에 일치하는 원본 DWORD 패치만 수행한다.
            void Patch(string kind, uint t, uint index) { foreach (string[] p in patches) if (p[0] == kind && X86Fixture.UInt(p[1]) == t && X86Fixture.UInt(p[2]) == index) raw.Patch(X86Fixture.UInt(p[3]), X86Fixture.Int(p[4]), X86Fixture.UInt(p[5])); }
            // 현재 전체 실제 바이트의 CRC와 원본 인자를 기록한다.
            void Record(string kind, uint t, uint index, params string[] args) => calls.Add(kind + "," + t + "," + index + (args.Length == 0 ? "" : "," + string.Join(',', args)) + "@" + raw.Crc());
            // 일반 가상 경계는 관측 후 입력 패치를 적용한다.
            void Respond(string kind, uint t, params uint[] args) { Record(kind, t, uint.MaxValue, args.Select(v => v.ToString()).ToArray()); Patch(kind, t, uint.MaxValue); }
            var lifetime = new GumpLifetime(create, new(), new() { Node = t => raw.Nodes.GetValueOrDefault(t) }); var mh = new MenuGumpHooks { Node = t => raw.Nodes.GetValueOrDefault(t), BaseNode = t => raw.Nodes.GetValueOrDefault(t), Push = t => Respond("push", t), Pop = t => Respond("pop", t), Attach = (t, x, y, p, m) => Respond("attach", t, BitConverter.SingleToUInt32Bits(x), BitConverter.SingleToUInt32Bits(y), p, m) }; var menu = new MenuGump(create, new(), lifetime, mh);
            var hooks = new MenuGumpLayoutHooks { Node = mh.Node, BaseNode = mh.BaseNode, Push = mh.Push, Attach = mh.Attach, TitleFit = (t, a) => Respond("fit", t, a), TitleLayout = (t, a, b) => Respond("title_layout", t, a, b) };
            hooks.Plain = (t, i, ctx) => { Record("plain", token, (uint)i, uint.MaxValue.ToString(), HexWords(ctx), TextHex(MenuGumpLayout.MenuGumpItemBytes(raw.Nodes[t].Items[i]))); Patch("plain", token, (uint)i); return (unchecked((int)metrics[i * 4]), unchecked((int)metrics[i * 4 + 1])); };
            hooks.Marker = (t, ctx) => { Record("marker", token, uint.MaxValue, uint.MaxValue.ToString(), HexWords(ctx), "203e"); Patch("marker", token, uint.MaxValue); return (unchecked((int)metrics[^2]), unchecked((int)metrics[^1])); };
            hooks.Styled = (t, i) => { Record("styled", token, (uint)i); Patch("styled", token, (uint)i); return (unchecked((int)metrics[i * 4 + 2]), unchecked((int)metrics[i * 4 + 3])); };
            hooks.ClearStyle = (t, i) => { Record("clear", token, (uint)i); raw.Nodes[t].Items[i].Styled = 0; Patch("clear", token, (uint)i); };
            hooks.ParseStyle = (uint t, int i, ReadOnlySpan<byte> input, uint[] ctx) => { Record("parse", token, (uint)i, TextHex(input), HexWords(ctx)); raw.Nodes[t].Items[i].Styled = 0x19000000 + (uint)i * 4; Patch("parse", token, (uint)i); }; var layout = new MenuGumpLayout(create, menu, hooks); Assert.Equal(r[3], raw.Snapshot(false)); var results = new List<string>();
            // 실행할 공개 연산은 원문 action 열에서만 읽는다.
            foreach (string action in r[2].Split(';'))
            {
                string[] p = action.Split(','); string result = "-";
                switch (p[0]) { case "Layout": layout.Reflow(token); break; case "Append": layout.Append(token, text, X86Fixture.UInt(p[1]), X86Fixture.UInt(p[2]), X86Fixture.UInt(p[3]), X86Fixture.UInt(p[4])); break; case "Set": layout.SetText(token, X86Fixture.Int(p[1]), text, context); break; case "Measure": var size = layout.MeasureItem(token, X86Fixture.Int(p[1]), context); result = HexWords(unchecked((uint)size.Width), unchecked((uint)size.Height)); break; default: Assert.Fail("원문 메뉴 레이아웃 action 없음"); break; } results.Add(result);
            }
            Assert.True(r[8] == raw.Snapshot(false), $"{edition} 레이아웃{r[1]} {r[2]} 최종 전체 바이트 불일치"); Assert.True(r[9] == (calls.Count == 0 ? "-" : string.Join('|', calls)), $"{edition} 레이아웃{r[1]} {r[2]} 콜백/CRC 불일치"); Assert.Equal(r[10], string.Join(';', results));
        }
    }
}
