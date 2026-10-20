using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Original;

/// <summary>
/// 원본 월드 코어의 그래프 표·공간 연결(원본 RawGraph, 10.78).
/// 255개 레코드(각 6바이트: 표면 수·사용 여부·예약 WORD)와 4096개 flood 스택 DWORD 를 소유하고,
/// 원본처럼 0번 예약 레코드·251..253 의 틈·254 sentinel 을 구분한다.
/// 이 조각은 표 저장소·초기화(<see cref="ResetTable"/>)·반납(<see cref="Free"/>)·공간/타입 일치와 함께
/// 원본 슬롯·해시·점유 지도에서 표면 스냅샷을 만들어 영역 무효화(<see cref="InvalidateRegion"/>)·
/// 추가(<see cref="Add"/>)·flood(<see cref="Flood"/>)·등록 후처리(<see cref="PostPop"/>)·빈 번호 할당(<see cref="Allocate"/>)을 수행한다.
/// 계산은 항상 사본에서 하고(<see cref="Calculate"/>) 성공한 계획만 commit 하므로, 거부된 호출은 표·스택·슬롯을 바꾸지 않는다.
/// 삭제 준비(<see cref="RawGraph"/> 의 Detach)·전체 재구성(Rebuild)·PostDestroy 는 다음 조각으로 남겼다.
/// 근거: cpppj/src/o/RawGraph.cpp (W02 열넷째 조각, 2026-10-20).
/// </summary>
public sealed class RawGraph : IRawPostPopGraph
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

    /// <summary>부호 있는 short SID 를 쓰는 원본 표면 탐색의 한계.</summary>
    private const int MaximumId = 32767;

    /// <summary>상태 바이트의 free 비트(빈 슬롯).</summary>
    private const byte FreeBit = 1;

    /// <summary>상태 바이트의 dead 비트(삭제 준비).</summary>
    private const byte DeadBit = 2;

    /// <summary>상태 바이트의 void 비트(공간에서 빠진 오브젝트).</summary>
    private const byte VoidBit = 4;

    /// <summary>상태 바이트의 들어 있는 내용물(contained) 비트.</summary>
    private const byte ContainedBit = 8;

    /// <summary>추가 비트의 매립(buried) 비트.</summary>
    private const byte BuriedBit = 8;

    /// <summary>슬롯 안 그래프 번호 바이트의 위치(10.78).</summary>
    private const int GraphOffset = 30;

    /// <summary>슬롯 안 저장 단계 바이트의 위치(10.78).</summary>
    private const int LevelOffset = 33;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78).</summary>
    private const int ExtraOffset = 40;

    /// <summary>슬롯 안 프레임(SHP) 번호의 위치(10.78, 4바이트).</summary>
    private const int FrameOffset = 36;

    /// <summary>슬롯 안 x 좌표(float, 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float, 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>호출 SID 의 상태·그래프 byte 를 읽을 슬롯 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>Pop·Unpop 과 같은 4단계 공간 해시. 공간 일치 검사와 영역/후보 탐색에 쓴다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>Pop·Unpop 과 같은 65,536칸 점유 지도. 공간 일치 검사와 영역 계산에 쓴다.</summary>
    private readonly byte[] _spots;

    /// <summary>게임 타입 번호 → 타입 레코드. 플래그·발자국을 읽는다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입 번호 → 프레임 코드 목록. 표면 후보의 방향 코드를 읽는다(없으면 빈 목록).</summary>
    private readonly IReadOnlyList<IReadOnlyList<FrameCode>> _frames;

    /// <summary>그래프 표 255 레코드. 인덱스가 그래프 번호다.</summary>
    private readonly GraphRecord[] _records = new GraphRecord[TableSize];

    /// <summary>flood 스택 4,096 DWORD. 쓰고 난 흔적도 원본처럼 보존한다.</summary>
    private readonly uint[] _stack = new uint[FloodSize];

    /// <summary>풀이 소진됐을 때 전체 표를 다시 만드는지 여부(원본 GraphRecovery).</summary>
    private readonly GraphRecovery _recovery;

    /// <summary>
    /// 표와 스택을 소유하고 호출자의 타입·프레임 표 수명과 분리한다. 타입 188개·점유 지도 65,536바이트가 아니면 거부한다.
    /// 레코드를 주지 않으면 254 sentinel 만 채우고, 스택을 주지 않으면 0 으로 둔다.
    /// </summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="hash">4단계 공간 해시(Pop·Unpop 과 같은 몸체).</param>
    /// <param name="spots">칸별 점유 바이트 65,536개(Pop·Unpop 과 같은 배열).</param>
    /// <param name="types">타입 번호 순서의 타입 레코드(10.78 은 188개).</param>
    /// <param name="records">초기 표 레코드 255개. 없으면 sentinel 만 채운다.</param>
    /// <param name="stack">초기 flood 스택 4,096개. 없으면 0 으로 둔다.</param>
    /// <param name="frames">타입별 프레임 코드 목록. 없으면 모든 타입이 빈 목록이며 표면 후보를 읽는 연산은 거부된다.</param>
    /// <param name="recovery">풀 소진 때 표를 다시 만드는 정책.</param>
    public RawGraph(RawSidPool pool, RawSquidHash hash, byte[] spots, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<GraphRecord>? records = null, IReadOnlyList<uint>? stack = null,
        IReadOnlyList<IReadOnlyList<FrameCode>>? frames = null, GraphRecovery recovery = GraphRecovery.Reject)
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
        if (frames is not null && frames.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException("raw 그래프 프레임 표 크기 오류", nameof(frames));
        }
        _pool = pool;
        _hash = hash;
        _spots = spots;
        _types = types;
        _recovery = recovery;
        _frames = frames ?? Enumerable.Range(0, OriginalConstructors.TypeCount)
            .Select(_ => (IReadOnlyList<FrameCode>)Array.Empty<FrameCode>()).ToArray();
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

    /// <summary>이 표가 쓰는 슬롯 풀. Pop·Unpop·PostPop 과 같은 풀인지 확인하는 데 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>표 레코드 255개를 읽기 전용으로 본다. 초기화·연산 뒤 결과 비교에 쓴다.</summary>
    public IReadOnlyList<GraphRecord> Records => _records;

    /// <summary>flood 스택 4,096개를 읽기 전용으로 본다. 쓰지 않는 칸의 흔적도 포함한다.</summary>
    public IReadOnlyList<uint> FloodStack => _stack;

    /// <summary>한 번의 계산이 바꿀 표·스택·슬롯 그래프 byte 목록이다. 성공 뒤에만 반영한다.</summary>
    private sealed class Plan
    {
        /// <summary>계산 결과 표(255 레코드).</summary>
        public required GraphRecord[] Records;

        /// <summary>계산 결과 flood 스택(4,096).</summary>
        public required uint[] Stack;

        /// <summary>바꿀 슬롯 그래프 byte 목록(번호, 값). 같은 번호가 두 번 나오지 않는다.</summary>
        public List<(int Sid, byte Graph)> Numbers = [];

        /// <summary>Flood/Allocate 가 돌려주는 변경 수 또는 할당 번호.</summary>
        public uint Changed;
    }

    /// <summary>원본 계획 계산이 구분하는 연산 종류.</summary>
    private enum Operation
    {
        /// <summary>표면 추가(가장 큰 연결 그래프에 합친다).</summary>
        Add,

        /// <summary>목적 번호로 flood.</summary>
        Flood,

        /// <summary>발자국 영역 무효화(번호를 무효로 바꾼다).</summary>
        Region,

        /// <summary>영역 무효화 뒤 표면 추가.</summary>
        RegionAdd,

        /// <summary>삭제 준비(다음 조각).</summary>
        Detach,

        /// <summary>전체 재구성(다음 조각).</summary>
        Rebuild,

        /// <summary>빈 그래프 번호 할당.</summary>
        Allocate,
    }

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

    /// <summary>추가 전 검사. 계산만 하고 표·스택·슬롯을 바꾸지 않는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="projection">Pop 이 예측한 좌표·단계·타입. 없으면 슬롯의 현재 값을 읽는다.</param>
    public void ValidateAdd(int sid, PopProjection? projection = null) => _ = Calculate(sid, Operation.Add, 0, projection);

    /// <summary>영역 무효화·추가가 가능한지 계산만 한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="invalidate">발자국 영역을 무효화하는지.</param>
    /// <param name="add">표면을 추가하는지.</param>
    /// <param name="projection">Pop 이 예측한 좌표·단계·타입. 없으면 슬롯의 현재 값을 읽는다.</param>
    public void ValidatePostPop(int sid, bool invalidate, bool add, PopProjection? projection = null)
    {
        if (invalidate || add)
        {
            _ = Calculate(sid, invalidate ? (add ? Operation.RegionAdd : Operation.Region) : Operation.Add, 0, projection);
        }
    }

    /// <summary>등록 후처리가 영역 무효화·표면 추가를 함께 요청할 때 계획을 반영한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="invalidate">발자국 영역을 무효화하는지.</param>
    /// <param name="add">표면을 추가하는지.</param>
    public void PostPop(int sid, bool invalidate, bool add)
    {
        if (invalidate || add)
        {
            Commit(Calculate(sid, invalidate ? (add ? Operation.RegionAdd : Operation.Region) : Operation.Add, 0, null));
        }
    }

    /// <summary>4단계 해시의 발자국 영역 후보 번호를 무효로 바꾼다(원본 00462d40).</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void InvalidateRegion(int sid) => Commit(Calculate(sid, Operation.Region, 0, null));

    /// <summary>표면을 가장 큰 연결 그래프에 추가한다(원본 004633c0).</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    public void Add(int sid) => Commit(Calculate(sid, Operation.Add, 0, null));

    /// <summary>목적 번호로 표면 연결을 flood 하고 바뀐 번호 수를 돌려준다(원본 00462f10).</summary>
    /// <param name="sid">시작 슬롯 번호.</param>
    /// <param name="graph">목적 그래프 번호(254 는 무효).</param>
    public uint Flood(int sid, byte graph)
    {
        Plan plan = Calculate(sid, Operation.Flood, graph, null);
        Commit(plan);
        return plan.Changed;
    }

    /// <summary>
    /// 사용하지 않는 가장 낮은 그래프 번호를 확보한다(원본 00463330 의 소진 복구 포함). 사용 중이면 표면 수를 0 으로 쓴다.
    /// 풀·프레임·지도는 읽지 않는다.
    /// </summary>
    public byte Allocate()
    {
        Plan plan = Calculate(0, Operation.Allocate, 0, null);
        Commit(plan);
        return (byte)plan.Changed;
    }

    /// <summary>공통 PostPop 이 요구하는 후처리 연결(영역·표면 등록)을 수행한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="region">영역 등록인지.</param>
    /// <param name="add">표면 생성 등록인지.</param>
    void IRawPostPopGraph.PostPop(int sid, bool region, bool add) => PostPop(sid, region, add);

    /// <summary>공통 PostPop 이 요구하는 사전 검사를 수행한다(쓰기 없음).</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="region">영역 등록인지.</param>
    /// <param name="add">표면 생성 등록인지.</param>
    /// <param name="projection">Pop 이 예측한 좌표·단계·타입(없으면 기본값).</param>
    void IRawPostPopGraph.ValidatePostPop(int sid, bool region, bool add, in PopProjection projection) =>
        ValidatePostPop(sid, region, add, projection.Type is null ? null : projection);

    /// <summary>
    /// 원본 계획 계산. 표·스택 사본에서 모든 검사와 계산을 끝내고 성공한 계획을 돌려준다.
    /// 거부 경로는 아무것도 바꾸지 않는다. 삭제 준비·전체 재구성은 이 조각에서 지원하지 않는다.
    /// </summary>
    /// <param name="sid">대상 슬롯 번호(Allocate 는 0).</param>
    /// <param name="operation">연산 종류.</param>
    /// <param name="target">Flood 목적 번호 또는 재구성 정책.</param>
    /// <param name="pop">Pop 예측값. 없으면 슬롯의 현재 좌표·단계를 읽는다.</param>
    private Plan Calculate(int sid, Operation operation, byte target, PopProjection? pop)
    {
        bool global = operation is Operation.Rebuild or Operation.Allocate;
        var plan = new Plan
        {
            Records = (GraphRecord[])_records.Clone(),
            Stack = (uint[])_stack.Clone(),
        };
        if (operation == Operation.Allocate)
        {
            // 원본 여유 번호 경로는 풀·프레임·지도를 읽지 않는다
            for (int i = 0; i < Count; i++)
            {
                if (plan.Records[i].InUse == 0)
                {
                    plan.Records[i].Surfaces = 0;
                    plan.Records[i].InUse = 1;
                    plan.Changed = (uint)i;
                    return plan;
                }
            }
        }
        if (operation is Operation.Detach or Operation.Rebuild)
        {
            throw new NotSupportedException("raw 그래프 삭제 준비·전체 재구성은 아직 옮기지 않았습니다");
        }
        bool region = operation is Operation.Region or Operation.RegionAdd;
        bool add = operation is Operation.Add or Operation.RegionAdd;
        bool fullPool = global || (_recovery == GraphRecovery.FullPool && add);
        ReadOnlySpan<byte> root = _pool.Slot(sid);
        if (!global && (sid < 5 || sid > MaximumId || (root[RawSidPool.StateOffset] & (FreeBit | ContainedBit)) != 0
            || root[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType || root[RawSidPool.TypeOffset] >= _types.Count
            || (operation != Operation.Region && (_types[root[RawSidPool.TypeOffset]].Flags1 & OriginalTypeRecord.Surface) == 0)))
        {
            throw new InvalidOperationException("raw 그래프 표면 SID 오류");
        }
        if (pop is { } projected)
        {
            OriginalTypeRecord type = _types[root[RawSidPool.TypeOffset]];
            if (operation == Operation.Flood || projected.Type is null || projected.Level < 0 || projected.Level > 3
                || type.Flags1 != projected.Type.Flags1 || type.Flags2 != projected.Type.Flags2
                || type.FootX != projected.Type.FootX || type.FootY != projected.Type.FootY)
            {
                throw new ArgumentException("Pop/raw 그래프 타입/단계 오류", nameof(pop));
            }
        }
        // 추가의 비활성 입력은 원본처럼 표·스택을 읽지 않고 돌아간다
        byte state = pop is null ? root[RawSidPool.StateOffset] : (byte)(root[RawSidPool.StateOffset] & ~VoidBit);
        if (add && (state & 7) != 0 && !region)
        {
            return plan;
        }
        ushort[] map = _hash.Entries(0).ToArray();
        byte[] spots = (byte[])_spots.Clone();
        if (pop is { } prediction)
        {
            int px = Coordinate(prediction.X);
            int py = Coordinate(prediction.Y);
            OriginalTypeRecord type = _types[root[RawSidPool.TypeOffset]];
            if (prediction.Level == 0)
            {
                map[py * RawSquidHash.WorldCells + px] = (ushort)sid;
            }
            if ((root[ExtraOffset] & BuriedBit) == 0)
            {
                if (type.FootX < 1 || type.FootY < 1 || px - type.FootX + 1 < 1 || py - type.FootY + 1 < 1)
                {
                    throw new ArgumentOutOfRangeException(nameof(pop), "raw 그래프 Pop 발자국 오류");
                }
                // 실제 Pop 의 y/x 순서와 genus 하위 BYTE OR 를 예측한다
                for (int cy = py - type.FootY + 1; cy <= py; cy++)
                {
                    // 각 발자국 행을 최종 점유로 만든다
                    for (int cx = px - type.FootX + 1; cx <= px; cx++)
                    {
                        int index = cy * RawSquidHash.WorldCells + cx;
                        spots[index] |= (byte)SpotRules.EffectiveGenus(type.Flags2, prediction.X, prediction.Y, type.FootX, type.FootY, cx, cy);
                    }
                }
            }
        }
        if (region)
        {
            Region(sid, pop, spots, plan);
        }
        if (operation == Operation.Region || (add && (state & 7) != 0))
        {
            return plan;
        }
        bool[] required = new bool[_pool.Capacity];
        if (!global)
        {
            required[sid] = true;
        }
        // 0단계 머리만 읽는다. 다른 해시 단계를 섞지 않는다
        for (int i = 0; i < map.Length; i++)
        {
            int id = map[i];
            if (id == 0)
            {
                continue;
            }
            if (id < 5 || id > MaximumId || id >= _pool.Capacity)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), id, "raw 그래프 해시 SID 오류");
            }
            ReadOnlySpan<byte> candidate = _pool.Slot(id);
            // 로컬 표면 스냅샷에만 비표면 필터를 적용한다
            if ((candidate[RawSidPool.StateOffset] & (FreeBit | ContainedBit | VoidBit)) == 0
                && candidate[RawSidPool.TypeOffset] >= OriginalConstructors.FirstAssetType
                && candidate[RawSidPool.TypeOffset] < _types.Count
                && (_types[candidate[RawSidPool.TypeOffset]].Flags1 & OriginalTypeRecord.Surface) == 0)
            {
                map[i] = 0;
                continue;
            }
            required[id] = true;
        }
        var objects = new List<SurfaceObject>();
        var members = new List<GraphMembership>();
        // 현재 살아 있는 표면과 지도에 실제 등장하는 비표면 후보를 읽는다
        for (int id = 5; id < _pool.Capacity; id++)
        {
            ReadOnlySpan<byte> bytes = _pool.Slot(id);
            byte currentState = pop is not null && id == sid ? state : bytes[RawSidPool.StateOffset];
            if ((currentState & (fullPool ? FreeBit : (FreeBit | VoidBit))) != 0)
            {
                if (required[id] && id != sid)
                {
                    throw new InvalidOperationException("raw 그래프 해시의 free/void SID");
                }
                if (id != sid)
                {
                    continue;
                }
            }
            if (bytes[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType)
            {
                if (required[id])
                {
                    throw new InvalidOperationException("raw 그래프 후보 타입 오류");
                }
                continue;
            }
            if (bytes[RawSidPool.TypeOffset] >= _types.Count)
            {
                if (required[id] || fullPool)
                {
                    throw new InvalidOperationException("raw 그래프 후보 타입 오류");
                }
                continue;
            }
            OriginalTypeRecord type = _types[bytes[RawSidPool.TypeOffset]];
            bool surface = (type.Flags1 & OriginalTypeRecord.Surface) != 0;
            if (!surface && !required[id])
            {
                continue;
            }
            if (id > MaximumId || ((currentState & ContainedBit) != 0 && (!fullPool || required[id]))
                || (fullPool && (currentState & VoidBit) != 0 && required[id]))
            {
                throw new ArgumentOutOfRangeException(nameof(sid), id, "raw 그래프 후보 상태/SID 범위 오류");
            }
            float x = pop is not null && id == sid ? pop.Value.X : BitConverter.UInt32BitsToSingle(Read(bytes, XOffset));
            float y = pop is not null && id == sid ? pop.Value.Y : BitConverter.UInt32BitsToSingle(Read(bytes, YOffset));
            FrameCode code = default;
            if (surface)
            {
                uint frame = Read(bytes, FrameOffset, 4);
                if (frame >= _frames[bytes[RawSidPool.TypeOffset]].Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), frame, "raw 그래프 프레임 번호 오류");
                }
                code = _frames[bytes[RawSidPool.TypeOffset]][(int)frame];
                int staged = StagedIndex(plan, id);
                members.Add(new GraphMembership(id, staged < 0 ? bytes[GraphOffset] : plan.Numbers[staged].Graph, currentState));
            }
            objects.Add(new SurfaceObject(id, Coordinate(x), Coordinate(y), type.FootX, type.FootY,
                type.Flags1, type.Flags2, code, (currentState & DeadBit) != 0, (bytes[ExtraOffset] & BuriedBit) != 0));
        }
        var finder = new SurfaceFinder(objects, map, spots);
        var graph = new SurfaceGraph(finder, members, plan.Records, plan.Stack);
        if (operation == Operation.Allocate)
        {
            plan.Changed = graph.AllocateWithRecovery();
        }
        else if (add)
        {
            graph.Add(sid, _recovery);
        }
        else
        {
            plan.Changed = graph.Flood(sid, target);
        }
        for (int i = 0; i < TableSize; i++)
        {
            plan.Records[i] = graph.Records[i];
        }
        for (int i = 0; i < FloodSize; i++)
        {
            plan.Stack[i] = graph.FloodStack[i];
        }
        // 영역에서 바꾼 번호도 최종 결과로 덮고 다른 raw 필드는 쓰지 않는다
        foreach (GraphMembership member in members)
        {
            MergeNumber(plan, member.Id, graph.Number(member.Id), member.Graph);
        }
        return plan;
    }

    /// <summary>
    /// 발자국 영역의 4단계 해시 후보를 훑어 걸리는 후보의 그래프 번호를 무효로 바꾼다(원본 00462d40).
    /// 단계마다 끝 버킷을 한 칸 늘려 큰 발자국 후보를 잡고, 각 버킷은 머리/next 순서를 따른다.
    /// 손상된 순환·범위 밖 번호는 계획 안에서 거부한다(원본 assert 대신).
    /// </summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="pop">Pop 예측값(그 단계·버킷은 새 객체를 먼저 넣은 것으로 본다).</param>
    /// <param name="spots">예측 반영이 끝난 점유 지도 사본.</param>
    /// <param name="plan">번호 변경을 쌓을 계획.</param>
    private void Region(int sid, PopProjection? pop, byte[] spots, Plan plan)
    {
        ReadOnlySpan<byte> root = _pool.Slot(sid);
        OriginalTypeRecord source = _types[root[RawSidPool.TypeOffset]];
        int x = Coordinate(pop is { } prediction ? prediction.X : BitConverter.UInt32BitsToSingle(Read(root, XOffset)));
        int y = Coordinate(pop is not null ? pop.Value.Y : BitConverter.UInt32BitsToSingle(Read(root, YOffset)));
        if (source.FootX < 1 || source.FootY < 1 || source.FootX > RawSquidHash.WorldCells || source.FootY > RawSquidHash.WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "그래프 영역 발자국 오류");
        }
        int left = Math.Max(0, x - source.FootX + 1);
        int top = Math.Max(0, y - source.FootY + 1);
        int right = x;
        int bottom = y;
        // 단계별 끝 버킷을 한 칸 늘리는 원본 탐색을 보존한다
        for (int level = 0; level < RawSquidHash.Levels; level++)
        {
            int scale = HashScale(level);
            int side = RawSquidHash.Side(level);
            int startX = left / scale;
            int startY = top / scale;
            int endX = Math.Min(side - 1, right / scale + 1);
            int endY = Math.Min(side - 1, bottom / scale + 1);
            ReadOnlySpan<ushort> heads = _hash.Entries(level);
            // 일반 탐색의 y 바깥/x 안쪽 순서다
            for (int cy = startY; cy <= endY; cy++)
            {
                // 최대 발자국이 더 큰 객체의 기준점도 끝 버킷에서 잡는다
                for (int cx = startX; cx <= endX; cx++)
                {
                    int index = cy * side + cx;
                    bool projected = pop is { } value && value.Level == level
                        && index == RawSquidHash.BucketIndex(level, value.X, value.Y);
                    int current = projected ? sid : heads[index];
                    var chain = new List<int>();
                    // 각 버킷의 실제 머리/next 순서로 조회하며 손상된 순환은 쓰기 전에 거부한다
                    while (current != 0)
                    {
                        if (current < 5 || current > MaximumId || current >= _pool.Capacity || chain.Contains(current))
                        {
                            throw new InvalidOperationException("그래프 영역 해시 범위/순환 오류");
                        }
                        chain.Add(current);
                        int candidate = current;
                        ReadOnlySpan<byte> bytes = _pool.Slot(candidate);
                        int next = pop is { } projectedValue && candidate == sid
                            ? _hash.Entries(projectedValue.Level)[RawSquidHash.BucketIndex(projectedValue.Level, projectedValue.X, projectedValue.Y)]
                            : (int)Read(bytes, RawSidPool.NextOffset, 2);
                        current = next;
                        if ((bytes[RawSidPool.StateOffset] & (FreeBit | ContainedBit)) != 0)
                        {
                            throw new InvalidOperationException("그래프 영역 후보 상태 오류");
                        }
                        if ((bytes[ExtraOffset] & BuriedBit) != 0)
                        {
                            continue;
                        }
                        if (bytes[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType || bytes[RawSidPool.TypeOffset] >= _types.Count)
                        {
                            throw new InvalidOperationException("그래프 영역 후보 타입 오류");
                        }
                        // 실제 일반 탐색의 비표면은 그래프를 바꾸지 않는다
                        if ((_types[bytes[RawSidPool.TypeOffset]].Flags1 & OriginalTypeRecord.Surface) == 0)
                        {
                            continue;
                        }
                        int ax = Coordinate(pop is { } currentProjection && candidate == sid
                            ? currentProjection.X
                            : BitConverter.UInt32BitsToSingle(Read(bytes, XOffset)));
                        int ay = Coordinate(pop is { } yProjection && candidate == sid
                            ? yProjection.Y
                            : BitConverter.UInt32BitsToSingle(Read(bytes, YOffset)));
                        if (ax <= 0 || ay <= 0)
                        {
                            throw new ArgumentOutOfRangeException(nameof(sid), "그래프 영역 후보의 유효 좌표가 필요합니다");
                        }
                        OriginalTypeRecord type = _types[bytes[RawSidPool.TypeOffset]];
                        if (type.FootX < 1 || type.FootY < 1 || type.FootX > RawSquidHash.WorldCells || type.FootY > RawSquidHash.WorldCells)
                        {
                            throw new ArgumentOutOfRangeException(nameof(sid), "그래프 영역 후보 발자국 오류");
                        }
                        int aLeft = ax - type.FootX + 1;
                        int aTop = ay - type.FootY + 1;
                        if (ax < left || aLeft > right || ay < top || aTop > bottom
                            || (spots[ay * RawSquidHash.WorldCells + ax] & 8) == 0)
                        {
                            continue;
                        }
                        int staged = StagedIndex(plan, candidate);
                        byte graph = staged < 0 ? bytes[GraphOffset] : plan.Numbers[staged].Graph;
                        if (graph != Invalid)
                        {
                            if (graph >= Count)
                            {
                                throw new ArgumentOutOfRangeException(nameof(sid), graph, "그래프 영역 후보 번호 오류");
                            }
                            ref GraphRecord record = ref plan.Records[graph];
                            // Remove 의 활성/양수 조건을 유지하며 0 이 되면 사용 여부도 지운다
                            if (record.InUse != 0 && record.Surfaces > 0)
                            {
                                record.Surfaces--;
                                if (record.Surfaces == 0)
                                {
                                    record.InUse = 0;
                                }
                            }
                            if (staged < 0)
                            {
                                plan.Numbers.Add((candidate, Invalid));
                            }
                            else
                            {
                                plan.Numbers[staged] = (candidate, Invalid);
                            }
                        }
                    }
                }
            }
        }
    }

    /// <summary>단계별 버킷 한 칸이 맡는 월드 칸 수(원본 005424f8: 1·2·4·16).</summary>
    /// <param name="level">해시 단계 0~3.</param>
    private static int HashScale(int level) => level switch
    {
        0 => 1,
        1 => 2,
        2 => 4,
        _ => 16,
    };

    /// <summary>계획의 슬롯 번호 목록에서 대상의 위치를 찾는다. 없으면 -1.</summary>
    /// <param name="plan">대상 계획.</param>
    /// <param name="sid">찾을 슬롯 번호.</param>
    private static int StagedIndex(Plan plan, int sid) => plan.Numbers.FindIndex(item => item.Sid == sid);

    /// <summary>최종 그래프 번호를 계획에 합친다. 이미 있으면 값을 바꾸고, 없고 원래와 다르면 새로 적는다.</summary>
    /// <param name="plan">대상 계획.</param>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="number">최종 그래프 번호.</param>
    /// <param name="previous">스냅샷을 읽을 때의 그래프 번호.</param>
    private static void MergeNumber(Plan plan, int sid, byte number, byte previous)
    {
        int staged = StagedIndex(plan, sid);
        if (staged >= 0)
        {
            plan.Numbers[staged] = (sid, number);
        }
        else if (number != previous)
        {
            plan.Numbers.Add((sid, number));
        }
    }

    /// <summary>성공한 계획의 슬롯 그래프 byte·표·스택만 반영한다. 좌표·소유자·상태·payload 는 건드리지 않는다.</summary>
    /// <param name="plan">반영할 계획.</param>
    private void Commit(Plan plan)
    {
        // 성공한 계산의 번호 변경만 raw 풀에 쓴다
        foreach ((int sid, byte number) in plan.Numbers)
        {
            _pool.AllocatedBytes(sid)[GraphOffset] = number;
        }
        Array.Copy(plan.Records, _records, TableSize);
        Array.Copy(plan.Stack, _stack, FloodSize);
    }

    /// <summary>
    /// 지도 안 정수 좌표만 받는다. 원본은 float 좌표를 그대로 비교하지만, 이 경로는 정상 입력만 받으므로
    /// 유한하지 않거나 지도 밖이거나 소수부가 있으면 쓰기 전에 거부한다.
    /// </summary>
    /// <param name="value">월드 좌표.</param>
    private static int Coordinate(float value)
    {
        if (!float.IsFinite(value) || value < 0 || value >= RawSquidHash.WorldCells || Math.Truncate(value) != value)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "raw 그래프는 지도 안 정수 좌표가 필요합니다");
        }
        return (int)value;
    }

    /// <summary>슬롯 안의 정렬되지 않은 리틀 엔디언 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽을 바이트 수(1~4).</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }
        return value;
    }
}
