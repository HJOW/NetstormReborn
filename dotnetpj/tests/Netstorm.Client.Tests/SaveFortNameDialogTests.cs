using Netstorm.Client.Gump;
using Xunit;

namespace Netstorm.Client.Tests;

/// <summary>원본 이름 및 숫자 모달의 배치와 선택 결과를 확인한다.</summary>
public sealed class SaveFortNameDialogTests
{
    /// <summary>자식 저장소의 생성·부착을 추적해 원본 배치와 사건 분기를 검사한다.</summary>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void CreatesOriginalChildrenAndHandlesResult(bool money)
    {
        var state = new GumpCreateState { ScreenWidth = 1024, ScreenHeight = 768 }; state.Entries[0] = 99;
        var nodes = new Dictionary<uint, GumpCreateNode>(); uint next = 0, killed = 0, modal = 0; string excluded = "", initial = ""; uint accepted = 0, cancelled = 0;
        var hooks = new SaveFortNameDialogHooks
        {
            Node = t => nodes[t] as SaveFortNameDialogNode, Base = t => nodes[t], Entry = t => nodes[t] as TextEntryGumpNode,
            Allocate = size => { uint t = ++next; nodes[t] = size switch { 0xdc or 0xd8 when t == 1 => new SaveFortNameDialogNode(), 0x110 => new TextEntryGumpNode(), _ => new GumpCreateNode() }; nodes[t].Serial = t + 100; return t; },
            Decoration = (t, w, h) => { nodes[t].BaseWidth = w; nodes[t].BaseHeight = h; nodes[t].Scale = 1; },
            TextEntry = (t, w, h, value) => { nodes[t].BaseWidth = w; nodes[t].BaseHeight = h; initial = value; },
            Excluded = (_, value) => excluded = value,
            Background = (t, w, h) => { nodes[t].BaseWidth = w; nodes[t].BaseHeight = h; },
            Title = (t, _) => nodes[t].BaseWidth = 120, Stone = (t, _) => { nodes[t].BaseWidth = 70; nodes[t].Flags = 8; },
            FitTitle = _ => { }, FitStone = _ => { }, Push = _ => { }, Translate = s => s,
            Attach = (t, x, y, parent, _) => { nodes[t].X = x; nodes[t].Y = y; nodes[t].Parent = parent; },
            Depth = (_, _) => { }, Modal = t => modal = t, Kill = t => killed = t,
            Accept = _ => accepted, Cancel = _ => cancelled, BaseHandle = (_, _, _, _) => 23
        };
        var dialog = new SaveFortNameDialog(state, hooks); uint rootToken = money ? dialog.ShowMoney(42) : dialog.Show(1, 0, "Current", "Chosen");
        var root = (SaveFortNameDialogNode)nodes[rootToken]; var entry = (TextEntryGumpNode)nodes[root.Entry];
        Assert.Equal(rootToken, modal); Assert.Equal(money ? "0" : "Chosen", initial); Assert.Equal(money ? 10 : 40, entry.MaxLength); Assert.Equal(0u, entry.Multiline);
        Assert.Equal(160, entry.BaseWidth); Assert.Equal(45, entry.Y); Assert.Equal(money ? 200 : 150, root.BaseWidth);
        Assert.Equal((1024 - root.BaseWidth) / 2, root.X); Assert.Equal((768 - 104) / 2, root.Y);
        Assert.Contains('.', excluded); if (money) Assert.DoesNotContain('0', excluded); else Assert.Contains(',', excluded);
        Assert.Equal(1u, dialog.Handle(rootToken, 0x02000038, 0, 0)); Assert.Equal(0u, killed);
        accepted = 1; Assert.Equal(1u, dialog.Handle(rootToken, 0x0200002c, nodes[root.Ok].Serial, 0)); Assert.Equal(rootToken, killed);
        killed = 0; cancelled = 1; Assert.Equal(1u, dialog.Handle(rootToken, 0x0200002c, nodes[root.Cancel].Serial, 0)); Assert.Equal(rootToken, killed);
        Assert.Equal(23u, dialog.Handle(rootToken, 0x0200002c, 999, 0));
    }

    /// <summary>Scenario 전용 입력 모달의 기하·제한·다중 줄 형식을 확인한다.</summary>
    /// <remarks>2026-11-11 0.354에서 S1 시나리오 입력 재대조와 함께 추가했다.</remarks>
    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public void CreatesScenarioInputWithClampedGeometry(bool multiline)
    {
        var state = new GumpCreateState { ScreenWidth = 1024, ScreenHeight = 768 }; state.Entries[0] = 99;
        var nodes = new Dictionary<uint, GumpCreateNode>(); uint next = 0, modal = 0; string initial = "", inputTitle = "";
        var hooks = new SaveFortNameDialogHooks
        {
            Node = t => nodes[t] as SaveFortNameDialogNode, Base = t => nodes[t], Entry = t => nodes[t] as TextEntryGumpNode,
            Allocate = size => { uint t = ++next; nodes[t] = size switch { 0x128 when t == 1 => new SaveFortNameDialogNode(), 0x110 => new TextEntryGumpNode(), _ => new GumpCreateNode() }; nodes[t].Serial = t + 100; return t; },
            Decoration = (t, w, h) => { nodes[t].BaseWidth = w; nodes[t].BaseHeight = h; nodes[t].Scale = 1; },
            TextEntry = (t, w, h, value) => { nodes[t].BaseWidth = w; nodes[t].BaseHeight = h; initial = value; },
            Background = (t, w, h) => { nodes[t].BaseWidth = w; nodes[t].BaseHeight = h; },
            Title = (t, _) => nodes[t].BaseWidth = 120, Stone = (t, _) => { nodes[t].BaseWidth = 70; nodes[t].Flags = 8; },
            InputTitle = (_, _, value) => inputTitle = value,
            FitTitle = _ => { }, FitStone = _ => { }, Push = _ => { }, Translate = s => "T:" + s,
            Attach = (t, x, y, parent, _) => { nodes[t].X = x; nodes[t].Y = y; nodes[t].Parent = parent; },
            Depth = (_, _) => { }, Modal = t => modal = t,
            Accept = _ => 0u, Cancel = _ => 0u, BaseHandle = (_, _, _, _) => 23
        };
        var dialog = new SaveFortNameDialog(state, hooks);
        Assert.Throws<ArgumentOutOfRangeException>(() => dialog.ShowScenarioInput("T", 1, multiline));
        Assert.Throws<ArgumentOutOfRangeException>(() => dialog.ShowScenarioInput("T", 16385, multiline));
        uint rootToken = dialog.ShowScenarioInput("Subject", 200, multiline);
        var root = (SaveFortNameDialogNode)nodes[rootToken]; var entry = (TextEntryGumpNode)nodes[root.Entry];
        Assert.Equal(rootToken, modal); Assert.Equal("", initial); Assert.Equal("T:Subject", inputTitle);
        Assert.Equal(200, entry.MaxLength); Assert.Equal(multiline ? 1u : 0u, entry.Multiline);
        Assert.Equal(multiline ? 1u : 0u, entry.ShowVertical); Assert.Equal(1u, entry.ShowHorizontal);
        Assert.Equal(620f, root.BaseWidth); Assert.Equal(multiline ? 360f : 120f, root.BaseHeight);
        Assert.Equal(1u, root.FortMode); Assert.Equal(1u, root.CanCancel);
        Assert.Equal((1024 - 620) / 2f, root.X); Assert.Equal((768 - root.BaseHeight) / 2f, root.Y);
    }
}
