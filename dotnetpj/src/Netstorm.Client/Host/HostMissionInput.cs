using Netstorm.Client.Input;
using Netstorm.Client.Mission;

namespace Netstorm.Client.Host;

/// <summary>같은 입력 큐의 Guide 우선 처리와 나머지 UI 입력을 실제 호스트 소비자로 연결한다.
/// 근거: cpppj/src/client/UberGump.cpp의 Event 및 Input 큐 처리.</summary>
public sealed class HostMissionInput
{
    /// <summary>현재 Guide 미션 명령 소유자.</summary>
    private readonly GuideMissionOwner _guide;
    /// <summary>Guide가 소비하지 않은 현재 입력을 처리하고 즉시 종료 여부를 반환하는 UI.</summary>
    private readonly Func<InputEvent, bool> _remaining;
    /// <summary>한 사건 처리 뒤 현재 Tell 표시를 동기화한다.</summary>
    private readonly Action _syncTell;

    /// <summary>Guide와 실제 UI·Tell 동기화 효과를 묶는다.</summary>
    /// <param name="guide">같은 Client의 Guide.</param><param name="remaining">남은 사건의 실제 UI 처리.</param>
    /// <param name="syncTell">종료 요청이 없는 사건 뒤 실제 Tell 표시 동기화.</param>
    public HostMissionInput(GuideMissionOwner guide, Func<InputEvent, bool> remaining, Action syncTell)
    {
        _guide = guide ?? throw new ArgumentNullException(nameof(guide));
        _remaining = remaining ?? throw new ArgumentNullException(nameof(remaining));
        _syncTell = syncTell ?? throw new ArgumentNullException(nameof(syncTell));
    }

    /// <summary>Guide 명령을 먼저 분배하고 미소비 사건을 UI에 넘긴다. 종료 요청은 Tell 동기화보다 우선한다.</summary>
    /// <param name="input">실제 입력 큐에서 꺼낸 사건.</param>
    public bool Dispatch(InputEvent input)
    {
        if (!_guide.Dispatch(input) && _remaining(input)) return true;
        _syncTell();
        return false;
    }
}
