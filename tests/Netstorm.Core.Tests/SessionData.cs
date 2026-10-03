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

    /// <summary>
    /// 사제가 공사장에 도착해 건설이 시작되는 틱까지 한 틱씩 진행한다 (이미 시작됐으면 그대로). 시작된 바로 그 틱에서 멈춘다.
    /// 제한 시간 안에 시작되지 않으면 예외.
    /// </summary>
    /// <param name="session">세션</param>
    /// <param name="building">짓는 건물 (공사장)</param>
    /// <param name="maxSeconds">기다릴 최대 게임 시간(초)</param>
    public static void RunUntilStarted(BattleSession session, GameEntity building, int maxSeconds = 120)
    {
        // 사제가 도착할 때까지 한 틱씩 진행한다
        for (int tick = 0; tick < maxSeconds * session.TicksPerSecond && building.AwaitingBuilder; tick++)
        {
            session.RunTicks(1);
        }
        if (building.AwaitingBuilder)
        {
            throw new InvalidOperationException($"사제가 {maxSeconds}초 안에 {building.DisplayName} 공사장에 도착하지 않았습니다.");
        }
    }

    /// <summary>
    /// 건물이 완성되는 틱까지 한 틱씩 진행한다 (사제가 현장까지 걸어가는 시간 + 건설 시간). 완성된 바로 그 틱에서 멈추므로
    /// 완공 틱에 일어나는 일(튜토리얼 단계 전환 등)을 그대로 확인할 수 있다. 제한 시간 안에 끝나지 않으면 예외.
    /// </summary>
    /// <param name="session">세션</param>
    /// <param name="building">짓는 건물 (공사장)</param>
    /// <param name="maxSeconds">기다릴 최대 게임 시간(초)</param>
    public static void RunUntilComplete(BattleSession session, GameEntity building, int maxSeconds = 120)
    {
        // 완성될 때까지 한 틱씩 진행한다
        for (int tick = 0; tick < maxSeconds * session.TicksPerSecond && !building.IsComplete; tick++)
        {
            session.RunTicks(1);
        }
        if (!building.IsComplete)
        {
            throw new InvalidOperationException($"{building.DisplayName} 이(가) {maxSeconds}초 안에 완성되지 않았습니다 (사제 대기 중: {building.AwaitingBuilder}).");
        }
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
