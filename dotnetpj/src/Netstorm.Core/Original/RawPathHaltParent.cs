// 같은 경로 수명 감시와 현재 진단으로 원문 Halt의 삭제 뒤 복귀를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>489ff0이 선택한 위치에서만 빌리는 현재 부모/가상/수명 장치다.</summary>
public sealed class PathHaltParentHooks
{
    /// <summary>가상 Halt 전에 저장하는 최초 부모다.</summary>
    public Func<int>? Parent { get; set; }
    /// <summary>같은 부모+38의 실제10/11/12 반환이다.</summary>
    public Func<int, uint>? Halt { get; set; }
    /// <summary>경로 소멸 뒤에도 살아 있는 별도 수명 관찰이다.</summary>
    public Func<bool>? Alive { get; set; }
    /// <summary>결과10 뒤 저장 부모의 현재 flags40 비트20이다.</summary>
    public Func<int, bool>? ForcedStop { get; set; }
    /// <summary>현재 권한 또는 Form 로컬 범위다.</summary>
    public Func<bool>? MayKill { get; set; }
    /// <summary>같은 Host의 flags0 제거다.</summary>
    public Action? Kill { get; set; }
    /// <summary>재지정 앞의 현재 권한이다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>저장 부모의 현재TYPE 부류30000이다.</summary>
    public Func<int, bool>? Carrier { get; set; }
    /// <summary>현재 TargetB 다음 TargetA의 비영 조회다.</summary>
    public Func<bool>? HasTargets { get; set; }
    /// <summary>기존 실제 운반 재탐색이며 자신을 삭제할 수 있다.</summary>
    public Action<int>? Retarget { get; set; }
    /// <summary>삭제 뒤에도 유효하며 정상 복귀를 허용하는 원문 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>같은 raw 비주소 watch 토큰을 켜는 선택 경계다.</summary>
    public Action? ArmWatch { get; set; }
    /// <summary>같은 raw watch 토큰을 지우는 선택 경계다.</summary>
    public Action? ClearWatch { get; set; }
}

/// <summary>cpppj/src/o/RawPathHaltParent.cpp의 공통 Halt 부모다.</summary>
public static class RawPathHaltParent
{
    /// <summary>실제 선택된 함수만 요구하고 선행 효과는 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("공통 Halt의 선택된 실제 경계 미연결");
    /// <summary>최초 가상 결과를 유지하고 강제 정지10만11로 바꾼다. checking은 원문 값형 뷰로 빌린다.</summary>
    public static bool Run(RawCheckingFlag checking, PathHaltParentHooks hooks)
    {
        var currentChecking = new RawCheckingFlag(checking); int parent = Need(hooks.Parent)(); hooks.ArmWatch?.Invoke(); uint result = Need(hooks.Halt)(parent);
        if (!Need(hooks.Alive)())
        {
            if (currentChecking && result != 12) Need(hooks.Assertion)("result == haltALREADY_DEAD", @"\Ns\O\Pathprocess.cpp", 0x2d5);
            return true;
        }
        hooks.ClearWatch?.Invoke(); if (result == 10 && Need(hooks.ForcedStop)(parent)) result = 11;
        if (result == 11) { if (Need(hooks.MayKill)()) Need(hooks.Kill)(); return true; }
        if (Need(hooks.Boss)() && Need(hooks.Carrier)(parent) && Need(hooks.HasTargets)())
        {
            hooks.ArmWatch?.Invoke(); Need(hooks.Retarget)(parent); if (!Need(hooks.Alive)()) return true; hooks.ClearWatch?.Invoke();
        }
        return result != 10;
    }
}
