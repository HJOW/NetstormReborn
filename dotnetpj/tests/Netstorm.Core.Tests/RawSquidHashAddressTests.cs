// 공간 해시의 초기화·버킷 주소·선택 단계를 실제 기계어 기대값으로 대조한다.
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 공간 해시 주소를 검증하는 테스트 클래스다. 2026-11-07 F2d 저장소 조립에서 추가됐다.
public sealed class RawSquidHashAddressTests
{
    /// <summary>실제 네 초기화와 반환 주소 807개를 같은 배열의 인덱스와 비교한다. 2026-11-07 F2d 저장소 조립에서 추가됐다.</summary>
    [Fact]
    public void HashReplaysNativeInitializationAndBucketAddresses()
    {
        string[][] rows = X86Fixture.Read("hash");
        var hash = new RawSquidHash();
        int initialized = 0;
        int at = 0;
        int integer = 0;
        int floating = 0;
        // 네이티브 주소 결과와 상태 필드를 직접 비교하는 반복문이다. 2026-11-07 F2d 저장소 조립에서 추가됐다.
        foreach (string[] row in rows)
        {
            if (row[0] == "Init")
            {
                hash.Select(X86Fixture.Int(row[1]));
                hash.At(0, 0) = 7;
                hash.Reset();
                Assert.Equal(X86Fixture.Int(row[2]), hash.Selected());
                Assert.Equal(X86Fixture.Int(row[3]), RawSquidHash.WorldCells / RawSquidHash.Side(hash.Selected()));
                Assert.Equal(X86Fixture.Int(row[4]), RawSquidHash.Side(hash.Selected()));
                // 원본이 전체 배열을 지운 결과와 비교한다
                for (int level = 0; level < RawSquidHash.Levels; ++level)
                {
                    Assert.True(hash.Entries(level).ToArray().All(id => id == 0));
                }

                ++initialized;
            }
            else if (row[0] == "At" || row[0] == "IntBucket" || row[0] == "FloatBucket")
            {
                int level = X86Fixture.Int(row[1]);
                int expected = X86Fixture.Int(row[4]);
                ReadOnlySpan<ushort> entries = hash.Entries(level);
                ref ushort head = ref MemoryMarshal.GetReference(entries);
                if (row[0] == "At")
                {
                    hash.Select(level);
                    Assert.True(Unsafe.AreSame(ref Unsafe.Add(ref head, expected), ref hash.At(X86Fixture.Int(row[2]), X86Fixture.Int(row[3]))));
                    ++at;
                }
                else if (row[0] == "IntBucket")
                {
                    Assert.True(Unsafe.AreSame(ref Unsafe.Add(ref head, expected), ref hash.Cell(level, X86Fixture.Int(row[2]), X86Fixture.Int(row[3]))));
                    ++integer;
                }
                else
                {
                    Assert.True(Unsafe.AreSame(ref Unsafe.Add(ref head, expected), ref hash.Bucket(level, X86Fixture.FloatBits(row[2]), X86Fixture.FloatBits(row[3]))));
                    ++floating;
                }
            }
        }

        Assert.Equal(4, initialized);
        Assert.Equal(100, at);
        Assert.Equal(100, integer);
        Assert.Equal(607, floating);
    }

    /// <summary>단계별 머리 배열의 독립성과 초기화가 선택 단계·할당 주소를 유지하는지 확인한다. 2026-11-07 F2d 저장소 조립에서 추가됐다.</summary>
    [Fact]
    public void IndependentLevelsAndCompleteReset()
    {
        var hash = new RawSquidHash();
        hash.Select(3);
        // 각 단계의 시작·마지막 버킷을 다른 값으로 채운다
        for (int level = 0; level < RawSquidHash.Levels; ++level)
        {
            int last = RawSquidHash.Side(level) - 1;
            hash.Cell(level, 0, 0) = (ushort)(level + 1);
            hash.Cell(level, last, last) = (ushort)(level + 11);
        }

        Assert.Equal(4, hash.At(0, 0));
        Assert.Equal(13, hash.Bucket(2, 255.9f, 255.9f));
        hash.Reset();
        Assert.Equal(3, hash.Selected());
        // 다른 단계도 모두 지워졌으며 공간 체인으로 추정한 번호가 남지 않는다
        for (int level = 0; level < RawSquidHash.Levels; ++level)
        {
            Assert.True(hash.Entries(level).ToArray().All(id => id == 0));
        }
    }

    /// <summary>선택·At·가변 항목의 읽기쓰기가 같은 실제 저장소에 닿는다. 2026-11-07 F2d 저장소 조립에서 추가됐다.</summary>
    [Fact]
    public void SelectAtAndMutableEntriesShareOneStorage()
    {
        var hash = new RawSquidHash();
        Assert.Equal(0, hash.Selected());
        hash.Select(2);
        Assert.Equal(2, hash.Selected());
        hash.At(3, 5) = 9;
        Assert.Equal(9, hash.Cell(2, 3, 5));
        Span<ushort> mutable = hash.MutableEntries(2);
        mutable[5 * RawSquidHash.Side(2) + 3] = 12;
        Assert.Equal(12, hash.At(3, 5));
        Assert.Equal(12, hash.Entries(2)[5 * RawSquidHash.Side(2) + 3]);
        hash.Select(0);
        Assert.Equal(0, hash.At(0, 0));
    }

    /// <summary>원본이 정의하지 않은 배열 밖 입력을 메모리 주소 계산으로 진행하지 않는다. 2026-11-07 F2d 저장소 조립에서 추가됐다.</summary>
    [Fact]
    public void RejectsInvalidLevelsCoordinatesAndSizes()
    {
        var hash = new RawSquidHash();
        Assert.Throws<ArgumentOutOfRangeException>(() => hash.Select(4));
        Assert.Equal(0, hash.Selected());
        Assert.Throws<ArgumentOutOfRangeException>(() => hash.Cell(3, 16, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => hash.At(-1, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => hash.Bucket(0, 256f, 0f));
        Assert.Throws<ArgumentOutOfRangeException>(() => hash.Bucket(0, float.NaN, 0f));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.ObjectLevel(0u, -1f, 1f));
        Assert.Throws<ArgumentOutOfRangeException>(() => hash.MutableEntries(4));
    }
}
