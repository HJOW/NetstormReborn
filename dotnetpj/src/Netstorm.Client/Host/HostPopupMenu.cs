using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

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
    /// <summary>음향 장치 준비 판정이다. 거짓이면 소리 행을 사용 안 함으로 그린다.</summary>
    private readonly Func<bool> _soundReady;

    /// <summary>같은 공통 Gump 트리와 메뉴 수명·화면을 사용한다.</summary>
    public HostPopupMenu(HostMainMenu root, HostHtmlWindows windows, HostHtmlMenus menus, Screen screen, HostStartupScene startup, Action changed, Action applyAudio, ScriptStringRing strings, Func<bool>? soundReady = null)
    { _root = root; _windows = windows; _menus = menus; _screen = screen; _startup = startup; _changed = changed; _applyAudio = applyAudio; _strings = strings ?? throw new ArgumentNullException(nameof(strings)); _soundReady = soundReady ?? (() => true); }

    /// <summary>Help·설정 팝업을 같은 Gump에 부착한다. 하위 메뉴는 원문처럼 미리 만들어 부모 행에 붙인다.</summary>
    public bool Open(string name)
    {
        switch (name.ToLowerInvariant())
        {
            case "help": OpenHelp(); return true;
            case "options": OpenOptions(); return true;
            default: return false;
        }
    }

    /// <summary>Options 팝업의 실제 전체 행을 앵커 버튼에 붙인다.
    /// 근거: cpppj/src/client/RawGameMenus.cpp 107~160행의 <c>Options()</c>를 메인 메뉴 맥락(미션·전투·편집 없음)으로 옮긴 것이다.</summary>
    /// <remarks>2026-11-12 0.384에서 L4a menu-options 비교로 옛 12행 자리표시를 실제 구성으로 바꿨다.
    /// Language·Frame Rate·Difficulty 하위 메뉴와 Anti-Aliasing·FSR 1.0 토글, 구분선 3개, 앵커 부착을 더했다.
    /// 하위 메뉴는 원문의 펼침 자식 대신 기존 방식의 중앙 팝업으로 연다(펼침 표시·입력은 R3 잔여).
    /// 후처리 토글 행은 항상 보이며 장치 판정은 R8 8차에서 잇는다. 마지막 진단 행은 원문처럼 항상 사용이다.</remarks>
    /// <summary>Options 팝업의 실제 전체 행을 앵커 버튼에 붙인다.
    /// 근거: cpppj/src/client/RawGameMenus.cpp 107~160행의 <c>Options()</c>를 메인 메뉴 맥락(미션·전투·편집 없음)으로 옮긴 것이다.</summary>
    /// <remarks>2026-11-12 0.384에서 L4a menu-options 비교로 옛 12행 자리표시를 실제 구성으로 바꿨다.
    /// Language·Frame Rate·Difficulty 하위 메뉴와 Anti-Aliasing·FSR 1.0 토글, 구분선 3개를 더했다.
    /// 하위 메뉴 6종은 원문처럼 미리 만들어 부모 행에 자식으로 붙인다(AddSubmenu). 자식 행의 실제 명령은 큐에 그대로 넣는다.
    /// 자식 펼침 선택 흐름도 원문과 같은 공통 메뉴 엔진을 쓰므로 펼침 장면의 픽셀도 같은 코드에서 나온다.
    /// 음량 선택은 기존 동작(설정 쓰기·음향 반영·팝업 닫기)을 유지하려고 <c>HostMenuInput</c>의 원시 분기에서
    /// <c>Execute</c>로 돌린다. 언어·프레임·난이도·해상도 선택 적용은 R8 해당 차수에서 잇는다.
    /// 후처리 토글 행은 항상 보이며 장치 판정은 R8 8차에서 잇는다. 마지막 진단 행은 원문처럼 항상 사용이다.</remarks>
    private void OpenOptions()
    {
        var actions = new List<MainMenuAction>();
        int items = 0;
        int Get(string key) => _startup.Configuration.GetInt(key);
        // 없는 설정은 원문처럼 0으로 읽는다(GetInt는 누락에 예외라 ReadInt로 보존한다).
        int GetOr(string key, int fallback) { int value = fallback; _startup.Configuration.ReadInt(key, ref value); return value; }
        // 부모 열의 선택 행이다. 선택 번호와 같은 순서로 행동 표를 채운다.
        void ParentChecked(bool check, byte[] label, MainMenuAction action, bool enabled = true)
        { _menus.Controls.AddChecked(check, label, SelectCommand, (uint)items + 1, true, enabled); actions.Add(action); items++; }
        void ParentAscii(string label, string command, string argument, bool check = false, bool enabled = true)
            => ParentChecked(check, Encoding.ASCII.GetBytes(label), new(command, argument), enabled);
        void ParentGap() { _menus.Controls.AddSeparator(); actions.Add(new("", "")); items++; }
        void ParentSubmenu(byte[] label, uint child, bool enabled = true)
        { _menus.Controls.AddSubmenu(label, child, enabled); actions.Add(new("", "")); items++; }
        // 자식 열의 실제 명령행이다. 선택하면 실제 명령이 큐에 들어가며 번호표는 쓰지 않는다.
        void ChildChecked(bool check, byte[] label, uint command, uint parameter, bool enabled = true)
            => _menus.Controls.AddChecked(check, label, command, parameter, true, enabled);
        // 자식 열을 새로 열어 행들을 채운 뒤 꺼낸다. 부모 열로 복귀한다.
        uint TakeChild(Action build)
        { _menus.Widgets.BeginVertical(); build(); return _menus.Menu.TakeCurrent(); }
        byte[] Utf8(string label) => Encoding.ASCII.GetBytes(label);

        _windows.RequestCloseAll();
        _menus.Widgets.BeginVertical();
        if ((_screen.Flags & ScreenMode.FullScreen) != 0) { ParentAscii("Adjust Direct Draw", "Tell", "AdjustDirectDraw"); ParentAscii("Window Mode", "ScreenModeAbsChange", "10"); }
        else ParentAscii("Direct Draw / Full Screen", "Tell", "DirectDrawManager");
        // 원문의 해상도 3행+호환 화면비 5행이다. 근거: cpppj ScreenResolutions.h의 kOriginal(3)+kCompatibility(5).
        // FSR 2행은 장치가 있을 때만 들어가며(같은 파일의 kFsrScreenResolutionChoices), 후처리 토글 행과 같이 항상 보인다(R8 8차에서 장치 판정을 잇는다).
        uint resolutions = TakeChild(() =>
        {
            foreach (var (width, height) in new[] { (640, 480), (800, 600), (1024, 768), (1280, 720), (1280, 800), (1366, 768), (1600, 900), (1920, 1080), (2560, 1440), (3840, 2160) })
                ChildChecked(_screen.Width == width && _screen.Height == height,
                    Utf8($"{width} by {height}"), 0x542948U, _strings.Store($"SETSIZE({width},{height})"));
        });
        ParentSubmenu(Utf8("Resolution"), resolutions);
        // 6차 한국어 항목이다. 각 언어 자신의 표기를 쓰고 표시 부호화로 바꾼다.
        // 현재 언어 번호 장치가 없어 영어를 기본 선택으로 두며(R8 6차·13차에서 잇는다).
        uint languages = TakeChild(() =>
        {
            ChildChecked(true, ExtendedText.EncodeDisplayText("English"), 0x542988U, 0);
            ChildChecked(false, ExtendedText.EncodeDisplayText("한국어"), 0x542988U, 7);
            ChildChecked(false, ExtendedText.EncodeDisplayText("日本語"), 0x542988U, 5);
        });
        ParentSubmenu(Utf8("Language"), languages);
        // 후순위 F1 항목이다. 설정이 없거나 범위 밖이면 원본 기본 75에 체크한다.
        uint frameRates = TakeChild(() =>
        {
            int configured = 0; _startup.Configuration.ReadInt("maxFPS", ref configured);
            int current = configured >= 1 && configured <= 1000 ? configured : 75;
            foreach (int rate in new[] { 60, 75, 120 })
                ChildChecked(rate == current, Utf8($"{rate} FPS" + (rate == 75 ? " (Original)" : "")),
                    0x542948U, _strings.Store($"Patch1101.SetFrameRate({rate})"));
        });
        ParentSubmenu(Utf8("Frame Rate"), frameRates);
        // 11차 난이도 항목이다. 설정이 없거나 범위 밖이면 보통에 체크한다.
        uint difficulty = TakeChild(() =>
        {
            int configured = 0; _startup.Configuration.ReadInt("difficulty", ref configured);
            AiDifficultyLevel level = AiDifficulty.FromValue(unchecked((uint)configured));
            ChildChecked(level == AiDifficultyLevel.Easy, Utf8("Easy"), 0x542948U, _strings.Store("Patch1101.SetDifficulty(1)"));
            ChildChecked(level == AiDifficultyLevel.Normal, Utf8("Normal"), 0x542948U, _strings.Store("Patch1101.SetDifficulty(2)"));
            ChildChecked(level == AiDifficultyLevel.Hard, Utf8("Hard"), 0x542948U, _strings.Store("Patch1101.SetDifficulty(3)"));
        });
        ParentSubmenu(Utf8("Difficulty"), difficulty);
        ParentGap();
        // 소리 4행과 음량 하위 2행은 음향 장치 준비 판정을 사용한다(RawGameMenus.cpp 144·150행의 0x5c7b14).
        // 음소거(--mute-audio) 실행이면 거짓이다.
        bool ready = _soundReady();
        ParentAscii("Sound On", "Toggle", "sound", Get("sound") != 0, ready); ParentAscii("Play Music", "Toggle", "music", Get("music") != 0, ready);
        ParentAscii("Wind Noise", "Toggle", "playWind", Get("playWind") != 0, ready); ParentAscii("Speaker Swap L/R", "Toggle", "swapLeftRightSpeakers", Get("swapLeftRightSpeakers") != 0, ready);
        uint soundVolume = TakeChild(() =>
        {
            for (int i = 1; i <= 5; i++) ChildChecked(Get("soundVolume") == i, Utf8($"Volume {i}"), 0x542a8cU, (uint)i);
        });
        ParentSubmenu(Utf8("Sound Effect Volume"), soundVolume, ready);
        uint musicVolume = TakeChild(() =>
        {
            for (int i = 1; i <= 5; i++) ChildChecked(Get("musicVolume") == i, Utf8($"Volume {i}"), 0x542a90U, (uint)i);
        });
        ParentSubmenu(Utf8("Music Volume"), musicVolume, ready);
        ParentGap();
        ParentAscii("Edge Scroll in Fullscreen", "Toggle", "edgeScroll", Get("edgeScroll") != 0);
        ParentAscii("Auto-Demo", "Toggle", "autoDemo", Get("autoDemo") != 0); ParentAscii("Tell Tips at Startup", "Toggle", "tellTips", Get("tellTips") != 0);
        ParentAscii("Anti-Aliasing", "Toggle", "antiAliasing", GetOr("antiAliasing", 0) != 0); ParentAscii("FSR 1.0 Upscaling", "Toggle", "fsr1", GetOr("fsr1", 0) != 0);
        ParentGap();
        ParentAscii("Pass Server Diagnostic ", "Tell", "ChangeFirewall");
        _actions = actions.ToArray();
        uint current = _menus.Menu.TakeCurrent();
        ScreenRect anchor = _root.ControlBounds("Options") ?? throw new InvalidOperationException("메인 Options의 현재 버튼 없음");
        // 원문 앵커 공식(baseWidth*0.5+x, y-40)을 같은 실수 연산으로 둔다(RawMenuControls.cpp 1047~1048행).
        float x = anchor.Left + (anchor.Right - anchor.Left) * 0.5f;
        _menus.Widgets.Attach(current, x, anchor.Top - 40, _root.Tree.Create.Entries[0], 0);
        _menus.Menu.AddMenuFlags(current, 0x04000000U);
        _changed();
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
