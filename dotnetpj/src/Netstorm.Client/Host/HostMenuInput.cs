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

    /// <summary>같은 실제 메뉴·창·큐와 후속 명령 경계를 빌린다.</summary>
    public HostMenuInput(HostMainMenu menu, HostHtmlWindows windows, HostHtmlFactory factory, InputQueue queue,
        ScriptStringRing strings, Action changed, Action<MainMenuAction> unhandled)
    { _menu = menu ?? throw new ArgumentNullException(nameof(menu)); _windows = windows ?? throw new ArgumentNullException(nameof(windows)); _factory = factory ?? throw new ArgumentNullException(nameof(factory)); _queue = queue ?? throw new ArgumentNullException(nameof(queue)); _strings = strings ?? throw new ArgumentNullException(nameof(strings)); _changed = changed ?? throw new ArgumentNullException(nameof(changed)); _unhandled = unhandled ?? throw new ArgumentNullException(nameof(unhandled)); }

    /// <summary>메인 버튼 명령을 원본처럼 즉시 큐 또는 한 칸의 다음 Tick에 예약한다.</summary>
    public void Post(MainMenuAction action)
    {
        if (action.Command is "AskEditFort" or "RequestQuitApp")
            ScriptCommand.QueueScriptCommand(action.Command + "," + action.Argument, _queue, _strings,
                new() { Lookup = DialogScriptCommands.Lookup, Unknown = name => throw new InvalidOperationException("메뉴 명령 조회 실패: " + name) });
        else _pending ??= action;
    }

    /// <summary>한 프레임 앞에 예약된 상위 행동 하나를 소비한다.</summary>
    public void Tick()
    {
        if (_pending is not { } action) return; _pending = null;
        if (action.Command == "Tell") ShowTell(action.Argument); else _unhandled(action);
    }

    /// <summary>Blank는 기존 창을 지연 삭제하고, 나머지는 같은 실제 Tell 공장에 전달한다.</summary>
    private void ShowTell(string target)
    {
        if (string.Equals(target, "Blank", StringComparison.OrdinalIgnoreCase)) _windows.RequestCloseAll();
        else if (!_factory.ShowTell(target).Found) throw new InvalidOperationException("Tell 섹션 없음: " + target);
        _changed();
    }

    /// <summary>공통 Gump가 먼저 소비한 입력을 제외하고 복원된 일반 명령을 처리한다. true는 앱 종료다.</summary>
    public bool Dispatch(InputEvent input)
    {
        if (_menu.Route(input) != 0) { _changed(); return false; }
        uint code = input.Code & 0x03ffffff; bool released = (input.Code & InputCode.Release) != 0;
        if (code == 0x020000be) { _windows.RequestCloseAll(); _changed(); return false; }
        if (code == 0x02000089) return false;
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

    /// <summary>공통 지연 삭제를 먼저 처리한 뒤 실제 idle 표를 순회한다.</summary>
    public void FinishFrame()
    {
        _menu.Tree.Lifetime.Cleanup(); _menu.Tree.Frame.Idle();
    }
}
