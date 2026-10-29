using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests; // 도움말 제어를 실제 공통 입력/등록/트리/지연 수명·원시 큐에 연결한다.

/// <summary>독립 원문 전수 외 현재 효과/바이트 alias와 실제 UI 장치 조립을 확인한다.</summary>
public sealed class QuickHelpControlTests
{
    /// <summary>실제 등록/모달 전달·요청/이력/Back·현재 popup/원시 큐·자식 우선 반납을 함께 확인한다.</summary>
    [Fact]
    public void ActualCommonOpenRequestHistoryBackRawQueueAndDelayedLifetime()
    {
        var common = new ScrollWidgetScene(); var window = new QuickHelpWindowNode { BackButton = 41 }; var back = new GumpCreateNode(); common.Nodes.Add(40, window); common.Nodes.Add(41, back); var history = new QuickHelpHistory(); var popup = new QuickHelpPopupState(); var shown = new List<(uint Type, string Topic)>(); var queue = new InputQueue(); var stored = new Dictionary<int, byte[]>(); var request = new QuickHelpRequest(window, new() { Lookup = text => { Assert.Equal("theme", text); return 0; }, Print = (pattern, text) => { Assert.Equal("%s", pattern); return text; }, History = history.Push, Show = (type, text) => shown.Add((type, text)), Push = () => common.Tree.Push(40), Attach = (x, y, parent, mode) => common.TreeHooks.Attach!(40, x, y, parent, mode) }); var events = new QuickHelpEventHooks { Node = token => common.Nodes.GetValueOrDefault(token), TypeName = type => { Assert.Equal(71U, type); return "sunCannon"; }, Translate = text => text, Text = token => { Assert.Equal(0x61000000U, token); return "themeHelp"; }, Request = request.Open, Link = text => request.Open(0, text), Base = _ => 0, PageHeight = () => 70, Scroll = _ => { }, Queue = (code, first, second) => { Assert.True(first.Equals(second)); Assert.Equal("afterKill", Encoding.Latin1.GetString(first.Span)); stored.Add(123, first.ToArray()); queue.Push(code, 123, 123); } }; QuickHelpControl.BindQuickHelpDevices(events, 40, common.Collection, common.Lifetime); var kill = events.Kill!; events.Kill = () => { kill(); popup.Mark(0x2000abc, "afterKill"); }; QuickHelpControl.BindQuickHelpWindow(common.EventHooks, common.InputHooks, history, popup, token => token == 40 ? window : null, _ => events);
        var state = new QuickHelpOpenState { Root = 1, AreaWidth = 64 }; int constructs = 0; var open = new QuickHelpOpenHooks { ClosePopups = () => { }, Lookup = serial => common.Create.Entries.FirstOrDefault(token => token != 0 && common.Nodes[token].Serial == serial), NoModal = () => common.LifeState.CurrentDialog == 0, CurrentModal = () => common.LifeState.CurrentDialog, Construct = _ => { constructs++; common.Factory.Construct(40, 19, 40, 70, 1); window.Flags |= 0x800; window.Depth = 120; common.Factory.Construct(41, 0, 6, 5, 1); common.Tree.Attach(41, 0, 60, 40, 1); state.HelpId = window.Serial; return 40; }, Node = token => common.Nodes.GetValueOrDefault(token), Attach = common.Tree.Attach, AddDepth = common.Tree.AddDepth, Modal = (token, mode) => { Assert.Equal(1U, mode); common.Input.AddDialog(token); }, Handle = (token, code, first, second) => common.InputHooks.Handle!(token, code, first, second) };
        popup.Mark(0x20000a0); Assert.Equal(40U, QuickHelpControl.OpenQuickHelpWindow(state, open, 0x61000000, 1, 0)); Assert.Equal((12f, 40f, 164U, 161U), (window.X, window.Y, window.Depth, back.Depth)); Assert.Equal(40U, common.LifeState.CurrentDialog); Assert.Equal(40U, QuickHelpControl.OpenQuickHelpWindow(state, open, 0x61000000, 1, 0)); Assert.Equal(1, constructs); Assert.Equal(new[] { "themeHelp" }, history.Topics); Assert.Equal(40U, common.Input.Route(0x2000017, 0, 71)); Assert.Equal(new[] { "themeHelp", "sunCannonType" }, history.Topics); Assert.Equal(40U, common.Input.Route(0x200002c, back.Serial, 0)); Assert.Equal(new[] { "themeHelp" }, history.Topics); Assert.Equal("themeHelp", shown[^1].Topic); Assert.Equal(40U, common.Input.Route(0x200002c, back.Serial, 0)); Assert.Empty(history.Topics); Assert.Equal(0U, common.LifeState.CurrentDialog); Assert.NotEqual(0U, window.Flags & 0x10000); var pending = queue.Pop(false); Assert.Equal(new InputEvent(0x20000a0, 123, 123), pending); Assert.Equal("afterKill", Encoding.Latin1.GetString(stored[pending.X])); Assert.True(queue.Empty); common.Lifetime.Cleanup(); Assert.Equal(1U, common.Create.Count); Assert.Contains(40U, common.Released); Assert.Contains(41U, common.Released);
    }
    /// <summary>두 번의 현재 모달 조회와 부착 후 현재 depth/화면 상태를 구별한다.</summary>
    [Fact]
    public void OpenReadsCurrentModalTwiceAndDepthAfterAttach()
    {
        var state = new QuickHelpOpenState { HelpId = 13, AlternateId = 14, Root = 9, AreaWidth = 80 }; var node = new GumpCreateNode { BaseWidth = 40, Depth = 120 }; int modalCalls = 0; uint deltaSeen = 0; var hooks = new QuickHelpOpenHooks { ClosePopups = () => state.AreaLeft = 7, Lookup = serial => serial == 13 ? 0U : 2, NoModal = () => false, CurrentModal = () => ++modalCalls == 1 ? 99U : 2, Construct = _ => 1, Node = _ => node, Attach = (token, x, y, parent, mode) => { Assert.Equal((1U, 27f, 40f, 9U, 0U), (token, x, y, parent, mode)); node.Depth = 250; }, AddDepth = (_, delta) => deltaSeen = delta, Modal = (_, _) => { }, Handle = (_, code, _, second) => { Assert.Equal((0x2000057U, 17U), (code, second)); return 1; } }; Assert.Equal(1U, QuickHelpControl.OpenQuickHelpWindow(state, hooks, 17, 0, 0)); Assert.Equal((2, 174U), (modalCalls, deltaSeen)); hooks.Lookup = serial => serial == 13 ? 77U : 2; hooks.CurrentModal = () => 4; hooks.Construct = null; hooks.Attach = null; hooks.Modal = null; hooks.Handle = null; Assert.Equal(77U, QuickHelpControl.OpenQuickHelpWindow(state, hooks, 17, 0, 0));
    }
    /// <summary>큐 콜백 중에도 두 인자는 같은 실제80BYTE를 참조하며 사본으로 대체하지 않는다.</summary>
    [Fact]
    public void BackQueueUsesOldCommandAndSameCurrentMutablePopupByteView()
    {
        var window = new QuickHelpWindowNode { Width = 40, Height = 30, BackButton = 1 }; var back = new GumpCreateNode { Serial = 83 }; var history = new QuickHelpHistory(); history.Push("themeHelp"); var popup = new QuickHelpPopupState(); popup.Mark(0x72000190, "before"); var hooks = new QuickHelpEventHooks { Local = _ => (1, 1), Node = _ => back, Kill = () => popup.Mark(0x2000abc, "afterKill"), Queue = (code, first, second) => { Assert.Equal(0x72000190U, code); Assert.True(first.Equals(second)); Assert.Equal("afterKill", Encoding.Latin1.GetString(first.Span)); popup.ArgumentBytes[0] = (byte)'Z'; Assert.Equal((byte)'Z', first.Span[0]); Assert.Equal((byte)'Z', second.Span[0]); } }; Assert.Equal(1U, QuickHelpControl.DispatchQuickHelpWindow(window, new(0x200002c, 83, 0), history, popup, hooks)); Assert.Empty(history.Topics); Assert.Equal("ZfterKill", popup.Argument());
    }
    /// <summary>null/빈문자열/짧은 덮어쓰기와 물리80BYTE/NUL·거부 전 상태를 확인한다.</summary>
    [Fact]
    public void PopupAndHistoryPreserveRawBytesNullEmptyAndAsciiOnlyDuplicates()
    {
        var popup = new QuickHelpPopupState(); popup.Mark(7, "before"); popup.Mark(8, ""); Assert.Equal(new byte[] { 0, (byte)'e', (byte)'f', (byte)'o', (byte)'r', (byte)'e', 0 }, popup.ArgumentBytes.AsSpan(0, 7).ToArray()); byte[] prior = popup.ArgumentBytes.ToArray(); Assert.Throws<ArgumentException>(() => popup.Mark(9, new string('x', 80))); Assert.Equal(8U, popup.Command); Assert.Equal(prior, popup.ArgumentBytes); popup.Mark(10); Assert.All(popup.ArgumentBytes, item => Assert.Equal(0, item)); Array.Fill(popup.ArgumentBytes, (byte)'x'); Assert.Throws<ArgumentException>(() => popup.ArgumentView()); var history = new QuickHelpHistory(); history.Push("Alpha\0ignore"); history.Push("ALPHA"); history.Push("Beta"); history.Push("Alpha"); history.Push("é"); history.Push("É"); Assert.Equal(new[] { "Alpha", "Beta", "Alpha", "é", "É" }, history.Topics); history.Push(""); Assert.Equal("É", history.Back()); Assert.Equal("é", history.Back()); var empty = new QuickHelpHistory(); empty.Push(""); empty.Push("Other"); Assert.Equal("", empty.Back()); Assert.Null(empty.Back());
    }
    /// <summary>짧은 접미/한도는 외부 효과 전 거부하고 직접 TYPE는 원문 주제 길이를 바꾸지 않는다.</summary>
    [Fact]
    public void RequestRejectsUndefinedBuffersButDirectTypeKeepsOriginalTopic()
    {
        var node = new GumpCreateNode { Flags = 8 }; var request = new QuickHelpRequest(node, new());
        // 두 접미 비교의 버퍼 앞 읽기와 종료 NUL 공간 부재를 구별한다.
        foreach (string topic in new[] { "abc", "type", new string('x', 80) }) Assert.ThrowsAny<ArgumentException>(() => request.Open(0, topic)); string? shown = null; var direct = new QuickHelpRequest(node, new() { History = text => Assert.Equal(100, text.Length), Show = (type, text) => { Assert.Equal(17U, type); shown = text; } }); direct.Open(17, new string('x', 100) + "\0ignore"); Assert.Equal(new string('x', 100), shown);
    }
    /// <summary>부호 있는 좌표 칸의 낮은 TYPE 경로와 미연결 효과를 현재 분기에서만 요구한다.</summary>
    [Fact]
    public void SignedTypeArgumentAndSelectedDeviceBoundariesRemainExact()
    {
        var window = new QuickHelpWindowNode { Width = 30, Height = 20 }; var hooks = new QuickHelpEventHooks { Local = _ => (1, 1), TypeName = type => { Assert.Equal(uint.MaxValue, type); return "Unit\0ignored"; }, Translate = text => { Assert.Equal("Type", text); return "Type\0ignored"; }, Request = (type, text) => Assert.Equal((uint.MaxValue, "UnitType"), (type, text)), Base = _ => 7 }; Assert.Equal(1U, QuickHelpControl.DispatchQuickHelpWindow(window, new(0x2000017, 0, -1), new(), new(), hooks)); hooks.TypeName = _ => new string('x', 76); Assert.Throws<ArgumentException>(() => QuickHelpControl.DispatchQuickHelpWindow(window, new(0x2000017, 0, 1), new(), new(), hooks)); Assert.Equal(7U, QuickHelpControl.DispatchQuickHelpWindow(window, new((uint)'A', 0, 0), new(), new(), hooks)); Assert.Equal(1U, QuickHelpControl.DispatchQuickHelpWindow(window, new(0x2000025, 0, 0), new(), new(), hooks)); hooks.Local = _ => (float.NaN, 1); int killed = 0; hooks.Kill = () => killed++; Assert.Equal(1U, QuickHelpControl.DispatchQuickHelpWindow(window, new(InputCode.LeftButton, 0, 0), new(), new(), hooks)); Assert.Equal(1, killed);
    }
}
