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
/// 기준 판본은 10.78 뿐이다(CD 는 생성 권한·주소가 달라서 옮기지 않았다).
/// 근거: cpppj/src/o/DependFormHost.cpp (W03 삭제·수명 열한째 조각, 2026-10-22).
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
    }

    /// <summary>다른 모듈이 같은 풀을 쓰는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 반납 뒤에도 타입 바이트가 보존되므로 free 필터는 호출자가 적용한다. 원본의 타입 1 판정만 한다.
    /// </summary>
    /// <param name="form">확인할 번호.</param>
    public bool IsDependForm(int form) =>
        form >= RawSidPool.FirstOrdinary && form < _pool.Capacity && form != RawSidPool.PredictableFirst
        && _pool.Slot(form)[TypeOffset] == DependType;

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
        if (watcher == watched || _pool.Slot(watcher)[TypeOffset] == DependType
            || _pool.Slot(watched)[TypeOffset] < OriginalConstructors.FirstAssetType)
        {
            throw new InvalidOperationException("DependForm 감시 대상 타입 오류");
        }
    }

    /// <summary>
    /// 원본 004b1510: watcher 쪽 Form 을 먼저 만들어 각 부모에 부착한다. 어느 한 부모가 client 번호면 두 Form 모두 client 영역에 만든다.
    /// </summary>
    /// <param name="watched">감시 대상 부모 번호.</param>
    /// <param name="watcher">감시하는 쪽 부모 번호.</param>
    /// <returns>두 Form 의 방향.</returns>
    public DependFormPair Create(int watched, int watcher)
    {
        // 생성은 제거 helper 의 타입 제한을 더하지 않는다. 같은 부모 안의 두 Form 도 원본처럼 지원한다
        foreach (int parent in new[] { watcher, watched })
        {
            if (parent < RawSidPool.FirstOrdinary || parent >= _pool.Capacity || parent == RawSidPool.PredictableFirst
                || (_pool.Slot(parent)[StateOffset] & (FreeBit | DeadBit)) != 0)
            {
                throw new InvalidOperationException("DependForm 생성 부모 상태 오류");
            }
        }
        bool client = watcher < RawSidPool.ServerFirst || watched < RawSidPool.ServerFirst;
        if (!client && !_pool.IsServer)
        {
            throw new InvalidOperationException("DependForm 서버 쌍 생성 권한 필요");
        }
        SidAllocation allocation = client ? SidAllocation.Client : SidAllocation.Server;
        int first = _forms.Create(DependType, allocation);
        int second = _forms.Create(DependType, allocation);
        Put(first, WatchedOffset, second);
        Put(first, WatcherOffset, 0);
        Put(second, WatcherOffset, first);
        Put(second, WatchedOffset, 0);
        _forms.Attach(first, watcher);
        _forms.Attach(second, watched);
        return new DependFormPair(second, first);
    }

    /// <summary>
    /// +0x12 watcherForm 을 통해 상대 부모를 읽는다. 번호가 0 이면 부모 0 을 돌려준다.
    /// ProcessForm 의 원래 타입도 공통 Form 부모 getter 를 통과한다.
    /// </summary>
    /// <param name="form">감시 Form 번호.</param>
    public int Watcher(int form)
    {
        ValidateForm(form);
        int watcher = Word(form, WatcherOffset);
        return watcher != 0 ? _forms.Parent(watcher) : 0;
    }

    /// <summary>
    /// 원본 004ae900: watched 의 Form 체인에서 watcher 부모가 일치하는 첫 감시 Form 을 찾는다.
    /// free/dead/void 필터를 더하지 않고 현재 Form 타입과 watcher 번호만 비교한다.
    /// </summary>
    /// <param name="watched">감시 대상 부모 번호.</param>
    /// <param name="watcher">감시하는 쪽 부모 번호.</param>
    public bool Has(int watched, int watcher)
    {
        if (watched == 0 || watched >= _pool.Capacity)
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
            if (_pool.Slot(current)[TypeOffset] == DependType && Watcher(current) == watcher)
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
        if (watched == 0 || watched >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(watched), watched, "DependForm 제거 부모 번호 오류");
        }
        if ((_pool.Slot(watched)[StateOffset] & DeadBit) != 0)
        {
            return;
        }
        Parents(watcher, watched);
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
            if (_pool.Slot(current)[TypeOffset] == DependType && Watcher(current) == watcher)
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
        ValidateForm(form);
        int watcher = Word(form, WatcherOffset);
        if (watcher == 0)
        {
            return;
        }
        int parent = _forms.Parent(watcher);
        if (watcher < RawSidPool.ServerFirst || _pool.IsServer)
        {
            if (_notice is null)
            {
                throw new InvalidOperationException("DependForm watcher 통지 경계 필요");
            }
            _notice(parent, watched, form);
        }
        if (Word(form, WatchedOffset) != 0)
        {
            throw new InvalidOperationException("DependForm 통지 뒤 watchedFid 오류");
        }
    }

    /// <summary>
    /// 가상 +0x10: 공통 삭제 본체를 같은 훅으로 실행한다. 상대 Form 삭제·자신 Unpop·번호 반납을 공유한다.
    /// </summary>
    /// <param name="form">삭제할 감시 Form 번호.</param>
    /// <param name="flags">삭제 플래그.</param>
    public void Destroy(int form, uint flags = 0)
    {
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
        UnpopForm = sid =>
        {
            if (IsDependForm(sid))
            {
                // Form 공통 Unpop 은 부모 dead 의 링크를 보존한다
                _forms.Unpop(sid);
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
