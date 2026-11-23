// Rune·다이스·제단 정기의 분기 본문을 같은 실제 부모/현재 장치에서 공유한다. 제단(Altar) 분기는 별도 fixture에서 잇는다.
namespace Netstorm.Core.Original;

/// <summary>RawRitualRegular.h의 304에서 선택한 현재 검색/진행/가상 삭제만 요구한다.</summary>
public sealed class RuneRegularHooks
{
    /// <summary>같은 현재 역할과 owner로 반경1을 검색한다.</summary>
    public Func<int, int>? FindDais { get; set; }
    /// <summary>발견한 실제 다이스의 전체 진행 본문이다.</summary>
    public Action<int>? Progress { get; set; }
    /// <summary>Rune의 현재 가상10에 flags48을 전달한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다.</summary>
    internal RuneRegularHooks Copy() => (RuneRegularHooks)MemberwiseClone();
}

/// <summary>RawRitualRegular.h의 305/306/307과 공통 Damageable을 원래 선택 위치에서 호출한다.</summary>
public sealed class DaisHandleHooks
{
    /// <summary>저장한 프레임에 현재 가상 Count/단계 표를 적용한다.</summary>
    public Func<int, int, uint>? Stage { get; set; }
    /// <summary>같은 실제 Frame.Set이다.</summary>
    public Action<int, int, uint>? Frame { get; set; }
    /// <summary>실제44a200 전체 후단이다.</summary>
    public Func<int, float>? SacrificeAdvance { get; set; }
    /// <summary>현재 강한 사제 검색 뒤 실제447e40을 호출한다.</summary>
    public Action<int>? Ritual { get; set; }
    /// <summary>나머지 사건의 같은44b900 본문이다.</summary>
    public RegularHandler? Damageable { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다.</summary>
    internal DaisHandleHooks Copy() => (DaisHandleHooks)MemberwiseClone();
}

/// <summary>RawRitualRegular.cpp의 304/305/306/307 분기다. 제단 분기는 후속 fixture에서 잇는다.</summary>
public static class RawRitualRegular
{
    /// <summary>도달한 장치만 요구하며 호출 중 자신의 연결 교체에도 수명을 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("의식 정기의 선택된 실제 효과 미연결");
    /// <summary>304에서만 검색/진행/삭제를 실행하고 나머지는 원문base 그대로 반환한다. 원문44a3a0이다.</summary>
    public static float RunRuneRegular(RawSidPool pool, int sid, uint @event, float payload, RuneRegularHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (@event != 0x304) return payload;
        int dais = Need(hooks.FindDais)(sid);
        if (dais != 0 && dais < pool.Capacity) Need(hooks.Progress)(dais);
        Need(hooks.Destroy)(sid, 0x48); return -1.0F;
    }
    /// <summary>저장한 frame과 next를 현재 단계 조회 두 번에 원문 순서로 전달한다. 원문44b150이다.</summary>
    public static float RunDaisHandle(RawSidPool pool, int sid, uint @event, uint count, float payload, DaisHandleHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hooks);
        if (@event == 0x305)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            int frame = pool.Edition == OriginalEdition.Patch1078 ? System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
            int next = unchecked(frame + 1);
            uint nextStage = Need(hooks.Stage)(sid, next), stage = Need(hooks.Stage)(sid, frame);
            Need(hooks.Frame)(sid, next, 0); return stage == nextStage ? 0.1F : 0;
        }
        if (@event == 0x306) return Need(hooks.SacrificeAdvance)(sid);
        if (@event == 0x307 && count != 0) { Need(hooks.Ritual)(sid); return 0; }
        return Need(hooks.Damageable)(sid, @event, count, payload);
    }
}
