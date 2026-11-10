// 패킷 등록 표와 세션 대여의 사전 구현을 공유한다.
// 근거: cpppj `src/o/PacketRegistrationTable.h`·`PacketRegistrationTable.cpp`, `src/o/RawPacketLengths.h`,
//   `src/o/RawPacketDispatch.h`(등록·수신·연결 종류 선언만).
// 분배기 본체(`RawPacketDispatch`)·지연 큐(`RawPacketDelayQueue`)·구성 자료(`RawConstructionData`) 연결은 3차 로컬 구현 때 잇는다.
// 2026-11-11 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.
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

// 연결 한 개의 플래그·송신자·시각·깊이·주소다. 지연 큐는 3차에서 실제 형으로 바꾼다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class PacketConnection
{
    /// <summary>+0x34 플래그다. bit 0x40은 서버 연결에서만 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Flags { get; set; }

    /// <summary>+0x40 처리자 첫 DWORD 송신자다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Sender { get; set; }

    /// <summary>+0x44 일반 패킷 받은 시각 DWORD다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Received { get; set; }

    /// <summary>+0x48 타입 6 패킷 받은 시각 DWORD다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Heartbeat { get; set; }

    /// <summary>+0x8c 콜백 깊이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint Depth { get; set; }

    /// <summary>지연 큐 배치 여부다. 참이면 송신 때 지연 큐에 넣는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Delayed { get; set; }

    /// <summary>+0x4c..5f 상대 주소 20바이트다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public byte[] Address { get; } = new byte[20];

    /// <summary>+74..88 링 owner다. 3차에서 실제 지연 큐 형으로 바꾼다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public object? DelayQueue { get; set; }
}

// 패킷 수신 함수 F와 원본 주소의 한 쌍이다. NULL이면 분배기 기본 처리를 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class CanonicalPacketReceiver
{
    /// <summary>원본 함수 주소 식별자다. 호스트 함수 포인터 캐스팅용이 아니다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint NativeEntry { get; }

    /// <summary>받을 수신 본체다. 송신자·연결·packet을 그대로 받는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint, PacketConnection?, ReadOnlySpan<byte>> Receive { get; }

    /// <summary>원본 주소와 수신 본체로 한 쌍을 만든다.</summary>
    /// <param name="nativeEntry">원본 함수 주소 식별자다.</param>
    /// <param name="receive">받을 수신 본체다.</param>
    public CanonicalPacketReceiver(uint nativeEntry, Action<uint, PacketConnection?, ReadOnlySpan<byte>> receive)
    {
        ArgumentNullException.ThrowIfNull(receive);
        NativeEntry = nativeEntry;
        Receive = receive;
    }

    /// <summary>수신 본체를 그대로 부른다. 버전·observer·기록·깊이 검증은 분배기 쪽에서 한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="sender">보낸 소유자 번호다.</param>
    /// <param name="connection">부를 연결이다. 없으면 null이다.</param>
    /// <param name="packet">부를 packet이다.</param>
    public void Invoke(uint sender, PacketConnection? connection, ReadOnlySpan<byte> packet) => Receive(sender, connection, packet);
}

// 한 타입의 receiver·observer·길이·버전·대여 상태를 함께 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class PacketRegistration
{
    /// <summary>대여 중인 세션 owner pin이다. 없으면 비어 있다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public WeakReference<PacketRegistrationLease>? RegistrationLease { get; set; }

    /// <summary>세션 대여 여부다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Leased { get; set; }

    /// <summary>처리 본체다. 송신자·연결·읽기 packet을 받는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint, PacketConnection, ReadOnlySpan<byte>>? Receive { get; set; }

    /// <summary>직접 호출 본체다. R 연결 호출이며 NULL 연결을 허용한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint, PacketConnection?, ReadOnlySpan<byte>>? DirectReceive { get; set; }

    /// <summary>표준 F 수신 한 쌍이다. 없으면 일반 분배를 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public CanonicalPacketReceiver? CanonicalReceive { get; set; }

    /// <summary>받은 알림만 보는 observer다. 분배기에서는 부르지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint>? Observer { get; set; }

    /// <summary>가변 packet을 보는 observer다. mutable this를 받는 송신 쪽에서만 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<byte[], uint>? PacketObserver { get; set; }

    /// <summary>선언 길이다. 0xffff면 길이 검사를 하지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public ushort Length { get; set; } = 0xffff;

    /// <summary>등록 버전이다. incoming bit 0x80은 버전을 가린다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public byte Version { get; set; }

    /// <summary>불일치 모드다. 0이면 모두 분배한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public byte Mismatch { get; set; }

    /// <summary>등록된 observer에 packet·송신을 알린다. 대여 pin을 먼저 잡는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="packet">알릴 packet이다. 호출자가 소유한다.</param>
    /// <param name="sending">알릴 송신 구분값이다.</param>
    public void NotifyObserver(byte[] packet, uint sending)
    {
        ArgumentNullException.ThrowIfNull(packet);
        _ = Pin();
        Action<byte[], uint>? withPacket = PacketObserver;
        if (withPacket is not null)
        {
            withPacket(packet, sending);
            return;
        }

        Observer?.Invoke(sending);
    }

    /// <summary>이 등록을 쓴 세션 owner를 pin한다. 대여가 아니면 빈 값을 돌려준다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <returns>세션 owner pin이다.</returns>
    /// <exception cref="InvalidOperationException">이미 반납한 대여·끊긴 owner의 pin이다.</exception>
    public object? Pin()
    {
        if (!Leased)
        {
            return null;
        }

        PacketRegistrationLease? lease = null;
        if (RegistrationLease is not null && !RegistrationLease.TryGetTarget(out lease))
        {
            throw new InvalidOperationException("이미 반납한 패킷 등록의 콜백 pin");
        }

        if (lease is null)
        {
            throw new InvalidOperationException("이미 반납한 패킷 등록의 콜백 pin");
        }

        return lease.Pin();
    }
}

// 256 타입 등록 표와 세션 대여를 함께 둔다. backend now·close 훅은 분배기 쪽에서 받는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class PacketRegistrationTable
{
    /// <summary>같은 256 등록 표다. 0번은 NULL 대여이며 호스트 256개까지만 읽는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly PacketRegistration[] _entries = new PacketRegistration[256];

    /// <summary>판본별 마지막 타입이다. PATCH 137·CD 126이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly uint _lastType;

    /// <summary>활성 대여의 약한 참조다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private WeakReference<PacketRegistrationLease>? _current;

    /// <summary>반납 중 대여의 약한 참조다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private WeakReference<PacketRegistrationLease>? _retiring;

    /// <summary>활성 대여의 같은 인스턴스 비교용이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private PacketRegistrationLease? _currentIdentity;

    /// <summary>판본별 길이 표로 0..마지막 타입 길이를 채우고 나머지는 비워 둔다.</summary>
    /// <param name="edition">쓸 판본이다. 마지막 타입을 가른다.</param>
    public PacketRegistrationTable(OriginalEdition edition)
    {
        _lastType = edition == OriginalEdition.Patch1078 ? 137u : 126u;
        ushort[] lengths = edition == OriginalEdition.Patch1078 ? PacketLengths.Patch : PacketLengths.Cd;
        // 판본별 타입 개수만큼 길이를 두고 나머지는 비운 표를 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (int type = 0; type < _entries.Length; ++type)
        {
            _entries[type] = new PacketRegistration { Length = type <= (int)_lastType ? lengths[type] : (ushort)0xffff };
        }
    }

    /// <summary>지정 타입의 현재 등록을 읽는다. 0..마지막 타입만 받는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="type">읽을 타입 번호다.</param>
    /// <exception cref="ArgumentOutOfRangeException">타입 할당 범위 오류다.</exception>
    public PacketRegistration Registration(uint type)
    {
        if (type > _lastType)
        {
            throw new ArgumentOutOfRangeException(nameof(type), type, "패킷 등록 표 할당 범위 오류");
        }

        return _entries[(int)type];
    }

    /// <summary>판본별 마지막 타입 번호다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <returns>마지막 타입 번호다.</returns>
    public uint LastType() => _lastType;

    /// <summary>0..마지막 타입의 receiver·observer를 비우고 길이·owner 표는 둔다. 대여·콜백 중에는 멈춘다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <exception cref="InvalidOperationException">세션 대여·콜백 중 초기화다.</exception>
    public void ResetCallbacks()
    {
        if (_currentIdentity is not null || ConnectionCallbackActive())
        {
            throw new InvalidOperationException("세션 대여·콜백 중 패킷 등록 초기화");
        }

        // 마지막 타입까지 함수·버전을 지우고 길이·owner 표만 남긴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (uint type = 0u; type <= _lastType; ++type)
        {
            ushort length = _entries[(int)type].Length;
            _entries[(int)type] = new PacketRegistration { Length = length };
        }
    }

    /// <summary>활성·반납 중 대여에 걸린 콜백이 있는지 본다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <returns>걸린 콜백이 있으면 참이다.</returns>
    public bool ConnectionCallbackActive()
    {
        PacketRegistrationLease? current = null;
        PacketRegistrationLease? retiring = null;
        if (_current is not null)
        {
            _current.TryGetTarget(out current);
        }

        if (_retiring is not null)
        {
            _retiring.TryGetTarget(out retiring);
        }

        return (current is not null && current.Calls > 0) || (retiring is not null && retiring.Calls > 0);
    }

    /// <summary>세션 owner를 받아 대여를 건다. 중복·반납 중 콜백이 있으면 멈춘다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="owner">묶을 세션 owner다.</param>
    /// <exception cref="ArgumentNullException">묶을 세션 owner 누락이다.</exception>
    /// <exception cref="InvalidOperationException">세션 중복·반납 중 대여다.</exception>
    public PacketRegistrationLease BindSession(object owner)
    {
        ArgumentNullException.ThrowIfNull(owner);
        if (_currentIdentity is not null)
        {
            throw new InvalidOperationException("패킷 등록 세션 중복 대여");
        }

        if (_retiring is not null && _retiring.TryGetTarget(out PacketRegistrationLease? retiring) && retiring is not null && retiring.Calls > 0)
        {
            throw new InvalidOperationException("반납 세션 콜백 중 패킷 등록 대여");
        }

        var lease = new PacketRegistrationLease(this, owner);
        _current = new WeakReference<PacketRegistrationLease>(lease);
        _currentIdentity = lease;
        return lease;
    }

    /// <summary>타입 등록 API를 건드리면 활성 대여에 표시하고 핀한다. 비활성 때는 대여 표시를 지운다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="type">건드린 타입 번호다.</param>
    /// <exception cref="InvalidOperationException">반납 뒤·반납 중 콜백의 등록 변경이다.</exception>
    internal void Touch(uint type)
    {
        if (_current is not null && _current.TryGetTarget(out PacketRegistrationLease? current) && current is not null && current.Active)
        {
            current.Touch(type);
            _entries[(int)type].RegistrationLease = new WeakReference<PacketRegistrationLease>(current);
            _entries[(int)type].Leased = true;
            return;
        }

        if (_currentIdentity is not null)
        {
            throw new InvalidOperationException("반납 뒤 대여 정리의 패킷 등록 변경");
        }

        if (_retiring is not null && _retiring.TryGetTarget(out PacketRegistrationLease? retiring) && retiring is not null && retiring.Calls > 0)
        {
            throw new InvalidOperationException("반납 중 콜백의 패킷 등록 변경");
        }

        _entries[(int)type].RegistrationLease = null;
        _entries[(int)type].Leased = false;
    }

    /// <summary>표준 F 등록의 baseline을 저장한다. 활성 대여에만 snapshot을 남긴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="type">저장할 타입 번호다.</param>
    /// <exception cref="InvalidOperationException">표준 F 없음·반납 뒤·반납 중 콜백의 baseline 저장이다.</exception>
    internal void PersistCanonicalBaseline(uint type)
    {
        if (_entries[(int)type].CanonicalReceive is null)
        {
            throw new InvalidOperationException("표준 F 없는 패킷 baseline 저장");
        }

        PacketRegistrationLease? current = null;
        if (_current is not null)
        {
            _current.TryGetTarget(out current);
        }

        if (_currentIdentity is not null && (current is null || !current.Active))
        {
            throw new InvalidOperationException("반납 뒤 대여의 패킷 baseline 저장");
        }

        if (current is null && ConnectionCallbackActive())
        {
            throw new InvalidOperationException("반납 중 콜백의 패킷 baseline 저장");
        }

        PacketRegistration selected = _entries[(int)type];
        selected.RegistrationLease = null;
        selected.Leased = false;
        PacketRegistration previous = selected;
        if (current is not null)
        {
            current.SaveBaseline((int)type, previous);
            current.Touch(type);
        }

        _entries[(int)type] = selected;
    }

    /// <summary>반납 때 활성 대여 자리를 비우고 바꾼 항목을 되돌린다. 대여 쪽에서만 부른다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="lease">빠지는 대여다.</param>
    /// <param name="previous">되돌릴 snapshot 표다.</param>
    /// <param name="touched">바꾼 항목 표시다.</param>
    internal void DetachLease(PacketRegistrationLease lease, PacketRegistration[] previous, bool[] touched)
    {
        if (!ReferenceEquals(_currentIdentity, lease))
        {
            return;
        }

        // 바꾼 항목만 이전 snapshot과 바꾼다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        for (uint type = 1u; type <= _lastType; ++type)
        {
            if ((int)type < touched.Length && touched[(int)type])
            {
                PacketRegistration current = _entries[(int)type];
                _entries[(int)type] = previous[(int)type];
                previous[(int)type] = current;
            }
        }

        _retiring = new WeakReference<PacketRegistrationLease>(lease);
        _current = null;
        _currentIdentity = null;
    }

    /// <summary>전체 256 등록 표의 얕은 사본이다. 대여 snapshot용이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <returns>표 사본이다.</returns>
    internal PacketRegistration[] SnapshotEntries() => (PacketRegistration[])_entries.Clone();
}

// 세션 owner와 표를 함께 묶는 대여다. 반납하면 바꾼 항목을 되돌린다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class PacketRegistrationLease
{
    /// <summary>묶은 표다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly PacketRegistrationTable _table;

    /// <summary>묶은 세션 owner의 약한 참조다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly WeakReference<object> _owner;

    /// <summary>바꾼 항목의 되돌리기 snapshot 256개다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly PacketRegistration[] _previous = new PacketRegistration[256];

    /// <summary>바꾼 항목 표시 256비트다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly bool[] _touched = new bool[256];

    /// <summary>걸린 콜백 수다. 0이 아니면 반납·초기화를 막는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    internal int Calls { get; private set; }

    /// <summary>활성 여부다. 거짓이면 다시 쓸 수 없다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public bool Active { get; private set; } = true;

    /// <summary>표와 owner로 대여를 건다. 표 쪽에서만 만든다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="table">묶을 표다.</param>
    /// <param name="owner">묶을 세션 owner다.</param>
    internal PacketRegistrationLease(PacketRegistrationTable table, object owner)
    {
        _table = table;
        _owner = new WeakReference<object>(owner);
        _previous = table.SnapshotEntries();
    }

    /// <summary>빠질 때 바꾼 항목을 되돌리고 표에서 빠진다. 두 번 불러도 한 번만 돈다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public void Detach()
    {
        if (!Active)
        {
            return;
        }

        Active = false;
        _table.DetachLease(this, _previous, _touched);
    }

    /// <summary>버려지면 표에서 빠진다. 결정적 반납은 <see cref="Detach"/>를 쓴다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    ~PacketRegistrationLease()
    {
        try
        {
            Detach();
        }
        catch
        {
        }
    }

    /// <summary>세션 owner와 함께 콜백 pin을 잡는다. 반납 뒤·끊긴 owner면 멈춘다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <returns>콜백 pin이다.</returns>
    /// <exception cref="InvalidOperationException">반납·끊긴 owner의 pin이다.</exception>
    public object Pin()
    {
        if (!Active)
        {
            throw new InvalidOperationException("반납한 세션 대여의 콜백 pin");
        }

        if (!_owner.TryGetTarget(out object? owner) || owner is null)
        {
            throw new InvalidOperationException("세션 owner 끊긴 패킷 등록의 pin");
        }

        return new CallbackPin(this, owner);
    }

    /// <summary>baseline 저장 때 이전 항목을 둔다. 표 쪽에서만 부른다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="type">둘 위치다.</param>
    /// <param name="previous">둘 이전 항목이다.</param>
    internal void SaveBaseline(int type, PacketRegistration previous) => _previous[type] = previous;

    /// <summary>바꾼 항목 표시를 둔다. 표 쪽에서만 부른다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="type">표시할 타입 번호다.</param>
    internal void Touch(uint type) => _touched[(int)type] = true;

    /// <summary>콜백 pin이다. 살아 있는 동안 Calls를 올리고, 버리면 내린다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private sealed class CallbackPin : IDisposable
    {
        /// <summary>묶은 대여다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
        private readonly PacketRegistrationLease _lease;

        /// <summary>함께 잡은 세션 owner다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
        private readonly object _owner;

        /// <summary>버렸는지다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
        private bool _disposed;

        /// <summary>pin을 잡고 Calls를 올린다.</summary>
        /// <param name="lease">묶을 대여다.</param>
        /// <param name="owner">함께 잡을 세션 owner다.</param>
        public CallbackPin(PacketRegistrationLease lease, object owner)
        {
            _lease = lease;
            _owner = owner;
            ++lease.Calls;
        }

        /// <summary>pin을 놓고 Calls를 내린다. 두 번 불러도 한 번만 돈다.</summary>
        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }

            _disposed = true;
            --_lease.Calls;
            GC.SuppressFinalize(this);
        }

        /// <summary>버리지 않고 버려지면 Calls를 내린다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
        ~CallbackPin() => --_lease.Calls;
    }
}
