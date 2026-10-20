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
}
