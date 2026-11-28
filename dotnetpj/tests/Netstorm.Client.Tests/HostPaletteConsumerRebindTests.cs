using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;

// 실제 파일/메뉴/StyleText/등록노드는 유지하고 현재 기본 팔레트만 재결선한다.
namespace Netstorm.Client.Tests;

/// <summary>새 helper 경계만 호출한 성공과 구별해 실제 Host UI setter를 소비하는 검사다.</summary>
public sealed class HostPaletteConsumerRebindTests
{
    /// <summary>같은 Style객체/조각과 이미 생성된명시문맥은보존하고이후색명령/버튼/슬라이더기본입력은새명명색을읽는다.</summary>
    [Fact]
    public void ExistingUiOwnersKeepTheirIdentityAndNewSemanticColorsUseCurrentPalette()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        int redraws = 0;
        var menu = new HostMainMenu(startup, screen, new()
        {
            Now = () => 0, Poll = _ => throw new IOException("재바인딩중입력금지"), Sound = () => throw new IOException("재바인딩중음향금지"),
            Redraw = (_, _) => redraws++, HandleBase = (_, _, _, _) => throw new IOException("재바인딩중기본사건금지"),
        });
        var queue = new InputQueue(); var windows = new HostHtmlWindows(menu, queue, () => 0, _ => throw new IOException("닫기금지"));
        var text = new HostHtmlText(menu, windows, screen, _ => throw new IOException("입력금지"), (_, _) => redraws++,
            _ => throw new IOException("커서금지"), _ => throw new IOException("음향금지"));
        text.Bind(windows);
        var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => redraws++);
        using var entry = new HostTextEntry(menu, windows, text, screen, () => 0, (_, _) => redraws++);
        var style = text.Style; var widget = text.WidgetState; uint original = 0; uint[] initial = new uint[8];
        style.Parse(ref original, "~wA"u8, initial); var fragment = text.Fragments.Fragments[original];
        uint[] oldContext = fragment.Context.ToArray(); byte[] oldText = fragment.Text.ToArray();
        uint oldButton = menu.Tree.AddStone(40, 20, "A\0"u8.ToArray(), 2, 2);
        uint entryToken = entry.Create(120, 30, "working draft"u8.ToArray(), 0, 0, screen.Colors().Basic[7]);
        var editor = entry.Entry;
        var oldNode = Assert.IsType<StoneButtonGumpNode>(menu.Tree.Node(oldButton)); var oldButtonContexts = oldNode.Contexts.ToArray();
        int registered = menu.Tree.Create.Entries.Count(value => value != 0), fragments = text.Fragments.Fragments.Count, beforeRedraw = redraws;
        uint random = startup.Random.State(), loaded = screen.PaletteLoaded; int fileIndex = startup.Files.FileNameOwner.Index;
        var palette = PaletteColors.BuildPaletteColorTable(screen.LogicalPalette);
        uint white = (palette.Basic[7] + 17) & 255; palette.Basic[7] = white; palette.Basic[0] = 13; palette.Basic[5] = 97;
        palette.Named[9] = palette.Named[9] with { Index = 73 };
        menu.Tree.SetPalette(new(13, white, 119)); text.SetPalette(palette); menus.SetPalette(palette); entry.SetPalette(palette);
        Assert.Same(style, text.Style); Assert.Same(widget, text.WidgetState); Assert.Same(fragment, text.Fragments.Fragments[original]);
        Assert.Equal(oldContext, fragment.Context); Assert.Equal(oldText, fragment.Text); Assert.Equal(oldButtonContexts, oldNode.Contexts);
        Assert.Equal(registered, menu.Tree.Create.Entries.Count(value => value != 0)); Assert.Equal(fragments, text.Fragments.Fragments.Count);
        Assert.Equal(beforeRedraw, redraws);
        Assert.Same(editor, entry.Entry);
        var buffer = new byte[256]; entry.Entry.CopyText(entryToken, buffer);
        Assert.Equal("working draft", System.Text.Encoding.ASCII.GetString(buffer).Split('\0')[0]);
        Assert.Equal(random, startup.Random.State()); Assert.Equal(fileIndex, startup.Files.FileNameOwner.Index); Assert.Equal(loaded, screen.PaletteLoaded);
        Assert.Equal(white, text.ProductionHints.White); Assert.Equal(13u, text.ProductionHints.Black);
        Assert.Equal(73u, text.SliderArrowType); Assert.Equal(13u, text.SliderArrowFrame); Assert.Equal(73u, widget.SliderType); Assert.Equal(13u, widget.SliderFrame);
        uint current = 0; style.Parse(ref current, "~wB"u8, initial);
        Assert.Equal(white, text.Fragments.Fragments[current].Context[2]);
        uint newButton = menu.Tree.AddStone(40, 20, "B\0"u8.ToArray(), 42, 2);
        Assert.Equal(white, Assert.IsType<StoneButtonGumpNode>(menu.Tree.Node(newButton)).Contexts[0].Color);
    }
}
