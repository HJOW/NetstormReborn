namespace Netstorm.Core.Original;

/// <summary>
/// 사제 postPop·preDestroy 가 함께 읽고 갱신하는 상태(원본 PriestPostPopState).
/// 이번 조각은 사전 파괴가 실제로 읽는 사제 목록만 옮겼고, 소유자 상태 배열(원본 00595428)
/// 은 사제 postPop 몸체를 옮길 때 같은 클래스에 더한다.
/// </summary>
public sealed class PriestPostPopState
{
    /// <summary>중복 없이 등록하는 사제 목록(원본 005954c4 / CD 00549188). 사전 파괴는 같은 목록을 앞방향으로 압축한다.</summary>
    public RawPostPopList Priests { get; } = new();
}

/// <summary>
/// 사제 가상 사전 파괴(원본 004919b0)가 바깥에 요구하는 경계 묶음(원본 PriestPreDestroyHooks).
/// 보호막 조회·보호막 삭제 요청·Carrier 사전 검사·Carrier 전체 준비 네 갈래다.
/// </summary>
public sealed class PriestPreDestroyHooks
{
    /// <summary>같은 소유자·위치의 보호막 조회(원본 00493210 계열). 반환 번호가 1~풀 크기-1 이면 삭제를 요청한다.</summary>
    public Func<int, uint>? FindForcefield { get; set; }

    /// <summary>조회한 보호막의 가상 삭제 요청(플래그 0). 원본은 타입·free·dead 를 다시 검사하지 않는다.</summary>
    public Action<int, uint>? DestroyForcefield { get; set; }

    /// <summary>Carrier 사전 검사. raw·목록을 바꾸지 않고 미연결 하위 효과만 거부한다.</summary>
    public Action<int, uint>? ValidateCarrier { get; set; }

    /// <summary>Carrier→Damageable 전체 삭제 준비(원본 0044b4b0 계열). 목록·보호막 효과 뒤에 부른다.</summary>
    public Action<int, uint>? CarrierPre { get; set; }
}

/// <summary>
/// 사제의 가상 사전 파괴(원본 004919b0 / CD 0040c330, RawPriestPreDestroy, 10.78).
/// 순서는 원본 그대로 "raw·가상 표 확인 → (일반) 목록 개수 검사 → Carrier 사전 검사 →
/// (일반) 사제 목록 압축·보호막 조회와 삭제 요청 → Carrier 전체 준비"다.
/// 원본은 공통 삭제가 dead 를 켠 뒤 진입하므로 dead 를 이유로 생략하지 않는다. 깊이 감소는 연결한 Carrier 의 책임이며
/// 이 몸체는 일반 사제 Pop·공간 해제를 하지 않는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 가상 표 값과 추가 비트 위치가 달라 옮기지 않았다).
/// 근거: cpppj/src/o/RawPriestPreDestroy.cpp (W03 다리 밖 자산 삭제 재정의 조각, 2026-10-23).
/// </summary>
public sealed class RawPriestPreDestroy
{
    /// <summary>10.78 실제 사제 클래스의 가상 표 값(원본 kPatchPriestVtable).</summary>
    private const uint PatchPriestVtable = 0x0050F210;

    /// <summary>실제 사제 타입 번호(158).</summary>
    private const byte PriestType = 158;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>상태 바이트에서 거부할 비트(원본 kInvalidState = 9).</summary>
    private const byte InvalidState = 9;

    /// <summary>슬롯 안 추가 비트(abstract 1·매립 8) 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>일반 삭제를 건너뛰는 추가 비트 마스크(원본 kSkipPrefix = 9).</summary>
    private const byte SkipPrefix = 9;

    /// <summary>사제 목록·보호막을 공유하는 슬롯 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>사제 목록을 가진 상태. 콜백이 같은 목록을 바꿀 수 있어 매번 현재 값을 읽는다.</summary>
    private readonly PriestPostPopState _state;

    /// <summary>보호막·Carrier 의 실제 소비자.</summary>
    private readonly PriestPreDestroyHooks _hooks;

    /// <summary>
    /// 사제 사전 파괴를 만든다. 네 경계가 모두 있어야 하며, 누락하면 첫 삭제 전에 거부한다(부분 복원에 따른 효과 누락 방지).
    /// </summary>
    /// <param name="pool">사제 슬롯을 가진 풀.</param>
    /// <param name="state">사제 목록 상태. 이 객체보다 오래 살아야 한다.</param>
    /// <param name="hooks">보호막·Carrier 경계.</param>
    public RawPriestPreDestroy(RawSidPool pool, PriestPostPopState state, PriestPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        // 네 외부 경계를 모두 요구한다
        if (hooks.FindForcefield is null || hooks.DestroyForcefield is null
            || hooks.ValidateCarrier is null || hooks.CarrierPre is null)
        {
            throw new ArgumentException("사제 삭제 준비 경계 미연결", nameof(hooks));
        }
        _pool = pool;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. 연결 어댑터가 참조 대상을 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 예약·풀 밖 번호와 다른 파생 표는 사제 경로로 분배하지 않는다(원본의 진입 조건).
    /// </summary>
    /// <param name="sid">확인할 번호.</param>
    public bool Handles(int sid)
    {
        // 5 미만(목록 밖)과 풀 범위 밖은 사제 슬롯이 아니다
        if (sid < RawSidPool.FirstOrdinary || sid >= _pool.Capacity)
        {
            return false;
        }
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        // 실제 사제 타입이면서 10.78 사제 가상 표일 때만 이 몸체가 처리한다
        return raw[TypeOffset] == PriestType && UInt(raw, 0) == PatchPriestVtable;
    }

    /// <summary>
    /// 원본 004919b0 전체다. dead·void 여도 실행하며 추가 비트 9 가 켜져 있으면 Carrier 만 부른다.
    /// 일반 경로는 목록 개수를 먼저 검사하고 Carrier 사전 검사 뒤에 목록 압축·보호막 삭제를 한다.
    /// </summary>
    /// <param name="sid">삭제되는 사제 번호.</param>
    /// <param name="flags">삭제 플래그. Carrier 사전 검사·전체 준비에 그대로 쓴다.</param>
    public void PreDestroy(int sid, uint flags)
    {
        // 사제가 아니거나 상태에 abstract·free 비트가 있으면 쓰기 전에 거부한다
        if (!Handles(sid) || (_pool.Slot(sid)[StateOffset] & InvalidState) != 0)
        {
            throw new InvalidOperationException("사제 삭제 준비의 raw 상태 오류");
        }
        // 추가 비트가 0 이면 일반 사제 경로다
        bool ordinary = (_pool.Slot(sid)[ExtraOffset] & SkipPrefix) == 0;
        // 일반 경로는 목록을 쓰기 전에 활성 개수를 검사한다
        if (ordinary)
        {
            _ = _state.Priests.Items();
        }
        // Carrier 사전 검사는 목록·보호막보다 먼저이며 일반·추상 모두 부른다
        _hooks.ValidateCarrier!(sid, flags);
        // 일반 경로만 목록 압축과 보호막 삭제를 한다
        if (ordinary)
        {
            Remove(sid);
            uint forcefield = _hooks.FindForcefield!(sid);
            // 원본은 SID 1~4 도 주소 범위 안으로 인정한다. 타입·free·dead 를 여기서 더 검사하지 않는다
            if (forcefield >= 1 && forcefield < (uint)_pool.Capacity)
            {
                // 원본 SID 는 16비트라 int 범위 안이다
                _hooks.DestroyForcefield!((int)forcefield, 0);
            }
        }
        // Carrier 전체 준비는 일반·추상 모두 부른다
        _hooks.CarrierPre!(sid, flags);
    }

    /// <summary>
    /// 원본 removeAll 과 같은 앞방향 압축이다. 일치 항목을 모두 빼고 남은 활성 항목만 당기며,
    /// 비활성 꼬리(개수 밖 저장소)는 지우지 않는다.
    /// </summary>
    /// <param name="sid">뺄 사제 번호.</param>
    private void Remove(int sid)
    {
        RawPostPopList list = _state.Priests;
        // 손상된 개수는 목록을 쓰기 전에 거부한다
        _ = list.Items().Length;
        uint removed = 0;
        // 생존 항목은 최초 상대 순서를 유지한다
        for (uint index = 0; index < list.Count; index++)
        {
            uint current = list.Entries[(int)index];
            // 같은 번호는 세고, 다르면 앞으로 당긴다
            if (current == (uint)sid)
            {
                removed++;
            }
            else
            {
                list.Entries[(int)(index - removed)] = current;
            }
        }
        list.Count -= removed;
    }

    /// <summary>
    /// 기존 공통 삭제 효과 위에 사제 가상 사전 파괴만 얹는 훅 묶음을 만든다(원본 MakePriestPreDestroyHooks).
    /// 다른 풀의 사제 몸체를 섞을 수 없도록 풀 참조를 확인하고, 후속 경계가 없으면 첫 삭제 전에 거부한다.
    /// </summary>
    /// <param name="pool">분배기와 사제 몸체가 공유하는 풀.</param>
    /// <param name="fallback">사제가 아닌 자산의 기존 공통 효과 묶음.</param>
    public SquidDestroyHooks WrapHooks(RawSidPool pool, SquidDestroyHooks fallback)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(fallback);
        // 다른 월드의 목록·슬롯을 섞지 않는다
        if (!ReferenceEquals(pool, _pool))
        {
            throw new ArgumentException("사제 삭제 분배의 SID 풀 불일치", nameof(pool));
        }
        // 후속 효과가 없으면 사제 사전 파괴만 하고 나머지를 잃는다
        if (fallback.Emit is null)
        {
            throw new ArgumentException("사제 삭제 분배의 후속 효과 경계 누락", nameof(fallback));
        }
        Action<SquidDestroyEvent> emit = fallback.Emit;
        return new SquidDestroyHooks
        {
            // 선택·Unpop 경계는 기존 묶음 그대로 쓴다
            Selected = fallback.Selected,
            UnpopForm = fallback.UnpopForm,
            Emit = effect =>
            {
                // 사제의 사전 파괴 사건만 이 몸체가 가로챈다
                if (effect.Effect == SquidDestroyEffect.PreDestroy && Handles(effect.Sid))
                {
                    PreDestroy(effect.Sid, effect.Flags);
                }
                else
                {
                    emit(effect);
                }
            },
        };
    }

    /// <summary>슬롯의 4바이트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint UInt(ReadOnlySpan<byte> raw, int offset)
    {
        // 호스트 바이트 순서에 기대지 않고 낮은 바이트부터 합친다
        return (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
    }
}
