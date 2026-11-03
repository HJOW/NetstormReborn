namespace Netstorm.Net;

/// <summary>
/// 단독 미션의 Zone·로그인 바이트와 로컬 두 연결을 같은 패킷 분배기에 연결한다
/// (cpppj <c>src/zacket/LocalMissionTransport.h</c>의 `LocalMissionPacket`·`LocalMissionLoginView`·`LocalMissionTransport` 골격).
/// </summary>
/// <remarks>2026-11-03 멀티플레이 사전 구현에서 추가했다. 테스트 없이 cpppj와 원본 디컴파일만 참고해 골격만 만든다. 세션 연결(F3)에서 실제 Raw 분배기에 연결한다.</remarks>
public sealed class LocalMissionTransport : IDisposable
{
    /// <summary>원본 예약 전체와 observer가 확정한 선언 길이를 함께 보관한다.</summary>
    public sealed class LocalMissionPacket
    {
        /// <summary>Zone 512·로그인 Fort 이미지+67의 전체 물리 바이트다.</summary>
        public byte[] Storage { get; set; } = [];

        /// <summary>
        /// 실제 송신·State 이미지 복사가 사용하는 현재 선언 WORD 길이만 빌린다.
        /// </summary>
        /// <returns>선언 길이만큼의 읽기 뷰 Stabil다.</returns>
        /// <exception cref="ArgumentOutOfRangeException">선언 길이가 예약을 넘는다.</exception>
        public ReadOnlySpan<byte> Bytes()
        {
            if (Storage.Length < 4)
            {
                throw new ArgumentOutOfRangeException(nameof(Storage), "로컬 미션 패킷 헤더 부족");
            }
            uint length = (uint)(Storage[2] | (Storage[3] << 8));
            if (length > (uint)Storage.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(Storage), "로컬 미션 패킷 선언 길이 초과");
            }
            return new ReadOnlySpan<byte>(Storage, 0, (int)length);
        }
    }

    /// <summary>
    /// 현재 0x27 버퍼의 필드 참조이며 처리기 호출 동안만 유효하다.
    /// </summary>
    /// <param name="Subscriber">+8의 실제 subscriber DWORD다.</param>
    /// <param name="Handle">+17의 현재 NUL 문자열이다.</param>
    /// <param name="FortImage">+17+이름 길이에서 fort 파일 크기만큼의 실제 파일 이미지다.</param>
    public readonly record struct LocalMissionLoginView(uint Subscriber, string Handle, byte[] FortImage);

    /// <summary>실제 0x27 패킷을 같은 큐에 넣는 송신 경계다.</summary>
    /// <param name="destination">받을 소유자 번호다.</param>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    /// <returns>받아들였으면 참이다.</returns>
    public delegate bool PacketSend(int destination, ReadOnlySpan<byte> packet);

    /// <summary>같은 실제 분배기에 건설·준비·서버 응답 처리기를 더하는 경계다.</summary>
    /// <param name="frame">수신한 한 프레임이다.</param>
    /// <returns>분배했으면 참이다.</returns>
    public delegate bool FrameDispatch(ReadOnlySpan<byte> frame);

    /// <summary>서버로 가는 완성 프레임의 소유 큐다.</summary>
    private readonly Queue<byte[]> _toServer = new();

    /// <summary>클라이언트로 가는 완성 프레임의 소유 큐다.</summary>
    private readonly Queue<byte[]> _toClient = new();

    /// <summary>실제 두 끝의 열림·서버 pending·buffered 모드다.</summary>
    private bool _open;

    /// <summary>서버 측이 pending-1로 등록됐는지다.</summary>
    private bool _pending;

    /// <summary>서버 측 pending 등록이 남아 있는지다.</summary>
    /// <returns>접속 전이면 참이다.</returns>
    public bool HasPending()
    {
        return _pending;
    }

    /// <summary>현재 owner·열린 로컬 포트다.</summary>
    private uint _owner;

    /// <summary>실제 열린 로컬 포트다.</summary>
    private uint _port;

    /// <summary>수신 프레임을 같은 등록표에 분배하는 경계다.</summary>
    private FrameDispatch? _dispatch;

    /// <summary>이미 닫혔는지다.</summary>
    private bool _disposed;

    /// <summary>
    /// 같은 Player9와 서버 전역·실제 분배 효과를 빌린다. hooks의 모든 경계는 실제 소비자여야 한다.
    /// 세션 연결 전까지 분배기는 비워 둔다.
    /// </summary>
    public LocalMissionTransport()
    {
    }

    /// <summary>
    /// 원본 zlocal 서버·접속 두 끝을 열고 서버 측을 pending-1로 등록한다. 포트는 실제 게임 설정값이다.
    /// 외부 주소 바인딩은 거부하고 루프백·리슨만 허용한다.
    /// </summary>
    /// <param name="host">바인딩할 호스트다. 루프백·리슨만 허용한다.</param>
    /// <param name="port">열 로컬 포트다.</param>
    /// <param name="connectImmediately">바로 접속할지다.</param>
    /// <exception cref="ArgumentException">외부 주소다.</exception>
    public void Open(string host, uint port, bool connectImmediately = true)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        LocalNetworkPolicy.RequireLoopback(host);
        _open = true;
        _pending = true;
        _port = port;
        if (connectImmediately)
        {
            Connect();
        }
    }

    /// <summary>이미 열린 로컬 서버에 실제 클라이언트 끝을 접속한다.</summary>
    /// <exception cref="InvalidOperationException">먼저 열지 않았다.</exception>
    public void Connect()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (!_open)
        {
            throw new InvalidOperationException("로컬 전송을 먼저 열어야 한다");
        }
        _pending = false;
    }

    /// <summary>실제 pending-1을 같은 연결 객체 owner로 이동한다.</summary>
    /// <param name="pending">옮길 pending 번호다. -1만 받는다.</param>
    /// <param name="owner">옮겨갈 소유자다.</param>
    /// <exception cref="ArgumentOutOfRangeException">pending이 -1이 아니다.</exception>
    public void AssignConnection(int pending, int owner)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (pending != -1)
        {
            throw new ArgumentOutOfRangeException(nameof(pending), "로컬 pending 이동은 -1만 받는다");
        }
        _owner = unchecked((uint)owner);
    }

    /// <summary>현재 flags·NULL 연결 선택 뒤 선언 길이 바이트를 실제 다른 끝의 송신 큐에 쓴다.</summary>
    /// <param name="destination">받을 소유자 번호다.</param>
    /// <param name="packet">선언 길이만큼 보낼 패킷이다.</param>
    /// <returns>받아들였으면 참이다.</returns>
    /// <exception cref="InvalidOperationException">열지 않았다.</exception>
    public bool Send(int destination, ReadOnlySpan<byte> packet)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (!_open)
        {
            throw new InvalidOperationException("로컬 전송을 먼저 열어야 한다");
        }
        // 목적지 검사는 실제 분배기가 세션 연결 때 맡는다
        byte[] copy = packet.ToArray();
        if (destination == (int)_owner)
        {
            _toClient.Enqueue(copy);
        }
        else
        {
            _toServer.Enqueue(copy);
        }
        return true;
    }

    /// <summary>실제 endpoint의 존재를 검증한다.</summary>
    /// <returns>열려 있으면 참이다.</returns>
    public bool IsOpen()
    {
        return _open && !_disposed;
    }

    /// <summary>현재 owner를 돌려준다.</summary>
    /// <returns>pending 재배치 뒤의 현재 owner다.</returns>
    public uint Owner()
    {
        return _owner;
    }

    /// <summary>열린 로컬 포트를 돌려준다.</summary>
    /// <returns>실제 열린 로컬 포트다.</returns>
    public uint Port()
    {
        return _port;
    }

    /// <summary>받아둘 프레임 수를 돌려준다.</summary>
    /// <returns>두 큐에 남은 완성 프레임 수다.</returns>
    public int QueuedFrames()
    {
        return _toServer.Count + _toClient.Count;
    }

    /// <summary>같은 실제 분배기에 처리기를 더한다.</summary>
    /// <param name="dispatch">수신 프레임을 분배하는 경계다.</param>
    public void RegisterDispatch(FrameDispatch dispatch)
    {
        _dispatch = dispatch ?? throw new ArgumentNullException(nameof(dispatch));
    }

    /// <summary>등록된 연결을 번호순으로 읽고 같은 등록표에 분배한다.</summary>
    /// <returns>분배한 프레임 수다.</returns>
    public int Poll()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        int count = 0;
        // pending을 먼저 한 번 읽고 등록된 연결을 번호순으로 분배한다
        while (_toServer.Count > 0)
        {
            byte[] frame = _toServer.Dequeue();
            if (_dispatch?.Invoke(frame) == true)
            {
                count++;
            }
        }
        while (_toClient.Count > 0)
        {
            byte[] frame = _toClient.Dequeue();
            if (_dispatch?.Invoke(frame) == true)
            {
                count++;
            }
        }
        return count;
    }

    /// <summary>현재 호스트 수명에서 없어지는 자기 양끝의 남은 별칭·장치를 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }
        _disposed = true;
        _open = false;
        _toServer.Clear();
        _toClient.Clear();
    }
}
