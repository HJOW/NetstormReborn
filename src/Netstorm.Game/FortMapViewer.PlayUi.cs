using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Bridges;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>공개 캠페인(1-1·1-2)의 마우스 생산·건설·이동·수확 조작과 미니맵.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>원본 생산창·미니맵을 포함한 왼쪽 사이드바의 폭.</summary>
    private const int PlaySidebarWidth = 84;

    /// <summary>
    /// 생산 버튼 3~5번에 놓을 지식의 group 순서. 원본 지식 창의 행 안 순서(battery → cannon → archer → walker → blocker → fence → aviary → balloon)를 따른다.
    /// 골렘(walker)은 2번 버튼에 따로 두므로 뺀다.
    /// </summary>
    private static readonly string[] ProductionGroupOrder = ["battery", "cannon", "archer", "blocker", "fence", "aviary", "balloon"];

    /// <summary>생산 버튼 3~5번에 놓을 유닛 타입 (미션 시작 지식에서 고른다, 1-1: Rain Generator·Sun Cannon·Whirlibase).</summary>
    private string[] _productionTypes = [];
    private bool _playUi;
    private readonly List<PlayButton> _playButtons = [];
    private int _playWidth;
    private int _playHeight;
    private Texture2D? _sky;
    /// <summary>자동 입력 검사가 확인할 플레이 화면 상태.</summary>
    public string UiState => _leaveMissionPrompt ? "leave" : TutorialDialogOpen ? "briefing" : _missionMenuVisible ? "mission-menu"
        : _placementMode ? "placement" : _bridgeMode ? (_session.Player(TestPlayer).HeldPiece != null ? "holding" : "bridges") : "battle";

    /// <summary>첫 캠페인에서 개발용 규칙 변경 입력을 차단하고 플레이 조작을 켠다.</summary>
    public void EnablePlayUi(GameResources resources)
    {
        _playUi = true; _simulationPaused = false;
        _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
        _sky = MainMenuView.LoadImage(_device, resources, "d/Gifcloud.gif");
        _productionTypes = StartingProductionTypes();
        CenterOnPriest();
    }

    /// <summary>내 시작 지식에서 생산 버튼에 올릴 타입을 group 순서로 최대 세 개 고른다.</summary>
    private string[] StartingProductionTypes()
    {
        IReadOnlyCollection<string> known = _session.Player(TestPlayer).Deck.Knowledge;
        // group 순서대로, 같은 group 안에서는 지식 순서대로 모은다
        return [.. ProductionGroupOrder
            .SelectMany(group => known.Select(name => _candidates.FirstOrDefault(t => t.Name.Equals(name, StringComparison.OrdinalIgnoreCase)))
                .Where(type => type != null && string.Equals(type.Definition.GetString("group"), group, StringComparison.OrdinalIgnoreCase))
                .Select(type => type!.Name))
            .Distinct(StringComparer.OrdinalIgnoreCase).Take(3)];
    }

    /// <summary>생산 버튼 index(3~5)에 놓인 유닛 타입 (없으면 null).</summary>
    private string? ProductionType(int index) => index - 3 is int slot && slot >= 0 && slot < _productionTypes.Length ? _productionTypes[slot] : null;

    /// <summary>생산 버튼에 표시할 유닛 이름. 한국어는 group 별 짧은 이름, 영어는 원본 description.</summary>
    private string ProductionLabel(string typeName)
    {
        TypeInfo? type = _candidates.FirstOrDefault(t => t.Name.Equals(typeName, StringComparison.OrdinalIgnoreCase));
        string english = type?.Definition.GetString("description") ?? typeName;
        string korean = type?.Definition.GetString("group")?.ToLowerInvariant() switch
        {
            "battery" => "발전기", "cannon" => "대포", "archer" => "원반 투척기", "aviary" => "비행 기지",
            "blocker" => "방벽", "fence" => "울타리", "balloon" => "비행 수송", _ => english,
        };
        return Ui(korean, english);
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
        // 기존 생산·명령 버튼을 원본처럼 왼쪽 작은 두 열에 모은다.
        void Add(int index, string label, Action action, bool enabled = true) => _playButtons.Add(new PlayButton(
            new Rectangle(4 + index % 2 * 39, 164 + index / 2 * 28, 37, 26), label, enabled, action));
        Add(0, Ui("워크샵 800", "Workshop 800"), () => ChooseProduction("sunFactory"));
        Add(1, Ui("제단", "Altar"), () => ChooseProduction("altar"));
        Add(2, Ui("골렘", "Golem"), () => ChooseProduction("sunWalker"), _session.Player(TestPlayer).HasTemple);
        // 3~5번: 미션 시작 지식의 생산 유닛 (없는 칸은 비활성)
        for (int slot = 3; slot <= 5; slot++)
        {
            string? produce = ProductionType(slot);
            Add(slot, produce == null ? "" : ProductionLabel(produce), () => { if (produce != null) ChooseProduction(produce); }, produce != null);
        }
        Add(6, Ui("다리", "Bridges"), () => { CancelCursor(); _bridgeMode = true; });
        Add(7, Ui("업그레이드", "Upgrade 1000"), UpgradeWorkshop);
        Add(8, Ui("내 사제", "My Priest"), () =>
        { CancelCursor(); CenterOnPriest(); GameEntity? priest = _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Priest); SubmitCommand(new SelectEntityCommand(TestPlayer, priest?.Id ?? 0)); });
        Add(9, Ui("정지", "Stop"), () => SubmitCommand(new StopEntityCommand(TestPlayer, _session.Player(TestPlayer).SelectedEntityId)));
        Add(10, Ui("회수", "Salvage"), () => SubmitCommand(new SalvageCommand(TestPlayer, _session.Player(TestPlayer).SelectedEntityId)));
        Add(11, Ui("지식", "Knowledge"), () => { CancelCursor(); OpenKnowledge(); });
        Add(12, Ui("취소", "Cancel"), CancelCursor);
        Add(13, Ui("기타", "Other"), () => { }, false);
    }

    /// <summary>지도 명령으로 전달하지 않을 버튼·미니맵·다리 칸의 클릭을 소비한다.</summary>
    private bool UpdatePlayUi(MouseState mouse, int width, int height)
    {
        _playWidth = width; _playHeight = height;
        BuildPlayButtons(width);
        bool clicked = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released;
        if (clicked)
        {
            if (mouse.Y < 18 && mouse.X >= PlaySidebarWidth && mouse.X < PlaySidebarWidth + 50)
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
            if (mouse.X >= 6 && mouse.X < TraySlotSize.X * TrayColumns + 6 && mouse.Y >= HeaderHeight + 6)
            {
                int column = (mouse.X - 6) / TraySlotSize.X;
                int row = (mouse.Y - HeaderHeight - 6) / TraySlotSize.Y;
                int index = row * TrayColumns + column;
                PlayerState player = _session.Player(TestPlayer);
                if (column >= 0 && column < TrayColumns && row >= 0 && index < player.Tray.Pieces.Count)
                {
                    _placementMode = false; _bridgeMode = true;
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

    /// <summary>사이드바·메뉴·하단 상태줄을 지도 배치·명령에서 제외한다.</summary>
    private bool IsPlayUiPoint(int x, int y) => x < PlaySidebarWidth || y < HeaderHeight || y >= _playHeight - 36;

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
        // 원본 도움말: 가이저를 우클릭하면 남은 Storm Power 를 보여 준다 (왼쪽 클릭은 수확 명령)
        if (right && target?.Kind == ObjectKind.Geyser)
        {
            _notice = target.IsDepletedGeyser ? Ui("빈 Storm 가이저", "Empty Storm Geyser")
                : Ui($"Storm 가이저 · 남은 Storm Power {target.StoredStormPower}", $"Storm Geyser · Storm Power {target.StoredStormPower}");
        }
        else if (target?.Kind == ObjectKind.Geyser)
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

    /// <summary>원본처럼 미니맵을 사이드바 맨 아래에 배치한다.</summary>
    private static Rectangle MiniMap(int width, int height) => new(4, height - 74, 76, 70);

    /// <summary>사이드바 아이콘이 사용할 원본 유닛 타입 이름.</summary>
    private string? ProductionIcon(int index) => index switch
    {
        0 => "sunFactory", 1 => "altar", 2 => "sunWalker", 3 or 4 or 5 => ProductionType(index), _ => null,
    };

    /// <summary>그림 없는 명령 버튼에 사용할 짧은 표시 이름.</summary>
    private string CompactPlayLabel(int index) => index switch
    {
        6 => Ui("다리", "Bridge"), 7 => Ui("강화", "Up"), 8 => Ui("사제", "Home"), 9 => Ui("정지", "Stop"),
        10 => Ui("회수", "Sell"), 11 => Ui("지식", "Info"), 12 => Ui("취소", "X"), _ => Ui("기타", "Other"),
    };

    /// <summary>원본 생산창 돌 바탕·아이콘·Storm Power·왼쪽 미니맵을 그린다.</summary>
    private void DrawPlayUi(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, int width, int height)
    {
        BuildPlayButtons(width);
        PlayerState player = _session.Player(TestPlayer);
        _uiSkin.Menu(batch, new Rectangle(0, 0, PlaySidebarWidth, height));
        _uiSkin.DrawFrame(batch, "A02", new Point(2, 22));
        _uiSkin.DrawFrame(batch, "A04", Point.Zero);
        Color moneyColor = StormPower.DisplayColor(player.StormPower) switch
        { StormPowerColor.Red => Color.Red, StormPowerColor.Yellow => Color.Yellow, _ => Color.White };
        OriginalUiSkin.Text(batch, _uiSkin.Title, player.StormPower.ToString(), new Vector2(8, 1), moneyColor);
        _uiSkin.DrawFrame(batch, "A03", new Point(60, 4));
        _uiSkin.Menu(batch, new Rectangle(PlaySidebarWidth, 0, width - PlaySidebarWidth, 18));
        OriginalUiSkin.Text(batch, small, Ui("메뉴", "Game"), new Vector2(PlaySidebarWidth + 5, 1));
        TimeSpan time = TimeSpan.FromSeconds(_session.Seconds);
        OriginalUiSkin.Text(batch, small, $"{_mission?.Campaign?.Code ?? ""}  {(int)time.TotalMinutes:00}:{time.Seconds:00}", new Vector2(width - 94, 1));
        // 실제 유닛 그림을 작은 생산 버튼 안에 넣고 자세한 이름·비용은 상태줄에 표시한다.
        for (int i = 0; i < _playButtons.Count; i++)
        {
            PlayButton button = _playButtons[i];
            bool hover = button.Bounds.Contains(_previousMouse.Position);
            string? icon = ProductionIcon(i);
            _uiSkin.Button(batch, button.Bounds, icon == null ? CompactPlayLabel(i) : "", button.Enabled, hover,
                hover && _previousMouse.LeftButton == ButtonState.Pressed);
            if (icon != null && _candidates.FirstOrDefault(t => t.Name.Equals(icon, StringComparison.OrdinalIgnoreCase)) is { } type)
                DrawTypePicture(batch, type, new Rectangle(button.Bounds.X + 2, button.Bounds.Y + 2, button.Bounds.Width - 4, button.Bounds.Height - 4));
        }
        Rectangle mini = MiniMap(width, height);
        batch.Draw(_pixel, mini, Color.Black); _uiSkin.Bevel(batch, mini);
        // 원본 지면 칸을 월드 전체 범위에 축소해 표시한다.
        foreach (FortTerrainTile tile in _terrain.Tiles.Where(t => _session.Bridges.IsIsland(t.X, t.Y)))
            batch.Draw(_pixel, new Rectangle(mini.X + tile.X * mini.Width / BridgeGrid.WorldSize, mini.Y + tile.Y * mini.Height / BridgeGrid.WorldSize, 1, 1), new Color(93, 112, 64));
        // 살아 있는 오브젝트를 내 색·적 색으로 구분한다.
        foreach (GameEntity entity in _session.Entities.Where(e => e.Owner > 0))
            batch.Draw(_pixel, new Rectangle(mini.X + entity.Footprint.AnchorX * mini.Width / BridgeGrid.WorldSize, mini.Y + entity.Footprint.AnchorY * mini.Height / BridgeGrid.WorldSize, entity.Kind == ObjectKind.Priest ? 3 : 2, entity.Kind == ObjectKind.Priest ? 3 : 2), entity.Owner == TestPlayer ? Color.Turquoise : Color.OrangeRed);
        _uiSkin.Menu(batch, new Rectangle(PlaySidebarWidth, height - 36, width - PlaySidebarWidth, 36));
        GameEntity? selected = _session.Entity(player.SelectedEntityId);
        PlayButton? hovered = _playButtons.FirstOrDefault(b => b.Bounds.Contains(_previousMouse.Position));
        string state = hovered?.Label ?? (selected == null ? Ui("오브젝트를 선택하십시오.", "Select an object.")
            : $"{selected.DisplayName}  HP {selected.HitPoints:0}/{selected.MaxHitPoints:0}");
        if (selected is { Kind: ObjectKind.Workshop, Owner: TestPlayer }) state += $"  Level {player.Deck.WorkshopLevel(selected.Id)}";
        OriginalUiSkin.Text(batch, small, state, new Vector2(96, height - 34));
        string notice = _bridgeMode ? Ui("다리 칸 클릭: 선택 · 지도 클릭: 배치 · 우클릭: 회전", "Click tray: choose | Click map: place | Right click: rotate") : _notice;
        OriginalUiSkin.Text(batch, small, notice, new Vector2(96, height - 18), Color.Wheat);
    }

    /// <summary>플레이 버튼의 영역·문구·동작.</summary>
    private sealed record PlayButton(Rectangle Bounds, string Label, bool Enabled, Action Action);

    /// <summary>원본 캠페인 본문과 버튼의 한국어 표시(1-1·1-2). 스크립트 동작·섹션·조건은 바꾸지 않는다.</summary>
    private TutorialDialogContent LocalizeCampaign(TutorialDialogContent content)
    {
        if (!_playUi || Language != GameLanguage.Korean) return content;
        string code = _mission?.Campaign?.Code ?? "";
        string[]? paragraphs = (code, content.Section) switch
        {
            ("1-1", "A.") => ["“어둠과 폭정은 우리 족쇄의 자물쇠다. 열쇠가 될 용기는 누구에게 있는가?” — 빛의 탄식",
                "님버스의 주민들을 정복하고 노예로 삼은 어둠의 군주들은 하늘을 지배하고 있습니다.",
                "그들은 당신의 섬에 경비대를 주둔시키고 사제를 가두었지만, 대담한 기습으로 당신의 사제가 풀려났습니다. 이제 자유를 되찾고 모든 님버스를 위해 싸울 때입니다!"],
            ("1-1", "Succeeded" or "BadTeamDead") => ["당신의 섬이 자유를 되찾았습니다!", "더 많은 자유로운 섬들과 힘을 합쳐 어둠의 세력을 물리쳐야 합니다."],
            ("1-1", "Failed") => ["기습 공격이 실패했습니다.", "어둠의 세력은 주민들에게 보복하며 더 강한 경계를 명령했습니다. 이번 자유의 기회를 잃었습니다."],
            // 1-2 masterofwhirligigs.english 의 [A.]·[Succeeded][BadTeamDead]·[Failed] 번역
            ("1-2", "A.") => ["“님버스 전쟁의 내력과 기원은 바로 님버스의 전쟁이다.” — 역사가 갈트루프 엔딕스",
                "어둠의 군주들은 당신의 섬이 약하다고 믿습니다. 그들은 휘리기그의 지배자를 보내 당신의 땅을 빼앗고 백성을 노예로 삼으려 합니다.",
                "적들은 당신이 무능하다고 생각합니다. 그들의 사악한 대사제를 사로잡아 분노의 신들에게 제물로 바쳐 그들이 틀렸음을 증명하십시오."],
            ("1-2", "Succeeded" or "BadTeamDead") => ["당신은 이 잔혹하고 무자비한 공격으로부터 백성을 훌륭히 지켜냈습니다!",
                "이른바 휘리기그의 “지배자”의 대사제를 사로잡아 처단했습니다. 이제 누가 진정한 지배자인지 분명하지 않습니까?"],
            ("1-2", "Failed") => ["이번 전투에서 적 사제를 사로잡지 못했습니다.",
                "휘리기그의 지배자가 당신의 군대를 쓸어버렸습니다. 결국 어둠의 군주들의 판단이 옳았던 것 같습니다.",
                "이제 당신은 어둠의 군주들의 제국에서 하찮은 하수인이 되고, 당신의 백성은 사슬에 묶인 채 숨 쉴 때마다 당신의 이름을 저주하며 살아갈 것입니다."],
            _ => null,
        };
        string? briefingTitle = code switch { "1-1" => "전쟁의 시작!", "1-2" => "휘리기그의 지배자", _ => null };
        // 원본 명령 인자를 유지하고 표시 문자열만 번역한다.
        string Label(string value) => value switch
        {
            "Review Knowledge" => "지식 확인", "Play Mission" => "미션 시작", "Leave Missions" => "메인 메뉴", "Next Mission" => "다음 미션",
            "Continue" => "계속", "Try Again" => "다시 시도", "Replay Mission" => "다시 시작", "Main Menu" => "메인 메뉴", "Back" => "뒤로", _ => value,
        };
        return content with
        {
            Title = content.Section switch { "A." => briefingTitle ?? content.Title, "Succeeded" or "BadTeamDead" => "승리!", "Failed" => "실패!", _ => content.Title },
            Runs = paragraphs == null ? content.Runs : paragraphs.Select(p => new TutorialTextRun(p, TutorialTextStyle.Body, TutorialTextBreak.Paragraph)).ToArray(),
            Buttons = content.Buttons.Select(b => b with { Label = Label(b.Label) }).ToArray(),
        };
    }
}
