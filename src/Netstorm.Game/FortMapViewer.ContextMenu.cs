using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 오브젝트·생산 창 항목의 우클릭 정보 창. 원본 녹화(2026-10-01 네 번째 g552·g817·g824·g865·g660, 1-2 녹화 00:08·11:00)처럼
/// 금색 모서리 돌 창에 큰 제목(`이름 Level I`), 노란 값의 정보 줄(Owner·Alignment·Class·Carrying·Cost),
/// 밝은 안쪽 판에 구분선으로 묶은 명령(`Upgrade costs 800`·`Salvage gains 200` 뒤 Storm Power 아이콘)을 둔다.
/// 하위 창(Construct Building·Knowledge Available·Current Production)도 같은 모양이다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>정보 창의 최소 폭 (녹화 사제 창 약 215px)</summary>
    private const int ContextMinimumWidth = 200;

    /// <summary>정보 창의 최대 폭</summary>
    private const int ContextMaximumWidth = 380;

    /// <summary>창 왼쪽 끝에서 제목·정보 글자까지의 여백</summary>
    private const int ContextTextInset = 16;

    /// <summary>창 위쪽 끝에서 제목까지의 여백</summary>
    private const int ContextTopInset = 12;

    /// <summary>제목 줄 높이</summary>
    private const int ContextTitleHeight = 22;

    /// <summary>정보 줄 높이 (녹화 약 15px)</summary>
    private const int ContextInfoHeight = 15;

    /// <summary>정보 줄과 안쪽 판 사이 간격</summary>
    private const int ContextGroupGap = 10;

    /// <summary>안쪽 판의 좌우 여백</summary>
    private const int ContextInnerInset = 14;

    /// <summary>명령 줄 높이 (녹화 약 17px)</summary>
    private const int ContextRowHeight = 17;

    /// <summary>명령 그룹 사이 구분선 자리 높이</summary>
    private const int ContextSeparatorHeight = 8;

    /// <summary>창 아래쪽 여백</summary>
    private const int ContextBottomInset = 14;

    /// <summary>정보 값·강조 글자의 노란색 (녹화의 You·Sun·High Priest 등)</summary>
    private static readonly Color ContextValueColor = new(240, 214, 90);

    /// <summary>Storm Power 가 모자랄 때의 원본 빨간 경고 글자색</summary>
    private static readonly Color ContextWarningColor = new(255, 72, 56);

    /// <summary>쓸 수 없는 명령의 흐린 글자색</summary>
    private static readonly Color ContextDisabledColor = new(185, 180, 166);

    private TypeInfo? _contextType;
    private GameEntity? _contextEntity;
    private bool _contextBridge;
    private Point _contextPoint;
    private string? _contextSubmenu;
    private readonly List<ContextRow> _contextRows = [];
    private readonly List<ContextPanel> _contextPanels = [];
    /// <summary>지도 입력·가장자리 스크롤을 차단하는 우클릭 목록 상태.</summary>
    public bool ContextMenuOpen => _contextType != null || _contextBridge;

    /// <summary>정보 줄 하나: "이름: " 은 흰색, 값은 노란색(경고면 빨간 문장 한 줄)</summary>
    /// <param name="Label">이름 (값이 없으면 문장 전체)</param>
    /// <param name="Value">노란 값</param>
    /// <param name="Warning">빨간 경고 문장인지</param>
    private sealed record ContextInfo(string Label, string Value, bool Warning = false);

    /// <summary>명령 줄 하나</summary>
    /// <param name="Label">명령 문구 (하위 창이 있으면 " >" 포함)</param>
    /// <param name="Action">고르면 할 일 (null 이면 표시만)</param>
    /// <param name="Highlight">명령 문구 뒤에 노란색으로 붙일 대상 이름 (예: Drop 다음 High Priest)</param>
    /// <param name="StormIcon">뒤에 Storm Power 결정 아이콘을 붙일지</param>
    /// <param name="Enabled">고를 수 있는지</param>
    /// <param name="Accent">노란 글자로 쓸지 (Current Production 항목)</param>
    private sealed record ContextCommand(string Label, Action? Action, string Highlight = "", bool StormIcon = false, bool Enabled = true, bool Accent = false);

    /// <summary>창 하나 (부모 또는 하위 창)의 내용과 화면 영역</summary>
    /// <param name="Title">큰 제목</param>
    /// <param name="Infos">정보 줄</param>
    /// <param name="Groups">구분선으로 나눈 명령 그룹</param>
    /// <param name="Bounds">창 영역</param>
    /// <param name="Inner">명령을 담는 밝은 안쪽 판 영역 (명령이 없으면 빈 사각형)</param>
    private sealed record ContextPanel(string Title, List<ContextInfo> Infos, List<List<ContextCommand>> Groups, Rectangle Bounds, Rectangle Inner);

    /// <summary>들고 있는 다리는 회전시키고 나머지 유닛·생산 항목의 우클릭은 정보 메뉴를 연다.</summary>
    private bool TryOpenContextMenu(MouseState mouse)
    {
        if (!_playUi || mouse.RightButton != ButtonState.Pressed || _previousMouse.RightButton == ButtonState.Pressed
            || _placementMode || _session.Player(TestPlayer).HeldPiece != null) return false;
        TypeInfo? type = null; GameEntity? entity = null; bool bridge = false;
        // 생산 창 유닛·다리 칸은 정보 메뉴를 연다 (원본 FUN_0043d4c0 의 0x20000 → FUN_0043d330, opengump.wav)
        if (DeckUnitAt(mouse.Position, _playHeight) is { } unit) type = unit.Type;
        else if (DeckTraySlotAt(mouse.Position) != null) bridge = true;
        else if (!IsPlayUiPoint(mouse.X, mouse.Y))
        {
            (int x, int y) = CellAt(new(mouse.X, mouse.Y));
            entity = _session.EntityAt(x, y); type = entity?.Type;
        }
        if (type == null && !bridge) return false;
        QueueSound(OpenGumpSound);
        _contextType = type; _contextEntity = entity; _contextBridge = bridge;
        _contextPoint = mouse.Position; _contextSubmenu = null; return true;
    }

    /// <summary>하위 창을 열고 원본처럼 openGump.wav 를 낸다 (1-2 녹화 00:08.5 Construct 클릭 0.08초 뒤).</summary>
    /// <param name="name">하위 창 이름</param>
    private void OpenContextSubmenu(string name)
    {
        _contextSubmenu = name;
        QueueSound(OpenGumpSound);
    }

    /// <summary>정보 행은 그대로 두고 선택한 명령에서만 상위·하위 메뉴를 함께 닫는다.</summary>
    private void CloseContextMenu() { _contextType = null; _contextEntity = null; _contextBridge = false; _contextSubmenu = null; }

    /// <summary>우클릭 대상의 표시 레벨. 워크샵은 생산 덱의 단계, 사제는 1, 그 밖은 타입의 level이며 없으면 0이다.</summary>
    private int ContextLevel()
    {
        if (_contextEntity is not { } entity || _contextType == null) return _contextType?.Definition.GetInt("level") ?? 0;
        if (entity.Kind == ObjectKind.Workshop) return _session.Player(entity.Owner).Deck.WorkshopLevel(entity.Id);
        if (entity.Kind == ObjectKind.Priest) return 1;
        return _contextType.Definition.GetInt("level") ?? 0;
    }

    /// <summary>1~3 단계를 원본 제목처럼 로마 숫자로 바꾼다.</summary>
    private static string RomanLevel(int level) => level switch { 1 => "I", 2 => "II", 3 => "III", _ => level.ToString() };

    /// <summary>타입의 원소 이름 (원소가 없으면 None)</summary>
    private static string AlignmentOf(TypeInfo type) => Elements.FromTheme(type.Definition.GetString("theme"))?.ToString() ?? "None";

    /// <summary>현재 대상의 정보·명령과 실제 생산 덱의 등록 상태로 부모·하위 창을 만든다.</summary>
    private void BuildContextRows(int width, int height)
    {
        _contextRows.Clear(); _contextPanels.Clear();
        string title = _contextBridge ? "Bridge" : _contextType!.Definition.GetString("description") ?? _contextType.Name;
        bool own = _contextEntity?.Owner == TestPlayer;
        // 녹화 대조: 사제·워크샵·골렘 등은 제목 뒤에 "Level I"이 붙고 템플은 붙지 않는다.
        if (!_contextBridge && ContextLevel() is int level and > 0) title += $" Level {RomanLevel(level)}";
        var infos = new List<ContextInfo>();
        var groups = new List<List<ContextCommand>>();
        if (_contextEntity != null)
        {
            infos.Add(new("Owner: ", own ? "You" : $"Player {_contextEntity.Owner}"));
            // 녹화 대조: 원소가 없는 사제는 Alignment: None으로 표시한다.
            infos.Add(new("Alignment: ", AlignmentOf(_contextType!)));
            infos.Add(new("Class: ", _contextType!.Definition.GetString("class") ?? ""));
            // 1-2 녹화 11:00: 사제를 든 골렘은 Carrying 줄과 Drop High Priest 명령이 붙는다
            if (_session.Entity(_contextEntity.CarriedPriestId) is { } carried)
                infos.Add(new("Carrying: ", carried.Type.Definition.GetString("description") ?? "High Priest"));
        }
        else if (_contextType != null)
        {
            // 생산 창 항목: 녹화(01:05.5)의 Golem 메뉴처럼 소유자·원소·분류 뒤에 비용, SP 가 모자라면 원본 빨간 경고 문구
            int cost = StormPower.TypeCost(_contextType.Definition);
            infos.Add(new("Owner: ", "You"));
            infos.Add(new("Alignment: ", AlignmentOf(_contextType)));
            infos.Add(new("Class: ", _contextType.Definition.GetString("class") ?? ""));
            infos.Add(new("Cost: ", cost.ToString()));
            if (cost > _session.Player(TestPlayer).StormPower)
                infos.Add(new(Ui("Storm Power가 더 필요합니다!", "Need more Storm Power to Build!"), "", Warning: true));
        }
        if (own && _contextEntity!.Kind == ObjectKind.Priest)
        {
            groups.Add([new(Ui("건설", "Construct") + " >", () => OpenContextSubmenu("construct")),
                new(Ui("NetStorm 지식 보기", "View Netstorm Knowledge"), () => { CloseContextMenu(); OpenKnowledge(); })]);
        }
        if (own && _contextEntity is { Kind: ObjectKind.Transport } transport && transport.CarriedPriestId != 0)
        {
            string carriedName = _session.Entity(transport.CarriedPriestId)?.Type.Definition.GetString("description") ?? "High Priest";
            int dropX = transport.Footprint.AnchorX, dropY = transport.Footprint.AnchorY, transportId = transport.Id;
            groups.Add([new(Ui("내려놓기 ", "Drop "), () => { SubmitCommand(new DropPriestCommand(TestPlayer, transportId, dropX, dropY)); CloseContextMenu(); }, carriedName)]);
        }
        var tail = new List<ContextCommand>();
        if (own && _contextEntity!.Kind == ObjectKind.Workshop && _contextEntity.IsComplete)
        {
            GameEntity workshop = _contextEntity;
            int cost = BattleSession.WorkshopUpgradeCostFor(workshop.Type.Definition);
            groups.Add([new(Ui("현재 생산 보기", "View Current Production") + " >", () => OpenContextSubmenu("production")),
                new(Ui("지식을 생산에 등록", "Put Knowledge into Production") + " >", () => OpenContextSubmenu("knowledge")),
                new(Ui($"업그레이드 비용 {cost}", $"Upgrade costs {cost}"), () => { SubmitCommand(new UpgradeWorkshopCommand(TestPlayer, workshop.Id)); CloseContextMenu(); }, StormIcon: true)]);
        }
        if (own && _contextEntity!.Kind is not (ObjectKind.Priest or ObjectKind.Geyser))
        {
            int id = _contextEntity.Id;
            int refund = StormPower.SalvageValue(StormPower.TypeCost(_contextType!.Definition));
            tail.Add(new(Ui($"회수하면 {refund}", $"Salvage gains {refund}"), () => { SubmitCommand(new SalvageCommand(TestPlayer, id)); CloseContextMenu(); }, StormIcon: true));
        }
        string topic = _contextBridge ? "bridge" : _contextType!.Name + "Type";
        tail.Add(new(Ui("정보", "About"), () => { CloseContextMenu(); HelpRequested?.Invoke(topic); }));
        // 맵 위 오브젝트는 Player > 가 붙는다 (동맹·송금 등 멀티플레이 명령이라 아직 쓸 수 없다)
        if (_contextEntity != null) tail.Add(new(Ui("플레이어", "Player") + " >", null, Enabled: false));
        groups.Add(tail);
        ContextPanel parent = LayoutContextPanel(title, infos, groups, _contextPoint, width, height);
        if (_contextSubmenu == null) return;
        var childInfos = new List<ContextInfo>();
        var childGroups = new List<List<ContextCommand>>();
        string childTitle = "";
        if (_contextSubmenu == "construct")
        {
            childTitle = Ui("건물 건설", "Construct Building");
            var rows = new List<ContextCommand> { new(Ui("템플", "Temple") + " >", () => OpenContextSubmenu("temples")), new(Ui("워크샵", "Workshop") + " >", () => OpenContextSubmenu("workshops")) };
            TypeInfo? altar = _candidates.FirstOrDefault(t => t.Name.Equals("altar", StringComparison.OrdinalIgnoreCase));
            string altarLevel = RomanLevel(altar?.Definition.GetInt("level") ?? 1);
            if (altar != null) rows.Add(Construct(altar, Ui($"제단 Level {altarLevel} 건설: ", $"Build Level {altarLevel} Altar for ")));
            childGroups.Add(rows);
        }
        else if (_contextSubmenu is "temples" or "workshops")
        {
            ObjectKind kind = _contextSubmenu == "temples" ? ObjectKind.Temple : ObjectKind.Workshop;
            childTitle = _contextSubmenu == "temples" ? Ui("템플", "Temple") : Ui("워크샵", "Workshop");
            // 사제가 실제로 건설할 수 있는 타입만 원소 순서로 표시한다.
            childGroups.Add([.. _candidates.Where(t => ObjectKinds.Of(t) == kind && IsBuildable(t))
                .Select(t => Construct(t, Ui($"{t.Definition.GetString("description")} 건설: ", $"{t.Definition.GetString("description")} for ")))]);
        }
        else if (_contextEntity is { Kind: ObjectKind.Workshop } workshop)
        {
            PlayerState player = _session.Player(TestPlayer);
            if (_contextSubmenu == "knowledge")
            {
                childTitle = Ui("등록할 수 있는 지식", "Knowledge Available");
                int free = player.Deck.FreeSlots(workshop.Id);
                childInfos.Add(new(Ui("남은 생산 칸: ", "Production Slots Available: "), free.ToString()));
                IEnumerable<ProducibleUnit> units = _candidates.OrderBy(type => Array.IndexOf(ProductionGroupOrder,
                    type.Definition.GetString("group")?.ToLowerInvariant())).Select(ProducibleUnit.FromType).OfType<ProducibleUnit>();
                // 등록된 지식은 이 목록에서 빠지고 등록할 워크샵의 원소 조건을 적용한다.
                childGroups.Add([.. player.Deck.AvailableKnowledge(workshop.Id, units).Select(unit =>
                {
                    string name = unit.Name;
                    return new ContextCommand(ProductionLabel(name), () => { SubmitCommand(new RegisterKnowledgeCommand(TestPlayer, workshop.Id, name)); CloseContextMenu(); });
                })]);
            }
            else
            {
                childTitle = Ui("현재 생산", "Current Production");
                // 이 워크샵에 등록된 생산 항목은 노란 글자로 보여 주고, 원본처럼 구분선 뒤에 About 을 붙인다.
                childGroups.Add([.. player.Deck.RegisteredAt(workshop.Id).Select(name =>
                    new ContextCommand(ProductionLabel(name), () => { CloseContextMenu(); ChooseProduction(name); }, Accent: true))]);
                childGroups.Add([new(Ui("정보", "About"), () => { CloseContextMenu(); HelpRequested?.Invoke(workshop.Type.Name + "Type"); })]);
            }
        }
        if (childGroups.All(g => g.Count == 0)) childGroups = [[new(Ui("없음", "None"), null, Enabled: false)]];
        // 원본처럼 하위 창은 부모 오른쪽에 겹쳐 열고, 화면 끝이면 왼쪽에 연다
        Point anchor = new(parent.Bounds.Right - 20, parent.Bounds.Y + 10);
        LayoutContextPanel(childTitle, childInfos, childGroups, anchor, width, height, parent.Bounds);
        // 선택한 건물을 배치 커서로 전환하는 명령 줄 (이름 뒤에 비용과 Storm Power 아이콘)
        ContextCommand Construct(TypeInfo type, string prefix)
        {
            string name = type.Name;
            return new ContextCommand(prefix + StormPower.TypeCost(type.Definition), () => { CloseContextMenu(); ChooseProduction(name); }, StormIcon: true);
        }
    }

    /// <summary>창 하나의 크기를 글자 폭으로 정하고 화면 안에 놓은 뒤, 명령 줄의 입력 영역을 기록한다.</summary>
    /// <param name="parent">하위 창이면 부모 창 영역 (오른쪽이 모자라면 부모 왼쪽에 연다)</param>
    private ContextPanel LayoutContextPanel(string title, List<ContextInfo> infos, List<List<ContextCommand>> groups, Point anchor,
        int width, int height, Rectangle? parent = null)
    {
        SpriteFontBase body = _uiSkin.Body;
        int icon = _uiSkin.FrameWidth("A03") + 4;
        // 제목·정보·명령 가운데 가장 긴 글자에 맞춘다
        float textWidth = Math.Max(_uiSkin.Title.MeasureString(title).X, infos.Count == 0 ? 0 : infos.Max(i => body.MeasureString(i.Label + i.Value).X));
        float commandWidth = groups.SelectMany(g => g).Select(c => body.MeasureString(c.Label + c.Highlight).X + (c.StormIcon ? icon : 0)).DefaultIfEmpty(0).Max();
        int panelWidth = Math.Clamp((int)Math.Max(textWidth + ContextTextInset * 2, commandWidth + ContextInnerInset * 2 + 24), ContextMinimumWidth, ContextMaximumWidth);
        List<List<ContextCommand>> filled = [.. groups.Where(g => g.Count > 0)];
        int rows = filled.Sum(g => g.Count);
        int innerHeight = rows == 0 ? 0 : rows * ContextRowHeight + (filled.Count - 1) * ContextSeparatorHeight + 6;
        int panelHeight = ContextTopInset + ContextTitleHeight + infos.Count * ContextInfoHeight + ContextGroupGap + innerHeight + ContextBottomInset;
        int x = anchor.X;
        if (x + panelWidth > width - 2) x = parent is { } owner ? owner.X - panelWidth + 20 : width - panelWidth - 2;
        Rectangle bounds = new(Math.Max(2, x), Math.Clamp(anchor.Y, 2, Math.Max(2, height - panelHeight - 2)), panelWidth, panelHeight);
        int innerTop = bounds.Y + ContextTopInset + ContextTitleHeight + infos.Count * ContextInfoHeight + ContextGroupGap;
        Rectangle inner = rows == 0 ? Rectangle.Empty : new(bounds.X + ContextInnerInset, innerTop, bounds.Width - ContextInnerInset * 2, innerHeight);
        var panel = new ContextPanel(title, infos, filled, bounds, inner);
        _contextPanels.Add(panel);
        int y = inner.Y + 3;
        // 그룹 순서대로 명령 줄을 쌓고 그룹 사이에는 구분선 자리를 둔다
        foreach (List<ContextCommand> group in filled)
        {
            // 한 그룹 안의 명령 줄을 같은 높이로 쌓는다
            foreach (ContextCommand command in group)
            {
                _contextRows.Add(new(new(inner.X + 2, y, inner.Width - 4, ContextRowHeight), command, bounds, parent != null));
                y += ContextRowHeight;
            }
            y += ContextSeparatorHeight;
        }
        return panel;
    }

    /// <summary>메뉴 밖 클릭을 닫힘으로 소비하고 하위 항목의 명령을 실행한다.</summary>
    private void UpdateContextMenu(MouseState mouse, int width, int height)
    {
        BuildContextRows(width, height);
        if (mouse.LeftButton != ButtonState.Pressed || _previousMouse.LeftButton == ButtonState.Pressed) return;
        ContextRow? row = _contextRows.LastOrDefault(r => r.Bounds.Contains(mouse.Position));
        if (row != null) { if (row.Command.Enabled) row.Command.Action?.Invoke(); }
        else if (!_contextRows.Any(r => r.Panel.Contains(mouse.Position))) CloseContextMenu();
    }

    /// <summary>금색 모서리 돌 창·큰 제목·노란 값 정보·밝은 안쪽 판의 명령 줄을 그린다. 커서 행은 어둡게 강조한다.</summary>
    private void DrawContextMenu(SpriteBatch batch, int width, int height)
    {
        if (!ContextMenuOpen) return;
        BuildContextRows(width, height);
        SpriteFontBase body = _uiSkin.Body;
        // 부모 창 위에 하위 창을 차례로 그린다
        foreach (ContextPanel panel in _contextPanels)
        {
            _uiSkin.Panel(batch, panel.Bounds);
            OriginalUiSkin.Text(batch, _uiSkin.Title, panel.Title, new(panel.Bounds.X + ContextTextInset, panel.Bounds.Y + ContextTopInset));
            int y = panel.Bounds.Y + ContextTopInset + ContextTitleHeight;
            // 정보 줄: 이름은 흰색, 값은 노란색, 경고는 빨간 문장
            foreach (ContextInfo info in panel.Infos)
            {
                var position = new Vector2(panel.Bounds.X + ContextTextInset, y);
                OriginalUiSkin.Text(batch, body, info.Label, position, info.Warning ? ContextWarningColor : Color.White);
                if (info.Value.Length > 0) OriginalUiSkin.Text(batch, body, info.Value, position + new Vector2(body.MeasureString(info.Label).X, 0), ContextValueColor);
                y += ContextInfoHeight;
            }
            if (panel.Inner == Rectangle.Empty) continue;
            batch.Draw(_pixel, panel.Inner, Color.White * 0.13f);
            _uiSkin.Bevel(batch, panel.Inner);
            // 그룹 사이(마지막 그룹 제외)에 구분선을 긋는다
            int lineY = panel.Inner.Y + 3;
            for (int group = 0; group < panel.Groups.Count - 1; group++)
            {
                lineY += panel.Groups[group].Count * ContextRowHeight;
                _uiSkin.Separator(batch, panel.Inner.X + 1, lineY + ContextSeparatorHeight / 2 - 1, panel.Inner.Width - 2);
                lineY += ContextSeparatorHeight;
            }
        }
        // 클릭 영역과 같은 줄에 명령을 그린다
        foreach (ContextRow row in _contextRows)
        {
            ContextCommand command = row.Command;
            if (command.Enabled && command.Action != null && row.Bounds.Contains(_previousMouse.Position)) batch.Draw(_pixel, row.Bounds, Color.Black * 0.25f);
            Color color = !command.Enabled ? ContextDisabledColor : command.Accent ? ContextValueColor : Color.White;
            var position = new Vector2(row.Bounds.X + 10, row.Bounds.Y + 1);
            OriginalUiSkin.Text(batch, body, command.Label, position, color);
            float x = position.X + body.MeasureString(command.Label).X;
            if (command.Highlight.Length > 0)
            {
                OriginalUiSkin.Text(batch, body, command.Highlight, new Vector2(x, position.Y), ContextValueColor);
                x += body.MeasureString(command.Highlight).X;
            }
            if (command.StormIcon) _uiSkin.DrawFrame(batch, "A03", new Point((int)x + 3, row.Bounds.Y + 1));
        }
    }

    /// <summary>우클릭 메뉴의 명령 줄 입력 영역·명령·소속 창.</summary>
    /// <param name="Bounds">명령 줄 영역</param>
    /// <param name="Command">명령</param>
    /// <param name="Panel">속한 창 영역</param>
    /// <param name="Submenu">하위 창의 줄인지</param>
    private sealed record ContextRow(Rectangle Bounds, ContextCommand Command, Rectangle Panel, bool Submenu);
}
