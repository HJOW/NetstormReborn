using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 메뉴 선택/명령/하위 트리/캡처/지연 삭제 연결의 고정 계약 검사다.

/// <summary>정적 cpppj 계약과 실제 조립 검사.독립 PE 입력 관찰/L1이나 사용자 플레이 완료 검증은 아니다.</summary>
public sealed class MenuGumpInputTests
{
    /// <summary>실제 hover/누름→전체 명령/인자→소리→Kill/닫힘 부모 통지→Cleanup의 연결.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, true)]
    [InlineData(OriginalEdition.Cd1072, true)]
    [InlineData(OriginalEdition.Patch1078, false)]
    [InlineData(OriginalEdition.Cd1072, false)]
    public void ActualRouteCommandParameterAndClosedNotice(OriginalEdition edition, bool explicitParameter)
    {
        var s = new MenuInputScene(edition); s.View.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 1U | (explicitParameter ? 0x80U : 0), 77, 0x89abcdef); s.View.Attach(); uint id = s.Ui.Menu.Node.Id, serial = s.Ui.Menu.Node.Serial; s.Motion(3, 4); Assert.Equal(0, s.Ui.Menu.Node.Selection); Assert.Equal(8U, s.Ui.Menu.Node.Items[0].Flags & 8); s.Ui.Scene.Draw.Draw(40); Assert.NotEmpty(s.View.Shapes); Assert.Equal(40U, s.Click()); Assert.Equal((77U, id, explicitParameter ? 0x89abcdefU : 0x10009U), Assert.Single(s.Commands)); Assert.Equal(("openSubGump.wav", 0, 0, 0, 0, 0), Assert.Single(s.Sounds)); Assert.Contains((s.State.ClosedEvent, serial, serial), s.Ui.Scene.Notices); Assert.NotEqual(0U, s.Ui.Menu.Node.Flags & 0x10000); Assert.DoesNotContain(40U, s.Ui.Scene.Released); s.Ui.Scene.Lifetime.Cleanup(); Assert.Contains(40U, s.Ui.Scene.Released); Assert.Equal(0U, s.Ui.Menu.State.Instances);
    }
    /// <summary>disabled/strict 상한/outside/마지막 행 선택과 강조 해제를 실제 프레임 통지에서 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualHoverDisabledOutsideAndLastRow(OriginalEdition edition)
    {
        var s = new MenuInputScene(edition); s.View.Begin(); s.Ui.Control.Controls.AddText("A"u8); s.Ui.Control.Controls.AddText("A"u8, enabled: false); s.Ui.Control.Controls.AddText("A"u8); s.View.Attach(); s.Motion(3, 4); Assert.Equal(0, s.Ui.Menu.Node.Selection); s.Motion(3, 20); Assert.Equal(-1, s.Ui.Menu.Node.Selection); Assert.Equal(0U, s.Ui.Menu.Node.Items[0].Flags & 8); s.Motion(3, 36); Assert.Equal(2, s.Ui.Menu.Node.Selection); s.Motion((int)s.Ui.Menu.Node.X + (int)s.Ui.Menu.Node.Width, 4); Assert.Equal(-1, s.Ui.Menu.Node.Selection); s.Motion(3, 4, 0x4000000); Assert.Equal(-1, s.Ui.Menu.Node.Selection); s.Ui.Menu.Node.Offsets[0] = 100; s.Ui.Menu.Node.Offsets[1] = 100; s.Motion(3, 4); Assert.Equal(2, s.Ui.Menu.Node.Selection);
    }
    /// <summary>가로는 localX/항목 폭으로 선택하고 전체 Redraw를 요청한다.compact 세로는 안쪽 모든 y에서0을 선택한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualHorizontalAndCompactSelection(OriginalEdition edition)
    {
        var s = new MenuInputScene(edition); s.View.Begin(true); s.Ui.Control.Controls.AddText("A"u8); s.Ui.Control.Controls.AddText("A"u8); s.View.Attach(); s.Motion(2 + (int)s.Ui.Menu.Node.ItemWidth + 1, 4); Assert.Equal(1, s.Ui.Menu.Node.Selection); Assert.Empty(s.Invalidated); Assert.Contains("base:40,False", s.Ui.Menu.Calls); s.Motion(3, 4); Assert.Equal(0, s.Ui.Menu.Node.Selection); Assert.Equal(0U, s.Ui.Menu.Node.Items[1].Flags & 8); var compact = new MenuInputScene(edition); compact.View.Begin(); compact.Ui.Control.Controls.AddText("A"u8); compact.Ui.Control.Controls.AddText("A"u8); compact.View.Attach(); compact.Ui.Menu.Node.MenuFlags |= 0x1000000; compact.Motion(3, 20); Assert.Equal(0, compact.Ui.Menu.Node.Selection);
    }
    /// <summary>이전 행 변경 영역 콜백 뒤 현재 selection 필드를 다시 읽어 강조를 지우는 부분 갱신.</summary>
    [Fact]
    public void RedrawCallbackUsesCurrentOldSelection()
    {
        var s = new MenuInputScene(); s.View.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0); s.View.Attach(); s.Ui.Menu.Node.Selection = 0; s.Ui.Menu.Node.Items[0].Flags |= 8; s.Ui.Menu.Node.Items[1].Flags |= 8; int calls = 0; s.OnInvalidate = _ => { if (++calls == 1) s.Ui.Menu.Node.Selection = 1; }; s.Motion(3, 36); Assert.Equal(2, s.Ui.Menu.Node.Selection); Assert.Equal(new uint[] { 8, 0, 8 }, s.Ui.Menu.Node.Items.Take(3).Select(i => i.Flags & 8)); Assert.Equal(new[] { new ScreenRect(2, 3, 35, 19), new ScreenRect(2, 35, 35, 51) }, s.Invalidated);
    }
    /// <summary>명령 콜백이 바꾼 현재 선택/flags가 후속 Close/삭제를 결정하며 선택 인덱스는 그대로 유지된다.</summary>
    [Fact]
    public void CommandCallbackChangesFollowingCloseDecision()
    {
        var s = new MenuInputScene(); s.View.Begin(); s.Ui.Control.Controls.AddText("A"u8, 77, 88); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 2); s.View.Attach(); s.OnCommand = (_, _, _) => s.Ui.Menu.Node.Selection = 1; Assert.Equal(40U, s.Click()); Assert.Single(s.Commands); Assert.Equal(1, s.Ui.Menu.Node.Selection); Assert.Equal(0U, s.Ui.Menu.Node.Flags & 0x10000); Assert.Equal(0U, s.Ui.Scene.LifeState.KillCount); Assert.Empty(s.Ui.Scene.Notices); Assert.Equal("openSubGump.wav", Assert.Single(s.Sounds).Name);
    }
    /// <summary>실제 동적 요청→생성/부착→idle 확장→닫기/동적 Kill→다음 정리의 전체 연결.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualDynamicSubmenuExpandAndDeferredClose(OriginalEdition edition)
    {
        var s = new MenuInputScene(edition); s.View.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 0x880, 0, 91); s.View.Attach(); uint child = 0; s.Request = (t, e, serial, parameter) => { Assert.Equal((40U, s.State.RequestEvent, s.Ui.Menu.Node.Serial, 91U), (t, e, serial, parameter)); child = s.Ui.Widgets.BeginHorizontal(); s.Ui.Control.Controls.AddText("A"u8); return s.Ui.Menu.Menu.TakeCurrent(); }; Assert.Equal(40U, s.Click()); var n = (MenuGumpNode)s.Ui.Scene.Nodes[child]; Assert.Equal(n.Id, s.Ui.Menu.Node.Items[0].ChildId); Assert.NotEqual(0U, s.Ui.Menu.Node.Items[0].Flags & 0x20); Assert.Equal((40U, 1U, 0f), (n.Parent, n.Expanding, n.Width)); Assert.Contains(child, s.Ui.Scene.LifeState.Idle); s.Frame.Idle(); Assert.Equal(n.TargetWidth, n.Width); s.Frame.Idle(); Assert.Equal(0U, n.Expanding); Assert.Equal("openGump.wav", Assert.Single(s.Sounds).Name); s.Input.Close(40, true); Assert.Equal(0U, s.Ui.Menu.Node.Items[0].ChildId); Assert.Equal(0U, s.Ui.Menu.Node.Items[0].Flags & (0x20 | 8)); Assert.Equal(0, s.Ui.Menu.Node.Selection); Assert.NotEqual(0U, n.Flags & 0x10000); s.Ui.Scene.Lifetime.Cleanup(); Assert.Contains(child, s.Ui.Scene.Released); Assert.Equal(1U, s.Ui.Menu.State.Instances); Assert.Equal(2U, s.Ui.Scene.Create.Count);
    }
    /// <summary>정적 자식은 닫을 때 숨기며 계속 소유한다.동적 자식은 이미 숨겨져 있어도 Kill한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualStaticChildHideAndHiddenDynamicDelete(OriginalEdition edition)
    {
        var s = new MenuInputScene(edition); s.View.Begin(); uint child = s.Ui.Widgets.BeginVertical(); s.Ui.Control.Controls.AddText("A"u8); s.Ui.Menu.Menu.TakeCurrent(); s.Ui.Control.Controls.AddSubmenu("A"u8, child); s.View.Attach(); Assert.Equal(40U, s.Click()); var n = (MenuGumpNode)s.Ui.Scene.Nodes[child]; s.Input.Close(40, false); Assert.Equal(0U, n.Flags & 1); Assert.Equal(0U, n.Flags & 0x10000); Assert.Equal(n.Id, s.Ui.Menu.Node.Items[0].ChildId); Assert.Equal(2U, s.Ui.Menu.State.Instances); s.Ui.Menu.Node.Items[0].Flags |= 0x800; s.Input.Close(40, false); Assert.NotEqual(0U, n.Flags & 0x10000); s.Ui.Scene.Lifetime.Cleanup(); Assert.Equal(1U, s.Ui.Menu.State.Instances);
    }
    /// <summary>숨긴 메뉴 자식의 표시 손자도 가림으로 인정하되 메뉴 아닌 종류는 전체 하위 탐색을 생략한다.</summary>
    [Fact]
    public void HiddenMenuDescendantCoverageAndNonMenuBoundary()
    {
        var s = new MenuInputScene(); s.View.Begin(); uint child = s.Ui.Widgets.BeginVertical(); uint grandchild = s.Ui.Widgets.BeginVertical(); s.Ui.Control.Controls.AddText("A"u8); s.Ui.Menu.Menu.TakeCurrent(); s.Ui.Control.Controls.AddSubmenu("A"u8, grandchild); s.Ui.Menu.Menu.TakeCurrent(); s.Ui.Control.Controls.AddSubmenu("A"u8, child); s.View.Attach(); var below = (MenuGumpNode)s.Ui.Scene.Nodes[child]; var leaf = (MenuGumpNode)s.Ui.Scene.Nodes[grandchild]; leaf.Flags |= 1; below.X = below.Y = leaf.X = leaf.Y = 0; s.Motion(3, 4); Assert.Equal(-1, s.Ui.Menu.Node.Selection); below.Command = 99; s.Motion(3, 4); Assert.Equal(0, s.Ui.Menu.Node.Selection);
    }
    /// <summary>캡처 입력도 같은 메뉴 몸체를 사용하며 release/확장/다른 종류는 통지 전 반환한다.</summary>
    [Fact]
    public void ActualCaptureAndEarlyGuards()
    {
        var s = new MenuInputScene(); s.View.Begin(); s.Ui.Menu.Layout.Append(40, "A\0"u8.ToArray(), 2); s.View.Attach(); int notices = 0; var broadcast = s.Hooks.Broadcast; s.Hooks.Broadcast = (t, e, a, b) => { notices++; broadcast!(t, e, a, b); }; Assert.Equal(0U, s.Input.Handle(40, 0x80010000, 3, 4)); Assert.Equal(0U, s.Input.Handle(40, 12, 3, 4)); s.Ui.Menu.Node.Expanding = 1; Assert.Equal(0U, s.Input.Handle(40, 0x10000, 3, 4)); Assert.Equal(0, notices); s.Ui.Menu.Node.Expanding = 0; s.Ui.Scene.Input.Capture(40); Assert.Equal(40U, s.Click()); Assert.Equal(0U, s.Ui.Scene.LifeState.Focus); Assert.Equal(1, notices); s.Ui.Menu.Node.MenuFlags |= 0x4000000; s.Ui.Scene.Input.Capture(40); Assert.Equal(0U, s.Click(60, 100)); Assert.NotEqual(0U, s.Ui.Menu.Node.Flags & 0x10000);
    }
    /// <summary>행 변경 영역은 부모 visible만 검사하고 compact에서는 유효 항목 범위를 요구하지 않는다.</summary>
    [Fact]
    public void ActualRowRegionCompactAndParentContract()
    {
        var s = new MenuInputScene(); s.View.Begin(); s.Ui.Control.Controls.AddText("A"u8); s.View.Attach(); s.Ui.Menu.Node.Flags |= 0x10008; s.Input.RedrawItem(40, 0); Assert.Equal(new ScreenRect(2, 3, 35, 20), Assert.Single(s.Invalidated)); s.Invalidated.Clear(); s.Ui.Scene.Nodes[1].Flags &= ~1U; s.Input.RedrawItem(40, 99); Assert.Empty(s.Invalidated); s.Ui.Scene.Nodes[1].Flags |= 1; s.Ui.Menu.Node.MenuFlags |= 0x1000000; s.Input.RedrawItem(40, 99); Assert.Single(s.Invalidated); s.Ui.Scene.Nodes[1].Parent = 40; Assert.Throws<InvalidOperationException>(() => s.Input.RedrawItem(40, 0));
    }
    /// <summary>실제 루트 sticky/비dead 직계만 닫고 override/비팝업/dead 항목을 추가로 삭제하지 않는다.</summary>
    [Fact]
    public void ActualPopupRootTraversalAndDeadFilter()
    {
        var s = new MenuInputScene(); s.View.Begin(); s.Ui.Control.Controls.AddText("A"u8); s.View.Attach(); s.Ui.Menu.Node.Flags &= ~0x4000U;
        // 세 기본 자식을 같은 실제 루트에 부착한다.팝업/지연 삭제 비트만 입력으로 다르게 준다.
        for (uint token = 70; token <= 72; token++) { var n = new GumpCreateNode(); s.Ui.Scene.Nodes.Add(token, n); s.Ui.Scene.Factory.Construct(token, 0, 1, 1, 1); s.Ui.Scene.Tree.Attach(token, 0, 0, 1, 1); n.Flags |= token == 70 ? 0x4000U : token == 71 ? 0x14000U : 0; }
        s.Ui.Scene.Create.Collection.OverrideHead = 72; s.Input.ClosePopups(); Assert.Equal(1U, s.Ui.Scene.LifeState.KillCount); Assert.Equal(70U, s.Ui.Scene.LifeState.Kill[0]); Assert.Equal(0U, s.Ui.Menu.Node.Flags & 0x10000); Assert.Equal(0U, s.Ui.Scene.Nodes[72].Flags & 0x10000);
    }
    /// <summary>도달한 음향/등록/항목 오류만 거부하며 그 앞의 실제 명령/강조 쓰기는 남는다.</summary>
    [Fact]
    public void ReachedErrorsKeepEarlierCommandAndSelection()
    {
        var s = new MenuInputScene(); s.View.Begin(); s.Ui.Control.Controls.AddText("A"u8, 77); s.View.Attach(); s.Hooks.Sound = null; Assert.Throws<ArgumentException>(() => s.Click()); Assert.Single(s.Commands); Assert.Equal(0, s.Ui.Menu.Node.Selection); Assert.NotEqual(0U, s.Ui.Menu.Node.Items[0].Flags & 8); s.Ui.Menu.Node.Items[0].Flags = 0x20; s.Ui.Menu.Node.Items[0].ChildId = 1000; Assert.Throws<InvalidOperationException>(() => s.Input.Close(40, true)); Assert.Equal(0U, s.Ui.Menu.Node.Items[0].Flags & 8); s.Ui.Menu.Node.Items[0].Flags = 0; s.Ui.Menu.Node.ItemCount = 41; Assert.Throws<ArgumentOutOfRangeException>(() => s.Input.Close(40, true));
    }
}
