// Factory와 전체 지식 창이 공유하는 원본70바이트 행의 의미 필드와 CRT 정렬이다.
// cpppj/src/client/KnowledgeSort.cpp 를 이식한다.
// 0.917에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump;

// 원본 행의 번호/사용 가능/생산 중/마나/번역 설명이며 포인터나 원문 메모리 배치는 호스트에 노출하지 않는다.
public sealed class KnowledgeRecord
{
    /// <summary>+0의 실제 타입 번호다.</summary>
    public uint Type { get; set; }

    /// <summary>+8의 원본 비제로 판정이다.</summary>
    public bool Available { get; set; }

    /// <summary>+c의 원본 비제로 판정이다.</summary>
    public bool InProduction { get; set; }

    /// <summary>+14부터 첫NUL까지의 원본 글꼴 바이트다.</summary>
    public string Mana { get; set; } = "";

    /// <summary>+1e부터 첫NUL까지의 원본 글꼴 바이트다.</summary>
    public string Description { get; set; } = "";
}

// 00453f70의 첫 공통 우선 flags2 분류와 004e6010/shortsort의 피벗·동률 교환을 공유한다.
public static class KnowledgeSort
{
    /// <summary>453f70의 첫 공통 플래그에서 멈추는 순서다. 0.917에서 추가됐다.</summary>
    private static readonly uint[] Priority = [0x800, 0x8000000, 0x10000, 0x4000000, 0x20000000, 0x40000, 0x20000];

    /// <summary>원본 CRT가 shortsort를 선택하는 포함 구간 길이다. 0.917에서 추가됐다.</summary>
    private const int SmallRange = 8;

    /// <summary>원본 CRT 구간 스택의 최대 쌍 개수다. 0.917에서 추가됐다.</summary>
    private const int RangeStackSize = 30;

    /// <summary>현재 포함 구간의 첫/마지막 인덱스다.</summary>
    private readonly struct Range
    {
        /// <summary>포함 구간의 첫 인덱스다.</summary>
        public readonly int Low;

        /// <summary>포함 구간의 마지막 인덱스다.</summary>
        public readonly int High;

        /// <summary>포함 구간의 첫/마지막 인덱스를 보관한다.</summary>
        public Range(int low, int high)
        {
            Low = low;
            High = high;
        }
    }

    /// <summary>첫 공통 우선 플래그에서 분류를 끝내고 같은 분류는 원본 C 로캘 이름으로 비교한다. 0.917에서 추가됐다.</summary>
    /// <param name="first">앞 지식 행이다.</param>
    /// <param name="second">뒤 지식 행이다.</param>
    /// <param name="types">같은 현재 TYPE 표다.</param>
    public static int CompareKnowledgeRecords(KnowledgeRecord first, KnowledgeRecord second, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(first);
        ArgumentNullException.ThrowIfNull(second);
        ArgumentNullException.ThrowIfNull(types);
        if (first.Type >= (uint)types.Count || second.Type >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(types), "지식 정렬 TYPE 범위");
        }

        uint a = types[(int)first.Type].Flags2;
        uint b = types[(int)second.Type].Flags2;
        // 한쪽에만 우선 플래그가 있으면 즉시 반환한다. 둘 다 있으면 뒤 플래그를 보지 않는다.
        foreach (uint bit in Priority)
        {
            if ((a & bit) != 0)
            {
                if ((b & bit) == 0)
                {
                    return -1;
                }

                break;
            }

            if ((b & bit) != 0)
            {
                return 1;
            }
        }

        // 첫NUL까지의 unsigned ASCII 차이를 반환한다. 대문자만 소문자로 접는다.
        for (int index = 0; ; index++)
        {
            uint x = index < first.Description.Length ? (uint)first.Description[index] : 0u;
            uint y = index < second.Description.Length ? (uint)second.Description[index] : 0u;
            if (x >= 'A' && x <= 'Z')
            {
                x += 32;
            }

            if (y >= 'A' && y <= 'Z')
            {
                y += 32;
            }

            if (x != y)
            {
                return (int)x - (int)y;
            }

            if (x == 0)
            {
                return 0;
            }
        }
    }

    /// <summary>원본 CRT의 작은 구간 선택/중앙 피벗/동률 제거/큰 구간 보관을 같은 순서로 실행한다. 0.917에서 추가됐다.</summary>
    /// <param name="entries">같은 지식 행 목록이다.</param>
    /// <param name="types">같은 현재 TYPE 표다.</param>
    public static void SortKnowledgeRecords(List<KnowledgeRecord> entries, IReadOnlyList<OriginalTypeRecord> types)
    {
        ArgumentNullException.ThrowIfNull(entries);
        ArgumentNullException.ThrowIfNull(types);
        Range[] stack = new Range[RangeStackSize];
        int used = 0;
        Range range = new(0, entries.Count - 1);
        // 주소 대신 현재 인덱스의 원소를 비교한다.
        int Compare(int a, int b) => CompareKnowledgeRecords(entries[a], entries[b], types);
        // 원본 byte swap과 같은 항목 전체 교환이다.
        void Swap(int a, int b)
        {
            if (a != b)
            {
                (entries[a], entries[b]) = (entries[b], entries[a]);
            }
        }

        // 큰 구간을 고정 스택에 보관하며 다음에 작은 구간을 처리한다.
        void Push(Range next)
        {
            if (used == stack.Length)
            {
                throw new InvalidOperationException("Factory 지식 정렬 스택 넘침");
            }

            stack[used++] = next;
        }

        // 현재 구간과 보관했던 구간이 모두 끝날 때까지 원본 분기 순서를 따른다.
        for (; ; )
        {
            if (range.High - range.Low + 1 <= SmallRange)
            {
                // small sort는 최대 항목을 마지막으로 보내는 선택 정렬이다.
                while (range.Low < range.High)
                {
                    int maximum = range.Low;
                    // 엄격히 큰 항목만 최대 위치로 바꾼다.
                    for (int current = range.Low + 1; current <= range.High; current++)
                    {
                        if (Compare(current, maximum) > 0)
                        {
                            maximum = current;
                        }
                    }

                    Swap(maximum, range.High);
                    range = new Range(range.Low, range.High - 1);
                }
            }
            else
            {
                int middle = range.Low + (range.High - range.Low + 1) / 2;
                if (Compare(range.Low, middle) > 0)
                {
                    Swap(range.Low, middle);
                }

                if (Compare(range.Low, range.High) > 0)
                {
                    Swap(range.Low, range.High);
                }

                if (Compare(middle, range.High) > 0)
                {
                    Swap(middle, range.High);
                }

                int left = range.Low;
                int right = range.High;
                // 실제 CRT처럼 피벗의 자기 비교를 건너뛰고 교차할 때까지 분할한다.
                for (; ; )
                {
                    if (left < middle)
                    {
                        // 피벗 앞의 작은/동률 항목을 지난다.
                        do
                        {
                            ++left;
                        }
                        while (left < middle && Compare(left, middle) <= 0);
                    }

                    if (left >= middle)
                    {
                        // 피벗 뒤의 작은/동률 항목을 끝까지 검사한다.
                        do
                        {
                            ++left;
                        }
                        while (left <= range.High && Compare(left, middle) <= 0);
                    }

                    // 오른쪽은 엄격히 큰 항목만 지난다.
                    do
                    {
                        --right;
                    }
                    while (right > middle && Compare(right, middle) > 0);

                    if (left > right)
                    {
                        break;
                    }

                    Swap(left, right);
                    if (middle == right)
                    {
                        middle = left;
                    }
                }

                ++right;
                if (right > middle)
                {
                    // 피벗 오른쪽의 실제0 비교 항목도 제거한다.
                    do
                    {
                        --right;
                    }
                    while (right > middle && Compare(right, middle) == 0);
                }

                if (right <= middle)
                {
                    // 피벗을 건너뛰고 왼쪽 끝까지 동률 구간을 제거한다.
                    do
                    {
                        --right;
                    }
                    while (right > range.Low && Compare(right, middle) == 0);
                }

                if (right - range.Low < range.High - left)
                {
                    if (left < range.High)
                    {
                        Push(new Range(left, range.High));
                    }

                    range = new Range(range.Low, right);
                    if (range.Low < range.High)
                    {
                        continue;
                    }
                }
                else
                {
                    if (range.Low < right)
                    {
                        Push(new Range(range.Low, right));
                    }

                    range = new Range(left, range.High);
                    if (range.Low < range.High)
                    {
                        continue;
                    }
                }
            }

            if (used == 0)
            {
                return;
            }

            range = stack[--used];
        }
    }
}
