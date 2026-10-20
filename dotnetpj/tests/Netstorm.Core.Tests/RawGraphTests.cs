using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 그래프 저장소·연산(<see cref="RawGraph"/>)의 경계를 확인한다.
/// 원본 기대값 재생(X86RawGraphTests·X86RawGraphReplayTests)이 초기화·영역·추가/flood 결과를 대조하고,
/// 이 검사들은 크기 검사·Free 번호 범위·초기화 보존 범위·공간/타입 일치·비활성 입력·비표면 거부·빈 번호 할당·
/// flood 의 그래프 byte 외 불변을 10.78 로 옮긴다.
/// 근거: cpppj/src/o/RawGraph.cpp (W02 열셋째·열넷째 조각, 2026-10-20).
/// </summary>
public sealed class RawGraphTests
{
    /// <summary>생성자 주소만 채운 10.78 타입 레코드 188개.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, OriginalConstructors.TypeCount)
        .Select(number => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(number) })
        .ToArray();

    /// <summary>표·스택 크기가 정확한 기본 입력을 만든다.</summary>
    private static (RawSidPool Pool, RawSquidHash Hash, byte[] Spots) Space()
    {
        var pool = new RawSidPool(24000, false);
        var hash = new RawSquidHash();
        return (pool, hash, new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells]);
    }

    /// <summary>표면 타입 74(플래그1=표면, 발자국 1×1)와 프레임 하나를 가진 장면을 만든다.</summary>
    private static (RawSidPool Pool, byte[] Spots, OriginalTypeRecord[] Types, RawGraph Graph) SurfaceScene(
        IReadOnlyList<GraphRecord>? records = null)
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        OriginalTypeRecord[] types = Types();
        types[74].Flags1 = OriginalTypeRecord.Surface;
        types[74].FootX = types[74].FootY = 1;
        List<IReadOnlyList<FrameCode>> frames = Enumerable.Range(0, types.Length)
            .Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode> { new FrameCode('A', 'P', 1, FrameCodeFlags.None) }).ToList();
        var graph = new RawGraph(pool, hash, spots, types, records, null, frames);
        return (pool, spots, types, graph);
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>클라이언트 슬롯을 확보해 표면 필드(가상 표·좌표·타입·상태·그래프·프레임)를 채운다.</summary>
    /// <param name="pool">대상 풀.</param>
    /// <param name="x">월드 x.</param>
    /// <param name="y">월드 y.</param>
    /// <param name="graph">그래프 번호.</param>
    /// <param name="state">상태 바이트(기본 void=4).</param>
    private static int Surface(RawSidPool pool, float x, float y, byte graph, byte state = 4)
    {
        int sid = pool.Allocate(SidAllocation.Client);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes.Clear();
        Put(bytes, 0, OriginalConstructors.BaseVtable);
        Put(bytes, 14, BitConverter.SingleToUInt32Bits(x));
        Put(bytes, 18, BitConverter.SingleToUInt32Bits(y));
        bytes[10] = 74;
        bytes[11] = state;
        bytes[30] = graph;
        bytes[34] = 1;
        Put(bytes, 36, 0);
        return sid;
    }

    /// <summary>표·스택·타입·지도 크기가 다르면 만들기 전에 거부한다.</summary>
    [Fact]
    public void ConstructorRejectsWrongStorageSizes()
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        OriginalTypeRecord[] types = Types();
        Assert.Throws<ArgumentException>(() => new RawGraph(pool, hash, spots, types.Take(187).ToArray()));
        Assert.Throws<ArgumentException>(() => new RawGraph(pool, hash, new byte[100], types));
        Assert.Throws<ArgumentException>(() => new RawGraph(pool, hash, spots, types, new GraphRecord[254]));
        Assert.Throws<ArgumentException>(() => new RawGraph(pool, hash, spots, types, null, new uint[100]));
    }

    /// <summary>
    /// 레코드를 주지 않으면 254 sentinel 만 0x7dfd 로 채운다. 초기화는 0번·254 를 원본 값으로 만들고
    /// 251~253 과 예약 WORD 의 윗 BYTE, 스택, 풀을 건드리지 않는다.
    /// </summary>
    [Fact]
    public void ResetTablePreservesGapReservedBitsStackAndPool()
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        OriginalTypeRecord[] types = Types();
        var records = new GraphRecord[RawGraph.TableSize];
        // 251~253(틈)과 0번의 예약 WORD 윗 BYTE 를 서로 다른 값으로 채운다
        for (int i = 0; i < records.Length; i++)
        {
            records[i] = new GraphRecord { Surfaces = 7, InUse = 9, Reserved = unchecked((short)0xA5A5) };
        }
        records[0].Reserved = unchecked((short)0x5A00);
        uint[] stack = Enumerable.Repeat(0xCAFEBABEU, RawGraph.FloodSize).ToArray();
        uint freeCount = pool.FreeCount;
        var graph = new RawGraph(pool, hash, spots, types, records, stack);
        graph.ResetTable();
        IReadOnlyList<GraphRecord> result = graph.Records;
        Assert.Equal(1, result[0].InUse);
        Assert.Equal(1, result[0].Reserved & 0xFF);
        Assert.Equal(0x5A, (result[0].Reserved >> 8) & 0xFF);
        Assert.Equal(0, result[1].Surfaces);
        Assert.Equal(0, result[1].InUse);
        Assert.Equal(unchecked((short)0xA5A5), result[1].Reserved);
        // 251~253 은 원본 표의 틈이라 그대로 남는다
        for (int i = 251; i <= 253; i++)
        {
            Assert.Equal(7, result[i].Surfaces);
            Assert.Equal(9, result[i].InUse);
        }
        Assert.Equal(0x7DFD, result[RawGraph.Invalid].Surfaces);
        Assert.Equal(0x7DFD, result[RawGraph.Invalid].InUse);
        Assert.All(graph.FloodStack, value => Assert.Equal(0xCAFEBABEU, value));
        // 풀은 바뀌지 않는다
        Assert.Equal(freeCount, pool.FreeCount);
        // 레코드를 주지 않은 표는 sentinel 만 채운다
        var empty = new RawGraph(pool, hash, spots, types);
        Assert.Equal(0x7DFD, empty.Records[RawGraph.Invalid].Surfaces);
        Assert.Equal(0x7DFD, empty.Records[RawGraph.Invalid].InUse);
        Assert.Equal(0, empty.Records[0].InUse);
        Assert.All(empty.FloodStack, value => Assert.Equal(0U, value));
    }

    /// <summary>Free 는 0~250 과 254 만 받고, 254 는 sentinel 을 보존하며, 유효 번호는 예약 WORD 를 남긴다.</summary>
    [Fact]
    public void FreeRejectsGapsAndPreservesReservedWord()
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        var graph = new RawGraph(pool, hash, spots, Types());
        // 251~253(틈)은 Free 가 거부해야 한다
        for (byte gap = 251; gap <= 253; gap++)
        {
            Assert.Throws<ArgumentOutOfRangeException>(() => graph.Free(gap));
        }
        Assert.Throws<ArgumentOutOfRangeException>(() => graph.Free(255));
        graph.ResetTable();
        graph.Free(RawGraph.Invalid);
        Assert.Equal(0x7DFD, graph.Records[RawGraph.Invalid].Surfaces);
        graph.Free(0);
        Assert.Equal(0, graph.Records[0].Surfaces);
        Assert.Equal(0, graph.Records[0].InUse);
        Assert.Equal(1, graph.Records[0].Reserved & 0xFF);
    }

    /// <summary>같은 공간·같은 타입 표만 받고, 내용이 같은 다른 객체는 거부한다.</summary>
    [Fact]
    public void ValidateSpaceAndTypesRequireSameBodies()
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        OriginalTypeRecord[] types = Types();
        var graph = new RawGraph(pool, hash, spots, types);
        graph.ValidateSpace(hash, spots);
        graph.ValidateTypes(types);
        Assert.Throws<ArgumentException>(() => graph.ValidateSpace(new RawSquidHash(), spots));
        Assert.Throws<ArgumentException>(() => graph.ValidateSpace(hash, new byte[spots.Length]));
        // 같은 값의 다른 타입 표는 필드 비교를 통과한다
        graph.ValidateTypes(Types());
        Assert.Throws<ArgumentException>(() => graph.ValidateTypes(types.Take(187).ToArray()));
        OriginalTypeRecord[] changed = Types();
        changed[74].Flags2 = 2;
        Assert.Throws<ArgumentException>(() => graph.ValidateTypes(changed));
        changed = Types();
        changed[74].FootX = 3;
        Assert.Throws<ArgumentException>(() => graph.ValidateTypes(changed));
    }

    /// <summary>비활성(void/dead) 원천의 Add 는 표·스택·슬롯을 바꾸지 않고 돌아간다.</summary>
    [Fact]
    public void AddSkipsInactiveSourceWithoutChanges()
    {
        (RawSidPool pool, byte[] spots, OriginalTypeRecord[] types, RawGraph graph) = SurfaceScene();
        int sid = Surface(pool, 10, 10, 3);
        byte[] before = pool.Bytes.ToArray();
        graph.Add(sid);
        // 표는 sentinel 만 남고, 슬롯 그래프 byte 도 그대로다
        Assert.Equal(0, graph.Records[0].InUse);
        Assert.Equal(3, pool.Slot(sid)[30]);
        Assert.Equal(before, pool.Bytes.ToArray());
        Assert.All(spots, spot => Assert.Equal(0, spot));
        _ = types;
    }

    /// <summary>표면 타입이 아닌 슬롯의 Add 는 표를 바꾸기 전에 거부한다.</summary>
    [Fact]
    public void AddRejectsNonSurfaceType()
    {
        (RawSidPool pool, byte[] spots, OriginalTypeRecord[] types, RawGraph graph) = SurfaceScene();
        types[74].Flags1 = 0;
        int sid = Surface(pool, 10, 10, 3);
        byte[] before = pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => graph.Add(sid));
        Assert.Equal(before, pool.Bytes.ToArray());
        Assert.All(spots, spot => Assert.Equal(0, spot));
    }

    /// <summary>빈 번호 할당은 가장 낮은 미사용 번호를 쓰고 예약 WORD 를 보존한다.</summary>
    [Fact]
    public void AllocateUsesLowestFreeAndPreservesReserved()
    {
        (RawSidPool pool, byte[] spots, OriginalTypeRecord[] types, RawGraph graph) = SurfaceScene();
        Assert.Equal(0, graph.Allocate());
        Assert.Equal(1, graph.Records[0].InUse);
        Assert.Equal(0, graph.Records[0].Surfaces);
        Assert.Equal(1, graph.Allocate());
        _ = pool;
        _ = spots;
        _ = types;
    }

    /// <summary>
    /// 0~250번이 모두 사용 중이면 원본 00463330 의 소진 경로로 전체를 재구성한 뒤 번호를 확보한다.
    /// 빈 번호가 없을 때 `AllocateForOperation(FullPool)` 은 표 앞 251개를 지우고(0번 사용 표시·예약 하위 바이트·254 sentinel 복원)
    /// 살아 있는 표면들을 임시 번호에 모았다가 무리마다 새 번호로 flood 한 뒤 임시 번호를 비우고 첫 빈 번호를 돌려준다
    /// (cpppj `Graph::AllocateForOperation`·`RebuildImpl`, `RawGraph::Calculate` 의 Allocate 경로, 2026-10-20).
    /// 그래프 5번에 있던 표면 하나는 새 무리 번호 2번에 다시 묶이고, 확보 번호로 빈 1번을 돌려준다.
    /// </summary>
    [Fact]
    public void AllocateRecoversExhaustedTableViaRebuild()
    {
        var records = new GraphRecord[RawGraph.TableSize];
        // 0~250번을 모두 사용 중으로 채워 소진 상태를 만든다
        for (int i = 0; i < RawGraph.Count; i++)
        {
            records[i] = new GraphRecord { Surfaces = 5, InUse = 1 };
        }
        // 251~253(틈)은 재구성이 건드리지 않아야 한다
        for (int i = 251; i <= 253; i++)
        {
            records[i] = new GraphRecord { Surfaces = 7, InUse = 9, Reserved = unchecked((short)0xA5A5) };
        }
        records[RawGraph.Invalid] = new GraphRecord { Surfaces = 0x7DFD, InUse = 0x7DFD };
        (RawSidPool pool, byte[] spots, OriginalTypeRecord[] types, RawGraph graph) = SurfaceScene(records);
        int sid = Surface(pool, 10, 10, 5, 0);
        byte[] before = pool.Bytes.ToArray();
        byte number = graph.Allocate();
        // 재구성 뒤 첫 빈 번호 1번을 돌려준다
        Assert.Equal(1, number);
        IReadOnlyList<GraphRecord> result = graph.Records;
        Assert.Equal(0, result[0].Surfaces);
        Assert.Equal(1, result[0].InUse);
        Assert.Equal(1, result[0].Reserved & 0xFF);
        Assert.Equal(0, result[1].Surfaces);
        Assert.Equal(1, result[1].InUse);
        // 살아 있던 표면 하나가 새 무리 번호 2번에 다시 묶인다
        Assert.Equal(1, result[2].Surfaces);
        Assert.Equal(1, result[2].InUse);
        // 나머지 번호는 비어 있다
        for (int i = 3; i < RawGraph.Count; i++)
        {
            Assert.Equal(0, result[i].Surfaces);
            Assert.Equal(0, result[i].InUse);
        }
        // 251~253(틈)과 254 sentinel 은 원본 값을 유지한다
        for (int i = 251; i <= 253; i++)
        {
            Assert.Equal(7, result[i].Surfaces);
            Assert.Equal(9, result[i].InUse);
            Assert.Equal(unchecked((short)0xA5A5), result[i].Reserved);
        }
        Assert.Equal(0x7DFD, result[RawGraph.Invalid].Surfaces);
        Assert.Equal(0x7DFD, result[RawGraph.Invalid].InUse);
        // flood 스택에는 표면 번호만 남고 나머지는 그대로다
        Assert.Equal((uint)sid, graph.FloodStack[0]);
        // 나머지 스택 칸은 그대로 0이다
        for (int i = 1; i < RawGraph.FloodSize; i++)
        {
            Assert.Equal(0U, graph.FloodStack[i]);
        }
        // 슬롯은 그래프 byte 하나만 5번에서 2번으로 바뀐다
        for (int i = 0; i < before.Length; i++)
        {
            byte expected = i == sid * RawSidPool.Stride + 30 ? (byte)2 : before[i];
            Assert.Equal(expected, pool.Bytes[i]);
        }
        // 점유 지도는 건드리지 않는다
        Assert.All(spots, spot => Assert.Equal(0, spot));
        _ = types;
    }

    /// <summary>Flood 는 그래프 byte 만 쓰고 좌표·소유자·상태·payload 는 보존한다.</summary>
    [Fact]
    public void FloodWritesOnlyGraphByte()
    {
        var records = new GraphRecord[RawGraph.TableSize];
        records[3] = new GraphRecord { Surfaces = 1, InUse = 1 };
        (RawSidPool pool, byte[] spots, OriginalTypeRecord[] types, RawGraph graph) = SurfaceScene(records);
        int sid = Surface(pool, 10, 10, 3);
        byte[] before = pool.Slot(sid).ToArray();
        Assert.Equal(1U, graph.Flood(sid, 4));
        ReadOnlySpan<byte> after = pool.Slot(sid);
        // 그래프 byte 하나만 바뀐다
        for (int i = 0; i < before.Length; i++)
        {
            byte expected = i == 30 ? (byte)4 : before[i];
            Assert.Equal(expected, after[i]);
        }
        Assert.Equal(0, graph.Records[3].Surfaces);
        Assert.Equal(1, graph.Records[4].Surfaces);
        _ = spots;
        _ = types;
    }

    /// <summary>
    /// 위치 조회의 무효·미사용 번호는 finder 를 만들기 전에 자연 반환한다.
    /// 원천 프레임이 깨져도 표·스택·풀·지도를 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void DetachInvalidOrUnusedHeadReturnsWithoutChanges()
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        OriginalTypeRecord[] types = Types();
        types[74].Flags1 = OriginalTypeRecord.Surface;
        types[74].FootX = types[74].FootY = 1;
        List<IReadOnlyList<FrameCode>> frames = Enumerable.Range(0, types.Length)
            .Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode> { new FrameCode('A', 'P', 1, FrameCodeFlags.None) }).ToList();
        var records = new GraphRecord[RawGraph.TableSize];
        records[0] = new GraphRecord { Surfaces = 3, InUse = 1 };
        records[1] = new GraphRecord { Surfaces = 4, InUse = 0 };
        var graph = new RawGraph(pool, hash, spots, types, records, null, frames);
        int root = Surface(pool, 20, 20, 0, 2);
        int head = Surface(pool, 20, 20, 0, 0);
        // 같은 위치의 조회 머리를 가리킨다
        hash.Bucket(0, 20, 20) = (ushort)head;
        byte[] beforeSpots = spots.ToArray();
        // 무효 번호와 미사용 번호는 잘못된 원천 프레임을 허용하고 그대로 둔다
        foreach (byte number in new byte[] { RawGraph.Invalid, 1 })
        {
            pool.AllocatedBytes(head)[30] = number;
            // 깨진 원천 프레임을 미리 넣어 둔다
            Put(pool.AllocatedBytes(root), 36, 99);
            byte[] beforePool = pool.Bytes.ToArray();
            graph.ValidateDetach(root);
            graph.Detach(root);
            Assert.Equal(beforePool, pool.Bytes.ToArray());
            Assert.Equal(beforeSpots, spots);
            Assert.Equal(3, graph.Records[0].Surfaces);
            Assert.Equal(4, graph.Records[1].Surfaces);
            Assert.Equal(0, graph.Records[1].InUse);
        }
        _ = types;
    }

    /// <summary>
    /// 죽지 않은 원천·잘못된 감소 수·손상된 조회 머리는 표·스택·풀·지도를 바꾸기 전에 거부한다.
    /// </summary>
    [Fact]
    public void DetachPreflightRejectsBeforeMutation()
    {
        (RawSidPool pool, RawSquidHash hash, byte[] spots) = Space();
        OriginalTypeRecord[] types = Types();
        types[74].Flags1 = OriginalTypeRecord.Surface;
        types[74].FootX = types[74].FootY = 1;
        List<IReadOnlyList<FrameCode>> frames = Enumerable.Range(0, types.Length)
            .Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode> { new FrameCode('A', 'P', 1, FrameCodeFlags.None) }).ToList();
        var records = new GraphRecord[RawGraph.TableSize];
        records[0] = new GraphRecord { Surfaces = 3, InUse = 1 };
        var graph = new RawGraph(pool, hash, spots, types, records, null, frames);
        int root = Surface(pool, 20, 20, 0, 2);
        int head = Surface(pool, 20, 20, 0, 0);
        // 같은 위치의 조회 머리를 가리킨다
        hash.Bucket(0, 20, 20) = (ushort)head;
        byte[] beforePool = pool.Bytes.ToArray();
        byte[] beforeSpots = spots.ToArray();
        // 감소 수는 1 또는 9만 받는다
        Assert.Throws<ArgumentException>(() => graph.ValidateDetach(root, false, 2));
        Assert.Throws<ArgumentException>(() => graph.Detach(root, false, 2));
        // 죽지 않은 원천은 거부한다
        pool.AllocatedBytes(root)[11] = 0;
        Assert.Throws<ArgumentException>(() => graph.Detach(root));
        pool.AllocatedBytes(root)[11] = 2;
        // 조회 머리의 좌표가 다르면 위치 불일치로 거부한다
        Put(pool.AllocatedBytes(head), 14, BitConverter.SingleToUInt32Bits(30));
        Put(pool.AllocatedBytes(head), 18, BitConverter.SingleToUInt32Bits(30));
        Assert.Throws<InvalidOperationException>(() => graph.ValidateDetach(root));
        Assert.Throws<InvalidOperationException>(() => graph.Detach(root));
        // 좌표를 되돌리면 풀·지도는 그대로다
        Put(pool.AllocatedBytes(head), 14, BitConverter.SingleToUInt32Bits(20));
        Put(pool.AllocatedBytes(head), 18, BitConverter.SingleToUInt32Bits(20));
        Assert.Equal(beforePool, pool.Bytes.ToArray());
        Assert.Equal(beforeSpots, spots);
        _ = types;
    }
}
