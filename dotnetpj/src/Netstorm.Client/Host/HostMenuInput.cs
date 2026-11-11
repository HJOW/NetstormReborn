using Netstorm.Client.Input;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Host; // 메인 메뉴의 실제 Gump와 일반 명령 큐를 분배한다.

/// <summary>메인 버튼의 다음 Tick 명령과 HTML 창의 일반 입력을 같은 트리·큐에 연결한다.
/// 근거: cpppj/src/client/UberGump.cpp의 ActivatePageAction·Event·Tick.</summary>
public sealed class HostMenuInput
{
    /// <summary>공통 Gump 입력과 삭제 수명.</summary>
    private readonly HostMainMenu _menu;
    /// <summary>현재 전체 HTML 창 소유자.</summary>
    private readonly HostHtmlWindows _windows;
    /// <summary>현재 설정에서 Tell을 만드는 공장.</summary>
    private readonly HostHtmlFactory _factory;
    /// <summary>공유 원시 큐와 문자열 고리.</summary>
    private readonly InputQueue _queue;
    /// <summary>큐가 가리키는 현재 원본 문자열 고리.</summary>
    private readonly ScriptStringRing _strings;
    /// <summary>변경을 실제 화면 재합성에 알린다.</summary>
    private readonly Action _changed;
    /// <summary>아직 이식하지 않은 상위 메뉴 명령의 명시 경계.</summary>
    private readonly Action<MainMenuAction> _unhandled;
    /// <summary>원본 State::Post처럼 비었을 때만 채우는 단일 다음 Tick 행동.</summary>
    private MainMenuAction? _pending;
    /// <summary>같은 공통 트리에 설치한 상위 팝업 메뉴다.</summary>
    private HostPopupMenu? _popup;
    private HostEditorSelection? _editor;
    /// <summary>새 이름 창을 여는 실제 현재 호스트 장치다.</summary>
    private Action<uint>? _newFort;
    private Func<uint>? _screenFlags;
    private Action<uint, bool>? _changeScreen;
    private Action? _acceptScreen;

    /// <summary>Help 등 상위 팝업의 실제 controlled 메뉴를 연결한다.</summary>
    public void BindPopup(HostPopupMenu popup) => _popup = popup ?? throw new ArgumentNullException(nameof(popup));
    /// <summary>실제 편집 맵 선택 전역과 입력 분배기를 연결한다.</summary>
    public void BindEditor(HostEditorSelection editor) => _editor = editor ?? throw new ArgumentNullException(nameof(editor));
    /// <summary>새 Fort 이름 입력 명령을 같은 호스트 모달에 연결한다.</summary>
    public void BindNewFort(Action<uint> show) => _newFort = show ?? throw new ArgumentNullException(nameof(show));
    /// <summary>실제 화면 장치의 현재 모드·변경·확인을 연결한다.</summary>
    public void BindScreenMode(Func<uint> flags, Action<uint, bool> change, Action accept)
    { _screenFlags = flags; _changeScreen = change; _acceptScreen = accept; }

    /// <summary>같은 실제 메뉴·창·큐와 후속 명령 경계를 빌린다.</summary>
    public HostMenuInput(HostMainMenu menu, HostHtmlWindows windows, HostHtmlFactory factory, InputQueue queue,
        ScriptStringRing strings, Action changed, Action<MainMenuAction> unhandled)
    { _menu = menu ?? throw new ArgumentNullException(nameof(menu)); _windows = windows ?? throw new ArgumentNullException(nameof(windows)); _factory = factory ?? throw new ArgumentNullException(nameof(factory)); _queue = queue ?? throw new ArgumentNullException(nameof(queue)); _strings = strings ?? throw new ArgumentNullException(nameof(strings)); _changed = changed ?? throw new ArgumentNullException(nameof(changed)); _unhandled = unhandled ?? throw new ArgumentNullException(nameof(unhandled)); }

    /// <summary>메인 버튼 명령을 원본처럼 즉시 큐 또는 한 칸의 다음 Tick에 예약한다.</summary>
    public void Post(MainMenuAction action)
    {
        if (action.Command is "AskEditFort" or "RequestQuitApp" or "ScreenModeAbsChange")
            ScriptCommand.QueueScriptCommand(action.Command + "," + action.Argument, _queue, _strings,
                new() { Lookup = DialogScriptCommands.Lookup, Unknown = name => throw new InvalidOperationException("메뉴 명령 조회 실패: " + name) });
        else _pending ??= action;
    }

    /// <summary>한 프레임 앞에 예약된 상위 행동 하나를 소비한다.</summary>
    public void Tick()
    {
        if (_pending is not { } action) return; _pending = null;
        if (action.Command == "Tell") ShowTell(action.Argument);
        else if (action.Command == "Menu" && _popup?.Open(action.Argument) == true) { }
        else if (_popup?.Execute(action) == true) { }
        else _unhandled(action);
    }

    /// <summary>Blank는 기존 창을 지연 삭제하고, 나머지는 같은 실제 Tell 공장에 전달한다.</summary>
    public void ShowTell(string target)
    {
        if (string.Equals(target, "Blank", StringComparison.OrdinalIgnoreCase)) _windows.RequestCloseAll();
        else if (!_factory.ShowTell(target).Found) throw new InvalidOperationException("Tell 섹션 없음: " + target);
        _changed();
    }

    /// <summary>공통 Gump가 먼저 소비한 입력을 제외하고 복원된 일반 명령을 처리한다. true는 앱 종료다.</summary>
    public bool Dispatch(InputEvent input)
    {
        if (_menu.Route(input) != 0) { _changed(); return false; }
        if (_editor?.Dispatch(input) == true) { _changed(); return false; }
        uint code = input.Code & 0x03ffffff; bool released = (input.Code & InputCode.Release) != 0;
        if (code == HostPopupMenu.QueuedSelectCommand && _popup != null) { if (!released) Post(_popup.Select(unchecked((uint)input.Y))); return false; }
        if (code == 0x02000079) { if (!released) { (_newFort ?? throw new InvalidOperationException("새 Fort 이름 창 미연결"))(unchecked((uint)input.Y)); _changed(); } return false; }
        if (code == 0x02000030) { if (!released) { (_changeScreen ?? throw new InvalidOperationException("화면 모드 장치 미연결"))(unchecked((uint)input.Y), true); _changed(); } return false; }
        if (code == 0x02000002) { if (!released) { uint previous = (_screenFlags ?? throw new InvalidOperationException("현재 화면 모드 미연결"))(); (_changeScreen ?? throw new InvalidOperationException("화면 모드 장치 미연결"))(previous ^ unchecked((uint)input.Y), false); ShowTell($"DirectDrawVerify({previous})"); } return false; }
        if (code == 0x020000e4) { if (!released) { (_acceptScreen ?? throw new InvalidOperationException("화면 모드 확인 미연결"))(); ShowTell("AdjustDirectDraw"); } return false; }
        if (code == 0x020000be) { _windows.RequestCloseAll(); _changed(); return false; }
        if (code == 0x02000089) return false;
        // Help 팝업의 실제 행 명령이다. 0x17 도움말 주제 창은 R3 잔여, 0x49 기술 도움말 외부 실행은 외부 경계라 명시 예외다.
        // 근거: cpppj/src/client/UberGump.cpp 1223~1224행과 TechnicalHelp.cpp 32행(0.383에서 행을 실제 구성으로 바꿈).
        if (code == 0x02000017) { if (!released) throw new InvalidOperationException("도움말 주제 창 미연결(R3 잔여)"); return false; }
        if (code == 0x02000049) { if (!released) throw new InvalidOperationException("기술 도움말 실행 미연결(외부 실행 경계)"); return false; }
        if (code == 0x02000095) { if (!released) ShowTell(_strings.Resolve(unchecked((uint)input.Y))); return false; }
        if (code == 0x0200006d)
        {
            if (!released && input.Y != 0) _factory.ApplyConfig(_strings.Resolve(unchecked((uint)input.Y)));
            if (!released) _changed(); return false;
        }
        if (code == 0x02000094) { if (!released) _queue.PushCommand(0x02000001, 0, 99); return false; }
        if (code == 0x02000001) { if (!released && input.Y != 99) throw new InvalidOperationException("종료 명령 인자 오류"); return !released; }
        if ((input.Code & InputCode.Command) != 0) _unhandled(new MainMenuAction("Command", code.ToString("x8")));
        return false;
    }

    /// <summary>현재 포인터가 화면 안이면 공통 overlay와 등록 노드의 이동 통지를 입력 큐보다 먼저 실행한다.</summary>
    public uint? NotifyPointer(InputEvent pointer)
    {
        if ((pointer.Code & InputCode.Outside) != 0) return null;
        uint overlay = _menu.Tree.Input.Overlay(pointer.Code, new(pointer.X, pointer.Y));
        _menu.Tree.Frame.Broadcast(pointer.Code, unchecked((uint)pointer.X), unchecked((uint)pointer.Y));
        return overlay;
    }

    /// <summary>공통 지연 삭제를 먼저 처리한 뒤 실제 idle 표를 순회한다.</summary>
    public void FinishFrame()
    {
        _menu.Tree.Lifetime.Cleanup(); _menu.Tree.Frame.Idle();
    }
}
