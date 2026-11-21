using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 바깥 게임 준비를 호출하지 않고 같은 원문 Cancel/Prepare 두 본체만 조립한다.
namespace Netstorm.Client;

/// <summary>ClientCursorStateCleanup.cpp의 Session 없는 첫메뉴 leaf 연결이다.</summary>
public static class ClientCursorStateCleanup
{
    /// <summary>같은 Pool/TYPE/커서/표면을 사용하며 선택되지 않은 장치는 미리 요구하지 않는다.</summary>
    public static void Cancel(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, CursorDropState drop, ClientCursorGlobals state, CursorLifecycleHooks effects, CursorPlacementHooks placement, ReadOnlyMemory<ushort> surfaceMap = default)
    {
        ArgumentNullException.ThrowIfNull(effects); ArgumentNullException.ThrowIfNull(state);
        var prepare = new RawCursorPlacement(pool, types, surfaceMap, drop, state.Lifecycle, state.Placement, placement); CursorLifecycleHooks selected = effects.Copy(); selected.PrepareCursor = prepare.Prepare;
        new RawCursorLifecycle(pool, types, drop, state.Lifecycle, selected).Cancel();
    }
}
