namespace Netstorm.Core.Simulation;

/// <summary>
/// 미션 초기 브리핑의 10프레임 잠금과 스크립트 정지 해제의 7프레임 연속 판정을 관리한다.
/// 화면 Update마다 한 번 호출하며 초/규칙 틱으로 프레임 수를 환산하지 않는다.
/// </summary>
/// <remarks>
/// 원본 10.78의 004c2990(+84=10), 004c34c0(+84/+b8/+c8)을 따른다.
/// 후속 튜토리얼 단계의 +78/+80 지연과 전체 미션 프레임 함수는 별도 이행 대상이다.
/// </remarks>
public sealed class MissionDialogFrameGate
{
    /// <summary>다이얼로그 없는 연속 화면 프레임이 이 수에 도달하면 초기 A. 브리핑을 요청한다.</summary>
    private const int BriefingLockFrames = 10;

    /// <summary>안내/다이얼로그/팝업이 모두 없는 연속 화면 프레임이 이 수에 도달하면 자기 정지를 해제한다.</summary>
    private const int ResumeLockFrames = 7;

    /// <summary>초기 브리핑까지 남은 연속 프레임 수. 다이얼로그가 생기면 10으로 되돌아간다.</summary>
    public int RemainingBriefingFrames { get; private set; } = BriefingLockFrames;

    /// <summary>마지막 차단 창 이후 재개 조건을 만족한 연속 프레임 수.</summary>
    public int ResumeQuietFrames { get; private set; }

    /// <summary>이 미션 안내가 게임 시계 정지를 소유하는지. 다른 소유자의 수동 정지와 독립적이다.</summary>
    public bool OwnsPause { get; private set; }

    /// <summary>초기 브리핑 요청을 아직 보내지 않았는지. 섹션이 없거나 설정 전용이어도 요청은 한 번이다.</summary>
    public bool BriefingPending => RemainingBriefingFrames != 0;

    /// <summary>
    /// 초기 잠금만 화면 프레임 하나 진행한다. 다이얼로그가 있으면 카운터를 초기화하고,
    /// 없으면 1씩 줄인다. 0에 도달한 이번 호출만 브리핑 요청을 반환한다.
    /// </summary>
    /// <param name="dialogOpen">원본 현재 다이얼로그 포인터에 대응하는 차단 창의 존재 여부.</param>
    public bool AdvanceBriefing(bool dialogOpen)
    {
        if (!BriefingPending) return false;
        if (dialogOpen)
        {
            RemainingBriefingFrames = BriefingLockFrames;
            return false;
        }
        RemainingBriefingFrames--;
        return !BriefingPending;
    }

    /// <summary>
    /// 실제로 열린 스크립트 안내의 정지 소유권을 잡는다. 반복 안내는 중첩 Pause를 추가하지 않는다.
    /// 연속 카운터 초기화는 원본처럼 화면 프레임의 실제 차단 조건 판정에서 수행한다.
    /// </summary>
    public void CapturePause() => OwnsPause = true;

    /// <summary>
    /// 초기 잠금이 끝나고 자기 정지가 있을 때 재개 조건을 화면 프레임 하나 판정한다.
    /// 세 조건 중 하나라도 막히면 연속 수를 0으로 돌리고, 7번째 빈 프레임에서 자기 소유권만 해제한다.
    /// </summary>
    /// <param name="guideOpen">원본 안내창 개수(+005cab2c)가 양수인지에 대응한다.</param>
    /// <param name="dialogOpen">원본 현재 다이얼로그 포인터(+00565dd4)가 비영인지에 대응한다.</param>
    /// <param name="popupOpen">원본 루트 직계 자식 중 flags 0x4000이 있는지에 대응한다.</param>
    public void AdvanceResume(bool guideOpen, bool dialogOpen, bool popupOpen)
    {
        if (!OwnsPause || BriefingPending) return;
        if (guideOpen || dialogOpen || popupOpen)
        {
            ResumeQuietFrames = 0;
            return;
        }
        ResumeQuietFrames++;
        if (ResumeQuietFrames < ResumeLockFrames) return;
        OwnsPause = false;
        ResumeQuietFrames = 0;
    }
}
