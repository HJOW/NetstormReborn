using Netstorm.Assets;
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Simulation;

/// <summary>저장된 가이저가 없는 튜토리얼 1에 접근 가능한 연습용 가이저 받침을 만든다.</summary>
internal static class TutorialGeysers
{
    /// <summary>섬과 가이저 받침 사이의 빈 칸 수. 원본 생성 위치를 아직 복원하지 못한 결정적 근사.</summary>
    private const int Gap = 4;

    /// <summary>가이저 3×3 발자국 둘레에 연결 가능한 지면을 한 칸씩 남긴 받침 한 변 칸 수.</summary>
    private const int PadSize = 5;

    /// <summary>가이저의 원본 .type 발자국 한 변 칸 수.</summary>
    private const int GeyserSize = 3;

    /// <summary>동쪽 해안의 다리 부착 가능한 칸을 골라 가이저와 받침을 만든다.</summary>
    public static (FortMapObject? Geyser, IReadOnlyList<(int X, int Y)> Pad) Create(
        IReadOnlyList<FortMapObject> objects, IReadOnlyCollection<FortTerrainCell> cells,
        IReadOnlySet<(int X, int Y)> edgeFarms, TypeCatalog types)
    {
        FortMapObject? priest = objects.FirstOrDefault(o => o.Object.Owner == 1 && o.Object.Type.Name.Equals("priest", StringComparison.OrdinalIgnoreCase));
        TypeInfo? geyserType = types.Find("geyser");
        if (priest == null || geyserType == null)
        {
            return (null, []);
        }
        HashSet<(int X, int Y)> island = cells.Select(cell => (cell.X, cell.Y)).ToHashSet();
        int? region = priest.Territory;
        FortTerrainCell? coast = cells.Where(cell => cell.Region == region && cell.X + Gap + PadSize < BridgeGrid.WorldSize
                && !island.Contains((cell.X + 1, cell.Y)) && !edgeFarms.Contains((cell.X, cell.Y)))
            .OrderBy(cell => Math.Abs(cell.Y - priest.Y)).ThenByDescending(cell => cell.X).FirstOrDefault();
        if (coast == null)
        {
            return (null, []);
        }
        int left = coast.X + Gap + 1;
        int top = Math.Clamp(coast.Y - 2, 0, BridgeGrid.WorldSize - PadSize);
        int anchorX = left + GeyserSize;
        int anchorY = top + GeyserSize;
        var fortObject = new FortObject(0, 0, geyserType, null, null, null, null, null, 0, []);
        var geyser = new FortMapObject(anchorX, anchorY, null, fortObject);
        var pad = new List<(int X, int Y)>();
        // 가이저 3×3 발자국 둘레에 한 칸을 남겨 다리가 붙고 사제가 걸을 수 있는 받침을 만든다.
        for (int y = top; y < top + PadSize; y++)
        {
            // 받침 한 행의 다섯 칸을 기록한다.
            for (int x = left; x < left + PadSize; x++)
            {
                pad.Add((x, y));
            }
        }
        return (geyser, pad);
    }
}
