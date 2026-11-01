using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Host;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Automation;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Tests;

/// <summary>실제 배포 자산의 설정·팔레트·메뉴 GIF가 논리 화면에 연결되는지 검사한다.</summary>
public sealed class HostStartupSceneTests
{
    /// <summary>실제 호스트 트리에서 문자 입력과 두 Slider를 생성·표시·반납한다.</summary>
    [Fact]
    public void HostTextEntryUsesSharedTreeDrawingAndLifetime()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        var queue = new InputQueue(); var windows = new HostHtmlWindows(menu, queue, () => 1, _ => { });
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); text.Bind(windows);
        using var entry = new HostTextEntry(menu, windows, text, screen, () => 1, (_, _) => { });
        var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { });
        uint token = entry.Create(200, 30, "Map Name"u8.ToArray(), 0, 0, screen.Colors().Basic[7]);
        menu.Tree.TreeHooks.Attach!(token, 100, 100, menu.Tree.Create.Entries[0], 0);
        var node = Assert.IsType<TextEntryGumpNode>(menu.Tree.Node(token)); Assert.NotEqual(0u, node.VerticalSlider); Assert.NotEqual(0u, node.HorizontalSlider);
        var output = new byte[256]; entry.Entry.CopyText(token, output); Assert.Equal("Map Name", System.Text.Encoding.ASCII.GetString(output).Split('\0')[0]);
        Assert.Equal(1u, menu.Tree.InputHooks.Handle!(token, 'A', 0, 0)); Array.Clear(output); entry.Entry.CopyText(token, output); Assert.Equal("A", System.Text.Encoding.ASCII.GetString(output).Split('\0')[0]);
        new HostGumpDrawing(menu, text, menus, screen, entry).Draw();
        menu.Tree.Lifetime.Kill(token); menu.Tree.Lifetime.Cleanup(); Assert.Null(menu.Tree.Node(token)); windows.CloseAll();
    }
    /// <summary>저장소의 게임 데이터에서 시작 자산을 찾아 팔레트와 화면을 채운다.</summary>
    [Fact]
    public void LoadsRealTitleAndPaletteIntoLogicalScreen()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
        startup.Draw(screen);
        Assert.Equal(1U, screen.PaletteLoaded);
        Assert.Contains(screen.Pixels.ToArray(), pixel => pixel != 0);
        Assert.NotEmpty(startup.Configuration.PathSpec("BackgroundGifSpec", "titleMenu"));
        string path = startup.Configuration.PathSpec("CloudFileSpec");
        var cloud = Netstorm.Client.Display.GifImage.DecodeBackgroundGif(startup.Files.ReadAllBytes(path));
        Assert.Equal(512, cloud.Width); Assert.Equal(512, cloud.Height);
        Assert.Equal(cloud.Indices[0], screen.Pixels[0]);
        Assert.Equal(cloud.Indices[0], screen.Pixels[512]);
        Assert.Equal(cloud.Indices[0], screen.Pixels[512 * screen.Pitch]);
    }
    /// <summary>호스트 설정은 실제 Config의 시작 인자로 전달되어 파일 설정에 우선한다.</summary>
    [Fact]
    public void HostOverridesReachActualConfiguration()
    {
        string? data = GameDataLocator.FindDataDirectory();
        Assert.NotNull(data);
        var options = HostOptions.Parse(["--set", "tellTips=1;maxFPS=35", "--set", "tellTips=0"]);
        var startup = new HostStartupScene(data, options.SettingsText);
        Assert.Equal(0, startup.Configuration.GetInt("tellTips"));
        Assert.Equal(35, startup.Configuration.GetInt("maxFPS"));
        Assert.Equal(35, options.FramesPerSecond);
    }

    /// <summary>실제 fortGump·글꼴을 사용해 메인 메뉴 여덟 버튼이 각각 타이틀 화면을 변경한다.</summary>
    [Fact]
    public void MainMenuDrawsEightRealAssetButtons()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        byte[] before = screen.Pixels.ToArray();
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => throw new InvalidOperationException("입력 전용"), Poll = _ => throw new InvalidOperationException("입력 전용"),
            Sound = () => throw new InvalidOperationException("입력 전용"), Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        menu.Draw(screen);
        // 버튼마다 해당 사각형 안에서 적어도 하나의 픽셀이 실제 자산 출력으로 달라진다.
        for (int index = 0; index < 8; index++)
        {
            int x = 1024 / 2 - 156 + (index % 4) * 79, y = 768 / 2 - 73 + (index / 4) * 23;
            bool changed = false;
            // 각 버튼의 표시 영역만 비교하며 다른 버튼의 변경은 세지 않는다.
            for (int row = y; row < y + 19 && !changed; row++)
                for (int col = x; col < x + 75; col++)
                    if (before[row * screen.Pitch + col] != screen.Pixels[row * screen.Pitch + col]) { changed = true; break; }
            Assert.True(changed, $"버튼 {index} 출력이 없다");
        }
    }

    /// <summary>공통 수집·정렬 출력의 초기 여덟 버튼이 기존 실제 버튼 출력과 같은 화면을 만든다.</summary>
    [Fact]
    public void SharedDrawingPreservesInitialMenuPixels()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        menu.Draw(screen); byte[] expected = screen.Pixels.ToArray(); startup.Draw(screen);
        var windows = new HostHtmlWindows(menu, new InputQueue(), () => 1, _ => { });
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { });
        text.Bind(windows); var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { });
        new HostGumpDrawing(menu, text, menus, screen).Draw(); Assert.Equal(expected, screen.Pixels.ToArray());
        windows.CloseAll();
    }

    /// <summary>메뉴와 공유한 실제 fortGump 배경·I 모서리 장식을 생성·출력·반납한다.</summary>
    [Fact]
    public void SharedTreeDrawsActualFortGumpDecoration()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        byte[] before = screen.Pixels.ToArray(); uint initial = menu.Tree.Create.Count;
        uint token = menu.Tree.AllocateNode(new HtmlWindowGumpNode());
        menu.Decoration.Construct(token, 160, 80); menu.Tree.Tree.Move(token, 100, 100, menu.Tree.Create.Entries[0]);
        var node = Assert.IsType<HtmlWindowGumpNode>(menu.Tree.Node(token));
        var type = new TypeCatalog(startup.Files).Find("fortGump")!;
        Assert.Equal((uint)type.RuntimeIndex, node.CornerType);
        Assert.Equal((uint)type.Definition.Frames.Find('I', TypeFrameTable.DefaultVariant, 0), node.CornerFrame);
        menu.Decoration.Draw(token, screen);
        // 각 모서리의 실제 프레임 출력 영역에서 픽셀 변경을 확인한다.
        foreach (var (cx, cy) in new[] { (100, 100), (260, 100), (260, 180), (100, 180) })
            Assert.Contains(Enumerable.Range(cy - 6, 13), y => Enumerable.Range(cx - 6, 13)
                .Any(x => before[y * screen.Pitch + x] != screen.Pixels[y * screen.Pitch + x]));
        menu.Tree.Lifetime.Kill(token); menu.Tree.Lifetime.Cleanup();
        Assert.Null(menu.Tree.Node(token)); Assert.Equal(initial, menu.Tree.Create.Count);
    }

    /// <summary>호스트 HTML 소유자가 실제 버튼·명령 큐와 공통 등록 및 창 정리를 공유한다.</summary>
    [Fact]
    public void HostHtmlOwnerRoutesActualButtonAndCleansPartialBody()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        var queue = new InputQueue(); var owner = new HostHtmlWindows(menu, queue, () => 12.5, _ => throw new InvalidOperationException());
        uint initial = menu.Tree.Create.Count; uint root = owner.Create(0, 0);
        owner.Window.AddButton(root, "Okay", 0x02000095, 7, 0);
        var html = Assert.IsType<HtmlWindowGumpNode>(menu.Tree.Node(root)); uint button = html.Controls[0].Child;
        menu.Tree.Stones.FitToText(button); menu.Tree.Tree.Move(root, 100, 100, menu.Tree.Create.Entries[0]);
        menu.Tree.Tree.Move(button, 5, 5, root); menu.Tree.Modal.SetModal(root, true);
        menu.Route(new(InputCode.LeftButton, 106, 106)); menu.Route(new(InputCode.LeftButton | InputCode.Release, 106, 106));
        InputEvent command = queue.Pop(false); Assert.Equal((0x02000095u, 7, 7), (command.Code, command.X, command.Y));
        Assert.Equal(0U, menu.Tree.LifeState.CurrentDialog); menu.Tree.Lifetime.Cleanup();
        Assert.Null(menu.Tree.Node(root)); Assert.Null(menu.Tree.Node(button)); Assert.Equal(0U, owner.State.Instances);
        uint partial = owner.Create(0, 0);
        Assert.Throws<InvalidOperationException>(() => owner.Window.SetBody(partial, "Pending body"));
        owner.CloseAll(); Assert.Null(menu.Tree.Node(partial)); Assert.Equal(initial, menu.Tree.Create.Count);
        Assert.Equal(12.5, owner.State.LastCloseTime);
    }

    /// <summary>실제 Config 변경을 HTML 조건에서 다시 읽고 링크가 부모 반납 때까지 유지되는지 확인한다.</summary>
    [Fact]
    public void HostHtmlParserUsesCurrentConfigAndOwnsLinksUntilRelease()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var owner = new HostHtmlWindows(menu, queue, () => 1, _ => { });
        void QueueCommand(string raw) => ScriptCommand.QueueScriptCommand(raw, queue, strings,
            new ScriptCommandHooks { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() });
        owner.BindParser(startup, QueueCommand);
        var parser = Assert.IsType<HtmlWindowTextParser>(owner.Parser);
        Assert.Throws<InvalidOperationException>(() => owner.BindParser(startup, QueueCommand));
        uint root = owner.Create(0, 0); startup.Configuration.Set("HostHtmlTest", "1");
        string output = parser.Parse(new(), root, "<?{HostHtmlTest}>Visible</?><a href=\"#Target\">Link</a><$Tell,Next>", 8192);
        Assert.Contains("Visible", output); InputEvent command = queue.Pop(false);
        Assert.Equal(0x02000095U, command.Code & 0x03ffffffU); Assert.Equal(command.X, command.Y);
        Assert.Equal("Next", strings.Resolve(unchecked((uint)command.X))); Assert.True(queue.Empty);
        var match = System.Text.RegularExpressions.Regex.Match(output, @"~\[T(\d+)\]");
        Assert.True(match.Success); uint link = uint.Parse(match.Groups[1].Value, System.Globalization.CultureInfo.InvariantCulture);
        Assert.Equal("Target", parser.Resolve(link));
        startup.Configuration.Set("HostHtmlTest", "0");
        Assert.DoesNotContain("Visible", parser.Parse(new(), root, "<?{HostHtmlTest}>Visible</?>", 8192));
        menu.Tree.Lifetime.Kill(root); Assert.Equal("Target", parser.Resolve(link));
        menu.Tree.Lifetime.Cleanup(); Assert.Throws<ArgumentOutOfRangeException>(() => parser.Resolve(link));
        owner.CloseAll();
    }

    /// <summary>실제 호스트 자산에서 HTML 본문·슬라이더·글꼴 배치와 자식 수명을 검증한다.</summary>
    [Fact]
    public void HostHtmlBodyUsesActualFontAndSharedSliderLifetime()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        var owner = new HostHtmlWindows(menu, new InputQueue(), () => 1, _ => throw new InvalidOperationException());
        var text = new HostHtmlText(menu, owner, screen, _ => default, (_, _) => { }, _ => throw new InvalidOperationException(), _ => throw new InvalidOperationException());
        text.Bind(owner); uint initial = menu.Tree.Create.Count; uint root = owner.Create(0, 0);
        owner.Window.SetBody(root, "Storm Islands\nBuild a bridge.\n");
        var html = Assert.IsType<HtmlWindowGumpNode>(menu.Tree.Node(root));
        var body = Assert.IsType<ScrollingTextGumpNode>(menu.Tree.Node(html.Body));
        var slider = Assert.IsType<ScrollGumpNode>(menu.Tree.Node(body.Slider));
        Assert.NotNull(menu.Tree.Node(slider.Decrement)); Assert.NotNull(menu.Tree.Node(slider.Increment)); Assert.NotNull(menu.Tree.Node(slider.Thumb));
        Assert.Equal(2, text.Buffers.Binding(body.Buffer).State.LineCount);
        Assert.Contains(text.Fragments.Fragments.Values, fragment => System.Text.Encoding.Latin1.GetString(fragment.Text).Contains("Storm", StringComparison.Ordinal));
        text.Text.FitContent(html.Body); Assert.True(body.BaseWidth > 0); Assert.True(body.BaseHeight > 0);
        menu.Tree.Tree.Move(root, 100, 100, menu.Tree.Create.Entries[0]);
        menu.Tree.Tree.Move(html.Body, 3, 4, root); text.Text.RequestScroll(html.Body, 0);
        ScreenRect bounds = menu.Tree.Collection.Bounds(html.Body); byte[] before = screen.Pixels.ToArray();
        text.Draw(html.Body);
        // 실제 원본 글꼴이 본문 영역에 출력되고 clip 밖의 화면 픽셀은 보존된다.
        bool changed = false;
        for (int y = 0; y < screen.Height; y++)
        {
            // 전체 논리 행을 순회해 출력 영역 밖 쓰기도 확인한다.
            for (int x = 0; x < screen.Width; x++)
            {
                if (before[y * screen.Pitch + x] == screen.Pixels[y * screen.Pitch + x]) continue;
                Assert.True(x >= bounds.Left && x < bounds.Right && y >= bounds.Top && y < bounds.Bottom);
                changed = true;
            }
        }
        Assert.True(changed);
        owner.CloseAll(); Assert.Equal(initial, menu.Tree.Create.Count); Assert.Empty(text.Fragments.Fragments);
    }

    /// <summary>실제 HTML 전체 배치가 화면 중앙·같은 폭의 버튼과 본문 교체를 공통 트리에 반영한다.</summary>
    [Fact]
    public void HostHtmlFitCentersAndReflowsRealBodyAndButtons()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        var owner = new HostHtmlWindows(menu, new InputQueue(), () => 1, _ => throw new InvalidOperationException());
        var text = new HostHtmlText(menu, owner, screen, _ => default, (_, _) => { }, _ => throw new InvalidOperationException(), _ => throw new InvalidOperationException());
        text.Bind(owner); uint initial = menu.Tree.Create.Count; uint root = owner.Create(400, 0);
        owner.Window.SetBody(root, "A real window body.\n");
        owner.Window.AddButton(root, "Okay", 0x02000095, 1, 0);
        owner.Window.AddButton(root, "Longer button", 0x02000095, 2, 0);
        menu.Tree.TreeHooks.Attach!(root, 0, 0, menu.Tree.Create.Entries[0], 0);
        var window = Assert.IsType<HtmlWindowGumpNode>(menu.Tree.Node(root));
        var left = Assert.IsType<StoneButtonGumpNode>(menu.Tree.Node(window.Controls[0].Child));
        var right = Assert.IsType<StoneButtonGumpNode>(menu.Tree.Node(window.Controls[1].Child));
        var body = Assert.IsType<ScrollingTextGumpNode>(menu.Tree.Node(window.Body));
        Assert.Equal(-80, unchecked((int)window.Depth)); Assert.Equal(left.BaseWidth, right.BaseWidth);
        Assert.Equal((1024 - window.BaseWidth) / 2, window.X); Assert.Equal((768 - window.BaseHeight) / 2, window.Y);
        Assert.Equal(20, body.Y); Assert.True(right.X > left.X); Assert.True(body.BaseHeight > 0);
        uint previousBody = window.Body;
        owner.Window.SetBody(root, "Changed text\nSecond line\n");
        Assert.NotEqual(previousBody, window.Body); Assert.Equal((1024 - window.BaseWidth) / 2, window.X);
        menu.Tree.Lifetime.Cleanup(); Assert.Null(menu.Tree.Node(previousBody));
        Assert.Equal(2, text.Buffers.Binding(((ScrollingTextGumpNode)menu.Tree.Node(window.Body)!).Buffer).State.LineCount);
        owner.CloseAll(); Assert.Equal(initial, menu.Tree.Create.Count); Assert.Empty(text.Fragments.Fragments);
    }

    /// <summary>실제 HTML 메타의 메뉴 열이 공유 트리에서 측정·부착·자식 우선 반납되는지 확인한다.</summary>
    [Fact]
    public void HostHtmlMenuMetadataBuildsAndFitsSharedMenu()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        var owner = new HostHtmlWindows(menu, new InputQueue(), () => 1, _ => { });
        var text = new HostHtmlText(menu, owner, screen, _ => default, (_, _) => { }, _ => { }, _ => { });
        text.Bind(owner); var menus = new HostHtmlMenus(menu, owner, text, startup, screen, (_, _) => { });
        uint initial = menu.Tree.Create.Count; uint root = owner.Create(400, 0);
        owner.Window.SetBody(root, "Choose a mission.\n");
        var strings = new ScriptStringRing(); var hooks = new HtmlWindowMetadataHooks
        {
            Node = token => menu.Tree.Node(token) as HtmlWindowGumpNode,
            Commands = new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() },
            Read = _ => 0
        };
        HtmlWindowMetadata.BindHtmlWindowMetadataDevices(hooks, owner.Window, strings, menu.Tree.Lifetime);
        menus.BindMetadata(hooks);
        byte[] buffer = new byte[8192]; System.Text.Encoding.Latin1.GetBytes("$Menu=First,Tell,First \n$Checked=Second,Tell,Second,1,1 \n").CopyTo(buffer, 0);
        Assert.Equal(1U, new HtmlWindowMetadata(hooks).Process(root, buffer));
        uint column = menus.Menu.Current(); var node = Assert.IsType<MenuGumpNode>(menu.Tree.Node(column));
        Assert.Equal(2U, node.ItemCount); Assert.True(node.BaseWidth > 0); Assert.True(node.BaseHeight > 0);
        Assert.Equal(0x02000095U, node.Items[0].Command); Assert.Equal("First", strings.Resolve(node.Items[0].Parameter));
        menu.Tree.TreeHooks.Attach!(root, 0, 0, menu.Tree.Create.Entries[0], 0);
        Assert.Equal(0U, menus.State.Index); Assert.Equal(root, node.Parent);
        owner.CloseAll(); Assert.Equal(0U, menus.State.Instances); Assert.Null(menu.Tree.Node(column));
        Assert.Equal(initial, menu.Tree.Create.Count); Assert.Empty(text.Fragments.Fragments);
        uint unfinished = menus.Widgets.BeginVertical();
        menus.Controls.AddText("Partial"u8, 0x02000095, 0, true, true, false);
        owner.CloseAll(); Assert.Equal(0U, menus.State.Index); Assert.Equal(0U, menus.State.Instances);
        Assert.Null(menu.Tree.Node(unfinished)); Assert.Equal(initial, menu.Tree.Create.Count); Assert.Empty(text.Fragments.Fragments);
    }

    /// <summary>실제 Credits Tell을 전체 창 생성기로 읽어 모달과 본문을 만들고 Enter로 닫는다.</summary>
    [Fact]
    public void HostHtmlFactoryShowsActualCreditsAndClosesThroughInput()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        });
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 42, _ => throw new InvalidOperationException());
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings,
            new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { });
        text.Bind(windows); var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { });
        var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 42, _ => 0,
            _ => throw new InvalidOperationException());
        var drawing = new HostGumpDrawing(menu, text, menus, screen); drawing.Draw(); byte[] before = screen.Pixels.ToArray();
        uint initial = menu.Tree.Create.Count; var result = factory.ShowTell("Credits");
        Assert.True(result.Found); Assert.NotEqual(0U, result.Serial);
        uint root = menu.Tree.Reference(result.Serial); var node = Assert.IsType<HtmlWindowGumpNode>(menu.Tree.Node(root));
        Assert.Equal(root, menu.Tree.LifeState.CurrentDialog); Assert.NotEqual(0U, node.Body); Assert.True(node.ControlCount > 0);
        Assert.NotEmpty(text.Fragments.Fragments);
        drawing.Draw(); Assert.False(before.SequenceEqual(screen.Pixels.ToArray()));
        Assert.NotEqual(0U, menu.Route(new(windows.State.EnterEvent, 0, 0)));
        Assert.Equal(0U, menu.Tree.LifeState.CurrentDialog); Assert.False(queue.Empty);
        menu.Tree.Lifetime.Cleanup(); Assert.Null(menu.Tree.Node(root)); Assert.Empty(text.Fragments.Fragments);
        Assert.Equal(initial, menu.Tree.Create.Count);
        startup.Draw(screen); drawing.Draw(); Assert.Equal(before, screen.Pixels.ToArray());
        uint serial = factory.Factory.Show("Choose<br>$Menu=First,Tell,First \n$Checked=Second,Tell,Second,1,1 \n", 0, 0, 0);
        root = menu.Tree.Reference(serial); var column = Assert.Single(menu.Tree.Create.Entries
            .Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.Equal(root, column.Parent); Assert.Equal("First", strings.Resolve(column.Items[0].Parameter));
        drawing.Draw(); Assert.False(before.SequenceEqual(screen.Pixels.ToArray()));
        windows.CloseAll(); Assert.Equal(initial, menu.Tree.Create.Count); Assert.Empty(text.Fragments.Fragments);
    }

    /// <summary>메인 Credits 클릭을 다음 Tick Tell과 실제 모달 클릭·큐·지연 정리까지 연결한다.</summary>
    [Fact]
    public void HostMenuInputOpensAndClosesCreditsThroughSharedQueue()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        HostMenuInput? router = null; int changes = 0;
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => changes++, HandleBase = (_, _, _, _) => 0
        }, action => router!.Post(action));
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 1, _ => throw new InvalidOperationException());
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings,
            new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => changes++, _ => { }, _ => { });
        text.Bind(windows); var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => changes++);
        menus.BindInput(queue, _ => { }, (_, _) => changes++);
        var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 1, _ => 0,
            _ => throw new InvalidOperationException());
        router = new HostMenuInput(menu, windows, factory, queue, strings, () => changes++, action => throw new InvalidOperationException(action.ToString()));
        var driver = new HostFrameDriver(screen, queue); driver.BindInputHandler(router.Dispatch);
        var drawing = new HostGumpDrawing(menu, text, menus, screen); drawing.Draw(); byte[] before = screen.Pixels.ToArray();
        ScreenPoint point = menu.ControlPoint("Credits")!.Value;
        queue.Push(InputCode.LeftButton, point.X, point.Y); queue.Push(InputCode.LeftButton | InputCode.Release, point.X, point.Y);
        Assert.False(driver.ProcessInput()); Assert.Equal(0U, windows.State.Instances); router.FinishFrame();
        router.Tick(); uint root = menu.Tree.LifeState.CurrentDialog; Assert.NotEqual(0U, root);
        drawing.Draw(); Assert.False(before.SequenceEqual(screen.Pixels.ToArray()));
        var node = Assert.IsType<HtmlWindowGumpNode>(menu.Tree.Node(root));
        ScreenRect button = menu.Tree.Collection.Bounds(node.Controls[0].Child); int x = (button.Left + button.Right) / 2, y = (button.Top + button.Bottom) / 2;
        router.NotifyPointer(new(InputCode.Release, x, y));
        queue.Push(InputCode.LeftButton, x, y); queue.Push(InputCode.LeftButton | InputCode.Release, x, y);
        Assert.False(driver.ProcessInput()); router.FinishFrame(); Assert.Equal(0U, windows.State.Instances);
        Assert.Equal(0U, menu.Tree.LifeState.CurrentDialog); Assert.True(changes > 0);
        startup.Draw(screen); drawing.Draw(); Assert.Equal(before, screen.Pixels.ToArray());
        uint assignment = strings.Store("HostMenuInputTest=7"); queue.PushCommand(0x0200006d, (int)assignment, (int)assignment);
        Assert.False(driver.ProcessInput()); Assert.Equal(7, startup.Configuration.GetInt("HostMenuInputTest"));
        router.Post(new("RequestQuitApp", "0")); Assert.True(driver.ProcessInput());
        windows.CloseAll();
    }

    /// <summary>실제 Credits 열의 첫 행을 공통 포인터 통지·입력·큐로 선택해 다음 Tell을 연다.</summary>
    [Fact]
    public void HostMenuInputSelectsActualCreditsRow()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        HostMenuInput? router = null; var sounds = new List<string>();
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 },
            action => router!.Post(action));
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 1, _ => throw new InvalidOperationException());
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings,
            new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { });
        text.Bind(windows); var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { });
        menus.BindInput(queue, sounds.Add, (_, _) => { });
        var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 1, _ => 0, _ => throw new InvalidOperationException());
        router = new HostMenuInput(menu, windows, factory, queue, strings, () => { }, action => throw new InvalidOperationException(action.ToString()));
        var driver = new HostFrameDriver(screen, queue); driver.BindInputHandler(router.Dispatch);
        router.Post(new("Tell", "Credits")); router.Tick(); uint original = menu.Tree.LifeState.CurrentDialog;
        var column = Assert.Single(menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.True(column.ItemCount > 0); uint selectedCommand = column.Items[0].Command;
        string selectedTarget = strings.Resolve(column.Items[0].Parameter); Assert.Equal(0x02000095U, selectedCommand);
        var position = menu.Tree.Collection.Position(menu.Tree.Create.Entries[column.Id]);
        int x = position.X + 8, y = position.Y + (int)column.Offsets[0] + 3;
        router.NotifyPointer(new(0, x, y)); Assert.Equal(0, column.Selection);
        queue.Push(InputCode.LeftButton, x, y); Assert.False(driver.ProcessInput());
        Assert.NotEqual(original, menu.Tree.LifeState.CurrentDialog); Assert.Equal("openSubGump.wav", Assert.Single(sounds));
        Assert.Equal(selectedTarget, System.Text.Encoding.Latin1.GetString(startup.TellState.Current).Split('\0')[0]);
        router.FinishFrame(); Assert.Null(menu.Tree.Node(original)); windows.CloseAll();
    }

    /// <summary>실제 캠페인 파일 목록이 장기 Config로 등록되고 첫 행의 다음 Tell을 같은 트리에서 연다.</summary>
    [Fact]
    public void HostCampaignFileListOpensActualEarlyMissions()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 1, _ => throw new InvalidOperationException());
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings,
            new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); text.Bind(windows);
        var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { }); menus.BindInput(queue, _ => { }, (_, _) => { });
        var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 1, _ => 0, _ => throw new InvalidOperationException());
        var router = new HostMenuInput(menu, windows, factory, queue, strings, () => { }, action => throw new InvalidOperationException(action.ToString()));
        var driver = new HostFrameDriver(screen, queue); driver.BindInputHandler(router.Dispatch);
        router.Post(new("Tell", "UCampaign")); router.Tick(); uint root = menu.Tree.LifeState.CurrentDialog;
        var column = Assert.Single(menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.True(column.ItemCount >= 6); string target = strings.Resolve(column.Items[0].Parameter);
        Assert.Contains("overview", target, StringComparison.OrdinalIgnoreCase);
        string prefix = target[..target.IndexOf('.')]; Assert.NotNull(startup.Configuration.Registry.FindByName(prefix));
        var position = menu.Tree.Collection.Position(menu.Tree.Create.Entries[column.Id]); int x = position.X + 8, y = position.Y + (int)column.Offsets[0] + 3;
        router.NotifyPointer(new(0, x, y)); queue.Push(InputCode.LeftButton, x, y); Assert.False(driver.ProcessInput());
        Assert.NotEqual(root, menu.Tree.LifeState.CurrentDialog); router.FinishFrame(); Assert.Null(menu.Tree.Node(root));
        Assert.Equal(target, System.Text.Encoding.Latin1.GetString(startup.TellState.Current).Split('\0')[0]);
        windows.CloseAll(); Assert.NotNull(startup.Configuration.Registry.FindByName(prefix)); factory.Dispose();
        Assert.Null(startup.Configuration.Registry.FindByName(prefix));
    }

    /// <summary>Help 버튼의 팝업과 About 선택을 같은 원본 Gump·큐·Tell로 연다.</summary>
    [Fact]
    public void HostHelpPopupOpensAboutFromActualMenuRow()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        HostMenuInput? router = null;
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }, action => router!.Post(action));
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 1, _ => throw new InvalidOperationException());
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings, new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); text.Bind(windows);
        var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { }); menus.BindInput(queue, _ => { }, (_, _) => { });
        using var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 1, _ => 0, _ => throw new InvalidOperationException());
        router = new HostMenuInput(menu, windows, factory, queue, strings, () => { }, action => throw new InvalidOperationException(action.ToString()));
        int audioUpdates = 0;
        router.BindPopup(new HostPopupMenu(menu, windows, menus, screen, startup, () => { }, () => audioUpdates++));
        var screenChanges = new List<(uint Mode, bool Remember)>(); int accepts = 0;
        router.BindScreenMode(() => screen.Flags, (mode, remember) => { screenChanges.Add((mode, remember)); Assert.True(screen.SetMode(mode)); }, () => accepts++);
        var driver = new HostFrameDriver(screen, queue); driver.BindInputHandler(router.Dispatch);
        var help = menu.ControlPoint("Help")!.Value;
        menu.Route(new(InputCode.LeftButton, help.X, help.Y)); menu.Route(new(InputCode.LeftButton | InputCode.Release, help.X, help.Y)); router.Tick();
        var column = Assert.Single(menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.Equal(1u, column.ItemCount); Assert.Equal(HostPopupMenu.SelectCommand, column.Items[0].Command);
        var position = menu.Tree.Collection.Position(menu.Tree.Create.Entries[column.Id]); int x = position.X + 8, y = position.Y + (int)column.Offsets[0] + 3;
        router.NotifyPointer(new(0, x, y)); queue.Push(InputCode.LeftButton, x, y); Assert.False(driver.ProcessInput()); router.Tick();
        Assert.Equal("About", System.Text.Encoding.Latin1.GetString(startup.TellState.Current).Split('\0')[0]);
        windows.CloseAll();
        int oldDemo = startup.Configuration.GetInt("autoDemo");
        router.Post(new("Menu", "options")); router.Tick();
        column = Assert.Single(menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.Equal(12u, column.ItemCount);
        position = menu.Tree.Collection.Position(menu.Tree.Create.Entries[column.Id]); x = position.X + 8; y = position.Y + (int)column.Offsets[9] + 3;
        router.NotifyPointer(new(0, x, y)); Assert.Equal(9, column.Selection); queue.Push(InputCode.LeftButton, x, y); Assert.False(driver.ProcessInput()); router.Tick();
        Assert.Equal(oldDemo == 0 ? 1 : 0, startup.Configuration.GetInt("autoDemo")); router.FinishFrame();
        router.Post(new("Menu", "resolution")); router.Tick();
        column = Assert.Single(menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.Equal(6u, column.ItemCount);
        windows.CloseAll(); router.Post(new("Menu", "soundvolume")); router.Tick();
        column = Assert.Single(menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>());
        Assert.Equal(5u, column.ItemCount);
        position = menu.Tree.Collection.Position(menu.Tree.Create.Entries[column.Id]); x = position.X + 8; y = position.Y + (int)column.Offsets[1] + 3;
        router.NotifyPointer(new(0, x, y)); queue.Push(InputCode.LeftButton, x, y); Assert.False(driver.ProcessInput()); router.Tick();
        Assert.Equal(2, startup.Configuration.GetInt("soundVolume")); Assert.Equal(1, audioUpdates);
        windows.CloseAll();
        router.Post(new("ScreenModeAbsChange", "10")); Assert.False(driver.ProcessInput()); Assert.Equal((10u, true), Assert.Single(screenChanges));
        uint before = screen.Flags; queue.PushCommand(0x02000002, 0, (int)ScreenMode.Clouds); Assert.False(driver.ProcessInput());
        Assert.Equal((before ^ ScreenMode.Clouds, false), screenChanges[1]);
        Assert.Equal("DirectDrawVerify", System.Text.Encoding.Latin1.GetString(startup.TellState.Current).Split('\0')[0]);
        windows.CloseAll(); queue.PushCommand(0x020000e4, 0, 1); Assert.False(driver.ProcessInput()); Assert.Equal(1, accepts);
        Assert.Equal("AdjustDirectDraw", System.Text.Encoding.Latin1.GetString(startup.TellState.Current).Split('\0')[0]); windows.CloseAll();
    }

    /// <summary>실제 Edit 버튼은 같은 Fort 선택·파일 목록·Tell 메뉴까지 도달한다.</summary>
    [Fact]
    public void HostEditOpensRealMapListAndQueuesSelection()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        HostMenuInput? router = null;
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        { Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 }, action => router!.Post(action));
        var queue = new InputQueue(); var strings = new ScriptStringRing();
        var windows = new HostHtmlWindows(menu, queue, () => 1, _ => throw new InvalidOperationException());
        windows.BindParser(startup, raw => ScriptCommand.QueueScriptCommand(raw, queue, strings, new() { Lookup = DialogScriptCommands.Lookup, Unknown = _ => throw new InvalidOperationException() }));
        var text = new HostHtmlText(menu, windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { }); text.Bind(windows);
        var menus = new HostHtmlMenus(menu, windows, text, startup, screen, (_, _) => { }); menus.BindInput(queue, _ => { }, (_, _) => { });
        using var factory = new HostHtmlFactory(menu, windows, menus, startup, screen, queue, strings, () => 1, _ => 0, _ => throw new InvalidOperationException());
        router = new HostMenuInput(menu, windows, factory, queue, strings, () => { }, action => throw new InvalidOperationException(action.ToString()));
        var editor = new HostEditorSelection(startup, strings, router.ShowTell);
        factory.BindMapCatalog(editor.Catalog, menus, strings, () => editor.CurrentName, () => editor.PendingName); router.BindEditor(editor);
        var driver = new HostFrameDriver(screen, queue); driver.BindInputHandler(router.Dispatch);
        var point = menu.ControlPoint("Edit")!.Value;
        menu.Route(new(InputCode.LeftButton, point.X, point.Y)); menu.Route(new(InputCode.LeftButton | InputCode.Release, point.X, point.Y)); Assert.False(driver.ProcessInput());
        Assert.Equal("LoadEditable", System.Text.Encoding.Latin1.GetString(startup.TellState.Current).Split('\0')[0]); Assert.Equal(2u, editor.State.Mode);
        var columns = menu.Tree.Create.Entries.Select(token => menu.Tree.Node(token)).OfType<MenuGumpNode>().ToArray();
        Assert.True(columns.Sum(column => (int)column.ItemCount) > 0);
        var first = columns.First(column => column.ItemCount != 0); uint parameter = first.Items[0].Parameter;
        Assert.Equal(0x020000cau, first.Items[0].Command); string target = strings.Resolve(parameter);
        var position = menu.Tree.Collection.Position(menu.Tree.Create.Entries[first.Id]); int x = position.X + 8, y = position.Y + (int)first.Offsets[0] + 3;
        router.NotifyPointer(new(0, x, y)); queue.Push(InputCode.LeftButton, x, y); Assert.False(driver.ProcessInput());
        Assert.Equal(target, editor.PendingName); Assert.Equal(1u, editor.State.Next[0]); Assert.Throws<InvalidOperationException>(() => editor.Poll());
        windows.CloseAll();
    }

    /// <summary>여덟 실제 버튼의 눌림·해제를 원본 상위 명령과 인자로 전달한다.</summary>
    [Fact]
    public void MainMenuRoutesEachButtonToItsOriginalAction()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var actions = new List<MainMenuAction>(); int sounds = 0;
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => sounds++,
            Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        }, actions.Add);
        // 실제 논리 화면의 각 버튼 내부에서 누른 뒤 같은 위치에서 뗀다.
        for (int index = 0; index < 8; index++)
        {
            int x = 1024 / 2 - 156 + (index % 4) * 79 + 3, y = 768 / 2 - 73 + (index / 4) * 23 + 3;
            Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton, x, y)));
            Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton | InputCode.Release, x, y)));
        }
        Assert.Equal(new MainMenuAction[] { new("Tell", "UCampaign"), new("Tell", "NotImplemented"), new("Tell", "DEMOVILLE"),
            new("Menu", "help"), new("AskEditFort", "0"), new("Tell", "Credits"), new("Menu", "options"), new("RequestQuitApp", "0") }, actions);
        Assert.Equal(8, sounds);
        // 버튼 밖에서 떼면 캡처를 풀고 명령은 발생하지 않는다.
        Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton, 360, 315)));
        Assert.NotEqual(0U, menu.Route(new(InputCode.LeftButton | InputCode.Release, 2, 2)));
        Assert.Equal(8, actions.Count);
    }

    /// <summary>실제 Campaign·Demo·Credits 버튼 명령이 같은 시작 설정의 Tell 본문에 도달한다.</summary>
    [Fact]
    public void MenuTellActionsReadActualSections()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var bodies = new List<string>(); var names = new List<string>();
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { },
            Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        }, action =>
        {
            Assert.Equal("Tell", action.Command); names.Add(action.Argument);
            var result = startup.ShowTell(action.Argument, 0, null, (body, _, _, _) => { bodies.Add(body); return 1; });
            Assert.Equal((true, 1u), result);
        });
        // 원본 상위 Tell을 쓰는 세 버튼만 실제 좌표에서 차례로 누른다.
        foreach (string label in new[] { "Campaign", "Demo", "Credits" })
        {
            ScreenPoint point = menu.ControlPoint(label)!.Value;
            menu.Route(new(InputCode.LeftButton, point.X, point.Y));
            menu.Route(new(InputCode.LeftButton | InputCode.Release, point.X, point.Y));
        }
        Assert.Equal(new[] { "UCampaign", "DEMOVILLE", "Credits" }, names);
        Assert.All(bodies, body => Assert.NotEmpty(body));
        Assert.Equal("DEMOVILLE", startup.Configuration.SubstituteLocal("{global.priorTell}"));
    }

    /// <summary>메뉴 생성 뒤 연결한 실제 Gump 경계로 스크립트 라벨 클릭이 상위 명령에 도달한다.</summary>
    [Fact]
    public void ScriptLocatorUsesLiveMenuButtons()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data); var screen = new Screen(1024, 768);
        Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet)); startup.Draw(screen);
        var queue = new InputQueue(); var actions = new List<MainMenuAction>();
        var runner = new UiScriptRunner(UiScript.Parse(["1\tclick\tCampaign", "1\tclick-if\tMissing"]), queue, screen);
        var driver = new HostFrameDriver(screen, queue, runner);
        var menu = new HostMainMenu(startup, screen, new GumpButtonTreeHooks
        {
            Now = () => 1, Poll = _ => new(InputCode.Release, 0, 0), Sound = () => { },
            Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0
        }, actions.Add);
        driver.BindControlLocator(menu);
        driver.BindInputHandler(input => { menu.Route(input); return false; });
        Assert.Equal(new ScreenPoint(393, 320), menu.ControlPoint("Campaign"));
        Assert.Null(menu.ControlPoint("campaign")); Assert.Null(menu.ControlPoint("Missing"));
        driver.BeginFrame(); Assert.False(driver.ProcessInput()); driver.EndFrame(); driver.Finish();
        Assert.Equal(new[] { new MainMenuAction("Tell", "UCampaign") }, actions);
    }

}
