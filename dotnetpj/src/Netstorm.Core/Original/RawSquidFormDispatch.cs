namespace Netstorm.Core.Original;

/// <summary>
/// 자산 뿌리의 가상 삭제 재정의를 공통 삭제 앞에 연결하는 경계(원본 SquidAssetDestroy).
/// 거짓을 돌려주면 그 타입의 원본 가상 삭제가 삭제를 거부한 것이다.
/// </summary>
/// <param name="sid">지울 뿌리 번호.</param>
/// <param name="flags">삭제 플래그.</param>
/// <param name="hooks">공통 효과 콜백 묶음.</param>
public delegate bool SquidAssetDestroy(int sid, uint flags, SquidDestroyHooks hooks);

/// <summary>
/// 월드·UI·프로세스의 내장 Form 가상 삭제를 같은 SID 풀·공통 삭제 깊이로 분배하는 몸체
/// (원본 SquidFormDispatch, 10.78).
/// 타입 번호마다 등록된 삭제 훅을 골라 Form 종류별 효과를 섞인 종속 체인에서도 중첩 삭제할 수 있게 한다.
/// 기준 판본은 10.78 뿐이다. 프로세스 Form(10~62) 등록은 실제 Kernel 프로세스 호스트가 아직 이식되지 않아 비워 둔다.
/// 근거: cpppj/src/o/SquidFormDispatch.cpp (W03 삭제·수명 열다섯째 조각, 2026-10-23).
/// </summary>
public sealed class RawSquidFormDispatch
{
    /// <summary>감시 쌍 Form(타입 1)의 번호.</summary>
    private const uint DependFormType = 1;

    /// <summary>내용물 Form(타입 6)의 번호.</summary>
    private const uint ContentFormType = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>Form 객체의 슬롯 풀. 등록된 호스트·삭제 본체와 같은 풀이어야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>공통 삭제 본체. Form 뿌리는 이 몸체로 지운다.</summary>
    private readonly RawSquidDestroy _destroy;

    /// <summary>자산 공통 훅. SetAssets 로 교체될 수 있어 읽기 전용이 아니다.</summary>
    private SquidDestroyHooks _assets;

    /// <summary>자산 뿌리의 가상 삭제 재정의. null 이면 모든 자산을 공통 본체로 지운다.</summary>
    private SquidAssetDestroy? _assetDestroy;

    /// <summary>타입 번호별로 선택할 삭제·통지 처리 경로.</summary>
    private readonly SquidDestroyHooks?[] _routes = new SquidDestroyHooks?[OriginalConstructors.FirstAssetType];

    /// <summary>타입 번호별 분배 경계가 연결됐는지.</summary>
    private readonly bool[] _bound = new bool[OriginalConstructors.FirstAssetType];

    /// <summary>
    /// 분배기를 만든다. 삭제 본체는 같은 풀이어야 하고, 자산 공통 훅에는 효과·선택 콜백이 모두 있어야 한다.
    /// </summary>
    /// <param name="pool">Form 슬롯을 가진 풀.</param>
    /// <param name="destroy">공통 삭제 본체. 같은 풀이어야 한다.</param>
    /// <param name="assets">자산 공통 효과·선택 경계.</param>
    /// <param name="assetDestroy">자산 뿌리의 가상 삭제 재정의. null 이면 공통 본체만 쓴다.</param>
    public RawSquidFormDispatch(RawSidPool pool, RawSquidDestroy destroy, SquidDestroyHooks assets,
        SquidAssetDestroy? assetDestroy = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(destroy);
        ArgumentNullException.ThrowIfNull(assets);
        if (!ReferenceEquals(destroy.Pool, pool))
        {
            throw new ArgumentException("Form 분배 공통 삭제 SID 풀 불일치", nameof(destroy));
        }
        if (assets.Emit is null || assets.Selected is null)
        {
            throw new ArgumentException("Form 분배 자산 공통 경계 필요", nameof(assets));
        }
        _pool = pool;
        _destroy = destroy;
        _assets = assets;
        _assetDestroy = assetDestroy;
    }

    /// <summary>월드·클라이언트가 같은 분배기를 공유하는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 가상 몸체의 효과·Unpop 두 경계가 함께 있어야 완전한 Form 등록으로 인정한다.
    /// 다른 풀·타입 범위 밖·중복 등록은 슬롯을 바꾸기 전에 거부한다.
    /// </summary>
    /// <param name="pool">등록하는 호스트의 풀. 분배기와 같아야 한다.</param>
    /// <param name="type">등록할 타입 번호(자산 타입 미만).</param>
    /// <param name="hooks">등록할 가상 훅 묶음.</param>
    private void Register(RawSidPool pool, uint type, SquidDestroyHooks hooks)
    {
        if (!ReferenceEquals(pool, _pool))
        {
            throw new ArgumentException("Form 분배 등록 풀 불일치", nameof(pool));
        }
        if (type >= _routes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "Form 분배 등록 타입 범위 오류");
        }
        if (_bound[type])
        {
            throw new InvalidOperationException("Form 분배 타입 중복 등록");
        }
        if (hooks.Emit is null || hooks.UnpopForm is null)
        {
            throw new ArgumentException("Form 분배 가상 경계 누락", nameof(hooks));
        }
        _routes[type] = hooks;
        _bound[type] = true;
    }

    /// <summary>감시 쌍(타입 1)의 상대 삭제·알림을 같은 분배기의 다른 Form·자산에도 도달시킨다.</summary>
    /// <param name="forms">감시 쌍 호스트.</param>
    public void Bind(RawDependFormHost forms)
    {
        ArgumentNullException.ThrowIfNull(forms);
        Register(forms.Pool, DependFormType, forms.Hooks());
    }

    /// <summary>소유 UI 알림이 중첩 Form·프로세스를 삭제하는 경로를 등록한다.</summary>
    /// <param name="forms">GumpForm 호스트.</param>
    public void Bind(RawGumpFormHost forms)
    {
        ArgumentNullException.ThrowIfNull(forms);
        Register(forms.Pool, RawGumpFormHost.GumpFormType, forms.Hooks());
    }

    /// <summary>내용물의 경제 효과와 부모 제거 알림을 등록한다.</summary>
    /// <param name="forms">내용물 수명 몸체.</param>
    public void Bind(RawContentLifetime forms)
    {
        ArgumentNullException.ThrowIfNull(forms);
        Register(forms.Pool, ContentFormType, forms.Hooks());
    }

    /// <summary>
    /// 전체 Form 경로가 합성한 현재 자산 훅·가상 삭제 경계를 공유하게 한다. 등록된 Form 훅은 그대로 현재 경계를 읽는다.
    /// </summary>
    /// <param name="assets">자산 공통 효과·선택 경계.</param>
    /// <param name="assetDestroy">자산 뿌리의 가상 삭제 재정의.</param>
    public void SetAssets(SquidDestroyHooks assets, SquidAssetDestroy? assetDestroy)
    {
        ArgumentNullException.ThrowIfNull(assets);
        if (assets.Emit is null || assets.Selected is null)
        {
            throw new ArgumentException("Form 분배 현재 자산 경계 필요", nameof(assets));
        }
        _assets = assets;
        _assetDestroy = assetDestroy;
    }

    /// <summary>
    /// 현재 타입별 가상 루트 삭제다. 자산 뿌리(타입 70 이상)는 등록된 재정의가 있으면 그쪽으로 보내고,
    /// Form 뿌리는 공통 본체로 지운다. 거부 값을 보존하기 위해 재정의의 반환 값을 그대로 돌려준다.
    /// </summary>
    /// <param name="sid">지울 루트 번호.</param>
    /// <param name="flags">삭제 플래그.</param>
    public bool Destroy(int sid, uint flags = 0)
    {
        uint type = _pool.Slot(sid)[TypeOffset];
        if (type >= OriginalConstructors.FirstAssetType && _assetDestroy is not null)
        {
            return _assetDestroy(sid, flags, Hooks());
        }
        _destroy.Destroy(sid, flags, Hooks());
        return true;
    }

    /// <summary>
    /// 공유 분배기의 훅을 만든다. 공통 효과는 파생 fallback 으로 다시 분배하지 않아 같은 타입의 순환 호출을 막는다.
    /// 전송·선택 해제는 자산 경계로 바로 보내고, 종속 삭제는 루트 재진입으로 처리해 자산 재정의를 보존한다.
    /// </summary>
    public SquidDestroyHooks Hooks() => new()
    {
        // 선택 번호는 현재 자산 경계에서 읽는다
        Selected = _assets.Selected ?? throw new InvalidOperationException("Form 분배 자산 선택 경계 미연결"),
        // 파생 Unpop 도 현재 타입별 실제 호스트가 처리한다
        UnpopForm = sid =>
        {
            SquidDestroyHooks route = Route(sid);
            // 등록 시점에 검사했지만 자산 경계 교체 뒤에도 계약을 지킨다
            if (route.UnpopForm is null)
            {
                throw new InvalidOperationException("Form 분배 등록 Unpop 경계 누락");
            }
            route.UnpopForm(sid);
        },
        Emit = effect =>
        {
            if (effect.Effect == SquidDestroyEffect.Transmit || effect.Effect == SquidDestroyEffect.ClearSelection)
            {
                EmitAssets(effect);
                return;
            }
            if (effect.Effect == SquidDestroyEffect.DestroyDependent)
            {
                Destroy(effect.Sid, effect.Flags);
                return;
            }
            if (_pool.Slot(effect.Sid)[TypeOffset] < OriginalConstructors.FirstAssetType)
            {
                SquidDestroyHooks route = Route(effect.Sid);
                if (route.Emit is null)
                {
                    throw new InvalidOperationException("Form 분배 등록 효과 경계 누락");
                }
                route.Emit(effect);
                return;
            }
            EmitAssets(effect);
        },
    };

    /// <summary>콜백 뒤 다음 효과는 현재 raw 타입으로 다시 선택한다.</summary>
    /// <param name="sid">효과 대상 번호.</param>
    private SquidDestroyHooks Route(int sid)
    {
        uint type = _pool.Slot(sid)[TypeOffset];
        if (type >= _routes.Length || !_bound[type])
        {
            throw new InvalidOperationException("Form 분배 미등록 타입");
        }
        return _routes[type]!;
    }

    /// <summary>자산 공통 효과를 현재 경계로 보낸다. 경계 교체 뒤에도 매번 최신 콜백을 읽는다.</summary>
    /// <param name="effect">보낼 삭제 효과.</param>
    private void EmitAssets(SquidDestroyEvent effect)
    {
        if (_assets.Emit is null)
        {
            throw new InvalidOperationException("Form 분배 자산 효과 경계 미연결");
        }
        _assets.Emit(effect);
    }
}
