using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 판본별 메뉴 작성과 실제 스타일·제목·설정·자식 트리의 고정 계약 검사다.

/// <summary>cpppj의 정적 계약과 실제 조립 검사다.독립 PE 관찰이 없는 제어부를 L1로 세지 않는다.</summary>
public sealed class MenuGumpControlsTests
{
    /// <summary>실제 소유 조각과 command0/명시 인자·체크/비활성의 판본 차이 및 픽셀.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualTextCheckedAndDisabledOwnership(OriginalEdition edition)
    {
        var s = new MenuControlScene(edition); var node = s.Menu.Node; node.Items[0].Command = 91; node.Items[1].Command = 92; bool patch = edition == OriginalEdition.Patch1078;
        s.Controls.AddText("A\0ignored"u8, 0, 0x89abcdef); s.Controls.AddChecked(true, "A"u8, 0, 42); s.Controls.AddText("A%%"u8, 7, 8, enabled: false);
        Assert.Equal(3U, node.ItemCount); Assert.Equal((91U, 0x89abcdefU, 0x10480U), (node.Items[0].Command, node.Items[0].Parameter, node.Items[0].Flags)); Assert.Equal((patch ? 0U : 92U, 42U, patch ? 0x10485U : 0x10484U), (node.Items[1].Command, node.Items[1].Parameter, node.Items[1].Flags));
        var normal = Assert.Single(s.ItemText(node, 0)); var check = Assert.Single(s.ItemText(node, 1)); var disabled = Assert.Single(s.ItemText(node, 2)); Assert.Equal("A", StyleTextTestSupport.Text(normal)); Assert.Equal(patch ? 123U : 171U, normal.Context[2]); Assert.Equal(patch ? 131U : 171U, check.Context[2]); Assert.Equal(patch ? "A%" : "A%%", StyleTextTestSupport.Text(disabled)); Assert.Equal(81U, disabled.Context[2]); Assert.Equal(0U, disabled.Context[0]);
        s.Menu.Style.Draw(node.Items[0].Styled, new(2, 3)); Assert.Equal(patch ? 123 : 171, s.Title.Scene.Pixels.Span[194]); Assert.Equal(7, s.Title.Scene.Guarded[0]); Assert.Equal(7, s.Title.Scene.Guarded[^1]); Assert.Equal(patch ? new[] { "MenuColor", "MenuBGColor", "MenuCheckedColor", "MenuCheckedBGColor", "MenuColor", "MenuBGColor" } : [], s.Reads);
    }
    /// <summary>색 콜백 뒤 현재 controlled 칸과 현재 입력 바이트를 다시 읽어 같은 실제 다른 메뉴에 추가한다.</summary>
    [Fact]
    public void ColorQueriesUseCurrentMenuAndMutableInput()
    {
        var s = new MenuControlScene(); var other = s.AddMenu(41); s.Menu.State.Entries[2] = 41; byte[] source = "Z\0"u8.ToArray(); var keys = new List<string>(); s.Hooks.ReadInt = (string key, ref int value) => { keys.Add(key); if (key == "MenuColor") { s.Menu.State.Index = 2; source[0] = 65; value = 143; } else { Assert.Equal(2U, s.Menu.State.Index); value = 144; } }; s.Controls.AddText(source, 77, 88);
        Assert.Equal(new[] { "MenuColor", "MenuBGColor" }, keys); Assert.Equal((0U, 1U), (s.Menu.Node.ItemCount, other.ItemCount)); var text = Assert.Single(s.ItemText(other, 0)); Assert.Equal("A", StyleTextTestSupport.Text(text)); Assert.Equal((143U, 9U, 77U, 88U), (text.Context[2], text.Context[5], other.Items[0].Command, other.Items[0].Parameter)); Assert.Equal(144U, Assert.Single(s.Brackets, b => b.Command == "B.144").Context[5]);
    }
    /// <summary>패치 null 현재 칸은 include보다 먼저 오류다.CD는 원문 조회/색 경계 없이 조용히 생략한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void IncludeNullAndReachedBoundaryOrder(OriginalEdition edition)
    {
        var s = new MenuControlScene(edition); s.Menu.State.Entries[1] = 0; s.Hooks.ReadInt = null;
        if (edition == OriginalEdition.Patch1078) { Assert.Throws<InvalidOperationException>(() => s.Controls.AddText("%s"u8, include: false)); Assert.Throws<InvalidOperationException>(() => s.Controls.AddChecked(true, "%s"u8, 0, include: false)); Assert.Throws<InvalidOperationException>(() => s.Controls.AddSubmenu("%s"u8, 0)); }
        else { s.Controls.AddText("%s"u8, include: false); s.Controls.AddChecked(true, "%s"u8, 0, include: false); s.Controls.AddSubmenu("%s"u8, 0); }
        s.Menu.State.Entries[1] = 40; s.Controls.AddText("%s"u8, include: false); s.Controls.AddChecked(true, "%s"u8, 0, include: false); Assert.Empty(s.Reads); Assert.Equal(0U, s.Menu.Node.ItemCount); if (edition == OriginalEdition.Patch1078) Assert.Throws<ArgumentException>(() => s.Controls.AddText("A"u8));
    }
    /// <summary>disabled 경로의 %%만 축약하고 forceColor의 %s는 데이터로 보존한다.서식 오류는 실제 추가 전이다.</summary>
    [Fact]
    public void DisabledFormattingAndForcedColorAreDistinct()
    {
        var s = new MenuControlScene(); s.Controls.AddText("A%%"u8, enabled: false); s.Controls.AddText("A%s%%"u8, enabled: false, forceColor: true); Assert.Equal("A%", StyleTextTestSupport.Text(Assert.Single(s.ItemText(s.Menu.Node, 0)))); var forced = Assert.Single(s.ItemText(s.Menu.Node, 1)); Assert.Equal("A%s%%", StyleTextTestSupport.Text(forced)); Assert.Equal((123U, 9U, 2U), (forced.Context[2], forced.Context[5], forced.Context[0])); Assert.Equal(124U, Assert.Single(s.Brackets, b => b.Command == "B.124").Context[5]); Assert.Throws<ArgumentException>(() => s.Controls.AddText("%s"u8, enabled: false)); Assert.Equal(2U, s.Menu.Node.ItemCount);
        s.Controls.AddText([0xe9, 65, 0, 90]); Assert.Equal(new byte[] { 0xe9, 65, 0 }, Assert.Single(s.ItemText(s.Menu.Node, 2)).Text); Assert.Equal(8, s.Reads.Count);
    }
    /// <summary>실제 등록/부착으로 자식을 숨기고 판본별 styled 선택과 현재 childId를 같은 항목에 보관한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void SubmenuUsesActualTreeAndOriginalByteSearch(OriginalEdition edition)
    {
        var s = new MenuControlScene(edition); var child = s.AddMenu(41); s.Controls.AddSubmenu("A\0~"u8, 41, check: true); var item = s.Menu.Node.Items[0]; Assert.Equal((child.Id, 40U, 0U), (item.ChildId, child.Parent, child.Flags & 1)); Assert.Equal(0x10024U | (edition == OriginalEdition.Patch1078 ? 0x400U : 0), item.Flags);
        if (edition == OriginalEdition.Patch1078) { Assert.Equal((141U, 9U), (Assert.Single(s.ItemText(s.Menu.Node, 0)).Context[2], Assert.Single(s.ItemText(s.Menu.Node, 0)).Context[5])); Assert.Equal(142U, Assert.Single(s.Brackets, b => b.Command == "B.142").Context[5]); } else { Assert.Equal(0U, item.Styled); Assert.Equal(65, item.Label[0]); }
        var second = s.AddMenu(42); s.Controls.AddSubmenu("~rA"u8, 42); Assert.NotEqual(0U, s.Menu.Node.Items[1].Styled); Assert.Equal(second.Id, s.Menu.Node.Items[1].ChildId);
    }
    /// <summary>fancy 제목/머리는 실제 다중 줄 버퍼에만 추가하며 메뉴 Reflow를 유발하지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void FancyTitleUsesSharedActualBufferAndPixels(OriginalEdition edition)
    {
        var s = new MenuControlScene(edition); var node = s.Menu.Node; node.MenuFlags |= 0x800000; node.Title = 30; node.TargetWidth = 91; node.BaseWidth = node.Width = 92; int notices = 0; s.Title.Buffer.State.OnAppend = _ => notices++; s.Controls.AddTitle("A\n"u8); s.Controls.AddHeader("A\n"u8);
        Assert.Equal((0U, 91f, 92f), (node.ItemCount, node.TargetWidth, node.Width)); Assert.Equal(2, s.Title.Buffer.State.LineCount); uint first = Assert.Single(StyleTextTestSupport.Chain(s.Menu.Fragments, s.Title.Buffer.State.HardLines[0])); uint second = Assert.Single(StyleTextTestSupport.Chain(s.Menu.Fragments, s.Title.Buffer.State.HardLines[1])); var title = s.Menu.Fragments.Fragments[first]; var header = s.Menu.Fragments.Fragments[second]; Assert.Equal((3U, 0U, 9U, 9U), (title.Context[1], header.Context[1], title.Metadata, header.Metadata)); Assert.Equal(edition == OriginalEdition.Patch1078 ? (151U, 161U) : (171U, 171U), (title.Context[2], header.Context[2])); Assert.True(notices >= 2); s.Title.Text.RequestScroll(30, 0); s.Title.Scene.Draw.Draw(30); Assert.Contains(edition == OriginalEdition.Patch1078 ? (byte)151 : (byte)171, s.Title.Scene.Pixels.ToArray()); Assert.Equal(7, s.Title.Scene.Guarded[0]); Assert.Equal(7, s.Title.Scene.Guarded[^1]); Assert.DoesNotContain("MenuColor", s.Reads);
    }
    /// <summary>일반 제목은 disabled 실제 항목이며 판본별 색/슬롯과 머리의 무인자 서식 결과를 보존한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void OrdinaryHeadingUsesActualStyledItem(OriginalEdition edition)
    {
        var s = new MenuControlScene(edition); s.Menu.Node.Items[0].Command = 99; s.Controls.AddTitle("A"u8); s.Controls.AddHeader("A%%"u8); var title = Assert.Single(s.ItemText(s.Menu.Node, 0)); var header = Assert.Single(s.ItemText(s.Menu.Node, 1)); bool patch = edition == OriginalEdition.Patch1078; Assert.Equal(2U, s.Menu.Node.ItemCount); Assert.Equal((99U, 0U, 0x10680U), (s.Menu.Node.Items[0].Command, s.Menu.Node.Items[0].Parameter, s.Menu.Node.Items[0].Flags)); Assert.Equal((3U, patch ? 151U : 81U), (title.Context[1], title.Context[2])); Assert.Equal((0U, patch ? 161U : 81U), (header.Context[1], header.Context[2])); Assert.Equal(patch ? "A%%" : "A%", StyleTextTestSupport.Text(header)); Assert.Equal(patch ? new[] { "MenuTitleColor", "MenuTitleBGColor", "MenuColor", "MenuBGColor", "MenuHeaderColor", "MenuHeaderBGColor", "MenuColor", "MenuBGColor" } : [], s.Reads);
    }
    /// <summary>CD는 숨김/dead를 거르지 않고 실제 등록 ID의 모든 자식 기본 문맥만 복사한다.저장 스타일 문맥은 유지한다.</summary>
    [Fact]
    public void CdContextRecursionUsesActualRegistrationsAndPreservesStyledOwners()
    {
        var s = new MenuControlScene(OriginalEdition.Cd1072); var child = s.AddMenu(41); var grandchild = s.AddMenu(42); s.Controls.AddSubmenu("A"u8, 41); s.Menu.State.Entries[1] = 41; s.Controls.AddText("A"u8); s.Controls.AddSubmenu("A"u8, 42); s.Menu.State.Entries[1] = 40; child.Flags = grandchild.Flags = 0x10000; uint head = child.Items[0].Styled; var original = (uint[])s.Menu.Fragments.Fragments[head].Context.Clone(); uint[] context = [1, 2, 3, 4, 5, 6, 7, 8]; var parent = (uint[])s.Menu.Node.TextContext.Clone(); s.Controls.CopyChildTextContextCd(40, context); Assert.Equal(parent, s.Menu.Node.TextContext); Assert.Equal(context, child.TextContext); Assert.Equal(context, grandchild.TextContext); Assert.Equal(original, s.Menu.Fragments.Fragments[head].Context); Assert.Equal(head, child.Items[0].Styled);
    }
    /// <summary>CD 문맥 입력은 값 복사가 아니므로 앞선 실제 기본 조회가 바꾼 현재 입력을 이후 자식에 사용한다.</summary>
    [Fact]
    public void CdContextPreservesInputAliasAcrossChildren()
    {
        var s = new MenuControlScene(OriginalEdition.Cd1072); var child = s.AddMenu(41); var second = s.AddMenu(42); s.Controls.AddSubmenu("A"u8, 41); s.Controls.AddSubmenu("A"u8, 42); child.TextContext[0] = 11; var basic = s.Hooks.Base; s.Hooks.Base = t => { if (t == 42) child.TextContext[0] = 99; return basic!(t); }; s.Controls.CopyChildTextContextCd(40, child.TextContext); Assert.Equal(99U, child.TextContext[0]); Assert.Equal(child.TextContext, second.TextContext);
    }
    /// <summary>CD 첫 visible 자식에서 중단해 뒤 항목 강조를 남긴다.비영 등록/표시 비트만 검사한다.</summary>
    [Fact]
    public void CdSelectionStopsAfterFirstVisibleRegisteredChild()
    {
        var s = new MenuControlScene(OriginalEdition.Cd1072); var hidden = s.AddMenu(41); var visible = s.AddMenu(42); s.Controls.AddSubmenu("A"u8, 41); s.Controls.AddSubmenu("A"u8, 42); s.Controls.AddText("A"u8); hidden.Flags = 0x10000; visible.Flags = 0x10001;
        // 세 실제 항목에 현재 강조를 넣고 원본의 부분 갱신만 관측한다.
        foreach (var item in s.Menu.Node.Items.Take(3)) item.Flags |= 8;
        Assert.True(s.Controls.ClearSelectionUntilVisibleChildCd(40)); Assert.Equal(new uint[] { 0, 0, 8 }, s.Menu.Node.Items.Take(3).Select(i => i.Flags & 8)); s.Title.Scene.Create.Entries[visible.Id] = 0; Assert.False(s.Controls.ClearSelectionUntilVisibleChildCd(40)); Assert.All(s.Menu.Node.Items.Take(3), item => Assert.Equal(0U, item.Flags & 8));
    }
    /// <summary>실제 도달한 순환/범위 오류에서도 앞선 문맥/선택 쓰기는 남는다.패치는 CD helper 변경 전에 거부한다.</summary>
    [Fact]
    public void InvalidCdPathsKeepEarlierWritesAndEditionGuard()
    {
        var s = new MenuControlScene(OriginalEdition.Cd1072); var child = s.AddMenu(41); s.Controls.AddSubmenu("A"u8, 41); s.Menu.State.Entries[1] = 41; s.Controls.AddSubmenu("A"u8, 40); s.Menu.State.Entries[1] = 40; uint[] context = [8, 7, 6, 5, 4, 3, 2, 1]; Assert.Throws<InvalidOperationException>(() => s.Controls.CopyChildTextContextCd(40, context)); Assert.Equal(context, s.Menu.Node.TextContext); Assert.Equal(context, child.TextContext); Assert.Throws<ArgumentException>(() => s.Controls.CopyChildTextContextCd(40, [1])); s.Menu.Node.Items[0].ChildId = 1000; s.Menu.Node.Items[0].Flags |= 8; Assert.Throws<ArgumentOutOfRangeException>(() => s.Controls.ClearSelectionUntilVisibleChildCd(40)); Assert.Equal(0U, s.Menu.Node.Items[0].Flags & 8); var patch = new MenuControlScene(); Assert.Throws<ArgumentException>(() => patch.Controls.CopyChildTextContextCd(40, context)); Assert.Throws<ArgumentException>(() => patch.Controls.ClearSelectionUntilVisibleChildCd(40));
    }
    /// <summary>설정 누락은0 초기값,실제 층/치환과 CRT 감김은 signed DWORD 색에 적용한다.메뉴 존재는 실제 생성 개수만 쓴다.</summary>
    [Fact]
    public void ConfigurationAndActualInstanceCount()
    {
        var s = new MenuControlScene(); s.Config.Pop(); s.Config.Push(new("MenuColor={base}\nbase=4294967295\nMenuBGColor=  +124rest\n")); s.Controls.AddText("A"u8); var item = Assert.Single(s.ItemText(s.Menu.Node, 0)); Assert.Equal((uint.MaxValue, 9U), (item.Context[2], item.Context[5])); Assert.Equal(124U, Assert.Single(s.Brackets, b => b.Command == "B.124").Context[5]); s.Config.Pop(); s.Controls.AddText("A"u8); var missing = Assert.Single(s.ItemText(s.Menu.Node, 1)); Assert.Equal((0U, 9U), (missing.Context[2], missing.Context[5])); Assert.Equal(0U, Assert.Single(s.Brackets, b => b.Command == "B.0").Context[5]); Assert.True(s.Controls.NoMenus()); s.Menu.State.Instances = 1; s.Menu.State.Entries[1] = 0; s.Menu.Node.Flags = 0x10000; Assert.False(s.Controls.NoMenus()); int current = 91; s.Hooks.ReadInt!("missing", ref current); Assert.Equal(91, current);
    }
}
