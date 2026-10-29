using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // 제품 작성 전에 동결한 도움말150/985행을 독립 대조한다.

/// <summary>현재 필드/원문/외부 효과 순서를 원본 기대값에만 비교한다.</summary>
public sealed class QuickHelpControlNativeTests
{
    /// <summary>원문이 문자열 입력으로 쓴 격리 주소 두 개.</summary>
    private const uint Topic = 306208768, Other = 306212864;
    /// <summary>실제 BYTE 문자열만16진수로 직렬화한다.</summary>
    private static string Hex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();
    /// <summary>저장소의 실제 현재 머리부터 이력을 읽는다.빈 항목도s로 남긴다.</summary>
    private static string History(QuickHelpHistory history) { string result = string.Join('/', history.Topics.Reverse().Select(text => 's' + Hex(text))); return result.Length == 0 ? "-" : result; }
    /// <summary>유효한 문자열 입력만 해석하며 PE 포인터를 호스트에서 역참조하지 않는다.</summary>
    private static string Resolve(uint token) { Assert.Contains(token, new[] { Topic, Other, 256U }); return token == Topic ? "themeHelp" : token == Other ? "otherHelp" : "lowHelp"; }
    /// <summary>원문 DWORD를 signed 좌표와 같은 비트로 실행한다.</summary>
    private static InputEvent Input(uint[] row) => new(row[0], unchecked((int)row[1]), unchecked((int)row[2]));
    /// <summary>signed/float 값을 원문 raw 비트 그대로 읽는다.</summary>
    private static float Float(uint bits) => BitConverter.UInt32BitsToSingle(bits);
    /// <summary>원본 열기 관문의 현재 등록/모달·생성/부착 입력만 공급한다.</summary>
    private static string Open(uint[] row)
    {
        var window = new GumpCreateNode { Depth = 120, BaseWidth = 450 }; var trace = new List<string>(); var state = new QuickHelpOpenState { HelpId = 13, AlternateId = 14, Root = 0, AreaLeft = 16, AreaWidth = 800 }; var hooks = new QuickHelpOpenHooks { ClosePopups = () => trace.Add("ClosePopups"), Lookup = id => id == 13 ? row[0] : 2, NoModal = () => row[1] == 0, CurrentModal = () => row[1], Construct = type => { trace.Add("Construct," + type); return 1; }, Node = token => { Assert.Equal(1U, token); return window; }, Attach = (token, x, y, parent, mode) => { Assert.Equal(1U, token); trace.Add($"Attach,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{parent},{mode}"); }, AddDepth = (token, delta) => { Assert.Equal(1U, token); window.Depth = unchecked(window.Depth + delta); }, Modal = (token, mode) => { Assert.Equal(1U, token); trace.Add("Modal," + mode); }, Handle = (token, code, first, second) => { Assert.Equal(1U, token); Assert.Equal(Topic, second); trace.Add($"Handle,{code},{first},{Hex("themeHelp")}"); return 1; } }; uint result = QuickHelpControl.OpenQuickHelpWindow(state, hooks, Topic, unchecked((int)row[2]), 71); return result + "|" + window.Depth + '|' + string.Join(';', trace);
    }
    /// <summary>원문 입력/버튼 serial/현재 이력·popup과 실제 분배 결과만 직렬화한다.</summary>
    private static string Event(uint[] row, bool zero)
    {
        var window = new QuickHelpWindowNode { Width = 450, Height = 350, CancelButton = 1, LearnButton = 2, BackButton = 3, LearnType = zero ? 0U : 71 }; var buttons = Enumerable.Range(0, 3).Select(index => new GumpCreateNode { Serial = (uint)(81 + index), Id = (uint)(901 + index) }).ToArray(); var history = new QuickHelpHistory(); var popup = new QuickHelpPopupState(); if (row[6] > 0) history.Push("firstHelp"); if (row[6] > 1) history.Push("secondHelp"); popup.Mark(row[7], "before"); var trace = new List<string>(); var hooks = new QuickHelpEventHooks
        {
            Local = _ => { trace.Add("Local"); return (Float(row[3]), Float(row[4])); }, Node = token => { Assert.InRange(token, 1U, 3U); return buttons[token - 1]; }, TypeName = type => "Unit" + type, Translate = text => { Assert.Equal("Type", text); return text; }, Text = Resolve, Request = (type, topic) => trace.Add($"Request,{type},{Hex(topic)}"), PageHeight = () => Float(row[5]), Scroll = delta => trace.Add("Scroll," + unchecked((uint)delta)), Link = topic => trace.Add("Link," + Hex(topic)), Kill = () => { trace.Add("Kill"); if (row[8] != 0) popup.Mark(0x2000abc, "changed"); }, Learn = type => trace.Add("Learn," + type), Queue = (code, first, second) => { Assert.True(first.Equals(second)); trace.Add($"Queue,{code},{Convert.ToHexString(first.Span).ToLowerInvariant()},{Convert.ToHexString(second.Span).ToLowerInvariant()}"); }, Base = _ => { trace.Add("Base"); return 7; }
        }; uint result = QuickHelpControl.DispatchQuickHelpWindow(window, Input(row), history, popup, hooks); return result + "|" + History(history) + '|' + string.Join(';', trace);
    }
    /// <summary>전체150행의 현재 반환/깊이/이력/80BYTE/입력 효과를 대조한다.</summary>
    [Fact]
    public void All150NativeOpenHistoryPopupAndEventObservationsMatch()
    {
        int count = 0;
        // 앞2열만 독립 실행 입력으로 쓰고 뒤 원본 문자열은 비교에만 쓴다.
        foreach (string[] row in X86Fixture.Read("quick-help-control"))
        {
            Assert.Equal(3, row.Length); string result;
            if (row[0] == "O") result = Open(row[1].Split(',').Select(X86Fixture.UInt).ToArray());
            else if (row[0] == "H") { string[] input = row[1].Split(','); var history = new QuickHelpHistory();
                // 입력의 중복/NUL/빈 문자열도 원문 순서 그대로 준다.
                foreach (string name in X86Fixture.Text(input[0]).Split(';')) history.Push(name); var returns = new List<string>();
                // Back 반환은 실제 제품 저장소에서만 읽는다.
                for (uint i = 0; i < X86Fixture.UInt(input[1]); i++) { string? value = history.Back(); returns.Add(value is null ? "n" : 's' + Hex(value)); } result = History(history) + '|' + string.Join(',', returns);
            }
            else if (row[0] == "P") { string[] input = row[1].Split(','); var popup = new QuickHelpPopupState(); popup.Mark(0, "before"); popup.Mark(0x20000a0, X86Fixture.UInt(input[0]) != 0 ? X86Fixture.Text(input[1]) : null); result = popup.Command + "|" + Convert.ToHexString(popup.ArgumentBytes).ToLowerInvariant(); }
            else { Assert.Contains(row[0], new[] { "E", "Z" }); result = Event(row[1].Split(',').Select(X86Fixture.UInt).ToArray(), row[0] == "Z"); } Assert.True(row[2] == result, $"Help 제어 원본행{count} {row[0]} {row[1]}\n기대:{row[2]}\n실제:{result}"); count++;
        }
        Assert.Equal(150, count);
    }
    /// <summary>전체985행의 두 접미/3단계 조회·원문 보존/Show 이후 flags/Push 뒤 좌표를 대조한다.</summary>
    [Fact]
    public void All985NativeRequestTopicsLookupAndCurrentWindowObservationsMatch()
    {
        int count = 0;
        // 앞2열만 입력이며 뒤 trace/현재 필드 관측은 비교에만 사용한다.
        foreach (string[] row in X86Fixture.Read("quick-help-request"))
        {
            Assert.Equal(3, row.Length); string[] input = row[1].Split(','); int[] results = input.Take(3).Select(X86Fixture.Int).ToArray(); int lookup = 0; uint mode = X86Fixture.UInt(input[4]); var node = new GumpCreateNode { Flags = mode is 1 or 3 ? 8U : 0, X = Float(0x41880000), Y = Float(0xc1100000) }; var trace = new List<string>(); var hooks = new QuickHelpRequestHooks { Print = (pattern, value) => { trace.Add($"Format,{Hex(pattern)},{Hex(value)}"); Assert.Contains("%s", pattern); return pattern.Replace("%s", value, StringComparison.Ordinal); }, Lookup = name => { Assert.True(lookup < results.Length); int result = results[lookup++]; trace.Add($"Lookup,{Hex(name)},{unchecked((uint)result)}"); return result; }, History = name => trace.Add("History," + Hex(name)), Show = (type, name) => { trace.Add($"Show,{type},{Hex(name)}"); if (mode == 2) node.Flags = 8; else if (mode == 3) node.Flags = 0; }, Push = () => { trace.Add("Push"); if (mode == 4) { node.Flags = 0x800; node.X = Float(0x80000000); node.Y = Float(0x7fc12345); } }, Attach = (x, y, parent, flags) => trace.Add($"Attach,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},{parent},{flags}") }; string topic = X86Fixture.Text(row[0]), before = topic; new QuickHelpRequest(node, hooks).Open(X86Fixture.UInt(input[3]), topic); string actual = string.Join(';', trace) + $"|{node.Flags},{BitConverter.SingleToUInt32Bits(node.X)},{BitConverter.SingleToUInt32Bits(node.Y)}"; Assert.True(row[2] == actual, $"Help 요청 원본행{count} {row[0]} {row[1]}\n기대:{row[2]}\n실제:{actual}"); Assert.Equal(before, topic); count++;
        }
        Assert.Equal(985, count);
    }
}
