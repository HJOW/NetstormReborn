using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 살아 있는 실제 선택 객체의 수명을 세션의 약한 연결로 유지한다.
namespace Netstorm.Client.Host;

/// <summary>살아 있는 실제 선택 객체의 주소다. 약한 참조로만 빌린다.</summary>
/// <remarks>2026-11-25 0.994에서 RU-4c용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 468행의 SelectionLink. UI 몸체가 먼저 파괴돼도 해제된 주소를 호출하지 않는다.</remarks>
public sealed class HostSelectionLink
{
    /// <summary>살아 있는 실제 선택 객체다. 선택이 이 고리를 소유해 수명을 유지한다.</summary>
    public RawUserSelection? Selection { get; set; }
}

/// <summary>세션의 살아 있는 실제 선택 객체 수명이다. 이동·낙하·재확인은 약한 연결로 읽는다.</summary>
public sealed partial class HostSession
{
    /// <summary>살아 있는 실제 선택 객체의 약한 연결이다. 선택이 반납되면 만료된다.</summary>
    private WeakReference<HostSelectionLink>? _activeSelection;

    /// <summary>현재 살아 있는 실제 선택 객체가 있으면 참이다. 해제된 주소는 보유하지 않는다.</summary>
    public bool HasActiveSelection => _activeSelection?.TryGetTarget(out HostSelectionLink? link) == true && link is not null && link.Selection is not null;

    /// <summary>같은 세션 Pool/TYPE/선택 owner로 실제 선택 객체를 만들고 살아 있는 연결로 등록한다.</summary>
    /// <param name="types">같은 세션의 현재 188TYPE 표다. 사본이나 다른 판본으로 바꾸지 않는다.</param>
    /// <param name="state">같은 장기 선택 owner의 상태다. 다른 선택 owner는 거부한다.</param>
    /// <param name="hooks">호출자가 묶은 실제 장치다. 월드 장치는 R6에서 잇는다.</param>
    /// <remarks>2026-11-25 0.994에서 RU-4c용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 6305행의 MakeUserSelection. 새 선택이 살아 있는 연결을 교체하며 이전 선택은 직접 호출만 유지한다.</remarks>
    public RawUserSelection MakeUserSelection(IReadOnlyList<OriginalTypeRecord> types, UserSelectionState state, UserSelectionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(state.Selected, Gameplay.Selection.Selected)
            || !ReferenceEquals(state.MouseType, Gameplay.Selection.MouseType)
            || !ReferenceEquals(state.RememberedPoint, Gameplay.Selection.Remembered)
            || !ReferenceEquals(state.MultipleCount, Gameplay.Selection.MultipleCount))
            throw new ArgumentException("살아 있는 선택의 동일 선택 owner 필요", nameof(state));
        var selection = new RawUserSelection(Pool, types, state, hooks);
        var link = new HostSelectionLink { Selection = selection };
        selection.SessionLifetime = link;
        _activeSelection = new WeakReference<HostSelectionLink>(link);
        return selection;
    }

    /// <summary>살아 있는 실제 선택 객체를 약한 연결로 읽는다. 해제됐으면 명시 실패한다.</summary>
    private HostSelectionLink RequireActiveSelection()
    {
        if (_activeSelection?.TryGetTarget(out HostSelectionLink? link) == true && link is not null && link.Selection is not null) return link;
        throw new InvalidOperationException("현재 선택 수명 미연결");
    }

    /// <summary>살아 있는 실제 선택 객체에 이동 요청의 현재 선택을 수행한다. 해제된 주소는 호출하지 않는다.</summary>
    /// <param name="sid">선택할 대상 SID다.</param>
    /// <param name="forceSelect">강제 선택 DWORD다.</param>
    /// <param name="activateMove">이동 활성화 DWORD다.</param>
    /// <remarks>2026-11-25 0.994에서 RU-4c용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 3942행의 move.select.</remarks>
    public uint SelectViaActiveSelection(uint sid, uint force, uint activate) => RequireActiveSelection().Selection!.Select(sid, force, activate);

    /// <summary>살아 있는 실제 선택 객체가 있을 때만 마우스 재확인을 수행한다. 없으면 조용히 반환한다.</summary>
    /// <param name="find">기억한 좌표/TYPE에서 대상 SID를 찾는 실제 찾기다.</param>
    /// <remarks>2026-11-25 0.994에서 RU-4c용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2849행의 재확인.</remarks>
    public void RecheckActiveSelection(Func<uint, uint, uint, uint> find)
    {
        if (_activeSelection?.TryGetTarget(out HostSelectionLink? link) != true || link is null || link.Selection is null) return;
        link.Selection.RecheckMouse(find);
    }

    /// <summary>살아 있는 실제 선택 객체의 단일 선택을 해제한다. 원문 낙하 선택 해제는 Select(0,0,1)와 같다.</summary>
    /// <remarks>2026-11-25 0.994에서 RU-4c용으로 추가했다. 근거: cpppj RawCampaignSession.cpp 2549행의 clearSelection.</remarks>
    public void ClearActiveSelection() => SelectViaActiveSelection(0, 0, 1);
}
