namespace Netstorm.Assets;

/// <summary>저장된 noIsland 3×3 묶음으로 확인한 작은 받침. 기준점은 오른쪽 아래 칸이다.</summary>
public sealed record FortIslandSupport(int X, int Y, int Owner);

/// <summary>원본 작은 받침의 논리 칸 묶음을 찾아 island·islandStalag 표시용으로 복원한다.</summary>
public static class FortIslandSupports
{
    /// <summary>원본 작은 받침 한 변의 논리 칸 수.</summary>
    public const int Size = 3;
    /// <summary>소유자색 테두리가 없는 원본 P09 클러스터 번호.</summary>
    public const int NeutralCluster = 8;

    /// <summary>완전하고 소유자가 같은 3×3 저장 칸만 받침으로 인정한다. 불완전한 칸은 미리보기에 남긴다.</summary>
    public static IReadOnlyList<FortIslandSupport> Find(IReadOnlyList<FortMapObject> objects)
    {
        var remaining = objects.Where(item => item.Object.Type.Name.Equals("noIsland", StringComparison.OrdinalIgnoreCase))
            .GroupBy(item => (item.X, item.Y)).ToDictionary(group => group.Key, group => group.First().Object.Owner ?? 0);
        var result = new List<FortIslandSupport>();
        // 저장된 건물·가이저 기준점을 먼저 확인하여 서로 붙어 있는 받침의 경계를 유지한다.
        foreach (FortMapObject item in objects.Where(item => item.Object.Type.Definition.HasFlag("createsisland")))
        {
            Take(item.X, item.Y);
        }
        // 건물이 없는 완전한 묶음도 y·x 순서로 찾아 표시한다.
        foreach (var (x, y) in remaining.Keys.OrderBy(cell => cell.Y).ThenBy(cell => cell.X).ToArray())
        {
            Take(x + Size - 1, y + Size - 1);
        }
        return result.OrderBy(item => item.Y).ThenBy(item => item.X).ToArray();

        /// <summary>기준점 왼쪽 위의 9칸이 모두 같은 소유자일 때 한 번만 소비한다.</summary>
        void Take(int x, int y)
        {
            if (!remaining.TryGetValue((x, y), out int owner))
            {
                return;
            }
            var cells = new List<(int X, int Y)>();
            // 기준점까지 세 행을 검사한다.
            for (int py = y - Size + 1; py <= y; py++)
            {
                // 하나라도 빠졌거나 소유자가 다르면 저장 칸을 소비하지 않는다.
                for (int px = x - Size + 1; px <= x; px++)
                {
                    if (!remaining.TryGetValue((px, py), out int other) || other != owner)
                    {
                        return;
                    }
                    cells.Add((px, py));
                }
            }
            // 완전한 묶음의 칸을 제거하여 중복 받침 생성을 막는다.
            foreach (var cell in cells)
            {
                remaining.Remove(cell);
            }
            result.Add(new FortIslandSupport(x, y, owner));
        }
    }

    /// <summary>004421a0처럼 플레이어의 1~8 색상 번호를 P01~P08에 대응하고 미지정·중립은 P09를 쓴다.</summary>
    public static int ColorCluster(int owner, IReadOnlyDictionary<int, int> playerColors) =>
        owner > 0 && playerColors.TryGetValue(owner, out int color) && color is >= 1 and <= 8
            ? color - 1 : NeutralCluster;
}
