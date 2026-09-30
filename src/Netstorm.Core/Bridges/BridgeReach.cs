namespace Netstorm.Core.Bridges;

/// <summary>다리 연결망 하나가 닿는 섬 영역과 그 연결망에 칸을 가진 플레이어</summary>
/// <param name="Owners">연결망에 다리 칸이 있는 플레이어 번호</param>
/// <param name="Territories">연결망의 연결 방향 끝이 닿는 섬 영역 번호</param>
public sealed record BridgeNetworkReach(IReadOnlySet<int> Owners, IReadOnlySet<int> Territories);

/// <summary>
/// 다리 연결망이 어느 섬(영역)에 닿는지 계산한다. "빈 섬이 내 섬과 다리로 연결되었는가"의 근사 판정에 쓴다
/// (docs/gameplay/island-ownership.md 규칙 4: 건물형 유닛은 내 섬과 다리로 연결된 빈 섬에 지을 수 있다).
/// 근사: 내 다리 연결망 하나가 내 섬과 그 빈 섬에 함께 닿으면 연결된 것으로 본다. 다른 빈 섬을 거치는 연쇄 연결과
/// 원본의 영역 소유 판정(Player.cpp FUN_0048fdb0)은 확인하지 못했다.
/// </summary>
public static class BridgeReach
{
    /// <summary>네 방향 (북·동·남·서)</summary>
    private static readonly BridgeLinks[] Directions = [BridgeLinks.North, BridgeLinks.East, BridgeLinks.South, BridgeLinks.West];

    /// <summary>모든 연결망이 닿는 섬 영역을 계산한다.</summary>
    /// <param name="grid">다리 칸 격자</param>
    /// <param name="territoryAt">칸 → 섬 영역 번호 (섬 칸이 아니면 null)</param>
    public static IReadOnlyList<BridgeNetworkReach> Compute(BridgeGrid grid, Func<int, int, int?> territoryAt)
    {
        var result = new List<BridgeNetworkReach>();
        // 연결망마다 소유자와 닿는 섬 영역을 모은다 (연결망 순서는 좌표 순으로 일정하다)
        foreach (IReadOnlyList<BridgeCellState> network in grid.Networks())
        {
            var owners = new SortedSet<int>();
            var territories = new SortedSet<int>();
            // 칸마다 연결 방향의 이웃 칸이 섬이면 그 영역을 더한다
            foreach (BridgeCellState cell in network)
            {
                owners.Add(cell.Owner);
                // 네 방향 가운데 이 칸이 이어지는 방향만 본다
                foreach (BridgeLinks direction in Directions)
                {
                    if (!cell.Cell.Links.HasFlag(direction))
                    {
                        continue;
                    }
                    (int dx, int dy) = BridgeDirections.Offset(direction);
                    if (territoryAt(cell.X + dx, cell.Y + dy) is int territory)
                    {
                        territories.Add(territory);
                    }
                }
            }
            result.Add(new BridgeNetworkReach(owners, territories));
        }
        return result;
    }

    /// <summary>
    /// 플레이어가 빈 섬(영역)에 다리로 연결되어 있는지: 그 플레이어의 다리 연결망이 이 영역과
    /// 플레이어 소유 섬 영역에 함께 닿는다.
    /// </summary>
    /// <param name="reaches">Compute 결과</param>
    /// <param name="player">플레이어</param>
    /// <param name="territory">확인할 영역</param>
    /// <param name="ownerOf">영역 → 소유 플레이어 (빈 섬이면 null)</param>
    public static bool IsConnectedToHome(IEnumerable<BridgeNetworkReach> reaches, int player, int territory, Func<int, int?> ownerOf)
    {
        // 플레이어의 다리가 있고 이 영역에 닿는 연결망 가운데, 내 섬에도 닿는 것이 있는지 찾는다
        return reaches.Any(r => r.Owners.Contains(player) && r.Territories.Contains(territory)
            && r.Territories.Any(t => ownerOf(t) == player));
    }
}
