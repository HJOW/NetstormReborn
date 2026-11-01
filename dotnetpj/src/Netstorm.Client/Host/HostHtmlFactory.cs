using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Host; // 실제 Tell과 HTML 전체 창 생성의 공통 소유 영역이다.

/// <summary>현재 Config·입력 큐·공통 트리와 HTML 본문·메뉴를 전체 창 생성기에 연결한다.
/// 근거: cpppj/src/client/RawMenuHtml.inc와 UberGump.cpp의 LoadingHtmlContext.</summary>
public sealed class HostHtmlFactory : IDisposable
{
    /// <summary>현재 설정과 Tell 섹션 소유자.</summary>
    private readonly HostStartupScene _startup;
    /// <summary>화면보다 오래 보유하는 파일별 Config 목록.</summary>
    private readonly TellFileMenus _fileMenus;
    /// <summary>현재 같은 실제 HTML 전체 생성기.</summary>
    public HtmlWindowFactory Factory { get; }
    /// <summary>실제 메타 처리기. 맵·파일·기술 목록 장치는 호출자가 같은 후크에 연결한다.</summary>
    public HtmlWindowMetadataHooks MetadataHooks { get; }

    /// <summary>같은 호스트 화면과 창·메뉴·설정·입력 수명에 전체 생성기를 설치한다.</summary>
    public HostHtmlFactory(HostMainMenu menu, HostHtmlWindows windows, HostHtmlMenus menus, HostStartupScene startup,
        Screen screen, InputQueue queue, ScriptStringRing strings, Func<double> wallSeconds,
        Func<uint, uint> menuGlobal, Action<string> unknown)
    {
        ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(windows); ArgumentNullException.ThrowIfNull(menus);
        ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(queue);
        ArgumentNullException.ThrowIfNull(strings); ArgumentNullException.ThrowIfNull(wallSeconds); ArgumentNullException.ThrowIfNull(menuGlobal);
        ArgumentNullException.ThrowIfNull(unknown); _startup = startup;
        _fileMenus = new TellFileMenus(startup.Configuration, startup.Files.MatchFinder);
        var parser = windows.Parser ?? throw new InvalidOperationException("HTML 전체 생성 전 파서 연결 필요");
        var tree = menu.Tree;
        uint Read(uint address) => address switch
        {
            0x531860 or 0x5c78f0 => unchecked((uint)screen.Width),
            0x531864 or 0x5c78f4 => unchecked((uint)screen.Height),
            0x563e38 => tree.Create.Entries[0], 0x5429ec => 0x020000be,
            0x5cab30 or 0x55a480 => menuGlobal(address),
            _ => throw new ArgumentOutOfRangeException(nameof(address), "HTML 전체 생성 전역 미연결")
        };
        MetadataHooks = new HtmlWindowMetadataHooks
        {
            Node = token => tree.Node(token) as HtmlWindowGumpNode,
            Commands = new() { Lookup = DialogScriptCommands.Lookup, Unknown = unknown }, WallSeconds = wallSeconds,
            Read = Read, Queue = (code, first, second) => queue.Push(code, unchecked((int)first), unchecked((int)second))
        };
        HtmlWindowMetadata.BindHtmlWindowMetadataDevices(MetadataHooks, windows.Window, strings, tree.Lifetime);
        menus.BindMetadata(MetadataHooks);
        void AppendFiles(string pattern, uint command, string argument, bool fallback) =>
            _fileMenus.Append(pattern, command, argument, fallback, strings, item =>
            {
                if (item.Separator) menus.Controls.AddSeparator();
                else menus.Controls.AddText(Encoding.Latin1.GetBytes(item.Label), item.Command, item.Parameter, true, true, false);
            });
        MetadataHooks.NamedList = (pattern, command, argument) => AppendFiles(pattern, command, argument, true);
        MetadataHooks.FileList = (pattern, command, argument) => AppendFiles(pattern, command, argument, false);
        MetadataHooks.InlineList = _fileMenus.Inline;
        var metadata = new HtmlWindowMetadata(MetadataHooks);
        var hooks = new HtmlWindowFactoryHooks
        {
            Create = windows.Create, Node = MetadataHooks.Node,
            Substitute = (raw, _) => Raw(startup.Configuration.SubstituteLocal(Text(raw))),
            Fallback = () => Raw(startup.Configuration.PathSpec("OkButton")), WallSeconds = wallSeconds,
            Attach = (token, x, y, parent, mode) => tree.TreeHooks.Attach!(token, x, y, parent, mode),
            Modal = tree.Modal.SetModal, Read = Read
        };
        HtmlWindowFactory.BindHtmlWindowFactoryDevices(hooks, windows.Window, metadata, parser, tree.Lifetime);
        Factory = new HtmlWindowFactory(windows.State, hooks);
    }

    /// <summary>원본 BYTE 표현을 Config의 문자열로 해독한다.</summary>
    private static string Text(string raw) => OriginalText.DecodeWindows1252(Encoding.Latin1.GetBytes(raw));
    /// <summary>Config의 문자열을 원본 BYTE 표현으로 바꾼다.</summary>
    private static string Raw(string text) => Encoding.Latin1.GetString(OriginalText.EncodeWindows1252(text));

    /// <summary>원본 BYTE 설정 명령을 같은 실제 Config에 적용한다.</summary>
    public void ApplyConfig(string source) => ScriptCommand.ApplyScriptConfig(source,
        (key, value) => _startup.Configuration.Set(Text(key), Text(value)));

    /// <summary>같은 실제 맵 목록·선택 이름·문자열 고리로 Tell의 맵 행을 연결한다.</summary>
    public void BindMapCatalog(SoloMapCatalog catalog, HostHtmlMenus menus, ScriptStringRing strings, Func<string> current, Func<string> fallback)
    {
        var hooks = TellMapMenus.Bind(catalog.List, catalog.State, catalog.SelectFileSpec, current, fallback,
            flags => menus.Widgets.BeginVertical(flags), (row, command) =>
                menus.Controls.AddChecked(row.Checked, Encoding.Latin1.GetBytes(row.Label), command, strings.Store(row.Name), true, row.Enabled));
        HtmlWindowMetadata.BindHtmlWindowMetadataMaps(MetadataHooks, hooks);
    }

    /// <summary>현재 Tell 설정과 선택 미션에서 본문을 읽어 실제 HTML 창을 만든다.</summary>
    /// <param name="target">Tell 이름과 선택적 매개변수.</param><param name="suppress">창 생략 값.</param>
    /// <param name="mission">현재 미션의 선택적 섹션 조회.</param>
    public (bool Found, uint Serial) ShowTell(string target, uint suppress = 0, Func<string, string?>? mission = null) =>
        _startup.ShowTell(target, suppress, mission, (body, hide, width, height) => Factory.Show(Raw(body), hide, width, height));

    /// <summary>파일 목록이 만든 장기 설정 객체를 Client 종료 때 반납한다.</summary>
    public void Dispose() => _fileMenus.Dispose();
}
