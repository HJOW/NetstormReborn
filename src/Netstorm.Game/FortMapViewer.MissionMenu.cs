using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;

namespace Netstorm.Game;

/// <summary>미션 메뉴가 게임 본체에 요청하는 화면 전환.</summary>
internal enum MissionMenuAction
{
    /// <summary>현재 미션을 다시 시작한다.</summary>
    Restart,
    /// <summary>떠나기 확인 창에서 같은 미션을 다시 시작한다.</summary>
    Replay,
    /// <summary>미션에서 나온다.</summary>
    MainMenu,
    /// <summary>프로그램을 종료한다.</summary>
    Quit,
    /// <summary>지도 위에서 공유 옵션 메뉴를 연다.</summary>
    Options,
    /// <summary>일반 도움말의 목차를 연다.</summary>
    Help,
}

/// <summary>원본에서 확인한 Esc → Game 메뉴와 Leave Mission 확인 창의 입력·표시.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>원본 상단 메뉴 막대 높이에 맞춘 메뉴 높이.</summary>
    private const int MissionMenuBarHeight = 18;

    /// <summary>Game 메뉴의 항목 한 줄 높이.</summary>
    private const int MissionMenuRowHeight = 24;

    /// <summary>원본 게임 왼쪽 패널 다음에서 시작하는 Game 메뉴 x 좌표.</summary>
    private const int MissionMenuLeft = 84;

    /// <summary>Game 펼침 메뉴의 폭.</summary>
    private const int MissionMenuWidth = 220;

    /// <summary>Esc로 표시한 상단 막대 상태.</summary>
    private bool _missionMenuVisible;

    /// <summary>Game 항목을 클릭해 연 목록 상태.</summary>
    private bool _missionGameDropdown;
    private bool _missionViewDropdown;

    /// <summary>Leave Mission을 누른 뒤의 확인 창 상태.</summary>
    private bool _leaveMissionPrompt;

    /// <summary>게임 본체가 다음 갱신에서 처리할 메뉴 동작.</summary>
    private MissionMenuAction? _pendingMissionMenuAction;

    /// <summary>가장자리 스크롤을 막아야 하는 메뉴 또는 확인 창이 열렸는지.</summary>
    public bool MissionMenuOpen => _missionMenuVisible || _leaveMissionPrompt;

    /// <summary>메뉴에서 선택한 화면 이동을 한 번만 돌려준다.</summary>
    public MissionMenuAction? TakeMissionMenuAction()
    {
        MissionMenuAction? action = _pendingMissionMenuAction;
        _pendingMissionMenuAction = null;
        return action;
    }

    /// <summary>미션의 Esc·Game·Leave 확인 창 입력을 처리하고 지도 입력 차단 여부를 돌려준다.</summary>
    private bool UpdateMissionMenu(KeyboardState keyboard, MouseState mouse, int width, int height)
    {
        if (!IsMissionMode)
        {
            return false;
        }
        bool clicked = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton != ButtonState.Pressed;
        if (_leaveMissionPrompt)
        {
            // 원본의 확인 창에서는 Esc가 동작하지 않고 버튼으로만 결정한다.
            if (clicked)
            {
                Rectangle panel = LeaveMissionPanel(width, height);
                if (LeaveMissionButton(panel, 0).Contains(mouse.X, mouse.Y))
                {
                    _pendingMissionMenuAction = MissionMenuAction.MainMenu;
                }
                else if (LeaveMissionButton(panel, 1).Contains(mouse.X, mouse.Y))
                {
                    _pendingMissionMenuAction = MissionMenuAction.Replay;
                }
                else if (LeaveMissionButton(panel, 2).Contains(mouse.X, mouse.Y))
                {
                    _leaveMissionPrompt = false;
                    _missionMenuVisible = false;
                }
            }
            return true;
        }
        if (Pressed(keyboard, Keys.Escape))
        {
            _missionMenuVisible = !_missionMenuVisible;
            _missionGameDropdown = false;
            _missionViewDropdown = false;
            return true;
        }
        if (!_missionMenuVisible)
        {
            return false;
        }
        if (clicked)
        {
            if (MissionTab(2).Contains(mouse.Position))
            { _pendingMissionMenuAction = MissionMenuAction.Options; _missionMenuVisible = false; _missionGameDropdown = _missionViewDropdown = false; return true; }
            if (MissionTab(1).Contains(mouse.Position))
            { _missionViewDropdown = !_missionViewDropdown; _missionGameDropdown = false; return true; }
            if (_missionViewDropdown)
            {
                Rectangle view = ViewMenuPanel();
                if (view.Contains(mouse.Position))
                {
                    if ((mouse.Y - view.Y) / MissionMenuRowHeight == 0) _pendingMissionMenuAction = MissionMenuAction.Help;
                    else OpenKnowledge();
                    _missionMenuVisible = false;
                }
                _missionViewDropdown = false; return true;
            }
            Rectangle gameTab = MissionTab(0);
            if (gameTab.Contains(mouse.X, mouse.Y))
            {
                _missionGameDropdown = !_missionGameDropdown;
            }
            else if (_missionGameDropdown)
            {
                var dropdown = new Rectangle(MissionMenuLeft, MissionMenuBarHeight, MissionMenuWidth, MissionMenuRowHeight * 4);
                if (dropdown.Contains(mouse.X, mouse.Y))
                {
                    int row = (mouse.Y - dropdown.Y) / MissionMenuRowHeight;
                    if (row == 0 && _tutorialDialog?.Review() == true)
                    {
                        ResetTutorialPage();
                        _missionMenuVisible = false;
                    }
                    else if (row == 1)
                    {
                        _pendingMissionMenuAction = MissionMenuAction.Restart;
                    }
                    else if (row == 2)
                    {
                        _leaveMissionPrompt = true;
                        _missionGameDropdown = false;
                    }
                    else if (row == 3)
                    {
                        _pendingMissionMenuAction = MissionMenuAction.Quit;
                    }
                }
                else
                {
                    _missionGameDropdown = false;
                }
            }
        }
        return true;
    }

    /// <summary>떠나기 확인 창을 작은 원본 돌 창으로 화면 중심에 둔다.</summary>
    private static Rectangle LeaveMissionPanel(int width, int height) =>
        new((width - 390) / 2, (height - 148) / 2, 390, 148);

    /// <summary>번역된 버튼 문구에 맞춰 확인 창의 세 클릭 영역을 배치한다.</summary>
    private Rectangle LeaveMissionButton(Rectangle panel, int index)
    {
        string[] labels = LeaveMissionLabels();
        int[] widths = labels.Select(label => Math.Max(60, (int)_uiSkin.Body.MeasureString(label).X + 12)).ToArray();
        int x = panel.Center.X - (widths.Sum() + 24) / 2;
        // 앞 버튼들의 문구 폭과 간격을 더한다.
        for (int i = 0; i < index; i++) x += widths[i] + 12;
        return new Rectangle(x, panel.Bottom - 35, widths[index], OriginalUiSkin.ButtonHeight);
    }

    /// <summary>원본 확인 창의 세 선택지를 표시 언어로 제공한다.</summary>
    private string[] LeaveMissionLabels() =>
        [Ui("메인 메뉴", "Main Menu"), Ui("다시 시작", "Replay Mission"), Ui("미션 계속", "Continue Mission")];

    /// <summary>원본 상단 항목의 표시 문구와 폭 계산을 입력·그리기에서 공유한다.</summary>
    private string[] MissionTabs() => [Ui("게임", "Game"), Ui("보기", "View"), Ui("옵션", "Options"), Ui("플레이어", "Players"), Ui("정보", "About")];
    /// <summary>언어별 실제 글자 폭에 맞춘 상단 항목의 클릭 영역.</summary>
    private Rectangle MissionTab(int index)
    {
        string[] tabs = MissionTabs(); int x = MissionMenuLeft + 5;
        // 앞 항목의 글자 폭과 여백을 더해 표시 위치와 클릭 영역을 일치시킨다.
        for (int i = 0; i < index; i++) x += Math.Max(50, (int)_uiSkin.Body.MeasureString(tabs[i]).X + 16);
        return new(x - 5, 0, Math.Max(50, (int)_uiSkin.Body.MeasureString(tabs[index]).X + 16), MissionMenuBarHeight);
    }
    /// <summary>지원한 두 보기 명령의 펼침 영역.</summary>
    private Rectangle ViewMenuPanel() => new(MissionTab(1).X, MissionMenuBarHeight, MissionMenuWidth, MissionMenuRowHeight * 2);

    /// <summary>상단 돌 메뉴 막대·Game 목록·떠나기 확인 창을 지도 위에 그린다.</summary>
    private void DrawMissionMenu(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (!IsMissionMode || (!_missionMenuVisible && !_leaveMissionPrompt) || TutorialDialogOpen && !_leaveMissionPrompt) return;
        if (_missionMenuVisible)
        {
            _uiSkin.Menu(batch, new Rectangle(MissionMenuLeft, 0, width - MissionMenuLeft, MissionMenuBarHeight));
            string[] tabs = MissionTabs();
            int tabX = MissionMenuLeft + 5;
            // 원본 메뉴 막대의 다섯 항목을 작은 글씨로 나란히 표시한다.
            for (int i = 0; i < tabs.Length; i++)
            {
                OriginalUiSkin.Text(batch, font, tabs[i], new Vector2(tabX, 1), i <= 2 ? Color.White : Color.Gray);
                tabX += Math.Max(50, (int)font.MeasureString(tabs[i]).X + 16);
            }
            if (_missionViewDropdown)
            {
                Rectangle list = ViewMenuPanel(); _uiSkin.Menu(batch, list);
                string[] labels = [Ui("도움말 - F1", "General Help - F1"), Ui("지식 보기 - F6", "View Netstorm Knowledge - F6")];
                // 보기 명령은 같은 행 높이로 표시하고 커서 강조를 따로 그린다.
                for (int i = 0; i < labels.Length; i++)
                {
                    var row = new Rectangle(list.X + 1, list.Y + i * MissionMenuRowHeight + 1, list.Width - 2, MissionMenuRowHeight - 2);
                    if (row.Contains(_previousMouse.Position)) batch.Draw(_pixel, row, Color.Black * 0.25f);
                    OriginalUiSkin.Text(batch, font, labels[i], new(row.X + 10, row.Y + 4));
                }
            }
            if (_missionGameDropdown)
            {
                var list = new Rectangle(MissionMenuLeft, MissionMenuBarHeight, MissionMenuWidth, MissionMenuRowHeight * 4);
                _uiSkin.Menu(batch, list);
                string[] labels = [Ui("미션 목표 다시 보기 - F8", "Review Mission Objectives - F8"), Ui("미션 재시작", "Restart Mission"), Ui("미션 떠나기", "Leave Mission"), Ui("게임 종료", "Quit Game")];
                // 원본에서 확인한 Game 메뉴 순서로 클릭 영역과 같은 행에 표시한다.
                for (int i = 0; i < labels.Length; i++)
                {
                    var row = new Rectangle(list.X + 1, list.Y + i * MissionMenuRowHeight + 1, list.Width - 2, MissionMenuRowHeight - 2);
                    if (row.Contains(_previousMouse.Position)) batch.Draw(_pixel, row, Color.Black * 0.25f);
                    OriginalUiSkin.Text(batch, font, labels[i], new Vector2(row.X + 10, row.Center.Y - font.MeasureString(labels[i]).Y / 2));
                    if (i is 0 or 2) _uiSkin.Separator(batch, list.X + 1, row.Bottom, list.Width - 2);
                }
            }
        }
        if (_leaveMissionPrompt)
        {
            Rectangle panel = LeaveMissionPanel(width, height);
            _uiSkin.Panel(batch, panel);
            OriginalUiSkin.Text(batch, _uiSkin.Title, Ui("미션을 떠나시겠습니까?", "Leave Mission?"), new Vector2(panel.X + 30, panel.Y + 20));
            OriginalUiSkin.Text(batch, font, Ui("이 미션을 종료하고\n지금 메인 메뉴로 돌아가시겠습니까?", "Do you wish to quit this mission\nand return to the Main Menu now?"), new Vector2(panel.X + 30, panel.Y + 54));
            string[] labels = LeaveMissionLabels();
            // 원본 확인 창의 세 선택지를 작은 돌 버튼으로 그린다.
            for (int i = 0; i < labels.Length; i++)
            {
                Rectangle button = LeaveMissionButton(panel, i);
                bool hover = button.Contains(_previousMouse.Position);
                _uiSkin.Button(batch, button, labels[i], hover: hover, pressed: hover && _previousMouse.LeftButton == ButtonState.Pressed);
            }
        }
    }
}
