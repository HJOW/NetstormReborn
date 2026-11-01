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
