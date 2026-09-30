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
}

/// <summary>원본에서 확인한 Esc → Game 메뉴와 Leave Mission 확인 창의 입력·표시.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>원본 상단 메뉴 막대 높이에 맞춘 메뉴 높이.</summary>
    private const int MissionMenuBarHeight = 30;

    /// <summary>Game 메뉴의 항목 한 줄 높이.</summary>
    private const int MissionMenuRowHeight = 32;

    /// <summary>원본 게임 왼쪽 패널 다음에서 시작하는 Game 메뉴 x 좌표.</summary>
    private const int MissionMenuLeft = 84;

    /// <summary>Game 펼침 메뉴의 폭.</summary>
    private const int MissionMenuWidth = 350;

    /// <summary>Esc로 표시한 상단 막대 상태.</summary>
    private bool _missionMenuVisible;

    /// <summary>Game 항목을 클릭해 연 목록 상태.</summary>
    private bool _missionGameDropdown;

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
            return true;
        }
        if (!_missionMenuVisible)
        {
            return false;
        }
        if (clicked)
        {
            var gameTab = new Rectangle(MissionMenuLeft, 0, 82, MissionMenuBarHeight);
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

    /// <summary>화면 중심에 놓는 Leave Mission 확인 창의 위치.</summary>
    private static Rectangle LeaveMissionPanel(int width, int height) =>
        new((width - 600) / 2, (height - 178) / 2, 600, 178);

    /// <summary>확인 창의 Main Menu·Replay Mission·Continue Mission 버튼 위치.</summary>
    private static Rectangle LeaveMissionButton(Rectangle panel, int index) =>
        new(panel.X + 16 + index * 192, panel.Bottom - 52, 180, 34);

    /// <summary>상단 메뉴 막대·Game 목록·떠나기 확인 창을 지도 위에 그린다.</summary>
    private void DrawMissionMenu(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (!IsMissionMode || (!_missionMenuVisible && !_leaveMissionPrompt) || TutorialDialogOpen)
        {
            return;
        }
        if (_missionMenuVisible)
        {
            batch.Draw(_pixel, new Rectangle(0, 0, width, MissionMenuBarHeight), new Color(73, 68, 60));
            batch.DrawString(font, "Game", new Vector2(MissionMenuLeft + 4, 2), Color.White);
            if (_missionGameDropdown)
            {
                var list = new Rectangle(MissionMenuLeft, MissionMenuBarHeight, MissionMenuWidth, MissionMenuRowHeight * 4);
                batch.Draw(_pixel, list, new Color(67, 63, 59));
                Outline(batch, list, Color.Wheat);
                // 원본의 Game 목록에 있는 네 동작만 표시한다.
                string[] labels = ["Review Mission Objectives - F8", "Restart Mission", "Leave Mission", "Quit Game"];
                // 원본에서 확인한 Game 메뉴 순서로 각 행을 표시한다.
                for (int index = 0; index < labels.Length; index++)
                {
                    batch.DrawString(font, labels[index], new Vector2(list.X + 8, list.Y + index * MissionMenuRowHeight + 2), Color.White);
                }
            }
        }
        if (_leaveMissionPrompt)
        {
            Rectangle panel = LeaveMissionPanel(width, height);
            batch.Draw(_pixel, panel, new Color(69, 64, 58));
            Outline(batch, panel, Color.Wheat);
            batch.DrawString(font, "Leave Mission?", new Vector2(panel.X + 20, panel.Y + 16), Color.White);
            batch.DrawString(font, "Do you wish to quit this mission", new Vector2(panel.X + 20, panel.Y + 54), Color.White);
            batch.DrawString(font, "and return to the Main Menu now?", new Vector2(panel.X + 20, panel.Y + 80), Color.White);
            // 원본 확인 창의 세 버튼을 왼쪽부터 같은 순서로 둔다.
            string[] labels = ["Main Menu", "Replay Mission", "Continue Mission"];
            // 원본 확인 창의 세 선택지를 왼쪽부터 같은 순서로 표시한다.
            for (int index = 0; index < labels.Length; index++)
            {
                Rectangle button = LeaveMissionButton(panel, index);
                batch.Draw(_pixel, button, new Color(49, 45, 43));
                Outline(batch, button, Color.Wheat);
                batch.DrawString(font, labels[index], new Vector2(button.X + 5, button.Y + 3), Color.White);
            }
        }
    }
}
