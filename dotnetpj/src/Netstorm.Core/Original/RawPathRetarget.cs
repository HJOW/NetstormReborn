// 운반 대상 사망 뒤 같은 현재 경로/내용물/후보 선택으로 재지정한다.
namespace Netstorm.Core.Original;

/// <summary>실제 내용물 조회·후보 선택·새 이동 장치의 함수 값이다.</summary>
public sealed record PathRetargetHooks
{
    /// <summary>현재 부모/mask20/현재너겟TYPE/순번0의 실제 내용물 조회다.</summary>
    public Func<int, uint, uint, uint, int>? Find { get; init; }
    /// <summary>현재 실제 귀환 후보를 고른다.</summary>
    public Func<int, int>? ChooseReturn { get; init; }
    /// <summary>현재 실제 간헐천 후보를 고른다.</summary>
    public Func<int, int>? ChooseGeyser { get; init; }
    /// <summary>실제 귀환 이동이며 기존 경로를 삭제할 수 있다.</summary>
    public Action<int, int>? GoReturn { get; init; }
    /// <summary>실제 간헐천 이동이며 기존 경로를 삭제할 수 있다.</summary>
    public Action<int, int>? GoGeyser { get; init; }
}

/// <summary>cpppj/src/o/RawPathRetarget.cpp의 실제 PATCH 대상 재확인 본체다.</summary>
public sealed class RawPathRetarget
{
    /// <summary>부모/두 대상의 같은 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>각 Find에서 읽는 같은 현재 너겟TYPE DWORD다.</summary>
    private readonly UIntBox _nuggetType;
    /// <summary>원문 값형으로 복사한 실제 함수다.</summary>
    private readonly PathRetargetHooks _hooks;
    /// <summary>PATCH 판본을 확인하고 같은 owner/함수만 빌린다.</summary>
    public RawPathRetarget(RawSidPool pool, UIntBox nuggetType, PathRetargetHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("경로 대상 재확인의 판본 오류"); Pool = pool; _nuggetType = nuggetType; _hooks = hooks with { };
    }
    /// <summary>선택한 큰 실제 하위만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("경로 대상 재확인의 실제 하위 미연결");
    /// <summary>실제로 조회할 때만 DWORD SID 물리 범위를 확인하고0도 허용한다.</summary>
    private int Number(uint value)
    {
        if (value >= Pool.Capacity || value > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(value), "경로 대상 재확인 물리 SID 범위"); return (ushort)value;
    }
    /// <summary>처음 저장한 대상의 현재 dead2만 읽는다.</summary>
    private bool Dead(uint value) => (Pool.Slot(Number(value))[11] & 2) != 0;
    /// <summary>최초 두 대상/현재 부모를 저장하고 새 이동 호출 뒤에는 Runtime을 다시 읽지 않는다.</summary>
    public void Update(RawPathRuntime runtime, int initialParent)
    {
        if (runtime.Bytes.Length != RawPathRuntime.LayoutFor(OriginalEdition.Patch1078).Size) throw new ArgumentException("경로 대상 재확인 Runtime 판본");
        uint savedReturn = runtime.TargetB, savedGeyser = runtime.TargetA, currentParent = (uint)runtime.ReadBits(8);
        if (runtime.TargetB != 0 && Dead(savedReturn) && Need(_hooks.Find)(Number(currentParent), 0x20, _nuggetType.Value, 0) != 0)
        {
            int next = Need(_hooks.ChooseReturn)(Number(currentParent)); runtime.SetTargets(runtime.TargetA, (uint)next); if (next != 0) Need(_hooks.GoReturn)(Number(currentParent), next); return;
        }
        if (runtime.TargetA != 0 && Dead(savedGeyser) && Need(_hooks.Find)(initialParent, 0x20, _nuggetType.Value, 0) == 0)
        {
            int next = Need(_hooks.ChooseGeyser)(Number(currentParent)); runtime.SetTargets((uint)next, runtime.TargetB);
            if (next != 0) { runtime.SetTargets(runtime.TargetA, 0); Need(_hooks.GoGeyser)(Number(currentParent), next); }
        }
    }
}
