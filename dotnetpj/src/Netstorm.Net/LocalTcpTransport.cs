using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;

namespace Netstorm.Net;

// 루프백 LAN의 TCP 리스너·수락 노드·클라이언트 22칸을 같은 실제 연결 표 없이 .NET 소켓으로 미리 둔다.
// 실제 Login/UI 분배·연결 등록표·송신 기록은 세션·호스트 연결 때 잇는다.
// 근거: cpppj/src/client/LocalTcpTransport.h·LocalTcpTransport.cpp. (2026-11-10 멀티플레이 사전 구현에서 추가했다)
/// <summary>루프백 TCP 전송의 사전 구현이다. 검사 없이 cpppj 소스만 참고해 먼저 만들며 실제 분배·등록 연결은 세션에서 잇는다.
/// 외부 도메인·IP 접속은 거부하고 루프백만 허용한다.</summary>
public sealed class LocalTcpTransport : IDisposable
{
    /// <summary>유선 헤더 바이트 수(version·type·길이 WORD)다.</summary>
    public const int HeaderSize = 4;

    /// <summary>유선 한 프레임의 바이트 상한이다.</summary>
    public const int WireLimit = 65535;

    /// <summary>실제 송신 트랜시버 칸 수다(0~21).</summary>
    public const int TransceiverSlots = 22;

    /// <summary>수락 대기 pending 칸 수다(-1~-31).</summary>
    public const int PendingSlots = 31;

    /// <summary>폐기 패킷 종류 2종이다.</summary>
    public const byte RetiredType1 = 0x7f;

    /// <summary>폐기 패킷 종류 2종이다.</summary>
    public const byte RetiredType2 = 0x80;

    /// <summary>집합 프레임 종류다.</summary>
    public const byte AggregateType = 4;

    /// <summary>연결 알림 종류다.</summary>
    public const byte NotifyConnectType = 1;

    /// <summary>종료 알림 종류다.</summary>
    public const byte NotifyTerminateType = 2;

    /// <summary>미전송 backlog 바이트 상한이다.</summary>
    public const int WriteBacklog = 100000;

    /// <summary>리스너 대기열 길이(would-block 때 무한 적재하지 않는다)다.</summary>
    public const int ListenBacklog = 31;

    /// <summary>20바이트 주소 +16의 TCP 프로토콜 비트다.</summary>
    public const byte TcpProtocolBit = 0x10;

    /// <summary>수락 노드의 pending 번호·소유자·버퍼·소켓·송수신 바이트를 하나로 묶는 구현 객체다.</summary>
    private sealed class Node
    {
        /// <summary>현재 번호다. 수락 직후에는 음수 pending, 배치 뒤에는 소유자다.</summary>
        public int Id;

        /// <summary>수락·접속이 든 OS 소켓이다. 없으면 null이다.</summary>
        public Socket? Socket;

        /// <summary>20바이트 상대 주소다.</summary>
        public byte[] Address = new byte[20];

        /// <summary>수신 미완성 바이트다.</summary>
        public readonly List<byte> Input = [];

        /// <summary>미전송 유선 바이트다.</summary>
        public readonly List<byte> Wire = [];

        /// <summary>버퍼 모드의 미확정 집합 payload다.</summary>
        public readonly List<byte> Aggregate = [];

        /// <summary>유선에서 이미 보낸 앞부분 길이다. would-block 때만 0이 아니다.</summary>
        public int Sent;

        /// <summary>수신한 첫 바이트 version이다.</summary>
        public byte Version = 0xff;

        /// <summary>마지막 OS 실패 코드다.</summary>
        public int Error;

        /// <summary>접속 진행 중인지다.</summary>
        public bool Connecting;

        /// <summary>접속이 끝났는지다.</summary>
        public bool Connected;

        /// <summary>연결 알림을 보냈는지다.</summary>
        public bool Notified;

        /// <summary>버퍼 모드인지다. 배치 뒤에만 참이다.</summary>
        public bool Buffered;

        /// <summary>닫힘 표시가 됐는지다.</summary>
        public bool Closed;

        /// <summary>물리 제거가 끝났는지다.</summary>
        public bool Retired;
    }

    /// <summary>번호로 실제 노드를 찾는 현재 연결 목록이다.</summary>
    private readonly Dictionary<int, Node> _nodes = new();

    /// <summary>다음 수락 pending 번호다. -1부터 -31까지 돈다.</summary>
    private int _nextPending = -1;

    /// <summary>실제 TCP 리스너 소켓이다. 없으면 null이다.</summary>
    private Socket? _listener;

    /// <summary>리스너가 묶은 실제 포트다.</summary>
    private uint _port;

    /// <summary>수신 프레임을 같은 등록표에 분배하는 경계다. 세션 연결 때 둔다.</summary>
    private LocalMissionTransport.FrameDispatch? _dispatch;

    /// <summary>마지막 OS 접속·수신·송신 실패 코드다.</summary>
    private int _lastError;

    /// <summary>Poll 진행 중인지다. 재진입을 거부한다.</summary>
    private bool _polling;

    /// <summary>이미 닫혔는지다.</summary>
    private bool _disposed;

    /// <summary>리틀 엔디언 값을 지정 폭으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int at, int width = 4)
    {
        if (at < 0 || width < 0 || at > bytes.Length || width > bytes.Length - at)
            throw new ArgumentOutOfRangeException(nameof(at), "TCP 유선 필드 읽기 범위 오류");
        uint value = 0;
        // 낮은 바이트부터 모은다.
        for (int index = 0; index < width; ++index)
            value |= (uint)bytes[at + index] << (8 * index);
        return value;
    }

    /// <summary>리틀 엔디언 값을 지정 폭으로 쓴다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int at, uint value, int width = 4)
    {
        if (at < 0 || width < 0 || at > bytes.Length || width > bytes.Length - at)
            throw new ArgumentOutOfRangeException(nameof(at), "TCP 유선 필드 쓰기 범위 오류");
        // 낮은 바이트부터 둔다.
        for (int index = 0; index < width; ++index)
            bytes[at + index] = unchecked((byte)(value >> (8 * index)));
    }

    /// <summary>폐기 종류인지 확인한다.</summary>
    /// <param name="type">패킷 종류 바이트다.</param>
    private static bool Retired(byte type) => type == RetiredType1 || type == RetiredType2;

    /// <summary>IPv4 숫자 주소를 4바이트로 푼다. 형식이 아니면 거부한다.</summary>
    /// <param name="text">점 표기 주소다.</param>
    private static byte[] ParseAddress(string text)
    {
        ArgumentNullException.ThrowIfNull(text);
        string[] parts = text.Split('.');
        if (parts.Length != 4)
            throw new ArgumentException("LAN TCP IPv4 숫자 주소가 필요합니다", nameof(text));
        var result = new byte[4];
        // 각 칸의 0~255 숫자 필드만 허용한다.
        for (int index = 0; index < 4; ++index)
        {
            if (!byte.TryParse(parts[index], out byte value))
                throw new ArgumentException("LAN TCP IPv4 필드 범위 오류", nameof(text));
            result[index] = value;
        }
        return result;
    }

    /// <summary>소켓 끝점의 IP·포트를 원문 20바이트 주소로 기록한다.</summary>
    /// <param name="address">기록할 IP다.</param>
    /// <param name="port">기록할 포트다.</param>
    private static byte[] AddressBytes(IPAddress address, int port)
    {
        var result = new byte[20];
        Put(result, 0, 2u, 2);
        result[2] = unchecked((byte)(port >> 8));
        result[3] = unchecked((byte)port);
        byte[] raw = address.MapToIPv4().GetAddressBytes();
        Array.Copy(raw, 0, result, 4, 4);
        result[16] = TcpProtocolBit;
        return result;
    }

    /// <summary>수신 프레임을 같은 등록표에 분배하는 경계를 둔다. 세션 연결 때 부른다.</summary>
    /// <param name="dispatch">수신 프레임을 분배하는 경계다.</param>
    public void RegisterDispatch(LocalMissionTransport.FrameDispatch dispatch)
    {
        _dispatch = dispatch ?? throw new ArgumentNullException(nameof(dispatch));
    }

    /// <summary>TCP 리스너만 연다. 바인드 주소는 와일드카드 또는 루프백 IPv4 숫자 주소다.</summary>
    /// <param name="port">묶을 포트다. 0이면 OS가 고른다.</param>
    /// <param name="bindAddress">바인드 주소다. 기본 와일드카드다.</param>
    public void Listen(uint port, string bindAddress = "0.0.0.0")
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_listener is not null)
            return;
        if (port > 65535u)
            throw new ArgumentOutOfRangeException(nameof(port), "LAN TCP 포트 범위 오류");
        ArgumentNullException.ThrowIfNull(bindAddress);
        IPAddress bind;
        if (bindAddress == "0.0.0.0")
        {
            bind = IPAddress.Any;
        }
        else
        {
            byte[] raw = ParseAddress(bindAddress);
            LocalNetworkPolicy.RequireLoopback(new IPAddress(raw).ToString());
            bind = new IPAddress(raw);
        }
        var listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        try
        {
            listener.Bind(new IPEndPoint(bind, (int)port));
            listener.Listen(ListenBacklog);
            listener.Blocking = false;
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
            listener.Dispose();
            throw new InvalidOperationException("LAN listen 실패 (소켓 " + _lastError + ")", exception);
        }
        _listener = listener;
        _port = (uint)((IPEndPoint)listener.LocalEndPoint!).Port;
    }

    /// <summary>수락·기존 연결은 보존하고 리스너 소켓만 중단한다.</summary>
    public void StopListening()
    {
        Socket? listener = _listener;
        if (listener is null)
            return;
        try
        {
            listener.Close();
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
            throw new InvalidOperationException("listener close 실패 (소켓 " + _lastError + ")", exception);
        }
        finally
        {
            _listener = null;
            _port = 0;
        }
    }

    /// <summary>기존 소켓 준비값이 1이어서 비어 있으면 1을 반환한다. 새 접속은 실패하면 거짓을 돌려주고 빈 범위는 null을 둔다.</summary>
    /// <param name="address">접속할 루프백 IPv4 숫자 주소 또는 호스트 이름이다.</param>
    /// <param name="port">접속할 포트다.</param>
    /// <param name="id">맡길 트랜시버 번호다. 기본 9다.</param>
    /// <param name="initialPacket">접속 뒤 처음에 보낼 패킷이다. 없으면 빈 범위다.</param>
    public bool Connect(string address, uint port, int id = 9, ReadOnlySpan<byte> initialPacket = default)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (id < 0 || id >= TransceiverSlots)
            throw new ArgumentOutOfRangeException(nameof(id), "LAN TCP 접속 ID는 실제 22칸 범위다");
        if (port == 0 || port > 65535u)
            throw new ArgumentOutOfRangeException(nameof(port), "LAN TCP 포트 범위 오류");
        if (_nodes.TryGetValue(id, out Node? current) && !current.Retired && current.Socket is not null)
            return true;
        ArgumentNullException.ThrowIfNull(address);
        IPAddress target = ResolveLoopback(address);
        var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        try
        {
            socket.Blocking = false;
            socket.Connect(target, (int)port);
        }
        catch (SocketException exception) when (exception.SocketErrorCode is SocketError.WouldBlock or SocketError.InProgress or SocketError.AlreadyInProgress)
        {
            // 비동기 접속 진행 중이다. Poll에서 완성한다.
        }
        catch (SocketException exception)
        {
            _lastError = exception.ErrorCode;
            socket.Dispose();
            return false;
        }
        var node = new Node
        {
            Id = id,
            Socket = socket,
            Address = AddressBytes(target, (int)port),
            Connecting = true,
        };
        _nodes[id] = node;
        if (!initialPacket.IsEmpty)
        {
            byte[] copy = initialPacket.ToArray();
            node.Wire.AddRange(copy);
        }
        return true;
    }

    /// <summary>호스트 이름을 첫 IPv4로 풀되 루프백만 허용한다. 숫자 주소는 점 파서로만 읽는다.</summary>
    /// <param name="address">풀 주소 문자열이다.</param>
    private static IPAddress ResolveLoopback(string address)
    {
        bool numeric = address.Length != 0;
        // 축약·8진 해석을 막으려 숫자 주소는 DNS 조회로 풀지 않는다.
        foreach (char value in address)
        {
            if ((value < '0' || value > '9') && value != '.')
                numeric = false;
        }
        if (numeric)
        {
            byte[] raw = ParseAddress(address);
            var literal = new IPAddress(raw);
            LocalNetworkPolicy.RequireLoopback(literal.ToString());
            return literal;
        }
        IPAddress[] resolved;
        try
        {
            resolved = Dns.GetHostAddresses(address);
        }
        catch (SocketException exception)
        {
            throw new ArgumentException("LAN TCP 호스트 조회 실패: " + address, nameof(address), exception);
        }
        // 첫 IPv4만 쓰며 LAN 범위를 적용한다.
        foreach (IPAddress candidate in resolved)
        {
            if (candidate.AddressFamily != AddressFamily.InterNetwork)
                continue;
            LocalNetworkPolicy.RequireLoopback(candidate.ToString());
            return candidate;
        }
        throw new ArgumentException("LAN TCP IPv4 호스트가 아니다: " + address, nameof(address));
    }

    /// <summary>같은 수락 노드를 owner 1~8로 옮기고 버퍼 모드로 바꾼다.</summary>
    /// <param name="pending">옮길 pending 번호다. 음수만 받는다.</param>
    /// <param name="owner">옮겨갈 소유자다.</param>
    public void AssignConnection(int pending, int owner)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (pending >= 0 || !_nodes.TryGetValue(pending, out Node? node) || node.Retired)
            throw new InvalidOperationException("LAN TCP 재배치의 현재 수락 노드 누락");
        _nodes.Remove(pending);
        node.Id = owner;
        node.Buffered = true;
        node.Aggregate.Clear();
        _nodes[owner] = node;
    }

    /// <summary>고른 닫힘 연결만 closed로 표시한다. 수신 깊이가 0이면 OS·목록도 바로 반납한다.</summary>
    /// <param name="id">닫을 번호다.</param>
    public void Close(int id)
    {
        if (_nodes.TryGetValue(id, out Node? node) && !node.Retired)
        {
            node.Closed = true;
            CloseReady(node);
        }
    }

    /// <summary>닫힌 리스너와 모든 닫힘 노드를 닫는다. 다른 끝·등록 공유는 건드리지 않는다.</summary>
    public void CloseAll()
    {
        foreach (Node node in _nodes.Values)
            node.Closed = true;
        foreach (Node node in _nodes.Values)
            CloseReady(node);
        StopListening();
    }

    /// <summary>수신 콜백이 끝난 뒤에만 실제 노드를 물리 제거한다.</summary>
    /// <param name="node">정리할 노드다.</param>
    private void CloseReady(Node node)
    {
        if (node.Retired || !node.Closed)
            return;
        CloseSocket(node);
        node.Retired = true;
    }

    /// <summary>OS 소켓만 닫고 닫힘 노드 표시는 호출자가 지킨다.</summary>
    /// <param name="node">닫을 노드다.</param>
    private void CloseSocket(Node node)
    {
        Socket? socket = node.Socket;
        node.Socket = null;
        node.Connected = false;
        node.Connecting = false;
        if (socket is null)
            return;
        try
        {
            socket.Close();
        }
        catch (SocketException exception)
        {
            node.Error = _lastError = exception.ErrorCode;
            throw new InvalidOperationException("TCP close 실패 (소켓 " + node.Error + ")", exception);
        }
    }

    /// <summary>선언 길이를 확정한 writer의 앞부분 WORD 바이트만 현재 닫힘 연결로 보낸다.</summary>
    /// <param name="destination">받을 번호다.</param>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    public bool Send(int destination, ReadOnlySpan<byte> packet)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (packet.Length < HeaderSize)
            throw new ArgumentOutOfRangeException(nameof(packet), "LAN TCP 송신 헤더 부족");
        if (Retired(packet[1]))
            return false;
        if (!_nodes.TryGetValue(destination, out Node? node) || node.Retired || node.Closed || node.Socket is null)
            return false;
        uint length = Read(packet, 2, 2);
        if (length < (uint)HeaderSize || length > (uint)packet.Length)
            throw new ArgumentOutOfRangeException(nameof(packet), "LAN TCP 송신 선언 길이 범위 오류");
        if (RetiredFrame(packet))
            return false;
        SendNode(node, packet[..(int)length]);
        return !node.Retired && !node.Closed;
    }

    /// <summary>집합 프레임을 검사해 폐기 종류가 섞였으면 거부한다.</summary>
    /// <param name="packet">검사할 패킷이다.</param>
    private static bool RetiredFrame(ReadOnlySpan<byte> packet)
    {
        if (Retired(packet[1]))
            return true;
        if (packet[1] != AggregateType)
            return false;
        uint total = Read(packet, 2, 2);
        if (total < (uint)HeaderSize || total > (uint)packet.Length)
            throw new ArgumentOutOfRangeException(nameof(packet), "LAN TCP 집합 송신 선언 길이 범위 오류");
        int at = HeaderSize;
        // 현재 집합 안의 개별 패킷 경계만 센다.
        while (at < (int)total)
        {
            if (total - (uint)at < (uint)HeaderSize)
                throw new ArgumentOutOfRangeException(nameof(packet), "LAN TCP 집합 송신 개별 헤더 부족");
            uint length = Read(packet, at + 2, 2);
            if (length < (uint)HeaderSize || length > total - (uint)at)
                throw new ArgumentOutOfRangeException(nameof(packet), "LAN TCP 집합 송신 개별 길이 오류");
            if (Retired(packet[at + 1]))
                return true;
            if (packet[at + 1] == AggregateType)
                throw new InvalidOperationException("LAN TCP 중첩 집합 송신 미지원");
            at += (int)length;
        }
        return false;
    }

    /// <summary>버퍼 payload는 집합 확정까지 보관하고 비버퍼는 부분 송신 꼬리를 남긴다.</summary>
    /// <param name="node">보낼 노드다.</param>
    /// <param name="packet">선언 길이만큼의 패킷이다.</param>
    private void SendNode(Node node, ReadOnlySpan<byte> packet)
    {
        if (node.Buffered)
        {
            if (node.Aggregate.Count + packet.Length + HeaderSize > WireLimit)
            {
                Finalize(node);
                Drain(node);
                if (node.Retired || node.Closed)
                    return;
            }
            if (packet.Length + HeaderSize > WireLimit)
            {
                Fail(node, 0);
                return;
            }
            node.Aggregate.AddRange(packet.ToArray());
        }
        else
        {
            Compact(node);
            if (node.Wire.Count + packet.Length > WriteBacklog)
            {
                Fail(node, 0);
                return;
            }
            node.Wire.AddRange(packet.ToArray());
            Drain(node);
        }
    }

    /// <summary>0xff version·종류4·전체WORD를 payload 앞에 붙여 유선 순서에 보관한다.</summary>
    /// <param name="node">확정할 노드다.</param>
    private void Finalize(Node node)
    {
        if (node.Aggregate.Count == 0)
            return;
        Compact(node);
        if (node.Aggregate.Count + HeaderSize > WireLimit || node.Wire.Count + node.Aggregate.Count + HeaderSize > WriteBacklog)
        {
            Fail(node, 0);
            return;
        }
        int start = node.Wire.Count;
        node.Wire.AddRange([(byte)0xff, AggregateType, 0, 0]);
        Put(CollectionsMarshal.AsSpan(node.Wire), start + 2, (uint)(node.Aggregate.Count + HeaderSize), 2);
        node.Wire.AddRange(node.Aggregate);
        node.Aggregate.Clear();
    }

    /// <summary>부분 송신이 이미 보낸 앞부분만 지워 같은 미전송 데이터의 순서를 보존한다.</summary>
    /// <param name="node">정리할 노드다.</param>
    private static void Compact(Node node)
    {
        if (node.Sent != 0)
        {
            node.Wire.RemoveRange(0, node.Sent);
            node.Sent = 0;
        }
    }

    /// <summary>실제 접속 send만큼만 앞부분을 띄우고 would-block에서 현재 바이트 순서를 지킨다.</summary>
    /// <param name="node">비울 노드다.</param>
    private void Drain(Node node)
    {
        if (node.Retired || !node.Connected || node.Closed || node.Socket is null)
            return;
        // 매 연결의 현재 확정 프레임을 OS가 받는 만큼만 전진시킨다.
        while (node.Sent < node.Wire.Count)
        {
            int amount;
            try
            {
                amount = node.Socket.Send(node.Wire.ToArray(), node.Sent, node.Wire.Count - node.Sent, SocketFlags.None);
            }
            catch (SocketException exception) when (exception.SocketErrorCode == SocketError.WouldBlock)
            {
                return;
            }
            catch (SocketException exception)
            {
                Fail(node, exception.ErrorCode);
                return;
            }
            if (amount == 0)
            {
                Fail(node, 0);
                return;
            }
            node.Sent += amount;
        }
        node.Wire.Clear();
        node.Sent = 0;
    }

    /// <summary>같은 노드 종료 알림 없이 명시 종료한다.</summary>
    /// <param name="node">끝낼 노드다.</param>
    /// <param name="error">기록할 OS 실패 코드다. 0이면 실패를 새로 기록하지 않는다.</param>
    private void Fail(Node node, int error)
    {
        if (node.Retired)
            return;
        if (error != 0)
        {
            node.Error = _lastError = error;
            Notify(node, NotifyTerminateType);
        }
        else if (!node.Closed)
        {
            Notify(node, NotifyTerminateType);
        }
        node.Closed = true;
        CloseReady(node);
    }

    /// <summary>현재 부분 수신뿐인 직접 분배만 하고 무효·sentinel이면 같은 Poll에 추가 분배를 멈춘다.</summary>
    /// <param name="node">알릴 노드다.</param>
    /// <param name="type">알림 종류다.</param>
    private void Notify(Node node, byte type)
    {
        byte[] frame = [node.Version, type, 4, 0];
        _dispatch?.Invoke(frame);
    }

    /// <summary>실제 Poll에서만 접속·종료 깊이를 올린 뒤 상위 상태 조회에서 닫힌 소켓을 지운다.</summary>
    public void BeginPoll()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_polling)
            throw new InvalidOperationException("LAN TCP Poll 중복 진입 금지");
        _polling = true;
        try
        {
            foreach (Node node in _nodes.Values)
                CloseReady(node);
        }
        catch
        {
            _polling = false;
            throw;
        }
    }

    /// <summary>실제 TCP 리스너의 accept만 수행한다. 생성 뒤 pending을 만들지 않는다.</summary>
    public void AcceptPending()
    {
        if (!_polling)
            throw new InvalidOperationException("TCP accept는 Poll 대기 필요");
        Accept();
    }

    /// <summary>리스너에 쌓인 접속을 pending 31칸이 허락하는 만큼만 수락한다.</summary>
    private void Accept()
    {
        if (_listener is null)
            return;
        // 현재 시점에 쌓인 만큼만 비동기로 받는다.
        while (CountPending() < PendingSlots)
        {
            Socket accepted;
            try
            {
                accepted = _listener.Accept();
            }
            catch (SocketException exception) when (exception.SocketErrorCode == SocketError.WouldBlock)
            {
                return;
            }
            catch (SocketException exception)
            {
                _lastError = exception.ErrorCode;
                return;
            }
            var remote = (IPEndPoint)accepted.RemoteEndPoint!;
            if (!IsLoopback(remote.Address))
            {
                accepted.Dispose();
                continue;
            }
            accepted.Blocking = false;
            var node = new Node
            {
                Id = _nextPending,
                Socket = accepted,
                Address = AddressBytes(remote.Address, remote.Port),
                Connected = true,
            };
            _nodes[_nextPending] = node;
            _nextPending = _nextPending <= -PendingSlots ? -1 : _nextPending - 1;
        }
    }

    /// <summary>현재 pending 노드 수를 센다.</summary>
    private int CountPending()
    {
        int count = 0;
        // 음수 번호의 살아 있는 노드만 센다.
        foreach (int id in _nodes.Keys)
        {
            if (id < 0 && !_nodes[id].Retired)
                ++count;
        }
        return count;
    }

    /// <summary>루프백 주소인지 확인한다.</summary>
    /// <param name="address">확인할 주소다.</param>
    private static bool IsLoopback(IPAddress address) => IPAddress.IsLoopback(address.MapToIPv4());

    /// <summary>같은 닫힘 ID의 현재 부분 수신과 미전송 송신만 전진시킨다.</summary>
    /// <param name="id">받을 번호다.</param>
    /// <returns>소비한 프레임 수와 계속 여부다.</returns>
    public (int Consumed, bool Again) ReceiveOne(int id)
    {
        if (!_polling)
            throw new InvalidOperationException("TCP per-ID 수신은 Poll 대기 필요");
        if (!_nodes.TryGetValue(id, out Node? node) || node.Retired)
            return (0, false);
        bool consumed = false;
        bool again = ReceiveOne(node, ref consumed);
        Drain(node);
        return (consumed ? 1 : 0, again);
    }

    /// <summary>4바이트 헤더와 선언 길이를 TCP 스트림에서 정확히 프레임만 모은다.</summary>
    /// <param name="node">받을 노드다.</param>
    /// <param name="consumed">완성 프레임을 소비했는지다.</param>
    private bool ReceiveOne(Node node, ref bool consumed)
    {
        if (!CompleteConnection(node))
            return false;
        Drain(node);
        if (node.Retired || node.Closed)
            return false;
        // 헤더·본문이 모자라면 현재 부분 버퍼를 다음 Poll에 그대로 둔다.
        for (; ; )
        {
            int required = HeaderSize;
            if (node.Input.Count >= HeaderSize)
            {
                required = (int)Read(CollectionsMarshal.AsSpan(node.Input), 2, 2);
                if (required < HeaderSize || required > WireLimit)
                {
                    Fail(node, 0);
                    return false;
                }
            }
            if (node.Input.Count < required)
            {
                int before = node.Input.Count;
                for (int index = before; index < required; ++index)
                    node.Input.Add(0);
                int count;
                try
                {
                    count = node.Socket!.Receive(node.Input.ToArray(), before, required - before, SocketFlags.None);
                }
                catch (SocketException exception) when (exception.SocketErrorCode == SocketError.WouldBlock)
                {
                    node.Input.RemoveRange(before, required - before);
                    return false;
                }
                catch (SocketException exception)
                {
                    node.Input.RemoveRange(before, required - before);
                    Fail(node, exception.ErrorCode);
                    return false;
                }
                if (count == 0)
                {
                    node.Input.RemoveRange(before, required - before);
                    Fail(node, 0);
                    return false;
                }
                node.Input.RemoveRange(before + count, required - before - count);
                if (node.Input.Count < required)
                    return false;
                if (required == HeaderSize)
                    continue;
            }
            byte[] frame = node.Input.GetRange(0, required).ToArray();
            node.Input.RemoveRange(0, required);
            consumed = true;
            node.Version = frame[0];
            if (Retired(frame[1]))
                return true;
            if (_dispatch?.Invoke(frame) != true)
                return false;
            if (node.Retired || node.Closed)
            {
                CloseReady(node);
                return false;
            }
            return true;
        }
    }

    /// <summary>접속 진행을 끝내고 첫 연결 알림을 한 번만 보낸다.</summary>
    /// <param name="node">완성할 노드다.</param>
    private bool CompleteConnection(Node node)
    {
        if (node.Retired || node.Closed)
            return false;
        if (node.Socket is null)
        {
            Fail(node, node.Error);
            return false;
        }
        if (node.Connecting)
        {
            bool writable;
            try
            {
                writable = node.Socket.Poll(0, SelectMode.SelectWrite);
            }
            catch (SocketException exception)
            {
                Fail(node, exception.ErrorCode);
                return false;
            }
            if (!writable)
                return false;
            // 접속 실패는 소켓 오류로만 확인한다.
            try
            {
                _ = node.Socket.RemoteEndPoint;
            }
            catch (SocketException exception)
            {
                Fail(node, exception.ErrorCode);
                return false;
            }
            node.Connecting = false;
            node.Connected = true;
        }
        if (!node.Notified)
        {
            node.Notified = true;
            Notify(node, NotifyConnectType);
        }
        return !node.Retired && !node.Closed;
    }

    /// <summary>실제 정상 22칸의 현재 순서대로 먼저 직접 수신한 pending과 버퍼 flush가 아닌지 확인한다.</summary>
    public void FlushOne(int id)
    {
        if (id < 0 || id >= TransceiverSlots)
            throw new ArgumentOutOfRangeException(nameof(id), "TCP flush 실제 ID 범위 오류");
        if (_nodes.TryGetValue(id, out Node? node))
        {
            Finalize(node);
            Drain(node);
        }
    }

    /// <summary>단독 TCP 사용을 같은 실제 0~21 조회로 callback 없이 현재 ID를 다시 쓴다.</summary>
    public void Flush()
    {
        // 모든 실제 노드를 고르지 않는다.
        for (int id = 0; id < TransceiverSlots; ++id)
            FlushOne(id);
    }

    /// <summary>callback·송신 이후의 최외곽에서만 실제 노드를 물리 제거한다.</summary>
    public void EndPoll()
    {
        if (!_polling)
            throw new InvalidOperationException("TCP Poll 대기가 아니다");
        _polling = false;
        var retired = new List<int>();
        // 물리 제거는 Poll 끝에서만 수행한다.
        foreach (var pair in _nodes)
        {
            if (pair.Value.Retired)
                retired.Add(pair.Key);
        }
        foreach (int id in retired)
            _nodes.Remove(id);
    }

    /// <summary>기존 단독 진입을 per-ID 경계만 조립해 실제 리스너·대기·일반 순서로 돈다.</summary>
    public int Poll()
    {
        BeginPoll();
        int count = 0;
        try
        {
            AcceptPending();
            // 각 현재 pending은 한 번만 받는다.
            for (int id = -1; id > -PendingSlots - 1; --id)
            {
                (int consumed, _) = ReceiveOne(id);
                count += consumed;
            }
            // 재배치 반복 뒤 현재 노드를 다시 고른다.
            for (int id = 0; id < TransceiverSlots; ++id)
            {
                for (; ; )
                {
                    (int consumed, bool again) = ReceiveOne(id);
                    count += consumed;
                    if (!again)
                        break;
                }
            }
        }
        catch
        {
            _polling = false;
            throw;
        }
        EndPoll();
        return count;
    }

    /// <summary>버퍼·접속 완료는 보존하고 객체 존재만 확인한다.</summary>
    /// <param name="id">확인할 번호다.</param>
    public bool HasConnection(int id) =>
        _nodes.TryGetValue(id, out Node? node) && !node.Retired;

    /// <summary>실제 소켓 존재·상위 closed 미설정인지 확인한다. 접속 진행 상태는 구분한다.</summary>
    /// <param name="id">확인할 번호다.</param>
    public bool IsOpen(int id) =>
        _nodes.TryGetValue(id, out Node? node) && !node.Retired && node.Socket is not null && !node.Closed;

    /// <summary>같은 실제 소켓 준비 조회 0/1/-1을 그대로 반환한다. 이전 오류·연결 값을 캐시하지 않는다.</summary>
    /// <param name="id">조회할 번호다.</param>
    public int Status(int id)
    {
        if (!_nodes.TryGetValue(id, out Node? node) || node.Retired || node.Socket is null)
            return 0;
        bool error;
        bool writable;
        try
        {
            writable = node.Socket.Poll(0, SelectMode.SelectWrite);
            error = node.Socket.Poll(0, SelectMode.SelectError);
        }
        catch (SocketException exception)
        {
            node.Error = _lastError = exception.ErrorCode;
            return -1;
        }
        if (error)
            return -1;
        return writable ? 1 : 0;
    }

    /// <summary>20바이트 상대 주소 또는 0.0.0.0:0의 AF_INET/TCP 서명을 돌려준다.</summary>
    /// <param name="id">조회할 번호다.</param>
    public byte[] Address(int id)
    {
        if (_nodes.TryGetValue(id, out Node? node) && !node.Retired)
            return (byte[])node.Address.Clone();
        var result = new byte[20];
        Put(result, 0, 2u, 2);
        result[16] = TcpProtocolBit;
        return result;
    }

    /// <summary>실제 리스너 소켓 존재를 반환한다.</summary>
    public bool IsListening() => _listener is not null;

    /// <summary>묶은 현재 리스너 포트만 공개한다. 리스너가 없으면 0이다.</summary>
    public uint ListeningPort() => _port;

    /// <summary>마지막 OS 접속·수신·송신 실패 코드를 UI 진단으로 반환한다.</summary>
    public int LastError() => _lastError;

    /// <summary>직접 OS에 도달하지 않은 닫힘 송신 바이트만 센다.</summary>
    public long QueuedBytes()
    {
        long total = 0;
        // 반납 완료 노드는 과거 바이트를 세지 않는다.
        foreach (Node node in _nodes.Values)
        {
            if (!node.Retired)
                total += node.Wire.Count - node.Sent + node.Aggregate.Count;
        }
        return total;
    }

    /// <summary>현재 호스트 수명에서 없어지는 자기 노드·소켓을 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed)
            return;
        _disposed = true;
        foreach (Node node in _nodes.Values)
        {
            try
            {
                node.Socket?.Close();
            }
            catch (SocketException)
            {
            }
            node.Socket = null;
        }
        _nodes.Clear();
        try
        {
            _listener?.Close();
        }
        catch (SocketException)
        {
        }
        _listener = null;
        _polling = false;
    }
}
