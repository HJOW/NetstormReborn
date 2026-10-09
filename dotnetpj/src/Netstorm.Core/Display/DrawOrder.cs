namespace Netstorm.Core.Display;

/// <summary>오브젝트 하나의 그리기 순서 키</summary>
/// <param name="X">월드 x 좌표</param>
/// <param name="Y">월드 y 좌표</param>
/// <param name="Depth">깊이 (타입의 zorder, 부호 있는 16비트). 큰 값이 먼저(뒤쪽에) 그려진다</param>
public readonly record struct DrawOrder(float X, float Y, short Depth)
{
    /// <summary>
    /// 원본 정렬 비교 (10.78 의 00497900): 깊이 내림차순 → y 오름차순 → x 오름차순.
    /// 음수면 a 가 먼저 그려진다. 완전히 같은 키에는 0 이 아니라 1 을 돌려준다 (원본 그대로).
    /// 검증은 원본 기계어 기대값 renderer-x86.tsv 의 Order 행으로 한다 (docs/exe/cpp-renderer-reconstruction.md).
    /// </summary>
    /// <param name="a">첫 키</param>
    /// <param name="b">둘째 키</param>
    public static int Compare(DrawOrder a, DrawOrder b)
    {
        int depth = b.Depth - a.Depth;
        if (depth != 0)
        {
            return depth;
        }
        float delta = a.Y - b.Y;
        return (delta == 0 ? a.X - b.X : delta) < 0 ? -1 : 1;
    }

    /// <summary>
    /// 그리기 순서대로 늘어놓는다. 완전히 같은 키의 순서는 원본 qsort 에 달려 있어 확정되지 않았으므로
    /// cpppj 처럼 입력 순서를 유지한다 (안정 정렬).
    /// </summary>
    /// <param name="items">정렬할 항목</param>
    /// <param name="key">항목의 그리기 순서 키 (좌표는 유한한 값이어야 한다)</param>
    public static IReadOnlyList<T> Sort<T>(IEnumerable<T> items, Func<T, DrawOrder> key)
    {
        (T Item, DrawOrder Key)[] keyed = items.Select(item => (item, key(item))).ToArray();
        // 유한하지 않은 좌표는 비교가 순서를 이루지 못한다
        foreach ((T _, DrawOrder order) in keyed)
        {
            if (!float.IsFinite(order.X) || !float.IsFinite(order.Y))
            {
                throw new ArgumentException("그리기 순서의 좌표가 유한한 값이 아닙니다", nameof(items));
            }
        }
        // "a 가 b 보다 앞이다"를 원본 비교의 음수로 정의하고, 어느 쪽도 앞이 아니면 같은 것으로 본다
        return keyed.Order(Comparer<(T Item, DrawOrder Key)>.Create((a, b) =>
            Compare(a.Key, b.Key) < 0 ? -1 : Compare(b.Key, a.Key) < 0 ? 1 : 0)).Select(pair => pair.Item).ToArray();
    }
}
