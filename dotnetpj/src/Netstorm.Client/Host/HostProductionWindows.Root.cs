using Netstorm.Client.Gump;

namespace Netstorm.Client.Host; // 생산 부모/예산의 실제 생성과 재부착 순서다.

/// <summary>Source43e640/43e490/43da10/43ddd0의 전체 공통 UI 본체다.</summary>
public sealed partial class HostProductionWindows
{
    /// <summary>실제 현재 가상 부착 장치를 같은 생성 owner 범위에서 실행한다.</summary>
    private void VirtualAttach(uint token, float x, float y, uint parent, uint mode) => _tree.WithNodeOwner(this, () => { Need(_tree.TreeHooks.Attach)(token, x, y, parent, mode); return 0; });
    /// <summary>같은 실제 배경 생성기를 사용한다.</summary>
    private uint Background(float width, float height, uint frame)
    {
        uint token = Allocate(new GumpBackgroundNode()); _tree.Backgrounds.Construct(token, width, height, 0x02000000, 1, _globals.BackgroundType, frame); return token;
    }
    /// <summary>원문 전체 생성만 실행하며 추가 Open 배치나 Deck 동기화를 앞당기지 않는다.</summary>
    public uint Construct()
    {
        ObjectDisposedException.ThrowIf(_disposed, this); if (_token != 0) throw new InvalidOperationException("현재 생산 패널 중복 생성"); _constructing = true;
        try
        {
            Prepare(); _token = Allocate(_root); _tree.Factory.Construct(_token, 0, 0, 0, 1); _root.Vtable = 0x5069c0; _root.Panel.BridgeUntil = 0; _root.SecondTimer = 0; _root.Command = 44; _root.Flags |= 0xe00; _root.Panel.OldMoney = 0; _root.Panel.Temple = 0; _tree.Lifetime.SetIdle(_token, true);
            _globals.PolicyFirst = 0; _globals.PolicyByte = 0; _root.BaseWidth = 40; _root.Width = (float)(40.0 * _root.Scale); _root.BaseHeight = _tree.Create.ScreenHeight; _root.Height = (float)((double)_tree.Create.ScreenHeight * _root.Scale);
            _root.Budget = 0; _root.Budget = Allocate(_budget); _text.Single.Construct(_root.Budget, 0, 0, 0, 0); _budget.Vtable = 0x506948; _budget.Until = 0; _globals.BudgetSerial = _budget.Serial; _budget.Amount = 0; _budget.Flash = 0; _tree.Lifetime.SetIdle(_root.Budget, true);
            _tree.SetHelpText(_root.Budget, Need(_devices.Translate)("Storm Power Available")); _tree.SetHelpWord(_root.Budget, "moneyGump"); _budget.AlignFlags |= 4; _tree.Tree.Attach(_root.Budget, 0, 0, _token, 2); BudgetText();
            _root.Top = 0; _root.Top = Background(_root.BaseWidth, _root.BaseHeight, 4); var top = Node(_root.Top)!; top.AlignFlags |= 1; top.BaseHeight = 23; top.Height = (float)(23.0 * top.Scale); _tree.Tree.Attach(_root.Top, 0, 0, _token, 0);
            _root.ReservedAC = 0; _root.Panel.BridgeUntil = _devices.Now(); _root.Panel.Initialized = true; _slotLimit = (uint)_context.Options[1] * 2 + 2; _root.Panel.Slots = unchecked((int)_slotLimit);
            _root.BridgeGrid = 0; _root.BridgeGrid = Allocate(new InventoryGridNode()); _grid.ConstructGrid(_root.BridgeGrid, 2, (int)(_slotLimit / 2), 0x40); var bridge = (InventoryGridNode)Node(_root.BridgeGrid)!; bridge.Fixed = 1; bridge.BaseHeight = _slotLimit * 36 / 2; bridge.Height = (float)((double)(_slotLimit * 36 / 2) * bridge.Scale); bridge.AcceptedCursor = 0x40;
            _tree.SetHelpText(_root.BridgeGrid, Need(_devices.Translate)("Production Window")); _tree.SetHelpWord(_root.BridgeGrid, "teleportView"); bridge.AlignFlags |= 1; bridge.ItemScale = BitConverter.UInt32BitsToSingle(0x3f2147ae); VirtualAttach(_root.BridgeGrid, 0, 26, _token, 0);
            _root.Middle = 0; _root.Middle = Background(_root.BaseWidth, _root.BaseHeight, _globals.PanelFrame); _tree.SetHelpText(_root.Middle, Need(_devices.Translate)("Production Window")); _tree.SetHelpWord(_root.Middle, "teleportView"); var middle = Node(_root.Middle)!; middle.AlignFlags |= 1; middle.BaseHeight = _slotLimit * 36 / 2; middle.Height = (float)((double)(_slotLimit * 36 / 2) * middle.Scale); _tree.Tree.Attach(_root.Middle, 0, 26, _token, 0);
            _root.TeleportGrid = 0; _root.TeleportGrid = Allocate(new InventoryGridNode()); _grid.ConstructGrid(_root.TeleportGrid, 1, 6, 8); var teleport = (InventoryGridNode)Node(_root.TeleportGrid)!; teleport.BaseHeight = 250; teleport.Height = (float)(250.0 * teleport.Scale); teleport.AcceptedCursor = 8; teleport.AlignFlags |= 1; VirtualAttach(_root.TeleportGrid, 0, (float)((double)middle.Y + middle.BaseHeight), _token, 0);
            _root.Bottom = 0; _root.Bottom = Background(_root.BaseWidth, _root.BaseHeight, _globals.PanelFrame); _tree.SetHelpText(_root.Bottom, Need(_devices.Translate)("Production Window")); _tree.SetHelpWord(_root.Bottom, "teleportView"); var bottom = Node(_root.Bottom)!; bottom.AlignFlags |= 1; bottom.BaseHeight = 250; bottom.Height = (float)(250.0 * bottom.Scale); _tree.Tree.Attach(_root.Bottom, 0, teleport.Y, _token, 0);
            _root.Separator = 0; _root.Separator = Allocate(new HudDividerNode()); _divider.Construct(_root.Separator, 0, (int)_root.BaseWidth, 5, 0x02a00000, _globals.BackgroundType, _globals.BackgroundFrame); Node(_root.Separator)!.AlignFlags |= 1; VirtualAttach(_root.Separator, 0, 21, _token, 0);
            _root.ReservedC0 = 0; _root.ReservedC4 = 0; _globals.RootSerial = _root.Serial; _devices.Changed(); return _token;
        }
        finally { _constructing = false; }
    }
    /// <summary>설치 후 추가 폭/하단 예약과 한 번의 실제 동기화를 원문 위치에서 실행한다.</summary>
    public void Open()
    {
        Construct(); _root.BaseWidth = _root.Width = 80; _root.BaseHeight = _root.Height = _tree.Create.ScreenHeight - 80; Attach(_token, 0, 0, _tree.Create.Collection.Root, 0); Handle(_token, 0x0200003d, 1, 0);
        // 설치된 현재 내용물도 실제 격자로 다시 부착한다.
        foreach (uint token in new[] { _root.BridgeGrid, _root.TeleportGrid }) { _tree.Tree.Push(token); var node = Node(token)!; Owned(() => _grid.AttachGrid(token, node.X, node.Y, 0, 0)); } _devices.Changed();
    }
    /// <summary>같은 owner 범위에서 실제 자식 생성/부착을 실행한다.</summary>
    private void Owned(Action action) => _tree.WithNodeOwner(this, () => { action(); return 0; });
    /// <summary>예산의 단계색/아이콘·점멸·실제 문자 Fit와 원문 깊이를 보존한다.</summary>
    private void BudgetText()
    {
        char color = _budget.Amount < 1001 ? 'r' : _budget.Amount < 2001 ? 'y' : 'w'; string text = "~3~E~" + color + _budget.Amount + "~[I" + _globals.BackgroundType + ".3]"; _text.Single.Clear(_root.Budget); var depth = _budget.Depth; _tree.Tree.Push(_root.Budget);
        if ((_budget.Flash & 1) == 0) { bool editor = _context.Gameplay.World.Editor != 0; Style(_root.Budget, editor ? "" : text); if (!editor) _text.Single.Fit(_root.Budget); }
        _tree.Tree.Attach(_root.Budget, 0, 2, 0, 2); _budget.Depth = depth; _devices.Changed();
    }
    /// <summary>같은 가상 부착을 분배하고 현재 실제 자식만 Push→크기→재부착한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        if (token != 0 && token == _root.Budget) { _tree.Tree.Attach(token, x, y, parent, mode); return; }
        Prepare(); var node = Node(token); if (node is HudDividerNode) { _divider.Attach(token, x, y, parent, mode); return; } if (node is InventoryGridNode) { Owned(() => _grid.AttachGrid(token, x, y, parent, mode)); return; } if (node is InventoryItemNode) { Owned(() => _item.Attach(token, x, y, parent, mode)); return; } if (node is ImageGumpNode) { _image.Attach(token, x, y, parent, mode); return; }
        _tree.Tree.Attach(token, x, y, parent, mode); if (token != _token) return; _tree.Tree.Push(_root.Budget); _tree.Tree.Attach(_root.Budget, 0, 2, _token, 2);
        _tree.Tree.Push(_root.Bottom); var bottom = Node(_root.Bottom)!; bottom.BaseHeight = _root.BaseHeight - bottom.Y; bottom.Height = bottom.BaseHeight * bottom.Scale; _tree.Tree.Attach(_root.Bottom, bottom.X, bottom.Y, 0, 0);
        _tree.Tree.Push(_root.TeleportGrid); var teleport = Node(_root.TeleportGrid)!; teleport.BaseHeight = _root.BaseHeight - teleport.Y; teleport.Height = teleport.BaseHeight * teleport.Scale; var middle = Node(_root.Middle)!; Owned(() => _grid.AttachGrid(_root.TeleportGrid, 0, middle.Y + middle.BaseHeight, _token, 0));
        Node(_root.Top)!.Depth++; _budget.Depth = Node(_root.Top)!.Depth - 1; Node(_root.Middle)!.Depth++; Node(_root.Bottom)!.Depth++;
    }
    /// <summary>원래 토큰/내용물을 유지하며 높이가 달라진 경우만 재부착한다.</summary>
    public void Resize(int screenHeight)
    {
        if (screenHeight <= 80 || _token == 0) throw new ArgumentOutOfRangeException(nameof(screenHeight), "생산 패널 화면 높이/수명 오류"); float height = screenHeight - 80; if (_root.BaseHeight == height) return; _root.BaseHeight = _root.Height = height; _tree.Tree.Push(_token); Attach(_token, 0, 0, _tree.Create.Collection.Root, 0);
    }
}
