using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 장식의 실제 생성·배경/원색 모서리·현재 필드·변경 영역·수명 계약 검사다.

/// <summary>cpppj 정적 계약과 실제 RLE/픽셀 조립 검사.새 독립 PE 대조나 L1은 아니다.</summary>
public sealed class GumpDecorationTests
{
    /// <summary>두 판본의 실제 생성·캐시·배경과 바깥 모서리 픽셀을 같은 표면에서 확인한다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualBackgroundAndFourUnmappedCorners(OriginalEdition edition)
    {
        var s = new DecorationScene(edition); s.State.Alignment = -7; s.Construct(); Assert.Equal((edition == OriginalEdition.Patch1078 ? 0x50ae7cU : 0x504928U, unchecked((uint)-7), 82U, 82U, 0U), (s.Node.Vtable, s.Node.Command, s.Node.CornerType, s.Node.CornerShape, s.Node.CornerFrame)); s.Scene.Draw.Draw(50);
        Assert.Equal(new[] { (82U, 0U, new ScreenPoint(5, 6)), (82U, 1U, new ScreenPoint(15, 6)), (82U, 2U, new ScreenPoint(15, 13)), (82U, 3U, new ScreenPoint(5, 13)) }, s.Corners); Assert.Equal(41, s.Scene.Pixels.Span[5 * 64 + 4]); Assert.Equal(42, s.Scene.Pixels.Span[5 * 64 + 15]); Assert.Equal(43, s.Scene.Pixels.Span[13 * 64 + 15]); Assert.Equal(44, s.Scene.Pixels.Span[13 * 64 + 4]); Assert.Equal(7, s.Scene.Guarded[0]); Assert.Equal(7, s.Scene.Guarded[^1]); Assert.Equal(new ScreenRect(0, 0, 64, 128), s.Scene.Clip.Clip);
    }
    /// <summary>앞선 실제 배경/모서리 콜백 뒤 현재 크기/캐시를 다시 읽되 위치는 배경 뒤 한 번 저장한다.</summary>
    [Fact]
    public void CurrentCornerFieldsAndStoredPosition()
    {
        var s = new DecorationScene(); s.Construct(); s.Node.BackgroundFlags = GumpBackgroundFlags.Solid | 1; var fill = s.BackgroundHooks.Fill; s.BackgroundHooks.Fill = (r, c, check) => { fill!(r, c, check); s.Node.X = 7; s.Node.Y = 9; }; s.Hooks.Shape = (shape, frame, p) => { s.Corners.Add((shape, frame, p)); if (s.Corners.Count == 1) { s.Node.CornerShape = 83; s.Node.CornerFrame = 1; s.Node.Width = 12.9f; s.Node.X = 90; s.Node.Y = 90; } else if (s.Corners.Count == 2) { s.Node.Height = 9.9f; } else if (s.Corners.Count == 3) { s.Node.Height = 11.9f; } }; s.Decoration.Draw(50);
        Assert.Equal(new[] { (82U, 0U, new ScreenPoint(7, 9)), (83U, 2U, new ScreenPoint(19, 9)), (83U, 3U, new ScreenPoint(19, 18)), (83U, 4U, new ScreenPoint(7, 20)) }, s.Corners);
    }
    /// <summary>영역은 자신/부모 visible만 검사하고 erase/dead/void는 걸러내지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualRedrawRegionAndVisibleParents(OriginalEdition edition)
    {
        var s = new DecorationScene(edition); s.Construct(); s.Node.Flags |= 0x10008; var frame = new GumpFrame(s.Scene.Create, s.Scene.LifeState, s.Scene.FrameHooks); frame.RedrawNode(50); Assert.Equal(new ScreenRect(4, 5, 18, 16), Assert.Single(s.Invalidated)); s.Invalidated.Clear(); s.Scene.Nodes[1].Flags &= ~1U; s.Decoration.Redraw(50, false); Assert.Empty(s.Invalidated); s.Scene.Nodes[1].Flags |= 1; s.Node.Flags &= ~1U; s.Decoration.Redraw(50, false); Assert.Empty(s.Invalidated); s.Node.Flags |= 1; s.Node.Width = -3.25f; s.Decoration.Redraw(50, false); Assert.Empty(s.Invalidated); s.Node.Width = float.NaN; s.Decoration.Redraw(50, false); Assert.Empty(s.Invalidated);
    }
    /// <summary>사용한 영역 경계만 요구하며 부모 순환과 비공유 저장소는 명시 거부한다.</summary>
    [Fact]
    public void ReachedContractsAndPartialConstructorWrites()
    {
        var s = new DecorationScene(); s.State.Alignment = -3; s.Hooks.Type = null; Assert.Throws<ArgumentException>(() => s.Decoration.Construct(50, 3, 4)); Assert.Equal((0x50ae7cU, unchecked((uint)-3), 82U, 2U), (s.Node.Vtable, s.Node.Command, s.Node.CornerType, s.Scene.Create.Count)); s.Hooks.Type = _ => new(82, uint.MaxValue); s.Decoration.Construct(50, 3, 4); s.Scene.Tree.Attach(50, 5, 6, 1, 1); s.Hooks.Invalidate = null; Assert.Throws<ArgumentException>(() => s.Decoration.Redraw(50, false)); s.Scene.Nodes[1].Parent = 50; Assert.Throws<InvalidOperationException>(() => s.Decoration.Redraw(50, false)); s.Scene.Nodes[1].Parent = 0; s.Hooks.Base = _ => new GumpCreateNode(); Assert.Throws<InvalidOperationException>(() => s.Decoration.Construct(50, 3, 4));
    }
    /// <summary>실제 현재 클립/물리 프레임 범위/특수 프레임과 최초 캐시 번호를 원색 출력에서 사용한다.</summary>
    [Fact]
    public void ActualPixelsClipSpecialAndPhysicalBounds()
    {
        var s = new DecorationScene(); var hooks = s.Pixels.Hooks(); GumpDraw.SetGumpDrawClip(s.Scene.Clip, new(4, 5, 5, 6)); hooks.Shape!(82, 0, new(5, 6)); Assert.Equal(41, s.Scene.Pixels.Span[5 * 64 + 4]); byte[] saved = s.Scene.Pixels.ToArray(); hooks.Shape(82, 4, new(5, 6)); hooks.Shape(82, 1, new(15, 6)); Assert.Equal(saved, s.Scene.Pixels.ToArray()); Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Shape(82, 5, new())); Assert.Throws<ArgumentOutOfRangeException>(() => hooks.Shape(99, 0, new())); Assert.Equal(new GumpDecorationType(83, 1), s.Assets.Type(83)); Assert.Throws<ArgumentOutOfRangeException>(() => s.Assets.Type(99));
    }
    /// <summary>가상 수명 분배는 실제 Kill/Destroy와 등록/idle/overlay/텍스트 반납을 사용한다.SHP는 해제하지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078)]
    [InlineData(OriginalEdition.Cd1072)]
    public void ActualDeferredLifetimeAndBaseFallback(OriginalEdition edition)
    {
        var s = new DecorationScene(edition); s.Construct(); uint id = s.Node.Id; s.Node.TextPointer = 100; s.Node.AlternateTextPointer = 101; s.Scene.Lifetime.SetIdle(50, true); s.Scene.Lifetime.SetOverlay(50, true); s.Scene.Lifetime.Kill(50); Assert.DoesNotContain(50U, s.Scene.Released); s.Scene.Lifetime.Cleanup(); Assert.Equal(new uint[] { 100, 101, 50 }, s.Scene.Released); Assert.Equal(0U, s.Scene.Create.Entries[id]); Assert.Equal(1U, s.Scene.Create.Count); Assert.DoesNotContain(50U, s.Scene.LifeState.Idle); Assert.DoesNotContain(50U, s.Scene.LifeState.Overlay); Assert.Equal(82U, s.Node.CornerShape); Assert.Equal(0U, s.Scene.LifeState.KillCount);
    }
}
