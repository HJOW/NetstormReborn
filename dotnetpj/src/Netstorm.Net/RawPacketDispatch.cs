using Netstorm.Core.Original;
using System.Buffers.Binary;

// 같은 실제 등록표와 backend 고유의 시계·기록·연결 수명을 조립한다.
namespace Netstorm.Net;

/// <summary>분배 계산 밖의 실제 시계·기록·연결 종료 장치다.</summary>
public sealed class PacketDispatchHooks
{
    /// <summary>원문 시계의 현재 DWORD 변환 결과다.</summary>
    public Func<uint>? Now { get; set; }
    /// <summary>현재 원문 버퍼와 선언 길이를 수신 방향으로 기록한다.</summary>
    public Action<PacketConnection, ReadOnlyMemory<byte>, ushort>? Record { get; set; }
    /// <summary>버전 정책과 손상 집합의 실제 종료 통지다.</summary>
    public Action<PacketConnection>? Terminate { get; set; }
    /// <summary>닫힌 연결의 실제 소멸 본체다.</summary>
    public Action<PacketConnection>? Close { get; set; }
    /// <summary>소멸 뒤 실제 연결 저장소를 반납한다.</summary>
    public Action<PacketConnection>? Release { get; set; }
    /// <summary>실제 송신 복사/쓰기 뒤 방향1 기록이다.수신 Record와 구분한다.</summary>
    public Action<PacketConnection, ReadOnlyMemory<byte>, ushort>? Sent { get; set; }
    /// <summary>재할당 뒤에도 같은 현재 Player9 기록을 조회한다.</summary>
    public Func<ReadOnlyMemory<byte>>? Players { get; set; }
    /// <summary>원문 함수 객체의 값 복사로 외부 재연결과 현재 실행을 분리한다.</summary>
    internal PacketDispatchHooks Copy() => (PacketDispatchHooks)MemberwiseClone();
}

/// <summary>cpppj RawPacketDispatch의 길이/버전·즉시 수신·현재 표와 직접 호출 본체다.</summary>
public sealed class RawPacketDispatch
{
    /// <summary>원문 PATCH137/CD126의 등록 한도다.</summary>
    private readonly uint _lastType;
    /// <summary>이 backend의 실제 장치다.공유 표에는 넣지 않는다.</summary>
    private readonly PacketDispatchHooks _hooks;
    /// <summary>Local/TCP/Editor가 같은 owner로 빌릴 실제 표다.</summary>
    public PacketRegistrationTable Registrations { get; }
    /// <summary>필수 실제 장치와 공유 표 판본을 효과 없이 확인한다.</summary>
    public RawPacketDispatch(OriginalEdition edition, PacketDispatchHooks hooks, PacketRegistrationTable? registrations = null)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        _lastType = edition == OriginalEdition.Patch1078 ? 137u : 126u;
        Registrations = registrations ?? new(edition);
        if (hooks.Now is null || hooks.Record is null || hooks.Terminate is null || hooks.Close is null || hooks.Release is null
            || Registrations.LastType() != _lastType) throw new ArgumentException("패킷 분배 실제 장치/공유 표 판본 불일치");
        _hooks = hooks.Copy();
    }
    /// <summary>등록만0번을 거부하며 원문 한도 밖 쓰기를 막는다.</summary>
    private void CheckType(uint type)
    {
        if (type == 0 || type > _lastType) throw new ArgumentOutOfRangeException(nameof(type), "패킷 등록 타입 범위 오류");
    }
    /// <summary>선택한 등록 칸만 빌리며0번 일반 등록 접근은 거부한다.</summary>
    public PacketRegistration Registration(uint type) { CheckType(type); return Registrations.Registration(type); }
    /// <summary>일반 재등록은 이전 nullable 직접 입구와 고정 F를 함께 제거한다.</summary>
    public void Register(uint type, Action<uint, PacketConnection, ReadOnlyMemory<byte>>? receive)
    {
        CheckType(type); Registrations.Touch(type); PacketRegistration entry = Registrations.Registration(type);
        entry.Data = entry.Data with { Receive = receive, Direct = null, Canonical = null };
    }
    /// <summary>동일 callable을 일반/nullable 입구가 공유하며 공개 전 모든 대리자를 준비한다.</summary>
    public void RegisterDirect(uint type, Action<uint, PacketConnection?, ReadOnlyMemory<byte>>? receive)
    {
        CheckType(type);
        Action<uint, PacketConnection, ReadOnlyMemory<byte>>? framed = receive is null ? null : (sender, connection, packet) => receive(sender, connection, packet);
        Registrations.Touch(type); PacketRegistration entry = Registrations.Registration(type);
        entry.Data = entry.Data with { Receive = framed, Direct = receive, Canonical = null };
    }
    /// <summary>원문 주소 정체성과 실제 고정 F를 같은 등록 칸에 함께 설치한다.</summary>
    public void RegisterCanonicalDirect(uint type, uint nativeEntry, Action<uint, PacketConnection?, ReadOnlyMemory<byte>> receive)
        => RegisterCanonical(type, new(nativeEntry, receive));
    /// <summary>이미 선택한 F를 먼저 보존하므로 같은 현재 칸 인자여도 자기 재등록에 안전하다.</summary>
    public void RegisterCanonical(uint type, CanonicalPacketReceiver? receive)
    {
        CheckType(type); CanonicalPacketReceiver? selected = receive;
        RegisterDirect(type, selected is null ? null : selected.Invoke);
        Registrations.Registration(type).Data = Registrations.Registration(type).Data with { Canonical = selected };
    }
    /// <summary>검토한 장기 F의 실제 전역 setter 결과만 세션 반납 기준으로 승격한다.</summary>
    public void PersistCanonicalBaseline(uint type) { CheckType(type); Registrations.PersistCanonicalBaseline(type); }
    /// <summary>지연 선택은 고정 F 값만 복사하며 짧은 세션 closure를 안전하다고 가장하지 않는다.</summary>
    public CanonicalPacketReceiver? SnapshotReceiver(uint type)
    {
        PacketRegistration entry = Registration(type);
        if (entry.Data.Receive is null && entry.Data.Direct is null) return null;
        return entry.CanonicalReceive ?? throw new InvalidOperationException("지연 receiver의 장기 고정 F 전환 미완료");
    }
    /// <summary>검증/관찰/기록/시각/깊이를 추가하지 않고 현재 표의 직접 입구만 실행한다.</summary>
    public void InvokeDirect(uint type, uint sender, PacketConnection? connection, ReadOnlyMemory<byte> packet)
    {
        PacketRegistration entry = Registration(type);
        if (entry.Data.Receive is null && entry.Data.Direct is null) return;
        using IDisposable? pin = entry.Pin(); Action<uint, PacketConnection?, ReadOnlyMemory<byte>>? direct = entry.Data.Direct;
        if (direct is not null) { direct(sender, connection, packet); return; }
        Action<uint, PacketConnection, ReadOnlyMemory<byte>>? receive = entry.Data.Receive;
        if (connection is null) throw new ArgumentException("NULL 연결을 지원하지 않는 일반 패킷 수신기");
        receive?.Invoke(sender, connection, packet);
    }
    /// <summary>방향 관찰기 재등록은 같은 칸의 패킷 관찰기를 제거한다.</summary>
    public void Observe(uint type, Action<uint>? observer)
    {
        CheckType(type); Registrations.Touch(type); PacketRegistration entry = Registrations.Registration(type);
        entry.Data = entry.Data with { Observer = observer, PacketObserver = null };
    }
    /// <summary>패킷 관찰기 재등록은 같은 칸의 방향 관찰기를 제거한다.</summary>
    public void ObservePacket(uint type, Action<Memory<byte>, uint>? observer)
    {
        CheckType(type); Registrations.Touch(type); PacketRegistration entry = Registrations.Registration(type);
        entry.Data = entry.Data with { PacketObserver = observer, Observer = null };
    }
    /// <summary>현재 프로토콜과 불일치 정책 BYTE만 바꾼다.</summary>
    public void Version(uint type, byte version, byte mismatch)
    {
        CheckType(type); Registrations.Touch(type); PacketRegistration entry = Registrations.Registration(type);
        entry.Data = entry.Data with { Version = version, Mismatch = mismatch };
    }
    /// <summary>현재 원문 길이 WORD만 바꾼다.ffff는 가변이다.</summary>
    public void RequireLength(uint type, ushort length)
    {
        CheckType(type); Registrations.Touch(type); PacketRegistration entry = Registrations.Registration(type);
        entry.Data = entry.Data with { Length = length };
    }
    /// <summary>최소4BYTE·실제 타입/등록·선언 길이만 검사하며 실제 버퍼 길이와 선언 길이를 같게 강제하지 않는다.</summary>
    public bool Validate(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 4) throw new ArgumentException("패킷 헤더 길이 부족", nameof(packet));
        byte type = packet[1]; if (type == 0 || type > _lastType) return false;
        PacketRegistration entry = Registrations.Registration(type);
        return (type <= 9 || entry.Data.Receive is not null) && (entry.Length == ushort.MaxValue || entry.Length == Length(packet));
    }
    /// <summary>시계 전에 버전을 검사하며 record 뒤 저장 type의 현재 F를 선택한다.</summary>
    public bool Dispatch(PacketConnection connection, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(connection);
        if ((connection.Flags & 0x40) != 0) return false;
        if (!Validate(packet.Span)) return true;
        byte type = packet.Span[1]; if (type == 4) throw new InvalidOperationException("단일 분배의 집합 패킷은 공통 프레임 부모가 필요합니다");
        PacketRegistration entry = Registrations.Registration(type); int incoming = packet.Span[0] & 0x7f;
        if (incoming != entry.Version)
        {
            bool lower = incoming < entry.Version;
            if ((entry.Mismatch & (lower ? 8 : 4)) != 0) { _hooks.Terminate!(connection); return false; }
            if ((entry.Mismatch & (lower ? 2 : 1)) != 0) return true;
        }
        using IDisposable? recordPin = entry.Pin(); uint now = _hooks.Now!();
        if (type == 6) connection.Heartbeat = now; else connection.Received = now;
        if (type > 9) _hooks.Record!(connection, packet, Length(packet.Span));
        _ = DeliverReceiver(type, connection, packet);
        if (connection.Depth == 0 && (connection.Flags & 0x40) != 0) { _hooks.Close!(connection); _hooks.Release!(connection); return false; }
        return true;
    }
    /// <summary>저장된 type의 현재 F를 한 번 선택하고 실제 큐 또는 즉시 깊이로 실행한다.</summary>
    public int DeliverReceiver(uint type, PacketConnection connection, ReadOnlyMemory<byte> packet, int delay = -1)
    {
        PacketRegistration entry = Registration(type);
        if (entry.Data.Receive is null && entry.Data.Direct is null) return 0;
        if (connection.DelayQueue is { } queue) return queue(connection, SnapshotReceiver(type), this, packet, delay);
        if (connection.Delayed) throw new InvalidOperationException("지연 표시가 있는 연결의 실제 FIFO 미연결");
        using IDisposable? pin = entry.Pin(); Action<uint, PacketConnection, ReadOnlyMemory<byte>>? receive = entry.Data.Receive;
        if (receive is null) return 0;
        connection.Depth = unchecked(connection.Depth + 1);
        try { receive(connection.Sender, connection, packet); }
        finally { connection.Depth = unchecked(connection.Depth - 1); }
        return 0;
    }
    /// <summary>집합과 은퇴 검증 정책을 포함한 같은 실제 프레임 부모로 분배한다.</summary>
    public bool DispatchFrame(PacketConnection connection, Memory<byte> frame) => RawPacketFrame.Dispatch(this, connection, frame, _hooks);
    /// <summary>4BYTE 확보 뒤에만 선언 길이 WORD를 읽는다.</summary>
    internal static ushort Length(ReadOnlySpan<byte> packet) => BinaryPrimitives.ReadUInt16LittleEndian(packet[2..]);
    /// <summary>선택한 현재 Player base의 owner+50 낮은 BYTE만 읽는다.</summary>
    public static bool PlayerActive(ReadOnlyMemory<byte> fallback, Func<ReadOnlyMemory<byte>>? current, uint owner)
    {
        ReadOnlyMemory<byte> players = current is not null ? current() : fallback;
        if (players.Length != 9 * 180 || owner >= 9) throw new ArgumentOutOfRangeException(nameof(owner), "현재 Player9 물리 범위 오류");
        return (players.Span[checked((int)owner) * 180 + 0x50] & 1) != 0;
    }
}
