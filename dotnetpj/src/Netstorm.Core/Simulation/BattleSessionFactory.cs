using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Simulation;

/// <summary>
/// 저장된 맵(.fort)과 미션 시작 조건에서 <see cref="BattleSession"/> 을 조립한다.
/// 섬 칸·다리 시작 불가 칸 같은 지형 정보는 지면 미리보기(FortTerrainPreview 등)가 만든 값을 받는다
/// (화면 표시와 규칙 판정이 같은 지형을 보도록 한 번만 계산한다).
/// </summary>
public static class BattleSessionFactory
{
    /// <summary>bridge 타입 이름 (저장된 다리 칸의 프레임 표를 얻는 데 쓴다)</summary>
    private const string BridgeTypeName = "bridge";

    /// <summary>튜토리얼 2(Secret Workshop) 번호 — 원본이 시작할 때 전투 옵션을 덮어쓴다 (BattleOptions.ApplyTutorialTwoOverrides)</summary>
    private const int TutorialTwo = 2;

    /// <summary>원본 실행 시 동적으로 가이저를 만드는 튜토리얼 1 번호.</summary>
    private const int TutorialOne = 1;

    /// <summary>
    /// 세션을 만든다.
    /// </summary>
    /// <param name="map">월드 좌표가 정해진 맵</param>
    /// <param name="islandCells">지면 미리보기의 섬 칸 (영역 번호 0 이상은 본섬, 음수는 작은 받침 등)</param>
    /// <param name="edgeFarmCells">가장자리 초목(edgeFarm) 칸 — 다리를 시작할 수 없는 칸</param>
    /// <param name="types">타입 목록</param>
    /// <param name="mission">미션 시작 조건 (없으면 null)</param>
    /// <param name="humanPlayer">사람 플레이어 번호</param>
    /// <param name="startStormPower">미션 값이 없을 때 쓸 시작 Storm Power (없으면 전투 옵션의 시작 금액)</param>
    /// <param name="seed">전역 난수 시드</param>
    public static BattleSession Create(FortMap map, IEnumerable<FortTerrainCell> islandCells, IEnumerable<(int X, int Y)> edgeFarmCells,
        TypeCatalog types, MissionStart? mission = null, int humanPlayer = 1, int? startStormPower = null, uint seed = 0)
    {
        FortTerrainCell[] cells = [.. islandCells];
        var options = new BattleOptions();
        // 현재 클론의 전투는 모두 싱글 플레이라 원본 전투 초기화의 처치 보상 25%를 쓴다 (멀티플레이 구현 때 분리)
        options.ApplySinglePlayerKillReward();
        if (mission?.TutorialNumber == TutorialTwo)
        {
            options.ApplyTutorialTwoOverrides();
        }
        // 본섬 칸(영역 번호 0 이상)만 섬 영역으로 본다. 작은 받침·발판은 영역 밖으로 둔다.
        var territories = new Dictionary<(int X, int Y), int>();
        // 본섬 칸마다 칸 → 영역 번호를 기록한다
        foreach (FortTerrainCell cell in cells.Where(c => c.Region >= 0))
        {
            territories[(cell.X, cell.Y)] = cell.Region;
        }
        var edgeSet = edgeFarmCells.ToHashSet();
        var objects = map.Objects.ToList();
        IReadOnlyList<(int X, int Y)> geyserPad = [];
        if (mission?.TutorialNumber == TutorialOne && !objects.Any(item => ObjectKinds.Of(item.Object.Type) == ObjectKind.Geyser))
        {
            (FortMapObject? geyser, geyserPad) = TutorialGeysers.Create(objects, cells, edgeSet, types);
            if (geyser != null)
            {
                objects.Add(geyser);
            }
        }
        // 미션 동맹 목록(aiNAllyList)이 있으면 공급·전투·포획 판정에 함께 쓴다
        var battle = new BattleMap(objects, (x, y) => territories.TryGetValue((x, y), out int t) ? t : null, options,
            mission != null ? (first, second) => mission.AreAllied(first, second, humanPlayer) : null);

        // 다리 격자의 섬 칸 = 본섬 미리보기 칸 + 작은 받침(noIsland) 칸
        var island = cells.Select(c => (c.X, c.Y)).ToHashSet();
        FortMapObject[] supportObjects = objects.Where(o => o.Object.Type.Definition.HasFlag("createsisland")).ToArray();
        // 저장된 noIsland 중 건물 발자국 아래의 칸은 그 건물의 동적 받침이다. 본섬·독립 받침은 영구 지면으로 남긴다.
        island.UnionWith(objects.Where(o => o.Object.Type.Name == "noIsland" &&
            !supportObjects.Any(s => Footprint.ForType(s.Object.Type.Definition, s.X, s.Y).Contains(o.X, o.Y)))
            .Select(o => (o.X, o.Y)));
        island.UnionWith(geyserPad);
        // 다리·지면 외 오브젝트의 발자국 칸은 다리가 겹칠 수 없다
        var occupied = new HashSet<(int X, int Y)>();
        // 오브젝트마다 발자국 칸을 모은다
        foreach (FortMapObject item in objects.Where(o => o.Object.Type.Name != "noIsland" &&
            !o.Object.Type.Definition.HasFlag("balloon") && ObjectKinds.Of(o.Object.Type) is not (ObjectKind.Bridge or ObjectKind.Flyer)))
        {
            occupied.UnionWith(Footprint.ForType(item.Object.Type.Definition, item.X, item.Y).Cells());
        }
        // 가장자리 초목(edgeFarm)·dropBlocking 오브젝트 칸에서는 다리를 시작할 수 없다 (사용자 확인 규칙)
        HashSet<(int X, int Y)> dropBlocking = BridgeAnchors.DropBlockingCells(objects, edgeSet);
        // 다리 프레임 표: 저장된 다리 칸의 해석과 금 간 프레임이 있는지의 판정에 쓴다
        TypeFrameTable frames = (types.Find(BridgeTypeName) ?? throw new InvalidDataException("bridge 타입이 없습니다.")).Definition.Frames;
        BattleSession? session = null;
        // 초기화 뒤에는 살아 있는 엔티티를 조회해 파괴·회수된 자리와 새로 놓은 유닛의 점유를 반영한다.
        var grid = new BridgeGrid((x, y) => island.Contains((x, y)) || (session == null
                ? supportObjects.Any(s => Footprint.ForType(s.Object.Type.Definition, s.X, s.Y).Contains(x, y))
                : session.Entities.Any(e => e.IsComplete && e.Type.Definition.HasFlag("createsisland") && e.Footprint.Contains(x, y))),
            (x, y) => session == null ? occupied.Contains((x, y)) : session.Entities.Any(e => e.OccupiesGround && e.Footprint.Contains(x, y)),
            (x, y, _, _) => session == null ? !dropBlocking.Contains((x, y)) :
                !edgeSet.Contains((x, y)) && !session.Entities.Any(e => e.OccupiesGround && BridgeAnchors.IsDropBlocking(e.Type) && e.Footprint.Contains(x, y)),
            frames: frames);
        // 저장된 다리 칸을 격자에 넣는다 (연결·붕괴 계산에 쓴다). 화면이 무너진 저장 다리를 숨길 수 있도록 오브젝트와 칸을 짝지어 둔다.
        var stored = new Dictionary<FortMapObject, BridgeCellState>();
        // 저장 다리 오브젝트마다 격자 칸을 만들어 짝지어 둔다
        foreach (FortMapObject item in objects.Where(o => ObjectKinds.Of(o.Object.Type) == ObjectKind.Bridge && o.Object.BridgeShape is not null))
        {
            stored[item] = grid.AddStored(frames, item.Object.BridgeShape!.Value, item.X, item.Y, item.Object.Owner ?? 0);
        }
        session = new BattleSession(battle, grid, types, mission, humanPlayer, startStormPower, seed, stored);
        return session;
    }
}
