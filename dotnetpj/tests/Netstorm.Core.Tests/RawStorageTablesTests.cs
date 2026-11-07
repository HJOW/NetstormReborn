// 저장소 물리 자료형의 기본값·부분 reset을 검증한다.
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// 저장소 물리 자료형을 검증하는 테스트 클래스다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
public sealed class RawStorageTablesTests
{
    /// <summary>세 표는 0으로 시작하고 reset은 앞 255칸만 지우며 256번째 칸을 보존한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void TotalMadeDefaultsAndPartialReset()
    {
        var storage = new RawTotalMadeStorage();
        Assert.True(storage.LocalCounts.All(value => value == 0u));
        Assert.True(storage.LocalSecondaryCounts.All(value => value == 0u));
        Assert.True(storage.GlobalCounts.All(value => value == 0u));
        Array.Fill(storage.LocalCounts, 0xDEADu);
        Array.Fill(storage.LocalSecondaryCounts, 0xBEEFu);
        Array.Fill(storage.GlobalCounts, 0x1234u);
        RawTotalMadeStorage.ResetCounts(storage);
        // 앞 255칸만 0이며 256번째 물리 칸을 보존하는 반복문이다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
        for (int index = 0; index < 255; ++index)
        {
            Assert.Equal(0u, storage.LocalCounts[index]);
            Assert.Equal(0u, storage.LocalSecondaryCounts[index]);
            Assert.Equal(0u, storage.GlobalCounts[index]);
        }

        Assert.Equal(0xDEADu, storage.LocalCounts[255]);
        Assert.Equal(0xBEEFu, storage.LocalSecondaryCounts[255]);
        Assert.Equal(0x1234u, storage.GlobalCounts[255]);
        Assert.Throws<ArgumentNullException>(() => RawTotalMadeStorage.ResetCounts(null!));
    }

    /// <summary>표 초기화는 앞 251 두 WORD·0번 사용·예약 하위·sentinel만 쓰고 나머지를 보존한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void GraphDefaultsAndTableReset()
    {
        var storage = new RawGraphStorage();
        Assert.True(storage.Records.All(record => record.Surfaces == 0 && record.InUse == 0 && record.Reserved == 0));
        Assert.True(storage.Stack.All(value => value == 0u));
        // 전체를 표식으로 채워 보존 범위를 가리는 반복문이다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
        for (int index = 0; index < storage.Records.Length; ++index)
        {
            storage.Records[index].Surfaces = 7;
            storage.Records[index].InUse = 8;
            storage.Records[index].Reserved = 0x1234;
        }

        Array.Fill(storage.Stack, 0xABCDu);
        RawGraphStorage.ResetTable(storage);
        // 1~250번의 두 WORD가 지워졌는지 확인하는 반복문이다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
        for (int index = 1; index < SurfaceGraph.Count; ++index)
        {
            Assert.Equal(0, storage.Records[index].Surfaces);
            Assert.Equal(0, storage.Records[index].InUse);
        }

        Assert.Equal(0, storage.Records[0].Surfaces);
        Assert.Equal(1, storage.Records[0].InUse);
        Assert.Equal(0x1201, storage.Records[0].Reserved);
        // 틈 3행은 손대지 않는다
        for (int index = SurfaceGraph.Count; index < SurfaceGraph.Invalid; ++index)
        {
            Assert.Equal(7, storage.Records[index].Surfaces);
            Assert.Equal(8, storage.Records[index].InUse);
            Assert.Equal(0x1234, storage.Records[index].Reserved);
        }

        Assert.Equal(0x7DFD, storage.Records[SurfaceGraph.Invalid].Surfaces);
        Assert.Equal(0x7DFD, storage.Records[SurfaceGraph.Invalid].InUse);
        Assert.True(storage.Stack.All(value => value == 0xABCDu));
        Assert.Throws<ArgumentNullException>(() => RawGraphStorage.ResetTable(null!));
    }

    /// <summary>거리표·큐는 0으로 시작하고 크기가 원문과 같다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void PathSearchWorkDefaults()
    {
        var work = new RawPathSearchWork();
        Assert.Equal(65536, work.Distance.Length);
        Assert.Equal(32768, work.Queue.Length);
        Assert.True(work.Distance.All(value => value == 0));
        Assert.True(work.Queue.All(value => value == 0));
    }

    /// <summary>탐색 전역은 원문 PE 초기값으로 시작한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void PathSearchStateDefaults()
    {
        var state = new RawPathSearchState();
        Assert.True(state.GraphsEnabled);
        Assert.True(state.SearchEnabled);
        Assert.True(state.AvoidFlagTwo);
        Assert.Equal(6, state.PolicySelector);
        Assert.Equal([0u, 5u, 0u, 20u, 0u, 80u, 0u, 65u], state.DirectionPrerequisite);
        Assert.Equal([0, 2, 4, 6, 1, 3, 5, 7], state.DirectionOrder);
        Assert.Equal(90u, state.FenceMarkType);
        Assert.Equal(140u, state.WindArcherType);
    }

    /// <summary>공유 소유자는 비어 시작하고 같은 작업 배열을 pin한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    [Fact]
    public void PathSearchStorageSharesWork()
    {
        var storage = new RawPathSearchStorage();
        Assert.Null(storage.Work);
        Assert.False(storage.Initialized);
        var work = new RawPathSearchWork();
        storage.Work = work;
        storage.Initialized = true;
        Assert.Same(work, storage.Work);
        Assert.True(storage.Initialized);
    }
}
