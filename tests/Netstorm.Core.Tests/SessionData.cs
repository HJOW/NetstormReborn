using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>게임 세션 테스트용 도우미: 원본 미션·맵에서 세션을 만들고, 조건에 맞는 칸을 찾는다.</summary>
internal static class SessionData
{
    /// <summary>미션 이름(예: tutorial2)으로 맵과 시작 조건을 읽어 세션을 만든다. 원본이 없으면 테스트를 건너뛴다.</summary>
    /// <param name="mission">미션 이름</param>
    public static BattleSession FromMission(string mission)
    {
        GameResources resources = OriginalData.RequireResources();
        MissionStart start = MissionStart.FromScript(resources.TryLoadMission(mission)!.Script);
        return FromMap(start.LoadFort!, start);
    }

    /// <summary>맵 이름으로 세션을 만든다 (지면 미리보기와 가장자리 초목 칸을 규칙 판정에 쓴다).</summary>
    /// <param name="fortName">맵 이름</param>
    /// <param name="mission">미션 시작 조건 (없으면 null)</param>
    public static BattleSession FromMap(string fortName, MissionStart? mission = null)
    {
        GameResources resources = OriginalData.RequireResources();
        TypeCatalog types = OriginalData.RequireTypes();
        var map = new FortMap(resources.LoadFort(fortName, types));
        TypeDefinition isle = types.Find("isle")!.Definition;
        var terrain = new FortTerrainPreview(map, isle);
        var edgeFarms = FortEdgeFarmPreview.Create(map, terrain, isle, types.Find("edgeFarm")!.Definition);
        return BattleSessionFactory.Create(map, terrain.IslandCells, edgeFarms.Select(t => (t.X, t.Y)), types, mission);
    }

    /// <summary>월드 칸을 위·왼쪽부터 훑어 조건을 처음 만족하는 칸을 찾는다. 없으면 예외.</summary>
    /// <param name="predicate">칸 조건</param>
    public static (int X, int Y) FindCell(Func<int, int, bool> predicate)
    {
        // 월드의 모든 칸을 y·x 순서로 검사한다
        for (int y = 0; y < BridgeGrid.WorldSize; y++)
        {
            // 한 행의 칸을 왼쪽부터 검사한다
            for (int x = 0; x < BridgeGrid.WorldSize; x++)
            {
                if (predicate(x, y))
                {
                    return (x, y);
                }
            }
        }
        throw new InvalidOperationException("조건에 맞는 칸이 없습니다.");
    }

    /// <summary>다리 조각 위치와 회전을 찾는다: 놓을 수 있는 첫 (회전, x, y).</summary>
    /// <param name="session">세션</param>
    /// <param name="pattern">모양 번호</param>
    public static (int Rotation, int X, int Y) FindBridgeSite(BattleSession session, int pattern)
    {
        // 회전 번호마다 월드를 훑어 처음 놓을 수 있는 곳을 찾는다
        for (int rotation = 0; rotation < BridgeDirections.RotationCount; rotation++)
        {
            // 회전마다 월드의 행을 위에서부터 훑는다
            for (int y = 0; y < BridgeGrid.WorldSize; y++)
            {
                // 한 행의 칸을 왼쪽부터 검사한다
                for (int x = 0; x < BridgeGrid.WorldSize; x++)
                {
                    if (session.CheckBridge(1, pattern, rotation, x, y).Allowed)
                    {
                        return (rotation, x, y);
                    }
                }
            }
        }
        throw new InvalidOperationException("다리 조각을 놓을 수 있는 곳이 없습니다.");
    }
}
