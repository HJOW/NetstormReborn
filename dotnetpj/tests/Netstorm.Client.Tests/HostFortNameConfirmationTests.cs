using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Host;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;
using static Netstorm.Client.Tests.TemplateStorageTestSupport;

namespace Netstorm.Client.Tests;

/// <summary>
/// Save as(현재 Fort 모드)의 이름 확인이 세션이 주입하는 Template 소유자 헤더와 현재 이름 설정 장치로 이어지는 경로를 실제 저장소와
/// 현재 Fort 이름 장치(<see cref="FortCurrentNameDevices"/>)로 검사한다. 두 장치를 주입하지 않으면 해당 지점에서 명시 오류로 멈추는지도 확인한다.
/// </summary>
/// <remarks>
/// 2026-11-02 TS6에서 추가했다. 이 호스트에는 아직 월드 세션(F3)이 없어 현재 Template 소유자와 전체 snapshot을 이 검사의 장면이 대신 공급한다.
/// 원본 x86 fixture가 없는 어댑터 계층이라 L1이 아니다.
/// </remarks>
public sealed class HostFortNameConfirmationTests
{
    /// <summary>
    /// 호스트 공통 UI(트리·창·문자 입력)와 편집 작업본을 만든다. 장치 주입 여부만 검사마다 다르다.
    /// </summary>
    private sealed class HostScene : IDisposable
    {
        /// <summary>임시 작업본 루트다.</summary>
        public readonly EditorTempRoot Temp = new();

        /// <summary>실제 시작 장면(설정·파일)이다.</summary>
        public readonly HostStartupScene Startup;

        /// <summary>실제 공통 Gump 트리를 가진 메인 메뉴다.</summary>
        public readonly HostMainMenu Menu;

        /// <summary>HTML 창 소유자다.</summary>
        public readonly HostHtmlWindows Windows;

        /// <summary>입력 큐다.</summary>
        public readonly InputQueue Queue = new();

        /// <summary>문자 입력 소유자다.</summary>
        public readonly HostTextEntry Entry;

        /// <summary>이름 확인 창을 여는 호스트 장치다.</summary>
        public HostSaveFortDialog Dialog = null!;

        /// <summary>목록과 이름 확인이 공유하는 편집 선택기다.</summary>
        public readonly HostEditorSelection Editor;

        /// <summary>Tell 안내 창 이름들이다.</summary>
        public readonly List<string> Notices = [];

        /// <summary>문자열 고리다.</summary>
        public readonly ScriptStringRing Strings = new();

        /// <summary>실제 자산과 공통 트리를 만든다.</summary>
        public HostScene()
        {
            string? data = GameDataLocator.FindDataDirectory();
            Assert.NotNull(data);
            Startup = new HostStartupScene(data);
            Screen screen = new(1024, 768);
            Assert.True(screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet));
            Startup.Draw(screen);
            Menu = new HostMainMenu(Startup, screen, new GumpButtonTreeHooks
            { Now = () => 1, Poll = _ => default, Sound = () => { }, Redraw = (_, _) => { }, HandleBase = (_, _, _, _) => 0 });
            Windows = new HostHtmlWindows(Menu, Queue, () => 1, _ => { });
            HostHtmlText text = new(Menu, Windows, screen, _ => default, (_, _) => { }, _ => { }, _ => { });
            text.Bind(Windows);
            Entry = new HostTextEntry(Menu, Windows, text, screen, () => 1, (_, _) => { });
            Editor = new HostEditorSelection(Startup, Strings, Notices.Add, workspace: new EditorCatalogWorkspace(Startup.Files, Temp.Work));
            _text = text;
            _screen = screen;
        }

        /// <summary>HTML 글자 소유자다.</summary>
        private readonly HostHtmlText _text;

        /// <summary>논리 화면이다.</summary>
        private readonly Screen _screen;

        /// <summary>이름 확인 장치(주입 여부는 호출자가 정한다)와 연결한 이름 창을 만든다.</summary>
        /// <param name="header">현재 Template 소유자 헤더 장치.</param>
        /// <param name="setName">현재 Fort 이름 설정 장치.</param>
        public void BindDialog(Func<Memory<byte>>? header, Action<string>? setName)
        {
            HostFortNameConfirmation confirm = new(Startup, Menu, Entry, Queue, Strings, Editor.Workspace, Notices.Add, header, setName);
            Dialog = new HostSaveFortDialog(Menu, Windows, _text, Entry, Startup, _screen, confirm.Confirm, Notices.Add);
        }

        /// <summary>열려 있는 창과 임시 루트를 정리한다.</summary>
        public void Dispose()
        {
            Windows.CloseAll();
            Entry.Dispose();
            Editor.Dispose();
            Temp.Dispose();
        }
    }

    /// <summary>현재 Fort 모드의 Save as는 소유자 헤더의 isMine을 지우고 같은 이름을 실제 현재 이름 장치로 전달해 manager 이름과 작업 파일을 바꾼다.</summary>
    [Fact]
    public void SaveAsInCurrentFortModeReachesCurrentNameDevices()
    {
        using HostScene scene = new();
        EditorFiles work = scene.Editor.Workspace.Files;
        CampaignTemplateStorage storage = new();
        ClientFortGlobals names = new();
        const uint owner = 1;
        byte[] original = RealFortBytes("TEST01");
        string oldPath = work.Prepare("Before", original);
        storage.InstallFile(owner, FortTemplate.ReadSections(original), original, oldPath);
        storage.Headers.Span[(int)owner * 20 + 8] = 1;
        SetCurrent(names, "Before");
        uint depth = 0;
        int saves = 0;
        List<string> snapshots = [];
        RawPlayerTemplateState state = new(OriginalEdition.Patch1078, storage.Headers, slot => storage.SelectState(slot), slot => storage.Name(slot),
            () => depth, value => depth = value, () => false, _ => throw new InvalidOperationException("예상 밖 State 진단"), _ => throw new InvalidOperationException("예상 밖 CD 경계"));
        state.BindMinimumSize((slot, size) => storage.EnsureStateSize(slot, size));
        state.BindFileSave(slot =>
        {
            saves++;
            storage.SaveFile(slot, new DataManagerFileSaveState { ReadOnlyForts = () => 0, LocalOwner = () => owner, HiddenPlayers = () => new byte[9] },
                new FortImageHooks { TypeCount = () => 188, Assertion = _ => throw new InvalidOperationException("예상 밖 이미지 진단") }, work.Writer());
        });
        FortCurrentNameGlobals globals = new()
        {
            Owner = () => owner, Fort = () => 1, Editor = () => 1, Mission = () => 0,
            Dirty = () => names.Catalog.Dirty, SetDirty = value => names.Catalog.Dirty = value
        };
        FortCurrentNameHooks hooks = new()
        {
            Snapshot = () => snapshots.Add(names.Current()), Notify = slot => state.Changed(slot), RefreshTitle = () => { },
            Assertion = line => throw new InvalidOperationException("예상 밖 이름 진단 " + line), State = slot => state.Get(slot)
        };
        FortCurrentNameDevices devices = new(names, storage, scene.Startup.Configuration, globals, hooks, work, scene.Editor.Workspace.Resolve);
        scene.BindDialog(() => storage.Headers[((int)owner * 20)..((int)owner * 20 + 20)],
            name => devices.Set([.. Encoding.UTF8.GetBytes(name), 0]));
        uint token = scene.Dialog.Show(1, 1, "Fresh Name");
        Assert.Equal(1u, scene.Menu.Tree.EventHooks.Handle!(token, 0x02000038, 0, 0));
        // 실제 설정의 fortSpec 이름 변환과 작업본 경로 대응을 그대로 따른다
        string expected = scene.Editor.Workspace.Resolve(scene.Startup.Configuration.PathSpec("fortSpec", "Fresh Name"));
        Assert.Equal("Fresh Name", names.Current());
        Assert.Equal(expected, storage.Name(owner));
        Assert.Equal(0, storage.Headers.Span[(int)owner * 20 + 8]);
        Assert.Equal(["Before", "Fresh Name"], snapshots);
        Assert.Equal(1, saves);
        Assert.True(File.Exists(expected));
        Assert.False(File.Exists(oldPath));
        Assert.Empty(scene.Notices);
        scene.Menu.Tree.Lifetime.Cleanup();
    }

    /// <summary>소유자 헤더 장치를 주입하지 않으면 현재 Fort 모드의 확인은 그 지점에서 명시 오류로 멈춘다.</summary>
    [Fact]
    public void SaveAsWithoutHeaderDeviceStopsExplicitly()
    {
        using HostScene scene = new();
        scene.BindDialog(null, null);
        uint token = scene.Dialog.Show(1, 1, "No Owner");
        Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.EventHooks.Handle!(token, 0x02000038, 0, 0));
    }

    /// <summary>헤더만 주입하고 이름 설정 장치가 없으면 이름 설정 지점에서 멈추고, 그 전에 쓰는 헤더 isMine 지우기까지만 일어난다.</summary>
    [Fact]
    public void SaveAsWithoutNameDeviceStopsAfterHeaderClear()
    {
        using HostScene scene = new();
        byte[] header = new byte[20];
        header[0] = 5;
        header[8] = 1;
        scene.BindDialog(() => header, null);
        uint token = scene.Dialog.Show(1, 1, "No Name");
        Assert.Throws<InvalidOperationException>(() => scene.Menu.Tree.EventHooks.Handle!(token, 0x02000038, 0, 0));
        Assert.Equal(5, header[0]);
        Assert.Equal(0, header[8]);
    }

    /// <summary>현재 이름 버퍼의 명시 입력만 준비한다(검사 장면 도우미).</summary>
    /// <param name="names">이름 전역.</param>
    /// <param name="value">현재 이름.</param>
    private static void SetCurrent(ClientFortGlobals names, string value) =>
        FortSelection.SetPendingFortName(names.CurrentBytes(), [.. Encoding.UTF8.GetBytes(value), 0]);
}
