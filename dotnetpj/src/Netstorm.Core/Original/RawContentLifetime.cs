namespace Netstorm.Core.Original;

/// <summary>
/// 내용물(너겟) 삭제 중 효과가 갱신할 수 있는 원본 전역. 매 분기·요청에서 최신 값을 다시 읽는다
/// (원본 ContentLifetimeState, cpppj/src/o/ContentFormLifetime.h).
/// </summary>
public sealed class ContentLifetimeState
{
    /// <summary>잡고 있는 내용물 번호(원본 패치 0055a2a8 / CD 0051b658). 0 이면 없음.</summary>
    public int Cursor { get; set; }

    /// <summary>너겟 종류 WORD(+18)와 비교하는 현재 타입 번호(원본 패치 005412c0). 기본 154.</summary>
    public uint NuggetType { get; set; } = 154;

    /// <summary>로컬 소유자 번호(원본 패치 00540c70). 수입을 로컬 알림으로 보낼지 정한다.</summary>
    public uint LocalOwner { get; set; } = 1;

    /// <summary>로컬 수입 알림 명령 번호(원본 패치 005429c4).</summary>
    public uint Notice { get; set; }

    /// <summary>다른 소유자에게 너겟 알림을 보낼 권한(원본 패치 00540bc4).</summary>
    public bool Boss { get; set; } = true;

    /// <summary>
    /// 실제 세션이 현재 커서를 매 삭제 시 읽게 하는 경계. null 이면 <see cref="Cursor"/> 값형 입력을 쓴다
    /// (미연결 fixture 는 값형 입력만 쓴다).
    /// </summary>
    public Func<int>? CurrentCursor { get; set; }
}

/// <summary>
/// 내용물 삭제가 바깥에 요구하는 실제 UI·패킷·소리·경제 경계 묶음(원본 ContentLifetimeHooks).
/// SP·지갑은 <see cref="RawSquidReward"/> 를 넘기면 그 저장소로 연결되며, 넘기지 않을 때만 지갑·SP 콜백을 요구한다.
/// </summary>
public sealed class ContentLifetimeHooks
{
    /// <summary>잡고 있는 내용물을 놓는 UI 경계(원본 커서 취소 몸체). 내부 중첩 삭제는 같은 월드 분배기로 돌아와야 한다.</summary>
    public Action? CancelCursor { get; set; }

    /// <summary>로컬 수입 알림 큐(원본 00452fe0(command,0,0)).</summary>
    public Action<uint>? LocalNotice { get; set; }

    /// <summary>다른 소유자에게 보내는 빈 수입 패킷(원본 0040abe0 → 004e2dc0(owner,packet)).</summary>
    public Action<uint>? RemoteNotice { get; set; }

    /// <summary>SP 가산에 앞선 별도 AI 지갑 가산(원본 00413bb0(owner, 절삭(cost))).</summary>
    public Action<uint, int>? Wallet { get; set; }

    /// <summary>SP 가산(원본 0040eec0 로컬 / 0041df80 다른 소유자). 마지막 인자는 로컬 여부다.</summary>
    public Action<uint, float, bool>? AddSp { get; set; }

    /// <summary>너겟 수입 위치 소리(원본 004a9d70(center.x,center.y,"guy.wav",0)).</summary>
    public Action<float, float>? Sound { get; set; }
}

/// <summary>
/// 생산 목록 ContentForm(타입 6)의 삭제 수명을 원본 raw 슬롯 풀에 옮긴 몸체
/// (원본 패치 004aeeb0 / CD 004b0300, ContentFormLifetime).
/// 순서는 원본 그대로 "커서 취소 검사 → 너겟 효과 → 공통 pre 깊이 반납"이며, 삭제 본체는 같은 공통 삭제기를 쓴다.
/// 너겟 효과는 "로컬/원격 수입 알림 → 선지급 AI 지갑 → 현재 소유자·비용 재조회 뒤 SP 가산 → 위치 소리" 순서이고,
/// 모든 콜백 사이에서 부모 소유자·타입 비용·로컬 소유자를 다시 읽는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 공통 깊이 반납만 하고 너겟 효과가 없어서 옮기지 않았다).
/// 근거: cpppj/src/o/ContentFormLifetime.cpp (W03 내용물 수명 조각, 2026-10-23).
/// </summary>
public sealed class RawContentLifetime
{
    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 부모 번호(16비트)의 위치.</summary>
    private const int ParentOffset = 16;

    /// <summary>슬롯 안 내용물 종류(16비트)의 위치. y 좌표와 같은 자리지만 대상 객체가 다르다.</summary>
    private const int KindOffset = 18;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 선지급 억제 표시(1바이트)의 위치. 목록 순서 BYTE(+23)와 다른 자리다.</summary>
    private const int PaidOffset = 22;

    /// <summary>슬롯 안 부모 소유자(1바이트)의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>ContentForm 타입 번호.</summary>
    private const byte ContentType = 6;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 반납(죽음) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>일반 번호의 첫 값. 0~4 번은 일반 할당 대상이 아니다.</summary>
    private const int FirstOrdinary = 5;

    /// <summary>CRT 의 유효한 signed64 변환 범위 안에서 기존 SP 모듈과 같은 하위 DWORD 정책을 쓰는 한계.</summary>
    private const double IntegerLimit = 9.2e18;

    /// <summary>내용물 슬롯을 읽고 쓰는 풀. Form·삭제·보상 모듈과 같아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 비용·발자국을 조회하는 타입 표. 콜백이 바꾼 현재 값을 읽기 위해 복사하지 않는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>객체에 부착된 Form 의 생성·통지·수명 처리기.</summary>
    private readonly RawContentFormHost _forms;

    /// <summary>객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.</summary>
    private readonly RawSquidDestroy _destroy;

    /// <summary>삭제 중 효과가 갱신할 수 있는 원본 전역. 매 분기·요청에서 최신 값을 읽는다.</summary>
    private readonly ContentLifetimeState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음.</summary>
    private readonly ContentLifetimeHooks _hooks;

    /// <summary>Content 밖 파생 Form/자산 분배를 넘기는 대체 훅. emit 은 같은 풀을 바꿀 수 있다.</summary>
    private readonly SquidDestroyHooks _fallback;

    /// <summary>HP·SP·지갑 보상을 실제 장부에 적용하는 처리기. null 이면 콜백으로 지갑·SP 를 요구한다.</summary>
    private readonly RawSquidReward? _reward;

    /// <summary>
    /// 내용물 수명을 만든다. 풀·타입 표·Form·삭제 처리기는 이 객체보다 오래 살아야 한다.
    /// 서로 다른 풀의 모듈이나 10.78 이 아닌 타입 표는 쓰기 전에 거부한다.
    /// </summary>
    /// <param name="pool">내용물 슬롯을 가진 풀.</param>
    /// <param name="types">10.78 타입 표(188개). 콜백이 바꾼 값을 읽으므로 복사하지 않는다.</param>
    /// <param name="forms">ContentForm 처리기. 같은 풀이어야 한다.</param>
    /// <param name="destroy">공통 삭제 본체. 같은 풀이어야 한다.</param>
    /// <param name="state">삭제 중 효과가 갱신하는 전역.</param>
    /// <param name="hooks">UI·패킷·소리·경제 경계.</param>
    /// <param name="fallback">Content 밖 파생 Form/자산 삭제 효과를 넘길 훅. null 이면 그 효과에서 예외다.</param>
    /// <param name="reward">SP·지갑 보상 처리기. null 이면 콜백 경계를 요구한다.</param>
    public RawContentLifetime(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawContentFormHost forms,
        RawSquidDestroy destroy, ContentLifetimeState state, ContentLifetimeHooks hooks,
        SquidDestroyHooks? fallback = null, RawSquidReward? reward = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(forms);
        ArgumentNullException.ThrowIfNull(destroy);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(forms.Pool, pool) || !ReferenceEquals(destroy.Pool, pool)
            || (reward is not null && !ReferenceEquals(reward.Pool, pool)))
        {
            throw new ArgumentException("Content 수명 SID 풀 불일치", nameof(pool));
        }
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"Content 수명 타입 표가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types;
        _forms = forms;
        _destroy = destroy;
        _state = state;
        _hooks = hooks;
        _fallback = fallback ?? new SquidDestroyHooks();
        _reward = reward;
    }

    /// <summary>전체 월드 연결기의 동일 풀 검증에 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// ContentForm 타입 6 인지 확인한다. 반납된(dead) 슬롯도 타입 번호를 보존하므로 참일 수 있으며,
    /// 실제 필드 접근은 각 분기가 free 를 따로 검사한다.
    /// </summary>
    /// <param name="form">확인할 Form 번호.</param>
    public bool IsContentForm(int form)
    {
        // 범위·예측 머리 밖은 원본처럼 내용물이 아니다
        return form >= FirstOrdinary && form < _pool.Capacity && form != RawSidPool.PredictableFirst
            && _pool.Slot(form)[TypeOffset] == ContentType;
    }

    /// <summary>정렬되지 않은 WORD 를 리틀 엔디언으로 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private ushort Word(int sid, int offset)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>패치 너겟 효과에서만 쓰는 현재 부모 소유자 바이트다.</summary>
    /// <param name="parent">부모 번호.</param>
    private uint Owner(int parent) => _pool.Slot(parent)[OwnerOffset];

    /// <summary>효과마다 현재 전역 타입 번호로 현재 비용을 다시 읽는다.</summary>
    private float Cost()
    {
        if (_state.NuggetType >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(_state), _state.NuggetType, "Content 너겟 타입 번호 오류");
        }
        float cost = _types[(int)_state.NuggetType].Cost;
        if (!float.IsFinite(cost))
        {
            throw new ArgumentException("Content 너겟 비용 유한값 필요");
        }
        return cost;
    }

    /// <summary>
    /// 원본 004adea0: footZ 를 포함하지 않은 발자국 절반을 현재 좌표에서 빼고 각 결과를 float 에 저장한다.
    /// </summary>
    /// <param name="parent">중심을 계산할 부모 번호.</param>
    private (float X, float Y) Center(int parent)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(parent);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new InvalidOperationException("Content 중심 부모 타입 오류");
        }
        OriginalTypeRecord type = _types[raw[TypeOffset]];
        // 원본은 각 뺄셈 결과를 단정밀도에 저장한다
        float x = (float)((double)Coordinate(raw, XOffset) - (double)type.FootX * 0.5);
        float y = (float)((double)Coordinate(raw, YOffset) - (double)type.FootY * 0.5);
        return (x, y);
    }

    /// <summary>
    /// 너겟의 선지급 AI 금액을 CRT 처럼 0 방향 절삭하고 하위 32비트를 그대로 쓴다. SP float 금액과 구별한다.
    /// </summary>
    /// <param name="cost">현재 타입 비용.</param>
    private static int Wallet(float cost)
    {
        double value = cost;
        // 원본 CRT 변환이 정의되는 범위 밖이면 쓰기 전에 거부한다
        if (!double.IsFinite(value) || Math.Abs(value) >= IntegerLimit)
        {
            throw new ArgumentOutOfRangeException(nameof(cost), cost, "Content 너겟 지갑 금액 범위 오류");
        }
        return unchecked((int)(uint)(long)value);
    }

    /// <summary>
    /// 004aeeb0/CD 004b0300: 커서 취소 검사 → 너겟 효과 → 같은 공통 pre 깊이 반납.
    /// 이 직접 진입은 이미 공통 삭제기가 pre 깊이를 올린 상태에서 호출한다.
    /// </summary>
    /// <param name="form">삭제 중인 ContentForm 번호.</param>
    public void PreDestroy(int form)
    {
        if (!IsContentForm(form) || (_pool.Slot(form)[StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("Content preDestroy 슬롯 오류");
        }
        int cursor = _state.CurrentCursor is not null ? _state.CurrentCursor() : _state.Cursor;
        if (cursor == form || Word(form, ParentOffset) == form)
        {
            if (_hooks.CancelCursor is null)
            {
                throw new InvalidOperationException("Content 커서 취소 경계 필요");
            }
            _hooks.CancelCursor();
        }
        // 콜백이 바꾼 현재 종류·부모·소유자를 이후 분기에서 다시 읽는다
        if ((uint)Word(form, KindOffset) == _state.NuggetType)
        {
            int parent = Word(form, ParentOffset);
            if (parent == 0 || parent >= _pool.Capacity)
            {
                throw new InvalidOperationException("Content 너겟 부모 번호 오류");
            }
            if ((_pool.Slot(parent)[StateOffset] & DeadBit) == 0)
            {
                if (Owner(parent) == _state.LocalOwner)
                {
                    if (_hooks.LocalNotice is null)
                    {
                        throw new InvalidOperationException("Content 로컬 수입 알림 경계 필요");
                    }
                    _hooks.LocalNotice(_state.Notice);
                }
                else if (_state.Boss)
                {
                    if (_hooks.RemoteNotice is null)
                    {
                        throw new InvalidOperationException("Content 원격 수입 알림 경계 필요");
                    }
                    _hooks.RemoteNotice(Owner(parent));
                }
                if (_pool.Slot(form)[PaidOffset] == 0)
                {
                    uint recipient = Owner(parent);
                    int amount = Wallet(Cost());
                    if (_reward is not null)
                    {
                        _reward.CreditWallet(recipient, amount);
                    }
                    else
                    {
                        if (_hooks.Wallet is null)
                        {
                            throw new InvalidOperationException("Content 지갑 선가산 경계 필요");
                        }
                        _hooks.Wallet(recipient, amount);
                    }
                    // 지갑 콜백 뒤 현재 소유자·비용·로컬 소유자를 다시 읽어 정확한 SP 가산 경로를 고른다
                    uint owner = Owner(parent);
                    float cost = Cost();
                    bool local = owner == _state.LocalOwner;
                    if (_reward is not null)
                    {
                        _reward.AddSp(owner, cost);
                    }
                    else
                    {
                        if (_hooks.AddSp is null)
                        {
                            throw new InvalidOperationException("Content SP 가산 경계 필요");
                        }
                        _hooks.AddSp(owner, cost, local);
                    }
                    if (_hooks.Sound is null)
                    {
                        throw new InvalidOperationException("Content 너겟 위치 소리 경계 필요");
                    }
                    (float x, float y) = Center(parent);
                    _hooks.Sound(x, y);
                }
            }
        }
        _destroy.CompletePreDestroy();
    }

    /// <summary>
    /// 가상 +0x10 의 공통 삭제를 같은 분배 훅으로 실행한다. dead 이면 공통 몸체가 바로 반환한다.
    /// </summary>
    /// <param name="form">삭제할 ContentForm 번호.</param>
    /// <param name="flags">삭제 플래그.</param>
    public void Destroy(int form, uint flags = 0)
    {
        if (!IsContentForm(form))
        {
            throw new InvalidOperationException("Content 삭제 타입 오류");
        }
        _destroy.Destroy(form, flags, Hooks());
    }

    /// <summary>
    /// Content 만 이 몸체가 분배하고 다른 타입의 기존 효과는 fallback 으로 넘기는 공통 삭제 훅을 만든다.
    /// 전체 월드 분배기와 내부 삭제가 같은 훅을 써야 하므로 매 삭제마다 새로 넘긴다.
    /// </summary>
    public SquidDestroyHooks Hooks()
    {
        var hooks = new SquidDestroyHooks
        {
            // 선택 번호가 없으면 원본처럼 0(없음)을 돌려준다
            Selected = _fallback.Selected ?? (() => 0),
            // 실제 제거 알림·부모 dead 링크 보존은 앞서 검증한 ContentFormHost 로 연결한다
            UnpopForm = form =>
            {
                if (IsContentForm(form))
                {
                    _forms.Unpop(form);
                }
                else
                {
                    if (_fallback.UnpopForm is null)
                    {
                        throw new InvalidOperationException("Content 외 Unpop 경계 필요");
                    }
                    _fallback.UnpopForm(form);
                }
            },
            // 깊이 반납과 종속 삭제도 같은 원본 순서를 사용한다
            Emit = effect =>
            {
                if (IsContentForm(effect.Sid))
                {
                    switch (effect.Effect)
                    {
                        case SquidDestroyEffect.PreDestroy:
                            PreDestroy(effect.Sid);
                            return;
                        case SquidDestroyEffect.PostDestroy:
                            _destroy.CompletePostDestroy();
                            return;
                        case SquidDestroyEffect.ReleaseDependent:
                            return;
                        case SquidDestroyEffect.DestroyDependent:
                            Destroy(effect.Sid, effect.Flags);
                            return;
                        default:
                            break;
                    }
                }
                if (_fallback.Emit is null)
                {
                    throw new InvalidOperationException("Content 외 삭제 효과 경계 필요");
                }
                _fallback.Emit(effect);
            },
        };
        return hooks;
    }

    /// <summary>정렬되지 않은 float 의 비트를 원본 바이트 순서대로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">좌표 바이트 위치.</param>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint bits = (uint)(raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24));
        return BitConverter.UInt32BitsToSingle(bits);
    }
}
