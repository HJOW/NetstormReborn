// 원본 미션 Guide의 현재 객체와 단계 반복을 제공한다.
namespace Netstorm.Core.Original;

/// <summary>Guide가 호출 시점마다 조회하는 실제 미션·창·상태와 가상 단계 효과다.</summary>
public sealed class MissionGuideHooks
{
    /// <summary>현재 미션 상태를 조회한다. 없으면 null이다.</summary>
    public Func<MissionStartState?>? Current { get; init; }
    /// <summary>현재 대기 중인 State 값을 조회한다.</summary>
    public Func<uint>? Pending { get; init; }
    /// <summary>현재 대화창이 모두 닫혔는지 조회한다.</summary>
    public Func<bool>? DialogsClear { get; init; }
    /// <summary>선택된 현재 객체의 가상 단계를 실행한다.</summary>
    public Action<MissionStartState, uint>? Stage { get; init; }
    /// <summary>막힌 현재 객체의 안내를 재지연한다.</summary>
    public Action<MissionStartState>? Postpone { get; init; }
    /// <summary>실제 현재 미션을 반납한다.</summary>
    public Action? Release { get; init; }
    /// <summary>범위 밖 단계의 원본 진단을 실행한다.</summary>
    public Action? InvalidStage { get; init; }
}

/// <summary>원본 Guide의 현재 객체 조회·대기 관문·단계 반복을 수행한다.
/// 근거: cpppj/src/o/RawMissionGuide.h/.cpp, 원본 00463bb0·00482c80.</summary>
public sealed class RawMissionGuide
{
    /// <summary>실제 미션과 상태의 조회 및 효과다.</summary>
    private readonly MissionGuideHooks _hooks;

    /// <summary>10.78의 모든 현재 조회와 가상 단계 경계를 연결한다.</summary>
    /// <param name="edition">기준 판본.</param><param name="hooks">현재 객체와 실제 효과.</param>
    public RawMissionGuide(OriginalEdition edition, MissionGuideHooks hooks)
    {
        if (edition != OriginalEdition.Patch1078 || hooks.Current == null || hooks.Pending == null || hooks.DialogsClear == null ||
            hooks.Stage == null || hooks.Postpone == null || hooks.Release == null || hooks.InvalidStage == null)
            throw new ArgumentException("Guide의 현재 미션·상태·단계 경계가 누락됐다");
        _hooks = hooks;
    }

    /// <summary>원본이 역참조하는 위치에서 현재 미션이 없으면 명시적으로 실패한다.</summary>
    private MissionStartState RequireCurrent() => _hooks.Current!() ?? throw new InvalidOperationException("Guide 가상 단계의 현재 미션이 없다");

    /// <summary>현재 미션의 단계 DWORD를 증가하고 완료 요청 비트만 지운다.</summary>
    /// <param name="state">실제 현재 미션 상태.</param>
    public static void Advance(MissionStartState state) { state.Sequence = unchecked(state.Sequence + 1); state.Flags &= ~1u; }

    /// <summary>취소·대기·창 관문과 가상 단계를 처리하고 단계가 바뀌면 같은 호출에서 다시 방문한다.</summary>
    public void Run()
    {
        // 원본 do 루프는 현재 단계가 바뀐 경우에만 다음 가상 단계로 이어진다.
        while (true)
        {
            MissionStartState? current = _hooks.Current!();
            if (current == null) return;
            uint before = current.Sequence;
            if ((current.Flags & 8u) != 0) { if (_hooks.Current!() != null) _hooks.Release!(); return; }
            if (_hooks.Pending!() == 0 && _hooks.DialogsClear!())
            {
                MissionStartState selected = RequireCurrent();
                if ((selected.Flags & 1u) != 0) Advance(selected);
                uint sequence = RequireCurrent().Sequence;
                if (sequence <= 6) { _hooks.Stage!(RequireCurrent(), sequence); if (sequence == 6) _hooks.Release!(); }
                else _hooks.InvalidStage!();
            }
            else _hooks.Postpone!(RequireCurrent());
            current = _hooks.Current!();
            if (current == null || current.Sequence == 6 || current.Sequence == before) return;
        }
    }
}
