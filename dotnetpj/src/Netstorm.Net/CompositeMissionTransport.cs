using Netstorm.Core.Original;

// 같은 실제 내부 양끝/Registry/등록표를 장기 소유하고 현재 전역ID마다 한 장치만 선택한다.
namespace Netstorm.Net;

/// <summary>원문 선택된 전송 역할이다. 현재 이식 범위는 InternalMission이다.</summary>
public enum CompositeTransportRole
{
    /// <summary>단독 미션의 실제 내부 listener와 두 끝이다.</summary>
    InternalMission,
    /// <summary>기존 Editor 전송이며 후속 실제 backend 이식이 필요하다.</summary>
    Editor,
    /// <summary>실제 LAN 호스트이며 후속 TCP backend 이식이 필요하다.</summary>
    LanHost,
    /// <summary>실제 LAN 클라이언트이며 후속 TCP backend 이식이 필요하다.</summary>
    LanClient,
}

/// <summary>복합 전송의 내부 미션 분기와 이전 Source 노드의 실제 가상 호출을 조정한다.</summary>
public sealed class CompositeMissionTransport : IDisposable
{
    /// <summary>같은 실제 Client 물리를 모든 등록/호출 대여 끝까지 유지한다.</summary>
    private readonly object _globalsLifetime;
    /// <summary>현재 실제 서버 권한을 읽으며 새 권한을 만들지 않는다.</summary>
    private readonly Func<bool> _server;
    /// <summary>기존 실제 내부 양끝과 완성 프레임/출력 큐다.</summary>
    private readonly LocalMissionTransport _internal;
    /// <summary>같은 실제 Source에 대한 외부/노드/호출 수명 장부다.</summary>
    private readonly ConnectionDeviceLifetime _lifetime;
    /// <summary>이번 단일ID순회가 실행 중인지다.</summary>
    private bool _polling;
    /// <summary>Source의 실제 최종 반납을 마쳤는지다.</summary>
    private bool _released;
    /// <summary>모든 전송과 State가 함께 빌리는 실제 현재 표다.</summary>
    public RawConnectionRegistry Registry { get; }
    /// <summary>모든 등록/송수신이 공유하는 같은 canonical 표다.</summary>
    public RawPacketDispatch Dispatch => _internal.Dispatch;
    /// <summary>실제 내부 분기를 선택한 역할이며 권한/플레이어 상태와 별개다.</summary>
    public CompositeTransportRole Role => CompositeTransportRole.InternalMission;
    /// <summary>현재 구현된 실제 backend를 특수 설치/진단에서 동일 객체로 빌린다.</summary>
    public LocalMissionTransport InternalTransport => _internal;
    /// <summary>등록 대여를 포함한 Source 수명이 실제로 끝났는지 조회한다.</summary>
    public bool Released => _released;
    /// <summary>생성 자체는 listener/Client9를 만들지 않으며 실제 Player9/장기 소유자를 요구한다.</summary>
    public CompositeMissionTransport(CompositeTransportRole role, ReadOnlyMemory<byte> players, Func<bool> server, PacketDispatchHooks hooks, RawConnectionRegistry registry, object globalsLifetime, PacketRegistrationTable? registrations = null)
    {
        ArgumentNullException.ThrowIfNull(server); ArgumentNullException.ThrowIfNull(registry); ArgumentNullException.ThrowIfNull(globalsLifetime);
        if (role != CompositeTransportRole.InternalMission) throw new NotSupportedException("복합 전송의 Editor/LAN 실제 backend는 후속 이식 대상");
        if (players.Length != 1620) throw new ArgumentException("복합 전송의 실제 Player9 누락", nameof(players));
        _server = server; _globalsLifetime = globalsLifetime; Registry = registry;
        _internal = new(players, server, hooks, registry, registrations); _lifetime = new(this, static owner => ((CompositeMissionTransport)owner).ReleaseDevice());
    }
    /// <summary>실제 노드/배송 호출이 같은 Source 수명을 선택한 동안만 대여한다.</summary>
    internal ConnectionDevicePin Pin() => _lifetime.Pin();
    /// <summary>배송값은 살아 있는 같은 실제 Source에서만 복구한다.</summary>
    internal NativeBattleSource? Borrow() => _released ? null : new(this, Pin());
    /// <summary>최종 수명 뒤에만 실제 내부 큐/별칭을 반납한다. Native 방 종료와 구별한다.</summary>
    private void ReleaseDevice()
    {
        _internal.Dispose(); _released = true; GC.KeepAlive(_globalsLifetime);
    }
    /// <summary>retired 노드도 실제 같은 두 물리 주소로 식별한다.</summary>
    public bool OwnsConnection(PacketConnection connection) => _internal.OwnsConnection(connection);
    /// <summary>현재 token이 없으면 Missing, 자기 실제 끝이면 Internal, 그 외는 이전 Source다.</summary>
    private int Select(int id) => RawTransportPolling.CurrentTransportToken(Registry, id) == 0 ? 0 : _internal.Connection(id) is not null ? 1 : 2;
    /// <summary>실제 서버 권한으로 같은 내부 listener만 연다. 이미 열려 있으면 유지한다.</summary>
    public void Serve(uint port)
    {
        using var pin = Pin(); if (!_server()) throw new InvalidOperationException("서버 권한 없는 내부 listener 요청");
        if (!_internal.IsOpen()) _internal.Open("127.0.0.1", port, false);
    }
    /// <summary>현재 실제20바이트 주소 계약을 확인하고 같은 내부 Client9만 연결한다.</summary>
    public void Connect(ReadOnlyMemory<byte> address, uint port)
    {
        using var pin = Pin(); if (address.Length != 20) throw new ArgumentOutOfRangeException(nameof(address), "현재 State8 주소20바이트 누락"); _internal.Connect();
    }
    /// <summary>내부 역할의 문자열 재접속도 같은 기존 끝을 사용한다.</summary>
    public void ConnectPeerText(string address, uint port) { using var pin = Pin(); _internal.Connect(); }
    /// <summary>현재 수락된 자기 노드만 같은 실제 backend에서 소유자에게 재배치한다.</summary>
    public void Assign(int pending, int owner)
    {
        using var pin = Pin(); if (Select(pending) != 1) throw new InvalidOperationException("현재 accepted 노드 없는 복합 전송 재배치"); _internal.AssignConnection(pending, owner);
    }
    /// <summary>현재 선택한 등록값과 실제 호출자 버퍼 소유자를 전체 송신 동안 유지한다.</summary>
    public bool Send(int target, ReadOnlyMemory<byte> packet)
    {
        using var pin = Pin(); int selected = Select(target); if (selected == 0) return false;
        if (packet.Length < 4) throw new ArgumentOutOfRangeException(nameof(packet), "복합 송신의 원문 헤더 부족"); using IDisposable? sessionPin = Dispatch.Registration(packet.Span[1]).Pin();
        if (selected != 1) throw new InvalidOperationException("복합 전송이 소유하지 않은 현재 송신 노드"); return _internal.Send(target, packet);
    }
    /// <summary>같은 mutable 예약의 observer1은 실제 backend가 한 번만 실행한다.</summary>
    public bool SendMutable(int target, Memory<byte> packet)
    {
        using var pin = Pin(); int selected = Select(target); if (selected == 0) return false;
        if (packet.Length < 4) throw new ArgumentOutOfRangeException(nameof(packet), "복합 mutable 송신의 원문 헤더 부족"); using IDisposable? sessionPin = Dispatch.Registration(packet.Span[1]).Pin();
        if (selected != 1) throw new InvalidOperationException("복합 전송이 소유하지 않은 현재 mutable 송신 노드"); return _internal.SendMutable(target, packet);
    }
    /// <summary>004e2920의 현재 정상0..21을 순회하며 이전 Source도 같은 실제 가상flush를 호출한다.</summary>
    public void Flush()
    {
        using var pin = Pin();
        // 각 callback 뒤 다음ID의 현재 token을 다시 조회한다.
        for (int id = 0; id < 22; id++)
        {
            int selected = Select(id); if (selected == 1) _internal.FlushOne(id); else if (selected == 2) Registry.FlushDirect(RawTransportPolling.CurrentTransportToken(Registry, id));
        }
    }
    /// <summary>현재ID의 실제 backend 한 프레임만 소비하고 이전 Source도 등록 장치를 사용한다.</summary>
    private TransportPollResult ReceiveOne(int id)
    {
        int selected = Select(id); if (selected == 0) return default; if (selected == 1) return _internal.ReceiveOne(id);
        uint token = RawTransportPolling.CurrentTransportToken(Registry, id); if (Registry.Owns(token)) return Registry.ReceiveDirect(token);
        throw new InvalidOperationException($"복합 전송의 현재 노드 장치 누락: id={id} token={token}");
    }
    /// <summary>실제 listener 수락 뒤 pending각1회·정상ID별 반복을 단 한 번 조립한다.</summary>
    public int Poll()
    {
        using var pin = Pin(); if (_polling) throw new InvalidOperationException("복합 전송 Poll 중첩 호출"); _polling = true; bool begun = false;
        try
        {
            _internal.BeginPoll(); begun = true; _internal.AcceptPending(); int count = 0;
            // 원문 pending-1..-31을 각각 한 번 소비한다.
            for (int id = -1; id > -32; id--) if (ReceiveOne(id).Consumed) count++;
            // 정상0..21은 callback의 재배치/반납 뒤 같은 현재ID를 다시 읽는다.
            for (int id = 0; id < 22; id++)
            {
                // 실제 하위가 again을 반환한 동안만 같은ID를 이어간다.
                for (;;) { TransportPollResult result = ReceiveOne(id); if (result.Consumed) count++; if (!result.Again) break; }
            }
            return count;
        }
        finally { try { if (begun) _internal.EndPoll(); } finally { _polling = false; } }
    }
    /// <summary>현재 Registry의 실제 close/free 부모를 사용한다.</summary>
    public void Close(int id) { using var pin = Pin(); Registry.Disconnect(id); }
    /// <summary>자기 실제 정상/pending 노드와 listener만 닫고 다른 Source 표는 보존한다.</summary>
    public void CloseRoom()
    {
        using var pin = Pin();
        // 현재 자기 정상 노드만 번호순으로 종료한다.
        for (int id = 0; id < 22; id++) if (Select(id) == 1) Registry.Disconnect(id);
        // 현재 자기 pending만 번호순으로 종료한다.
        for (int id = -1; id > -32; id--) if (Select(id) == 1) Registry.Disconnect(id);
        _internal.CloseAll();
    }
    /// <summary>같은 현재 자기 노드만 반환하며 다른 Source를 자기 노드로 가장하지 않는다.</summary>
    public PacketConnection? Connection(int id) => Select(id) switch { 0 => null, 1 => _internal.Connection(id), _ => throw new InvalidOperationException("다른 Source의 연결 주소 조회") };
    /// <summary>객체 존재만 읽으며 Ready/Player 성공을 쓰지 않는다.</summary>
    public bool Exists(int id) => Connection(id) is not null;
    /// <summary>현재 내부 끝의 상대 종료/닫힘을 실제 backend에서 조회한다.</summary>
    public bool IsOpen(int id) => Select(id) switch { 0 => false, 1 => _internal.ConnectionOpen(id), _ => throw new InvalidOperationException("다른 Source의 open 조회") };
    /// <summary>현재 내부 접속 완료/진행 값을 읽으며 buffered비트를 성공으로 쓰지 않는다.</summary>
    public int Status(int id) => Select(id) switch { 0 => 0, 1 => _internal.ConnectionStatus(id), _ => throw new InvalidOperationException("다른 Source의 status 조회") };
    /// <summary>같은 현재 노드의 전체20바이트 주소를 반환한다.</summary>
    public byte[] Address(int id) { _ = Connection(id); return Registry.Address(id); }
    /// <summary>같은 실제 Session/World owner를 등록표가 약하게 빌리게 한다.</summary>
    public PacketRegistrationLease BindSession(object owner) => Dispatch.Registrations.BindSession(owner);
    /// <summary>반납 중인 이전 Session의 실제 callback도 포함한다.</summary>
    public bool ConnectionCallbackActive() => Dispatch.Registrations.ConnectionCallbackActive();
    /// <summary>외부 Source 소유권만 반납한다. 등록 노드/호출 대여가 남으면 최종 반납을 미룬다.</summary>
    public void Dispose() => _lifetime.Dispose();
}
