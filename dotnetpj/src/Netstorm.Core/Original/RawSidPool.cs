using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 월드 코어의 오브젝트 슬롯 저장소. 원본 Squid.cpp(10.78)의 SID 풀을 슬롯 바이트째 옮겼다.
/// 슬롯 하나는 50바이트이고 번호가 곧 슬롯 위치다. 다음 빈 번호(+4, 16비트)·타입(+10)·상태(+11)와 그 밖의 필드는 모두 이 바이트 안에 있다.
/// 생성자·Pop·Unpop·소유자·프레임 같은 후속 원본 모듈이 같은 슬롯 바이트를 읽고 쓰므로, 필드를 따로 모델링하지 않고 바이트를 그대로 둔다
/// (원본 기대값이 슬롯 전체 바이트로 되어 있어 일대일로 대조할 수 있다).
/// 할당·반납·재구성 규칙은 기존 <see cref="SidPool"/>(압축 표현)과 같고, 이 클래스는 원본 월드 코어가 세션에 연결될 때 그것을 대체한다
/// (docs/dotnet-cpp-port-plan-20261016.md 4절). 검증은 원본 기대값 sid-x86.tsv 의 10.78 행으로 풀 전체 바이트까지 한다.
/// 번호 0~4 는 일반 할당에서 빠지고, 일반 클라이언트 5~14999, 일반 서버 15000~23000, 예측 머리 23001(첫 예측 번호 23002)이다.
/// 0번 슬롯에는 클라이언트 목록 머리(+0)와 서버 목록 머리(+14)가 겹쳐 놓인다.
/// 근거: docs/exe/cpp-sid-reconstruction.md, cpppj/src/o/SidPool.cpp (W02 첫 조각, 2026-10-16; ClearDeletionHistory·공유 서버 판정은 R0-4 재대조, 2026-10-28).
/// </summary>
public sealed class RawSidPool
{
    /// <summary>슬롯 하나의 바이트 수(10.78).</summary>
    public const int Stride = 50;

    /// <summary>일반 할당의 첫 번호. 0~4 는 할당 대상이 아니다.</summary>
    public const int FirstOrdinary = 5;

    /// <summary>일반 서버 영역의 첫 번호.</summary>
    public const int ServerFirst = 15000;

    /// <summary>예측 머리 번호. 그 자체는 할당하지 않는다.</summary>
    public const int PredictableFirst = 23001;

    /// <summary>지원하는 최대 슬롯 수. 원본 Clientmain.cpp 가 실제로 확보하는 수와 같고, 빈 목록 연결은 16비트라 넘치는 번호는 원본처럼 잘린다.</summary>
    public const int MaximumCapacity = 120000;

    /// <summary>슬롯 안 "다음 빈 번호"(16비트)의 위치.</summary>
    public const int NextOffset = 4;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치.</summary>
    public const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치.</summary>
    public const int StateOffset = 11;

    /// <summary>상태 바이트의 free 비트(빈 슬롯).</summary>
    public const byte FreeBit = 1;

    /// <summary>상태 바이트의 dead 비트(반납된 슬롯).</summary>
    public const byte DeadBit = 2;

    /// <summary>상태 바이트의 void 비트(월드에 놓이지 않은 상태).</summary>
    public const byte VoidBit = 4;

    /// <summary>영역마다 남기는 삭제 기록의 수.</summary>
    public const int DeletionHistory = 20;

    /// <summary>초기화 때 free 비트가 켜지기 시작하는 번호. 0·1 번은 상태 0 을 유지한다.</summary>
    private const int FirstFreeFlagged = 2;

    /// <summary>두 일반 빈 목록의 머리가 놓인 0번 슬롯 안 위치(클라이언트 +0, 서버 +14). 머리의 "다음 빈 번호"는 그 위치 +4 에 있다.</summary>
    private static readonly int[] Heads = [0, 14];

    /// <summary>풀 전체의 슬롯 바이트. 번호 N 의 슬롯은 N × 50 에서 시작한다.</summary>
    private byte[] _bytes;

    /// <summary>생성자 입력의 서버 여부. 공유 판정이 연결되면 읽지 않는다.</summary>
    private readonly bool _server;

    /// <summary>Pool보다 오래 사는 실제 서버 판정. 미연결 독립 풀은 생성자 입력을 읽는다.</summary>
    private readonly Func<bool>? _sharedServer;

    /// <summary>일반 목록의 예약 꼬리(클라이언트, 서버).</summary>
    private readonly ushort[] _tails = new ushort[2];

    /// <summary>최근 삭제 기록(클라이언트, 서버). 앞이 최신이다.</summary>
    private readonly SidDeletion[][] _deletions = [new SidDeletion[DeletionHistory], new SidDeletion[DeletionHistory]];

    /// <summary>풀을 만들고 원본 초기 상태로 만든다.</summary>
    /// <param name="capacity">슬롯 수(예측 머리보다 2 이상 커야 하며 <see cref="MaximumCapacity"/> 이하).</param>
    /// <param name="server">서버인지. 서버가 아니면 서버 영역을 할당할 수 없고 생성 시 소유자·HP 초기화도 하지 않는다.</param>
    /// <param name="sharedServer">서버 판정을 함께 읽는 장기 소유자. null이면 생성자 입력을 쓴다. R4 세션 조립 때 연결한다.</param>
    public RawSidPool(int capacity, bool server = true, Func<bool>? sharedServer = null)
    {
        CheckCapacity(capacity);
        Capacity = capacity;
        _server = server;
        _sharedServer = sharedServer;
        _bytes = new byte[capacity * Stride];
        Reset();
    }

    /// <summary>확보한 슬롯 수.</summary>
    public int Capacity { get; private set; }

    /// <summary>서버인지. 공유 판정이 연결되면 그 값을 읽는다.</summary>
    public bool IsServer => _sharedServer?.Invoke() ?? _server;

    /// <summary>빈 번호 카운터. 원본 값 그대로 재구성은 다시 세지 않고 더하며, 예측 번호의 반납은 늘리지 않는다.</summary>
    public uint FreeCount { get; private set; }

    /// <summary>다음 예측 번호 = 예측 머리 + 이 값.</summary>
    public uint PredictableCursor { get; private set; }

    /// <summary>풀 전체의 슬롯 바이트를 읽기 전용으로 돌려준다(검사용).</summary>
    public ReadOnlySpan<byte> Bytes => _bytes;

    /// <summary>번호의 슬롯 바이트(50개)를 읽기 전용으로 돌려준다. 범위를 벗어난 번호는 예외다.</summary>
    /// <param name="sid">슬롯 번호.</param>
    public ReadOnlySpan<byte> Slot(int sid) => _bytes.AsSpan(Offset(sid), Stride);

    /// <summary>영역의 머리가 가리키는 첫 빈 번호.</summary>
    /// <param name="client">클라이언트 영역이면 참.</param>
    public int FirstFree(bool client) => ReadNext(Heads[client ? 0 : 1]);

    /// <summary>영역의 예약 꼬리(마지막 빈 번호).</summary>
    /// <param name="client">클라이언트 영역이면 참.</param>
    public int Tail(bool client) => _tails[client ? 0 : 1];

    /// <summary>영역의 최근 삭제 기록 20개(앞이 최신).</summary>
    /// <param name="client">클라이언트 영역이면 참.</param>
    public IReadOnlyList<SidDeletion> Deletions(bool client) => _deletions[client ? 0 : 1];

    /// <summary>
    /// 각 SID/type DWORD만 순서대로 지운다. 하위 Reset은 기존처럼 기록을 보존한다.
    /// other(서버 목록) 기록 20쌍을 먼저, 다음 client 기록 20쌍을 지운다.
    /// 근거: cpppj/src/o/SidPool.cpp ClearDeletionHistory (R0-4 재대조, 2026-10-28).
    /// </summary>
    public void ClearDeletionHistory()
    {
        // 원문 첫 40DWORD인 other 기록의 실제 20쌍, 다음 40DWORD인 client 기록의 실제 20쌍이다
        Array.Clear(_deletions[1]);
        Array.Clear(_deletions[0]);
    }

    /// <summary>
    /// 전체를 지우고 세 영역의 빈 목록을 잇는다(원본 004abb10). 삭제 기록은 유지한다.
    /// 양수 <paramref name="capacity"/> 를 주면 풀 크기를 다시 확보하고(실패하면 기존 풀을 그대로 둔다), 0 이하면 기존 풀만 초기화한다.
    /// 재확보 뒤에는 이전에 얻은 <see cref="Bytes"/>·<see cref="Slot"/> 범위를 쓰지 않는다.
    /// </summary>
    /// <param name="capacity">새 슬롯 수. 0 이면 크기를 바꾸지 않는다.</param>
    public void Reset(int capacity = 0)
    {
        if (capacity > 0)
        {
            CheckCapacity(capacity);
            _bytes = new byte[capacity * Stride];
            Capacity = capacity;
        }
        Array.Clear(_bytes);
        // 2 번부터 free 비트를 켠다(0·1 번은 초기 상태 0)
        for (int sid = FirstFreeFlagged; sid < Capacity; sid++)
        {
            _bytes[sid * Stride + StateOffset] = FreeBit;
        }
        // 클라이언트·서버·예측 영역 안에서 다음 번호로 잇고 영역의 끝은 0 으로 닫는다
        for (int sid = FirstOrdinary; sid < Capacity; sid++)
        {
            int end = sid < ServerFirst ? ServerFirst : sid < PredictableFirst ? PredictableFirst : Capacity;
            WriteNext(sid * Stride, (ushort)(sid + 1 == end ? 0 : sid + 1));
        }
        WriteNext(Heads[0], FirstOrdinary);
        WriteNext(Heads[1], ServerFirst);
        _tails[0] = ServerFirst - 1;
        _tails[1] = PredictableFirst - 1;
        FreeCount = (uint)(Capacity - FirstOrdinary);
        PredictableCursor = 1;
    }

    /// <summary>
    /// 번호를 할당한다(원본 004af1d0). 할당된 슬롯은 전부 0 으로 지워지고 void 상태로 시작한다.
    /// 예측 비트가 클라이언트 비트보다 우선한다. 일반 영역이 예약 꼬리만 남았거나 예측 영역이 끝나면 예외이며 풀은 바뀌지 않는다
    /// (원본의 소진 처리 — 다리 최대 50개 삭제 등 — 는 옮기지 않았다).
    /// </summary>
    /// <param name="flags">할당 영역.</param>
    public int Allocate(SidAllocation flags = SidAllocation.Server)
    {
        int sid;
        if ((flags & SidAllocation.Predictable) != 0)
        {
            int head = PredictableFirst * Stride;
            sid = PredictableFirst + (int)PredictableCursor;
            if (sid >= Capacity || (_bytes[head + StateOffset] & FreeBit) == 0)
            {
                throw new InvalidOperationException("예측 번호가 소진됐습니다");
            }
            int offset = sid * Stride;
            if ((_bytes[offset + StateOffset] & FreeBit) == 0)
            {
                throw new InvalidOperationException("예측 번호가 이미 사용 중입니다");
            }
            if (IsServer)
            {
                int next = ReadNext(offset);
                if (next != sid + 1 || next >= Capacity || (_bytes[next * Stride + StateOffset] & FreeBit) == 0)
                {
                    throw new InvalidOperationException("예측 번호의 서버 꼬리가 소진됐습니다");
                }
                WriteNext(head, (ushort)next);
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
            int list = client ? 0 : 1;
            sid = ReadNext(Heads[list]);
            if (sid == 0 || sid == _tails[list])
            {
                throw new InvalidOperationException("일반 번호가 소진됐습니다 (예약 꼬리는 남긴다)");
            }
            int offset = Offset(sid);
            if ((_bytes[offset + StateOffset] & FreeBit) == 0)
            {
                throw new InvalidOperationException("빈 목록의 번호가 사용 중입니다");
            }
            int next = ReadNext(offset);
            if (next == 0 || next >= Capacity)
            {
                throw new InvalidOperationException("빈 목록의 연결이 잘못됐습니다");
            }
            WriteNext(Heads[list], (ushort)next);
        }
        int slot = sid * Stride;
        Array.Clear(_bytes, slot, Stride);
        _bytes[slot + StateOffset] = VoidBit;
        FreeCount--;
        return sid;
    }

    /// <summary>
    /// 번호를 반납한다(원본 004abf60). void 상태로 할당된 번호만 받는다.
    /// 삭제 기록을 남기고, 타입 번호만 보존한 채 슬롯을 지운 뒤, 일반 영역이면 그 목록의 꼬리에 붙인다.
    /// 예측 영역의 번호는 목록에 붙지 않고 빈 번호 카운터도 늘지 않는다.
    /// </summary>
    /// <param name="sid">반납할 번호.</param>
    public void Release(int sid)
    {
        int offset = Offset(sid);
        if (sid < FirstOrdinary || sid == PredictableFirst
            || (_bytes[offset + StateOffset] & VoidBit) == 0 || (_bytes[offset + StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("반납할 번호가 void 상태의 할당된 번호가 아닙니다");
        }
        byte type = _bytes[offset + TypeOffset];
        Record(sid, type);
        Array.Clear(_bytes, offset, Stride);
        _bytes[offset + TypeOffset] = type;
        if (!IsServer && sid >= ServerFirst)
        {
            _bytes[offset + StateOffset] = FreeBit;
            return;
        }
        if (sid < PredictableFirst)
        {
            int list = sid < ServerFirst ? 0 : 1;
            WriteNext(_tails[list] * Stride, (ushort)sid);
            _tails[list] = (ushort)sid;
            FreeCount++;
        }
        _bytes[offset + StateOffset] = FreeBit | DeadBit;
    }

    /// <summary>
    /// 서버 영역의 빈 번호를 번호순으로 다시 잇는다(원본 004abe00). 빈 번호 카운터는 다시 세지 않고 빈 서버 번호 수를 더한다.
    /// </summary>
    public void RebuildServer()
    {
        var free = new List<ushort>();
        // 서버 영역에서 free 비트가 켜진 번호를 번호순으로 모은다
        for (int sid = ServerFirst; sid < PredictableFirst; sid++)
        {
            if ((_bytes[sid * Stride + StateOffset] & FreeBit) != 0)
            {
                free.Add((ushort)sid);
            }
        }
        if (free.Count == 0)
        {
            throw new InvalidOperationException("서버 영역에 빈 번호가 없습니다");
        }
        WriteNext(Heads[1], free[0]);
        // 각 번호가 뒤 번호를 가리키고 마지막은 0 으로 닫는다
        for (int i = 0; i < free.Count; i++)
        {
            WriteNext(free[i] * Stride, i + 1 < free.Count ? free[i + 1] : (ushort)0);
        }
        _tails[1] = free[^1];
        FreeCount += (uint)free.Count;
    }

    /// <summary>
    /// 할당된 번호의 슬롯 바이트를 쓸 수 있게 돌려준다. 생성자가 필드를 채울 때 쓴다.
    /// 빈 슬롯·0~4 번·예측 머리에는 쓸 수 없다.
    /// </summary>
    /// <param name="sid">할당된 번호.</param>
    public Span<byte> AllocatedBytes(int sid)
    {
        int offset = Offset(sid);
        if (sid < FirstOrdinary || sid == PredictableFirst || (_bytes[offset + StateOffset] & FreeBit) != 0)
        {
            throw new InvalidOperationException("할당되지 않은 번호에는 쓸 수 없습니다");
        }
        return _bytes.AsSpan(offset, Stride);
    }

    /// <summary>
    /// 상태에 상관없이 슬롯 바이트를 쓸 수 있게 돌려준다. 빈 슬롯을 받는 Take, 공간 해제, 건물 옆면 갱신처럼
    /// 풀 목록 밖에서 슬롯을 직접 고치는 원본 모듈만 쓴다(같은 어셈블리 안의 코어 모듈에 한정).
    /// </summary>
    /// <param name="sid">슬롯 번호.</param>
    internal Span<byte> RawSlot(int sid) => _bytes.AsSpan(Offset(sid), Stride);

    /// <summary>슬롯 위치에서 "다음 빈 번호"(16비트 리틀 엔디언)를 읽는다. 0번 슬롯의 서버 머리는 위치 14 를 준다.</summary>
    /// <param name="offset">슬롯(또는 겹친 머리 객체)의 시작 바이트 위치.</param>
    internal int ReadNext(int offset) => _bytes[offset + NextOffset] | (_bytes[offset + NextOffset + 1] << 8);

    /// <summary>슬롯 위치에 "다음 빈 번호"를 16비트 리틀 엔디언으로 쓴다.</summary>
    /// <param name="offset">슬롯(또는 겹친 머리 객체)의 시작 바이트 위치.</param>
    /// <param name="next">쓸 번호.</param>
    internal void WriteNext(int offset, ushort next)
    {
        _bytes[offset + NextOffset] = (byte)next;
        _bytes[offset + NextOffset + 1] = (byte)(next >> 8);
    }

    /// <summary>번호가 풀 안인지 확인하고 슬롯의 시작 바이트 위치를 돌려준다.</summary>
    /// <param name="sid">슬롯 번호.</param>
    private int Offset(int sid) => sid >= 0 && sid < Capacity
        ? sid * Stride
        : throw new ArgumentOutOfRangeException(nameof(sid), sid, "번호가 풀의 범위를 벗어났습니다");

    /// <summary>풀 크기가 허용 범위인지 확인한다. 예측 머리보다 2 이상 커야 하고 최대 슬롯 수 이하여야 한다.</summary>
    /// <param name="capacity">확인할 슬롯 수.</param>
    private static void CheckCapacity(int capacity)
    {
        if (capacity <= PredictableFirst + 1 || capacity > MaximumCapacity)
        {
            throw new ArgumentOutOfRangeException(nameof(capacity), capacity, "번호 풀의 크기가 잘못됐습니다");
        }
    }

    /// <summary>삭제 기록을 한 칸씩 뒤로 밀고 맨 앞에 넣는다. 마지막 항목은 버린다.</summary>
    /// <param name="sid">반납한 번호.</param>
    /// <param name="type">반납한 오브젝트의 타입 번호.</param>
    private void Record(int sid, byte type)
    {
        SidDeletion[] records = _deletions[sid >= FirstOrdinary && sid < ServerFirst ? 0 : 1];
        Array.Copy(records, 0, records, 1, records.Length - 1);
        records[0] = new SidDeletion((uint)sid, type);
    }
}
