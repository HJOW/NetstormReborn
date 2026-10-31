using Netstorm.Core.Original;

// 원본 튜토리얼 미션의 가상 단계와 실제 수명 효과를 연결한다.
namespace Netstorm.Client.Mission;

/// <summary>튜토리얼 가상 단계가 사용하는 실제 월드·안내·정지 효과다.</summary>
public sealed class TutorialMissionStageHooks
{
    /// <summary>설정과 월드를 설치한다.</summary>
    public Action? Initialize { get; init; }
    /// <summary>전역 init Tell을 호출한다.</summary>
    public Action<string, uint>? TellGlobal { get; init; }
    /// <summary>실제 미션 시작을 수행한다.</summary>
    public Action? Start { get; init; }
    /// <summary>공통 프레임을 수행한다.</summary>
    public Action? Frame { get; init; }
    /// <summary>현재 단계 안내를 다시 표시한다.</summary>
    public Action? ShowStage { get; init; }
    /// <summary>현재 미션의 성공·실패 안내를 호출하고 표시 결과 DWORD를 반환한다.</summary>
    public Func<string, uint>? TellMission { get; init; }
    /// <summary>일시 팝업을 닫는다.</summary>
    public Action? CloseTransient { get; init; }
    /// <summary>현재 미션의 게임 시계 정지를 요청한다.</summary>
    public Action? Pause { get; init; }
}

/// <summary>원본 튜토리얼 미션의 가상 단계 및 월드 진입 정지다.
/// 근거: cpppj/src/client/MissionLifecycle.h/.cpp.</summary>
public static class MissionLifecycle
{
    /// <summary>실제 도달한 효과가 없으면 명시적으로 실패한다.</summary>
    /// <param name="effect">호출할 효과.</param>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("미션 단계의 실제 효과 연결 누락");

    /// <summary>원본 가상 단계 0~6을 같은 미션 상태와 실제 효과에 분배한다.</summary>
    /// <param name="state">현재 미션 상태.</param><param name="sequence">가상 단계 번호.</param><param name="hooks">실제 단계 효과.</param>
    public static void RunTutorialStage(MissionStartState state, uint sequence, TutorialMissionStageHooks hooks)
    {
        switch (sequence)
        {
            case 0: MissionGuideInput.MissionFirst(state, hooks.ShowStage); return;
            case 1: Need(hooks.Initialize)(); return;
            case 2: Need(hooks.TellGlobal)("init", 1); state.Flags |= 1u; return;
            case 3: Need(hooks.Start)(); return;
            case 4: Need(hooks.Frame)(); return;
            case 5:
                uint shown = Need(hooks.TellMission)((state.Flags & 4u) != 0 ? "Success" : "Failure");
                if (shown != 0) { var close = Need(hooks.CloseTransient); var pause = Need(hooks.Pause); close(); pause(); }
                state.Flags |= 1u; return;
            case 6: return;
            default: throw new InvalidOperationException("Guide 가상 단계 0~6 범위 위반");
        }
    }

    /// <summary>월드 진입에서 현재 미션이 있을 때만 가상 Pause를 호출한다.</summary>
    /// <param name="current">현재 미션 유무 조회.</param><param name="pause">현재 미션의 실제 Pause.</param>
    public static void PauseWorldEntry(Func<bool>? current, Action? pause)
    {
        if (Need(current)()) Need(pause)();
    }
}
