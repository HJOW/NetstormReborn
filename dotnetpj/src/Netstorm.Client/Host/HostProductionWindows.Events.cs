using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 생산 입력과 현재 같은 패널의 Idle/가상 수명 분배다.

/// <summary>Source43ec00/43e530/43ef10의 현재 UI 분기와 같은 공통 수명을 연결한다.</summary>
public sealed partial class HostProductionWindows
{
    /// <summary>원문 항목 serial의 실제 현재 위치에 안내를 생성한다.</summary>
    private void Failure(bool empty, uint serial) { uint token = Reference(serial); var point = _tree.Collection.Position(token); Owned(() => _hint.Show(empty, point.X, point.Y, token)); _devices.Changed(); }
    /// <summary>실제 격자/항목/부모의 입력을 같은 현재 상태로 분배한다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second)
    {
        Prepare(); var node = Node(token); if (node is HudDividerNode) return _dividerInput.Handle(token, code, first, second); if (node is InventoryGridNode) return _grid.HandleGrid(token, code, first, second); if (node is InventoryItemNode) return _item.Handle(token, code, first, second); if (token != _token) return _devices.BaseHandle(token, code, first, second);
        code &= 0x03ffffff;
        if (code == 0x02000008)
        {
            uint flags = Word(first, 20); if ((flags & 0x100) != 0 || _context.Gameplay.Cursor.LockDepth == 0) return 1;
            if ((flags & 8) != 0) { if (_item.Affordable(unchecked((ushort)first)) == 0) { _budget.Flash = 10; _budget.Until = _devices.Now() + FlashInterval; Failure(false, second); return 1; } if (_globals.StockEnabled != 0 && _context.Storage.Pool.Slot(unchecked((ushort)first))[30] == 0) { Failure(true, second); return 1; } }
            if (_context.Storage.Types[(int)Word(first, 18)].Flags2 == 0) return 1; _ = _context.Gameplay.RequireSession(); Need(_devices.Take)(unchecked((ushort)first), (Word(first, 20) & 8) != 0 ? 0x41u : 1); _devices.Sound("getPiece.wav"); _devices.Changed(); return 1;
        }
        if (code == 0x0200003d) { BindForm(_root.BridgeGrid, Technology()); BindForm(_root.TeleportGrid, Technology()); if (first == 1) Need(_devices.Synchronize)(); return 0; }
        if (code == ProductionShortcuts.BridgeCommand)
        {
            if (_context.Gameplay.Player.PlayerKnown == 0) throw new InvalidOperationException("생산 명시 PlayerKnown 진단"); uint parent = PlayerField(_context.Gameplay.Player.TemplateOwner, 0x9c); var finder = new RawContainedFinder(_context.Storage.Pool, _context.Gameplay.ProductionFinder);
            // 같은 템플릿 기술 부모의 flags40/slot BYTE를 순서대로 조회한다.
            for (int sid = finder.Begin(unchecked((ushort)parent)); sid != 0; sid = finder.Next()) if ((Word((uint)sid, 20) & 0x40) != 0 && _context.Storage.Pool.Slot(sid)[23] == first) { _ = _context.Gameplay.RequireSession(); Need(_devices.Take)((uint)sid, 0); _devices.Sound("getPiece.wav"); return 1; }
        }
        return 0;
    }
    /// <summary>현재 실제 예산 완급/점멸과 dirty→부모 재연결→유지를 순서대로 실행한다.</summary>
    public void Idle(uint token)
    {
        Prepare(); var node = Node(token); if (node is ProductionHintNode) { _hint.Idle(token); return; }
        if (token != 0 && token == _root.Budget)
        {
            int target = _context.Gameplay.Player.PlayerKnown != 0 ? unchecked((int)Money(_context.Gameplay.Player.BattleOwner)) : 0;
            if (_budget.Flash != 0 && _budget.Until <= _devices.Now()) { _budget.Flash--; _budget.Until = _devices.Now() + FlashInterval; BudgetText(); }
            if (_budget.Amount != target) { long distance = (long)target - _budget.Amount; _budget.Amount = unchecked(_budget.Amount + (distance < 0 ? distance < -300 ? -100 : distance < -30 ? -10 : -1 : distance > 300 ? 100 : distance > 30 ? 10 : 1)); if (_context.Storage.OwnerMode.Fort) _budget.Amount = target; BudgetText(); }
        }
        else if (token != 0 && token == _token)
        {
            var mode = _context.Storage.OwnerMode;
            if ((mode.Fort || mode.Battle || mode.Challenge) && !mode.Challenge && Technology() != 0) { _ = _context.Gameplay.RequireSession(); if (Need(_devices.UpdateContents)()) _devices.Changed(); }
            uint parent = Technology(); if (parent == 0 || (!mode.Fort && !mode.Battle) || mode.Challenge) return;
            // 부모가 달라진 실제 격자만 원문 순서로 Push/Bind/재부착한다.
            if (_context.Gameplay.World.Editor == 0) foreach (uint current in new[] { _root.BridgeGrid, _root.TeleportGrid }) if (Node(current)!.Object != parent) { _tree.Tree.Push(current); BindForm(current, parent); var grid = Node(current)!; Owned(() => _grid.AttachGrid(current, grid.X, grid.Y, 0, 0)); }
            if (_maintenance is null) { _ = _context.Gameplay.RequireSession(); _maintenance = Need(_devices.Maintenance)(_root.Panel, Refresh); } _maintenance.Run();
        }
        else if (node is InventoryItemNode) _grid.IdleItem(token);
    }
    /// <summary>텔레포트 격자를 먼저, 다리 격자를 뒤에 실제 재부착한다.</summary>
    private void Refresh(uint mask)
    {
        // 같은 현재 노드와 위치를 각 호출 직전에 읽는다.
        foreach (var entry in new[] { (Token: _root.TeleportGrid, Bit: 1u), (Token: _root.BridgeGrid, Bit: 2u) }) if ((mask & entry.Bit) != 0) { _tree.Tree.Push(entry.Token); var node = Node(entry.Token)!; Owned(() => _grid.AttachGrid(entry.Token, node.X, node.Y, 0, 0)); } _devices.Changed();
    }
    /// <summary>현재 자기 노드만 선택하는 weak 가상 슬롯/외부 그림을 등록한다.</summary>
    private void Bind()
    {
        _releaseBase = Need(_tree.LifetimeHooks.Release); var weak = new WeakReference<HostProductionWindows>(this); HostProductionWindows? Resolve(uint token) => weak.TryGetTarget(out var owner) && !owner._disposed && owner.Node(token) is not null ? owner : null;
        var attach = _tree.TreeHooks.Attach; var events = _tree.EventHooks.Handle; var input = _tree.InputHooks.Handle; var idle = _tree.FrameHooks.Idle; var deleted = _tree.LifetimeHooks.DeletingDestructor; var broadcast = _tree.FrameHooks.Broadcast; var release = _releaseBase;
        _tree.TreeHooks.Attach = (token, x, y, parent, mode) => { if (Resolve(token) is { } owner) owner.Attach(token, x, y, parent, mode); else Need(attach)(token, x, y, parent, mode); };
        _tree.EventHooks.Handle = (token, code, first, second) => Resolve(token) is { } owner ? owner.Handle(token, code, first, second) : Need(events)(token, code, first, second);
        _tree.InputHooks.Handle = (token, code, first, second) => Resolve(token) is { } owner ? owner.Handle(token, code, first, second) : Need(input)(token, code, first, second);
        _tree.FrameHooks.Idle = token => { if (Resolve(token) is { } owner) owner.Idle(token); else Need(idle)(token); };
        _tree.LifetimeHooks.DeletingDestructor = (token, flags) => { if (Resolve(token) is { } owner) owner.DeleteScalar(token, flags); else Need(deleted)(token, flags); };
        _tree.FrameHooks.Broadcast = (token, code, first, second) => { if (Resolve(token) is { } owner) { if (owner.Node(token) is HudDividerNode) owner._dividerInput.Broadcast(token, code, first, second); } else Need(broadcast)(token, code, first, second); };
        _tree.LifetimeHooks.Release = token => { if (weak.TryGetTarget(out var owner)) { if (token == owner._token) owner._token = 0; if (token == owner._root.Budget) owner._root.Budget = 0; } release(token); };
        _drawing = _tree.ExternalDrawing.RegisterExternalGumps(new() { Text = token => Resolve(token)?.Text(token), Owns = token => Resolve(token) is not null, Draw = (token, target, colors, background) => (Resolve(token) ?? throw new InvalidOperationException("생산 출력 owner 반납됨")).Draw(token, target, colors, background) });
    }
    /// <summary>같은 실제 안내 문자열→Text 또는 기본 scalar 순서다.</summary>
    private void DeleteScalar(uint token, uint flags) { if (Node(token) is ProductionHintNode) _hint.DeleteScalar(token, flags); else if (Text(token) is not null) _text.Single.DeleteScalar(token, flags); else _tree.Lifetime.DeleteScalar(token, flags); }
    /// <summary>실제 Native GumpForm의 부모 소실을 격자 가상+4 또는 기본 Kill에 분배한다.</summary>
    public void ParentGone(uint token, uint parent) { if (Node(token) is InventoryGridNode) _grid.ParentGone(token, parent); else _tree.Lifetime.Kill(token); }
    /// <summary>부분 생성/미부착/안내 형제를 포함한 자기 노드만 실제 공통 수명으로 반납한다.</summary>
    public void Close()
    {
        // 안내는 Root의 형제이므로 원문처럼 먼저 Kill/정리한다.
        foreach (uint token in _tree.OwnedNodes(this)) if (Node(token) is ProductionHintNode hint && (hint.Flags & 0x10000) == 0) _tree.Lifetime.Kill(token); _tree.Lifetime.Cleanup();
        // 현재 등록 순서에서 다른 UI와 이미 죽은 자식을 건드리지 않는다.
        for (int index = 1; index < _tree.Create.Entries.Length; index++) { uint token = _tree.Create.Entries[index]; if (token != 0 && Node(token) is { } node && (node.Flags & 0x10000) == 0) _tree.Lifetime.Kill(token); } _tree.Lifetime.Cleanup();
        // 등록 전 실패한 물리도 원문 관리 소유자처럼 명시 반납한다.
        foreach (uint token in _tree.OwnedNodes(this)) _tree.ReleaseUnregistered(token); _token = 0; _root.Budget = 0; _maintenance = null;
    }
    /// <summary>자기 노드가 정리된 뒤 현재 외부 그림 등록만 반납한다.</summary>
    public void Dispose() { if (_disposed) return; Close(); _tree.ExternalDrawing.UnregisterExternalGumps(_drawing); _disposed = true; }
}
