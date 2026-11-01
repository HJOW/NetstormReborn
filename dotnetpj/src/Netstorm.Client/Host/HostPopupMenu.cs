using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

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
    private readonly Action _changed;
    private MainMenuAction[] _actions = [];

    /// <summary>같은 공통 Gump 트리와 메뉴 수명·화면을 사용한다.</summary>
    public HostPopupMenu(HostMainMenu root, HostHtmlWindows windows, HostHtmlMenus menus, Screen screen, Action changed)
    { _root = root; _windows = windows; _menus = menus; _screen = screen; _changed = changed; }

    /// <summary>복원한 Help 팝업을 같은 Gump에 부착한다. 다른 메뉴는 상위 처리기에 남긴다.</summary>
    public bool Open(string name)
    {
        if (!string.Equals(name, "help", StringComparison.OrdinalIgnoreCase)) return false;
        _windows.RequestCloseAll();
        _actions = [new("Tell", "About")];
        _menus.Widgets.BeginVertical();
        _menus.Controls.AddChecked(false, Encoding.ASCII.GetBytes("About NetStorm"), SelectCommand, 1, true, true);
        _menus.Menu.AttachCurrent(_screen.Width / 2 + 2, _screen.Height / 2 - 27, _root.Tree.Create.Entries[0]);
        _changed(); return true;
    }

    /// <summary>현재 행 번호를 원본 상위 메뉴 행동으로 반환한다.</summary>
    public MainMenuAction Select(uint index)
    {
        if (index == 0 || index > _actions.Length) throw new ArgumentOutOfRangeException(nameof(index), "현재 팝업 행 범위 오류");
        return _actions[index - 1];
    }
}
