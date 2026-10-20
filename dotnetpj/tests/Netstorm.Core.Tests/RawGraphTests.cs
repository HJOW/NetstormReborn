using Netstorm.Core.Bridges;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 그래프 표 저장소(<see cref="RawGraph"/>)의 경계를 확인한다.
/// 원본 기대값 재생(X86RawGraphTests)이 초기화 결과를 대조하고, 이 검사들은 cpppj RawGraph 의 크기 검사·Free 번호 범위·
/// 초기화가 건드리지 않는 범위(251~253·예약 WORD·스택·풀)·공간/타입 일치 계약을 10.78 로 옮긴다.
/// 근거: cpppj/src/o/RawGraph.cpp (W02 열셋째 조각, 2026-10-20).
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
}
