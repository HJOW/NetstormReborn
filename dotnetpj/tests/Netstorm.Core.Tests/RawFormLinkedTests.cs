using System.Buffers.Binary;
using Netstorm.Core.Original;

// 공통 Form 부모 체인의 원본 관측을 검증하는 네임스페이스다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj/tests/RawFormLinkedTests.cpp의 전체 체인 관측과 현재 물리 재조회 검사를 옮긴다.</summary>
public sealed class RawFormLinkedTests
{
    /// <summary>원본 관측에 전달한 열두 체인 입력이다. 기대 결과는 fixture에서만 읽는다.</summary>
    private static readonly ushort[][] Chains = [[], [20], [50], [20, 50], [50, 20], [20, 30, 50, 63], [20, 64], [50, 64], [64], [65535], [20, 50, 65535], [63, 30, 20]];

    /// <summary>명시 입력의 지정 WORD만 쓰며 기존 파일이나 관측 기대값은 수정하지 않는다.</summary>
    private static void Put(Span<byte> bytes, int stride, int sid, int offset, ushort value) =>
        BinaryPrimitives.WriteUInt16LittleEndian(bytes[(sid * stride + offset)..], value);

    /// <summary>원본 고유 입력 120개에서 전체 체인 검사 결과와 읽기 전용 바이트를 대조한다.</summary>
    [Fact]
    public void X86Native120WholeChainAndReadOnlyBytes()
    {
        int count = 0;
        // fixture의 각 관측에 동일한 체인 입력을 공급한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "form-linked-parent-x86.tsv")))
        {
            if (line.StartsWith('#') || string.IsNullOrWhiteSpace(line)) continue;
            string[] row = line.Split('\t');
            int[] input = row[0].Split(',').Select(int.Parse).ToArray();
            ushort[] chain = Chains[input[2]];
            byte[] bytes = new byte[64 * 50];
            Put(bytes, 50, input[1], 6, chain.Length == 0 ? (ushort)0 : chain[0]);
            // 논리 범위 밖 번호는 head와 next의 입력값으로만 사용한다.
            for (int index = 0; index < chain.Length; index++)
                if (chain[index] < 64) Put(bytes, 50, chain[index], 4, index + 1 < chain.Length ? chain[index + 1] : (ushort)0);
            byte[] before = (byte[])bytes.Clone();
            Assert.Equal(row[1] == "1", RawFormLinked.CheckChain(bytes, 50, 64, input[0], input[1]));
            Assert.Equal($"P:{input[0]}:{input[1]}", row[2]);
            Assert.Equal("1", row[3]);
            Assert.Equal(before, bytes);
            count++;
        }
        Assert.Equal(120, count);
    }

    /// <summary>getter 중 풀 재확보 뒤의 물리를 읽고 자기 뒤의 순환·범위 실패를 구분한다.</summary>
    [Fact]
    public void CurrentPoolAfterParentAndExplicitCycle()
    {
        var pool = new RawSidPool(32768);
        int calls = 0;
        Assert.True(RawFormLinked.Check(pool, 50, form =>
        {
            Assert.Equal(50, form); calls++;
            pool.Reset(32770);
            BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(1)[6..], 50);
            BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(50)[4..], 0);
            return 1;
        }));
        Assert.Equal(1, calls);
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(50)[4..], 50);
        Assert.Throws<InvalidOperationException>(() => RawFormLinked.Check(pool, 50, _ => 1));
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(50)[4..], 32770);
        Assert.False(RawFormLinked.Check(pool, 50, _ => 1));
        Assert.Throws<InvalidOperationException>(() => RawFormLinked.Check(pool, 50, null));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFormLinked.CheckChain(new byte[64 * 50], 50, 65, 50, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFormLinked.CheckChain(new byte[64 * 50], 7, 64, 50, 1));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawFormLinked.CheckChain(new byte[64 * 50], 50, 64, 50, 64));
    }

    /// <summary>양판본 ProcessForm의 실제 getter는 raw 상태와 TYPE를 무시하고 부모 WORD 전체만 읽는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0x512098u)]
    [InlineData(OriginalEdition.Cd1072, 0x505990u)]
    public void ProcessGetterKeepsWordAndIgnoresStateType(OriginalEdition edition, uint table)
    {
        var pool = new RawSidPool(32768, edition: edition);
        BinaryPrimitives.WriteUInt32LittleEndian(pool.RawSlot(50), table);
        pool.RawSlot(50)[10] = 0; pool.RawSlot(50)[11] = 255;
        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(50)[16..], 0xabcd);
        Assert.Equal(0xabcd, RawFormLinked.ReadProcessFormParent(pool, 50));
        pool.RawSlot(50)[0] = 0;
        Assert.Throws<InvalidOperationException>(() => RawFormLinked.ReadProcessFormParent(pool, 50));
    }
}
