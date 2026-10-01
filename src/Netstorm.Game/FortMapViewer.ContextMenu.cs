using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>네 번째 녹화에서 확인한 사제·워크샵·생산 항목의 우클릭 메뉴.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>능력치와 명령을 읽을 수 있는 우클릭 목록 폭.</summary>
    private const int ContextWidth = 250;
    private TypeInfo? _contextType;
    private GameEntity? _contextEntity;
    private bool _contextBridge;
    private Point _contextPoint;
    private string? _contextSubmenu;
    private readonly List<ContextRow> _contextRows = [];
    /// <summary>지도 입력·가장자리 스크롤을 차단하는 우클릭 목록 상태.</summary>
    public bool ContextMenuOpen => _contextType != null || _contextBridge;

    /// <summary>들고 있는 다리는 회전시키고 나머지 유닛·생산 항목의 우클릭은 정보 메뉴를 연다.</summary>
    private bool TryOpenContextMenu(MouseState mouse)
    {
        if (!_playUi || mouse.RightButton != ButtonState.Pressed || _previousMouse.RightButton == ButtonState.Pressed
            || _placementMode || _session.Player(TestPlayer).HeldPiece != null) return false;
        TypeInfo? type = null; GameEntity? entity = null; bool bridge = false;
        PlayButton? button = _playButtons.FirstOrDefault(b => b.Bounds.Contains(mouse.Position));
        if (button != null)
        {
            int index = _playButtons.IndexOf(button);
            string? name = ProductionIcon(index);
            if (name != null) type = _candidates.FirstOrDefault(t => t.Name.Equals(name, StringComparison.OrdinalIgnoreCase));
            bridge = index == 6;
        }
        else if (mouse.X >= 6 && mouse.X < 6 + TraySlotSize.X * TrayColumns && mouse.Y >= HeaderHeight + 6
            && mouse.Y < HeaderHeight + 6 + TraySlotSize.Y * ((_session.Player(TestPlayer).Tray.Pieces.Count + TrayColumns - 1) / TrayColumns)) bridge = true;
        else if (!IsPlayUiPoint(mouse.X, mouse.Y))
        {
            (int x, int y) = CellAt(new(mouse.X, mouse.Y));
            entity = _session.EntityAt(x, y); type = entity?.Type;
        }
        if (type == null && !bridge) return false;
        _contextType = type; _contextEntity = entity; _contextBridge = bridge;
        _contextPoint = mouse.Position; _contextSubmenu = null; return true;
    }

    /// <summary>정보 행은 그대로 두고 선택한 명령에서만 상위·하위 메뉴를 함께 닫는다.</summary>
    private void CloseContextMenu() { _contextType = null; _contextEntity = null; _contextBridge = false; _contextSubmenu = null; }

    /// <summary>현재 대상의 능력치·명령과 실제 생산 덱의 등록 상태로 목록을 만든다.</summary>
    private void BuildContextRows(int width, int height)
    {
        _contextRows.Clear();
        var parent = new List<(string Label, Action? Action)>();
        string title = _contextBridge ? Ui("다리", "Bridge") : _contextType!.Definition.GetString("description") ?? _contextType.Name;
        bool own = _contextEntity?.Owner == TestPlayer;
        parent.Add((title, null));
        if (_contextEntity != null)
        {
            parent.Add(($"Owner: {(own ? "You" : _contextEntity.Owner)}", null));
            parent.Add(($"Class: {_contextType!.Definition.GetString("class")}", null));
        }
        else if (_contextType != null) parent.Add(($"Cost: {StormPower.TypeCost(_contextType.Definition)}", null));
        if (own && _contextEntity!.Kind == ObjectKind.Priest)
        {
            parent.Add((Ui("건설", "Construct") + " >", () => _contextSubmenu = "construct"));
            parent.Add((Ui("지식 보기", "View Netstorm Knowledge"), () => { CloseContextMenu(); OpenKnowledge(); }));
        }
        if (own && _contextEntity!.Kind == ObjectKind.Workshop && _contextEntity.IsComplete)
        {
            GameEntity workshop = _contextEntity;
            parent.Add((Ui("현재 생산 보기", "View Current Production") + " >", () => _contextSubmenu = "production"));
            parent.Add((Ui("지식을 생산에 등록", "Put Knowledge into Production") + " >", () => _contextSubmenu = "knowledge"));
            int cost = BattleSession.WorkshopUpgradeCostFor(workshop.Type.Definition);
            parent.Add(($"{Ui("업그레이드", "Upgrade")} ({cost})", () => { SubmitCommand(new UpgradeWorkshopCommand(TestPlayer, workshop.Id)); CloseContextMenu(); }));
        }
        if (own && _contextEntity!.Kind is not (ObjectKind.Priest or ObjectKind.Geyser))
        {
            int id = _contextEntity.Id;
            int refund = StormPower.SalvageValue(StormPower.TypeCost(_contextType!.Definition));
            parent.Add(($"{Ui("회수", "Salvage")} ({refund})", () => { SubmitCommand(new SalvageCommand(TestPlayer, id)); CloseContextMenu(); }));
        }
        string topic = _contextBridge ? "bridge" : _contextType!.Name + "Type";
        parent.Add((Ui("도움말", "About"), () => { CloseContextMenu(); HelpRequested?.Invoke(topic); }));
        Rectangle area = new(Math.Clamp(_contextPoint.X, 2, Math.Max(2, width - ContextWidth - 2)),
            Math.Clamp(_contextPoint.Y, 2, Math.Max(2, height - parent.Count * OriginalUiSkin.RowHeight - 4)),
            ContextWidth, parent.Count * OriginalUiSkin.RowHeight + 4);
        AddRows(parent, area, false);
        if (_contextSubmenu == null) return;
        var child = new List<(string Label, Action? Action)>();
        if (_contextSubmenu == "construct")
        {
            child.Add((Ui("템플", "Temple") + " >", () => _contextSubmenu = "temples"));
            child.Add((Ui("워크샵", "Workshop") + " >", () => _contextSubmenu = "workshops"));
            TypeInfo? altar = _candidates.FirstOrDefault(t => t.Name.Equals("altar", StringComparison.OrdinalIgnoreCase));
            if (altar != null) Construct(altar);
        }
        else if (_contextSubmenu is "temples" or "workshops")
        {
            ObjectKind kind = _contextSubmenu == "temples" ? ObjectKind.Temple : ObjectKind.Workshop;
            // 사제가 실제로 건설할 수 있는 타입만 원소 순서로 표시한다.
            foreach (TypeInfo type in _candidates.Where(t => ObjectKinds.Of(t) == kind && IsBuildable(t))) Construct(type);
        }
        else if (_contextEntity is { Kind: ObjectKind.Workshop } workshop)
        {
            PlayerState player = _session.Player(TestPlayer);
            if (_contextSubmenu == "knowledge")
            {
                IEnumerable<ProducibleUnit> units = _candidates.OrderBy(type => Array.IndexOf(ProductionGroupOrder,
                    type.Definition.GetString("group")?.ToLowerInvariant())).Select(ProducibleUnit.FromType).OfType<ProducibleUnit>();
                // 등록된 지식은 이 목록에서 빠지고 등록할 워크샵의 원소 조건을 적용한다.
                foreach (ProducibleUnit unit in player.Deck.AvailableKnowledge(workshop.Id, units))
                {
                    string name = unit.Name;
                    child.Add((ProductionLabel(name), () => { SubmitCommand(new RegisterKnowledgeCommand(TestPlayer, workshop.Id, name)); CloseContextMenu(); }));
                }
            }
            else
            {
                // 이 워크샵에 등록된 생산 항목만 보여 준다.
                foreach (string name in player.Deck.RegisteredAt(workshop.Id))
                    child.Add((ProductionLabel(name), () => { CloseContextMenu(); ChooseProduction(name); }));
            }
        }
        if (child.Count == 0) child.Add((Ui("없음", "None"), null));
        int subX = area.Right + ContextWidth < width ? area.Right - 1 : Math.Max(2, area.X - ContextWidth + 1);
        Rectangle sub = new(subX, Math.Clamp(area.Y, 2, Math.Max(2, height - child.Count * OriginalUiSkin.RowHeight - 4)),
            ContextWidth, child.Count * OriginalUiSkin.RowHeight + 4);
        AddRows(child, sub, true);
        // 선택한 건물을 배치 커서로 전환한다.
        void Construct(TypeInfo type)
        {
            string name = type.Name;
            child.Add(($"{type.Definition.GetString("description")} ({StormPower.TypeCost(type.Definition)})",
                () => { CloseContextMenu(); ChooseProduction(name); }));
        }
        // 부모·자식의 입력 영역과 그리기 위치를 함께 기록한다.
        void AddRows(List<(string Label, Action? Action)> rows, Rectangle panel, bool submenu)
        {
            // 원본의 낮은 돌 목록 간격으로 각 행을 배치한다.
            for (int i = 0; i < rows.Count; i++) _contextRows.Add(new(new(panel.X + 2, panel.Y + 2 + i * OriginalUiSkin.RowHeight,
                panel.Width - 4, OriginalUiSkin.RowHeight), rows[i].Label, rows[i].Action, panel, submenu));
        }
    }

    /// <summary>메뉴 밖 클릭을 닫힘으로 소비하고 하위 항목의 명령을 실행한다.</summary>
    private void UpdateContextMenu(MouseState mouse, int width, int height)
    {
        BuildContextRows(width, height);
        if (mouse.LeftButton != ButtonState.Pressed || _previousMouse.LeftButton == ButtonState.Pressed) return;
        ContextRow? row = _contextRows.LastOrDefault(r => r.Bounds.Contains(mouse.Position));
        if (row != null) row.Action?.Invoke();
        else if (!_contextRows.Any(r => r.Panel.Contains(mouse.Position))) CloseContextMenu();
    }

    /// <summary>커서의 어두운 행 강조를 정보 값이나 선택 표시와 구분하여 그린다.</summary>
    private void DrawContextMenu(SpriteBatch batch, int width, int height)
    {
        if (!ContextMenuOpen) return;
        BuildContextRows(width, height);
        // 부모 목록 위에 자식 목록을 차례로 그린다.
        foreach (Rectangle panel in _contextRows.Select(r => r.Panel).Distinct()) _uiSkin.Menu(batch, panel);
        // 클릭 영역과 동일한 낮은 행에 명령·정보를 표시한다.
        foreach (ContextRow row in _contextRows)
        {
            if (row.Action != null && row.Bounds.Contains(_previousMouse.Position)) batch.Draw(_pixel, row.Bounds, Color.Black * 0.25f);
            OriginalUiSkin.Text(batch, _uiSkin.Body, row.Label, new(row.Bounds.X + 7, row.Bounds.Y + 1));
        }
    }
    /// <summary>우클릭 메뉴의 표시·입력·소속 창.</summary>
    private sealed record ContextRow(Rectangle Bounds, string Label, Action? Action, Rectangle Panel, bool Submenu);
}
