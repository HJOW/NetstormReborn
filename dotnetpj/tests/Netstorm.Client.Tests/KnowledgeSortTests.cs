// Factory와 전체 지식 창이 공유하는 원본70바이트 행의 의미 필드와 CRT 정렬이다.
// cpppj/src/client/KnowledgeSort.cpp 를 이식한다.
// 0.917에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

/// <summary>같은 실제 TYPE 표와 지식 행 비교·정렬의 계약이다.</summary>
public sealed class KnowledgeSortTests
{
    /// <summary>같은 실제 TYPE 표를 준비한다.</summary>
    private static List<OriginalTypeRecord> Types(params (int Theme, uint Flags2)[] rows)
    {
        List<OriginalTypeRecord> types = [];
        foreach ((int theme, uint flags2) in rows)
        {
            types.Add(new OriginalTypeRecord { Theme = theme, Flags2 = flags2 });
        }

        return types;
    }

    /// <summary>같은 지식 행을 준비한다.</summary>
    private static KnowledgeRecord Row(uint type, string description) => new() { Type = type, Description = description };

    /// <summary>우선 플래그가 있는 행이 먼저 온다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void PriorityFlagSortsFirst()
    {
        var types = Types((0, 0), (0, 0x800));
        Assert.True(KnowledgeSort.CompareKnowledgeRecords(Row(0, "b"), Row(1, "a"), types) > 0);
        Assert.True(KnowledgeSort.CompareKnowledgeRecords(Row(1, "b"), Row(0, "a"), types) < 0);
        Assert.Equal(0, KnowledgeSort.CompareKnowledgeRecords(Row(1, "a"), Row(1, "a"), types));
    }

    /// <summary>같은 분류는 대소문자 없이 ASCII로 비교한다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void SamePriorityComparesAsciiCaseInsensitive()
    {
        var types = Types((0, 0), (0, 0), (0, 0));
        Assert.True(KnowledgeSort.CompareKnowledgeRecords(Row(0, "Apple"), Row(1, "banana"), types) < 0);
        Assert.True(KnowledgeSort.CompareKnowledgeRecords(Row(0, "BANANA"), Row(1, "apple"), types) > 0);
        Assert.Equal(0, KnowledgeSort.CompareKnowledgeRecords(Row(0, "Sun"), Row(2, "sun"), types));
        Assert.True(KnowledgeSort.CompareKnowledgeRecords(Row(0, "a"), Row(1, "aa"), types) < 0);
    }

    /// <summary>표 밖 TYPE은 명시 거부한다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void OutOfRangeTypeThrows()
    {
        var types = Types((0, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => KnowledgeSort.CompareKnowledgeRecords(Row(0, "a"), Row(5, "b"), types));
        Assert.Throws<ArgumentOutOfRangeException>(() => KnowledgeSort.CompareKnowledgeRecords(Row(9, "a"), Row(0, "b"), types));
        Assert.Throws<ArgumentNullException>(() => KnowledgeSort.CompareKnowledgeRecords(null!, Row(0, "b"), types));
    }

    /// <summary>빈·단일 목록은 그대로 둔다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void EmptyAndSingleStayPut()
    {
        var types = Types((0, 0));
        List<KnowledgeRecord> empty = [];
        KnowledgeSort.SortKnowledgeRecords(empty, types);
        Assert.Empty(empty);
        List<KnowledgeRecord> single = [Row(0, "only")];
        KnowledgeSort.SortKnowledgeRecords(single, types);
        Assert.Equal("only", single[0].Description);
    }

    /// <summary>큰 목록은 우선순위·이름 순으로 정렬한다. 0.917에서 추가됐다.</summary>
    [Fact]
    public void LargeListSortsByPriorityThenName()
    {
        var types = Types((0, 0), (0, 0x800), (1, 0), (2, 0x8000000));
        List<KnowledgeRecord> rows =
        [
            Row(2, "zebra"), Row(0, "banana"), Row(1, "apple"), Row(3, "mango"),
            Row(0, "apple"), Row(1, "zebra"), Row(2, "apple"), Row(3, "apple"),
            Row(0, "cherry"), Row(2, "mango"),
        ];
        KnowledgeSort.SortKnowledgeRecords(rows, types);
        for (int i = 1; i < rows.Count; i++)
        {
            Assert.True(KnowledgeSort.CompareKnowledgeRecords(rows[i - 1], rows[i], types) <= 0);
        }

        Assert.Equal(1u, rows[0].Type);
    }
}
