using System.Buffers.Binary;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// watched 부모가 삭제될 때 watcher 부모의 가상 +0x34(watched, form)를 호출하는 경계(원본 DependFormNotice).
/// </summary>
/// <param name="watcher">감시하는 쪽 부모 번호.</param>
/// <param name="watched">감시 대상 부모 번호.</param>
/// <param name="form">알림을 보내는 감시 Form 번호.</param>
public delegate void DependFormNotice(int watcher, int watched, int form);

/// <summary>
/// 감시 쌍 두 Form 의 방향. <paramref name="WatchedForm"/> 은 watched 안에, <paramref name="WatcherForm"/> 은 watcher 안에 붙는다.
/// </summary>
/// <param name="WatchedForm">감시 대상 부모에 붙은 Form 번호.</param>
/// <param name="WatcherForm">감시하는 부모에 붙은 Form 번호.</param>
public readonly record struct DependFormPair(int WatchedForm, int WatcherForm);

/// <summary>
/// 원본 Squid.cpp 의 감시 쌍(타입 1 DependForm) 생성·조회·제거와 부모 삭제 통지를 같은 raw 슬롯 풀·공통 삭제 처리기에 옮긴 몸체
/// (원본 004b1510·004ae900·004ae780 ↔ CD 004af940·004afbc0·004afa50).
/// 두 Form 이 서로를 참조하므로 삭제 재진입에서 상대의 dead 비트를 먼저 확인하고, 상대 링크는 효과가 끝난 뒤 다시 읽는다.
/// 다른 타입의 Form·자산 분배는 fallback 훅으로 넘긴다.
/// 생성은 양판본 현재 공장을 사용하며 Native 가상 삭제·검색 진단은 명시 바인딩한 경우에만 선택한다.
/// 근거: cpppj/src/o/DependFormHost.cpp와 RawDependCreate.cpp.
/// </summary>
public sealed class RawDependFormHost
{
    /// <summary>슬롯 안 다음 종속 번호(16비트)의 위치.</summary>
    private const int NextOffset = 4;

    /// <summary>슬롯 안 종속 머리(16비트)의 위치.</summary>
    private const int HeadOffset = 6;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    private const int StateOffset = 11;

    /// <summary>감시 Form 이 가리키는 반대편 Form 번호(16비트)의 위치.</summary>
    private const int WatcherOffset = 18;

    /// <summary>감시 Form 이 가리키는 감시 대상 Form 번호(16비트)의 위치.</summary>
    private const int WatchedOffset = 20;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 반납(죽음) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>내장 감시 Form 타입 번호.</summary>
    private const byte DependType = 1;

    /// <summary>감시 Form 슬롯을 공유하는 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>공통 삭제 본체. 같은 풀을 쓰는 인스턴스만 연결할 수 있다.</summary>
    private readonly RawSquidDestroy _destroy;

    /// <summary>watcher 부모에 감시 대상 삭제를 알리는 경계. 없으면 그 경로에서 예외다.</summary>
    private readonly DependFormNotice? _notice;

    /// <summary>이 몸체가 처리하지 않는 타입의 기존 분배 훅.</summary>
    private readonly SquidDestroyHooks _fallback;

    /// <summary>공통 Form 생성·부착·Unpop 몸체.</summary>
    private readonly RawSquidForms _forms;

    /// <summary>현재 공장 부착이 사용하는 실제 전송 장치다.</summary>
    private readonly Action<int, uint>? _transmit;
    /// <summary>같은 풀의 현재 공장이다. 없으면 동일 TYPE 입력의 독립 공장을 사용한다.</summary>
    private RawSquidFactory? _factory;
    /// <summary>두 Create와 검색 위치에서 읽는 현재 감시 타입 DWORD다.</summary>
    private UIntBox? _dependType;
    /// <summary>생성/제거의 정상 복귀 원문 진단 장치다.</summary>
    private Action<string, string, uint>? _creationAssertion;
    /// <summary>같은 실제 공장으로 가는 공통 Form 해제다.</summary>
    private Action<int>? _unpop;
    /// <summary>Native의 현재 가상 삭제 장치다. 다른 클래스의 삭제를 고정 감시 처리기로 바꾸지 않는다.</summary>
    private Action<int, uint>? _virtualDestroy;
    /// <summary>Native FormFinder를 시작하기 직전의 현재 부모 진단이다.</summary>
    private Action<int>? _beforeFind;

    /// <summary>
    /// 감시 쌍 처리기를 만든다. 풀·삭제 처리기는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">감시 Form 슬롯을 가진 풀.</param>
    /// <param name="destroy">같은 풀의 공통 삭제 본체.</param>
    /// <param name="notice">watcher 부모 통지.</param>
    /// <param name="fallback">다른 타입의 삭제 분배 훅.</param>
    /// <param name="transmit">서버 Pop 전송 경계.</param>
    public RawDependFormHost(RawSidPool pool, RawSquidDestroy destroy, DependFormNotice? notice = null,
        SquidDestroyHooks? fallback = null, Action<int, uint>? transmit = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(destroy);
        if (!ReferenceEquals(destroy.Pool, pool))
        {
            throw new ArgumentException("DependForm 삭제 SID 풀 불일치", nameof(destroy));
        }
        _pool = pool;
        _destroy = destroy;
        _notice = notice;
        _fallback = fallback ?? new SquidDestroyHooks();
        _forms = new RawSquidForms(pool, null, transmit);
        _transmit = transmit;
    }

    /// <summary>공통 해제에서 현재 공장의 진단·분리 순서를 사용하도록 연결하거나 반납한다.</summary>
    public void BindUnpop(Action<int>? unpop) => _unpop = unpop;

    /// <summary>현재 Native 가상 삭제와 부모 검색 진단을 연결한다. 연결 시 게임 상태를 읽거나 수정하지 않는다.</summary>
    public void BindVirtualDestroy(Action<int, uint> destroy, Action<int> beforeFind)
    {
        ArgumentNullException.ThrowIfNull(destroy); ArgumentNullException.ThrowIfNull(beforeFind);
        _virtualDestroy = destroy; _beforeFind = beforeFind;
    }

    /// <summary>동일 풀의 현재 공장과 장기 타입 DWORD를 빌린다. 각 Create 직전에 번호와 생성자를 다시 읽는다.</summary>
    public void BindCreation(RawSquidFactory factory, UIntBox type, Action<string, string, uint>? assertion = null)
    {
        ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(type);
        if (!ReferenceEquals(factory.Pool, _pool)) throw new ArgumentException("Depend 생성 공장의 실제 SID 풀 불일치", nameof(factory));
        _factory = factory; _dependType = type; _creationAssertion = assertion;
    }

    /// <summary>현재 감시 타입을 BYTE로 줄이지 않고 전체 DWORD로 읽는다.</summary>
    private uint CurrentDependType() => _dependType?.Value ?? DependType;

    /// <summary>다른 모듈이 같은 풀을 쓰는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 반납 뒤에도 가상표가 보존되므로 free 필터는 호출자가 적용한다. 현재 TYPE와 독립적인 실제 감시 클래스 가상표를 판정한다.
    /// </summary>
    /// <param name="form">확인할 번호.</param>
    public bool IsDependForm(int form) =>
        form >= RawSidPool.FirstOrdinary && form < _pool.Capacity && form != RawSidPool.PredictableFirst
        && BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(form)) == _forms.Vtable(DependType);

    /// <summary>
    /// 살아 있거나 삭제 중인 감시 Form 의 현재 raw 상태를 확인한다. free 슬롯의 가상 필드는 읽지 않는다.
    /// </summary>
    /// <param name="form">확인할 Form 번호.</param>
    private void ValidateForm(int form)
    {
        if (!IsDependForm(form) || (_pool.Slot(form)[StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("DependForm 슬롯 상태 오류");
        }
        _forms.Parent(form);
    }

    /// <summary>WORD 전체를 읽어 상위 바이트를 잃지 않는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private int Word(int sid, int offset)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return raw[offset] | (raw[offset + 1] << 8);
    }

    /// <summary>WORD 두 바이트만 써서 기존 상대 payload 를 그대로 둔다.</summary>
    /// <param name="sid">쓸 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    /// <param name="value">쓸 번호.</param>
    private void Put(int sid, int offset, int value)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>
    /// watcher 는 ProcessForm 도 가능하지만 DependForm(타입 1)은 금지이며, watched 는 자산이어야 한다.
    /// </summary>
    /// <param name="watcher">감시하는 쪽 부모 번호.</param>
    /// <param name="watched">감시 대상 부모 번호.</param>
    private void Parents(int watcher, int watched)
    {
        // 두 부모 번호는 예약 영역과 빈 슬롯을 제외한다
        foreach (int parent in new[] { watcher, watched })
        {
            if (parent < RawSidPool.FirstOrdinary || parent >= _pool.Capacity || parent == RawSidPool.PredictableFirst
                || (_pool.Slot(parent)[StateOffset] & FreeBit) != 0)
            {
                throw new InvalidOperationException("DependForm 부모 상태 오류");
            }
        }
        if (watcher == watched || _pool.Slot(watcher)[TypeOffset] == CurrentDependType()
            || _pool.Slot(watched)[TypeOffset] < OriginalConstructors.FirstAssetType)
        {
            throw new InvalidOperationException("DependForm 감시 대상 타입 오류");
        }
    }

    /// <summary>
    /// 현재 공장으로 watcher 쪽 Form을 먼저 만들고 판본별 링크·저장 가상표·부착 순서를 실행한다. 부모 상태 선검사를 추가하지 않는다.
    /// </summary>
    /// <param name="watched">감시 대상 부모 번호.</param>
    /// <param name="watcher">감시하는 쪽 부모 번호.</param>
    /// <returns>두 Form 의 방향.</returns>
    public DependFormPair Create(int watched, int watcher)
    {
        // 객체의 실제 소비 위치에서만 물리 범위를 확인하고 원문 SID WORD로 변환한다.
        int Object(uint token) => token < _pool.Capacity ? unchecked((ushort)token) : throw new ArgumentOutOfRangeException(nameof(token), "Depend 생성의 물리 객체 범위");
        RawSquidFactory factory;
        if (_factory is not null) factory = _factory;
        else
        {
            factory = new RawSquidFactory(_pool, _destroy.Types);
            factory.BindDiagnostics(new() { Types = _destroy.Types, Assertion = _creationAssertion });
        }
        var hooks = new DependCreateHooks
        {
            Server = () => _pool.IsServer,
            FormType = CurrentDependType,
            Assertion = _creationAssertion,
            Create = (type, flags) => unchecked((uint)factory.Create(type, (SidAllocation)flags)),
            Sid = token => token,
            Word = (token, offset, value) => BinaryPrimitives.WriteUInt16LittleEndian(_pool.RawSlot(Object(token))[offset..], value),
            Vtable = token => BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(Object(token))),
            Attach = (token, table, parent, flags) =>
            {
                bool common = false;
                // Process2는 별도 직렬화 접두가 필요하며 실제 공통 thunk 세 개만 허용한다.
                foreach (uint type in new uint[] { 1, 3, 6 }) if (table == _forms.Vtable(type)) common = true;
                if (!common) throw new InvalidOperationException("Depend 생성의 선택된 실제 VT+74 미연결");
                factory.AttachForm(Object(token), Object(parent), flags, _transmit);
            },
        };
        DependCreateObjects pair = RawDependCreate.Create(_pool.Edition, unchecked((uint)watched), unchecked((uint)watcher), hooks);
        return new(Object(pair.Watched), Object(pair.Watcher));
    }

    /// <summary>
    /// +0x12 watcherForm 을 통해 상대 부모를 읽는다. 번호가 0 이면 부모 0 을 돌려준다.
    /// ProcessForm 의 원래 타입도 공통 Form 부모 getter 를 통과한다.
    /// </summary>
    /// <param name="form">감시 Form 번호.</param>
    public int Watcher(int form)
    {
        if (_virtualDestroy is null) ValidateForm(form);
        int watcher = Word(form, WatcherOffset);
        return watcher == 0 ? 0 : _virtualDestroy is not null ? RawContainedLink.CommonContainedParent(_pool, watcher) : _forms.Parent(watcher);
    }

    /// <summary>
    /// 원본 004ae900: watched 의 Form 체인에서 watcher 부모가 일치하는 첫 감시 Form 을 찾는다.
    /// free/dead/void 필터를 더하지 않고 현재 Form 타입과 watcher 번호만 비교한다.
    /// </summary>
    /// <param name="watched">감시 대상 부모 번호.</param>
    /// <param name="watcher">감시하는 쪽 부모 번호.</param>
    public bool Has(int watched, int watcher)
    {
        if (_virtualDestroy is not null) _beforeFind!(watched);
        else if (watched == 0 || watched >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(watched), watched, "DependForm 조회 부모 번호 오류");
        }
        var visited = new HashSet<int>();
        int current = Word(watched, HeadOffset);
        // 후보를 반환하기 전에 next 를 저장하는 원본 FormFinder 규칙을 지킨다
        while (current != 0)
        {
            if (current >= _pool.Capacity || !visited.Add(current))
            {
                throw new InvalidOperationException("DependForm 조회 체인 손상");
            }
            int next = Word(current, NextOffset);
            if ((_virtualDestroy is null || _pool.Slot(current)[TypeOffset] < OriginalConstructors.FirstAssetType)
                && _pool.Slot(current)[TypeOffset] == CurrentDependType() && Watcher(current) == watcher)
            {
                return true;
            }
            current = next;
        }
        return false;
    }

    /// <summary>
    /// 원본 004ae780: watched 가 이미 dead 이면 타입·부모 검사보다 먼저 돌아간다.
    /// 일치하는 모든 감시 쌍을 같은 삭제기로 제거하며, 삭제 콜백이 현재 슬롯을 반납해도 선저장한 next 로 이어간다.
    /// </summary>
    /// <param name="watcher">감시하는 쪽 부모 번호.</param>
    /// <param name="watched">감시 대상 부모 번호.</param>
    /// <param name="flags">삭제 플래그.</param>
    public void Remove(int watcher, int watched, uint flags = 0)
    {
        if (_virtualDestroy is null && (watched == 0 || watched >= _pool.Capacity))
        {
            throw new ArgumentOutOfRangeException(nameof(watched), watched, "DependForm 제거 부모 번호 오류");
        }
        if ((_pool.Slot(watched)[StateOffset] & DeadBit) != 0)
        {
            return;
        }
        if (_virtualDestroy is not null)
        {
            var assertion = _creationAssertion ?? throw new InvalidOperationException("Native Depend 제거의 실제 진단 미연결");
            if (watcher == 0 || watcher >= _pool.Capacity) assertion("watcher.isValid()", @"\Ns\O\Squid.cpp", 0xb48);
            if (watched == 0 || watched >= _pool.Capacity) assertion("watched.isValid()", @"\Ns\O\Squid.cpp", 0xb49);
            if (_pool.Slot(watcher)[TypeOffset] == CurrentDependType()) assertion("watcher.getType() != dependFormType", @"\Ns\O\Squid.cpp", 0xb4b);
            if (_pool.Slot(watched)[TypeOffset] < OriginalConstructors.FirstAssetType) assertion("!watched.isForm()", @"\Ns\O\Squid.cpp", 0xb4c);
            if (watcher == watched) assertion("watcherSid != watchedSid", @"\Ns\O\Squid.cpp", 0xb4e);
            _beforeFind!(watched);
        }
        else Parents(watcher, watched);
        var visited = new HashSet<int>();
        int current = Word(watched, HeadOffset);
        // 삭제 콜백이 현재 슬롯을 반납해도 선저장한 next 로 이어간다
        while (current != 0)
        {
            if (current >= _pool.Capacity || !visited.Add(current))
            {
                throw new InvalidOperationException("DependForm 제거 체인 손상");
            }
            int next = Word(current, NextOffset);
            if ((_virtualDestroy is null || _pool.Slot(current)[TypeOffset] < OriginalConstructors.FirstAssetType)
                && _pool.Slot(current)[TypeOffset] == CurrentDependType() && Watcher(current) == watcher)
            {
                Destroy(current, flags);
            }
            current = next;
        }
    }

    /// <summary>
    /// 가상 +0x1c: client 쌍이거나 서버 권한일 때 watcher 부모에 알린다. 콜백 뒤 현재 watchedFid 가 0 인지 다시 확인한다.
    /// </summary>
    /// <param name="form">알림을 보내는 감시 Form 번호.</param>
    /// <param name="watched">감시 대상 부모 번호.</param>
    public void Notify(int form, int watched)
    {
        if (_virtualDestroy is null) ValidateForm(form);
        int watcher = Word(form, WatcherOffset);
        if (watcher == 0)
        {
            return;
        }
        int parent = _virtualDestroy is not null ? RawContainedLink.CommonContainedParent(_pool, watcher) : _forms.Parent(watcher);
        int firstServer = _pool.Edition == OriginalEdition.Patch1078 ? 15000 : 6000;
        if (watcher < firstServer || _pool.IsServer)
        {
            if (_notice is null)
            {
                throw new InvalidOperationException("DependForm watcher 통지 경계 필요");
            }
            _notice(parent, watched, form);
        }
        if (Word(form, WatchedOffset) != 0)
        {
            if (_virtualDestroy is not null && _creationAssertion is { } assertion) assertion("watchedFid == 0", @"\Ns\O\Squid.cpp", 0xadf);
            else throw new InvalidOperationException("DependForm 통지 뒤 watchedFid 오류");
        }
    }

    /// <summary>
    /// 가상 +0x10: 공통 삭제 본체를 같은 훅으로 실행한다. 상대 Form 삭제·자신 Unpop·번호 반납을 공유한다.
    /// </summary>
    /// <param name="form">삭제할 감시 Form 번호.</param>
    /// <param name="flags">삭제 플래그.</param>
    public void Destroy(int form, uint flags = 0)
    {
        if (_virtualDestroy is { } selected) { selected(form, flags); return; }
        if (!IsDependForm(form))
        {
            throw new InvalidOperationException("DependForm 삭제 타입 오류");
        }
        _destroy.Destroy(form, flags, Hooks());
    }

    /// <summary>
    /// 타입 1 만 처리하고 다른 타입의 기존 분배를 fallback 으로 넘기는 삭제 훅을 만든다.
    /// 상대 삭제도 같은 훅을 쓰므로 두 Form 이 서로를 재삭제해도 dead 에서 즉시 돌아간다.
    /// </summary>
    public SquidDestroyHooks Hooks() => new()
    {
        Selected = _fallback.Selected ?? (() => 0),
        // 전체 DWORD 선택 getter의 함수 사본도 함께 전달한다.
        SelectedRaw = _fallback.SelectedRaw,
        UnpopForm = sid =>
        {
            if (IsDependForm(sid))
            {
                // Form 공통 Unpop 은 부모 dead 의 링크를 보존한다
                if (_unpop is { } unpop) unpop(sid);
                else _forms.Unpop(sid);
            }
            else if (_fallback.UnpopForm is not null)
            {
                _fallback.UnpopForm(sid);
            }
            else
            {
                throw new InvalidOperationException("DependForm 외 Unpop 경계 필요");
            }
        },
        Emit = e =>
        {
            if (IsDependForm(e.Sid))
            {
                switch (e.Effect)
                {
                    case SquidDestroyEffect.PreDestroy:
                        {
                            // watched → 현재 watcher 순서로 다시 읽어 삭제하고 마지막에 공통 깊이를 반납한다
                            int watched = Word(e.Sid, WatchedOffset);
                            if (watched != 0)
                            {
                                Destroy(watched, e.Flags);
                            }
                            int watcher = Word(e.Sid, WatcherOffset);
                            if (watcher != 0)
                            {
                                Destroy(watcher, e.Flags);
                            }
                            _destroy.CompletePreDestroy();
                            return;
                        }
                    case SquidDestroyEffect.PostDestroy:
                        _destroy.CompletePostDestroy();
                        return;
                    case SquidDestroyEffect.ReleaseDependent:
                        Notify(e.Sid, e.Parent);
                        return;
                    case SquidDestroyEffect.DestroyDependent:
                        Destroy(e.Sid, e.Flags);
                        return;
                    default:
                        break;
                }
            }
            if (_fallback.Emit is null)
            {
                throw new InvalidOperationException("DependForm 외 삭제 효과 경계 필요");
            }
            _fallback.Emit(e);
        },
    };
}
