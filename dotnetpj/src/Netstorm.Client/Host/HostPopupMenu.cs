using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Host;

/// <summary>cpppj 상위 메뉴의 실제 controlled 팝업과 선택 명령을 소유한다.</summary>
public sealed class HostPopupMenu
{
    /// <summary>팝업에서 선택한 행을 상위 명령으로 변환하는 전용 코드다.</summary>
    public const uint SelectCommand = 0x01ffffff;
    /// <summary>원본 큐의 명령 마스크를 거친 선택 사건 코드다.</summary>
    public const uint QueuedSelectCommand = (SelectCommand & 0x0100ffff) | InputCode.Command;
    private readonly HostMainMenu _root;
    private readonly HostHtmlWindows _windows;
    private readonly HostHtmlMenus _menus;
    private readonly Screen _screen;
    private readonly HostStartupScene _startup;
    private readonly Action _applyAudio;
    private readonly Action _changed;
    private MainMenuAction[] _actions = [];
    /// <summary>팝업 행의 실제 명령 인자로 쓰는 공유 문자열 고리다.</summary>
    private readonly ScriptStringRing _strings;

    /// <summary>같은 공통 Gump 트리와 메뉴 수명·화면을 사용한다.</summary>
    public HostPopupMenu(HostMainMenu root, HostHtmlWindows windows, HostHtmlMenus menus, Screen screen, HostStartupScene startup, Action changed, Action applyAudio, ScriptStringRing strings)
    { _root = root; _windows = windows; _menus = menus; _screen = screen; _startup = startup; _changed = changed; _applyAudio = applyAudio; _strings = strings ?? throw new ArgumentNullException(nameof(strings)); }

    /// <summary>Help·설정 및 해상도/음량 팝업을 같은 Gump에 부착한다.</summary>
    public bool Open(string name)
    {
        var rows = new List<(string Label, MainMenuAction Action, bool Checked, bool Enabled)>();
        void Add(string label, string command, string argument, bool check = false, bool enabled = true) => rows.Add((label, new(command, argument), check, enabled));
        int Get(string key) => _startup.Configuration.GetInt(key);
        switch (name.ToLowerInvariant())
        {
            case "help": OpenHelp(); return true;
            case "options":
                if ((_screen.Flags & ScreenMode.FullScreen) != 0) { Add("Adjust Direct Draw", "Tell", "AdjustDirectDraw"); Add("Window Mode", "ScreenModeAbsChange", "10"); }
                else Add("Direct Draw / Full Screen", "Tell", "DirectDrawManager");
                Add("Resolution >", "Menu", "resolution");
                Add("Sound On", "Toggle", "sound", Get("sound") != 0); Add("Play Music", "Toggle", "music", Get("music") != 0);
                Add("Wind Noise", "Toggle", "playWind", Get("playWind") != 0); Add("Speaker Swap L/R", "Toggle", "swapLeftRightSpeakers", Get("swapLeftRightSpeakers") != 0);
                Add("Sound Effect Volume >", "Menu", "soundvolume"); Add("Music Volume >", "Menu", "musicvolume");
                Add("Edge Scroll in Fullscreen", "Toggle", "edgeScroll", Get("edgeScroll") != 0);
                Add("Auto-Demo", "Toggle", "autoDemo", Get("autoDemo") != 0); Add("Tell Tips at Startup", "Toggle", "tellTips", Get("tellTips") != 0);
                Add("Pass Server Diagnostic", "", "", false, false); break;
            case "resolution":
                foreach (var (width, height) in new[] { (640, 480), (800, 600), (1024, 768), (1280, 720), (1280, 800), (1366, 768) })
                    Add($"{width} by {height}", "Tell", $"SETSIZE({width},{height})", _screen.Width == width && _screen.Height == height);
                break;
            case "soundvolume": case "musicvolume":
                string key = name.Equals("soundvolume", StringComparison.OrdinalIgnoreCase) ? "soundVolume" : "musicVolume";
                for (int i = 1; i <= 5; i++) Add($"Volume {i}", "Volume", $"{key}={i}", Get(key) == i);
                break;
            default: return false;
        }
        _windows.RequestCloseAll();
        _actions = rows.Select(row => row.Action).ToArray();
        _menus.Widgets.BeginVertical();
        for (int i = 0; i < rows.Count; i++) _menus.Controls.AddChecked(rows[i].Checked, Encoding.ASCII.GetBytes(rows[i].Label), SelectCommand, (uint)i + 1, true, rows[i].Enabled);
        _menus.Menu.AttachCurrent(_screen.Width / 2 + 2, _screen.Height / 2 - 27, _root.Tree.Create.Entries[0]);
        _changed(); return true;
    }

    /// <summary>Help 팝업의 실제 3행(일반·기술·Version)을 앵커 버튼에 붙인다.
    /// 근거: cpppj/src/client/UberGump.cpp 1211~1229행의 <c>OpenMainMenuPopup(false)</c>.</summary>
    /// <remarks>2026-11-12 0.383에서 L4a menu-help 비교로 옛 1행(About) 자리표시를 실제 구성으로 바꿨다.
    /// 영어는 번역 그대로 쓰며(D4) 한국어·일본어 번역은 R8 6차·13차에서 잇는다.
    /// 원문의 깊이 조정(<c>AddDepth(-90-depth)</c>)과 모달 지정은 R3 잔여로 미뤘다. 모달을 붙이면
    /// <c>HostGumpDrawing</c>의 모달 게이트가 팝업 아래 버튼까지 숨기므로 게이트도 함께 손봐야 한다(0.382 기록).
    /// 행 선택의 실제 명령은 큐에 그대로 넣는다. 0x17 도움말 주제 창과 0x49 기술 도움말 외부 실행은
    /// R3 잔여·외부 경계라 <c>HostMenuInput</c>에서 명시 예외다.</remarks>
    private void OpenHelp()
    {
        _windows.RequestCloseAll();
        _menus.Widgets.BeginVertical();
        uint topic = _strings.Store("F1Help");
        _menus.Controls.AddText(Encoding.ASCII.GetBytes("General Help - ~iF1~."), 0x02000017U, 0x70000000U | topic);
        _menus.Controls.AddText(Encoding.ASCII.GetBytes("Technical Help"), 0x02000049U, 0, true, TechnicalHelpAvailable());
        _menus.Controls.AddText(Encoding.ASCII.GetBytes("Version"), 0x02000095U, _strings.Store("About"));
        uint current = _menus.Menu.TakeCurrent();
        ScreenRect anchor = _root.ControlBounds("Help") ?? throw new InvalidOperationException("메인 Help의 현재 버튼 없음");
        // 원문 앵커 공식(baseWidth*0.5+x, y)을 같은 실수 연산으로 둔다(RawMenuControls.cpp 1047~1048행).
        float x = anchor.Left + (anchor.Right - anchor.Left) * 0.5f;
        _menus.Widgets.Attach(current, x, anchor.Top, _root.Tree.Create.Entries[0], 0);
        _menus.Menu.AddMenuFlags(current, 0x04004000U);
        _changed();
    }

    /// <summary>기술 도움말(외부 HLP 실행기) 행의 사용 가능 여부를 같은 설정·파일 장치로 판정한다.
    /// 근거: cpppj/src/client/TechnicalHelp.cpp 13~18행(두 설정 읽기+파일 해석).</summary>
    /// <returns>실행기와 도움말 파일이 모두 있으면 참이다.</returns>
    /// <remarks>2026-11-12 0.383에서 Help 팝업 행의 사용·사용 안 함을 맞추려고 추가했다.
    /// 원문의 FileSpec 장치·아카이브·경로 해석 순서와 완전히 같은 판정은 R3 잔여이며 여기서는 읽기 가능만 확인한다.</remarks>
    private bool TechnicalHelpAvailable()
    {
        string? exe = _startup.Configuration.Configuration.Get("techHelpExe");
        string? file = _startup.Configuration.Configuration.Get("techHelpFile");
        if (string.IsNullOrEmpty(exe) || string.IsNullOrEmpty(file)) return false;
        return _startup.Files.TryReadAllBytes(exe) is not null && _startup.Files.TryReadAllBytes(file) is not null;
    }

    /// <summary>현재 설정을 변경하는 팝업 행동을 실행하고 실제 음향 경계에 반영한다.</summary>
    public bool Execute(MainMenuAction action)
    {
        if (action.Command == "Toggle")
        {
            _startup.Configuration.SetInt(action.Argument, _startup.Configuration.GetInt(action.Argument) == 0 ? 1 : 0);
            if (action.Argument is "sound" or "music" or "swapLeftRightSpeakers") _applyAudio();
        }
        else if (action.Command == "Volume")
        {
            int equals = action.Argument.IndexOf('=');
            if (equals < 0) throw new ArgumentException("음량 설정 인자 오류");
            _startup.Configuration.SetInt(action.Argument[..equals], Netstorm.Assets.ConfigNumbers.ParseLong(action.Argument[(equals + 1)..])); _applyAudio();
        }
        else return false;
        _windows.RequestCloseAll(); _changed(); return true;
    }

    /// <summary>현재 행 번호를 원본 상위 메뉴 행동으로 반환한다.</summary>
    public MainMenuAction Select(uint index)
    {
        if (index == 0 || index > _actions.Length) throw new ArgumentOutOfRangeException(nameof(index), "현재 팝업 행 범위 오류");
        return _actions[index - 1];
    }
}
