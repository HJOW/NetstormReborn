using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 메뉴 생성/장식·제목/controlled 표/부착/지연 수명 통합 검사다.

/// <summary>cpppj 정적 계약과 같은 실제 처리기 조립이다.독립 PE 관찰/L1은 아니다.</summary>
public sealed class MenuGumpWidgetsTests
{
    /// <summary>고정 원본 BYTE 입력의 문자열 생성 항목.</summary>
    private static MenuGumpConstructionEntry Text(uint flags = 0x10400, uint command = 0, uint parameter = 0, uint condition = 1) => new(new MenuGumpConstructionText("A"u8), flags, command, parameter, condition);
    /// <summary>조건 false의 선행 필드 쓰기/실제 자식 연결/전체 레이아웃과 미작성 배열 보존.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualConstructionConditionalFieldsChildAndStableItems(OriginalEdition edition)
    {
        var s = new MenuWidgetScene(edition); var n = s.Menu.Node; var item = n.Items[39]; item.Command = 99; n.Offsets[39] = 88; n.Heights[39] = 77; var child = new GumpCreateNode(); s.Scene.Nodes.Add(41, child); s.Scene.Factory.Construct(41, 0, 1, 1, 1); s.State.Alignment = -7;
        MenuGumpConstructionEntry[] entries = [Text(0x10581, 91, 92, 0), Text(), new(new MenuGumpConstructionChild(41), 0x20000), Text(0x10000)]; Assert.Equal(40U, s.Widgets.Construct(40, entries, 0x280000)); Assert.Same(item, n.Items[39]); Assert.Equal((edition == OriginalEdition.Patch1078 ? 0x50c6f0U : 0x501fa0U, unchecked((uint)-7), 0xffffffc4U, -1), (n.Vtable, n.Command, n.Depth, n.Selection)); Assert.Equal((2U, 1U, 37f, 32f), (n.ItemCount, s.Menu.State.Instances, n.TargetWidth, n.TargetHeight)); Assert.Equal((91U, 92U, child.Id, 0x10420U), (n.Items[0].Command, n.Items[0].Parameter, n.Items[0].ChildId, n.Items[0].Flags)); Assert.Equal((40U, 0U), (child.Parent, child.Flags & 1)); Assert.Equal((0f, 16f, 88f, 0f, 0U), (n.Offsets[0], n.Offsets[1], n.Offsets[39], n.Heights[39], item.Command)); Assert.Equal(new uint[] { 2, 0, 171, 0, 0, 9, 1, 0 }, n.TextContext); Assert.Equal("A", StyleTextTestSupport.Text(s.Menu.Fragments.Fragments[n.Items[0].Styled]));
    }
    /// <summary>방향 없는 생성은 미작성 목표/offset을 유지하고 헤드/항목은 실제 고정 저장소를0으로 만든다.</summary>
    [Fact]
    public void ConstructorPreservesUnwrittenTargetsAndFullItemReset()
    {
        var s = new MenuWidgetScene(); s.Menu.Node.TargetWidth = 81; s.Menu.Node.TargetHeight = 82; s.Menu.Node.Offsets[7] = 83; s.Menu.Node.Items[39].Label[39] = 84; s.Menu.Node.Items[39].Styled = 85; s.Widgets.Construct(40, [], 0); Assert.Equal((81f, 82f, 83f, 0f), (s.Menu.Node.TargetWidth, s.Menu.Node.TargetHeight, s.Menu.Node.Offsets[7], s.Menu.Node.Width)); Assert.Equal(0U, s.Menu.Node.Items[39].Styled); Assert.All(s.Menu.Node.Items[39].Label, b => Assert.Equal(0, b)); Assert.Empty(s.Menu.Released);
    }
    /// <summary>가로/세로 controlled 시작의 실제 표·idle와 추가 배경 플래그.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, false)]
    [InlineData(OriginalEdition.Cd1072, false)]
    [InlineData(OriginalEdition.Patch1078, true)]
    [InlineData(OriginalEdition.Cd1072, true)]
    public void ControlledBeginUsesActualIdleAndCurrentStack(OriginalEdition edition, bool horizontal)
    {
        var s = new MenuWidgetScene(edition); uint token = horizontal ? s.Widgets.BeginHorizontal() : s.Widgets.BeginVertical(0x8000); Assert.Equal(40U, token); Assert.Equal((1U, 40U, 1U), (s.Menu.State.Index, s.Menu.State.Entries[1], s.Menu.State.Instances)); Assert.Equal(new[] { 0xb90 }, s.Allocations); Assert.Contains(40U, s.Scene.LifeState.Idle); Assert.Equal(horizontal ? 0x140000U : 0x280000U, s.Menu.Node.MenuFlags); Assert.Equal(horizontal ? 0U : 0x8000U, s.Menu.Node.BackgroundFlags & 0x8000); s.Control.Controls.AddText("A"u8, 77, 88); s.Menu.Menu.AttachCurrent(2, 3, 1); Assert.Equal((0U, 1U, 2f, 3f), (s.Menu.State.Index, s.Menu.Node.Parent, s.Menu.Node.X, s.Menu.Node.Y)); Assert.True((s.Menu.Node.Flags & 0x4000) != 0);
    }
    /// <summary>할당 콜백이 index를 바꾸면 생성 뒤 현재 칸을 쓴다.증가/예약/한도 오류 전 쓰기도 유지한다.</summary>
    [Fact]
    public void BeginCallbacksAndPartialFailures()
    {
        var s = new MenuWidgetScene(); var allocate = s.Hooks.Allocate; s.Hooks.Allocate = bytes => { s.Menu.State.Index = 2; return allocate!(bytes); }; s.Widgets.BeginVertical(); Assert.Equal((2U, 0U, 40U), (s.Menu.State.Index, s.Menu.State.Entries[1], s.Menu.State.Entries[2])); var over = new MenuWidgetScene(); over.Menu.State.Index = 19; Assert.Throws<ArgumentOutOfRangeException>(() => over.Widgets.BeginHorizontal()); Assert.Equal(20U, over.Menu.State.Index); Assert.Empty(over.Allocations); var reserved = new MenuWidgetScene(); reserved.Menu.State.Index = uint.MaxValue; Assert.Throws<ArgumentOutOfRangeException>(() => reserved.Widgets.BeginFancy()); Assert.Equal(0U, reserved.Menu.State.Index); var missing = new MenuWidgetScene(); missing.Hooks.Allocate = _ => 0; Assert.Equal(0U, missing.Widgets.BeginFancy()); Assert.Equal((1U, 0U), (missing.Menu.State.Index, missing.Menu.State.Instances)); var noIdle = new MenuWidgetScene(); noIdle.Hooks.Allocate = _ => 0; Assert.Throws<InvalidOperationException>(() => noIdle.Widgets.BeginHorizontal()); Assert.Equal(1U, noIdle.Menu.State.Index);
    }
    /// <summary>fancy 생성의 실제 장식/제목·버퍼·Slider·overlay와 RequestScroll/FitContent를 같은 트리에 연결한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void FancyUsesActualChildrenAndTitleLayout(OriginalEdition edition)
    {
        var s = new MenuWidgetScene(edition); Assert.Equal(40U, s.Widgets.BeginFancy()); Assert.Equal(new[] { 0xb90, 0xc4, 0xd8 }, s.Allocations); Assert.Equal((50U, 30U, 40U, 50U, 1U, uint.MaxValue), (s.Menu.Node.Fancy, s.Menu.Node.Title, s.Decoration.Node.Parent, s.Title.Node.Parent, s.Decoration.Node.SecondaryDepth, s.Title.Node.SecondaryDepth)); Assert.NotEqual(0U, s.Title.Node.Buffer); Assert.Equal(31U, s.Title.Node.Slider); Assert.Contains(30U, s.Scene.LifeState.Overlay); Assert.DoesNotContain(40U, s.Scene.LifeState.Idle); Assert.Equal((1f, 0f, 33f, 48f, -16f, -32f), (s.Menu.Node.TargetWidth, s.Menu.Node.TargetHeight, s.Decoration.Node.Width, s.Decoration.Node.Height, s.Decoration.Node.X, s.Decoration.Node.Y));
        s.Control.Controls.AddTitle("A\n"u8); Assert.Equal(0U, s.Menu.Node.ItemCount); s.Menu.Layout.Reflow(40); Assert.Equal(1, s.Title.Buffer.State.LineCount); Assert.Equal(edition == OriginalEdition.Patch1078 ? (4f, 21f, 4f, 36f, 69f, -53f) : (3f, 20f, 3f, 35f, 68f, -52f), (s.Title.Node.BaseWidth, s.Title.Node.BaseHeight, s.Menu.Node.TargetWidth, s.Decoration.Node.Width, s.Decoration.Node.Height, s.Decoration.Node.Y)); s.Menu.Menu.AttachCurrent(0, 0, 1); Assert.Equal((16f, edition == OriginalEdition.Patch1078 ? 53f : 52f), (s.Menu.Node.X, s.Menu.Node.Y)); Assert.Equal(new ScreenPoint(0, 0), s.Scene.Collection.Position(50)); s.Scene.Draw.Draw(50); s.Title.Text.RequestScroll(30, 0); s.Scene.Draw.Draw(30); Assert.Contains(edition == OriginalEdition.Patch1078 ? (byte)151 : (byte)171, s.Scene.Pixels.ToArray()); Assert.Equal(7, s.Scene.Guarded[0]); Assert.Equal(7, s.Scene.Guarded[^1]);
    }
    /// <summary>현재 부모 절대 위치로 전체 화면 보정을 한 뒤 mode1/기본 비트의 생략과 parent0 재사용을 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualAttachClampAndBypass(OriginalEdition edition)
    {
        var s = new MenuWidgetScene(edition); s.Widgets.Construct(40, [], 0x280000); s.Menu.Node.Width = s.Menu.Node.BaseWidth = 10; s.Menu.Node.Height = s.Menu.Node.BaseHeight = 20; var parent = new GumpCreateNode(); s.Scene.Nodes.Add(41, parent); s.Scene.Factory.Construct(41, 0, 2, 2, 1); s.Scene.Tree.Attach(41, 11, 13, 1, 1); s.Widgets.Attach(40, 100, 200, 41, 0); Assert.Equal((43f, 95f), (s.Menu.Node.X, s.Menu.Node.Y)); s.Scene.Tree.Push(40); s.Widgets.Attach(40, -21, -22, 0, 1); Assert.Equal((41U, -21f, -22f), (s.Menu.Node.Parent, s.Menu.Node.X, s.Menu.Node.Y)); s.Scene.Tree.Push(40); s.Menu.Node.Flags |= 0x20000; s.Widgets.Attach(40, 101, 102, 0, 0); Assert.Equal((101f, 102f), (s.Menu.Node.X, s.Menu.Node.Y));
    }
    /// <summary>39개까지 유효하고40번째 쓰기 후 실패한다.조건 false도 명령/인자 쓰기 후 한도를 검사한다.</summary>
    [Fact]
    public void ConstructorLimitKeepsPartialOriginalWrites()
    {
        var s = new MenuWidgetScene(); MenuGumpConstructionEntry[] entries = Enumerable.Range(0, 40).Select(_ => Text()).ToArray(); Assert.Throws<ArgumentException>(() => s.Widgets.Construct(40, entries, 0x280000)); Assert.Equal((40U, 1U, 0U), (s.Menu.Node.ItemCount, s.Menu.State.Instances, s.Menu.Node.MenuFlags)); Assert.NotEqual(0U, s.Menu.Node.Items[39].Styled); Assert.Equal(0U, s.Menu.Node.Flags & 0x100); var invalid = new MenuWidgetScene(); Assert.Throws<ArgumentException>(() => invalid.Widgets.Construct(40, [new(new MenuGumpConstructionChild(1), 0x30081, 77, 88)], 0)); Assert.Equal((77U, 88U, 0U), (invalid.Menu.Node.Items[0].Command, invalid.Menu.Node.Items[0].Parameter, invalid.Menu.Node.ItemCount));
    }
    /// <summary>선택 콜백 뒤 현재 소유 필드40개를 역순으로 해제하며 개수/플래그에 따라 줄이지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualReverseOwnersAndSelectionCallback(OriginalEdition edition)
    {
        var s = new MenuWidgetScene(edition); s.Widgets.BeginVertical(); s.Control.Controls.AddText("A"u8); uint first = s.Menu.Node.Items[0].Styled; s.Menu.Node.Items[39].Flags = 0x400; s.Menu.Layout.SetText(40, 39, "A\0"u8.ToArray(), s.Menu.Node.TextContext); uint last = s.Menu.Node.Items[39].Styled; s.Menu.Node.Selection = -7; s.Menu.Node.SelectionCallback = 91; s.OnSelection = (c, index) => { Assert.Equal((91U, -7, 0U), (c, index, s.Menu.State.Instances)); s.Menu.Node.Items[38].Flags = 0x400; s.Menu.Layout.SetText(40, 38, "A\0"u8.ToArray(), s.Menu.Node.TextContext); }; s.Menu.Menu.AttachCurrent(2, 3, 1); s.Scene.Lifetime.Kill(40); s.Scene.Lifetime.Cleanup(); Assert.Equal("selection:91,-7", s.Order[0]); Assert.Equal("style:" + last, s.Order[1]); Assert.Equal("style:" + first, s.Order[^2]); Assert.Equal("node:40", s.Order[^1]); Assert.All(s.Menu.Node.Items, i => Assert.Equal(0U, i.Styled)); Assert.Equal(1U, s.Scene.Create.Count); Assert.Equal(0U, s.Menu.State.Instances); Assert.DoesNotContain(40U, s.Scene.LifeState.Idle);
    }
    /// <summary>fancy 삭제는 실제 제목/버퍼/Slider/장식이 메뉴 선택 통지보다 먼저 소멸한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualFancyChildFirstLifetime(OriginalEdition edition)
    {
        var s = new MenuWidgetScene(edition); s.Widgets.BeginFancy(); s.Control.Controls.AddTitle("A\n"u8); s.Menu.Node.SelectionCallback = 77; s.OnSelection = (_, _) => { Assert.Equal(0U, s.Title.Node.Buffer); Assert.Contains(30U, s.Scene.Released); Assert.Contains(31U, s.Scene.Released); Assert.Contains(50U, s.Scene.Released); }; s.Menu.Menu.AttachCurrent(2, 3, 1); s.Scene.Lifetime.Kill(40); s.Scene.Lifetime.Cleanup(); Assert.Equal(1U, s.Scene.Create.Count); Assert.Equal(0U, s.Menu.State.Instances); Assert.DoesNotContain(30U, s.Scene.LifeState.Overlay); Assert.Equal("node:40", s.Order[^1]); Assert.True(s.Order.IndexOf("node:50") < s.Order.IndexOf("selection:77,-1")); Assert.Contains("buffer:500", s.Title.Released);
    }
}
