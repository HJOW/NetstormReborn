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
    /// 워크샵 지식 목록의 group 순서. 원본 지식 창의 행 안 순서(battery → cannon → archer → walker → blocker → fence → aviary → balloon)를 따른다.
    /// 골렘(walker)은 템플이 공급하므로 뺀다.
    /// </summary>
    private static readonly string[] ProductionGroupOrder = ["battery", "cannon", "archer", "blocker", "fence", "aviary", "balloon"];

    /// <summary>녹음에서 들린 골렘 명령 응답음 변형</summary>
    private static readonly string[] GolemMoveSounds = ["golemMove1.wav", "golemMove2.WAV", "golemMove4.wav", "golemMove5.wav"];

    /// <summary>녹음에서 들린 사제 명령 응답음 변형</summary>
    private static readonly string[] PriestMoveSounds = ["priestmove1.wav", "priestMove3.WAV"];

    /// <summary>명령 응답음 변형을 고르는 난수 (화면 연출 전용, 세션 결정론과 무관)</summary>
    private readonly Random _orderSoundRandom = new();

    /// <summary>생산 창에서 마지막으로 집은 유닛 (D 키로 다시 집는다, 원본 DAT_00557d6c)</summary>
    private string? _lastProductionType;
    private bool _playUi;
    private int _playWidth;
    private int _playHeight;
    private Texture2D? _sky;
    /// <summary>자동 입력 검사가 확인할 플레이 화면 상태.</summary>
    public string UiState => _leaveMissionPrompt ? "leave" : KnowledgeOpen ? "knowledge" : TutorialDialogOpen ? "briefing" : ContextMenuOpen ? "context:" + (_contextSubmenu ?? "main") : _missionMenuVisible ? "mission-menu"
        : _placementMode ? "placement" : _bridgeMode ? (_session.Player(TestPlayer).HeldPiece != null ? "holding" : "bridges") : "battle";

    /// <summary>첫 캠페인에서 개발용 규칙 변경 입력을 차단하고 플레이 조작을 켠다.</summary>
    public void EnablePlayUi(GameResources resources)
    {
        _playUi = true; _simulationPaused = false;
        _previousMouse = Mouse.GetState(); _previousKeyboard = Keyboard.GetState();
        _sky = MainMenuView.LoadImage(_device, resources, "d/Gifcloud.gif");
        CenterOnPriest();
    }

    /// <summary>생산 항목에 표시할 유닛 이름. 한국어는 group 별 짧은 이름, 영어는 원본 description.</summary>
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

    /// <summary>
    /// 생산 창 유닛이나 사제 Construct 의 건물을 배치 커서로 집는다. 원본처럼 유닛은 워크샵에 등록되어 덱에 있어야 하며
    /// 덱에 없으면 등록하지 않고 알린다(이전 클론의 자동 등록은 원본과 달라 없앴다).
    /// </summary>
    /// <param name="name">타입 이름</param>
    /// <param name="builderId">건물을 맡을 사제 번호 (Construct 메뉴를 연 사제, 유닛이나 개발용 선택이면 0)</param>
    private void ChooseProduction(string name, int builderId = 0)
    {
        TypeInfo type = _candidates.First(t => t.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
        if (!IsBuilding(type))
        {
            if (!_session.Player(TestPlayer).Deck.Entries().Any(e => e.Kind != DeckEntryKind.Bridge && e.TypeName.Equals(name, StringComparison.OrdinalIgnoreCase)))
            {
                _notice = Ui("워크샵에서 생산에 등록해야 합니다.", "Put this knowledge into production at a workshop first.");
                return;
            }
            _lastProductionType = name;
        }
        _bridgeMode = false;
        _buildingPriestId = builderId;
        StartPlacement(name, null);
    }

    /// <summary>배치·다리 커서를 취소하고 들고 있던 다리를 되돌린다.</summary>
    private void CancelCursor()
    {
        _placementMode = false; _bridgeMode = false; _dropPriestArmed = false;
        if (_session.Player(TestPlayer).HeldPiece != null) SubmitCommand(new ReturnBridgePieceCommand(TestPlayer));
    }

    /// <summary>지도 명령으로 전달하지 않을 생산 창(덱)·미니맵·메뉴 막대의 클릭을 소비한다.</summary>
    private bool UpdatePlayUi(MouseState mouse, int width, int height)
    {
        _playWidth = width; _playHeight = height;
        if (TryOpenContextMenu(mouse)) return true;
        bool clicked = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released;
        // 미니맵 누르기·끌기는 버튼을 떼기 전까지 화면을 계속 옮긴다 (커서가 상자 밖으로 나가도 유지)
        if (UpdateMiniMap(mouse, width, height, clicked)) return true;
        if (clicked)
        {
            // 생산 창 유닛 칸: 좌클릭으로 배치 커서에 집는다 (원본 FUN_0043d4c0 의 0x10000 처리).
            // 원본(0x43ec9e~0x43eccd)은 재충전 중이면 아무것도 하지 않고, Storm Power 가 모자라면 집지 않고 숫자를 깜빡인다.
            if (DeckUnitAt(mouse.Position, height) is { } unit)
            {
                if (_session.IsUnitRecharging(TestPlayer, unit.Type.Name)) return true;
                if (StormPower.TypeCost(unit.Type.Definition) > _session.Player(TestPlayer).StormPower) { BlinkStormPower(); return true; }
                CancelCursor(); ChooseProduction(unit.Type.Name);
                return true;
            }
            // 다리 칸: 조각을 집는다. 들고 있던 조각은 먼저 제자리로 돌린다
            if (DeckTraySlotAt(mouse.Position) is int slot)
            {
                PlayerState player = _session.Player(TestPlayer);
                _placementMode = false; _bridgeMode = true;
                if (player.HeldPiece != null) SubmitCommand(new ReturnBridgePieceCommand(TestPlayer));
                SubmitCommand(new PickBridgePieceCommand(TestPlayer, slot)); _heldRotation = 0;
                return true;
            }
        }
        if (_placementMode && mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Released)
        {
            // 고정 캐논·Crossbow의 사격 방향과 Wind Tower의 면역 면을 배치 전에 고른다.
            if (EmplacementDirection.RequiresChoice(_candidates[_candidateIndex]))
            {
                _cannonRotation = (_cannonRotation + (_reverseRotation ? 3 : 1)) % 4;
                QueueSound(RotatePieceSound);
            }
            else CancelCursor();
            return true;
        }
        if (_bridgeMode && mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Released && HeldPreview is { } held)
        { held.RotateByPlayer(_reverseRotation); _heldRotation = held.Rotation; QueueSound(RotatePieceSound); return true; }
        return IsPlayUiPoint(mouse.X, mouse.Y);
    }

    /// <summary>생산 창(사이드바)을 지도 배치·명령에서 제외한다. 원본은 상단 메뉴 막대·하단 상태줄이 없어 나머지는 모두 지도다.</summary>
    private bool IsPlayUiPoint(int x, int y) => x < PlaySidebarWidth;

    /// <summary>내 오브젝트 클릭은 선택, 가이저·적 사제·제단·빈 칸 클릭은 선택한 이동체의 명령으로 해석한다.</summary>
    private void UpdatePlayOrders(MouseState mouse)
    {
        if (_placementMode || _bridgeMode || IsPlayUiPoint(mouse.X, mouse.Y)) return;
        bool left = mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released;
        bool right = mouse.RightButton == ButtonState.Pressed && _previousMouse.RightButton == ButtonState.Released;
        if (!left && !right) return;
        (int x, int y) = CellAt(new Vector2(mouse.X, mouse.Y));
        // 사제·골렘은 칸이 아니라 그려진 그림(몸통·머리)으로 누른다. 건물·가이저는 차지한 칸으로 찾는다.
        GameEntity? target = PickEntityAt(mouse.Position);
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
            GameEntity? collector = selected is { Owner: TestPlayer, Kind: ObjectKind.Priest or ObjectKind.Transport } ? selected : null;
            SubmitCommand(new HarvestGeyserCommand(TestPlayer, target.Id, collector?.Id ?? 0));
            if (collector != null) AcknowledgeOrder(collector);
        }
        else if (selected is { Owner: TestPlayer, Kind: ObjectKind.Transport } && target?.Kind == ObjectKind.Priest)
        { SubmitCommand(new CapturePriestCommand(TestPlayer, selected.Id, target.Id)); AcknowledgeOrder(selected); }
        else if (target is { Kind: ObjectKind.Altar, Owner: TestPlayer } && selected?.Owner == TestPlayer)
        {
            if (selected.Kind == ObjectKind.Transport) { SubmitCommand(new DeliverPriestCommand(TestPlayer, selected.Id, target.Id)); AcknowledgeOrder(selected); }
            else if (selected.Kind == ObjectKind.Priest)
            {
                SubmitCommand(new MovePriestToAltarCommand(TestPlayer, target.Id, selected.Id));
                AcknowledgeOrder(selected);
                // 원본은 포로가 묶인 제단으로 사제를 보내는 명령 때 희생 음악을 요청한다 (2026-10-01 캠페인 1-2 녹화: 명령 18:28.2 → 음악 18:28.8, 도착은 18:32 무렵)
                if (_session.Entities.Any(e => e.Kind == ObjectKind.Priest && e.CaptorId == target.Id)) _mySacrificeMusicRequested = true;
            }
        }
        else if (selected?.Owner == TestPlayer && selected.Kind is ObjectKind.Priest or ObjectKind.Transport && target == null)
        {
            // 목적지는 클릭한 곳에 보이는 칸이다. 유닛 그림은 hotFootRatio 때문에 그 칸 안쪽(사제·골렘은 가로 가운데)에 선다.
            (int goalX, int goalY) = CellAt(new Vector2(mouse.X, mouse.Y));
            // 원본 TEST02: 사제의 허공 클릭은 이동·응답음 없이 선택만 풀고 하던 작업을 유지한다.
            if (selected.Kind == ObjectKind.Priest && !_session.Bridges.IsIsland(goalX, goalY)
                && _session.Bridges.At(goalX, goalY) is not { Owner: TestPlayer })
            {
                SubmitCommand(new SelectEntityCommand(TestPlayer, 0));
                return;
            }
            SubmitCommand(new MoveEntityCommand(TestPlayer, selected.Id, goalX, goalY)); AcknowledgeOrder(selected);
            // 원본은 이동 명령을 내리면(이동할 수 없는 곳이어도) 곧바로 선택이 풀린다 (2026-10-03 자동 분석 녹화).
            SubmitCommand(new SelectEntityCommand(TestPlayer, 0));
        }
        else if (left) SubmitCommand(new SelectEntityCommand(TestPlayer, target?.Id ?? 0));
    }

    /// <summary>
    /// 사제·골렘에게 명령을 내렸을 때의 응답음. 원본은 타입의 moveSound(골렘 golemMove1·사제 priestMove1) 계열을 쓰며,
    /// 2026-10-01 캠페인 1-2 녹음에서는 명령 클릭 0.06~0.10초 뒤에 골렘 golemMove1·2·4·5, 사제 priestmove1·priestMove3 이 섞여 들렸다.
    /// 들리지 않은 번호(GOLEMMOVE3·PRIESTMOVE2·4)는 쓰지 않는다. 다른 수송 유닛은 타입의 moveSound 를 그대로 쓴다.
    /// </summary>
    /// <param name="mover">명령을 받은 내 사제·수송 유닛</param>
    private void AcknowledgeOrder(GameEntity mover)
    {
        string? moveSound = mover.Type.Definition.GetString("moveSound");
        string[] variants = moveSound?.ToLowerInvariant() switch
        {
            "golemmove1.wav" => GolemMoveSounds,
            "priestmove1.wav" => PriestMoveSounds,
            _ => moveSound is { Length: > 0 } ? [moveSound] : [],
        };
        if (variants.Length > 0) QueueSound(variants[_orderSoundRandom.Next(variants.Length)]);
    }

    /// <summary>
    /// 원본 생산창 돌 바탕·Storm Power·다리/유닛 칸·미니맵을 그린다. 원본 미션 화면은 상단 메뉴 막대(Esc 로 열림)와
    /// 하단 상태줄이 없으므로 지도 위에는 타이머(T)와 짧은 알림만 그림자 글자로 띄운다(2026-10-01 녹화 대조).
    /// </summary>
    private void DrawPlayUi(SpriteBatch batch, SpriteFontBase font, SpriteFontBase small, int width, int height)
    {
        _uiSkin.Menu(batch, new Rectangle(0, 0, PlaySidebarWidth, height));
        // A02 는 위쪽 26px 이 검은 Storm Power 칸인 80×774 생산 창 바탕이므로 (0,0)에 그린다 (다리 칸은 y 26부터)
        _uiSkin.DrawFrame(batch, "A02", Point.Zero);
        _uiSkin.DrawFrame(batch, "A04", Point.Zero);
        // 원본은 표시 중인 숫자(실제 값을 천천히 따라감)로 색을 고르고, 깜빡임 중에는 숫자를 숨긴다 (FUN_0043da10)
        int shown = _shownStormPower ?? _session.Player(TestPlayer).StormPower;
        Color moneyColor = StormPower.DisplayColor(shown) switch
        { StormPowerColor.Red => Color.Red, StormPowerColor.Yellow => Color.Yellow, _ => Color.White };
        if (!StormPowerHidden) OriginalUiSkin.Text(batch, _uiSkin.Title, shown.ToString(), new Vector2(8, 1), moneyColor);
        _uiSkin.DrawFrame(batch, "A03", new Point(60, 4));
        TimeSpan time = TimeSpan.FromSeconds(_session.Seconds);
        if (_showTimer) OriginalUiSkin.Text(batch, small, $"{_mission?.Campaign?.Code ?? ""}  {(int)time.TotalMinutes:00}:{time.Seconds:00}", new Vector2(width - 94, 1));
        // 원본 생산 창: 다리 칸 2열과 유닛 칸 1열
        DrawDeck(batch, height);
        DrawMiniMap(batch, width, height);
        DrawNotice(batch, small, width, height);
    }

    /// <summary>공개 캠페인에서 내 명령이 거부된 이유를 화면 언어로 짧게 알린다.</summary>
    /// <param name="failure">거부 이유</param>
    private string PlayFailureText(CommandFailure failure) => Language == GameLanguage.Korean ? SessionText.Describe(failure) : failure switch
    {
        CommandFailure.TechDenied => "That knowledge is not allowed in this mission.",
        CommandFailure.NotInDeck => "Put this knowledge into production at a workshop first.",
        CommandFailure.NotReady => "Still recharging.",
        CommandFailure.Placement => "You cannot build there.",
        CommandFailure.RegisterFailed => "This workshop cannot produce that.",
        CommandFailure.NotOwner => "That is not yours.",
        CommandFailure.NotComplete => "Still under construction.",
        CommandFailure.SalvageDenied or CommandFailure.CannotSalvage => "That cannot be salvaged.",
        CommandFailure.TrayFull => "The bridge slots are full.",
        CommandFailure.BridgeBlocked => "The bridge cannot be placed there.",
        CommandFailure.NoPriest => "No High Priest to move.",
        CommandFailure.NoTemple => "No Temple to deliver to.",
        CommandFailure.GeyserEmpty => "The Storm Geyser is empty.",
        CommandFailure.NoRoute => "There is no path to the destination.",
        CommandFailure.NotCapturable => "That High Priest cannot be captured.",
        CommandFailure.AltarOccupied => "A High Priest is already bound to that Altar.",
        CommandFailure.NotCarrying => "Not carrying a High Priest.",
        CommandFailure.AlreadyCarrying => "Already carrying a High Priest.",
        _ => "Cannot do that.",
    };

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
