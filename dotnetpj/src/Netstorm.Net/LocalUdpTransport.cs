using System.Net;
using System.Net.Sockets;

namespace Netstorm.Net;

// 루프백 UDP 12번 단일 노드를 같은 실제 연결 표 없이 .NET 소켓으로 미리 둔다.
// 실제 분배·등록·송신 기록은 세션 연결 때 잇는다.
// 근거: cpppj/src/client/LocalUdpTransport.h·LocalUdpTransport.cpp. (2026-11-10 멀티플레이 사전 구현에서 추가했다)
/// <summary>루프백 UDP 전송의 사전 구현이다. 검사 없이 cpppj 소스만 참고해 먼저 만들며 실제 분배·등록 연결은 세션에서 잇는다.
/// 외부 브로드캐스트를 보내지 않고 루프백에만 송수신한다.</summary>
public sealed class LocalUdpTransport : IDisposable
{
    /// <summary>원본 UDP 12번 단일 노드 번호다.</summary>
    public const int UdpNodeId = 12;

    /// <summary>수신 물리 바이트 상한이다.</summary>
    public const int Capacity = 4096;

    /// <summary>유선 헤더 바이트 수다.</summary>
    public const int HeaderSize = 4;

    /// <summary>폐기 패킷 종류 2종이다.</summary>
    public const byte RetiredType1 = 0x7f;

    /// <summary>폐기 패킷 종류 2종이다.</summary>
    public const byte RetiredType2 = 0x80;

    /// <summary>집합 프레임 종류다.</summary>
    public const byte AggregateType = 4;

    /// <summary>수신 프레임을 같은 등록표에 분배하는 경계다. 세션 연결 때 둔다.</summary>
    private LocalMissionTransport.FrameDispatch? _dispatch;

    /// <summary>실제 UDP 소켓이다. 없으면 null이다.</summary>
    private Socket? _socket;

    /// <summary>마지막으로 받은 상대 끝점이다. 송신 대상으로 쓴다.</summary>
    private EndPoint? _remote;

    /// <summary>닫힘 표시가 됐는지다.</summary>
    private bool _closed;

    /// <summary>물리 제거가 끝났는지다.</summary>
    private bool _retired;

    /// <summary>마지막 OS 실패 코드다.</summary>
    private int _lastError;

    /// <summary>Poll 진행 중인지다. 재진입을 거부한다.</summary>
    private bool _polling;

    /// <summary>이미 닫혔는지다.</summary>
    private bool _disposed;

    /// <summary>물리 헤더가 있는 현재 패킷의 비정렬 선언 WORD를 읽는다.</summary>
    /// <param name="packet">읽을 패킷이다.</param>
    private static int Length(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < HeaderSize)
            throw new ArgumentOutOfRangeException(nameof(packet), "UDP 실제 헤더 부족");
        return packet[2] | (packet[3] << 8);
    }

    /// <summary>폐기·집합 송신을 모두 봐서 실제 유선 이전에 제외한다.</summary>
    /// <param name="packet">검사할 패킷이다.</param>
    private static bool Retired(ReadOnlySpan<byte> packet)
    {
        if (packet[1] == RetiredType1 || packet[1] == RetiredType2)
            return true;
        if (packet[1] != AggregateType)
            return false;
        int total = Length(packet);
        if (total < HeaderSize || total > packet.Length)
            throw new ArgumentOutOfRangeException(nameof(packet), "UDP 집합 물리 부족");
        // 단일 집합 안의 내부 패킷 경계만 확인하며 중첩 집합은 원본처럼 거부한다.
        for (int at = HeaderSize; at < total;)
        {
            if (total - at < HeaderSize)
                throw new ArgumentOutOfRangeException(nameof(packet), "UDP 개별 헤더 부족");
            int length = packet[at + 2] | (packet[at + 3] << 8);
            if (length < HeaderSize || length > total - at)
                throw new ArgumentOutOfRangeException(nameof(packet), "UDP 개별 길이 오류");
            if (packet[at + 1] == RetiredType1 || packet[at + 1] == RetiredType2)
                return true;
            if (packet[at + 1] == AggregateType)
                throw new InvalidOperationException("UDP 중첩 집합 미지원");
            at += length;
        }
        return false;
    }

    /// <summary>소켓 끝점의 IP·포트를 원문 20바이트 주소로 만든다. 마지막 프로토콜 코드는 원문처럼 둔다.</summary>
    /// <param name="endpoint">바꿀 끝점이다.</param>
    private static byte[] AddressBytes(EndPoint endpoint)
    {
        var result = new byte[20];
        if (endpoint is IPEndPoint inet && inet.AddressFamily == AddressFamily.InterNetwork)
        {
            result[0] = 2;
            result[2] = unchecked((byte)(inet.Port >> 8));
            result[3] = unchecked((byte)inet.Port);
            byte[] raw = inet.Address.MapToIPv4().GetAddressBytes();
            Array.Copy(raw, 0, result, 4, 4);
        }
        return result;
    }

    /// <summary>수신 프레임을 같은 등록표에 분배하는 경계를 둔다. 세션 연결 때 부른다.</summary>
    /// <param name="dispatch">수신 프레임을 분배하는 경계다.</param>
    public void RegisterDispatch(LocalMissionTransport.FrameDispatch dispatch)
    {
        _dispatch = dispatch ?? throw new ArgumentNullException(nameof(dispatch));
    }

    /// <summary>단일 UDP 12번을 루프백에 연다. 외부 브로드캐스트 수신용이 아니라 루프백 송수신용이다.</summary>
    /// <param name="port">묶을 포트다.</param>
    public bool Open(uint port)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (port > 65535u)
            throw new ArgumentOutOfRangeException(nameof(port), "UDP 포트 물리 범위 오류");
        if (_socket is not null)
            return true;
        var socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
        try
        {
            socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            socket.Bind(new IPEndPoint(IPAddress.Loopback, (int)port));
            socket.Blocking = false;
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
            socket.Dispose();
            return false;
        }
        _socket = socket;
        _closed = false;
        _retired = false;
        return true;
    }

    /// <summary>깊이·수신 노드 묘사 없이 실제 분배기까지 물리를 보존한다.</summary>
    public void Close()
    {
        _closed = true;
        CloseReady();
    }

    /// <summary>닫힌 소켓만 반납하고 분배기 목록은 호출자가 지킨다.</summary>
    private void CloseReady()
    {
        if (_retired || !_closed)
            return;
        CloseSocket();
        _retired = true;
    }

    /// <summary>현재 실제 OS 소켓만 닫고 실패 원시 호출을 보존한다.</summary>
    private void CloseSocket()
    {
        Socket? socket = _socket;
        _socket = null;
        if (socket is null)
            return;
        try
        {
            socket.Close();
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
            throw new InvalidOperationException("UDP close 실패:" + _lastError, exception);
        }
    }

    /// <summary>같은 패킷을 루프백 상대에게 보낸다. 외부 브로드캐스트 주소로는 보내지 않는다.</summary>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    /// <param name="port">루프백 목적지 포트다. 0이면 마지막 수신 상대에게 보낸다.</param>
    public bool Broadcast(ReadOnlySpan<byte> packet, uint port)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (port > 65535u)
            throw new ArgumentOutOfRangeException(nameof(port), "UDP 송신 포트 물리 범위 오류");
        return SendTo(packet, port == 0 ? null : new IPEndPoint(IPAddress.Loopback, (int)port));
    }

    /// <summary>현재 단일 12번의 일반 가변 송신을 하고 가변 경계에서만 observer1을 수행한다.</summary>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    public bool Send(ReadOnlySpan<byte> packet)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        return SendTo(packet, null);
    }

    /// <summary>같은 this의 observer를 한 번 수행하는 가변 송신을 한다.</summary>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    public bool SendMutable(ReadOnlySpan<byte> packet)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        return SendTo(packet, null);
    }

    /// <summary>같은 패킷을 루프백 목적지에 보낸다. 목적지가 없으면 마지막 수신 상대를 쓴다.</summary>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    /// <param name="target">보낼 끝점이다. 없으면 마지막 수신 상대다.</param>
    private bool SendTo(ReadOnlySpan<byte> packet, EndPoint? target)
    {
        if (packet.Length < HeaderSize)
            throw new ArgumentOutOfRangeException(nameof(packet), "UDP 송신 물리 헤더 부족");
        if (Retired(packet))
            return false;
        Socket? socket = _socket;
        if (_retired || socket is null || _closed)
            return false;
        int length = Length(packet);
        if (length > packet.Length)
            throw new ArgumentOutOfRangeException(nameof(packet), "UDP 송신 선언 길이 물리 초과");
        target ??= _remote;
        if (target is null)
            return false;
        try
        {
            socket.SendTo(packet[..length].ToArray(), target);
        }
        catch (SocketException exception) when (exception.SocketErrorCode == SocketError.WouldBlock)
        {
            return false;
        }
        catch (SocketException exception)
        {
            Fail(exception.ErrorCode);
            return false;
        }
        return true;
    }

    /// <summary>2번 수신 무리가 있을 때만 실제 수신 물리 4바이트로 종류를 둔다.</summary>
    /// <param name="error">기록할 OS 실패 코드다.</param>
    private void Fail(int error)
    {
        if (error != 0)
            _lastError = error;
        if (_retired)
            return;
        byte[] frame = [0xff, 2, 4, 0];
        _dispatch?.Invoke(frame);
        CloseReady();
    }

    /// <summary>단일 12번만 받고 callback 없이 바로 반환하며 다음 조회에서 현재 값을 다시 센다.</summary>
    public void BeginPoll()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_polling)
            throw new InvalidOperationException("UDP 중복 진입 금지");
        _polling = true;
        try
        {
            CloseReady();
        }
        catch
        {
            _polling = false;
            throw;
        }
    }

    /// <summary>현재 12번의 데이터그램 하나·4096 상한·검증을 현재 본체로 처리한다.</summary>
    /// <param name="id">받을 번호다. 12번만 받는다.</param>
    /// <returns>소비한 프레임 수와 계속 여부다.</returns>
    public (int Consumed, bool Again) ReceiveOne(int id)
    {
        if (!_polling)
            throw new InvalidOperationException("UDP per-ID 수신은 Poll 대기 필요");
        if (id != UdpNodeId)
            return (0, false);
        Socket? socket = _socket;
        if (_retired || socket is null || _closed)
            return (0, false);
        var buffer = new byte[Capacity];
        EndPoint remote = new IPEndPoint(IPAddress.Any, 0);
        int count;
        try
        {
            count = socket.ReceiveFrom(buffer, ref remote);
        }
        catch (SocketException exception) when (exception.SocketErrorCode == SocketError.WouldBlock)
        {
            return (0, false);
        }
        catch (SocketException exception)
        {
            Fail(exception.ErrorCode);
            return (0, false);
        }
        if (remote is IPEndPoint inet && !IPAddress.IsLoopback(inet.Address.MapToIPv4()))
        {
            // 루프백 밖에서 온 데이터그램은 버리고 계속 받는다.
            return (1, true);
        }
        _remote = remote;
        if (count < HeaderSize)
            return (count > 0 ? 1 : 0, false);
        byte[] frame = buffer[..count];
        int length = Length(frame);
        if (length == 0 || length >= Capacity || length != count)
        {
            Fail(0);
            return (1, false);
        }
        if (frame[1] == RetiredType1 || frame[1] == RetiredType2)
            return (1, true);
        if (_dispatch?.Invoke(frame) != true)
            return (1, false);
        if (_retired || _closed)
        {
            CloseReady();
            return (1, false);
        }
        return (1, true);
    }

    /// <summary>전체 조회가 보통 polling 끝에만 반납 노드를 실제 물리 제거한다.</summary>
    public void EndPoll()
    {
        if (!_polling)
            throw new InvalidOperationException("UDP Poll 대기가 아니다");
        _polling = false;
    }

    /// <summary>단독 메뉴와 같은 per-ID 본체를 반복해 위치·주소를 새로 만들지 않는다.</summary>
    public int Poll()
    {
        BeginPoll();
        int count = 0;
        try
        {
            for (; ; )
            {
                (int consumed, bool again) = ReceiveOne(UdpNodeId);
                count += consumed;
                if (!again)
                    break;
            }
        }
        catch
        {
            EndPoll();
            throw;
        }
        EndPoll();
        return count;
    }

    /// <summary>같은 지정 실제 노드의 직접 지참이며 다른 ID는 UDP로 해석하지 않는다.</summary>
    /// <param name="id">확인할 번호다. 12번만 유효하다.</param>
    public bool IsOpen(int id) =>
        id == UdpNodeId && !_retired && _socket is not null && !_closed;

    /// <summary>현재 실제 12번의 소켓 준비 조회다. 같은 등록의 실제 가변 위치를 그대로 사용한다.</summary>
    /// <param name="id">조회할 번호다. 12번만 유효하다.</param>
    public int Status(int id)
    {
        if (id != UdpNodeId || _retired || _socket is null)
            return 0;
        try
        {
            return _socket.Poll(0, SelectMode.SelectRead) ? 1 : 0;
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
            return 0;
        }
    }

    /// <summary>원본처럼 자기 호스트명 첫 IPv4를 단일 프로필 주소로 둔다. 실패 시 family 0을 보존한다.</summary>
    public byte[] LocalAddress()
    {
        var result = new byte[20];
        try
        {
            string name = Dns.GetHostName();
            foreach (IPAddress candidate in Dns.GetHostAddresses(name))
            {
                if (candidate.AddressFamily != AddressFamily.InterNetwork)
                    continue;
                byte[] raw = candidate.MapToIPv4().GetAddressBytes();
                Array.Copy(raw, 0, result, 4, 4);
                if (result[4] != 0 || result[5] != 0 || result[6] != 0 || result[7] != 0)
                    result[0] = 2;
                return result;
            }
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
        }
        return result;
    }

    /// <summary>OS 실패를 직접 노출하지 않고 호출만 전달한다.</summary>
    public int LastError() => _lastError;

    /// <summary>현재 호스트 수명에서 없어지는 자기 소켓을 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed)
            return;
        _disposed = true;
        try
        {
            _socket?.Close();
        }
        catch (SocketException)
        {
        }
        _socket = null;
        _polling = false;
    }
}
