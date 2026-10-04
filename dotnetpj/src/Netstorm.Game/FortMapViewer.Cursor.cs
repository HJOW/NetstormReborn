using Microsoft.Xna.Framework;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>TEST02에서 확인한 사제 선택·건물 배치의 커서 전환.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>논리 화면 좌표와 명령 상태로 원본 커서를 고른다. 메뉴·생산 창에서는 화살표다.</summary>
    public GameCursor CursorAt(Point point)
    {
        if (!_playUi || TutorialDialogOpen || KnowledgeOpen || ContextMenuOpen || _missionMenuVisible || _leaveMissionPrompt
            || IsPlayUiPoint(point.X, point.Y)) return GameCursor.Arrow;
        // 건물 배치는 가능·불가·허공 모두 같은 굵은 십자다. 판정은 발자국 색으로 표시한다.
        if (_placementMode && IsBuilding(_candidates[_candidateIndex])) return GameCursor.Place;
        if (_placementMode || _bridgeMode) return GameCursor.Arrow;
        GameEntity? selected = _session.Entity(_session.Player(TestPlayer).SelectedEntityId);
        if (selected is not { Kind: ObjectKind.Priest, Owner: TestPlayer, IsStunned: false, IsSuspended: false,
            Captivity: PriestCaptivity.Free }) return GameCursor.Arrow;
        GameEntity? target = PickEntityAt(point);
        if (target is { Kind: ObjectKind.Temple, Owner: TestPlayer, IsComplete: true }) return GameCursor.Temple;
        if (target != null) return GameCursor.Forbidden;
        (int x, int y) = CellAt(point.ToVector2());
        return _session.Bridges.IsIsland(x, y) || _session.Bridges.At(x, y) is { Owner: TestPlayer }
            ? GameCursor.Move : GameCursor.Forbidden;
    }
}
