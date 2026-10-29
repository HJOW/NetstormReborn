using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 원본 메뉴 본체/레이아웃과 실제 스타일 소유·글꼴·픽셀·idle의 연결을 검사한다.

/// <summary>원본568행 대조와 별개인 실제 조립/호스트 유효 범위 검사.</summary>
public sealed class MenuGumpLayoutTests
{
    /// <summary>원본 두 판본의 일반/스타일 측정·세로 offset/가로 공통 높이와 실제 소유 반납/픽셀.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualStyledOwnershipFontsPixelsAndReflow(OriginalEdition edition)
    {
        var s = new MenuLayoutScene(edition); s.Layout.Append(40, "A\0"u8.ToArray(), 0); s.Layout.Append(40, "A\0"u8.ToArray(), 0x600); uint old = s.Node.Items[1].Styled; Assert.NotEqual(0U, old); Assert.Equal((0U, 81U), (s.Fragments.Fragments[old].Context[1], s.Fragments.Fragments[old].Context[2])); Assert.Equal((32f, 32f, 16f, 16f), (s.Node.TargetWidth, s.Node.TargetHeight, s.Node.Heights[0], s.Node.Heights[1])); Assert.Equal((0f, 16f), (s.Node.Offsets[0], s.Node.Offsets[1])); s.Node.TextContext[1] = 3; s.Layout.SetText(40, 1, "A~rA\0"u8.ToArray(), s.Node.TextContext); Assert.Equal(new[] { old }, s.Released); s.Style.Draw(s.Node.Items[1].Styled, new(2, 3)); Assert.Equal(81, s.Scene.Pixels.Span[194]); Assert.Equal(171, s.Scene.Pixels.Span[196]); Assert.Equal(7, s.Scene.Guarded[0]); Assert.Equal(7, s.Scene.Guarded[^1]);
        s.Node.TextContext[1] = 0; s.Node.MenuFlags = 0x40000; s.Layout.Reflow(40); Assert.Equal((24f, 16f, 12f), (s.Node.TargetWidth, s.Node.TargetHeight, s.Node.ItemWidth)); Assert.Equal((0f, 0f), (s.Node.Offsets[0], s.Node.Offsets[1])); Assert.Equal((12f, 16f), s.Menu.ItemPosition(40, 1));
    }
    /// <summary>40바이트의 비NUL 라벨 뒤 childId 바이트도 실제 일반 측정에 포함된다.</summary>
    [Fact]
    public void PlainFortyBytesCanReadFollowingOriginalFields()
    {
        var s = new MenuLayoutScene(); var item = s.Node.Items[0]; item.ChildId = 65; s.Layout.SetText(40, 0, Enumerable.Repeat((byte)65, 40).ToArray(), s.Node.TextContext); Assert.All(item.Label, b => Assert.Equal(65, b)); Assert.Equal((82, 14), s.Layout.MeasureItem(40, 0, s.Node.TextContext)); byte[] raw = MenuGumpLayout.MenuGumpItemBytes(item); Assert.Equal(60, raw.Length); Assert.Equal(new byte[] { 65, 0, 0, 0 }, raw.AsSpan(40, 4).ToArray());
    }
    /// <summary>스타일Clear는 길이 검사보다 먼저 소유를 반납하며 disabled 접두도256바이트 한도에 포함한다.</summary>
    [Fact]
    public void StyledBufferLimitAndFailureKeepActualOwnerWrites()
    {
        var s = new MenuLayoutScene(); s.Layout.Append(40, "A\0"u8.ToArray(), 0x600); uint old = s.Node.Items[0].Styled; byte[] tooLong = [.. Enumerable.Repeat((byte)65, 252), 0]; Assert.Throws<ArgumentException>(() => s.Layout.SetText(40, 0, tooLong, s.Node.TextContext)); Assert.Equal(0U, s.Node.Items[0].Styled); Assert.Equal(new[] { old }, s.Released); byte[] accepted = [.. Enumerable.Repeat((byte)65, 251), 0]; s.Layout.SetText(40, 0, accepted, s.Node.TextContext); Assert.Equal((502, 14), s.Layout.MeasureItem(40, 0, s.Node.TextContext)); s.Hooks.ParseStyle = null; Assert.Throws<ArgumentException>(() => s.Layout.SetText(40, 0, "A\0"u8.ToArray(), s.Node.TextContext)); Assert.Equal(0U, s.Node.Items[0].Styled);
    }
    /// <summary>실제 Clear 뒤의 현재 disabled/입력 바이트를 쓰되 문맥은Clear 전 복사본을 쓴다.</summary>
    [Fact]
    public void ClearCallbacksUseCurrentSourceAndSavedContext()
    {
        var s = new MenuLayoutScene(); s.Layout.Append(40, "A\0"u8.ToArray(), 0x400); byte[] input = "Z\0"u8.ToArray(); var clear = s.Hooks.ClearStyle; s.Node.TextContext[2] = 171; s.Node.TextContext[1] = 5; s.Hooks.ClearStyle = (t, i) => { clear!(t, i); input[0] = 65; s.Node.Items[i].Flags |= 0x200; s.Node.TextContext[1] = 6; s.Node.TextContext[2] = 7; }; s.Layout.SetText(40, 0, input, s.Node.TextContext); uint token = s.Node.Items[0].Styled; Assert.Equal("A", StyleTextTestSupport.Text(s.Fragments.Fragments[token])); Assert.Equal((0U, 81U), (s.Fragments.Fragments[token].Context[1], s.Fragments.Fragments[token].Context[2])); Assert.Equal((6U, 7U), (s.Node.TextContext[1], s.Node.TextContext[2]));
    }
    /// <summary>실제 기본 트리/수집으로 controlled 메뉴를 부착하고 관리 칸만 제거한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ControlledAttachUsesActualTree(OriginalEdition edition)
    {
        var s = new MenuLayoutScene(edition); s.Layout.Append(40, "A\0"u8.ToArray(), 0); s.Menu.AttachCurrent(2, 3, 1); Assert.Equal((1U, 2f, 3f, 0U), (s.Node.Parent, s.Node.X, s.Node.Y, s.State.Index)); Assert.Equal(0U, s.State.Entries[1]); Assert.Equal(2U, s.Scene.Create.Count); var entries = new RenderCollectionEntry[4]; int count = s.Scene.Collection.Collect(new(0, 0, 64, 128), entries, 4); Assert.Equal(1, count); Assert.Equal(40U, entries[0].Token);
    }
    /// <summary>실제 idle 등록/프레임 분배로 확장한 뒤 완료 호출에서도 등록은 남긴다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ExpansionUsesActualFrameAndLifetime(OriginalEdition edition)
    {
        var s = new MenuLayoutScene(edition); s.Layout.Append(40, "A\0"u8.ToArray(), 0); s.Menu.StartExpand(40, 0x80000); var frame = new GumpFrame(s.Scene.Create, s.Scene.LifeState, s.Scene.FrameHooks); Assert.Contains(40U, s.Scene.LifeState.Idle); Assert.Equal(0f, s.Node.Height); frame.Idle(); Assert.Equal((16f, 1U), (s.Node.Height, s.Node.Expanding)); frame.Idle(); Assert.Equal(0U, s.Node.Expanding); Assert.Contains(40U, s.Scene.LifeState.Idle); frame.RedrawNode(40); Assert.Contains("base:40,False", s.Calls);
    }
    /// <summary>한도 조기 반환/부분 필드/손상 범위와 signed 음수 개수의 반복 생략.</summary>
    [Fact]
    public void LimitsPartialFieldsAndSignedCount()
    {
        var s = new MenuLayoutScene(); s.Layout.Append(40, null, 0x10); Assert.Equal(0U, s.Node.ItemCount); var item = s.Node.Items[0]; item.Command = 91; item.Parameter = 92; item.ChildId = 93; s.Layout.Append(40, "A\0"u8.ToArray(), 0); Assert.Equal((91U, 92U, 93U), (item.Command, item.Parameter, item.ChildId)); s.Node.ItemCount = 40; s.Layout.Append(40, null, uint.MaxValue); Assert.Equal(40U, s.Node.ItemCount); Assert.Throws<ArgumentOutOfRangeException>(() => s.Layout.SetText(40, 40, "A\0"u8.ToArray(), s.Node.TextContext)); Assert.Throws<ArgumentException>(() => s.Layout.SetText(40, 0, null, s.Node.TextContext)); s.Node.ItemCount = uint.MaxValue; s.Node.Heights[7] = 99; s.Node.Offsets[7] = 88; s.Layout.Reflow(40); Assert.Equal((0f, 0f, 99f, 88f), (s.Node.TargetWidth, s.Node.TargetHeight, s.Node.Heights[7], s.Node.Offsets[7])); s.State.Index = 20; Assert.Throws<ArgumentOutOfRangeException>(() => s.Menu.Current()); s.State.Index = 0; Assert.Throws<ArgumentOutOfRangeException>(() => s.Menu.TakeCurrent());
    }
}
