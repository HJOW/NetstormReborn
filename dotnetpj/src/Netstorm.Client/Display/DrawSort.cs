using Netstorm.Core.Original;

namespace Netstorm.Client.Display; // 원본 표시 수집 항목을 CRT 교환 순서대로 정렬한다.

/// <summary>cpppj/src/client/DrawSort.cpp와 Renderer.cpp의 10.78 및 CD 표시 정렬이다.</summary>
public static class DrawSort
{
    /// <summary>작은 구간은 원본 선택 정렬을 쓴다.</summary>
    private const int Small = 8;
    /// <summary>원본 CRT가 보관하는 구간의 최대 수.</summary>
    private const int StackCapacity = 30;
    /// <summary>원본 signed 바이트 거리로 표현할 수 있는 항목 수.</summary>
    private const int MaxEntries = int.MaxValue / 16;
    /// <summary>양 끝을 포함하는 미처리 구간.</summary>
    private readonly record struct Range(int Low, int High);

    /// <summary>깊이는 큰 값부터, 같은 깊이는 Y·X 오름차순으로 비교한다. 완전 동률도 원본처럼 1이다.</summary>
    public static int Compare(in RenderCollectionEntry a, in RenderCollectionEntry b)
    {
        int depth = b.Depth - a.Depth; if (depth != 0) return depth;
        float delta = a.Y - b.Y; return (delta == 0 ? a.X - b.X : delta) < 0 ? -1 : 1;
    }

    /// <summary>항목 전체를 이동해 토큰과 종류·tail을 키와 함께 보존한다.</summary>
    private static void Swap(Span<RenderCollectionEntry> entries, int a, int b)
    { if (a != b) (entries[a], entries[b]) = (entries[b], entries[a]); }

    /// <summary>작은 구간에서 최대 항목을 뒤로 보내는 원본 선택 정렬.</summary>
    private static void SmallSort(Span<RenderCollectionEntry> entries, int low, int high)
    {
        // 각 마지막 위치를 확정하며 남은 구간을 줄인다.
        while (low < high)
        {
            int maximum = low;
            // 현재 최대보다 엄격히 큰 항목만 선택한다.
            for (int i = low + 1; i <= high; i++) if (Compare(entries[i], entries[maximum]) > 0) maximum = i;
            Swap(entries, maximum, high--);
        }
    }

    /// <summary>더 큰 구간을 원본의 고정 스택에 넣는다.</summary>
    private static void Push(Span<Range> stack, ref int used, int low, int high)
    { if (used == stack.Length) throw new InvalidOperationException("표시 정렬 구간 스택 초과"); stack[used++] = new(low, high); }

    /// <summary>10.78의 중앙값 피벗과 비교·교환 순서로 정렬한다.</summary>
    private static void Patch(Span<RenderCollectionEntry> entries)
    {
        Span<Range> stack = stackalloc Range[StackCapacity]; int used = 0, low = 0, high = entries.Length - 1;
        // 현재 구간과 저장한 구간을 작은 쪽부터 처리한다.
        for (;;)
        {
            if (high - low + 1 <= Small) SmallSort(entries, low, high);
            else
            {
                int middle = low + (high - low + 1) / 2;
                if (Compare(entries[low], entries[middle]) > 0) Swap(entries, low, middle);
                if (Compare(entries[low], entries[high]) > 0) Swap(entries, low, high);
                if (Compare(entries[middle], entries[high]) > 0) Swap(entries, middle, high);
                int left = low, right = high;
                // 피벗 자기 비교는 건너뛰며 양쪽 탐색 위치를 교차시킨다.
                for (;;)
                {
                    if (left < middle) { do { left++; } while (left < middle && Compare(entries[left], entries[middle]) <= 0); }
                    if (left >= middle) { do { left++; } while (left <= high && Compare(entries[left], entries[middle]) <= 0); }
                    do { right--; } while (right > middle && Compare(entries[right], entries[middle]) > 0);
                    if (left > right) break;
                    Swap(entries, left, right); if (middle == right) middle = left;
                }
                right++;
                if (right > middle) { do { right--; } while (right > middle && Compare(entries[right], entries[middle]) == 0); }
                if (right <= middle) { do { right--; } while (right > low && Compare(entries[right], entries[middle]) == 0); }
                if (right - low < high - left)
                {
                    if (left < high) Push(stack, ref used, left, high); high = right;
                    if (low < high) continue;
                }
                else
                {
                    if (low < right) Push(stack, ref used, low, right); low = left;
                    if (low < high) continue;
                }
            }
            if (used == 0) return; var next = stack[--used]; low = next.Low; high = next.High;
        }
    }

    /// <summary>CD의 첫 칸 피벗과 바이트 거리 비교 순서로 정렬한다.</summary>
    private static void Cd(Span<RenderCollectionEntry> entries)
    {
        Span<Range> stack = stackalloc Range[StackCapacity]; int used = 0, low = 0, high = entries.Length - 1;
        // 현재 구간과 저장한 구간을 원본 순서대로 처리한다.
        for (;;)
        {
            if (high - low + 1 <= Small) SmallSort(entries, low, high);
            else
            {
                Swap(entries, low + (high - low + 1) / 2, low); int left = low, right = high + 1;
                // 첫 칸 피벗의 양쪽에서 교환할 항목을 찾는다.
                for (;;)
                {
                    do { left++; } while (left <= high && Compare(entries[left], entries[low]) <= 0);
                    do { right--; } while (right > low && Compare(entries[right], entries[low]) >= 0);
                    if (left > right) break; Swap(entries, left, right);
                }
                Swap(entries, low, right);
                if ((long)(right - low) * 16 - 1 < (long)(high - left) * 16)
                {
                    if (left < high) Push(stack, ref used, left, high);
                    if (low + 1 < right) { high = right - 1; continue; }
                }
                else
                {
                    if (low + 1 < right) Push(stack, ref used, low, right - 1); low = left;
                    if (low < high) continue;
                }
            }
            if (used == 0) return; var next = stack[--used]; low = next.Low; high = next.High;
        }
    }

    /// <summary>좌표를 먼저 전수 검사한 뒤 판본에 맞는 원본 CRT 순서로 항목을 제자리 정렬한다.</summary>
    public static void Sort(Span<RenderCollectionEntry> entries, OriginalEdition edition = OriginalEdition.Patch1078)
    {
        if (entries.Length > MaxEntries) throw new ArgumentOutOfRangeException(nameof(entries));
        // 실패할 좌표가 있으면 어떤 항목도 교환하지 않는다.
        foreach (var entry in entries) if (!float.IsFinite(entry.X) || !float.IsFinite(entry.Y)) throw new ArgumentException("표시 정렬 좌표 오류", nameof(entries));
        if (entries.Length < 2) return;
        if (edition == OriginalEdition.Patch1078) Patch(entries); else Cd(entries);
    }
}
