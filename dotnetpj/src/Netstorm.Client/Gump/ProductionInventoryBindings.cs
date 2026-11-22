namespace Netstorm.Client.Gump; // 실제 생산 owner의 현재 파생을 공통 가상 슬롯에만 연결한다.

/// <summary>Source RawProductionControls의 격자/항목/안내 가상 분배 조립이다. 생성/물리 소유권은 같은 부모에 남긴다.</summary>
public static class ProductionInventoryBindings
{
    /// <summary>선택한 기존 슬롯도 없으면 원문 누락을 드러낸다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("생산 UI 실제 가상 슬롯 미연결");
    /// <summary>동일 owner의 현재 격자/항목만 부착·사건·입력·점멸로 분배하고 다른 파생 슬롯을 보존한다.</summary>
    public static void BindInventory(GumpTreeHooks tree, GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame,
        Func<uint, InventoryGump?> grid, Func<uint, InventoryItemGump?> item, Func<uint, InventoryGump?> blink)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(input); ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(grid); ArgumentNullException.ThrowIfNull(item); ArgumentNullException.ThrowIfNull(blink);
        var attach = tree.Attach; var oldEvent = events.Handle; var oldInput = input.Handle; var idle = frame.Idle;
        tree.Attach = (token, x, y, parent, mode) => { if (grid(token) is { } g) g.AttachGrid(token, x, y, parent, mode); else if (item(token) is { } i) i.Attach(token, x, y, parent, mode); else Need(attach)(token, x, y, parent, mode); };
        events.Handle = (token, code, first, second) => grid(token) is { } g ? g.HandleGrid(token, code, first, second) : item(token) is { } i ? i.Handle(token, code, first, second) : Need(oldEvent)(token, code, first, second);
        input.Handle = (token, code, first, second) => grid(token) is { } g ? g.HandleGrid(token, code, first, second) : item(token) is { } i ? i.Handle(token, code, first, second) : Need(oldInput)(token, code, first, second);
        frame.Idle = token => { if (blink(token) is { } owner) owner.IdleItem(token); else Need(idle)(token); };
    }
    /// <summary>현재 안내만 실제 만료와 소유 문자열→Text scalar로 분배한다. 기본 문자 장치 뒤에 설치한다.</summary>
    public static void BindHint(GumpFrameHooks frame, GumpLifetimeHooks life, Func<uint, ProductionHintGump?> resolve)
    {
        ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(life); ArgumentNullException.ThrowIfNull(resolve); var idle = frame.Idle; var deleting = life.DeletingDestructor;
        frame.Idle = token => { if (resolve(token) is { } hint) hint.Idle(token); else Need(idle)(token); };
        life.DeletingDestructor = (token, flags) => { if (resolve(token) is { } hint) hint.DeleteScalar(token, flags); else Need(deleting)(token, flags); };
    }
}
