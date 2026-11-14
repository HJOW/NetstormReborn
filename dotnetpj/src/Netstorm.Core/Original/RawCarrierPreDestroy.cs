namespace Netstorm.Core.Original;

/// <summary>
/// 캐리어(Carrier) 삭제 준비가 바깥에 요구하는 콜백 묶음(원본 CarrierPreDestroyHooks).
/// </summary>
public sealed class CarrierPreDestroyHooks
{
    /// <summary>효과 전 하위 경계의 순수 사전 검사.</summary>
    public Action<int, uint>? ValidateDamageable { get; set; }

    /// <summary>피해가능객체 사전 파괴(원본 0044b4b0)의 하위 효과 경계.</summary>
    public Action<int, uint>? DamageablePre { get; set; }

    /// <summary>종속을 찾은 뒤의 인자 없는 전역 후처리 경계(원본 00485e40).</summary>
    public Action? DependentFound { get; set; }
}

/// <summary>
/// 캐리어(Carrier) 삭제 준비와 내용물 finder 의 타입·WORD 조건 조회를 복원한다(원본 RawCarrierPreDestroy, 00426890).
/// 순서는 원본 그대로 "asset/타입 검사 → Damageable 사전 검사 → Damageable 사전 파괴 →
/// 권한이 있으면 종속 조회(1,0,0) → 전역 후처리"다.
/// Damageable 에 넘긴 플래그는 종속 조회 인자에 영향을 주지 않는다.
/// 판본별 타입 상한과 PATCH Native의선행검사 생략·현재boss 조회를 구별한다.
/// 근거: cpppj/src/o/RawCarrierPreDestroy.cpp (W03 삭제·수명 다섯째 조각, 2026-10-21).
/// </summary>
public sealed class RawCarrierPreDestroy
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 종속 kind WORD 의 위치(10.78).</summary>
    private const int KindOffset = 18;

    /// <summary>슬롯 안 종속 mask WORD 의 위치(10.78).</summary>
    private const int MaskOffset = 20;

    /// <summary>상태 바이트의 free·contained 마스크. 직접 자산 호출에서는 둘 다 거부한다.</summary>
    private const byte FreeContainedMask = 9;

    /// <summary>객체 슬롯을 읽고 쓰는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>권한·타입 전역을 읽는 상태. Damageable 과 같은 전역을 쓴다.</summary>
    private readonly ContainedFinderState _state;

    /// <summary>외부 효과 콜백 묶음.</summary>
    private readonly CarrierPreDestroyHooks _hooks;

    /// <summary>실제PATCH부모만 선행typed검사를 생략하며 독립/CD경로는 기존검사를 유지한다.</summary>
    private bool _nativeBody;

    /// <summary>
    /// 캐리어 삭제 준비를 만든다. 세 하위 효과가 모두 있어야 직접 부를 수 있다.
    /// </summary>
    /// <param name="pool">삭제 대상 슬롯 풀.</param>
    /// <param name="state">권한·타입 전역. 피해가능객체와 같은 것을 쓴다.</param>
    /// <param name="hooks">외부 효과 콜백 묶음.</param>
    public RawCarrierPreDestroy(RawSidPool pool, ContainedFinderState state, CarrierPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.ValidateDamageable is null || hooks.DamageablePre is null || hooks.DependentFound is null)
        {
            throw new ArgumentException("캐리어 사전 파괴 하위 효과 누락", nameof(hooks));
        }
        _pool = pool;
        _state = state;
        _hooks = new() { ValidateDamageable = hooks.ValidateDamageable, DamageablePre = hooks.DamageablePre, DependentFound = hooks.DependentFound };
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. 연결 어댑터가 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>같은 현재 내용물TYPE/권한/checking 장기 상태를 복사 없이 제공한다.</summary>
    public ContainedFinderState FinderState => _state;

    /// <summary>실제PATCH의직접Damageable부모를 선택하며 게임 상태/난수/내용물은 읽거나 바꾸지 않는다.</summary>
    public void EnableNativeBody()
    {
        if (_pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Native Carrier 전처리 판본 오류");
        _nativeBody = true;
    }

    /// <summary>
    /// 사제 목록 변경 전에 하위 몸체와 raw 자산의 지원 상태를 검사한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void Validate(int sid, uint flags)
    {
        if (_nativeBody) return;
        // 빈 슬롯·할당되지 않은 번호는 여기에서 거부된다.
        Span<byte> raw = _pool.AllocatedBytes(sid);
        if ((raw[StateOffset] & FreeContainedMask) != 0 || raw[TypeOffset] < OriginalConstructors.FirstAssetType
            || raw[TypeOffset] >= (_pool.Edition == OriginalEdition.Patch1078 ? 188 : 171))
        {
            throw new InvalidOperationException("캐리어 사전 파괴 raw 자산 상태/타입 오류");
        }
        _hooks.ValidateDamageable!(sid, flags);
    }

    /// <summary>
    /// 캐리어의 사전 파괴를 실행한다.
    /// </summary>
    /// <param name="sid">삭제 대상 슬롯 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트.</param>
    public void PreDestroy(int sid, uint flags)
    {
        Validate(sid, flags);
        _hooks.DamageablePre!(sid, flags);
        if (_state.ReadBoss() && FindContained(sid, 1, 0, 0) != 0)
        {
            _hooks.DependentFound!();
        }
    }

    /// <summary>
    /// 타입이 일치하는 종속 가운데 mask WORD 와 kind WORD 조건을 만족하는 것을 찾는다.
    /// 0 인 조건은 생략하고, index 개만큼의 일치를 건너뛴다.
    /// </summary>
    /// <param name="parent">부모 슬롯 번호.</param>
    /// <param name="mask">mask WORD 와 AND 할 값. 0 이면 조건을 생략한다.</param>
    /// <param name="kind">kind WORD 와 같아야 할 값. 0 이면 조건을 생략한다.</param>
    /// <param name="index">앞에서 건너뛸 일치 개수.</param>
    /// <param name="allowDead">죽은 부모 조회를 허용할지.</param>
    /// <returns>찾은 종속 번호. 없으면 0.</returns>
    public int FindContained(int parent, uint mask, uint kind, uint index, bool allowDead = false)
    {
        var finder = new RawContainedFinder(_pool, _state);
        // 공용 커서는 가장 최근 종속부터 진행하며 다음 링크를 먼저 저장한다.
        for (int current = finder.Begin(parent, allowDead); current != 0; current = finder.Next())
        {
            ReadOnlySpan<byte> candidate = _pool.Slot(current);
            if ((kind == 0 || Read(candidate, KindOffset, 2) == kind)
                && (mask == 0 || (Read(candidate, MaskOffset, 2) & mask) != 0))
            {
                if (index == 0)
                {
                    return current;
                }
                index--;
            }
        }
        return 0;
    }

    /// <summary>같은 풀의사제Carrier경계만 실제 Validate/PreDestroy로 교체하고 보호막 함수 사본은 보존한다.</summary>
    public static PriestPreDestroyHooks MakeCarrierPreDestroyHooks(RawSidPool pool, RawCarrierPreDestroy carrier, PriestPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(carrier); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, carrier.Pool)) throw new ArgumentException("사제/Carrier preDestroy의 SID 풀이 다릅니다");
        return new() { FindForcefield = hooks.FindForcefield, DestroyForcefield = hooks.DestroyForcefield, ValidateCarrier = carrier.Validate, CarrierPre = carrier.PreDestroy };
    }

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다.
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
