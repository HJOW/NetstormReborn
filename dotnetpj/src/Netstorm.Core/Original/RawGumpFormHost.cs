namespace Netstorm.Core.Original;

/// <summary>
/// GumpForm 의 소유 UI 등록 ID 와 부모 번호를 알리는 경계(원본 GumpFormNotice).
/// 소유 ID 는 슬롯 +0x12 의 정렬되지 않은 DWORD 전체이며, 호출 안에서 같은 풀의 다른 Form 을 삭제할 수 있다.
/// </summary>
/// <param name="owner">Gump 등록 ID(0 이면 알리지 않는다).</param>
/// <param name="parent">알림 시점의 Form 부모 번호.</param>
public delegate void GumpFormNotice(uint owner, int parent);

/// <summary>
/// 메뉴 GumpForm(타입 3)의 생성·소유·부착·통지·삭제를 원본 raw 슬롯 풀에 옮긴 몸체
/// (원본 004af160 부착, 004ae140 ↔ CD 004af270 Unpop, 가상 +0x1c/+0x78 알림, 가상 +0x10 삭제, 004644e0 소유 ID).
/// 일반 Form 부착과 달리 부모의 첫 종속 항목으로 들어가고, 소유 UI 알림은 부모 삭제 중에도 한 번 나간다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 부모 0 부착이 assert 경로라서 옮기지 않았다).
/// 근거: cpppj/src/o/GumpFormHost.cpp (W03 삭제·수명 열다섯째 조각, 2026-10-23).
/// </summary>
public sealed class RawGumpFormHost
{
    /// <summary>두 판본의 내장 GumpForm 타입 번호. ProcessForm(2)·ContentForm(6)과 별도 타입이다.</summary>
    public const uint GumpFormType = 3;

    /// <summary>슬롯 안 다음 종속 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치. 부모가 첫 종속을 가리킨다.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 이전 종속 번호(16비트)의 위치. Form 은 여기, contained 자산은 부모 자리(+16)를 쓴다.</summary>
    private const int PreviousOffset = 14;

    /// <summary>슬롯 안 부모 번호(16비트)의 위치.</summary>
    private const int ParentOffset = 16;

    /// <summary>슬롯 안 소유 UI 등록 ID(정렬되지 않은 4바이트)의 위치.</summary>
    private const int OwnerOffset = 18;

    /// <summary>contained 자산이 이전 종속 번호를 두는 위치. Form 의 +14 와 자리가 겹치지 않게 구분한다.</summary>
    private const int AssetPreviousOffset = 16;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 죽음(삭제 준비) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 월드 밖(void) 비트. Pop 전에는 켜져 있고 부착하면 꺼진다.</summary>
    private const byte VoidBit = 4;

    /// <summary>상태 바이트의 부모 종속(contained) 비트. 부착하면 켜진다.</summary>
    private const byte ContainedBit = 8;

    /// <summary>상태 바이트의 전송 중 임시 비트. Pop 상태 검사에서 dead 와 함께 거부한다.</summary>
    private const byte TransmittingBit = 0x10;

    /// <summary>Pop 플래그의 서버 쪽 표시(원본 kServerPop=8).</summary>
    private const uint ServerPop = 8;

    /// <summary>Pop 플래그의 클라이언트 쪽 표시(원본 kClientPop=0x10).</summary>
    private const uint ClientPop = 0x10;

    /// <summary>Pop 플래그의 종속 전송 억제 비트(원본 kDependentPop=0x40).</summary>
    private const uint DependentPop = 0x40;

    /// <summary>GumpForm 부착을 막는 emplacement·vortex·factory genus 묶음(원본 0x44200).</summary>
    private const uint NoContainerGenus = 0x44200;

    /// <summary>GumpForm 슬롯을 가진 풀. 생성·삭제와 같은 풀이어야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>내장 Form 생성기. 별도 풀을 만들지 않고 같은 생성 계약을 쓴다.</summary>
    private readonly RawSquidFactory _factory;

    /// <summary>타입 번호 순서의 타입 레코드. 생성 시 복사해 부착 판정 중의 입력을 고정한다.</summary>
    private readonly OriginalTypeRecord[] _types;

    /// <summary>공통 삭제 본체. 같은 풀을 써야 한다.</summary>
    private readonly RawSquidDestroy _destroy;

    /// <summary>소유 UI 알림 경계. 소유 ID 가 0 이 아닌 Form 에만 필요하다.</summary>
    private readonly GumpFormNotice? _notice;

    /// <summary>다른 Form 타입의 기존 삭제 분배. 이 몸체가 처리하지 않는 타입은 여기로 넘긴다.</summary>
    private readonly SquidDestroyHooks? _fallback;

    /// <summary>공통 Take/Unpop 몸체. 같은 풀을 쓴다.</summary>
    private readonly RawSquidForms _forms;

    /// <summary>
    /// GumpForm 처리기를 만든다. 풀·팩토리·삭제 본체는 같은 풀이어야 하고, 타입 표는 10.78 의 188개여야 한다.
    /// 알림·fallback 은 나중에 필요해질 수 있으므로 null 을 허용하되, 그 경로에 닿으면 쓰기 전에 예외로 멈춘다.
    /// </summary>
    /// <param name="pool">GumpForm 슬롯을 가진 풀.</param>
    /// <param name="factory">내장 Form 생성기. 같은 풀이어야 한다.</param>
    /// <param name="types">타입 번호 순서의 타입 레코드(188개).</param>
    /// <param name="destroy">공통 삭제 본체. 같은 풀이어야 한다.</param>
    /// <param name="notice">소유 UI 알림 경계. 소유 ID 가 있는 Form 을 다루려면 필요하다.</param>
    /// <param name="fallback">다른 Form/자산 타입의 기존 삭제 분배.</param>
    public RawGumpFormHost(RawSidPool pool, RawSquidFactory factory, IReadOnlyList<OriginalTypeRecord> types,
        RawSquidDestroy destroy, GumpFormNotice? notice = null, SquidDestroyHooks? fallback = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(destroy);
        if (!ReferenceEquals(factory.Pool, pool) || !ReferenceEquals(destroy.Pool, pool))
        {
            throw new ArgumentException("GumpForm 생성/삭제 SID 풀이 다릅니다");
        }
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"GumpForm 타입 표가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _factory = factory;
        // 타입 표를 값으로 복사해 부착 판정 중 호출자가 값을 바꿔도 기준이 흔들리지 않게 한다
        OriginalTypeRecord[] copy = new OriginalTypeRecord[types.Count];
        // 타입 번호 순서대로 한 항목씩 복사한다
        for (int i = 0; i < types.Count; i++)
        {
            copy[i] = types[i].Clone();
        }
        _types = copy;
        _destroy = destroy;
        _notice = notice;
        _fallback = fallback;
        // 부모 제거 알림은 ContentForm 전용이므로 이 몸체는 통지 없이 공통 Take/Unpop 만 쓴다
        _forms = new RawSquidForms(pool);
    }

    /// <summary>이 처리기가 쓰는 슬롯 풀. 전체 월드 분배기가 같은 풀인지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 메뉴의 GumpForm 을 실제 팩토리로 만든다(원본 004af530 계열). 기본 flags 2 는 클라이언트 영역 생성이다.
    /// 생성 자체는 부모·소유 ID 를 쓰지 않는다.
    /// </summary>
    /// <param name="flags">생성 영역 플래그(1 예측, 2 클라이언트, 0 서버).</param>
    public int Create(uint flags = 2) => _factory.CreateGumpForm(flags);

    /// <summary>
    /// 서버·예측 번호를 같은 raw 풀의 지정 Take 로 받는다. 기존 GumpForm 이면 부모에서 해제한 뒤 다시 쓴다.
    /// </summary>
    /// <param name="form">받을 슬롯 번호(서버 영역만).</param>
    public int Take(int form) => _forms.Take(GumpFormType, form);

    /// <summary>
    /// 번호가 GumpForm 인지 판별한다. 반납된 슬롯의 타입 바이트도 보존되므로 free 여부는 여기서 보지 않는다
    /// (이미 dead 인 중첩 삭제를 원본처럼 건너뛰기 위해서다).
    /// </summary>
    /// <param name="form">확인할 번호.</param>
    public bool IsGumpForm(int form) =>
        form >= RawSidPool.FirstOrdinary && form != RawSidPool.PredictableFirst
        && form < _pool.Capacity && _pool.Slot(form)[TypeOffset] == GumpFormType;

    /// <summary>현재 raw DWORD 소유 UI 등록 ID 를 읽는다. free 슬롯은 읽기 계약 밖이므로 거부한다.</summary>
    /// <param name="form">읽을 GumpForm 번호.</param>
    public uint Owner(int form)
    {
        ReadOnlySpan<byte> raw = Form(form);
        uint owner = 0;
        // 높은 두 바이트를 버리지 않고 원본 소유 ID 를 조립한다
        for (int index = 0; index < 4; index++)
        {
            owner |= (uint)raw[OwnerOffset + index] << (index * 8);
        }
        return owner;
    }

    /// <summary>원본 Form getParent(+0x0)와 같이 부모 WORD 를 돌려준다.</summary>
    /// <param name="form">부모를 읽을 GumpForm 번호.</param>
    public int Parent(int form)
    {
        Form(form);
        return Word(form, ParentOffset);
    }

    /// <summary>
    /// Gump 등록 ID 를 슬롯 +0x12 에 쓴다(원본 004644e0). WORD 로 절단하거나 호스트 포인터로 해석하지 않는다.
    /// 생성 후 부모에 붙기 전에도 허용한다.
    /// </summary>
    /// <param name="form">쓸 GumpForm 번호.</param>
    /// <param name="owner">Gump 등록 ID.</param>
    public void SetOwner(int form, uint owner)
    {
        Form(form);
        Put(form, OwnerOffset, owner, 4);
    }

    /// <summary>
    /// 가상 +0x74 → 공통 contained 부착(원본 004af160). 부모의 종속 머리 앞에 넣고 void 를 끈다.
    /// 10.78 은 부모 0 부착을 조용히 넘기며, 부모·머리·목록 genus 검사는 쓰기 전에 끝난다.
    /// 서버 Form 의 실제 Pop 직렬화/전송은 아직 복원하지 않았으므로 그 경로는 쓰기 전에 거부한다.
    /// </summary>
    /// <param name="form">부착할 GumpForm 번호.</param>
    /// <param name="parent">부모 번호. 0 이면 무동작.</param>
    /// <param name="flags">Pop 플래그.</param>
    public void Attach(int form, int parent, uint flags = 0)
    {
        if (parent == 0)
        {
            // 10.78 GumpForm 은 부모 0 부착을 조용히 넘긴다
            return;
        }
        ReadOnlySpan<byte> raw = Form(form);
        int type = raw[TypeOffset];
        if (type >= _types.Length)
        {
            throw new InvalidOperationException("GumpForm 타입 번호 오류");
        }
        if ((_types[type].Flags2 & NoContainerGenus) != 0)
        {
            throw new InvalidOperationException("GumpForm contained genus 오류");
        }
        if (parent < RawSidPool.FirstOrdinary || parent == RawSidPool.PredictableFirst || parent >= _pool.Capacity
            || (_pool.Slot(parent)[StateOffset] & (FreeBit | DeadBit)) != 0 || parent == form)
        {
            throw new InvalidOperationException("GumpForm 부착 부모 상태 오류");
        }
        ValidatePop(form, flags);
        int head = Word(parent, HeadOffset);
        // 이전 머리는 살아 있는 일반 번호여야 하고 자기 자신이면 순환이 된다
        if (head == form || head >= _pool.Capacity || (head != 0 && (head < RawSidPool.FirstOrdinary
            || head == RawSidPool.PredictableFirst || (_pool.Slot(head)[StateOffset] & FreeBit) != 0)))
        {
            throw new InvalidOperationException("GumpForm 부모 종속 머리 오류");
        }
        // 부모 기록 → 이전 머리를 next 로 기록 → 이전 머리의 prev → 부모 head → 자신 prev → contained → void 해제 순서다
        Put(form, ParentOffset, (uint)parent, 2);
        Put(form, NextOffset, (uint)head, 2);
        if (head != 0)
        {
            SetPrevious(head, (ushort)form);
        }
        Put(parent, HeadOffset, (uint)form, 2);
        Put(form, PreviousOffset, 0, 2);
        _pool.AllocatedBytes(form)[StateOffset] |= ContainedBit;
        _pool.AllocatedBytes(form)[StateOffset] &= unchecked((byte)~VoidBit);
        // GumpForm postPop 은 늘린 공통 깊이를 즉시 되돌리는 빈 몸체이므로 바깥 효과가 없다
    }

    /// <summary>
    /// 원본 004ae140 ↔ CD 004af270: void 를 켜고 살아 있는 부모의 체인에서 뺀다.
    /// 부모 free/dead 이면 링크와 contained 를 원본처럼 보존한다.
    /// </summary>
    /// <param name="form">해제할 GumpForm 번호.</param>
    public void Unpop(int form)
    {
        Form(form);
        _forms.Unpop(form);
    }

    /// <summary>
    /// 가상 +0x1c·+0x78 은 같은 알림이다. 호출 인자로 받은 삭제 부모 대신 Form 의 현재 부모를 보낸다.
    /// 소유 ID 와 부모를 모두 읽은 뒤 알리며, 알림 뒤에는 Form 을 다시 읽지 않는다(콜백이 슬롯을 바꿀 수 있다).
    /// </summary>
    /// <param name="form">알릴 GumpForm 번호.</param>
    public void Notify(int form)
    {
        uint owner = Owner(form);
        if (owner == 0)
        {
            return;
        }
        int parent = Parent(form);
        if (_notice is null)
        {
            throw new InvalidOperationException("GumpForm 소유 UI 알림 경계 필요");
        }
        _notice(owner, parent);
    }

    /// <summary>
    /// 가상 +0x10: 공통 삭제 → 실제 Unpop → 번호 반납을 같은 훅으로 실행한다. dead 번호는 본체가 바로 돌아온다.
    /// Form 자체의 삭제에는 소유 UI 알림이 없다(부모 삭제의 종속 알림만 낸다).
    /// </summary>
    /// <param name="form">삭제할 GumpForm 번호.</param>
    /// <param name="flags">삭제 플래그.</param>
    public void Destroy(int form, uint flags = 0)
    {
        if (!IsGumpForm(form))
        {
            throw new InvalidOperationException("GumpForm 삭제 타입 오류");
        }
        _destroy.Destroy(form, flags, Hooks());
    }

    /// <summary>
    /// GumpForm 만 이 몸체가 처리하고 다른 타입의 기존 분배는 fallback 으로 넘기는 삭제 훅을 만든다.
    /// 전체 월드 삭제도 이 훅을 써야 자산에 붙은 GumpForm 의 UI 알림이 실제로 도달한다.
    /// 종속 파괴는 같은 가상 분배로 다시 들어가고, 종속 해제는 부모 알림이다.
    /// </summary>
    public SquidDestroyHooks Hooks() => new()
    {
        // 기존 월드의 선택 번호를 유지하며 연결이 없으면 선택 없음으로 답한다
        Selected = _fallback?.Selected ?? (() => 0),
        // 공통 삭제가 실제 Unpop 을 요청하면 GumpForm 만 여기서 처리한다
        UnpopForm = sid =>
        {
            if (IsGumpForm(sid))
            {
                Unpop(sid);
            }
            else if (_fallback?.UnpopForm is not null)
            {
                _fallback.UnpopForm(sid);
            }
            else
            {
                throw new InvalidOperationException("GumpForm 외 Unpop 경계 필요");
            }
        },
        Emit = effect =>
        {
            if (IsGumpForm(effect.Sid))
            {
                switch (effect.Effect)
                {
                    // GumpForm 사전·사후는 이 몸체가 공통 깊이를 직접 닫는다
                    case SquidDestroyEffect.PreDestroy:
                        _destroy.CompletePreDestroy();
                        return;
                    case SquidDestroyEffect.PostDestroy:
                        _destroy.CompletePostDestroy();
                        return;
                    case SquidDestroyEffect.ReleaseDependent:
                        Notify(effect.Sid);
                        return;
                    case SquidDestroyEffect.DestroyDependent:
                        Destroy(effect.Sid, effect.Flags);
                        return;
                    default:
                        break;
                }
            }
            if (_fallback?.Emit is null)
            {
                throw new InvalidOperationException("GumpForm 외 삭제 효과 경계 필요");
            }
            _fallback.Emit(effect);
        },
    };

    /// <summary>
    /// 원본 필드를 읽되 범위·타입·free 상태를 검사한다. dead 는 공통 삭제 중에 필요하므로 허용한다.
    /// </summary>
    /// <param name="form">읽을 GumpForm 번호.</param>
    private ReadOnlySpan<byte> Form(int form)
    {
        if (!IsGumpForm(form) || (_pool.Slot(form)[StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("GumpForm 슬롯 상태 오류");
        }
        return _pool.Slot(form);
    }

    /// <summary>정렬되지 않은 WORD 를 리틀 엔디언으로 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private ushort Word(int sid, int offset)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>
    /// 원본 WORD/DWORD store 를 같은 폭의 리틀 엔디언 쓰기로 옮긴다. 옆의 정렬되지 않은 필드는 보존한다.
    /// </summary>
    /// <param name="sid">쓸 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓸 바이트 수.</param>
    private void Put(int sid, int offset, uint value, int width)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        // 지정한 폭만 쓰므로 이웃 payload 를 건드리지 않는다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 종속 체인에는 Form 과 contained 자산이 섞일 수 있으며 각 가상 setPrev 의 위치가 다르다.
    /// </summary>
    /// <param name="sid">이전 종속을 받을 이웃 번호.</param>
    /// <param name="previous">쓸 이전 종속 번호.</param>
    private void SetPrevious(int sid, ushort previous)
    {
        int offset = _pool.Slot(sid)[TypeOffset] < OriginalConstructors.FirstAssetType ? PreviousOffset : AssetPreviousOffset;
        Put(sid, offset, previous, 2);
    }

    /// <summary>
    /// 원본 Pop 상태·권한 검사를 그대로 수행한다. void 가 아니거나 dead·전송 중이면 거부하고,
    /// 서버 풀의 일반 Form 은 클라이언트 표시를, 클라이언트 Form 은 서버 표시를 요구한다.
    /// 10.78 GumpForm 은 서버 Form 의 실제 전송 몸체가 미복원이라 그 경로를 쓰기 전에 거부한다.
    /// </summary>
    /// <param name="form">검사할 GumpForm 번호.</param>
    /// <param name="flags">호출자가 준 Pop 플래그.</param>
    private void ValidatePop(int form, uint flags)
    {
        byte state = Form(form)[StateOffset];
        if ((state & VoidBit) == 0 || (state & (DeadBit | TransmittingBit)) != 0)
        {
            throw new InvalidOperationException("GumpForm Pop 상태 오류");
        }
        // 클라이언트 풀은 원본처럼 권한 판정을 하지 않는다
        if (!_pool.IsServer)
        {
            return;
        }
        if (form < RawSidPool.ServerFirst)
        {
            if ((flags & ServerPop) != 0)
            {
                throw new InvalidOperationException("client GumpForm의 서버 Pop 금지");
            }
            flags |= ClientPop;
        }
        else if ((flags & ClientPop) == 0)
        {
            flags |= ServerPop;
        }
        if ((flags & (ClientPop | DependentPop)) == 0)
        {
            throw new InvalidOperationException("서버 GumpForm의 Pop 전송은 미복원입니다");
        }
    }
}
