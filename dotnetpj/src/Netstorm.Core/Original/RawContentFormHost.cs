namespace Netstorm.Core.Original;

/// <summary>
/// 부모 가상 +0x24(form, reason)의 추가(1)·제거(2) 알림을 받는 경계. 부모의 실제 생산·UI 효과를 연결한다.
/// </summary>
/// <param name="parent">알림을 받는 부모 번호.</param>
/// <param name="form">부착·제거된 Form 번호.</param>
/// <param name="reason">1 = 부착, 2 = 제거.</param>
public delegate void ContentFormNotice(int parent, int form, uint reason);

/// <summary>
/// 사제를 워커·벌룬에 담은 전투 부착 뒤 포획 알림(원본 004927f0, CD 0040df80)을 요청하는 경계.
/// </summary>
/// <param name="parent">내용물을 담은 부모 번호.</param>
/// <param name="previousOwner">내용물 +12 하위 7비트에 남은 이전 사제 소유자.</param>
public delegate void ContentPriestNotice(int parent, uint previousOwner);

/// <summary>
/// 실제 월드의 전투·검사 전역. 부착 알림과 전송이 끝난 뒤의 최신 값을 읽는다(원본 ContentFormState).
/// </summary>
public sealed class ContentFormState
{
    /// <summary>내부 전투 중인지. 전투 중 사제 내용물 부착은 포획 알림을 요구한다.</summary>
    public bool InBattle { get; set; }

    /// <summary>연결 검사를 하는 중인지. 켜져 있으면 포획 알림 전에 실제 체인 연결을 다시 확인한다.</summary>
    public bool CheckingLinks { get; set; }
}

/// <summary>
/// 생산 목록 ContentForm(타입 6)의 정렬 부착·재부착·부모 알림을 원본 raw 슬롯 풀에 옮긴 몸체
/// (원본 004aec10 ↔ CD 004afff0, 004ae080 ↔ CD 004af1c0, 004ae080 의 +0x5c).
/// 일반 Form 과 달리 같은 목록 그룹(플래그 0xfcff) 안에서 순서 번호(+23)가 맞는 자리에 들어간다.
/// 이 조각에서는 contained 생성기(RawContainedCreate) 연결만 옮기지 않았다(다음 이식 대상).
/// 기준 판본은 10.78 뿐이다(CD 는 dead/free 부착을 조용히 넘기는 등 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/ContentFormHost.cpp (W03 삭제·수명 열한째 조각, 2026-10-22).
/// </summary>
public sealed class RawContentFormHost
{
    /// <summary>슬롯 안 다음 종속 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 이전 종속 번호(16비트)의 위치.</summary>
    private const int PreviousOffset = 14;

    /// <summary>슬롯 안 부모 번호(16비트)의 위치. contained 자산은 이 자리에 이전 종속을 둔다.</summary>
    private const int ParentOffset = 16;

    /// <summary>슬롯 안 내용물(생산) 타입 번호(16비트)의 위치.</summary>
    private const int ContentTypeOffset = 18;

    /// <summary>슬롯 안 목록 플래그(16비트)의 위치.</summary>
    private const int FlagsOffset = 20;

    /// <summary>10.78 ContentForm 의 순서 번호(1바이트) 위치.</summary>
    private const int OrderOffset = 23;

    /// <summary>ContentForm 타입 번호.</summary>
    private const byte ContentType = 6;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 반납(죽음) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 부모 종속(contained) 비트.</summary>
    private const byte ContainedBit = 8;

    /// <summary>목록 그룹 비교에서 무시하는 플래그 묶음(원본 0xfcff 마스크).</summary>
    private const ushort GroupMask = 0xfcff;

    /// <summary>WORLD 목록 플래그. 원본은 이 목록의 Robust 를 assert 로 막는다.</summary>
    private const ushort WorldFlag = 0x80;

    /// <summary>정렬(그룹) Robust 판정 비트(0x34).</summary>
    private const ushort RobustFlags = 0x34;

    /// <summary>재부착 전에 끄는 순서 변경 표시 비트.</summary>
    private const ushort ChangedFlag = 0x100;

    /// <summary>ContentForm 슬롯을 공유하는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호 순서의 타입 레코드. 생산 타입과 부모 genus 의 플래그2를 읽는다.</summary>
    private readonly OriginalTypeRecord[] _types;

    /// <summary>부모에 부착(1)·제거(2)를 알리는 경계. 없으면 만들 수 없다.</summary>
    private readonly ContentFormNotice _notice;

    /// <summary>공통 Form 생성·부착·Unpop 몸체. 부모 제거도 이 몸체를 통해 알린다.</summary>
    private readonly RawSquidForms _forms;

    /// <summary>실제 월드의 전투·검사 전역. null 이면 전투 판정을 하지 않는다.</summary>
    private readonly ContentFormState? _state;

    /// <summary>전투 부착 뒤 사제 포획을 알리는 경계. null 이면 그 경로에서 예외다.</summary>
    private readonly ContentPriestNotice? _captured;

    /// <summary>
    /// ContentForm 처리기를 만든다. 풀·타입 표·알림 경계는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">ContentForm 슬롯을 가진 풀.</param>
    /// <param name="types">10.78 타입 표(188개).</param>
    /// <param name="notice">부모 부착·제거 알림.</param>
    /// <param name="transmit">서버 Pop 전송 경계.</param>
    /// <param name="state">전투·검사 전역. null 이면 전투 판정을 하지 않는다.</param>
    /// <param name="captured">전투 사제 포획 알림.</param>
    public RawContentFormHost(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, ContentFormNotice notice,
        Action<int, uint>? transmit = null, ContentFormState? state = null, ContentPriestNotice? captured = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(notice);
        if (types.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"ContentForm 타입 표가 {types.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(types));
        }
        _pool = pool;
        _types = types.ToArray();
        _notice = notice;
        _forms = new RawSquidForms(pool, (parent, form) => notice(parent, form, 2), transmit);
        _state = state;
        _captured = captured;
    }

    /// <summary>다른 모듈이 같은 풀을 쓰는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>공통 Form 몸체. 전체 월드 분배기가 같은 인스턴스를 써야 한다.</summary>
    public RawSquidForms Forms => _forms;

    /// <summary>
    /// ContentForm 타입 6 과 생성자 가상 표를 확인한다. 오염된 슬롯의 필드 변경을 막는다.
    /// </summary>
    /// <param name="form">확인할 Form 번호.</param>
    private void ValidateForm(int form)
    {
        if (_pool.Slot(form)[TypeOffset] != ContentType)
        {
            throw new InvalidOperationException("ContentForm 타입 오류");
        }
        _forms.Parent(form);
    }

    /// <summary>정렬되지 않은 WORD 를 리틀 엔디언으로 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private ushort Word(int sid, int offset)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>부모·링크·플래그의 이웃 payload 를 보존하면서 WORD 두 바이트만 쓴다.</summary>
    /// <param name="sid">쓸 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private void Put(int sid, int offset, ushort value)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>Form 과 contained 자산은 이전 종속 번호의 자리가 다르다.</summary>
    /// <param name="sid">이전 종속을 읽을 번호.</param>
    private ushort Previous(int sid) =>
        Word(sid, _pool.Slot(sid)[TypeOffset] < OriginalConstructors.FirstAssetType ? PreviousOffset : ParentOffset);

    /// <summary>가상 setPrev 의 실제 write 폭만 지킨다.</summary>
    /// <param name="sid">이전 종속을 받을 번호.</param>
    /// <param name="previous">쓸 이전 종속 번호.</param>
    private void SetPrevious(int sid, ushort previous) =>
        Put(sid, _pool.Slot(sid)[TypeOffset] < OriginalConstructors.FirstAssetType ? PreviousOffset : ParentOffset, previous);

    /// <summary>타입 번호의 레코드를 원본 at 처럼 범위 검사해 돌려준다.</summary>
    /// <param name="type">타입 번호.</param>
    private OriginalTypeRecord TypeAt(int type)
    {
        if (type < 0 || type >= _types.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "타입 번호가 표 범위를 벗어났습니다");
        }
        return _types[type];
    }

    /// <summary>
    /// 원본 004aec10: flags 의 0x300 을 뺀 그룹 안에서 기존 순서 번호보다 앞에 삽입하고 부착 알림을 보낸다.
    /// dead/free 부모·Form 은 슬롯을 바꾸기 전에 거부한다.
    /// </summary>
    /// <param name="form">부착할 ContentForm 번호.</param>
    /// <param name="parent">부모 번호.</param>
    /// <param name="flags">Pop 플래그.</param>
    public void Attach(int form, int parent, uint flags = 0)
    {
        if (form < RawSidPool.FirstOrdinary || parent < RawSidPool.FirstOrdinary || parent >= _pool.Capacity || parent == form)
        {
            throw new InvalidOperationException("ContentForm 부착 번호 오류");
        }
        if (((_pool.Slot(parent)[StateOffset] | _pool.Slot(form)[StateOffset]) & (FreeBit | DeadBit)) != 0)
        {
            throw new InvalidOperationException("ContentForm dead/free 부착 금지");
        }
        ValidateForm(form);
        _forms.ValidatePop(form, flags);
        int stored = Word(form, ContentTypeOffset);
        if (stored >= _types.Length)
        {
            throw new InvalidOperationException("ContentForm 생산 타입 오류");
        }
        if (_state is not null && _state.InBattle && (TypeAt(stored).Flags2 & 0x200000) != 0
            && (TypeAt(_pool.Slot(parent)[TypeOffset]).Flags2 & 0x30000) != 0 && _captured is null)
        {
            throw new InvalidOperationException("ContentForm 전투 사제 포획 알림 경계 필요");
        }
        ushort listFlags = Word(form, FlagsOffset);
        uint order = 0;
        int previous = 0;
        int current = Word(parent, HeadOffset);
        bool inserted = false;
        var visited = new HashSet<int>();
        // 일치하는 그룹만 순서 카운터를 늘리며 모든 항목은 꼬리 후보로 기억한다
        while (current != 0)
        {
            if (current >= _pool.Capacity || current == form || !visited.Add(current)
                || (_pool.Slot(current)[StateOffset] & FreeBit) != 0)
            {
                throw new InvalidOperationException("ContentForm 부착 체인 손상");
            }
            ReadOnlySpan<byte> raw = _pool.Slot(current);
            if (raw[TypeOffset] == ContentType && ((Word(current, FlagsOffset) ^ listFlags) & GroupMask) == 0)
            {
                if (order < raw[OrderOffset])
                {
                    Put(form, NextOffset, (ushort)current);
                    SetPrevious(form, Previous(current));
                    SetPrevious(current, (ushort)form);
                    int before = Previous(form);
                    if (before != 0)
                    {
                        Put(before, NextOffset, (ushort)form);
                    }
                    else
                    {
                        Put(parent, HeadOffset, (ushort)form);
                    }
                    inserted = true;
                    break;
                }
                order++;
            }
            previous = current;
            current = Word(current, NextOffset);
        }
        if (!inserted)
        {
            if (previous != 0)
            {
                Put(previous, NextOffset, (ushort)form);
            }
            else
            {
                Put(parent, HeadOffset, (ushort)form);
            }
            Put(form, NextOffset, 0);
            SetPrevious(form, (ushort)previous);
        }
        _pool.AllocatedBytes(form)[OrderOffset] = (byte)order;
        Put(form, ParentOffset, (ushort)parent);
        _pool.AllocatedBytes(form)[StateOffset] |= ContainedBit;
        // 0x100 은 원본처럼 부착 중의 서버 전송을 건너뛴다
        _forms.Pop(form, flags | 0x100);
        _notice(parent, form, 1);
        // 부모 알림·전송이 바꾼 현재 내용물 타입·부모 genus·전투 상태를 다시 읽어 포획 알림 조건을 본다
        if (_state is not null && _state.InBattle && (TypeAt(Word(form, ContentTypeOffset)).Flags2 & 0x200000) != 0
            && (TypeAt(_pool.Slot(parent)[TypeOffset]).Flags2 & 0x30000) != 0)
        {
            if (_state.CheckingLinks && !Linked(form))
            {
                throw new InvalidOperationException("ContentForm 사제 포획 알림 연결 오류");
            }
            if (Linked(form))
            {
                if (_captured is null)
                {
                    throw new InvalidOperationException("ContentForm 전투 사제 포획 알림 경계 필요");
                }
                _captured(parent, (uint)(_pool.Slot(form)[12] & 0x7f));
            }
        }
    }

    /// <summary>
    /// 기존 contained 생성기의 타입6 생성과 부착 경계를 같은 저장소로 연결한다(원본 ContentFormHost::BindCreate).
    /// 생성 콜백은 실제 팩토리의 contained 전용 생성자를 쓰고, 부착 콜백은 이 몸체의 정렬 부착기를 쓴다.
    /// 그래야 생성기는 슬롯 필드만 채우고 부모 알림·목록 순서는 ContentForm 규칙을 따른다.
    /// 추가: W03 삭제·수명 열다섯째 조각(2026-10-23). 근거: cpppj/src/o/ContentFormHost.cpp BindCreate.
    /// </summary>
    /// <param name="factory">내장 Form 생성기. 같은 풀이어야 한다.</param>
    /// <param name="hooks">표면 통지 등 나머지 경계를 채운 묶음.</param>
    /// <returns>부착만 이 몸체로 바꾼 contained 생성 경계.</returns>
    public ContainedCreateHooks BindCreate(RawSquidFactory factory, ContainedCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(hooks);
        // 실제 팩토리 연결은 같은 풀 only·타입 6 only 를 생성 시점 계약으로 삼는다
        ContainedCreateHooks bound = RawContainedCreate.ConnectFactory(_pool, factory, hooks);
        bound.Attach = (form, parent, flags) => Attach(form, parent, flags);
        return bound;
    }

    /// <summary>
    /// 공통 Form Unpop 을 수행한다. 부모 dead/free 판정과 알림 시점은 공통 몸체가 정한다.
    /// </summary>
    /// <param name="form">해제할 ContentForm 번호.</param>
    public void Unpop(int form)
    {
        ValidateForm(form);
        _forms.Unpop(form);
    }

    /// <summary>
    /// 가상 +0x60: WORLD(0x80) 목록은 원본 assert 경로라 거부하고, 0x34 중 하나라도 있으면 참이다.
    /// </summary>
    /// <param name="form">검사할 ContentForm 번호.</param>
    public bool Robust(int form)
    {
        ValidateForm(form);
        ushort flags = Word(form, FlagsOffset);
        if ((flags & WorldFlag) != 0)
        {
            throw new InvalidOperationException("WORLD ContentForm Robust 금지");
        }
        return (flags & RobustFlags) != 0;
    }

    /// <summary>
    /// 원본 004ae080: 부모 머리에서 모든 next 를 따라가 자기 번호 포함 여부를 검사한다.
    /// 상태·타입 필터를 더하지 않아 뒤의 무효 next 에서도 원본처럼 false 로 끝난다.
    /// </summary>
    /// <param name="form">찾을 ContentForm 번호.</param>
    public bool Linked(int form)
    {
        ValidateForm(form);
        int parent = Word(form, ParentOffset);
        if (parent == 0 || parent >= _pool.Capacity)
        {
            throw new InvalidOperationException("ContentForm 연결 검사 부모 번호 오류");
        }
        int current = Word(parent, HeadOffset);
        bool found = false;
        var visited = new HashSet<int>();
        // 부모 체인의 실제 next 만 따라간다
        while (current != 0)
        {
            if (current >= _pool.Capacity)
            {
                return false;
            }
            if (!visited.Add(current))
            {
                throw new InvalidOperationException("ContentForm 연결 검사 체인 순환");
            }
            if (current == form)
            {
                found = true;
            }
            current = Word(current, NextOffset);
        }
        return found;
    }

    /// <summary>
    /// 가상 +0x5c: 양수 피해는 Unpop → 순서 변경 표시 해제 → 0x10 로컬 재부착 뒤 0 을 돌려준다.
    /// 0·음수 피해는 피해량을 그대로 돌려주며 아무것도 바꾸지 않는다.
    /// </summary>
    /// <param name="form">피해를 받는 ContentForm 번호.</param>
    /// <param name="damage">피해량.</param>
    /// <param name="amount">돌려줄 값(0 이하 피해일 때).</param>
    public float Damage(int form, int damage, float amount)
    {
        ValidateForm(form);
        if (damage <= 0)
        {
            return amount;
        }
        int parent = _forms.Parent(form);
        Unpop(form);
        Put(form, FlagsOffset, (ushort)(Word(form, FlagsOffset) & ~ChangedFlag));
        Attach(form, parent, 0x10);
        return 0;
    }
}
