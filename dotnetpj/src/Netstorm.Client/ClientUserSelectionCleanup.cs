using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 첫 메뉴의 빈 경로도 실제 Clear/Highlight를 실행하고 객체 효과에서만 현재 Runtime을 요구한다.
namespace Netstorm.Client;

/// <summary>같은 Gameplay 선택 owner와 장기 Surface의 실제 표시 실행기를 조립한다.</summary>
public static class ClientUserSelectionCleanup
{
    /// <summary>객체 통지에 도달한 시점의 실제 Session과 같은 Pool만 허용한다.</summary>
    private static void RequireCurrentPool(ClientGameplayGlobals game, RawSurfaceStorage storage)
    {
        if (!ReferenceEquals(game.RequireSession().Pool, storage.Pool)) throw new InvalidOperationException("선택정리의현재Session/Pool불일치");
    }
    /// <summary>count0/음수는 객체 조회 없이 정리하며 같은 mode0이면 커서 효과도 요구하지 않는다.</summary>
    public static void Clear(ClientGameplayGlobals game, RawSurfaceStorage storage, Action<uint>? selectCursor = null)
    {
        ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(storage);
        var highlight = new RawUserHighlight(game.Selection.HighlightState(), new() { SelectCursor = selectCursor });
        var selection = new RawUserSelection(storage.Pool, storage.Types, game.Selection.SelectionState(), new()
        {
            Enabled = () => game.Selection.Enabled.Value, Highlight = highlight.Highlight,
            SelectNotice = (sid, on) =>
            {
                RequireCurrentPool(game, storage);
                new RawSquidSelection(storage.Pool, current => storage.DisplayExecutor().Update(storage.Pool.Slot(current))).Mark(sid, on);
            },
            Refresh = sid => { RequireCurrentPool(game, storage); storage.DisplayExecutor().Update(storage.Pool.Slot(sid)); },
        });
        selection.Clear();
    }
}
