using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Bridges;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>캠페인 1-1의 마우스 생산·건설·이동·수확 조작과 미니맵.</summary>
internal sealed partial class FortMapViewer
{
    private bool _playUi;
    private readonly List<PlayButton> _playButtons = [];
    private int _playWidth;
    private int _playHeight;
    private Texture2D? _sky;
    /// <summary>자동 입력 검사가 확인할 플레이 화면 상태.</summary>
    public string UiState => TutorialDialogOpen ? "briefing" : _leaveMissionPrompt ? "leave" : _missionMenuVisible ? "mission-menu"
        : _placementMode ? "placement" : _bridgeMode ? (_session.Player(TestPlayer).HeldPiece != null ? "holding" : "bridges") : "battle";

    /// <summary>첫 캠페인에서 개발용 규칙 변경 입력을 차단하고 플레이 조작을 켠다.</summary>
    public void EnablePlayUi(GameResources resources)
    {
        _playUi = true; _simulationPaused = false;
        _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
        _sky = MainMenuView.LoadImage(_device, resources, "d/Gifcloud.gif");
    }

    /// <summary>현재 UI 언어의 문구.</summary>
    private string Ui(string korean, string english) => Language == GameLanguage.Korean ? korean : english;

    /// <summary>생산 버튼을 선택하면 필요한 지식을 워크샵에 등록하고 배치 커서를 켠다.</summary>
    private void ChooseProduction(string name)
    {
        TypeInfo type = _candidates.First(t => t.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
        if (!IsBuilding(type) && !_session.Player(TestPlayer).Deck.Entries().Any(e => e.TypeName.Equals(name, StringComparison.OrdinalIgnoreCase)))
            RegisterSelectedUnit(type);
        _bridgeMode = false;
        StartPlacement(name, null);
    }

    /// <summary>선택한 워크샵 또는 첫 완공 워크샵을 업그레이드한다.</summary>
    private void UpgradeWorkshop()
    {
        GameEntity? selected = _session.Entity(_session.Player(TestPlayer).SelectedEntityId);
        GameEntity? workshop = selected is { Kind: ObjectKind.Workshop, Owner: TestPlayer } ? selected
            : _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Workshop && e.IsComplete);
        if (workshop == null) { _notice = Ui("먼저 Sun Workshop을 건설하십시오.", "Build a Sun Workshop first."); return; }
        SubmitCommand(new UpgradeWorkshopCommand(TestPlayer, workshop.Id));
    }

    /// <summary>배치·다리 커서를 취소하고 들고 있던 다리를 되돌린다.</summary>
    private void CancelCursor()
    {
        _placementMode = false; _bridgeMode = false; _dropPriestArmed = false;
        if (_session.Player(TestPlayer).HeldPiece != null) SubmitCommand(new ReturnBridgePieceCommand(TestPlayer));
    }

    /// <summary>플레이 버튼을 같은 입력·표시 목록에 배치한다.</summary>
    private void BuildPlayButtons(int width)
    {
        _playButtons.Clear();
        int slot = Math.Max(1, width / 7);
        // 두 줄의 기능 버튼을 같은 폭으로 배열한다.
        void Add(int index, string label, Action action, bool enabled = true) => _playButtons.Add(new PlayButton(
            new Rectangle(4 + index % 7 * slot, 31 + index / 7 * 31, slot - 8, 27), label, enabled, action));
        Add(0, Ui("워크샵 800", "Workshop 800"), () => ChooseProduction("sunFactory"));
        Add(1, Ui("제단", "Altar"), () => ChooseProduction("altar"));
        Add(2, Ui("골렘", "Golem"), () => ChooseProduction("sunWalker"), _session.Player(TestPlayer).HasTemple);
        Add(3, Ui("발전기", "Rain Generator"), () => ChooseProduction("rainBattery"));
        Add(4, Ui("대포", "Sun Cannon"), () => ChooseProduction("sunCannon"));
        Add(5, Ui("비행 기지", "Whirlibase"), () => ChooseProduction("sunAviary"));
        Add(6, Ui("다리", "Bridges"), () => { CancelCursor(); _bridgeMode = true; });
        Add(7, Ui("업그레이드", "Upgrade 1000"), UpgradeWorkshop);
        Add(8, Ui("내 사제", "My Priest"), () =>
        { CancelCursor(); CenterOnPriest(); GameEntity? priest = _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Priest); SubmitCommand(new SelectEntityCommand(TestPlayer, priest?.Id ?? 0)); });
        Add(9, Ui("정지", "Stop"), () => SubmitCommand(new StopEntityCommand(TestPlayer, _session.Player(TestPlayer).SelectedEntityId)));
        Add(10, Ui("회수", "Salvage"), () => SubmitCommand(new SalvageCommand(TestPlayer, _session.Player(TestPlayer).SelectedEntityId)));
        Add(11, Ui("지식", "Knowledge"), () => { CancelCursor(); OpenKnowledge(); });
        Add(12, Ui("취소", "Cancel"), CancelCursor);
        Add(13, Ui("기타 [잠금]", "Other [Locked]"), () => { }, false);
    }

    /// <summary>지도 명령으로 전달하지 않을 버튼·미니맵·다리 칸의 클릭을 소비한다.</summary>
    private bool UpdatePlayUi(MouseState mouse, int width, int height)
    {
        _playWidth = width; _playHeight = height;
        BuildPlayButtons(width);
        bool clicked = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released;
        if (clicked)
        {
            if (mouse.Y < 30 && mouse.X < 118)
            { CancelCursor(); _missionMenuVisible = true; _missionGameDropdown = true; return true; }
            PlayButton? button = _playButtons.FirstOrDefault(b => b.Bounds.Contains(mouse.X, mouse.Y));
            if (button != null) { if (button.Enabled) button.Action(); return true; }
            Rectangle mini = MiniMap(width, height);
            if (mini.Contains(mouse.X, mouse.Y))
            {
                _camera = WorldPixels((mouse.X - mini.X) * BridgeGrid.WorldSize / mini.Width,
                    (mouse.Y - mini.Y) * BridgeGrid.WorldSize / mini.Height);
                return true;
            }
            if (_bridgeMode && mouse.X < TraySlotSize.X * TrayColumns + 12 && mouse.Y >= HeaderHeight)
            {
                int column = (mouse.X - 6) / TraySlotSize.X;
                int row = (mouse.Y - HeaderHeight - 6) / TraySlotSize.Y;
                int index = row * TrayColumns + column;
                PlayerState player = _session.Player(TestPlayer);
                if (column >= 0 && column < TrayColumns && row >= 0 && index < player.Tray.Pieces.Count)
                {
                    if (player.HeldPiece != null) SubmitCommand(new ReturnBridgePieceCommand(TestPlayer));
                    SubmitCommand(new PickBridgePieceCommand(TestPlayer, index)); _heldRotation = 0;
                    return true;
                }
            }
        }
        if (_placementMode && mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Released)
        { CancelCursor(); return true; }
        if (_bridgeMode && mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Released && HeldPreview is { } held)
        { held.RotateByPlayer(_reverseRotation); _heldRotation = held.Rotation; QueueSound(RotatePieceSound); return true; }
        return IsPlayUiPoint(mouse.X, mouse.Y);
    }

    /// <summary>창 위·하단 안내·미니맵·다리 칸을 지도 배치에서 제외한다.</summary>
    private bool IsPlayUiPoint(int x, int y) => y < HeaderHeight || y >= _playHeight - (_placementMode || _bridgeMode ? 94 : 54)
        || MiniMap(_playWidth, _playHeight).Contains(x, y)
        || _bridgeMode && new Rectangle(0, HeaderHeight, TraySlotSize.X * TrayColumns + 12,
            ((_session.Player(TestPlayer).Tray.Capacity + TrayColumns - 1) / TrayColumns) * TraySlotSize.Y + 12).Contains(x, y);

    /// <summary>내 오브젝트 클릭은 선택, 가이저·적 사제·제단·빈 칸 클릭은 선택한 이동체의 명령으로 해석한다.</summary>
    private void UpdatePlayOrders(MouseState mouse)
    {
        if (_placementMode || _bridgeMode || IsPlayUiPoint(mouse.X, mouse.Y)) return;
        bool left = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released;
        bool right = mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Released;
        if (!left && !right) return;
        (int x, int y) = CellAt(new Vector2(mouse.X, mouse.Y));
        GameEntity? target = _session.EntityAt(x, y);
        GameEntity? selected = _session.Entity(_session.Player(TestPlayer).SelectedEntityId);
        if (left && target?.Owner == TestPlayer && !(target.Kind == ObjectKind.Altar && selected?.Kind is ObjectKind.Priest or ObjectKind.Transport))
        { SubmitCommand(new SelectEntityCommand(TestPlayer, target.Id)); return; }
        if (target?.Kind == ObjectKind.Geyser)
        {
            SubmitCommand(new HarvestGeyserCommand(TestPlayer, target.Id, selected?.Kind is ObjectKind.Priest or ObjectKind.Transport ? selected.Id : 0));
        }
        else if (selected is { Owner: TestPlayer, Kind: ObjectKind.Transport } && target?.Kind == ObjectKind.Priest)
            SubmitCommand(new CapturePriestCommand(TestPlayer, selected.Id, target.Id));
        else if (target is { Kind: ObjectKind.Altar, Owner: TestPlayer } && selected?.Owner == TestPlayer)
        {
            if (selected.Kind == ObjectKind.Transport) SubmitCommand(new DeliverPriestCommand(TestPlayer, selected.Id, target.Id));
            else if (selected.Kind == ObjectKind.Priest) SubmitCommand(new MovePriestToAltarCommand(TestPlayer, target.Id, selected.Id));
        }
        else if (selected?.Owner == TestPlayer && selected.Kind is ObjectKind.Priest or ObjectKind.Transport && target == null)
            SubmitCommand(new MoveEntityCommand(TestPlayer, selected.Id, x, y));
        else if (left) SubmitCommand(new SelectEntityCommand(TestPlayer, target?.Id ?? 0));
    }

    /// <summary>미니맵을 화면 오른쪽 아래 안내 줄 위에 배치한다.</summary>
    private static Rectangle MiniMap(int width, int height) => new(width - 160, height - 224, 152, 124);

    /// <summary>Storm Power·생산·선택 상태와 조작 안내를 표시한다.</summary>
    private void DrawPlayUi(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, int width, int height)
    {
        BuildPlayButtons(width);
        PlayerState player = _session.Player(TestPlayer);
        batch.Draw(_pixel, new Rectangle(0, 0, width, HeaderHeight), new Color(35, 32, 28));
        batch.DrawString(small, Ui("메뉴 / Esc", "Menu / Esc"), new Vector2(12, 8), Color.White);
        batch.DrawString(font, $"Storm Power: {player.StormPower:N0}", new Vector2(130, 3), Color.Gold);
        TimeSpan time = TimeSpan.FromSeconds(_session.Seconds);
        batch.DrawString(small, $"1-1  {(int)time.TotalMinutes:00}:{time.Seconds:00}", new Vector2(width - 118, 8), Color.White);
        // 버튼을 생산 재충전 상태와 함께 그린다.
        foreach (PlayButton button in _playButtons)
        {
            bool hovered = button.Bounds.Contains(_previousMouse.X, _previousMouse.Y);
            batch.Draw(_pixel, button.Bounds, !button.Enabled ? new Color(35, 35, 35) : hovered ? new Color(102, 83, 43) : new Color(66, 59, 47));
            Outline(batch, button.Bounds, button.Enabled ? Color.Tan : Color.DimGray);
            Vector2 size = small.MeasureString(button.Label);
            batch.DrawString(small, button.Label, new Vector2(button.Bounds.Center.X - size.X / 2, button.Bounds.Y + 5), button.Enabled ? Color.White : Color.Gray);
        }
        Rectangle mini = MiniMap(width, height);
        batch.Draw(_pixel, mini, new Color(35, 44, 64)); Outline(batch, mini, Color.Tan);
        // 원본 지면 칸을 월드 전체 범위에 축소해 표시한다.
        foreach (FortTerrainTile tile in _terrain.Tiles.Where(t => _session.Bridges.IsIsland(t.X, t.Y)))
            batch.Draw(_pixel, new Rectangle(mini.X + tile.X * mini.Width / BridgeGrid.WorldSize, mini.Y + tile.Y * mini.Height / BridgeGrid.WorldSize, 2, 2), new Color(93, 112, 64));
        // 살아 있는 오브젝트를 내 색·적 색으로 구분한다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Owner > 0))
            batch.Draw(_pixel, new Rectangle(mini.X + entity.Footprint.AnchorX * mini.Width / BridgeGrid.WorldSize, mini.Y + entity.Footprint.AnchorY * mini.Height / BridgeGrid.WorldSize, entity.Kind == ObjectKind.Priest ? 4 : 2, entity.Kind == ObjectKind.Priest ? 4 : 2), entity.Owner == TestPlayer ? Color.Turquoise : Color.OrangeRed);
        batch.Draw(_pixel, new Rectangle(0, height - 54, width, 54), new Color(35, 32, 28));
        GameEntity? selected = _session.Entity(player.SelectedEntityId);
        string state = selected == null ? Ui("적 사제 포획 → 내 제단 운반 → 내 사제를 제단으로", "Capture priest > deliver to altar > bring your priest")
            : $"{selected.DisplayName}  HP {selected.HitPoints:0}/{selected.MaxHitPoints:0}";
        if (selected is { Kind: ObjectKind.Workshop, Owner: TestPlayer }) state += $"  Level {_session.Player(TestPlayer).Deck.WorkshopLevel(selected.Id)}";
        batch.DrawString(small, state, new Vector2(12, height - 51), Color.Gold);
        batch.DrawString(small, Ui("클릭: 선택/명령 · 우클릭 드래그/방향키: 화면 이동 · 휠: 확대 · Esc: 메뉴", "Click: select/order | Right drag/arrows: pan | Wheel: zoom | Esc: menu"), new Vector2(12, height - 33), Color.Wheat);
        batch.DrawString(small, _notice, new Vector2(12, height - 17), Color.LightGreen);
    }

    /// <summary>플레이 버튼의 영역·문구·동작.</summary>
    private sealed record PlayButton(Rectangle Bounds, string Label, bool Enabled, Action Action);

    /// <summary>원본 1-1 본문과 버튼의 한국어 표시. 스크립트 동작·섹션·조건은 바꾸지 않는다.</summary>
    private TutorialDialogContent LocalizeFirstCampaign(TutorialDialogContent content)
    {
        if (!_playUi || Language != GameLanguage.Korean) return content;
        string[]? paragraphs = content.Section switch
        {
            "A." => ["“어둠과 폭정은 우리 족쇄의 자물쇠다. 열쇠가 될 용기는 누구에게 있는가?” — 빛의 탄식",
                "님버스의 주민들을 정복하고 노예로 삼은 어둠의 군주들은 하늘을 지배하고 있습니다.",
                "그들은 당신의 섬에 경비대를 주둔시키고 사제를 가두었지만, 대담한 기습으로 당신의 사제가 풀려났습니다. 이제 자유를 되찾고 모든 님버스를 위해 싸울 때입니다!"],
            "Succeeded" or "BadTeamDead" => ["당신의 섬이 자유를 되찾았습니다!", "더 많은 자유로운 섬들과 힘을 합쳐 어둠의 세력을 물리쳐야 합니다."],
            "Failed" => ["기습 공격이 실패했습니다.", "어둠의 세력은 주민들에게 보복하며 더 강한 경계를 명령했습니다. 이번 자유의 기회를 잃었습니다."],
            _ => null,
        };
        // 원본 명령 인자를 유지하고 표시 문자열만 번역한다.
        string Label(string value) => value switch
        {
            "Review Knowledge" => "지식 확인", "Play Mission" => "미션 시작", "Leave Missions" => "메인 메뉴", "Next Mission" => "다음 미션",
            "Continue" => "계속", "Try Again" => "다시 시도", "Replay Mission" => "다시 시작", "Main Menu" => "메인 메뉴", "Back" => "뒤로", _ => value,
        };
        return content with
        {
            Title = content.Section switch { "A." => "전쟁의 시작!", "Succeeded" or "BadTeamDead" => "승리!", "Failed" => "실패!", _ => content.Title },
            Runs = paragraphs == null ? content.Runs : paragraphs.Select(p => new TutorialTextRun(p, TutorialTextStyle.Body, TutorialTextBreak.Paragraph)).ToArray(),
            Buttons = content.Buttons.Select(b => b with { Label = Label(b.Label) }).ToArray(),
        };
    }
}
