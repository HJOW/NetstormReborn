// 패킷 등록 표와 세션 대여의 사전 구현을 공유한다.
// 근거: cpppj `src/o/PacketRegistrationTable.h`·`PacketRegistrationTable.cpp`, `src/o/RawPacketLengths.h`,
//   `src/o/RawPacketDispatch.h`(등록·수신·연결 종류 선언만).
// 분배기/연구 등록은 같은 Net 모듈을 사용하며 실제 지연 FIFO와 구성 자료 연결은 후속이다.
// 2026-11-11 사전 구현을 바탕으로 0.687에서 공유 값 snapshot과 결정적 콜백 반납을 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Net;

// 패킷 등록 범주를 담는 네임스페이스다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.

// 타입별 패킷 길이 표다. 0xffff는 길이 검사를 하지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
internal static class PacketLengths
{
    /// <summary>10.78 타입 0~137 길이 138개다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    internal static readonly ushort[] Patch = [
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0x0008, 0x000c, 0x000c, 0xffff, 0xffff, 0xffff, 0xffff, 0x000e,
        0xffff, 0xffff, 0xffff, 0x0026, 0xffff, 0x000d, 0x000e, 0x000c,
        0xffff, 0xffff, 0x0011, 0x0014, 0x0005, 0x0008, 0xffff, 0xffff,
        0x0012, 0x0022, 0xffff, 0xffff, 0x0023, 0x0012, 0x0008, 0x000f,
        0xffff, 0xffff, 0x0020, 0x0012, 0x000c, 0x0014, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0009, 0x0019, 0xffff,
        0x0012, 0xffff, 0x000c, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0x0007, 0x0006, 0xffff, 0x000a, 0xffff, 0x0009,
        0x0009, 0x0008, 0x0005, 0x0006, 0x0008, 0xffff, 0xffff, 0xffff,
        0x0010, 0xffff, 0x0005, 0x0006, 0x0006, 0xffff, 0xffff, 0xffff,
        0x000a, 0x000b, 0xffff, 0x0008, 0x0005, 0x000d, 0x000c, 0xffff,
        0x0005, 0xffff, 0xffff, 0x000a, 0x000b, 0x0008, 0x0008, 0x0006,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0006, 0x0006,
        0x0006, 0x0006, 0xffff, 0xffff, 0x001c, 0xffff, 0x001c, 0xffff,
        0x0008, 0xffff, 0xffff, 0x00b0, 0x0008, 0xffff, 0x0014, 0x0112,
        0x0062, 0x010b,
    ];

    /// <summary>CD 타입 0~126 길이 127개다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    internal static readonly ushort[] Cd = [
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0x0008, 0x000c, 0x000c, 0xffff, 0xffff, 0xffff, 0xffff, 0x000e,
        0xffff, 0xffff, 0xffff, 0x0026, 0xffff, 0x000d, 0x000e, 0x000c,
        0xffff, 0xffff, 0x0011, 0x0014, 0x0005, 0x0008, 0xffff, 0xffff,
        0x0012, 0x0022, 0xffff, 0xffff, 0x0018, 0x0012, 0x0008, 0x000f,
        0xffff, 0xffff, 0x0020, 0x0012, 0x000c, 0x0014, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0009, 0x0019, 0xffff,
        0x0012, 0xffff, 0x000c, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0x0007, 0x0006, 0xffff, 0x000a, 0xffff, 0x0009,
        0x0008, 0x0008, 0x0005, 0x0006, 0x0008, 0xffff, 0xffff, 0xffff,
        0x0010, 0xffff, 0x0005, 0x0006, 0x0006, 0xffff, 0xffff, 0xffff,
        0x0008, 0x000b, 0xffff, 0x0008, 0x0005, 0x000d, 0x000c, 0xffff,
        0x0005, 0xffff, 0xffff, 0x000a, 0x000b, 0x0006, 0x0008, 0x0006,
        0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0x0006, 0x0006,
        0x0006, 0x0006, 0xffff, 0xffff, 0x001c, 0xffff, 0x001c,
    ];
}

/// <summary>원문 단일 연결에서 읽고 쓰는 필드와 현재 지연 FIFO 경계다.</summary>
public sealed class PacketConnection
{
    /// <summary>+34의 현재 연결 비트다.40은 닫힘이다.</summary>
    public uint Flags { get; set; }
    /// <summary>+40의 현재 송신자다.</summary>
    public uint Sender { get; set; }
    /// <summary>+44의 일반 수신 시각이다.</summary>
    public uint Received { get; set; }
    /// <summary>+48의6번 별도 수신 시각이다.</summary>
    public uint Heartbeat { get; set; }
    /// <summary>+8c의 현재 콜백 중첩 깊이다.</summary>
    public uint Depth { get; set; }
    /// <summary>실제 지연 큐 설치 표시다.큐가 없는데 선택되면 명시 실패한다.</summary>
    public bool Delayed { get; set; }
    /// <summary>+4c의 같은 현재 주소20바이트다.</summary>
    public byte[] Address { get; } = new byte[20];
    /// <summary>별도 원문 지연 FIFO의 실제 enqueue다.내부 미션에는 임의 설치하지 않는다.</summary>
    public PacketDelayEnqueue? DelayQueue { get; set; }
}

/// <summary>선택한 고정 F와 같은 연결·원문 버퍼를 실제 FIFO에 넘긴다.</summary>
public delegate int PacketDelayEnqueue(PacketConnection connection, CanonicalPacketReceiver? receiver,
    RawPacketDispatch dispatch, ReadOnlyMemory<byte> packet, int delay);

/// <summary>원문 함수 정체성과 장기 본문만 보관하며 현재 타입으로 다른 F를 다시 선택하지 않는다.</summary>
public sealed class CanonicalPacketReceiver
{
    /// <summary>함수 주소 정체성 표식이며 역참조하지 않는다.</summary>
    public uint NativeEntry { get; }
    /// <summary>같은 고정 본문과 장기 데이터다.</summary>
    public Action<uint, PacketConnection?, ReadOnlyMemory<byte>> Receive { get; }
    /// <summary>실제 비영 주소 정체성과 비NULL 본문만 받는다.</summary>
    public CanonicalPacketReceiver(uint nativeEntry, Action<uint, PacketConnection?, ReadOnlyMemory<byte>> receive)
    {
        ArgumentNullException.ThrowIfNull(receive);
        if (nativeEntry == 0) throw new ArgumentOutOfRangeException(nameof(nativeEntry));
        NativeEntry = nativeEntry; Receive = receive;
    }
    /// <summary>검증/시각/기록/깊이를 추가하지 않고 저장된 F만 호출한다.</summary>
    public void Invoke(uint sender, PacketConnection? connection, ReadOnlyMemory<byte> packet) => Receive(sender, connection, packet);
}

/// <summary>등록 한 칸의 함수/정책 값이다.값 복사로 snapshot하고 공개 칸의 주소는 유지한다.</summary>
internal readonly record struct PacketRegistrationData
{
    /// <summary>등록 세대의 약한 실제 대여다.</summary>
    public WeakReference<PacketRegistrationLease>? Lease { get; init; }
    /// <summary>세션 대여 등록인지 구별한다.</summary>
    public bool Leased { get; init; }
    /// <summary>실제 연결이 있는 일반 수신 입구다.</summary>
    public Action<uint, PacketConnection, ReadOnlyMemory<byte>>? Receive { get; init; }
    /// <summary>원문의 nullable 연결 직접 입구다.</summary>
    public Action<uint, PacketConnection?, ReadOnlyMemory<byte>>? Direct { get; init; }
    /// <summary>지연 배송이 보관할 검토된 장기 F다.</summary>
    public CanonicalPacketReceiver? Canonical { get; init; }
    /// <summary>방향만 사용하는 기존 관찰기다.</summary>
    public Action<uint>? Observer { get; init; }
    /// <summary>같은 실제 mutable 패킷을 받는 관찰기다.</summary>
    public Action<Memory<byte>, uint>? PacketObserver { get; init; }
    /// <summary>선언 WORD 비교 길이다.ffff는 가변이다.</summary>
    public ushort Length { get; init; }
    /// <summary>등록된 프로토콜 BYTE다.</summary>
    public byte Version { get; init; }
    /// <summary>높은 버전4/1·낮은 버전8/2의 종료/무시 정책이다.</summary>
    public byte Mismatch { get; init; }
}

/// <summary>실제 공유 표의 안정된 한 칸이다.세대 복원도 이 객체의 값만 바꾼다.</summary>
public sealed class PacketRegistration
{
    /// <summary>호스트 대리자/정책의 원자적 값 복사다.새 슬롯은 가변 길이다.</summary>
    internal PacketRegistrationData Data { get; set; } = new() { Length = ushort.MaxValue };
    /// <summary>기존 Net 등록 소비자의 같은 약한 세대 속성이다.</summary>
    public WeakReference<PacketRegistrationLease>? RegistrationLease { get => Data.Lease; set => Data = Data with { Lease = value }; }
    /// <summary>기존 Net 등록 소비자의 같은 대여 비트다.</summary>
    public bool Leased { get => Data.Leased; set => Data = Data with { Leased = value }; }
    /// <summary>실제 일반 수신 본문이다.</summary>
    public Action<uint, PacketConnection, ReadOnlyMemory<byte>>? Receive { get => Data.Receive; set => Data = Data with { Receive = value }; }
    /// <summary>nullable 연결을 지원하는 직접 수신 본문이다.</summary>
    public Action<uint, PacketConnection?, ReadOnlyMemory<byte>>? DirectReceive { get => Data.Direct; set => Data = Data with { Direct = value }; }
    /// <summary>현재 장기 고정 F다.</summary>
    public CanonicalPacketReceiver? CanonicalReceive { get => Data.Canonical; set => Data = Data with { Canonical = value }; }
    /// <summary>현재 방향 전용 관찰기다.</summary>
    public Action<uint>? Observer { get => Data.Observer; set => Data = Data with { Observer = value }; }
    /// <summary>현재 패킷 관찰기다.</summary>
    public Action<Memory<byte>, uint>? PacketObserver { get => Data.PacketObserver; set => Data = Data with { PacketObserver = value }; }
    /// <summary>선언 WORD 비교 길이다.</summary>
    public ushort Length { get => Data.Length; set => Data = Data with { Length = value }; }
    /// <summary>현재 프로토콜 BYTE다.</summary>
    public byte Version { get => Data.Version; set => Data = Data with { Version = value }; }
    /// <summary>현재 불일치 정책 BYTE다.</summary>
    public byte Mismatch { get => Data.Mismatch; set => Data = Data with { Mismatch = value }; }
    /// <summary>현재 대여 등록이면 실제 세션 전체를 콜백 복귀까지 강하게 빌린다.</summary>
    public IDisposable? Pin()
    {
        PacketRegistrationData data = Data;
        if (!data.Leased) return null;
        if (data.Lease is null || !data.Lease.TryGetTarget(out PacketRegistrationLease? lease))
            throw new InvalidOperationException("이미 반납된 패킷 등록 세대의 콜백");
        return lease.Pin();
    }
    /// <summary>한 종류의 관찰기만 선택하고 자기 재등록 중에도 이미 읽은 함수를 유지한다.</summary>
    public void NotifyObserver(Memory<byte> packet, uint sending)
    {
        using IDisposable? pin = Pin(); Action<Memory<byte>, uint>? selected = Data.PacketObserver;
        if (selected is not null) selected(packet, sending);
        else { Action<uint>? direction = Data.Observer; direction?.Invoke(sending); }
    }
    /// <summary>원문의 값 복사 관찰 항목으로 같은 세대/대리자만 보존한다.</summary>
    internal PacketRegistration Snapshot() => new() { Data = Data };
}

/// <summary>cpppj PacketRegistrationTable의 실제 공유 표와 세대별 부분 복원이다.</summary>
public sealed class PacketRegistrationTable
{
    /// <summary>호스트256칸은 고정하며 원문 접근은0..LastType만 허용한다.</summary>
    private readonly PacketRegistration[] _entries = new PacketRegistration[256];
    /// <summary>현재 활성 세대의 약한 실제 대여다.</summary>
    private WeakReference<PacketRegistrationLease>? _current;
    /// <summary>반납했지만 콜백이 복귀하지 않은 실제 대여다.</summary>
    private WeakReference<PacketRegistrationLease>? _retiring;
    /// <summary>주소 비교를 대신하는 현재 세대 번호다.수명 owner의 대용이 아니다.</summary>
    private long _currentIdentity;
    /// <summary>같은 표 안에서만 전진하는 세대 번호다.</summary>
    private long _nextIdentity;
    /// <summary>PATCH137/CD126의 실제 마지막 타입이다.</summary>
    private readonly uint _lastType;
    /// <summary>다른 backend가 공유 표의 판본을 검증하는 실제 마지막 타입이다.</summary>
    public uint LastType() => _lastType;
    /// <summary>판본별 길이만 처음 복사하고 함수/버전은0으로 둔다.</summary>
    public PacketRegistrationTable(OriginalEdition edition)
    {
        _lastType = edition == OriginalEdition.Patch1078 ? 137u : 126u;
        ReadOnlySpan<ushort> lengths = edition == OriginalEdition.Patch1078 ? PacketLengths.Patch : PacketLengths.Cd;
        // 물리 슬롯은 한 번 만들고 실제 길이는 판본 한도까지만 복사한다.
        for (int index = 0; index < _entries.Length; index++)
            _entries[index] = new() { Data = new() { Length = index <= _lastType ? lengths[index] : ushort.MaxValue } };
    }
    /// <summary>0번 NULL 관찰 슬롯을 허용하며 호스트 물리폭으로 Native 범위를 늘리지 않는다.</summary>
    public PacketRegistration Registration(uint type) => type <= _lastType ? _entries[type]
        : throw new ArgumentOutOfRangeException(nameof(type), "패킷 표의 원문 할당 범위 밖");
    /// <summary>실제 활성/반납 세대의 콜백 실행 수를 함께 확인한다.</summary>
    public bool ConnectionCallbackActive() => Calls(_current) || Calls(_retiring);
    /// <summary>약한 대여가 살아 있고 실제 콜백 수가 비영인지 확인한다.</summary>
    private static bool Calls(WeakReference<PacketRegistrationLease>? source) => source is not null && source.TryGetTarget(out PacketRegistrationLease? lease) && lease.Calls != 0;
    /// <summary>길이표와 공개 슬롯 주소를 유지하며 함수/버전만 초기화한다.</summary>
    public void ResetCallbacks()
    {
        if (_currentIdentity != 0 || ConnectionCallbackActive()) throw new InvalidOperationException("세션 대여/콜백 실행 중 패킷 표 초기화 금지");
        // 원문 last+1개 슬롯의 길이는 보존한다.
        for (uint type = 0; type <= _lastType; type++) _entries[type].Data = new() { Length = _entries[type].Length };
    }
    /// <summary>snapshot 확보 뒤에만 새 세대를 공개하고 실제 세션 전체 owner를 약하게 빌린다.</summary>
    public PacketRegistrationLease BindSession(object owner)
    {
        ArgumentNullException.ThrowIfNull(owner);
        if (_currentIdentity != 0 || Calls(_retiring)) throw new InvalidOperationException("패킷 세션 중복 대여 또는 이전 콜백 미복귀");
        var lease = new PacketRegistrationLease(this, owner, checked(_nextIdentity + 1), _entries.Select(entry => entry.Data).ToArray());
        _nextIdentity = lease.Identity; _current = new(lease, true); _currentIdentity = lease.Identity; return lease;
    }
    /// <summary>등록 API가 실제로 바꾼 슬롯과 같은 세대 pin만 기록한다.</summary>
    internal void Touch(uint type)
    {
        PacketRegistration entry = Registration(type);
        if (_current is not null && _current.TryGetTarget(out PacketRegistrationLease? current) && current.Active)
        {
            current.Touched[type] = true; entry.Data = entry.Data with { Lease = _current, Leased = true }; return;
        }
        if (_currentIdentity != 0 || Calls(_retiring)) throw new InvalidOperationException("반납 중 또는 반납 콜백 안의 패킷 표 수정 금지");
        entry.Data = entry.Data with { Lease = null, Leased = false };
    }
    /// <summary>장기 고정 F의 실제 현재 정책을 세션 복원 snapshot에도 보존한다.</summary>
    internal void PersistCanonicalBaseline(uint type)
    {
        PacketRegistration entry = Registration(type);
        if (entry.CanonicalReceive is null) throw new InvalidOperationException("고정 F 없는 패킷 baseline 승격 거부");
        PacketRegistrationLease? current = null; _current?.TryGetTarget(out current);
        if ((_currentIdentity != 0 && (current is null || !current.Active)) || (current is null && ConnectionCallbackActive()))
            throw new InvalidOperationException("반납 중 패킷 baseline 변경 금지");
        PacketRegistrationData selected = entry.Data with { Lease = null, Leased = false };
        if (current is not null) { current.Previous[type] = selected; current.Touched[type] = true; }
        entry.Data = selected;
    }
    /// <summary>자기 세대만 할당 없는 값 교환으로 복원하며 실행 중 반납 세대는 따로 추적한다.</summary>
    internal void Detach(PacketRegistrationLease lease)
    {
        if (_currentIdentity != lease.Identity) return;
        // 바꾸지 않은 슬롯과 다른 전역 표는 보존한다.
        for (uint type = 1; type <= _lastType; type++)
        {
            if (!lease.Touched[type]) continue;
            PacketRegistrationData current = _entries[type].Data; _entries[type].Data = lease.Previous[type]; lease.Previous[type] = current;
        }
        _retiring = _current; _current = null; _currentIdentity = 0;
    }
}

/// <summary>실제 세션 owner의 약한 대여와 실행 중 강한 pin을 관리한다.사용 후 Dispose한다.</summary>
public sealed class PacketRegistrationLease : IDisposable
{
    /// <summary>backend보다 먼저 사라지지 않는 실제 공유 표다.</summary>
    private readonly PacketRegistrationTable _table;
    /// <summary>등록만으로 실제 세션을 붙잡지 않는 약한 owner다.</summary>
    private readonly WeakReference<object> _owner;
    /// <summary>자기 세대 비교용 번호다.</summary>
    internal long Identity { get; }
    /// <summary>대여 시작 때의 모든 값이다.반납은 touched만 복원한다.</summary>
    internal PacketRegistrationData[] Previous { get; }
    /// <summary>실제로 등록 API가 변경한 칸이다.</summary>
    internal bool[] Touched { get; } = new bool[256];
    /// <summary>최외곽까지 유지되는 현재 실제 콜백 수다.</summary>
    internal int Calls { get; private set; }
    /// <summary>반납된 대여의 콜백 재호출을 거부한다.</summary>
    public bool Active { get; private set; }
    /// <summary>표가 준비한 snapshot과 실제 owner만 받는다.</summary>
    internal PacketRegistrationLease(PacketRegistrationTable table, object owner, long identity, PacketRegistrationData[] previous)
    { _table = table; _owner = new(owner); Identity = identity; Previous = previous; Active = true; }
    /// <summary>명시 반납을 놓친 관리 객체도 자기 세대만 정리한다.</summary>
    ~PacketRegistrationLease() => Detach();
    /// <summary>함수 실행 없이 자기 세대의 변경 슬롯만 한 번 복원한다.</summary>
    public void Detach() { if (!Active) return; Active = false; _table.Detach(this); }
    /// <summary>실제 owner가 살아 있을 때만 콜백 실행 수와 두 강한 참조를 확보한다.</summary>
    public IDisposable Pin()
    {
        if (!Active || !_owner.TryGetTarget(out object? owner)) throw new InvalidOperationException("반납됐거나 실제 세션보다 오래 남은 패킷 등록");
        return new CallbackPin(this, owner);
    }
    /// <summary>반납 뒤 finalizer 재호출을 생략한다.</summary>
    public void Dispose() { Detach(); GC.SuppressFinalize(this); }
    /// <summary>실제 세션과 대여를 함께 유지하고 owner 반납 전에 실행 수를 내린다.</summary>
    private sealed class CallbackPin : IDisposable
    {
        /// <summary>현재 실행 중인 같은 대여다.</summary>
        private PacketRegistrationLease? _lease;
        /// <summary>실행이 끝날 때까지 필요한 실제 세션 전체다.</summary>
        private object? _owner;
        /// <summary>두 참조를 먼저 보관한 뒤 실행 수를 올린다.</summary>
        public CallbackPin(PacketRegistrationLease lease, object owner) { _lease = lease; _owner = owner; lease.Calls++; }
        /// <summary>중복 반납을 허용하고 owner를 놓기 전에 실행 수를 내린다.</summary>
        public void Dispose()
        {
            if (_lease is not { } lease) return;
            lease.Calls--; _lease = null; GC.KeepAlive(_owner); _owner = null;
        }
    }
}
