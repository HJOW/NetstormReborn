// Client 원문의 지연 분포와 각 실제 연결의 FIFO를 복원하며 소켓/Session을 새로 만들지 않는다.
// cpppj/src/o/RawPacketDelay.h·RawPacketDelay.cpp 의 PacketDelayGlobals/RawPacketDelayQueue/PreparePacketDelay를 이식한다.
// 0.965에서 R4-6 전송·수신(수송 팩토리의 hooks.delay 자리) 묶음용으로 추가됐다.
namespace Netstorm.Net;

/// <summary>005e4784의 선택 분포와 같은 Client의 시계/CRT 장치다.</summary>
public sealed class PacketDelayGlobals
{
    /// <summary>00543600의 기본01ff다. 0.965에서 추가됐다.</summary>
    private static readonly sbyte[] DefaultDistribution = [1, -1];

    /// <summary>005435e0의 lag1 실제 signed BYTE다. 0.965에서 추가됐다.</summary>
    private static readonly sbyte[] Lag1Distribution = [0, 4, 12, 6, 2, 1, 1, 1, 1, -1];

    /// <summary>005435ec의 lag2 실제 signed BYTE다. 0.965에서 추가됐다.</summary>
    private static readonly sbyte[] Lag2Distribution = [0, 0, 0, 0, 1, -1];

    /// <summary>005435f4의 lag3 실제 signed BYTE다. 0.965에서 추가됐다.</summary>
    private static readonly sbyte[] Lag3Distribution = [0, 0, 0, 0, 0, 0, 0, 1, -1];

    /// <summary>실제 정적 signed BYTE 분포의 주소이며 기존 큐의 분포 사본을 재설정하지 않는다.</summary>
    public sbyte[] Distribution { get; private set; } = DefaultDistribution;

    /// <summary>원문의 현재 벽시각을 실제 장기 시계에서 읽는다. 호스트가 묶기 전까지 비어 있다.</summary>
    public Func<double>? Wall { get; set; }

    /// <summary>같은 Client CRT Next를 소비하며 새 난수 스트림을 만들지 않는다. 호스트가 묶기 전까지 비어 있다.</summary>
    public Func<uint>? Random { get; set; }

    /// <summary>새 Client의 최초 기본 선택만 구성하며 난수나 시계는 읽지 않는다. 0.965에서 추가됐다.</summary>
    public PacketDelayGlobals()
    {
        ResetDefault();
    }

    /// <summary>원문의 최초 지도 모드가 요청한 기본01ff 주소로 돌아간다. 0.965에서 추가됐다.</summary>
    public void ResetDefault() => Distribution = DefaultDistribution;

    /// <summary>435220의1/2/3만 분포를 교체하고 누락/다른 값은 현재 선택을 보존한다. 0.965에서 추가됐다.</summary>
    /// <param name="lag">선택한 지연 단계다.</param>
    public void SelectLag(int lag)
    {
        if (lag == 1)
        {
            Distribution = Lag1Distribution;
        }
        else if (lag == 2)
        {
            Distribution = Lag2Distribution;
        }
        else if (lag == 3)
        {
            Distribution = Lag3Distribution;
        }
    }
}

/// <summary>4def40의100칸 ring과 순서/주소 복원을 보유하는 실제 지연 FIFO다.</summary>
public sealed class RawPacketDelayQueue
{
    /// <summary>PE0050a668의 실제 double 밀리초 환산이다. 0.965에서 추가됐다.</summary>
    private const double SecondsPerMillisecond = 0.001;

    /// <summary>4def40의 최초 ring 크기와 signed 분포 상한이다. 0.965에서 추가됐다.</summary>
    private const int InitialCapacity = 100;

    /// <summary>원문 분포를 읽는 실제 상한 칸 수다. 0.965에서 추가됐다.</summary>
    private const int MaximumWeights = 50;

    /// <summary>원문 node의 현재 발신자/F/패킷/기한/주소/연결 owner다.</summary>
    private sealed class Entry
    {
        /// <summary>enqueue 때 저장한 원문 첫DWORD 인자다.</summary>
        public uint Sender;

        /// <summary>현재 표가 교체돼도 이미 선택한 동일 F를 사용한다.</summary>
        public CanonicalPacketReceiver? Receiver;

        /// <summary>observer1 뒤 선언WORD 길이만 복사한 실제 버퍼다.</summary>
        public byte[] Packet = [];

        /// <summary>원문 wall+delay*0.001이며 FIFO 안에서 재정렬하지 않는다.</summary>
        public double Deadline;

        /// <summary>enqueue 때 같은 연결의 실제 주소20BYTE 사본이다.</summary>
        public byte[] Address = [];

        /// <summary>ring을 소유한 같은 실제 노드이며 별도 연결 사본이 아니다.</summary>
        public PacketConnection? Connection;
    }

    /// <summary>같은 실제 Client 시계/CRT owner이며 Session을 보유하지 않는다.</summary>
    private readonly PacketDelayGlobals _globals;

    /// <summary>설치 당시 같은 불변 분포 주소다. 기존 큐는 재설정하지 않는다.</summary>
    private readonly sbyte[] _distribution;

    /// <summary>최대50 signed BYTE를ff 전까지 더한 실제 합계다.</summary>
    private readonly int _weight;

    /// <summary>현재 안정된 Entry owner 배열이다.</summary>
    private Entry?[] _ring;

    /// <summary>현재 서로 구별되는 쓰기 위치다.</summary>
    private int _write;

    /// <summary>현재 서로 구별되는 읽기 위치다.</summary>
    private int _read;

    /// <summary>분포는 ff 또는50BYTE까지만 읽고 현재 Client 장치를 그대로 보유한다. 0.965에서 추가됐다.</summary>
    /// <param name="globals">같은 실제 Client 시계/CRT owner다.</param>
    public RawPacketDelayQueue(PacketDelayGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(globals);
        if (globals.Wall is null || globals.Random is null || globals.Distribution.Length == 0)
        {
            throw new ArgumentException("지연큐의 실제 현재 분포/벽시각/CRT 미연결", nameof(globals));
        }

        _globals = globals;
        _distribution = globals.Distribution;
        int weight = 0;
        // 원문의 signed BYTE 합계를 ff까지 보존한다.
        for (int index = 0; index < _distribution.Length && index < MaximumWeights; index++)
        {
            if (_distribution[index] == -1)
            {
                break;
            }

            weight += _distribution[index];
        }

        _weight = weight;
        _ring = new Entry[InitialCapacity];
    }

    /// <summary>감긴 접두만 기존 capacity 뒤로 옮기며 read 위치를 바꾸지 않는다. 0.965에서 추가됐다.</summary>
    private void Grow()
    {
        int capacity = _ring.Length;
        if (capacity > int.MaxValue / 2)
        {
            throw new InvalidOperationException("지연 ring 두 배 크기 초과");
        }

        Array.Resize(ref _ring, capacity * 2);
        // 원문 realloc 뒤 감긴0..write 접두를 새 뒤쪽으로 이동한다.
        if (_write < _read)
        {
            for (int index = 0; index < _write; index++)
            {
                _ring[capacity + index] = _ring[index];
                _ring[index] = null;
            }

            _write += capacity;
        }
    }

    /// <summary>같은 집합의 고정 지연도 bucket 난수는 먼저 소비하며 receiver NULL은 앞에서 반환한다. 0.965에서 추가됐다.</summary>
    /// <param name="connection">지연 FIFO를 소유한 같은 실제 노드다.</param>
    /// <param name="receiver">이미 선택한 동일 F다. NULL이면 0을 반환한다.</param>
    /// <param name="dispatch">현재 등록 표와 관찰기의 실제 owner다.</param>
    /// <param name="packet">호출자 소유의 같은 현재 버퍼다. 선언 길이까지만 복사하며 호출자 버퍼는 바꾸지 않는다.</param>
    /// <param name="delay">고정 지연 밀리초다. -1이면 분포와 난수로 정한다.</param>
    /// <returns>실제 적용한 지연 밀리초다.</returns>
    public int Enqueue(PacketConnection connection, CanonicalPacketReceiver? receiver, RawPacketDispatch dispatch, ReadOnlyMemory<byte> packet, int delay)
    {
        ArgumentNullException.ThrowIfNull(connection);
        ArgumentNullException.ThrowIfNull(dispatch);
        if (receiver is null)
        {
            return 0;
        }

        if (_weight == 0)
        {
            throw new InvalidOperationException("원문 지연 분포 합계0 나눗셈");
        }

        // 원문은 동일 경계에서 다음 bucket으로 넘어가지 않는다.
        int draw = unchecked((int)(_globals.Random!() % unchecked((uint)_weight)));
        int bucket = 0;
        int cumulative = _distribution[0];
        while (cumulative < draw)
        {
            ++bucket;
            if (bucket >= _distribution.Length || bucket >= MaximumWeights || _distribution[bucket] == -1)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "원문 지연 분포 bucket 물리 범위");
            }

            cumulative += _distribution[bucket];
        }

        int available = _write < _read ? _read - _write : _ring.Length - _write + _read;
        if (available < 2)
        {
            Grow();
        }

        if (delay == -1)
        {
            delay = unchecked((int)(_globals.Random!() % 100u)) + bucket * 100;
        }

        if (packet.Length < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "지연 packet 헤더 물리 부족");
        }

        // 원문은 생성자 값 인수로 넘긴 주소를 observer1보다 먼저 저장한다. 호출자 버퍼 대신 사본에 observer를 실행한다.
        byte[] address = new byte[connection.Address.Length];
        Array.Copy(connection.Address, address, address.Length);
        byte[] mutable = packet.ToArray();
        dispatch.Registration(mutable[1]).NotifyObserver(mutable, 1);
        int length = mutable[2] | (mutable[3] << 8);
        if (length > mutable.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "지연 packet 선언 길이 물리 초과");
        }

        byte[] stored = new byte[length];
        Array.Copy(mutable, stored, length);
        var entry = new Entry
        {
            Sender = connection.Sender,
            Receiver = receiver,
            Packet = stored,
            Deadline = delay * SecondsPerMillisecond + _globals.Wall!(),
            Address = address,
            Connection = connection,
        };
        _ring[_write] = entry;
        _write = (_write + 1) % _ring.Length;
        return delay;
    }

    /// <summary>예외에서도 실제 주소/깊이는 복구하고 실패한 선두는 보존하여 정상 배송으로 숨기지 않는다. 0.965에서 추가됐다.</summary>
    /// <param name="connection">배송할 같은 실제 노드다.</param>
    /// <returns>배송한 수다. 닫힘 표시를 만나면 -1이다.</returns>
    public int Pump(PacketConnection connection)
    {
        ArgumentNullException.ThrowIfNull(connection);
        int delivered = 0;
        // 선두가 준비되지 않으면 뒤 deadline이 더 작아도 앞지르지 않는다.
        while (_read != _write)
        {
            int slot = _read;
            Entry? entry = _ring[slot];
            if (entry is null || !ReferenceEquals(entry.Connection, connection))
            {
                throw new InvalidOperationException("지연 ring의 실제 현재 연결/선두 불일치");
            }

            if (!(entry.Deadline < _globals.Wall!()))
            {
                return delivered;
            }

            byte[] previous = new byte[connection.Address.Length];
            Array.Copy(connection.Address, previous, previous.Length);
            Array.Copy(entry.Address, connection.Address, entry.Address.Length);
            connection.Depth = unchecked(connection.Depth + 1);
            try
            {
                entry.Receiver!.Invoke(entry.Sender, connection, entry.Packet);
            }
            catch
            {
                Array.Copy(previous, connection.Address, previous.Length);
                connection.Depth = unchecked(connection.Depth - 1);
                throw;
            }

            // packet 메모리는 주소/깊이 복구보다 먼저 반납한다.
            entry.Packet = [];
            Array.Copy(previous, connection.Address, previous.Length);
            connection.Depth = unchecked(connection.Depth - 1);
            _ring[slot] = null;
            _read = (_read + 1) % _ring.Length;
            ++delivered;
            if ((connection.Flags & 0x40u) != 0)
            {
                return -1;
            }
        }

        return delivered;
    }
}

/// <summary>실제4dfc00 Poll에서만 현재 분포가 있고 큐가 없으면 설치한다.</summary>
public static class RawPacketDelay
{
    /// <summary>이미 설치된 큐의 분포는 새 선택/Session 교체 때 다시 초기화하지 않는다. 0.965에서 추가됐다.</summary>
    /// <param name="connection">같은 실제 노드다.</param>
    /// <param name="globals">같은 실제 Client 시계/CRT owner다. NULL이면 설치하지 않는다.</param>
    public static void Prepare(PacketConnection connection, PacketDelayGlobals? globals)
    {
        ArgumentNullException.ThrowIfNull(connection);
        if (connection.DelayQueue is null && globals is not null && globals.Distribution.Length != 0)
        {
            var queue = new RawPacketDelayQueue(globals);
            connection.DelayQueue = queue.Enqueue;
        }

        if (connection.DelayQueue is not null)
        {
            connection.Delayed = true;
        }
    }
}
