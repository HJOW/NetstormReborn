using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 Squid.cpp 의 내장 Form 네 종류(타입 1·2·3·6) 생성자, 지정 슬롯 Take, 부모 종속 체인 부착,
/// 공통 Pop/Unpop 을 같은 raw 슬롯 풀에 옮긴 몸체 (004af530·004af610·004ac560·004ae140, CD 004ab390·004ab440·004abfb0·004af270).
/// 부모 제거를 요청하는 ContentForm 은 <c>notice</c> 경계로, 서버 Pop 전송은 <c>transmit</c> 경계로 알린다.
/// 두 경계가 없으면 원본이 요구하는 시점에 예외를 내고 슬롯을 바꾸지 않는다.
/// 이 몸체는 상태 변경 전에 모든 계약을 검사하므로 잘못된 입력에서도 풀이 부분 변경되지 않는다.
/// 기준 판본은 10.78 뿐이다(CD 10.72 는 가상 표 값과 Pop/Unpop 의 분기 순서가 달라 옮기지 않았다).
/// 근거: cpppj/src/o/SquidForms.cpp (W03 삭제·수명 열한째 조각, 2026-10-22).
/// </summary>
public sealed class RawSquidForms
{
    /// <summary>지원하는 내장 Form 타입 번호 네 개. 이 밖의 생성자는 자산 factory 의 책임이다.</summary>
    private static readonly uint[] Types = [1, 2, 3, 6];

    /// <summary>10.78 생성자가 슬롯 +0 에 기록하는 가상 표 값(타입 1·2·3·6 순서).</summary>
    private static readonly uint[] PatchVtables = [0x512110, 0x512098, 0x512188, 0x512208];

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

    /// <summary>contained 자산이 이전 종속 번호를 두는 위치. Form 과 자리가 겹치지 않게 구분한다.</summary>
    private const int AssetPreviousOffset = 16;

    /// <summary>상태 바이트의 빈 슬롯 비트.</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 반납(죽음) 비트.</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 월드 밖(void) 비트. Pop 뒤에 켜지고 Pop 전에 꺼진다.</summary>
    private const byte VoidBit = 4;

    /// <summary>상태 바이트의 부모 종속(contained) 비트.</summary>
    private const byte ContainedBit = 8;

    /// <summary>ContentForm 타입 번호. 부모에서 빠질 때 부모에 알린다.</summary>
    private const uint ContentForm = 6;

    /// <summary>ProcessForm 타입 번호. 프로세스 타입 번호가 이 생성자의 가상 표를 쓴다.</summary>
    private const uint ProcessForm = 2;

    /// <summary>프로세스 타입 번호의 첫 값. 이 번호부터 ProcessForm 가상 표를 확인한다.</summary>
    private const uint ProcessFirst = 10;

    /// <summary>10.78 프로세스 타입 번호의 끝(포함하지 않음).</summary>
    private const uint PatchProcessEnd = 63;

    /// <summary>일반 client 번호의 첫 번호. 0~4 번은 일반 할당 대상이 아니다.</summary>
    private const int FirstOrdinary = 5;

    /// <summary>Pop 플래그의 client 비트. 서버 Pop 에서 클라이언트 쪽 Form 을 표시한다.</summary>
    private const uint ClientFlag = 8;

    /// <summary>Pop 플래그의 서버 비트. 이미 있으면 앞의 서버 전송을 건너뛴다.</summary>
    private const uint ServerFlag = 0x10;

    /// <summary>Pop 플래그의 전송 묶음 비트(0x40|0x10). 원본이 서버 전송 여부를 정할 때 본다.</summary>
    private const uint TransmitMask = 0x50;

    /// <summary>Pop 플래그의 임시 서버 표시 값. 전송 경계가 이 값을 보고 전송 여부를 정한다.</summary>
    private const byte TransmittingBit = 0x10;

    /// <summary>Form 슬롯을 읽고 쓰는 풀. 다른 모듈과 같은 raw 바이트를 공유해야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>부모에서 빠진 ContentForm 을 부모에 알리는 경계. 없으면 그 시점에 예외다.</summary>
    private readonly Action<int, int>? _notice;

    /// <summary>서버 Pop 을 데이터 쓰기·송신에 넘기는 경계. 없으면 서버 전송이 필요한 Pop 은 예외다.</summary>
    private readonly Action<int, uint>? _transmit;

    /// <summary>
    /// Form 수신 몸체를 만든다. 풀은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">Form 슬롯을 가진 풀.</param>
    /// <param name="notice">ContentForm 부모 제거 알림(부모, Form). 없으면 그 경로에서 예외다.</param>
    /// <param name="transmit">서버 Pop 전송 경계(Form, 플래그). 없으면 서버 전송 경로에서 예외다.</param>
    public RawSquidForms(RawSidPool pool, Action<int, int>? notice = null, Action<int, uint>? transmit = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        _pool = pool;
        _notice = notice;
        _transmit = transmit;
    }

    /// <summary>다른 모듈이 같은 풀을 쓰는지 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 타입의 생성자가 기록하는 10.78 가상 표 값을 돌려준다. 지원하지 않는 타입은 슬롯을 바꾸기 전에 예외다.
    /// </summary>
    /// <param name="type">내장 Form 타입 번호.</param>
    private static uint Vtable(uint type)
    {
        // 고정된 네 타입만 이 몸체가 만들 수 있다
        int index = Array.IndexOf(Types, type);
        if (index < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "미복원 내장 Form 생성자 타입");
        }
        return PatchVtables[index];
    }

    /// <summary>
    /// 원본의 정렬되지 않은 WORD/DWORD 를 낮은 바이트부터 읽는다.
    /// </summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    /// <param name="width">읽을 바이트 수.</param>
    private uint Read(int sid, int offset, int width)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint value = 0;
        // 필드 폭만 읽고 주변 payload 는 건드리지 않는다
        for (int index = 0; index < width; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 지정한 폭만큼만 리틀 엔디언으로 써서 주변 payload 를 보존한다. 빈 슬롯에도 쓸 수 있는 원본 store 다.
    /// </summary>
    /// <param name="sid">쓸 슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓸 바이트 수.</param>
    private void Put(int sid, int offset, uint value, int width)
    {
        Span<byte> raw = _pool.RawSlot(sid);
        // 지정한 write 폭을 그대로 지킨다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// Form 슬롯의 현재 타입과 가상 표를 확인한다. dead 는 공통 삭제 중에도 허용하고 free·범위 밖은 거부한다.
    /// 프로세스가 자기 타입 번호로 바꾼 Form 도 생성자 가상 표(ProcessForm)로 확인한다.
    /// </summary>
    /// <param name="sid">확인할 Form 번호.</param>
    /// <returns>확인한 슬롯 바이트.</returns>
    private ReadOnlySpan<byte> Form(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint type = raw[TypeOffset];
        if (sid < FirstOrdinary || (raw[StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("Form 슬롯 상태 오류");
        }
        uint constructorType = type >= ProcessFirst && type < PatchProcessEnd ? ProcessForm : type;
        if (Read(sid, 0, 4) != Vtable(constructorType))
        {
            throw new InvalidOperationException("Form 가상 표 미복원");
        }
        return raw;
    }

    /// <summary>
    /// 원본 Form 부모 getter 와 같이 +16 WORD 전체를 돌려준다.
    /// </summary>
    /// <param name="form">부모를 읽을 Form 번호.</param>
    public int Parent(int form)
    {
        Form(form);
        return (int)(ushort)Read(form, ParentOffset, 2);
    }

    /// <summary>
    /// 원본 Form 이전 getter 와 같이 +14 WORD 전체를 돌려준다.
    /// </summary>
    /// <param name="form">이전 종속을 읽을 Form 번호.</param>
    public int Previous(int form)
    {
        Form(form);
        return (int)(ushort)Read(form, PreviousOffset, 2);
    }

    /// <summary>
    /// 타입의 Form 을 새로 만든다. Allocate 가 지운 슬롯에 생성자 가상 표와 타입만 쓰며 자산 초기화는 하지 않는다.
    /// </summary>
    /// <param name="type">내장 Form 타입 번호(1·2·3·6).</param>
    /// <param name="flags">할당 영역 플래그. 예측 비트가 client 비트보다 우선한다.</param>
    /// <returns>할당한 Form 번호.</returns>
    public int Create(uint type, SidAllocation flags = SidAllocation.Server)
    {
        uint vtable = Vtable(type);
        int sid = _pool.Allocate(flags);
        Put(sid, 0, vtable, 4);
        _pool.RawSlot(sid)[TypeOffset] = (byte)type;
        return sid;
    }

    /// <summary>
    /// 지정한 서버·예측 슬롯을 같은 타입의 Form 으로 다시 쓴다. 살아 있는 슬롯은 같은 타입일 때만 공통 Unpop 을 먼저 수행한다.
    /// </summary>
    /// <param name="type">만들 타입 번호.</param>
    /// <param name="sid">지정 슬롯 번호. 서버 영역 번호만 받는다.</param>
    /// <returns>같은 슬롯 번호.</returns>
    public int Take(uint type, int sid)
    {
        uint vtable = Vtable(type);
        if (sid < RawSidPool.ServerFirst)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "Form Take 는 서버 영역 SID 만 받습니다");
        }
        byte state = _pool.RawSlot(sid)[StateOffset];
        if ((state & FreeBit) == 0)
        {
            if (_pool.RawSlot(sid)[TypeOffset] != type)
            {
                throw new InvalidOperationException("Form Take 타입 불일치");
            }
            Unpop(sid);
        }
        // 해제 알림이 바꾼 현재 슬롯을 다시 읽어 free/dead 만 끄고 나머지 상태를 보존한다
        Span<byte> raw = _pool.RawSlot(sid);
        raw[StateOffset] &= unchecked((byte)~(FreeBit | DeadBit));
        Put(sid, 0, vtable, 4);
        raw[TypeOffset] = (byte)type;
        raw[StateOffset] |= VoidBit;
        return sid;
    }

    /// <summary>
    /// 부착·Pop 전에 현재 상태와 전송 경계를 확인하고 원본이 보완하는 플래그를 돌려준다.
    /// void 가 아니거나 dead·0x10 상태이면 슬롯을 바꾸기 전에 예외다.
    /// </summary>
    /// <param name="form">확인할 Form 번호.</param>
    /// <param name="flags">호출자가 준 Pop 플래그.</param>
    /// <returns>권한에 맞게 보완한 플래그.</returns>
    public uint ValidatePop(int form, uint flags)
    {
        byte state = Form(form)[StateOffset];
        if ((state & VoidBit) == 0 || (state & (DeadBit | 0x10)) != 0)
        {
            throw new InvalidOperationException("Form Pop 상태 오류");
        }
        if (_pool.IsServer)
        {
            if (form < RawSidPool.ServerFirst)
            {
                if ((flags & ClientFlag) != 0)
                {
                    throw new InvalidOperationException("client Form 서버 Pop 금지");
                }
                flags |= ServerFlag;
            }
            else if ((flags & ServerFlag) == 0)
            {
                flags |= ClientFlag;
            }
            if ((flags & TransmitMask) == 0 && _transmit is null)
            {
                throw new InvalidOperationException("Form 서버 Pop 전송 경계 필요");
            }
        }
        else if ((flags & ClientFlag) == 0)
        {
            flags |= ServerFlag;
        }
        return flags;
    }

    /// <summary>
    /// 원본 004ac560 의 순서를 지켜 void 를 먼저 끄고, 필요한 서버 전송을 한 번 부른 뒤 임시 비트를 되돌린다.
    /// </summary>
    /// <param name="form">Pop 할 Form 번호.</param>
    /// <param name="flags">Pop 플래그.</param>
    public void Pop(int form, uint flags)
    {
        flags = ValidatePop(form, flags);
        _pool.RawSlot(form)[StateOffset] &= unchecked((byte)~VoidBit);
        if (_pool.IsServer && (flags & TransmitMask) == 0)
        {
            if ((flags & 7) != 0)
            {
                _pool.RawSlot(form)[StateOffset] |= TransmittingBit;
            }
            _transmit!(form, flags);
            _pool.RawSlot(form)[StateOffset] &= unchecked((byte)~TransmittingBit);
        }
    }

    /// <summary>
    /// 기본 Form 을 부모의 첫 종속 항목으로 부착하고 공통 Pop 을 호출한다. ContentForm 은 정렬 부착기를 써야 한다.
    /// 부모 0 은 원본 10.78 처럼 아무것도 바꾸지 않고 돌아온다.
    /// </summary>
    /// <param name="form">부착할 Form 번호.</param>
    /// <param name="parent">부모 번호.</param>
    /// <param name="flags">Pop 플래그.</param>
    public void Attach(int form, int parent, uint flags = 0)
    {
        if (parent == 0)
        {
            // 10.78 은 부모 0 부착을 조용히 넘긴다
            return;
        }
        ReadOnlySpan<byte> raw = Form(form);
        if (raw[TypeOffset] == ContentForm)
        {
            throw new InvalidOperationException("ContentForm 정렬 부착기 필요");
        }
        if (parent < FirstOrdinary || parent >= _pool.Capacity || parent == form
            || (_pool.Slot(parent)[StateOffset] & (FreeBit | DeadBit)) != 0)
        {
            throw new InvalidOperationException("Form 부착 부모 상태 오류");
        }
        ValidatePop(form, flags);
        int head = (int)(ushort)Read(parent, HeadOffset, 2);
        if (head == form || head >= _pool.Capacity || (head != 0 && (_pool.Slot(head)[StateOffset] & FreeBit) != 0))
        {
            throw new InvalidOperationException("Form 부착 종속 머리 오류");
        }
        Put(form, ParentOffset, (uint)parent, 2);
        Put(form, NextOffset, (uint)head, 2);
        if (head != 0)
        {
            SetPrevious(head, form);
        }
        Put(parent, HeadOffset, (uint)form, 2);
        Put(form, PreviousOffset, 0, 2);
        _pool.RawSlot(form)[StateOffset] |= ContainedBit;
        // 0x100 플래그는 원본처럼 부착 뒤의 서버 전송을 건너뛴다
        Pop(form, flags | 0x100);
    }

    /// <summary>
    /// 가상 setPrev 는 이웃의 현재 타입에 따라 Form(+14)과 contained 자산(+16)의 서로 다른 자리를 쓴다.
    /// </summary>
    /// <param name="sid">이전 종속 번호를 받을 이웃 번호.</param>
    /// <param name="previous">쓸 이전 종속 번호.</param>
    private void SetPrevious(int sid, int previous)
    {
        int offset = _pool.Slot(sid)[TypeOffset] < OriginalConstructors.FirstAssetType ? PreviousOffset : AssetPreviousOffset;
        Put(sid, offset, (uint)previous, 2);
    }

    /// <summary>
    /// 공통 Form Unpop. void 를 켜고, 부모가 free 면 링크를 보존한 채 돌아가며, dead 면 체인을 건드리지 않고 끝낸다.
    /// 살아 있는 부모에서 빠진 ContentForm 만 부모 제거 알림을 보낸다.
    /// </summary>
    /// <param name="form">해제할 Form 번호.</param>
    public void Unpop(int form)
    {
        ReadOnlySpan<byte> raw = Form(form);
        if ((raw[StateOffset] & VoidBit) != 0)
        {
            throw new InvalidOperationException("void Form Unpop 금지");
        }
        int parent = Parent(form);
        if (parent == 0 || parent >= _pool.Capacity)
        {
            throw new InvalidOperationException("Form 부모 번호 오류");
        }
        byte parentState = _pool.Slot(parent)[StateOffset];
        bool notify = raw[TypeOffset] == ContentForm && (parentState & (FreeBit | DeadBit)) == 0;
        if (notify && _notice is null)
        {
            throw new InvalidOperationException("ContentForm 부모 제거 알림 미연결");
        }
        _pool.RawSlot(form)[StateOffset] |= VoidBit;
        if ((parentState & FreeBit) != 0)
        {
            return;
        }
        if ((_pool.RawSlot(form)[StateOffset] & ContainedBit) == 0)
        {
            throw new InvalidOperationException("Form contained 누락");
        }
        if ((parentState & DeadBit) != 0)
        {
            return;
        }
        int next = (int)(ushort)Read(form, NextOffset, 2);
        int previous = Previous(form);
        // 손상 번호는 배열 밖 쓰기 전에 거부한다
        foreach (int neighbor in new[] { next, previous })
        {
            if (neighbor != 0 && (neighbor < FirstOrdinary || neighbor >= _pool.Capacity || neighbor == form
                || (_pool.Slot(neighbor)[StateOffset] & FreeBit) != 0))
            {
                throw new InvalidOperationException("Form 이웃 링크 오류");
            }
        }
        if (previous != 0 && previous == next)
        {
            throw new InvalidOperationException("Form 종속 체인 순환");
        }
        if (previous != 0)
        {
            Put(previous, NextOffset, (uint)next, 2);
        }
        if (next != 0)
        {
            SetPrevious(next, previous);
        }
        if (Read(parent, HeadOffset, 2) == (uint)form)
        {
            if (previous != 0)
            {
                throw new InvalidOperationException("Form 머리 이전 항목 오류");
            }
            Put(parent, HeadOffset, (uint)next, 2);
        }
        _pool.RawSlot(form)[StateOffset] &= unchecked((byte)~ContainedBit);
        if (notify)
        {
            _notice!(parent, form);
        }
    }
}
