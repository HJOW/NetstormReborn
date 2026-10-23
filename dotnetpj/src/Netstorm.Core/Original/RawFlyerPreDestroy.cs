namespace Netstorm.Core.Original;

/// <summary>
/// 실제 플라이어 삭제(원본 0045e7e0)가 바깥에 요구하는 하위 경계 묶음(원본 FlyerPreDestroyHooks).
/// 현재 +8 WORD 대상의 가상 삭제와 같은 개체의 Damageable 사전 파괴 두 갈래다.
/// </summary>
public sealed class FlyerPreDestroyHooks
{
    /// <summary>현재 +8 WORD 가 가리키는 대상의 가상 삭제(원본 +0x10, 플래그 0)다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>대상 삭제 뒤 같은 개체·같은 플래그의 Damageable 사전 파괴(원본 0044b4b0)다.</summary>
    public Action<int, uint>? Damageable { get; set; }
}

/// <summary>
/// 실제 플라이어의 가상 사전 파괴(원본 0045e7e0, RawFlyerPreDestroy, 10.78).
/// 순서는 원본 그대로 "현재 +8 WORD 읽기 → 대상 가상 삭제(0) → 같은 개체·같은 플래그로 Damageable"이다.
/// +8 은 WORD 이므로 DWORD 나 종속 head 로 넓혀 읽지 않는다(넓게 읽으면 타입·상태 바이트가 대상이 된다).
/// 호출 뒤 raw 가 바뀌어도 하위 호출에 쓰는 this·flags 는 원본 그대로 유지한다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 옮기지 않았다).
/// 근거: cpppj/src/o/RawFlyerPreDestroy.cpp (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class RawFlyerPreDestroy
{
    /// <summary>대상 WORD 가 놓인 슬롯 안 위치(+8, 2바이트).</summary>
    private const int TargetOffset = 8;

    /// <summary>원본과 같은 50바이트 슬롯을 읽는 풀. 생성 뒤 바뀌지 않는다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>대상 삭제와 Damageable 사전 파괴의 실제 소비자.</summary>
    private readonly FlyerPreDestroyHooks _hooks;

    /// <summary>
    /// 플라이어 전처리를 만든다. 풀과 두 경계는 이 객체보다 오래 살아야 하며,
    /// 두 경계가 모두 연결되지 않으면 첫 삭제 전에 거부한다.
    /// </summary>
    /// <param name="pool">플라이어 슬롯을 가진 풀.</param>
    /// <param name="hooks">대상 삭제·Damageable 사전 파괴 경계.</param>
    public RawFlyerPreDestroy(RawSidPool pool, FlyerPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hooks);
        // 미복원 하위 경계가 있으면 효과를 내기 전에 거부한다
        if (hooks.Destroy is null || hooks.Damageable is null)
        {
            throw new ArgumentException("플라이어 전처리 실제 하위 경계 누락", nameof(hooks));
        }
        _pool = pool;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 읽는 슬롯 풀. 연결 어댑터가 같은 월드인지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 원본 0045e7e0 전체다. 현재 +8 WORD 가 0 이 아니면 그 대상을 플래그 0 으로 먼저 지우고,
    /// 그 뒤 같은 개체·같은 플래그로 Damageable 사전 파괴를 한 번 부른다.
    /// </summary>
    /// <param name="sid">삭제되는 플라이어 번호.</param>
    /// <param name="flags">삭제 플래그. Damageable 사전 파괴에 그대로 쓴다.</param>
    public void PreDestroy(int sid, uint flags)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        // +8 은 WORD 이므로 두 바이트만 낮은 바이트부터 합친다
        int target = raw[TargetOffset] | (raw[TargetOffset + 1] << 8);
        // 0 번은 대상 없음이며 원본도 삭제하지 않는다
        if (target != 0)
        {
            _hooks.Destroy!(target, 0);
        }
        // callback 이 raw 를 바꿔도 원본 this·flags 로 하위를 한 번 부른다
        _hooks.Damageable!(sid, flags);
    }
}
