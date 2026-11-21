// 실제 도착의 현재 SID 진단과 삭제 뒤 외부 효과 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>1be80의 실제 Form/부모/물리 풀 상한과 진단이다.</summary>
public sealed class PathBaseRobustHooks
{
    /// <summary>현재 base+4의 DWORD다.</summary>
    public Func<uint>? Form { get; set; }
    /// <summary>현재 base+8의 DWORD다.</summary>
    public Func<uint>? Parent { get; set; }
    /// <summary>같은 물리 풀의 현재 상한이다.</summary>
    public Func<uint>? Capacity { get; set; }
    /// <summary>정상 복귀 뒤 다음 현재 조회를 계속하는 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>48b340이 원문 위치에서 선택하는 실제 도착 장치다.</summary>
public sealed class PathArrivalParentHooks
{
    /// <summary>가상 호출 전에 저장할 현재 부모다.</summary>
    public Func<int>? Parent { get; set; }
    /// <summary>최초 부모의 범위/dead/free 판정이다.</summary>
    public Func<int, bool>? ValidAlive { get; set; }
    /// <summary>같은 경로의 실제 최종 Step 좌표다.</summary>
    public Func<RawPathPoint>? Step { get; set; }
    /// <summary>저장 부모의 실제 가상 Unpop0이다.</summary>
    public Action<int>? Unpop { get; set; }
    /// <summary>저장 부모의 실제 가상 Pop0이다.</summary>
    public Action<int, RawPathPoint>? Pop { get; set; }
    /// <summary>객체 삭제 뒤에도 유효한 외부 수명 관찰이다.</summary>
    public Func<bool>? Alive { get; set; }
    /// <summary>항상참인 원문 함수의 현재 SID 진단 부작용이다.</summary>
    public Action? Robust { get; set; }
    /// <summary>현재 spot8/일반 finder의 실제 겹침 판정이다.</summary>
    public Func<RawPathPoint, bool>? Blocked { get; set; }
    /// <summary>같은 경로의 실제 가상 Halt다.</summary>
    public Func<bool>? Halt { get; set; }
    /// <summary>같은 raw finished DWORD의 쓰기다.</summary>
    public Action<bool>? Finished { get; set; }
    /// <summary>Halt 뒤 저장 부모의 현재 두 상태 비트다.</summary>
    public Func<int, bool>? DeadOrFree { get; set; }
    /// <summary>현재 raw deathWatch의 비영 조회다.</summary>
    public Func<bool>? WatchPresent { get; set; }
    /// <summary>같은 raw 비주소 watch 토큰을 켠다.</summary>
    public Action? ArmWatch { get; set; }
    /// <summary>같은 raw watch 토큰을 지운다.</summary>
    public Action? ClearWatch { get; set; }
    /// <summary>현재 두 targets로 호출하는 부모+3c다.</summary>
    public Func<int, uint>? Arrival { get; set; }
    /// <summary>같은 Host의 flags0 제거다.</summary>
    public Action? Kill { get; set; }
    /// <summary>삭제 뒤에도 저장 부모의 현재 위치/TYPE/owner로 겹침을 정리한다.</summary>
    public Action<int>? Clear { get; set; }
    /// <summary>삭제 뒤에도 유효하며 정상 복귀 가능한 원문 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawPathArrivalParent.cpp의 겹침/기본 진단/공통 도착이다.</summary>
public static class RawPathArrivalParent
{
    /// <summary>실제 선택 위치에서만 함수 누락을 거부한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("공통 도착의 선택된 실제 경계 미연결");
    /// <summary>서로 다른 원문 파일/줄 중 실제 Pathprocess 진단을 전달한다.</summary>
    private static void Assert(PathArrivalParentHooks hooks, string expression, uint line) => Need(hooks.Assertion)(expression, @"\Ns\O\Pathprocess.cpp", line);
    /// <summary>현재 spot8 다음 같은 일반 finder를 사용하며 첫 부류 일치에서 끝낸다.</summary>
    public static bool Blocked(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, uint blockingGenus, RawPathPoint point)
    {
        int x = RawPathSlot.Cell(point.X, true), y = RawPathSlot.Cell(point.Y, true); if ((spots[y * 256 + x] & 8) == 0) return false;
        var finder = new RawSquidFinder(pool, hash, types); int fx = RawPathSlot.Cell(point.X, false), fy = RawPathSlot.Cell(point.Y, false); int candidate = finder.Begin(new(fx, fy, fx, fy));
        // 원문 현재 후보의 TYPE/부류만 읽고 첫 일치 뒤 다른 후보를 소비하지 않는다.
        while (candidate != 0)
        {
            if ((types[pool.Slot(candidate)[10]].Flags2 & blockingGenus) != 0) return true; candidate = finder.Next();
        }
        return false;
    }
    /// <summary>SID0을 허용하고 진단 정상 복귀 뒤 현재 capacity/부모를 다시 읽으며 항상참을 반환한다.</summary>
    public static bool Robust(PathBaseRobustHooks hooks)
    {
        uint form = Need(hooks.Form)(), capacity = Need(hooks.Capacity)();
        if (form != 0 && form >= capacity) { Need(hooks.Assertion)("formSid == 0 || form.isValid()", @"\Ns\O\Baseprocess.cpp", 0x69); capacity = Need(hooks.Capacity)(); }
        uint parent = Need(hooks.Parent)(); if (parent != 0 && parent >= capacity) Need(hooks.Assertion)("attachToSid == 0 || s.isValid()", @"\Ns\O\Baseprocess.cpp", 0x6c);
        return true;
    }
    /// <summary>원문 각 효과 뒤 실제 수명을 확인하고 삭제 뒤에는 보존한 진단/clear만 사용한다.</summary>
    public static void Run(RawCheckingFlag checking, PathArrivalParentHooks hooks)
    {
        var currentChecking = new RawCheckingFlag(checking); int parent = Need(hooks.Parent)();
        if (!Need(hooks.ValidAlive)(parent)) Assert(hooks, "walker.isValid() && !walker.isDead() && !walker.isFree()", 0x25c);
        if (!Need(hooks.Alive)()) return; RawPathPoint point = Need(hooks.Step)(); if (!Need(hooks.Alive)()) return;
        Need(hooks.Unpop)(parent); if (!Need(hooks.Alive)()) return; Need(hooks.Pop)(parent, point); if (!Need(hooks.Alive)()) return;
        if (currentChecking) Need(hooks.Robust)(); if (!Need(hooks.Alive)()) return;
        Need(hooks.Robust)(); if (!Need(hooks.Alive)()) return;
        bool blocked = Need(hooks.Blocked)(point);
        if (Need(hooks.Halt)()) { if (blocked) Need(hooks.Clear)(parent); return; }
        if (!Need(hooks.Alive)()) { if (blocked) Need(hooks.Clear)(parent); return; }
        Need(hooks.Finished)(true); if (currentChecking) Need(hooks.Robust)(); if (!Need(hooks.Alive)()) return;
        if (Need(hooks.DeadOrFree)(parent)) Assert(hooks, "!walker.isDead() && !walker.isFree()", 0x286);
        if (!Need(hooks.Alive)()) return;
        if (Need(hooks.WatchPresent)()) Assert(hooks, "!deadFlag", 0x289);
        if (!Need(hooks.Alive)()) return;
        Need(hooks.ArmWatch)(); uint result = Need(hooks.Arrival)(parent);
        if (result != 1000 && result != 1001 && result != 1002) Assert(hooks, "returnCode == pathKILL || returnCode == pathALREADY_DEAD || returnCode == pathLINGER", 0x28d);
        if (!Need(hooks.Alive)())
        {
            if (currentChecking && result != 1001) Assert(hooks, "returnCode == pathALREADY_DEAD", 0x290);
            if (blocked) Need(hooks.Clear)(parent); return;
        }
        Need(hooks.ClearWatch)(); if (currentChecking) Need(hooks.Robust)();
        if (Need(hooks.Alive)()) { if (result == 1002) Need(hooks.Finished)(false); else Need(hooks.Kill)(); }
        if (blocked) Need(hooks.Clear)(parent);
    }
}
