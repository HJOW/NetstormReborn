using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

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

/// <summary>
/// 원본에서 확인한 Esc → 상단 메뉴 막대(Game·View·Options·Players·About)와 Leave Mission 확인 창의 입력·표시.
/// 2026-10-01 네 번째 녹화 01:31.5 의 Game 목록, 2026-09-30 첫 녹화 04:50~05:08 의 View 목록(단축키는 노란 글씨,
/// 켜진 보기 항목은 파란 마크)을 따른다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>
    /// 상단 메뉴 막대의 높이. 2026-10-10: 18 → 17 (원본 캡처 screenShots/The War Begins! - In game menu.png 의 막대는 테두리 2줄을 포함해 17줄이다).
    /// </summary>
    private const int MissionMenuBarHeight = 17;

    /// <summary>펼침 목록 한 줄 높이 (원본 17).</summary>
    private const int MissionMenuRowHeight = OriginalUiSkin.RowHeight;

    /// <summary>목록 그룹 사이 구분 구간의 높이 (원본 16).</summary>
    private const int MissionMenuSeparatorHeight = OriginalUiSkin.MenuSeparatorHeight;

    /// <summary>원본 게임 왼쪽 패널 다음에서 시작하는 메뉴 막대 x 좌표.</summary>
    private const int MissionMenuLeft = 84;

    /// <summary>
    /// 막대 항목 사이의 간격. 원본은 문구 길이와 관계없이 53픽셀마다 항목을 둔다 (캡처의 View·Options·Players·About 글자 x 142·195·248·301).
    /// 2026-10-10: 문구 폭에 맞추던 것을 고정 간격으로 바꿨다.
    /// </summary>
    private const int MissionTabPitch = 53;

    /// <summary>항목 영역의 왼쪽에서 글자까지의 거리.</summary>
    private const int MissionTabTextInset = 5;

    /// <summary>단축키 글씨 색 (녹화의 노란 키 이름)</summary>
    private static readonly Color MenuKeyColor = OriginalUiSkin.ValueColor;

    /// <summary>Esc로 표시한 상단 막대 상태.</summary>
    private bool _missionMenuVisible;

    /// <summary>열려 있는 펼침 목록 번호 (-1 = 없음, 0 Game · 1 View · 4 About)</summary>
    private int _missionDropdown = -1;

    /// <summary>Leave Mission을 누른 뒤의 확인 창 상태.</summary>
    private bool _leaveMissionPrompt;
    /// <summary>Leave Mission 확인 창 세 버튼의 누름·떼기 처리기 (번호 0 Main Menu · 1 Replay Mission · 2 Continue Mission).</summary>
    private readonly ButtonGump _leaveGump = new();

    /// <summary>게임 본체가 다음 갱신에서 처리할 메뉴 동작.</summary>
    private MissionMenuAction? _pendingMissionMenuAction;

    /// <summary>펼침 목록 한 줄: 표시 문구·단축키·사용 가능·켜짐 표시·동작·뒤에 구분선</summary>
    /// <param name="Label">항목 이름</param>
    /// <param name="Key">단축키 표기 (없으면 빈 문자열)</param>
    /// <param name="Enabled">고를 수 있는지</param>
    /// <param name="Check">켜짐 표시 (null 이면 표시 없는 명령)</param>
    /// <param name="Action">고르면 할 일</param>
    /// <param name="SeparatorAfter">이 행 뒤에 구분선이 있는지</param>
    private sealed record MissionMenuRow(string Label, string Key, bool Enabled, bool? Check, Action? Action, bool SeparatorAfter = false);

    /// <summary>가장자리 스크롤을 막아야 하는 메뉴 또는 확인 창이 열렸는지.</summary>
    public bool MissionMenuOpen => _missionMenuVisible || _leaveMissionPrompt;

    /// <summary>메뉴에서 선택한 화면 이동을 한 번만 돌려준다.</summary>
    public MissionMenuAction? TakeMissionMenuAction()
    {
        MissionMenuAction? action = _pendingMissionMenuAction;
        _pendingMissionMenuAction = null;
        return action;
    }

    /// <summary>메뉴 막대와 펼침 목록을 모두 닫는다.</summary>
    private void CloseMissionMenu() { _missionMenuVisible = false; _missionDropdown = -1; }

    /// <summary>펼침 목록의 내용. 녹화에서 확인한 순서·구분선·단축키 표기를 그대로 쓴다.</summary>
    /// <param name="tab">상단 항목 번호</param>
    private List<MissionMenuRow> MissionMenuRows(int tab) => tab switch
    {
        0 =>
        [
            new(Ui("미션 목표 다시 보기", "Review Mission Objectives"), "F8", true, null, ReviewObjectives, SeparatorAfter: true),
            new(Ui("미션 재시작", "Restart Mission"), "", true, null, () => _pendingMissionMenuAction = MissionMenuAction.Restart),
            new(Ui("미션 떠나기", "Leave Mission"), "", true, null, () => { _leaveMissionPrompt = true; _missionDropdown = -1; }, SeparatorAfter: true),
            new(Ui("게임 종료", "Quit Game"), "", true, null, () => _pendingMissionMenuAction = MissionMenuAction.Quit),
        ],
        1 =>
        [
            new(Ui("건물 숨기기", "Hide buildings"), "F2", true, _hideBuildings, () => _hideBuildings = !_hideBuildings),
            new(Ui("섬 테마 숨기기/보이기", "Hide/Show Island Themes"), "Shift F3", true, _hideIslandThemes, () => _hideIslandThemes = !_hideIslandThemes),
            new(Ui("본거지 템플 보기", "View Home Temple"), "F4", true, null, ViewHomeTemple),
            new(Ui("내 사제 보기", "View Your Priest"), "F5", true, null, CenterOnPriest),
            new(Ui("NetStorm 지식 보기", "View Netstorm Knowledge"), "F6", true, null, OpenKnowledge),
            new(Ui("섬 소유 표시 숨기기", "Hide Island Ownership"), "F7", true, !_islandColors, () => _islandColors = !_islandColors),
            new(Ui("미션 목표 다시 보기", "Review Mission Objectives"), "F8", true, null, ReviewObjectives),
            // 플레이어 목록 창은 멀티플레이 화면이라 아직 없다
            new(Ui("플레이어 목록 보기", "View Player List"), "F9", false, null, null),
        ],
        4 => [new(Ui("일반 도움말", "General Help"), "F1", true, null, () => _pendingMissionMenuAction = MissionMenuAction.Help)],
        _ => [],
    };

    /// <summary>F8 처럼 미션 목표(브리핑)를 다시 연다.</summary>
    private void ReviewObjectives()
    {
        if (_tutorialDialog?.Review() == true) ResetTutorialPage();
    }

    /// <summary>F4/H 처럼 내 템플로 화면을 옮기고 귀환 명령을 낸다.</summary>
    private void ViewHomeTemple()
    {
        GameEntity? temple = _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Temple);
        if (temple != null) _camera = WorldPixels(temple.Footprint.AnchorX, temple.Footprint.AnchorY);
        _session.Submit(new ReturnHomeCommand(TestPlayer));
    }

    /// <summary>펼침 목록의 화면 영역 (행 높이와 구분선 높이를 더한다).</summary>
    /// <param name="tab">상단 항목 번호</param>
    /// <param name="rows">목록 행</param>
    /// <remarks>
    /// 2026-10-10: 원본 캡처의 Game 목록 치수로 바꿨다 — 왼쪽은 항목 영역 + 1(글자가 항목 글자와 같은 x 에서 10 오른쪽),
    /// 위쪽은 막대 바로 아래, 폭은 가장 긴 행("문구 - 키") + 31(Game 목록 207), 높이는 행 17 × 개수 + 구분 구간 16 × 개수(Game 목록 100).
    /// </remarks>
    private Rectangle MissionDropdownPanel(int tab, List<MissionMenuRow> rows)
    {
        int width = OriginalUiSkin.MenuWidth(rows.Select(r => _uiSkin.MenuLabelWidth(r.Key.Length > 0 ? r.Label + " - " + r.Key : r.Label)));
        int height = rows.Count * MissionMenuRowHeight + rows.Count(r => r.SeparatorAfter) * MissionMenuSeparatorHeight;
        return new Rectangle(MissionTab(tab).X + 1, MissionMenuBarHeight, width, height);
    }

    /// <summary>펼침 목록 행의 화면 영역을 위에서부터 차례로 돌려준다.</summary>
    /// <param name="tab">상단 항목 번호</param>
    private IEnumerable<(MissionMenuRow Row, Rectangle Bounds)> MissionDropdownLayout(int tab)
    {
        List<MissionMenuRow> rows = MissionMenuRows(tab);
        Rectangle panel = MissionDropdownPanel(tab, rows);
        int y = panel.Y;
        // 행을 목록 맨 위부터 같은 높이·목록 폭 전체로 쌓고 구분 구간을 건너뛴다
        foreach (MissionMenuRow row in rows)
        {
            yield return (row, new Rectangle(panel.X, y, panel.Width, MissionMenuRowHeight));
            y += MissionMenuRowHeight + (row.SeparatorAfter ? MissionMenuSeparatorHeight : 0);
        }
    }

    /// <summary>미션의 Esc·상단 메뉴·Leave 확인 창 입력을 처리하고 지도 입력 차단 여부를 돌려준다.</summary>
    private bool UpdateMissionMenu(KeyboardState keyboard, MouseState mouse, int width, int height)
    {
        if (!IsMissionMode)
        {
            return false;
        }
        bool clicked = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton != ButtonState.Pressed;
        if (_leaveMissionPrompt)
        {
            // 원본의 확인 창에서는 Esc가 동작하지 않고 버튼으로만 결정한다. 버튼은 누르는 순간 소리가 나고 안쪽에서 뗄 때 실행한다.
            Rectangle panel = LeaveMissionPanel(width, height);
            var buttons = new List<GumpButton>();
            // 세 선택지의 판정 영역
            for (int i = 0; i < 3; i++) buttons.Add(OriginalUiSkin.Hit(i, LeaveMissionButton(panel, i)));
            GumpResult result = _leaveGump.Update(buttons, mouse.X, mouse.Y, mouse.LeftButton == ButtonState.Pressed);
            if (result.Pressed != null) QueueSound(OriginalUiSkin.ButtonSound);
            if (result.Activated == 0) _pendingMissionMenuAction = MissionMenuAction.MainMenu;
            else if (result.Activated == 1) _pendingMissionMenuAction = MissionMenuAction.Replay;
            else if (result.Activated == 2)
            {
                _leaveMissionPrompt = false;
                CloseMissionMenu();
            }
            return true;
        }
        if (Pressed(keyboard, Keys.Escape))
        {
            bool open = !_missionMenuVisible;
            CloseMissionMenu();
            _missionMenuVisible = open;
            return true;
        }
        if (!_missionMenuVisible)
        {
            return false;
        }
        if (!clicked) return true;
        // 열린 목록의 행을 먼저 본다 (목록이 막대 아래로 펼쳐져 있다)
        if (_missionDropdown >= 0)
        {
            // 클릭한 행을 찾아 실행한다
            foreach ((MissionMenuRow row, Rectangle bounds) in MissionDropdownLayout(_missionDropdown))
            {
                if (!bounds.Contains(mouse.Position)) continue;
                if (!row.Enabled) return true;
                row.Action?.Invoke();
                // Leave Mission 은 확인 창을 남기고, 나머지는 원본처럼 메뉴 막대까지 닫는다
                if (!_leaveMissionPrompt) CloseMissionMenu();
                return true;
            }
        }
        // 상단 항목: Options 는 공유 옵션 목록, Players 는 멀티플레이 전용이라 비활성, 나머지는 펼침 목록을 연다
        for (int tab = 0; tab < MissionTabs().Length; tab++)
        {
            if (!MissionTab(tab).Contains(mouse.Position)) continue;
            if (tab == 2) { _pendingMissionMenuAction = MissionMenuAction.Options; CloseMissionMenu(); }
            else if (tab != 3) _missionDropdown = _missionDropdown == tab ? -1 : tab;
            return true;
        }
        // 목록 밖을 누르면 목록만 닫는다
        _missionDropdown = -1;
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

    /// <summary>상단 항목의 클릭 영역: 막대 왼쪽부터 53픽셀 간격으로 놓인다. 글자는 영역 왼쪽 + 5 에 쓴다.</summary>
    /// <param name="index">항목 번호 (0 Game · 1 View · 2 Options · 3 Players · 4 About)</param>
    private static Rectangle MissionTab(int index) =>
        new(MissionMenuLeft + index * MissionTabPitch, 0, MissionTabPitch, MissionMenuBarHeight);

    /// <summary>상단 돌 메뉴 막대·펼침 목록·떠나기 확인 창을 지도 위에 그린다.</summary>
    private void DrawMissionMenu(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (!IsMissionMode || (!_missionMenuVisible && !_leaveMissionPrompt) || TutorialDialogOpen && !_leaveMissionPrompt) return;
        if (_missionMenuVisible)
        {
            _uiSkin.Menu(batch, new Rectangle(MissionMenuLeft, 0, width - MissionMenuLeft, MissionMenuBarHeight));
            string[] tabs = MissionTabs();
            // 원본 메뉴 막대의 다섯 항목을 나란히 표시한다 (Players 는 멀티플레이 전용이라 흐리게)
            for (int i = 0; i < tabs.Length; i++)
            {
                Rectangle tab = MissionTab(i);
                if (i == _missionDropdown) batch.Draw(_pixel, tab, Color.Black * 0.25f);
                OriginalUiSkin.Text(batch, font, tabs[i], new Vector2(tab.X + MissionTabTextInset, 1), i == 3 ? new Color(185, 180, 166) : Color.White);
            }
            if (_missionDropdown >= 0)
            {
                List<MissionMenuRow> rows = MissionMenuRows(_missionDropdown);
                Rectangle panel = MissionDropdownPanel(_missionDropdown, rows);
                _uiSkin.Menu(batch, panel);
                // 커서 강조·켜짐 표시·"이름 - 키" 를 한 줄씩 그리고 그룹 뒤에 구분선을 붙인다
                foreach ((MissionMenuRow row, Rectangle bounds) in MissionDropdownLayout(_missionDropdown))
                {
                    if (row.Enabled && bounds.Contains(_previousMouse.Position)) batch.Draw(_pixel, bounds, Color.Black * 0.25f);
                    if (row.Check == true) _uiSkin.Pip(batch, new Point(bounds.X + OriginalUiSkin.MenuPipInset, bounds.Y + MissionMenuRowHeight / 2), true);
                    Color textColor = row.Enabled ? Color.White : new Color(185, 180, 166);
                    _uiSkin.MenuRow(batch, panel.X, bounds, row.Label, textColor, row.Key, row.Enabled ? MenuKeyColor : textColor);
                    if (row.SeparatorAfter)
                        _uiSkin.Separator(batch, panel.X + OriginalUiSkin.FrameThickness, bounds.Bottom, panel.Width - OriginalUiSkin.FrameThickness * 2);
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
                _uiSkin.Button(batch, LeaveMissionButton(panel, i), labels[i], pressed: _leaveGump.IsPressed(i));
            }
        }
    }
}
