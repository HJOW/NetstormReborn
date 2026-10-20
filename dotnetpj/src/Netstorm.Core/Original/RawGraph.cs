using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 월드 코어의 그래프 표 저장소(원본 RawGraph 의 표 부분, 10.78).
/// 255개 레코드(각 6바이트: 표면 수·사용 여부·예약 WORD)와 4096개 flood 스택 DWORD 를 소유하고,
/// 원본처럼 0번 예약 레코드·251..253 의 틈·254 sentinel 을 구분한다.
/// 이 조각은 저장소·초기화(<see cref="ResetTable"/>)·반납(<see cref="Free"/>)·공간/타입 일치 검사만 옮겼다.
/// 영역 무효화·추가/flood·삭제 준비·전체 재구성·PostDestroy 는 다음 조각에서 같은 표 위에 얹는다.
/// 초기화 순서는 원본 그대로 "앞 251 레코드의 표면 수·사용 여부만 0 → 0번 사용=1 과 예약 WORD 하위 BYTE=1 →
/// 254 sentinel 의 표면 수·사용 여부=0x7dfd" 이며, 251..253 과 모든 레코드의 예약 WORD 나머지 비트는 보존한다.
/// 근거: cpppj/src/o/RawGraph.cpp 의 ResetTable/Free, cpppj/tests/GraphResetTests.cpp (W02 열셋째 조각, 2026-10-20).
/// </summary>
public sealed class RawGraph
{
    /// <summary>유효 그래프 번호 수(원본 Graph::kCount). 0~250 이 쓰이고 251~253 은 틈이다.</summary>
    public const int Count = SurfaceGraph.Count;

    /// <summary>표 레코드 수(원본 Graph::kTableSize). 6바이트 × 255 = 1,530바이트다.</summary>
    public const int TableSize = SurfaceGraph.TableSize;

    /// <summary>flood 스택 DWORD 수(원본 Graph::kFloodSize).</summary>
    public const int FloodSize = SurfaceGraph.FloodSize;

    /// <summary>무효 번호(원본 Graph::kInvalid). 표의 sentinel 레코드가 이 번호에 있다.</summary>
    public const byte Invalid = SurfaceGraph.Invalid;

    /// <summary>원본 초기화가 sentinel 레코드의 표면 수·사용 여부에 쓰는 값(0x7dfd).</summary>
    private const short Sentinel = 0x7DFD;

    /// <summary>호출 SID 의 상태·그래프 byte 를 읽을 슬롯 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>Pop·Unpop 과 같은 4단계 공간 해시. 공간 일치 검사에만 쓴다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>Pop·Unpop 과 같은 65,536칸 점유 지도. 공간 일치 검사에만 쓴다.</summary>
    private readonly byte[] _spots;

    /// <summary>게임 타입 번호 → 타입 레코드. 타입 일치 검사에 쓴다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>그래프 표 255 레코드. 인덱스가 그래프 번호다.</summary>
    private readonly GraphRecord[] _records = new GraphRecord[TableSize];

    /// <summary>flood 스택 4,096 DWORD. 쓰고 난 흔적도 원본처럼 보존한다.</summary>
    private readonly uint[] _stack = new uint[FloodSize];

    /// <summary>
    /// 표와 스택을 소유하고 호출자의 타입 표 수명과 분리한다. 타입 188개·점유 지도 65,536바이트가 아니면 거부한다.
    /// 레코드를 주지 않으면 254 sentinel 만 채우고, 스택을 주지 않으면 0 으로 둔다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">4단계 공간 해시(Pop·Unpop 과 같은 몸체).</param>
    /// <param name="spots">칸별 점유 바이트 65,536개(Pop·Unpop 과 같은 배열).</param>
    /// <param name="types">타입 번호 순서의 타입 레코드(10.78 은 188개).</param>
    /// <param name="records">초기 표 레코드 255개. 없으면 sentinel 만 채운다.</param>
    /// <param name="stack">초기 flood 스택 4,096개. 없으면 0 으로 둔다.</param>
    public RawGraph(RawSidPool pool, RawSquidHash hash, byte[] spots, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<GraphRecord>? records = null, IReadOnlyList<uint>? stack = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count != OriginalConstructors.TypeCount || spots.Length != RawSquidHash.WorldCells * RawSquidHash.WorldCells)
        {
            throw new ArgumentException("raw 그래프 타입/지도 크기 오류", nameof(types));
        }
        if (records is not null && records.Count != TableSize)
        {
            throw new ArgumentException("raw 그래프 표 크기 오류", nameof(records));
        }
        if (stack is not null && stack.Count != FloodSize)
        {
            throw new ArgumentException("raw 그래프 스택 크기 오류", nameof(stack));
        }
        _pool = pool;
        _hash = hash;
        _spots = spots;
        _types = types;
        if (records is null)
        {
            _records[Invalid] = new GraphRecord { Surfaces = Sentinel, InUse = Sentinel };
        }
        else
        {
            // 호출자 레코드를 그대로 복사한다(배열 공유 금지)
            for (int i = 0; i < TableSize; i++)
            {
                _records[i] = records[i];
            }
        }
        if (stack is not null)
        {
            // 스택의 쓰지 않는 칸도 원본처럼 그대로 보존한다
            for (int i = 0; i < FloodSize; i++)
            {
                _stack[i] = stack[i];
            }
        }
    }

    /// <summary>이 표가 쓰는 슬롯 풀. Pop·Unpop 과 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>표 레코드 255개를 읽기 전용으로 본다. 초기화 뒤 결과 비교에 쓴다.</summary>
    public IReadOnlyList<GraphRecord> Records => _records;

    /// <summary>flood 스택 4,096개를 읽기 전용으로 본다. 쓰지 않는 칸의 흔적도 포함한다.</summary>
    public IReadOnlyList<uint> FloodStack => _stack;

    /// <summary>
    /// 그래프 표를 원본 초기 상태로 되돌린다. 앞 251 레코드의 표면 수·사용 여부만 지우고,
    /// 0번 레코드의 사용을 1 로, 예약 WORD 의 하위 BYTE 를 1 로 만들며, 254 sentinel 의 표면 수·사용 여부를 0x7dfd 로 쓴다.
    /// 251~253 과 모든 예약 WORD 의 나머지 비트, 스택, 풀은 바꾸지 않는다.
    /// </summary>
    public void ResetTable()
    {
        // 앞 251 레코드의 첫 두 WORD 만 지운다
        for (int i = 0; i < Count; i++)
        {
            _records[i].Surfaces = 0;
            _records[i].InUse = 0;
        }
        _records[0].InUse = 1;
        // 예약 WORD 는 하위 BYTE 만 1 로 바꾸고 윗 BYTE 는 보존한다
        _records[0].Reserved = unchecked((short)(ushort)(((ushort)_records[0].Reserved & 0xFF00) | 1));
        _records[Invalid].Surfaces = Sentinel;
        _records[Invalid].InUse = Sentinel;
    }

    /// <summary>
    /// 반납 번호를 검사한다. 유효 번호 0~250 과 무효 번호 254 만 허용하고, 251~253 과 255 이상은 거부한다.
    /// </summary>
    /// <param name="graph">그래프 번호.</param>
    public void ValidateFree(byte graph)
    {
        if (graph >= Count && graph != Invalid)
        {
            throw new ArgumentOutOfRangeException(nameof(graph), graph, "raw 그래프 Free 번호 오류");
        }
    }

    /// <summary>
    /// 그래프 레코드의 표면 수·사용 여부를 지운다. 무효 번호 254 는 원본처럼 sentinel 을 그대로 두고 돌아간다.
    /// 예약 WORD 는 보존한다. 범위 밖 번호는 아무것도 바꾸지 않고 예외다.
    /// </summary>
    /// <param name="graph">그래프 번호.</param>
    public void Free(byte graph)
    {
        ValidateFree(graph);
        if (graph != Invalid)
        {
            _records[graph].Surfaces = 0;
            _records[graph].InUse = 0;
        }
    }

    /// <summary>
    /// Pop·Unpop 과 같은 공간 해시·같은 점유 지도인지 확인한다. 크기가 같은 다른 배열도 실제 공간 연결이 아니므로 거부한다.
    /// </summary>
    /// <param name="hash">Pop·Unpop 이 쓰는 공간 해시.</param>
    /// <param name="spots">Pop·Unpop 이 쓰는 점유 지도.</param>
    public void ValidateSpace(RawSquidHash hash, byte[] spots)
    {
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        if (!ReferenceEquals(hash, _hash) || !ReferenceEquals(spots, _spots))
        {
            throw new ArgumentException("Pop/raw 그래프 공간이 다릅니다");
        }
    }

    /// <summary>
    /// 그래프가 쓰는 필드(플래그1·플래그2·발자국)가 같은 타입 표인지 확인한다. 크기가 다르거나 한 타입이라도 다르면 거부한다.
    /// </summary>
    /// <param name="types">비교할 타입 표.</param>
    public void ValidateTypes(IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count != _types.Count)
        {
            throw new ArgumentException("postPop/raw 그래프 타입 크기가 다릅니다", nameof(types));
        }
        // 같은 번호의 네 필드가 모두 같아야 한다
        for (int i = 0; i < types.Count; i++)
        {
            OriginalTypeRecord other = types[i];
            OriginalTypeRecord own = _types[i];
            if (other.Flags1 != own.Flags1 || other.Flags2 != own.Flags2 || other.FootX != own.FootX || other.FootY != own.FootY)
            {
                throw new ArgumentException("postPop/raw 그래프 타입이 다릅니다", nameof(types));
            }
        }
    }
}
