using System.Buffers.Binary;

// 현재 Content Form의 정렬·필드·재부착 몸체를 담는다.
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
    public bool InBattle { get => InBattleFlag; set => InBattleFlag.Set(value); }

    /// <summary>같은 현재 전투 DWORD를 빌리는 뷰다. Native 연결은 Bind로 실제 owner를 지정한다.</summary>
    public RawCheckingFlag InBattleFlag { get; } = new();

    /// <summary>연결 검사를 하는 중인지. 켜져 있으면 포획 알림 전에 실제 체인 연결을 다시 확인한다.</summary>
    public bool CheckingLinks { get => CheckingLinksFlag; set => CheckingLinksFlag.Set(value); }

    /// <summary>두 진단 위치에서 각각 읽는 같은 현재 checking DWORD 뷰다.</summary>
    public RawCheckingFlag CheckingLinksFlag { get; } = new();

    /// <summary>원문 식·파일·행을 전달하는 복귀 가능한 진단 장치다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>정렬 순회에서 읽는 전체 현재 Content 타입 DWORD다. 독립 입력은6을 사용한다.</summary>
    public Func<uint>? ContentType { get; set; }
}

/// <summary>
/// 생산 목록 ContentForm(타입 6)의 정렬 부착·재부착·부모 알림을 원본 raw 슬롯 풀에 옮긴 몸체
/// (원본 004aec10 ↔ CD 004afff0, 004ae080 ↔ CD 004af1c0, 004ae080 의 +0x5c).
/// 일반 Form 과 달리 같은 목록 그룹(플래그 0xfcff) 안에서 순서 번호(+23)가 맞는 자리에 들어간다.
/// PATCH의 현재 정렬 부모와 CD의 기존 정렬 계약을 구분하며 현재 TYPE·전역·가상 재부착 장치를 선택한다.
/// 근거: cpppj/src/o/ContentFormHost.cpp.
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

    /// <summary>실제 World가 빌려 준 현재 TYPE 표다. 미연결 또는 빈 표이면 독립 입력 사본을 읽는다.</summary>
    private IReadOnlyList<OriginalTypeRecord>? _currentTypes;
    /// <summary>정렬 링크 뒤에 도달하는 실제 공통 Pop이다.</summary>
    private Action<int, uint>? _pop;
    /// <summary>현재 공장의 공통 Form 해제다.</summary>
    private Action<int>? _unpop;
    /// <summary>분리 또는 생성 뒤 마지막 현재 가상 부착을 선택하는 장치다.</summary>
    private Action<int, int, uint>? _reattach;

    /// <summary>
    /// ContentForm 처리기를 만든다. 풀·타입 표·알림 경계는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">ContentForm 슬롯을 가진 풀.</param>
    /// <param name="types">독립 사본을 만들 판본별 TYPE 표(PATCH188·CD171)다.</param>
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
        int expected = pool.Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (types.Count != expected)
        {
            throw new ArgumentException($"ContentForm 타입 판본/크기 오류: {types.Count}/{expected}", nameof(types));
        }
        _pool = pool;
        _types = types.Select(type => type.Clone()).ToArray();
        _notice = notice;
        _forms = new RawSquidForms(pool, (parent, form) => notice(parent, form, 2), transmit);
        _state = state;
        _captured = captured;
        if (state is not null) _forms.BindUnpop(new() { Assertion = state.Assertion });
    }

    /// <summary>다른 모듈이 같은 풀을 쓰는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>공통 Form 몸체. 전체 월드 분배기가 같은 인스턴스를 써야 한다.</summary>
    public RawSquidForms Forms => _forms;

    /// <summary>선택된 원문 진단이 정상 복귀하면 현재 슬롯·전역 판정을 계속한다.</summary>
    private void Assert(string expression, uint line) =>
        (_state?.Assertion ?? throw new InvalidOperationException("ContentForm 실제 원문 진단 장치 미연결"))(expression, @"\Ns\O\Squid.cpp", line);

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
        unchecked((ushort)RawContainedLink.CommonContainedPrevious(_pool, sid));

    /// <summary>가상 setPrev 의 실제 write 폭만 지킨다.</summary>
    /// <param name="sid">이전 종속을 받을 번호.</param>
    /// <param name="previous">쓸 이전 종속 번호.</param>
    private void SetPrevious(int sid, ushort previous) =>
        RawContainedLink.CommonContainedSetPrevious(_pool, sid, previous);

    /// <summary>통지 뒤 실제 현재 TYPE를 소비 위치에서 읽으며 독립 입력은 사본의 genus를 사용한다.</summary>
    /// <param name="type">타입 번호.</param>
    private uint TypeGenus(uint type)
    {
        IReadOnlyList<OriginalTypeRecord> current = _currentTypes is { Count: > 0 } ? _currentTypes : _types;
        if (type >= current.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "Content 통지 뒤 현재 TYPE 물리 범위");
        }
        return current[(int)type].Flags2;
    }

    /// <summary>
    /// 원본 004aec10: flags 의 0x300 을 뺀 그룹 안에서 기존 순서 번호보다 앞에 삽입하고 부착 알림을 보낸다.
    /// PATCH는 dead/free 진단의 정상 복귀 뒤 현재 상태로 재판정하고, CD의 같은 입력은 조용히 반환한다.
    /// </summary>
    /// <param name="form">부착할 ContentForm 번호.</param>
    /// <param name="parent">부모 번호.</param>
    /// <param name="flags">Pop 플래그.</param>
    public void Attach(int form, int parent, uint flags = 0)
    {
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            var hooks = new ContentAttachHooks
            {
                ContentType = () => { var getter = _state?.ContentType; return getter is null ? 6u : getter(); },
                Previous = (sid, table) => RawContainedLink.CommonContainedPreviousAtTable(_pool, sid, table),
                SetPrevious = (sid, table, previous) => RawContainedLink.CommonContainedSetPreviousAtTable(_pool, sid, table, previous),
                SetParent = (sid, _, owner) => RawContainedLink.CommonContainedSetParent(_pool, sid, owner),
                Pop = (sid, value) => { if (_pop is { } selected) selected(sid, value); else _forms.Pop(sid, value); },
                Notice = (owner, sid, reason) => _notice(owner, sid, reason),
                Assertion = _state?.Assertion,
            };
            if (!RawContentAttach.Attach(_pool, form, parent, flags, hooks)) return;
        }
        else
        {
            // CD의 기존 정렬 계약은 PATCH 복귀 진단 부모와 구분한다.
            if (form < 5 || parent < 5 || parent >= _pool.Capacity || parent == form)
                throw new InvalidOperationException("ContentForm 부착 번호 오류");
            if (((_pool.Slot(parent)[StateOffset] | _pool.Slot(form)[StateOffset]) & (FreeBit | DeadBit)) != 0) return;
            _forms.Parent(form); _forms.ValidatePop(form, flags);
            if (Word(form, ContentTypeOffset) >= _types.Length) throw new InvalidOperationException("ContentForm 생산 타입 오류");
            ushort listFlags = Word(form, FlagsOffset);
            uint order = 0;
            int previous = 0, current = Word(parent, HeadOffset);
            bool inserted = false;
            var visited = new HashSet<int>();
            // 일치하는 그룹만 순서 카운터를 늘리고 모든 항목을 tail 후보로 기억한다.
            while (current != 0)
            {
                if (current >= _pool.Capacity || current == form || !visited.Add(current) || (_pool.Slot(current)[StateOffset] & FreeBit) != 0)
                    throw new InvalidOperationException("ContentForm 부착 체인 손상");
                ReadOnlySpan<byte> raw = _pool.Slot(current);
                if (raw[TypeOffset] == ContentType && ((Word(current, FlagsOffset) ^ listFlags) & GroupMask) == 0)
                {
                    if (order < raw[22])
                    {
                        Put(form, NextOffset, (ushort)current); SetPrevious(form, Previous(current)); SetPrevious(current, (ushort)form);
                        int before = Previous(form);
                        if (before != 0) Put(before, NextOffset, (ushort)form); else Put(parent, HeadOffset, (ushort)form);
                        inserted = true; break;
                    }
                    order++;
                }
                previous = current; current = Word(current, NextOffset);
            }
            if (!inserted)
            {
                if (previous != 0) Put(previous, NextOffset, (ushort)form); else Put(parent, HeadOffset, (ushort)form);
                Put(form, NextOffset, 0); SetPrevious(form, (ushort)previous);
            }
            _pool.AllocatedBytes(form)[22] = (byte)order;
            Put(form, ParentOffset, (ushort)parent); _pool.AllocatedBytes(form)[StateOffset] |= ContainedBit;
            if (_pop is { } pop) pop(form, flags | 0x100); else _forms.Pop(form, flags | 0x100);
            _notice(parent, form, 1);
        }
        // 통지 뒤 현재 내용물 TYPE·부모 genus를 먼저 읽고 그 다음 현재 전투 상태를 판정한다.
        if ((TypeGenus(Word(form, ContentTypeOffset)) & 0x200000) != 0
            && (TypeGenus(_pool.Slot(parent)[TypeOffset]) & 0x30000) != 0 && _state is not null && _state.InBattle)
        {
            if (_state.CheckingLinks)
            {
                if (form < 1 || form >= _pool.Capacity) Assert("isValid()", 0xc2b);
                if (_state.CheckingLinks && !Linked(form)) Assert("isRobust()", 0xc2c);
            }
            if (form >= 1 && form < _pool.Capacity && Linked(form))
                (_captured ?? throw new InvalidOperationException("ContentForm 전투 사제 포획 알림 경계 필요"))(parent, (uint)(_pool.Slot(form)[12] & 0x7f));
        }
    }

    /// <summary>
    /// 기존 contained 생성기의 타입6 생성과 부착 경계를 같은 저장소로 연결한다(원본 ContentFormHost::BindCreate).
    /// 생성 콜백은 실제 팩토리의 contained 생성자를 쓰고, 마지막 부착 위치에서 현재 장치·가상표를 선택한다.
    /// 생성/통지가 바꾼 클래스의 실제 부착을 요구하고 미연결을 다른 클래스의 성공으로 바꾸지 않는다.
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
        bound.Attach = AttachCurrent;
        return bound;
    }

    /// <summary>
    /// 공통 Form Unpop 을 수행한다. 부모 dead/free 판정과 알림 시점은 공통 몸체가 정한다.
    /// </summary>
    /// <param name="form">해제할 ContentForm 번호.</param>
    public void Unpop(int form)
    {
        if (_unpop is { } selected) selected(form); else _forms.Unpop(form);
    }

    /// <summary>정렬 뒤 같은 World의 현재 실제 Pop을 연결하거나 반납한다.</summary>
    public void BindPop(Action<int, uint>? pop) => _pop = pop;

    /// <summary>현재 공장의 공통 해제를 연결하거나 독립 기본 종단으로 돌린다.</summary>
    public void BindUnpop(Action<int>? unpop) => _unpop = unpop;

    /// <summary>Unpop 또는 생성 필드 쓰기 뒤 도달하는 현재 가상 부착을 연결한다.</summary>
    public void BindReattach(Action<int, int, uint>? reattach) => _reattach = reattach;

    /// <summary>같은 판본 길이의 실제 TYPE 표를 빌린다. 빈 표는 독립 사본으로 돌아간다.</summary>
    public void BindCurrentTypes(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count != 0 && types.Count != _types.Length) throw new ArgumentException("Content 현재 TYPE 표의 판본/길이 계약", nameof(types));
        _currentTypes = types;
    }

    /// <summary>WORLD 진단이 복귀하면 TYPE/상태 필터 없이 현재 목록 flags의0x34를 읽는다.</summary>
    public bool Robust(int form)
    {
        if ((Word(form, FlagsOffset) & WorldFlag) != 0)
        {
            var assertion = _state?.Assertion ?? throw new InvalidOperationException("ContentForm Robust의 선택된 원문 진단 장치 미연결");
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            assertion("!(getListFlags() & CFF_WORLD)", patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", patch ? 0xbb1u : 0xb68u);
        }
        return (Word(form, FlagsOffset) & RobustFlags) != 0;
    }

    /// <summary>현재 가상 부모 getter와 공통 전체 체인 검사를 사용한다. 부모0도 실제 물리 head를 읽는다.</summary>
    public bool Linked(int form) => RawFormLinked.Check(_pool, form, sid => RawContainedLink.CommonContainedParent(_pool, sid));

    /// <summary>분리 또는 생성 뒤 현재 장치/가상표를 읽고 선택한 부착 함수를 복귀까지 보유한다.</summary>
    public void AttachCurrent(int form, int parent, uint flags)
    {
        if (_reattach is { } selected) { selected(form, parent, flags); return; }
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(form));
        if (table != _forms.Vtable(6)) throw new InvalidOperationException("ContentForm 현재 가상+50 부착 장치 미연결");
        Attach(form, parent, flags);
    }

    /// <summary>양수 count만 현재 부모 getter→Unpop→flags 쓰기→현재 가상 재부착을 실행한다. 나머지는 this를 읽지 않는다.</summary>
    public float Damage(int form, int damage, float amount)
    {
        if (damage <= 0) return amount;
        int parent = RawContainedLink.CommonContainedParent(_pool, form);
        Unpop(form);
        if (_pool.Edition == OriginalEdition.Patch1078) _pool.RawSlot(form)[FlagsOffset + 1] &= 0xfe;
        else BinaryPrimitives.WriteUInt16LittleEndian(_pool.RawSlot(form)[FlagsOffset..], (ushort)(Word(form, FlagsOffset) & ~ChangedFlag));
        AttachCurrent(form, parent, 0x10);
        return 0;
    }
}
