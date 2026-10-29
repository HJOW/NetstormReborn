using System.Text;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 내장 Tell/HTML 부모의 독립 원본82행을 그대로 대조한다.

/// <summary>Config/파서/다중 줄/가상 장치만 명시하고 현재 부모/호출 순서를 검사한다.</summary>
public sealed class EmbeddedTellNativeTests
{
    /// <summary>현재 원문 BYTE를16진수 관측으로 바꾼다.</summary>
    private static string Hex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();
    /// <summary>첫 NUL 뒤를 포함한 전체8192BYTE의 원본 FNV32를 관측한다.</summary>
    private static uint Fingerprint(byte[] bytes)
    {
        uint value = 2166136261;
        // 원본 기대값을 만들지 않고 실제 현재 영역 전체를 누적한다.
        foreach (byte item in bytes) value = unchecked((value ^ item) * 16777619); return value;
    }
    /// <summary>두 x87 제어모드의 조회/설치·전체 버퍼 지문/현재 serial/효과순서를 전수 대조한다.</summary>
    [Fact]
    public void All82NativeEmbeddedTellQueriesInstallBuffersAndCurrentSerialMatch()
    {
        int count = 0;
        // 앞5열은 입력이며 뒤9열의 결과는 비교에만 사용한다.
        foreach (string[] row in X86Fixture.Read("embedded-tell"))
        {
            if (row[0] == "op") continue; Assert.Equal(14, row.Length); uint mask = X86Fixture.UInt(row[1]); string key = X86Fixture.Text(row[2]), source = X86Fixture.Text(row[3]); var node = new HtmlBodyGumpNode(); Array.Fill(node.Body, (byte)'Z'); Array.Fill(node.Links, (byte)'Z'); var child = new GumpCreateNode(); var oldChild = new GumpCreateNode(); var trace = new List<string>(); var create = new GumpCreateState();
            // 같은 부모/현재 두 자식의 실제 기본 저장소만 반환한다.
            GumpCreateNode? Base(uint token) => token == 1 ? node : token == 2 ? child : token == 3 ? oldChild : null;
            var factory = new GumpCreate(create, new() { Node = Base }); var hooks = new HtmlBodyGumpHooks
            {
                Node = token => token == 1 ? node : null, Base = Base, ConstructBase = (t, c, w, h, f) => factory.Construct(t, c, w, h, f), Allocate = bytes => { Assert.Equal(0xd8, bytes); trace.Add("allocateText"); return 2; }, ConstructText = (t, w, h, font, color) => { Assert.Equal(2U, t); trace.Add($"constructText:{w}:{h}:{font}:{color}"); }, Kill = _ => trace.Add("kill"), Push = token => { trace.Add("push"); Base(token)!.Flags |= 8; }, Attach = (token, x, y, _, mode) => { trace.Add($"attach:{(token == 1 ? "parent" : "text")}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{mode}"); Base(token)!.X = x; Base(token)!.Y = y; }, Styled = (_, text) => trace.Add("styled:" + Hex(text)), Scroll = (_, value) => trace.Add("scroll:" + value), Slider = (_, value) => trace.Add("slider:" + value), Scrollable = (_, value) => trace.Add("scrollable:" + value)
            }; var body = new HtmlBodyGump(new() { ScreenWidth = 640, ScreenHeight = 480, Kind = 20, Color = 7 }, hooks);
            // 원본에 공급한 같은 명시 파서 장치만 쓰고 기대 본문/지문은 공급하지 않는다.
            void Parse(string input, Memory<byte> output, Memory<byte> links) { int nul = input.IndexOf('\0'); if (nul >= 0) input = input[..nul]; Assert.Equal(0, output.Span[0]); Assert.Equal(0, links.Span[0]); trace.Add("parse:" + Hex(input)); byte[] value = Encoding.Latin1.GetBytes('[' + input + ']'); value.AsSpan().CopyTo(output.Span); output.Span[value.Length] = 0; "link\0"u8.CopyTo(links.Span); if ((mask & 32) != 0) node.Serial = 9001; }
            uint result = 0;
            if (row[0] == "SetHtml") { body.Construct(1); node.X = 2.25f; node.Y = -3.5f; if ((mask & 16) != 0) node.Text = 3; if ((mask & 8) != 0) node.Flags &= ~8U; trace.Clear(); body.SetHtml(1, source, Parse); }
            else
            {
                Assert.Equal("Tell", row[0]); var tell = new EmbeddedTellHooks { Mission = (mask & 1) == 0 ? null : name => { trace.Add("mission:" + Hex(name)); return (mask & 2) != 0 ? source : null; }, Fallback = name => { trace.Add("fallback:" + Hex(name)); return (mask & 4) != 0 ? source : null; }, Missing = name => trace.Add("missing:" + Hex(name)), Construct = () => { trace.Add("new"); return body.Construct(1); }, SetHtml = (token, input) => body.SetHtml(token, input, Parse), Serial = token => { Assert.Equal(1U, token); return node.Serial; } }; result = EmbeddedTell.ShowEmbeddedTell(key, tell);
            }
            uint[] actual = [result, node.Flags, node.Vtable, node.Serial, node.Text != 0 ? 1U : 0, Fingerprint(node.Body), Fingerprint(node.Links)]; Assert.Equal(row.Skip(5).Take(7).Select(X86Fixture.UInt), actual); Assert.Equal(row[12], string.Join(';', trace)); Assert.Contains(X86Fixture.UInt(row[4]), new uint[] { 639, 895 }); Assert.True(X86Fixture.UInt(row[13]) > 0); count++;
        }
        Assert.Equal(82, count);
    }
}
