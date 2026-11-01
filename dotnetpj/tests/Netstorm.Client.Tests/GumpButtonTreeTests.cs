using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Tests;

/// <summary>한 실제 Gump 트리에서 여러 버튼의 입력·캡처·등록·자식 삭제를 확인한다.</summary>
public sealed class GumpButtonTreeTests
{
    /// <summary>서로 다른 두 버튼의 등록과 클릭을 분리하고 한 버튼 삭제가 다른 버튼을 보존한다.</summary>
    [Fact]
    public void MultipleButtonsShareTreeInputAndLifetime()
    {
        var fixture = new GumpFontFixture("gumptext");
        var notified = new List<(uint Code, uint Serial, uint Value)>();
        int sound = 0;
        var tree = new GumpButtonTree(640, 480, GumpBackgroundTestAssets.Assets(), 82, 0, new(9, 243, 57), fixture.Fonts,
            new() { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => sound++,
                Redraw = (_, _) => { }, HandleBase = (_, code, a, b) => { if (code == 0x0200002c) notified.Add((code, a, b)); return 0; } });
        uint first = tree.AddStone(75, 19, "First\0"u8.ToArray(), 10, 10);
        uint second = tree.AddStone(75, 19, "Second\0"u8.ToArray(), 100, 10);
        var a = Assert.IsType<StoneButtonGumpNode>(tree.Node(first));
        var b = Assert.IsType<StoneButtonGumpNode>(tree.Node(second));
        Assert.Equal(5U, tree.Create.Count);
        Assert.Equal(first, tree.Input.Route(InputCode.LeftButton, 11, 11));
        Assert.Equal(first, tree.LifeState.Focus);
        Assert.Equal(first, tree.Input.Route(InputCode.LeftButton | InputCode.Release, 11, 11));
        Assert.Equal(0U, tree.LifeState.Focus);
        Assert.Equal(new[] { (0x0200002cU, a.Serial, 0U) }, notified);
        Assert.Equal(1, sound);
        uint background = a.Background;
        tree.Remove(first);
        Assert.Null(tree.Node(first)); Assert.Null(tree.Node(background));
        Assert.Same(b, tree.Node(second)); Assert.Equal(3U, tree.Create.Count);
        Assert.Equal(second, tree.Input.Route(InputCode.LeftButton, 101, 11));
        Assert.Equal(second, tree.Input.Route(InputCode.LeftButton | InputCode.Release, 101, 11));
        Assert.Equal(b.Serial, notified[1].Serial); Assert.Equal(2, sound);
    }
    /// <summary>중첩 모달은 이전 자식의 최소 깊이 앞에 오고 입력을 제한하며 제거 시 이전 모달로 돌아간다.</summary>
    [Fact]
    public void ModalUsesActualTreeDepthAndInputStack()
    {
        var fixture = new GumpFontFixture("gumptext"); var actions = new List<uint>(); int redraws = 0;
        var tree = new GumpButtonTree(640, 480, GumpBackgroundTestAssets.Assets(), 82, 0, new(9, 243, 57), fixture.Fonts,
            new() { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { },
                Redraw = (_, _) => redraws++, HandleBase = (_, code, a, _) => { if (code == 0x0200002c) actions.Add(a); return 0; } });
        uint first = tree.AddStone(75, 19, "First\0"u8.ToArray(), 10, 10);
        uint second = tree.AddStone(75, 19, "Second\0"u8.ToArray(), 100, 10);
        var one = Assert.IsType<StoneButtonGumpNode>(tree.Node(first));
        var two = Assert.IsType<StoneButtonGumpNode>(tree.Node(second));
        GumpCreateNode firstChild = tree.Node(one.Background)!; GumpCreateNode secondChild = tree.Node(two.Background)!;
        one.Depth = 10; firstChild.Depth = unchecked((uint)-30); firstChild.Flags |= 1;
        two.Depth = 80; secondChild.Depth = 82;
        int before = redraws;
        tree.Modal.SetModal(first, true); tree.Modal.SetModal(second, true);
        Assert.Equal(unchecked((uint)-33), two.Depth); Assert.Equal(unchecked((uint)-31), secondChild.Depth);
        Assert.Equal(second, tree.LifeState.CurrentDialog); Assert.Equal(2U, tree.LifeState.DialogCount);
        tree.Input.Route(InputCode.LeftButton, 11, 11);
        tree.Input.Route(InputCode.LeftButton | InputCode.Release, 11, 11);
        Assert.Empty(actions);
        Assert.Equal(second, tree.Input.Route(InputCode.LeftButton, 101, 11));
        Assert.Equal(second, tree.Input.Route(InputCode.LeftButton | InputCode.Release, 101, 11));
        Assert.Equal(new[] { two.Serial }, actions);
        tree.Modal.SetModal(second, true); Assert.Equal(2U, tree.LifeState.DialogCount); Assert.Equal(unchecked((uint)-33), two.Depth);
        tree.Modal.SetModal(second, false); Assert.Equal(first, tree.LifeState.CurrentDialog);
        Assert.Equal(first, tree.Input.Route(InputCode.LeftButton, 11, 11));
        Assert.Equal(first, tree.Input.Route(InputCode.LeftButton | InputCode.Release, 11, 11));
        Assert.Equal(new[] { two.Serial, one.Serial }, actions); Assert.True(redraws > before);
        tree.Remove(first); Assert.Equal(0U, tree.LifeState.CurrentDialog);
    }

    /// <summary>같은 생성 노드의 배경과 부모를 실제 인덱스 표면에 출력한다.</summary>
    [Fact]
    public void DrawUsesSameNodesAndActualPixelSurface()
    {
        byte[] advances = new byte[1024], mask = new byte[256];
        GumpFontFixture.Put(advances, 65 * 4, 2); mask[65] = 1;
        byte[] font = GumpFontFixture.GlyphCache(advances, mask); GumpFontFixture.Put(font, 36 + 65 * 4, 2);
        var fonts = new FontStore(path => path == "d/!Arial.normal.14.700.chfnt" ? font : null, "Arial",
            (_, _, _, _) => throw new InvalidOperationException("검사 글꼴 누락"));
        var assets = GumpBackgroundTestAssets.Assets();
        var tree = new GumpButtonTree(100, 40, assets, 82, 0, new(9, 243, 57), fonts,
            new() { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        uint button = tree.AddStone(75, 19, [65, 0], 10, 10);
        var clip = new GumpDrawState(); GumpDraw.SetGumpDrawClip(clip, new(0, 0, 100, 40));
        byte[] pixels = Enumerable.Repeat((byte)7, 4000).ToArray();
        var target = new GumpPixelTarget(pixels, 100, 40, clip);
        var palette = new PaletteColorTable();
        tree.DrawStone(button, target, clip, GumpBackgroundTestAssets.Maps(), palette);
        Assert.Contains(pixels, value => value == 243);
        Assert.Contains(pixels, value => value == 57);
        Assert.Equal(7, pixels[0]);
        Assert.Equal(new ScreenRect(0, 0, 100, 40), clip.Clip);
    }

}
