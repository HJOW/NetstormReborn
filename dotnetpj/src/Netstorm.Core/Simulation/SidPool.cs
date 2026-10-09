namespace Netstorm.Core.Simulation;

/// <summary>오브젝트 번호를 어느 영역에서 할당할지 (원본 할당 함수의 flags 인자)</summary>
[Flags]
public enum SidAllocation
{
    /// <summary>일반 서버 영역 (싱글 플레이는 로컬이 서버다)</summary>
    Server = 0,
    /// <summary>예측 영역 (비트 1). 클라이언트 비트보다 우선한다</summary>
    Predictable = 1,
    /// <summary>일반 클라이언트 영역 (비트 2)</summary>
    Client = 2,
}

/// <summary>반납 직전에 남기는 최근 삭제 기록 한 항목</summary>
/// <param name="Sid">반납한 번호</param>
/// <param name="Type">그 오브젝트의 타입 번호</param>
public readonly record struct SidDeletion(uint Sid, uint Type);

/// <summary>
/// 오브젝트 번호(SID) 할당기. 원본 Squid.cpp(10.78)의 004abb10 초기화·004af1d0 할당·004abf60 반납·004abe00 서버 목록 재구성을 옮겼다
/// (docs/exe/cpp-sid-reconstruction.md. 기준 구현은 cpppj/src/o/SidPool.cpp).
/// - 번호 0~4 는 일반 할당에서 빠진다. 일반 클라이언트 5~14999, 일반 서버 15000~23000, 예측 머리 23001(첫 예측 번호 23002).
/// - 일반 할당은 빈 목록의 머리에서 떼는 선입선출이고 마지막 빈 번호는 예약 꼬리로 남긴다.
///   반납한 번호는 그 목록의 꼬리에 붙으므로 한참 뒤에야 다시 쓰인다.
/// - 예측 번호는 커서로만 증가하며 반납해도 커서가 돌아가지 않는다. 세대 비트는 없다.
/// 다리 붕괴 스캔은 번호 15000~23001 을 차례로 훑으므로, 이 순서가 다리 칸의 처리 순서를 정한다.
/// 원본의 50바이트 슬롯은 복제하지 않고 번호마다 다음 번호·타입·상태만 가진다. 월드에는 아직 연결하지 않았다.
/// 검증은 원본 기계어 기대값 sid-x86.tsv 로 한다 (같은 값에서 원본 슬롯 바이트를 되살려 풀 전체의 체크섬까지 비교한다).
/// </summary>
public sealed class SidPool
{
    /// <summary>일반 할당의 첫 번호. 0~4 는 할당 대상이 아니다</summary>
    public const int FirstOrdinary = 5;

    /// <summary>10.78 의 일반 서버 영역 첫 번호</summary>
    public const int ServerFirst1078 = 15000;

    /// <summary>10.78 의 예측 머리 번호 (그 자체는 할당하지 않는다)</summary>
    public const int PredictableFirst1078 = 23001;

    /// <summary>번호가 가질 수 있는 최대 개수 (부호 없는 16비트)</summary>
    public const int MaximumCapacity = 65535;

    /// <summary>영역마다 남기는 삭제 기록의 수</summary>
    public const int DeletionHistory = 20;

    /// <summary>상태 바이트의 free 비트</summary>
    public const byte FreeBit = 1;

    /// <summary>상태 바이트의 dead 비트</summary>
    public const byte DeadBit = 2;

    /// <summary>상태 바이트의 void 비트 (월드에 놓이지 않은 상태)</summary>
    public const byte VoidBit = 4;

    /// <summary>초기화 때 free 비트가 켜지기 시작하는 번호. 0·1 번은 상태 0 을 유지한다</summary>
    private const int FirstFreeFlagged = 2;

    /// <summary>클라이언트 영역의 목록 번호</summary>
    private const int ClientList = 0;

    /// <summary>서버 영역의 목록 번호</summary>
    private const int ServerList = 1;

    /// <summary>번호 → 다음 빈 번호. 0 번 칸은 클라이언트 목록의 머리다 (원본 풀 +0 의 겹친 객체)</summary>
    private readonly ushort[] _next;

    /// <summary>번호 → 타입 번호 (반납 뒤에도 남는다)</summary>
    private readonly byte[] _type;

    /// <summary>번호 → 상태 바이트</summary>
    private readonly byte[] _state;

    /// <summary>일반 목록의 예약 꼬리 (클라이언트, 서버)</summary>
    private readonly ushort[] _tails = new ushort[2];

    /// <summary>최근 삭제 기록 (클라이언트, 서버). 앞이 최신이다</summary>
    private readonly SidDeletion[][] _deletions = [new SidDeletion[DeletionHistory], new SidDeletion[DeletionHistory]];

    /// <summary>서버 목록의 머리 (원본 풀 +14 의 겹친 객체)</summary>
    private ushort _serverHead;

    /// <summary>풀을 만들고 초기화한다.</summary>
    /// <param name="capacity">번호 개수 (예측 머리보다 2 이상 커야 한다)</param>
    /// <param name="server">서버인지 (싱글 플레이는 참)</param>
    /// <param name="serverFirst">서버 영역 첫 번호 (기본 10.78)</param>
    /// <param name="predictableFirst">예측 머리 번호 (기본 10.78)</param>
    public SidPool(int capacity, bool server = true, int serverFirst = ServerFirst1078, int predictableFirst = PredictableFirst1078)
    {
        if (serverFirst <= FirstOrdinary || predictableFirst <= serverFirst || capacity <= predictableFirst + 1 || capacity > MaximumCapacity)
        {
            throw new ArgumentOutOfRangeException(nameof(capacity), capacity, "번호 풀의 크기 또는 영역 경계가 잘못됐습니다");
        }
        Capacity = capacity;
        IsServer = server;
        ServerFirst = serverFirst;
        PredictableFirst = predictableFirst;
        _next = new ushort[capacity];
        _type = new byte[capacity];
        _state = new byte[capacity];
        Reset();
    }

    /// <summary>번호 개수</summary>
    public int Capacity { get; }

    /// <summary>서버인지. 서버가 아니면 서버 영역을 할당할 수 없다</summary>
    public bool IsServer { get; }

    /// <summary>서버 영역 첫 번호</summary>
    public int ServerFirst { get; }

    /// <summary>예측 머리 번호</summary>
    public int PredictableFirst { get; }

    /// <summary>빈 번호 카운터 (원본 값 그대로: 재구성은 다시 세지 않고 더하며, 예측 번호의 반납은 늘리지 않는다)</summary>
    public uint FreeCount { get; private set; }

    /// <summary>다음 예측 번호 = 예측 머리 + 이 값</summary>
    public uint PredictableCursor { get; private set; }

    /// <summary>서버 목록의 머리가 가리키는 첫 빈 번호</summary>
    public int ServerHead => _serverHead;

    /// <summary>영역의 머리가 가리키는 첫 빈 번호</summary>
    /// <param name="client">클라이언트 영역이면 참</param>
    public int FirstFree(bool client) => client ? _next[0] : _serverHead;

    /// <summary>영역의 예약 꼬리 (마지막 빈 번호)</summary>
    /// <param name="client">클라이언트 영역이면 참</param>
    public int Tail(bool client) => _tails[client ? ClientList : ServerList];

    /// <summary>영역의 최근 삭제 기록 20개 (앞이 최신)</summary>
    /// <param name="client">클라이언트 영역이면 참</param>
    public IReadOnlyList<SidDeletion> Deletions(bool client) => _deletions[client ? ClientList : ServerList];

    /// <summary>번호의 다음 빈 번호 (빈 목록의 연결)</summary>
    /// <param name="sid">번호</param>
    public int Next(int sid) => _next[Checked(sid)];

    /// <summary>번호의 타입 번호</summary>
    /// <param name="sid">번호</param>
    public byte Type(int sid) => _type[Checked(sid)];

    /// <summary>번호의 상태 바이트</summary>
    /// <param name="sid">번호</param>
    public byte State(int sid) => _state[Checked(sid)];

    /// <summary>
    /// 전체를 지우고 세 영역의 빈 목록을 잇는다 (원본 004abb10). 삭제 기록은 별도 계층이라 유지한다.
    /// </summary>
    public void Reset()
    {
        Array.Clear(_next);
        Array.Clear(_type);
        Array.Clear(_state);
        // 2 번부터 free 비트를 켠다
        for (int sid = FirstFreeFlagged; sid < Capacity; sid++)
        {
            _state[sid] = FreeBit;
        }
        // 클라이언트·서버·예측 영역 안에서 다음 번호로 잇고 영역의 끝은 0 으로 닫는다
        for (int sid = FirstOrdinary; sid < Capacity; sid++)
        {
            int end = sid < ServerFirst ? ServerFirst : sid < PredictableFirst ? PredictableFirst : Capacity;
            _next[sid] = (ushort)(sid + 1 == end ? 0 : sid + 1);
        }
        _next[0] = FirstOrdinary;
        _serverHead = (ushort)ServerFirst;
        _tails[ClientList] = (ushort)(ServerFirst - 1);
        _tails[ServerList] = (ushort)(PredictableFirst - 1);
        FreeCount = (uint)(Capacity - FirstOrdinary);
        PredictableCursor = 1;
    }

    /// <summary>
    /// 번호를 할당한다 (원본 004af1d0). 할당된 번호는 void 상태로 시작한다.
    /// 일반 영역이 예약 꼬리만 남았거나 예측 영역이 끝나면 예외이며 풀은 바뀌지 않는다
    /// (원본의 소진 처리 — 다리 최대 50개 삭제 등 — 는 옮기지 않았다).
    /// </summary>
    /// <param name="flags">할당 영역</param>
    public int Allocate(SidAllocation flags = SidAllocation.Server)
    {
        int sid;
        if ((flags & SidAllocation.Predictable) != 0)
        {
            sid = PredictableFirst + (int)PredictableCursor;
            if (sid >= Capacity || (_state[PredictableFirst] & FreeBit) == 0)
            {
                throw new InvalidOperationException("예측 번호가 소진됐습니다");
            }
            if ((_state[sid] & FreeBit) == 0)
            {
                throw new InvalidOperationException("예측 번호가 이미 사용 중입니다");
            }
            if (IsServer)
            {
                int next = _next[sid];
                if (next != sid + 1 || next >= Capacity || (_state[next] & FreeBit) == 0)
                {
                    throw new InvalidOperationException("예측 번호의 서버 꼬리가 소진됐습니다");
                }
                _next[PredictableFirst] = (ushort)next;
            }
            PredictableCursor++;
        }
        else
        {
            bool client = (flags & SidAllocation.Client) != 0;
            if (!client && !IsServer)
            {
                throw new InvalidOperationException("서버 번호를 할당할 권한이 없습니다");
            }
            int list = client ? ClientList : ServerList;
            sid = FirstFree(client);
            if (sid == 0 || sid == _tails[list])
            {
                throw new InvalidOperationException("일반 번호가 소진됐습니다 (예약 꼬리는 남긴다)");
            }
            if ((_state[Checked(sid)] & FreeBit) == 0)
            {
                throw new InvalidOperationException("빈 목록의 번호가 사용 중입니다");
            }
            int next = _next[sid];
            if (next == 0 || next >= Capacity)
            {
                throw new InvalidOperationException("빈 목록의 연결이 잘못됐습니다");
            }
            SetHead(client, (ushort)next);
        }
        _next[sid] = 0;
        _type[sid] = 0;
        _state[sid] = VoidBit;
        FreeCount--;
        return sid;
    }

    /// <summary>
    /// 번호를 반납한다 (원본 004abf60). void 상태의 할당된 번호만 받는다.
    /// 삭제 기록을 남기고, 타입 번호만 보존한 채 지운 뒤, 일반 영역이면 그 목록의 꼬리에 붙인다.
    /// 예측 영역의 번호는 목록에 붙지 않고 빈 번호 카운터도 늘지 않는다.
    /// </summary>
    /// <param name="sid">반납할 번호</param>
    public void Release(int sid)
    {
        Checked(sid);
        if (sid < FirstOrdinary || sid == PredictableFirst || (_state[sid] & VoidBit) == 0 || (_state[sid] & FreeBit) != 0)
        {
            throw new InvalidOperationException("반납할 번호가 void 상태의 할당된 번호가 아닙니다");
        }
        Record(sid, _type[sid]);
        _next[sid] = 0;
        if (!IsServer && sid >= ServerFirst)
        {
            _state[sid] = FreeBit;
            return;
        }
        if (sid < PredictableFirst)
        {
            int list = sid < ServerFirst ? ClientList : ServerList;
            _next[_tails[list]] = (ushort)sid;
            _tails[list] = (ushort)sid;
            FreeCount++;
        }
        _state[sid] = FreeBit | DeadBit;
    }

    /// <summary>
    /// 서버 영역의 빈 번호를 번호순으로 다시 잇는다 (원본 004abe00). 빈 번호 카운터는 다시 세지 않고 빈 서버 번호 수를 더한다.
    /// </summary>
    public void RebuildServer()
    {
        var free = new List<ushort>();
        // 서버 영역에서 free 비트가 켜진 번호를 번호순으로 모은다
        for (int sid = ServerFirst; sid < PredictableFirst; sid++)
        {
            if ((_state[sid] & FreeBit) != 0)
            {
                free.Add((ushort)sid);
            }
        }
        if (free.Count == 0)
        {
            throw new InvalidOperationException("서버 영역에 빈 번호가 없습니다");
        }
        _serverHead = free[0];
        // 각 번호가 뒤 번호를 가리키고 마지막은 0 으로 닫는다
        for (int i = 0; i < free.Count; i++)
        {
            _next[free[i]] = i + 1 < free.Count ? free[i + 1] : (ushort)0;
        }
        _tails[ServerList] = free[^1];
        FreeCount += (uint)free.Count;
    }

    /// <summary>할당된 번호의 타입 번호를 적는다 (오브젝트 생성자가 채우는 값).</summary>
    /// <param name="sid">할당된 번호</param>
    /// <param name="type">타입 번호</param>
    public void SetType(int sid, byte type) => _type[Allocated(sid)] = type;

    /// <summary>할당된 번호의 상태 바이트를 적는다 (월드 등록·삭제가 void·dead 비트를 바꾼다).</summary>
    /// <param name="sid">할당된 번호</param>
    /// <param name="state">상태 바이트</param>
    public void SetState(int sid, byte state) => _state[Allocated(sid)] = state;

    /// <summary>번호가 풀 안인지 확인한다.</summary>
    private int Checked(int sid) => sid >= 0 && sid < Capacity
        ? sid : throw new ArgumentOutOfRangeException(nameof(sid), sid, "번호가 풀의 범위를 벗어났습니다");

    /// <summary>번호가 할당된 일반·예측 번호인지 확인한다 (빈 번호·머리 번호에는 쓸 수 없다).</summary>
    private int Allocated(int sid)
    {
        Checked(sid);
        if (sid < FirstOrdinary || sid == PredictableFirst || (_state[sid] & FreeBit) != 0)
        {
            throw new InvalidOperationException("할당되지 않은 번호에는 쓸 수 없습니다");
        }
        return sid;
    }

    /// <summary>영역의 머리를 바꾼다.</summary>
    private void SetHead(bool client, ushort next)
    {
        if (client)
        {
            _next[0] = next;
        }
        else
        {
            _serverHead = next;
        }
    }

    /// <summary>삭제 기록을 한 칸씩 뒤로 밀고 맨 앞에 넣는다. 마지막 항목은 버린다.</summary>
    private void Record(int sid, byte type)
    {
        SidDeletion[] records = _deletions[sid >= FirstOrdinary && sid < ServerFirst ? ClientList : ServerList];
        Array.Copy(records, 0, records, 1, records.Length - 1);
        records[0] = new SidDeletion((uint)sid, type);
    }
}
