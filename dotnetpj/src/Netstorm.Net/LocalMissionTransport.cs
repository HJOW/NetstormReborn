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

/// <summary>
/// 단독 미션의 Zone·로그인 바이트 작성과 로그인 뷰 판독을 공유한다
/// (cpppj <c>src/zacket/LocalMissionTransport.cpp</c>의 `MakeLocalMissionZone`·`MakeGuideMissionZone`·`MakeLocalMissionLogin`·`ReadLocalMissionLogin`).
/// </summary>
/// <remarks>2026-11-07 멀티플레이 사전 구현에서 추가했다. 테스트 없이 cpppj와 원본 디컴파일만 참고해 먼저 만든다. 설치·분배기 연결(`InstallLocalMissionZone`·`Dispatch`)은 세션 연결(F3)에서 잇는다.</remarks>
public static class LocalMissionPackets
{
    /// <summary>Zone 예약 바이트 수다.</summary>
    public const int ZoneReserve = 512;

    /// <summary>Zone 한 슬롯의 바이트 수다.</summary>
    public const int ZoneClient = 71;

    /// <summary>Zone 첫 슬롯의 위치다.</summary>
    public const int ZoneHead = 15;

    /// <summary>Zone 패킷 종류 바이트다.</summary>
    public const byte ZoneType = 0x1a;

    /// <summary>로그인 패킷 종류 바이트다.</summary>
    public const byte LoginType = 0x27;

    /// <summary>패킷 버전 바이트다.</summary>
    public const byte PacketVersion = 1;

    /// <summary>004b51e0의 실제 네 정적 이름이다.</summary>
    private static readonly string[] ZoneNames = ["zsimpson", "jgreer", "kdemarest", "tbratton"];

    /// <summary>원본 비정렬 little endian 값을 지정 폭으로 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        if (offset < 0 || offset > bytes.Length || width < 0 || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 원문 읽기 범위 오류");
        }
        uint value = 0;
        // 낮은 바이트부터 같은 폭의 원본 비트를 모은다
        for (int index = 0; index < width; ++index)
        {
            value |= (uint)bytes[offset + index] << (8 * index);
        }
        return value;
    }

    /// <summary>주변 stale 바이트를 유지하며 지정 원본 폭만 쓴다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        if (offset < 0 || offset > bytes.Length || width < 0 || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 원문 쓰기 범위 오류");
        }
        // 원본 little endian의 바이트 위치만 변경한다
        for (int index = 0; index < width; ++index)
        {
            bytes[offset + index] = unchecked((byte)(value >> (8 * index)));
        }
    }

    /// <summary>원본 C문자열의 첫 NUL까지만 읽는다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static string Text(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset < 0 || offset > bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 문자열 위치 오류");
        }
        int end = offset;
        // 끝까지 NUL이 없으면 호스트 밖 읽기로 보고 거부한다
        while (end < bytes.Length && bytes[end] != 0)
        {
            ++end;
        }
        if (end >= bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 원문 문자열 NUL 누락");
        }
        return System.Text.Encoding.Latin1.GetString(bytes[offset..end]);
    }

    /// <summary>원본 strcpy처럼 NUL까지 복사한다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="text">쓸 문자열이다.</param>
    private static void Copy(Span<byte> bytes, int offset, string text)
    {
        int nul = text.IndexOf('\0');
        if (nul >= 0)
        {
            text = text.Substring(0, nul);
        }
        byte[] encoded = System.Text.Encoding.Latin1.GetBytes(text);
        if (offset < 0 || offset >= bytes.Length || encoded.Length >= bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "로컬 미션 strcpy 물리 저장소 부족");
        }
        // 선언 필드 폭을 넘어도 실제 물리 저장소 안에서는 원본 쓰기를 유지한다
        encoded.CopyTo(bytes[offset..]);
        bytes[offset + encoded.Length] = 0;
    }

    /// <summary>004df590의 전체 0xff 예약·종류·버전·선언 길이 헤더로 패킷을 만든다.</summary>
    /// <param name="reserve">예약 바이트 수다.</param>
    /// <param name="type">패킷 종류다.</param>
    private static LocalMissionTransport.LocalMissionPacket Packet(int reserve, byte type)
    {
        var result = new LocalMissionTransport.LocalMissionPacket
        {
            Storage = new byte[reserve],
        };
        Array.Fill(result.Storage, (byte)0xff);
        result.Storage[0] = PacketVersion;
        result.Storage[1] = type;
        Put(result.Storage, 2, 4u, 2);
        return result;
    }

    /// <summary>PATCH 004b51e0의 Zone 512·네 슬롯 작성과 86바이트 길이 확정이다.</summary>
    /// <returns>선언 길이가 확정된 Zone 패킷이다.</returns>
    public static LocalMissionTransport.LocalMissionPacket MakeLocalMissionZone()
    {
        LocalMissionTransport.LocalMissionPacket packet = Packet(ZoneReserve, ZoneType);
        Span<byte> bytes = packet.Storage;
        Put(bytes, 4, 0x82000004u);
        Put(bytes, 8, 0u, 2);
        Put(bytes, 10, 0u);
        bytes[14] = 1;
        // 004b51e0의 네 번 쓰기를 보존한다. State에 복제되는 길이는 마지막에 첫 슬롯 하나로 확정된다
        for (int index = 0; index < ZoneNames.Length; ++index)
        {
            int slot = ZoneHead + index * ZoneClient;
            bytes[slot] = unchecked((byte)index);
            Put(bytes, slot + 1, (uint)(index + 1));
            Put(bytes, slot + 5, 0u);
            Copy(bytes, slot + 9, ZoneNames[index]);
            bytes[slot + 41] = 0;
        }
        Put(bytes, 2, (uint)(ZoneHead + bytes[14] * ZoneClient), 2);
        return packet;
    }

    /// <summary>004b50b0의 같은 Zone 표식과 한 슬롯의 현재 handle 작성이다. 나머지 예약 0xff를 보존한다.</summary>
    /// <param name="handle">쓸 안내 handle이다.</param>
    /// <returns>선언 길이 4를 유지하는 안내 Zone 패킷이다.</returns>
    public static LocalMissionTransport.LocalMissionPacket MakeGuideMissionZone(string handle)
    {
        LocalMissionTransport.LocalMissionPacket packet = Packet(ZoneReserve, ZoneType);
        Span<byte> bytes = packet.Storage;
        Put(bytes, 4, 0x82000004u);
        Put(bytes, 8, 0u, 2);
        Put(bytes, 10, 0u);
        bytes[14] = 1;
        bytes[ZoneHead] = 0;
        Put(bytes, ZoneHead + 1, 1u);
        Put(bytes, ZoneHead + 5, 0u);
        Copy(bytes, ZoneHead + 9, handle);
        bytes[ZoneHead + 41] = 0;
        return packet;
    }

    /// <summary>PATCH 004b63d0의 실제 외부 Fort 이미지 이후 패킷 작성과 0040a450의 길이 확정이다. Template 읽기·파일 선택은 상위 원본 부모의 책임이다.</summary>
    /// <param name="subscriber">쓸 subscriber DWORD다.</param>
    /// <param name="handle">쓸 handle이다.</param>
    /// <param name="fortImage">뒤에 붙일 Fort 파일 이미지다.</param>
    /// <returns>선언 길이가 확정된 로그인 패킷이다.</returns>
    public static LocalMissionTransport.LocalMissionPacket MakeLocalMissionLogin(uint subscriber, string handle, ReadOnlySpan<byte> fortImage)
    {
        int nul = handle.IndexOf('\0');
        if (nul >= 0)
        {
            handle = handle.Substring(0, nul);
        }
        if ((ulong)fortImage.Length > uint.MaxValue - 67ul)
        {
            throw new ArgumentOutOfRangeException(nameof(fortImage), "로컬 로그인 이미지 크기 초과");
        }
        LocalMissionTransport.LocalMissionPacket packet = Packet(fortImage.Length + 67, LoginType);
        Span<byte> bytes = packet.Storage;
        Put(bytes, 8, subscriber);
        byte[] encoded = System.Text.Encoding.Latin1.GetBytes(handle);
        bytes[12] = unchecked((byte)(encoded.Length + 1));
        Copy(bytes, 17, handle);
        Put(bytes, 13, (uint)fortImage.Length);
        int start = 17 + bytes[12];
        if (start < 0 || start > bytes.Length || fortImage.Length > bytes.Length - start)
        {
            throw new ArgumentOutOfRangeException(nameof(fortImage), "로컬 로그인 실제 이미지 복사 범위 초과");
        }
        fortImage.CopyTo(bytes[start..]);
        Put(bytes, 2, (uint)(17 + bytes[12]) + Read(bytes, 13, 2), 2);
        return packet;
    }

    /// <summary>원본 0040a580·0040a4b0의 위치와 실제 물리 버퍼 경계를 함께 확인해 로그인 뷰를 읽는다. 선언 길이를 고쳐 만든 포인터를 재사용하되 손상된 원본의 범위 밖 host 접근은 명시적으로 거부한다.</summary>
    /// <param name="packet">읽을 로그인 패킷이다.</param>
    /// <returns>처리기 호출 동안만 유효한 필드 뷰이다.</returns>
    public static LocalMissionTransport.LocalMissionLoginView ReadLocalMissionLogin(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 17 || packet[1] != LoginType)
        {
            throw new ArgumentException("실제 0x27 로그인 패킷 누락", nameof(packet));
        }
        uint start = 17u + Read(packet, 12, 1);
        uint size = Read(packet, 13);
        if (start > (uint)packet.Length || size > (uint)packet.Length - start)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "실제 0x27 이미지 물리 범위 부족");
        }
        string name = Text(packet, 17);
        if ((uint)(System.Text.Encoding.Latin1.GetByteCount(name) + 1) > Read(packet, 12, 1))
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "실제 0x27 nameLen의 NUL 범위 부족");
        }
        return new LocalMissionTransport.LocalMissionLoginView(Read(packet, 8), name, packet.Slice((int)start, (int)size).ToArray());
    }
}
